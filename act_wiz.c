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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "merc.h"
#include "recycle.h"
#include "clan.h"
#include "flags.h"
#include "vote.h"
#include "utils.h"


#define MAXLINE 1024

extern bool DNSup;
int ctf_score_blue = 0;
int ctf_score_red = 0;
int double_xp_ticks = 0;
int double_qp_ticks = 0;
CLAN_LEADER_DATA clan_leadership[MAX_CLAN];
extern void end_null(void* vo, int target);

void removeAllEffects(CHAR_DATA *ch);
char *connect_lookup(int x);
void do_restore(CHAR_DATA * ch, char *argument);
void do_save(CHAR_DATA * ch, char *argument);
void create_ident(DESCRIPTOR_DATA * desc, long ip, int port);
void show_char_stats_to_char(CHAR_DATA * victim, CHAR_DATA * ch);
extern int remort_pts_level(CHAR_DATA *);
extern void advance_weapon(CHAR_DATA *, OBJ_DATA *, int);

/* command procedures needed */
DECLARE_DO_FUN(do_rstat);
DECLARE_DO_FUN(do_mstat);
DECLARE_DO_FUN(do_ostat);
DECLARE_DO_FUN(do_rset);
DECLARE_DO_FUN(do_mset);
DECLARE_DO_FUN(do_oset);
DECLARE_DO_FUN(do_sset);
DECLARE_DO_FUN(do_mfind);
DECLARE_DO_FUN(do_ofind);
DECLARE_DO_FUN(do_slookup);
DECLARE_DO_FUN(do_mload);
DECLARE_DO_FUN(do_oload);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_stand);
DECLARE_DO_FUN(do_cset);
DECLARE_DO_FUN(do_quit);
DECLARE_DO_FUN(do_cast);
DECLARE_DO_FUN(do_advance);
DECLARE_DO_FUN(do_set);
DECLARE_DO_FUN(do_restore);
DECLARE_DO_FUN(do_eqoutput);

/*
 * Local functions.
 */
ROOM_INDEX_DATA *find_location(CHAR_DATA * ch, char *arg);
CHAR_DATA *get_char_area(CHAR_DATA * ch, char *arg);
char *fread_line(FILE * fp);
char fread_letter(FILE *);
void save_clan_report();

/* local functions for voting system */
void vote_create(CHAR_DATA *ch);
void vote_info(CHAR_DATA *ch);
void vote_set(CHAR_DATA *ch, char *argument);
void vote_voting(CHAR_DATA *ch, int choice);
void vote_open(CHAR_DATA *ch);
void vote_close(CHAR_DATA *ch);
void vote_delete(CHAR_DATA *ch);
void vote_results(CHAR_DATA *ch);
void save_vote();
void load_vote();

/* local functions for crafted items system */
void workshop_list(CHAR_DATA *, char*);
void workshop_new(CHAR_DATA*, char*);
void workshop_delete(CHAR_DATA*, char*);
void save_citems();
void load_citems();

/* local functions for generating eq lists */
bool item_is_valid(OBJ_INDEX_DATA *obj, int num);
void write_object(FILE *fp, OBJ_INDEX_DATA *obj);


/* Function: colorstrlen
   Modified by Tsongas 02/05/01
   Updated to allow brackets and better functionality.
*/
int
colorstrlen(char *argument)
{
	int length = 0;
	int index = 0;

	if (argument == NULL || argument[0] == '\0')
		return 0;

	while (index < strlen (argument))
	{
		if (argument[index] != '{')
			length++ ;
		else if (argument[index] == '{' && argument[index + 1] == '{')
		{
			index++ ;
			length++ ;
		}
		else if (argument[index] == '{' && argument[index + 1] != '{')
			index++ ;
	
		index++ ;
	}
	
	return length ;
}

void
do_dispvnum(CHAR_DATA * ch, char *argument)
{
	if (!IS_IMMORTAL(ch))
		return;

	if (IS_SET(ch->wiznet, WIZ_DISP))
	{
		Cprintf(ch, "VNUM display turned off.\n\r");
		REMOVE_BIT(ch->wiznet, WIZ_DISP);
		return;
	}
	else
	{
		Cprintf(ch, "VNUM display turned on.\n\r");
		SET_BIT(ch->wiznet, WIZ_DISP);
		return;
	}
}


void
do_check(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	BUFFER *buffer;
	CHAR_DATA *victim;
	int count = 1;

	one_argument(argument, arg);

	if (((strcmp(arg, "stats") < 0) || (strcmp(arg, "eq") < 0))
		&& (arg[0] != '\0'))
	{
		if (ch->level < 58)
		{
			Cprintf(ch, "Not for you, bub.\n\r");
			return;
		}
	}

	if (arg[0] == '\0' || !str_prefix(arg, "stats"))
	{
		buffer = new_buf();
		for (victim = char_list; victim != NULL; victim = victim->next)
		{
			if (IS_NPC(victim) || !can_see(ch, victim))
				continue;

			if (victim->desc == NULL)
			{
				sprintf(buf, "%3d) %s is linkdead. {RTimer: %d ticks.{x\n\r", count, victim->name, victim->timer);
				add_buf(buffer, buf);
				count++;
				continue;
			}

			if (victim->desc->connected >= CON_GET_NEW_RACE && victim->desc->connected <= CON_PICK_WEAPON)
			{
				sprintf(buf, "%3d) %s is being created.\n\r", count, victim->name);
				add_buf(buffer, buf);
				count++;
				continue;
			}

			if ((victim->desc->connected == CON_GET_OLD_PASSWORD || victim->desc->connected >= CON_READ_IMOTD) && get_trust(victim) <= get_trust(ch))
			{
				sprintf(buf, "%3d) %s is connecting.\n\r", count, victim->name);
				add_buf(buffer, buf);
				count++;
				continue;
			}

			if (victim->desc->connected == CON_PLAYING)
			{
				if (get_trust(victim) > get_trust(ch))
					sprintf(buf, "%3d) %s.\n\r", count, victim->name);
				else
				{
					sprintf(buf, "%3d) %s, Level %d connected for %d hour%s (%d/%d total hour%s).\n\r",
							count,
							victim->name,
							victim->level,
							((int) (current_time - victim->logon)) / 3600,
							(((int) (current_time - victim->logon)) / 3600 == 1 ? "" : "s"),
							get_perm_hours(victim),
							get_hours(victim),
							get_hours(victim) == 1 ? "" : "s");
					add_buf(buffer, buf);

					if (arg[0] != '\0' && !str_prefix(arg, "stats"))
					{
						sprintf(buf, "     {G%d{x HP {G%d{x Mana ({C%d %d %d %d %d{x) {Y%ld{x gold {Y%ld{x silver {R%d{x Tr {R%d{x Pr {R%d{x QP.\n\r",
								MAX_HP(victim),
								MAX_MANA(victim),
								victim->perm_stat[STAT_STR],
								victim->perm_stat[STAT_INT],
								victim->perm_stat[STAT_WIS],
								victim->perm_stat[STAT_DEX],
								victim->perm_stat[STAT_CON],
								victim->gold,
								victim->silver,
								victim->train,
								victim->practice,
								victim->questpoints);
						add_buf(buffer, buf);
					}
					count++;
				}
				continue;
			}

			sprintf(buf, "%3d) bug (oops)...please report to Delstar: %s %d\n\r", count, victim->name, victim->desc->connected);
			add_buf(buffer, buf);
			count++;
		}

		page_to_char(buf_string(buffer), ch);
		free_buf(buffer);
		return;
	}

	if (!str_prefix(arg, "eq"))
	{
		buffer = new_buf();
		for (victim = char_list; victim != NULL; victim = victim->next)
		{
			if (IS_NPC(victim) || !(victim->desc && victim->desc->connected == CON_PLAYING) || !can_see(ch, victim) || get_trust(victim) > get_trust(ch))
				continue;

			sprintf(buf, "%3d) %s, %d items (weight %d) Hit: {G%d{x Dam: {G%d{x Save: {G%d{x AC: {C%d %d %d %d{x.\n\r",
					count,
					victim->name,
					victim->carry_number,
					victim->carry_weight,
					victim->hitroll,
					victim->damroll,
					victim->saving_throw,
					victim->armor[AC_PIERCE],
					victim->armor[AC_BASH],
					victim->armor[AC_SLASH],
					victim->armor[AC_EXOTIC]);
			add_buf(buffer, buf);
			count++;
		}

		page_to_char(buf_string(buffer), ch);
		free_buf(buffer);
		return;
	}

	Cprintf(ch, "Syntax: 'check'       display info about players\n\r");
	Cprintf(ch, "        'check stats' display info and resume stats\n\r");
	Cprintf(ch, "        'check eq'    resume eq of all players\n\r");
	Cprintf(ch, "Use the stat command in case of doubt about someone...\n\r");
	return;
}


void
do_wiznet(CHAR_DATA * ch, char *argument)
{
	int flag;
	char buf[MAX_STRING_LENGTH];

	if (argument[0] == '\0')
	{
		if (IS_SET(ch->wiznet, WIZ_ON))
		{
			Cprintf(ch, "Signing off of Wiznet.\n\r");
			REMOVE_BIT(ch->wiznet, WIZ_ON);
		}
		else
		{
			Cprintf(ch, "Welcome to Wiznet!\n\r");
			SET_BIT(ch->wiznet, WIZ_ON);
		}
		return;
	}

	if (!str_prefix(argument, "on"))
	{
		Cprintf(ch, "Welcome to Wiznet!\n\r");
		SET_BIT(ch->wiznet, WIZ_ON);
		return;
	}

	if (!str_prefix(argument, "off"))
	{
		Cprintf(ch, "Signing off of Wiznet.\n\r");
		REMOVE_BIT(ch->wiznet, WIZ_ON);
		return;
	}

	/* show wiznet status */
	if (!str_prefix(argument, "status") || !str_prefix(argument, "show"))
	{

		Cprintf(ch, "Wiznet Status\n\r");
		Cprintf(ch, "-------------------\n\r");

		buf[0] = '\0';

		for (flag = 0; wiznet_table[flag].name != NULL; flag++)
		{
			if (get_trust(ch) < wiznet_table[flag].level)
				continue;

			buf[0] = '\0';
			sprintf(buf, "%12s", wiznet_table[flag].name);

			if (IS_SET(ch->wiznet, wiznet_table[flag].flag))
				strcat(buf, ": ON");
			else
				strcat(buf, ": OFF");

			strcat(buf, "\n\r");
			Cprintf(ch, "%s", buf);
		}

		return;
	}

	flag = wiznet_lookup(argument);

	if (flag == -1 || get_trust(ch) < wiznet_table[flag].level)
	{
		Cprintf(ch, "No such option.\n\r");
		return;
	}

	if (IS_SET(ch->wiznet, wiznet_table[flag].flag))
	{
		Cprintf(ch, "You will no longer see %s on wiznet.\n\r", wiznet_table[flag].name);
		REMOVE_BIT(ch->wiznet, wiznet_table[flag].flag);
		return;
	}
	else
	{
		Cprintf(ch, "You will now see %s on wiznet.\n\r", wiznet_table[flag].name);
		SET_BIT(ch->wiznet, wiznet_table[flag].flag);
		return;
	}
}


void
wiznet(char *string, CHAR_DATA * ch, OBJ_DATA * obj, long flag, long flag_skip, int min_level) {
    DESCRIPTOR_DATA *d;
    char *time;

    time = ctime(&current_time);
    *(time + strlen(time) - 1) = '\0';

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->connected == CON_PLAYING
                && IS_IMMORTAL(d->character)
                && IS_SET(d->character->wiznet, WIZ_ON)
                && (!flag || IS_SET(d->character->wiznet, flag))
                && (!flag_skip || !IS_SET(d->character->wiznet, flag_skip))
                && get_trust(d->character) >= min_level
                && d->character != ch) {
            if (IS_SET(d->character->wiznet, WIZ_PREFIX)) {
                Cprintf(d->character, "{r--> ");
            }

            if (IS_SET(d->character->wiznet, WIZ_TIMES)) {
                Cprintf(d->character, "%s :: ", time);
            }

            act(string, d->character, obj, ch, TO_CHAR, POS_DEAD);
            Cprintf(d->character, "{x");
        }
    }

    return;
}


void
do_dice(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int d1, d2;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: dice <dicenum> <diceface>\n\r");
		return;
	}

	d1 = atoi(arg1);
	d2 = atoi(arg2);

	if (d1 < 1 || d2 < 1)
	{
		Cprintf(ch, "Use numbers greater than 0.\n\r");
		return;
	}

	Cprintf(ch, "Weapon: %dd%d -> Average: %d.\n\r", d1, d2, (1 + d2) * d1 / 2);
	return;
}


void
do_hal(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "You MUST type the full command.\n\r");
	return;
}


void
do_half(CHAR_DATA * ch, char *argument)
{
	int sn;
	CHAR_DATA *victim;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Syntax: half <charname>\n\r");
		return;
	}

	if (!str_prefix(argument, "all"))
	{
		Cprintf(ch, "You can't half everyone.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, argument, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't playing.\n\r");
		return;
	}


	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;

		if (victim->level < skill_table[sn].skill_level[victim->charClass] || victim->pcdata->learned[sn] < 1 /* skill is not known */ )
			continue;

		victim->pcdata->learned[sn] = (victim->pcdata->learned[sn]) / 2;

		if (victim->pcdata->learned[sn] == 0)
			victim->pcdata->learned[sn] = 1;
	}

	Cprintf(victim, "All your skills and spell percentages were halved by %s.\n\r", ch->name);
	Cprintf(ch, "All of %s's skills and spell percentages were halved.\n\r", victim->name);
	save_char_obj(victim, FALSE);
	return;
}


void
do_maxstats(CHAR_DATA * ch, char *argument)
{
	int sn;
	CHAR_DATA *victim;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Syntax: maxstats <charname>\n\r");
		return;
	}

	if (!str_prefix(argument, "all"))
	{
		Cprintf(ch, "You can't give everyone maximum stats.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, argument, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't playing.\n\r");
		return;
	}

	if (IS_NPC(victim)) {
                Cprintf(ch, "Don't maxstat mobs.\n\r");
                return;
        }                    

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;

		if (victim->level < skill_table[sn].skill_level[victim->charClass] || victim->pcdata->learned[sn] < 1 /* skill is not known */ )
			continue;

		victim->pcdata->learned[sn] = 100;
	}

	Cprintf(victim, "All your known skills and spell percentages were maxed out by %s.\n\r", ch->name);
	Cprintf(ch, "All of %s's known skills and spell percentages were maxed out.\n\r", victim->name);
	save_char_obj(victim, FALSE);
	return;
}


void
do_ident(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Syntax: ident <charname>\n\r");
		return;
	}

	if (!str_prefix(argument, "all"))
	{
		Cprintf(ch, "You can't ident everyone at once.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, argument, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't playing.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "That is a mob, with no port to ident\n\r");
		return;
	}

	if (victim->desc == NULL || victim->pcdata == NULL)
	{
		Cprintf(ch, "They dont have a descriptor or pcdata, no ident possible.\n\r");
		return;
	}

	create_ident(victim->desc, victim->desc->addr, victim->desc->port);

	Cprintf(ch, "Running ident on %s\n\r", victim->name);

	return;
}


void
do_guild(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	OBJ_DATA *obj_in;
	int clan;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: guild <char> <clan name>\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't playing.\n\r");
		return;
	}

	if (in_own_hall(victim))
	{
		Cprintf(ch, "Can't guild %s into %s while he's in his clan hall.\n\r", arg1, arg2);
		return;
	}

	if (!str_prefix(arg2, "none"))
	{
		if (ch != victim)
			Cprintf(ch, "They are now clanless. (Items adjusted)\n\r");
		Cprintf(victim, "You are now a member of no clan!\n\r");
		victim->clan = 0;
		victim->delegate = 0;
		REMOVE_BIT(victim->wiznet, CAN_CLAN);
 		// Klunky fix for new clan status flags
                for(obj = victim->carrying; obj != NULL; obj = obj->next_content) {
                        if(obj->contains) {
                               for(obj_in = obj->contains; obj_in != NULL; obj_in = obj_in->next_content) {
                      	         if(obj_in->clan_status == CS_CLANNER)
                                         obj_in->clan_status = CS_NONCLANNER;
                                 }
                        }

                        if(obj->clan_status == CS_CLANNER)
                                 obj->clan_status = CS_NONCLANNER;
                }
		return;
	}

	if ((clan = clan_lookup(arg2)) == 0)
	{
		Cprintf(ch, "No such clan exists.\n\r");
		return;
	}

	if (victim->clan == clan_lookup("outcast"))
	{
		victim->outcast_timer = 0;
	}

	if (clan == clan_lookup("outcast"))
	{
		victim->outcast_timer = 50;
	}

	if (clan_table[clan].independent)
	{
		if (ch != victim)
			Cprintf(ch, "They are now a %s.\n\r", clan_table[clan].name);
		Cprintf(victim, "You are now a %s.\n\r", clan_table[clan].name);
	}
	else
	{
		if (ch != victim)
			Cprintf(ch, "They are now a member of clan %s.\n\r", capitalize(clan_table[clan].name));
		Cprintf(victim, "You are now a member of clan %s.\n\r", capitalize(clan_table[clan].name));
	}

	victim->clan = clan;
	victim->delegate = 0;
	if(victim->clan_rank)
		free_string(victim->clan_rank);
	victim->clan_rank = NULL;
	SET_BIT(victim->wiznet, CAN_CLAN);
}


void
do_delegate(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int count;
	int level;

	if (!str_cmp(argument, "show"))
	{
		count = 0;
		Cprintf(ch, "Trusted people in your clan:\n\r");
		for (victim = char_list; victim != NULL; victim = victim->next)
		{
			if (victim != NULL)
			{
				if (ch->clan == victim->clan && victim->delegate > 0)
				{
					Cprintf(ch, "%s delegated to loner people level %d or below.\n\r", victim->name, victim->delegate);
					++count;
				}
			}
		}

		if (count == 0)
			Cprintf(ch, "None found online.\n\r");

		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: delegate <char> <level>\n\r");
		Cprintf(ch, "        delegate show\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't playing.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "You really want a mob to do your job? Think again.\n\r");
		return;
	}

	if (!is_number(arg2))
	{
		Cprintf(ch, "Syntax: delegate <char> <level>.\n\r");
		Cprintf(ch, "        delegate show\n\r");
		return;
	}

	level = atoi(arg2);

	if (get_trust(ch) < 53)
	{
		Cprintf(ch, "And which clan do YOU lead that you can delegate for?n\r");
		return;
	}

	if (ch->clan != victim->clan)
	{
		Cprintf(ch, "And you're asking for a member of a different clan to loner from yours because... ?\n\r");
		return;
	}

	if (get_trust(victim) != 52)
	{
		Cprintf(ch, "You can only delegate to your recruiters.\n\r");
		return;
	}

	if (level > 52)
	{
		Cprintf(ch, "You can't delegate above level 52.\n\r");
		return;
	}

	if (level < 0)
	{
		Cprintf(ch, "Sadistic S.O.B., aren't you?  You can only delegate level 0 and up.\n\r");
		return;
	}

	victim->delegate = level;
	Cprintf(ch, "You delegate %s to loner people of level %d or lower.\n\r", victim->name, victim->delegate);
	Cprintf(victim, "You are now delegated to loner people of level %d or lower.\n\r", victim->delegate);
}


/* equips a character */
void
do_outfit(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj;
	int i, sn, vnum;

	if (ch->level > 5 || IS_NPC(ch))
	{
		Cprintf(ch, "Find it yourself!\n\r");
		return;
	}

	if ((obj = get_eq_char(ch, WEAR_LIGHT)) == NULL)
	{
		obj = create_object(get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0);
		obj->cost = 0;
		obj_to_char(obj, ch);
		equip_char(ch, obj, WEAR_LIGHT);
	}

	if ((obj = get_eq_char(ch, WEAR_BODY)) == NULL)
	{
		obj = create_object(get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0);
		obj->cost = 0;
		obj_to_char(obj, ch);
		equip_char(ch, obj, WEAR_BODY);
	}

	/* do the weapon thing */
	if ((obj = get_eq_char(ch, WEAR_WIELD)) == NULL)
	{
		sn = 0;
		vnum = OBJ_VNUM_SCHOOL_SWORD;	/* just in case! */

		for (i = 0; weapon_table[i].name != NULL; i++)
		{
			if (ch->pcdata->learned[sn] < ch->pcdata->learned[*weapon_table[i].gsn])
			{
				sn = *weapon_table[i].gsn;
				vnum = weapon_table[i].vnum;
			}
		}

		obj = create_object(get_obj_index(vnum), 0);
		obj_to_char(obj, ch);
		equip_char(ch, obj, WEAR_WIELD);
	}

	if (((obj = get_eq_char(ch, WEAR_WIELD)) == NULL || !IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)) && (obj = get_eq_char(ch, WEAR_SHIELD)) == NULL)
	{
		obj = create_object(get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0);
		obj->cost = 0;
		obj_to_char(obj, ch);
		equip_char(ch, obj, WEAR_SHIELD);
	}

	Cprintf(ch, "You have been equipped by %s.\n\r", godName(ch));
}


/* RT nochannels command, for those spammers */
void
do_nochannels(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Nochannel whom?");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (get_trust(victim) >= get_trust(ch))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (IS_SET(victim->comm, COMM_NOCHANNELS))
	{
		REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
		Cprintf(victim, "The gods have restored your channel privileges.\n\r");
		Cprintf(ch, "NOCHANNELS removed.\n\r");
		sprintf(buf, "$N restores channels to %s", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}
	else
	{
		SET_BIT(victim->comm, COMM_NOCHANNELS);
		Cprintf(victim, "The gods have revoked your channel privileges.\n\r");
		Cprintf(ch, "NOCHANNELS set.\n\r");
		sprintf(buf, "$N revokes %s's channels.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}

	return;
}

/* RT nochannels command, for those spammers */
void
do_nonote(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Nonote whom?");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (get_trust(victim) >= get_trust(ch))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (IS_SET(victim->comm, COMM_NONOTE))
	{
		REMOVE_BIT(victim->comm, COMM_NONOTE);
		Cprintf(victim, "The gods have restored your note-writing abilities.\n\r");
		Cprintf(ch, "NONOTE removed.\n\r");
		sprintf(buf, "$N restores note writing to %s", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}
	else
	{
		SET_BIT(victim->comm, COMM_NONOTE);
		Cprintf(victim, "The gods have revoked your note-writing abilities.\n\r");
		Cprintf(ch, "NONOTE set.\n\r");
		sprintf(buf, "$N revokes %s's note writing.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}

	return;
}


void
do_smote(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *vch;
	char *letter, *name;
	char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
	unsigned int matches = 0;

	if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE))
	{
		Cprintf(ch, "You can't show your emotions.\n\r");
		return;
	}

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Emote what?\n\r");
		return;
	}

	if (strstr(argument, ch->name) == NULL)
	{
		Cprintf(ch, "You must include your name in an smote.\n\r");
		return;
	}

	Cprintf(ch, "%s\n\r", argument);

	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (vch->desc == NULL || vch == ch)
			continue;

		if ((letter = strstr(argument, vch->name)) == NULL)
		{
			Cprintf(vch, "%s\n\r", argument);
			continue;
		}

		strcpy(temp, argument);
		temp[strlen(argument) - strlen(letter)] = '\0';
		last[0] = '\0';
		name = vch->name;

		for (; *letter != '\0'; letter++)
		{
			if (*letter == '\'' && matches == strlen(vch->name))
			{
				strcat(temp, "r");
				continue;
			}

			if (*letter == 's' && matches == strlen(vch->name))
			{
				matches = 0;
				continue;
			}

			if (matches == strlen(vch->name))
			{
				matches = 0;
			}

			if (*letter == *name)
			{
				matches++;
				name++;
				if (matches == strlen(vch->name))
				{
					strcat(temp, "you");
					last[0] = '\0';
					name = vch->name;
					continue;
				}
				strncat(last, letter, 1);
				continue;
			}

			matches = 0;
			strcat(temp, last);
			strncat(temp, letter, 1);
			last[0] = '\0';
			name = vch->name;
		}

		Cprintf(vch, "%s\n\r", temp);
	}

	return;
}


void
do_bamfin(CHAR_DATA * ch, char *argument)
{
	if (!IS_NPC(ch))
	{
		smash_tilde(argument);

		if (argument[0] == '\0')
		{
			Cprintf(ch, "Your poofin is now %s\n\r", ch->pcdata->bamfin);
			return;
		}

		if (strstr(argument, ch->name) == NULL)
		{
			Cprintf(ch, "You must include your name.\n\r");
			return;
		}

		free_string(ch->pcdata->bamfin);
		ch->pcdata->bamfin = str_dup(argument);

		Cprintf(ch, "Your poofin is now %s\n\r", ch->pcdata->bamfin);
	}
	return;
}


void
do_bamfout(CHAR_DATA * ch, char *argument)
{
	if (!IS_NPC(ch))
	{
		smash_tilde(argument);

		if (argument[0] == '\0')
		{
			Cprintf(ch, "Your poofout is %s\n\r", ch->pcdata->bamfout);
			return;
		}

		if (strstr(argument, ch->name) == NULL)
		{
			Cprintf(ch, "You must include your name.\n\r");
			return;
		}

		free_string(ch->pcdata->bamfout);
		ch->pcdata->bamfout = str_dup(argument);

		Cprintf(ch, "Your poofout is now %s\n\r", ch->pcdata->bamfout);
	}
	return;
}


void
do_deny(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		Cprintf(ch, "Deny whom?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPC's.\n\r");
		return;
	}

	if (get_trust(victim) >= get_trust(ch))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	SET_BIT(victim->act, PLR_DENY);
	Cprintf(victim, "You are denied access!\n\r");
	sprintf(buf, "$N denies access to %s", victim->name);
	wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	Cprintf(ch, "OK.\n\r");
	save_char_obj(victim, TRUE);
	stop_fighting(victim, TRUE);
	quit_character(victim);

	return;
}


void
do_disconnect(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;

	one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		Cprintf(ch, "Disconnect whom?\n\r");
		return;
	}

	if (is_number(arg))
	{
		int desc;

		desc = atoi(arg);
		for (d = descriptor_list; d != NULL; d = d->next)
		{
			if (d->descriptor == desc)
			{
				close_socket(d);
				Cprintf(ch, "Ok.\n\r");
				return;
			}
		}
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim->desc == NULL)
	{
		act("$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	for (d = descriptor_list; d != NULL; d = d->next)
	{
		if (d == victim->desc)
		{
			close_socket(d);
			Cprintf(ch, "Ok.\n\r");
			return;
		}
	}

	bug("Do_disconnect: desc not found.", 0);
	Cprintf(ch, "Descriptor not found!\n\r");
	return;
}


void
do_pardon(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: pardon <character> <killer|thief>.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPC's.\n\r");
		return;
	}

	if (!str_cmp(arg2, "killer"))
	{
		if (IS_SET(victim->act, PLR_KILLER))
		{
			REMOVE_BIT(victim->act, PLR_KILLER);
			if (ch != victim)
				Cprintf(ch, "Killer flag removed.\n\r");
			Cprintf(victim, "You are no longer a KILLER.\n\r");
		}
		return;
	}

	if (!str_cmp(arg2, "thief"))
	{
		if (IS_SET(victim->act, PLR_THIEF))
		{
			REMOVE_BIT(victim->act, PLR_THIEF);
			if (ch != victim)
				Cprintf(ch, "Thief flag removed.\n\r");
			Cprintf(victim, "You are no longer a THIEF.\n\r");
		}
		return;
	}

	Cprintf(ch, "Syntax: pardon <character> <killer|thief>.\n\r");

	return;
}


void
do_echo(CHAR_DATA * ch, char *argument)
{
	DESCRIPTOR_DATA *d;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Global echo what?\n\r");
		return;
	}

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->connected == CON_PLAYING)
		{
			if (get_trust(d->character) >= get_trust(ch))
				Cprintf(d->character, "global> ");
			Cprintf(d->character, "%s\n\r", argument);
		}
	}

	return;
}


void
do_recho(CHAR_DATA * ch, char *argument)
{
	DESCRIPTOR_DATA *d;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Local echo what?\n\r");
		return;
	}

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->connected == CON_PLAYING && d->character->in_room == ch->in_room)
		{
			if (get_trust(d->character) >= get_trust(ch))
				Cprintf(d->character, "local> ");
			Cprintf(d->character, "%s\n\r", argument);
		}
	}

	return;
}


void
do_zecho(CHAR_DATA * ch, char *argument)
{
	DESCRIPTOR_DATA *d;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Zone echo what?\n\r");
		return;
	}

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->connected == CON_PLAYING && d->character->in_room != NULL && ch->in_room != NULL && d->character->in_room->area == ch->in_room->area)
		{
			if (get_trust(d->character) >= get_trust(ch))
				Cprintf(d->character, "zone> ");
			Cprintf(d->character, "%s\n\r", argument);
		}
	}
}


void
do_pecho(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg);

	if (argument[0] == '\0' || arg[0] == '\0')
	{
		Cprintf(ch, "Personal echo what?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "Target not found.\n\r");
		return;
	}

	if (get_trust(victim) >= get_trust(ch) && get_trust(ch) != MAX_LEVEL)
		Cprintf(victim, "personal> ");

	Cprintf(victim, "%s\n\r", argument);
	Cprintf(ch, "personal> %s\n\r", argument);
}


ROOM_INDEX_DATA *
find_location(CHAR_DATA * ch, char *arg)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;

	if (is_number(arg))
		return get_room_index(atoi(arg));

	if ((victim = get_char_world(ch, arg, TRUE)) != NULL)
		return victim->in_room;

	if ((obj = get_obj_world(ch, arg)) != NULL)
		return obj->in_room;

	return NULL;
}


void
do_transfer(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Transfer whom (and where)?\n\r");
		return;
	}

	if (!str_cmp(arg1, "all"))
	{
		if(ch->level < 59)
		{
			Cprintf(ch, "You huff, and you puff, but you can't transfer all!\n\r");
			return;
		}

		for (d = descriptor_list; d != NULL; d = d->next)
		{
			if (d->connected == CON_PLAYING && d->character != ch && d->character->in_room != NULL && can_see(ch, d->character))
			{
				char buf[MAX_STRING_LENGTH];

				sprintf(buf, "%s %s", d->character->name, arg2);
				do_transfer(ch, buf);
			}
		}
		return;
	}

	/*
	 * Thanks to Grodyn for the optional location parameter.
	 */
	if (arg2[0] == '\0')
		location = ch->in_room;
	else
	{
		if ((location = find_location(ch, arg2)) == NULL)
		{
			Cprintf(ch, "No such location.\n\r");
			return;
		}

		if (!is_room_owner(ch, location) && room_is_private(location) && get_trust(ch) < MAX_LEVEL)
		{
			Cprintf(ch, "That room is private right now.\n\r");
			return;
		}
	}

	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim->in_room == NULL)
	{
		Cprintf(ch, "They are in limbo.\n\r");
		return;
	}

	if (IS_TRUSTED(victim, 60) && !IS_TRUSTED(ch, 60))
	{
		Cprintf(ch, "You cannot trans a level 60.\n\r");
		return;
	}

	if (victim->level > get_trust(ch) && !IS_NPC(victim))
	{
		Cprintf(ch, "You cannot trans someone above your level.\n\r");
		return;
	}

	if (victim->fighting != NULL)
		stop_fighting(victim, TRUE);

	act("$n disappears in a flash of light.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	char_from_room(victim);
	char_to_room(victim, location);
	act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	if (ch != victim)
		act("$n has transferred you.", ch, NULL, victim, TO_VICT, POS_RESTING);
	do_look(victim, "auto");
	if (IS_SET(victim->toggles, TOGGLES_SOUND))
		Cprintf(victim, "!!SOUND(sounds/wav/trans.wav V=80 P=20 T=admin)");
	Cprintf(ch, "Ok.\n\r");
}


void
do_site(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Site who?  Syntax: site <char name>.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, argument, TRUE)) == NULL)
	{
		Cprintf(ch, "That player is not here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPC's.\n\r");
		return;
	}

	Cprintf(ch, "Character:  %s\n\rCreated at: %s  Secret: %s.\n\r", victim->name, victim->site, victim->short_descr);
	return;
}


void
do_at(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	ROOM_INDEX_DATA *original;
	OBJ_DATA *on;
	CHAR_DATA *wch;
	char atbuf[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg);

	if (arg[0] == '\0' || argument[0] == '\0')
	{
		Cprintf(ch, "At where what?\n\r");
		return;
	}

	if (!str_cmp(arg, "all"))
	{
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 1)
		{
			Cprintf(ch, "Not at your level!\n\r");
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next)
		{
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch))
			{
				char *argP = argument;
				char *bufP = atbuf;

				strcpy(bufP, vch->name);
				bufP += strlen(vch->name);
				*bufP = ' ';
				bufP++;

				while (*argP)
				{
					if ((*argP == '{') && (*(argP + 1) == '}'))
					{
						strcpy(bufP, vch->name);
						bufP += strlen(vch->name);
						argP++;
					}
					else
					{
						*bufP = *argP;
						bufP++;
					}

					argP++;
				}

				*bufP = '\0';
				do_at(ch, atbuf);
			}
		}
	}
	else if (!str_cmp(arg, "players"))
	{
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 2)
		{
			Cprintf(ch, "Not at your level!\n\r");
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next)
		{
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch) && vch->level < LEVEL_HERO)
			{
				sprintf(atbuf, "%s %s", vch->name, argument);
				do_at(ch, atbuf);
			}
		}
	}
	else if (!str_cmp(arg, "gods"))
	{
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 2)
		{
			Cprintf(ch, "Not at your level!\n\r");
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next)
		{
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch) && vch->level >= LEVEL_HERO)
			{
				sprintf(atbuf, "%s %s", vch->name, argument);
				do_at(ch, atbuf);
			}
		}
	}
	else
	{
		if ((location = find_location(ch, arg)) == NULL)
		{
			Cprintf(ch, "No such location.\n\r");
			return;
		}

		if (!is_room_owner(ch, location) && room_is_private(location) && get_trust(ch) < MAX_LEVEL)
		{
			Cprintf(ch, "That room is private right now.\n\r");
			return;
		}

		original = ch->in_room;
		on = ch->on;
		char_from_room(ch);
		char_to_room(ch, location);
		interpret(ch, argument);

		/*
		 * See if 'ch' still exists before continuing!
		 * Handles 'at XXXX quit' case.
		 */
		for (wch = char_list; wch != NULL; wch = wch->next)
		{
			if (wch == ch)
			{
				char_from_room(ch);
				char_to_room(ch, original);
				ch->on = on;
				break;
			}
		}
	}

	return;
}


void
do_goto(CHAR_DATA * ch, char *argument)
{
	ROOM_INDEX_DATA *location;
	CHAR_DATA *rch;
	int count = 0;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Goto where?\n\r");
		return;
	}

	if ((location = find_location(ch, argument)) == NULL)
	{
		Cprintf(ch, "No such location.\n\r");
		return;
	}

	count = 0;
	for (rch = location->people; rch != NULL; rch = rch->next_in_room)
		count++;

	if (!is_room_owner(ch, location) && room_is_private(location) && (count > 1 || get_trust(ch) < MAX_LEVEL))
	{
		Cprintf(ch, "That room is private right now.\n\r");
		return;
	}

	if (!IS_NPC(ch) && ch->level < 55 && ch->trust < 55 && !IS_BUILDER(ch, location->area))
	{
		Cprintf(ch, "Builders can only goto rooms in their area.\n\r");
		return;
	}

	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
	{
		if (get_trust(rch) >= ch->invis_level)
		{
			if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
				act("$t", ch, ch->pcdata->bamfout, rch, TO_VICT, POS_RESTING);
			else
				act("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT, POS_RESTING);
		}
	}

	char_from_room(ch);
	char_to_room(ch, location);

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
	{
		if (get_trust(rch) >= ch->invis_level)
		{
			if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
				act("$t", ch, ch->pcdata->bamfin, rch, TO_VICT, POS_RESTING);
			else
				act("$n appears in a swirling mist.", ch, NULL, rch, TO_VICT, POS_RESTING);
		}
	}

	do_look(ch, "auto");
	return;
}


void
do_violate(CHAR_DATA * ch, char *argument)
{
	ROOM_INDEX_DATA *location;
	CHAR_DATA *rch;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Violate the privacy of which room?\n\r");
		return;
	}

	if ((location = find_location(ch, argument)) == NULL)
	{
		Cprintf(ch, "No such location.\n\r");
		return;
	}

	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
		if (get_trust(rch) >= ch->invis_level)
		{
			if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
				act("$t", ch, ch->pcdata->bamfout, rch, TO_VICT, POS_RESTING);
			else
				act("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT, POS_RESTING);
		}

	char_from_room(ch);
	char_to_room(ch, location);


	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
		if (get_trust(rch) >= ch->invis_level)
		{
			if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
				act("$t", ch, ch->pcdata->bamfin, rch, TO_VICT, POS_RESTING);
			else
				act("$n appears in a swirling mist.", ch, NULL, rch, TO_VICT, POS_RESTING);
		}

	do_look(ch, "auto");
	return;
}


/* RT to replace the 3 stat commands */
void
do_stat(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char *string;
	OBJ_DATA *obj;
	ROOM_INDEX_DATA *location;
	CHAR_DATA *victim;

	string = one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		Cprintf(ch, "Syntax:\n\r");
		Cprintf(ch, "  stat <name>\n\r");
		Cprintf(ch, "  stat obj <name>\n\r");
		Cprintf(ch, "  stat mob <name>\n\r");
		Cprintf(ch, "  stat room <number>\n\r");
		Cprintf(ch, "  stat clans\n\r");
		return;
	}

	if (!str_cmp(arg, "clans"))
	{
		int i;

		for (i = 0; i < MAX_CLAN; i++)
		{
			Cprintf(ch, "%10s: Leader: %10s Recruiters %10s, %10s\n\r",
					clan_table[i].name ? clan_table[i].name : "none",
			 clan_leadership[i].leader ? clan_leadership[i].leader : "none",
					clan_leadership[i].recruiter1 ? clan_leadership[i].recruiter1 : "none",
					clan_leadership[i].recruiter2 ? clan_leadership[i].recruiter2 : "none");
		}
		return;
	}

	if (!str_cmp(arg, "room"))
	{
		do_rstat(ch, string);
		return;
	}

	if (!str_cmp(arg, "obj"))
	{
		do_ostat(ch, string);
		return;
	}

	if (!str_cmp(arg, "char") || !str_cmp(arg, "mob"))
	{
		do_mstat(ch, string);
		return;
	}

	/* do it the old way */

	obj = get_obj_world(ch, argument);
	if (obj != NULL)
	{
		do_ostat(ch, argument);
		return;
	}

	victim = get_char_world(ch, argument, TRUE);
	if (victim != NULL)
	{
		do_mstat(ch, argument);
		return;
	}

	location = find_location(ch, argument);
	if (location != NULL)
	{
		do_rstat(ch, argument);
		return;
	}

	Cprintf(ch, "Nothing by that name found anywhere.\n\r");
}


void
do_rstat(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	AFFECT_DATA *paf;
	OBJ_DATA *obj;
	CHAR_DATA *rch;
	bool Found;
	int door;
	int flag_num;

	one_argument(argument, arg);
	location = (arg[0] == '\0') ? ch->in_room : find_location(ch, arg);
	if (location == NULL)
	{
		Cprintf(ch, "No such location.\n\r");
		return;
	}

	if (!is_room_owner(ch, location) && ch->in_room != location && room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR))
	{
		Cprintf(ch, "That room is private right now.\n\r");
		return;
	}

	Cprintf(ch, "{cName:{x '%s'\n\r", location->name);
	Cprintf(ch, "{cArea:{x '%s'\n\r", location->area->name);
	Cprintf(ch, "{cVnum:{x %d  ", location->vnum);
	Cprintf(ch, "{cSector:{x %s (%d)  ", sector_flags[location->sector_type].name, location->sector_type);
	Cprintf(ch, "{cLight:{x %d  ", location->light);
	Cprintf(ch, "{cHealing:{x %d  ", location->heal_rate);
	Cprintf(ch, "{cMana:{x %d\n\r", location->mana_rate);

	/* let's set up some descriptive room flags */
	strcpy(buf, "");
	for (flag_num = 0; room_flags[flag_num].name; flag_num++)
		if (location->room_flags & room_flags[flag_num].bit)
		{
			strcat(buf, room_flags[flag_num].name);
			strcat(buf, " ");
		}

	Cprintf(ch, "{cRoom flags:{x %s (%d).\n\r", buf, location->room_flags);
	Cprintf(ch, "{cDescription:{x\n\r%s", location->description);

	if (location->sound != NULL)
		Cprintf(ch, "{cSound:{x %s\n\r", location->sound);

	if (location->extra_descr != NULL)
	{
		EXTRA_DESCR_DATA *ed;

		Cprintf(ch, "{cExtra description keywords:{x '");
		for (ed = location->extra_descr; ed; ed = ed->next)
		{
			Cprintf(ch, "%s", ed->keyword);
			if (ed->next != NULL)
				Cprintf(ch, " ");
		}
		Cprintf(ch, "'.\n\r");
	}

	Cprintf(ch, "{cCharacters:{x");
	for (rch = location->people; rch; rch = rch->next_in_room)
		if (can_see(ch, rch))
		{
			Cprintf(ch, " ");
			one_argument(rch->name, buf);
			Cprintf(ch, "%s", buf);
		}

	Cprintf(ch, ".\n\r{cObjects:{x ");
	for (obj = location->contents; obj; obj = obj->next_content)
	{
		Cprintf(ch, " ");
		one_argument(obj->name, buf);
		Cprintf(ch, "%s", buf);
	}
	Cprintf(ch, ".\n\r");

	for (door = 0; door <= 5; door++)
	{
		EXIT_DATA *pexit;

		char *doors[] =
		{"north", "east", "south", "west", "up", "down"};

		if ((pexit = location->exit[door]) != NULL)
		{
			for (flag_num = 0; exit_flags[flag_num].name; flag_num++)
				if (pexit->exit_info && exit_flags[flag_num].bit)
				{
					strcpy(buf, exit_flags[flag_num].name);
					strcpy(buf, " ");
				}

			Cprintf(ch, "{cDoor:{x %s (%d).  {cTo:{x %d.  {cKey:{x %d.  {cExit flags:{x %s (%d).\n\r     {cKeyword:{x '%s'.  {cDescription:{x %s",
					doors[door],
					door,
				 (pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
					pexit->key,
					buf,
					pexit->exit_info,
					pexit->keyword,
					pexit->description[0] != '\0' ? pexit->description : "(none).\n\r");
		}
	}

	Cprintf(ch, "{cRoom affected by:{x\n\r");

	Found = FALSE;
	for (paf = location->affected; paf != NULL; paf = paf->next)
	{
		Found = TRUE;
		Cprintf(ch, "Affects %s by %d, level %d", affect_loc_name(paf->location), paf->modifier, paf->level);
		if (paf->duration > -1)
			sprintf(buf, ", %d hours.\n\r", paf->duration);
		else
			sprintf(buf, ".\n\r");

		Cprintf(ch, "%s", buf);
	}

	if (Found == FALSE)
		Cprintf(ch, "Nothing.\n\r");

	return;
}


void
do_ostat(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	AFFECT_DATA *paf;
	OBJ_DATA *obj;
	ROOM_INDEX_DATA *location;
	int i;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Stat what?\n\r");
		return;
	}

	if ((obj = get_obj_world(ch, argument)) == NULL)
	{
		Cprintf(ch, "Nothing like that in hell, earth, or heaven.\n\r");
		return;
	}

	Cprintf(ch, "{cName(s):{x %s\n\r", obj->name);

	Cprintf(ch, "{cVnum:{x %d  ", obj->pIndexData->vnum);
	Cprintf(ch, "{cFormat:{x %s  ", obj->pIndexData->new_format ? "new" : "old");
	Cprintf(ch, "{cType:{x %s  ", item_name(obj->item_type));
	Cprintf(ch, "{cResets:{x %d  ", obj->pIndexData->reset_num);
	Cprintf(ch, "{cMaterial:{x %s\n\r", obj->material);

	Cprintf(ch, "{cShort description:{x %s\n\r", obj->short_descr);
	Cprintf(ch, "{cLong description:{x %s\n\r", obj->description);

	Cprintf(ch, "{cWear bits:{x %s\n\r", wear_bit_name(obj->wear_flags));
	Cprintf(ch, "{cExtra bits:{x %s\n\r", extra_bit_name(obj->extra_flags));

	Cprintf(ch, "{cNumber:{x %d/%d  ", 1, get_obj_number(obj));
	Cprintf(ch, "{cWeight:{x %d/%d/%d (10th pounds)\n\r", obj->weight, get_obj_weight(obj), get_true_weight(obj));

	Cprintf(ch, "{cLevel:{x %d  ", obj->level);
	Cprintf(ch, "{cCost:{x %d  ", obj->cost);
	Cprintf(ch, "{cCondition:{x %d  ", obj->condition);
	Cprintf(ch, "{cTimer:{x %d\n\r", obj->timer);

	Cprintf(ch, "{cIn room:{x %d  ", obj->in_room == NULL ? 0 : obj->in_room->vnum);
	Cprintf(ch, "{cIn object:{x %s  ", obj->in_obj == NULL ? "(none)" : obj->in_obj->short_descr);
	Cprintf(ch, "{cCarried by:{x %s  ", obj->carried_by == NULL ? "(none)" : can_see(ch, obj->carried_by) ? obj->carried_by->name : "someone");
	Cprintf(ch, "{cWear_loc:{x %d\n\r", obj->wear_loc);

	Cprintf(ch, "{cValues:{x %d %d %d %d %d\n\r", obj->value[0], obj->value[1], obj->value[2], obj->value[3], obj->value[4]);
	Cprintf(ch, "{cSpecials:{x %d %d %d %d %d\n\r", obj->special[0], obj->special[1], obj->special[2], obj->special[3], obj->special[4]);
	Cprintf(ch, "{cExtras:{x %d %d %d %d %d\n\r", obj->extra[0], obj->extra[1], obj->extra[2], obj->extra[3], obj->extra[4]);
	/* now give out vital statistics as per identify */

	switch (obj->item_type)
	{
	case ITEM_SCROLL:
	case ITEM_POTION:
	case ITEM_PILL:
		Cprintf(ch, "{cLevel {x%d{c spells of:{x", obj->value[0]);

		for (i = 1; i < 5; i++)
			if (obj->value[i] >= 0 && obj->value[i] < MAX_SKILL)
				Cprintf(ch, " {c'{x%s{c'{x", skill_table[obj->value[i]].name);

		Cprintf(ch, "{c.{x\n\r");
		break;

	case ITEM_WAND:
	case ITEM_STAFF:
		Cprintf(ch, "{cHas {x%d{c({x%d{c) charges of level {x%d", obj->value[1], obj->value[2], obj->value[0]);

		if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL)
			Cprintf(ch, " {c'{x%s{c'{x", skill_table[obj->value[3]].name);

		Cprintf(ch, "{c.{x\n\r");
		break;

	case ITEM_THROWING:
		Cprintf(ch, "{cDice:{x %d {cFaces:{x %d {cDamtype:{x %s {cSpell lvl:{x %d {cSpell:{x '%s'\n", obj->value[0], obj->value[1], attack_table[obj->value[2]].name, obj->value[3], skill_table[obj->value[4]].name);
		break;

	case ITEM_DRINK_CON:
		Cprintf(ch, "{cIt holds {x%s{c-colored {x%s{c.{x\n\r", liq_table[obj->value[2]].liq_color, liq_table[obj->value[2]].liq_name);
		break;

	case ITEM_RECALL:
		location = get_room_index(obj->value[0]);
		if (location == NULL)
			break;

		Cprintf(ch, "{cCharges:{x %d  {cMax:{x %d  {cRecalls to:{x %s{c.{x\n\r", obj->value[1], obj->value[2], location->name);
		break;

	case ITEM_ORB:
		Cprintf(ch, "{cCharges:{x %d  {cMax:{x %d  {cSeeks:{x %s{c.{x\n\r", obj->value[1], obj->value[2], ((obj->seek) ? obj->seek : "No one yet"));
		break;

	case ITEM_WEAPON:
		Cprintf(ch, "{cWeapon type is{x ");
		switch (obj->value[0])
		{
		case (WEAPON_EXOTIC):
			Cprintf(ch, "exotic\n\r");
			break;
		case (WEAPON_SWORD):
			Cprintf(ch, "sword\n\r");
			break;
		case (WEAPON_DAGGER):
			Cprintf(ch, "dagger\n\r");
			break;
		case (WEAPON_SPEAR):
			Cprintf(ch, "spear/staff\n\r");
			break;
		case (WEAPON_MACE):
			Cprintf(ch, "mace/club\n\r");
			break;
		case (WEAPON_AXE):
			Cprintf(ch, "axe\n\r");
			break;
		case (WEAPON_FLAIL):
			Cprintf(ch, "flail\n\r");
			break;
		case (WEAPON_WHIP):
			Cprintf(ch, "whip\n\r");
			break;
		case (WEAPON_POLEARM):
			Cprintf(ch, "polearm\n\r");
			break;
		default:
			Cprintf(ch, "unknown\n\r");
			break;
		}

		if (obj->pIndexData->new_format)
			Cprintf(ch, "{cDamage is {x%d{cd{x%d {c(average {x%d{c){x\n\r", obj->value[1], obj->value[2], (1 + obj->value[2]) * obj->value[1] / 2);
		else
			Cprintf(ch, "{cDamage is {x%d{c to {x%d{c (average {x%d{c){x\n\r", obj->value[1], obj->value[2], (obj->value[1] + obj->value[2]) / 2);

		Cprintf(ch, "{cDamage noun is {x%s{c.{x\n\r", (obj->value[3] > 0 && obj->value[3] < MAX_DAMAGE_MESSAGE) ? attack_table[obj->value[3]].noun : "undefined");

		if (obj->value[4])		/* weapon flags */
			Cprintf(ch, "{cWeapons flags: {x%s\n\r", weapon_bit_name(obj->value[4]));

		break;

	case ITEM_ARMOR:
		Cprintf(ch, "{cArmor class is {x%d{c pierce, {x%d{c bash, {x%d{c slash, and {x%d{c vs. magic{x\n\r", obj->value[0], obj->value[1], obj->value[2], obj->value[3]);
		break;

	case ITEM_CONTAINER:
		Cprintf(ch, "{cCapacity: {x%d{c#  Maximum weight: {x%d{c#  flags: {x%s\n\r", obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));

		if (obj->value[4] != 100)
			Cprintf(ch, "{cWeight multiplier: {x%d%{c%{x\n\r", obj->value[4]);

		break;
	}

	if (obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL)
	{
		EXTRA_DESCR_DATA *ed;

		Cprintf(ch, "{cExtra description keywords: {x'");

		for (ed = obj->extra_descr; ed != NULL; ed = ed->next)
		{
			Cprintf(ch, "%s", ed->keyword);

			if (ed->next != NULL)
				Cprintf(ch, " ");
		}

		for (ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next)
		{
			Cprintf(ch, "%s", ed->keyword);

			if (ed->next != NULL)
				Cprintf(ch, " ");
		}

		Cprintf(ch, "'\n\r");
	}

	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
		Cprintf(ch, "{cAffects {x%s{c by {x%d{c, level {x%d", affect_loc_name(paf->location), paf->modifier, paf->level);

		if (paf->duration > -1)
			Cprintf(ch, "{c, {x%d{c hours{x", paf->duration);

		Cprintf(ch, "{c.{x\n\r");

		if (paf->bitvector)
		{
			switch (paf->where)
			{
			case TO_AFFECTS:
				Cprintf(ch, "{cAdds {x%s{c affect.{x\n\r", affect_bit_name(paf->bitvector));
				break;
			case TO_WEAPON:
				Cprintf(ch, "{cAdds {x%s{c weapon flags.{x\n\r", weapon_bit_name(paf->bitvector));
				break;
			case TO_OBJECT:
				if(IS_SET(paf->bitvector, AFF_DAYLIGHT))
					Cprintf(ch, "{cPrevious affect during day time only.{x\n\r");
				else if(IS_SET(paf->bitvector, AFF_DARKLIGHT))
					Cprintf(ch, "{cPrevious affect during night time only.{x\n\r");
				else
					Cprintf(ch, "{cAdds {x%s{c object flag.{x\n\r", extra_bit_name(paf->bitvector));
				break;
			case TO_IMMUNE:
				Cprintf(ch, "{cAdds immunity to {x%s{c.{x\n\r", imm_bit_name(paf->bitvector));
				break;
			case TO_RESIST:
				Cprintf(ch, "{cAdds resistance to {x%s{c.{x\n\r", imm_bit_name(paf->bitvector));
				break;
			case TO_VULN:
				Cprintf(ch, "{cAdds vulnerability to {x%s{c.{x\n\r", imm_bit_name(paf->bitvector));
				break;
			default:
				Cprintf(ch, "{cUnknown bit {x%d{c:{x %d\n\r", paf->where, paf->bitvector);
				break;
			}
		}
	}

	if (!obj->enchanted)
		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
		{
			Cprintf(ch, "{cAffects {x%s{c by {x%d{c, level {x%d{c.{x\n\r", affect_loc_name(paf->location), paf->modifier, paf->level);

			if (paf->bitvector)
			{
				switch (paf->where)
				{
				case TO_AFFECTS:
					Cprintf(ch, "{cAdds {x%s{c affect.{x\n\r", affect_bit_name(paf->bitvector));
					break;
				case TO_OBJECT:
					Cprintf(ch, "{cAdds {x%s{c object flag.{c\n\r", extra_bit_name(paf->bitvector));
					break;
				case TO_IMMUNE:
					Cprintf(ch, "{cAdds immunity to {x%s{c.{x\n\r", imm_bit_name(paf->bitvector));
					break;
				case TO_RESIST:
					Cprintf(ch, "{cAdds resistance to {x%s{c.{x\n\r", imm_bit_name(paf->bitvector));
					break;
				case TO_VULN:
					Cprintf(ch, "{cAdds vulnerability to {x%s{c.{x\n\r", imm_bit_name(paf->bitvector));
					break;
				default:
					Cprintf(ch, "{cUnknown bit {x%d{c: {x%d\n\r", paf->where, paf->bitvector);
					break;
				}
			}
		}

	return;
}


void
do_mstat(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	AFFECT_DATA *paf;
	CHAR_DATA *victim;
	int name_col = 10;
	int col = 25;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Stat whom?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, argument, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (ch->level < GOD)
	{
		if (!IS_NPC(victim))
		{
			Cprintf(ch, "You cannot yet apply this command to a player.\n\r");
			return;
		}
	}

	Cprintf(ch, "%s\n\r", victim->name);

	show_char_stats_to_char(victim, ch);

	Cprintf(ch, "-------------------------------- Wiz Stat -------------------------------\n\r");

	Cprintf(ch, "{cClan:{x      %s\n\r", capitalize(clan_table[victim->clan].name));
	Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*d\n\r",
			name_col, "Group:",
			col, IS_NPC(victim) ? victim->group : 0,
			name_col, "Room:",
			col, victim->in_room == NULL ? 0 : victim->in_room->vnum);

	if (victim->site != NULL)
	{
		Cprintf(ch, "{c%-*s{x %-*s\n\r",
				name_col, "Creation Site:",
				col, victim->site);
	}

	if (!IS_NPC(victim))
	{
		Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*d\n\r",
				name_col, "Thirst:",
				col, victim->pcdata->condition[COND_THIRST],
				name_col, "Hunger:",
				col, victim->pcdata->condition[COND_HUNGER]);
		Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*d\n\r",
				name_col, "Full:",
				col, victim->pcdata->condition[COND_FULL],
				name_col, "Drunk:",
				col, victim->pcdata->condition[COND_DRUNK]);

		Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*d\n\r",
				name_col, "Played:",
				col, victim->played,
				name_col, "Last Level:",
				col, victim->pcdata->last_level);

		Cprintf(ch, "{c%-*s{x %-*d  ",
				name_col, "Timer:",
				col, victim->timer);

		if (IS_IMMORTAL(victim))
		{
			Cprintf(ch, "{c%-*s{x %-*d\n\r",
					name_col, "Gift VNUM:",
					col, victim->gift);
		}
		else
		{
			Cprintf(ch, "\n\r");
		}
	}

	Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
			name_col, "Master:",
			col, victim->master ? victim->master->name : "(none)",
			name_col, "Leader:",
			col, victim->leader ? victim->leader->name : "(none)");

	Cprintf(ch, "{c%-*s{x %-*s\n\r",
			name_col, "Pet:",
			col, victim->pet ? victim->pet->name : "(none)");

	if (victim->act)
	{
		Cprintf(ch, "{c%-*s{x %s\n\r",
				name_col, "Act:",
				act_bit_name(victim->act));
	}
	if (victim->comm)
	{
		Cprintf(ch, "{c%-*s{x %s\n\r",
				name_col, "Comm:",
				comm_bit_name(victim->comm));
	}
	if (IS_NPC(victim) && victim->off_flags)
	{
		Cprintf(ch, "{c%-*s{x %s\n\r",
				name_col, "Off:",
				off_bit_name(victim->off_flags));
	}
	if (victim->imm_flags)
	{
		Cprintf(ch, "{c%-*s{x %s\n\r",
				name_col, "Imm:",
				imm_bit_name(victim->imm_flags));
	}
	if (victim->res_flags)
	{
		Cprintf(ch, "{c%-*s{x %s\n\r",
				name_col, "Res:",
				imm_bit_name(victim->res_flags));
	}
	if (victim->vuln_flags)
	{
		Cprintf(ch, "{c%-*s{x %s\n\r",
				name_col, "Vuln:",
				imm_bit_name(victim->vuln_flags));
	}
	if (victim->affected_by)
	{
		Cprintf(ch, "{c%-*s{x %s\n\r",
				name_col, "Aff:",
				affect_bit_name(victim->affected_by));
	}

	Cprintf(ch, "{c%-*s{x %s\n\r",
			name_col, "Form:",
			form_bit_name(victim->form));

	Cprintf(ch, "{c%-*s{x %s\n\r",
			name_col, "Parts:",
			part_bit_name(victim->parts));

	Cprintf(ch, "{c%-*s{x %s\n\r",
                name_col, "Size:",
		flag_list(size_flags, victim->size));

	Cprintf(ch, "{c%-20s%-20s%-4s%-6s%-20s%-5s{x\n\r",
			"Spell Name",
			"Modifies",
			"By",
			"Hours",
			"Bits",
			"Level");

	for (paf = victim->affected; paf != NULL; paf = paf->next)
	{
		if(paf->where == TO_RESIST || paf->where == TO_VULN)
			continue;	
		Cprintf(ch, "%-20s%-20s%-4d%-6d%-20s%-5d\n\r",
				skill_table[(int) paf->type].name,
				affect_loc_name(paf->location),
				paf->modifier,
				paf->duration,
				affect_bit_name(paf->bitvector),
				paf->level);
	}

	return;
}


/* ofind and mfind replaced with vnum, vnum skill also added */
void
do_vnum(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char *string;

	string = one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Syntax:\n\r");
		Cprintf(ch, "  vnum obj <name>\n\r");
		Cprintf(ch, "  vnum mob <name>\n\r");
		Cprintf(ch, "  vnum skill <skill or spell>\n\r");
		return;
	}

	if (!str_cmp(arg, "obj"))
	{
		do_ofind(ch, string);
		return;
	}

	if (!str_cmp(arg, "mob") || !str_cmp(arg, "char"))
	{
		do_mfind(ch, string);
		return;
	}

	if (!str_cmp(arg, "skill") || !str_cmp(arg, "spell"))
	{
		do_slookup(ch, string);
		return;
	}
	/* do both */
	do_mfind(ch, argument);
	do_ofind(ch, argument);
}


void
do_mfind(CHAR_DATA * ch, char *argument)
{
	BUFFER *buffer;
	extern int top_mob_index;
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	MOB_INDEX_DATA *pMobIndex;
	int vnum;
	int nMatch;
	bool fAll;
	bool found;

	one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		Cprintf(ch, "Find whom?\n\r");
		return;
	}

	fAll = FALSE;
	found = FALSE;
	nMatch = 0;

	/*
	 * Yeah, so iterating over all vnum's takes 10,000 loops.
	 * Get_mob_index is fast, and I don't feel like threading another link.
	 * Do you?
	 * -- Furey
	 */
	buffer = new_buf();
	for (vnum = 0; nMatch < top_mob_index; vnum++)
	{
		if ((pMobIndex = get_mob_index(vnum)) != NULL)
		{
			nMatch++;
			if (fAll || is_name(argument, pMobIndex->player_name))
			{
				found = TRUE;
				sprintf(buf, "{c[{x%5d{c]{x %s\n\r", pMobIndex->vnum, pMobIndex->short_descr);
				add_buf(buffer, buf);
			}
		}
	}

	if (!found)
		Cprintf(ch, "No mobiles by that name.\n\r");
	else
		page_to_char(buf_string(buffer), ch);

	free_buf(buffer);
	return;
}


void
do_ofind(CHAR_DATA * ch, char *argument)
{
	BUFFER *buffer;
	extern int top_obj_index;
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	OBJ_INDEX_DATA *pObjIndex;
	int vnum;
	int nMatch;
	bool fAll;
	bool found;

	one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		Cprintf(ch, "Find what?\n\r");
		return;
	}

	fAll = FALSE;
	found = FALSE;
	nMatch = 0;

	buffer = new_buf();

	/*
	 * Yeah, so iterating over all vnum's takes 10,000 loops.
	 * Get_obj_index is fast, and I don't feel like threading another link.
	 * Do you?
	 * -- Furey
	 */
	for (vnum = 0; nMatch < top_obj_index; vnum++)
	{
		if ((pObjIndex = get_obj_index(vnum)) != NULL)
		{
			nMatch++;
			if (fAll || is_name(argument, pObjIndex->name))
			{
				found = TRUE;
				sprintf(buf, "{c[{x%5d{c]{x %s\n\r", pObjIndex->vnum, pObjIndex->short_descr);
				add_buf(buffer, buf);
			}
		}
	}

	if (!found)
		Cprintf(ch, "No objects by that name.\n\r");
	else
		page_to_char(buf_string(buffer), ch);

	free_buf(buffer);
	return;
}


void
passpass(CHAR_DATA * ch, char *characterName, char *newPassword)
{
	char temp[MAXLINE];
	char charFilename[512];
	char tempCharFilename[512];
	char *p;
	char *pwdnew;
	FILE *charFile;
	FILE *tempCharFile;

	/* check whether the character exists
	 */

	/* create the filename
	 */
	sprintf(charFilename, "%s%s", PLAYER_DIR, capitalize(characterName));

	/* try to open the file, if it fails, report the error
	 */
	if ((charFile = fopen(charFilename, "r")) == NULL)
	{
		Cprintf(ch, "That Pfile doesn't exist.\n\r");
		return;
	}

	/* close the file -- we only wanted to test if it exists.
	 */
	fclose(charFile);

	pwdnew = crypt(newPassword, charFilename);
	for (p = pwdnew; *p != '\0'; p++)
	{
		if (*p == '~')
		{
			Cprintf(ch, "'%s' is not acceptable as a password, try again.\n\r", newPassword);
			return;
		}
	}

	/* rename the filename to "<filename>.tmp"  (don't want to lose
	 * it...)
	 */

	/* create the command which renames the file
	 */
	sprintf(temp, "mv %s %s.tmp", charFilename, charFilename);

	/* call system() (ie/ use the OS) to rename the file
	 */
	system(temp);

	/* create the name for the renamed file
	 */
	sprintf(tempCharFilename, "%s.tmp", charFilename);

	/* open the renamed file for reading (ascii)
	 */
	tempCharFile = fopen(tempCharFilename, "ra");

	/* if that failed, report and barf
	 */
	if (tempCharFile == NULL)
	{
		Cprintf(ch, "Major error opening %s for reading.  Quitting.\n\r", tempCharFilename);
		return;
	}

	/* open the old filename for writing (ascii) -- overwrites
	 */
	charFile = fopen(charFilename, "wa");

	/* if that failed, report and barf
	 */
	if (charFile == NULL)
	{
		Cprintf(ch, "Unable to open %s for writing.  Quitting.\n\r", charFilename);
		return;
	}

	/* ok, read through tempCharFile, paste it into charFile except
	 * for the new password -- replace that.
	 */

	/* fgets() returns the value of temp (address), unless EOF or error,
	 * in which case it returns NULL
	 */
	while (fgets(temp, MAXLINE, tempCharFile))
	{
		/* use some crude string comparisons to see if we're on the
		 * password line
		 */
		if (strncmp("Pass ", temp, 5) == 0)
		{
			/* password line... replace password
			 */
			sprintf(temp, "Pass %s~\n", pwdnew);
		}

		/* copy string to new file, check for error
		 */
		if (fputs(temp, tempCharFile) == EOF)
		{
			Cprintf(ch, "Error while writing to %s.  Old character saved in %s.  Quitting.\n\r", charFilename, tempCharFilename);
			fclose(tempCharFile);
			fclose(charFile);
			return;
		}
	}

	/* close the files
	 */
	fclose(tempCharFile);
	fclose(charFile);

	/* report success
	 */
	Cprintf(ch, "%s's password has been changed.\n\r", charFilename);

	/* erase the temp file
	 */
	sprintf(temp, "rm %s", tempCharFilename);
	system(temp);
}


void
do_setpass(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	char *p;
	char *pwdnew;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "You need to provide a name.\n\r");
		return;
	}

	if (argument[0] == '\0')
	{
		Cprintf(ch, "You need to provide a password.\n\r");
		return;
	}


	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		passpass(ch, arg, argument);

		return;
	}
	else
	{
		pwdnew = crypt(argument, victim->name);
		for (p = pwdnew; *p != '\0'; p++)
		{
			if (*p == '~')
			{
				Cprintf(ch, "'%s' is not acceptable as a password, try again.\n\r", argument);
				return;
			}
		}
		free_string(victim->pcdata->pwd);
		victim->pcdata->pwd = str_dup(pwdnew);
		save_char_obj(victim, FALSE);
		Cprintf(ch, "Password changed.\n\r");
		return;
	}

	return;
}


void
do_gather(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj;
	int count;

	count = 0;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "SYNTAX: gather <obj name>\n\r");
		return;
	}

	for (obj = object_list; obj != NULL; obj = obj->next)
	{
		if (can_see_obj(ch, obj) &&
			is_name(argument, obj->name) &&
			IS_NPC(obj->carried_by) &&
			ch->level > obj->level &&
			obj->carried_by != ch &&
			IS_SET(obj->wear_flags, ITEM_TAKE))
		{
			if (count > 60)
			{
				Cprintf(ch, "The maximum of 60 objects has been attained.\n\r");
				return;
			}

			if (obj->carried_by == NULL)
			{
				get_obj(ch, obj, NULL);
				obj = object_list;
				count++;
			}
			else if (obj->carried_by->pIndexData->pShop == NULL)
			{
				obj_from_char(obj);
				obj_to_char(obj, ch);

				obj = object_list;
				count++;
			}
		}
	}

	Cprintf(ch, "All objects have been gathered.\n\r");
}


void
do_disperseroom(CHAR_DATA * ch, char *argument) {
    OBJ_DATA *obj;
    int roomup;
    ROOM_INDEX_DATA *room;
    int counter;
    OBJ_DATA *next_obj;
    int dispersedCount = 0;
    bool savedObj = FALSE;

    if (argument[0] == '\0') {
        Cprintf(ch, "SYNTAX: disperseroom <obj name>\n\r");
        return;
    }

    if (ch->carrying == NULL) {
        Cprintf(ch, "You have nothing to disperse!\n\r");
        return;
    }

    for (obj = ch->carrying; obj != NULL; obj = next_obj) {
        next_obj = obj->next_content;
        if (is_name(argument, obj->name)) {
            // Don't disperse the first one (save it)
            if (!savedObj) {
                savedObj = TRUE;
                continue;
            }

            counter = 0;
            do {
                roomup = number_range(1, 32000);
                room = get_room_index(roomup);
                counter++;
            } while (counter < 100 && (room == NULL || room->area->security < 9));

            if (counter == 100) {
                if (dispersedCount == 0) {
                    Cprintf(ch, "Having trouble finding a room to disperse to.\n\r");
                } else {
                    Cprintf(ch, "Having trouble finding rooms to disperse your items.\n\r");
                }
                break;
            }

            obj_from_char(obj);
            obj_to_room(obj, room);
            dispersedCount++;
        }
    }

    if (dispersedCount == 0) {
        Cprintf(ch, "You didn't disperse any items.\n\r");
    } else {
        Cprintf(ch, "You dispersed %d items.\n\r", dispersedCount);
    }
}

void
do_disperse(CHAR_DATA * ch, char *argument) {
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    CHAR_DATA *victim;
    int count;
    int mobcount;
    int mobup;
    int dispersedCount = 0;
    bool savedObj = FALSE;

    if (argument[0] == '\0') {
        Cprintf(ch, "SYNTAX: disperse <obj name>\n\r");
        return;
    }

    count = 0;

    for (victim = char_list; victim != NULL; victim = victim->next) {
        count++;
    }

    if (count == 0) {
        Cprintf(ch, "No one here to disperse your stuff to, buddy, sorry.\n\r");
        return;
    }

    if (ch->carrying == NULL) {
        Cprintf(ch, "You have nothing to disperse!\n\r");
        return;
    }

    for (obj = ch->carrying; obj != NULL; obj = obj_next) {
        obj_next = obj->next_content;
        if (is_name(argument, obj->name)) {
            int npcCount = 0;

            // Don't disperse the first one (save it)
            if (!savedObj) {
                savedObj = TRUE;
                continue;
            }

            victim = char_list;
            mobup = number_range(2, count);
            for (mobcount = 1; mobcount <= mobup; mobcount++) {
                if (IS_NPC(victim)) {
                    npcCount++;
                }

                victim = victim->next;

                if (victim == NULL) {
                    if (npcCount == 0) {
                        Cprintf(ch, "There are no NPCs left to disperse items to.\n\r");

                        if (dispersedCount == 0) {
                            Cprintf(ch, "You didn't disperse any items.\n\r");
                        } else {
                            Cprintf(ch, "You dispersed %d items.\n\r", dispersedCount);
                        }
                    } else {
                        victim = char_list;
                        // Set npcCount to 0 incase there are now no NPCs left
                        npcCount = 0;
                    }
                }
            }

            if (IS_NPC(victim) && victim->pIndexData->pShop == NULL && victim->in_room->area->security == 9) {
                obj_from_char(obj);
                obj_to_char(obj, victim);
                obj = ch->carrying;
                dispersedCount++;
            }
        }
    }

    if (dispersedCount == 0) {
        Cprintf(ch, "You didn't disperse any items.\n\r");
    } else {
        Cprintf(ch, "You dispersed %d items.\n\r", dispersedCount);
    }
}


void
do_owhere(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_INPUT_LENGTH];
	BUFFER *buffer;
	OBJ_DATA *obj;
	OBJ_DATA *in_obj;
	bool found;
	int number = 0, max_found;

	found = FALSE;
	number = 0;
	max_found = 200;

	buffer = new_buf();

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Find what?\n\r");
		return;
	}

	for (obj = object_list; obj != NULL; obj = obj->next)
	{
		if (!can_see_obj(ch, obj) || !is_name(argument, obj->name) || ch->level < obj->level)
			continue;

		found = TRUE;
		number++;

		for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj);

		if (in_obj->carried_by != NULL 
			&& can_see(ch, in_obj->carried_by) 
			&& in_obj->carried_by->in_room != NULL
			&& (ch->level > 55 || ch->trust > 55 || IS_BUILDER(ch, in_obj->carried_by->in_room->area)))
			sprintf(buf, "%3d{c){x %s {cis carried by {x%s {c[Room {x%d{c]{x\n\r",
					number,
					obj->short_descr,
					PERS(in_obj->carried_by, ch),
					in_obj->carried_by->in_room->vnum);
		else if (in_obj->in_room != NULL 
			&& can_see_room(ch, in_obj->in_room)
			&& (ch->level > 55 || ch->trust > 55 || IS_BUILDER(ch, in_obj->in_room->area)))
			sprintf(buf, "%3d{c){x %s {cis in {x%s {c[Room {x%d{c]{x\n\r",
					number,
					obj->short_descr,
					in_obj->in_room->name,
					in_obj->in_room->vnum);

		else if (in_obj->carried_by != NULL
			 && (in_obj->carried_by->invis_level > ch->level
			     || in_obj->carried_by->incog_level > ch->level))
	        {
			continue;
		}
		else
			sprintf(buf, "%3d{c){x %s {cis somewhere{x\n\r", number, obj->short_descr);

		buf[0] = UPPER(buf[0]);
		add_buf(buffer, buf);

		if (number >= max_found)
			break;
	}

	if (!found)
		Cprintf(ch, "Nothing like that in heaven or earth.\n\r");
	else
		page_to_char(buf_string(buffer), ch);

	free_buf(buffer);
}


void
do_mwhere(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	BUFFER *buffer;
	CHAR_DATA *victim;
	bool found;
	int count = 0;
	DESCRIPTOR_DATA *d;

	if (argument[0] == '\0')
	{
		if (ch->level < 56 && ch->trust < 56)
		{
			Cprintf(ch, "You must be level 56 to mwhere for players. Please provide an argument.\n\r");
			return;
		}


		/* show characters logged */

		buffer = new_buf();
		for (d = descriptor_list; d != NULL; d = d->next)
		{
			if (d->character != NULL &&
				d->connected == CON_PLAYING &&
				d->character->in_room != NULL &&
				can_see(ch, d->character) &&
				ch->level >= d->character->invis_level &&
				ch->level >= d->character->incog_level &&
				can_see_room(ch, d->character->in_room))
			{
				victim = d->character;
				count++;
				if (d->original != NULL)
					sprintf(buf, "%3d{c){x %s {c(in the body of {x%s{c) is in {x%s{c [{x%d{c]{x\n\r",
							count,
							d->original->name,
							victim->short_descr,
							victim->in_room->name,
							victim->in_room->vnum);
				else
					sprintf(buf, "%3d{c) {x%s{c is in {x%s{c [{x%d{c]{x\n\r", count, victim->name, victim->in_room->name, victim->in_room->vnum);

				add_buf(buffer, buf);
			}
		}

		page_to_char(buf_string(buffer), ch);
		free_buf(buffer);
		return;
	}

	found = FALSE;
	buffer = new_buf();
	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->in_room != NULL 
			&& is_name(argument, victim->name) 
			&& ch->level >= victim->invis_level 
			&& ch->level >= victim->incog_level
			&& (ch->level >= 56 || (IS_NPC(victim) && victim->in_room != NULL && IS_BUILDER(ch, victim->in_room->area))))
		{
			found = TRUE;
			count++;
			sprintf(buf, "%3d{c) [{x%5d{c] {x%-28s{c [{x%5d{c] {x%s\n\r",
					count,
					IS_NPC(victim) ? victim->pIndexData->vnum : 0,
					IS_NPC(victim) ? victim->short_descr : victim->name,
					victim->in_room->vnum,
					victim->in_room->name);
			add_buf(buffer, buf);
		}
	}

	if (!found)
		act("You didn't find any $T.", ch, NULL, argument, TO_CHAR, POS_RESTING);
	else
		page_to_char(buf_string(buffer), ch);

	free_buf(buffer);

	return;
}


void
do_reboo(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "If you want to REBOOT, spell it out.\n\r");
	return;
}


void
do_reboot(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	extern bool merc_down;
	DESCRIPTOR_DATA *d, *d_next;
	CHAR_DATA *vch;
	bool SYSTEMHAPPY = FALSE;

	if (!str_cmp(ch->name, "System")) { 	
		SYSTEMHAPPY = TRUE;
	}

	if (ch->invis_level < LEVEL_HERO)
	{
		sprintf(buf, "{GReboot by {R%s{G.{x", ch->name);
		do_echo(ch, buf);
	}

	do_restore(ch, "all");
	if(!SYSTEMHAPPY)
		Cprintf(ch, "Saving all characters.\n\r");

	merc_down = TRUE;
	for (d = descriptor_list; d != NULL; d = d_next)
	{
		d_next = d->next;
		vch = d->original ? d->original : d->character;
		if(vch != NULL) {
			if(!SYSTEMHAPPY)
				Cprintf(ch, "%s saved.\n\r", vch->name);
			save_char_obj(vch, TRUE);
		}
		close_socket(d);
	}

	return;
}


void
do_shutdow(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "If you want to SHUTDOWN, spell it out.\n\r");
	return;
}


void
do_shutdown(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	extern bool merc_down;
	DESCRIPTOR_DATA *d, *d_next;
	CHAR_DATA *vch;

	if (ch->invis_level < LEVEL_HERO)
		sprintf(buf, "Shutdown by %s.", ch->name);
	append_file(ch, SHUTDOWN_FILE, buf);
	strcat(buf, "\n\r");
	if (ch->invis_level < LEVEL_HERO)
		do_echo(ch, buf);
	merc_down = TRUE;
	for (d = descriptor_list; d != NULL; d = d_next)
	{
		d_next = d->next;
		vch = d->original ? d->original : d->character;
		if (vch != NULL)
			save_char_obj(vch, TRUE);
		close_socket(d);
	}
	return;
}


void
do_protect(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Protect whom from snooping?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, argument, TRUE)) == NULL)
	{
		Cprintf(ch, "You can't find them.\n\r");
		return;
	}

	if (IS_SET(victim->comm, COMM_SNOOP_PROOF))
	{
		act("$N is no longer snoop-proof.", ch, NULL, victim, TO_CHAR, POS_DEAD);
		Cprintf(victim, "Your snoop-proofing was just removed.\n\r");
		REMOVE_BIT(victim->comm, COMM_SNOOP_PROOF);
	}
	else
	{
		act("$N is now snoop-proof.", ch, NULL, victim, TO_CHAR, POS_DEAD);
		Cprintf(victim, "You are now immune to snooping.\n\r");
		SET_BIT(victim->comm, COMM_SNOOP_PROOF);
	}
}


void
do_snoop(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		bool fFound = FALSE;

		Cprintf(ch, "You are currently snooping");
		for (d = descriptor_list; d != NULL; d = d->next)
			if (d->snoop_by == ch->desc)
			{
				if (!fFound)
				{
					fFound = TRUE;
					Cprintf(ch, ":\n\r");
				}

				Cprintf(ch, "%s\n\r", d->character->name);
			}

		if (!fFound)
			Cprintf(ch, " noone.\n\r");

		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim->desc == NULL)
	{
		Cprintf(ch, "No descriptor to snoop.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "Cancelling all snoops.\n\r");
		wiznet("$N stops being such a snoop.", ch, NULL, WIZ_SNOOPS, 0, get_trust(ch));
		for (d = descriptor_list; d != NULL; d = d->next)
			if (d->snoop_by == ch->desc)
				d->snoop_by = NULL;

		return;
	}

	if (victim->desc->snoop_by && victim->desc->snoop_by != ch->desc)
	{
		Cprintf(ch, "Busy already.\n\r");
		return;
	}

	if (victim->desc->snoop_by == ch->desc)
	{
		Cprintf(ch, "You stop snooping %s.\n\r", ((IS_NPC(victim)) ? victim->short_descr : victim->name));
		victim->desc->snoop_by = NULL;
		sprintf(buf, "$N stops snooping on %s", (IS_NPC(victim) ? victim->short_descr : victim->name));
		wiznet(buf, ch, NULL, WIZ_SNOOPS, 0, get_trust(ch));
		return;
	}

	if (get_trust(victim) >= get_trust(ch) || (IS_SET(victim->comm, COMM_SNOOP_PROOF) && !IS_TRUSTED(ch, 60)))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (ch->desc != NULL)
		for (d = ch->desc->snoop_by; d != NULL; d = d->snoop_by)
			if (d->character == victim || d->original == victim)
			{
				Cprintf(ch, "No snoop loops.\n\r");
				return;
			}

	victim->desc->snoop_by = ch->desc;
	sprintf(buf, "$N starts snooping on %s", (IS_NPC(victim) ? victim->short_descr : victim->name));
	wiznet(buf, ch, NULL, WIZ_SNOOPS, 0, get_trust(ch));
	Cprintf(ch, "Ok.\n\r");
	return;
}


void
do_switch(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Switch into whom?\n\r");
		return;
	}

	if (ch->desc == NULL)
		return;

	if (ch->desc->original != NULL)
	{
		Cprintf(ch, "You are already switched.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You more truly become yourself.  Don't ask how that works.\n\r");
		return;
	}

	if (!IS_NPC(victim))
	{
		Cprintf(ch, "You can only switch into mobiles.\n\r");
		return;
	}

	if (!is_room_owner(ch, victim->in_room) &&
		ch->in_room != victim->in_room &&
		room_is_private(victim->in_room) &&
		!IS_TRUSTED(ch, IMPLEMENTOR))
	{
		Cprintf(ch, "That character is in a private room.\n\r");
		return;
	}

	if (victim->desc != NULL)
	{
		Cprintf(ch, "Character in use.\n\r");
		return;
	}

	sprintf(buf, "$N switches into %s", victim->short_descr);
	wiznet(buf, ch, NULL, WIZ_SWITCHES, 0, get_trust(ch));

	ch->desc->character = victim;
	ch->desc->original = ch;
	victim->desc = ch->desc;
	ch->desc = NULL;
	/* change communications to match */
	if (ch->prompt != NULL)
		victim->prompt = str_dup(ch->prompt);
	victim->comm = ch->comm;
	victim->lines = ch->lines;
	Cprintf(victim, "Ok.\n\r");
	return;
}


void
do_return(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];

	if (ch->desc == NULL)
		return;

	if (ch->desc->original == NULL)
	{
		Cprintf(ch, "You aren't switched.\n\r");
		return;
	}

	Cprintf(ch, "You return to your original body. Type replay to see any missed tells.\n\r");

	if (ch->prompt != NULL)
	{
		free_string(ch->prompt);
		ch->prompt = NULL;
	}

	sprintf(buf, "$N returns from %s.", ch->short_descr);
	wiznet(buf, ch->desc->original, 0, WIZ_SWITCHES, 0, get_trust(ch));
	ch->desc->character = ch->desc->original;
	ch->desc->original = NULL;
	ch->desc->character->desc = ch->desc;
	ch->desc = NULL;
	return;
}


/* trust levels for load and clone */
bool
obj_check(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (IS_TRUSTED(ch, GOD)
	  || (IS_TRUSTED(ch, IMMORTAL) && obj->level <= 20 && obj->cost <= 1000)
		|| (IS_TRUSTED(ch, DEMI) && obj->level <= 10 && obj->cost <= 500)
		|| (IS_TRUSTED(ch, ANGEL) && obj->level <= 5 && obj->cost <= 250)
		|| (IS_TRUSTED(ch, AVATAR) && obj->level == 0 && obj->cost <= 100))
		return TRUE;
	else
		return FALSE;
}


/* for clone, to insure that cloning goes many levels deep */
void
recursive_clone(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * clone)
{
	OBJ_DATA *c_obj, *t_obj;

	for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content)
		if (obj_check(ch, c_obj))
		{
			t_obj = create_object(c_obj->pIndexData, 0);
			clone_object(c_obj, t_obj);
			obj_to_obj(t_obj, clone);
			recursive_clone(ch, c_obj, t_obj);
		}
}


/* command that is similar to load */
void
do_clone(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char *rest;
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	int number;
	int count;

	rest = one_argument(argument, arg);
	number = mult_argument(rest, arg2);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Clone what?\n\r");
		return;
	}

	if (!str_prefix(arg, "object"))
	{
		mob = NULL;
		obj = get_obj_here(ch, arg2);
		if (obj == NULL)
		{
			Cprintf(ch, "You don't see that here.\n\r");
			return;
		}
	}
	else if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character"))
	{
		obj = NULL;
		mob = get_char_room(ch, arg2);
		if (mob == NULL)
		{
			Cprintf(ch, "You don't see that here.\n\r");
			return;
		}
	}
	else
		/* find both */
	{
		Cprintf(ch, "Please specify, obj or mob.\n\r");
		return;
	}

	/* clone an object */
	if (obj != NULL)
	{
		OBJ_DATA *clone = '\0';

		if (!obj_check(ch, obj))
		{
			Cprintf(ch, "Your powers are not great enough for such a task.\n\r");
			return;
		}

		count = number;
		while (count > 0)
		{
			clone = create_object(obj->pIndexData, 0);
			clone_object(obj, clone);
			if (obj->carried_by != NULL)
				obj_to_char(clone, ch);
			else
				obj_to_room(clone, ch->in_room);
			recursive_clone(ch, obj, clone);

			count--;
		}

		act("$n has created $p.", ch, clone, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You clone %s", obj->short_descr);
		if (number > 1)
			Cprintf(ch, "[%d]", number);
		Cprintf(ch, ".\n\r");

		sprintf(buf, "$N clones $p [%d].", number);
		wiznet(buf, ch, clone, WIZ_LOAD, 0, get_trust(ch));
		return;
	}
	else if (mob != NULL)
	{
		CHAR_DATA *clone = '\0';
		OBJ_DATA *new_obj;
		char buf[MAX_STRING_LENGTH];

		if (!IS_NPC(mob))
		{
			Cprintf(ch, "You can only clone mobiles.\n\r");
			return;
		}

		if ((mob->level > 20 && !IS_TRUSTED(ch, GOD))
			|| (mob->level > 10 && !IS_TRUSTED(ch, IMMORTAL))
			|| (mob->level > 5 && !IS_TRUSTED(ch, DEMI))
			|| (mob->level > 0 && !IS_TRUSTED(ch, ANGEL))
			|| !IS_TRUSTED(ch, AVATAR))
		{
			Cprintf(ch, "Your powers are not great enough for such a task.\n\r");
			return;
		}

		count = number;
		while (count > 0)
		{
			clone = create_mobile(mob->pIndexData);
			clone_mobile(mob, clone);
			clone->is_clone = TRUE;

			for (obj = mob->carrying; obj != NULL; obj = obj->next_content)
			{
				if (obj_check(ch, obj))
				{
					new_obj = create_object(obj->pIndexData, 0);
					clone_object(obj, new_obj);
					recursive_clone(ch, obj, new_obj);
					obj_to_char(new_obj, clone);
					new_obj->wear_loc = obj->wear_loc;
				}
			}

			char_to_room(clone, ch->in_room);

			count--;
		}

		act("$n has created $N.", ch, NULL, clone, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You clone %s", clone->short_descr);
		if (number > 1)
			Cprintf(ch, "[%d]", number);
		Cprintf(ch, ".\n\r");

		sprintf(buf, "$N clones %s.", clone->short_descr);
		wiznet(buf, ch, NULL, WIZ_LOAD, 0, get_trust(ch));
		return;
	}
}


/* RT to replace the two load commands */
void
do_load(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Syntax:\n\r");
		Cprintf(ch, "  load mob <vnum>\n\r");
		Cprintf(ch, "  load obj <vnum> <level>\n\r");
		return;
	}

	if (!str_cmp(arg, "mob") || !str_cmp(arg, "char"))
	{
		do_mload(ch, argument);
		return;
	}

	if (!str_cmp(arg, "obj"))
	{
		do_oload(ch, argument);
		return;
	}

	/* echo syntax */
	do_load(ch, "");
}


void
do_mload(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	MOB_INDEX_DATA *pMobIndex;
	AREA_DATA *Area;
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];
	int level;

	one_argument(argument, arg);

	if (arg[0] == '\0' || !is_number(arg))
	{
		Cprintf(ch, "Syntax: mload <vnum>\n\r");
		return;
	}

	if ((pMobIndex = get_mob_index(atoi(arg))) == NULL)
	{
		Cprintf(ch, "No mob has that vnum.\n\r");
		return;
	}

	level = get_trust(ch);
	Area = get_vnum_area(atoi(arg));

	if (level < 56 && !IS_BUILDER(ch, Area))
	{
		Cprintf(ch, "You can only load from within your area.\n\r");
		return;
	}

	victim = create_mobile(pMobIndex);
	char_to_room(victim, ch->in_room);
	victim->is_clone = TRUE;
	act("$n has created $N!", ch, NULL, victim, TO_ROOM, POS_RESTING);
	sprintf(buf, "$N loads %s [%d].", victim->short_descr, victim->pIndexData->vnum);
	wiznet(buf, ch, NULL, WIZ_LOAD, 0, get_trust(ch));
	Cprintf(ch, "You create %s.\n\r", victim->short_descr);
	return;
}


void
do_oload(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	AREA_DATA *Area;
	OBJ_INDEX_DATA *pObjIndex;
	OBJ_DATA *obj;
	int level;

	argument = one_argument(argument, arg1);
	one_argument(argument, arg2);

	if (arg1[0] == '\0' || !is_number(arg1))
	{
		Cprintf(ch, "Syntax: oload <vnum>\n\r");
		Cprintf(ch, "        oload <vnum> <level>\n\r");
		return;
	}

	level = get_trust(ch);		/* default */

	if (arg2[0] != '\0')		/* load with a level */
	{
		if (!is_number(arg2))
		{
			Cprintf(ch, "Syntax: oload <vnum> <level>.\n\r");
			return;
		}
		level = atoi(arg2);

		if (level < 0 || level > get_trust(ch))
		{
			Cprintf(ch, "Level must be be between 0 and your level.\n\r");
			return;
		}
	}

	if ((pObjIndex = get_obj_index(atoi(arg1))) == NULL)
	{
		Cprintf(ch, "No object has that vnum.\n\r");
		return;
	}

	Area = get_vnum_area(atoi(arg1));

	if (level < 56 && !IS_BUILDER(ch, Area))
	{
		Cprintf(ch, "You can only load from within your area.\n\r");
		return;
	}

	obj = create_object(pObjIndex, level);
	if (CAN_WEAR(obj, ITEM_TAKE))
		obj_to_char(obj, ch);
	else
		obj_to_room(obj, ch->in_room);
	act("$n has created $p!", ch, obj, NULL, TO_ROOM, POS_RESTING);
	sprintf(buf, "$N loads $p [%d].", obj->pIndexData->vnum);
	wiznet(buf, ch, obj, WIZ_LOAD, 0, get_trust(ch));
	Cprintf(ch, "You create %s.\n\r", obj->short_descr);

	return;
}


void
do_purge(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	DESCRIPTOR_DATA *d;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{

		/* 'purge' */
		CHAR_DATA *vnext;
		OBJ_DATA *obj_next;

		if (!IS_NPC(ch) && ch->level < 55 && ch->trust < 55 && !IS_BUILDER(ch, ch->in_room->area))
		{
			Cprintf(ch, "Builders can only purge rooms in their area.\n\r");
			return;
		}

		for (victim = ch->in_room->people; victim != NULL; victim = vnext)
		{
			vnext = victim->next_in_room;
			if (IS_NPC(victim) && !IS_SET(victim->act, ACT_NOPURGE) && victim != ch)
				extract_char(victim, TRUE);
		}

		for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			if (!IS_OBJ_STAT(obj, ITEM_NOPURGE))
				extract_obj(obj);
		}

		act("$n purges the room!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You purge the room.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (!IS_NPC(victim))
	{
		if (ch == victim)
		{
			Cprintf(ch, "Ho ho ho.\n\r");
			return;
		}

		if (ch->level < 55 && ch->trust < 55)
		{
			Cprintf(ch, "You failed.\n\r");
			return;
		}

		if (get_trust(ch) <= get_trust(victim))
		{
			Cprintf(ch, "Maybe that wasn't a good idea...\n\r");
			Cprintf(victim, "%s tried to purge you!\n\r", ch->name);
			return;
		}

		act("$n disintegrates $N.", ch, 0, victim, TO_NOTVICT, POS_RESTING);

		if (victim->level > 1)
			save_char_obj(victim, TRUE);
		d = victim->desc;
		extract_char(victim, TRUE);
		if (d != NULL)
			close_socket(d);

		return;
	}

	act("$n purges $N.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	extract_char(victim, TRUE);
	return;
}


void
do_vapourize(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	bool fFound = FALSE;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Please specify a target to vapourize.\n\r");
		return;
	}

	if (ch->level < 55 && ch->trust < 55 && !IS_BUILDER(ch, ch->in_room->area))
	{
		Cprintf(ch, "Builders can only purge rooms in their area.\n\r");
		return;
	}

	/* 'purge' */
	for (obj = ch->in_room->contents; !fFound && obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;

		if (is_name(arg, obj->short_descr))
		{
			fFound = TRUE;
			strcpy(arg, obj->short_descr);
			extract_obj(obj);
		}
	}

	if (!fFound)
		Cprintf(ch, "You could not find %s in this room.\n\r", arg);
	else
		Cprintf(ch, "You vapourize %s.\n\r", arg);

	return;
}


void
do_advance(CHAR_DATA * ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int level;
	int iLevel;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
		Cprintf(ch, "Syntax: advance <char> <level>.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL) {
		Cprintf(ch, "That player is not here.\n\r");
		return;
	}

	if (IS_NPC(victim)) {
		Cprintf(ch, "Not on NPC's.\n\r");
		return;
	}

	if (!TEST_PORT && ch == victim) {
		Cprintf(ch, "Now, that's just SILLY.\n\r");
		return;
	}

	if ((level = atoi(arg2)) < 1 || level > 60) {
		Cprintf(ch, "Level must be 1 to 60.\n\r");
		return;
	}

	if (level == victim->level) {
		Cprintf(ch, "That's a waste of time.\n\r");
		return;
	}

	if (level > get_trust(ch)) {
		Cprintf(ch, "Limited to your trust level.\n\r");
		return;
	}

	/* new code for leader stuff */

	/*
	 * Lower level
	 */
	if (level <= victim->level) {
        int modifierToHit, modifierToMana, modifierToMove;
        int currentMana, currentHit, currentMove;
        int baseMana, baseHit, baseMove;
        int gainedMana, gainedHit, gainedMove;
        int levelLoss;
        int averageMana, averageHit, averageMove;
        int lostMana, lostHit, lostMove;
        int newMana, newHit, newMove;
        int totalMana, totalHit, totalMove;

        if (victim->level == 53) {
			Cprintf(ch, "Lowering %s to level %d!\n\r", victim->name, level);
			Cprintf(ch, "This will free the leader slot.\n\r");

			demote_leader(victim);
			return;
		} else if (victim->level == 52) {
			Cprintf(ch, "Lowering %s to level %d!\n\r", victim->name, level);
			Cprintf(ch, "This will free a recruiter spot.\n\r");

			demote_recruiter(victim);
			return;
		}

		// modfiers due to eq
		modifierToHit = ch->max_hit_bonus; 
		modifierToMana = ch->max_mana_bonus;
		modifierToMove = ch->max_move_bonus;

		// Current values
		currentMana = victim->max_mana;
		currentHit = victim->max_hit;
		currentMove = victim->max_move;

		// Base values (without eq)
		baseMana = currentMana - modifierToMana;
		baseHit = currentHit - modifierToHit;
		baseMove = currentMove - modifierToMove;

		// Amount gained through leveling
		gainedMana = baseMana - 100;
		gainedHit = baseHit - 20;
		gainedMove = baseMove - 100;

		levelLoss = victim->level - level;

		// average gain / level
		averageMana = (int) (((float) gainedMana / (float) victim->level) + 0.5);
		averageHit = (int) (((float) gainedHit / (float) victim->level) + 0.5);
		averageMove = (int) (((float) gainedMove / (float) victim->level) + 0.5);

		// loss (due to being set back)
		lostMana = averageMana * levelLoss;
		lostHit = averageHit * levelLoss;
		lostMove = averageMove * levelLoss;

		// new base values
		newMana = baseMana - lostMana;
		newHit = baseHit - lostHit;
		newMove = baseMove - lostMove;

		// new total values
		totalMana = newMana + modifierToMana;
		totalHit = newHit + modifierToHit;
		totalMove = newMove + modifierToMove;

		Cprintf(ch, "Lowering %s to level %d!\n\r", victim->name, level);
		Cprintf(victim, "**** SHIT BALLS ****\n\r");
		Cprintf(ch, "Lost %5d hit, %5d mana, %5d move.\n\r", lostHit, lostMana, lostMove);

		victim->level = level;
		victim->max_hit = totalHit;
		victim->max_mana = totalMana;
		victim->max_move = totalMove;
		victim->hit = MAX_HP(victim);
		victim->mana = MAX_MANA(victim);
		victim->move = MAX_MOVE(victim);
		victim->practice -= 5 * levelLoss;
		victim->practice = (victim->practice >= 0) ? victim->practice : 0;
		victim->train = levelLoss;
		victim->train = (victim->train >= 0) ? victim->train : 0;
	} else {
		int iMana, iHit, iMove, mana, move, hit;

		if (level == 53) {
            advance_leader(victim);
            
            if (victim->level != 53 && victim->trust != 53) {
                Cprintf(ch, "%s cannot be raised to %d since %s already has a leader (%s).\n\r", victim->name, level, capitalize(clan_table[victim->clan].name), clan_leadership[victim->clan].leader);
                return;
            }
            
			Cprintf(ch, "Raising %s to level %d!\n\r", victim->name, level);

			return;
		} else if (level == 52 && victim->trust != 52) {
            advance_recruiter(victim);
            
            if (victim->level != 52) {
                Cprintf(ch, "%s cannot be raised to %d because %s already has two recruiters (%s, %s).\n\r", victim->name, level, capitalize(clan_table[victim->clan].name), clan_leadership[victim->clan].recruiter1, clan_leadership[victim->clan].recruiter2);
                return;
            }
            
			Cprintf(ch, "Raising %s to level %d!\n\r", victim->name, level);

			return;
		}

		iMana = victim->max_mana;
		iMove = victim->max_move;
		iHit = victim->max_hit;

		if (victim->level < 54 && level >= 54) {
			SET_BIT(ch->toggles, TOGGLES_AUTOVALUE);
		}

		for (iLevel = victim->level; iLevel < level; iLevel++) {
			victim->level += 1;
			advance_level(victim, TRUE);
		}

		hit = victim->max_hit - iHit;
		mana = victim->max_mana - iMana;
		move = victim->max_move - iMove;

		Cprintf(ch, "Gained %5d hit, %5d mana, %5d move.\n\r", hit, mana, move);
	}

	victim->exp = exp_per_level(victim, victim->pcdata->points) * UMAX(1, victim->level);
	Cprintf(victim, "You are now level %d.\n\r", victim->level);
	victim->trust = 0;
	victim->delegate = 0;
	save_char_obj(victim, FALSE);
	return;
}


void
do_trust(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int level;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2))
	{
		Cprintf(ch, "Syntax: trust <char> <level>.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
	{
		Cprintf(ch, "That player is not here.\n\r");
		return;
	}

	if ((level = atoi(arg2)) < 0 || level > 60)
	{
		Cprintf(ch, "Level must be 0 (reset) or 1 to 60.\n\r");
		return;
	}

	if (level > get_trust(ch))
	{
		Cprintf(ch, "Limited to your trust.\n\r");
		return;
	}

	victim->trust = level;
	victim->delegate = 0;
	return;
}


void
do_restore(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *vch;
	DESCRIPTOR_DATA *d;

	one_argument(argument, arg);

	if (arg[0] == '\0' || !str_cmp(arg, "room"))
	{
		/* cure room */
		for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
		{
			restore_char(vch);
			act("$n has restored you.", ch, NULL, vch, TO_VICT, POS_RESTING);
		}

		sprintf(buf, "$N restored room %d.", ch->in_room->vnum);
		wiznet(buf, ch, NULL, WIZ_RESTORE, 0, get_trust(ch));

		Cprintf(ch, "Room restored.\n\r");
		return;
	}

	if (get_trust(ch) >= MAX_LEVEL - 1 && !str_cmp(arg, "all"))
	{
		/* cure all */

		for (d = descriptor_list; d != NULL; d = d->next)
		{
			victim = d->character;

			if (victim == NULL || IS_NPC(victim))
				continue;

			restore_char(victim);
			act("$n has restored you.", ch, NULL, victim, TO_VICT, POS_RESTING);
		}

		if (str_cmp(ch->name, "System"))
			Cprintf(ch, "All active players restored.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	restore_char(victim);

	act("$n has restored you.", ch, NULL, victim, TO_VICT, POS_RESTING);
	sprintf(buf, "$N restored %s", IS_NPC(victim) ? victim->short_descr : victim->name);
	wiznet(buf, ch, NULL, WIZ_RESTORE, 0, get_trust(ch));
	Cprintf(ch, "%s restored.\n\r", (IS_NPC(victim) ? victim->short_descr : victim->name));
	return;
}


void
restore_char(CHAR_DATA * ch)
{
	int age;

	affect_strip(ch, gsn_plague);
	affect_strip(ch, gsn_poison);
	affect_strip(ch, gsn_blindness);
	affect_strip(ch, gsn_sleep);
	affect_strip(ch, gsn_curse);
	affect_strip(ch, gsn_faerie_fire);
	affect_strip(ch, gsn_weaken);
	affect_strip(ch, gsn_charm_person);
	affect_strip(ch, gsn_change_sex);
	affect_strip(ch, gsn_loneliness);
	affect_strip(ch, gsn_slow);
	affect_strip(ch, gsn_chill_touch);
	affect_strip(ch, gsn_confusion);
	affect_strip(ch, gsn_corruption);
	affect_strip(ch, gsn_scramble);
	affect_strip(ch, gsn_tame_animal);
	affect_strip(ch, gsn_wail);
	affect_strip(ch, gsn_shifting_sands);
	affect_strip(ch, gsn_detonation);
	affect_strip(ch, gsn_sunray);
	affect_strip(ch, gsn_nightmares);
	affect_strip(ch, gsn_jinx);
	affect_strip(ch, gsn_ensnare);
	affect_strip(ch, gsn_frost_breath);
	affect_strip(ch, gsn_acid_breath);
	affect_strip(ch, gsn_denounciation);
	affect_strip(ch, gsn_hurricane);
	affect_strip(ch, gsn_fire_breath);
	affect_strip(ch, gsn_feeblemind);
	affect_strip(ch, gsn_pain_touch);

	ch->hit = MAX_HP(ch);
	ch->mana = MAX_MANA(ch);
	ch->move = MAX_MOVE(ch);

	age = get_age(ch);
	ch->breath = 10 * ((age > 100) ? 10 :
					   (age > 80) ? 9 :
					   (age > 60) ? 8 :
					   (age > 50) ? 7 :
					   (age > 40) ? 6 :
					   (age > 30) ? 5 :
					   (age > 25) ? 4 : 3);
	ch->can_lay = 0;

	update_pos(ch);
}


void
do_freeze(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Freeze whom?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPC's.\n\r");
		return;
	}

	if (get_trust(victim) >= get_trust(ch))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (IS_SET(victim->act, PLR_FREEZE))
	{
		REMOVE_BIT(victim->act, PLR_FREEZE);
		Cprintf(victim, "You can play again.\n\r");
		Cprintf(ch, "FREEZE removed.\n\r");
		sprintf(buf, "$N thaws %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}
	else
	{
		SET_BIT(victim->act, PLR_FREEZE);
		Cprintf(victim, "You can't do ANYthing!\n\r");
		Cprintf(ch, "FREEZE set.\n\r");
		sprintf(buf, "$N puts %s in the deep freeze.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}

	save_char_obj(victim, FALSE);

	return;
}


void
do_log(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Log whom?\n\r");
		return;
	}

	if (!str_cmp(arg, "all"))
	{
		if (fLogAll)
		{
			fLogAll = FALSE;
			Cprintf(ch, "Log ALL off.\n\r");
		}
		else
		{
			fLogAll = TRUE;
			Cprintf(ch, "Log ALL on.\n\r");
		}
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPC's.\n\r");
		return;
	}

	/*
	 * No level check, gods can log anyone.
	 */
	if (IS_SET(victim->act, PLR_LOG))
	{
		REMOVE_BIT(victim->act, PLR_LOG);
		Cprintf(ch, "LOG removed.\n\r");
	}
	else
	{
		SET_BIT(victim->act, PLR_LOG);
		Cprintf(ch, "LOG set.\n\r");
	}

	return;
}


void
do_notitle(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Notitle whom?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}


	if (get_trust(victim) >= get_trust(ch))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (IS_SET(victim->toggles, TOGGLES_NOTITLE))
	{
		Cprintf(ch, "%s can now use title without restriction.\n\r", victim->name);
		Cprintf(victim, "The gods reinstated your right to express yourself.\n\r");
		REMOVE_BIT(victim->toggles, TOGGLES_NOTITLE);
	}
	else
	{
		Cprintf(ch, "%s can no longer use title freely.\n\r", victim->name);
		Cprintf(victim, "The gods have revoked your freedom of expression.\n\r");
		SET_BIT(victim->toggles, TOGGLES_NOTITLE);
	}
}


void
do_noemote(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Noemote whom?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}


	if (get_trust(victim) >= get_trust(ch))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (IS_SET(victim->comm, COMM_NOEMOTE))
	{
		REMOVE_BIT(victim->comm, COMM_NOEMOTE);
		Cprintf(victim, "You can emote again.\n\r");
		Cprintf(ch, "NOEMOTE removed.\n\r");
		sprintf(buf, "$N restores emotes to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}
	else
	{
		SET_BIT(victim->comm, COMM_NOEMOTE);
		Cprintf(victim, "You can't emote!\n\r");
		Cprintf(ch, "NOEMOTE set.\n\r");
		sprintf(buf, "$N revokes %s's emotes.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}

	return;
}


void
do_noshout(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Noshout whom?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPC's.\n\r");
		return;
	}

	if (get_trust(victim) >= get_trust(ch))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (IS_SET(victim->comm, COMM_NOSHOUT))
	{
		REMOVE_BIT(victim->comm, COMM_NOSHOUT);
		Cprintf(victim, "You can shout again.\n\r");
		Cprintf(ch, "NOSHOUT removed.\n\r");
		sprintf(buf, "$N restores shouts to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}
	else
	{
		SET_BIT(victim->comm, COMM_NOSHOUT);
		Cprintf(victim, "You can't shout!\n\r");
		Cprintf(ch, "NOSHOUT set.\n\r");
		sprintf(buf, "$N revokes %s's shouts.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}

	return;
}


void
do_notell(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Notell whom?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (get_trust(victim) >= get_trust(ch))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (IS_SET(victim->comm, COMM_NOTELL))
	{
		REMOVE_BIT(victim->comm, COMM_NOTELL);
		Cprintf(victim, "You can tell again.\n\r");
		Cprintf(ch, "NOTELL removed.\n\r");
		sprintf(buf, "$N restores tells to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}
	else
	{
		SET_BIT(victim->comm, COMM_NOTELL);
		Cprintf(victim, "You can't tell!\n\r");
		Cprintf(ch, "NOTELL set.\n\r");
		sprintf(buf, "$N revokes %s's tells.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}

	return;
}


void
do_peace(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *rch;

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
	{
		if (rch->fighting != NULL)
			stop_fighting(rch, TRUE);
		if (IS_NPC(rch) && IS_SET(rch->act, ACT_AGGRESSIVE))
			REMOVE_BIT(rch->act, ACT_AGGRESSIVE);
		if (rch->hunting != NULL) {
			rch->hunting = NULL;
			rch->hunt_timer = 0;
		}
	}

	Cprintf(ch, "All fighting in the room has stopped.\n\r");
	return;
}


void
do_wizlock(CHAR_DATA * ch, char *argument)
{
	extern bool wizlock;

	wizlock = !wizlock;

	if (wizlock)
	{
		wiznet("$N has wizlocked the game.", ch, NULL, 0, 0, 0);
		Cprintf(ch, "Game wizlocked.\n\r");
	}
	else
	{
		wiznet("$N removes wizlock.", ch, NULL, 0, 0, 0);
		Cprintf(ch, "Game un-wizlocked.\n\r");
	}

	return;
}


/* RT anti-newbie code */
void
do_newlock(CHAR_DATA * ch, char *argument)
{
	extern bool newlock;

	newlock = !newlock;

	if (newlock)
	{
		wiznet("$N locks out new characters.", ch, NULL, 0, 0, 0);
		Cprintf(ch, "New characters have been locked out.\n\r");
	}
	else
	{
		wiznet("$N allows new characters back in.", ch, NULL, 0, 0, 0);
		Cprintf(ch, "Newlock removed.\n\r");
	}

	return;
}


void
do_slookup(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	int sn;

	one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		Cprintf(ch, "Lookup which skill or spell?\n\r");
		return;
	}

	if (!str_cmp(arg, "all"))
	{
		for (sn = 0; sn < MAX_SKILL; sn++)
		{
			if (skill_table[sn].name == NULL)
				break;
			Cprintf(ch, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r", sn, skill_table[sn].slot, skill_table[sn].name);
		}
	}
	else
	{
		if ((sn = skill_lookup(arg)) < 0)
		{
			Cprintf(ch, "No such skill or spell.\n\r");
			return;
		}

		Cprintf(ch, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r", sn, skill_table[sn].slot, skill_table[sn].name);
	}

	return;
}


/* RT set replaces sset, mset, oset, and rset */
void
do_set(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Syntax:\n\r");
		Cprintf(ch, "  set mob   <name> <field> <value>\n\r");
		Cprintf(ch, "  set obj   <name> <field> <value>\n\r");
		Cprintf(ch, "  set room  <room> <field> <value>\n\r");
		Cprintf(ch, "  set skill <name> <spell or skill> <value>\n\r");
		Cprintf(ch, "  set clan  <clan> [leader|r1|r2|recruiter1|recruiter2] <name>\n\r");
		return;
	}

	if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character"))
	{
		do_mset(ch, argument);
		return;
	}

	if (!str_prefix(arg, "skill") || !str_prefix(arg, "spell"))
	{
		do_sset(ch, argument);
		return;
	}

	if (!str_prefix(arg, "object"))
	{
		do_oset(ch, argument);
		return;
	}

	if (!str_prefix(arg, "room"))
	{
		do_rset(ch, argument);
		return;
	}

	if (!str_prefix(arg, "clans"))
	{
		do_cset(ch, argument);
		return;
	}

	/* echo syntax */
	do_set(ch, "");
}

void
do_cset(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	int clan;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
	{
		Cprintf(ch, "Syntax:\n\r");
		Cprintf(ch, "  set clan <clan> [leader|r1|r2|recruiter1|recruiter2] <name>\n\r");
		return;
	}

	/* arg1 -> clan name */
	clan = clan_lookup(arg1);
	if (clan_table[clan].independent)
	{
		Cprintf(ch, "That is not a valid clan name.\n\r");
		return;
	}

	/* up to IMM to watch the spelling, no checks so we can do it offline */
	if (!str_prefix(arg2, "leader"))
	{
		free_string(clan_leadership[clan].leader);
		if (str_cmp(arg3, "none"))
		{
			clan_leadership[clan].leader = NULL;
		}
		else
		{
			clan_leadership[clan].leader = str_dup(arg3);
		}
	}
	else if (!str_prefix(arg2, "r1") || !str_prefix(arg2, "recruiter1"))
	{
		free_string(clan_leadership[clan].recruiter1);
		if (str_cmp(arg3, "none"))
		{
			clan_leadership[clan].recruiter1 = NULL;
		}
		else
		{
			clan_leadership[clan].recruiter1 = str_dup(arg3);
		}
	}
	else if (!str_prefix(arg2, "r2") || !str_prefix(arg2, "recruiter2"))
	{
		free_string(clan_leadership[clan].recruiter2);
		if (str_cmp(arg3, "none"))
		{
			clan_leadership[clan].recruiter2 = NULL;
		}
		else
		{
			clan_leadership[clan].recruiter2 = str_dup(arg3);
		}
	}
	else
	{
		Cprintf(ch, "Please specify a leader or recruiter position.\n\r");
		return;
	}

	write_clanleaders();

	return;
}

void
do_sset(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int value;
	int sn;
	bool fAll;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
	{
		Cprintf(ch, "Syntax:\n\r");
		Cprintf(ch, "  set skill <name> <spell or skill> <value>\n\r");
		Cprintf(ch, "  set skill <name> all <value>\n\r");
		Cprintf(ch, "   (use the name of the skill, not the number)\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPC's.\n\r");
		return;
	}

	fAll = !str_cmp(arg2, "all");
	sn = 0;
	if (!fAll && (sn = skill_lookup(arg2)) < 0)
	{
		Cprintf(ch, "No such skill or spell.\n\r");
		return;
	}

	/*
	 * Snarf the value.
	 */
	if (!is_number(arg3))
	{
		Cprintf(ch, "Value must be numeric.\n\r");
		return;
	}

	value = atoi(arg3);
	if (value < 0 || value > 130)
	{
		Cprintf(ch, "Value range is 0 to 130.\n\r");
		return;
	}

	if (sn != victim->pcdata->specialty && value > 100)
	{	
		Cprintf(ch, "That can only be done to a specialization.\n\r");
		return;
	}

	if (fAll)
	{
		for (sn = 0; sn < MAX_SKILL; sn++)
		{
			if (skill_table[sn].name != NULL)
				victim->pcdata->learned[sn] = value;
		}
	}
	else
	{
		victim->pcdata->learned[sn] = value;
	}

	if (ch != victim)
	{
		if (fAll)
			Cprintf(ch, "All of %s's skills and spells set to %d.\n\r", victim->name, value);
		else
			Cprintf(ch, "'%s' set to %d for %s.\n\r", skill_table[sn].name, value, victim->name);
	}

	if (fAll)
		Cprintf(victim, "All of your skills set to %d.\n\r", value);
	else
		Cprintf(victim, "'%s' set to %d.\n\r", skill_table[sn].name, value);

	return;
}


void
do_mset(CHAR_DATA * ch, char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int value;
    char *vicName;

    smash_tilde(argument);
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    strcpy(arg3, argument);

    if ((arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') && arg1[0] != '?') {
        Cprintf(ch, "Syntax:\n\r");
        Cprintf(ch, "  set char <name> <field> <value>\n\r");
        Cprintf(ch, "  Field being one of:\n\r");
        Cprintf(ch, "    str int wis dex con sex class level sliver\n\r");
        Cprintf(ch, "    race group gold silver hp mana move prac\n\r");
        Cprintf(ch, "    align train thirst hunger drunk full bounty\n\r");
        Cprintf(ch, "    hours questpoints hunt pkills deaths security\n\r");
        Cprintf(ch, "    delegate npc rptitle deity npc patronpts\n\r");
        Cprintf(ch, "    plimit remort reclass\n\r");
        Cprintf(ch, "  Use 'set char ?' or 'mset ?' to get a detailed description of the fields.\n\r");
        return;
    }

    if (arg1[0] == '?') {
        char buf[MAX_STRING_LENGTH];
        BUFFER *buffer;

        buffer = new_buf();

        sprintf(buf, "  set char <name> <field> <value>\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "  Field being one of:\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     align:    sets align.  Range: -1000 to 1000.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     bounty:   sets bounty\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     class:    sets the characters class.  Mobs don't have class.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     con:      sets a character's intelligence.  Range: 3 to racial max.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     deaths:   sets pk'd.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     deity:    sets the character's diety points.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     delegate: sets what character can loner till.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     dex:      sets a character's intelligence.  Range: 3 to racial max.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     drunk:    sets drunkeness on PCs.  Range: -1 to 100.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     full:     sets full level on PCs.  Range: -1 to 100.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     gold:     sets gold.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     group:    I have no idea what this does.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     hours:    sets hours.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     hp:       sets hp.  Range: -10 to 30000.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     hunger:   sets hunger level on PCs.  Range: -1 to 100.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     hunt:     sets who the mob is hunting. '.' sets to none.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     int:      sets a character's intelligence.  Range: 3 to racial max.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     level:    sets an NPC's level.  Range: 0 to 60.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     mana:     sets mana.  Range: 0 to 30000.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     move:     sets movement.  Range: 0 to 30000.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     npc:      turns a mob into a quest mob.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     patronpts: set patron points\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     pkills:   sets pkills.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     plimit: reset the claim limit on patron pts (0 or reset)\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     practice: sets pracs.  Range: 0 to 250.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     quest:    sets questpoints.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     race:     sets race.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     rptitle:  sets a character's rptitle.  Use 0 to reset the rptitle.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     security: sets a characters security rating (OLC).  Range: 0 to your sec.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     sex:      male, female, neuter.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     sliver:   sets sliver type.  Use name instead of numeric value.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     str:      sets a character's strength.  Range: 3 to racial max.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     thirst:   sets thirst on PCs.  Range: -1 to 100.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     train:    sets trains.  Range: 0 to 50.\n\r");
        add_buf(buffer, buf);
        sprintf(buf, "     wis:      sets a character's intelligence.  Range: 3 to racial max.\n\r");
        add_buf(buffer, buf);

        page_to_char(buf_string(buffer), ch);
        free_buf(buffer);

        return;
    }

    if ((victim = get_char_world(ch, arg1, TRUE)) == NULL) {
        Cprintf(ch, "They aren't here.\n\r");
        return;
    }

    vicName = capitalize((IS_NPC(victim)) ? victim->short_descr : victim->name);

    /* clear zones for mobs */
    victim->zone = NULL;

    /*
     * Snarf the value (which need not be numeric).
     */
    value = is_number(arg3) ? atoi(arg3) : -1;

    if (!IS_NPC(victim) && (victim->level > ch->level)) {
        Cprintf(ch, "Yeah, setting higher level imms is a smart idea.\n\r");
        return;
    }

    /*
     * Set something.
     */
    if (!str_cmp(arg2, "rptitle")) {
        if (IS_NPC(victim)) {
            Cprintf(ch, "Only players can have rptitles.\n\r");
            return;
        }

        if (victim->rptitle) {
            free_string(victim->rptitle);
        }

        if (arg3[0] == '0') {
            victim->rptitle = &str_empty[0];
        } else {
            victim->rptitle = str_dup(arg3);
        }

        if (ch != victim) {
            Cprintf(ch, "%s's rptitle is now [%s].\n\r", vicName, victim->rptitle);
        }

        Cprintf(victim, "Your rptitle has been changed to [%s].\n\r", victim->rptitle);

        return;
    }

    if (!str_cmp(arg2, "str")) {
        if (value < 3 || value > get_max_train(victim, STAT_STR)) {
            Cprintf(ch, "Strength range is 3 to %d\n\r.", get_max_train(victim, STAT_STR));
            return;
        }

        victim->perm_stat[STAT_STR] = value;

        if (ch != victim) {
            Cprintf(ch, "%s's strength set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your strength has been set to %d.\n\r", value);
        return;
    }

    if (!str_prefix(arg2, "security")) {
        /* OLC */

        if (ch->level < 60) {
            Cprintf(ch, "You must be an Imp to set OLC security levels.\n\r");
            return;
        }

        if (IS_NPC(victim)) {
            Cprintf(ch, "Not on NPC's.\n\r");
            return;
        }

        if (victim == ch) {
            if (value > ch->pcdata->security) {
                Cprintf(ch, "Shya, right.  And monkeys'll fly outta my butt.\n\r");
            } else if (value < ch->pcdata->security) {
                Cprintf(ch, "NOT a good idea.\n\r");
            } else {
                Cprintf(ch, "What good is that?\n\r");
            }

            return;
        }

        if (value > ch->pcdata->security || value < 0) {
            if (ch->pcdata->security != 0) {
                Cprintf(ch, "Valid security is 0-%d.\n\r", ch->pcdata->security);
    	        } else {
                Cprintf(ch, "Valid security is 0 only.\n\r");
            }

            return;
        }

        victim->pcdata->security = value;

        if (ch != victim) {
            Cprintf(ch, "%s's security set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your security set to %d.\n\r", value);

        return;
    }

    if (!str_cmp(arg2, "int")) {
        if (value < 3 || value > get_max_train(victim, STAT_INT)) {
            Cprintf(ch, "Intelligence range is 3 to %d.\n\r", get_max_train(victim, STAT_INT));
            return;
        }

        victim->perm_stat[STAT_INT] = value;
        return;

        if (ch != victim) {
            Cprintf(ch, "%s's intelligence set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your intelligence has been set to %d.\n\r", value);
    }

    if (!str_prefix(arg2, "npc")) {
        if (!IS_NPC(victim)) {
            Cprintf(ch, "You can only do this to NPC's.\n\r");
            return;
        }

        if (victim->on_who) {
            victim->on_who = FALSE;
        } else {
            victim->on_who = TRUE;
        }

        Cprintf(ch, "%s is %s a quest mob.\n\r", vicName, ((victim->on_who) ? "now" : "no longer"));

        return;
    }

    /* Change their hunting status. */
    if (!str_prefix(arg2, "hunt")) {
        CHAR_DATA *hunted = NULL;

        if (!IS_NPC(victim)) {
            Cprintf(ch, "Not on PC's.\n\r");
            return;
        }

        if (str_cmp(arg3, ".")) {
            if ((hunted = get_char_area(victim, arg3)) == NULL) {
                Cprintf(ch, "Mob couldn't locate the victim to hunt.\n\r");
                return;
            }

            if (ch != victim) {
                Cprintf(ch, "%s starts hunting %s.\n\r", victim->short_descr, capitalize((IS_NPC(hunted)) ? hunted->short_descr : hunted->name));
            }

            Cprintf(hunted, "%s starts hunting you.\n\r", victim->short_descr);
        } else {
            Cprintf(ch, "%s stops hunting %s.  He is now in room %d.\n\r", vicName, ((victim->hunting) ? capitalize((IS_NPC(victim->hunting)) ? victim->hunting->short_descr : victim->hunting->name) : "no one"), ((victim->in_room) ? victim->in_room->vnum : -1));

            if (victim->hunting) {
                Cprintf(victim->hunting, "%s stops hunting you.\n\r", vicName);
            }
        }

        victim->hunting = hunted;
        return;
    }

    if (!str_cmp(arg2, "wis")) {
        if (value < 3 || value > get_max_train(victim, STAT_WIS)) {
            Cprintf(ch, "Wisdom range is 3 to %d.\n\r", get_max_train(victim, STAT_WIS));
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's wisdom set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your wisdom has been set to %d.\n\r", value);

        victim->perm_stat[STAT_WIS] = value;
        return;
    }

    if (!str_cmp(arg2, "dex")) {
        if (value < 3 || value > get_max_train(victim, STAT_DEX)) {
            Cprintf(ch, "Dexterity ranges is 3 to %d.\n\r", get_max_train(victim, STAT_DEX));
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's dexterity set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your dexterity has been set to %d.\n\r", value);

        victim->perm_stat[STAT_DEX] = value;
        return;
    }

    if (!str_cmp(arg2, "con")) {
        if (value < 3 || value > get_max_train(victim, STAT_CON)) {
            Cprintf(ch, "Constitution range is 3 to %d.\n\r", get_max_train(victim, STAT_CON));
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's constitution set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your constitution has been set to %d.\n\r", value);

        victim->perm_stat[STAT_CON] = value;
        return;
    }

    if (!str_cmp(arg2, "deity")) {
        if (value < 0) {
            Cprintf(ch, "Only positive values please.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's deity points set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your deity points has been set to %d.\n\r", value);

        victim->deity_points = value;
        return;
    }

    if (!str_prefix(arg2, "sex")) {
        char *tSex, *oSex, *nSex;

        if (str_prefix(arg3, "male") && str_prefix(arg3, "female") && str_prefix(arg3, "neuter")) {
            Cprintf(ch, "Please enter male, female or neuter.\n\r");
            return;
        }

        if (!str_prefix(arg3, "male")) {
            value = SEX_MALE;
        } else if (!str_prefix(arg3, "female")) {
            value = SEX_FEMALE;
        } else {
            value = SEX_NEUTRAL;
        }

        oSex = (victim->sex == SEX_MALE) ? "male" : ((victim->sex == SEX_FEMALE) ? "female" : "neuter");
        nSex = (value == SEX_MALE) ? "male" : ((value == SEX_FEMALE) ? "female" : "neuter");

        if (ch != victim) {
            Cprintf(ch, "%s's sex changed from %s to %s.\n\r", vicName, oSex, nSex);
        }

        if (!IS_NPC(victim) && ch != victim) {
            tSex = (victim->pcdata->true_sex == SEX_MALE) ? "male" : ((victim->pcdata->true_sex == SEX_FEMALE) ? "female" : "neuter");
            Cprintf(ch, "%s's true sex was %s, is now %s.\n\r", victim->name, tSex, nSex);
        }

        Cprintf(victim, "Your sex changed from %s to %s.\n\r", oSex, nSex);

        victim->sex = value;
        if (!IS_NPC(victim)) {
            victim->pcdata->true_sex = value;
        }

        return;
    }

    if (!str_prefix(arg2, "class")) {
        int charClass;

        if (IS_NPC(victim)) {
            Cprintf(ch, "Mobiles have no class.  They wear white after Labour Day.\n\r");
            return;
        }

        charClass = class_lookup(arg3);

        if (charClass == -1) {
            Cprintf(ch, "Possible classes are: ");

            for (charClass = 0; charClass < MAX_CLASS; charClass++) {
                if (charClass > 0) {
                    Cprintf(ch, " ");
                }

                Cprintf(ch, "%s", class_table[charClass].name);
            }

            Cprintf(ch, ".\n\r");

            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's class changed from %s to %s.\n\r", vicName, class_table[victim->charClass].name, class_table[charClass].name);
        }

        Cprintf(ch, "You are now a %s.\n\r", class_table[charClass].name);
        victim->charClass = charClass;
        return;
    }

    if (!str_prefix(arg2, "level")) {
        if (!IS_NPC(victim)) {
            Cprintf(ch, "Not on PC's.\n\r");
            return;
        }

        if (value < 0 || value > 60) {
            Cprintf(ch, "Level range is 0 to 60.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's level set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your level set to %d.\n\r", value);

        victim->level = value;
        return;
    }

    if (!str_prefix(arg2, "gold")) {
        if (ch != victim) {
            Cprintf(ch, "%s's gold set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your gold set to %d.\n\r", value);

        victim->gold = value;
        return;
    }

    if (!str_prefix(arg2, "delegate")) {
        if (ch != victim) {
            Cprintf(ch, "%s's delegation level set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your delegation level set to %d.\n\r", value);

        victim->delegate = value;
        return;
    }

    if (!str_prefix(arg2, "silver")) {
        if (ch != victim) {
            Cprintf(ch, "%s's silver set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your silver set to %d.\n\r", value);

        victim->silver = value;
        return;
    }

    if (!str_prefix(arg2, "sliver")) {
        int sliver, colCount = 0;
        bool fFound = FALSE;

        for (sliver = 1; sliver_table[sliver].name; sliver++) {
            if (!str_prefix(arg3, sliver_table[sliver].name)) {
                fFound = TRUE;
                break;
            }
        }

        if (!fFound) {
            Cprintf(ch, "Invalid sliver type.  Valid types are:\n\r");

            for (sliver = 1; sliver_table[sliver].name; sliver++) {
                Cprintf(ch, "%-15s", sliver_table[sliver].name);

                if (!(++colCount % 5)) {
                    Cprintf(ch, "\n\r");
                }
            }

            if (colCount % 5) {
                Cprintf(ch, "\n\r");
            }

            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's sliver type set to %s.\n\r", vicName, sliver_table[sliver].name);
        }

        Cprintf(victim, "Your sliver type set to %s.\n\r", sliver_table[sliver].name);

        victim->sliver = sliver;
        return;
    }

    if (!str_prefix(arg2, "bounty")) {
        if (ch != victim) {
            Cprintf(ch, "%s's bounty set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your bounty set to %d.\n\r", value);

        victim->bounty = value;
        return;
    }

    if (!str_prefix(arg2, "questpoints")) {
        if (ch != victim) {
            Cprintf(ch, "%s's questpoints set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your questpoints set to %d.\n\r", value);

        victim->questpoints = value;
        return;
    }

    if (!str_prefix(arg2, "hours")) {
        if (ch != victim) {
            Cprintf(ch, "%s's set to %d hour%s.\n\r", vicName, value, ((value == 1) ? "" : "s"));
        }

        Cprintf(victim, "You're set to %d hour%s.\n\r", value, ((value == 1) ? "" : "s"));

        victim->played -= victim->played_perm;
        victim->played_perm = value * 3600;
        victim->played = victim->played + victim->played_perm;
        return;
    }

    if (!str_prefix(arg2, "pkills")) {
        if (ch != victim) {
            Cprintf(ch, "%s's set to %d pkill%s.\n\r", vicName, value, ((value == 1) ? "" : "s"));
        }

        Cprintf(victim, "You're set to %d pkill%s.\n\r", value, ((value == 1) ? "" : "s"));

        victim->pkills = value;
        return;
    }

    if (!str_prefix(arg2, "deaths")) {
        if (ch != victim) {
            Cprintf(ch, "%s's set to %d death%s.\n\r", vicName, value, ((value == 1) ? "" : "s"));
        }

        Cprintf(victim, "You're set to %d death%s.\n\r", value, ((value == 1) ? "" : "s"));

        victim->deaths = value;
        return;
    }

    if (!str_prefix(arg2, "hp")) {
        if (value < -10 || value > 30000) {
            Cprintf(ch, "Hp range is -10 to 30,000 hit points.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's hitpoints set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your hitpoints set to %d.\n\r", value);

        victim->max_hit = value - victim->max_hit_bonus;

        return;
    }

    if (!str_prefix(arg2, "mana")) {
        if (value < 0 || value > 30000) {
            Cprintf(ch, "Mana range is 0 to 30,000 mana points.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's mana set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your mana set to %d.\n\r", value);

        victim->max_mana = value - victim->max_mana_bonus;

        return;
    }

    if (!str_prefix(arg2, "move")) {
        if (value < 0 || value > 30000) {
            Cprintf(ch, "Move range is 0 to 30,000 move points.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's movement set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your movement set to %d.\n\r", value);

        victim->max_move = value - victim->max_move_bonus;

        return;
    }

    if (!str_prefix(arg2, "practice")) {
        if (value < 0 || value > 250) {
            Cprintf(ch, "Practice range is 0 to 250 sessions.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's set to %d practice$s.\n\r", vicName, value, ((value == 1) ? "" : "s"));
        }

        Cprintf(victim, "You're set to %d practice$s.\n\r", value, ((value == 1) ? "" : "s"));

        victim->practice = value;
        return;
    }

    if (!str_prefix(arg2, "train")) {
        if (value < 0 || value > 50) {
            Cprintf(ch, "Training session range is 0 to 50 sessions.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's set to %d train%s.\n\r", vicName, value, ((value == 1) ? "" : "s"));
        }

        Cprintf(victim, "You're set to %d train%s.\n\r", value, ((value == 1) ? "" : "s"));

        victim->train = value;
        return;
    }

    if (!str_prefix(arg2, "align")) {
        if (value < -1000 || value > 1000) {
            Cprintf(ch, "Alignment range is -1000 to 1000.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's align set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your align set to %d.\n\r", value);

        victim->alignment = value;
        return;
    }

    if (!str_prefix(arg2, "thirst")) {
        if (IS_NPC(victim)) {
            Cprintf(ch, "Not on NPC's.\n\r");
            return;
        }

        if (value < -1 || value > 100) {
            Cprintf(ch, "Thirst range is -1 to 100.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's thirst set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your thirst set to %d.\n\r", value);

        victim->pcdata->condition[COND_THIRST] = value;
        return;
    }

    if (!str_prefix(arg2, "drunk")) {
        if (IS_NPC(victim)) {
            Cprintf(ch, "Not on NPC's.\n\r");
            return;
        }

        if (value < -1 || value > 100) {
            Cprintf(ch, "Drunk range is -1 to 100.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's drunkeness set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your drunkeness set to %d.\n\r", value);

        victim->pcdata->condition[COND_DRUNK] = value;
        return;
    }

    if (!str_prefix(arg2, "full")) {
        if (IS_NPC(victim)) {
            Cprintf(ch, "Not on NPC's.\n\r");
            return;
        }

        if (value < -1 || value > 100) {
            Cprintf(ch, "Full range is -1 to 100.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's full level set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your full level set to %d.\n\r", value);

        victim->pcdata->condition[COND_FULL] = value;
        return;
    }

    if (!str_prefix(arg2, "hunger")) {
        if (IS_NPC(victim)) {
            Cprintf(ch, "Not on NPC's.\n\r");
            return;
        }

        if (value < -1 || value > 100) {
            Cprintf(ch, "Hunger range is -1 to 100.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's hunger level set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your hunger level set to %d.\n\r", value);

        victim->pcdata->condition[COND_HUNGER] = value;
        return;
    }

    if (!str_prefix(arg2, "race")) {
        int race;

        race = race_lookup(arg3);

        if (race == 0) {
            Cprintf(ch, "That is not a valid race.\n\r");
            return;
        }

        if (!IS_NPC(victim) && !race_table[race].pc_race) {
            Cprintf(ch, "That is not a valid player race.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's race set to %s.\n\r", vicName, race_table[race].name);
        }

        Cprintf(victim, "Your race set to %s.\n\r", race_table[race].name);

        victim->race = race;
        return;
    }

    if (!str_prefix(arg2, "group")) {
        if (!IS_NPC(victim)) {
            Cprintf(ch, "Only on NPCs.\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's group set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your group set to %d.\n\r", value);

        victim->group = value;
        return;
    }

    if (!str_prefix(arg2, "patronpts")) {
        if (IS_NPC(victim)) {
            Cprintf(ch, "Only players can have patron points.\n\r");
            return;
        }

        if (arg3[0] == '\0' || value < 0) {
            Cprintf(ch, "set char <name> patronpts <amount>\n\r");
            return;
        }

        if (ch != victim) {
            Cprintf(ch, "%s's patron points set to %d.\n\r", vicName, value);
        }

        Cprintf(victim, "Your patron points set to %d.\n\r", value);

        victim->pass_along = value;	
        return;
    }

    if (!str_prefix(arg2, "plimit")) {
        int newlimit = 0;
        if (IS_NPC(victim)) {
            Cprintf(ch, "Only players can have patron points.\n\r");
            return;
        }

        if (value == 0) {
            Cprintf(ch, "That player can no longer claim patron points.\n\r");
            victim->pass_along_limit = 0;
            return;
        } else {
            //patron change: reset cap on patron points
            if (ch->level >= 51) {
                newlimit = 10000;
            } else {
                newlimit = exp_per_level(ch, ch->pcdata->points) / 2;
            }

            if (ch != victim) {
                Cprintf(ch, "%s's patron claim limit set to %d.\n\r", vicName, newlimit);
            }

            Cprintf(victim, "Your patron claim limit set to %d.\n\r", newlimit);

            victim->pass_along_limit = newlimit;
            return;
        }
    }

    if (!str_prefix(arg2, "remort")) {
        Cprintf(ch, "mset remort isn't implemented yet.  This is a place-holder.\n\r");
        return;
    }

    if (!str_prefix(arg2, "reclass")) {
        Cprintf(ch, "mset reclass isn't implemented yet.  This is a place-holder.\n\r");
        return;
    }

    /*
     * Generate usage message.
     */
    do_mset(ch, "");
    return;
}


void
do_string(CHAR_DATA * ch, char *argument)
{
	char type[MAX_INPUT_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;

	smash_tilde(argument);
	argument = one_argument(argument, type);
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	strcpy(arg3, argument);


	if (type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
	{
		if (!str_prefix(type, "character") && !str_prefix(arg2, "short"))
		{
			if (ch->level == 58)
				Cprintf(ch, "Reseting character whodesc.\n\r");
			else
				Cprintf(ch, "You have to be level 58 to set the whodesc.\n\r");
		}
		else
		{
			Cprintf(ch, "Syntax:\n\r");
			Cprintf(ch, "  string char <name> <field> <string>\n\r");
			Cprintf(ch, "    fields: name short long desc title spec\n\r");
			Cprintf(ch, "  string obj  <name> <field> <string>\n\r");
			Cprintf(ch, "    fields: name short long extended\n\r");
			return;
		}
	}

	if (!str_prefix(type, "character") || !str_prefix(type, "mobile"))
	{
		if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
		{
			Cprintf(ch, "They aren't here.\n\r");
			return;
		}

		if (ch->trust < victim->trust)
		{
			Cprintf(ch, "Seemed like a good idea at the time...Try that on someone lower.\n\r");
			return;
		}

		/* clear zone for mobs */
		victim->zone = NULL;

		/* string something */

		if (!str_prefix(arg2, "name"))
		{
			if (!IS_NPC(victim))
			{
				Cprintf(ch, "Not on PC's.\n\r");
				return;
			}
			free_string(victim->name);
			victim->name = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "description"))
		{
                        if (victim->level > ch->level)
                        {
                                Cprintf(ch, "You should leave higher imms alone.\n\r");
                                return;
                        }

			free_string(victim->description);
			victim->description = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "short"))
		{
			if (ch->level >= 58)
			{
				if (IS_NPC(victim))
				{
					free_string(victim->short_descr);
					victim->short_descr = str_dup(arg3);
					return;
				}
				if (strlen(arg3) >= 18)
				{
					Cprintf(ch, "Sorry, must be less than 18 characters.\n\r");
					return;
				}
				else
				{
					free_string(victim->short_descr);
					victim->short_descr = str_dup(arg3);
					return;
				}
			}
			else
			{
				if (IS_NPC(victim))
				{
					free_string(victim->short_descr);
					victim->short_descr = str_dup(arg3);
					return;
				}
				else
				{
					Cprintf(ch, "Not on players.\n\r");
					return;
				}
			}
		}

		if (!str_prefix(arg2, "long"))
		{
			free_string(victim->long_descr);
			strcat(arg3, "\n\r");
			victim->long_descr = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "title"))
		{
			if (victim->level > ch->level)
			{
				Cprintf(ch, "You should leave higher imms alone.\n\r");
				return;
			}

			if (IS_NPC(victim))
			{
				Cprintf(ch, "Not on NPC's.\n\r");
				return;
			}

			set_title(victim, arg3);
			return;
		}

		if (!str_prefix(arg2, "spec"))
		{
			if (!IS_NPC(victim))
			{
				Cprintf(ch, "Not on PC's.\n\r");
				return;
			}

			if ((victim->spec_fun = spec_lookup(arg3)) == 0)
			{
				Cprintf(ch, "No such spec function.\n\r");
				return;
			}

			return;
		}
	}

	if (!str_prefix(type, "object"))
	{
		/* string an obj */

		if ((obj = get_obj_world(ch, arg1)) == NULL)
		{
			Cprintf(ch, "Nothing like that in heaven or earth.\n\r");
			return;
		}

		if (!str_prefix(arg2, "name"))
		{
			free_string(obj->name);
			obj->name = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "short"))
		{
			free_string(obj->short_descr);
			obj->short_descr = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "long"))
		{
			free_string(obj->description);
			obj->description = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "ed") || !str_prefix(arg2, "extended"))
		{
			EXTRA_DESCR_DATA *ed;

			argument = one_argument(argument, arg3);
			if (argument == NULL)
			{
				Cprintf(ch, "Syntax: oset <object> ed <keyword> <string>\n\r");
				return;
			}

			strcat(argument, "\n\r");
			ed = new_extra_descr();
			ed->keyword = str_dup(arg3);
			ed->description = str_dup(argument);
			ed->next = obj->extra_descr;
			obj->extra_descr = ed;
			return;
		}
	}


	/* echo bad use message */
	do_string(ch, "");
}



void
do_oset(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int value;
	char *oName, *cDesc;

	smash_tilde(argument);
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	strcpy(arg3, argument);

	if ((arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') && (arg1[0] != '?'))
	{
		Cprintf(ch, "Syntax:\n\r");
		Cprintf(ch, "  set obj <object> <field> <value>\n\r");
		Cprintf(ch, "  Field being one of:\n\r");
		Cprintf(ch, "    value0 value1 value2 value3 value4 (v0-v4)\n\r");
		Cprintf(ch, "    evalue0 evalue1 evalue2 evalue3 evalue4 (e0-e4)\n\r");
		Cprintf(ch, "    svalue0 svalue1 svalue2 svalue3 svalue4 (s0-s4)\n\r");
		Cprintf(ch, "    extra wear level weight cost timer\n\r");
		return;
	}
	else if (arg1[0] == '?')
	{
		Cprintf(ch, "  set obj <object> <field> <value>\n\r");
		Cprintf(ch, "     value0, v0: sets the object's v0\n\r");
		Cprintf(ch, "     value1, v1: sets the object's v1\n\r");
		Cprintf(ch, "     value2, v2: sets the object's v2\n\r");
		Cprintf(ch, "     value3, v3: sets the object's v3\n\r");
		Cprintf(ch, "     value4, v4: sets the object's v4\n\r");
		Cprintf(ch, "     extra:      sets the object's extra flags\n\r");
		Cprintf(ch, "     wear:       sets the object's wear flags\n\r");
		Cprintf(ch, "     level:      sets the object's level\n\r");
		Cprintf(ch, "     weight:     sets the object's weight\n\r");
		Cprintf(ch, "     cost:       sets the object's cost\n\r");
		Cprintf(ch, "     timer:      sets the object's decay timer\n\r");
		return;
	}

	if ((obj = get_obj_world(ch, arg1)) == NULL)
	{
		Cprintf(ch, "Nothing like that in heaven or earth.\n\r");
		return;
	}

	oName = (obj->short_descr) ? obj->short_descr : "Something";

	/*
	 * Snarf the value (which need not be numeric).
	 */
	value = atoi(arg3);

	/*
	 * Set something.
	 */
	if (!str_cmp(arg2, "value0") || !str_cmp(arg2, "v0"))
	{
		cDesc = arg2;
		obj->value[0] = UMIN(50, value);
	}
	else if (!str_cmp(arg2, "value1") || !str_cmp(arg2, "v1"))
	{
		cDesc = arg2;
		obj->value[1] = value;
	}
	else if (!str_cmp(arg2, "value2") || !str_cmp(arg2, "v2"))
	{
		cDesc = arg2;
		obj->value[2] = value;
	}
	else if (!str_cmp(arg2, "value3") || !str_cmp(arg2, "v3"))
	{
		cDesc = arg2;
		obj->value[3] = value;
	}
	else if (!str_cmp(arg2, "value4") || !str_cmp(arg2, "v4"))
	{
		cDesc = arg2;
		obj->value[4] = value;
	}
	else if (!str_cmp(arg2, "svalue0") || !str_cmp(arg2, "s0"))
	{
		cDesc = arg2;
		obj->special[0] = value;
	}
	else if (!str_cmp(arg2, "svalue1") || !str_cmp(arg2, "s1"))
	{
		cDesc = arg2;
		obj->special[1] = value;
	}
	else if (!str_cmp(arg2, "svalue2") || !str_cmp(arg2, "s2"))
	{
		cDesc = arg2;
		obj->special[2] = value;
	}
	else if (!str_cmp(arg2, "svalue3") || !str_cmp(arg2, "s3"))
	{
		cDesc = arg2;
		obj->special[3] = value;
	}
	else if (!str_cmp(arg2, "svalue4") || !str_cmp(arg2, "s4"))
	{
		cDesc = arg2;
		obj->special[4] = value;
	}
	else if (!str_cmp(arg2, "evalue0") || !str_cmp(arg2, "e0"))
	{
		cDesc = arg2;
		obj->extra[0] = value;
	}
	else if (!str_cmp(arg2, "evalue1") || !str_cmp(arg2, "e1"))
	{
		cDesc = arg2;
		obj->extra[1] = value;
	}
	else if (!str_cmp(arg2, "evalue2") || !str_cmp(arg2, "e2"))
	{
		cDesc = arg2;
		obj->extra[2] = value;
	}
	else if (!str_cmp(arg2, "evalue3") || !str_cmp(arg2, "e3"))
	{
		cDesc = arg2;
		obj->extra[3] = value;
	}
	else if (!str_cmp(arg2, "evalue4") || !str_cmp(arg2, "e4"))
	{
		cDesc = arg2;
		obj->extra[4] = value;
	}
	else if (!str_prefix(arg2, "extra"))
	{
		cDesc = "extra flags";
		obj->extra_flags = value;
	}
	else if (!str_prefix(arg2, "wear"))
	{
		cDesc = "wear flags";
		obj->wear_flags = value;
	}
	else if (!str_prefix(arg2, "level"))
	{
		cDesc = "level";
		obj->level = value;
	}
	else if (!str_prefix(arg2, "weight"))
	{
		cDesc = "weight";
		obj->weight = value;
	}
	else if (!str_prefix(arg2, "cost"))
	{
		cDesc = "cost";
		obj->cost = value;
	}
	else if (!str_prefix(arg2, "timer"))
	{
		cDesc = "decay timer";
		obj->timer = value;
	}
	else
	{
		/*
		 * Generate usage message.
		 */
		do_oset(ch, "");
		return;
	}

	Cprintf(ch, "%s's %s changed to %d.\n\r", oName, cDesc, value);
}


void
do_rset(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	int value;
	char *cVal;

	smash_tilde(argument);
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	strcpy(arg3, argument);

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
	{
		Cprintf(ch, "Syntax:\n\r");
		Cprintf(ch, "  set room <location> <field> <value>\n\r");
		Cprintf(ch, "  Field being one of:\n\r");
		Cprintf(ch, "    flags sector\n\r");
		return;
	}

	if ((location = find_location(ch, arg1)) == NULL)
	{
		Cprintf(ch, "No such location.\n\r");
		return;
	}

	if (!is_room_owner(ch, location) && ch->in_room != location && room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR))
	{
		Cprintf(ch, "That room is private right now.\n\r");
		return;
	}

	/*
	 * Snarf the value.
	 */
	if (!is_number(arg3))
	{
		Cprintf(ch, "Value must be numeric.\n\r");
		return;
	}

	value = atoi(arg3);

	/*
	 * Set something.
	 */
	if (!str_prefix(arg2, "flags"))
	{
		cVal = "flags";
		location->room_flags = value;
	}
	else if (!str_prefix(arg2, "sector"))
	{
		cVal = "sector";
		location->sector_type = value;
	}
	else
	{
		/*
		 * Generate usage message.
		 */
		do_rset(ch, "");
		return;
	}

	Cprintf(ch, "The %s of room %d set to %d", cVal, location->vnum, value);
}


char *
connect_lookup(int Con)
{
	struct
	{
		int state;
		char *name;
	}
	states[] =
	{
		{
			CON_PLAYING, "playing"
		}
		,
		{
			CON_GET_NAME, "get_name"
		}
		,
		{
			CON_GET_OLD_PASSWORD, "get_old_pword"
		}
		,
		{
			CON_CONFIRM_NEW_NAME, "confirm_name"
		}
		,
		{
			CON_GET_NEW_PASSWORD, "get_new_pword"
		}
		,
		{
			CON_CONFIRM_NEW_PASSWORD, "confirm_new_pword"
		}
		,
		{
			CON_GET_NEW_RACE, "get_new_race"
		}
		,
		{
			CON_GET_NEW_SEX, "get_new_sex"
		}
		,
		{
			CON_GET_NEW_CLASS, "get_new_class"
		}
		,
		{
			CON_GET_ALIGNMENT, "get_alingnment"
		}
		,
		{
			CON_DEFAULT_CHOICE, "is_default_char"
		}
		,
		{
			CON_GEN_GROUPS, "get_groups"
		}
		,
		{
			CON_PICK_WEAPON, "pick_weapon"
		}
		,
		{
			CON_READ_IMOTD, "read_imotd"
		}
		,
		{
			CON_READ_MOTD, "read_motd"
		}
		,
		{
			CON_BREAK_CONNECT, "break_connect"
		}
		,
		{
			CON_CAN_CLAN, "get_clan_clan"
		}
		,
		{
			CON_GET_CONTINENT, "get_continent"
		}
		,
		{
			CON_NOTE_TO, "note_to"
		}
		,
		{
			CON_NOTE_SUBJECT, "note_subject"
		}
		,
		{
			CON_NOTE_EXPIRE, "note_expire"
		}
		,
		{
			CON_NOTE_TEXT, "note_text"
		}
		,
		{
			CON_NOTE_FINISH, "note_finish"
		}
		,
		{
			0, NULL
		}
	};

	int i;

	for (i = 0; states[i].name; i++)
		if (states[i].state == Con)
			return states[i].name;

	return "Unknown";
}


void
do_sockets(CHAR_DATA * ch, char *argument)
{
	char buf[2 * MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	char buf3[5 * MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	DESCRIPTOR_DATA *d;
	int count = 0;

	int numDescs = 0;

	buf[0] = '\0';

	argument = one_argument(argument, arg);

	if (str_prefix(arg, "host") && str_prefix(arg, "ip"))
        {
                Cprintf(ch, "You must select a socket method: host or ip\n\r");
		Cprintf(ch, "Syntax: sockets <host/ip> [name]\n\r");
                return;
        }
		
	one_argument(argument, arg2);
	
	/* get a count of the number of descriptors on.
	 */
	for (d = descriptor_list; d != NULL; d = d->next)
		numDescs++;

	{
		/*
		 *   socketData[][0][] will be the descriptor number
       *   socketData[][1][] will be the connect status
		 *   socketData[][2][] will be the user name, where applicable ("" otherwise)
		 *   socketData[][3][] will be the user.
		 *   socketData[][4][] will be the host
		 */
		char socketData[numDescs + 1][5][MAX_STRING_LENGTH];
		char temp[5][80];
		int sizes[] = {0, 0, 15, 0, 0};
		int pos;
		int i, j, k, m;

		/* 
		 * Let's seed it this way for length comparisons later:
		 */
		sprintf(socketData[0][0], "Desc");
		sprintf(socketData[0][1], "Connect Status");
		sprintf(socketData[0][2], "Player");
		sprintf(socketData[0][3], "User");
		sprintf(socketData[0][4], "Site");

		/* create the array that is to be sorted.
		 */
		for (d = descriptor_list; d != NULL; d = d->next)
		{
			if (d->character == NULL && arg2[0] == '\0')
			{
			/*
				count++;

				sprintf(socketData[count][0], "%d", d->descriptor);
				sprintf(socketData[count][1], "LOGIN");
				sprintf(socketData[count][2], "%s", "");
				sprintf(socketData[count][3], "%s", (!str_cmp(d->ident, "???")) ? "" : d->ident);
				sprintf(socketData[count][4], "%s", d->host);
 			*/
			}
			else if (d->character != NULL && can_see(ch, d->character) &&
					 (arg2[0] == '\0' || is_name(arg2, d->character->name)
					  || (d->original && is_name(arg2, d->original->name))))
			{
				count++;

				sprintf(socketData[count][0], "%d", d->descriptor);
				sprintf(socketData[count][1], "%s", connect_lookup(d->connected));
				sprintf(socketData[count][2], "%s", d->original ? d->original->name : d->character ? d->character->name : "(none)");
				sprintf(socketData[count][3], "%s", (!str_cmp(d->ident, "???")) ? "" : d->ident);
				if (!str_prefix(arg, "host"))
					sprintf(socketData[count][4], "%s", d->host);
				if (!str_prefix(arg, "ip"))
					sprintf(socketData[count][4], "%s", d->iphost);
			}
		}

		if (count == 0)
		{
			Cprintf(ch, "No one by that name is connected.\n\r");
			return;
		}

		/* now, sort the hosts and users.  first sort by user, and then by host, with empty values being put first.
		 */
		for (k = 3; k < 5; k++)
		{
			for (i = 1; i <= count; i++)
			{
				pos = i;

				/* minimize the number of strcpy's used.
			 	*/
				for (j = i; j <= count; j++)
					if (strcmp(socketData[pos][k], socketData[j][k]) > 0)
						pos = j;

				if (i != pos)
				{
					for (m = 0; m < 5; m++)
					{
						strcpy(temp[m], socketData[i][m]);
						strcpy(socketData[i][m], socketData[pos][m]);
						strcpy(socketData[pos][m], temp[m]);
					}
				}
			}
		}

		/*
		 * finally, store everything into buf
		 */

		for (i = 0; i <= count; i++)
		{
			for (j = 0; j < 5; j++)
			{
				int len = strlen(socketData[i][j]);

				if (sizes[j] < len)
				{
					sizes[j] = len;
				}
			}
		}

		*buf = '\0';
		for (i = 0; i <= count; i++)
		{
			char line[MAX_STRING_LENGTH];
			char mod[5][MAX_STRING_LENGTH];
			char formatLine[MAX_STRING_LENGTH];

			sprintf(mod[0], "%%-%d.%ds", sizes[0], sizes[0]);
			sprintf(mod[1], "%%-%d.%ds", sizes[1], sizes[1]);
			sprintf(mod[2], "%%-%d.%ds", sizes[2], sizes[2]);
			sprintf(mod[3], "%%%d.%ds", sizes[3], sizes[3]);
			sprintf(mod[4], "%%-%d.%ds", sizes[4], sizes[4]);

			/* Creating a format String. */
			if (i == 0)
			{
				sprintf(formatLine, "[%s] %s  %s\n\r", mod[0], mod[2], mod[4]);
			}
			//else if (strlen(socketData[i][3]) != 0)
			//{
			//	sprintf(formatLine, "[%s][%s] %s (%s@%s)\n\r", mod[0], mod[1], mod[2], mod[3], mod[4]);
			//}
			else
			{
				sprintf(formatLine, "[%s] %s (%s)\n\r", mod[0], mod[2], mod[4]);
			}

			sprintf(line, formatLine, socketData[i][0], socketData[i][2], socketData[i][4]);

			strcat(buf, line);
		}
	}

	sprintf(buf2, "%d user%s\n\r", count, count == 1 ? "" : "s");
	sprintf(buf3, "%s", buf);
	strcat(buf3, buf2);
	page_to_char(buf3, ch);
	return;
}



/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void
do_force(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg);

	if (arg[0] == '\0' || argument[0] == '\0')
	{
		Cprintf(ch, "Force whom to do what?\n\r");
		return;
	}

	one_argument(argument, arg2);

	if (!str_cmp(arg2, "delete") || !str_cmp(arg2, "remort") || !str_prefix(arg2, "mob"))
	{
		Cprintf(ch, "That will NOT be done.\n\r");
		return;
	}

	sprintf(buf, "$n forces you to '%s'.", argument);

	if (!str_cmp(arg, "all"))
	{
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 1)
		{
			Cprintf(ch, "Not at your level!\n\r");
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next)
		{
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch))
			{
				act(buf, ch, NULL, vch, TO_VICT, POS_RESTING);
				interpret(vch, argument);
			}
		}
	}
	else if (!str_cmp(arg, "players"))
	{
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 2)
		{
			Cprintf(ch, "Not at your level!\n\r");
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next)
		{
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch) && vch->level < LEVEL_HERO)
			{
				act(buf, ch, NULL, vch, TO_VICT, POS_RESTING);
				interpret(vch, argument);
			}
		}
	}
	else if (!str_cmp(arg, "gods"))
	{
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 2)
		{
			Cprintf(ch, "Not at your level!\n\r");
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next)
		{
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch) && vch->level >= LEVEL_HERO)
			{
				act(buf, ch, NULL, vch, TO_VICT, POS_RESTING);
				interpret(vch, argument);
			}
		}
	}
	else
	{
		CHAR_DATA *victim;

		if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
		{
			Cprintf(ch, "They aren't here.\n\r");
			return;
		}

		if (victim == ch)
		{
			Cprintf(ch, "Aye aye, right away!\n\r");
			return;
		}

		if (!is_room_owner(ch, victim->in_room) && ch->in_room != victim->in_room && room_is_private(victim->in_room) && !IS_TRUSTED(ch, IMPLEMENTOR))
		{
			Cprintf(ch, "That character is in a private room.\n\r");
			return;
		}

		if (get_trust(victim) >= get_trust(ch))
		{
			Cprintf(ch, "Do it yourself!\n\r");
			return;
		}

		if (!IS_NPC(victim) && get_trust(ch) < MAX_LEVEL - 3)
		{
			Cprintf(ch, "Not at your level!\n\r");
			return;
		}

		act(buf, ch, NULL, victim, TO_VICT, POS_RESTING);
		interpret(victim, argument);
	}

	Cprintf(ch, "Ok.\n\r");
	return;
}



/*
 * New routines by Dionysos.
 */
void
do_invis(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *vch;
	int level;
	char arg[MAX_STRING_LENGTH];

	/* RT code for taking a level argument */
	one_argument(argument, arg);

	if (arg[0] == '\0')			/* take the default path */
		if (ch->invis_level)
		{
			ch->invis_level = 0;
			act("$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(ch, "You slowly fade back into existence.\n\r");
		}
		else
		{
			ch->invis_level = get_trust(ch);
			act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(ch, "You slowly vanish into thin air.\n\r");
		}
	else
		/* do the level thing */
	{
		level = atoi(arg);
		if (level < 2 || level > get_trust(ch))
		{
			Cprintf(ch, "Invis level must be between 2 and your level.\n\r");
			return;
		}
		else
		{
			int fDir = ch->invis_level < level;

			ch->reply = NULL;
			if (ch->invis_level != 0)
			{
				Cprintf(ch, "You change your wizi level from %d to %d.\n\r", ch->invis_level, level);
			}

			for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
			{
				if (vch == ch)
					continue;

				if (can_see(vch, ch))
				{
					if (ch->invis_level == 0)
					{
						act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
						Cprintf(ch, "You slowly vanish into thin air.\n\r");
						ch->invis_level = level;
					}
					else
					{
						if (fDir)
						{
							Cprintf(vch, "%s slowly becomes more transparent.\n\r", capitalize((IS_NPC(ch)) ? ch->short_descr : ch->name));
						}
						else
						{
							Cprintf(vch, "%s slowly becomes less transparent.\n\r", capitalize((IS_NPC(ch)) ? ch->short_descr : ch->name));
						}
					}
				}
				else
				{
					ch->invis_level = level;
					if (can_see(vch, ch) && !fDir)
					{
						Cprintf(vch, "%s slowly becomes less transparent.\n\r", capitalize((IS_NPC(ch)) ? ch->short_descr : ch->name));
					}
					ch->invis_level = level;
				}
			}

			ch->invis_level = level;
		}
	}

	/* Now lets get rid of annoying reply-tos. :-) */
	for (vch = char_list; vch != NULL; vch = vch->next)
	{
		if (!IS_NPC(vch) && !can_see(vch, ch) && vch->reply == ch)
		{
			vch->reply = NULL;
		}
	}

	return;
}


void
do_incognito(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *vch;
	int level;
	char arg[MAX_STRING_LENGTH];

	/* RT code for taking a level argument */
	one_argument(argument, arg);

	if (arg[0] == '\0')			/* take the default path */
		if (ch->incog_level)
		{
			ch->incog_level = 0;
			act("$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(ch, "You are no longer cloaked.\n\r");
		}
		else
		{
			ch->incog_level = get_trust(ch);
			act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(ch, "You cloak your presence.\n\r");
		}
	else
		/* do the level thing */
	{
		level = atoi(arg);
		if (level < 2 || level > get_trust(ch))
		{
			Cprintf(ch, "Incog level must be between 2 and your level.\n\r");
			return;
		}
		else
		{
			int fDir = ch->incog_level < level;

			ch->reply = NULL;
			if (ch->incog_level != 0)
			{
				Cprintf(ch, "You change your inco level from %d to %d.\n\r", ch->incog_level, level);
			}
			ch->incog_level = level;

			for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
			{
				if (vch == ch)
					continue;

				if (can_see(vch, ch))
				{
					if (ch->incog_level == 0)
					{
						act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
						Cprintf(ch, "You cloak your presence.\n\r");
						ch->invis_level = level;
					}
					else
					{
						if (fDir)
						{
							Cprintf(vch, "%s cloaks their presence even further.\n\r", capitalize((IS_NPC(ch)) ? ch->short_descr : ch->name));
						}
						else
						{
							Cprintf(vch, "%s cloaks their presence to a lesser degree.\n\r", capitalize((IS_NPC(ch)) ? ch->short_descr : ch->name));
						}
					}
				}
				else
				{
					ch->incog_level = level;
					if (can_see(vch, ch) && !fDir)
					{
						Cprintf(vch, "%s cloaks their presence to a lesser degree.\n\r", capitalize((IS_NPC(ch)) ? ch->short_descr : ch->name));
					}
				}
			}

			ch->incog_level = level;
		}
	}

	/* Now lets get rid of annoying reply-tos. :-) */
	for (vch = char_list; vch != NULL; vch = vch->next)
	{
		if (!IS_NPC(vch) && !can_see(vch, ch) && vch->reply == ch)
		{
			vch->reply = NULL;
		}
	}

	return;
}


void
do_holylight(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_HOLYLIGHT))
	{
		REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
		Cprintf(ch, "Holy light mode off.\n\r");
	}
	else
	{
		SET_BIT(ch->act, PLR_HOLYLIGHT);
		Cprintf(ch, "Holy light mode on.\n\r");
	}

	return;
}


/* prefix command: it will put the string typed on each line typed */
void
do_prefi(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "You cannot abbreviate the prefix command.\r\n");
	return;
}

void
do_prefix(CHAR_DATA * ch, char *argument)
{
	if (argument[0] == '\0')
	{
		if (ch->prefix[0] == '\0')
		{
			Cprintf(ch, "You have no prefix to clear.\r\n");
			return;
		}

		Cprintf(ch, "Prefix removed.\r\n");
		free_string(ch->prefix);
		ch->prefix = str_dup("");
		return;
	}

	if (ch->prefix[0] != '\0')
	{
		Cprintf(ch, "Prefix changed to %s.\r\n", argument);
		free_string(ch->prefix);
	}
	else
	{
		Cprintf(ch, "Prefix set to %s.\r\n", argument);
	}

	ch->prefix = str_dup(argument);
}


void
do_fuck(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];

	one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		Cprintf(ch, "Fuck whom?\n\r");
		return;
	}

	if (is_number(arg))
	{
		int desc;

		desc = atoi(arg);
		for (d = descriptor_list; d != NULL; d = d->next)
		{
			if (d->descriptor == desc)
			{
				write_to_descriptor(d->descriptor, "\x01B[1;34m [5m", 0);
				Cprintf(ch, "Was that good for you too? :)\n\r");
				close_socket(d);
				return;
			}
		}
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim->desc == NULL)
	{
		act("$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (get_trust(victim) > get_trust(ch))
	{
		Cprintf(ch, "Use your hand. :-p\n\r");
		sprintf(buf, "$N tried to proposition %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
		return;
	}

	for (d = descriptor_list; d != NULL; d = d->next)
	{
		if (d == victim->desc)
		{
			write_to_descriptor(d->descriptor, "\x01B[1;34m [5m", 0);
			Cprintf(ch, "Was that good for you, too? :).\n\r");
			sprintf(buf, "$N fucks %s really good.", victim->name);
			wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
			close_socket(d);
			return;
		}
	}

	bug("Do_fuck: desc not found.", 0);
	Cprintf(ch, "Descriptor not found!\n\r");
	return;
}


void
do_lag(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;


	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Whom do you wish to sic the LAG beast upon?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPC's.\n\r");
		return;
	}

	if (get_trust(victim) >= get_trust(ch))
	{
		Cprintf(ch, "Sorry, lagging them would be a BAD idea! Fucknuts :P\n\r");
		return;
	}

	if (IS_SET(victim->comm, COMM_LAG))
	{
		REMOVE_BIT(victim->comm, COMM_LAG);
		Cprintf(victim, "The LAG monster has decided to prey upon some other fool.\n\r");
		Cprintf(ch, "LAG removed.\n\r");
		sprintf(buf, "$N distracts the LAG monster from %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}
	else
	{
		SET_BIT(victim->comm, COMM_LAG);
		Cprintf(victim, "You can't do ANYthing, fast...Welcome to LAGsville!\n\r");
		Cprintf(ch, "The LAG monster is now chasing fucknuts.\n\r");
		sprintf(buf, "$N sics the LAG beast on %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}

	save_char_obj(victim, FALSE);

	return;
}


void
do_no_quit(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "No quit whom?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPC's.\n\r");
		return;
	}

	if (get_trust(victim) >= get_trust(ch))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (IS_SET(victim->wiznet, NO_QUIT))
	{
		REMOVE_BIT(victim->wiznet, NO_QUIT);
		Cprintf(ch, "No quit removed.\n\r");
		sprintf(buf, "$N gives the quitting privilege back to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}
	else
	{
		SET_BIT(victim->wiznet, NO_QUIT);
		Cprintf(ch, "No quit set.\n\r");
		sprintf(buf, "$N makes %s unable to quit.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, 0, 0);
	}

	return;
}


void
do_whodesc(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	smash_tilde(argument);

	argument = one_argument(argument, arg);
	argument = macro_one_argument(argument, arg2);

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPCs.\n\r");
		return;
	}

	if (colorstrlen(arg2) > 18)
	{
		Cprintf(ch, "Sorry, the whodesc cannot be longer than 18 characters.\n\r");
		return;
	}

	if (arg2 == NULL || arg2[0] == '\0')
	{
		Cprintf(ch, "Sorry, 0 lenght strings are BAD <tm>, try again, with somethign this time.\n\r");
		Cprintf(ch, "To reset someone's whodesc, use 'whodesc <name> .'\n\r");
		return;
	}

	if (!strcmp(arg2, "."))
	{
		if (victim->short_descr != NULL)
			free_string(victim->short_descr);

		if (ch != victim)
			Cprintf(ch, "%s's whodesc reset.\n\r", victim->name);
		Cprintf(victim, "Your whodesc has been reset.\n\r");
	}
	else
	{
		if (victim->short_descr != NULL)
			free_string(victim->short_descr);

		victim->short_descr = str_dup(arg2);

		if (ch != victim)
			Cprintf(ch, "%s's whodesc changed to '%s'.\n\r", victim->name, victim->short_descr);
		Cprintf(victim, "Your whodesc changed to '%s'.\n\r", victim->short_descr);
	}
}

void
do_rptitle(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	smash_tilde(argument);

	if (argument == NULL || argument[0] == '\0')
	{
		Cprintf(ch, "Syntax: rptitle <name> \'<string>\'\n\r");
		return;
	}
	
	argument = one_argument(argument, arg);
	argument = macro_one_argument(argument, arg2);

	if ((victim = get_char_world(ch, arg, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "Not on NPCs.\n\r");
		return;
	}

	if (arg2 == NULL || arg2[0] == '\0')
	{
		Cprintf(ch, "Sorry, 0 lenght strings are BAD <tm>, try again, with somethign this time.\n\r");
		Cprintf(ch, "To reset someone's whodesc, use 'rptitle <name> .'\n\r");
		return;
	}

	/* Checks colorstrlen to avoid that annoying 20 character limit when it's not! -Tsongas */
	if (colorstrlen(arg2) > 20)
	{
		Cprintf(ch, "Sorry, the rptitles cannot be longer than 20 characters.\n\r");
		return;
	}

	if (victim->level > ch->level)
	{
		Cprintf(ch, "Sorry, BAD idea! Fucknuts :P\n\r");
		return;
	}

	if (!strcmp(arg2, "."))
	{
		if (victim->rptitle != NULL)
			free_string(victim->rptitle);

		victim->rptitle = &str_empty[0];

		if (ch != victim)
			Cprintf(ch, "%s's rptitle reset.\n\r", victim->name);
		Cprintf(victim, "Your rptitle has been reset.\n\r");
	}
	else
	{
		if (victim->rptitle != NULL)
			free_string(victim->rptitle);

		victim->rptitle = str_dup(arg2);

		if (ch != victim)
			Cprintf(ch, "%s's rptitle changed to '[%s]'.\n\r", victim->name, victim->rptitle);
		Cprintf(victim, "Your rptitle changed to '[%s]'.\n\r", victim->rptitle);
	}
}

void
do_join(CHAR_DATA * ch, char *argument)
{
	int clan, i;
	CHAR_DATA *vch;
	int show = TRUE;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Join which clan?\n\r");
		return;
	}

	if (IS_IMMORTAL(ch))
	{
		Cprintf(ch, "Immortals cannot join clans.\n\r");
		return;
	}

	if (ch->level < 10 && !ch->remort && !ch->reclass)
	{
		Cprintf(ch, "You are not old enough to clan.\n\r");
		return;
	}

	if (ch->clan == clan_lookup("outcast"))
	{
		Cprintf(ch, "Outcasts may not join clans. You may join a clan in %d hours.\n\r", ch->outcast_timer);
		return;
	}

	if (ch->clan >= MIN_PKILL_CLAN)
	{
		Cprintf(ch, "You cannot join a new clan while you owe you alleigance elsewhere!\n\r");
		return;
	}

	if ((clan = clan_lookup(argument)) == 0)
	{
		Cprintf(ch, "No such clan exists.\n\r");
		return;
	}
	 
	if (clan_table[clan].pkiller != clan_table[ch->clan].pkiller)
	{
	        Cprintf(ch, "You cannot join that kind of clan.\n\r");
		return;
	}

	for (i = MIN_PKILL_CLAN; i <= MAX_PKILL_CLAN; i++)
	{
		if (IS_SET(ch->wiznet, clan_wiznet_lookup(i)))
			show = FALSE;
		REMOVE_BIT(ch->wiznet, clan_wiznet_lookup(i));
	}

	SET_BIT(ch->wiznet, clan_wiznet_lookup(clan));

	Cprintf(ch, "You are now able to join clan %s.\n\r", capitalize(clan_table[clan].name));

	for(vch = char_list;vch != NULL;vch = vch->next) {
		if((vch->level > 51 || vch->trust > 51)
		&& vch->clan == clan
		&& show == TRUE) 
			Cprintf(vch, "{R%s is prepared to join your clan.\n\r{x", ch->name);
	}
}


void
do_accept(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int i;

	argument = one_argument(argument, arg1);

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Syntax: accept <player>\n\r");
		return;
	}
	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't playing.\n\r");
		return;
	}

	if (victim->level == 53 || victim->trust == 53)
	{
		Cprintf(ch, "They are a leader of a different clan, please let them pick another leader first.\n\r");
		return;
	}

	if (IS_SET(victim->wiznet, clan_wiznet_lookup(ch->clan)))
	{
		if (victim->level == 52 || victim->trust == 52)
		{
			demote_recruiter(victim);
		}

		if (in_own_hall(victim))
		{
			int dest = (victim->in_room->area->continent) ? 31004 : 3014;

			act("$n disappears in a flash of light.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			char_from_room(victim);
			char_to_room(victim, get_room_index(dest));
			act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(victim, "You're nolonger in your hall.\n\r");
		}

		Cprintf(ch, "They are now a member of clan %s.\n\r", capitalize(clan_table[ch->clan].name));
		Cprintf(victim, "You are now a member of clan %s.\n\r", capitalize(clan_table[ch->clan].name));
	}
	else
	{
		Cprintf(ch, "They do not wish to join your clan.\n\r");
		return;
	}

	victim->clan = ch->clan;
	victim->delegate = 0;
	for (i = 0; i < MAX_CLAN; i++)
	{
	    if (clan_table[i].independent)
	    	continue;

	    REMOVE_BIT(ch->wiznet, clan_table[i].join_constant);
	}
	if(victim->clan_rank)
		free_string(victim->clan_rank);
	victim->clan_rank = NULL;

}

/*
   void
   do_check_dns(CHAR_DATA * ch, char *arg)
   {
   if (DNSup == TRUE)
   {
   Cprintf(ch, "DNS CHECK IS NOW OFF\n\r");
   DNSup = FALSE;
   }
   else
   {
   Cprintf(ch, "DNS CHECK IS NOW ON\n\r");
   DNSup = TRUE;
   }
   return;
   }
 */

void
do_rename(CHAR_DATA * ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	char newName[MAX_STRING_LENGTH];
	char oldName[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA d;
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0') {
		Cprintf(ch, "Syntax: rename <target> <new name>\n\r");
		return;
	}


	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL) {
		Cprintf(ch, "They aren't playing.\n\r");
		return;
	}

	if(ch->level < 57 && victim->level > 10) {
		Cprintf(ch, "You may only rename people level 10 and under.\n\r");
		return;
	}
	
	if (!check_parse_name(arg2)) {
		Cprintf(ch, "The name is not valid.\n\r");
		return;
	}

	if (load_char_obj(&d, arg2)) {
		Cprintf(ch, "Someone already has that name.\n\r");
		return;
	}

    // First, rename the pfile
    sprintf(oldName, "%s%s", PLAYER_DIR, capitalize(victim->name));
    sprintf(newName, "%s%s", PLAYER_DIR, capitalize(arg2));
	

	if (rename(oldName, newName) == 0 || errno == ENOENT) {
		// next, rename the God file (if appropriate)
		if (IS_IMMORTAL(victim) || victim->level >= LEVEL_IMMORTAL) {
			sprintf(oldName, "%s%s", GOD_DIR, capitalize(victim->name));
			sprintf(newName, "%s%s", GOD_DIR, capitalize(arg2));
			
			rename(oldName, newName);
		}
		
	    Cprintf(ch, "%s renamed to %s.\n\r", victim->name, capitalize(arg2));
	    Cprintf(victim, "You have been renamed to %s.\n\r", capitalize(arg2));
		
		free_string(victim->name);
		victim->name = str_dup( capitalize(arg2) );
		save_char_obj(victim, FALSE);
	} else {
		Cprintf(ch, "The rename failed.  The errno: %d.\n\r", errno);
	}
}

void
do_double(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_STRING_LENGTH];
	char disp_msg[MAX_STRING_LENGTH];
	bool toggle;
	DESCRIPTOR_DATA *d;

	argument = one_argument(argument, arg1);

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Possible things to double are: xp, qp.\n\r");
		return;
	}

	if (argument[0] == '\0')
		toggle = 0;
	else
		toggle = 1;

	if (!str_cmp("xp", arg1))
	{
		double_xp_ticks = toggle;
		strcpy(disp_msg, "experience points");
	}
	else if (!str_cmp("qp", arg1))
	{
		double_qp_ticks = toggle;
		strcpy(disp_msg, "quest points");
	}
	else
	{
		Cprintf(ch, "Possible things to double are: xp, qp.\n\r");
		return;
	}

	if (!toggle)
	{
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->connected == CON_PLAYING)
			{
				Cprintf(d->character, "Double %s turned off :(\n\r", disp_msg);
			}
		}
	}
	else
	{
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->connected == CON_PLAYING)
			{
				Cprintf(d->character, "Now doubling %s.\n\r", disp_msg);
			}
		}
	}
}

void
do_swapleade(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "You must type the whole command to change leadership\n\r");
	return;
}

void
do_droprecruite(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "You must type the whole command to change leadership\n\r");
	return;
}

void
do_addrecruite(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "You must type the whole command to change leadership\n\r");
	return;
}

void
do_swapleader(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	argument = macro_one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: swapleader <password> <new leader>\n\r");
		return;
	}

	if (IS_NPC(ch))
	{
		Cprintf(ch, "You are not the leader of a clan!\n\r");
		return;
	}

	if (strcmp(crypt(arg1, ch->pcdata->pwd), ch->pcdata->pwd))
	{
		Cprintf(ch, "Wrong password.\n\r");
		return;
	}

	victim = get_char_world(ch, arg2, TRUE);
	if (victim == NULL)
	{
		Cprintf(ch, "You can't find them.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You already are the leader.\n\r");
		return;
	}

	if (victim->clan != ch->clan)
	{
		Cprintf(ch, "They aren't even in your clan!\n\r");
		return;
	}

	/* okay, so we have a good leader, and victim, lets do the tango */
	if (victim->level == 53)
	{
		/* this is bad */
		Cprintf(ch, "Your clan has _2_ leaders!!!! Report to IMMS please.\n\r");
		return;
	}

	demote_recruiter(victim);
	demote_leader(ch);
	advance_leader(victim);

	Cprintf(victim, "%s has given you the honor to lead your clan to new heights!\n\r", ch->name);
	Cprintf(ch, "You have passed on the honor of leading your clan to %s\n\r.", victim->name);

	return;
}

void
do_droprecruiter(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	argument = macro_one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: droprecruiter <password> <new leader>\n\r");
		return;
	}

	if (IS_NPC(ch))
	{
		Cprintf(ch, "Your not the leader of a clan!\n\r");
		return;
	}

	if (strcmp(crypt(arg1, ch->pcdata->pwd), ch->pcdata->pwd))
	{
		Cprintf(ch, "Wrong password.\n\r");
		return;
	}

	victim = get_char_world(ch, arg2, TRUE);
	if (victim == NULL)
	{
		Cprintf(ch, "You can't find them.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You already are the leader.\n\r");
		return;
	}

	if (victim->clan != ch->clan)
	{
		Cprintf(ch, "They aren't even in your clan!\n\r");
		return;
	}

	demote_recruiter(victim);

	Cprintf(ch, "%s has been stripped of their Recruiting powers.\n\r", victim->name);
	Cprintf(victim, "You are no longer fit to be a Recruiter of this clan!\n\r");

	return;
}

void
do_addrecruiter(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	argument = macro_one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: addrecruiter <password> <new leader>\n\r");
		return;
	}

	if (IS_NPC(ch))
	{
		Cprintf(ch, "Your not the leader of a clan!\n\r");
		return;
	}

	if (strcmp(crypt(arg1, ch->pcdata->pwd), ch->pcdata->pwd))
	{
		Cprintf(ch, "Wrong password.\n\r");
		return;
	}

	victim = get_char_world(ch, arg2, TRUE);
	if (victim == NULL)
	{
		Cprintf(ch, "You can't find them.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You already are the leader.\n\r");
		return;
	}

	if (victim->clan != ch->clan)
	{
		Cprintf(ch, "They aren't even in your clan!\n\r");
		return;
	}

	advance_recruiter(victim);

	Cprintf(ch, "You have advanced %s to a Recruiter.\n\r", victim->name);
	Cprintf(victim, "You have been made a Recruiter of this clan!\n\r");

	return;
}

void
demote_recruiter(CHAR_DATA * ch)
{
	int found;

	found = 0;
	if (str_cmp(ch->name, clan_leadership[ch->clan].recruiter1) == 0)
	{
		found = 1;
	}

	if (str_cmp(ch->name, clan_leadership[ch->clan].recruiter2) == 0)
	{
		found = 2;
	}

	if (found == 0)
	{
		return;
	}

	if (ch->level == 52)
	{
		ch->level--;
		ch->max_hit -= 20;
		ch->max_mana -= 20;
	}

	ch->trust = 0;
	ch->delegate = 0;

	if (found == 1)
	{
		free_string(clan_leadership[ch->clan].recruiter1);
		clan_leadership[ch->clan].recruiter1 = NULL;
	}
	else
	{
		free_string(clan_leadership[ch->clan].recruiter2);
		clan_leadership[ch->clan].recruiter2 = NULL;
	}

	save_char_obj(ch, FALSE);

	write_clanleaders();
}

void
demote_leader(CHAR_DATA * ch)
{
	if (str_cmp(ch->name, clan_leadership[ch->clan].leader))
	{
		return;
	}

	if (ch->level == 53)
	{
		ch->level -= 2;
		ch->max_hit -= 40;
		ch->max_mana -= 40;
	}

	ch->trust = 0;
	ch->delegate = 0;

	free_string(clan_leadership[ch->clan].leader);
	clan_leadership[ch->clan].leader = NULL;

	save_char_obj(ch, FALSE);

	write_clanleaders();
}

void
advance_recruiter(CHAR_DATA * ch)
{
	if (clan_leadership[ch->clan].recruiter1 == NULL)
	{
		clan_leadership[ch->clan].recruiter1 = str_dup(ch->name);
	}
	else if (clan_leadership[ch->clan].recruiter2 == NULL)
	{
		clan_leadership[ch->clan].recruiter2 = str_dup(ch->name);
	}
	else
	{
		return;
	}

	if (ch->level == 51)
	{
		ch->level++;
		ch->max_hit += 20;
		ch->max_mana += 20;
	}
	else
	{
		ch->trust = 52;
	}

	save_char_obj(ch, FALSE);

	write_clanleaders();
}

void
advance_leader(CHAR_DATA * ch)
{
	if (clan_leadership[ch->clan].leader == NULL)
	{
		clan_leadership[ch->clan].leader = str_dup(ch->name);
	}
	else
	{
		return;
	}

	if (ch->level == 52)
	{
		ch->level++;
		ch->max_hit += 20;
		ch->max_mana += 20;
	}
	else if (ch->level == 51)
	{
		ch->level += 2;
		ch->max_hit += 40;
		ch->max_mana += 40;
	}
	else
	{
		ch->trust = 53;
	}

	save_char_obj(ch, FALSE);

	write_clanleaders();
}

void
do_loner(CHAR_DATA * ch, char *argument) {
	CHAR_DATA *victim;
	int cont;
	int clanreport = 0;
	int time_outcast = 50;
    char arg1[MAX_INPUT_LENGTH];

    int trustedLevel = UMAX(ch->level, ch->trust);
    int minpk = MIN_PKILL_CLAN;
    one_argument(argument, arg1);

    // NC's can't loner
    if (ch->clan <= 1) {
        Cprintf(ch, "You have to be in a clan first.\n\r");
        return;
    }

    if (trustedLevel < 52) {
        // Prevent people from doing "loner so-and-so" and lonering themselves
        if (arg1[0] != '\0') {
            Cprintf(ch, "You cannot loner others.  To loner yourself, type 'loner' without a name.\n\r");
            return;
        }
        
        if (victim->clan == clan_lookup("outcast") ||
            victim->clan == clan_lookup("loner")) {
            Cprintf(ch, "You are already clanless.\n\r");
            return;
        } else {
            time_outcast = 30;
            victim = ch;
        }
    } else {
		if (arg1[0] == '\0') {
			Cprintf(ch, "Syntax: loner who?\n\r");
			return;
		}
		
		victim = get_char_world(ch, arg1, TRUE);

		if (victim == NULL) {
			Cprintf(ch, "They aren't playing.\n\r");
			return;
		}

		if (get_trust(ch) < 53 && ch->delegate < victim->level) {
			Cprintf(ch, "You can loner people of level %d or below only.\n\r", ch->delegate);
			return;
		}

		if ((victim->level < 53) && (get_trust(victim) == 53)) {
			Cprintf(ch, "You want the clan that bad? Mutiny for it! :P\n\r");
			return;
		}

		if (ch == victim && (ch->level == 53 || ch->trust == 53)) {
			Cprintf(ch, "You can't loner yourself, you're the leader!\n\r");
			return;
		}

	if (ch->clan != victim->clan)
	{
		Cprintf(ch, "Try someone in your own clan!.\n\r");
		return;
	}
	}

	if (clan_table[victim->clan].independent)
	{
	    Cprintf(ch, "You are not in a clan.\n\r");
	    return;
	}

	demote_recruiter(victim);

	if (in_own_hall(victim)) {
		act("$n disappears in a flash of light.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		cont = victim->in_room->area->continent;
		char_from_room(victim);
		char_to_room(victim, get_room_index(cont ? 31004 : 3014));
		act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(victim, "You're no longer in your hall.\n\r");
	}

	if (ch == victim) {
		Cprintf(ch, "You are now an outcast.\n\r");
	} else {
		Cprintf(ch, "They are now clanless.\n\r");
		Cprintf(victim, "You are *SO* banished and outcast.\n\r");
	}

	/* Check the clanreport to see if they're on there. If so, ditch them. */
	if ( !strcmp(victim->name, clan_report[victim->clan - minpk].best_pkill) ) {
		strcpy(clan_report[victim->clan - minpk].best_pkill, "None");
		clan_report[victim->clan - minpk].player_pkills = 0;
		clanreport = 1;
	}
	
	if ( !strcmp(victim->name, clan_report[victim->clan - minpk].worst_pkilled) ) {
		strcpy(clan_report[victim->clan - minpk].worst_pkilled, "None");
		clan_report[victim->clan - minpk].player_pkilled = 0;
		clanreport = 1;
	}
	
	if (clanreport == 1) {
		Cprintf(victim, "Your standings on the clanreport have been removed.\n\r");
		save_clan_report();
	}

        if (clan_table[victim->clan].pkiller)
	{
	    victim->clan = clan_lookup("outcast");
	    victim->outcast_timer = time_outcast;
	}
	else
	     victim->clan = 0;
	
	if (victim->clan_rank) {
		free_string(victim->clan_rank);
	}
	
	victim->clan_rank = NULL;
	victim->trust = 0;
	victim->delegate = 0;
}

void
do_resetpassword(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	char *pwdnew;

	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	victim = get_char_world(ch, arg1, TRUE);

	if ((ch->pcdata->pwd != NULL) &&
		(arg1[0] == '\0' || arg2[0] == '\0'))
	{
		Cprintf(ch, "Syntax: password <char> <new>.\n\r");
		return;
	}

	if (victim == NULL)
	{
		Cprintf(ch, "That person isn't here, they have to be here to reset pwd's.\n\r");
		return;
	}
	if (IS_NPC(victim))
	{
		Cprintf(ch, "You cannot change the password of NPCs!\n\r");
		return;
	}

	if ((victim->level > LEVEL_IMMORTAL) || (get_trust(victim) > LEVEL_IMMORTAL))
	{
		Cprintf(ch, "You can't change the password of that player.\n\r");
		return;
	}

	if (strlen(arg2) < 5)
	{
		Cprintf(ch, "New password must be at least five characters long.\n\r");
		return;
	}

	pwdnew = crypt(arg2, victim->name);
	free_string(victim->pcdata->pwd);
	victim->pcdata->pwd = str_dup(pwdnew);
	save_char_obj(victim, FALSE);
	Cprintf(ch, "Ok.\n\r");
	Cprintf(victim, "Your password has been changed to %s.", arg2);
	return;
}

/** Function: do_pload
  * Descr   : Loads a player object into the mud, bringing them (and their
  *           pet) to you for easy modification.  Player must not be connected.
  *           Note: be sure to send them back when your done with them.
  * Returns : (void)
  * Syntax  : pload (who)
  * Written : v1.0 12/97
  * Author  : Gary McNickle <gary@dharvest.com>
  */

/* Please feel free to disembowel the above individual for attempting
   to reference an out of scope variable in our char_list.

   -- StarX, Sept 2001.
*/
void
do_pload(CHAR_DATA * ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	bool isChar = FALSE;
	char name[MAX_INPUT_LENGTH];

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Load who?\n\r");
		return;
	}

	argument[0] = UPPER(argument[0]);
	argument = one_argument(argument, name);

	/* Dont want to load a second copy of a player who's allready online! */
	if (get_char_world(ch, name, TRUE) != NULL)
	{
		Cprintf(ch, "That person is already connected!\n\r");
		return;
	}

	d = new_descriptor();
	isChar = load_char_obj(d, name);	/* char pfile exists? */

	if (!isChar)
	{
		Cprintf(ch, "Load Who? Are you sure? I cant seem to find them.\n\r");
		return;
	}

	d->character->desc = NULL;
	d->character->next = char_list;
	char_list = d->character;
	d->connected = CON_PLAYING;
	reset_char(d->character);

	/* bring player to imm */
	if (d->character->in_room != NULL)
	{
		char_to_room(d->character, ch->in_room);		/* put in room imm is in */
	}

	Cprintf(ch, "You pull %s from the pattern!\n\r", d->character->name);
	act("$n has pulled $N from the pattern!", ch, NULL, d->character, TO_ROOM, POS_RESTING);

	if (d->character->pet != NULL)
	{
		char_to_room(d->character->pet, d->character->in_room);
		act("$n has entered the game.", d->character->pet, NULL, NULL, TO_ROOM, POS_RESTING);
	}
}

/** Function: do_punload
  * Descr   : Returns a player, previously 'ploaded' back to the void from
  *           whence they came.  This does not work if the player is actually 
  *           connected.
  * Returns : (void)
  * Syntax  : punload (who)
  * Written : v1.0 12/97
  * Author  : Gary McNickle <gary@dharvest.com>
  */
void
do_punload(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	char who[MAX_INPUT_LENGTH];

	argument = one_argument(argument, who);

	if ((victim = get_char_world(ch, who, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	/** Person is legitametly logged on... was not ploaded.
	*/
	if (victim->desc != NULL)
	{
		Cprintf(ch, "I dont think that would be a good idea...\n\r");
		return;
	}

	if (victim->was_in_room != NULL)	/* return player and pet to orig room */
	{
		char_to_room(victim, victim->was_in_room);
		if (victim->pet != NULL)
			char_to_room(victim->pet, victim->was_in_room);
	}

	save_char_obj(victim, TRUE);
	do_quit(victim, "");

	act("$n has released $N back to the Pattern.", ch, NULL, victim, TO_ROOM, POS_RESTING);
}

char* strip(char* s)
{
	char* p;

	for(p = s; p; p++)
	{
		if(*p == ',')
			*p = '.';
	}

	return s;
}

void
do_mobdump(CHAR_DATA* ch, char* argument)
{
	MOB_INDEX_DATA* mob;
	char buf[2048];
	FILE *fp;
	int vnum;

	fp = fopen("mobdump.txt", "w");
	fprintf(fp, "Vnum,Name,Short,Long,Race,Level,Hp,Affs,Off,Imm,Res,Vuln,Area\n");

	for (vnum = 0; vnum < 40000; vnum++)
	{
		mob = get_mob_index(vnum);

		if (mob != NULL)
		{
			sprintf(buf, "%dd%d+%d",
				mob->hit[0], mob->hit[1], mob->hit[2] );

			fprintf(fp, "%d,%s,%s,%s,%s,%d,%s,%s,%s,%s,%s,%s,%s\n",
				mob->vnum,
				strip(mob->player_name),
				strip(mob->short_descr),
				strip(mob->long_descr),
				race_table[mob->race].name,
				mob->level,
				buf,
				affect_bit_name(mob->affected_by),
				off_bit_name(mob->off_flags),
				imm_bit_name(mob->imm_flags),
				imm_bit_name(mob->imm_flags),
				imm_bit_name(mob->imm_flags),
				strip(mob->area->name));
		}
	}

	fclose(fp);
}

void workshop_list(CHAR_DATA *ch, char *argument) {
        CRAFTED_ITEM *item;
        CRAFTED_COMPONENT *part;
	OBJ_INDEX_DATA *pObjIndex, *partIndex;
	char arg[MAX_INPUT_LENGTH];
	BUFFER *buffer;
	int found = FALSE;
	char buf[MAX_STRING_LENGTH];

	argument = one_argument(argument, arg);
	
        if(crafted_items == NULL) {
                Cprintf(ch, "There are currently no crafted items loaded.\n\r");
                return;
        }

	buffer = new_buf();

        Cprintf(ch, "Crafted Item Formulae:\n\r");

        for(item = crafted_items; item != NULL; item = item->next_ci) {
		if ((pObjIndex = get_obj_index(item->ci_vnum)) == NULL) {
                        bug("Bad vnum for crafted item %d", item->ci_vnum);
                        return;
                }

                if(arg[0] == '\0'
		|| !str_cmp(arg, "all")
		|| is_name(arg, pObjIndex->name)) {
			
			found = TRUE;
                	sprintf(buf, "{W(%d) %d -{x %s (%s){x requirements:\n\r", item->index, item->ci_vnum, pObjIndex->short_descr, pObjIndex->name);
			add_buf(buffer, buf);

                	for(part = item->parts; part != NULL; part = part->next_part) {
				if ((partIndex = get_obj_index(part->part_vnum)) == NULL) {
                        		bug("Bad vnum for crafted item component %d", item->ci_vnum);
                        		return;
                		}
	                        sprintf(buf, "    {g%d of %d -{x %s (%s)\n\r", part->qty, part->part_vnum, partIndex->short_descr, partIndex->name);
				add_buf(buffer, buf);
			}                 
                }             
        }


	if (!found)
        	Cprintf(ch, "No crafted items found.\n\r");
	else
        	page_to_char(buf_string(buffer), ch);

	free_buf(buffer);
	return;
}                     


void load_citems() {
        FILE *fptr;
	CRAFTED_ITEM *item = NULL, *last_item = NULL;
	CRAFTED_COMPONENT *part = NULL, *last_part = NULL;
	char letter;
	int done = FALSE;
	int nextIndex = 0;
	
        fptr = fopen("crafted_list.txt", "r");

	if(fptr == NULL) {
                crafted_items = NULL;
                return;
        }       

	// Empty database.
	// Potential small memory leak, but won't be in release version.
	crafted_items = NULL;

	while(TRUE) {
		letter = fread_letter(fptr);	
		if(letter == '\0')
			break;

		switch(letter) {
			case 'I':
				item = (CRAFTED_ITEM*)alloc_perm(sizeof(*item));  
				// Attach the head of the list
				if(last_item == NULL)
					crafted_items = item;
				if(last_item != NULL)
					last_item->next_ci = item;
				item->index = nextIndex++;
				item->ci_vnum = fread_number(fptr);
				item->parts = NULL;
				item->next_ci = NULL;
				part = item->parts;
				break;			
			case 'P':
				part = (CRAFTED_COMPONENT*)alloc_perm(sizeof(*item->parts));
				if(last_part == NULL)
					item->parts = part;
				if(last_part != NULL)
					last_part->next_part = part;
				part->part_vnum = fread_number(fptr);
				part->qty = fread_number(fptr);
				part->next_part = NULL;
				last_part = part;
				part = part->next_part;
				break;
			case '~':
				last_item = item;
				last_part = NULL;
				item = item->next_ci;
				break;
			case '#':
				done = TRUE;
				break;	
			default:
				break;
		}
		if(done)
			break;
	}

	fclose(fptr);

	// Don't forget to attach this list somewhere permanent!
	return;
}

void save_citems() {
        FILE *fptr;
	CRAFTED_ITEM *ci;
	CRAFTED_COMPONENT *part;

        fptr = fopen("crafted_list.txt", "w");

	for(ci = crafted_items; ci != NULL; ci = ci->next_ci) {
		// The completed item vnum
		fprintf(fptr, "I%d\n", ci->ci_vnum);
		for(part = ci->parts; part != NULL; part = part->next_part) {
			// All the parts.
			fprintf(fptr, "P%d %d\n", part->part_vnum, part->qty);
		}
		fprintf(fptr, "~\n");
	}		
	fprintf(fptr, "#END\n");
        fclose(fptr);
}

void workshop_new(CHAR_DATA *ch, char *argument) {
	CRAFTED_ITEM *tail;
	char obj_vnum[MAX_INPUT_LENGTH];
	OBJ_INDEX_DATA *pObjIndex;
	int lastIndex = 0;

	// Get the tail of the ci list
	tail = crafted_items;

	while(TRUE) {
		if(tail == NULL)
			break;
		if(tail->next_ci == NULL)
			break;
		tail = tail->next_ci;
		
	}

	// Make sure they are adding a valid item.
	argument = one_argument(argument, obj_vnum);

	if(!is_number(obj_vnum)) {
		Cprintf(ch, "Workshop: Syntax is 'workshop new <crafted item vnum>'.\n\r");
		return;
	}

	if ((pObjIndex = get_obj_index(atoi(obj_vnum))) == NULL) {
		Cprintf(ch, "Workshop_new: No obj with that vnum.\n\r");
		return;
	}

	if (!IS_SET(pObjIndex->wear_flags, ITEM_CRAFTED)) {
		Cprintf(ch, "Workshop_new: That object isn't a crafted item.\n\r");
		return;
	}

	// Append it to the list with an empty "parts" record.
	if(tail != NULL) {
		lastIndex = tail->index;	
		tail->next_ci = (CRAFTED_ITEM*)alloc_perm(sizeof(*tail->next_ci));
		tail = tail->next_ci;
	}
	// It is the first item in the list.
	if(tail == NULL) {
		tail = (CRAFTED_ITEM*)alloc_perm(sizeof(*tail));
	}
	tail->index = lastIndex + 1;
	tail->ci_vnum = atoi(obj_vnum);
	tail->parts = NULL;
	tail->next_ci = NULL;		

	if(crafted_items == NULL)
		crafted_items = tail;
	
	Cprintf(ch, "New crafted item added to database.\n\r");
	return;
}

void workshop_part(CHAR_DATA *ch, char *argument) {
	CRAFTED_ITEM *tail;
	CRAFTED_COMPONENT *part;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	int seekIndex, pvnum, qty;
	OBJ_INDEX_DATA *pObjIndex;

	// Make sure they are adding a valid item.
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	if(arg1[0] == '\0'
	|| arg2[0] == '\0'
	|| arg3[0] == '\0'
	|| !is_number(arg1)
	|| !is_number(arg2)
	|| !is_number(arg3)) {
		Cprintf(ch, "Workshop: Syntax is 'workshop part <crafted item vnum> <part vnum> <quantity>'.\n\r");
		return;
	}

	seekIndex = atoi(arg1);
	pvnum = atoi(arg2);
	qty = atoi(arg3);

	if(qty < 1) {
		Cprintf(ch, "Workshop_part: Must use workshop delete to remove components.\n\r");
		return;
	}

	if ((pObjIndex = get_obj_index(pvnum)) == NULL) {
		Cprintf(ch, "Workshop_part: No obj with that vnum.\n\r");
		return;
	}	

	if (!IS_SET(pObjIndex->wear_flags, ITEM_INCOMPLETE)) {
		Cprintf(ch, "Workshop_part: Components must have the 'incomplete' wear flag.\n\r");
		return;
	}

	tail = crafted_items;
	if(tail == NULL) {
		Cprintf(ch, "Workshop_part: No crafted items in database to modify.\n\r");
		return;
	}

	// Check to see if this is a modification first
	while(TRUE) {		
		if(tail->index == seekIndex) {
			part = tail->parts;
			if(part == NULL)
				break;
			while(TRUE) {
				if(part->part_vnum == pvnum) {
					part->qty = qty;
					Cprintf(ch, "Crafted item component modified.\n\r");
					save_citems();
					return;
				}
				part = part->next_part;
				if(part == NULL)
					break;
			}	
		}		
		tail = tail->next_ci;
		if(tail == NULL)
			break;
	}

	// Add it to the part list since it is not there.
	tail = crafted_items;
	while(TRUE) {		
		if(tail->index == seekIndex) {
			part = tail->parts;
			if (part != NULL) {
				while(part->next_part != NULL) {
					part = part->next_part;
				}
				part->next_part = (CRAFTED_COMPONENT*)alloc_perm(sizeof(*part));
				part = part->next_part;
			}
			else {
				part = (CRAFTED_COMPONENT*)alloc_perm(sizeof(*part));
				tail->parts = part;
			}
			part->part_vnum = pvnum;
			part->qty = qty;
			part->next_part = NULL;
			Cprintf(ch, "Crafted item component added.\n\r");
			save_citems();
			return;
		}		
		tail = tail->next_ci;
		if(tail == NULL)
			break;
	}

	Cprintf(ch, "Error in searching crafted items.\n\r");
	return;
}

void reorder_crafted_items() {
	int nextIndex = 0;
	CRAFTED_ITEM *tail = NULL;

	// Re-order the indices
        tail = crafted_items;
	if(tail == NULL)
		return;
        nextIndex = 0;
        while(TRUE) {
                tail->index = nextIndex++;
                tail = tail->next_ci;
                if(tail == NULL)
                        break;
        }
        return;                  
}

void workshop_delete(CHAR_DATA *ch, char *argument) {
	CRAFTED_ITEM *tail, *last_tail = NULL;
	char arg1[MAX_INPUT_LENGTH];
	int seekIndex;

	// Make sure they are adding a valid item.
	argument = one_argument(argument, arg1);
	
	if(arg1[0] == '\0'
	|| !is_number(arg1)) {
		Cprintf(ch, "Workshop: Syntax is 'workshop delete <crafted item vnum>'.\n\r");
		return;
	}

	seekIndex = atoi(arg1);

	tail = crafted_items;
	// Special case: Empty database.
	if(tail == NULL) {
		Cprintf(ch, "Workshop_delete: No crafted items in database to delete.\n\r");
		return;
	}
	// Special case: Last item in database.
	if(tail->next_ci == NULL) {
		crafted_items = NULL;
		Cprintf(ch, "Crafted item removed from game.\n\r");
		reorder_crafted_items(); 
		return;
	}
	// Special case: Head of list removed
	if(tail->index == seekIndex) {
		crafted_items = tail->next_ci;
		Cprintf(ch, "Crafted item removed from game.\n\r");
		reorder_crafted_items(); 
		return;
	}

	// General case
	last_tail = crafted_items;
	tail = crafted_items->next_ci;
	while(TRUE) {		
		if(tail->index == seekIndex) {
			last_tail->next_ci = tail->next_ci;
			Cprintf(ch, "Crafted item removed from game.\n\r");
			save_citems();
			reorder_crafted_items(); 
			return;
		}	
		last_tail = tail;	
		tail = tail->next_ci;
		if(tail == NULL)
			break;
	}

	reorder_crafted_items();
	return;
}

void 
do_workshop(CHAR_DATA *ch, char *argument) {
	char command[MAX_STRING_LENGTH];
	
	if(IS_NPC(ch)) {
		Cprintf(ch, "Players only!\n\r");
		return;
	}

	argument = one_argument(argument, command);

	if(!str_prefix(command, "list")) {
		workshop_list(ch, argument);
	}
	else if(!str_prefix(command, "new")) {
		workshop_new(ch, argument);
	}
	else if(!str_prefix(command, "part")) {
		workshop_part(ch, argument);
	}
	else if(!str_prefix(command, "delete")) {
		workshop_delete(ch, argument);
	}
	else {
		Cprintf(ch, "Workshop: Unknown command.\n\r");
		return;	
	}

	return;
}

// Receive an exp award for crafting.
void advance_craft(CHAR_DATA *gch, OBJ_DATA *obj)
{
	int weapon_count = 0;
	OBJ_DATA *wield = NULL;
	int xp = 0;
	CHAR_DATA *vch;
	int vassalpts;
	int weapon_xp;

	if(IS_NPC(gch))
		return;

	// Exp award based on your exp per level.
	xp = exp_per_level(gch, gch->pcdata->points) / 25;
	xp = xp + number_range(1, 100);

	if(xp == 0) {
		return;
	}

	// Intelligent weapons get some xp.
        // Watch out for dual wield.
        weapon_count = 0;
        if((wield = get_eq_char(gch, WEAR_WIELD)) != NULL
        && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT))
        	weapon_count++;
	if((wield = get_eq_char(gch, WEAR_DUAL)) != NULL
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
        gain_exp(gch, xp);  

	return;
}          


void do_craft(CHAR_DATA *ch, char *argument) 
{
	CRAFTED_ITEM *item;
	CRAFTED_COMPONENT *part;
	OBJ_INDEX_DATA *pObjIndex, *partIndex;
	char arg[MAX_INPUT_LENGTH];
	int found = FALSE;
	int i = 0;
	int chance = 0;
	int missingItems = FALSE;
	OBJ_DATA *obj;

	// Find out if they gave a valid name.
	argument = one_argument(argument, arg);

	if(get_skill(ch, gsn_craft_item) < 1) {
		Cprintf(ch, "You aren't sure how to craft items yet.\n\r");
		return;
	}             
 	if(ch->in_room == NULL)
		return;

	if(!IS_SET(ch->in_room->room_flags, ROOM_WORKSHOP)) {
    		Cprintf(ch, "Looking around, you realize you can't craft items without a workshop.\n\r");
		return;
	}

	if(ch->mana < 100) {
			Cprintf(ch, "Your magical powers are insufficient to try.\n\r");
			return;
	}

	if(ch->craft_timer < 0) {
		Cprintf(ch, "You're already working on one item, wait until it's done.\n\r");
		return;
	}

	if(ch->craft_timer > 0) {
		Cprintf(ch, "You can try to craft again in %d hours.\n\r", ch->craft_timer);
		return;	
	}

	item = crafted_items;
	while(TRUE) {
		// Load each object once to check for name
		if ((pObjIndex = get_obj_index(item->ci_vnum)) == NULL) {
				bug("Bad vnum for crafted item %d", item->ci_vnum);
				return;
		}
		// Check to find out if this is the recipe they want to create.
  		if (is_name(pObjIndex->name, arg)) {
			// They got the name, do they have they items?	
			part = item->parts;
			found = TRUE;
			missingItems = FALSE;
			while(TRUE) {
				if ((partIndex = get_obj_index(part->part_vnum)) == NULL) {
						bug("Bad vnum for crafted item component %d", item->ci_vnum);
						return;
				}

				if (get_obj_qty(ch, partIndex) < part->qty) {
					missingItems = TRUE;
					break;
				}

				part = part->next_part;
				if(part == NULL)
					break;
			}          
		}

		if(found && !missingItems)
			break;
		item = item->next_ci;
		if(item == NULL)
			break;
	}

	if(!found) {
        	Cprintf(ch, "You try and craft an item, but you have have no idea what you are doing.\n\r");
        	check_improve(ch, gsn_craft_item, FALSE, 1);
       		ch->mana -= 50;
        	return;
    	}
		      
	if(missingItems) {
		Cprintf(ch, "You have the right idea, but you are missing something.\n\r");
        	check_improve(ch, gsn_craft_item, FALSE, 1);
        	ch->mana -= 50;
        	return;
	}                                

	if(pObjIndex->level > ch->level) {
        	Cprintf(ch, "This item is beyond your ability to craft.\n\r");
        	return;
   	}

    if(item->parts == NULL) {
        Cprintf(ch, "Crafted item error. Bug logged.\n\r");
        bug("No components for crafted item %d", item->ci_vnum);
        return;
    }

    ch->mana -= 100;

    // Failure has a big price.
    chance = get_skill(ch, gsn_craft_item);
    chance += get_curr_stat(ch, STAT_WIS);
    chance += get_curr_stat(ch, STAT_INT);
    chance -= (pObjIndex->level + 25);     
    
    if(number_percent() > chance) {
        	Cprintf(ch, "You try and craft an item, but your skills fail you!\n\r");
		Cprintf(ch, "You will have to try again in 60 hours.\n\r");
		ch->craft_timer = 60;
        	return;
    }

    Cprintf(ch, "You begin to craft %s. Make sure your work is not interrupted.\n\r", pObjIndex->short_descr);

    ch->craft_timer = -5;
    ch->craft_target = pObjIndex->vnum;

    // Deduct the components from the character
    part = item->parts;
    while(TRUE) {
    	partIndex = get_obj_index(part->part_vnum);

    	for(i = 0; i < part->qty; i++) {
            	for(obj = ch->carrying; obj != NULL; obj = obj->next_content) {
                    if(obj->wear_loc == WEAR_NONE
                    && obj->pIndexData->vnum == partIndex->vnum) {
                            extract_obj(obj);
                            break;
                }
        }
    }
    part = part->next_part;
    if(part == NULL)
            break;
}

    return;
}                     

void 
do_vote(CHAR_DATA *ch, char* argument)
{
	const int min_level = 54;
	char command[MAX_STRING_LENGTH];

	argument = one_argument(argument, command);

	if(IS_NPC(ch)) {
		Cprintf(ch, "Players only!\n\r");
		return;
	}
	
	if(!str_cmp(command, "info"))
		vote_info(ch);
	else if(!str_cmp(command, "set"))
		vote_set(ch, argument);
	else if(!str_cmp(command, "1")
	|| !str_cmp(command, "2")
	|| !str_cmp(command, "3")
	|| !str_cmp(command, "4")
	|| !str_cmp(command, "5"))
		vote_voting(ch, atoi(command)-1); 
	else if(!str_cmp(command, "create"))
		vote_create(ch);
	else if(!str_cmp(command, "open"))
		vote_open(ch);
	else if(!str_cmp(command, "close"))
		vote_close(ch);
	else if(!str_cmp(command, "delete"))
		vote_delete(ch);
	else if(!str_prefix(command, "results"))
                vote_results(ch);        
	else {
		Cprintf(ch, "Vote commands are: ");
		if(ch->level < min_level) {
			Cprintf(ch, "info 1 2 3 4 5\n\r");
			return;	
		}
		else {
			Cprintf(ch, "create set open close delete info results 1 2 3 4 5\n\r");
			return;
		}
	}
	
	return;
}

void 
vote_info(CHAR_DATA *ch)
{
	int i, clan;
	bool valid;

	if(vote.creator[0] == '\0')
	{
		Cprintf(ch, "There is no current vote item.\n\r");
		return;
	}
	if(vote.active == TRUE)
	{
		Cprintf(ch, "{WThe voting polls are open! Please only vote once.{x\n\r");
		Cprintf(ch, "{B================================================={x\n\r");
		Cprintf(ch, "%s\n\r\n\r", vote.question);
		for(i=0;i<5;i++) {
			if(vote.result[i].description[0] != '\0')
				Cprintf(ch, "%d) %s\n\r", i+1, vote.result[i].description);
		}
		Cprintf(ch, "\n\r{B================================================={x\n\r");
		Cprintf(ch, "Type 'vote #' to lock in your vote.\n\r");
		return;
	}
	else if(vote.active == FALSE && !str_cmp(ch->name, vote.creator)) {
		Cprintf(ch, "The following voting item is being edited by you.\n\r\n\r");
		Cprintf(ch, "Topic:\t%s\n\r", vote.question[0] == '\0' ? "Not specified" : vote.question);
		for(i=0;i<5;i++) {
			Cprintf(ch, "%d)\t%s\n\r", i+1, vote.result[i].description[0] == '\0' ? "Not specified" : vote.result[i].description);
		}
		Cprintf(ch, "Must be level %d to vote on this topic.\n\r", vote.vote_level);
		valid = FALSE;
		Cprintf(ch, "Allowed clans:");
		for (clan = 0; clan < MAX_CLAN; clan++)
		{
			if (vote.restriction[clan]) {
				valid = TRUE;
				Cprintf(ch, " %s", clan_table[clan].name);
			}
		}

		if (!valid)
			Cprintf(ch, "None, not even nonclanner. This is a bad idea.\n\r");
		else
			Cprintf(ch, "\n\r");
		Cprintf(ch, "Use 'vote set <field> <value> to edit the voting item.\n\r");
	}
	else if(vote.creator[0] != '\0'
	&& str_cmp(vote.creator, ch->name)) {
		Cprintf(ch, "%s is preparing the voting topic. Please be patient.\n\r", vote.creator);
		return;
	}
	return;
}

void 
vote_create(CHAR_DATA *ch)
{
	int min_level = 54;
	int clan;
	
	if(ch->level < min_level) {
		Cprintf(ch, "Only level %d can create a voting item.\n\r", min_level);
		return;
	}
	
	if(vote.creator[0] != '\0') {
		Cprintf(ch, "There is already a voting item. %s must delete theirs first.\n\r", vote.creator);
		return;
	}

	sprintf(vote.creator, "%s", ch->name);
	for (clan = 0; clan < MAX_CLAN; clan++)
		vote.restriction[clan] = TRUE;
	Cprintf(ch, "You start a new vote.\n\r");
	save_vote();
	return;
}

void 
vote_delete(CHAR_DATA *ch)
{
	int i;

	if(vote.creator[0] == '\0') {
		Cprintf(ch, "There is no current voting item.\n\r");
		return;
	}
	if(str_cmp(vote.creator, ch->name) && ch->level < 60) {
		Cprintf(ch, "Only %s can delete the voting item.\n\r", vote.creator);
		return;
	}
	if(vote.active == TRUE) {
		Cprintf(ch, "The vote must be closed before it can be deleted.\n\r", vote.creator);
		return;
	}
	vote.creator[0] = '\0';
	vote.question[0] = '\0';
	for(i=0;i<5;i++) {
		vote.result[i].description[0] = '\0';
		vote.result[i].tally = 0;
	}
	for (i = 0; i < MAX_CLAN; i++)
		vote.restriction[i] = TRUE;
	vote.active = FALSE;
	vote.voted[0] = '\0';
	Cprintf(ch, "You completely delete the vote. Ready for another topic now!\n\r");
	save_vote();
	return;
}

void 
vote_set(CHAR_DATA *ch, char *argument)
{
	char field[MAX_STRING_LENGTH], value[MAX_INPUT_LENGTH];
	int min_level = 54;
	int numvalue;

	if(ch->level < min_level) {
		Cprintf(ch, "Only level %d can edit the voting item.\n\r", min_level);
		return;
	}
	if(vote.creator[0] == '\0') {
		Cprintf(ch, "There is no vote to set. Use 'vote create' first.\n\r");
		return;
	}
	if(str_cmp(vote.creator, ch->name)) {
		Cprintf(ch, "Only %s can edit the voting item.\n\r", vote.creator);
		return;
	}

	argument = one_argument(argument, field);
	sprintf(value, "%s", argument);

	if(field[0] == '\0') {
		Cprintf(ch, "Vote set fields: question 1 2 3 4 5 level clan\n\r");
		return;
	}
	if(value[0] == '\0') {
		Cprintf(ch, "Please provide a value for this field.\n\r");
		return;
	}
	
	if(!str_prefix(field, "question") || !str_prefix(field, "topic")) {
		sprintf(vote.question,"%s",value);
		Cprintf(ch, "Voting topic set.\n\r");
	}
	else if(!str_cmp(field, "1")
	|| !str_cmp(field, "2")
	|| !str_cmp(field, "3")
	|| !str_cmp(field, "4")
	|| !str_cmp(field, "5")) {
		sprintf(vote.result[atoi(field)-1].description, "%s", value);
		Cprintf(ch, "Voting choice %s set.\n\r", field);
	}
	else if(!str_prefix(field, "level")) {
		if(!is_number(value)) {
			Cprintf(ch, "Please provide a number for the minimum level needed to vote.\n\r");
			return;
		}
		numvalue = atoi(value);
		if(numvalue < 0 || numvalue > 60) {
			Cprintf(ch, "Please choose a value between 0 and 60.\n\r");
			return;
		}
		vote.vote_level = numvalue;
		Cprintf(ch, "Only characters of level %d or above will be able to vote now.\n\r", numvalue);
	}
	else if(!str_prefix(field, "clan")) {
		/* we wanna set more than one clan at a time */
		Cprintf(ch, "Clan toggled: ");
		while(TRUE) {
			argument = one_argument(argument, value);
			if(value[0] == '\0')
                                break;       
			
			vote.restriction[clan_lookup(value)] = !vote.restriction[clan_lookup(value)];
			Cprintf(ch, clan_table[clan_lookup(value)].name);
		}
		Cprintf(ch, "\n\r");
	}
	save_vote();
	return;
}

void vote_voting(CHAR_DATA *ch, int choice)
{
	if(vote.creator[0] == '\0' || vote.active == FALSE) {
		Cprintf(ch, "There is no voting topic active at the moment.\n\r");
		return;
	}
	if(ch->level < vote.vote_level) {
		Cprintf(ch, "You must be level %d to vote on this question.\n\r", vote.vote_level);
		return;
	}
	if(vote.result[choice].description[0] == '\0') {
		Cprintf(ch, "That's not a valid choice for this vote.\n\r");
		return;
	}
	if(is_name(ch->name, vote.voted)) {
                Cprintf(ch, "You have already voted on this topic.\n\r");
                return;
        }         
	if (vote.restriction[ch->clan]){
		vote.result[choice].tally++;
		Cprintf(ch, "Your vote has been counted! Thank you.\n\r");
		sprintf(vote.voted, "%s %s", vote.voted, ch->name);
		save_vote();
	}
	else
		Cprintf(ch, "Your clan status prevents you from participating in this vote.\n\r");
	return;
}

void vote_open(CHAR_DATA *ch)
{
	int min_level = 54;

	if(ch->level < min_level) {
		Cprintf(ch, "Only level %d can edit the voting item.\n\r", min_level);
		return;
	}
	if(vote.creator[0] == '\0') {
		Cprintf(ch, "There is no vote to open. Use 'vote create' first.\n\r");
		return;
	}
	if(str_cmp(vote.creator, ch->name) && ch->level < 60) {
		Cprintf(ch, "Only %s can edit the voting item.\n\r", vote.creator);
		return;
	}
	if(vote.active == TRUE) {
		Cprintf(ch, "The vote is already open to the public.\n\r");
		return;
	}	

	/* Is the voting item complete? */
	if(vote.question[0] == '\0'
	|| (vote.result[0].description[0] == '\0'
	&& vote.result[1].description[0] == '\0'
	&& vote.result[2].description[0] == '\0'
	&& vote.result[3].description[0] == '\0'
	&& vote.result[4].description[0] == '\0')) {
		Cprintf(ch, "The voting item is not complete.\n\rUse 'vote set' to set question and answers.\n\r");
		return;
	}
	vote.active = TRUE;
	Cprintf(ch, "The voting system is now open. Let the games begin!\n\r");
	save_vote();
	return;
}

void vote_close(CHAR_DATA *ch) {
	int min_level = 54;

	if(ch->level < min_level) {
		Cprintf(ch, "Only level %d can edit the voting item.\n\r", min_level);
		return;
	}
	if(vote.creator[0] == '\0') {
		Cprintf(ch, "There is no vote to close. Use 'vote create' first.\n\r");
		return;
	}
	if(str_cmp(vote.creator, ch->name) && ch->level < 60) {
		Cprintf(ch, "Only %s can edit the voting item.\n\r", vote.creator);
		return;
	}
	if(vote.active == FALSE) {
		Cprintf(ch, "The vote is already closed to the public.\n\r");
		return;
	}

	vote.active = FALSE;
	Cprintf(ch, "The voting system is now closed to the public. No more votes accepted.\n\r");
	save_vote();
	return;
}

void vote_results(CHAR_DATA *ch) {
	int min_level = 54;
	int i;
	
	/* Any imm can view the results of a vote once its been setup */
	if(ch->level < min_level) {
		Cprintf(ch, "Only level %d can view the results.\n\r", min_level);
		return;
	}
	if(vote.creator[0] == '\0') {
		Cprintf(ch, "There is no vote to view. Use 'vote create' first.\n\r");
		return;
	}
	/* Is the voting item complete? */
	if(vote.question[0] == '\0'
	|| (vote.result[0].description[0] == '\0'
	&& vote.result[1].description[0] == '\0'
	&& vote.result[2].description[0] == '\0'
	&& vote.result[3].description[0] == '\0'
	&& vote.result[4].description[0] == '\0')) {
		Cprintf(ch, "Voting item must be set up before you can view results.\n\r");
		return;
	}
	
	Cprintf(ch, "The votes are in! Here are the results so far.\n\r");
	Cprintf(ch, "%s\n\r", vote.question);
	for(i=0;i<5;i++) {
		if(vote.result[i].description[0] != '\0')
			Cprintf(ch, "{C%d{x\t%d) %s\n\r", vote.result[i].tally, i+1, vote.result[i].description);
	}
	Cprintf(ch, "Use 'vote open/close' to start and end the voting.\n\r");
	return;	
}

/* This is slow, so only call it when really needed! */
void save_vote() {
	FILE *fptr;

	fptr = fopen("voteinfo.txt", "wb");
	fwrite(&vote, sizeof(vote), 1, fptr);
	fclose(fptr);
}

/* Should be called during startup some time. */
void load_vote() {
	FILE *fptr;
	int clan;

	fptr = fopen("voteinfo.txt", "rb");

	if(fptr == NULL) {
		vote.creator[0] = '\0';
		vote.question[0] = '\0';
		vote.voted[0] = '\0';
		vote.result[0].description[0] = '\0';
		vote.result[1].description[0] = '\0';
		vote.result[2].description[0] = '\0';
		vote.result[3].description[0] = '\0';
		vote.result[4].description[0] = '\0';
		vote.result[0].tally = 0;
		vote.result[1].tally = 0;
		vote.result[2].tally = 0;
		vote.result[3].tally = 0;
		vote.result[4].tally = 0;
		vote.voted[0] = '\0';
		for (clan = 0; clan < MAX_CLAN; clan++)
			vote.restriction[clan] = TRUE;
		vote.active = 0;
		return;	
	}
	
	fread(&vote, sizeof(vote), 1, fptr);
	fclose(fptr);
	return;
}

void 
do_nosac( CHAR_DATA *ch, char *argument ) 
{
	OBJ_DATA *obj;

	obj = get_obj_list(ch, argument, ch->carrying);
	if(obj == NULL)
	{
		Cprintf(ch, "You aren't carrying that.\n\r");
		return;
	}
	
	if(!str_cmp(obj->name, obj->pIndexData->name)
	&& !str_cmp(obj->short_descr, obj->pIndexData->short_descr)
	&& !str_cmp(obj->description, obj->pIndexData->description))
	{
		Cprintf(ch, "Only restrings can be made nosac this way.\n\r");
		return;
	}

	obj->wear_flags = obj->wear_flags ^ ITEM_NO_SAC;

	Cprintf(ch, "%s %s the no_sacrifice flag.\n\r", obj->short_descr, (obj->wear_flags & ITEM_NO_SAC) ? "gains" : "loses");
	return;
}

void do_blank(CHAR_DATA *ch, char *argument) {
    AFFECT_DATA *paf = NULL, *paf_next = NULL;
    CHAR_DATA *victim = NULL;

    if (IS_NPC(ch)) {
        return;
    }

    if(argument[0] == '\0') {
        Cprintf(ch, "All affects removed from yourself.\n\r");

        removeAllEffects(ch);
    } else if(!str_prefix(argument, "room")) {
        if (ch->level < 55) {
            Cprintf(ch, "Builders cannot blank rooms.\n\r");
            return;
        }

        Cprintf(ch, "All affects removed from this room.\n\r");

        for (paf = ch->in_room->affected; paf != NULL;paf = paf_next) {
            paf_next = paf->next;

            // Call end function if needed
            if (skill_table[paf->type].end_fun != end_null) {
                skill_table[paf->type].end_fun((void*)ch->in_room, TARGET_ROOM);
            }

            affect_remove_room(ch->in_room, paf);
        }
    } else {
        if (ch->level < 56 && ch->trust < 56) {
            Cprintf(ch, "You must be level 56 to blank another player.\n\r");
            return;
        }

        // Target a player here.
        victim = get_char_room(ch, argument);
        if (victim == NULL) {
            Cprintf(ch, "You don't see them here.\n\r");
            return;
        }

        Cprintf(ch, "All affects removed from %s.\n\r", victim->name);

        for (paf = victim->affected;paf != NULL;paf = paf_next) {
            paf_next = paf->next;

            // Call end function if needed
            if (skill_table[paf->type].end_fun != end_null) {
                skill_table[paf->type].end_fun((void*)victim, TARGET_CHAR);
            }

            affect_remove(victim, paf);
        }
    }

    return;
}

void
removeAllEffects(CHAR_DATA *ch) {
    AFFECT_DATA *paf, *paf_next;
    for (paf = ch->affected; paf != NULL; paf = paf_next) {
        paf_next = paf->next;

        // Call end function if needed
        if (skill_table[paf->type].end_fun != end_null) {
            skill_table[paf->type].end_fun((void*)ch, TARGET_CHAR);
        }

        affect_remove(ch, paf);
    }
}

void do_fixcp(CHAR_DATA *ch, char *argument) {
	int oldpoints;
	int oldexp;
	int newpoints; 
	int newexp;
	
	if(ch == NULL
	|| IS_NPC(ch))
		return;

	if(ch->level > 25) {
		Cprintf(ch, "Sorry, this command can only be used by characters below level 25.\n\r");
		return;
	}

	if(is_affected(ch, gsn_gladiator)
	|| is_affected(ch, gsn_spell_stealing)) {
		Cprintf(ch, "Your temporary skills cause fixcp to fail.\n\r");
		return;
	}

	oldpoints = ch->pcdata->points;
	oldexp = exp_per_level(ch, oldpoints);
	newpoints = remort_pts_level(ch);
	newexp = exp_per_level(ch, newpoints);

	if(argument[0] == '\0'
	&& ch->pcdata->confirm_fixcp) {
		ch->pcdata->points = newpoints;
		Cprintf(ch, "All your skills have been recounted and cp adjusted.\n\r");
		log_string("%s used fixcp went from %d cp to %d cp.", ch->name, oldpoints, newpoints);

		// Don't allow free level ups.
		if(ch->exp / newexp > ch->level)
			ch->exp = ((ch->level + 1) * newexp) - 1;

		return;
	}
	else if(ch->pcdata->confirm_fixcp == FALSE
	&& argument[0] == '\0') {
		ch->pcdata->confirm_fixcp = TRUE;
		Cprintf(ch, "Warning: This command will recount all the skills and groups you have\n\r");
		Cprintf(ch, "and adjust your creation points. If you have gained skills using trains\n\r");
		Cprintf(ch, "or used gain points, your exp per level will increase. If bugs or costs\n\r");
		Cprintf(ch, "of skills added cp incorrectly, this will correct them. If you fix cp,\n\r");
		Cprintf(ch, "you will have %d cp and be %d xp per level.\n\r",
			newpoints, exp_per_level(ch, newpoints));
		Cprintf(ch, "Type fixcp to proceed, or type fixcp <something> to cancel confirmation.\n\r");
	}
	else if(argument[0] != '\0') {
		ch->pcdata->confirm_fixcp = FALSE;
		Cprintf(ch, "Fix cp confirmation removed.\n\r");
	}
	
	return;
}


void do_crash(CHAR_DATA *ch, char *arg) {

	CHAR_DATA *pooper = NULL;

	pooper->name = "crashorific";
	return;
}

void
do_clan_rank(CHAR_DATA * ch, char *argument) {
    char command[MAX_INPUT_LENGTH];
    char target[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL;
    DESCRIPTOR_DATA *d = NULL;
    bool found = FALSE;
    int i;

    //count colour codes
    int bracecount;
    char formatstring[MAX_INPUT_LENGTH];
    char* brace;

    argument = one_argument(argument, command);
    argument = one_argument(argument, target);
    // Keep the actual rank in argument otherwise we get no capitalization.

    if (command[0] == '\0' 
            || !str_cmp(command, "list")
            || !str_cmp(command, "show")) {
        Cprintf(ch, "Character        Rank                       Clan\n\r");
        Cprintf(ch, "---------        ----                       ----\n\r");

	for (i = 0; i < MAX_CLAN; i++) {
	    if (clan_table[i].independent)
	        continue;
            for (d = descriptor_list; d != NULL; d = d->next) {
                if (d->character
                        && !IS_IMMORTAL(d->character)
                        && can_see(ch, d->character)
                        && d->character->clan == i) {
                    if (d->character->clan_rank) {
                        bracecount = 0;
                        brace = d->character->clan_rank;

                        while ((brace = strstr(brace, "{\0")) != NULL) {
                            bracecount++;
                            brace++;
                        }

                        sprintf(formatstring, "%%-15s  %%-%ds  %%-10s\n\r", 25+(2*bracecount));

                        found = TRUE;
                        Cprintf(ch, formatstring, d->character->name, d->character->clan_rank, capitalize(clan_table[d->character->clan].name));
                    } else {
                        found = TRUE;
                        Cprintf(ch, "%-15s  %-25s  %-10s\n\r", d->character->name, d->character->clan_rank ? d->character->clan_rank : "Member", capitalize(clan_table[d->character->clan].name));
                    }
                }
            }
        }

        if (found == FALSE) {
            Cprintf(ch, "No clanned characters found.\n\r");
        }

        return;
    }

    if (!ch->clan
            || (ch->trust < 52 
                    && ch->level < 52)) {
        Cprintf(ch, "Only clan leaders can change clan ranks.\n\r");
        return;
    } 

    if (target[0] == '\0') {
        Cprintf(ch, "You must specify a target who is connected.\n\r");
        return;
    }

    if ((victim = get_char_world(ch, target, TRUE)) == NULL 
            || !can_see(ch, victim)) {
        Cprintf(ch, "There is no such player online.\n\r");
        return;
    }

    if (IS_IMMORTAL(victim)) {
        Cprintf(ch, "Don't mess with immortal ranks.\n\r");
        return;
    }

    if (IS_NPC(ch)) {
        return;
    }

    if (IS_NPC(victim)) {
        Cprintf(ch, "You can't modify mobs with this command.\n\r");
        return;
    }

    if (victim->clan != ch->clan) {
        Cprintf(ch, "They aren't in your clan.\n\r");
        return;
    }

    // Now check commands.
    if (!str_cmp(command, "set")) {
        if(argument[0] == '\0') {
            Cprintf(ch, "You must specify a rank.\n\r");
            return;
        }

        if (strlen(argument) > 25) {
            Cprintf(ch, "Ranks must be 25 characters or less.\n\r");
            return;
        }

        victim->clan_rank = str_dup(argument);
        Cprintf(ch, "Rank set.\n\r");
        sprintf(buf, "%s's rank has been changed!\n\r", capitalize(victim->name));
    }

    if (!str_cmp(command, "strip")
            || !str_cmp(command, "clear")
            || !str_cmp(command, "remove")) {
        free_string(victim->clan_rank);
        victim->clan_rank = NULL;
        Cprintf(ch, "Rank stripped.\n\r");
        sprintf(buf, "%s's rank has been stripped!\n\r", capitalize(victim->name));
        return;
    }

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL
                && d->character->clan == victim->clan
                && !IS_NPC(d->character)
                && d->character != ch
                && d->character != victim) {
            Cprintf(d->character, "%s", buf);
        }
    }

    return;
}

/* Online Member Lists, coded by Tsongas April 7, 2003 *
 * Syntax: memberlist <clan> <recipient>               */
void
do_member_list(CHAR_DATA * ch, char *argument) {
    FILE *grep_file;
    char grep_input[80];
    char clan_name[MAX_INPUT_LENGTH];
    char pretext[4 * MAX_STRING_LENGTH - 1000];
    char text[4 * MAX_STRING_LENGTH - 1000];
    char current;
    int i = 0;
    int j = 0;
    char line[MAX_STRING_LENGTH];
    char note[4 * MAX_STRING_LENGTH - 1000];

    argument = one_argument(argument, clan_name);

    /* No argument fool! */
    if(clan_name[0] == '\0') {
        Cprintf(ch, "Please specify clan name and who to send the note to.\n\r");
        return;
    }

    /* Not an existing clan name?! */
	if (clan_lookup(clan_name) == 0)
	{
		Cprintf(ch, "You require an existing clan to lookup.\n\r");
		return;
	}

    if(argument[0] == '\0') {
        Cprintf(ch, "You require a recipient.\n\r");
        return;
    }

    /* Use system's grep command and write it out to a file. */
    sprintf(grep_input, "grep -l \"Clan %s\" ../player/* > grep_output.dat", clan_name);	
    system(grep_input);

    /* Open the file for scanning */
    grep_file = fopen("grep_output.dat", "r");

    /* Send the info to a text buffer so we can write a nice note */
    while(fscanf(grep_file, "%c", &current) != EOF) {
        pretext[i] = current;
        i++;
    }

    /* Cap that mofo. */
    pretext[i] = '\0';

    /* Format the text to only have the names: Ditch the path */
    for (i = 0, j = 0; pretext[j] != '\0'; i++) {
        if (pretext[j] == '.' && pretext[j + 1] == '.') {
            j += 10;
        }

        text[i] = pretext[j];
        j++;
    }

    text[i] = '\0';

    i = 0;
    j = 0;
    note[0] = '\0';
    while (sscanf(text + j, "%s", line) != EOF) {
        char buf[MAX_STRING_LENGTH];

        j += strlen(line) + 1;
        i++;

        sprintf(buf, "%-20s", line);
        strcat(note, buf);

        if ((i % 4) == 0) {
            strcat(note, "\n\r");
        }
    }

    /* And we have a nice friendly note from the mud!*/
    make_note("Personal", ch->name, argument, "Member List", 30, note) ;

    /* Close the file and remove the temp file */
    fclose(grep_file);
    system("rm grep_output.dat");

    Cprintf(ch, "Member list for clan %s has been sent.\n\r", capitalize(clan_name));

    return;
}

void load_tips() {
	FILE *fp;
	TIP_DATA *pTip;
	top_tips = 0;

	fp = fopen("tips.txt", "r");

	if(fp == NULL) {
		log_string("No tips file found.");
		first_tip = NULL;
		last_tip = NULL;
		return;
	}
	
	// Read help file entries
	for (;;)
	{
        	pTip = (TIP_DATA *) alloc_perm(sizeof(*pTip));       

        	pTip->id = fread_number(fp);
		pTip->message = fread_string(fp);

		if(pTip->message[0] == '$')
			break;

        	if (first_tip == NULL)
                	first_tip = pTip;
        	if (last_tip != NULL)
                	last_tip->next = pTip;

        	last_tip = pTip;
        	pTip->next = NULL;
        	top_tips++;
	}

	return;
}

/* patron code changes start */
void
do_pledge(CHAR_DATA * ch, char *argument)
{
        CHAR_DATA *vch;
        char arg1[MAX_INPUT_LENGTH];

        argument = one_argument(argument, arg1);

        if (arg1[0] == '\0')
        {
                Cprintf(ch, "Pledge to which character?\n\r");
                return;
        }

        if ((vch = get_char_world(ch, arg1, TRUE)) == NULL)
        {
                Cprintf(ch, "That person is not online at the moment.\n\r");
                return;
        }
                                                  
	if (IS_NPC(vch))
        {
                Cprintf(ch, "You cannot pledge to mobs. Nice try.\n\r");
                return;
        }

        if (ch == vch)
        {
                Cprintf(ch, "You cannot pledge to yourself! Nice try.\n\r");
                return;
        }

        if (vch->patron != NULL && !str_cmp(vch->patron, ch->name))
        {
                Cprintf(ch, "You cannot pledge to your vassal! Nice try.\n\r");
                return;
        }

        if (clan_table[ch->clan].pkiller != clan_table[vch->clan].pkiller)
        {
                Cprintf(ch, "You cannot pledge to someone with another pkill status.\n\r");
                return;
        }

        if (ch->level >= vch->level)
        {
                Cprintf(ch, "You can only pledge to a character higher than you.\n\r");
                return;
        }

        if (IS_IMMORTAL(vch))
        {
                Cprintf(ch, "You cannot pledge to an immortal. Bootlicking is prohibited.\n\r");
                return;
        }                             

        if (ch->in_room != vch->in_room)
        {
                Cprintf(ch, "Your prospective patron, %s, is not in the room.\n\r", vch->name);
                return;
        }

        if ((ch->patron != NULL) && !IS_SET(ch->toggles, TOGGLES_PLEDGE))
        {
                Cprintf(ch, "You have already pledged your allegiance to %s.\n\r", ch->patron);
                return;
        }

        SET_BIT(ch->toggles, TOGGLES_PLEDGE);
        free_string(ch->patron);
        ch->patron = str_dup(vch->name);
        Cprintf(ch, "%s has been notified of your desire to become a vassal.\n\r", ch->patron);
        Cprintf(vch, "{R%s wishes to become your vassal.  Use 'assume' to become patron.{x\n\r", ch->name);                    

        return;
}               

void
do_assume(CHAR_DATA * ch, char *argument)
{
        CHAR_DATA *vch;
        char arg1[MAX_INPUT_LENGTH];

        argument = one_argument(argument, arg1);

        if (arg1[0] == '\0')
        {
                Cprintf(ch, "Assume responsibility for which character?\n\r");
                return;
        }

        if ((vch = get_char_world(ch, arg1, TRUE)) == NULL)
        {
                Cprintf(ch, "That person is not online at the moment.\n\r");
                return;
        }

        if (!IS_SET(vch->toggles, TOGGLES_PLEDGE))
        {
                Cprintf(ch, "That person is not looking for a patron.\n\r");
                return;
        }

        if (str_cmp(vch->patron, ch->name))
        {
                Cprintf(ch, "That person is not pledging to you!\n\r");
                return;
        }

        if (ch->vassal != NULL)
        {
                Cprintf(ch, "You are already the patron of %s!\n\r", ch->vassal);
                return;
        }

        if (ch->in_room != vch->in_room)
        {
                Cprintf(ch, "Your prospective vassal, %s, is not in the room.\n\r", vch->name);
                return;
        }                                                            

        REMOVE_BIT(vch->toggles, TOGGLES_PLEDGE);
        free_string(ch->vassal);
        ch->vassal = str_dup(vch->name);
        Cprintf(ch, "You are now the patron of %s.\n\r", vch->name);
        Cprintf(vch, "You are now the vassal of %s.\n\r", ch->name);

        return;
}
                            
void
do_droppatron(CHAR_DATA * ch, char *argument)
{
        DESCRIPTOR_DATA *d, *desc;
        bool isChar = FALSE;
        CHAR_DATA *vch;

        if (ch->patron == NULL)
        {
                Cprintf(ch, "You do not have a patron!\n\r");
                return;
        }

	// Find patron if he is online.
	for(desc = descriptor_list; desc != NULL; desc = desc->next) 
	{
		vch = desc->character;

		if(vch == NULL)
			continue;

        	if (!str_cmp(vch->name, ch->patron))
        	{
                	Cprintf(ch, "%s is no longer your patron.\n\r", ch->patron);
                	Cprintf(vch, "{R%s is no longer your vassal.{x\n\r", ch->name);

                	if (!str_cmp(vch->vassal, ch->name))
                	{
                        	free_string(vch->vassal);
                        	vch->vassal = NULL;                
                        	save_char_obj(vch, FALSE);
                	}
                	free_string(ch->patron);
                	ch->patron = NULL;
                	ch->to_pass = 0;

                	REMOVE_BIT(ch->toggles, TOGGLES_PLEDGE);
                	save_char_obj(ch, FALSE);
                	return;
		}
        }

        d = new_descriptor();

        isChar = load_char_obj(d, ch->patron);        /* char pfile exists? */

        if (!isChar)
        {
                Cprintf(ch, "Your patron no longer exists.  You are free to pledge again.\n\r");
                free_string(ch->patron);                      
                ch->patron = NULL;
                ch->to_pass = 0;
                REMOVE_BIT(ch->toggles, TOGGLES_PLEDGE);
                save_char_obj(ch, FALSE);
                return;
        }

        d->connected = CON_PLAYING;
        reset_char(d->character);

        if (!str_cmp(ch->name, d->character->vassal))
        {
                free_string(d->character->vassal);
                d->character->vassal = NULL;
        }

        Cprintf(ch, "%s is no longer your patron.\n\r", ch->patron);
        free_string(ch->patron);
        ch->patron = NULL;
        ch->to_pass = 0;

        REMOVE_BIT(ch->toggles, TOGGLES_PLEDGE);            
        save_char_obj(d->character, FALSE);
        save_char_obj(ch, FALSE);
        free_descriptor(d);

        return;
 }

 void
 do_dropvassal(CHAR_DATA * ch, char *argument)
 {
        DESCRIPTOR_DATA *d, *desc;
        bool isChar = FALSE;
        CHAR_DATA *vch;

        if (ch->vassal == NULL)
        {
                Cprintf(ch, "You do not have a vassal!\n\r");
                return;
        }
                                                   
	// Find patron if he is online.
	for(desc = descriptor_list; desc != NULL; desc = desc->next)
	{
        	vch = desc->character;

        	if(vch == NULL)
                	continue;

        	if (!str_cmp(vch->name, ch->vassal))
        	{
                	Cprintf(ch, "%s is no longer your vassal.\n\r", ch->vassal);
                	Cprintf(vch, "{R%s is no longer your patron.{x\n\r", ch->name);

                	if (!str_cmp(vch->patron, ch->name))
   	                {
                        	free_string(vch->patron);
                        	vch->patron = NULL;
                        	vch->to_pass = 0;
                        	save_char_obj(vch, FALSE);
                	}

                	free_string(ch->vassal);
                	ch->vassal = NULL;
                	save_char_obj(ch, FALSE);
                	return;
		}
        }

                                             
        d = new_descriptor();
        isChar = load_char_obj(d, ch->vassal);        /* char pfile exists? */

        if (!isChar)
        {
                Cprintf(ch, "Your vassal no longer exists.  You are free to mentor someone else.\n\r");
                free_string(ch->vassal);
                ch->vassal = NULL;
                save_char_obj(ch, FALSE);
                return;
        }

        d->connected = CON_PLAYING;
        reset_char(d->character);

        if (!str_cmp(ch->name, d->character->patron))
        {
                free_string(d->character->patron);
                d->character->patron = NULL;
                d->character->to_pass = 0;
        }                                                      
        Cprintf(ch, "%s is no longer your vassal.\n\r", ch->vassal);
        free_string(ch->vassal);
        ch->vassal = NULL;
        save_char_obj(d->character, FALSE);
        save_char_obj(ch, FALSE);

        return;
}                      

// end of patron and vassal code

void
do_ctf(CHAR_DATA * ch, char *argument)
{
        CHAR_DATA *victim;
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	char arg3[MAX_STRING_LENGTH];
	OBJ_DATA *flag;
	OBJ_DATA *flag2;
	ROOM_INDEX_DATA *room;
	AFFECT_DATA af;

	DESCRIPTOR_DATA *d;
	CHAR_DATA *fch;
	int team;
	int newscore;

        argument = one_argument(argument, arg1);

	//different commands are available to the imm running it
	//and to the mortals participating
	if (IS_IMMORTAL(ch) && ch->level >= 56)
	{
		if (arg1 == NULL || arg1[0] == '\0')
		{
        		Cprintf(ch, "Possible commands are start, end, score, set, add, and remove.\n\r");
			return;
		}
		
		//takes every char on a team & puts them in the flag room
		if (!strcmp(arg1, "start"))
		{
			if (CTF_SCORE(CTF_RED) > 0 || CTF_SCORE(CTF_BLUE) > 0)
			{
				Cprintf(ch, "A game of capture the flag is already underway!");
				return;
			}

			for (d = descriptor_list; d != NULL; d = d->next)
			{
				if (d->connected == CON_PLAYING && d->character->pcdata->ctf_team > 0 && !IS_IMMORTAL(d->character))
				{
					char_from_room(d->character);
					char_to_room(d->character, get_room_index(ROOM_CTF_FLAG(d->character->pcdata->ctf_team)));
					Cprintf(d->character, "Capture the flag has begun!\n\r");
					do_look(d->character, "auto");
				}
			}

			wiznet("A game of capture the flag has begun.", ch, NULL, WIZ_ON, 0, get_trust(ch));
			Cprintf(ch, "Capture the flag started. Score reset to 0.\n\r");
			//CTF_SCORE(CTF_RED) = 0;
			//CTF_SCORE(CTF_BLUE) = 0;
         ctf_score_blue = 0;
         ctf_score_red = 0;
			flag = create_object(get_obj_index(OBJ_CTF_FLAG(CTF_BLUE)),0);
			obj_to_room(flag, get_room_index(ROOM_CTF_FLAG(CTF_BLUE)));
			flag2 = create_object(get_obj_index(OBJ_CTF_FLAG(CTF_RED)),0);
			obj_to_room(flag2, get_room_index(ROOM_CTF_FLAG(CTF_RED)));
			return;
		}

		if (!strcmp(arg1, "end"))
		{
			for(fch = char_list; fch != NULL; fch = fch->next)
			{
				if(!IS_NPC(fch) && fch->pcdata->ctf_team > 0)
				{
					char_from_room(fch);
					char_to_room(fch, get_room_index(clan_table[fch->clan].hall));
					Cprintf(fch, "Capture the flag is over. Good game!\n\r");
					affect_strip(fch, gsn_aura);
					if (is_affected(fch, gsn_jail))
						affect_strip(fch, gsn_jail);
					fch->pcdata->ctf_team = 0;
					fch->pcdata->ctf_flag = FALSE;
					restore_char(fch);
					do_look(fch, "auto");
				}
			}

			for (flag = object_list; flag != NULL; flag = flag->next)
			{
				if (flag->pIndexData->vnum == OBJ_CTF_FLAG(CTF_BLUE) || flag->pIndexData->vnum == OBJ_CTF_FLAG(CTF_RED))
				{
					obj_from_room(flag);
				}
			}
			Cprintf(ch, "Capture the flag game terminated. Get back to work!\n\r");
			//CTF_SCORE(CTF_RED) = 0;
			//CTF_SCORE(CTF_BLUE) = 0;
         ctf_score_red = 0;
         ctf_score_blue = 0;

			return;
		}

		if (!strcmp(arg1, "set"))
		{
			argument = one_argument(argument, arg2);

			if (arg2 == NULL || arg2[0] == '\0')
			{
        			Cprintf(ch, "Syntax: ctf set <red/blue> <number>.\n\r");
				return;
			}

			if (!strcmp(arg2, "red"))
			{
				team = CTF_RED;
			}
			else if (!strcmp(arg2, "blue"))
			{
				team = CTF_BLUE;
			}
			else
			{
        			Cprintf(ch, "Syntax: ctf set <red/blue> <number>.\n\r");
				return;
			}

        		argument = one_argument(argument, arg3);

			if (arg3 == NULL || *arg3 == '\0')
			{
        			Cprintf(ch, "Syntax: ctf set <red/blue> <number>.\n\r");
				return;
			}

			newscore = atoi(arg3);

			if (newscore < 0 || newscore > 50)
			{
				Cprintf(ch, "Please select a score between 0 and 50.\n\r");
				return;
			}

			Cprintf(ch, "The %s team's score changed from %d to %d.\n\r", arg2, CTF_SCORE(team), newscore);

			//CTF_SCORE(team) = newscore;
         if (team == CTF_RED) {
            ctf_score_red = newscore;
         } else {
            ctf_score_blue = newscore;
         }

			return;
		}
			

		if (!strcmp(arg1, "add"))
		{
		        argument = one_argument(argument, arg2);

			if (arg2 == NULL || arg2[0] == '\0')
			{
        			Cprintf(ch, "You need to pick someone to add to the game.\n\r");
				return;
			}

			if ((victim = get_char_world(ch, arg2, TRUE)) == NULL)
			{
        			Cprintf(ch, "Unable to add that person.\n\r");
				return;
			}

			if (IS_NPC(victim))
			{
				Cprintf(ch, "Only players can partake in CTF.\n\r");
				return;
			}

			if (IS_IMMORTAL(victim))
			{
				Cprintf(ch, "Immortals have more important things to do than play CTF.\n\r");
				return;
			}

			if (victim->pcdata->ctf_team > 0)
			{
				Cprintf(ch, "That person is already on a CTF team!\n\r");
				return;
			}

        		argument = one_argument(argument, arg3);

			if (arg3 == NULL || *arg3 == '\0')
			{
				Cprintf(ch, "What team will you add them to?\n\r");
				return;
			}

			if (!strcmp(arg3, "blue"))
			{
				room = get_room_index(ROOM_CTF_PREP(CTF_BLUE));
				if (room == NULL)
				{
					Cprintf(ch, "Cannot find the blue team's prep room.\n\r");
					return;
				}

				victim->pcdata->ctf_team = CTF_BLUE;
				victim->pcdata->ctf_flag = FALSE;

				af.where = TO_AFFECTS;
				af.type = gsn_aura;
				af.level = ch->level;
				af.location = APPLY_NONE;
				af.modifier = CTF_BLUE;
				af.duration = -1;
				af.bitvector = 0;
				affect_to_char(victim, &af);
			
				Cprintf(victim, "Welcome to the blue team! Prepare for Capture The Flag!\n\r");
				char_from_room(victim);
				char_to_room(victim, room);
				do_look(victim, "auto");
				Cprintf(ch, "%s added to the blue team.\n\r", victim->name);
				return;
			}

			if (!strcmp(arg3, "red"))
			{
				room = get_room_index(ROOM_CTF_PREP(CTF_RED));
				if (room == NULL)
				{
					Cprintf(ch, "Cannot find the red team's prep room.\n\r");
					return;
				}

				victim->pcdata->ctf_team = CTF_RED;
				victim->pcdata->ctf_flag = FALSE;

				af.where = TO_AFFECTS;
				af.type = gsn_aura;
				af.level = ch->level;
				af.location = APPLY_NONE;
				af.modifier = CTF_RED;
				af.duration = -1;
				af.bitvector = 0;
				affect_to_char(victim, &af);
			
				Cprintf(victim, "Welcome to the red team! Prepare for Capture The Flag!\n\r");
				char_from_room(victim);
				char_to_room(victim, room);
				do_look(victim, "auto");
				Cprintf(ch, "%s added to the red team.\n\r", victim->name);
				return;
			}
		
			Cprintf(ch, "That's not a valid team.");
			return;	
		}

		if (!strcmp(arg1, "remove"))
		{
		        argument = one_argument(argument, arg2);

			if (arg2 == NULL || arg2[0] == '\0')
			{
        			Cprintf(ch, "You need to pick someone to remove from the game.\n\r");
				return;
			}

			if ((victim = get_char_world(ch, arg2, TRUE)) == NULL)
			{
        			Cprintf(ch, "Unable to remove that person.\n\r");
				return;
			}

			if (victim->pcdata->ctf_team == 0)
			{
				Cprintf(ch, "That person is not currently playing CTF.\n\r");
				return;
			}

			if (victim->pcdata->ctf_flag)
			{
				//make them drop the flag.
				victim->pcdata->ctf_flag = FALSE;
				flag = create_object(get_obj_index(OBJ_CTF_FLAG(CTF_OTHER_TEAM(victim->pcdata->ctf_team))),0);
				obj_to_room(flag, ch->in_room);
			}

			char_from_room(victim);
			char_to_room(victim, get_room_index(clan_table[victim->clan].hall));
			affect_strip(victim, gsn_aura);
			if (is_affected(victim, gsn_jail))
				affect_strip(victim, gsn_jail);

			Cprintf(victim, "You have been removed from capture the flag.\n\r");
			restore_char(victim);
			victim->pcdata->ctf_team = 0;
			do_look(victim, "auto");
			Cprintf(ch, "%s has been removed from CTF.\n\r", victim->name);
			return;
		}

		if (!strcmp(arg1, "score"))
		{
			Cprintf(ch, "The current score is Blue:%d, Red:%d.\n\r", CTF_SCORE(CTF_BLUE), CTF_SCORE(CTF_RED));
			return;
		}

       		Cprintf(ch, "Possible commands are start, end, score, set, add, and remove.\n\r");
		return;

	}
	else
	{
	
		if (ch->pcdata->ctf_team == 0)
		{
			Cprintf(ch, "You are not playing capture the flag.\n\r");
			return;
		}

		if (arg1 == NULL || arg1[0] == '\0')
		{
        		Cprintf(ch, "Possible commands are score and capture (cap).\n\r");
			return;
		}

		if (!strcmp(arg1, "score"))
		{
			Cprintf(ch, "The current score is Blue:%d, Red:%d.\n\r", CTF_SCORE(CTF_BLUE), CTF_SCORE(CTF_RED));
			return;
		}

		if (!strcmp(arg1, "capture") || !strcmp(arg1, "cap"))
		{
			if (ch->pcdata->ctf_flag == FALSE)
			{
				Cprintf(ch, "You cannot do that, you don't have the flag!\n\r");
				return;
			}

			if (ch->in_room != get_room_index(ROOM_CTF_FLAG(ch->pcdata->ctf_team)))
			{
				Cprintf(ch, "You aren't in your flag room yet!\n\r");
				return;
			}
		
			//CTF_SCORE(ch->pcdata->ctf_team)++;
         if (ch->pcdata->ctf_team == CTF_RED) {
            ctf_score_red++;
         } else {
            ctf_score_blue++;
         }

			for (d = descriptor_list; d; d = d->next)
			{
				if (d->connected == CON_PLAYING && d->character->pcdata->ctf_team > 0 && !IS_IMMORTAL(d->character))
				{
					if (ch->pcdata->ctf_team == CTF_BLUE)
						Cprintf(d->character, "The {Bblue{x team has scored a point!\n\r");	
					if (ch->pcdata->ctf_team == CTF_RED)
						Cprintf(d->character, "The {Rred{x team has scored a point!\n\r");	
				}
			}

			if (ch->pcdata->ctf_team == CTF_BLUE)
			{
				wiznet("The Blue team has scored a point in CTF.", ch, NULL, WIZ_ON, 0, get_trust(ch));
			}
			else
			{
				wiznet("The Red team has scored a point in CTF.", ch, NULL, WIZ_ON, 0, get_trust(ch));

			}
			ch->pcdata->ctf_flag = FALSE;

			flag = create_object(get_obj_index(OBJ_CTF_FLAG(CTF_OTHER_TEAM(ch->pcdata->ctf_team))),0);
			obj_to_room(flag, get_room_index(ROOM_CTF_FLAG(CTF_OTHER_TEAM(ch->pcdata->ctf_team))));

		}
	}
}

void
do_flag (CHAR_DATA * ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char word[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	bitset *flag;
	long old = 0;
	long replace = 0;
	long marked = 0;
	long pos;
	char type;
	const struct flag_type *flag_table;

	argument = one_argument (argument, arg1);
	argument = one_argument (argument, arg2);
	argument = one_argument (argument, arg3);

	type = argument[0];

	if (type == '=' || type == '-' || type == '+') {
		argument = one_argument (argument, word);
	}

	if (arg1[0] == '\0') {
		Cprintf (ch, "Syntax:\n\r");
		Cprintf (ch, "  flag mob  <name> <field> <flags>\n\r");
		Cprintf (ch, "  flag char <name> <field> <flags>\n\r");
		Cprintf (ch, "  mob  flags: act,aff,off,imm,res,vuln,form,part\n\r");
		Cprintf (ch, "  char flags: plr,comm,aff,imm,res,vuln,\n\r");
		Cprintf (ch, "  +: add flag, -: remove flag, = set equal to\n\r");
		Cprintf (ch, "  otherwise flag toggles the flags listed.\n\r");
		return;
	}

	if (arg2[0] == '\0') {
		Cprintf (ch, "What do you wish to set flags on?\n\r");
		return;
	}

	if (arg3[0] == '\0') {
		Cprintf (ch, "You need to specify a flag to set.\n\r");
		return;
	}

	if (argument[0] == '\0') {
		Cprintf (ch, "Which flags do you wish to change?\n\r");
		return;
	}

	if (!str_prefix (arg1, "mob") || !str_prefix (arg1, "char")) {
		victim = get_char_world (ch, arg2, TRUE);

		if (victim == NULL) {
			Cprintf (ch, "You can't find them.\n\r");
			return;
		}

		/* select a flag to set */
		if (!str_prefix (arg3, "act")) {
			if (!IS_NPC (victim)) {
				Cprintf (ch, "Use plr for PCs.\n\r");
				return;
			}

			flag = &victim->act;
			flag_table = act_flags;
		} else if (!str_prefix (arg3, "plr")) {
			if (IS_NPC (victim)) {
				Cprintf (ch, "Use act for NPCs.\n\r");
				return;
			}

			flag = &victim->act;
			flag_table = plr_flags;
		} else if (!str_prefix (arg3, "aff")) {
			flag = &victim->affected_by;
			flag_table = affect_flags;
		} else if (!str_prefix (arg3, "immunity")) {
			flag = &victim->imm_flags;
			flag_table = imm_flags;
		} else if (!str_prefix (arg3, "resist")) {
			flag = &victim->res_flags;
			flag_table = imm_flags;
		} else if (!str_prefix (arg3, "vuln")) {
			flag = &victim->vuln_flags;
			flag_table = imm_flags;
		} else if (!str_prefix (arg3, "form")) {
			if (!IS_NPC (victim)) {
				Cprintf (ch, "Form can't be set on PCs.\n\r");
				return;
			}

			flag = &victim->form;
			flag_table = form_flags;
		} else if (!str_prefix (arg3, "parts")) {
			if (!IS_NPC (victim)) {
				Cprintf (ch, "Parts can't be set on PCs.\n\r");
				return;
			}

			flag = &victim->parts;
			flag_table = part_flags;
		} else if (!str_prefix (arg3, "comm")) {
			if (IS_NPC (victim)) {
				Cprintf (ch, "Comm can't be set on NPCs.\n\r");
				return;
			}

			flag = &victim->comm;
			flag_table = comm_flags;
		} else {
			Cprintf (ch, "That's not an acceptable flag.\n\r");
			return;
		}

		old = *flag;
		victim->zone = NULL;

		if (type != '=') {
			replace = old;
		}

		/* mark the words */
		for (;;) {
			argument = one_argument (argument, word);

			if (word[0] == '\0') {
				break;
			}

			pos = flag_lookup (word, flag_table);
			if (pos == 0 || pos == NO_FLAG) {
				Cprintf (ch, "That flag doesn't exist!\n\r");
				return;
			} else {
				SET_BIT (marked, pos);
			}
		}

		for (pos = 0; flag_table[pos].name != NULL; pos++) {
			if (!flag_table[pos].settable && IS_SET (old, flag_table[pos].bit)) {
				SET_BIT (replace, flag_table[pos].bit);
				continue;
			}

			if (IS_SET (marked, flag_table[pos].bit)) {
				switch (type) {
					case '=':
					case '+':
						SET_BIT (replace, flag_table[pos].bit);
						break;
					case '-':
						REMOVE_BIT (replace, flag_table[pos].bit);
						break;
					default:
						if (IS_SET (replace, flag_table[pos].bit)) {
							REMOVE_BIT (replace, flag_table[pos].bit);
						} else {
							SET_BIT (replace, flag_table[pos].bit);
						}
				}
			}
		}
		
		*flag = replace;
		return;
	}
}

void do_eqoutput(CHAR_DATA *ch, char *argument) {
    //declarations
    int items[500]; //array of objects to store, 500 per item type is *max*
    OBJ_INDEX_DATA *obj; //our object
    int n;
    char arg[MAX_STRING_LENGTH]; //eqoutput <thing>
    char filename[MAX_STRING_LENGTH]; //file, hurray
    int num;
    FILE *fp;
    int vnum;
    int count;

    // error checking
    if (IS_NPC(ch)) {
        return;
    }
    
    if (ch->level < 60) {
        return;  //only imps can use this
    }

    one_argument(argument, arg);

    // initiate our array
    for (n = 0; n < 500; n++) {
        items[n] = 0;
    }

    // eqoutput with no params
    if (arg[0] == '\0') {
        Cprintf(ch, "Output to which file?\n\r");
        Cprintf(ch, "Valid files are:\n\r");
        Cprintf(ch, "arms body feet finger hand head legs neck shield\n\r");
        Cprintf(ch, "torso waist wrist axes daggers exotic flails mace katana\n\r");
        Cprintf(ch, "polearms spears swords whips throwing boats float held ranged\n\r");
        Cprintf(ch, "light pill potion scroll staff wands recall warp other\n\r");

        return;
    }

    // translate the argument into an easier-worked with number
    // to add new stuff just put it at the bottom
    if (!str_cmp(arg,"arms")) {
        num = 0;
    } else if (!str_cmp(arg,"body")) {
        num = 1;
    } else if (!str_cmp(arg,"feet")) {
        num = 2;
    } else if (!str_cmp(arg,"finger")) {
        num = 3;
    } else if (!str_cmp(arg,"hand")) {
        num = 4;
    } else if (!str_cmp(arg,"head")) {
        num = 5;
    } else if (!str_cmp(arg,"legs")) {
        num = 6;
    } else if (!str_cmp(arg,"neck")) {
        num = 7;
    } else if (!str_cmp(arg,"shield")) {
        num = 8;
    } else if (!str_cmp(arg,"torso")) {
        num = 9;
    } else if (!str_cmp(arg,"waist")) {
        num = 10;
    } else if (!str_cmp(arg,"wrist")) {
        num = 11;
    } else if (!str_cmp(arg,"axes")) {
        num = 12;
    } else if (!str_cmp(arg,"daggers")) {
        num = 13;
    } else if (!str_cmp(arg,"exotic")) {
        num = 14;
    } else if (!str_cmp(arg,"flails")) {
        num = 15;
    } else if (!str_cmp(arg,"mace")) {
        num = 16;
    } else if (!str_cmp(arg,"polearms")) {
        num = 17;
    } else if (!str_cmp(arg,"spears")) {
        num = 18;
    } else if (!str_cmp(arg,"swords")) {
        num = 19;
    } else if (!str_cmp(arg,"whips")) {
        num = 20;
    } else if (!str_cmp(arg,"throwing")) {
        num = 21;
    } else if (!str_cmp(arg,"boats")) {
        num = 22;
    } else if (!str_cmp(arg,"held")) {
        num = 23;
    } else if (!str_cmp(arg,"float")) {
        num = 24;
    } else if (!str_cmp(arg,"light")) {
        num = 25;
    } else if (!str_cmp(arg,"pill")) {
        num = 26;
    } else if (!str_cmp(arg,"potion")) {
        num = 27;
    } else if (!str_cmp(arg,"scroll")) {
        num = 28;
    } else if (!str_cmp(arg,"staff")) {
        num = 29;
    } else if (!str_cmp(arg,"wands")) {
        num = 30;
    } else if (!str_cmp(arg,"recall")) {
        num = 31;
    } else if (!str_cmp(arg,"warp")) {
        num = 32;
    } else if (!str_cmp(arg,"other")) {
        num = 33;
    } else if (!str_cmp(arg,"katana")) {
        num = 34;
    } else if (!str_cmp(arg,"ranged")) {
        num = 35;
    } else {
        Cprintf(ch, "Please choose a valid file.\n\r");
        return;
    }

    // load the filename-- ie. output_wand.html
    sprintf(filename, "webfiles/%s.php",arg);

    fp = fopen(filename, "w");
    if (fp == NULL ) {
        //something went WRONG (usually full hard drive)
        perror( "eq_output was not able to create the output file, oh noes" );
        Cprintf(ch, "Unable to create the output file: %s\n\r", strerror(errno));
        return;
    }

    // start loading html
    // here, you can hardcode in your webpage if you want
    fprintf(fp, "<?php\n");
    fprintf(fp, "$sidebar = 'equipment';\n");
    fprintf(fp, "require_once(\"../header.php\");\n");
    fprintf(fp, "?>\n");
    fprintf(fp, "<p class='title'><a href='http://www.redemptionmud.com/equipment.php'>Armor And Equipment List</a></p>\n");
    fprintf(fp, "<p class='header'>");
    fprintf(fp, arg);
    fprintf(fp, ":</p>\n");
    fprintf(fp, "<table class='index' border='1'>\n");
    fprintf(fp, "<tbody>\n");
     
    // the big loop!
    // searches through all 32000 obj vnums
    // dumps invalid ones, stores valid ones in our items[] array
    count = 0;
    for (vnum = 0; vnum < 32000; vnum++) {
        if ((obj = get_obj_index(vnum)) == NULL) {
            continue;
        }
        
        if (!item_is_valid(obj, num)) {
            continue;
        }
        
        items[count] = obj->vnum;
        count++;

        if (count >= 500) {
            Cprintf(ch, "Array of qualifying items has been overrun. Not all items may be included.\n\r");
            break;
        }
    }

    // loop through all valid levels
    for (n = 0; n < 54; n++) {
        // loop through our items[] array
        int i;
        for (i = 0; i < 500; i++) {
            if (items[i] == 0) {
                break;
            }
  	      
            // need a new obj to get the obj level and write it
            if ((obj = get_obj_index(items[i])) == NULL) {
                continue;
            }

            // if we're on the right level, write this suckah
            if (obj->level == n && item_is_valid(obj,num)) { 
                write_object(fp, obj);
            }
        }
    }

    // finish the table off
    fprintf(fp, "</tbody>\n");
    fprintf(fp, "</table>\n");
    fprintf(fp, "<?php\n");
    fprintf(fp, "require_once(\"./eqfooter.php\");\n");
    fprintf(fp, "?>\n");
    
    // close file
    fclose(fp);
	 
    Cprintf(ch, "File created.\n\r");
    return;
}

//this function actually writes the obj. data to the html file
//it is quite simple.
void write_object(FILE *fp, OBJ_INDEX_DATA *obj) {
    AFFECT_DATA *af; //object affects '+2 dam' etc
    char buf[256];
    char* replace; 

    //start our table, plunk default stats down
    //[level |  name  |  area  | stats  ]
    
    fprintf(fp, "<tr>\n");
    fprintf(fp, "<td>%d</td>\n", obj->level);
    sprintf(buf, "<td>%s</td>\n", obj->short_descr);

    //make sure there's no "'s in the buf
    while ((replace = strchr(buf, '\"')) != NULL) {
        replace[0] = '\'';     
    }
    
    //make sure there's no color codes in the buf
    while ((replace = strchr(buf, '{')) != NULL) {
        strcpy(&(replace[0]), &(replace[2]));     
    }
    
    fprintf(fp, buf);
    if (IS_SET(obj->wear_flags, ITEM_CRAFTED)) {
        fprintf(fp, "<td>-</td>\n");
    } else {
        fprintf(fp, "<td>%s</td>\n", obj->area->name);
    }

    fprintf(fp, "<td>");
    
    //deal with armor s/b/p/m stats
    if (obj->item_type == ITEM_ARMOR) {
        fprintf(fp, "%d/%d/%d/%d ", obj->value[0], obj->value[1],obj->value[2],obj->value[3]);
    }
    
    //deal with wands/items
    if (obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF) {
        fprintf(fp, "%d/%d charges of level %d '%s' \n", obj->value[1],obj->value[2], obj->value[0], obj->value[3] != -1 ? skill_table[obj->value[3]].name : "none" );
    } 
    
    //container stats
    if (obj->item_type == ITEM_CONTAINER) {
        fprintf(fp, "%dlbs max, %dlbs per item, %d multiplier \n", obj->value[0],obj->value[3], obj->value[4]);
    }
    
    //light -- show total hours
    if (obj->item_type == ITEM_LIGHT) {
    }
    
    //pills
    if (obj->item_type == ITEM_PILL || obj->item_type == ITEM_SCROLL || obj->item_type == ITEM_POTION) {
        fprintf(fp, "level %d '%s' '%s' '%s' '%s' \n",
                obj->value[0],
                obj->value[1] != -1 ? skill_table[obj->value[1]].name : "none",
                obj->value[2] != -1 ? skill_table[obj->value[2]].name : "none",
                obj->value[3] != -1 ? skill_table[obj->value[3]].name : "none",
                obj->value[4] != -1 ? skill_table[obj->value[4]].name : "none");
    }
    
    //weapons, with noun
    if (obj->item_type == ITEM_WEAPON) {
        fprintf(fp, "%dd%d(%d) ", obj->value[1], obj->value[2], (1 + obj->value[2]) * obj->value[1] / 2);
        fprintf(fp, "[%s] \n", attack_table[obj->value[3]].noun);
        
        if (obj->value[4]) {
            /* weapon flags */
            fprintf(fp, "%s ",weapon_bit_name(obj->value[4]));
        }
    }

    //show each affect
    for (af = obj->affected; af != NULL; af = af->next) {
        fprintf(fp, "%d %s ",  af->modifier, affect_loc_name( af->location ) );
    }

    //extra flags
    if (obj->extra_flags) {
        fprintf(fp, "(");
    }
   
    if (obj->extra_flags > 0) {
        fprintf(fp, "%s ", extra_bit_name(obj->extra_flags ));
    }

    if (obj->extra_flags) {
        fprintf(fp, ")");
    }

    fprintf(fp, "</td>\n");
    fprintf(fp, "</tr>\n");
}


bool item_is_valid(OBJ_INDEX_DATA *obj, int num) {
    bool wearable;

    //basic error checking
    if (obj == NULL) {
        return FALSE;
    }
    
    if (obj->level > 53) {
        return FALSE;
    }
    
    if (!obj->area) {
        return FALSE;
    }
    
    if (obj->area->security != 9) {
        return FALSE;
    }
    
    if (obj->vnum < 100) {
        return FALSE;
    }
    
    if (obj->vnum >= 10600 && obj->vnum < 10850) {
        return FALSE;
    }
    
    if (obj->vnum >= 1250 && obj->vnum < 1300) {
        return FALSE;
    }
    
    if (obj->vnum >= 23000 && obj->vnum < 23500) {
        return FALSE;
    }
    
    if (obj->wear_flags == 0) {
        return FALSE;
    }
    
    if (obj->vnum >= 3723 && obj->vnum <= 3727) {
        return FALSE;
    }
    
    if (obj->vnum >= 1005 && obj->vnum <= 1008) {
        return FALSE;
    }
    
    if (obj->vnum >= 10009 && obj->vnum <= 10015) {
        return FALSE;
    }
    
    if (obj->vnum >= 15311 && obj->vnum <= 15399) {
        return FALSE;
    }
    
    switch (obj->vnum) {
        case 3381:
        case 3382:
        case 3384:
        case 6644:
        case 6717:
        case 6719:
        case 6720:
        case 6721:
        case 6760:
        case 6762:
        case 10025:
        case 10026:
        case 10027:
        case 10029:
        case 10905:
        case 11155:
        case 31109:
        case 31111:
        case 31112:
            return FALSE;
        default:
            ;
    }

    wearable = FALSE;

    switch (num) {
        //giant switch valdated items required fields,
	    //e.g. picking only arms or what not. So if something's
	    //in two lists, e.g. a container AND belt, it'll appear
	    //in both lists.
	
        case 0:
            wearable = CAN_WEAR(obj, ITEM_WEAR_ARMS);
            break;
        case 1:
            wearable = CAN_WEAR(obj, ITEM_WEAR_ABOUT);
            break;
        case 2:
            wearable = CAN_WEAR(obj, ITEM_WEAR_FEET);
            break;
        case 3:
            wearable = CAN_WEAR(obj, ITEM_WEAR_FINGER);
            break;
        case 4:
            wearable = CAN_WEAR(obj, ITEM_WEAR_HANDS);
            break;
        case 5:
            wearable = CAN_WEAR(obj, ITEM_WEAR_HEAD);
            break;
        case 6:
            wearable = CAN_WEAR(obj, ITEM_WEAR_LEGS);
            break;
        case 7:
            wearable = CAN_WEAR(obj, ITEM_WEAR_NECK);
            break;
        case 8:
            wearable = CAN_WEAR(obj, ITEM_WEAR_SHIELD);
            break;
        case 9:
            wearable = CAN_WEAR(obj, ITEM_WEAR_BODY);
            break;
        case 10:
            wearable = CAN_WEAR(obj, ITEM_WEAR_WAIST);
            break;
        case 11:
            wearable = CAN_WEAR(obj, ITEM_WEAR_WRIST);
            break;
        case 12:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_AXE;
            break;
        case 13:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_DAGGER;
            break;
        case 14:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_EXOTIC;
            break;
        case 15:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_FLAIL;
            break;
        case 16:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_MACE;
            break;
        case 17:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_POLEARM;
            break;
        case 18:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_SPEAR;
            break;
        case 19:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_SWORD;
            break;
        case 20:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_WHIP;
            break;
        case 21:
            wearable = obj->item_type == ITEM_THROWING;
            break;
        case 22:
            wearable = obj->item_type == ITEM_BOAT;
            break;
        case 23:
            wearable = CAN_WEAR(obj, ITEM_HOLD) && obj->item_type != ITEM_TRASH && obj->item_type != ITEM_GEM && obj->item_type != ITEM_POTION && obj->item_type != ITEM_CONTAINER && obj->item_type != ITEM_PILL && obj->item_type != ITEM_WARP_STONE && obj->item_type != ITEM_RECALL && obj->item_type != ITEM_WAND && obj->item_type != ITEM_STAFF;
            break;
        case 24:
            wearable = CAN_WEAR(obj, ITEM_WEAR_FLOAT);
            break;
        case 25:
            wearable = obj->item_type == ITEM_LIGHT;
            break;
        case 26:
            wearable = obj->item_type == ITEM_PILL;
            break;
        case 27:
            wearable = obj->item_type == ITEM_POTION;
            break;
        case 28:
            wearable = obj->item_type == ITEM_SCROLL;
            break;
        case 29:
            wearable = obj->item_type == ITEM_STAFF;
            break;
        case 30:
            wearable = obj->item_type == ITEM_WAND;
            break;
        case 31:
            wearable = obj->item_type == ITEM_RECALL;
            break;
        case 32:
            wearable = obj->item_type == ITEM_WARP_STONE;
            break;
        case 33:
            wearable = obj->item_type == ITEM_CONTAINER;
            break;
        case 34:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_KATANA;
            break;
        case 35:
            wearable = CAN_WEAR(obj, ITEM_WIELD) && obj->value[0] == WEAPON_RANGED;
            break;
        default:
            wearable = FALSE;
            break;
    }

    return wearable;
}

