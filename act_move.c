/* revision 1.2 - August 1 1999 - making it compilable under g++ */
/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

 /***************************************************************************
 *  ROM 2.4 is copyright 1993-1996 Russ Taylor                              *
 *  ROM has been brought to you by the ROM consortium                       *
 *      Russ Taylor (rtaylor@pacinfo.com)                                   *
 *      Gabrielle Taylor (gtaylor@pacinfo.com)                              *
 *      Brian Moore (rom@rom.efn.org)                                       *
 *  By using this code, you have agreed to follow the terms of the          *
 *  ROM license, in the file Rom24/doc/rom.license                          *
 ***************************************************************************/

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "merc.h"
#include "clan.h"
#include "utils.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_recall);
DECLARE_DO_FUN(do_stand);
DECLARE_DO_FUN(do_kill);
DECLARE_DO_FUN(do_say);

char *const
 dir_name[] =
{
	"north", "east", "south", "west", "up", "down"
};

const int
 rev_dir[] =
{
	2, 3, 0, 1, 5, 4
};

const int
 movement_loss[SECT_MAX] =
{
	1, 2, 2, 3, 4, 6, 8, 8, 0, 3, 4, 8, 30
};



/*
 * Local functions.
 */
int find_door(CHAR_DATA * ch, char *arg);
bool has_key(CHAR_DATA * ch, int key);
void trap_needle(CHAR_DATA *ch);
void trap_darts(CHAR_DATA *ch);
void trap_wire(CHAR_DATA *ch);
void fire_rune(CHAR_DATA *ch);
void shackle_rune(CHAR_DATA *ch);


void
move_char(CHAR_DATA * ch, int door, bool follow)
{
	CHAR_DATA *fch = NULL, *rch = NULL;
	CHAR_DATA *fch_next = NULL;
	CHAR_DATA *blocker = NULL;
	ROOM_INDEX_DATA *in_room = NULL;
	ROOM_INDEX_DATA *to_room = NULL;
	EXIT_DATA *pexit = NULL;
	AFFECT_DATA *paf, af;
	int over;
	int blocked = FALSE;

	if (door < 0 || door > 5)
	{
		bug("Do_move: bad door %d.", door);
		return;
	}

	/*
	 * Exit trigger, if activated, bail out. Only PCs are triggered.
	 */
	// commented out b/c/ mobprogs don't exist
	//if (!IS_NPC(ch))// && mp_exit_trigger(ch, door))
	//	return;

	/* no hiding and moving */
	REMOVE_BIT(ch->affected_by, AFF_HIDE);

	// NEW CHANGE TO SNEAK: Checked as you move.
	if(number_percent() > get_skill(ch, gsn_sneak)
	&& IS_AFFECTED(ch, AFF_SNEAK)) {
		Cprintf(ch, "You fail to move silently.\n\r");
		REMOVE_BIT(ch->affected_by, AFF_SNEAK);
		affect_strip(ch, gsn_sneak);
	}

	if(is_affected(ch, gsn_block)) {
		Cprintf(ch, "The exit opens up as you move to leave.\n\r");
		affect_strip(ch, gsn_block);
	}

	if(is_affected(ch, gsn_ambush)) {
		Cprintf(ch, "You lose the element of surprise.\n\r");
		affect_strip(ch, gsn_ambush);
	}

	if (ch->in_room == NULL)
		char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));

	in_room = ch->in_room;
	if ((pexit = in_room->exit[door]) == NULL
		|| (to_room = pexit->u1.to_room) == NULL
		|| !can_see_room(ch, pexit->u1.to_room)
		|| to_room == NULL)
	{
		Cprintf(ch, "Alas, you cannot go that way.\n\r");
		return;
	}

	if (room_is_affected(ch->in_room, gsn_web)
	&& ch->race != race_lookup("sliver")) {
		paf = affect_find(ch->in_room->affected, gsn_web);
		if(paf != NULL
	      	&& ch->level < paf->level + 8
	      	&& ch->level > paf->level - 8
	      	&& (IS_NPC(ch) || is_clan(ch))
		&& number_percent() < 65) {
			Cprintf(ch, "You are stuck in a sliver's web!\n\r");
			act("$n is stuck in a sliver's web!.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			WAIT_STATE(ch, PULSE_VIOLENCE * 1 / 3);
			return;
		}
	}
	if (room_is_affected(ch->in_room, gsn_cave_in)) {
		paf = affect_find(ch->in_room->affected, gsn_cave_in);
		if(paf != NULL
		&& ch->level < paf->level + 8
		&& ch->level > paf->level - 8
	      	&& (IS_NPC(ch) || is_clan(ch))
		&& number_percent() < 50)
		{
			Cprintf(ch, "A pile of rocks blocks your path!\n\r");
			act("$n stumbles on a pile of rubble!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			WAIT_STATE(ch, PULSE_VIOLENCE);
			return;
		}
	}

	if (IS_SET(pexit->exit_info, EX_CLOSED)
		&& (!is_affected(ch, gsn_pass_door) || IS_SET(pexit->exit_info, EX_NOPASS))
		&& !IS_TRUSTED(ch, ANGEL))
	{
		act("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR, POS_RESTING);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)
		&& ch->master != NULL
		&& in_room == ch->master->in_room)
	{
		Cprintf(ch, "What?  And leave your beloved master?\n\r");
		return;
	}

	if (!is_room_owner(ch, to_room) && room_is_private(to_room) && ch->trust < 60)
	{
		Cprintf(ch, "That room is private right now.\n\r");
		return;
	}

	if (to_room->clan != ch->clan 
		&& to_room->clan
		&& IS_SET(to_room->room_flags, ROOM_CLAN)
		&& !clan_table[ch->clan].independent)
	{
		Cprintf(ch, "You cannot go that way.\n\r");
		return;
	}

	if (to_room->clan && 
	    !IS_IMMORTAL(ch) &&
	    clan_table[ch->clan].pkiller != clan_table[to_room->clan].pkiller)
	{
		Cprintf(ch, "You cannot go that way.\n\r");
		return;
	}

	if (!IS_NPC(ch))
	{
		int move;

		if (in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR)
		{
			if (!IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch))
			{
				Cprintf(ch, "You can't fly.\n\r");
				return;
			}
		}

		if ((in_room->sector_type == SECT_WATER_NOSWIM ||
			 to_room->sector_type == SECT_WATER_NOSWIM) &&
			!IS_AFFECTED(ch, AFF_FLYING))
		{
			OBJ_DATA *obj;
			bool found;

			/* Look for a boat */
			found = FALSE;

			if (IS_IMMORTAL(ch))
				found = TRUE;

			for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
			{
				if (obj->item_type == ITEM_BOAT)
				{
					found = TRUE;
					break;
				}
			}

			if (IS_AFFECTED(ch, AFF_WATERWALK))
				found = TRUE;

			if (!found)
			{
				Cprintf(ch, "You need a boat to go there.\n\r");
				return;
			}
		}

		move = movement_loss[UMIN(SECT_MAX - 1, in_room->sector_type)]
			+ movement_loss[UMIN(SECT_MAX - 1, to_room->sector_type)];

		move /= 2;				/* i.e. the average */

		over = can_carry_w(ch) - get_carry_weight(ch);
		/* give them a little le-way */
		if (over < -10)
		{
			move -= (over / 100);
		}

		if(is_affected(ch, gsn_stance_turtle)) {
			move++;
		}

	    if (in_room->sector_type == SECT_UNDERWATER) {
	        AFFECT_DATA *affect = get_room_affect(ch->in_room, gsn_flood);
	        bool ignoreFlood = FALSE;
            
	        if (affect != NULL) {
	            if (!is_clan(ch)) {
	                // Non-clanners are not affected by flood
	                ignoreFlood = TRUE;
	            } else if (is_clan(ch) && !affect->clan) {
	                // Clanners are not affected by NC flood
	                ignoreFlood = TRUE;
	            }
	        }
            
	        if (ignoreFlood) {
                Cprintf(ch, "You are tempted to join the wildlife playing in the water.\n\r");
                act("$n is unaffected by the splashing water.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
            } else if ( !IS_AFFECTED(ch, AFF_WATERWALK) ) {
                Cprintf(ch, "You thrash about awkwardly and start to swim.\n\r");
                act("$n flails their limbs and tries to swim.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
                WAIT_STATE(ch, 6);
            } else {
                Cprintf(ch, "You glide smoothly through the water.\n\r");
                act("$n glides smoothly through the water.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
                move /= 4;
            }
        }

		/* conditional effects */
		if (IS_AFFECTED(ch, AFF_FLYING) || IS_AFFECTED(ch, AFF_HASTE))
			move /= 2;
		if (IS_AFFECTED(ch, AFF_SLOW))
			move *= 2;

		if (ch->move < move)
		{
			Cprintf(ch, "You are too exhausted.\n\r");
			return;
		}


		// Check for block exit
		for(rch = ch->in_room->people;rch != NULL;rch = rch->next_in_room) {
			if(is_affected(rch, gsn_block)) {
				paf = affect_find(rch->affected, gsn_block);
				if(paf->modifier == door) {
					blocker = rch;
					break;
				}
			}
		}

		// Are they blocked?
		if(blocker != NULL) {
			if(is_clan(blocker)
			&& is_clan(ch)
			&& blocker->level > ch->level - 8
			&& blocker->level < ch->level + 8
			&& number_percent() < get_skill(blocker, gsn_block))
				blocked = TRUE;

			if(IS_NPC(ch)
			&& number_percent() < get_skill(blocker, gsn_block))
				blocked = TRUE;
		}

		if(blocker != NULL && blocked) {
			act("You block $N from leaving the room!", blocker, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "%s is blocking this exit.  You can't go that way.\n\r", blocker->name);
			WAIT_STATE(ch, PULSE_VIOLENCE);
			return;
		}

		if(room_is_affected(ch->in_room->exit[door]->u1.to_room, gsn_repel)) {
                	paf = affect_find(ch->in_room->exit[door]->u1.to_room->affected, gsn_repel);
                	if(paf != NULL
                	&& ch->level < paf->level + 8
                	&& ch->level > paf->level - 8
                	&& (IS_NPC(ch) || is_clan(ch))
                	&& number_percent() < 40)
                	{
	      			Cprintf(ch, "You try to enter the next room, but a powerful magic repels you!\n\r");
       	  			act("$n is repelled by a powerful force!.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
				WAIT_STATE(ch, PULSE_VIOLENCE);
                      		return;
                	}
        	}

		if(!is_affected(ch, gsn_shadow_walk)) {
		WAIT_STATE(ch, 1);
		}
		ch->move -= move;
	}

	if (IS_AFFECTED(ch, AFF_SNEAK))
	{
		for(rch = ch->in_room->people;rch != NULL;rch = rch->next_in_room)
		{
			if(IS_SET(rch->act, PLR_HOLYLIGHT))
				Cprintf(rch, "%s sneaks away to the %s.\n\r", IS_NPC(ch) ? ch->short_descr : ch->name, dir_name[door]);
		}
	}

	if (!IS_AFFECTED(ch, AFF_SNEAK)
	&& ch->invis_level < LEVEL_HERO)
	{
		act("$n leaves $T.", ch, NULL, dir_name[door], TO_ROOM, POS_RESTING);
	}

	in_room = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, to_room);
	to_room = ch->in_room;

	if (IS_AFFECTED(ch, AFF_SNEAK))
        {
                for(rch = ch->in_room->people;rch != NULL;rch = rch->next_in_room)
                {
                        if(IS_SET(rch->act, PLR_HOLYLIGHT))
				 Cprintf(rch, "%s sneaks into the room.\n\r", IS_NPC(ch) ? ch->short_descr : ch->name);
                }
        }

	if (!IS_AFFECTED(ch, AFF_SNEAK) && ch->invis_level < LEVEL_HERO)
	{
		act("$n has arrived.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	do_look(ch, "auto");

	if (in_room == to_room)		/* no circular follows */
		return;

	// Check people in the room we just left behind
	for (fch = in_room->people; fch != NULL; fch = fch_next)
	{
		fch_next = fch->next_in_room;

		/* sometimes we don't want to follow! */
		if(follow)
			break;

		if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM)
			&& fch->position < POS_STANDING)
			do_stand(fch, "");

		if (fch->master == ch && fch->position == POS_STANDING
			&& can_see_room(fch, to_room))
		{
			if (IS_SET(ch->in_room->room_flags, ROOM_LAW)
				&& (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE)))
			{
				if (ch->charClass != class_lookup("enchanter"))
				{
					act("You can't bring $N into the city.", ch, NULL, fch, TO_CHAR, POS_RESTING);
					act("You aren't allowed in the city.", fch, NULL, NULL, TO_CHAR, POS_RESTING);
					continue;
				}
			}

			if (!IS_NPC(fch) && fch->pcdata->ctf_flag)
			{
				act("You cannot follow anyone, you're busy carrying the flag.", fch, NULL, NULL, TO_CHAR, POS_RESTING);
				return;
			}

			act("You follow $N.", fch, NULL, ch, TO_CHAR, POS_RESTING);
			move_char(fch, door, FALSE);
		}
	}

	/*
	 * If someone is following the char, these triggers get activated
	 * for the followers before the char, but it's safer this way...
	if (IS_NPC(ch) && HAS_TRIGGER(ch, TRIG_ENTRY))
		mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_ENTRY);
	if (!IS_NPC(ch))
		mp_greet_trigger(ch);
	 */

	if(!is_affected(ch, gsn_jump))
	{
		// Do some trap tracking
        	trap_needle(ch);
        	trap_darts(ch);
        	trap_wire(ch);

	        // check for runes
        	fire_rune(ch);
        	shackle_rune(ch);
	}

	/* probably the most appropriate place for this... basically,
	 * you stand a chance of falling asleep if you move into an outdoors
	 * room during the day if you have stone sleep */
	if ((((time_info.hour > 4) && (time_info.hour < 20))
		&& (number_bits(2) == 0)) && (get_skill(ch,gsn_stone_sleep) > 0))
	{
		if (number_percent() == 1)
		{
			if (ch->position == POS_STANDING)
			{
				Cprintf(ch, "You feel the stone calling as you fall asleep!\n\r");
				act("$n suddenly stiffens and falls asleep!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
				ch->position = POS_SLEEPING;
			}
		}
	}

	// Kirre shouldn't venture outside in the day.
	if(ch->race == race_lookup("kirre")
	&& ch->remort
	&& IS_OUTSIDE(ch)
	&& number_percent() <= 3
	&& time_info.hour > 10
	&& time_info.hour < 20
	&& (weather_info.sky == SKY_CLOUDLESS
	   || weather_info.sky == SKY_CLOUDY)
	&& !IS_AFFECTED(ch, AFF_BLIND)) {
		Cprintf(ch, "Your sensitive eyes are stricken blind by the sun!\n\r");
		act("$n is stricken blind by the sun light!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		af.where = TO_AFFECTS;
                af.type = gsn_blindness;
                af.level = ch->level;
                af.duration = 0;
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.bitvector = AFF_BLIND;
                affect_join(ch, &af);
	}

	if(is_affected(ch, gsn_shackle_rune))
	{
		act("$n tries to move while magically shackled.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You struggle against the shackles!\n\r");
		ch->move -= number_range(1, 12);
		ch->move = UMAX(ch->move, 0);
	}

	//if you're playing CTF and have the flag, stay put a while before you cap
	if (!IS_NPC(ch))
	{
		if (ch->pcdata->ctf_team > 0 && ch->pcdata->ctf_flag && ch->in_room->vnum == ROOM_CTF_FLAG(ch->pcdata->ctf_team))
		{
			WAIT_STATE(ch, 8 * PULSE_VIOLENCE);
			Cprintf(ch, "You prepare to capture the flag!\n\r");
		}

		//walk a little slower with the flag
		if (ch->pcdata->ctf_flag && ch->pcdata->ctf_team > 0)
		{
			WAIT_STATE(ch, 2 * PULSE_VIOLENCE / 3);
		}
	}

	return;
}


void
do_north(CHAR_DATA * ch, char *argument)
{
	move_char(ch, DIR_NORTH, FALSE);
	return;
}

void
do_east(CHAR_DATA * ch, char *argument)
{
	move_char(ch, DIR_EAST, FALSE);
	return;
}


void
do_south(CHAR_DATA * ch, char *argument)
{
	move_char(ch, DIR_SOUTH, FALSE);
	return;
}


void
do_west(CHAR_DATA * ch, char *argument)
{
	move_char(ch, DIR_WEST, FALSE);
	return;
}


void
do_up(CHAR_DATA * ch, char *argument)
{
	move_char(ch, DIR_UP, FALSE);
	return;
}


void
do_down(CHAR_DATA * ch, char *argument)
{
	move_char(ch, DIR_DOWN, FALSE);
	return;
}


int
find_door(CHAR_DATA * ch, char *arg)
{
	EXIT_DATA *pexit;
	int door;

	if (!str_cmp(arg, "n") || !str_cmp(arg, "north"))
		door = 0;
	else if (!str_cmp(arg, "e") || !str_cmp(arg, "east"))
		door = 1;
	else if (!str_cmp(arg, "s") || !str_cmp(arg, "south"))
		door = 2;
	else if (!str_cmp(arg, "w") || !str_cmp(arg, "west"))
		door = 3;
	else if (!str_cmp(arg, "u") || !str_cmp(arg, "up"))
		door = 4;
	else if (!str_cmp(arg, "d") || !str_cmp(arg, "down"))
		door = 5;
	else
	{
		for (door = 0; door <= 5; door++)
		{
			if ((pexit = ch->in_room->exit[door]) != NULL
				&& IS_SET(pexit->exit_info, EX_ISDOOR)
				&& pexit->keyword != NULL
				&& is_name(arg, pexit->keyword))
				return door;
		}
		act("I see no $T here.", ch, NULL, arg, TO_CHAR, POS_RESTING);
		return -1;
	}

	if ((pexit = ch->in_room->exit[door]) == NULL)
	{
		act("I see no door $T here.", ch, NULL, arg, TO_CHAR, POS_RESTING);
		return -1;
	}

	if (!IS_SET(pexit->exit_info, EX_ISDOOR))
	{
		Cprintf(ch, "You can't do that.\n\r");
		return -1;
	}

	return door;
}


void
do_warp(CHAR_DATA * ch, char *argument)
{
	ROOM_INDEX_DATA *location;
	CHAR_DATA *mob;
	int monmon;
	int cost = 0;
	char buf[255];

	if (IS_NPC(ch))
		return;

	for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
	{
		if (IS_NPC(mob) && mob->spec_fun == spec_lookup("spec_portal_keeper"))
			break;
	}

	if (mob == NULL)
	{
		Cprintf(ch, "You can't do that here.\n\r");
		return;
	}

	cost = (ch->level * 2);

	if(cost < 1)
		cost = 1;

	if ((ch->silver / 100 + ch->gold) < cost)
	{
		sprintf(buf, "It will cost %d gold to use Selina's Bridge.", cost);
		do_say(mob, buf);
		return;
	}

	Cprintf(ch, "You touch the statue and vanish into Selina's Bridge...\n\r");
	if (ch->gold >= cost)
	{
		ch->gold = ch->gold - cost;
	}
	else
	{
		monmon = cost - ch->gold;
		ch->gold = 0;
		ch->silver = ch->silver - monmon * 100;
	}


	if (ch->in_room->area->continent == 0)
		location = get_room_index(ROOM_VNUM_PORTAL_DOMINIA);
	else
		location = get_room_index(ROOM_VNUM_PORTAL_TERRA);

	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);

	act("$n vanishes into Selina's Bridge.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	char_from_room(ch);
	char_to_room(ch, location);

	act("$n pops into existence next to the statue.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	do_look(ch, "auto");

	Cprintf(ch, "You feel disoriented being transported.\n\r");
	WAIT_STATE(ch, 3 * PULSE_VIOLENCE);

	return;
}


void
do_open(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int door;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Open what?\n\r");
		return;
	}
	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
	}

	if ((obj = get_obj_here(ch, arg)) != NULL)
	{
		/* open portal */
		if (obj->item_type == ITEM_PORTAL)
		{
			if (!IS_SET(obj->value[1], EX_ISDOOR))
			{
				Cprintf(ch, "You can't do that.\n\r");
				return;
			}

			if (!IS_SET(obj->value[1], EX_CLOSED))
			{
				Cprintf(ch, "It's already open.\n\r");
				return;
			}

			if (IS_SET(obj->value[1], EX_LOCKED))
			{
				Cprintf(ch, "It's locked.\n\r");
				return;
			}

			REMOVE_BIT(obj->value[1], EX_CLOSED);
			act("You open $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n opens $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);

			if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 50)
				Cprintf(ch, "!!SOUND(sounds/wav/open*.wav V=80 P=20 T=admin)");

			return;
		}
		if ((str_cmp(arg, "n") && str_cmp(arg, "north"))
			&& (str_cmp(arg, "e") && str_cmp(arg, "east"))
			&& (str_cmp(arg, "s") && str_cmp(arg, "south"))
			&& (str_cmp(arg, "w") && str_cmp(arg, "west"))
			&& (str_cmp(arg, "u") && str_cmp(arg, "up"))
			&& (str_cmp(arg, "d") && str_cmp(arg, "down")))
		{

			/* 'open object' */
			if (obj->item_type != ITEM_CONTAINER)
			{
				Cprintf(ch, "That's not a container.\n\r");
				return;
			}
			if (!IS_SET(obj->value[1], CONT_CLOSED))
			{
				Cprintf(ch, "It's already open.\n\r");
				return;
			}
			if (!IS_SET(obj->value[1], CONT_CLOSEABLE))
			{
				Cprintf(ch, "You can't do that.\n\r");
				return;
			}
			if (IS_SET(obj->value[1], CONT_LOCKED))
			{
				Cprintf(ch, "It's locked.\n\r");
				return;
			}

			REMOVE_BIT(obj->value[1], CONT_CLOSED);
			act("You open $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n opens $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);

			return;
		}
	}

	if ((door = find_door(ch, arg)) >= 0)
	{
		/* 'open door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (!IS_SET(pexit->exit_info, EX_CLOSED))
		{
			Cprintf(ch, "It's already open.\n\r");
			return;
		}
		if (IS_SET(pexit->exit_info, EX_LOCKED))
		{
			Cprintf(ch, "It's locked.\n\r");
			return;
		}

		REMOVE_BIT(pexit->exit_info, EX_CLOSED);
		act("$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM, POS_RESTING);
		Cprintf(ch, "Ok.\n\r");

		/* open the other side */
		if ((to_room = pexit->u1.to_room) != NULL
			&& (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
			&& pexit_rev->u1.to_room == ch->in_room)
		{
			CHAR_DATA *rch;

			REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
			for (rch = to_room->people; rch != NULL; rch = rch->next_in_room)
				act("The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR, POS_RESTING);
		}
	}

	return;
}


void
do_close(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int door;

	one_argument(argument, arg);

	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
	}

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Close what?\n\r");
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL)
	{
		/* portal stuff */
		if (obj->item_type == ITEM_PORTAL)
		{

			if (!IS_SET(obj->value[1], EX_ISDOOR) || IS_SET(obj->value[1], EX_NOCLOSE))
			{
				Cprintf(ch, "You can't do that.\n\r");
				return;
			}

			if (IS_SET(obj->value[1], EX_CLOSED))
			{
				Cprintf(ch, "It's already closed.\n\r");
				return;
			}

			SET_BIT(obj->value[1], EX_CLOSED);
			act("You close $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n closes $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);

			return;
		}

		/* 'close object' */
		if (obj->item_type != ITEM_CONTAINER)
		{
			Cprintf(ch, "That's not a container.\n\r");
			return;
		}
		if (IS_SET(obj->value[1], CONT_CLOSED))
		{
			Cprintf(ch, "It's already closed.\n\r");
			return;
		}
		if (!IS_SET(obj->value[1], CONT_CLOSEABLE))
		{
			Cprintf(ch, "You can't do that.\n\r");
			return;
		}

		SET_BIT(obj->value[1], CONT_CLOSED);
		act("You close $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$n closes $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);

		return;
	}

	if ((door = find_door(ch, arg)) >= 0)
	{
		/* 'close door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (IS_SET(pexit->exit_info, EX_CLOSED))
		{
			Cprintf(ch, "It's already closed.\n\r");
			return;
		}

		SET_BIT(pexit->exit_info, EX_CLOSED);
		act("$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM, POS_RESTING);
		Cprintf(ch, "Ok.\n\r");

		/* close the other side */
		if ((to_room = pexit->u1.to_room) != NULL
			&& (pexit_rev = to_room->exit[rev_dir[door]]) != 0
			&& pexit_rev->u1.to_room == ch->in_room)
		{
			CHAR_DATA *rch;

			SET_BIT(pexit_rev->exit_info, EX_CLOSED);
			for (rch = to_room->people; rch != NULL; rch = rch->next_in_room)
				act("The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR, POS_RESTING);
		}
	}

	return;
}


bool
has_key(CHAR_DATA * ch, int key)
{
	OBJ_DATA *obj;

	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	{
		if (obj->pIndexData->vnum == key)
			return TRUE;
	}

	return FALSE;
}


void
do_lock(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int door;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Lock what?\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
	}

	if ((obj = get_obj_here(ch, arg)) != NULL)
	{
		/* portal stuff */
		if (obj->item_type == ITEM_PORTAL)
		{
			if (!IS_SET(obj->value[1], EX_ISDOOR)
				|| IS_SET(obj->value[1], EX_NOCLOSE))
			{
				Cprintf(ch, "You can't do that.\n\r");
				return;
			}
			if (!IS_SET(obj->value[1], EX_CLOSED))
			{
				Cprintf(ch, "It's not closed.\n\r");
				return;
			}

			if (obj->value[4] < 0 || IS_SET(obj->value[1], EX_NOLOCK))
			{
				Cprintf(ch, "It can't be locked.\n\r");
				return;
			}

			if (!has_key(ch, obj->value[4]))
			{
				Cprintf(ch, "You lack the key.\n\r");
				return;
			}

			if (IS_SET(obj->value[1], EX_LOCKED))
			{
				Cprintf(ch, "It's already locked.\n\r");
				return;
			}

			SET_BIT(obj->value[1], EX_LOCKED);
			act("You lock $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n locks $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			return;
		}

		/* 'lock object' */
		if (obj->item_type != ITEM_CONTAINER)
		{
			Cprintf(ch, "That's not a container.\n\r");
			return;
		}
		if (!IS_SET(obj->value[1], CONT_CLOSED))
		{
			Cprintf(ch, "It's not closed.\n\r");
			return;
		}
		if (obj->value[2] < 0)
		{
			Cprintf(ch, "It can't be locked.\n\r");
			return;
		}
		if (!has_key(ch, obj->value[2]))
		{
			Cprintf(ch, "You lack the key.\n\r");
			return;
		}
		if (IS_SET(obj->value[1], CONT_LOCKED))
		{
			Cprintf(ch, "It's already locked.\n\r");
			return;
		}

		SET_BIT(obj->value[1], CONT_LOCKED);
		act("You lock $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$n locks $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0)
	{
		/* 'lock door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (!IS_SET(pexit->exit_info, EX_CLOSED))
		{
			Cprintf(ch, "It's not closed.\n\r");
			return;
		}
		if (pexit->key < 0)
		{
			Cprintf(ch, "It can't be locked.\n\r");
			return;
		}
		if (!has_key(ch, pexit->key))
		{
			Cprintf(ch, "You lack the key.\n\r");
			return;
		}
		if (IS_SET(pexit->exit_info, EX_LOCKED))
		{
			Cprintf(ch, "It's already locked.\n\r");
			return;
		}

		SET_BIT(pexit->exit_info, EX_LOCKED);
		Cprintf(ch, "*Click*\n\r");
		act("$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM, POS_RESTING);

		/* lock the other side */
		if ((to_room = pexit->u1.to_room) != NULL
			&& (pexit_rev = to_room->exit[rev_dir[door]]) != 0
			&& pexit_rev->u1.to_room == ch->in_room)
		{
			SET_BIT(pexit_rev->exit_info, EX_LOCKED);
		}
	}

	return;
}


void
do_unlock(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int door;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Unlock what?\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
	}

	if ((obj = get_obj_here(ch, arg)) != NULL)
	{
		/* portal stuff */
		if (obj->item_type == ITEM_PORTAL)
		{
			if (!IS_SET(obj->value[1], EX_ISDOOR))
			{
				Cprintf(ch, "You can't do that.\n\r");
				return;
			}

			if (!IS_SET(obj->value[1], EX_CLOSED))
			{
				Cprintf(ch, "It's not closed.\n\r");
				return;
			}

			if (obj->value[4] < 0)
			{
				Cprintf(ch, "It can't be unlocked.\n\r");
				return;
			}

			if (!has_key(ch, obj->value[4]))
			{
				Cprintf(ch, "You lack the key.\n\r");
				return;
			}

			if (!IS_SET(obj->value[1], EX_LOCKED))
			{
				Cprintf(ch, "It's already unlocked.\n\r");
				return;
			}

			REMOVE_BIT(obj->value[1], EX_LOCKED);
			act("You unlock $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n unlocks $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			return;
		}

		/* 'unlock object' */
		if (obj->item_type != ITEM_CONTAINER)
		{
			Cprintf(ch, "That's not a container.\n\r");
			return;
		}
		if (!IS_SET(obj->value[1], CONT_CLOSED))
		{
			Cprintf(ch, "It's not closed.\n\r");
			return;
		}
		if (obj->value[2] < 0)
		{
			Cprintf(ch, "It can't be unlocked.\n\r");
			return;
		}
		if (!has_key(ch, obj->value[2]))
		{
			Cprintf(ch, "You lack the key.\n\r");
			return;
		}
		if (!IS_SET(obj->value[1], CONT_LOCKED))
		{
			Cprintf(ch, "It's already unlocked.\n\r");
			return;
		}

		REMOVE_BIT(obj->value[1], CONT_LOCKED);
		act("You unlock $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$n unlocks $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0)
	{
		/* 'unlock door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (!IS_SET(pexit->exit_info, EX_CLOSED))
		{
			Cprintf(ch, "It's not closed.\n\r");
			return;
		}
		if (pexit->key < 0)
		{
			Cprintf(ch, "It can't be unlocked.\n\r");
			return;
		}
		if (!has_key(ch, pexit->key))
		{
			Cprintf(ch, "You lack the key.\n\r");
			return;
		}
		if (!IS_SET(pexit->exit_info, EX_LOCKED))
		{
			Cprintf(ch, "It's already unlocked.\n\r");
			return;
		}

		REMOVE_BIT(pexit->exit_info, EX_LOCKED);
		Cprintf(ch, "*Click*\n\r");
		act("$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM, POS_RESTING);

		/* unlock the other side */
		if ((to_room = pexit->u1.to_room) != NULL
			&& (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
			&& pexit_rev->u1.to_room == ch->in_room)
		{
			REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
		}
	}

	return;
}


void
do_bash_door(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *gch;
	int door;

		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;


	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Bash what door?\n\r");
		return;
	}

	WAIT_STATE(ch, skill_table[gsn_bashdoor].beats);

	/* look for guards */
	for (gch = ch->in_room->people; gch; gch = gch->next_in_room)
	{
		if (IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level)
		{
			act("$N is standing too close to the door.", ch, NULL, gch, TO_CHAR, POS_RESTING);
			return;
		}
	}

	if ((door = find_door(ch, arg)) < 0)
	{
		return;
	}


	if (!IS_NPC(ch) && number_percent() > (get_skill(ch, gsn_bashdoor) * ch->perm_stat[STAT_STR] / 25))
	{
		Cprintf(ch, "You failed.\n\r");
		check_improve(ch, gsn_bashdoor, FALSE, 2);
		return;
	}

/*	if ((door = find_door(ch, arg)) >= 0)
	{
*/

		pexit = ch->in_room->exit[door];
		if (!IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
		{
			Cprintf(ch, "It's not closed.\n\r");
			return;
		}

		if (IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
		{
			Cprintf(ch, "This door is too strong, even for you.\n\r");
			return;
		}

		if (IS_SET(pexit->exit_info, EX_LOCKED))
			REMOVE_BIT(pexit->exit_info, EX_LOCKED);

		REMOVE_BIT(pexit->exit_info, EX_CLOSED);
		Cprintf(ch, "*Bang!* You bash the door in.\n\r");
		act("$n bashes in the $d.", ch, NULL, pexit->keyword, TO_ROOM, POS_RESTING);
		check_improve(ch, gsn_bashdoor, TRUE, 2);

		/* pick the other side */
		if ((to_room = pexit->u1.to_room) != NULL
			&& (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
			&& pexit_rev->u1.to_room == ch->in_room)
		{
			if (IS_SET(pexit_rev->exit_info, EX_LOCKED))
				REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);

			REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
		}
/*	}
*/
	return;
}


void
do_pick(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *gch;
	OBJ_DATA *obj;
	int door;

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Pick what?\n\r");
		return;
	}

	WAIT_STATE(ch, skill_table[gsn_pick_lock].beats);

	/* look for guards */
	for (gch = ch->in_room->people; gch; gch = gch->next_in_room)
	{
		if (IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level)
		{
			act("$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR, POS_RESTING);
			return;
		}
	}

	if ((obj = get_obj_here(ch, arg)) != NULL)
	{
		if (!IS_NPC(ch) && number_percent() > get_skill(ch, gsn_pick_lock))
		{
			Cprintf(ch, "You failed.\n\r");
			check_improve(ch, gsn_pick_lock, FALSE, 2);
			return;
		}

		/* portal stuff */
		if (obj->item_type == ITEM_PORTAL)
		{
			if (!IS_SET(obj->value[1], EX_ISDOOR))
			{
				Cprintf(ch, "You can't do that.\n\r");
				return;
			}

			if (!IS_SET(obj->value[1], EX_CLOSED))
			{
				Cprintf(ch, "It's not closed.\n\r");
				return;
			}

			if (obj->value[4] < 0)
			{
				Cprintf(ch, "It can't be unlocked.\n\r");
				return;
			}

			if (IS_SET(obj->value[1], EX_PICKPROOF))
			{
				Cprintf(ch, "You failed.\n\r");
				return;
			}

			REMOVE_BIT(obj->value[1], EX_LOCKED);
			act("You pick the lock on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			check_improve(ch, gsn_pick_lock, TRUE, 2);
			return;
		}

		/* 'pick object' */
		if (obj->item_type != ITEM_CONTAINER)
		{
			Cprintf(ch, "That's not a container.\n\r");
			return;
		}

		if (!IS_SET(obj->value[1], CONT_CLOSED))
		{
			Cprintf(ch, "It's not closed.\n\r");
			return;
		}

		if (obj->value[2] < 0)
		{
			Cprintf(ch, "It can't be unlocked.\n\r");
			return;
		}

		if (!IS_SET(obj->value[1], CONT_LOCKED))
		{
			Cprintf(ch, "It's already unlocked.\n\r");
			return;
		}

		if (IS_SET(obj->value[1], CONT_PICKPROOF))
		{
			Cprintf(ch, "You failed.\n\r");
			return;
		}

		REMOVE_BIT(obj->value[1], CONT_LOCKED);
		act("You pick the lock on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		check_improve(ch, gsn_pick_lock, TRUE, 2);
		return;
	}

	if ((door = find_door(ch, arg)) < 0)
		return;

	if ((door = find_door(ch, arg)) >= 0)
	{
		/* 'pick door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		if (!IS_NPC(ch) && number_percent() > get_skill(ch, gsn_pick_lock))
		{
			Cprintf(ch, "You failed.\n\r");
			check_improve(ch, gsn_pick_lock, FALSE, 2);
			return;
		}

		pexit = ch->in_room->exit[door];
		if (!IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
		{
			Cprintf(ch, "It's not closed.\n\r");
			return;
		}
		if (pexit->key < 0 && !IS_IMMORTAL(ch))
		{
			Cprintf(ch, "It can't be picked.\n\r");
			return;
		}
		if (!IS_SET(pexit->exit_info, EX_LOCKED))
		{
			Cprintf(ch, "It's already unlocked.\n\r");
			return;
		}
		if (IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
		{
			Cprintf(ch, "You failed.\n\r");
			return;
		}

		REMOVE_BIT(pexit->exit_info, EX_LOCKED);
		Cprintf(ch, "*Click*\n\r");
		act("$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM, POS_RESTING);
		check_improve(ch, gsn_pick_lock, TRUE, 2);

		/* pick the other side */
		if ((to_room = pexit->u1.to_room) != NULL
			&& (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
			&& pexit_rev->u1.to_room == ch->in_room)
		{
			REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
		}
	}

	return;
}


void
do_stand(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj = NULL;

	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
	}

	if (argument[0] != '\0')
	{
		if (ch->position == POS_FIGHTING)
		{
			Cprintf(ch, "Maybe you should finish fighting first?\n\r");
			return;
		}

		obj = get_obj_list(ch, argument, ch->in_room->contents);
		if (obj == NULL)
		{
			Cprintf(ch, "You don't see that here.\n\r");
			return;
		}

		if (obj->item_type != ITEM_FURNITURE
			|| (!IS_SET(obj->value[2], STAND_AT)
				&& !IS_SET(obj->value[2], STAND_ON)
				&& !IS_SET(obj->value[2], STAND_IN)))
		{
			Cprintf(ch, "You can't seem to find a place to stand.\n\r");
			return;
		}
		if (ch->on != obj && count_users(obj) >= obj->value[0])
		{
			act("There's no room to stand on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
			return;
		}
		ch->on = obj;
	}

	switch (ch->position)
	{
	case POS_SLEEPING:
		if (IS_AFFECTED(ch, AFF_SLEEP))
		{
			Cprintf(ch, "You can't wake up!\n\r");
			return;
		}

		if (obj == NULL)
		{
			Cprintf(ch, "You wake and stand up.\n\r");
			act("$n wakes and stands up.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			ch->on = NULL;
		}
		else if (IS_SET(obj->value[2], STAND_AT))
		{
			act("You wake and stand at $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes and stands at $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], STAND_ON))
		{
			act("You wake and stand on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes and stands on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else
		{
			act("You wake and stand in $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes and stands in $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		ch->position = POS_STANDING;
		do_look(ch, "auto");
		break;

	case POS_RESTING:
	case POS_SITTING:
		if (obj == NULL)
		{
			Cprintf(ch, "You stand up.\n\r");
			act("$n stands up.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			ch->on = NULL;
		}
		else if (IS_SET(obj->value[2], STAND_AT))
		{
			act("You stand at $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n stands at $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], STAND_ON))
		{
			act("You stand on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n stands on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else
		{
			act("You stand in $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n stands on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		ch->position = POS_STANDING;
		break;

	case POS_STANDING:
		Cprintf(ch, "You are already standing.\n\r");
		break;

	case POS_FIGHTING:
		Cprintf(ch, "You are already fighting!\n\r");
		break;
	}

	return;
}


void
do_rest(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj = NULL;

	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
	}
	if (ch->position == POS_FIGHTING)
	{
		Cprintf(ch, "You are already fighting!\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_SLEEP))
	{
		Cprintf(ch, "You can't wake up!\n\r");
		return;
	}

	/* okay, now that we know we can rest, find an object to rest on */
	if (argument[0] != '\0')
	{
		obj = get_obj_list(ch, argument, ch->in_room->contents);
		if (obj == NULL)
		{
			Cprintf(ch, "You don't see that here.\n\r");
			return;
		}
	}
	else
		obj = ch->on;

	if (obj != NULL)
	{
		if (!IS_SET(obj->item_type, ITEM_FURNITURE)
			|| (!IS_SET(obj->value[2], REST_ON)
				&& !IS_SET(obj->value[2], REST_IN)
				&& !IS_SET(obj->value[2], REST_AT)))
		{
			Cprintf(ch, "You can't rest on that.\n\r");
			return;
		}

		if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
		{
			act("There's no more room on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
			return;
		}

		ch->on = obj;
	}

	switch (ch->position)
	{
	case POS_SLEEPING:
		if (IS_AFFECTED(ch, AFF_SLEEP))
		{
			Cprintf(ch, "You can't wake up!\n\r");
			return;
		}

		if (obj == NULL)
		{
			Cprintf(ch, "You wake up and start resting.\n\r");
			act("$n wakes up and starts resting.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], REST_AT))
		{
			act("You wake up and rest at $p.", ch, obj, NULL, TO_CHAR, POS_SLEEPING);
			act("$n wakes up and rests at $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], REST_ON))
		{
			act("You wake up and rest on $p.", ch, obj, NULL, TO_CHAR, POS_SLEEPING);
			act("$n wakes up and rests on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else
		{
			act("You wake up and rest in $p.", ch, obj, NULL, TO_CHAR, POS_SLEEPING);
			act("$n wakes up and rests in $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}

		ch->position = POS_RESTING;
		do_look(ch, "auto");
		break;

	case POS_RESTING:
		Cprintf(ch, "You are already resting.\n\r");
		break;

	case POS_STANDING:
		if (obj == NULL)
		{
			Cprintf(ch, "You rest.\n\r");
			act("$n sits down and rests.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], REST_AT))
		{
			act("You sit down at $p and rest.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n sits down at $p and rests.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], REST_ON))
		{
			act("You sit on $p and rest.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n sits on $p and rests.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else
		{
			act("You rest in $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n rests in $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}

		ch->position = POS_RESTING;
		break;

	case POS_SITTING:
		if (obj == NULL)
		{
			Cprintf(ch, "You rest.\n\r");
			act("$n rests.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], REST_AT))
		{
			act("You rest at $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n rests at $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], REST_ON))
		{
			act("You rest on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n rests on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else
		{
			act("You rest in $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n rests in $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		ch->position = POS_RESTING;
		break;
	}

	return;
}

/* yah, yah, blatantly ripped off from MHS, but so what? :) */
void do_roar(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *vch, *vch_next;
	if ((ch->race == race_lookup("red dragon"))
		|| (ch->race == race_lookup("blue dragon"))
		|| (ch->race == race_lookup("black dragon"))
		|| (ch->race == race_lookup("green dragon"))
		|| (ch->race == race_lookup("white dragon")))
	{
		if ((ch->mana < 4) || (ch->move < 4))
			Cprintf(ch, "You're in no shape for roaring.\n\r");
		else
		{
			Cprintf(ch, "You let out an earthshaking ROAR!!\n\r");
			act("$n ROARS and your entire body quivers!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			for (vch = char_list; vch != NULL; vch = vch_next)
			{
				vch_next = vch->next;
				if (vch->in_room->area == ch->in_room->area)
					Cprintf(vch, "You hear the echoes of a dragon roar in the distance.\n\r");
			}
			ch->mana -= 4;
			ch->move -= 4;
		}
	}
	else if (ch->race == race_lookup("gargoyle"))
	{
		if ((ch->mana < 3) || (ch->move < 3))
			Cprintf(ch,"You're in no shape for roaring.\n\r");
		else
		{
			Cprintf(ch, "A menacing ROAR erupts from your throat!\n\r");
			act("$n ROARS with ferocious anger!\n\r", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			for (vch = char_list; vch != NULL; vch = vch_next)
			{
				vch_next = vch->next;
				if (vch->in_room->area == ch->in_room->area)
					Cprintf(vch,"An angry gargoyle's roar echoes through the distance.\n\r");
			}
			ch->mana -= 3;
			ch->move -= 3;
		}
	}
	else if (ch->race == race_lookup("sliver"))
	{
		Cprintf(ch, "You attempt to ROAR, but instead HISS loudly!\n\r");
		act("$n HISSES with the force of a ROAR!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	else if (ch->race == race_lookup("kirre"))
	{
		Cprintf(ch, "You ROAR loudly enough to frighten everyone nearby!\n\r");
		act("$n ROARS so loud you cover your ears!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	else if (ch->race == race_lookup("hatchling"))
	{
		Cprintf(ch, "You open your mouth to ROAR and... {*PEEP!\n\r");
		act("$n opens $s mouth to ROAR and... {*PEEP!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	else
	{
		Cprintf(ch, "You make a really dumb roaring sound.\n\r");
		act("$n makes a really dumb roaring sound in a futile attempt to sound scary.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}
}

void
do_sit(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj = NULL;

	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
	}

	if (ch->position == POS_FIGHTING)
	{
		Cprintf(ch, "Maybe you should finish this fight first?\n\r");
		return;
	}

	/* okay, now that we know we can sit, find an object to sit on */
	if (argument[0] != '\0')
	{
		obj = get_obj_list(ch, argument, ch->in_room->contents);

		if (obj == NULL)
		{
			Cprintf(ch, "You don't see that here.\n\r");
			return;
		}
	}
	else
		obj = ch->on;

	if (obj != NULL)
	{
		if (!IS_SET(obj->item_type, ITEM_FURNITURE)
			|| (!IS_SET(obj->value[2], SIT_ON)
				&& !IS_SET(obj->value[2], SIT_IN)
				&& !IS_SET(obj->value[2], SIT_AT)))
		{
			Cprintf(ch, "You can't sit on that.\n\r");
			return;
		}

		if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
		{
			act("There's no more room on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
			return;
		}

		ch->on = obj;
	}

	switch (ch->position)
	{
	case POS_SLEEPING:
		if (IS_AFFECTED(ch, AFF_SLEEP))
		{
			Cprintf(ch, "You can't wake up!\n\r");
			return;
		}

		if (obj == NULL)
		{
			Cprintf(ch, "You wake and sit up.\n\r");
			act("$n wakes and sits up.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], SIT_AT))
		{
			act("You wake and sit at $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], SIT_ON))
		{
			act("You wake and sit on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else
		{
			act("You wake and sit in $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes and sits in $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}

		ch->position = POS_SITTING;
		break;

	case POS_RESTING:
		if (obj == NULL)
			Cprintf(ch, "You stop resting.\n\r");
		else if (IS_SET(obj->value[2], SIT_AT))
		{
			act("You sit at $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n sits at $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], SIT_ON))
		{
			act("You sit on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n sits on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		ch->position = POS_SITTING;
		break;

	case POS_SITTING:
		Cprintf(ch, "You are already sitting down.\n\r");
		break;

	case POS_STANDING:
		if (obj == NULL)
		{
			Cprintf(ch, "You sit down.\n\r");
			act("$n sits down on the ground.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], SIT_AT))
		{
			act("You sit down at $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n sits down at $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else if (IS_SET(obj->value[2], SIT_ON))
		{
			act("You sit on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n sits on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		else
		{
			act("You sit down in $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n sits down in $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		}
		ch->position = POS_SITTING;
		break;
	}

	return;
}


void
do_sleep(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj = NULL;

	if (is_affected(ch, gsn_nightmares)) {
		Cprintf(ch, "You barely close your eyes before the nightmares begin to torment you!!\n\r");
		damage(ch, ch, dice(ch->level, 6), gsn_nightmares, DAM_MENTAL, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);
		return;
	}

	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
	}

	switch (ch->position)
	{
	case POS_SLEEPING:
		Cprintf(ch, "You are already sleeping.\n\r");
		break;

	case POS_RESTING:
	case POS_SITTING:
	case POS_STANDING:
		if (argument[0] == '\0' && ch->on == NULL)
		{
			Cprintf(ch, "You go to sleep.\n\r");
			act("$n goes to sleep.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			ch->position = POS_SLEEPING;
		}
		else
			/* find an object and sleep on it */
		{
			if (argument[0] == '\0')
				obj = ch->on;
			else
				obj = get_obj_list(ch, argument, ch->in_room->contents);

			if (obj == NULL)
			{
				Cprintf(ch, "You don't see that here.\n\r");
				return;
			}
			if (obj->item_type != ITEM_FURNITURE
				|| (!IS_SET(obj->value[2], SLEEP_ON)
					&& !IS_SET(obj->value[2], SLEEP_IN)
					&& !IS_SET(obj->value[2], SLEEP_AT)))
			{
				Cprintf(ch, "You can't sleep on that!\n\r");
				return;
			}

			if (ch->on != obj && count_users(obj) >= obj->value[0])
			{
				act("There is no room on $p for you.", ch, obj, NULL, TO_CHAR, POS_DEAD);
				return;
			}

			ch->on = obj;
			if (IS_SET(obj->value[2], SLEEP_AT))
			{
				act("You go to sleep at $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				act("$n goes to sleep at $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			}
			else if (IS_SET(obj->value[2], SLEEP_ON))
			{
				act("You go to sleep on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				act("$n goes to sleep on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			}
			else
			{
				act("You go to sleep in $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				act("$n goes to sleep in $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			}
			ch->position = POS_SLEEPING;
		}
		break;

	case POS_FIGHTING:
		Cprintf(ch, "You are already fighting!\n\r");
		break;
	}

	return;
}


void
do_wake(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
	}

	one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		do_stand(ch, argument);
		return;
	}

	if (!IS_AWAKE(ch))
	{
		Cprintf(ch, "You are asleep yourself!\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_AWAKE(victim))
	{
		act("$N is already awake.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (IS_AFFECTED(victim, AFF_SLEEP))
	{
		act("You can't wake $M!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	act("$n wakes you.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
	do_stand(victim, "");
	return;
}


void
do_sneak(CHAR_DATA * ch, char *argument)
{
	AFFECT_DATA af;

	Cprintf(ch, "You attempt to move silently.\n\r");
	affect_strip(ch, gsn_sneak);

	if (number_percent() < get_skill(ch, gsn_sneak))
	{
		check_improve(ch, gsn_sneak, TRUE, 3);
		af.where = TO_AFFECTS;
		af.type = gsn_sneak;
		af.level = ch->level;
		af.duration = ch->level;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_SNEAK;
		affect_to_char(ch, &af);
	}
	else
		check_improve(ch, gsn_sneak, FALSE, 3);

	return;
}


int
check_hide_area(CHAR_DATA * ch)
{
	if (ch->race == race_lookup("black dragon"))
		return TRUE;

	if ((ch->race == race_lookup("elf")
		 || ch->charClass == class_lookup("ranger")
		 || ch->charClass == class_lookup("druid"))
		&& (ch->in_room->sector_type == SECT_FIELD
			|| ch->in_room->sector_type == SECT_FOREST
			|| ch->in_room->sector_type == SECT_HILLS
			|| ch->in_room->sector_type == SECT_MOUNTAIN
			|| ch->in_room->sector_type == SECT_DESERT))
		return TRUE;

	if (ch->charClass == class_lookup("thief") && ch->in_room->sector_type <= SECT_CITY)
		return TRUE;

	return FALSE;
}


void
do_hide(CHAR_DATA * ch, char *argument)
{
	if (IS_AFFECTED(ch, AFF_FAERIE_FIRE))
	{
		Cprintf(ch, "Your outline is too noticable to hide with.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		Cprintf(ch, "You are already concealed.\n\r");
		return;
	}

	if (ch->fighting)
	{
		Cprintf(ch, "You attempt to hide, but you are immediately seen.\n\r");
		return;
	}

	if (number_percent() < get_skill(ch, gsn_hide))
	{
		if (check_hide_area(ch))
		{
			SET_BIT(ch->affected_by, AFF_HIDE);
			check_improve(ch, gsn_hide, TRUE, 3);
			Cprintf(ch, "You attempt to hide.\n\r");
		}
		else if (number_percent() > 50)
		{
			SET_BIT(ch->affected_by, AFF_HIDE);
			check_improve(ch, gsn_hide, TRUE, 3);
			Cprintf(ch, "You attempt to hide.\n\r");
		}
		else
		{
			check_improve(ch, gsn_hide, FALSE, 3);
			Cprintf(ch, "You attempt to hide.\n\r");
		}
	}
	else
		check_improve(ch, gsn_hide, FALSE, 3);

	return;
}


/*
 * Contributed by Alander.
 */
void
do_visible(CHAR_DATA * ch, char *argument)
{
	affect_strip(ch, gsn_invisibility);
	affect_strip(ch, gsn_mass_invis);
	affect_strip(ch, gsn_sneak);
	affect_strip(ch, gsn_oculary);
	REMOVE_BIT(ch->affected_by, AFF_HIDE);
	REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
	REMOVE_BIT(ch->affected_by, AFF_SNEAK);
	Cprintf(ch, "You are now visible.\n\r");
	return;
}


void
do_recall(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	ROOM_INDEX_DATA *location;
	OBJ_DATA *crystal;
	int RECALL_ROOM, chance;
	AFFECT_DATA *paf;
	int gulliver_keep_mv = 0;

	if (is_affected(ch, gsn_gullivers_travel))
		gulliver_keep_mv = 1;

	if (ch->in_room->area->continent == 0)
		RECALL_ROOM = ROOM_VNUM_TEMPLE;
	else
		RECALL_ROOM = ROOM_VNUM_DOMINIA;

	if (ch->in_room != NULL && ch->was_in_room != NULL
		&& (ch->in_room == get_room_index(ROOM_VNUM_LIMBO)
			|| ch->in_room == get_room_index(ROOM_VNUM_LIMBO_DOMINIA)))
	{
		if (ch->was_in_room->area->continent == 0)
			RECALL_ROOM = ROOM_VNUM_TEMPLE;
		else
			RECALL_ROOM = ROOM_VNUM_DOMINIA;
	}

	/* Add-ons for gullivers travel */
	if (is_affected(ch, gsn_gullivers_travel) && !str_prefix(argument, "dominia"))
	{
		RECALL_ROOM = ROOM_VNUM_DOMINIA;
		gulliver_keep_mv = 0;
	}

	if (is_affected(ch, gsn_gullivers_travel) && !str_prefix(argument, "terra"))
	{
		RECALL_ROOM = ROOM_VNUM_TEMPLE;
		gulliver_keep_mv = 0;
	}

	/* End Add-ons */

	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
	}

	if (IS_NPC(ch) && !IS_SET(ch->act, ACT_PET))
	{
		Cprintf(ch, "Only players can recall.\n\r");
		return;
	}

	if (ch->fighting != NULL && (IS_AFFECTED(ch, AFF_TAUNT)
		|| is_affected(ch, gsn_taunt)))
	{
		paf = affect_find(ch->affected, gsn_taunt);
		if(paf != NULL)
		{
			chance = paf->modifier * 10 - 10;
			if( number_percent() < chance )
			{
				Cprintf(ch, "You are taunted and cannot leave the fight!\n\r");
				act("$n is taunted and cannot leave the fight!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
				return;
			}
		}
	}

	if(!IS_NPC(ch)
	&& ch->fighting
 	&& ch->reclass == reclass_lookup("zealot")
 	&& number_percent() < 80 - (ch->hit * 80 / MAX_HP(ch))) {
		Cprintf(ch, "In your zeal for blood, you refuse to recall!\n\r");

		act("$n prays for transportation!", ch, 0, 0, TO_ROOM, POS_RESTING);
		return;
	}

	if (ch->no_quit_timer > 0) {
		if (number_percent() > get_skill(ch, gsn_recall))
		{
			if (gulliver_keep_mv && number_percent() < 25)
			{
				gulliver_keep_mv = 0;
				Cprintf(ch, "By the grace of Taelisan, you recall!\n\r");	
			}
			else
			{
				Cprintf(ch, "You are too nervous to recall.\n\r");
				check_improve(ch, gsn_recall, FALSE, 4);
				return;
			}
		}
	}

	/* This is 'silent prayer' automatic ability not a skill */
	if(ch->reclass != reclass_lookup("templar") &&
	   ch->reclass != reclass_lookup("exorcist"))
		act("$n prays for transportation!", ch, 0, 0, TO_ROOM, POS_RESTING);

	crystal = get_eq_char(ch, WEAR_HOLD);

	if (crystal != NULL)
	{
		if (crystal->item_type == ITEM_RECALL)
		{
			RECALL_ROOM = crystal->value[0];
			if (crystal->value[1] > 1)
				crystal->value[1]--;
			else
			{
				Cprintf(ch, "%s vibrates and shatters.\n\r", crystal->short_descr);
				extract_obj(crystal);
			}
		}
	}

	if (is_affected(ch, gsn_beacon))
	{
		if (! (is_affected(ch, gsn_gullivers_travel) &&
			   (!str_prefix(argument, "dominia") ||
			    !str_prefix(argument, "terra") )
			))
		{
			AFFECT_DATA* paf;
			/* k, beacon active, and not gulivarsing */
			paf = affect_find(ch->affected, gsn_beacon);
			RECALL_ROOM = paf->modifier;
			affect_strip(ch, gsn_beacon);

		}
	}


	if ((location = get_room_index(RECALL_ROOM)) == NULL)
	{
		Cprintf(ch, "You are completely lost.\n\r");
		return;
	}

	if((paf = affect_find(location->affected, gsn_beacon)) != NULL) {
		affect_remove_room(location, paf);
	}


	if (location->area->continent != ch->in_room->area->continent
		&& !is_affected(ch, gsn_gullivers_travel))
	{
		Cprintf(ch, "You can't recall to another continent, even with crystals or beacons.\n\r");
		return;
	}

	if (ch->in_room == location)
		return;

	if (ch->in_room == NULL)
	{
		Cprintf(ch, "%s wants you to reconsider.\n\r", godName(ch));
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) || IS_AFFECTED(ch, AFF_CURSE))
	{
		Cprintf(ch, "%s has forsaken you.\n\r", godName(ch));
		return;
	}

	if(wearing_norecall_item(ch)) {
		Cprintf(ch, "Your prayer has been silenced!\n\r");
		return;
	}


	if ((victim = ch->fighting) != NULL)
	{
		int lose, skill;

		skill = get_skill(ch, gsn_recall);

		if (number_percent() > skill * 2 / 3)
		{
			check_improve(ch, gsn_recall, FALSE, 5);
			WAIT_STATE(ch, 4);
			Cprintf(ch, "You failed!.\n\r");
			return;
		}

		lose = (ch->desc != NULL) ? 25 : 50;
		gain_exp(ch, 0 - lose);
		check_improve(ch, gsn_recall, TRUE, 4);
		Cprintf(ch, "You recall from combat!  You lose %d exps.\n\r", lose);
		stop_fighting(ch, TRUE);
	}

	if (gulliver_keep_mv == 0)
	{
		ch->move /= 2;
	}

	if(ch->reclass != reclass_lookup("templar") &&
	   ch->reclass != reclass_lookup("exorcist"))
		act("$n disappears.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	char_from_room(ch);
	char_to_room(ch, location);
	if(ch->reclass != reclass_lookup("templar") &&
	   ch->reclass != reclass_lookup("exorcist"))
		act("$n appears in the room.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	do_look(ch, "auto");

	if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 66)
		Cprintf(ch, "!!SOUND(sounds/wav/recall*.wav V=80 P=20 T=admin)");

	if (ch->pet != NULL)
		do_recall(ch->pet, "");

	return;
}


void
do_train(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *mob;
	CHAR_DATA *gch;
	bool found;
	int stat = -1;
	char *pOutput = NULL;
	int slivtype;
	int cost;

	if (IS_NPC(ch))
		return;

	/*
	 * Check for trainer.
	 */
	for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
		if (IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN))
			break;

	if (mob == NULL)
	{
		Cprintf(ch, "You can't do that here.\n\r");
		return;
	}

	if (argument[0] == '\0')
	{
		Cprintf(ch, "You have %d training sessions.\n\r", ch->train);
		argument = "foo";
	}

	cost = 1;

	if (!str_prefix(argument, "strength"))
	{
		if (class_table[ch->charClass].attr_prime == STAT_STR)
			cost = 1;
		stat = STAT_STR;
		pOutput = "strength";
	}

	else if (!str_prefix(argument, "intelligence"))
	{
		if (class_table[ch->charClass].attr_prime == STAT_INT)
			cost = 1;
		stat = STAT_INT;
		pOutput = "intelligence";
	}

	else if (!str_prefix(argument, "wisdom"))
	{
		if (class_table[ch->charClass].attr_prime == STAT_WIS)
			cost = 1;
		stat = STAT_WIS;
		pOutput = "wisdom";
	}

	else if (!str_prefix(argument, "dexterity"))
	{
		if (class_table[ch->charClass].attr_prime == STAT_DEX)
			cost = 1;
		stat = STAT_DEX;
		pOutput = "dexterity";
	}

	else if (!str_prefix(argument, "constitution"))
	{
		if (class_table[ch->charClass].attr_prime == STAT_CON)
			cost = 1;
		stat = STAT_CON;
		pOutput = "constitution";
	}

	else if (!str_cmp(argument, "hp"))
		cost = 1;

	else if (!str_cmp(argument, "mana"))
		cost = 1;
	else if (!str_cmp(argument, "sliver"))
		cost = 3;
	else
	{
		strcpy(buf, "You can train:");
		if (ch->perm_stat[STAT_STR] < get_max_train(ch, STAT_STR))
			strcat(buf, " str");
		if (ch->perm_stat[STAT_INT] < get_max_train(ch, STAT_INT))
			strcat(buf, " int");
		if (ch->perm_stat[STAT_WIS] < get_max_train(ch, STAT_WIS))
			strcat(buf, " wis");
		if (ch->perm_stat[STAT_DEX] < get_max_train(ch, STAT_DEX))
			strcat(buf, " dex");
		if (ch->perm_stat[STAT_CON] < get_max_train(ch, STAT_CON))
			strcat(buf, " con");
		if (ch->race == race_lookup("sliver") && ch->level > 10)
			strcat(buf, " sliver");
		strcat(buf, " hp mana");

		if (buf[strlen(buf) - 1] != ':')
			Cprintf(ch, "%s.\n\r", buf);
		else
		{
			/*
			 * This message dedicated to Jordan ... you big stud!
			 */
			act("You have nothing left to train, you $T!",
				ch, NULL,
				ch->sex == SEX_MALE ? "big stud" :
				ch->sex == SEX_FEMALE ? "hot babe" :
				"wild thing",
				TO_CHAR,
				POS_RESTING);
		}

		return;
	}

	if (!str_cmp("sliver", argument))
	{
		if (cost > ch->train)
		{
			Cprintf(ch, "You don't have enough training sessions.\n\r");
			return;
		}

		if (ch->race != race_lookup("sliver"))
		{
			Cprintf(ch, "You have to be a sliver to train that.\n\r");
			return;
		}

		if (ch->level < 10)
		{
			Cprintf(ch, "You have to have matured before you can train that.\n\r");
			return;
		}

		found = FALSE;

		for (gch = char_list; gch != NULL; gch = gch->next)
		{
			if (is_same_group(gch, ch) && ch != gch)
				found = TRUE;
		}

		if (found == TRUE)
		{
			Cprintf(ch, "You can't be grouped with anyone when training sliver.\n\r");
			return;
		}

		ch->train -= cost;
		slivtype = ch->sliver;
		while (slivtype == ch->sliver)
		{
			ch->sliver = number_range(1, 25);

			if (ch->sliver < 1)
				ch->sliver = 1;

			if (ch->sliver > 25)
				ch->sliver = 25;
		}

		act("$n morphs into a new kind of sliver.!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You morph into a %s sliver!\n\r", sliver_table[ch->sliver].name);
		return;
	}

	if (!str_cmp("hp", argument))
	{
		if (cost > ch->train)
		{
			Cprintf(ch, "You don't have enough training sessions.\n\r");
			return;
		}

		ch->train -= cost;
		ch->max_hit += 10;
		ch->hit += 10;
		act("Your durability increases!", ch, NULL, NULL, TO_CHAR, POS_RESTING);
		act("$n's durability increases!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	if (!str_cmp("mana", argument))
	{
		if (cost > ch->train)
		{
			Cprintf(ch, "You don't have enough training sessions.\n\r");
			return;
		}

		ch->train -= cost;
		ch->max_mana += 10;
		ch->mana += 10;
		act("Your power increases!", ch, NULL, NULL, TO_CHAR, POS_RESTING);
		act("$n's power increases!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	if (ch->perm_stat[stat] >= get_max_train(ch, stat))
	{
		act("Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR, POS_RESTING);
		return;
	}

	if (cost > ch->train)
	{
		Cprintf(ch, "You don't have enough training sessions.\n\r");
		return;
	}

	ch->train -= cost;

	ch->perm_stat[stat] += 1;
	act("Your $T increases!", ch, NULL, pOutput, TO_CHAR, POS_RESTING);
	act("$n's $T increases!", ch, NULL, pOutput, TO_ROOM, POS_RESTING);
	return;
}

void do_jump(CHAR_DATA *ch, char *argument) {
	int chance, direction;
	char *description;
	AFFECT_DATA af;

	if((chance = get_skill(ch, gsn_jump)) < 1) {
		Cprintf(ch, "You can barely skip rope let alone jump that far.\n\r");
		return;
	}

	if(ch->fighting != NULL) {
		Cprintf(ch, "You don't have room to jump away right now!\n\r");
		return;
	}

	if(!str_cmp(argument,"n") || !str_cmp(argument,"north"))  {
		direction = DIR_NORTH; description = "north"; }
	else if(!str_cmp(argument,"s") || !str_cmp(argument,"south"))  {
		direction = DIR_SOUTH; description = "south"; }
	else if(!str_cmp(argument,"e") || !str_cmp(argument,"east"))   {
		direction = DIR_EAST; description = "east";   }
	else if(!str_cmp(argument,"w") || !str_cmp(argument,"west"))   {
		direction = DIR_WEST; description = "west";   }
	else if(!str_cmp(argument,"u") || !str_cmp(argument,"up"))     {
		direction = DIR_UP; description = "sky";      }
	else if(!str_cmp(argument,"d") || !str_cmp(argument,"down"))   {
		direction = DIR_DOWN; description = "down";   }
	else {
		Cprintf(ch, "There is no direction like that in this game.\n\r");
		return;
	}

	if(ch->move < 5) {
		Cprintf(ch, "You're too tired to jump that far right now.\n\r");
		return;
	}

	// Make sure there's a landing point!
	if(ch->in_room->exit[direction] == NULL
	|| ch->in_room->exit[direction]->u1.to_room == NULL
	|| ch->in_room->exit[direction]->u1.to_room->exit[direction] == NULL
	|| ch->in_room->exit[direction]->u1.to_room->exit[direction]->u1.to_room == NULL) {
		Cprintf(ch, "There's no safe place to land in that direction.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = gsn_jump;
	af.level = ch->level;
	af.duration = 1;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = 0;

	if(number_percent() < chance) {
		Cprintf(ch, "With a short sprint you leap high into the air!\n\r");
		affect_to_char(ch, &af);
		move_char(ch, direction, TRUE);
		act("$n jumps clear across the room!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "\n\r");
		move_char(ch, direction, TRUE);
		act("$n gracefully lands from $s jump.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You land gracefully from your jump.\n\r");
		ch->move -= 5;
		affect_strip(ch, gsn_jump);
		WAIT_STATE(ch, PULSE_VIOLENCE / 3);
		check_improve(ch, gsn_jump, TRUE, 3);
		return;
	}
	Cprintf(ch, "You stumble and fall on your face.\n\r");
	check_improve(ch, gsn_jump, FALSE, 3);
}

void trap_needle(CHAR_DATA *victim) {
	AFFECT_DATA *paf, af;

	/* Find the right affect */
	paf = affect_find(victim->in_room->affected, gsn_ensnare);

	if(paf != NULL
	&& paf->modifier == 1
	&& !is_affected(victim, gsn_ensnare)
	&& victim->level < paf->level + 8
	&& victim->level > paf->level - 8
	&& !IS_NPC(victim)
	&& is_clan(victim)) {
		/* Thieves can avoid traps somewhat */
		if(victim->charClass == class_lookup("thief")
		&& number_percent() < victim->level + 40) {
			Cprintf(victim, "You recognize and avoid a poison needle trap in this room.\n\r");
			return;
		}
		if(number_percent() < 20)
			return;

		Cprintf(victim, "Oops! You've stumbled into a trap!\n\rYou feel the pin prick of a poison needle!\n\r");
		act("$n has been ensnared by a trap!", victim, NULL, NULL, TO_NOTVICT, POS_RESTING);
		af.where     = TO_AFFECTS;
        	af.type      = gsn_ensnare;
        	af.level     = paf->level;
        	af.duration  = 0;
        	af.modifier  = paf->modifier;
        	af.location  = APPLY_NONE;
        	af.bitvector = 0;
		affect_to_char(victim, &af);
		affect_remove_room(victim->in_room, paf);
	}
}

void trap_darts(CHAR_DATA *victim) {
	AFFECT_DATA *paf;
	int i;

	/* Find the right dart affect */
	paf = affect_find(victim->in_room->affected, gsn_ensnare);

	if(paf != NULL
	&& paf->modifier == 2
	&& victim->level < paf->level + 8
	&& victim->level > paf->level - 8
	&& !IS_NPC(victim)
	&& is_clan(victim)) {
		/* Thieves can avoid traps somewhat */
		if(victim->charClass == class_lookup("thief")
		&& number_percent() < victim->level + 40) {
			Cprintf(victim, "You recognize and avoid a dart trap in this room.\n\r");
			return;
		}
		if(number_percent() < 20)
			return;
		Cprintf(victim, "Oops! You've stumbled into a trap!\n\rA barrage of darts hits you!\n\r");
		act("$n has been ensnared by a trap!", victim, NULL, NULL, TO_NOTVICT, POS_RESTING);
		affect_remove_room(victim->in_room, paf);
		for(i=0;i<8;i++) {
			if(number_percent() > 50) {
				damage(victim, victim, number_range(1, paf->level), gsn_ensnare, DAM_PIERCE, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);
			}
		}
	}
}

void trap_wire(CHAR_DATA *victim) {
	AFFECT_DATA *paf;

	/* Find the right dart affect */
	paf = affect_find(victim->in_room->affected, gsn_ensnare);

	if(paf != NULL
	&& paf->modifier == 3
	&& victim->level < paf->level + 8
	&& victim->level > paf->level - 8
	&& !IS_NPC(victim)
	&& is_clan(victim)) {
		/* Thieves can avoid traps somewhat */
		if(victim->charClass == class_lookup("thief")
		&& number_percent() < victim->level + 40) {
			Cprintf(victim, "You recognize and avoid a trip wire in this room.\n\r");
			return;
		}
		if(number_percent() < 20)
			return;

		Cprintf(victim, "Oops! You've stumbled into a trap!\n\rYou stumble and trip on a hidden wire!\n\r");
		act("$n has been ensnared by a trap!", victim, NULL, NULL, TO_NOTVICT, POS_RESTING);
		WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
		affect_remove_room(victim->in_room, paf);
	}
}

void do_ensnare(CHAR_DATA *ch, char *argument)
{
	AFFECT_DATA af;
	int traptype, chance;
	int i;

	if (room_is_affected (ch->in_room, gsn_ensnare))
	{
		Cprintf(ch, "This room is already trapped.\n\r");
		return;
	}

	traptype = number_range(1,3);
	af.where     = TO_AFFECTS;
	af.type      = gsn_ensnare;
	af.level     = ch->level;
	af.duration  = ch->level / 10;
	af.modifier  = traptype;
	af.location  = APPLY_NONE;
	af.bitvector = 0;

   	if (get_skill(ch, gsn_ensnare) < 1)
   	{
		Cprintf(ch, "You have no idea how to set traps.\n\r");
      		return;
   	}

	chance = ( get_skill(ch, gsn_ensnare) * 3/4 );

	if(chance < 1)
		chance = 1;

	if(ch->move < 50)
	{
		Cprintf(ch, "You are too weary to set up any traps right now.\n\r");
		return;
	}

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

	/* Badness if you fail */
	if(number_percent() > chance)
	{
		ch->move = ch->move * 3/4;
		check_improve(ch, gsn_ensnare, FALSE, 1);
		switch(traptype)
		{
		case 1:
			Cprintf(ch, "You accidently poke yourself!\n\r");
			af.duration = 0;
			if(!is_affected(ch, gsn_ensnare))
				affect_to_char(ch, &af);
			return;
		case 2:
			Cprintf(ch, "The darts fire before you're ready!\n\r");
			for(i=0;i<8;i++) {
				if(number_percent() > 50) {
					damage(ch, ch, number_range(1, af.level), gsn_ensnare, DAM_PIERCE, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);
				}
			}
			return;
		case 3:
			Cprintf(ch, "The trip wire snaps.\n\r");
			return;
		}
	}
	/* all good */
	switch(traptype)
	{
	case 1:
		Cprintf(ch, "You set up a conceiled poison needle trap.\n\r"); break;
	case 2:
		Cprintf(ch, "You set up a trap to fire a barrage of darts.\n\r"); break;
	case 3:
		Cprintf(ch, "You set up a conceiled trip wire to take someone down.\n\r"); break;
	default:
		Cprintf(ch, "Invalid trap set.\n\r"); return;
	}
	ch->move -= 50;
	check_improve(ch, gsn_ensnare, TRUE, 2);
	affect_to_room(ch->in_room, &af);
	return;
}

void fire_rune(CHAR_DATA *victim)
{
        AFFECT_DATA *paf;
	AFFECT_DATA af;
        int dam=0;

	paf = affect_find(victim->in_room->affected, gsn_fire_rune);

	if(is_affected(victim, gsn_fire_rune))
	{
		return;
	}

        if(paf != NULL
           && victim->level < paf->level + 8
           && victim->level > paf->level - 8
           && is_clan(victim))
	{
                /* Runists have chance of recognizing runes */
                if(victim->charClass == class_lookup("runist")
                && number_percent() < victim->level + 40) {
                        Cprintf(victim, "You sense the power of the room's rune and avoid it!\n\r");
                        return;
        	}

		if(victim->pktimer > 0) {
                	Cprintf(victim, "A fire glows menacingly, but you just died.\n\r");
                	return;
        	}

                if(number_percent() < 20)
                        return;

                Cprintf(victim, "You are engulfed in flames as you enter the room!\n\r");
                act("$n has been engulfed in flames!", victim, NULL, NULL, TO_NOTVICT, POS_RESTING);

	        af.where = TO_AFFECTS;
       		af.type = gsn_fire_rune;
       		af.level = victim->level;
      		af.duration = 0;
     		af.modifier = 0;
    		af.location = APPLY_NONE;
   		af.bitvector = 0;

		if(!is_affected(victim, gsn_fire_rune))
		{
			affect_to_char(victim, &af);
		}

		dam = number_range(victim->level, victim->level * 3);
		damage(victim, victim, dam, gsn_fire_rune, DAM_FIRE, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);
		fire_effect(victim->in_room, victim->level, dam, TARGET_ROOM);
		fire_effect(victim, victim->level, dam, TARGET_CHAR);
       }
}

void shackle_rune(CHAR_DATA *victim)
{
	AFFECT_DATA *paf;
	AFFECT_DATA af;

	paf = affect_find(victim->in_room->affected, gsn_shackle_rune);

	if(is_affected(victim, gsn_shackle_rune))
	{
		return;
	}

	if(paf != NULL
	   && victim->level < paf->level + 8
	   && victim->level > paf->level - 8
	   && is_clan(victim))
	{

	        /* Runists have chance of recognizing runes */
		if(victim->charClass == class_lookup("runist")
                   && number_percent() < victim->level + 40)
		{
                        Cprintf(victim, "You sense the power of the room's rune and avoid it!\n\r");
                        return;
	        }

                if(number_percent() < 20
		|| victim->pktimer > 0)
                        return;

                Cprintf(victim, "You are bound and restricted by runic shackles!\n\r");
                act("$n has been bound by runic shackles!", victim, NULL, NULL, TO_NOTVICT, POS_RESTING);

                af.where = TO_AFFECTS;
                af.type = gsn_shackle_rune;
                af.level = victim->level;
                af.duration = 0;
                af.modifier = 0;
                af.location = APPLY_NONE;
                af.bitvector = 0;

                if(!is_affected(victim, gsn_shackle_rune))
                {
                        affect_to_char(victim, &af);
                }

		victim->move = (victim->move - number_range(1, 12));
		victim->move = UMAX(victim->move, 0);
	}
}
