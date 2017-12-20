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
*        ROM 2.4 is copyright 1993-1996 Russ Taylor                        *
*        ROM has been brought to you by the ROM consortium                 *
*            Russ Taylor (rtaylor@pacinfo.com)                             *
*            Gabrielle Taylor (gtaylor@pacinfo.com)                        *
*            Brian Moore (rom@rom.efn.org)                                 *
*        By using this code, you have agreed to follow the terms of the    *
*        ROM license, in the file Rom24/doc/rom.license                    *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "magic.h"
#include "clan.h"
#include "utils.h"

#define REFLECT_FAIL    0
#define REFLECT_BOUNCE  1
#define REFLECT_CASTER  2

/* command procedures needed */
DECLARE_DO_FUN(do_yell);
DECLARE_DO_FUN(do_open);
DECLARE_DO_FUN(do_close);
DECLARE_DO_FUN(do_say);
DECLARE_DO_FUN(do_backstab);
DECLARE_DO_FUN(do_flee);
DECLARE_DO_FUN(do_murder);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_emote);
DECLARE_DO_FUN(do_kill);
DECLARE_DO_FUN(do_restore);
DECLARE_DO_FUN(do_zecho);

void say_spell(CHAR_DATA * ch, int sn);
ROOM_INDEX_DATA *find_location(CHAR_DATA * ch, char *arg);
void gate_in_demons(CHAR_DATA * ch, CHAR_DATA * victim, int size);
void super_spec(CHAR_DATA * ch, CHAR_DATA * victim);

/*
 * The following special functions are available for mobiles.
 */
DECLARE_SPEC_FUN(spec_breath_any);
DECLARE_SPEC_FUN(spec_breath_acid);
DECLARE_SPEC_FUN(spec_breath_fire);
DECLARE_SPEC_FUN(spec_breath_frost);
DECLARE_SPEC_FUN(spec_breath_gas);
DECLARE_SPEC_FUN(spec_breath_lightning);
DECLARE_SPEC_FUN(spec_cast_adept_clan);
DECLARE_SPEC_FUN(spec_cast_adept);
DECLARE_SPEC_FUN(spec_cast_cleric);
DECLARE_SPEC_FUN(spec_cast_judge);
DECLARE_SPEC_FUN(spec_cast_mage);
DECLARE_SPEC_FUN(spec_cast_undead);
DECLARE_SPEC_FUN(spec_executioner);
DECLARE_SPEC_FUN(spec_fido);
DECLARE_SPEC_FUN(spec_familiar);
DECLARE_SPEC_FUN(spec_guard);
DECLARE_SPEC_FUN(spec_janitor);
DECLARE_SPEC_FUN(spec_questmaster);
DECLARE_SPEC_FUN(spec_mayor);
DECLARE_SPEC_FUN(spec_poison);
DECLARE_SPEC_FUN(spec_thief);
DECLARE_SPEC_FUN(spec_nasty);
DECLARE_SPEC_FUN(spec_troll_member);
DECLARE_SPEC_FUN(spec_ogre_member);
DECLARE_SPEC_FUN(spec_patrolman);
DECLARE_SPEC_FUN(spec_goon);
DECLARE_SPEC_FUN(spec_slasher);
DECLARE_SPEC_FUN(spec_doorman);
DECLARE_SPEC_FUN(spec_tet_nasty);
DECLARE_SPEC_FUN(spec_cast_thief);
DECLARE_SPEC_FUN(spec_gatekeeper);
DECLARE_SPEC_FUN(spec_portal_keeper);
DECLARE_SPEC_FUN(spec_homonculus);
DECLARE_SPEC_FUN(spec_tree);
DECLARE_SPEC_FUN(spec_lowbie_spellup);
DECLARE_SPEC_FUN(spec_selina);
DECLARE_SPEC_FUN(spec_nether_dragon);
DECLARE_SPEC_FUN(spec_sliver_pod);
DECLARE_SPEC_FUN(spec_adamantite_sliver);
DECLARE_SPEC_FUN(spec_king_of_the_hill);
DECLARE_SPEC_FUN(spec_tourney_cure);
DECLARE_SPEC_FUN(spec_assault);
DECLARE_SPEC_FUN(spec_unremort);
/* deamon specs */
DECLARE_SPEC_FUN(spec_demon_lesser);
DECLARE_SPEC_FUN(spec_demon_greater);
DECLARE_SPEC_FUN(spec_demon_lord);

// Externs
extern int generate_int(unsigned char, unsigned char);
extern int check_reflection(CHAR_DATA *, CHAR_DATA *);
extern int check_leaf_shield(CHAR_DATA *, CHAR_DATA *);
extern void set_fighting(CHAR_DATA * ch, CHAR_DATA * victim);

/* the function table */
const struct spec_type spec_table[] =
{
	{"spec_breath_any", spec_breath_any},
	{"spec_breath_acid", spec_breath_acid},
	{"spec_breath_fire", spec_breath_fire},
	{"spec_breath_frost", spec_breath_frost},
	{"spec_breath_gas", spec_breath_gas},
	{"spec_breath_lightning", spec_breath_lightning},
	{"spec_cast_adept", spec_cast_adept},
	{"spec_cast_adept_clan", spec_cast_adept_clan},
	{"spec_cast_cleric", spec_cast_cleric},
	{"spec_cast_judge", spec_cast_judge},
	{"spec_cast_mage", spec_cast_mage},
	{"spec_cast_undead", spec_cast_undead},
	{"spec_executioner", spec_executioner},
	{"spec_fido", spec_fido},
	{"spec_familiar", spec_familiar},
	{"spec_guard", spec_guard},
	{"spec_janitor", spec_janitor},
	{"spec_questmaster", spec_questmaster},
	{"spec_mayor", spec_mayor},
	{"spec_poison", spec_poison},
	{"spec_thief", spec_thief},
	{"spec_nasty", spec_nasty},
	{"spec_troll_member", spec_troll_member},
	{"spec_ogre_member", spec_ogre_member},
	{"spec_patrolman", spec_patrolman},
	{"spec_goon", spec_goon},
	{"spec_slasher", spec_slasher},
	{"spec_doorman", spec_doorman},
	{"spec_cast_thief", spec_cast_thief},
	{"spec_gatekeeper", spec_gatekeeper},
	{"spec_demon_lesser", spec_demon_lesser},
	{"spec_demon_greater", spec_demon_greater},
	{"spec_demon_lord", spec_demon_lord},
	{"spec_portal_keeper", spec_portal_keeper},
	{"spec_homonculus", spec_homonculus},
	{"spec_tree", spec_tree},
	{"spec_lowbie_spellup", spec_lowbie_spellup},
	{"spec_selina", spec_selina},
	{"spec_nether_dragon", spec_nether_dragon},
	{"spec_sliver_pod", spec_sliver_pod},
	{"spec_adamantite_sliver", spec_adamantite_sliver},
	{"spec_king_of_the_hill", spec_king_of_the_hill},
	{"spec_tourney_cure", spec_tourney_cure},
	{"spec_assault", spec_assault},
	{"spec_unremort", spec_unremort},
	{NULL, NULL}
};

/*
 * Given a name, return the appropriate spec fun.
 */
SPEC_FUN *
spec_lookup(const char *name)
{
	int i;

	for (i = 0; spec_table[i].name != NULL; i++)
	{
		if (LOWER(name[0]) == LOWER(spec_table[i].name[0])
			&& !str_prefix(name, spec_table[i].name))
			return spec_table[i].function;
	}

	return 0;
}

char *
spec_name(SPEC_FUN * function)
{
	int i;

	for (i = 0; spec_table[i].function != NULL; i++)
	{
		if (function == spec_table[i].function)
			return spec_table[i].name;
	}

	return NULL;
}

bool
spec_troll_member(CHAR_DATA * ch)
{
	CHAR_DATA *vch, *victim = NULL;
	int count = 0;
	char *message;

	if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL
		|| IS_AFFECTED(ch, AFF_CHARM) || ch->fighting != NULL)
		return FALSE;

	/* find an ogre to beat up */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (!IS_NPC(vch) || ch == vch)
			continue;

		if (vch->pIndexData->vnum == MOB_VNUM_PATROLMAN)
			return FALSE;

		if (vch->pIndexData->group == GROUP_VNUM_OGRES
			&& ch->level > vch->level - 2 && !is_safe(ch, vch))
		{
			if (number_range(0, count) == 0)
				victim = vch;

			count++;
		}
	}

	if (victim == NULL)
		return FALSE;

	/* say something, then raise hell */
	switch (number_range(0, 6))
	{
	default:
		message = NULL;
		break;
	case 0:
		message = "$n yells 'I've been looking for you, punk!'";
		break;
	case 1:
		message = "With a scream of rage, $n attacks $N.";
		break;
	case 2:
		message =
			"$n says 'What's slimy Ogre trash like you doing around here?'";
		break;
	case 3:
		message = "$n cracks his knuckles and says 'Do ya feel lucky?'";
		break;
	case 4:
		message = "$n says 'There's no cops to save you this time!'";
		break;
	case 5:
		message = "$n says 'Time to join your brother, spud.'";
		break;
	case 6:
		message = "$n says 'Let's rock.'";
		break;
	}

	if (message != NULL)
		act(message, ch, NULL, victim, TO_ALL, POS_RESTING);
	multi_hit(ch, victim, TYPE_UNDEFINED);
	return TRUE;
}

bool
spec_slasher(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	bool outcome;

	outcome = FALSE;

	if (number_range(1, 5) != 1)
		return outcome;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		outcome = TRUE;
		if (victim != ch)
		{

			if ((victim->max_hit / 20) < victim->hit)
			{
				Cprintf(victim, "Life draws away from you.  Don't stay in this room!\n\r");
				victim->hit = victim->hit / 2;
			}
			else
			{
				Cprintf(victim, "You don't listen do you? This room finally proved to be lethal to you.\n\r");
			}
		}
	}

	return outcome;
}


bool
spec_goon(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int RECALL_ROOM;


	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch)
		{
			if ((!clan_table[victim->clan].pkiller && !IS_NPC(victim))
				|| (IS_NPC(victim) && victim->master == NULL)
				|| (IS_NPC(victim) && victim->master != NULL && victim->master->in_room != ch->in_room)
				|| (IS_NPC(victim) && !clan_table[victim->master->clan].pkiller))
			{
				if (victim->fighting != NULL)
					stop_fighting(victim, TRUE);

				if (victim->in_room->area->continent == 0)
					RECALL_ROOM = ROOM_VNUM_TEMPLE;
				else
					RECALL_ROOM = ROOM_VNUM_DOMINIA;

				act("$n disappears in a flash of light.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
				char_from_room(victim);
				char_to_room(victim, get_room_index(RECALL_ROOM));
				act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
				if (ch != victim)
					act("$n has booted you out of the clan hall.", ch, NULL, victim, TO_VICT, POS_RESTING);
				do_look(victim, "auto");
				return TRUE;
			}
			break;
		}

	}

	if (victim == NULL)
		return FALSE;

/* Sends message an average of once every 3 rounds */

	if (number_range(1, 3) != 1)
		return TRUE;

	for (vch = char_list; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next;

		if (!IS_NPC(vch) && vch->clan == ch->clan)
		{
			Cprintf(vch, "Your clan goon screams: 'Help! Help! I'm being attacked by %s!!!'\n\r", victim->name);
		}
	}

	return TRUE;
}


bool
spec_doorman(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (!IS_NPC(victim) && victim->clan == ch->clan && victim->fighting == NULL)
		{
			act("$n disappears in a flash of light.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			if (victim->in_room->area->continent == 0)
			{
				char_from_room(victim);
				char_to_room(victim, get_room_index(clan_table[victim->clan].hall));
			}
			else
			{
				char_from_room(victim);
				char_to_room(victim, get_room_index(clan_table[victim->clan].hall_dominia));
			}
			act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			do_look(victim, "auto");
		}
	}
	return TRUE;
}

bool
spec_portal_keeper(CHAR_DATA * ch)
{
	return TRUE;
}

bool
spec_questmaster(CHAR_DATA * ch)
{
	return TRUE;
}

bool
spec_ogre_member(CHAR_DATA * ch)
{
	CHAR_DATA *vch, *victim = NULL;
	int count = 0;
	char *message;

	if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL
		|| IS_AFFECTED(ch, AFF_CHARM) || ch->fighting != NULL)
		return FALSE;

	/* find an troll to beat up */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (!IS_NPC(vch) || ch == vch)
			continue;

		if (vch->pIndexData->vnum == MOB_VNUM_PATROLMAN)
			return FALSE;

		if (vch->pIndexData->group == GROUP_VNUM_TROLLS
			&& ch->level > vch->level - 2 && !is_safe(ch, vch))
		{
			if (number_range(0, count) == 0)
				victim = vch;

			count++;
		}
	}

	if (victim == NULL)
		return FALSE;

	/* say something, then raise hell */
	switch (number_range(0, 6))
	{
	default:
		message = NULL;
		break;
	case 0:
		message = "$n yells 'I've been looking for you, punk!'";
		break;
	case 1:
		message = "With a scream of rage, $n attacks $N.'";
		break;
	case 2:
		message =
			"$n says 'What's Troll filth like you doing around here?'";
		break;
	case 3:
		message = "$n cracks his knuckles and says 'Do ya feel lucky?'";
		break;
	case 4:
		message = "$n says 'There's no cops to save you this time!'";
		break;
	case 5:
		message = "$n says 'Time to join your brother, spud.'";
		break;
	case 6:
		message = "$n says 'Let's rock.'";
		break;
	}

	if (message != NULL)
		act(message, ch, NULL, victim, TO_ALL, POS_RESTING);
	multi_hit(ch, victim, TYPE_UNDEFINED);
	return TRUE;
}

bool
spec_patrolman(CHAR_DATA * ch)
{
	CHAR_DATA *vch, *victim = NULL;
	OBJ_DATA *obj;
	char *message;
	int count = 0;

	if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL
		|| IS_AFFECTED(ch, AFF_CHARM) || ch->fighting != NULL)
		return FALSE;

	/* look for a fight in the room */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (vch == ch)
			continue;

		if (vch->fighting != NULL)	/* break it up! */
		{
			if (number_range(0, count) == 0)
				victim = (vch->level > vch->fighting->level)
					? vch : vch->fighting;
			count++;
		}
	}

	if (victim == NULL || (IS_NPC(victim) && victim->spec_fun == ch->spec_fun))
		return FALSE;

	if (((obj = get_eq_char(ch, WEAR_NECK_1)) != NULL
		 && obj->pIndexData->vnum == OBJ_VNUM_WHISTLE)
		|| ((obj = get_eq_char(ch, WEAR_NECK_2)) != NULL
			&& obj->pIndexData->vnum == OBJ_VNUM_WHISTLE))
	{
		act("You blow down hard on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$n blows on $p, ***WHEEEEEEEEEEEET***", ch, obj, NULL, TO_ROOM, POS_RESTING);

		for (vch = char_list; vch != NULL; vch = vch->next)
		{
			if (vch->in_room == NULL)
				continue;

			if (vch->in_room != ch->in_room
				&& vch->in_room->area == ch->in_room->area)
				Cprintf(vch, "You hear a shrill whistling sound.\n\r");
		}
	}

	switch (number_range(0, 6))
	{
	default:
		message = NULL;
		break;
	case 0:
		message = "$n yells 'All roit! All roit! break it up!'";
		break;
	case 1:
		message =
			"$n says 'Society's to blame, but what's a bloke to do?'";
		break;
	case 2:
		message =
			"$n mumbles 'bloody kids will be the death of us all.'";
		break;
	case 3:
		message = "$n shouts 'Stop that! Stop that!' and attacks.";
		break;
	case 4:
		message = "$n pulls out his billy and goes to work.";
		break;
	case 5:
		message =
			"$n sighs in resignation and proceeds to break up the fight.";
		break;
	case 6:
		message = "$n says 'Settle down, you hooligans!'";
		break;
	}

	if (message != NULL)
		act(message, ch, NULL, NULL, TO_ALL, POS_RESTING);

	multi_hit(ch, victim, TYPE_UNDEFINED);

	return TRUE;
}


bool
spec_nasty(CHAR_DATA * ch)
{
	CHAR_DATA *victim, *v_next;
	long gold;

	if (!IS_AWAKE(ch))
	{
		return FALSE;
	}

	if (ch->position != POS_FIGHTING)
	{
		for (victim = ch->in_room->people; victim != NULL; victim = v_next)
		{
			v_next = victim->next_in_room;
			if (!IS_NPC(victim)
				&& (victim->level > ch->level)
				&& (victim->level < ch->level + 10))
			{
				do_backstab(ch, victim->name);
				if (ch->position != POS_FIGHTING)
					do_murder(ch, victim->name);
				/* should steal some coins right away? :) */
				return TRUE;
			}
		}
		return FALSE;			/*  No one to attack */
	}

	/* okay, we must be fighting.... steal some coins and flee */
	if ((victim = ch->fighting) == NULL)
		return FALSE;			/* let's be paranoid.... */

	switch (number_bits(2))
	{
	case 0:
		act("$n rips apart your coin purse, spilling your gold!", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("You slash apart $N's coin purse and gather his gold.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		act("$N's coin purse is ripped apart!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		gold = victim->gold / 10;	/* steal 10% of his gold */
		victim->gold -= gold;
		ch->gold += gold;
		return TRUE;

	case 1:
		do_flee(ch, "");
		return TRUE;

	default:
		return FALSE;
	}
}

/*
 * Core procedure for dragons.
 */
bool
dragon(CHAR_DATA * ch, char *spell_name)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int sn, spell_level;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(3) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	if ((sn = skill_lookup(spell_name)) < 0)
		return FALSE;

	spell_level = generate_int(ch->level, ch->level);

	(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);
	return TRUE;
}



/*
 * Special procedures for mobiles.
 */
bool
spec_breath_any(CHAR_DATA * ch)
{
	int number = 0;
	if (ch->position != POS_FIGHTING)
		return FALSE;


	number = dice(1, 5);
	switch (number)
	{
	case 1:
		return dragon(ch, "fire breath");
	case 2:
		return dragon(ch, "lightning breath");
	case 3:
		return dragon(ch, "gas breath");
	case 4:
		return dragon(ch, "acid breath");
	case 5:
		return dragon(ch, "frost breath");
	}

	return FALSE;
}



bool
spec_breath_acid(CHAR_DATA * ch)
{
	return dragon(ch, "acid breath");
}



bool
spec_breath_fire(CHAR_DATA * ch)
{
	return dragon(ch, "fire breath");
}



bool
spec_breath_frost(CHAR_DATA * ch)
{
	return dragon(ch, "frost breath");
}



bool
spec_breath_gas(CHAR_DATA * ch)
{
	return dragon(ch, "gas breath");
}



bool
spec_breath_lightning(CHAR_DATA * ch)
{
	return dragon(ch, "lightning breath");
}


bool
spec_cast_adept_clan(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int spell_level;

	if (!IS_AWAKE(ch))
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim != ch && can_see(ch, victim) && number_bits(1) == 0
			&& !IS_NPC(victim) && victim->level < 11)
			break;
	}

	if (victim == NULL)
		return FALSE;

	spell_level = generate_int(ch->level, ch->level);

	switch (number_bits(4))
	{
	case 0:
		act("$n utters the word 'abrazak'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_armor(gsn_armor, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 1:
		act("$n utters the word 'fido'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_bless(gsn_bless, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 2:
		act("$n utters the words 'judicandus noselacri'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_cure_blindness(gsn_cure_blindness, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 3:
		act("$n utters the words 'judicandus dies'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_cure_light(gsn_cure_light, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 4:
		act("$n utters the words 'judicandus sausabru'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_cure_poison(gsn_cure_poison, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 5:
		act("$n utters the word 'candusima'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_refresh(gsn_refresh, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 6:
		act("$n utters the words 'judicandus eugzagz'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_cure_disease(gsn_cure_disease, spell_level, ch, victim, TARGET_CHAR);
	}

	return FALSE;
}

bool
spec_cast_adept(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int spell_level;

	if (!IS_AWAKE(ch))
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim != ch && can_see(ch, victim) && number_bits(1) == 0
			&& !IS_NPC(victim) && victim->level < 11)
			break;
	}

	if (victim == NULL)
		return FALSE;

	spell_level = generate_int(ch->level, ch->level);
	switch (number_bits(4))
	{
	case 0:
		act("$n utters the word 'abrazak'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_armor(gsn_armor, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 1:
		act("$n utters the word 'fido'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_bless(gsn_bless, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 2:
		act("$n utters the words 'judicandus noselacri'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_cure_blindness(gsn_cure_blindness, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 3:
		act("$n utters the words 'judicandus dies'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_cure_light(gsn_cure_light, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 4:
		act("$n utters the words 'judicandus sausabru'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_cure_poison(gsn_cure_poison, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 5:
		act("$n utters the word 'candusima'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_refresh(gsn_refresh, spell_level, ch, victim, TARGET_CHAR);
		return TRUE;

	case 6:
		act("$n utters the words 'judicandus eugzagz'.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		spell_cure_disease(gsn_cure_disease, spell_level, ch, victim, TARGET_CHAR);
	}

	return FALSE;
}



bool
spec_cast_cleric(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int sn = -1;
	int spell_level;
	int ref;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	for (;;)
	{
		int min_level;

		switch (number_bits(4))
		{
		case 0:
			min_level = 0;
			sn = gsn_blindness;
			break;
		case 1:
			min_level = 3;
			sn = gsn_cause_serious;
			break;
		case 2:
			min_level = 7;
			sn = gsn_earthquake;
			break;
		case 3:
			min_level = 9;
			sn = gsn_cause_critical;
			break;
		case 4:
			min_level = 10;
			sn = gsn_dispel_evil;
			break;
		case 5:
			min_level = 12;
			sn = gsn_curse;
			break;
		case 6:
			min_level = 12;
			sn = gsn_change_sex;
			break;
		case 7:
			min_level = 13;
			sn = gsn_fireball;
			break;
		case 8:
		case 9:
		case 10:
			min_level = 15;
			sn = gsn_harm;
			break;
		case 11:
			min_level = 15;
			sn = gsn_plague;
			break;
		default:
			min_level = 16;
			sn = gsn_dispel_magic;
			break;
		}

		if (ch->level >= min_level)
			break;
	}

	if (sn < 0)
		return FALSE;

	ref =  check_reflection(ch, victim);
	if(ref == REFLECT_BOUNCE)
        	victim   = NULL;
	else if(ref == REFLECT_CASTER)
        	victim = ch;

	ref = check_leaf_shield(ch, victim);
	if(ref == REFLECT_BOUNCE)
        	victim   = NULL;

	spell_level = generate_int(ch->level, ch->level);

	if(victim != NULL)
		(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);
	return TRUE;
}

bool
spec_cast_judge(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int sn = -1;
	int spell_level;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	if ((sn = gsn_high_explosive) < 0)
		return FALSE;
	spell_level = generate_int(ch->level, ch->level);
	(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);
	return TRUE;
}



bool
spec_cast_mage(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int sn = -1;
	int spell_level;
	int ref;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	for (;;)
	{
		int min_level;

		switch (number_bits(4))
		{
		case 0:
			min_level = 0;
			sn = gsn_blindness;
			break;
		case 1:
			min_level = 3;
			sn = gsn_chill_touch;
			break;
		case 2:
			min_level = 7;
			sn = gsn_weaken;
			break;
		case 3:
			min_level = 8;
			sn = gsn_teleport;
			break;
		case 4:
			min_level = 11;
			sn = gsn_colour_spray;
			break;
		case 5:
			min_level = 12;
			sn = gsn_change_sex;
			break;
		case 6:
			min_level = 13;
			sn = gsn_lightning_bolt;
			break;
		case 7:
		case 8:
		case 9:
			min_level = 15;
			sn = gsn_fireball;
			break;
		case 10:
			min_level = 20;
			sn = gsn_plague;
			break;
		default:
			min_level = 20;
			sn = gsn_acid_blast;
			break;
		}

		if (ch->level >= min_level)
			break;
	}

	if (sn < 0)
		return FALSE;

        ref =  check_reflection(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;
        else if(ref == REFLECT_CASTER)
                victim = ch;

        ref = check_leaf_shield(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;

	spell_level = generate_int(ch->level, ch->level);

	if(victim != NULL)
		(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);
	return TRUE;
}



bool
spec_cast_undead(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int sn = -1;
	int spell_level;
	int ref;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	for (;;)
	{
		int min_level;

		switch (number_bits(4))
		{
		case 0:
			min_level = 0;
			sn = gsn_curse;
			break;
		case 1:
			min_level = 3;
			sn = gsn_weaken;
			break;
		case 2:
			min_level = 6;
			sn = gsn_chill_touch;
			break;
		case 3:
			min_level = 9;
			sn = gsn_blindness;
			break;
		case 4:
			min_level = 12;
			sn = gsn_poison;
			break;
		case 5:
			min_level = 15;
			sn = gsn_energy_drain;
			break;
		case 6:
			min_level = 18;
			sn = gsn_harm;
			break;
		case 7:
			min_level = 21;
			sn = gsn_teleport;
			break;
		case 8:
			min_level = 20;
			sn = gsn_plague;
			break;
		default:
			min_level = 18;
			sn = gsn_harm;
			break;
		}

		if (ch->level >= min_level)
			break;
	}

	if (sn < 0)
		return FALSE;

        ref =  check_reflection(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;
        else if(ref == REFLECT_CASTER)
                victim = ch;

        ref = check_leaf_shield(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;

	spell_level = generate_int(ch->level, ch->level);

	if(victim != NULL)
		(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);
	return TRUE;
}

bool
spec_homonculus(CHAR_DATA * ch)
{
	CHAR_DATA *victim, *v_next;
	int sn = -1;
	int spell_level;
	int ref;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	for (;;)
	{
		int min_level;

		switch (number_bits(5))
		{
		case 0:
		case 1:
		case 2:
			min_level = 0;
			act("{y$n says 'eeeeeeeeeeeEEEEEEEEEEEEEEEE!!!'{x", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			break;
		case 3:
		case 4:
		case 5:
			min_level = 0;
			act("You shoot a bird at $N.", ch, ch->fighting, NULL, TO_CHAR, POS_RESTING);
			act("$n gives you the bird...What an ass!", ch, ch->fighting, ch->fighting, TO_VICT, POS_RESTING);
			act("$n gives $N the bird.", ch, ch->fighting, ch->fighting, TO_ROOM, POS_RESTING);
			break;
		case 6:
			min_level = 3;
			sn = gsn_harm;
			break;
		case 7:
			min_level = 7;
			sn = gsn_faerie_fire;
			break;
		case 8:
			min_level = 8;
			sn = gsn_poison;
			break;
		case 9:
			min_level = 11;
			sn = gsn_plague;
			break;
		case 10:
			min_level = 0;
			act("You shoot a bird at $N.", ch, ch->fighting, NULL, TO_CHAR, POS_RESTING);
			act("$n gives you the bird...What an ass!", ch, ch->fighting, ch->fighting, TO_VICT, POS_RESTING);
			act("$n gives $N the bird.", ch, ch->fighting, ch->fighting, TO_ROOM, POS_RESTING);
			break;
		case 11:
			min_level = 0;
			act("{y$n says 'eeeeeeeeeeeEEEEEEEEEEEEEEEE!!!'{x", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			break;
		case 12:
			min_level = 12;
			sn = gsn_blindness;
			break;
		case 13:
			min_level = 13;
			sn = gsn_curse;
			break;
		case 14:
			min_level = 13;
			act("{y$n says 'eeeeeeeeeeeEEEEEEEEEEEEEEEE!!!'{x", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			sn = gsn_shriek;
			break;
		case 15:
			min_level = 14;
			act("You shoot a bird at $N.", ch, ch->fighting, NULL, TO_CHAR, POS_RESTING);
			act("$n gives you the bird...What an ass!", ch, ch->fighting, ch->fighting, TO_VICT, POS_RESTING);
			act("$n gives $N the bird.", ch, ch->fighting, ch->fighting, TO_ROOM, POS_RESTING);
			sn = gsn_curse;
			break;
		case 16:
			min_level = 15;
			sn = gsn_pyrotechnics;
			break;
		case 17:
			min_level = 20;
			sn = gsn_plague;
			break;
		case 18:
			min_level = 20;
			sn = gsn_ice_bolt;
			break;
		case 19:
			min_level = 0;
			act("{y$n says 'eeeeeeeeeeeEEEEEEEEEEEEEEEE!!!'{x", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			break;
		case 20:
			min_level = 0;
			act("You shoot a bird at $N.", ch, ch->fighting, NULL, TO_CHAR, POS_RESTING);
			act("$n gives you the bird...What an ass!", ch, ch->fighting, ch->fighting, TO_VICT, POS_RESTING);
			act("$n gives $N the bird.", ch, ch->fighting, ch->fighting, TO_ROOM, POS_RESTING);
			break;
		case 21:
			min_level = 20;
			sn = gsn_blast_of_rot;
			break;
		default:
			min_level = 0;
			act("{y$n says 'eeeeeeeeeeeEEEEEEEEEEEEEEEE!!!'{x", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			break;
		}
		if (ch->level >= min_level)
			break;
	}

	if (sn < 0)
		return FALSE;

        ref =  check_reflection(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;
        else if(ref == REFLECT_CASTER)
                victim = ch;

        ref = check_leaf_shield(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;

	spell_level = generate_int(ch->level, ch->level);

	if(victim != NULL)
		(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);
	return TRUE;
}

bool
spec_executioner(CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	char *crime;

	if (!IS_AWAKE(ch) || ch->fighting != NULL)
		return FALSE;

	crime = "";
	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;

		if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER)
			&& !is_affected(victim, gsn_cloak_of_mind))
		{
			crime = "KILLER";
			break;
		}

		if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF)
			&& !is_affected(victim, gsn_cloak_of_mind))
		{
			crime = "THIEF";
			break;
		}

		if (!IS_NPC(victim)
		&& victim->reclass == reclass_lookup("ninja")
		&& !is_affected(victim, gsn_cloak_of_mind))
		{
			crime = "NINJA";
			break;
		}
	}

	if (victim == NULL)
		return FALSE;

	sprintf(buf, "%s is a %s!  PROTECT THE INNOCENT!  MORE BLOOOOD!!!",
			victim->name, crime);
	REMOVE_BIT(ch->comm, COMM_NOSHOUT);
	do_yell(ch, buf);
	multi_hit(ch, victim, TYPE_UNDEFINED);
	return TRUE;
}


bool
spec_familiar(CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *v_next;

	if (!IS_AWAKE(ch) || ch->fighting != NULL)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim != ch && !IS_NPC(victim) && victim->seen == 0)
		{
			sprintf(buf, "{RAlert! Alert!{x %s is in the area!!!", victim->name);
			REMOVE_BIT(ch->comm, COMM_NOSHOUT);
			do_yell(ch, buf);
			victim->seen = 1;
			break;
		}
	}
	return FALSE;
}

bool
spec_fido(CHAR_DATA * ch)
{
	OBJ_DATA *corpse;
	OBJ_DATA *c_next;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	if (!IS_AWAKE(ch))
		return FALSE;

	for (corpse = ch->in_room->contents; corpse != NULL; corpse = c_next)
	{
		c_next = corpse->next_content;
		if (corpse->item_type != ITEM_CORPSE_NPC)
			continue;

		act("$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		for (obj = corpse->contains; obj; obj = obj_next)
		{
			obj_next = obj->next_content;
			obj_from_obj(obj);
			obj_to_room(obj, ch->in_room);
		}
		extract_obj(corpse);
		return TRUE;
	}

	return FALSE;
}



bool
spec_guard(CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	CHAR_DATA *ech;
	char *crime;
	int max_evil;

	if (!IS_AWAKE(ch) || ch->fighting != NULL)
		return FALSE;

	max_evil = 300;
	ech = NULL;
	crime = "";

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;

		if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER)
			&& can_see(ch, victim) && !is_affected(victim, gsn_cloak_of_mind))
		{
			crime = "KILLER";
			break;
		}

		if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF)
			&& can_see(ch, victim) && !is_affected(victim, gsn_cloak_of_mind))
		{
			crime = "THIEF";
			break;
		}

		if (!IS_NPC(victim)
                && victim->reclass == reclass_lookup("ninja")
		&& !is_affected(victim, gsn_cloak_of_mind))
                {
                        crime = "NINJA";
                        break;
                }

		if (victim->fighting != NULL
			&& victim->fighting != ch
			&& victim->alignment < max_evil)
		{
			max_evil = victim->alignment;
			ech = victim;
		}
	}

	if (victim != NULL)
	{
		sprintf(buf, "%s is a %s!  PROTECT THE INNOCENT!!  BANZAI!!",
				victim->name, crime);
		REMOVE_BIT(ch->comm, COMM_NOSHOUT);
		do_yell(ch, buf);
		multi_hit(ch, victim, TYPE_UNDEFINED);
		return TRUE;
	}

	if (ech != NULL)
	{
		act("$n screams 'PROTECT THE INNOCENT!!  BANZAI!!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		multi_hit(ch, ech, TYPE_UNDEFINED);
		return TRUE;
	}

	return FALSE;
}



bool
spec_janitor(CHAR_DATA * ch)
{
	OBJ_DATA *trash;
	OBJ_DATA *trash_next;

	if (!IS_AWAKE(ch))
		return FALSE;

	for (trash = ch->in_room->contents; trash != NULL; trash = trash_next)
	{
		trash_next = trash->next_content;
		if (!IS_SET(trash->wear_flags, ITEM_TAKE) || !can_loot(ch, trash))
			continue;
		if (trash->item_type == ITEM_DRINK_CON
			|| trash->item_type == ITEM_TRASH
			|| trash->cost < 10)
		{
			act("$n picks up some trash.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			obj_from_room(trash);
			obj_to_char(trash, ch);
			return TRUE;
		}
	}

	return FALSE;
}



bool
spec_mayor(CHAR_DATA * ch)
{
	static const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

	static const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

	static const char *path;
	static int pos;
	static bool move;

	if (!move)
	{
		if (time_info.hour == 6)
		{
			path = open_path;
			move = TRUE;
			pos = 0;
		}

		if (time_info.hour == 20)
		{
			path = close_path;
			move = TRUE;
			pos = 0;
		}
	}

	if (ch->fighting != NULL)
		return spec_cast_mage(ch);
	if (!move || ch->position < POS_SLEEPING)
		return FALSE;

	switch (path[pos])
	{
	case '0':
	case '1':
	case '2':
	case '3':
		move_char(ch, path[pos] - '0', FALSE);
		break;

	case 'W':
		ch->position = POS_STANDING;
		act("$n awakens and groans loudly.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		break;

	case 'S':
		ch->position = POS_SLEEPING;
		act("$n lies down and falls asleep.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		break;

	case 'a':
		act("$n says 'Hello Honey!'", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		break;

	case 'b':
		act("$n says 'What a view!  I must do something about that dump!'", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		break;

	case 'c':
		act("$n says 'Vandals!  Youngsters have no respect for anything!'", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		break;

	case 'd':
		act("$n says 'Good day, citizens!'", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		break;

	case 'e':
		act("$n says 'I hereby declare the city of Midgaard open!'", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		break;

	case 'E':
		act("$n says 'I hereby declare the city of Midgaard closed!'", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		break;

	case 'O':
/*      do_unlock( ch, "gate" ); */
		do_open(ch, "gate");
		break;

	case 'C':
		do_close(ch, "gate");
/*      do_lock( ch, "gate" ); */
		break;

	case '.':
		move = FALSE;
		break;
	}

	pos++;
	return FALSE;
}



bool
spec_poison(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	int spell_level;

	if (ch->position != POS_FIGHTING
		|| (victim = ch->fighting) == NULL
		|| number_percent() > ch->level * 3 / 2)
		return FALSE;

	act("You bite $N!", ch, NULL, victim, TO_CHAR, POS_RESTING);
	act("$n bites $N!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	act("$n bites you!", ch, NULL, victim, TO_VICT, POS_RESTING);
	spell_level = generate_int(ch->level, ch->level);
	spell_poison(gsn_poison, spell_level, ch, victim, TARGET_CHAR);
	return TRUE;
}



bool
spec_thief(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	long gold, silver;

	if (ch->position != POS_STANDING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;

		if (IS_NPC(victim)
			|| victim->level >= LEVEL_IMMORTAL
			|| number_bits(5) != 0
			|| !can_see(ch, victim)
			|| is_affected(victim, gsn_cloak_of_mind))
			continue;

		if (IS_AWAKE(victim) && number_range(0, ch->level) == 0)
		{
			act("You discover $n's hands in your wallet!", ch, NULL, victim, TO_VICT, POS_RESTING);
			act("$N discovers $n's hands in $S wallet!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
			return TRUE;
		}
		else
		{
			gold = victim->gold * UMIN(number_range(1, 20), ch->level / 2) / 100;
			gold = UMIN(gold, ch->level * ch->level * 10);
			ch->gold += gold;
			victim->gold -= gold;
			silver = victim->silver * UMIN(number_range(1, 20), ch->level / 2) / 100;
			silver = UMIN(silver, ch->level * ch->level * 25);
			ch->silver += silver;
			victim->silver -= silver;
			return TRUE;
		}
	}

	return FALSE;
}

bool
spec_cast_thief(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int sn = -1;
	int isspell;
	int spell_level;
	int ref;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;


	switch (number_bits(4))
	{
	case 0:
	case 1:
	case 2:
	case 3:
		sn = gsn_scramble;
		isspell = TRUE;
		break;
	case 4:
	case 5:
		sn = gsn_taunt;
		isspell = TRUE;
		break;
	case 6:
	case 7:
		sn = gsn_backstab;
		isspell = FALSE;
		break;
	case 8:
		sn = gsn_heat_metal;
		isspell = TRUE;
		break;
	case 9:
		sn = gsn_lightning_bolt;
		isspell = TRUE;
		break;
	case 10:
		sn = gsn_backstab;
		isspell = FALSE;
		break;
	default:
		sn = gsn_scramble;
		isspell = TRUE;
		break;
	}

	if (isspell == TRUE)
	{
		if (sn < 0)
			return FALSE;

	        ref =  check_reflection(ch, victim);
        	if(ref == REFLECT_BOUNCE)
                	victim   = NULL;
        	else if(ref == REFLECT_CASTER)
                	victim = ch;

        	ref = check_leaf_shield(ch, victim);
        	if(ref == REFLECT_BOUNCE)
                	victim   = NULL;

		spell_level = generate_int(ch->level, ch->level);

		if(victim != NULL)
			(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);
		say_spell(ch, sn);
	}
	else
		do_backstab(ch, "");

	return TRUE;
}

bool
spec_gatekeeper(CHAR_DATA * ch)
{
	static bool Act = TRUE;

	if (time_info.hour == 9
		|| time_info.hour == 21
		|| time_info.hour == 13)
		Act = TRUE;

	if (Act == TRUE)
	{
		if (ch->fighting != NULL)
		{
			return FALSE;
		}

		if (time_info.hour == 8)
		{
			do_say(ch, "The city of Stoneridge is now open to all.");
			do_open(ch, "gates");
		}
		else if (time_info.hour == 20)
		{
			do_say(ch, "The city of Stoneridge is closed for the night.");
			do_close(ch, "gates");
		}
		else if (time_info.hour == 12)
		{
			do_say(ch, "Twelve o'clock and all is well.");
		}
		else
		{
			return FALSE;
		}
		Act = FALSE;
	}

	return TRUE;
}

/* deamon specs */
bool
spec_demon_lesser(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int sn = -1;
	int spell_level = 0;
	int ref;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	if (ch->in_room == NULL)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	for (;;)
	{
		int min_level;

		switch (dice(1, 10))
		{
		case 1:
			min_level = 0;
			sn = gsn_curse;
			break;
		case 2:
			min_level = 3;
			sn = gsn_blindness;
			break;
		case 3:
			min_level = 10;
			sn = gsn_dispel_magic;
			break;
		case 4:
			min_level = 10;
			sn = gsn_burning_hands;
			break;
		case 5:
			gate_in_demons(ch, victim, 0);
			min_level = 20;
			break;
		default:
			min_level = 200;
			break;
		}

		if (ch->level >= min_level)
			break;
		else
			return FALSE;
	}

	if (sn < 0)
		return FALSE;

        ref =  check_reflection(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;
        else if(ref == REFLECT_CASTER)
                victim = ch;

        ref = check_leaf_shield(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;

	spell_level = generate_int(ch->level, ch->level);

	if(victim != NULL)
		(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);

	return TRUE;
}

bool
spec_demon_greater(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int sn = -1;
	int spell_level = 0;
	int ref;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	if (ch->in_room == NULL)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	for (;;)
	{
		int min_level;

		switch (dice(1, 10))
		{
		case 1:
			min_level = 0;
			sn = gsn_curse;
			break;
		case 2:
			min_level = 3;
			sn = gsn_blindness;
			break;
		case 3:
			min_level = 10;
			sn = gsn_dispel_magic;
			break;
		case 4:
			min_level = 18;
			sn = gsn_fireball;
			break;
		case 5:
			gate_in_demons(ch, victim, 1);
			min_level = 20;
			break;
		default:
			min_level = 200;
			break;
		}

		if (ch->level >= min_level)
			break;
		else
			return FALSE;
	}

	if (sn < 0)
		return FALSE;

	spell_level = generate_int(ch->level, ch->level);

        ref =  check_reflection(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;
        else if(ref == REFLECT_CASTER)
                victim = ch;

        ref = check_leaf_shield(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;

	if(victim != NULL)
		(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);

	return TRUE;
}

bool
spec_demon_lord(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int sn = -1;
	int spell_level = 0;
	int ref;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	if (ch->in_room == NULL)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = v_next)
	{
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	for (;;)
	{
		int min_level;

		switch (dice(1, 10))
		{
		case 1:
			min_level = 0;
			sn = gsn_curse;
			break;
		case 2:
			min_level = 3;
			sn = gsn_blindness;
			break;
		case 3:
			min_level = 10;
			sn = gsn_dispel_magic;
			break;
		case 4:
			min_level = 18;
			sn = gsn_fireball;
			break;
		case 5:
			min_level = 8;
			sn = gsn_faerie_fire;
			break;
		case 6:
			super_spec(ch, victim);
			min_level = 20;
			break;
		case 7:
			gate_in_demons(ch, victim, 2);
			min_level = 20;
			break;
		default:
			min_level = 200;
			break;
		}

		if (ch->level >= min_level)
			break;
		else
			return FALSE;
	}

	if (sn < 0)
		return FALSE;

	spell_level = generate_int(ch->level, ch->level);

        ref =  check_reflection(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;
        else if(ref == REFLECT_CASTER)
                victim = ch;

        ref = check_leaf_shield(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;

	if(victim != NULL)
		(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);

	return TRUE;
}

void
gate_in_demons(CHAR_DATA * ch, CHAR_DATA * victim, int size)
{
	int chance;
	CHAR_DATA *gatein;
	AFFECT_DATA af;
	int type, sn = -1;

	chance = ch->level;
	if (number_percent() > ch->level)
	{
		return;
	}

	if (size == 2 && dice(1, 2) == 1)
	{
		size = 1;
	}

	/* what type of deamon? */
	Cprintf(ch, "You call out to the demons of the planes of hell to aid you!\n\r");
	act("$n creates a shimmering black gate.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	if (size == 0)
	{
		type = dice(1, 6);
		sn = gsn_summon_lesser;
		switch (type)
		{
		case 1:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_MANES));
			break;
		case 2:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_SPINAGON));
			break;
		case 3:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_BARLGURA));
			break;
		case 4:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_SUCCUBUS));
			break;
		case 5:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_ABISHAI));
			break;
		case 6:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_OSYLUTH));
			break;
		default:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_MANES));
			break;
		}
	}
	else if (size == 1)
	{
		type = dice(1, 6);
		sn = gsn_summon_greater;
		switch (type)
		{
		case 1:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_CORNUGON));
			break;
		case 2:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_GLABREZU));
			break;
		case 3:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_VROCK));
			break;
		case 4:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_GELUGON));
			break;
		case 5:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_BALOR));
			break;
		case 6:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_PITFIEND));
			break;
		default:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_VROCK));
			break;
		}
	}
	else
	{
		/* gate in brother, or what? */
		type = dice(1, 6);
		sn = gsn_summon_lord;
		switch (type)
		{
		case 1:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_JUIBLEX));
			break;
		case 2:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_BEELZEBUB));
			break;
		case 3:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_ORCUS));
			break;
		case 4:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_MEPHISTOPHILIS));
			break;
		case 5:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_DEMOGORGON));
			break;
		case 6:
			gatein = create_mobile(get_mob_index(MOB_VNUM_DEMON_ASMODEUS));
			break;
		}
	}

	/* do some summoning */
	act("$N apears through your gate.", ch, gatein, gatein, TO_CHAR, POS_RESTING);
	act("$N apears through the gate.", ch, gatein, gatein, TO_ROOM, POS_RESTING);
	char_to_room(gatein, ch->in_room);

	if (ch->master == NULL)
	{
		act("$N can't find your master.", ch, gatein, gatein, TO_CHAR, POS_RESTING);
		act("$N cant find his new master.", ch, gatein, gatein, TO_ROOM, POS_RESTING);
		return;
	}

	if (gatein->master)
		stop_follower(gatein);
	add_follower(gatein, ch->master);
	gatein->leader = ch->master;

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = ch->master->level;
	af.duration = ch->master->level / 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(gatein, &af);

	return;
}

void
super_spec(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int sn = -1;
	int spell_level;
	int ref;

	/* BIG ass if/else */
	if (str_cmp(ch->name, "Juiblex") == 0)
	{
		switch (dice(1, 4))
		{
		case 1:
			sn = gsn_crushing_hand;
			break;
		case 2:
			sn = gsn_gas_breath;
			break;
		case 3:
			sn = gsn_poison;
			break;
		case 4:
			sn = gsn_plague;
			break;
		default:
			return;
		}
	}
	else if (str_cmp(ch->name, "Beelzebub") == 0)
	{
		switch (dice(1, 4))
		{
		case 1:
			sn = gsn_acid_breath;
			break;
		case 2:
			sn = gsn_holy_word;
			break;
		case 3:
			sn = gsn_poison;
			break;
		case 4:
			sn = gsn_plague;
			break;
		default:
			return;
		}
	}
	else if (str_cmp(ch->name, "Orcus") == 0)
	{
		switch (dice(1, 4))
		{
		case 1:
			sn = gsn_weaken;
			break;
		case 2:
			sn = gsn_gas_breath;
			break;
		case 3:
			sn = gsn_poison;
			break;
		case 4:
			sn = gsn_feeblemind;
			break;
		default:
			return;
		}
	}
	else if (str_cmp(ch->name, "Mephistophilis") == 0)
	{
		switch (dice(1, 4))
		{
		case 1:
			sn = gsn_frost_breath;
			break;
		case 2:
			sn = gsn_weaken;
			break;
		case 3:
			sn = gsn_slow;
			break;
		case 4:
			sn = gsn_plague;
			break;
		default:
			return;
		}
	}
	else if (str_cmp(ch->name, "Demogorgon") == 0)
	{
		switch (dice(1, 4))
		{
		case 1:
			sn = gsn_energy_drain;
			break;
		case 2:
			sn = gsn_lightning_breath;
			break;
		case 3:
			sn = gsn_heat_metal;
			break;
		case 4:
			sn = gsn_plague;
			break;
		default:
			return;
		}
	}
	else
	{
		switch (dice(1, 4))
		{
		case 1:
			sn = gsn_holy_word;
			break;
		case 2:
			sn = gsn_weaken;
			break;
		case 3:
			sn = gsn_slow;
			break;
		case 4:
			sn = gsn_fire_breath;
			break;
		default:
			return;
		}
	}

	if (sn < 0)
		return;

	spell_level = generate_int(ch->level, ch->level);
        ref =  check_reflection(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;
        else if(ref == REFLECT_CASTER)
                victim = ch;

        ref = check_leaf_shield(ch, victim);
        if(ref == REFLECT_BOUNCE)
                victim   = NULL;

	if(victim != NULL)
		(*skill_table[sn].spell_fun) (sn, spell_level, ch, victim, TARGET_CHAR);
	return;
}
// replaced by off_crush
bool
spec_tree(CHAR_DATA * ch)
{
	return FALSE;
}

bool spec_lowbie_spellup(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	int spell_level = generate_int(ch->level, ch->level);

	// Count people in room
	for(victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
		if(victim == ch)
			continue;

		if(number_percent() + 50 < victim->level * 10)
			continue;

		if(IS_NPC(victim))
			continue;

		if(number_percent() < 5) {
			do_emote(ch, "glows radiantly.");
			spell_sanctuary(gsn_sanctuary, spell_level, ch, (void*)victim, TARGET_CHAR);
			restore_char(ch);
		}

		if(number_percent() < 5) {
			do_emote(ch, "glows quickly.");
			spell_haste(gsn_haste, spell_level, ch, (void*)victim, TARGET_CHAR);
			restore_char(ch);
		}

		if(number_percent() < 5) {
			do_emote(ch, "glows protectively.");
			spell_blur(gsn_blur, spell_level, ch, (void*)victim, TARGET_CHAR);
			restore_char(ch);
		}

		if(number_percent() < 5) {
			do_emote(ch, "glows strangely.");
			spell_animal_growth(gsn_animal_growth, spell_level, ch, (void*)victim, TARGET_CHAR);
			restore_char(ch);
		}

		if(number_percent() < 5) {
			do_emote(ch, "glows brightly.");
			spell_giant_strength(gsn_giant_strength, spell_level, ch, (void*)victim, TARGET_CHAR);
			restore_char(ch);
		}

		if(number_percent() < 5) {
			do_emote(ch, "glows a bright green.");
			spell_lesser_revitalize(gsn_lesser_revitalize, spell_level, ch, (void*)victim, TARGET_CHAR);
			restore_char(ch);
		}

	}

	return FALSE;
}

bool spec_selina(CHAR_DATA *ch)
{
	CHAR_DATA *victim, *soldier;
	int count = 0;
	int spell_level = generate_int(ch->level, ch->level);

	if(ch->in_room == NULL)
		return FALSE;

	if(ch->hit > MAX_HP(ch))
		ch->hit = MAX_HP(ch);

	if(!ch->fighting)
	{
		// Recover hp quickly not in combat
		if(!is_affected(ch, gsn_dissolution))
			ch->hit += 100;
		else
			ch->hit += 50;

		// Count people in room
		for(victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
			if(victim == ch)
				continue;

			count++;

			if(!is_affected(ch, gsn_fortify)
			&& IS_DRAGON(victim)) {
				do_say(ch, "Prepare to die, FOUL WYRM!\n\r");
				do_emote(ch, "begins to chant a series of incantations!");
				spell_fortify(gsn_fortify, spell_level, ch, (void*)ch, TARGET_CHAR);
				spell_frenzy(gsn_frenzy, spell_level, ch, (void*)ch, TARGET_CHAR);
				spell_mass_protect(gsn_mass_protect, spell_level, ch, (void*)ch, TARGET_CHAR);
				return TRUE;
			}

			if(is_affected(ch, gsn_fortify)
			&& IS_DRAGON(victim)) {
				do_say(ch, "By all that is good, I STRIKE THEE DOWN!\n\r");
				do_kill(ch, victim->name);
				return TRUE;
			}

			if(number_percent() < 80)
			{
				count = 0;
				break;
			}

			if(number_percent() < 2) {
				do_emote(ch, "glows brightly.");
				spell_sanctuary(gsn_sanctuary, spell_level, ch, (void*)ch, TARGET_CHAR);
				spell_haste(gsn_haste, spell_level, ch, (void*)ch, TARGET_CHAR);
				restore_char(ch);
			}

			if(victim->race == race_lookup("human")
			|| victim->race == race_lookup("giant")
			|| victim->race == race_lookup("elf")
			|| victim->race == race_lookup("dwarf")
			|| victim->race == race_lookup("marid"))
			{
				do_say(ch, "Bothersome terran. Begone from my lands!");
				do_emote(ch, "creates a Sliver Soldier to deal with you!");
				soldier = create_mobile(get_mob_index(MOB_VNUM_SLIVER_SOLDIER));
				char_to_room(soldier, ch->in_room);
				soldier->hunting = victim;
				soldier->hunt_timer = 30;
				SET_BIT(soldier->toggles, TOGGLES_NOEXP);

				// Summon a sliver to deal with the terran

				return TRUE;
			}
			else if(victim->race == race_lookup("sliver")
			|| victim->race == race_lookup("troll")
			|| victim->race == race_lookup("gargoyle")
			|| victim->race == race_lookup("kirre"))
			{
				// Random greeting and free spell
				int bonus = dice(1, 7);

				switch(bonus) {
				case 1:
					do_say(ch, "Stay strong, my beloved creatures!");
					spell_giant_strength(gsn_giant_strength, spell_level, ch, (void*)victim, TARGET_CHAR);
					spell_animal_growth(gsn_animal_growth, spell_level, ch, (void*)victim, TARGET_CHAR);
					continue;
				case 2:
					do_look(ch, victim->name);
					if(ch->hit < ch->max_hit) {
						do_say(ch, "Oh my! You are wounded.");
						spell_mass_healing(gsn_mass_healing, spell_level, ch, (void*)victim, TARGET_CHAR);
						return TRUE;
					}
					continue;
				case 3:
					do_say(ch, "Do not fear death, for it is not the end for you.");
					spell_withstand_death(gsn_withstand_death, spell_level, victim, (void*)victim, TARGET_CHAR);
					continue;
				case 4:
					do_say(ch, "Your faith will make you strong.");
					spell_fortify(gsn_fortify, spell_level, ch, (void*)victim, TARGET_CHAR);
					continue;
				case 5:
					do_say(ch, "My magic will lift you up.");
					spell_fly(gsn_fly, spell_level, ch, (void*)victim, TARGET_CHAR);
					continue;
				case 6:
					do_say(ch, "Dominia has never looked better.");
					continue;
				case 7:
					do_emote(ch, "kneels down and prays briefly.");
					continue;
				}

			}
			else {
				continue;
			}
		}

		// If left alone wander
		if(count == 0) {
			if(number_percent() < 92)
				move_char(ch, number_range(0, 5), FALSE);
			else
				spell_teleport(gsn_teleport, ch->level, ch, (void*)ch, TARGET_CHAR);
			return TRUE;
		}

	}
	if((victim = ch->fighting))
	{
		if(!is_affected(ch, gsn_dissolution)) {
			ch->hit += dice(20, 5);
		}
		else
			ch->hit += dice(10, 5);

		// This is in combat
		if(number_percent() < 9)
			return FALSE;

		// Charmie killer
		if(IS_NPC(victim) && number_percent() < 50) {
			Cprintf(ch, "You strike a {RFIERCE{x blow!\n\r");
			act("$n draws back her weapons and strikes a {RFIERCE{x blow!",
				ch, NULL, NULL, TO_ROOM, POS_RESTING);
			act("$n's ultimate lethal strike deals {RUNSPEAKABLE{x damage to $N!!", ch, NULL, victim, TO_ROOM, POS_RESTING);
			act("$n deals a lethal strike to you!", ch, NULL, victim, TO_VICT, POS_RESTING);
			victim->hit = victim->hit / 10;
			return TRUE;
		}

		// Nasty spells
		switch(dice(1, 6)) {
			case 1:
				spell_rain(gsn_rain_of_tears, spell_level, ch, (void*)victim, TARGET_CHAR);
				return TRUE;
			case 2:
				spell_dispel_magic(gsn_dispel_magic, spell_level, ch, (void*)victim, TARGET_CHAR);
				return TRUE;
			case 3:
				spell_hold_person(gsn_hold_person, spell_level, ch, (void*)victim, TARGET_CHAR);
				return TRUE;
			case 4:
				spell_prismatic_spray(gsn_prismatic_spray, spell_level, ch, (void*)victim, TARGET_CHAR);
				return TRUE;
			case 5:
				spell_prismatic_sphere(gsn_prismatic_sphere, spell_level, ch, (void*)ch, TARGET_CHAR);
				return TRUE;
			case 6:
				spell_meteor_swarm(gsn_meteor_swarm, spell_level, ch, (void*)victim, TARGET_CHAR);
				return TRUE;
		}
	}
	return FALSE;
}

// Do damage and knock back.
void tail_swipe(CHAR_DATA *ch, ROOM_INDEX_DATA* room)
{
	CHAR_DATA *vch = NULL, *vch_next = NULL;
	int attempt;
	
	for (vch = room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (is_safe(ch, vch))
                continue;

        if (vch == ch)
                continue;

	    damage(ch, vch, number_range(100, 300), gsn_tail_trip, DAM_BASH, TRUE, TYPE_SKILL);

        for (attempt = 0; attempt < 6; attempt++)
        {
                EXIT_DATA *pexit;
                int door;

                door = number_door();
                if ((pexit = ch->in_room->exit[door]) == 0
                        || pexit->u1.to_room == NULL
                        || (IS_SET(pexit->exit_info, EX_CLOSED)&& !is_affected(vch, gsn_pass_door))
                        || (IS_NPC(vch) && IS_SET(vch->imm_flags, IMM_CHARM))
                        || (IS_NPC(vch) && IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB))
						// Knock people away from the dragon, never towards.
						|| pexit->u1.to_room == ch->in_room)
                        continue;

                Cprintf(vch, "You are flung from the room!\n\r");
                act("$n is flung from the room!", vch, NULL, NULL, TO_ROOM, POS_RESTING);
				stop_fighting(vch, TRUE);
                move_char(vch, door, FALSE);
                WAIT_STATE(vch, number_range(20, 30));
                break;
        }
    }

    return;

}

bool spec_nether_dragon(CHAR_DATA *ch)
{
	CHAR_DATA *vch;
	AFFECT_DATA af, *paf = NULL;
	DESCRIPTOR_DATA *d;
	EXIT_DATA *pexit;
	int action = 0;
	int exit;
	int i;

	if(ch->in_room == NULL)
		return FALSE;

	if(ch->hit > MAX_HP(ch))
		ch->hit = MAX_HP(ch);

	if(ch->fighting == NULL)
	{
		// Do nothing 75%
		if(!is_affected(ch, gsn_dissolution))
			ch->hit += 100;
		else
			ch->hit += 50;

		if(number_percent() < 75)
			return FALSE;
		
		action = dice(1, 8);
		switch(action) {
		case 1:
			// Notify area of its presence
			act("$n {YROARS{x, shaking the entire area!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			for (d = descriptor_list; d != NULL; d = d->next) {
				if(d->character == NULL)
					continue;

			        vch = d->character;

				if(vch->in_room && ch->in_room
				&& vch->in_room->area == ch->in_room->area
				&& !is_affected(vch, gsn_wail)) {
                        		Cprintf(vch, "The whole room is filled with the {RROAR{x of a gigantic dragon!\n\r");
					af.where = TO_AFFECTS;
					af.type = gsn_wail;
					af.level = ch->level;
					af.duration = 15;
					af.location = APPLY_NONE;
					af.modifier = 0;
					af.bitvector = 0;
					affect_to_char(vch, &af);

					Cprintf(vch, "You can't hear anything!\n\r");
				}
			}
			return TRUE;
		case 2:
		// Sniff hungrily
			do_emote(ch, "sniffs at you with a hungry gleam in its eyes. Not good.");
			return TRUE;
		case 3:
		// Stomach growl
			do_emote(ch, "growls fiercely at its own stomach! (which promptly growls back)");
			return TRUE;
		// Look for Bosco
		case 4:
			do_emote(ch, "swishes its head back and forth, looking around for its master, Bosco.");
			return TRUE;
		// Rage against prison
		case 5:
			do_emote(ch, "rages against its prison!");
			return TRUE;
		case 6:
		case 7:
		case 8:
		// Walk around
			move_char(ch, number_range(0, 5), FALSE);
			return TRUE;
		}
		
	}

	if(ch->fighting) {
		if(!is_affected(ch, gsn_dissolution))
			ch->hit += dice(20, 5);
		else
			ch->hit += dice(10, 5);

		// Do nothing 20%
		if(number_percent() < 20)
			return FALSE;

		action = dice(1, 6);
		switch(action) {
		case 1:
		// Wing Buffet (damage stun and lag against secondary attackers
			do_emote(ch, "buffets the area with its wings!");
			for(vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
				if(vch != ch
				&& vch != ch->fighting) {
					act("$n is buffeted by a blast from $N's wings!", vch, NULL, ch, TO_ROOM, POS_RESTING);
					Cprintf(ch, "You caught in blast of wind and dust from the wing buffet!");
					damage(ch, vch, number_range(100, 400), gsn_bash, DAM_BASH, FALSE, TYPE_SKILL);
					WAIT_STATE(vch, number_range(36, 96));
					DAZE_STATE(vch, 60);
				}
			}
			return TRUE;
		case 2:
		// Devour charmie (and heal hp lol) 70% chance
			if(number_percent() < 30)
				return FALSE;

			for(vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
				if(vch != ch
				&& IS_NPC(vch)) {
					act("$n is {RDEVOURS{x $N!", vch, NULL, ch, TO_ROOM, POS_RESTING);
					do_emote(ch, "looks much better!");
					ch->hit += vch->hit / 4;
					if(ch->hit > ch->max_hit)
						ch->hit = ch->max_hit;
					extract_char(vch, TRUE);
					break;
				}
			}
			
			return TRUE;
		case 3:
		// Tail swipe (damage multiple rooms and knockback)
		
			do_emote(ch, "flails out with its huge tail, clearing the area!");

			tail_swipe(ch, ch->in_room);

			/* go out each exit */
			for (exit = 0; exit < 6; exit++)
			{
				pexit = ch->in_room->exit[exit];
				if (pexit != NULL &&
				pexit->u1.to_room != NULL)           
					tail_swipe(ch, pexit->u1.to_room);
			}

			return TRUE;
		case 4:
		// Lift off and stop fight
			do_emote(ch, "flaps its wings and takes to the air, moving out of reach.");
			for(vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
				stop_fighting(vch, FALSE);
			}
		      	return TRUE;
		case 5:
		// Pre-charge breath
			do_emote(ch, "inhales deeply... but nothing happens {Wyet.{x");
			af.where = TO_AFFECTS;
			af.type = gsn_delayed_blast_fireball;
			af.level = ch->level;
			af.duration = 5;
			af.location = APPLY_DAMROLL;
			af.modifier = 1;
			af.bitvector = 0;

			affect_join(ch, &af);
			return TRUE;
		case 6:
		// Super breath
			do_emote(ch, "inhales deeply and unleashes a massive breath attack!");
		        (*skill_table[gsn_fire_breath].spell_fun) (gsn_fire_breath, generate_int(ch->level, ch->level), ch, ch, TARGET_CHAR);

			paf = affect_find(ch->affected, gsn_delayed_blast_fireball);
			if(paf == NULL)
				return TRUE;
			for(i=0; i < paf->modifier; i++) {
				(*skill_table[gsn_fire_breath].spell_fun) (gsn_fire_breath, generate_int(ch->level, ch->level), ch, ch, TARGET_CHAR);

			}
			affect_strip(ch, gsn_delayed_blast_fireball);
			return TRUE;
		}
	}

	return FALSE;
}

// Each of the 5 sliver pods contain an item needed to spawn the adamantite sliver guardian.
// // They will spawn LARGE numbers of sliver soldiers when attacked, but are otherwise not
// // very tough. A couple of medusa would be very useful!
// // They recover hp quickly and move/teleport.

bool spec_sliver_pod(CHAR_DATA *ch)
{
	CHAR_DATA *vch = NULL, *soldier = NULL;
	int victim_counter = 0;
	int current_victim = 0;
	int victim_index;

	if(ch->in_room == NULL)
		return FALSE;

	if(ch->hit > MAX_HP(ch))
		ch->hit = MAX_HP(ch);

	if(ch->fighting == NULL) {
		if(!is_affected(ch, gsn_dissolution))
			ch->hit += 100;

		if(number_percent() < 92)
			move_char(ch, number_range(0, 5), FALSE);
		else
			spell_teleport(gsn_teleport, ch->level, ch, (void*)ch, TARGET_CHAR);

		return TRUE;
	}

	// Select a target from everyone in the room.
	// Set it to aggressive, set it to hunt them.
	if(ch->fighting != NULL) 
	{
		if(number_percent() < 90)
			return FALSE;

		do_emote(ch, "breaks open with a wet splosh, and something emerges!");

												soldier = create_mobile(get_mob_index(MOB_VNUM_SLIVER_SOLDIER));												char_to_room(soldier, ch->in_room);
		SET_BIT(soldier->toggles, TOGGLES_NOEXP);
		victim_counter = 0;

		// Count the number of ppl in the room
		for(vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
			if(!IS_NPC(vch) && vch->level > 29)
				victim_counter++;

		if(victim_counter == 0)
		{
			do_flee(ch, "");
			return FALSE;
		}

		// Select one.
		victim_index = number_range(1, victim_counter);

		current_victim = 0;

		for(vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
			if(!IS_NPC(vch) && vch->level > 29)
				current_victim++;

			if(current_victim == victim_index) {
				soldier->hunting = vch;
					soldier->hunt_timer = 30;
					break;
			}
		}
		return TRUE;
	}
	return FALSE;
}

// Uses the descriptor list and area->nplayer
// and ranger finder to locate a victim for a
// volley of spikes. Do not return null unless
// there is no one in range.
CHAR_DATA *find_spikes_target(CHAR_DATA *ch)
{
	CHAR_DATA *victim = NULL;
	int victimcount = ch->in_room->area->nplayer + 1;
	DESCRIPTOR_DATA *d;

	for (d = descriptor_list; d != NULL; d = d->next) {
		if(d->character == NULL)
			continue;

		victim = d->character;

		if(victim->in_room
		&& !IS_NPC(victim)
		&& victim->in_room->area == ch->in_room->area
		&& victim->in_room != ch->in_room
		&& victim->level > 29
		&& !is_safe(ch, victim)
		&& number_range(1, victimcount) == 1) {
			return victim;
		}
	}

	return NULL;
}

// Deal the damage for one volley of adamantite spikes
void launch_spikes(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int dam;

	dam = 50 + dice(10, 6);
	damage(ch, victim, dam, gsn_barrage, DAM_PIERCE, TRUE, TYPE_MAGIC);
	poison_effect(victim, ch->level, dam, TARGET_CHAR);
}

bool spec_adamantite_sliver(CHAR_DATA *ch)
{
	int i, venom_count;
	int spell_level = 0;
	AFFECT_DATA af;
	CHAR_DATA *victim = NULL;

	if(ch->in_room == NULL)
		return FALSE;

	if(ch->hit > MAX_HP(ch))
		ch->hit = MAX_HP(ch);

        // Launch spikes or barrage of spikes randomly.
	// Can hit anyone in the zone.
	victim = find_spikes_target(ch);
	if(victim != NULL) {
		// Barrage
		if(number_percent() < 20) {
			do_emote(ch, "fires a massive barrage of adamantite spikes from his backplate!");
			act("$N is struck by the volley!", ch, NULL, victim, TO_ROOM, POS_RESTING);
			act("Razor sharp adamantite spikes rain down on the area!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(victim, "Razor sharp adamantite spikes rain down on the area!\n\r");
			venom_count = number_range(2, 6);
			for(i = 0; i < venom_count; i++) {
				launch_spikes(ch, victim);
			}
		}
		// Single
		else {
			do_emote(ch, "fires a razor sharp adamantite spike from his backplate!");
			act("$N is struck by the spike!", ch, NULL, victim, TO_ROOM, POS_RESTING);
			act("Razor sharp adamantite spikes rain down on the area!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(victim, "Razor sharp adamantite spikes rain down on the area!\n\r");
			launch_spikes(ch, victim);
		}
	}

	if(ch->fighting == NULL) {
		// Recover hp quickly not in combat
		if(!is_affected(ch, gsn_dissolution))
			ch->hit += 100;
		else
			ch->hit += 50;

		if(number_percent() < 70)
			move_char(ch, number_range(0, 5), FALSE);

		return TRUE;
	}
	else {
		if(number_percent() < 60)
			return FALSE;

		if(!is_affected(ch, gsn_dissolution))
			ch->hit += dice(20, 5);

		spell_level = generate_int(ch->level, ch->level);

		switch(dice(1, 6)) {
			// Rain of Tears
			case 1:
				spell_rain(gsn_rain_of_tears, spell_level, ch, (void*)victim, TARGET_CHAR);
				return TRUE;
			// Acid Breath
			case 2:
				(*skill_table[gsn_acid_breath].spell_fun) (gsn_acid_breath, spell_level, ch, ch->fighting, TARGET_CHAR);
				return TRUE;
			// Venom Spray
			case 3:
				victim = ch->fighting;
				act("$n is completed immersed in venom by $N!", victim, NULL, ch, TO_ROOM, POS_RESTING);
				Cprintf(victim, "You are completely immersed in venom by %s!\n\r", ch->short_descr);

				venom_count = dice(2, 5);

				for(i = 0; i < venom_count; i++) {
					spell_poison(gsn_poison, spell_level, ch, victim, TARGET_CHAR);
					spell_plague(gsn_plague, spell_level, ch, victim, TARGET_CHAR);
				}

				return TRUE;
			// Carapace Hardening
		      case 4:
				do_emote(ch, "glows and his adamantite carapace grows stronger!");
				af.where = TO_AFFECTS;
				af.type = gsn_shield;
				af.level = ch->level;
				af.duration = 2;
				af.location = APPLY_DAMAGE_REDUCE;
				af.modifier = 3;
				af.bitvector = 0;
				affect_join(ch, &af);
				return TRUE;
			// Mandible Sharpening
			case 5:
				do_emote(ch, "glows and his adamantite mandibles grow sharper!");
				af.where = TO_AFFECTS;
				af.type = gsn_sharpen;
				af.level = ch->level;
				af.duration = 2;
				af.location = APPLY_DAMROLL;
				af.modifier = 5;
				af.bitvector = 0;
				affect_join(ch, &af);
				return TRUE;
			// Area of Effect Confusion and Draw into Battle
			case 6:
				act("$n's wings being to buzz, drawing you in.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
				for(victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
					if(victim != ch
					&& !is_affected(victim, gsn_confusion)) {
						Cprintf(victim, "You feel disoriented by all the buzzing.\n\r");
						af.where = TO_AFFECTS;
						af.type = gsn_confusion;
						af.level = ch->level;
						af.duration = 10;
						af.bitvector = 0;
						af.modifier = -4;
						af.location = APPLY_HITROLL;
						affect_to_char(victim, &af);
					}
					if(!is_safe(ch, victim) && victim->fighting == NULL) {
						set_fighting(victim, ch);
					}
				}
				return TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

bool
spec_tourney_cure(CHAR_DATA * ch)
{
	CHAR_DATA* vch;
	int spell_level;

	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (IS_NPC(vch) || IS_IMMORTAL(vch))
			continue;

		if (number_percent() < 96) {
			continue;
			
		}

		if (is_affected(vch, gsn_blindness))
		{
			affect_strip(vch, gsn_blindness);
			Cprintf(vch, "Delstar shines down upon you and returns your sight!\n\r");
			act("$n's eyes glow with divine light, and then focus on you.", vch, NULL, NULL, TO_ROOM, POS_RESTING);

		}	

		if (is_affected(vch, gsn_sleep))
		{
			affect_strip(vch, gsn_sleep);
			Cprintf(vch, "Daisy approaches you in your dreams, and commands you to wake!\n\r");
			act("$n's eyes glow with divine light, and wakes up!", vch, NULL, NULL, TO_ROOM, POS_RESTING);

		}	

	}

	return FALSE;
}

bool
spec_king_of_the_hill(CHAR_DATA * ch)
{
	CHAR_DATA* vict1;
	CHAR_DATA* vict2;
	CHAR_DATA* vch;
	OBJ_DATA* obj;
	OBJ_DATA* give;
	char buf[MAX_STRING_LENGTH];

	//most of the time, don't declare a new king
	if (number_percent() < 85) {
		return FALSE;
	}

	vict1 = NULL;
	vict2 = NULL;

	//find out who the new kings are
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (IS_NPC(vch) || IS_IMMORTAL(vch))
			continue;
		vict1 = vict2;
		vict2 = vch;
	}

	obj = ch->carrying;

	if (obj == NULL)
	{
		act("Mob has no objects to give.", ch, NULL, NULL, TO_ROOM, POS_SLEEPING);
		return FALSE;
	}

	give = create_object(obj->pIndexData, 0);
	clone_object(obj, give);

	if (vict1 != NULL)
		obj_to_char(give, vict1);

	give = create_object(obj->pIndexData, 0);
	clone_object(obj, give);

	if (vict2 != NULL)
		obj_to_char(give, vict2);

	if (vict2 != NULL && vict1 != NULL)
	{
		sprintf(buf, "{M%s and %s are the current kings of the hill.{x", vict1->name, vict2->name);
		do_zecho(ch, buf);
	}
	else if (vict2 != NULL)
	{
		sprintf(buf, "{M%s is the current king of the hill.{x", vict2->name);
		do_zecho(ch, buf);
	}


	return TRUE;
}

bool spec_assault(CHAR_DATA *ch)
{
	CHAR_DATA *vch;

	// Seek out new prey
	if(!ch->fighting) {
		if(number_percent() < 35) {
                        move_char(ch, number_range(0, 5), FALSE);
			return TRUE;
		}

		if(ch->in_room == NULL)
			return FALSE;

		for(vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
			if(IS_NPC(vch)
			&& !is_name("assault", vch->name)
			&& !is_safe(ch, vch)) {
				multi_hit(ch, vch, TYPE_UNDEFINED);
				return TRUE;
			}
                }
	}

	return FALSE;
}

bool
spec_unremort(CHAR_DATA * ch)
{
        return TRUE;
}
