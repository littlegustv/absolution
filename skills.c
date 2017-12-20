/* revision 1.1 - August 1 1999 - making it compilable under g++ */
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
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "clan.h"
#include "utils.h"


/* command procedures needed */
DECLARE_DO_FUN(do_groups);
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_say);

/* Externals */
void advance_weapon(CHAR_DATA *, OBJ_DATA *, int);
extern int double_xp_ticks;

// Must go to a trainer to cash in patron points as exp.
void advance_patron(CHAR_DATA *gch, CHAR_DATA *trainer)
{
	int weapon_count = 0;
	OBJ_DATA *wield = NULL;
	int xp = gch->pass_along;
	CHAR_DATA *vch;
	int vassalpts;
	int weapon_xp;
	int maxed = FALSE;
	int leftover = 0;
	int temp_double_xp;

	if(xp == 0) {
		act("{y$N says, 'You don't have any patron points to claim.'{x", gch, NULL, trainer, TO_CHAR, POS_RESTING);
		return;
	}

	if(gch->pass_along_limit == 0) {
		act("{y$N says, 'You must advance a level before you can claim more patron points.'{x",gch, NULL, trainer, TO_CHAR, POS_RESTING);
		return;
	}

	if(xp > gch->pass_along_limit) {
		leftover = xp - gch->pass_along_limit;
		xp = gch->pass_along_limit;
		maxed = TRUE;
	}

	act("$N rewards you for helping your vassals!",gch, NULL, trainer, TO_CHAR, POS_RESTING);
	act("$n claims $s reward for being a patron.", gch, NULL, NULL, TO_ROOM, POS_RESTING);
	gch->pass_along_limit -= xp;

	// Intelligent weapons get some xp.
        // Watch out for dual wield.
        weapon_count = 0;
        if((wield = get_eq_char(gch, WEAR_WIELD)) != NULL
        && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT))
        	weapon_count++;
	if((wield = get_eq_char(gch, WEAR_DUAL)) != NULL
        && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT))
        	weapon_count++;
	if((wield = get_eq_char(gch, WEAR_RANGED)) != NULL
	&& IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT))
		weapon_count++;

	if((wield = get_eq_char(gch, WEAR_WIELD)) != NULL
        && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT)) {
		weapon_xp = xp / (5 + (5 * weapon_count));
                Cprintf(gch, "%s receives %d experience points.\n\r", capitalize(wield->short_descr), weapon_xp);
                advance_weapon(gch, wield, weapon_xp);
	}

	if((wield = get_eq_char(gch, WEAR_DUAL)) != NULL
        && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT)) {
        	weapon_xp = xp / (5 + (5 * weapon_count));
                Cprintf(gch, "%s receives %d experience points.\n\r", capitalize(wield->short_descr), weapon_xp);
                advance_weapon(gch, wield, weapon_xp);
	}

	if((wield = get_eq_char(gch, WEAR_RANGED)) != NULL
	&& IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT)) {
		weapon_xp = xp / (5 + (5 * weapon_count));
		Cprintf(gch, "%s receives %d experience points.\n\r", capitalize(wield->short_descr), weapon_xp);
		advance_weapon(gch, wield, weapon_xp);
	}     

	// Convert some exp into patron/vassal points
        // Patron code starts here
        vassalpts = xp / 10;

	if (gch->patron != NULL && !IS_SET(gch->toggles, TOGGLES_PLEDGE))
	{
        	if ((vch = get_char_world(gch, gch->patron, TRUE)) != NULL)
                {
	                if (gch->level >= vch->level)
                        {
        	                Cprintf(gch, "{cYou and your patron stands as equals now.{x\n\r");
                                vassalpts = 0;
                        }

                        vassalpts = vassalpts + gch->to_pass;
                        Cprintf(vch, "{cYou receive %d patron points from your vassal, %s.{x\n\r", vassalpts, gch->name);
                        Cprintf(gch, "{cYour patron %s receives %d vassal points.{x\n\r", vch->name, vassalpts);
                        xp = xp * 9 / 10;
                        vch->pass_along = vch->pass_along + vassalpts;
                        if (gch->to_pass > 0)
			{
                        	gch->to_pass = 0;
                                /* prevents drop starts for accumulation */
                                save_char_obj(gch, FALSE);
                        }
		}
                else
                {
                	Cprintf(gch, "{cYou receive %d vassal points.{x\n\r", vassalpts);
                        /* builds the bank while patron is offline. */
                        gch->to_pass = gch->to_pass + vassalpts;
                        xp = xp * 9 / 10;
                }
	}

	Cprintf(gch, "You receive %d experience points.\n\r", xp);
	temp_double_xp = double_xp_ticks;
	double_xp_ticks = 0;
        gain_exp(gch, xp);
	double_xp_ticks = temp_double_xp;

	if(maxed) {
                act("{y$N says, 'You must advance a level before you can claim more patron points.'{x",gch, NULL, trainer, TO_CHAR, POS_RESTING);
	}

	gch->pass_along = leftover;

	return;
}

/* used to get new skills */
void
do_gain(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *trainer;
	int gn = 0, sn = 0;

	if (IS_NPC(ch))
		return;

	/* find a trainer */
	for (trainer = ch->in_room->people;
		 trainer != NULL;
		 trainer = trainer->next_in_room)
		if (IS_NPC(trainer) && IS_SET(trainer->act, ACT_GAIN))
			break;

	if (trainer == NULL || !can_see(ch, trainer))
	{
		Cprintf(ch, "You can't do that here.\n\r");
		return;
	}

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		do_say(trainer, "Pardon me?");
		return;
	}

	if (!str_prefix(arg, "list"))
	{
		int col;

		col = 0;

		Cprintf(ch, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r",
				"group", "cost", "group", "cost", "group", "cost");

		for (gn = 0; gn < MAX_GROUP; gn++)
		{
			if (group_table[gn].name == NULL)
				break;

			if (!ch->pcdata->group_known[gn]
				&& group_table[gn].rating[ch->charClass] > 0)
			{
				Cprintf(ch, "%-18s %-5d ",
				group_table[gn].name, group_table[gn].rating[ch->charClass]);
				if (++col % 3 == 0)
					Cprintf(ch, "\n\r");
			}
		}
		if (col % 3 != 0)
			Cprintf(ch, "\n\r");

		Cprintf(ch, "\n\r");

		col = 0;

		Cprintf(ch, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r",
				"skill", "cost", "skill", "cost", "skill", "cost");

		for (sn = 0; sn < MAX_SKILL; sn++)
		{
			if (skill_table[sn].name == NULL)
				break;

			if (!ch->pcdata->learned[sn]
				&& skill_table[sn].rating[ch->charClass] > 0
				&& skill_table[sn].spell_fun == spell_null
				&& skill_table[sn].remort < 1)
			{
				Cprintf(ch, "%-18s %-5d ",
				skill_table[sn].name, skill_table[sn].rating[ch->charClass]);
				if (++col % 3 == 0)
					Cprintf(ch, "\n\r");
			}
		}
		if (col % 3 != 0)
			Cprintf(ch, "\n\r");
		return;
	}

	if (!str_prefix(arg, "patron"))
	{
		advance_patron(ch, trainer);
		return;
	}

	if (!str_prefix(arg, "convert"))
	{
		if (ch->practice < 10)
		{
			act("$N tells you 'You are not yet ready.'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}

		act("$N helps you apply your practice to training", ch, NULL, trainer, TO_CHAR, POS_RESTING);
		ch->practice -= 10;
		ch->train += 1;
		return;
	}

	if (!str_prefix(arg, "points"))
	{
		if (ch->train < 2)
		{
			act("$N tells you 'You are not yet ready.'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}

		if (ch->pcdata->points <= 40)
		{
			act("$N tells you 'There would be no point in that.'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}

		act("$N trains you, and you feel more at ease with your skills.", ch, NULL, trainer, TO_CHAR, POS_RESTING);

		ch->train -= 2;
		ch->pcdata->points -= 1;
		ch->exp = exp_per_level(ch, ch->pcdata->points) * ch->level;
		return;
	}

	/* else add a group/skill */

	gn = group_lookup(argument);
	if (gn > 0)
	{
		if (ch->pcdata->group_known[gn])
		{
			act("$N tells you 'You already know that group!'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}

		if (group_table[gn].rating[ch->charClass] <= 0)
		{
			act("$N tells you 'That group is beyond your powers.'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}

		if (ch->train < group_table[gn].rating[ch->charClass])
		{
			act("$N tells you 'You are not yet ready for that group.'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}

		/* add the group */
		gn_add(ch, gn);
		act("$N trains you in the art of $t", ch, group_table[gn].name, trainer, TO_CHAR, POS_RESTING);
		ch->train -= group_table[gn].rating[ch->charClass];
		return;
	}

	sn = skill_lookup(argument);
	if (sn > -1)
	{
		if (skill_table[sn].spell_fun != spell_null)
		{
			act("$N tells you 'You must learn the full group.'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}


		if (ch->pcdata->learned[sn])
		{
			act("$N tells you 'You already know that skill!'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}

		if (skill_table[sn].remort > 0)
		{
			act("$N tells you 'You can't gain that skill.'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}

		if (skill_table[sn].rating[ch->charClass] <= 0)
		{
			act("$N tells you 'That skill is beyond your powers.'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}

		if (ch->train < skill_table[sn].rating[ch->charClass])
		{
			act("$N tells you 'You are not yet ready for that skill.'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
			return;
		}

		/* add the skill */
		ch->pcdata->learned[sn] = 1;
		act("$N trains you in the art of $t", ch, skill_table[sn].name, trainer, TO_CHAR, POS_RESTING);
		ch->train -= skill_table[sn].rating[ch->charClass];
		return;
	}

	act("$N tells you 'I do not understand...'", ch, NULL, trainer, TO_CHAR, POS_RESTING);
}




/* RT spells and skills show the players spells (or skills) */

void
do_spells(CHAR_DATA * ch, char *argument)
{
	BUFFER *buffer;
	char arg[MAX_INPUT_LENGTH];
	char spell_list[53][MAX_STRING_LENGTH];
	char spell_columns[53];
	int sn, level, min_lev = 1, max_lev = 53, mana;
	bool fAll = FALSE, found = FALSE;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	if (argument[0] != '\0')
	{
		fAll = TRUE;

		if (str_prefix(argument, "all"))
		{
			argument = one_argument(argument, arg);
			if (!is_number(arg))
			{
				Cprintf(ch, "Arguments must be numerical or all.\n\r");
				return;
			}
			max_lev = atoi(arg);

			if (max_lev < 1 || max_lev > 53)
			{
				Cprintf(ch, "Levels must be between 1 and %d.\n\r", 53);
				return;
			}

			if (argument[0] != '\0')
			{
				argument = one_argument(argument, arg);
				if (!is_number(arg))
				{
					Cprintf(ch, "Arguments must be numerical or all.\n\r");
					return;
				}
				min_lev = max_lev;
				max_lev = atoi(arg);

				if (max_lev < 1 || max_lev > 53)
				{
					Cprintf(ch, "Levels must be between 1 and %d.\n\r", 53);
					return;
				}

				if (min_lev > max_lev)
				{
					Cprintf(ch, "That would be silly.\n\r");
					return;
				}
			}
		}
	}


	/* initialize data */
	for (level = 0; level < 52 + 1; level++)
	{
		spell_columns[level] = 0;
		spell_list[level][0] = '\0';
	}

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;

		if (strcmp(skill_table[sn].name, "acid breath") == 0
			|| strcmp(skill_table[sn].name, "fire breath") == 0
			|| strcmp(skill_table[sn].name, "gas breath") == 0
			|| strcmp(skill_table[sn].name, "frost breath") == 0
			|| strcmp(skill_table[sn].name, "lightning breath") == 0)
			continue;

		if ((level = skill_table[sn].skill_level[ch->charClass]) < 53
			&& level >= min_lev && level <= max_lev
			&& skill_table[sn].spell_fun != spell_null
			&& ch->pcdata->learned[sn] > 0)
		{
			found = TRUE;
			level = skill_table[sn].skill_level[ch->charClass];
			if (ch->level < level)
				sprintf(buf, "%-18s n/a      ", skill_table[sn].name);
			else
			{
				mana = UMAX(skill_table[sn].min_mana,
							100 / (2 + ch->level - level));
				sprintf(buf, "%-18s %3d mana  ", skill_table[sn].name, mana);
			}

			if (spell_list[level][0] == '\0')
				sprintf(spell_list[level], "\n\rLevel %2d: %s", level, buf);
			else
				/* append */
			{
				if (++spell_columns[level] % 2 == 0)
					strcat(spell_list[level], "\n\r          ");
				strcat(spell_list[level], buf);
			}
		}
	}

	/* return results */

	if (!found)
	{
		Cprintf(ch, "No spells found.\n\r");
		return;
	}

	buffer = new_buf();
	for (level = 0; level < 53; level++)
		if (spell_list[level][0] != '\0')
			add_buf(buffer, spell_list[level]);
	add_buf(buffer, "\n\r");
	page_to_char(buf_string(buffer), ch);
	free_buf(buffer);
}

void
do_skills(CHAR_DATA * ch, char *argument)
{
	BUFFER *buffer;
	char arg[MAX_INPUT_LENGTH];
	char skill_list[53][MAX_STRING_LENGTH];
	char skill_columns[53];
	int sn, level, min_lev = 1, max_lev = 53;
	bool fAll = FALSE, found = FALSE;
	char buf[MAX_STRING_LENGTH];
	bool breath;

	breath = FALSE;
	if (IS_NPC(ch))
		return;

	if (argument[0] != '\0')
	{
		fAll = TRUE;

		if (str_prefix(argument, "all"))
		{
			argument = one_argument(argument, arg);
			if (!is_number(arg))
			{
				Cprintf(ch, "Arguments must be numerical or all.\n\r");
				return;
			}
			max_lev = atoi(arg);

			if (max_lev < 1 || max_lev > 53)
			{
				Cprintf(ch, "Levels must be between 1 and %d.\n\r", 53);
				return;
			}

			if (argument[0] != '\0')
			{
				argument = one_argument(argument, arg);
				if (!is_number(arg))
				{
					Cprintf(ch, "Arguments must be numerical or all.\n\r");
					return;
				}
				min_lev = max_lev;
				max_lev = atoi(arg);

				if (max_lev < 1 || max_lev > 53)
				{
					Cprintf(ch, "Levels must be between 1 and %d.\n\r", 53);
					return;
				}

				if (min_lev > max_lev)
				{
					Cprintf(ch, "That would be silly.\n\r");
					return;
				}
			}
		}
	}


	/* initialize data */
	for (level = 0; level < 53; level++)
	{
		skill_columns[level] = 0;
		skill_list[level][0] = '\0';
	}

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;


		breath = (sn == gsn_acid_breath || sn == gsn_fire_breath || sn == gsn_gas_breath ||
				  sn == gsn_frost_breath || sn == gsn_lightning_breath);


		if (ch->pcdata->learned[sn] > 0 && (breath || ((level = skill_table[sn].skill_level[ch->charClass]) < 53 &&
													   level >= min_lev &&
													   level <= max_lev &&
								  skill_table[sn].spell_fun == spell_null)))
		{
			found = TRUE;
			level = skill_table[sn].skill_level[ch->charClass];
			if (ch->level < level)
				sprintf(buf, "%-18s n/a      ", breath ? "breath" : skill_table[sn].name);
			else
				sprintf(buf, "%-18s %3d%%      ", breath ? "breath" : skill_table[sn].name, ch->pcdata->learned[sn]);

			if (skill_list[level][0] == '\0')
				sprintf(skill_list[level], "\n\rLevel %2d: %s", level, buf);
			else
				/* append */
			{
				if (++skill_columns[level] % 2 == 0)
					strcat(skill_list[level], "\n\r          ");
				strcat(skill_list[level], buf);
			}
		}
	}

	/* return results */

	if (!found)
	{
		Cprintf(ch, "No skills found.\n\r");
		return;
	}

	buffer = new_buf();
	for (level = 0; level < 53; level++)
		if (skill_list[level][0] != '\0')
			add_buf(buffer, skill_list[level]);
	add_buf(buffer, "\n\r");
	page_to_char(buf_string(buffer), ch);
	free_buf(buffer);
}

/* shows skills, groups and costs (only if not bought) */
void
list_group_costs(CHAR_DATA * ch)
{
	int gn, sn, col;

	if (IS_NPC(ch))
		return;

	col = 0;

	Cprintf(ch, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "group", "cp", "group", "cp", "group", "cp");

	for (gn = 0; gn < MAX_GROUP; gn++)
	{
		if (group_table[gn].name == NULL)
			break;

		if (!ch->gen_data->group_chosen[gn]
			&& !ch->pcdata->group_known[gn]
			&& group_table[gn].rating[ch->charClass] > 0)
		{
			Cprintf(ch, "%-18s %-5d ", group_table[gn].name,
					group_table[gn].rating[ch->charClass]);
			if (++col % 3 == 0)
				Cprintf(ch, "\n\r");
		}
	}
	if (col % 3 != 0)
		Cprintf(ch, "\n\r");
	Cprintf(ch, "\n\r");

	col = 0;

	Cprintf(ch, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "skill", "cp", "skill", "cp", "skill", "cp");

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;

		if (!ch->gen_data->skill_chosen[sn]
			&& ch->pcdata->learned[sn] == 0
			&& skill_table[sn].spell_fun == spell_null
			&& skill_table[sn].rating[ch->charClass] > 0
			&& skill_table[sn].remort < 1)
		{
			Cprintf(ch, "%-18s %-5d ", skill_table[sn].name,
					skill_table[sn].rating[ch->charClass]);
			if (++col % 3 == 0)
				Cprintf(ch, "\n\r");
		}
	}
	if (col % 3 != 0)
		Cprintf(ch, "\n\r");
	Cprintf(ch, "\n\r");

	Cprintf(ch, "Creation points: %d\n\r", ch->pcdata->points);
	Cprintf(ch, "Experience per level: %d\n\r",
			exp_per_level(ch, ch->gen_data->points_chosen));
	return;
}


void
list_group_chosen(CHAR_DATA * ch)
{
	int gn, sn, col;

	if (IS_NPC(ch))
		return;

	col = 0;

	Cprintf(ch, "%-18s %-5s %-18s %-5s %-18s %-5s", "group", "cp", "group", "cp", "group", "cp\n\r");

	for (gn = 0; gn < MAX_GROUP; gn++)
	{
		if (group_table[gn].name == NULL)
			break;

		if (ch->gen_data->group_chosen[gn]
			&& group_table[gn].rating[ch->charClass] > 0)
		{
			Cprintf(ch, "%-18s %-5d ", group_table[gn].name,
					group_table[gn].rating[ch->charClass]);
			if (++col % 3 == 0)
				Cprintf(ch, "\n\r");
		}
	}
	if (col % 3 != 0)
		Cprintf(ch, "\n\r");
	Cprintf(ch, "\n\r");

	col = 0;

	Cprintf(ch, "%-18s %-5s %-18s %-5s %-18s %-5s", "skill", "cp", "skill", "cp", "skill", "cp\n\r");

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;

		if (ch->gen_data->skill_chosen[sn]
			&& skill_table[sn].rating[ch->charClass] > 0)
		{
			Cprintf(ch, "%-18s %-5d ", skill_table[sn].name,
					skill_table[sn].rating[ch->charClass]);
			if (++col % 3 == 0)
				Cprintf(ch, "\n\r");
		}
	}
	if (col % 3 != 0)
		Cprintf(ch, "\n\r");
	Cprintf(ch, "\n\r");

	Cprintf(ch, "Creation points: %d\n\r", ch->gen_data->points_chosen);
	Cprintf(ch, "Experience per level: %d\n\r",
			exp_per_level(ch, ch->gen_data->points_chosen));
	return;
}

int
exp_per_level(CHAR_DATA * ch, int points)
{
	int expl, inc;

	if (IS_NPC(ch))
		return 1000;

	expl = 1000;
	inc = 500;

	if (points < 40)
	{
		expl = 1000 * (pc_race_table[ch->race].class_mult[ch->charClass] ?
			   pc_race_table[ch->race].class_mult[ch->charClass] / 100 : 1);
		return UMAX(expl, 2000);
	}

	/* processing */
	points -= 40;

	while (points > 9)
	{
		expl += inc;
		points -= 10;
		if (points > 9)
		{
			expl += inc;
			inc *= 2;
			points -= 10;
		}
	}

	expl += points * inc / 10;

	expl = expl * pc_race_table[ch->race].class_mult[ch->charClass] / 100;

	return UMAX(expl, 2000);
}

/* this procedure handles the input parsing for the skill generator */
bool
parse_gen_groups(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	int gn, sn, i;

	if (argument[0] == '\0')
		return FALSE;

	argument = one_argument(argument, arg);

	if (!str_prefix(arg, "help"))
	{
		if (argument[0] == '\0')
		{
			do_help(ch, "group help");
			return TRUE;
		}

		do_help(ch, argument);
		return TRUE;
	}

	if (!str_prefix(arg, "add"))
	{
		if (argument[0] == '\0')
		{
			Cprintf(ch, "You must provide a skill name.\n\r");
			return TRUE;
		}

		gn = group_lookup(argument);
		if (gn != -1)
		{
			if (ch->gen_data->group_chosen[gn]
				|| ch->pcdata->group_known[gn])
			{
				Cprintf(ch, "You already know that group!\n\r");
				return TRUE;
			}

			if (group_table[gn].rating[ch->charClass] < 1)
			{
				Cprintf(ch, "That group is not available.\n\r");
				return TRUE;
			}

			Cprintf(ch, "%s group added\n\r", group_table[gn].name);
			ch->gen_data->group_chosen[gn] = TRUE;
			ch->gen_data->points_chosen += group_table[gn].rating[ch->charClass];
			gn_add(ch, gn);
			ch->pcdata->points += group_table[gn].rating[ch->charClass];
			return TRUE;
		}

		sn = skill_lookup(argument);
		if (sn != -1)
		{
			if (ch->gen_data->skill_chosen[sn]
				|| ch->pcdata->learned[sn] > 0)
			{
				Cprintf(ch, "You already know that skill!\n\r");
				return TRUE;
			}

			if (skill_table[sn].rating[ch->charClass] < 1
				|| skill_table[sn].spell_fun != spell_null
				|| skill_table[sn].remort > 0)
			{
				Cprintf(ch, "That skill is not available.\n\r");
				return TRUE;
			}
			Cprintf(ch, "%s skill added\n\r", skill_table[sn].name);
			ch->gen_data->skill_chosen[sn] = TRUE;
			ch->gen_data->points_chosen += skill_table[sn].rating[ch->charClass];
			ch->pcdata->learned[sn] = 1;
			ch->pcdata->points += skill_table[sn].rating[ch->charClass];
			return TRUE;
		}

		Cprintf(ch, "No skills or groups by that name...\n\r");
		return TRUE;
	}

	if (!strcmp(arg, "drop"))
	{
		if (argument[0] == '\0')
		{
			Cprintf(ch, "You must provide a skill to drop.\n\r");
			return TRUE;
		}

		gn = group_lookup(argument);
		if (gn != -1 && ch->gen_data->group_chosen[gn])
		{
			Cprintf(ch, "Group dropped.\n\r");
			ch->gen_data->group_chosen[gn] = FALSE;
			ch->gen_data->points_chosen -= group_table[gn].rating[ch->charClass];
			gn_remove(ch, gn);
			for (i = 0; i < MAX_GROUP; i++)
			{
				if (ch->gen_data->group_chosen[gn])
					gn_add(ch, gn);
			}
			ch->pcdata->points -= group_table[gn].rating[ch->charClass];
			return TRUE;
		}

		sn = skill_lookup(argument);
		if (sn != -1 && ch->gen_data->skill_chosen[sn])
		{
			Cprintf(ch, "Skill dropped.\n\r");
			ch->gen_data->skill_chosen[sn] = FALSE;
			ch->gen_data->points_chosen -= skill_table[sn].rating[ch->charClass];
			ch->pcdata->learned[sn] = 0;
			ch->pcdata->points -= skill_table[sn].rating[ch->charClass];
			return TRUE;
		}

		Cprintf(ch, "You haven't bought any such skill or group.\n\r");
		return TRUE;
	}

	if (!str_prefix(arg, "premise"))
	{
		do_help(ch, "premise");
		return TRUE;
	}

	if (!str_prefix(arg, "list"))
	{
		list_group_costs(ch);
		return TRUE;
	}

	if (!str_prefix(arg, "learned"))
	{
		list_group_chosen(ch);
		return TRUE;
	}

	if (!str_prefix(arg, "info"))
	{
		do_groups(ch, argument);
		return TRUE;
	}

	return FALSE;
}






/* shows all groups, or the sub-members of a group */
void
do_groups(CHAR_DATA * ch, char *argument)
{
	int gn, sn, col;

	if (IS_NPC(ch))
		return;

	col = 0;

	if (argument[0] == '\0')
	{							/* show all groups */

		for (gn = 0; gn < MAX_GROUP; gn++)
		{
			if (group_table[gn].name == NULL)
				break;
			if (ch->pcdata->group_known[gn])
			{
				Cprintf(ch, "%-20s ", group_table[gn].name);
				if (++col % 3 == 0)
					Cprintf(ch, "\n\r");
			}
		}
		if (col % 3 != 0)
			Cprintf(ch, "\n\r");
		Cprintf(ch, "Creation points: %d\n\r", ch->pcdata->points);
		return;
	}

	if (!str_cmp(argument, "all"))	/* show all groups */
	{
		for (gn = 0; gn < MAX_GROUP; gn++)
		{
			if (group_table[gn].name == NULL)
				break;
			Cprintf(ch, "%-20s ", group_table[gn].name);
			if (++col % 3 == 0)
				Cprintf(ch, "\n\r");
		}
		if (col % 3 != 0)
			Cprintf(ch, "\n\r");
		return;
	}


	/* show the sub-members of a group */
	gn = group_lookup(argument);
	if (gn == -1)
	{
		Cprintf(ch, "No group of that name exist.\n\r");
		Cprintf(ch, "Type 'groups all' or 'info all' for a full listing.\n\r");
		return;
	}

	for (sn = 0; sn < MAX_IN_GROUP; sn++)
	{
		if (group_table[gn].spells[sn] == NULL)
			break;
		Cprintf(ch, "%-20s ", group_table[gn].spells[sn]);
		if (++col % 3 == 0)
			Cprintf(ch, "\n\r");
	}
	if (col % 3 != 0)
		Cprintf(ch, "\n\r");
}

/* checks for skill improvement */
void
check_improve(CHAR_DATA * ch, int sn, bool success, int multiplier)
{
	int chance;

	if (IS_NPC(ch))
		return;

	if (ch->level < skill_table[sn].skill_level[ch->charClass]
		|| ch->pcdata->learned[sn] == 0
		|| (sn != ch->pcdata->specialty && ch->pcdata->learned[sn] >= 100)
		|| (sn == ch->pcdata->specialty && ch->pcdata->learned[sn] >= 130))
		return;					/* skill is not known */

	if (IS_SET(ch->comm, COMM_AFK))
		return;

	if (ch->desc != NULL && ch->desc->connected != CON_PLAYING)
		return;


	/* check to see if the character has a chance to learn */
	chance = 10 * int_app[get_curr_stat(ch, STAT_INT)].learn;
	if (ch->remort > 0 || ch->reclass > 0)
		chance = (int) (chance * 1.5);
	if (ch->remort > 0 && ch->reclass > 0)
		chance = (int) (chance * 1.5);

	chance = (int) (chance / (multiplier * (skill_table[sn].rating[ch->charClass] == 0 ? 8 : skill_table[sn].rating[ch->charClass]) * 4));
	chance += 40 + (ch->level / 5);

	if (number_range(1, 1000) > chance)
		return;

	/* now that the character has a CHANCE to learn, see if they really have */

	if (success)
	{
		chance = URANGE(5, 100 - ch->pcdata->learned[sn], 95);

		if (number_percent() < chance)
		{
			/* same sclan */
			if (ch->fighting != NULL) {
				if (!clan_table[ch->clan].independent && ch->fighting->clan == ch->clan && !IS_NPC(ch->fighting))
				{
					Cprintf(ch, "You can't train yourself against clan mates.\n\r");
					return;
				}
			}

			ch->pcdata->learned[sn]++;
			if((ch->pcdata->learned[sn] == 100 && ch->pcdata->specialty != sn)
			|| (ch->pcdata->learned[sn] == 130 && ch->pcdata->specialty == sn)) {
				Cprintf(ch, "{YYou have mastered the skill of %s! (%d%%){x\n\r", skill_table[sn].name, ch->pcdata->learned[sn]);
        			gain_exp(ch, number_range(1,3) * skill_table[sn].rating[ch->charClass] + 1);
			}
			else {
				Cprintf(ch, "{BYou have become better at %s! (%d%%){x\n\r", skill_table[sn].name, ch->pcdata->learned[sn]);
        			gain_exp(ch, number_range(1,3) * skill_table[sn].rating[ch->charClass] + 1);
			}
		}
	}

	else
	{
		chance = URANGE(5, ch->pcdata->learned[sn] / 2, 30);
		if (number_percent() < chance)
		{
			/* same clan */
			if (ch->fighting != NULL) {
				if (!clan_table[ch->clan].independent && ch->fighting->clan == ch->clan && !IS_NPC(ch->fighting))
				{
					Cprintf(ch, "Your simulated practice hones your body, but your skill remains unchanged.\n\r");
					return;
				}
			}

			ch->pcdata->learned[sn] += number_range(1, 3);
                        if (sn == ch->pcdata->specialty)
                        	ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn], 130);
                        else
                                ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn], 100);

			if((ch->pcdata->learned[sn] == 100 && ch->pcdata->specialty != sn)
			|| (ch->pcdata->learned[sn] == 130 && ch->pcdata->specialty == sn)) {
				Cprintf(ch, "{YYou have mastered the skill of %s! (%d%%){x\n\r", skill_table[sn].name, ch->pcdata->learned[sn]);
                        	gain_exp(ch, 2 * skill_table[sn].rating[ch->charClass]);
			}
			else {
				Cprintf(ch, "{BYou learn from your mistakes, and your %s skill improves. (%d%%){x\n\r", skill_table[sn].name, ch->pcdata->learned[sn]);
                        	gain_exp(ch, 2 * skill_table[sn].rating[ch->charClass]);
			}
		}
	}
}

/* returns a group index number given the name */
int
group_lookup(const char *name)
{
	int gn;

	for (gn = 0; gn < MAX_GROUP; gn++)
	{
		if (group_table[gn].name == NULL)
			break;
		if (LOWER(name[0]) == LOWER(group_table[gn].name[0])
			&& !str_prefix(name, group_table[gn].name))
			return gn;
	}

	return -1;
}

/* recursively adds a group given its number -- uses group_add */
void
gn_add(CHAR_DATA * ch, int gn)
{
	int i;

	ch->pcdata->group_known[gn] = TRUE;
	for (i = 0; i < MAX_IN_GROUP; i++)
	{
		if (group_table[gn].spells[i] == NULL)
			break;
		group_add(ch, group_table[gn].spells[i], FALSE);
	}
}

/* recusively removes a group given its number -- uses group_remove */
void
gn_remove(CHAR_DATA * ch, int gn)
{
	int i;

	ch->pcdata->group_known[gn] = FALSE;

	for (i = 0; i < MAX_IN_GROUP; i++)
	{
		if (group_table[gn].spells[i] == NULL)
			break;
		group_remove(ch, group_table[gn].spells[i]);
	}
}

/* use for processing a skill or group for addition  */
void
group_add(CHAR_DATA * ch, const char *name, bool deduct)
{
	int sn, gn;

	if (IS_NPC(ch))				/* NPCs do not have skills */
		return;

	sn = skill_lookup(name);

	if (sn != -1)
	{
		if (ch->pcdata->learned[sn] == 0)	/* i.e. not known */
		{
			ch->pcdata->learned[sn] = 1;
			if (deduct)
				ch->pcdata->points += skill_table[sn].rating[ch->charClass];
		}
		return;
	}

	/* now check groups */

	gn = group_lookup(name);

	if (gn != -1)
	{
		if (ch->pcdata->group_known[gn] == FALSE)
		{
			ch->pcdata->group_known[gn] = TRUE;
			if (deduct)
				ch->pcdata->points += group_table[gn].rating[ch->charClass];
		}
		gn_add(ch, gn);			/* make sure all skills in the group are known */
	}
}

/* used for processing a skill or group for deletion -- no points back! */

void
group_remove(CHAR_DATA * ch, const char *name)
{
	int sn, gn;

	sn = skill_lookup(name);

	if (sn != -1)
	{
		ch->pcdata->learned[sn] = 0;
		return;
	}

	/* now check groups */

	gn = group_lookup(name);

	if (gn != -1 && ch->pcdata->group_known[gn] == TRUE)
	{
		ch->pcdata->group_known[gn] = FALSE;
		gn_remove(ch, gn);		/* be sure to call gn_add on all remaining groups */
	}
}
