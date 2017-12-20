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
 *        ROM 2.4 is copyright 1993-1996 Russ Taylor                       *
 *        ROM has been brought to you by the ROM consortium                *
 *            Russ Taylor (rtaylor@pacinfo.com)                            *
 *            Gabrielle Taylor (gtaylor@pacinfo.com)                       *
 *            Brian Moore (rom@rom.efn.org)                                *
 *        By using this code, you have agreed to follow the terms of the   *
 *        ROM license, in the file Rom24/doc/rom.license                   *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "deity.h"
#include "flags.h"
#include "utils.h"


#define SPELL_DAMAGE_LOW	0
#define SPELL_DAMAGE_MEDIUM 1
#define SPELL_DAMAGE_HIGH   2

DECLARE_DO_FUN(do_kill);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_follow);
DECLARE_DO_FUN(do_autoassist);
DECLARE_DO_FUN(do_granite_stare);

bool max_no_charmies(CHAR_DATA* ch);
void raw_kill(CHAR_DATA * victim);
void size_mob(CHAR_DATA * ch, CHAR_DATA * victim, int level);
void size_obj(CHAR_DATA * ch, OBJ_DATA * obj, int level);
void death_cry(CHAR_DATA * ch);
bool remove_obj(CHAR_DATA * ch, int iWear, bool fReplace);
void wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace);
void obj_to_char(OBJ_DATA * obj, CHAR_DATA * ch);
void obj_from_char(OBJ_DATA* obj);
int get_random_bit(unsigned int bits);
void meteor_blast(CHAR_DATA *ch, ROOM_INDEX_DATA *room, int level);
bool in_own_hall(CHAR_DATA* ch);
bool in_enemy_hall(CHAR_DATA* ch);

extern char *target_name;

// external functions
extern int generate_int(unsigned char, unsigned char);
extern int get_caster_level(int);
extern int get_modified_level(int);
extern int saving_throw(CHAR_DATA*, CHAR_DATA*, int sn, int level, int diff, int stat, int damtype);
extern int spell_damage(CHAR_DATA*, CHAR_DATA*, int level, int type, int resist);
extern int hit_lookup(char *hit);
extern void start_scan(CHAR_DATA*ch, CHAR_DATA* victim, char*);
extern void add_hit_dam(CHAR_DATA *ch, OBJ_DATA *obj, int add_hit, int add_dam);

void
spell_farsight(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = NULL;
	int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (IS_AFFECTED(ch, AFF_BLIND))
        {
                Cprintf(ch, "Maybe it would help if you could see?\n\r");
                return;
        }

	if((victim = get_char_world(ch, target_name, FALSE)) == NULL) {
		Cprintf(ch, "You can't find them.\n\r");
		return;
	}

	if(victim != ch
	&& !IS_NPC(victim)) {
		Cprintf(ch, "You can't spy on players directly.\n\r");
		return;
	}


	if (victim != ch
	&& saving_throw(ch, victim, sn, modified_level + 2, SAVE_NORMAL, STAT_INT, DAM_NONE)) {
		Cprintf(ch, "You failed.\n\r");
		return;
	}

        start_scan(ch, victim, "");
        start_scan(ch, victim, "n");
        start_scan(ch, victim, "s");
        start_scan(ch, victim, "e");
        start_scan(ch, victim, "w");
        start_scan(ch, victim, "u");
        start_scan(ch, victim, "d");

	return;
}

void
spell_portal(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *portal, *stone;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if ((victim = get_char_world(ch, target_name, TRUE)) == NULL
                || victim == ch
                || victim->in_room == NULL
                || !can_see_room(ch, victim->in_room)
		|| ch->in_room->clan
		|| victim->in_room->clan
                || IS_SET(victim->in_room->room_flags, ROOM_SAFE)
                || IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
                || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
                || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
                || IS_SET(victim->in_room->room_flags, ROOM_LAW)
                || IS_SET(victim->in_room->room_flags, ROOM_NO_GATE)
		|| wearing_nogate_item(ch)
                || IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
                || victim->level >= level + 3
                || (is_clan(victim) && !is_same_clan(ch, victim))
                || (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
                || (IS_NPC(victim) && IS_AFFECTED(victim, AFF_CHARM) && victim->
master != ch)
                || (IS_NPC(victim)
				&& number_percent() < 20))

        {
                Cprintf(ch, "You failed.\n\r");
                return;
        }

	if (victim->in_room->area->security < 9)
	{
		Cprintf(ch, "Not in unfinished areas.  Sorry.\n\r");
		return;
	}

	stone = get_eq_char(ch, WEAR_HOLD);
	if (!IS_IMMORTAL(ch)
		&& (stone == NULL || stone->item_type != ITEM_WARP_STONE))
	{
		Cprintf(ch, "You lack the proper component for this spell.\n\r");
		return;
	}

	if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
	{
		act("You draw upon the power of $p.", ch, stone, NULL, TO_CHAR, POS_RESTING);
		act("It flares brightly and vanishes!", ch, stone, NULL, TO_CHAR, POS_RESTING);
		extract_obj(stone);
	}

	portal = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
	portal->timer = dice(modified_level/16, modified_level/16);
	portal->value[3] = victim->in_room->vnum;

	obj_to_room(portal, ch->in_room);

	act("$p rises up from the ground.", ch, portal, NULL, TO_ROOM, POS_RESTING);
	act("$p rises up before you.", ch, portal, NULL, TO_CHAR, POS_RESTING);
}

void
spell_nexus(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *portal, *stone;
	ROOM_INDEX_DATA *to_room, *from_room;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	from_room = ch->in_room;

	if ((victim = get_char_world(ch, target_name, TRUE)) == NULL
		|| victim == ch
		|| (to_room = victim->in_room) == NULL
		|| !can_see_room(ch, to_room) || !can_see_room(ch, from_room)
		|| to_room->clan || from_room->clan
		|| IS_SET(to_room->room_flags, ROOM_SAFE)
		|| IS_SET(from_room->room_flags, ROOM_SAFE)
		|| IS_SET(to_room->room_flags, ROOM_PRIVATE)
		|| IS_SET(to_room->room_flags, ROOM_SOLITARY)
		|| IS_SET(to_room->room_flags, ROOM_NO_GATE)
		|| wearing_nogate_item(ch)
		|| IS_SET(to_room->room_flags, ROOM_NO_RECALL)
		|| IS_SET(from_room->room_flags, ROOM_NO_RECALL)
		|| IS_SET(to_room->room_flags, ROOM_LAW)
		|| victim->level >= level + 3
		|| (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
		|| (IS_NPC(victim)
		&& number_percent() < 20)
		|| (is_clan(victim) && !is_same_clan(ch, victim)))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (victim->in_room->area->security < 9)
	{
		Cprintf(ch, "Not in unfinished areas.  Sorry.\n\r");
		return;
	}

	stone = get_eq_char(ch, WEAR_HOLD);
	if (!IS_IMMORTAL(ch)
		&& (stone == NULL || stone->item_type != ITEM_WARP_STONE))
	{
		Cprintf(ch, "You lack the proper component for this spell.\n\r");
		return;
	}

	if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
	{
		act("You draw upon the power of $p.", ch, stone, NULL, TO_CHAR, POS_RESTING);
		act("It flares brightly and vanishes!", ch, stone, NULL, TO_CHAR, POS_RESTING);
		extract_obj(stone);
	}

	/* portal one */
	portal = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
	portal->timer = modified_level / 4;
	portal->value[3] = to_room->vnum;

	obj_to_room(portal, from_room);

	act("$p rises up from the ground.", ch, portal, NULL, TO_ROOM, POS_RESTING);
	act("$p rises up before you.", ch, portal, NULL, TO_CHAR, POS_RESTING);

	/* no second portal if rooms are the same */
	if (to_room == from_room)
		return;

	/* portal two */
	portal = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
	portal->timer = modified_level / 4;
	portal->value[3] = from_room->vnum;

	obj_to_room(portal, to_room);

	if (to_room->people != NULL)
	{
		act("$p rises up from the ground.", to_room->people, portal, NULL, TO_ROOM, POS_RESTING);
		act("$p rises up from the ground.", to_room->people, portal, NULL, TO_CHAR, POS_RESTING);
	}
}


/* demonology spells */
void
do_pentagram(CHAR_DATA * ch, char *argument)
{
	int chance;
	AFFECT_DATA af;

	if (room_is_affected(ch->in_room, gsn_pentagram))
	{
		Cprintf(ch, "There is already a pentagram on the floor.\n\r");
		return;
	}

	if (ch->fighting)
	{
		Cprintf(ch, "You're much too busy to be drawing pentagrams.\n\r");
		return;
	}

	if ((chance = get_skill(ch, gsn_pentagram)) < 1)
	{
		Cprintf(ch, "You have no idea how to draw pentagrams.\n\r");
		return;
	}

	chance -= 40;
	chance += get_curr_stat(ch, STAT_INT);

	Cprintf(ch, "You scribe a chalk pentagram on the floor.\n\r");
	act("$n has created a pentagram.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	af.where = TO_AFFECTS;
	af.type = gsn_pentagram;
	af.level = ch->level;
	af.duration = ch->level / 3;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = 0;

	if (number_percent() < chance)
	{
		// Good pentagram (true)
		af.modifier = 1;
		check_improve(ch, gsn_pentagram, TRUE, 2);
	}
	else
	{
		// Bad pentagram (false)
		af.modifier = 0;
		chance = get_curr_stat(ch, STAT_INT);
		if (number_percent() < chance)
		{
			Cprintf(ch, "You notice a smudge in your pentagram.\n\r");
		}
		check_improve(ch, gsn_pentagram, FALSE, 2);
	}

	affect_to_room(ch->in_room, &af);
	WAIT_STATE(ch, skill_table[gsn_pentagram].beats);

	return;
}

void
spell_summon_lesser(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = NULL;
	CHAR_DATA *master = NULL;
	AFFECT_DATA af, *paf = NULL;
	int lesser_count;
	int type;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	lesser_count = 0;
	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->master == ch && IS_NPC(victim)
		&& is_affected(victim, gsn_summon_lesser))
			lesser_count++;
	}

	if (lesser_count >= 8)
	{
		Cprintf(ch, "You can't call more lesser demons.\n\r");
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	Cprintf(ch, "You call out to the lesser demons of the planes of hell!\n\r");
	act("$n creates a shimmering black gate.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	type = dice(1, 6);
	switch (type)
	{
	case 1:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_MANES));
		break;
	case 2:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_SPINAGON));
		break;
	case 3:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_BARLGURA));
		break;
	case 4:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_SUCCUBUS));
		break;
	case 5:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_ABISHAI));
		break;
	case 6:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_OSYLUTH));
		break;
	}
	act("$N appears through your gate.", ch, victim, victim, TO_CHAR, POS_RESTING);
	act("$N appears through the gate.", ch, victim, victim, TO_ROOM, POS_RESTING);
	char_to_room(victim, ch->in_room);

	// We have to handle the case of a demon summoning another demon. Make sure
	// it gets handled properly!
	SET_BIT(victim->toggles, TOGGLES_NOEXP);
	IS_NPC(ch) && ch->master ? (master = ch->master) : (master = ch);

	paf = affect_find(ch->in_room->affected, gsn_pentagram);
	if(paf == NULL) {
		act("$N looks around and notices a lack of pentagram.",
			master, victim, victim, TO_ALL, POS_RESTING);
		do_kill(victim, master->name);
		return;
	}
	if(paf != NULL
	&& number_percent() < 10
	&& paf->modifier == 0) {
		act("$N sees a small flaw in your pentagram.", master, victim, victim, TO_CHAR, POS_RESTING);
		act("$N sees a small flaw in the pentagram.", master, victim, victim, TO_ROOM, POS_RESTING);
		do_kill(victim, master->name);
		return;
	}

	if (victim->master)
		stop_follower(victim);
	add_follower(victim, master);
	victim->leader = master;

	af.where = TO_AFFECTS;
	af.type = gsn_summon_lesser;
	af.level = modified_level;
	af.duration = modified_level / 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(victim, &af);

	return;
}

void
spell_summon_horde(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = NULL;
	CHAR_DATA *master = NULL;
	AFFECT_DATA af, *paf = NULL;
	int lesser_count=0, demon_count=0;
	int type, i=0;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->master == ch && IS_NPC(victim)
		&& is_affected(victim, gsn_summon_lesser))
			lesser_count++;
	}

	if (lesser_count >= 8)
	{
		Cprintf(ch, "You can't call more lesser demons.\n\r");
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	// We have to handle the case of a demon summoning another demon. Make sure
	// it gets handled properly!
	Cprintf(ch, "You call out to the demonic hordes of the planes of hell!\n\r");
	act("$n creates a shimmering black gate.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	IS_NPC(ch) && ch->master ? (master = ch->master) : (master = ch);
	demon_count = dice(2, 3);

	for(i=0;i<demon_count;i++) {
		type = dice(1, 6);
		switch (type)
		{
		case 1:
			victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_MANES));
			break;
		case 2:
			victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_SPINAGON));
			break;
		case 3:
			victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_BARLGURA));
			break;
		case 4:
			victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_SUCCUBUS));
			break;
		case 5:
			victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_ABISHAI));
			break;
		case 6:
			victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_OSYLUTH));
			break;
		}
		act("$N appears through your gate.", ch, victim, victim, TO_CHAR, POS_RESTING);
		act("$N appears through the gate.", ch, victim, victim, TO_ROOM, POS_RESTING);
		SET_BIT(victim->toggles, TOGGLES_NOEXP);
		char_to_room(victim, ch->in_room);

		paf = affect_find(ch->in_room->affected, gsn_pentagram);
		if(paf == NULL) {
			act("$N looks around and notices a lack of pentagram.",
				master, victim, victim, TO_ALL, POS_RESTING);
			do_kill(victim, master->name);
			continue;
		}
		if(paf != NULL
		&& number_percent() < 10
		&& paf->modifier == 0) {
			act("$N sees a small flaw in your pentagram.", master, victim, victim, TO_CHAR, POS_RESTING);
			act("$N sees a small flaw in the pentagram.", master, victim, victim, TO_ROOM, POS_RESTING);
			do_kill(victim, master->name);
			continue;
		}
		if (victim->master)
                	stop_follower(victim);
        	add_follower(victim, master);
        	victim->leader = master;

        	af.where = TO_AFFECTS;
        	af.type = gsn_summon_lesser;
        	af.level = modified_level;
        	af.duration = modified_level / 2;
        	af.location = 0;
        	af.modifier = 0;
        	af.bitvector = AFF_CHARM;
        	affect_to_char(victim, &af);
	}

	return;
}

void
spell_summon_greater(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = NULL;
	CHAR_DATA *master = NULL;
	AFFECT_DATA af, *paf = NULL;
	int greater_count;
	int type;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	greater_count = 0;
	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->master == ch && IS_NPC(victim)
		&& is_affected(victim, gsn_summon_greater))
			greater_count++;
	}

	if (greater_count >= 3)
	{
		Cprintf(ch, "You can't call more greater demons.\n\r");
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	Cprintf(ch, "You call out to the greater demons of the planes of hell!\n\r");
	act("$n creates a shimmering black gate.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	type = dice(1, 6);
	switch (type)
	{
	case 1:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_CORNUGON));
		break;
	case 2:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_GLABREZU));
		break;
	case 3:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_VROCK));
		break;
	case 4:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_GELUGON));
		break;
	case 5:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_BALOR));
		break;
	case 6:
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_PITFIEND));
		break;
	}

	act("$N appears through your gate.", ch, victim, victim, TO_CHAR, POS_RESTING);
	act("$N appears through the gate.", ch, victim, victim, TO_ROOM, POS_RESTING);
	SET_BIT(victim->toggles, TOGGLES_NOEXP);
	char_to_room(victim, ch->in_room);

	// We have to handle the case of a demon summoning another demon. Make sure
	// it gets handled properly!
	IS_NPC(ch) && ch->master ? (master = ch->master) : (master = ch);

	paf = affect_find(ch->in_room->affected, gsn_pentagram);
	if(paf == NULL) {
		act("$N looks around and notices a lack of pentagram.",
			master, victim, victim, TO_ALL, POS_RESTING);
		do_kill(victim, master->name);
		return;
	}
	if(paf != NULL
	&& number_percent() < 30
	&& paf->modifier == 0) {
		act("$N sees a small flaw in your pentagram.", master, victim, victim, TO_CHAR, POS_RESTING);
		act("$N sees a small flaw in the pentagram.", master, victim, victim, TO_ROOM, POS_RESTING);
		do_kill(victim, master->name);
		return;
	}

	if (victim->master)
		stop_follower(victim);
	add_follower(victim, master);
	victim->leader = master;

	af.where = TO_AFFECTS;
	af.type = gsn_summon_greater;
	af.level = modified_level;
	af.duration = modified_level / 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(victim, &af);

	return;
}

void
spell_summon_lord(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = NULL, *master = NULL;
	AFFECT_DATA af, *paf;
	char name[MAX_STRING_LENGTH];
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->master == ch && IS_NPC(victim)
		&& is_affected(victim, gsn_summon_lord)) {
			Cprintf(ch, "Don't you think one demon lord is enough to control?\n\r");
			return;
		}
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	Cprintf(ch, "You call out to the demon lords of the lower planes!\n\r");
	act("$n creates a shimmering black gate.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	target_name = one_argument(target_name, name);

	if (str_cmp(name, "Juiblex") == 0)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_JUIBLEX));
	}
	else if (str_cmp(name, "Beelzebub") == 0)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_BEELZEBUB));
	}
	else if (str_cmp(name, "Orcus") == 0)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_ORCUS));
	}
	else if (str_cmp(name, "Mephistophilis") == 0)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_MEPHISTOPHILIS));
	}
	else if (str_cmp(name, "Demogorgon") == 0)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_DEMOGORGON));
	}
	else if (str_cmp(name, "Asmodeus") == 0)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_DEMON_ASMODEUS));
	}
	else
	{
		Cprintf(ch, "No demons of that name respond to your call.\n\r");
		Cprintf(ch, "After a few moments your gate collapses in on itself.\n\r");
		act("A shimmering black gate collapses inward.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	act("$N appears through your gate.", ch, victim, victim, TO_CHAR, POS_RESTING);
	act("$N appears through the gate.", ch, victim, victim, TO_ROOM, POS_RESTING);
	SET_BIT(victim->toggles, TOGGLES_NOEXP);
	char_to_room(victim, ch->in_room);

	// We have to handle the case of a demon summoning another demon. Make sure
	// it gets handled properly!
	IS_NPC(ch) && ch->master ? (master = ch->master) : (master = ch);

	paf = affect_find(ch->in_room->affected, gsn_pentagram);
	if(paf == NULL) {
		act("$N looks around and notices a lack of pentagram.",
			master, victim, victim, TO_ALL, POS_RESTING);
		do_kill(victim, master->name);
		return;
	}
	if(paf != NULL
	&& paf->modifier == 0) {
		act("$N sees a small flaw in your pentagram.", master, victim, victim, TO_CHAR, POS_RESTING);
		act("$N sees a small flaw in the pentagram.", master, victim, victim, TO_ROOM, POS_RESTING);
		do_kill(victim, master->name);
		return;
	}
	if(paf != NULL
	&& number_percent() <= 5
	&& paf->modifier == 1) {
		act("$N ignores your pitiful pentagram and attacks!", master, victim, victim, TO_CHAR, POS_RESTING);
		do_kill(victim, master->name);
		return;
	}

	if (victim->master)
		stop_follower(victim);
	add_follower(victim, master);
	victim->leader = master;

	af.where = TO_AFFECTS;
	af.type = gsn_summon_lord;
	af.level = modified_level;
	af.duration = modified_level / 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(victim, &af);

	return;
}

void
spell_banish(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (victim->race != race_lookup("deamon"))
	{
		Cprintf(ch, "You may only banish demons!\n\r");
		return;
	}

	if (ch == victim)
	{
		Cprintf(ch, "Suicide is a mortal sin.\n\r");
		return;
	}


	if (saving_throw(ch, victim, sn, caster_level + 2, SAVE_HARD, STAT_WIS, DAM_NONE))
	{
		act("You failed to banish $M!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		act("$n attempted to banish $M!", ch, NULL, victim, TO_ROOM, POS_RESTING);
		return;
	}

	act("You drive $M back to $N home plane!", ch, NULL, victim, TO_CHAR, POS_RESTING);
	act("$n forces you back to your plane of existence!", ch, NULL, victim, TO_VICT, POS_RESTING);
	act("$n banishes $M from this realm!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	extract_char(victim, TRUE);
}

/* newconj */
void
spell_crushing_hand(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, gsn_crushing_hand))
	{
		Cprintf(ch, "They are already caught in a huge crushing hand.\n\r");
		return;
	}

	if (get_curr_stat(victim, STAT_DEX) > number_percent())
	{
		Cprintf(ch, "Your victim manages to dodge out of the way!\n\r");
		Cprintf(victim, "You dodge out of the way of a massive crushing hand!\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 4;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = 0;
	affect_to_char(victim, &af);

	Cprintf(victim, "You are caught in a massive crushing hand!\n\r");
	act("$n is crushed in a massive hand!", victim, NULL, NULL, TO_ROOM, POS_RESTING);

	return;
}

void
spell_clenched_fist(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
	dam = dam * 11 / 10;

	damage(ch, victim, dam, sn, DAM_SLASH, TRUE, TYPE_MAGIC);
	return;
}

void
spell_feeblemind(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam, saved;
    AFFECT_DATA af;
    int caster_level, modified_level;

    caster_level = get_caster_level(level);
    modified_level = get_modified_level(level);

    dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_HIGH, TRUE);

    dam = dam * 9 / 10;

    saved = saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_INT, DAM_MENTAL);

    if (is_affected(victim, sn)) {
        saved = TRUE;
    }

    if(!saved) {
        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 4;
	if (IS_NPC(victim))
		af.duration *= 2;
        af.modifier = -(modified_level / 10);
        af.bitvector = 0;
        af.location = APPLY_INT;
        affect_to_char(victim, &af);
        af.location = APPLY_WIS;
        affect_to_char(victim, &af);

        Cprintf(victim, "You feel your brain turn to jelly!\n\r");
        act("$n gets a real dumb look on their face!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
    }

    damage(ch, victim, dam, sn, DAM_MENTAL, TRUE, TYPE_MAGIC);

    return;
}

void
spell_psychic_crush(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
	dam = dam * 11 / 10;

	damage(ch, victim, dam, sn, DAM_MENTAL, TRUE, TYPE_MAGIC);
	return;
}


void
spell_turn_undead(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA* vch;
	CHAR_DATA* vch_next;
	int caster_level, modified_level;
	int dam;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	Cprintf(ch, "You raise your hands toward the sky in a holy prayer.\n\r");
	act("$n raises their hands toward the sky in a holy prayer.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;

		if (is_safe(ch, vch))
			continue;

		if (ch == vch)
			continue;

		if (!IS_NPC(vch))
			continue;

		dam = dice(modified_level, 10) + (2 * modified_level);

		if (race_lookup("undead") == vch->race
		|| is_name("undead", vch->name)
		|| IS_SET(vch->act, ACT_UNDEAD) ) /* Allow act bit, too. Coded by Tsongas. */
		{
			damage(ch, vch, dam, sn, DAM_HOLY, TRUE, TYPE_MAGIC);
		}
	}

	return;
}

void
spell_withstand_death(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(ch, sn))
	{
		Cprintf(ch, "You refresh your protection from death.\n\r");
		affect_refresh(ch, sn, modified_level / 5);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 5;
	af.modifier = 0;
	af.bitvector = 0;
	af.location = 0;
	affect_to_char(ch, &af);

	act("$n is protected from death.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You protect yourself from death.\n\r");
}


void
spell_animate_tree(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
    CHAR_DATA* victim = NULL;
    AFFECT_DATA af;
    int mobcount = 0;
    int caster_level, modified_level;

    caster_level = get_caster_level(level);
    modified_level = get_modified_level(level);

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE) || wearing_nogate_item(ch)) {
        Cprintf(ch, "Not in this room.\n\r");
        return;
    }

    for (victim = char_list; victim != NULL; victim = victim->next) {
        if ((victim->master == ch) && IS_NPC(victim) && (victim->pIndexData->vnum == MOB_VNUM_TREE)) {
            mobcount++;
            if (mobcount > 3) {
                Cprintf(ch, "The trees cannot answer your call.\n\r");
                return;
            }
        }
    }

    if (max_no_charmies(ch)) {
        Cprintf(ch, "You can't control anymore trees.\n\r");
        return;
    }

    if (ch->in_room->sector_type != SECT_FOREST &&
            ch->in_room->sector_type != SECT_HILLS &&
            ch->in_room->sector_type != SECT_SWAMP)
    {
        Cprintf(ch, "You cannot find any trees to animate.\n\r");
        return;
    }

    Cprintf(ch, "You utter a chant, and bless a tree, and it comes to life.\n\r");
    act("$n blesses a tree and it comes to life!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

    victim = create_mobile(get_mob_index(MOB_VNUM_TREE));
    char_to_room(victim, ch->in_room);

    size_mob(ch, victim, modified_level);
    SET_BIT(victim->toggles, TOGGLES_NOEXP);
    victim->max_hit = victim->max_hit * 3 / 2;
    victim->hit = victim->max_hit;
    victim->hitroll = modified_level / 8 + 1;
    victim->damroll = modified_level / 2 + 10;

    add_follower(victim, ch);
    victim->leader = ch;

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = modified_level;
    af.duration = modified_level / 2;
    af.location = 0;
    af.modifier = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, &af);

    return;
}

void
spell_prismatic_sphere(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(ch, sn))
	{
		Cprintf(ch, "You refresh your sphere.\n\r");
		affect_refresh(ch, sn, modified_level / 6);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 6;
	af.modifier = 2 + (modified_level > 35) + (modified_level > 44) + (modified_level > 51);
	af.location = APPLY_CON;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	act("$n is surrounded by a rainbow aura.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You are surrounded by a rainbow aura.\n\r");
	return;
}

void
spell_spell_stealing(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA* victim = (CHAR_DATA*)vo;
    int spell_sn;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

 	target_name = one_argument(target_name, arg1);
    target_name = one_argument(target_name, arg2);

	if (is_affected(ch, gsn_spell_stealing)) {
		Cprintf(ch, "Your mind is already full of arcane knowledge.\n\r");
		return;
	}

 	if (arg2[0] == '\0') {
            Cprintf(ch, "Which spell were you trying to steal?\n\r");
            return;
    }
    if (IS_NPC(victim)) {
            Cprintf(ch, "You may only steal spells from players.\n\r");
            return;
    }

	spell_sn = find_spell(victim, arg2);

    if (spell_sn < 1
	|| skill_table[spell_sn].spell_fun == spell_null
	|| victim->level < skill_table[spell_sn].skill_level[victim->charClass]
	|| victim->pcdata->learned[spell_sn] == 0) {

        Cprintf(ch, "They don't know any spells of that name.\n\r");
        return;
    }
	if(skill_table[spell_sn].remort > 0) {
		Cprintf(ch, "You cannot steal that spell.\n\r");
		return;
	}
	if(get_skill(ch, spell_sn) > 0) {
		Cprintf(ch, "You already know this spell.\n\r");
		return;
	}
    if (spell_sn == gsn_acid_breath
    || spell_sn == gsn_frost_breath
    || spell_sn == gsn_gas_breath
    || spell_sn == gsn_fire_breath
    || spell_sn == gsn_lightning_breath) {
            Cprintf(ch, "They don't know any spells of that name.\n\r");
            return;
    }

	ch->pcdata->learned[spell_sn] = get_skill(victim, spell_sn);

	af.where = TO_AFFECTS;
    af.type = gsn_spell_stealing;
    af.level = caster_level;
    af.duration = caster_level / 4;
    af.location = APPLY_NONE;
    af.modifier = spell_sn;
    af.bitvector = 0;
    affect_to_char(ch, &af);

	Cprintf(ch, "Your mind is flooded with knowledge of %s!\n\r", skill_table[spell_sn].name);
	Cprintf(victim, "You feel your mind being drained of %s!\n\r", skill_table[spell_sn].name);
}

void
end_spell_stealing(void *vo, int target)
{
        CHAR_DATA *ch = (CHAR_DATA *) vo;
        AFFECT_DATA *paf;

	// No Mobs
	if(IS_NPC(ch)) {
		return;
	}

        paf = affect_find(ch->affected, gsn_spell_stealing);
        if (paf != NULL)
        {
                ch->pcdata->learned[paf->modifier] = 0;
        }
}

void
spell_oculary(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(ch, sn))
	{
		Cprintf(ch, "You refresh the oculary.\n\r");
		affect_refresh(ch, sn, modified_level / 2);
		return;
	}

	act("$n ripples out of existence.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level/2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);
	Cprintf(ch, "You ripple out of existence.\n\r");
	return;
}

void
spell_soul_trap(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA* victim =(CHAR_DATA*)vo;
	OBJ_DATA* obj;
	char buf[MAX_STRING_LENGTH];
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_safe(ch, victim))
		return;

	if (!IS_NPC(victim))
	{
		Cprintf(ch, "Not on players.\n\r");
		return;
	}

	if (IS_AFFECTED(victim, AFF_CHARM)
		|| IS_AFFECTED(ch, AFF_CHARM)
		|| IS_SET(victim->imm_flags, IMM_CHARM)
		|| victim->hunting != NULL
		|| victim->level > modified_level + 3)
	{
		Cprintf(ch, "That soul is too powerful for you to contain.\n\r");
		return;
	}

	/* they must have a blank figurine, held */
	obj = get_eq_char(ch, WEAR_HOLD);
	if(obj == NULL
	|| !is_name("figurine blank_fig", obj->name))
	{
		Cprintf(ch, "You lack the proper component for this spell.\n\r");
		return;
	}

	if (!is_name("blank_fig", obj->name))
	{
		Cprintf(ch, "This figurine has already been used.\n\r");
		return;
	}

	if(saving_throw(ch, victim, sn, caster_level + 2, SAVE_HARD, STAT_NONE, DAM_MENTAL)) {
		Cprintf(ch, "Your soul trap is resisted.\n\r");
		return;
	}
	free_string(obj->name);
	free_string(obj->short_descr);
	free_string(obj->description);

	obj->level = UMIN(ch->level, victim->level);
	obj->cost = 0;
	obj->value[0] = victim->level;
	obj->value[4] = victim->pIndexData->vnum;

	obj->timer = number_range(1000, 1200);

	sprintf(buf, "figurine %s", victim->name);
	obj->name = str_dup(buf);
	sprintf(buf, "figurine of %s", victim->short_descr);
	obj->short_descr = str_dup(buf);
	sprintf(buf, "A figurine of %s is here.\n\r", victim->short_descr);
	obj->description = str_dup(buf);

	/* k, kill mob, set figurine with important info */
	act("You channel $M's soul into your figurine!", ch, NULL, victim, TO_CHAR, POS_RESTING);
	act("$n channels your soul into a figurine!", ch, NULL, victim, TO_VICT, POS_RESTING);
	act("$n channels $N's soul into a figurine!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);

	stop_fighting(victim, TRUE);
	death_cry(victim);
	extract_char(victim, TRUE);
}

void
spell_figurine_spell(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA* obj;
	CHAR_DATA* mob, *victim;
	AFFECT_DATA af;
	char buf[MAX_STRING_LENGTH];
	int mobcount=0;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	obj = get_eq_char(ch, WEAR_HOLD);
	if (obj == NULL || is_name("blank_fig", obj->name))
	{
		Cprintf(ch, "You must use a figurine with a soul in it.\n\r");
		return;
	}
	if (modified_level < 5
	|| obj->value[4] == 0) {
		Cprintf(ch, "That figurine is broken.\n\r");
		return;
	}

	for (victim = char_list; victim != NULL; victim = victim->next) {
            if (victim->master == ch && IS_NPC(victim)
            && is_affected(victim, gsn_figurine_spell)) {
                    mobcount++;
                    if (mobcount > 3)
                    {
                            Cprintf(ch, "You can't control anymore figurines.\n\r");
                            return;
                    }
            }
	}

	mob = create_mobile(get_mob_index(obj->value[4]));

	Cprintf(ch, "A black form leaps from the figurine and solidifies.\n\r");
	act("A massive black form leaps from $n's figurine!.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	/* get funky with strings! */
	free_string(mob->name);
	free_string(mob->short_descr);
	free_string(mob->long_descr);

	mob->name = str_dup(obj->name);
	sprintf(buf, "an animated %s", obj->short_descr);
	mob->short_descr = str_dup(buf);

	strcpy(buf, "An animated");
	strcat(buf, &(obj->description[1]));
	mob->long_descr = str_dup(buf);
	SET_BIT(mob->toggles, TOGGLES_NOEXP);

	char_to_room(mob, ch->in_room);
	add_follower(mob, ch);
	mob->leader = ch;

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(mob, &af);
}


void
spell_remove_align(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA* obj = (OBJ_DATA*)vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (obj->level >= 54) {
		Cprintf(ch, "The purple aura fails to appear.\n\r");
		return;
	}

	if (IS_OBJ_STAT(obj, ITEM_ANTI_EVIL))
	{
		REMOVE_BIT(obj->extra_flags, ITEM_ANTI_EVIL);
	}

	if (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD))
	{
		REMOVE_BIT(obj->extra_flags, ITEM_ANTI_GOOD);
	}

	if (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL))
	{
		REMOVE_BIT(obj->extra_flags, ITEM_ANTI_NEUTRAL);
	}

	act("$p glows with a purple aura and then fades.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	act("$p glows with a purple aura and then fades.", ch, obj, NULL, TO_ROOM, POS_RESTING);


	// Lord knows why, big penalties for using this spell
	if (obj->level < 54)
		obj->level++;

	add_hit_dam(ch, obj, -1, -1);

}

void
spell_inform(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA* victim = (CHAR_DATA*)vo;

	Cprintf(ch, "You rattle your sticks of fortune.\n\r");
	Cprintf(ch, "Your augury tells you that %s has!\n\r", PERS(victim, ch));
	Cprintf(ch, "Hit Points: %d\n\r", victim->hit);
	Cprintf(ch, "Mana      : %d\n\r", victim->mana);
	Cprintf(ch, "Move      : %d\n\r", victim->move);
	Cprintf(ch, "Dam Roll  : %d\n\r", GET_DAMROLL(victim));
	Cprintf(ch, "Hit Roll  : %d\n\r", GET_HITROLL(victim));
	Cprintf(ch, "Saves     : %d\n\r", victim->saving_throw);

	act("$n tosses the sticks of fortune telling!\n\r", ch, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
spell_atheism(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA* victim = (CHAR_DATA*)vo;
	int dp;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_safe(ch, victim) || IS_NPC(victim))
	{
		Cprintf(ch, "Not on that target.\n\r");
		return;
	}

	if (saving_throw(ch, victim, sn, caster_level + 2, SAVE_HARD, STAT_WIS, DAM_NONE))
	{
		return;
	}

	dp = UMIN(victim->deity_points, 4 * dice(6, modified_level));
	victim->deity_points -= dp;
	ch->deity_points += dp;

	Cprintf(victim, "You feel your faith being drained away!\n\r");
	Cprintf(ch, "You steal power for your god!\n\r");
}

void
spell_missionary(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA* victim = (CHAR_DATA*)vo;
	int type;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_safe(ch, victim) || IS_NPC(victim))
	{
		Cprintf(ch, "Not on that target.\n\r");
		return;
	}

	if (victim->deity_type == 0)
	{
		Cprintf(ch, "They are an atheist.\n\r");
		return;
	}

	if (victim != ch
	&& saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_WIS, DAM_HOLY))
	{
		Cprintf(ch, "They resist your powers of persuasion.\n\r");
		return;
	}

	for (type = victim->deity_type; victim->deity_type == type; )
	{
		type = number_range(1, 5);
	}
	victim->deity_type = type;

	Cprintf(ch, "You force them to adopt the way of %s.\n\r", deity_table[victim->deity_type - 1].name);
	Cprintf(victim, "You are forced to adopt the way of %s.\n\r", deity_table[victim->deity_type - 1].name);
}

void
spell_attraction(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj_lose, *obj_next;
	bool fail = TRUE;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (ch == victim)
	{
		Cprintf(ch, "Is that smart?\n\r");
		return;
	}

	if (!saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_CON, DAM_FIRE)
		&& !IS_SET(victim->imm_flags, IMM_FIRE)
		&& fail == TRUE)
	{
		for (obj_lose = victim->carrying;
			 obj_lose != NULL;
			 obj_lose = obj_next)
		{
			obj_next = obj_lose->next_content;
			if(number_percent() < 20
				&& !IS_OBJ_STAT(obj_lose, ITEM_NONMETAL)
				&& !IS_OBJ_STAT(obj_lose, ITEM_BURN_PROOF))
			{
				switch (obj_lose->item_type)
				{
				case ITEM_ARMOR:
					if (obj_lose->wear_loc != -1)	/* remove the item */
					{
						if (can_drop_obj(victim, obj_lose)
							&& (obj_lose->weight / 10) <
							number_range(1, 2 * get_curr_stat(victim, STAT_DEX))
							&& remove_obj(victim, obj_lose->wear_loc, TRUE))
						{
							act("$p is drawn from $n!", victim, obj_lose, NULL, TO_ROOM, POS_RESTING);
							act("$p is drawn from you!", victim, obj_lose, NULL, TO_CHAR, POS_RESTING);
							obj_from_char(obj_lose);
							obj_to_char(obj_lose, ch);
							fail = FALSE;
						}
					}
					else
						/* drop it if we can */
					{
						if (can_drop_obj(victim, obj_lose))
						{
							act("$p is drawn from $n!", victim, obj_lose, NULL, TO_ROOM, POS_RESTING);
							act("$p is drawn from you!", victim, obj_lose, NULL, TO_CHAR, POS_RESTING);
							obj_from_char(obj_lose);
							obj_to_char(obj_lose, ch);
							fail = FALSE;
						}
					}
					break;
				case ITEM_WEAPON:
					if (obj_lose->wear_loc != -1)	/* try to drop it */
					{
						if (can_drop_obj(victim, obj_lose)
							&& remove_obj(victim, obj_lose->wear_loc, TRUE))
						{
							act("$p is ripped from $n's grasp!", victim, obj_lose, NULL, TO_ROOM, POS_RESTING);
							Cprintf(victim, "Your weapon is ripped from your grasp!\n\r");
							obj_from_char(obj_lose);
							obj_to_char(obj_lose, ch);
							fail = FALSE;
						}
					}
					else
						/* drop it if we can */
					{
						if (can_drop_obj(victim, obj_lose))
						{
							act("$p is drawn from $n!", victim, obj_lose, NULL, TO_ROOM, POS_RESTING);
							act("$p is drawn from you!", victim, obj_lose, NULL, TO_CHAR, POS_RESTING);
							obj_from_char(obj_lose);
							obj_to_char(obj_lose, ch);
							fail = FALSE;
						}
						else
							/* cannot drop */
						{
							act("Your skin tears as $p struggles to free itself!", victim, obj_lose, NULL, TO_CHAR, POS_RESTING);
							fail = FALSE;
						}
					}
					break;
				}
			}
		}
	}
	if (fail)
	{
		Cprintf(ch, "Your spell had no effect.\n\r");
		Cprintf(victim, "You feel your equipment momentarily lift up and fall back in place.\n\r");
		return;
	}

	Cprintf(ch, "Your insidious telekinetic attack succeeds!\n\r");
	return;
}

void
spell_flag(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj;
	AFFECT_DATA af;
	int cur_flag, new_flag;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	obj = (OBJ_DATA *) vo;

	if (obj->item_type == ITEM_WEAPON)
	{
		/* can only add to weapons with no flags, or one and only one flag */
		/* I don't even want to think about a flaming, frosting, shocking, poison
		   cerminal dagger (cerimonial, flag, envenom) 3 flags is WAY enough*/
		if ((!obj_is_affected(obj, sn) && (obj->value[4] == WEAPON_FLAMING ||
			obj->value[4] == WEAPON_FROST ||
			obj->value[4] == WEAPON_VAMPIRIC ||
			obj->value[4] == WEAPON_SHARP ||
			obj->value[4] == WEAPON_VORPAL ||
			obj->value[4] == WEAPON_TWO_HANDS ||
			obj->value[4] == WEAPON_SHOCKING ||
			obj->value[4] == WEAPON_SHARP ||
			obj->value[4] == WEAPON_POISON ||
			obj->value[4] == WEAPON_DRAGON_SLAYER ||
			obj->value[4] == WEAPON_DULL ||
			obj->value[4] == WEAPON_BLUNT ||
			obj->value[4] == WEAPON_CORROSIVE ||
			obj->value[4] == WEAPON_FLOODING ||
			obj->value[4] == WEAPON_INFECTED ||
			obj->value[4] == WEAPON_SOULDRAIN)) ||
			obj->value[4] == 0)
		{

			/* find cool new flag */
			cur_flag = new_flag = get_random_bit(obj->value[4]);

			do
			{
				dam = dice(1, 9);
				if (dam > 9 || dam < 1)
					dam = 1;

				if (dam == 1)
				{
					new_flag = WEAPON_FLAMING;
				}
				else if (dam == 2)
				{
					new_flag = WEAPON_FROST;
				}
				else if (dam == 3)
				{
					new_flag = WEAPON_VAMPIRIC;
				}
				else if (dam == 4)
				{
					new_flag = WEAPON_SHOCKING;
				}
				else if (dam == 5)
				{
					new_flag = WEAPON_CORROSIVE;
				}
				else if (dam == 6)
				{
					new_flag = WEAPON_FLOODING;
				}
				else if (dam == 7)
				{
					new_flag = WEAPON_POISON;
				}
				else if (dam == 8)
				{
					new_flag = WEAPON_INFECTED;
				}
				else if (dam == 9)
				{
					new_flag = WEAPON_SOULDRAIN;
				}
			}
			while (cur_flag == new_flag);

			if (new_flag == WEAPON_FLAMING)
			{
				Cprintf(ch, "The weapon glows with a white hot aura.\n\r");
			}
			if (new_flag == WEAPON_FROST)
			{
				Cprintf(ch, "The weapon glows with a freezing blue aura.\n\r");
			}
			if (new_flag == WEAPON_VAMPIRIC)
			{
				Cprintf(ch, "The weapon glows with a pitch black aura.\n\r");
			}
			if (new_flag == WEAPON_SHOCKING)
			{
				Cprintf(ch, "The weapon glows with an electric blue aura.\n\r");
			}
			if (new_flag == WEAPON_CORROSIVE)
			{
				Cprintf(ch, "The weapon glows with an acidic aura.\n\r");
			}
			if (new_flag == WEAPON_FLOODING)
			{
				Cprintf(ch, "The weapon becomes heavy and wet.\n\r");
			}
			if (new_flag == WEAPON_POISON)
			{
				Cprintf(ch, "The weapon glows with a deep green aura.\n\r");
			}
			if (new_flag == WEAPON_INFECTED)
			{
				Cprintf(ch, "The weapon gains a stench of sewers.\n\r");
			}
			if (new_flag == WEAPON_SOULDRAIN)
			{
				Cprintf(ch, "The weapon begins to howl out for blood...\n\r");
			}

			af.where = TO_WEAPON;
			af.type = sn;
			af.level = modified_level;
			af.duration = modified_level / 2;
			af.location = 0;
			af.modifier = 0;
			af.bitvector = new_flag;
			affect_to_obj(obj, &af);

			act("$p glows with a new light.", ch, obj, NULL, TO_ALL, POS_RESTING);
			return;
		}
		else
		{
			act("You can't seem to flag $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			return;
		}

	}

	act("You can't flag $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	return;
}

void
spell_jinx(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		Cprintf(ch, "They are already jinxed!\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 3;
	if (IS_NPC(victim))
		af.duration *= 2;
	af.location = APPLY_SAVING_SPELL;
	af.modifier = +5;
	af.bitvector = 0;
	affect_to_char(victim, &af);

	Cprintf(victim, "You have been jinxed!\n\r");
	Cprintf(ch, "Your jinx casts them down!\n\r");
}

void
spell_moonbeam(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	CHAR_DATA* vch;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (weather_info.sunlight == SUN_LIGHT)
	{
		Cprintf(ch, "You cannot call a moonbeam.. there is no moon yet.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 4;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = 0;

	/* and msg room */
	act("$n calls a moonbeam to envelop the room in darkness!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	act("You call a moonbeam to the room!", ch, NULL, NULL, TO_CHAR, POS_RESTING);

	/* first, affect undead with moonbeam */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if ((vch->race == race_lookup("undead")
		|| vch->race == race_lookup("deamon"))
		&& !is_affected(vch, sn))
		{
			act("A moonbeam infuses $N with unholy power.", ch, NULL, vch, TO_ROOM, POS_RESTING);
			act("Your moonbeam infuses $N with unholy power.", ch, NULL, vch, TO_CHAR, POS_RESTING);
			affect_to_char(vch, &af);
		}
	}

	/* now, nuke the light sources */
	ch->in_room->light = 0;
}

void
spell_sunray(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	CHAR_DATA* vch;
	CHAR_DATA* vch_next;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (weather_info.sunlight == SUN_DARK)
	{
		Cprintf(ch, "You cannot call a sun ray...there is no sun yet.\n\r");
		return;
	}

	/* and msg room */
	act("$n calls a blinding sun ray, bathing the room in brilliance!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	act("You call a dazzling sun ray!", ch, NULL, NULL, TO_CHAR, POS_RESTING);

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.location = APPLY_HITROLL;
	af.modifier = -4;
	af.duration = 0;
	af.bitvector = AFF_BLIND;

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;

		/* don't blind the poor newbies! */
		if (!IS_NPC(vch)
		&& is_safe(ch, vch))
			continue;

		if ( (vch != ch && !saving_throw(ch, vch, sn, caster_level + 2, SAVE_HARD, STAT_INT, DAM_LIGHT))
		||   (vch == ch && !saving_throw(ch, vch, sn, caster_level + 2, SAVE_NORMAL, STAT_NONE, DAM_LIGHT)))
		{
			/* keep going if already affected */
			if(IS_AFFECTED(vch, AFF_BLIND))
				continue;

			act("$N is struck down by the brilliant sun ray!", ch, NULL, vch, TO_ALL, POS_RESTING);
			affect_to_char(vch, &af);

			if (vch->race == race_lookup("undead"))
			{
				act("$N burns and starts to crumble...", ch, NULL, vch, TO_ALL, POS_RESTING);
				vch->rot_timer = 1;
			}
		}
	}
}

void heal_room(CHAR_DATA *ch, ROOM_INDEX_DATA* room, int level)
{
	CHAR_DATA* gch;
	int heal;

	if (room == NULL || level <= 0)
		return;

	for (gch = room->people; gch != NULL; gch = gch->next_in_room)
	{
		Cprintf(gch, "A shimmering wave passes through the room.\n\r");
		if (!IS_NPC(gch)
		&& (is_same_group(gch, ch)
		|| gch->clan == ch->clan
		|| gch->clan == 0))
		{
			heal = dice(10, level / 2) + 1;
			if(!is_affected(gch, gsn_dissolution))
				gch->hit = UMIN(gch->hit + heal, MAX_HP(gch));
			update_pos(gch);
			Cprintf(gch, "Life wave heals your wounds!\n\r");
		}
	}
}

void
spell_life_wave(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	int center;
	int middle;
	int outer;
	EXIT_DATA* pexit_main;
	EXIT_DATA* pexit_middle;
	EXIT_DATA* pexit_outer;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	heal_room(ch, ch->in_room, modified_level);

	for (center = 0; center < 6; center++)
	{
		pexit_main = ch->in_room->exit[center];
		if (pexit_main != NULL && pexit_main->u1.to_room != NULL &&
			pexit_main->u1.to_room != ch->in_room)
		{
			heal_room(ch, pexit_main->u1.to_room, modified_level);

			for (middle = 0; middle < 6; middle++)
			{
				pexit_middle = pexit_main->u1.to_room->exit[middle];
				if (pexit_middle != NULL && pexit_middle->u1.to_room != NULL &&
					pexit_middle->u1.to_room != pexit_main->u1.to_room &&
					pexit_middle->u1.to_room != ch->in_room)
				{
					heal_room(ch, pexit_middle->u1.to_room, modified_level * 2 / 3);

					for (outer = 0; outer < 6; outer++)
					{
						pexit_outer = pexit_middle->u1.to_room->exit[outer];
						if (pexit_outer != NULL && pexit_outer->u1.to_room != NULL &&
							pexit_outer->u1.to_room != pexit_middle->u1.to_room &&
							pexit_outer->u1.to_room != pexit_main->u1.to_room &&
							pexit_outer->u1.to_room != ch->in_room)
						{
							heal_room(ch, pexit_outer->u1.to_room, modified_level / 2);
						}
					}
				}
			}
		}
	}
	return;
}

void
spell_shock_wave(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int attempt;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	for (vch = victim->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;

		if (is_safe(ch, vch))
			continue;

		if (vch == ch)
			continue;

		if (saving_throw(ch, vch, sn, caster_level, SAVE_HARD, STAT_STR, DAM_LIGHTNING))
		{
			damage(ch, vch, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);
		}
		else
		{
			damage(ch, vch, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);
			for (attempt = 0; attempt < 6; attempt++)
			{
				EXIT_DATA *pexit;
				int door;

				door = number_door();
				if ((pexit = ch->in_room->exit[door]) == 0
					|| pexit->u1.to_room == NULL
					|| (IS_SET(pexit->exit_info, EX_CLOSED) && !is_affected(vch, gsn_pass_door))
					|| (IS_NPC(vch) && IS_SET(vch->imm_flags, IMM_CHARM))
					|| (IS_NPC(vch) && IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)))
					continue;

				Cprintf(vch, "You are blasted from the room!\n\r");
				act("$n is blasted from the room!", vch, NULL, NULL, TO_ROOM, POS_RESTING);
				move_char(vch, door, FALSE);
				stop_fighting(vch, TRUE);
				WAIT_STATE(vch, number_range(20, 30));
				break;
			}
		}
	}

	return;
}


void meteor_blast(CHAR_DATA *ch, ROOM_INDEX_DATA *room, int level) {
	CHAR_DATA *rch;
	CHAR_DATA *rch_next;
	int dam, chance;

	if(ch->in_room == NULL
	|| room == NULL)
		return;


	if(ch->in_room != room)
		Cprintf(ch, "A flaming meteor crashes into the ground nearby and explodes!\n\r");

	for(rch = char_list; rch != NULL; rch = rch_next) {
		rch_next = rch->next;
		if(rch->in_room == room) {
			if (is_safe(ch, rch))
				continue;

			chance = 90 - get_curr_stat(rch, STAT_DEX);

			if(rch == ch)
				chance /= 2;

			if(number_percent() >= chance) {
				Cprintf(rch, "By sheer luck you avoid the meteor!\n\r");
				continue;
			}

			dam = spell_damage(ch, rch, level, SPELL_DAMAGE_MEDIUM, TRUE);
			damage(ch, rch, dam, gsn_meteor_swarm, DAM_ENERGY, TRUE, TYPE_MAGIC);
			if(rch->hit < 1
	                || rch->position == POS_DEAD)
		                continue;
		}
	}
}

void
spell_meteor_swarm(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
	int chance, range, door, dist, i;
	int meteorcount;
	int exitcount = 0;
	EXIT_DATA *pexit;
	ROOM_INDEX_DATA *rooms[6];
	ROOM_INDEX_DATA *destination, *one_room;
	int caster_level, modified_level;
	CHAR_DATA *victim;

	// A fix so that meteors can be fired from a distance
	// using ranged weapons.
	if(ch->fighting)
		victim = ch->fighting;
	else
		victim = ch;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);
	meteorcount = dice(modified_level / 12, modified_level / 12);

	for(i=0; i<meteorcount; i++) {
	    // If the caster is killed by his own meteors
	    if(ch->in_room == NULL
	    || ch->position == POS_DEAD)
		    return;

	    // If the target no longer applies, find a new one.
	    if(victim == NULL
	    || victim->position == POS_DEAD
	    || victim->in_room == NULL
	    || victim->in_room->area != ch->in_room->area) {
		if(ch->fighting)
	        	victim = ch->fighting;
		else
	        	victim = ch;
	    }

	    chance = number_percent();
	    if(chance < 40)
		range = 0;
	    else if(chance < 80)
		range = 1;
	    else if(chance >= 80)
		range = 2;

	    if(range == 0) {
			meteor_blast(ch, victim->in_room, modified_level);
			continue;
	    }
	    if(range == 1) {
			exitcount = -1;
			for(door=0;door<6;door++) {
				pexit = victim->in_room->exit[door];
				if(pexit != NULL
				&& pexit->u1.to_room != NULL
				&& !IS_SET(pexit->exit_info, EX_CLOSED)) {
					exitcount++;
					rooms[exitcount] = pexit->u1.to_room;
				}
		}
		if(exitcount < 0) {
			meteor_blast(ch, victim->in_room, modified_level);
			continue;
		}
		destination = rooms[number_range(0, exitcount)];
		meteor_blast(ch, destination, modified_level);
		continue;
	    }
	    if(range == 2) {
		exitcount = -1;
		for(door=0; door<6; door++) {
                    one_room = victim->in_room;
                    for(dist=0; dist<2; dist++) {
                        pexit = one_room->exit[door];
                        if(pexit != NULL
                        && pexit->u1.to_room != NULL
                        && !IS_SET(pexit->exit_info, EX_CLOSED)) {
			    one_room = pexit->u1.to_room;
			    if(dist == 1) {
				exitcount++;
				rooms[exitcount] = one_room;
			    }
		        }
	            }
		}
		if(exitcount < 0) {
			exitcount = -1;
	                for(door=0;door<6;door++) {
                        	pexit = victim->in_room->exit[door];
                       		if(pexit != NULL
                        	&& pexit->u1.to_room != NULL
                        	&& !IS_SET(pexit->exit_info, EX_CLOSED)) {
                                	exitcount++;
                                	rooms[exitcount] = pexit->u1.to_room;
                        	}
                	}
                	if(exitcount < 0) {
                        	meteor_blast(ch, victim->in_room, modified_level);
                        	continue;
                	}
                	destination = rooms[number_range(0, exitcount)];
                	meteor_blast(ch, destination, modified_level);
                	continue;
		}
		destination = rooms[number_range(0, exitcount)];
		meteor_blast(ch, destination, modified_level);
		continue;
	    }
	}
}


void
spell_delayed_fireball(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA*)vo, *rch;
	CHAR_DATA *rch_next;
	AFFECT_DATA af;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(is_affected(victim, gsn_delayed_blast_fireball)) {
		Cprintf(ch, "Your spell fizzles, they are already affected.\n\r");
		return;
	}

	/* If delay fails, it goes off immediately. */
	if(saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_CON, DAM_FIRE)) {
		 Cprintf(ch, "You fail to delay the fireball, it explodes instantly!\n\r");
		 for(rch = char_list; rch != NULL; rch = rch_next) {
			rch_next = rch->next;
			if(rch->in_room == ch->in_room) {
				if (is_safe(ch, rch))
					continue;

       		        	if(ch != rch)
					Cprintf(rch, "A red ruby appears above %s and explodes violently!\n\r", rch->name);

				dam = spell_damage(ch, rch, modified_level, SPELL_DAMAGE_HIGH, TRUE);
				dam = dam * 11 / 10;

                		damage(ch, rch, dam, gsn_delayed_blast_fireball, DAM_FIRE, TRUE, TYPE_MAGIC);
			}
		}
	    return;
	}

	/* failed their save, they get a big surprise in a few ticks */
	Cprintf(ch, "A red ruby appears above %s and hovers patiently.\n\r", victim->name);
	Cprintf(victim, "A red ruby appears above your head, but nothing happens.\n\r");
	act("A red ruby appears above $n's head, but nothing bad happens.", victim, NULL, NULL, TO_ROOM, POS_RESTING);

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = number_range(0, 2);
	af.location = 0;
	af.modifier = 0;
	af.bitvector = 0;

	affect_to_char(victim, &af);
}

/* First use of thie end_fun thing since it was added..
   Hope it works!! */
void end_delayed_fireball(void * vo, int target) {
	CHAR_DATA *ch = (CHAR_DATA*) vo;
	CHAR_DATA *rch, *rch_next;
	int dam;

    	if(ch->in_room == NULL)
		return;

	for(rch = char_list; rch != NULL; rch = rch_next) {
		rch_next = rch->next;
		if(rch->in_room == ch->in_room
		&& rch != ch) {
			if (is_safe(ch, rch))
				continue;

	       		Cprintf(rch, "The strange red ruby following %s explodes violently!\n\r", ch->name);
       	     		dam = spell_damage(ch, rch, rch->level, SPELL_DAMAGE_HIGH, TRUE);
			dam = dam * 3 / 2;

            		damage(rch, rch, dam, gsn_delayed_blast_fireball, DAM_FIRE, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);
		}
	}
	/* damage owner last */
	dam = spell_damage(ch, ch, ch->level, SPELL_DAMAGE_HIGH, TRUE);
	dam = dam * 3 / 2;
	damage(ch, ch, dam, gsn_delayed_blast_fireball, DAM_FIRE, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);

	return;
}


void
spell_elemental_protection(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	int type;
	int resist_bit, vuln_bit, dam_type;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(ch, sn))
	{
		Cprintf(ch, "You refresh your protection.\n\r");
		affect_refresh(ch, sn, modified_level / 4);
		return;
	}

	type = number_range(1, 6);

	switch(type) {
	case 1:
		resist_bit = RES_FIRE;
		vuln_bit = VULN_FIRE;
		dam_type = DAM_FIRE;
		Cprintf(ch, "A raging inferno surrounds you, protecting against fire.\n\r");
		break;
	case 2:
		resist_bit = RES_COLD;
		vuln_bit = VULN_COLD;
		dam_type = DAM_COLD;
		Cprintf(ch, "Your skin tingles as a shield against cold surrounds you.\n\r");
		break;
	case 3:
		resist_bit = RES_LIGHTNING;
		vuln_bit = VULN_LIGHTNING;
		dam_type = DAM_LIGHTNING;
		Cprintf(ch, "A crackling wall appears to protect against lightning.\n\r");
		break;
	case 4:
		resist_bit = RES_POISON;
		vuln_bit = VULN_POISON;
		dam_type = DAM_POISON;
		Cprintf(ch, "A greenish haze will protect you against venom.\n\r");
		break;
	case 5:
		resist_bit = RES_ACID;
		vuln_bit = VULN_ACID;
		dam_type = DAM_ACID;
		Cprintf(ch, "A perfectly round bubble protects you from the effects of acid.\n\r");
		break;
	case 6:
		resist_bit = RES_DROWNING;
		vuln_bit = VULN_DROWNING;
		dam_type = DAM_DROWNING;
		Cprintf(ch, "A pocket of fresh air surrounds you, protecting against drowning.\n\r");
		break;
	}

	if(check_immune(ch, dam_type) == IS_IMMUNE
	|| check_immune(ch, dam_type) == IS_RESISTANT) {
		Cprintf(ch, "The barrier flickers and fades, you are already protected from that element.\n\r");
		return;
	}

	/* vuln -> norm */
	if (check_immune(ch, dam_type) == IS_VULNERABLE)
	{
		REMOVE_BIT(ch->vuln_flags, vuln_bit);

		/* remove bit, add affect for wear off to put back */
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = modified_level;
		af.duration = modified_level / 4;
		af.location = 0;
		af.modifier = vuln_bit;
		af.bitvector = 0;
		affect_to_char(ch, &af);
	}
	/* norm -> res */
	else
	{
		/* affect to resistences */
		af.where = TO_RESIST;
		af.type = sn;
		af.level = modified_level;
		af.duration = modified_level / 4;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = resist_bit;
		affect_to_char(ch, &af);

		/* also show affect to char in a non conflicting way */
		af.where = TO_AFFECTS;
		af.modifier = resist_bit + 1;
		af.bitvector = 0;
		affect_to_char(ch, &af);
	}

	act("$n looks more protected from the elements.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	return;
}

/* First use of thie end_fun thing since it was added..
   Hope it works!! */
void end_elemental_protection(void* vo, int target)
{
	CHAR_DATA *ch = (CHAR_DATA*) vo;
	AFFECT_DATA* paf;

	paf = affect_find(ch->affected, gsn_nature_protection);
	if (paf != NULL)
	{
		/* check to see if this is a resistance or vuln
		   only need to reset vuln! */
		if(paf->modifier % 2 == 0)
			SET_BIT(ch->vuln_flags, paf->modifier);
	}
}

void
spell_summon_angel(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
	CHAR_DATA *victim;
	AFFECT_DATA af;
	OBJ_DATA *obj;
	int mobcount = 0;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(ch->reclass == reclass_lookup("templar"))
		modified_level -= 4;

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
    {
            Cprintf(ch, "Not in this room.\n\r");
            return;
    }

    for (victim = char_list; victim != NULL; victim = victim->next) {
			if (victim->master == ch && IS_NPC(victim)
			&& victim->pIndexData->vnum == MOB_VNUM_ANGEL) {
                    mobcount++;
                    if (mobcount > 1)
                    {
                            Cprintf(ch, "Heaven helps those who help themselves.\n\r");
                            return;
                    }
            }
    }

	Cprintf(ch, "A glowing sphere appears, bringing an angelic warrior to aid you.\n\r");
	victim = create_mobile(get_mob_index(MOB_VNUM_ANGEL));
	char_to_room(victim, ch->in_room);
	obj = create_object(get_obj_index(OBJ_VNUM_ANGEL_SWORD), victim->level);
	size_mob(ch, victim, modified_level * 9/10);
	SET_BIT(victim->toggles, TOGGLES_NOEXP);
	size_obj(ch, obj, modified_level * 9/10);
	obj_to_char(obj, victim);
	equip_char(victim, obj, WEAR_WIELD);

	if (victim->master)
        		stop_follower(victim);
	add_follower(victim, ch);
	victim->leader = ch;

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(victim, &af);

	act("$n prays for help, and an angelic warrior heeds his call.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	return;
}

void
spell_flesh_golem(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
	CHAR_DATA *monster;
	int parts = 0;
	OBJ_DATA *head, *heart, *arm1, *leg1, *guts, *brain, *extra;
	AFFECT_DATA af;
	CHAR_DATA* victim;
	int mobcount;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);


	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
			Cprintf(ch, "Not in this room.\n\r");
			return;
	}

	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->master == ch && IS_NPC(victim)
		&& victim->pIndexData->vnum == MOB_VNUM_FLESH_GOLEM)
		{
			mobcount++;
			if (mobcount > 1)
			{
				Cprintf(ch, "Do you think you can control more than one of these?\n\r");
				return;
			}
		}
	}

	// Find out if they have required components.
	(head = get_obj_carry(ch, "head", ch)) == NULL ? Cprintf(ch, "Your flesh golem requires a head.\n\r") : parts++;
	(brain = get_obj_carry(ch, "brain", ch)) == NULL ? Cprintf(ch, "Your flesh golem requires a fresh brain.\n\r") : parts ++;
	(heart = get_obj_carry(ch, "heart", ch)) == NULL ? Cprintf(ch, "Your flesh golem requires a heart.\n\r") : parts ++;
	(guts = get_obj_carry(ch, "guts", ch)) == NULL ? Cprintf(ch, "Your flesh golem requires some guts.\n\r") : parts ++;
	(arm1 = get_obj_carry(ch, "arm", ch)) == NULL ? Cprintf(ch, "Your flesh golem requires an arm.\n\r") : parts ++;
	(leg1 = get_obj_carry(ch, "leg", ch)) == NULL ? Cprintf(ch, "Your flesh golem requires a leg.\n\r") : parts ++;

	if(parts != 6) {
		Cprintf(ch, "You cannot build an incomplete flesh golem. Spell failed.\n\r");
		return;
	}

	extract_obj(head);
	extract_obj(brain);
	extract_obj(heart);
	extract_obj(guts);
	extract_obj(arm1); //extract_obj(arm2);
	extract_obj(leg1); //extract_obj(leg2);

	Cprintf(ch, "Using the gruesome pile of body parts, you construct a flesh golem.\n\rArise my servant!\n\r");
	monster = create_mobile(get_mob_index(MOB_VNUM_FLESH_GOLEM));
	char_to_room(monster, ch->in_room);
	if (monster->master)
        	stop_follower(monster);
        add_follower(monster, ch);
        monster->leader = ch;

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = modified_level * 3 / 4;
        af.location = 0;
        af.modifier = 0;
        af.bitvector = AFF_CHARM;
        affect_to_char(monster, &af);

	/* Now build up the golem with whatever else they have. */
	/* Smart golem can use spells! */
	if((extra = get_obj_carry(ch, "brain", ch)) != NULL) {
		extract_obj(extra);
		Cprintf(ch, "Your golem seems more intelligent than normal.\n\r");
		monster->spec_fun = spec_lookup("spec_cast_mage");
	}
	/* Pumped golem has better hp */
	if((extra = get_obj_carry(ch, "heart", ch)) != NULL) {
		extract_obj(extra);
		Cprintf(ch, "Your golem is a lot tougher than normal.\n\r");
		monster->max_hit = monster->max_hit * 3/2;
		monster->hit = monster->max_hit;
	}
	/* Psycho golem has sick hit/dam */
	if((extra = get_obj_carry(ch, "guts", ch)) != NULL) {
		extract_obj(extra);
		Cprintf(ch, "Your golem looks braver than normal.\n\r");
		monster->hitroll += 6;
		monster->damroll += 12;
	}
	/* Extra arms is more warrior */
	if((extra = get_obj_carry(ch, "arm", ch)) != NULL) {
		extract_obj(extra);
		Cprintf(ch, "Your golem has the spirit of a warrior!\n\r");
		SET_BIT(monster->act, ACT_WARRIOR);
	}
	/* extra legs is faster */
	if((extra = get_obj_carry(ch, "leg", ch)) != NULL) {
		extract_obj(extra);
		Cprintf(ch, "Your golem begins to move faster.\n\r");
		SET_BIT(monster->off_flags, OFF_FAST);
	}
	/* wings can fly */
	if((extra = get_obj_carry(ch, "wing", ch)) != NULL) {
		extract_obj(extra);
		Cprintf(ch, "Fragile looking wings rise your golem off the ground.\n\r");
		SET_BIT(monster->affected_by, AFF_FLYING);
	}
	/* tail can use breathes */
	if((extra = get_obj_carry(ch, "tail", ch)) != NULL) {
		extract_obj(extra);
		Cprintf(ch, "Your golem oozes a disgusting bile!\n\r");
		monster->spec_fun = spec_lookup("spec_breath_acid");
	}

	act("$n fashions a flesh golem from a gruesome pile of body parts.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	return;
}

void
spell_animate_dead(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
        AFFECT_DATA af;
        CHAR_DATA *zombie, *owner;
        OBJ_DATA *corpse, *obj, *obj_next;
        char buf[MAX_STRING_LENGTH];
		int caster_level, modified_level;

		caster_level = get_caster_level(level);
		modified_level = get_modified_level(level);

        if (max_no_charmies(ch)) {
                Cprintf(ch, "You can't control so many creatures at once.\n\r");
                return;
        }

        if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
        {
                Cprintf(ch, "Not in this room.\n\r");
                return;
        }

	/* Need a fresh cadaver */
        corpse = get_obj_list(ch, "corpse", ch->in_room->contents);
        if (corpse == NULL) {
                Cprintf(ch, "You can't find a corpse to animate.\n\r");
                return;
        }

        if (!can_loot(ch, corpse))
	{
		Cprintf(ch, "Animating a non-clanner corpse seems like a BAD idea.\n\r");
		return;
	}

  	/* Make us a zombie */
        if(corpse->item_type == ITEM_CORPSE_NPC) {
		if (!get_mob_index(corpse->owner_vnum))
		{
			Cprintf(ch, "You cannot animate that corpse.\n\r");
			return;
		}

        	owner = create_mobile(get_mob_index(corpse->owner_vnum));
		char_to_room(owner, ch->in_room);
		zombie = create_mobile(get_mob_index(MOB_VNUM_ZOMBIE));
		char_to_room(zombie, ch->in_room);

		 /* Set descriptions */
        	sprintf(buf, "animated corpse zombie %s", owner->short_descr);
        	free_string(zombie->name);
        	zombie->name = str_dup(buf);
        	sprintf(buf, "The animated corpse of %s is here.\n\r", owner->short_descr);
        	free_string(zombie->long_descr);
        	zombie->long_descr = str_dup(buf);
        	sprintf(buf, "The corpse of %s", owner->short_descr);
        	free_string(zombie->short_descr);

        	zombie->short_descr = str_dup(buf);
		zombie->level = corpse->level * 5/6;
		// Fix zombie stats
		size_mob(ch, zombie, corpse->level * 5/6);
        	zombie->max_hit = owner->max_hit * 5/6;
		/* for elementals and so on */
		if(zombie->max_hit < 50)
			size_mob(ch, zombie, corpse->level * 5/6);
        	zombie->hit = zombie->max_hit;
		zombie->act |= owner->act;
		zombie->off_flags |= owner->off_flags;
		extract_char(owner, TRUE);
        }
	/*
        else if(corpse->item_type == ITEM_CORPSE_PC) {
        	zombie = create_mobile(get_mob_index(MOB_VNUM_ZOMBIE));
        	char_to_room(zombie, ch->in_room);
			size_mob(ch, zombie, corpse->level * 4/5);
        	sprintf(buf, "animated corpse zombie %s", corpse->owner);
        	free_string(zombie->name);
        	zombie->name = str_dup(buf);
        	sprintf(buf, "The animated corpse of %s is here.\n\r", corpse->owner);
        	free_string(zombie->long_descr);
        	zombie->long_descr = str_dup(buf);
        	sprintf(buf, "The corpse of %s", corpse->owner);
        	free_string(zombie->short_descr);
			zombie->short_descr = str_dup(buf);
        }*/
	else {
                Cprintf(ch, "You cannot animate that.\n\r");
                return;
        }

	if (IS_NPC(zombie) && IS_SET(zombie->act, ACT_AGGRESSIVE))
                REMOVE_BIT(zombie->act, ACT_AGGRESSIVE);

        act("The corpse rises to its feet and bows before $n.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
        Cprintf(ch, "The corpse rises to its feet and bows before you.\n\r");

	/* Get equipment from corpse to zombie */
	for(obj = corpse->contains;obj != NULL;obj = obj_next) {
		obj_next = obj->next_content;
		obj_from_obj(obj);
		obj_to_char(obj, zombie);
	}

	/* size zombie based on original mob */
        zombie->hitroll = zombie->level / 8;
        zombie->armor[0] = 0 - zombie->level * 3;
        zombie->armor[1] = 0 - zombie->level * 3;
        zombie->armor[2] = 0 - zombie->level * 3;
        zombie->armor[3] = 0 - zombie->level * 2;
        zombie->damage[0] = 2;
        zombie->damage[1] = zombie->level / 2;
        zombie->damroll = zombie->level / 2;
        zombie->damage[2] = zombie->level / 2;

        if (zombie->master)
                stop_follower(zombie);
        add_follower(zombie, ch);
        zombie->leader = ch;
		af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = modified_level;
        af.location = 0;
        af.modifier = 0;
        af.bitvector = AFF_CHARM;
        affect_to_char(zombie, &af);

	zombie->rot_timer = modified_level / 4 + number_range(5, 10);
	extract_obj(corpse);
	return;
}

void
spell_wizards_eye(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* first, only one eye at a time */
	if (is_affected(ch, sn))
	{
		Cprintf(ch, "You may only have one floating eye at a time.\n\r");
		return;
	}

	/* now, set up affect, and add to both ppl */
	af.where = TO_AFFECTS;
    	af.type = sn;
    	af.level = modified_level;
    	af.duration = modified_level / 4;
    	af.location = 0;
    	af.modifier = ch->in_room->vnum;
    	af.bitvector = 0;
    	affect_to_char(ch, &af);

	af.modifier = 0;
	affect_to_room(ch->in_room, &af);

	Cprintf(ch, "You create a floating wizards eye.\n\r");
	act("A floating wizards eye flickers into existance.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
spell_embelish(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	/* K, take an item, and up the casting level
	   scrolls, wands, staves */
	OBJ_DATA* obj = (OBJ_DATA*)vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_SET(obj->wear_flags, ITEM_FLAG_EMBELISHED))
	{
		Cprintf(ch, "If you embellish that any further, it will blow up.\n\r");
		return;
	}

	if (obj->item_type != ITEM_SCROLL &&
		obj->item_type != ITEM_WAND &&
		obj->item_type != ITEM_STAFF &&
		obj->item_type != ITEM_POTION)
	{
		Cprintf(ch, "You can't embellish that, its not a potion, scroll, wand, staff.\n\r");
		return;
	}

	if(obj->item_type == ITEM_WAND
	&& obj->value[3] == gsn_voodoo_doll)
	{
		Cprintf(ch, "You cannot embellish voodoo dolls.\n\r");
        	return;
	}

	obj->value[0] += UMIN(60, dice(1, 6) + 1);
	obj->wear_flags |= ITEM_FLAG_EMBELISHED;

	act("You embellish the magic of $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	act("$n embellishes the magic of $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
}

void
spell_beacon(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* first, only one eye at a time */
	if (is_affected(ch, sn))
	{
		Cprintf(ch, "You already have a beacon set.\n\r");
		return;
	}
	if (ch->in_room == NULL
		|| IS_SET(ch->in_room->room_flags, ROOM_SAFE)
		|| IS_SET(ch->in_room->room_flags, ROOM_PRIVATE)
		|| IS_SET(ch->in_room->room_flags, ROOM_SOLITARY)
		|| IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
		|| ch->fighting != NULL
		|| in_own_hall(ch)
		|| in_enemy_hall(ch))
	{
		Cprintf(ch, "You can't set a beacon here.\n\r");
		return;
	}

	/* now, set up affect, and add to both ppl */
	af.where = TO_AFFECTS;
    af.type = gsn_beacon;
    af.level = modified_level;
    af.duration = modified_level / 5;
    af.location = 0;
    af.modifier = ch->in_room->vnum;
    af.bitvector = 0;
    affect_to_char(ch, &af);
    affect_to_room(ch->in_room, &af);

	Cprintf(ch, "You create a recall beacon to this room.\n\r");
	act("$n creates a recall beacon in the room.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
spell_nightmares(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        AFFECT_DATA af;
	CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn)) {
			Cprintf(ch, "They are already stricken with nightmares.\n\r");
			return;
	}

	if (saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_WIS, DAM_MENTAL)) {
		Cprintf(ch, "They resist your nightmares.\n\r");
		return;
	}

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = modified_level;
    af.duration = modified_level / 8;
    if (IS_NPC(victim))
	af.duration *= 2;
    af.location = 0;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    Cprintf(victim, "Terrible nightmares fill your head!\n\r");
    act("$n is stricken down by a bout of nightmares.", victim, NULL, NULL, TO_ROOM, POS_RESTING);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
	damage(ch, victim, dam, sn, DAM_MENTAL, TRUE, TYPE_MAGIC);
}

void
spell_fortify(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
	AFFECT_DATA af;
	CHAR_DATA *victim = (CHAR_DATA*)vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

    	if (is_affected(victim, sn)) {
       		Cprintf(ch, "You refresh the fortification.\n\r");
		affect_refresh(ch, sn, modified_level / 3);
	        return;
    	}

	af.where = TO_AFFECTS;
    af.type = sn;
    af.level = modified_level;
    af.duration = modified_level / 3;
    af.bitvector = 0;

	af.location = APPLY_HITROLL;
    af.modifier = modified_level / 10;
    affect_to_char(victim, &af);

	af.location = APPLY_DAMROLL;
	affect_to_char(victim, &af);

	af.location = APPLY_AC;
    af.modifier = 0 - ((modified_level / 15) * 10);
    affect_to_char(victim, &af);

    Cprintf(victim, "Your strength is bolstered by heavenly power.\n\r");
    act("$n is bolstered by heavenly power.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
spell_replicate(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	/* K, take an item, and up the casting level
	   scrolls, wands, staves */
	OBJ_DATA* obj = (OBJ_DATA*)vo;
	OBJ_DATA* vial;
	OBJ_DATA* copy;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_SET(obj->wear_flags, ITEM_FLAG_REPLICATED))
	{
		Cprintf(ch, "This potion is not suitable for replicating.\n\r");
		return;
	}

	if (obj->item_type != ITEM_POTION)
	{
		Cprintf(ch, "You can only replicate potions.\n\r");
		return;
	}

	vial = get_eq_char(ch, WEAR_HOLD);
	if (vial == NULL
	|| !is_name("vial empty potion _unused_", vial->name))
	{
		Cprintf(ch, "You must be holding an empty vial.\n\r");
		return;
	}

	if (!is_name("_unused_", vial->name))
	{
		Cprintf(ch, "This vial has already been used.\n\r");
		return;
	}

	obj->wear_flags |= ITEM_FLAG_REPLICATED;

	copy = create_object(obj->pIndexData, 0);
	clone_object(obj, copy);
	copy->timer = number_range(1000, 1200);

	if((get_carry_weight(ch) + get_obj_weight(copy)) <= can_carry_w(ch))
	{
		obj_to_char(copy, ch);

		act("You copy the magic of $p into an empty vial.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$n copies the magic of $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
	}
	else
	{
		obj_to_room(copy, ch->in_room);

		act("You copy the magic of $p into an empty vial and place it on the ground.",
		    ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$n copies the magic of $p and places the vial on the ground.",
		    ch, obj, NULL, TO_ROOM, POS_RESTING);
	}
}

void
spell_duplication(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA* mob;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(ch, sn))
	{
		Cprintf(ch, "You may only duplicate your self once.\n\r");
		return;
	}


    if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
    {
            Cprintf(ch, "Not in this room.\n\r");
            return;
    }

	mob = create_mobile(get_mob_index(MOB_VNUM_DUP));
	mob->is_clone = TRUE;;

	size_mob(ch, mob, modified_level / 2);


	free_string(mob->name);
	mob->name = str_dup(ch->name);

	sprintf(buf, "%s%s is here.\n\r", ch->name, ch->pcdata->title);
	free_string(mob->long_descr);
	mob->long_descr = str_dup(buf);

	free_string(mob->short_descr);
	mob->short_descr = str_dup(ch->name);

	char_to_room(mob, ch->in_room);

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = modified_level;
	af.duration  = modified_level / 4;
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = 0;
	affect_to_char(mob, &af);
	affect_to_char(ch, &af);

	Cprintf(ch, "You create your duplicate.\n\r");
	act("$n creates a duplicate of $mself.\n\r", ch, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
end_duplication(void* vo, int target)
{
	CHAR_DATA* ch = (CHAR_DATA*)vo;

	if (IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_DUP)
	{
		act("$n flickers out of view.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		extract_char(ch, TRUE);
	}
}

void
spell_simularcum(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA* victim = (CHAR_DATA*) vo;
	CHAR_DATA* mob;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (max_no_charmies(ch))
	{
		Cprintf(ch, "You can't control so many creatures at once.\n\r");
		return;
	}


    if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
    {
            Cprintf(ch, "Not in this room.\n\r");
            return;
    }

	if (!IS_NPC(victim))
	{
		Cprintf(ch, "That is a bad idea.\n\r");
		return;
	}

	if (is_safe(ch, victim) ||
		victim == ch ||
		IS_SET(victim->imm_flags, IMM_CHARM))
	{
		Cprintf(ch, "You fail.\n\r");
		return;
	}

	mob = create_mobile(victim->pIndexData);
	size_mob(ch, mob, UMIN(mob->level, modified_level));
	SET_BIT(mob->toggles, TOGGLES_NOEXP);
	mob->max_hit = mob->max_hit * 3/2;
	mob->hit = mob->max_hit;

	if (IS_NPC(mob) && IS_SET(mob->act, ACT_AGGRESSIVE))
		REMOVE_BIT(mob->act, ACT_AGGRESSIVE);

	char_to_room(mob, ch->in_room);
	if (mob->master)
		stop_follower(mob);
	add_follower(mob, ch);
	mob->leader = ch;

	mob->rot_timer = modified_level / 6 + number_range(5, 10);

	Cprintf(ch, "A simulacrum appears.\n\r");
	act("$n makes a small copy of $N.", ch, NULL, victim, TO_ROOM, POS_RESTING);

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(mob, &af);
	act("Isn't $n just so nice?", ch, NULL, victim, TO_VICT, POS_RESTING);
}

void
spell_true_sight(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

    if (is_affected(ch, gsn_true_sight))
    {
            Cprintf(ch, "Your vision is already true.\n\r");
            return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = modified_level;
    af.duration = modified_level / 2;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch, &af);
    Cprintf(ch, "Your vision is now true.\n\r");

    return;
}

void
spell_voodoo(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA*) vo;
	OBJ_DATA *doll;
	char buf[MAX_STRING_LENGTH];
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(obj->pIndexData->vnum < OBJ_VNUM_SEVERED_HEAD
	|| obj->pIndexData->vnum > OBJ_VNUM_WING) {
		Cprintf(ch, "You are unable to use voodoo magic on this item.\n\r");
		return;
	}

	/* Make our voodoo doll with all the right info in all
	   the wrong places. */
	if(obj->owner == NULL && obj->owner_vnum == 0) {
		Cprintf(ch ,"You are unable to find a soul.\n\r");
		return;
	}

	doll = create_object(get_obj_index(OBJ_VNUM_VOODOO), 0);
	doll->owner = str_dup(obj->owner);
	doll->owner_vnum = obj->owner_vnum;
	doll->value[3] = gsn_voodoo_doll;
	doll->value[0] = number_range(1, 10);
	sprintf(buf, "voodoo doll %s", doll->owner);
        free_string(doll->name);
        doll->name = str_dup(buf);
	sprintf(buf, "a doll in the shape of %s", doll->owner);
        free_string(doll->short_descr);
        doll->short_descr = str_dup(buf);
	extract_obj(obj);
	doll->timer = number_range(2000, 2400);
	obj_to_char(doll, ch);

	Cprintf(ch, "You create a voodoo doll linked to %s.\n\r", doll->owner);
}

void
spell_wildfire(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA* victim = (CHAR_DATA*)vo;
	OBJ_DATA *obj_lose, *obj_next;
	AFFECT_DATA* paf1;
	AFFECT_DATA* paf2;
	AFFECT_DATA* paf3;
	AFFECT_DATA* paf;
	int dam = 0;
	bool fail = TRUE;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_CON, DAM_FIRE)) {
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	for (obj_lose = victim->carrying;
		 obj_lose != NULL;
		 obj_lose = obj_next)
	{
		obj_next = obj_lose->next_content;

		paf1 = affect_find(obj_lose->affected, gsn_bless);
		paf2 = affect_find(obj_lose->affected, gsn_fireproof);
		paf3 = affect_find(obj_lose->affected, gsn_curse);

		if (paf1 || paf2 || paf3)
		{
			act("$p glows bright purple and returns to normal!", victim, obj_lose, NULL, TO_ALL, POS_RESTING);

			if (paf1 != NULL) {
				while((paf = affect_find(obj_lose->affected, gsn_bless)) != NULL)
					affect_remove_obj(obj_lose, paf);
				dam += (obj_lose->level) / 2;
			}
			if (paf2 != NULL) {
				affect_remove_obj(obj_lose, paf2);
				dam += (obj_lose->level) / 2;
			}
			if (paf3 != NULL) {
				while((paf = affect_find(obj_lose->affected, gsn_curse)) != NULL)
					affect_remove_obj(obj_lose, paf);
				dam += (obj_lose->level) / 2;
			}
			fail = FALSE;
		}
	}

	if (fail)
	{
		Cprintf(ch, "Your spell had no effect.\n\r");
		Cprintf(victim, "You feel your equipment momentarily tingle.\n\r");
	}
	else
		damage(ch, victim, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);

	return;
}

void
spell_sharpen(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA* obj = (OBJ_DATA*)vo;
	AFFECT_DATA* paf;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	paf = affect_find(obj->affected, sn);
	if (paf != NULL)
	{
		Cprintf(ch, "That weapon is already sharpened.\n\r");
		return;
	}

	if (obj->item_type != ITEM_WEAPON)
	{
		Cprintf(ch, "And just how do you expect to sharpen that?\n\r");
		return;
	}

	af.where = TO_OBJECT;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 2;
	af.bitvector = 0;
	af.location = APPLY_DAMROLL;
	af.modifier = modified_level / 10;
	affect_to_obj(obj, &af);

	af.location = APPLY_HITROLL;
	affect_to_obj(obj, &af);

	act("$p sharpens to a bloodthirsty edge.", ch, obj, NULL, TO_ALL, POS_RESTING);

	return;
}

void
spell_symbol(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA*)vo;
	AFFECT_DATA af;
	int runetype;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (obj_is_affected(obj, gsn_symbol))
	{
		act("$p already bears a magic symbol.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		return;
	}

	act("$p now bears a symbol of warding.", ch, obj, NULL, TO_ALL, POS_RESTING);
	runetype = number_range(1,4);

	switch(runetype)
	{
	case 1:
		Cprintf(ch, "The symbol burns with fire.\n\r"); break;
	case 2:
		Cprintf(ch, "The symbol turns pitch black.\n\r"); break;
	case 3:
		Cprintf(ch, "The symbol turns to shining ice.\n\r"); break;
	case 4:
		Cprintf(ch, "The symbol turns a pale green.\n\r"); break;
	}

	af.where = TO_OBJECT;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level;
	af.modifier = runetype;
	af.location = APPLY_NONE;
	af.bitvector = 0;
	affect_to_obj(obj, &af);
	obj->owner = str_dup(ch->name);
	return;
}

void
spell_create_oil(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
	OBJ_DATA *obj;
	int flavour;
	char oname[MAX_STRING_LENGTH];
	char shortd[MAX_STRING_LENGTH];
	char longd[MAX_STRING_LENGTH];
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	obj = create_object(get_obj_index(OBJ_VNUM_OIL_BOMB), 0);

	flavour = number_range(1, 6);
	free_string(obj->short_descr);
	free_string(obj->name);
	free_string(obj->description);

	switch(flavour)
	{
		case 1:
			obj->value[4] = gsn_pyrotechnics;
			sprintf(oname, "greek fire oil flask");
			sprintf(shortd, "a flask of greek fire");
			sprintf(longd, "A flask of burning oil lies here.");
			break;
		case 2:
			obj->value[4] = gsn_acid_blast;
			sprintf(oname, "vial annointing oil acid");
			sprintf(shortd, "a vial of annointing oil");
			sprintf(longd, "A vial of annointing oil lies here.");
			break;
		case 3:
			obj->value[4] = gsn_ice_bolt;
			sprintf(oname, "chilled holy wine cold oil");
			sprintf(shortd, "a philter of cold wine");
			sprintf(longd, "A philter of icy wine rests here.");
			break;
		case 4:
			obj->value[4] = gsn_blast_of_rot;
			sprintf(oname, "block incense poison oil");
			sprintf(shortd, "a block of incense");
			sprintf(longd, "A small cube of incense sits here.");
			break;
		case 5:
			obj->value[4] = gsn_tsunami;
			sprintf(oname, "jar holy water oil");
			sprintf(shortd, "a jar of blessed holy water");
			sprintf(longd, "A jar of holy water sits here.");
			break;
		case 6:
			obj->value[4] = gsn_lightning_bolt;
			sprintf(oname, "host bread shock oil");
			sprintf(shortd, "a small piece of the host");
			sprintf(longd, "A deadly slice of bread lies here.");
			break;
	}
	obj->name = str_dup(oname);
	obj->short_descr = str_dup(shortd);
	obj->description = str_dup(longd);
	obj->value[0] = ch->level / 10;
	obj->value[1] = ch->level / 6;
	obj->value[3] = ch->level * 2/3;
	obj->level = ch->level * 2/3;
	obj->timer = number_range(1000, 1200);

	if((ch->carry_number < can_carry_n(ch)) && ((get_carry_weight(ch) + get_obj_weight(obj)) <= can_carry_w(ch)))
	{
        	obj_to_char(obj, ch);
        	act("$n mutters a bit and creates $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
        	act("You utter a few words and create $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	}
	else
	{
		obj_to_room(obj, ch->in_room);
		act("$n mutters a bit and places $p carefully on the ground.",
		    ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You utter a few words and carefully place $p on the ground.",
		    ch, obj, NULL, TO_CHAR, POS_RESTING);
	}
	return;
}

void
spell_drain_life(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_HIGH, TRUE);

	if(!is_affected(ch, gsn_dissolution))
		ch->hit += dam / 3;

	Cprintf(victim, "You feel your life essense draining away!\n\r");
	Cprintf(ch, "You drain away some life force!\n\r");
	damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE, TYPE_MAGIC);
	return;
}

void
spell_channel_energy(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_HIGH, TRUE);

	act("You channel a blast of crackling blue energy at $N!", ch, NULL, victim, TO_CHAR, POS_RESTING);
	act("$n channels a blast of crackling blue energy at you!", ch, NULL, victim, TO_VICT, POS_RESTING);
	act("$n channels a blast of crackling blue energy at $N!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);

	/* K, damage done, msg sent, check for mini-dispel */
	/* note, no msg here */
	damage(ch, victim, dam, sn, DAM_NONE, TRUE, TYPE_MAGIC);

	level = generate_int(caster_level * 3 / 4, modified_level * 3 / 4);

	spell_dispel_magic(gsn_dispel_magic, level, ch, (void *)victim, TARGET_CHAR);

	return;
}

void spell_lesser_dispel(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA*)vo;

	if(!IS_NPC(victim)) {
		Cprintf(ch, "This spell doesn't work on players.\n\r");
		return;
	}

	spell_dispel_magic(sn, level, ch, vo, target);

	return;
}

void
spell_concealment(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	if (is_affected(ch, sn))
       	{
                Cprintf(ch, "You are already conceiled.\n\r");
                return;
        }

	Cprintf(ch, "You conceal yourself in the shadows.\n\r");
	act("$N fades into the shadows.", ch, NULL, ch, TO_ROOM, POS_RESTING);

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = caster_level;
        af.duration = 0;
        af.modifier = 5;
        af.location = APPLY_HITROLL;
        af.bitvector = 0;
        affect_to_char(ch, &af);

	return;
}

void
spell_quill_armor(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	if (is_affected(ch, sn))
        {
                Cprintf(ch, "Your refresh your quill armor.\n\r");
                affect_refresh(ch, sn, modified_level / 4);
                return;
        }

	Cprintf(ch, "Your furr sharpens into pointy quills.\n\r");
	Cprintf(ch, "You are sheathed in gleaming quill armor!\n\r");
	act("$N is sheathed in pointed quills.", ch, NULL, ch, TO_ROOM, POS_RESTING);

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = caster_level;
        af.duration = modified_level / 4;
        af.modifier = 0 - ((modified_level * 3 / 4) + 2);
        af.location = APPLY_AC;
        af.bitvector = 0;
        affect_to_char(ch, &af);

	return;
}

void
spell_alarm_rune(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	if (is_affected(ch, gsn_alarm_rune))
	{
		Cprintf(ch, "You already sense others.\n\r");
		return;
	}

        if (room_is_affected (ch->in_room, gsn_alarm_rune))
        {
                Cprintf(ch, "This room is already being sensed.\n\r");
                return;
        }

        Cprintf(ch, "You place an alarm rune on the ground, increasing your senses.\n\r");
        act("$n places a strange rune on the ground.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = ch->level;
        af.duration  = (ch->level / 3);
	af.modifier  = ch->in_room->vnum;
        af.location  = APPLY_NONE;
        af.bitvector = 0;
        affect_to_room (ch->in_room, &af);
	affect_to_char (ch, &af);

        return;
}

void end_alarm_rune(void *vo, int target) {
        CHAR_DATA *ch = NULL;
        ROOM_INDEX_DATA *room = NULL;

        if(target == TARGET_CHAR) {
                ch = (CHAR_DATA*)vo;
                Cprintf(ch, "Your connection with the alarm rune is broken.\n\r");
                return;
        }

        if(target == TARGET_ROOM) {
                room = (ROOM_INDEX_DATA*)vo;
                for(ch = room->people;ch != NULL; ch = ch->next_in_room) {
                 Cprintf(ch, "The rune of warding on this room vanishes.\n\r");
                }
                return;
        }

        return;
}

void
spell_balance_rune(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;
	int dam = 0;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if(obj->item_type != ITEM_WEAPON
	&& obj->item_type != ITEM_ARMOR) {
                Cprintf(ch, "Only weapons and armor can be empowered with balance runes.\n\r");
                return;
        }

        if(obj_is_affected(obj, gsn_balance_rune)) {
                Cprintf(ch, "The object has already been balanced.\n\r");
                return;
        }

	if(number_percent() < 25) {
		act("$p shivers violently and explodes!", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$p shivers violently and explodes!", ch, obj, NULL, TO_ROOM, POS_RESTING);
		if (IS_SET(ch->toggles, TOGGLES_SOUND))
        		Cprintf(ch, "!!SOUND(sounds/wav/enchblow.wav V=80 P=20 T=admin)");
		dam = dice(ch->level, 6);
		damage(ch, ch, dam, sn, DAM_SLASH, TRUE, TYPE_MAGIC);

		extract_obj(obj);
		return;
	}

	Cprintf(ch, "You protect the object with a balance rune!\n\r");
        act("$n empowers $p with a balance rune.", ch, obj, NULL, TO_ROOM, POS_RESTING);

        af.where = TO_OBJECT;
        af.type = sn;
        af.level = ch->level;
        af.duration = 1;
	af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;
        affect_to_obj(obj, &af);

        return;
}

void
spell_blade_rune(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
 	OBJ_DATA *obj = (OBJ_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;
        int bladetype=0;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if(obj->item_type != ITEM_WEAPON) {
                Cprintf(ch, "Only weapons can be empowered with blade runes.\n\r");
                return;
        }

        if(obj_is_affected(obj, gsn_blade_rune)) {
                Cprintf(ch, "The existing blade rune repels your magic.\n\r");
                return;
        }
	Cprintf(ch, "You empower the weapon with a blade rune!\n\r");
        act("$n empowers $p with a blade rune.", ch, obj, NULL, TO_ROOM, POS_RESTING);

	af.where = TO_OBJECT;
        af.type = sn;
        af.level = ch->level;
        af.duration = ch->level / 2;
        af.bitvector = 0;

        bladetype = number_range(1, 6);
        switch(bladetype) {
                case 1:
                        Cprintf(ch, "The weapon begins to move faster.\n\r");
		        af.location = APPLY_NONE;
                        af.modifier = BLADE_RUNE_SPEED;
			affect_to_obj(obj, &af); break;
                case 2:
                        Cprintf(ch, "The weapon becomes armor-piercing.\n\r");
		        af.location = APPLY_HITROLL;
                        af.modifier = 20;
			affect_to_obj(obj, &af); break;
                case 3:
                        Cprintf(ch, "The weapon will deflect incoming attacks.\n\r");
		        af.location = APPLY_NONE;
                        af.modifier = BLADE_RUNE_PARRYING;
			affect_to_obj(obj, &af); break;
                case 4:
                        Cprintf(ch, "The weapon becomes more accurate.\n\r");
		        af.location = APPLY_NONE;
                        af.modifier = BLADE_RUNE_ACCURACY;
			affect_to_obj(obj, &af); break;
                case 5:
                        Cprintf(ch, "The weapon surrounds you with a glowing aura.\n\r");
			af.location = APPLY_AC;
                        af.modifier = 0 - UMIN(60, 20 + obj->level);
			affect_to_obj(obj, &af);
			break;
                case 6:
       			Cprintf(ch, "The weapon is endowed with killing dweomers.\n\r");
			af.location = APPLY_DAMROLL;
                        af.modifier = 10;
			affect_to_obj(obj, &af);
			if(!IS_WEAPON_STAT(obj, WEAPON_VORPAL)) {
				af.where = TO_WEAPON;
                        	af.type = gsn_blade_rune;
                        	af.level = obj->level;
                        	af.location = APPLY_NONE;
                        	af.modifier = -1;
                        	af.bitvector = WEAPON_VORPAL;
                        	affect_to_obj(obj, &af);
			}
			break;
        }

        return;
}

void
spell_burst_rune(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
  	OBJ_DATA *obj = (OBJ_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;
	int burst_sn=0;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	if(obj->item_type != ITEM_WEAPON) {
		Cprintf(ch, "Only weapons can be empowered with burst runes.\n\r");
		return;
	}

        if(obj_is_affected(obj, gsn_burst_rune)) {
		Cprintf(ch, "The existing burst rune repels your magic.\n\r");
		return;
        }

	Cprintf(ch, "You empower the weapon with an elemental burst rune!\n\r");
	act("$n empowers $p with an elemental burst rune.", ch, obj, NULL, TO_ROOM, POS_RESTING);

	burst_sn = number_range(1, 6);
	switch(burst_sn) {
		case 1:
			Cprintf(ch, "A {Ggreen{x and {Bblue{x rune appears.\n\r");
			af.modifier = gsn_acid_blast; break;
		case 2:
			Cprintf(ch, "A {Bblue{x and {Wwhite{x rune appears.\n\r");
			af.modifier = gsn_ice_bolt; break;
		case 3:
			Cprintf(ch, "A {Ggreen{x and {Dblack{x rune appears.\n\r");
			af.modifier = gsn_blast_of_rot; break;
		case 4:
			Cprintf(ch, "A {Rred{x and {Wwhite{x rune appears.\n\r");
			af.modifier = gsn_flamestrike; break;
		case 5:
			Cprintf(ch, "A {Ygold{x and {Bblue{x rune appears.\n\r");
			af.modifier = gsn_lightning_bolt; break;
		case 6:
			Cprintf(ch, "A {Dblack{x and {Bblue{x rune appears.\n\r");
			af.modifier = gsn_tsunami; break;
	}

	af.where = TO_OBJECT;
        af.type = sn;
        af.level = ch->level;
        af.duration = ch->level / 2;
        af.location = APPLY_NONE;
        af.bitvector = 0;
        affect_to_obj(obj, &af);

	return;
}

void
spell_fire_rune(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (room_is_affected (ch->in_room, gsn_fire_rune))
        {
                Cprintf(ch, "This room is already affected by the power of flames.\n\r");
                return;
        }

        if(!IS_NPC(ch) && is_affected(ch, gsn_pacifism))
	{
                Cprintf(ch, "The spirit of Bhaal prevents you from casting this spell.\n\r");
                return;
        }

        Cprintf(ch, "You place a fiery rune on the ground to singe your foes.\n\r");
        act("$n places a strange rune on the ground.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = ch->level;
        af.duration  = (ch->level / 6);
        af.modifier  = 0;
        af.location  = APPLY_NONE;
        af.bitvector = ROOM_AFF_FIRE_RUNE;
        affect_to_room (ch->in_room, &af);

        return;
}

void end_fire_rune(void *vo, int target) {
	CHAR_DATA *ch = NULL;
	ROOM_INDEX_DATA *room = NULL;

	if(target == TARGET_CHAR) {
		ch = (CHAR_DATA*)vo;
		Cprintf(ch, "Your burn marks heal.\n\r");
		return;
	}

	if(target == TARGET_ROOM) {
		room = (ROOM_INDEX_DATA*)vo;
		for(ch = room->people;ch != NULL; ch = ch->next_in_room) {
			Cprintf(ch, "The rune of warding on this room vanishes.\n\r");
		}
		return;
	}

	return;
}

void
spell_shackle_rune(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (room_is_affected (ch->in_room, gsn_shackle_rune))
        {
        	Cprintf(ch, "This room is already affected by the restriction of movement.\n\r");
        	return;
        }

        if(!IS_NPC(ch) && is_affected(ch, gsn_pacifism))
 	{
                Cprintf(ch, "The spirit of Bhaal prevents you from casting this spell.\n\r");
   		return;
        }

        Cprintf(ch, "You place a shackle on the ground preventing easy movement.\n\r");
        act("$n places a strange rune on the ground.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = ch->level;
        af.duration  = (ch->level / 6);
        af.modifier  = 0;
        af.location  = APPLY_NONE;
        af.bitvector = ROOM_AFF_SHACKLE_RUNE;
        affect_to_room (ch->in_room, &af);

        return;
}


void end_shackle_rune(void *vo, int target) {
        CHAR_DATA *ch = NULL;
        ROOM_INDEX_DATA *room = NULL;

        if(target == TARGET_CHAR) {
                ch = (CHAR_DATA*)vo;
                Cprintf(ch, "You feel less restricted in movement.\n\r");
                return;
        }

        if(target == TARGET_ROOM) {
                room = (ROOM_INDEX_DATA*)vo;
                for(ch = room->people;ch != NULL; ch = ch->next_in_room) {
                        Cprintf(ch, "The rune of warding on this room vanishes.\n\r");
                }
                return;
        }

        return;
}

void
spell_soul_rune(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	if(obj->item_type == ITEM_CONTAINER) {
		Cprintf(ch, "You can't place a soul rune on containers.\n\r");
		return;
	}
	if(obj->pIndexData->vnum == OBJ_VNUM_BANK_NOTE) {
		Cprintf(ch, "You can't place a soul rune on a bank note.\n\r");
		return;
	}
       	if(obj_is_affected(obj, gsn_soul_rune)) {
                Cprintf(ch, "The object is already transparent.\n\r");
                return;
        }

        Cprintf(ch, "You bind the object to your soul.\n\r");
        Cprintf(ch, "%s turns ghostly and transparent.\n\r", obj->short_descr);
        act("$p turns ghostly and transparent.", ch, obj, NULL, TO_ROOM, POS_RESTING);

        af.where = TO_OBJECT;
        af.type = sn;
        af.level = ch->level;
        af.duration = ch->level;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;
        affect_to_obj(obj, &af);

	return;
}

void
spell_wizard_mark(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA af;
	char newname[MAX_INPUT_LENGTH];

	/* Already marked */
	if (str_cmp(obj->name, obj->pIndexData->name))
	{
		Cprintf(ch, "You cannot wizard mark restrung items.\n\r");
		return;
	}

	/* Remove the NOLOCATE flag */
	if (IS_SET(obj->extra_flags, ITEM_NOLOCATE))
	{
		REMOVE_BIT(obj->extra_flags, ITEM_NOLOCATE);
	}

	/* Set person's name in the object */
	sprintf(newname, "%s %s", obj->name, ch->name);
        free_string(obj->name);
        obj->name = str_dup(newname);

	af.where     = TO_OBJECT;
	af.type      = sn;
	af.level     = obj->level;
	af.duration  = -1;
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = 0;
	affect_to_obj (obj, &af);

	Cprintf(ch, "You mark the object with your name.\n\r");
        act("$n marks $p with their name.", ch, obj, NULL, TO_ROOM, POS_RESTING);

	return;
}

// Ok, I know paint power isn't a spell, but this will still work.
// Use this when animated tattoo golems vanish.
void end_paint_power(void *vo, int target) {
	CHAR_DATA* ch;

	if(target == TARGET_CHAR) {
		ch = (CHAR_DATA*)vo;
		stop_follower(ch);
		act("$n seems to be losing cohesion...", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		ch->rot_timer = 2;
	}
}

void
spell_destroy_tattoo(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = NULL;
	int dam = 0;
	int modified_level = ch->level;
	CHAR_DATA *fch = NULL, *fch_next = NULL;
	AFFECT_DATA *paf = NULL;

	if(target_name[0] == '\0') {
		Cprintf(ch, "You need to target a tattoo you are wearing.\n\r");
		return;
	}

	obj = get_obj_list(ch, target_name, ch->carrying);
	if(obj == NULL) {
		Cprintf(ch, "You aren't wearing that.\n\r");
		return;
	}

	if(obj->pIndexData->vnum != OBJ_VNUM_NEW_TATTOO) {
		Cprintf(ch, "That object isn't a tattoo. Nice try.\n\r");
		return;
	}

	Cprintf(ch, "You focus your will and %s explodes into flames!\n\r", obj->short_descr);

	if((paf = affect_find(obj->affected, gsn_paint_power)) != NULL) {
		if(paf->modifier == TATTOO_ANIMATES) {
			for(fch = char_list; fch != NULL; fch = fch_next) {
				fch_next = fch->next;
 				if (fch->master == ch && IS_NPC(fch)
            			&& is_affected(fch, gsn_paint_power)) {
					extract_char(fch, FALSE);
                			return;
            			}
			}
		}
	}

	unequip_char(ch, obj);
	extract_obj(obj);
	dam = spell_damage(ch, ch, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
	damage(ch, ch, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);

	return;
}


void
spell_destroy_rune(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        OBJ_DATA *obj = NULL;
        int dam = 0;
        int modified_level = ch->level;
	int found = 0, chance = 0;
        CHAR_DATA *vch = NULL, *vch_next = NULL;
        AFFECT_DATA *paf = NULL;
	AFFECT_DATA *paf_next = NULL;
	// Okay, we can have several targets

	// Target room if left blank
        if(target_name[0] == '\0') {
		for(paf = ch->in_room->affected; paf != NULL; paf = paf_next) {
			paf_next = paf->next;
			if(paf->type == gsn_fire_rune
			|| paf->type == gsn_alarm_rune
			|| paf->type == gsn_shackle_rune) {
				chance = (modified_level * 2) - paf->level + 25;
				if(number_percent() < chance) {
					found = found + 1;
					if(paf->type == gsn_alarm_rune)
						affect_strip(ch, gsn_alarm_rune);
					affect_remove_room(ch->in_room, paf);
				}
			}
		}

		if(found) {
			act("Runes glow bright red and suddenly EXPLODE!", ch, NULL, NULL, TO_ALL, POS_RESTING);

			for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
        		{
		                vch_next = vch->next_in_room;

				if (is_safe(ch, vch))
                        		continue;

		                dam = spell_damage(ch, vch, modified_level, SPELL_DAMAGE_HIGH, TRUE);
                		damage(ch, vch, dam, sn, DAM_ENERGY, TRUE, TYPE_MAGIC);
		        }
	        }

		if(!found) {
			Cprintf(ch, "You failed.\n\r");
			return;
		}
		return;
	}

	// Otherwise, target item.
        obj = get_obj_list(ch, target_name, ch->carrying);
        if(obj == NULL) {
                Cprintf(ch, "You must target an item you are carrying.\n\r");
                return;
        }

	for(paf = obj->affected; paf != NULL; paf = paf_next) {
		paf_next = paf->next;
		if(paf->type == gsn_wizard_mark
		|| paf->type == gsn_burst_rune
		|| paf->type == gsn_blade_rune
		|| paf->type == gsn_soul_rune) {
			found = TRUE;
			if(paf->type == gsn_wizard_mark) {
				free_string(obj->name);
				obj->name = str_dup(obj->pIndexData->name);
			}
			affect_remove_obj(obj, paf);
		}
	}

	if(found) {
		act("The runes on $p slowly fade out of existence.", ch, obj, NULL, TO_ALL, POS_RESTING);
		return;
        }

	if(!found) {
		Cprintf(ch, "You failed.\n\r");
		return;
	}

}


int soul_blade_table[9][5] = {
              {2, 8,          40, 0,        1},
              {3, 6,          50, 0,        2},
              {5, 4,          60, 0,        3},
              {4, 6,          70, 0,        4},
              {4, 7,          80, 10,       4},
              {4, 8,          90, 15,       4},
              {8, 4,          100, 20,      4},
              {4, 10,         100, 20,      5},
              {7, 6,          100, 25,      5},
};

void spell_soul_blade(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        AFFECT_DATA af;
        int caster_level, modified_level;
	int max_skill = 0, class = 0;
	char buf[255];
	OBJ_DATA *blade;
	int index = 0, flag1, flag2;
	int wskills[9];
	int i = 0;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	// Collect all your weapon skills
	wskills[1] = ch->pcdata->learned[gsn_sword];
	wskills[2] = ch->pcdata->learned[gsn_dagger];
	wskills[3] = ch->pcdata->learned[gsn_spear];
	wskills[4] = ch->pcdata->learned[gsn_mace];
	wskills[5] = ch->pcdata->learned[gsn_axe];
	wskills[6] = ch->pcdata->learned[gsn_flail];
	wskills[7] = ch->pcdata->learned[gsn_whip];
	wskills[8] = ch->pcdata->learned[gsn_polearm];

	// Find the maximum skill
	max_skill = wskills[1];
	for(i = 2; i < 9; i++) {
		if(wskills[i] > max_skill)
			max_skill = wskills[i];
	}

	// Randomly pick one of the maximum skills
	while(1) {
		class = number_range(1, 8);
		if(wskills[class] < max_skill)
			continue;
		else
			break;
	}

	// index into the soul blade table based on level.
	index = (ch->level - 16) / 4;
	if(index > 8)
		index = 8;

	blade = create_object(get_obj_index(OBJ_VNUM_MAGICAL_SWORD), ch->level);
	blade->level = ch->level;
	blade->timer = ch->level / 2;
	blade->value[0] = class;
	blade->value[1] = soul_blade_table[index][0];
	blade->value[2] = soul_blade_table[index][1];
	blade->value[3] = hit_lookup("psi");
	blade->value[4] = 0;

	if(number_percent() < soul_blade_table[index][2]) {
		flag1 = weapon_type2[number_range(0, 15)].bit;
		blade->value[4] |= flag1;
	}
	if(number_percent() < soul_blade_table[index][3]) {
		flag2 = weapon_type2[number_range(0, 15)].bit;
		if(flag1 != flag2)
			blade->value[4] |= flag2;
	}

  	af.where = TO_OBJECT;
        af.type = gsn_soul_blade;
        af.level = modified_level;
        af.duration = -1;
        af.modifier = soul_blade_table[index][4];
        af.location = APPLY_HITROLL;
        af.bitvector = 0;
        affect_to_obj(blade, &af);

        af.location = APPLY_DAMROLL;
        affect_to_obj(blade, &af);

	free_string(blade->name);
	free_string(blade->short_descr);
	free_string(blade->description);

	sprintf(buf, "wave psionic energy %s",
		weapon_class[class].name);
	blade->name = str_dup(buf);

        sprintf(buf, "a wave of psionic energy");
        blade->short_descr = str_dup(buf);

	sprintf(buf, "A wave of pure psionic energy lies here, ready to be wielded.\n\r");
	blade->description = str_dup(buf);

        Cprintf(ch, "You draw a shimmering weapon of force from thin air.\n\r");
        act("$n draws a psionic wave from thin air.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	obj_to_char(blade, ch);
	wear_obj(ch, blade, TRUE);

        return;
}

void spell_death_rune(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	if(ch->pktimer > 0) {
		Cprintf(ch, "You're not ready to die again just yet.\n\r");			return;
 	}

        if (is_affected(ch, gsn_death_rune))
        {
                Cprintf(ch, "You refresh your rune of death.\n\r");
                affect_refresh(ch, sn, ch->level / 3);
                return;
        }

  	af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = ch->level / 3;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;

        affect_to_char(ch, &af);

        Cprintf(ch, "You scribe a rune of death on your chest.\n\r");
        act("$n is marked by a rune of death.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

        return;
}

void spell_animal_skins(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

        if (is_affected(ch, sn))
        {
                Cprintf(ch, "You are already protected by animal skins.\n\r");
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = modified_level;
        af.location = APPLY_AC;
        af.modifier = -40;
        af.bitvector = 0;
        affect_to_char(ch, &af);

	af.where = TO_RESIST;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = RES_PIERCE;
        affect_to_char(ch, &af);

        act("$n is protected by tough animal hides.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
        Cprintf(ch, "You gird yourself with thick animal skins.\n\r");
        return;
}

void
spell_ignore_wounds(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	if(is_affected(ch, gsn_ignore_wounds)) {
		Cprintf(ch, "You've already forgotten about pain.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = modified_level / 3;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;

        affect_to_char(victim, &af);

        Cprintf(victim, "You close your eyes and forget about pain.\n\r");
	act("$n closes their eyes and forgets about pain.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

        return;
}


void
clanspell_paradox(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	char buf[255];
	CHAR_DATA *victim = (CHAR_DATA*)vo;
	CHAR_DATA *mob;
	AFFECT_DATA *paf, af;
	OBJ_DATA *obj = NULL, *copy = NULL;

	// Make a close copy of the player
	mob = create_mobile(get_mob_index(MOB_VNUM_DUP));
	SET_BIT(mob->toggles, TOGGLES_NOEXP);

	// Set up some defaults
	mob->is_clone = TRUE;
	size_mob(ch, mob, ch->level);

	// String it nicely
	free_string(mob->name);
	mob->name = str_dup(ch->name);

	sprintf(buf, "%s%s is here.\n\r", ch->name, ch->pcdata->title);
	free_string(mob->long_descr);
	mob->long_descr = str_dup(buf);

	free_string(mob->short_descr);
	mob->short_descr = str_dup(ch->name);

	// Copy basic stats
	mob->max_hit = ch->max_hit;
	mob->hit = mob->max_hit;
	mob->hitroll = ch->hitroll * 4 / 5;
	mob->damroll = ch->damroll * 4 / 5;
	mob->rot_timer = 5;

	// Copy spell affects
	for(paf = ch->affected; paf != NULL; paf = paf->next) {
		af.where = TO_AFFECTS;
		af.type = paf->type;
		af.level = paf->level;
		af.duration = paf->duration;
		af.location = paf->location;
		af.modifier = paf->modifier;
		af.bitvector = paf->bitvector;
		affect_to_char(mob, &af);
	}

	// Set act flags
	if(ch->charClass <= class_lookup("invoker"))
		mob->spec_fun = spec_lookup("spec_cast_mage");
	else if(ch->charClass <= class_lookup("druid"))
		mob->spec_fun = spec_lookup("spec_cast_mage");
	else if(ch->charClass <= class_lookup("ranger"))
		SET_BIT(mob->act, ACT_WARRIOR);
	else if(ch->charClass <= class_lookup("runist"))
		SET_BIT(mob->act, ACT_THIEF);

	// Give it a weapon (temporarily)
	obj = get_eq_char(ch, WEAR_WIELD);
	if(obj != NULL) {
		copy = create_object(obj->pIndexData, 0);
		clone_object(obj, copy);
		obj_to_char(copy, mob);
		equip_char(mob, copy, WEAR_WIELD);
		SET_BIT(copy->extra_flags, ITEM_ROT_DEATH);
	}

	// Makes the mob show up on where.
	af.where     = TO_AFFECTS;
	af.type      = gsn_duplicate;
	af.level     = ch->level;
	af.duration  = ch->level;
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = 0;
	affect_to_char(mob, &af);

	af.type	     = gsn_paradox;
	af.duration  = 5;
        affect_to_char(ch, &af);

	// And set it to go go go
	char_to_room(mob, ch->in_room);

	mob->hunting = victim;
	mob->hunt_timer = 30;

	Cprintf(ch, "A dark vortex appears and %s steps through, bowing before you.\n\r", ch->name);


}

void corrupt_obj(CHAR_DATA *caster, OBJ_DATA *obj) {
	AFFECT_DATA af;

	if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
        	return;

	// Ruin food, potions, pills and drinks
	switch(obj->item_type) {
	case ITEM_FOOD:
	case ITEM_DRINK_CON:
		af.where = TO_AFFECTS;
        	af.type = gsn_corruption;
        	af.level = caster->level + 4;
        	af.duration = -1;
        	af.modifier = 0;
        	af.location = APPLY_NONE;
        	af.bitvector = 0;
        	affect_to_obj(obj, &af);
		break;
	case ITEM_FOUNTAIN:
		af.where = TO_AFFECTS;
		af.type = gsn_corruption;
		af.level = caster->level + 4;
		af.duration = caster->level / 5;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = 0;
		affect_to_obj(obj, &af);
		break;
	case ITEM_POTION:
	case ITEM_PILL:
		obj->value[0] = caster->level + 4;
		obj->value[1] = gsn_corruption;
		obj->value[2] = 0;
		obj->value[3] = 0;
		obj->value[4] = 0;
		break;
	}

	return;
}

void
spell_corruption(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = NULL;
        OBJ_DATA *obj = NULL, *obj_in;
        int dam = 0;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	if(target == TARGET_OBJ) {
		obj = (OBJ_DATA *)vo;

		if(obj->item_type != ITEM_POTION
		&& obj->item_type != ITEM_PILL
		&& obj->item_type != ITEM_FOOD
		&& obj->item_type != ITEM_DRINK_CON
		&& obj->item_type != ITEM_FOUNTAIN) {
			Cprintf(ch, "You can't corrupt this type of object.\n\r");
			return;
		}

		Cprintf(ch, "You taint %s with a vile corruption.\n\r", obj->short_descr);
		act("$n corrupts $p with an evil grin.\n\r", ch, obj, NULL, TO_ROOM, POS_RESTING);
		corrupt_obj(ch, obj);
		return;
	}

	victim = (CHAR_DATA *)vo;

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
	damage(ch, victim, dam, sn, DAM_DISEASE, TRUE, TYPE_MAGIC);

        if (!saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_CON, DAM_DISEASE))
        {
		act("$n's victuals become tainted with corruption!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(victim, "Your victuals become tainted!\n\r");
                for (obj = victim->carrying; obj != NULL; obj = obj->next_content)
                {
			corrupt_obj(ch, obj);
			if(obj->contains) {
				for(obj_in = obj->contains; obj_in != NULL; obj_in = obj_in->next_content)
					corrupt_obj(ch, obj);
			}

		}
	}

}

void
spell_dissolution(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(victim, gsn_dissolution))
        {
                act("$e is already being dissolved.", ch, NULL, victim, TO_CHAR, POS_RESTING);
                return;
        }

        if (saving_throw(ch, victim, gsn_dissolution, caster_level, SAVE_HARD, STAT_CON, DAM_ACID))
        {
	    Cprintf(ch, "Your spell has no effect.\n\r");
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.location = APPLY_AC;
        af.modifier = modified_level;
        af.duration = 6;
	if (IS_NPC(victim))
	    af.duration *= 2;
        af.bitvector = 0;

        affect_to_char(victim, &af);
        Cprintf(victim, "Your flesh starts to melt from your bones!\n\r");
        act("$n begins to be dissolved with a wet splosh!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
        return;
}

void
spell_flood( int sn, int level, CHAR_DATA *ch, void *vo, int target ) {
    AFFECT_DATA af;
    AFFECT_DATA *flood = get_room_affect(ch->in_room, gsn_flood);

    int caster_level = get_caster_level(level);
	int modified_level = get_modified_level(level);
	int spellDuration = (caster_level / 6) + 3;

    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE) || in_enemy_hall(ch)) {
        Cprintf(ch, "Not in this room.\n\r");
        return;
    }

    // If the room is affected by an NC flood, we'll turn it into a clanner flood,
    // and set its duration to be the greater of either its current duration,
    // or the duration that would be given to the "new" flood.
    if (flood != NULL && !flood->clan && ch->clan) {
        flood->clan = ch->clan;
        flood->duration = UMAX(flood->duration, spellDuration);
    } else if (ch->in_room->sector_type == SECT_UNDERWATER) {
        Cprintf(ch, "You are already submerged beneath the waves.\n\r");
        return;
    } else {
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = modified_level;
        af.duration  = spellDuration;
        af.modifier  = ch->in_room->sector_type;
        af.location  = APPLY_NONE;
        af.bitvector = 0;
        af.clan      = ch->clan;
        affect_to_room ( ch->in_room, &af );

        ch->in_room->sector_type = SECT_UNDERWATER;
    }

    Cprintf(ch, "A roaring flood fills the area with water, submerging you beneath the waves!\n\r");
    act("$n floods the room, submerging you beneath the waves!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

    return;
}

void
end_flood(void *vo, int target)
{
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	AFFECT_DATA *paf;

	paf = affect_find(room->affected, gsn_flood);
	if(paf == NULL) {
		bug("End_flood couldn't find a flood affect in room %d.", room->vnum);
		return;
	}

	room->sector_type = paf->modifier;

	if(room->people)
		act("The water flooding the room drains away.", room->people, NULL, NULL, TO_ALL, POS_RESTING);

	return;
}

void
spell_water_breathing(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;
	int effective = FALSE;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
        	Cprintf(ch, "You refresh the water breathing.\n\r");
        	affect_refresh(victim, sn, ch->level / 2);
       		return;
	}

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = modified_level / 2;
        af.location = APPLY_NONE;
        af.modifier = 0;

	if(!IS_AFFECTED(victim, AFF_WATER_BREATHING)) {
	        Cprintf(victim, "Your feel small gills form on the base of your neck.\n\r");
		act("$n grows small gills on their neck.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		effective = TRUE;
        	af.bitvector = AFF_WATER_BREATHING;
        	affect_to_char(victim, &af);
	}

	if(!IS_AFFECTED(victim, AFF_WATERWALK)) {
		Cprintf(victim, "You grow fins, allowing you to navigate better underwater.\n\r");
		act("$n grows fins all over their body.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		af.bitvector = AFF_WATERWALK;
		affect_to_char(victim, &af);
		effective = TRUE;
	}

	if(!effective) {
		Cprintf(ch, "Nothing happens. They are already adapted for underwater travel.\n\r");
		return;
	}
}

void
spell_shawl(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, gsn_shawl))
	{
		Cprintf(ch, "You refresh the magic of your shawl.\n\r");
		affect_refresh(victim, sn, ch->level / 3);
		return;
	}

	Cprintf(victim, "You cloak your body with a magical shawl of the Nereid.\n\r");
	act("$n cloaks themselves with a magical shawl.", victim, NULL, NULL, TO_ROOM, POS_RESTING);

	af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = modified_level / 3;
        af.location = APPLY_AC;
        af.modifier = -40;
        af.bitvector = 0;

	affect_to_char(victim, &af);

	af.location = APPLY_SAVES;
	af.modifier = -5;

	affect_to_char(victim, &af);

	return;
}

void
spell_lure(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = NULL;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	// Find victim in the same area
	victim = get_char_world(ch, target_name, FALSE);
	if(victim == NULL) {
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (is_safe(ch, victim)
	|| ch == victim) {
		Cprintf(ch, "Not on that target.\n\r");
		return;
	}

	if(ch->in_room
	&& victim->in_room
	&& victim->in_room->area != ch->in_room->area) {
		Cprintf(ch, "They are too far away to hear your song.\n\r");
		return;
	}

	if (victim->in_room->clan)
	{
		Cprintf(ch, "They insulated from your song by the walls.\n\r");
		return;
	}

	if (victim->in_room == get_room_index(ROOM_VNUM_LIMBO)
	||  victim->in_room == get_room_index(ROOM_VNUM_LIMBO_DOMINIA)) {
		Cprintf(ch, "Sound doesn't carry into the void.\n\r");
		return;
	}


	if (is_affected(victim, gsn_lure))
	{
		Cprintf(ch, "They dont heed your call because they are already being lured.");
		return;
}

	if (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
        	REMOVE_BIT(victim->act, ACT_AGGRESSIVE);

	check_killer(ch, victim);

	if (saving_throw(ch, victim, sn, caster_level + 1, SAVE_NORMAL, STAT_WIS, DAM_CHARM))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	Cprintf(ch, "You begin to sing a sweet gentle song that lures %s to you.\n\r",IS_NPC(victim) ? victim->short_descr : victim->name);
	Cprintf(victim, "You hear a gentle, sweet song.\n\r");
	act("$n begins to sing an enchanting melody.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	victim->lured_by = strdup(ch->name);

	af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 3;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = 0;

	affect_to_char(victim, &af);

	return;
}

void end_lure(void *vo, int target) {
	CHAR_DATA *ch = (CHAR_DATA *)vo;

	free(ch->lured_by);
	ch->lured_by = '\0';
}


void
spell_aura(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;
	char arg1[MAX_STRING_LENGTH];
	char stringColour[MAX_STRING_LENGTH];
	int numColour;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	target_name = one_argument(target_name, arg1);
	target_name = one_argument(target_name, stringColour);

	if (!strcmp(stringColour, "blue"))
		numColour = 1;
	else if (!strcmp(stringColour, "red"))
		numColour = 2;
	else if (!strcmp(stringColour, "none"))
		numColour = -1;
	else
		numColour = 0;

	if (numColour == -1)
	{
		Cprintf(ch, "You cannot give someone that aura\n\r.");
		return;

	}

        if (is_affected(victim, gsn_aura))
        {
		Cprintf(ch, "%s's old aura fades.\n\r",IS_NPC(victim) ? victim->short_descr : victim->name);
		Cprintf(victim, "Your old aura fades.\n\r");
		affect_strip(victim, gsn_aura);
        }

	switch (numColour) {
		case 0:
			return;
		case 1:
			if (victim != ch)
				Cprintf(ch, "%s is now surrounded by a blue aura.\n\r",IS_NPC(victim) ? victim->short_descr : victim->name);
			Cprintf(victim, "You are now surrounded by a blue aura.\n\r");
			break;
		case 2:
			if (victim != ch)
				Cprintf(ch, "%s is now surrounded by a red aura.\n\r",IS_NPC(victim) ? victim->short_descr : victim->name);
			Cprintf(victim, "You are now surrounded by a red aura.\n\r");
			break;
	}

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.location = APPLY_NONE;
        af.modifier = numColour;
        af.duration = -1;
        af.bitvector = 0;

        affect_to_char(victim, &af);
        return;
}

void
spell_seppuku(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(victim, gsn_seppuku))
        {
                Cprintf(ch, "Your weapon is already stained with blood.\n\r");
                return;
        }

        Cprintf(victim, "You grimly annoint your weapon with your own blood.\n\r");
        act("$n wets $s weapon with his own blood.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 5;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = 0;

        affect_to_char(victim, &af);

}

void
spell_minor_revitalize(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af, *paf = NULL;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(victim, sn))
        {

		paf = affect_find(victim->affected, sn);

                if (victim == ch)
                        Cprintf(ch, "You refresh your revitalization.\n\r");
                else
                {
                        act("$N refreshed your revitalization.", victim, NULL, ch, TO_CHAR, POS_RESTING);
                        Cprintf(ch, "You refresh the revitalization.\n\r");
                }
		paf->modifier = 2 * modified_level;
		affect_refresh(victim, sn, 10);
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 10;
        af.modifier = 2 * modified_level;
        af.location = APPLY_NONE;
        af.bitvector = 0;

        affect_to_char(victim, &af);

        Cprintf(victim, "You feel revitalized and your wounds begin to close slowly.\n\r");
        if (ch != victim)
                act("$N looks revitalized and begins to recover.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        return;
}

void
spell_lesser_revitalize(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af, *paf = NULL;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(victim, sn))
        {

		paf = affect_find(victim->affected, sn);

                if (victim == ch)
                        Cprintf(ch, "You refresh your revitalization.\n\r");
                else
                {
                        act("$N refreshed your revitalization.", victim, NULL, ch, TO_CHAR, POS_RESTING);
                        Cprintf(ch, "You refresh the revitalization.\n\r");
                }
		paf->modifier = 3 * modified_level;
		affect_refresh(victim, sn, 10);
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 10;
        af.modifier = 3 * modified_level;
        af.location = APPLY_NONE;
        af.bitvector = 0;

        affect_to_char(victim, &af);

        Cprintf(victim, "You feel revitalized and your wounds begin to close quickly.\n\r");
        if (ch != victim)
                act("$N looks revitalized and begins to recover.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        return;
}

void
spell_greater_revitalize(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af, *paf = NULL;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(victim, sn))
        {

		paf = affect_find(victim->affected, sn);

                if (victim == ch)
                        Cprintf(ch, "You refresh your revitalization.\n\r");
                else
                {
                        act("$N refreshed your revitalization.", victim, NULL, ch, TO_CHAR, POS_RESTING);
                        Cprintf(ch, "You refresh the revitalization.\n\r");
                }
		paf->modifier = 4 * modified_level;
		affect_refresh(victim, sn, 10);
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 10;
        af.modifier = 4 * modified_level;
        af.location = APPLY_NONE;
        af.bitvector = 0;

        affect_to_char(victim, &af);

        Cprintf(victim, "You feel revitalized and your wounds begin to close unnaturally fast!\n\r");
        if (ch != victim)
                act("$N looks revitalized and begins to recover.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        return;
}

void
spell_thorn_mantle(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(ch, sn))
        {
                Cprintf(ch, "You refresh the mantle of thorns.\n\r");
                affect_refresh(victim, sn, 5 + (modified_level / 5));
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 5 + modified_level / 5;
        af.location = APPLY_HITROLL;
        af.modifier = 3;
        af.bitvector = 0;
        affect_to_char(victim, &af);

	af.location = APPLY_DAMROLL;
	affect_to_char(victim, &af);

        act("$n is endowed with a mantle of thorns.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
        Cprintf(victim, "You are surrounded by a mantle of thorns.\n\r");
        return;
}

void
spell_leaf_shield(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(ch, sn))
        {
                Cprintf(ch, "You refresh the leaf shield.\n\r");
                affect_refresh(victim, sn, modified_level / 3);
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = modified_level / 3;
        af.location = 0;
        af.modifier = 0;
        af.bitvector = 0;
        affect_to_char(victim, &af);
        act("$n creates a leaf shield.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
        Cprintf(victim, "You create a flurry of leaves to shield you from magic.\n\r");
        return;
}

void
spell_stun(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

	DAZE_STATE(victim, PULSE_VIOLENCE * 3);

	act("Bands of force stun $n momentarily.", victim, NULL, NULL, TO_ROOM, POS_RESTING);

	Cprintf(victim, "Bands of force crush you, leaving you stunned momentarily.\n\r");
        return;
}

void
spell_aurora(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af, *paf = NULL;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(victim, sn))
        {

                paf = affect_find(victim->affected, sn);

                if (victim == ch)
                        Cprintf(ch, "You refresh your aurora.\n\r");
                else
                {
                        act("$N refreshed your aurora.", victim, NULL, ch, TO_CHAR, POS_RESTING);
                        Cprintf(ch, "You refresh the aurora.\n\r");
                }
                paf->modifier = 125;
                affect_refresh(victim, sn, 10);
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 10;
        af.modifier = 125;
        af.location = APPLY_NONE;
        af.bitvector = 0;

        affect_to_char(victim, &af);

        Cprintf(victim, "You are surrounded by an aurora of shimmering lights that recharges you!\n\r");
        if (ch != victim)
                act("$N is recharged by an aurora of shimmering lights!", ch, NULL, victim, TO_CHAR, POS_RESTING);
        return;
}

void
spell_misty_cloak(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA*)vo;
	AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(victim, sn))
        {
                Cprintf(ch, "You refresh your misty cloak.\n\r");
		if(victim != ch)
	                act("$N misty cloak is refreshed.", ch, NULL, victim, TO_CHAR, POS_RESTING);
                affect_refresh(victim, sn, modified_level / 3);
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = modified_level / 3;
        af.modifier = -30;
        af.location = APPLY_AC;
        af.bitvector = 0;

        affect_to_char(victim, &af);

	af.modifier = 1 + (modified_level > 24) + (modified_level > 39);
        af.location = APPLY_DEX;
        affect_to_char(victim, &af);

        Cprintf(victim, "You are surrounded by a shroud of mist.\n\r");
        if (ch != victim)
                act("$N is cloaked in a shroud of mist.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        return;
}

void
spell_granite_stare(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	int caster_level, modified_level;
	int old_skill;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

        // Set up
        old_skill = ch->pcdata->learned[sn];
        ch->real_level = ch->level; /* needed to for is_safe checks */

        ch->level = modified_level;
        ch->pcdata->learned[sn] = 100;
        ch->pcdata->any_skill = TRUE; /* needed to get around level restrict */

	do_granite_stare(ch, "");

        // Clean up
        ch->level = ch->real_level;
        ch->pcdata->learned[sn] = old_skill;
        ch->pcdata->any_skill = 0;
        ch->real_level = 0;

}

void
spell_shadow_magic(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(ch, sn))
        {
                Cprintf(ch, "You refresh your connection to the shadows.\n\r");
                affect_refresh(ch, sn, 5);
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 5;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;

        affect_to_char(ch, &af);

        Cprintf(ch, "You empower your magic with a connection to the shadows.\n\r");
        act("$n is empowered by shadows.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
        return;
}

void
spell_jail(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{

	CHAR_DATA* victim = (CHAR_DATA*)vo;

	if (IS_NPC(victim) || victim->pcdata->ctf_team == 0)
	{
		Cprintf(ch, "They can't go to jail, they're not playing CTF.\n\r");
		return;
	}

	char_from_room(victim);
        char_to_room(victim, get_room_index(ROOM_CTF_JAIL(CTF_OTHER_TEAM(victim->pcdata->ctf_team))));
	do_look(victim, "auto");
}

void
end_jail(void *vo, int target)
{
        CHAR_DATA *ch = (CHAR_DATA *) vo;

	if (IS_NPC(ch) || ch->pcdata->ctf_team == 0)
	{
		Cprintf(ch, "Your jail time is over, but you aren't playing CTF!\n\r");
		return;
	}

	char_from_room(ch);
        char_to_room(ch, get_room_index(ROOM_CTF_FLAG(ch->pcdata->ctf_team)));
        do_look(ch, "auto");
        restore_char(ch);
}

void
spell_purify(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{                                                                  
	int purify_ok[] =
	{ gsn_plague,		gsn_poison,		gsn_blindness, 
	  gsn_sleep,		gsn_curse,		gsn_faerie_fire,
	  gsn_weaken,		gsn_charm_person, 	gsn_change_sex,
	  gsn_loneliness,  	gsn_slow,		gsn_chill_touch,
	  gsn_confusion,	gsn_scramble,		gsn_tame_animal,
	  gsn_wail,		gsn_shifting_sands,	gsn_detonation,
	  gsn_sunray,		gsn_nightmares,		gsn_jinx,
	  gsn_calm,		gsn_ensnare,		gsn_frost_breath,
	  gsn_acid_breath,	gsn_hurricane,		gsn_fire_breath,
	  gsn_feeblemind,	gsn_pain_touch,		gsn_dissolution,
	  gsn_denounciation,    -1,
	};
	CHAR_DATA *victim = (CHAR_DATA*)vo;
	AFFECT_DATA *paf;
	int i;
	char buf[255];

	for(paf = victim->affected; paf != NULL; paf = paf->next) {

		i = 0;
		while(1) {
			if(purify_ok[i] == -1)
				break;

			if(paf->type == purify_ok[i]) {
				Cprintf(ch, "You offer a prayer of cleansing and purification.\n\r");
				sprintf(buf, "%s is no longer affected by %s!", IS_NPC(victim) ? victim->short_descr : victim->name, skill_table[paf->type].name);
				act(buf, ch, NULL, victim, TO_NOTVICT, POS_RESTING);
				Cprintf(victim, "You are no longer affected by %s.\n\r", skill_table[paf->type].name);
				if(victim != ch) {
					Cprintf(ch, "%s\n\r", buf);
				}
				affect_strip(victim, paf->type);

				return;	
			}
			i++;
		}				
	}
	
	Cprintf(ch, "You offer a prayer of cleansing and purification.\n\r");
	Cprintf(ch, "Nothing happens.\n\r");

}

void
spell_hallow(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{                                                                  
	ROOM_INDEX_DATA *room;
	AFFECT_DATA *paf = NULL;
	AFFECT_DATA *paf_next =  NULL;
	char buf[255];

	if(ch->in_room == NULL)
		return;

	room = ch->in_room;

	Cprintf(ch, "You kneel down and purge the room of magical influence.\n\r");
	for(paf = room->affected; paf != NULL; paf = paf_next) {
		paf_next = paf->next;

                if(skill_table[paf->type].end_fun != end_null)
                	skill_table[paf->type].end_fun((void*)ch->in_room, TARGET_ROOM);

		sprintf(buf, "The room is no longer affected by %s!", skill_table[paf->type].name);
		act(buf, ch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "%s\n\r", buf);
		
		affect_remove_room(room, paf);
	}

}
