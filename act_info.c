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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "flags.h"
#include "deity.h"
#include "clan.h"
#include "utils.h"


/* command procedures needed */
DECLARE_DO_FUN(do_exits);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_affects);
DECLARE_DO_FUN(do_play);
DECLARE_DO_FUN(do_where);

void removeAllEffects(CHAR_DATA *ch);
int list_npc(int where, BUFFER * output, CHAR_DATA * looker);
void char_line(CHAR_DATA * ch, CHAR_DATA * looker, char *text);
bool is_hushed(CHAR_DATA * ch, CHAR_DATA * vict);

char *const where_name[] =
{
	"<used as light>       ",
	"<worn on finger>      ",
	"<worn on finger>      ",
	"<worn around neck>    ",
	"<worn around neck>    ",
	"<worn on torso>       ",
	"<worn on head>        ",
	"<worn on legs>        ",
	"<worn on feet>        ",
	"<worn on hands>       ",
	"<worn on arms>        ",
	"<worn as shield>      ",
	"<worn about body>     ",
	"<worn about waist>    ",
	"<worn around wrist>   ",
	"<worn around wrist>   ",
	"<wielded>             ",
	"<held>                ",
	"<floating nearby>     ",
	"<orbiting nearby>     ",
	"<worn on other head>  ",
	"<second weapon>       ",
        "<ranged weapon>       ",
	"<ammunition>          ",
};


/* for do_count */
int max_on = 0;


/*
 * Local functions.
 */
char *format_obj_to_char(OBJ_DATA *, CHAR_DATA *, bool);
void show_list_to_char(OBJ_DATA *, CHAR_DATA *, bool, bool);
void show_char_to_char_room(CHAR_DATA *, CHAR_DATA *);
void show_char_look_at_char(CHAR_DATA *, CHAR_DATA *);
void show_char_stats_to_char(CHAR_DATA * victim, CHAR_DATA * ch);
void show_char_to_char(CHAR_DATA *, CHAR_DATA *);
bool check_blind(CHAR_DATA *);
int load_count(void);
void save_count(int);
void lore_obj(CHAR_DATA *ch, OBJ_DATA *obj);
int colorstrlen(char *argument);

// External functions
extern char *flag_string (const struct flag_type *flag_table, int bits);

char *
format_obj_to_char(OBJ_DATA * obj, CHAR_DATA * ch, bool fShort)
{
	static char buf[MAX_STRING_LENGTH];

	buf[0] = '\0';

	if ((fShort && (obj->short_descr == NULL || obj->short_descr[0] == '\0'))
		|| (obj->description == NULL || obj->description[0] == '\0'))
		return buf;

	if (IS_OBJ_STAT(obj, ITEM_INVIS))
		strcat(buf, "(Invis) ");
	if ((is_affected(ch, gsn_true_sight)
	|| IS_AFFECTED(ch, AFF_DETECT_MAGIC))
		&& IS_OBJ_STAT(obj, ITEM_MAGIC))
		strcat(buf, "(Magical) ");
	if (IS_OBJ_STAT(obj, ITEM_GLOW))
		strcat(buf, "(Glowing) ");
	if (IS_OBJ_STAT(obj, ITEM_HUM))
		strcat(buf, "(Humming) ");
	if (affect_find(obj->affected, gsn_wizard_mark))
		strcat(buf, "(Runic) ");
	if (affect_find(obj->affected, gsn_soul_rune))
		strcat(buf, "(Ghostly) ");

	if (fShort)
	{
		if (obj->short_descr != NULL)
			strcat(buf, obj->short_descr);
	}

	else
	{
		if (obj->description != NULL)
			strcat(buf, obj->description);
	}

	if (IS_SET(ch->wiznet, WIZ_DISP) && (ch->level > 55 || ch->trust > 55))
	{
		sprintf_cat(buf, " (%d)", obj->pIndexData->vnum);
	}

	return buf;
}


/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void
show_list_to_char(OBJ_DATA * list, CHAR_DATA * ch, bool fShort, bool fShowNothing)
{
	char buf[MAX_STRING_LENGTH];
	BUFFER *output;
	char **prgpstrShow;
	int *prgnShow;
	char *pstrShow;
	OBJ_DATA *obj;
	int nShow;
	int iShow;
	int count;
	bool fCombine;

	if (ch->desc == NULL)
		return;

	/*
	 * Alloc space for output lines.
	 */
	output = new_buf();

	count = 0;
	for (obj = list; obj != NULL; obj = obj->next_content)
		count++;

	prgpstrShow = (char **) alloc_mem(count * sizeof(char *));
	prgnShow = (int *) alloc_mem(count * sizeof(int));

	nShow = 0;

	/*
	 * Format the list of objects.
	 */
	for (obj = list; obj != NULL; obj = obj->next_content)
	{
		if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj))
		{
			pstrShow = format_obj_to_char(obj, ch, fShort);

			fCombine = FALSE;

			if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE))
			{
				/*
				 * Look for duplicates, case sensitive.
				 * Matches tend to be near end so run loop backwords.
				 */
				for (iShow = nShow - 1; iShow >= 0; iShow--)
				{
					if (!strcmp(prgpstrShow[iShow], pstrShow))
					{
						prgnShow[iShow]++;
						fCombine = TRUE;
						break;
					}
				}
			}

			/*
			 * Couldn't combine, or didn't want to.
			 */
			if (!fCombine)
			{
				prgpstrShow[nShow] = str_dup(pstrShow);
				prgnShow[nShow] = 1;
				nShow++;
			}
		}
	}

	/*
	 * Output the formatted list.
	 */
	for (iShow = 0; iShow < nShow; iShow++)
	{
		if (prgpstrShow[iShow][0] == '\0')
		{
			free_string(prgpstrShow[iShow]);
			continue;
		}

		if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE))
		{
			if (prgnShow[iShow] != 1)
			{
				sprintf(buf, "(%2d) ", prgnShow[iShow]);
				add_buf(output, buf);
			}
			else
			{
				add_buf(output, "     ");
			}
		}
		add_buf(output, prgpstrShow[iShow]);
		add_buf(output, "\n\r");
		free_string(prgpstrShow[iShow]);
	}

	if (fShowNothing && nShow == 0)
	{
		if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE))
			Cprintf(ch, "     ");
		Cprintf(ch, "Nothing.\n\r");
	}
	page_to_char(buf_string(output), ch);

	/*
	 * Clean up.
	 */
	free_buf(output);
	free_mem(prgpstrShow, count * sizeof(char *));
	free_mem(prgnShow, count * sizeof(int));

	return;
}


void
show_char_to_char_room(CHAR_DATA * victim, CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];

	buf[0] = '\0';

	if (is_affected(victim, gsn_shapeshift) &&
		!(!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)))
	{
		Cprintf(ch, "%s", victim->shift_long);
		return;
	}

	if (!IS_NPC(ch)
	&& IS_QUESTING(ch)
	&& (ch->pcdata->quest.type == QUEST_TYPE_VILLAIN
      	|| ch->pcdata->quest.type == QUEST_TYPE_ASSAULT
        || ch->pcdata->quest.type == QUEST_TYPE_RESCUE
	|| ch->pcdata->quest.type == QUEST_TYPE_ESCORT)
	&& IS_NPC(victim)
	&& victim->pIndexData->vnum == ch->pcdata->quest.target)
		strcat(buf, "[TARGET] ");

	if (!IS_NPC(victim) && victim->desc == NULL)
		strcat(buf, "[{RLinkDead{x] " );
	if (IS_SET(victim->comm, COMM_AFK) && !(is_affected(victim, gsn_shapeshift)))
		strcat(buf, "[AFK] ");
	if (IS_SET(victim->wiznet, WIZ_OLC) && !(is_affected(victim, gsn_shapeshift)))
		strcat(buf, "[OLC] ");
	if (IS_AFFECTED(victim, AFF_INVISIBLE))
		strcat(buf, "(Invis) ");
	if (is_affected(victim, gsn_oculary))
		strcat(buf, "(Shimmering) ");
	if ((victim->invis_level >= LEVEL_HERO && !is_affected(victim, gsn_shapeshift)) || (IS_NPC(victim) && IS_SET(victim->act, ACT_IS_WIZI)))
		strcat(buf, "(Wizi) ");
	if (IS_AFFECTED(victim, AFF_HIDE))
		strcat(buf, "(Hiding) ");
	if (IS_AFFECTED(victim, AFF_CHARM))
		strcat(buf, "(Charmed) ");
	if (is_affected(victim, gsn_pass_door))
		strcat(buf, "(Translucent) ");
	if (!IS_NPC(victim) && victim->pcdata->ctf_flag)
	{
		if (victim->pcdata->ctf_team == CTF_BLUE)
			strcat(buf, "(CTF {RFlag{X) ");
		if (victim->pcdata->ctf_team == CTF_RED)
			strcat(buf, "(CTF {BFlag{X) ");
	}

    if (is_affected(victim, gsn_aura)) {
        AFFECT_DATA *paf = NULL;

        for (paf = victim->affected; paf != NULL; paf = paf->next) {
            if (paf->type == gsn_aura) {
                break;
            }
        }

        if (paf != NULL) {
            switch (paf->modifier) {
                case 1:
                    strcat(buf, "({BBlue{x) ");
                    break;
                case 2:
                    strcat(buf, "({RRed{x) ");
                    break;
            }
        }
    }

	if (IS_AFFECTED(victim, AFF_FAERIE_FIRE))
		strcat(buf, "(Pink Aura) ");
	if (IS_AFFECTED(victim, AFF_SANCTUARY))
		strcat(buf, "(White Aura) ");
	if (is_affected(victim, gsn_prismatic_sphere))
		strcat(buf, "({RP{Gr{Bi{Ys{Mm{ra{gt{bi{yc{x Aura) ");
	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER) && !(is_affected(victim, gsn_shapeshift)))
		strcat(buf, "{R(KILLER){x ");
	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF) && !(is_affected(victim, gsn_shapeshift)))
		strcat(buf, "{Y(THIEF){x ");
	if (!IS_NPC(victim) && IS_SET(victim->comm, COMM_LAG) && !(is_affected(victim, gsn_shapeshift)))
		strcat(buf, "{M<LAGGED>{x ");
	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_FREEZE) && !(is_affected(victim, gsn_shapeshift)))
		strcat(buf, "{C[Frozen]{x ");
	if (!IS_NPC(victim) && IS_SET(victim->comm, COMM_NOCHANNELS) && !(is_affected(victim, gsn_shapeshift)))
		strcat(buf, "{G[NoChan]{x ");
	if (victim->position == victim->start_pos && victim->long_descr[0] != '\0')
	{
		if (IS_SET(ch->wiznet, WIZ_DISP) && IS_IMMORTAL(ch) && IS_NPC(victim))
		{
			sprintf_cat(buf, "(%d) ", victim->pIndexData->vnum);
		}


		strcat(buf, victim->long_descr);

		Cprintf(ch, "%s", buf);
		return;
	}

	strcat(buf, PERS(victim, ch));
	if (!IS_NPC(victim) && victim->position == POS_STANDING && ch->on == NULL)
	{
		strcat(buf, victim->pcdata->title);
	}

	switch (victim->position)
	{
	case POS_DEAD:
		strcat(buf, " is DEAD!!");
		break;
	case POS_MORTAL:
		strcat(buf, " is mortally wounded.");
		break;
	case POS_INCAP:
		strcat(buf, " is incapacitated.");
		break;
	case POS_STUNNED:
		strcat(buf, " is lying here stunned.");
		break;
	case POS_SLEEPING:

		if (victim->on != NULL)
		{
			if (IS_SET(victim->on->value[2], SLEEP_AT))
			{
				sprintf_cat(buf, " is sleeping at %s.", victim->on->short_descr);
			}
			else if (IS_SET(victim->on->value[2], SLEEP_ON))
			{
			    sprintf_cat(buf, " is sleeping on %s.", victim->on->short_descr);
			}
			else
			{
			    sprintf_cat(buf, " is sleeping in %s.", victim->on->short_descr);
			}
		}
		else
			strcat(buf, " is sleeping here.");
		break;
	case POS_RESTING:
		if (victim->on != NULL)
		{
			if (IS_SET(victim->on->value[2], REST_AT))
			{
				sprintf_cat(buf, " is resting at %s.", victim->on->short_descr);
			}
			else if (IS_SET(victim->on->value[2], REST_ON))
			{
			    sprintf_cat(buf, " is resting on %s.", victim->on->short_descr);
			}
			else
			{
			    sprintf_cat(buf, " is resting in %s.", victim->on->short_descr);
			}
		}
		else
			strcat(buf, " is resting here.");
		break;
	case POS_SITTING:
		if (victim->on != NULL)
		{
			if (IS_SET(victim->on->value[2], SIT_AT))
			{
			    sprintf_cat(buf, " is sitting at %s.", victim->on->short_descr);
			}
			else if (IS_SET(victim->on->value[2], SIT_ON))
			{
			    sprintf_cat(buf, " is sitting on %s.", victim->on->short_descr);
			}
			else
			{
			    sprintf_cat(buf, " is sitting in %s.", victim->on->short_descr);
			}
		}
		else
			strcat(buf, " is sitting here.");
		break;
	case POS_STANDING:
		if (victim->on != NULL)
		{
			if (IS_SET(victim->on->value[2], STAND_AT))
			{
			    sprintf_cat(buf, " is standing at %s.", victim->on->short_descr);
			}
			else if (IS_SET(victim->on->value[2], STAND_ON))
			{
			    sprintf_cat(buf, " is standing on %s.", victim->on->short_descr);
			}
			else
			{
			    sprintf_cat(buf, " is standing in %s.", victim->on->short_descr);
			}
		}
		else
			strcat(buf, " is here.");
		break;
	case POS_FIGHTING:
		strcat(buf, " is here, fighting ");
		if (victim->fighting == NULL)
			strcat(buf, "thin air??");
		else if (victim->fighting == ch)
			strcat(buf, "YOU!");
		else if (victim->in_room == victim->fighting->in_room)
		{
			strcat(buf, PERS(victim->fighting, ch));
			strcat(buf, ".");
		}
		else
			strcat(buf, "someone who left??");
		break;
	}

	if (IS_SET(ch->wiznet, WIZ_DISP) && IS_IMMORTAL(ch) && IS_NPC(victim))
	{
		sprintf_cat(buf, " (%d)", victim->pIndexData->vnum);
	}

	strcat(buf, "\n\r");
	buf[0] = UPPER(buf[0]);
	Cprintf(ch, "%s", buf);
	return;
}


void
do_lookup(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;

	victim = get_char_world(ch, argument, TRUE);


	if (victim == NULL)
	{
		Cprintf(ch, "You can't find them.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "You can't find them.\n\r");
		return;
	}

	if (!can_see(ch, victim))
	{
		Cprintf(ch, "You can't find them.\n\r");
		return;
	}

        if (is_hushed(victim, ch))
        {
                Cprintf(ch, "They don't want to hear from you, or want you seeing their description.\n\r");
                return;
        }

	if (victim->description[0] != '\0')
		Cprintf(ch, "%s", victim->description);
	else
		act("You see nothing special about $M.", ch, NULL, victim, TO_CHAR, POS_RESTING);

	if (can_see(victim, ch))
		Cprintf(victim, "%s looked up your description.\n\r", ch->name);
}


void
show_char_look_at_char(CHAR_DATA * victim, CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	int iWear;
	int percent;
	bool found;

	if (can_see(victim, ch))
	{
		if (ch == victim)
			act("$n looks at $mself.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		else
		{
			act("$n looks at you.", ch, NULL, victim, TO_VICT, POS_RESTING);
			act("$n looks at $N.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		}
	}

	if (victim->description[0] != '\0')
		Cprintf(ch, "%s", victim->description);
	else
		act("You see nothing special about $M.", ch, NULL, victim, TO_CHAR, POS_RESTING);

	if (MAX_HP(victim) > 0)
		percent = (100 * victim->hit) / MAX_HP(victim);
	else
		percent = -1;

	strcpy(buf, PERS(victim, ch));

	if (IS_AFFECTED(victim, AFF_GRANDEUR))
		strcat(buf, " is in excellent condition.\n\r");
	else if (IS_AFFECTED(victim, AFF_MINIMATION))
		strcat(buf, " is in awful condition.\n\r");
	else
	{
		if (percent >= 100)
			strcat(buf, " is in excellent condition.\n\r");
		else if (percent >= 90)
			strcat(buf, " has a few scratches.\n\r");
		else if (percent >= 75)
			strcat(buf, " has some small wounds and bruises.\n\r");
		else if (percent >= 50)
			strcat(buf, " has quite a few wounds.\n\r");
		else if (percent >= 30)
			strcat(buf, " has some big nasty wounds and scratches.\n\r");
		else if (percent >= 15)
			strcat(buf, " looks pretty hurt.\n\r");
		else if (percent >= 0)
			strcat(buf, " is in awful condition.\n\r");
		else
			strcat(buf, " is bleeding to death.\n\r");
	}
	buf[0] = UPPER(buf[0]);
	Cprintf(ch, "%s", buf);

	found = FALSE;
	for (iWear = 0; iWear < MAX_WEAR; iWear++)
	{
		if ((obj = get_eq_char(victim, iWear)) != NULL
			&& can_see_obj(ch, obj))
		{
			if (!found)
			{
				Cprintf(ch, "\n\r");
				act("$N is using:", ch, NULL, victim, TO_CHAR, POS_RESTING);
				found = TRUE;
			}
			Cprintf(ch, "%s%s\n\r", where_name[iWear], format_obj_to_char(obj, ch, TRUE));
		}
	}

	return;
}


void
show_char_to_char(CHAR_DATA * list, CHAR_DATA * ch)
{
	CHAR_DATA *rch;

	for (rch = list; rch != NULL; rch = rch->next_in_room)
	{
		if (rch == ch)
			continue;

		if (get_trust(ch) < rch->invis_level)
			continue;

		if (can_see(ch, rch))
		{
			show_char_to_char_room(rch, ch);
		}
		else if (room_is_dark(ch->in_room) && IS_AFFECTED(rch, AFF_DARK_VISION))
		{
			show_char_to_char_room(rch, ch);
		}
		else if (room_is_dark(ch->in_room) && IS_AFFECTED(rch, AFF_INFRARED))
		{
			Cprintf(ch, "You see glowing red eyes watching YOU!\n\r");
		}
	}

	return;
}


bool
check_blind(CHAR_DATA * ch)
{

	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
		return TRUE;

	if (IS_AFFECTED(ch, AFF_BLIND))
	{
		Cprintf(ch, "You can't see a thing!\n\r");
		return FALSE;
	}

	return TRUE;
}


void
do_peek(CHAR_DATA * ch, char *argument) {
    CHAR_DATA *victim;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int percent;
    OBJ_DATA *obj;
    OBJ_DATA* container;
    int iWear;
    bool found;

    if (ch->desc == NULL) {
        return;
    }

    if (ch->position < POS_SLEEPING) {
        Cprintf(ch, "You can't see anything but stars!\n\r");
        return;
    }

    if (ch->position == POS_SLEEPING) {
        Cprintf(ch, "You can't see anything, you're sleeping!\n\r");
        return;
    }

    if (!check_blind(ch)) {
        return;
    }

    if (room_is_affected(ch->in_room, gsn_darkness)
            && !IS_AFFECTED(ch, AFF_DARK_VISION)
            && !IS_SET(ch->act, PLR_HOLYLIGHT)
            && !is_affected(ch, gsn_detect_all)
            && number_percent() > 10) {
        Cprintf(ch, "It is pitch black ... \n\r");
        return;
    }

    if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT)
            && room_is_dark(ch->in_room)
            && !IS_AFFECTED(ch, AFF_DARK_VISION)
            && !is_affected(ch, gsn_detect_all)) {
        Cprintf(ch, "It is pitch black ... \n\r");
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    if ((victim = get_char_room(ch, arg1)) == NULL) {
        Cprintf(ch, "That person is not here.\n\r");
        return;
    }

    if (victim == ch) {
        Cprintf(ch, "Yes, very good.  There you are.\n\r");
        return;
    }

    if (!IS_NPC(victim) && !IS_NPC(ch)) {
        if (!IS_IMMORTAL(ch)) {
            if (is_clan(ch) && !is_clan(victim)) {
                Cprintf(ch, "Leave non-clanners alone.\n\r");
                return;
            } else if (!is_clan(ch) && is_clan(victim)) {
                Cprintf(ch, "Leave clanners alone.\n\r");
                return;
            }
        }
    }

    if (number_percent() < get_skill(ch, gsn_peek)) {
        if (ch->reclass == reclass_lookup("rogue")) {
            if ((!str_cmp(arg2, "in") && arg3[0] != '\0') || arg2[0] != '\0') {
                if (!str_cmp(arg2, "in") && arg3[0] != '\0') {
                    if ((container = get_obj_carry(victim, arg3, ch)) == NULL) {
                        if ((container = get_obj_wear(victim, arg3, ch)) == NULL) {
                            Cprintf(ch, "You can't find the container.\n\r");
                            return;
                        }
                    }
                } else {
                    if ((container = get_obj_carry(victim, arg2, ch)) == NULL) {
                        if ((container = get_obj_wear(victim, arg2, ch)) == NULL) {
                            Cprintf(ch, "You can't find the container.\n\r");
                            return;
                        }
                    }
                }

                if (container->item_type != ITEM_CONTAINER &&
                        container->item_type != ITEM_CORPSE_NPC &&
                        container->item_type != ITEM_CORPSE_PC) {
                    Cprintf(ch, "That is not a container.\n\r");
                    return;
                }

                if (IS_SET(container->value[1], CONT_CLOSED)) {
                    Cprintf(ch, "It is closed.\n\r");
                    return;
                }

                act("$p holds:", ch, container, NULL, TO_CHAR, POS_RESTING);
                show_list_to_char(container->contains, ch, TRUE, TRUE);
                check_improve(ch, gsn_peek, TRUE, 4);
                return;
            }
        }

        if (MAX_HP(victim) > 0) {
            percent = (100 * victim->hit) / MAX_HP(victim);
        } else {
            percent = -1;
        }

        strcpy(buf, PERS(victim, ch));

        if (IS_AFFECTED(victim, AFF_GRANDEUR)) {
            strcat(buf, " is in excellent condition.\n\r");
        } else if (IS_AFFECTED(victim, AFF_MINIMATION)) {
            strcat(buf, " is in awful condition.\n\r");
        } else {
            if (percent >= 100) {
                strcat(buf, " is in excellent condition.\n\r");
            } else if (percent >= 90) {
                strcat(buf, " has a few scratches.\n\r");
            } else if (percent >= 75) {
                strcat(buf, " has some small wounds and bruises.\n\r");
            } else if (percent >= 50) {
                strcat(buf, " has quite a few wounds.\n\r");
            } else if (percent >= 30) {
                strcat(buf, " has some big nasty wounds and scratches.\n\r");
            } else if (percent >= 15) {
                strcat(buf, " looks pretty hurt.\n\r");
            } else if (percent >= 0) {
                strcat(buf, " is in awful condition.\n\r");
            } else {
                strcat(buf, " is bleeding to death.\n\r");
            }
        }

        buf[0] = UPPER(buf[0]);

        Cprintf(ch, "%s", buf);

        Cprintf(ch, "\n\rThey are wearing:\n\r");

        found = FALSE;
        for (iWear = 0; iWear < MAX_WEAR; iWear++) {
            if ((obj = get_eq_char(victim, iWear)) == NULL) {
                continue;
            }

            Cprintf(ch, "%s", where_name[iWear]);

            if (can_see_obj(ch, obj)) {
                Cprintf(ch, "%s\n\r", format_obj_to_char(obj, ch, TRUE));
            } else {
                Cprintf(ch, "something.\n\r");
            }

            found = TRUE;
        }

        if (!found) {
            Cprintf(ch, "Nothing.\n\r");
        }

        Cprintf(ch, "\n\rThey are holding:\n\r");

        show_list_to_char(victim->carrying, ch, TRUE, TRUE);

        /* Gold code added by Delstar for Daisy */
        if (victim->gold < 5) {
            Cprintf(ch, "They are very poor.\n\r");
        } else if (victim->gold < 25) {
            Cprintf(ch, "They are rather poor.\n\r");
        } else if (victim->gold < 200) {
            Cprintf(ch, "They are of some means.\n\r");
        } else if (victim->gold < 1000) {
            Cprintf(ch, "They are rather wealthy.\n\r");
        } else if (victim->gold < 2500) {
            Cprintf(ch, "They are pretty rich.\n\r");
        } else {
            Cprintf(ch, "They are filthy rich.\n\r");
        }

        check_improve(ch, gsn_peek, TRUE, 4);
    } else {
        Cprintf(ch, "You cannot seem to catch a glimpse.\n\r");
        act("$n tries to look at what you're carrying!.", ch, NULL, victim, TO_VICT, POS_RESTING);
        check_improve(ch, gsn_peek, FALSE, 4);
    }
}


/* changes your scroll */
void
do_scroll(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	int lines;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		if (ch->lines == 0)
			Cprintf(ch, "You do not page long messages.\n\r");
		else
			Cprintf(ch, "You currently display %d lines per page.\n\r", ch->lines + 2);

		return;
	}

	if (!is_number(arg))
	{
		Cprintf(ch, "You must provide a number.\n\r");
		return;
	}

	lines = atoi(arg);

	if (lines == 0)
	{
		Cprintf(ch, "Paging disabled.\n\r");
		ch->lines = 0;
		return;
	}

	if (lines < 10 || lines > 100)
	{
		Cprintf(ch, "You must provide a reasonable number.\n\r");
		return;
	}

	Cprintf(ch, "Scroll set to %d lines.\n\r", lines);
	ch->lines = lines - 2;
}


/* RT does socials */
void
do_socials(CHAR_DATA * ch, char *argument)
{
	int iSocial;
	int col;

	col = 0;

	for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
	{
		Cprintf(ch, "%-12s", social_table[iSocial].name);
		if (++col % 6 == 0)
			Cprintf(ch, "\n\r");
	}

	if (col % 6 != 0)
		Cprintf(ch, "\n\r");
	return;
}


/* RT Commands to replace news, motd, imotd, etc from ROM */
void
do_motd(CHAR_DATA * ch, char *argument)
{
	do_help(ch, "motd");
}


void
do_imotd(CHAR_DATA * ch, char *argument)
{
	do_help(ch, "imotd");
}


void
do_rules(CHAR_DATA * ch, char *argument)
{
	do_help(ch, "rules");
}


void
do_story(CHAR_DATA * ch, char *argument)
{
	do_help(ch, "story");
}


void
do_wizlist(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;
	do_help(ch, "wizlist");
}


/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */
void
do_autolist(CHAR_DATA * ch, char *argument)
{
	/* lists most player flags */
	if (IS_NPC(ch))
		return;

	Cprintf(ch, "   action     status\n\r");
	Cprintf(ch, "---------------------\n\r");

	Cprintf(ch, "autoplayerassist  %s\n\r", (IS_SET(ch->act, PLR_PLRASSIST)) ? "ON" : "OFF");
	Cprintf(ch, "automobassist     %s\n\r", (IS_SET(ch->act, PLR_MOBASSIST)) ? "ON" : "OFF");
	Cprintf(ch, "autoexit          %s\n\r", (IS_SET(ch->act, PLR_AUTOEXIT)) ? "ON" : "OFF");
	Cprintf(ch, "autogold          %s\n\r", (IS_SET(ch->act, PLR_AUTOGOLD)) ? "ON" : "OFF");
	Cprintf(ch, "autoloot          %s\n\r", (IS_SET(ch->act, PLR_AUTOLOOT)) ? "ON" : "OFF");
	Cprintf(ch, "autosac           %s\n\r", (IS_SET(ch->act, PLR_AUTOSAC)) ? "ON" : "OFF");
	Cprintf(ch, "autotitle         %s\n\r", (IS_SET(ch->act, PLR_AUTOTITLE)) ? "ON" : "OFF");
	Cprintf(ch, "autotrash         %s\n\r", (IS_SET(ch->act, PLR_AUTOTRASH)) ? "ON" : "OFF");
	Cprintf(ch, "autosplit         %s\n\r", (IS_SET(ch->act, PLR_AUTOSPLIT)) ? "ON" : "OFF");
	Cprintf(ch, "notify            %s\n\r", (IS_SET(ch->act, PLR_NOTIFY)) ? "ON" : "OFF");
	Cprintf(ch, "compact mode      %s\n\r", (IS_SET(ch->comm, COMM_COMPACT)) ? "ON" : "OFF");
	Cprintf(ch, "prompt            %s\n\r", (IS_SET(ch->comm, COMM_PROMPT)) ? "ON" : "OFF");
	Cprintf(ch, "autocombine items     %s\n\r", (IS_SET(ch->comm, COMM_COMBINE)) ? "ON" : "OFF");
	Cprintf(ch, "linkdead void     %s\n\r", (IS_SET(ch->toggles, TOGGLES_LINKDEAD)) ? "ON" : "OFF");

	if (!IS_SET(ch->act, PLR_CANLOOT))
		Cprintf(ch, "Your corpse is safe from thieves.\n\r");
	else
		Cprintf(ch, "Your corpse may be looted.\n\r");

	if (IS_SET(ch->act, PLR_NOSUMMON))
		Cprintf(ch, "You cannot be summoned.\n\r");
	else
		Cprintf(ch, "You can be summoned.\n\r");

	if (IS_SET(ch->act, PLR_NOCAN))
		Cprintf(ch, "You cannot be cancelled.\n\r");
	else
		Cprintf(ch, "You can be cancelled.\n\r");

	if (IS_SET(ch->act, PLR_NORECALL))
		Cprintf(ch, "You do not recall out of combat when link dead.\n\r");
	else
		Cprintf(ch, "You do recall out of combat when link dead.\n\r");

	if (IS_SET(ch->act, PLR_NOFOLLOW))
		Cprintf(ch, "You do not welcome followers.\n\r");
	else
		Cprintf(ch, "You accept followers.\n\r");
}


void
do_autoassist(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_MOBASSIST) ||
		IS_SET(ch->act, PLR_PLRASSIST))
	{
		Cprintf(ch, "Player and mob autoassist removed.\n\r");
		REMOVE_BIT(ch->act, PLR_MOBASSIST);
		REMOVE_BIT(ch->act, PLR_PLRASSIST);
	}
	else
	{
		Cprintf(ch, "You will now assist against players and mobs when needed.\n\r");
		SET_BIT(ch->act, PLR_MOBASSIST);
		SET_BIT(ch->act, PLR_PLRASSIST);
	}
}

void
do_playerassist(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_PLRASSIST))
	{
		Cprintf(ch, "Player autoassist removed.\n\r");
		REMOVE_BIT(ch->act, PLR_PLRASSIST);
	}
	else
	{
		Cprintf(ch, "You will now assist against players when needed.\n\r");
		SET_BIT(ch->act, PLR_PLRASSIST);
	}
}

void
do_mobassist(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_MOBASSIST))
	{
		Cprintf(ch, "Mob autoassist removed.\n\r");
		REMOVE_BIT(ch->act, PLR_MOBASSIST);
	}
	else
	{
		Cprintf(ch, "You will now assist against mobs when needed.\n\r");
		SET_BIT(ch->act, PLR_MOBASSIST);
	}
}

void
do_autoexit(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOEXIT))
	{
		Cprintf(ch, "Exits will no longer be displayed.\n\r");
		REMOVE_BIT(ch->act, PLR_AUTOEXIT);
	}
	else
	{
		Cprintf(ch, "Exits will now be displayed.\n\r");
		SET_BIT(ch->act, PLR_AUTOEXIT);
	}
}


void
do_autogold(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOGOLD))
	{
		Cprintf(ch, "Autogold removed.\n\r");
		REMOVE_BIT(ch->act, PLR_AUTOGOLD);
	}
	else
	{
		Cprintf(ch, "Automatic gold looting set.\n\r");
		SET_BIT(ch->act, PLR_AUTOGOLD);
	}
}


void
do_autoloot(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOLOOT))
	{
		Cprintf(ch, "Autolooting removed.\n\r");
		REMOVE_BIT(ch->act, PLR_AUTOLOOT);
	}
	else
	{
		Cprintf(ch, "Automatic corpse looting set.\n\r");
		SET_BIT(ch->act, PLR_AUTOLOOT);
	}
}


void
do_norecall(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_NORECALL))
	{
		Cprintf(ch, "You will now recall.\n\r");
		REMOVE_BIT(ch->act, PLR_NORECALL);
	}
	else
	{
		Cprintf(ch, "You will no longer recall linkdead.\n\r");
		SET_BIT(ch->act, PLR_NORECALL);
	}
}


void
do_linkdead(CHAR_DATA * ch, char *argument)
{

	if (IS_SET(ch->toggles, TOGGLES_LINKDEAD))
	{
		Cprintf(ch, "You will no longer flee to limbo when link dead.\n\r");
		REMOVE_BIT(ch->toggles, TOGGLES_LINKDEAD);
	}
	else
	{
		Cprintf(ch, "You will now flee to limbo when link dead.\n\r");
		SET_BIT(ch->toggles, TOGGLES_LINKDEAD);
	}

}


void
do_autovalue(CHAR_DATA * ch, char *argument)
{

	if (IS_SET(ch->toggles, TOGGLES_AUTOVALUE))
	{
		Cprintf(ch, "OLC will no longer supply values for your mobs.\n\r");
		REMOVE_BIT(ch->toggles, TOGGLES_AUTOVALUE);
	}
	else
	{
		Cprintf(ch, "OLC will now enter default mob values based on level.\n\r");
		SET_BIT(ch->toggles, TOGGLES_AUTOVALUE);
	}

}


void
do_sound(CHAR_DATA * ch, char *argument)
{
	if (IS_SET(ch->toggles, TOGGLES_SOUND))
	{
		Cprintf(ch, "MSP Sounds are now turned off.\n\r");
		REMOVE_BIT(ch->toggles, TOGGLES_SOUND);
	}
	else
	{
		Cprintf(ch, "MSP Sounds are now ON!\n\r!!SOUND(sounds/wav/yahoo.wav V=80 P=20 T=admin)");
		SET_BIT(ch->toggles, TOGGLES_SOUND);
	}
}


void
do_nocancel(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_NOCAN))
	{
		Cprintf(ch, "You are no longer immune to cancellation.\n\r");
		REMOVE_BIT(ch->act, PLR_NOCAN);
	}
	else
	{
		Cprintf(ch, "You are now immune to cancellation.\n\r");
		SET_BIT(ch->act, PLR_NOCAN);
	}
}


void
do_autosac(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOSAC))
	{
		Cprintf(ch, "Autosacrificing removed.\n\r");
		REMOVE_BIT(ch->act, PLR_AUTOSAC);
	}
	else
	{
		Cprintf(ch, "Automatic corpse sacrificing set.\n\r");
		SET_BIT(ch->act, PLR_AUTOSAC);
	}
}


void
do_autotrash(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOTRASH))
	{
		Cprintf(ch, "Autotrashing removed.\n\r");
		REMOVE_BIT(ch->act, PLR_AUTOTRASH);
	}
	else
	{
		Cprintf(ch, "Automatic corpse trashing set.\n\r");
		SET_BIT(ch->act, PLR_AUTOTRASH);
	}
}


void
do_autosplit(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOSPLIT))
	{
		Cprintf(ch, "Autosplitting removed.\n\r");
		REMOVE_BIT(ch->act, PLR_AUTOSPLIT);
	}
	else
	{
		Cprintf(ch, "Automatic gold splitting set.\n\r");
		SET_BIT(ch->act, PLR_AUTOSPLIT);
	}
}


void
do_brief(CHAR_DATA * ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_BRIEF))
	{
		Cprintf(ch, "Full descriptions activated.\n\r");
		REMOVE_BIT(ch->comm, COMM_BRIEF);
	}
	else
	{
		Cprintf(ch, "Short descriptions activated.\n\r");
		SET_BIT(ch->comm, COMM_BRIEF);
	}
}


void
do_compact(CHAR_DATA * ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_COMPACT))
	{
		Cprintf(ch, "Compact mode removed.\n\r");
		REMOVE_BIT(ch->comm, COMM_COMPACT);
	}
	else
	{
		Cprintf(ch, "Compact mode set.\n\r");
		SET_BIT(ch->comm, COMM_COMPACT);
	}
}


void
do_show(CHAR_DATA * ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_SHOW_AFFECTS))
	{
		Cprintf(ch, "Affects will no longer be shown in score.\n\r");
		REMOVE_BIT(ch->comm, COMM_SHOW_AFFECTS);
	}
	else
	{
		Cprintf(ch, "Affects will now be shown in score.\n\r");
		SET_BIT(ch->comm, COMM_SHOW_AFFECTS);
	}
}


void
do_prompt(CHAR_DATA * ch, char *argument)
{
	const unsigned int maxPromptLen = 120;
	char buf[MAX_STRING_LENGTH];

	if (argument[0] == '\0')
	{
		if (IS_SET(ch->comm, COMM_PROMPT))
		{
			Cprintf(ch, "You will no longer see prompts.\n\r");
			REMOVE_BIT(ch->comm, COMM_PROMPT);
		}
		else
		{
			Cprintf(ch, "You will now see prompts.\n\r");
			SET_BIT(ch->comm, COMM_PROMPT);
		}
		return;
	}

	if (!strcmp(argument, "all"))
		strcpy(buf, "{c<%hhp %mm %vmv>{x ");
	else
	{
		if (strlen(argument) > maxPromptLen)
			argument[maxPromptLen] = '\0';
		strcpy(buf, argument);
		smash_tilde(buf);
		if (str_suffix("%c", buf))
			strcat(buf, " ");

	}

	free_string(ch->prompt);
	ch->prompt = str_dup(buf);
	Cprintf(ch, "Prompt set to %s\n\r", ch->prompt);
	return;
}


void
do_combine(CHAR_DATA * ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_COMBINE))
	{
		Cprintf(ch, "Long inventory selected.\n\r");
		REMOVE_BIT(ch->comm, COMM_COMBINE);
	}
	else
	{
		Cprintf(ch, "Combined inventory selected.\n\r");
		SET_BIT(ch->comm, COMM_COMBINE);
	}
}


void
do_noloot(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_CANLOOT))
	{
		Cprintf(ch, "Your corpse is now safe from thieves.\n\r");
		REMOVE_BIT(ch->act, PLR_CANLOOT);
	}
	else
	{
		Cprintf(ch, "Your corpse may now be looted.\n\r");
		SET_BIT(ch->act, PLR_CANLOOT);
	}
}


void
do_nofollow(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_AFFECTED(ch, AFF_CHARM))
	{
		Cprintf(ch, "Your master wouldn't like that.\n\r");
		return;
	}
	if (IS_SET(ch->act, PLR_NOFOLLOW))
	{
		Cprintf(ch, "You now accept followers.\n\r");
		REMOVE_BIT(ch->act, PLR_NOFOLLOW);
	}
	else
	{
		Cprintf(ch, "You no longer accept followers.\n\r");
		SET_BIT(ch->act, PLR_NOFOLLOW);
		die_follower(ch);
	}
}

void
do_nosummo(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "You must type the full command.\n\r");
	return;
}

void
do_nosummon(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
	{
		if (IS_SET(ch->imm_flags, IMM_SUMMON))
		{
			Cprintf(ch, "You are no longer immune to summon.\n\r");
			REMOVE_BIT(ch->imm_flags, IMM_SUMMON);
		}
		else
		{
			Cprintf(ch, "You are now immune to summoning.\n\r");
			SET_BIT(ch->imm_flags, IMM_SUMMON);
		}
	}
	else
	{
		if (IS_SET(ch->act, PLR_NOSUMMON))
		{
			Cprintf(ch, "You are no longer immune to summon.\n\r");
			REMOVE_BIT(ch->act, PLR_NOSUMMON);
		}
		else
		{
			Cprintf(ch, "You are now immune to summoning.\n\r");
			SET_BIT(ch->act, PLR_NOSUMMON);
		}
	}
}

void
do_autotitle(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOTITLE))
	{
		Cprintf(ch, "Autotitles removed.\n\r");
		REMOVE_BIT(ch->act, PLR_AUTOTITLE);
	}
	else
	{
		Cprintf(ch, "Automatic titles upon new levels set.\n\r");
		SET_BIT(ch->act, PLR_AUTOTITLE);
	}
}

void
do_notify(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_NOTIFY))
	{
		Cprintf(ch, "Notify removed.\n\r");
		REMOVE_BIT(ch->act, PLR_NOTIFY);
	}
	else
	{
		Cprintf(ch, "Notification of new notes set.\n\r");
		SET_BIT(ch->act, PLR_NOTIFY);
	}
}


void
do_look(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	EXIT_DATA *pexit;
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	char *pdesc;
	int door;
	int number, count;

	if (ch->desc == NULL)
		return;

	if (ch->position < POS_SLEEPING)
	{
		Cprintf(ch, "You can't see anything but stars!\n\r");
		return;
	}

	if (ch->position == POS_SLEEPING)
	{
		Cprintf(ch, "You can't see anything, you're sleeping!\n\r");
		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	number = number_argument(arg1, arg3);
	count = 0;

	if (!str_cmp(arg1, "eye"))
	{
		ROOM_INDEX_DATA* to_room;
		ROOM_INDEX_DATA* back_room;
		AFFECT_DATA* paf;

		if ((paf = affect_find(ch->affected, gsn_wizards_eye)) == NULL)
		{
			Cprintf(ch, "You don't have a wizards eye to look through.\n\r");
			return;
		}

		/* char to room, look, char back */
		back_room = ch->in_room;
		if ((to_room = get_room_index(paf->modifier)) == NULL)
		{
			return;
		}

		ch->in_room = to_room;

		do_where(ch, "");

		ch->in_room = back_room;

		return;
	}

	if (!check_blind(ch))
		return;

	if (room_is_affected(ch->in_room, gsn_darkness)
		&& !IS_AFFECTED(ch, AFF_DARK_VISION)
		&& !IS_SET(ch->act, PLR_HOLYLIGHT)
		&& !is_affected(ch, gsn_detect_all)
		&& number_percent() > 10)
	{
		Cprintf(ch, "It is pitch black ... \n\r");
		show_char_to_char(ch->in_room->people, ch);
		return;
	}

	if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT) && room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_DARK_VISION) && !is_affected(ch, gsn_detect_all))
	{
		Cprintf(ch, "It is pitch black ... \n\r");
		show_char_to_char(ch->in_room->people, ch);
		return;
	}

	if (arg1[0] == '\0' || !str_cmp(arg1, "auto"))
	{
		/* 'look' or 'look auto' */
		if (room_is_affected(ch->in_room, gsn_winter_storm))
		{
			Cprintf(ch, "A raging winter storm.");
		}
		else
		{
			Cprintf(ch, "%s", ch->in_room->name);
		}

		if (IS_SET(ch->toggles, TOGGLES_SOUND) && ch->in_room->sound != NULL)
		{
			if (ch->in_room->sound[0] != '\0')
			{
				if (ch->in_room->sound[0] == 'm')
				{
					sprintf(buf, "!!SOUND(sounds/%s V=40 P=10 T=admin)", ch->in_room->sound);
				}
				else
				{
					sprintf(buf, "!!SOUND(sounds/%s V=40 P=20 T=admin)", ch->in_room->sound);
				}

				Cprintf(ch, "%s", buf);
			}
		}

		if (IS_IMMORTAL(ch) && (IS_NPC(ch) || IS_SET(ch->act, PLR_HOLYLIGHT)))
			Cprintf(ch, " [Room %d]", ch->in_room->vnum);

		Cprintf(ch, "\n\r");

		if (arg1[0] == '\0' || (!IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF)))
		{
			Cprintf(ch, "  ");
			if (room_is_affected(ch->in_room, gsn_winter_storm))
			{
				Cprintf(ch, "You are lost in a blinding white hail of snow!");
			}
			else
			{
				Cprintf(ch, "%s", ch->in_room->description);
			}
		}

		if (!IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT))
		{
			Cprintf(ch, "\n\r");
			do_exits(ch, "auto");
		}

		if (room_is_affected(ch->in_room, gsn_flood))
        		Cprintf(ch, "The room is submerged beneath a flood of water!\n\r");

		if (room_is_affected(ch->in_room, gsn_build_fire))
			Cprintf(ch, "A beautiful camp fire warms you up.\n\r");

		if (room_is_affected(ch->in_room, gsn_oracle))
			Cprintf(ch, "An oracle infuses your magical senses.\n\r");

		if (room_is_affected(ch->in_room, gsn_hailstorm))
			Cprintf(ch, "A hailstorm pounds in this room!\n\r");

		if (room_is_affected(ch->in_room, gsn_rain_of_tears))
			Cprintf(ch, "A rain of tears drenches the room!\n\r");

		if (room_is_affected(ch->in_room, gsn_web))
			Cprintf(ch, "A sliver's web hangs all around the room.\n\r");

		if (room_is_affected(ch->in_room, gsn_pentagram))
			Cprintf(ch, "A faint chalk outline of a pentagram lies on the floor.\n\r");

		if (room_is_affected(ch->in_room, gsn_lair))
		{
			Cprintf(ch, "A dragon has set up their lair in this room.\n\r");
		}

		if (room_is_affected(ch->in_room, gsn_darkness))
		{
			Cprintf(ch, "A cloud of inpenetrable darkness covers the room!\n\r");
		}
		if (room_is_affected(ch->in_room, gsn_cloudkill))
		{
			Cprintf(ch, "A cloud of chlorine gas covers everything!\n\r");
		}

		if (room_is_affected(ch->in_room, gsn_earth_to_mud))
		{
			Cprintf(ch, "The ground has turned to sloppy mud here.\n\r");
		}

		if (room_is_affected(ch->in_room, gsn_repel)) {
			Cprintf(ch, "A shimmering mist protects this room from entry.\n\r");
		}

		if (room_is_affected(ch->in_room, gsn_abandon)) {
			Cprintf(ch, "A translucent space time anchor hovers here.\n\r");
		}

		if (room_is_affected(ch->in_room, gsn_whirlpool)) {
                        Cprintf(ch, "The water churns into a powerful whirlpool vortex here.\n\r");
		}
		if (room_is_affected(ch->in_room, gsn_tornado)) {
                        Cprintf(ch, "A roaring tornado fills the room!\n\r");
		}
		if (room_is_affected(ch->in_room, gsn_cave_in)) {
			Cprintf(ch, "A pile of rubble makes walking difficult.\n\r");
		}
		if (room_is_affected(ch->in_room, gsn_shackle_rune)) {
			Cprintf(ch, "A mystical rune empowers the room with energy.\n\r");
		}
		if (room_is_affected(ch->in_room, gsn_fire_rune)) {
			Cprintf(ch, "A mystical rune empowers the room with energy.\n\r");
		}
		if (room_is_affected(ch->in_room, gsn_alarm_rune)) {
			Cprintf(ch, "A mystical rune empowers the room with energy.\n\r");
		}

		if (room_is_affected(ch->in_room, gsn_wizards_eye)) {
			Cprintf(ch, "You can barely make out a blinking wizard eye here.\n\r");
		}

		if (room_is_affected(ch->in_room, gsn_beacon)) {
			Cprintf(ch, "A glowing beacon covers the floor.\n\r");
		}

		show_list_to_char(ch->in_room->contents, ch, FALSE, FALSE);
		show_char_to_char(ch->in_room->people, ch);
		return;
	}

	if (!str_cmp(arg1, "i") || !str_cmp(arg1, "in") || !str_cmp(arg1, "on"))
	{
		/* 'look in' */
		if (arg2[0] == '\0')
		{
			Cprintf(ch, "Look in what?\n\r");
			return;
		}

		if ((obj = get_obj_here(ch, arg2)) == NULL)
		{
			Cprintf(ch, "You do not see that here.\n\r");
			return;
		}

		switch (obj->item_type)
		{
		default:
			Cprintf(ch, "That is not a container.\n\r");
			break;

		case ITEM_DRINK_CON:
			if (obj->value[1] <= 0)
			{
				Cprintf(ch, "It is empty.\n\r");
				break;
			}

			Cprintf(ch, "It's %sfilled with  a %s liquid.\n\r",
					obj->value[1] < obj->value[0] / 4
					? "less than half-" :
					obj->value[1] < 3 * obj->value[0] / 4
					? "about half-" : "more than half-",
					liq_table[obj->value[2]].liq_color);

			break;

		case ITEM_FURNITURE:
			if (!IS_SET(obj->value[2], PUT_ON))
			{
				Cprintf(ch, "You can't put anyting on that.\n\r");
				break;
			}
			act("$p has on it:", ch, obj, NULL, TO_CHAR, POS_RESTING);
			show_list_to_char(obj->contains, ch, TRUE, TRUE);
			break;

		case ITEM_CONTAINER:
		case ITEM_CORPSE_NPC:
		case ITEM_CORPSE_PC:
			if (IS_SET(obj->value[1], CONT_CLOSED))
			{
				Cprintf(ch, "It is closed.\n\r");
				break;
			}

			act("$p holds:", ch, obj, NULL, TO_CHAR, POS_RESTING);
			show_list_to_char(obj->contains, ch, TRUE, TRUE);
			break;

		case ITEM_SHEATH:
			act("$p holds:", ch, obj, NULL, TO_CHAR, POS_RESTING);
			show_list_to_char(obj->contains, ch, TRUE, TRUE);
			break;
		}
		return;
	}

	if ((victim = get_char_room(ch, arg1)) != NULL)
	{
		show_char_look_at_char(victim, ch);
		return;
	}

	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	{
		if (can_see_obj(ch, obj))
		{						/* player can see object */
			pdesc = get_extra_descr(arg3, obj->extra_descr);
			if (pdesc != NULL)
			{
				if (++count == number)
				{
					Cprintf(ch, "%s", pdesc);
					return;
				}
				else
					continue;
			}

			pdesc = get_extra_descr(arg3, obj->pIndexData->extra_descr);
			if (pdesc != NULL)
			{
				if (++count == number)
				{
					Cprintf(ch, "%s", pdesc);
					return;
				}
				else
					continue;
			}

			if (is_name(arg3, obj->name))
				if (++count == number)
				{
					Cprintf(ch, "%s", obj->description);
					Cprintf(ch, "\n\r");
					if(obj->pIndexData->vnum == OBJ_VNUM_BANK_NOTE)
					{
						/* do some funky calcs */
						Cprintf(ch, "There are %d gold and %d silver pieces on this note.\n\r", obj->value[2] / 100, obj->value[2] - (obj->value[2] / 100) * 100);
					}
					return;
				}
		}
	}

	for (obj = ch->in_room->contents; obj != NULL; obj = obj->next_content)
	{
		if (can_see_obj(ch, obj))
		{
			pdesc = get_extra_descr(arg3, obj->extra_descr);
			if (pdesc != NULL)
				if (++count == number)
				{
					Cprintf(ch, "%s", pdesc);
					return;
				}

			pdesc = get_extra_descr(arg3, obj->pIndexData->extra_descr);
			if (pdesc != NULL)
				if (++count == number)
				{
					Cprintf(ch, "%s", pdesc);
					return;
				}

			if (is_name(arg3, obj->name))
				if (++count == number)
				{
					Cprintf(ch, "%s", obj->description);
					Cprintf(ch, "\n\r");
					return;
				}
		}
	}

	pdesc = get_extra_descr(arg3, ch->in_room->extra_descr);
	if (pdesc != NULL)
	{
		if (++count == number)
		{
			Cprintf(ch, "%s", pdesc);
			return;
		}
	}

	if (count > 0 && count != number)
	{
		if (count == 1)
			sprintf(buf, "You only see one %s here.\n\r", arg3);
		else
			sprintf(buf, "You only see %d of those here.\n\r", count);

		Cprintf(ch, "%s", buf);
		return;
	}

	if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north"))
		door = 0;
	else if (!str_cmp(arg1, "e") || !str_cmp(arg1, "east"))
		door = 1;
	else if (!str_cmp(arg1, "s") || !str_cmp(arg1, "south"))
		door = 2;
	else if (!str_cmp(arg1, "w") || !str_cmp(arg1, "west"))
		door = 3;
	else if (!str_cmp(arg1, "u") || !str_cmp(arg1, "up"))
		door = 4;
	else if (!str_cmp(arg1, "d") || !str_cmp(arg1, "down"))
		door = 5;
	else
	{
		Cprintf(ch, "You do not see that here.\n\r");
		return;
	}

	/* 'look direction' */
	if ((pexit = ch->in_room->exit[door]) == NULL)
	{
		Cprintf(ch, "Nothing special there.\n\r");
		return;
	}

	if (pexit->description != NULL && pexit->description[0] != '\0')
		Cprintf(ch, "%s", pexit->description);
	else
		Cprintf(ch, "Nothing special there.\n\r");

	if (pexit->keyword != NULL && pexit->keyword[0] != '\0' && pexit->keyword[0] != ' ')
	{
		if (IS_SET(pexit->exit_info, EX_CLOSED))
			act("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR, POS_RESTING);
		else if (IS_SET(pexit->exit_info, EX_ISDOOR))
			act("The $d is open.", ch, NULL, pexit->keyword, TO_CHAR, POS_RESTING);
	}

	return;
}


void
do_peer(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	EXIT_DATA *pexit;
	int door;
	int number, count;

	if (ch->desc == NULL)
		return;

	if (ch->position < POS_SLEEPING)
	{
		Cprintf(ch, "You can't see anything but stars!\n\r");
		return;
	}

	if (ch->position == POS_SLEEPING)
	{
		Cprintf(ch, "You can't see anything, you're sleeping!\n\r");
		return;
	}

	if (!check_blind(ch))
	{
		Cprintf(ch, "And how do you imagine you can do that while being blind?\n\r");
		return;
	}

	if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT) && room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_DARK_VISION))
	{
		Cprintf(ch, "It is pitch black ... \n\r");
		show_char_to_char(ch->in_room->people, ch);
		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	number = number_argument(arg1, arg3);
	count = 0;

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Peer in which direction?\n\r");
		return;
	}


	if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north"))
		door = 0;
	else if (!str_cmp(arg1, "e") || !str_cmp(arg1, "east"))
		door = 1;
	else if (!str_cmp(arg1, "s") || !str_cmp(arg1, "south"))
		door = 2;
	else if (!str_cmp(arg1, "w") || !str_cmp(arg1, "west"))
		door = 3;
	else if (!str_cmp(arg1, "u") || !str_cmp(arg1, "up"))
		door = 4;
	else if (!str_cmp(arg1, "d") || !str_cmp(arg1, "down"))
		door = 5;
	else
	{
		Cprintf(ch, "This is not a direction.\n\r");
		return;
	}

	pexit = ch->in_room->exit[door];

	if (pexit == NULL)
	{
		Cprintf(ch, "There is no room in that direction.\n\r");
		return;
	}

	if (IS_SET(pexit->exit_info, EX_CLOSED))
	{
		act("The $d is closed. You can't see anything in there.", ch, NULL, pexit->keyword, TO_CHAR, POS_RESTING);
		return;
	}

	if (pexit->u1.to_room == NULL)
	{
		Cprintf(ch, "You can't see in that room.\n\r");
		return;
	}

	Cprintf(ch, "You peer intensely and see...\n\r\n\r");
	Cprintf(ch, "%s\n\r", pexit->u1.to_room->name);
	Cprintf(ch, "%s\n\r", pexit->u1.to_room->description);

	if (room_is_affected(pexit->u1.to_room, gsn_flood))
                Cprintf(ch, "The room is submerged beneath a flood of water!\n\r");
	if (room_is_affected(pexit->u1.to_room, gsn_build_fire))
		Cprintf(ch, "A beautiful camp fire warms you up.\n\r");

	if (room_is_affected(pexit->u1.to_room, gsn_oracle))
		Cprintf(ch, "An oracle infuses your magical senses.\n\r");

	if (room_is_affected(pexit->u1.to_room, gsn_web))
		Cprintf(ch, "A sliver's web hangs all around the room.\n\r");

		if (room_is_affected(pexit->u1.to_room, gsn_pentagram))
			Cprintf(ch, "A faint chalk outline of a pentagram lies on the floor.\n\r");

		if (room_is_affected(pexit->u1.to_room, gsn_lair))
		{
			Cprintf(ch, "A dragon has set up their lair in this room.\n\r");
		}

		if (room_is_affected(pexit->u1.to_room, gsn_darkness))
		{
			Cprintf(ch, "A cloud of inpenetrable darkness covers the room!\n\r");
		}
		if (room_is_affected(pexit->u1.to_room, gsn_cloudkill))
		{
			Cprintf(ch, "A cloud of chlorine gas covers everything!\n\r");
		}

		if (room_is_affected(pexit->u1.to_room, gsn_earth_to_mud))
		{
			Cprintf(ch, "The ground has turned to sloppy mud here.\n\r");
		}

		if (room_is_affected(pexit->u1.to_room, gsn_repel)) {
			Cprintf(ch, "A shimmering mist protects this room from entry.\n\r");
		}

		if (room_is_affected(pexit->u1.to_room, gsn_abandon)) {
			Cprintf(ch, "A translucent space time anchor hovers here.\n\r");
		}

		if (room_is_affected(pexit->u1.to_room, gsn_whirlpool)) {
                        Cprintf(ch, "The water churns into a powerful whirlpool vortex here.\n\r");
		}
		if (room_is_affected(pexit->u1.to_room, gsn_tornado)) {
                        Cprintf(ch, "A roaring tornado fills the room!\n\r");
		}
		if (room_is_affected(pexit->u1.to_room, gsn_cave_in)) {
			Cprintf(ch, "A pile of rubble makes walking difficult.\n\r");
		}

		if (room_is_affected(pexit->u1.to_room, gsn_shackle_rune)) {
                        Cprintf(ch, "A mystical rune empowers the room with energy.\n\r");
                }
                if (room_is_affected(pexit->u1.to_room, gsn_fire_rune)) {
                        Cprintf(ch, "A mystical rune empowers the room with energy.\n\r");
                }
                if (room_is_affected(pexit->u1.to_room, gsn_alarm_rune)) {
                        Cprintf(ch, "A mystical rune empowers the room with energy.\n\r");
                }

                if (room_is_affected(pexit->u1.to_room, gsn_wizards_eye)) {
                        Cprintf(ch, "You can barely make out a blinking wizard eye here.\n\r");
                }

                if (room_is_affected(pexit->u1.to_room, gsn_beacon)) {
                        Cprintf(ch, "A glowing beacon covers the floor.\n\r");
                }

	show_list_to_char(pexit->u1.to_room->contents, ch, FALSE, FALSE);
	show_char_to_char(pexit->u1.to_room->people, ch);

	return;

}


/* RT added back for the hell of it */
void
do_read(CHAR_DATA * ch, char *argument)
{
	do_look(ch, argument);
}


void
do_examine(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	bool partial;
	bool frozen;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Examine what?\n\r");
		return;
	}

	do_look(ch, arg);

	if ((obj = get_obj_here(ch, arg)) != NULL)
	{
		switch (obj->item_type)
		{
		default:
			break;

		case ITEM_SLOT_MACHINE:
			if (obj->value[3] == 1)
				partial = TRUE;
			else
				partial = FALSE;
			if (obj->value[4] == 1)
				frozen = TRUE;
			else
				frozen = FALSE;

			Cprintf(ch, "This is a %d bar slot machine costing %d gold with a jackpot of %d gold coins.\n\rThe jackpot is %sfrozen with %spartial winnings.", obj->value[2], obj->value[0], obj->value[1], frozen ? "" : "not ", partial ? "" : "no ");
			break;

		case ITEM_JUKEBOX:
			do_play(ch, "list");
			break;

		case ITEM_MONEY:
			if (obj->value[0] == 0)
			{
				if (obj->value[1] == 0)
					sprintf(buf, "Odd...there's no coins in the pile.\n\r");
				else if (obj->value[1] == 1)
					sprintf(buf, "Wow. One gold coin.\n\r");
				else
					sprintf(buf, "There are %d gold coins in the pile.\n\r", obj->value[1]);
			}
			else if (obj->value[1] == 0)
			{
				if (obj->value[0] == 1)
					sprintf(buf, "Wow. One silver coin.\n\r");
				else
					sprintf(buf, "There are %d silver coins in the pile.\n\r", obj->value[0]);
			}
			else
				sprintf(buf, "There are %d gold and %d silver coins in the pile.\n\r", obj->value[1], obj->value[0]);

			Cprintf(ch, "%s", buf);
			break;

		case ITEM_DRINK_CON:
		case ITEM_SHEATH:
		case ITEM_CONTAINER:
		case ITEM_CORPSE_NPC:
		case ITEM_CORPSE_PC:
			sprintf(buf, "in %s", argument);
			do_look(ch, buf);
		}
	}

	return;
}


/*
 * Thanks to Zrin for auto-exit part.
 */
void
do_exits(CHAR_DATA * ch, char *argument)
{
	extern char *const dir_name[];
	char buf[MAX_STRING_LENGTH];
	EXIT_DATA *pexit;
	bool found;
	bool fAuto;
	int door;

	fAuto = !str_cmp(argument, "auto");

	if (!check_blind(ch))
		return;

	if (is_affected(ch, gsn_confusion))
	{
		Cprintf(ch, "You are too confused to make sense of your surroundings.\n\r");
		return;
	}

	if (fAuto)
		sprintf(buf, "[Exits:");
	else if (IS_IMMORTAL(ch))
		sprintf(buf, "Obvious exits from room %d:\n\r", ch->in_room->vnum);
	else
		sprintf(buf, "Obvious exits:\n\r");

	found = FALSE;
	for (door = 0; door <= 5; door++)
	{
		if ((pexit = ch->in_room->exit[door]) != NULL
			&& pexit->u1.to_room != NULL
			&& can_see_room(ch, pexit->u1.to_room))
		{
			found = TRUE;
			if (fAuto)
			{

				strcat(buf, " ");
				if (IS_SET(pexit->exit_info, EX_CLOSED))
			        	strcat(buf,"{c(");

				strcat(buf, dir_name[door]);

				if (IS_SET(pexit->exit_info, EX_CLOSED))
			        	strcat(buf,"){x");
			}
			else
			{
				sprintf(buf + strlen(buf), "%-5s - %s", capitalize(dir_name[door]), room_is_dark(pexit->u1.to_room) ? "Too dark to tell" : pexit->u1.to_room->name);
				if (IS_SET(pexit->exit_info, EX_CLOSED))
					strcat(buf," [Closed]");

				if (IS_IMMORTAL(ch))
					sprintf(buf + strlen(buf), " (room %d)\n\r", pexit->u1.to_room->vnum);
				else
					sprintf(buf + strlen(buf), "\n\r");
			}
		}
	}

	if (!found)
		strcat(buf, fAuto ? " none" : "None.\n\r");

	if (fAuto)
		strcat(buf, "]\n\r");

	Cprintf(ch, "%s", buf);
	return;
}


void
do_worth(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		Cprintf(ch, "You have %ld gold and %ld silver.\n\r", ch->gold, ch->silver);
	else
		Cprintf(ch, "You have %ld gold, %ld silver, and %d experience (%d exp to level).\n\r", ch->gold, ch->silver, ch->exp, (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp);

	return;
}

char *
get_ac_by_text(int val)
{
	static char buf[MAX_STRING_LENGTH];

	if (val >= 101)
		sprintf(buf, "hopelessly vulnerable");
	else if (val >= 80)
		sprintf(buf, "defenseless");
	else if (val >= 60)
		sprintf(buf, "barely protected");
	else if (val >= 40)
		sprintf(buf, "slightly armored");
	else if (val >= 20)
		sprintf(buf, "somewhat armored");
	else if (val >= 0)
		sprintf(buf, "armored");
	else if (val >= -20)
		sprintf(buf, "well-armored");
	else if (val >= -40)
		sprintf(buf, "very well-armored");
	else if (val >= -60)
		sprintf(buf, "heavily armored");
	else if (val >= -80)
		sprintf(buf, "superbly armored");
	else if (val >= -100)
		sprintf(buf, "almost invulnerable");
	else
		sprintf(buf, "divinely armored");

	return buf;
}

void
out_char_info(CHAR_DATA * victim, CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	int name_col = 10;
	int col = 25;

	Cprintf(ch, "---------------------------------- Info ---------------------------------\n\r");

	sprintf(buf, "%d - %d(%d) hours", get_age(victim), get_perm_hours(victim), get_hours(victim));

	Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*s\n\r",
			name_col, "Level:",
			col, victim->level,
			name_col, "Age:",
			col, buf);

	if (get_trust(victim) != victim->level)
	{
		Cprintf(ch, "{c%-*s{x %-*d  \n\r",
				name_col, "Trusted:",
				col, get_trust(victim));
	}

	if (victim->delegate > 0)
	{
		Cprintf(ch, "{c%-*s{x %-*d  \n\r",
				name_col, "Delegated To:",
				col, victim->delegate);
	}

	if (victim->race == race_lookup("sliver")
	&& (victim->remort || victim->reclass || victim->level > 9)) {
                sprintf(buf, "%s %s", sliver_table[victim->sliver].name,
                victim->remort ? remort_list.race_info[((victim->race-1)*2) + victim->rem_sub -1].remort_race : race_table[victim->race].name);
        }
        else
                sprintf(buf, "%s", victim->remort ? remort_list.race_info[((victim->race-1)*2) + victim->rem_sub-1].remort_race : race_table[victim->race].name);

	Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
			name_col, "Race:",
			col, buf,
			name_col, "Sex:",
			col, victim->sex == 0 ? "sexless" : victim->sex == 1 ? "male" : "female");

	if (victim->alignment > 900)
		sprintf(buf2, "angelic");
	else if (victim->alignment > 700)
		sprintf(buf2, "saintly");
	else if (victim->alignment > 350)
		sprintf(buf2, "good");
	else if (victim->alignment > 100)
		sprintf(buf2, "kind");
	else if (victim->alignment > -100)
		sprintf(buf2, "neutral");
	else if (victim->alignment > -350)
		sprintf(buf2, "mean");
	else if (victim->alignment > -700)
		sprintf(buf2, "evil");
	else if (victim->alignment > -900)
		sprintf(buf2, "demonic");
	else
		sprintf(buf2, "satanic");


	buf[0] = '\0';
	if (ch->level >= 10)
	{
		sprintf(buf, "%d (%s)", victim->alignment, buf2);
	}
	else
	{
		strcpy(buf, buf2);
	}


	/* store class name */
	if (IS_NPC(victim))
		sprintf(buf2, "mobile");
	else
		sprintf(buf2, "%s", victim->reclass ? reclass_list.class_info[victim->reclass].re_class : class_table[victim->charClass].name);

	if (victim->deity_type)
	{
		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
                		name_col, "Class:",
                		col, buf2,
                		name_col, "Deity:",
                		col, deity_table[victim->deity_type - 1].name);

		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*d\n\r",
				name_col, "Alignment:",
				col, buf,
				name_col, "Deity Points:",
				col, victim->deity_points);
	}
	else
	{
		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
				name_col, "Class:",
				col, buf2,
				name_col, "Deity:",
				col, "atheist");

		Cprintf(ch, "{c%-*s{x %-*s  \n\r",
				name_col, "Alignment:",
				col, buf);
	}

/* Patron and vassal code modified by StarX */
// START
	if(victim->patron != NULL
	|| victim->vassal != NULL
	|| victim->pass_along > 0)
	{
		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
                                name_col, "Patron:",
                                col, victim->patron == NULL ? "none" : victim->patron,
                                name_col, "Vassal:",
                                col, victim->vassal == NULL ? "none" : victim->vassal);

		Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*d\n\r",
                                name_col, "To Patron:",
                                col, victim->to_pass,
                                name_col, "Patron Pt:",
                                col, victim->pass_along);
	}
// END

	if (victim->practice > 0 || victim->train > 0)
	{
		Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*d\n\r",
				name_col, "Pracs:",
				col, victim->practice,
				name_col, "Trains:",
				col, victim->train);
	}

	if (!IS_NPC(victim))
	{
	    char xpInfo[25];
	    sprintf(xpInfo, "%d (%d/lvl)", victim->exp, exp_per_level(victim, victim->pcdata->points));

		if (victim->level < LEVEL_HERO)
		{
			Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*d\n\r",
					name_col, "Exp:",
					col, xpInfo,
					name_col, "Next Level:",
					col, ((victim->level + 1) * exp_per_level(victim, victim->pcdata->points) - victim->exp));
		}
		else
		{
			Cprintf(ch, "{c%-*s{x %-*s\n\r",
					name_col, "Exp:",
					col, xpInfo);
		}

		if (!victim->remort && !victim->reclass)
                        sprintf(buf, "%d (%d for remort/reclass)", victim->questpoints, remort_cp_qp(victim) - victim->questpoints);
                else if(victim->reclass && !victim->remort)
			sprintf(buf, "%d (%d for remort)", victim->questpoints, remort_cp_qp(victim) - victim->questpoints);
		else if(victim->remort && !victim->reclass)
			sprintf(buf, "%d (%d for reclass)", victim->questpoints, remort_cp_qp(victim) - victim->questpoints);
		else if(victim->remort && victim->reclass)
			sprintf(buf, "%d", victim->questpoints);

		Cprintf(ch, "{c%-*s{x %-*s  \n\r",
                                        name_col, "Quest Points:",
                                        col, buf);

		if (is_clan(victim))
		{
			Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*d\n\r",
					name_col, "Pkills:",
					col, victim->pkills,
					name_col, "Pkilled:",
					col, victim->deaths);
		}
	}
	else
	{
		Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*s\n\r",
				name_col, "Vnum:",
				col, victim->pIndexData->vnum,
				name_col, "Format:",
				col, "new");

		Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*d\n\r",
				name_col, "Count:",
				col, victim->pIndexData->count,
				name_col, "Killed",
				col, victim->pIndexData->killed);
		Cprintf(ch, "{c%-*s{x %s\n\r",
				name_col, "Short:",
				victim->short_descr);
		Cprintf(ch, "{c%-*s{x %s",
				name_col, "Long",
		 victim->long_descr[0] != '\0' ? victim->long_descr : "(none)\n\r");

		sprintf(buf, "%dd%d", victim->damage[DICE_NUMBER], victim->damage[DICE_TYPE]);
		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
				name_col, "Damage:",
				col, buf,
				name_col, "Message",
				col, attack_table[victim->dam_type].noun);

		if (victim->hunting)
		{
			if (IS_NPC(victim->hunting))
			{
				sprintf(buf, victim->hunting->short_descr);
			}
			else
			{
				sprintf(buf, victim->hunting->name);
			}
		}
		else
		{
			sprintf(buf, "(none)");
		}

		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
				name_col, "Clone:",
				col, victim->is_clone ? "true" : "false",
				name_col, "Hunting:",
				col, buf);

		if (victim->spec_fun != NULL)
		{
			Cprintf(ch, "{c%-*s{x %-*s\n\r",
					name_col, "Special Proc:",
					col, spec_name(victim->spec_fun));
		}
	}

	sprintf(buf, "%d of %d", victim->carry_number, can_carry_n(victim));
	sprintf(buf2, "%d of %d", get_carry_weight(victim) / 10, can_carry_w(victim) / 10);


	Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
			name_col, "Carrying:",
			col, buf,
			name_col, "Weight:",
			col, buf2);

	Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*d\n\r",
			name_col, "Gold:",
			col, victim->gold,
			name_col, "Silver:",
			col, victim->silver);

	if (!IS_NPC(victim) && is_clan(victim))
	{
		Cprintf(ch, "{c%-*s{x %-*d  \n\r",
				name_col, "Bounty:",
				col, victim->bounty);
	}
}

void
out_char_stats(CHAR_DATA * victim, CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	int a = get_age(victim);
	int name_col = 10;
	int col = 25;

	Cprintf(ch, "---------------------------------- Stats --------------------------------\n\r");

	sprintf(buf, "%d of %d(%d)", victim->hit, victim->max_hit, MAX_HP(victim));
	sprintf(buf2, "%d of %d(%d)", victim->mana, victim->max_mana, MAX_MANA(victim));
	Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
			name_col, "Hp:",
			col, buf,
			name_col, "Mana:",
			col, buf2);

	sprintf(buf, "%d of %d(%d)", victim->move, victim->max_move, MAX_MOVE(victim));
	Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*d\n\r",
			name_col, "Movement:",
			col, buf,
			name_col, "Wimpy:",
			col, victim->wimpy);

	sprintf(buf, "%d(%d) of %d", victim->perm_stat[STAT_STR], get_curr_stat(victim, STAT_STR), get_max_stat(victim, STAT_STR));
	sprintf(buf2, "%d(%d) of %d", victim->perm_stat[STAT_CON], get_curr_stat(victim, STAT_CON), get_max_stat(victim, STAT_CON));
	Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
			name_col, "Str:",
			col, buf,
			name_col, "Con:",
			col, buf2);

	sprintf(buf, "%d(%d) of %d", victim->perm_stat[STAT_INT], get_curr_stat(victim, STAT_INT), get_max_stat(victim, STAT_INT));
	sprintf(buf2, "%d(%d) of %d", victim->perm_stat[STAT_WIS], get_curr_stat(victim, STAT_WIS), get_max_stat(victim, STAT_WIS));
	Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
			name_col, "Int:",
			col, buf,
			name_col, "Wis:",
			col, buf2);

	sprintf(buf, "%d(%d) of %d", victim->perm_stat[STAT_DEX], get_curr_stat(victim, STAT_DEX), get_max_stat(victim, STAT_DEX));
	Cprintf(ch, "{c%-*s{x %-*s  ",
			name_col, "Dex:",
			col, buf);


	if (ch->level >= 15)
	{
		Cprintf(ch, "{c%-*s{x %-*d  \n\r",
				name_col, "Saves:",
				col, victim->saving_throw);

		sprintf(buf, "%d", get_main_hitroll(victim));
		sprintf(buf2, "%d", get_main_damroll(victim));
		if(get_skill(victim, gsn_dual_wield) > 0
		&& get_eq_char(victim, WEAR_SHIELD) == NULL
		&& get_eq_char(victim, WEAR_WIELD) != NULL
		&& !IS_WEAPON_STAT(get_eq_char(victim, WEAR_WIELD), WEAPON_TWO_HANDS)) {
			sprintf(buf, "%s/%d",
					buf,
					get_dual_hitroll(victim));
			sprintf(buf2, "%s/%d",
					buf2,
					get_dual_damroll(victim));
		}
		if(get_eq_char(victim, WEAR_RANGED) != NULL
		|| get_eq_char(victim, WEAR_AMMO) != NULL) {
			sprintf(buf, "%s (Ranged: %d)",
                		buf,
                		get_ranged_hitroll(victim));
			sprintf(buf2, "%s (Ranged: %d)",
                		buf2,
                		get_ranged_damroll(victim));
		}

		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
				name_col, "HitRoll:",
				col, buf,
				name_col, "DamRoll:",
				col, buf2);

		if(victim->damage_reduce != 0
		|| victim->spell_damroll != 0) {
			Cprintf(ch, "{c%-*s{x %-*d  {c%-*s{x %-*d\n\r",
                	name_col, "DamResist:",
                	col, victim->damage_reduce,
                	name_col, "MagicDam:",
                	col, victim->spell_damroll);
		}

		if(victim->attack_speed != 0) {
	        	Cprintf(ch, "{c%-*s{x %-*d\n\r",
			name_col, "AttackSpd:",
			col, victim->attack_speed);	
		}
	}
	else
	{
		Cprintf(ch, "\n\r");
	}

	if (victim->race >= race_lookup("black dragon") && victim->race <= race_lookup("white dragon"))
	{
		sprintf(buf, "%d of %d",
				victim->breath / 10,
				((a > 100) ? 10 : (a > 80) ? 9 : (a > 60) ? 8 : (a > 50) ? 7 : (a > 40) ? 6 : (a > 30) ? 5 : (a > 25) ? 4 : 3));

		Cprintf(ch, "{c%-*s{x %-*s  \n\r",
				name_col, "Breaths:",
				col, buf);
	}
}


void
show_char_stats_to_char(CHAR_DATA * victim, CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	int name_col = 10;
	int col = 25;

	if (IS_IMMORTAL(victim) && !IS_NPC(victim))
	{
		Cprintf(ch, "{c%-*s{x %-*s  \n\r",
				name_col, "Holy Light:",
				col, (IS_SET(victim->act, PLR_HOLYLIGHT)) ? "on" : "off");

		if (victim->invis_level)
		{
			Cprintf(ch, "{c%-*s{x %-*d  \n\r",
					name_col, "Invisible To:",
					col, victim->invis_level);
		}

		if (victim->incog_level)
		{
			Cprintf(ch, "{c%-*s{x %-*d  \n\r",
					name_col, "Incognito To:",
					col, victim->incog_level);
		}
	}

	out_char_info(victim, ch);
	out_char_stats(victim, ch);

	Cprintf(ch, "--------------------------------- Armour --------------------------------\n\r");

	/* print AC values */
	if (ch->level >= 25)
	{
		sprintf(buf, "%d (%s)", GET_AC(victim, AC_PIERCE), get_ac_by_text(GET_AC(victim, AC_PIERCE)));
		sprintf(buf2, "%d (%s)", GET_AC(victim, AC_BASH), get_ac_by_text(GET_AC(victim, AC_BASH)));
		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
				name_col, "Pierce:",
				col, buf,
				name_col, "Bash:",
				col, buf2);

		sprintf(buf, "%d (%s)", GET_AC(victim, AC_SLASH), get_ac_by_text(GET_AC(victim, AC_SLASH)));
		sprintf(buf2, "%d (%s)", GET_AC(victim, AC_EXOTIC), get_ac_by_text(GET_AC(victim, AC_EXOTIC)));
		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
				name_col, "Slash:",
				col, buf,
				name_col, "Magic:",
				col, buf2);
	}
	else
	{
		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
				name_col, "Pierce:",
				col, get_ac_by_text(GET_AC(victim, AC_PIERCE)),
				name_col, "Bash:",
				col, get_ac_by_text(GET_AC(victim, AC_BASH)));

		Cprintf(ch, "{c%-*s{x %-*s  {c%-*s{x %-*s\n\r",
				name_col, "Slash:",
				col, get_ac_by_text(GET_AC(victim, AC_SLASH)),
				name_col, "Magic:",
				col, get_ac_by_text(GET_AC(victim, AC_EXOTIC)));
	}

}

void
do_mystats(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "%s%s\n\r", ch->name, IS_NPC(ch) ? "" : ch->pcdata->title);

	out_char_stats(ch, ch);
}

void
do_new_score(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "%s%s\n\r", ch->name, IS_NPC(ch) ? "" : ch->pcdata->title);

	if(!clan_table[ch->clan].independent) {
		Cprintf(ch, "%s of clan %s\n\r",
		  ch->clan_rank != NULL ? ch->clan_rank : "Member",
		  capitalize(clan_table[ch->clan].name));
	}
	show_char_stats_to_char(ch, ch);

	Cprintf(ch, "------------------------- Condition and Affects -------------------------\n\r");

	if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
		Cprintf(ch, "You are drunk.\n\r");
	if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] == 0)
		Cprintf(ch, "You are thirsty.\n\r");
	if (!IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER] == 0)
		Cprintf(ch, "You are hungry.\n\r");

	switch (ch->position)
	{
	case POS_DEAD:
		Cprintf(ch, "You are DEAD!!\n\r");
		break;
	case POS_MORTAL:
		Cprintf(ch, "You are mortally wounded.\n\r");
		break;
	case POS_INCAP:
		Cprintf(ch, "You are incapacitated.\n\r");
		break;
	case POS_STUNNED:
		Cprintf(ch, "You are stunned.\n\r");
		break;
	case POS_SLEEPING:
		Cprintf(ch, "You are sleeping.\n\r");
		break;
	case POS_RESTING:
		Cprintf(ch, "You are resting.\n\r");
		break;
	case POS_SITTING:
		Cprintf(ch, "You are sitting.\n\r");
		break;
	case POS_STANDING:
		Cprintf(ch, "You are standing.\n\r");
		break;
	case POS_FIGHTING:
		Cprintf(ch, "You are fighting.\n\r");
		break;
	}


	if (IS_SET(ch->comm, COMM_SHOW_AFFECTS))
		do_affects(ch, "");

}

void
do_old_score(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char strclass[MAX_STRING_LENGTH];
	char strrace[MAX_STRING_LENGTH];
	int i;
	int a = get_age(ch);

	/* store race and class names */
        sprintf(strrace, "%s", ch->remort ? remort_list.race_info[((ch->race-1)*2) + ch->rem_sub-1].remort_race : race_table[ch->race].name);

        if (IS_NPC(ch))
                sprintf(strclass, "mobile");
        else
                sprintf(strclass, "%s", ch->reclass ? reclass_list.class_info[ch->reclass].re_class : class_table[ch->charClass].name);

	/* display it all! */
	Cprintf(ch, "You are %s%s\n\r", ch->name, IS_NPC(ch) ? "" : ch->pcdata->title);
	Cprintf(ch, "{cLevel:{x %d {cAge:{x %d years old - %d(%d) hours\n\r", ch->level, get_age(ch), get_perm_hours(ch), get_hours(ch));

	if (get_trust(ch) != ch->level)
		Cprintf(ch, "{cYou are trusted at level {x%d{c.{x\n\r", get_trust(ch));

	if (ch->delegate > 0)
		Cprintf(ch, "{cYou are delegated to loner people level {x%d {cor lower.{x\n\r", ch->delegate);

	Cprintf(ch, "{cRace:{x %s  {cSex:{x %s  {cClass:{x %s\n\r", strrace, ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female", strclass);

	if (race_lookup("sliver") == ch->race && (ch->remort || ch->level > 9) && ch->sliver > 0)
		Cprintf(ch, "{cYou are a {x%s{c sliver.{x\n\r", sliver_table[ch->sliver].name);

	if (ch->deity_type)
		Cprintf(ch, "You worship {c%s{x and have {c%d{x deity points.\n\r", deity_table[ch->deity_type - 1].name, ch->deity_points);
	else
		Cprintf(ch, "You are atheist.  No God presently grants you favours.\n\r");

	Cprintf(ch, "You have {G%d{x/{R%d{x hit, {G%d{x/{R%d{x mana, {G%d{x/{R%d{x movement", ch->hit, MAX_HP(ch), ch->mana, MAX_MANA(ch), ch->move, MAX_MOVE(ch));

	if (ch->race >= race_lookup("black dragon") && ch->race <= race_lookup("white dragon"))
		Cprintf(ch, ", {G%d{x/{R%d{x breaths", ch->breath / 10, ((a > 100) ? 10 : (a > 80) ? 9 : (a > 60) ? 8 : (a > 50) ? 7 : (a > 40) ? 6 : (a > 30) ? 5 : (a > 25) ? 4 : 3));

	Cprintf(ch, ".\n\r");

	Cprintf(ch, "You have {M%d{x practices and {M%d{x training sessions.\n\r", ch->practice, ch->train);

	Cprintf(ch, "You are carrying {C%d{x/{B%d{x items with weight {C%ld{x/{B%d{x pounds.\n\r", ch->carry_number, can_carry_n(ch), get_carry_weight(ch) / 10, can_carry_w(ch) / 10);

	Cprintf(ch, "{cStr:{x %d(%d)  {cInt:{x %d(%d)  {cWis:{x %d(%d)  {cDex:{x %d(%d)  {cCon:{x %d(%d)\n\r", ch->perm_stat[STAT_STR], get_curr_stat(ch, STAT_STR), ch->perm_stat[STAT_INT], get_curr_stat(ch, STAT_INT), ch->perm_stat[STAT_WIS], get_curr_stat(ch,

































































































































STAT_WIS), ch->perm_stat[STAT_DEX], get_curr_stat(ch, STAT_DEX), ch->perm_stat[STAT_CON], get_curr_stat(ch, STAT_CON));

	Cprintf(ch, "You have scored {Y%d{x exp, and have {Y%ld{x gold and {W%ld{x silver coins.\n\r", ch->exp, ch->gold, ch->silver);

	/* RT shows exp to level */
	if (!IS_NPC(ch) && ch->level < LEVEL_HERO)
		Cprintf(ch, "You need {R%d{x exp to level.\n\r", ((ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp));

	if (!IS_NPC(ch)) {
                Cprintf(ch, "You have earned {c%d{x quest points", ch->questpoints);

		if (!ch->remort && !ch->reclass)
			Cprintf(ch, " ({c%d{x until remort/reclass)", remort_cp_qp(ch) - ch->questpoints);
		else if (ch->reclass)
                        Cprintf(ch, " ({c%d{x until remort)", remort_cp_qp(ch) - ch->questpoints);
		else if (ch->remort)
			Cprintf(ch, " ({c%d{x until reclass)", remort_cp_qp(ch) - ch->questpoints);
                else if (ch->remort && ch->reclass)
                        Cprintf(ch, " (You cannot remort/reclass again)");

		Cprintf(ch, ".\n\r");
        }

	Cprintf(ch, "Wimpy set to {M%d{x hit points.\n\r", ch->wimpy);

	if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
		Cprintf(ch, "You are drunk.\n\r");
	if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] == 0)
		Cprintf(ch, "You are thirsty.\n\r");
	if (!IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER] == 0)
		Cprintf(ch, "You are hungry.\n\r");

	switch (ch->position)
	{
	case POS_DEAD:
		Cprintf(ch, "You are DEAD!!\n\r");
		break;
	case POS_MORTAL:
		Cprintf(ch, "You are mortally wounded.\n\r");
		break;
	case POS_INCAP:
		Cprintf(ch, "You are incapacitated.\n\r");
		break;
	case POS_STUNNED:
		Cprintf(ch, "You are stunned.\n\r");
		break;
	case POS_SLEEPING:
		Cprintf(ch, "You are sleeping.\n\r");
		break;
	case POS_RESTING:
		Cprintf(ch, "You are resting.\n\r");
		break;
	case POS_SITTING:
		Cprintf(ch, "You are sitting.\n\r");
		break;
	case POS_STANDING:
		Cprintf(ch, "You are standing.\n\r");
		break;
	case POS_FIGHTING:
		Cprintf(ch, "You are fighting.\n\r");
		break;
	}

	if (!IS_NPC(ch) && is_clan(ch))
		Cprintf(ch, "Pkills: {G%d{x  Killed: {R%d{x  Bounty: {M%d{x\n\r", ch->pkills, ch->deaths, ch->bounty);

	/* print AC values */
	if (ch->level >= 25)
		Cprintf(ch, "Armor: pierce: {c%d{x  bash: {c%d{x  slash: {c%d{x  magic: {c%d{x\n\r", GET_AC(ch, AC_PIERCE), GET_AC(ch, AC_BASH), GET_AC(ch, AC_SLASH), GET_AC(ch, AC_EXOTIC));

	for (i = 0; i < 4; i++)
	{
		char *temp;

		switch (i)
		{
		case (AC_PIERCE):
			temp = "piercing";
			break;
		case (AC_BASH):
			temp = "bashing";
			break;
		case (AC_SLASH):
			temp = "slashing";
			break;
		case (AC_EXOTIC):
			temp = "magic";
			break;
		default:
			temp = "error";
			break;
		}

		Cprintf(ch, "You are ");
		if (GET_AC(ch, i) >= 101)
			sprintf(buf, "hopelessly vulnerable to %s.\n\r", temp);
		else if (GET_AC(ch, i) >= 80)
			sprintf(buf, "defenseless against %s.\n\r", temp);
		else if (GET_AC(ch, i) >= 60)
			sprintf(buf, "barely protected from %s.\n\r", temp);
		else if (GET_AC(ch, i) >= 40)
			sprintf(buf, "slightly armored against %s.\n\r", temp);
		else if (GET_AC(ch, i) >= 20)
			sprintf(buf, "somewhat armored against %s.\n\r", temp);
		else if (GET_AC(ch, i) >= 0)
			sprintf(buf, "armored against %s.\n\r", temp);
		else if (GET_AC(ch, i) >= -20)
			sprintf(buf, "well-armored against %s.\n\r", temp);
		else if (GET_AC(ch, i) >= -40)
			sprintf(buf, "very well-armored against %s.\n\r", temp);
		else if (GET_AC(ch, i) >= -60)
			sprintf(buf, "heavily armored against %s.\n\r", temp);
		else if (GET_AC(ch, i) >= -80)
			sprintf(buf, "superbly armored against %s.\n\r", temp);
		else if (GET_AC(ch, i) >= -100)
			sprintf(buf, "almost invulnerable to %s.\n\r", temp);
		else
			sprintf(buf, "divinely armored against %s.\n\r", temp);

		Cprintf(ch, "%s", buf);
	}

	/* RT wizinvis and holy light */
	if (IS_IMMORTAL(ch))
	{
		Cprintf(ch, "{mHoly Light:{x %s", (IS_SET(ch->act, PLR_HOLYLIGHT)) ? "on" : "off");

		if (ch->invis_level)
			Cprintf(ch, "  {mInvisible:{x level %d", ch->invis_level);

		if (ch->incog_level)
			Cprintf(ch, "  {mIncognito:{x level %d", ch->incog_level);

		Cprintf(ch, "\n\r");
	}

	if (ch->level >= 15)
		Cprintf(ch, "Hitroll: {Y%d{x  Damroll: {Y%d{x  Saves: {Y%d{x.\n\r", GET_HITROLL(ch), GET_DAMROLL(ch), ch->saving_throw);

	if (ch->level >= 10)
		Cprintf(ch, "Alignment: {g%d{x.  ", ch->alignment);

	Cprintf(ch, "You are ");
	if (ch->alignment > 900)
		Cprintf(ch, "angelic.\n\r");
	else if (ch->alignment > 700)
		Cprintf(ch, "saintly.\n\r");
	else if (ch->alignment > 350)
		Cprintf(ch, "good.\n\r");
	else if (ch->alignment > 100)
		Cprintf(ch, "kind.\n\r");
	else if (ch->alignment > -100)
		Cprintf(ch, "neutral.\n\r");
	else if (ch->alignment > -350)
		Cprintf(ch, "mean.\n\r");
	else if (ch->alignment > -700)
		Cprintf(ch, "evil.\n\r");
	else if (ch->alignment > -900)
		Cprintf(ch, "demonic.\n\r");
	else
		Cprintf(ch, "satanic.\n\r");

	if (IS_SET(ch->comm, COMM_SHOW_AFFECTS))
		do_affects(ch, "");
}


void
do_affects(CHAR_DATA * ch, char *argument) {
    AFFECT_DATA *paf, *paf_last = NULL;
    CHAR_DATA *victim;
    char arg[MAX_STRING_LENGTH];

    victim = ch;
    /* Allows immortals to view affects of other chars */
    if (IS_IMMORTAL(ch)) {
        argument = one_argument( argument, arg );
        if (arg[0] != '\0') {
            if ( (victim = get_char_world(ch, arg, TRUE)) == NULL ) {
                if (strcmp(arg, "remove")) {
                    Cprintf(ch, "There is nobody called %s in heaven or on earth.\n\r", arg);
                    return;
                }

                victim = ch;
            } else {
                argument = one_argument(argument, arg);
            }

            /* Allows immortals to remove affects of other chars */
            if ( arg[0] != '\0' && !strcmp(arg, "remove") ) {
                if (get_trust(ch) < 56 && ch != victim) {
                    Cprintf(ch, "You must be at least level 56 to remove affects from another character.\n\r");
                    return;
                }

                /* remove affect */
                argument = one_argument(argument, arg);
                
                if (arg[0] == '\0') {
                    if (ch == victim) {
                        Cprintf(ch, "Which affect would you like to remove from yourself?\n\r");
                    } else {
                        Cprintf(ch, "Which affect would you like to remove from %s?\n\r", victim->name);
                    }

                    return;
                }

                if (!strcmp(arg, "all")) {
                    removeAllEffects(victim);

                    if (ch != victim) {
                        Cprintf(ch, "All affects have been removed from %s.\n\r", victim->name);
                    }

                    Cprintf(victim, "All of your affects have been removed.\n\r");
                    return;
                } else {
                    int sn = skill_lookup(arg);
                    if (sn == -1) {
                        Cprintf(ch, "That spell doesn't exist!\n\r");
                        return;
                    }

                    affect_strip(victim, sn);

                    if (ch != victim) {
                        Cprintf(ch, "Affect removed.\n\r");
                    }

                    Cprintf(victim, "The affects of %s have been removed from you.\n\r", skill_table[sn].name);
                    return;
                }
            }
        }
    }

    if (victim->affected != NULL || IS_AFFECTED(victim, AFF_HIDE)) {
        if (ch == victim) {
            Cprintf(ch, "You are affected by the following spells:\n\r");
        } else {
            Cprintf(ch, "%s is affected by the following spells:\n\r", victim->name);
        }

        for (paf = victim->affected; paf != NULL; paf = paf->next) {
            if (paf->where == TO_VULN || paf->where == TO_RESIST) {
                continue;
            }

            if (paf_last != NULL && paf->type == paf_last->type) {
                if (ch->level >= 20) {
                    Cprintf(ch, "                         ");
                } else {
                    continue;
                }
            } else {
                Cprintf(ch, "Spell: %-18s", skill_table[paf->type].name);
            }

            if (ch->level >= 20) {
                if (paf->type == gsn_minor_revitalize
                        || paf->type == gsn_lesser_revitalize
                        || paf->type == gsn_greater_revitalize) {
                    Cprintf(ch, ": gradually recover %d more hp", paf->modifier);
                } else if (paf->type == gsn_aurora) {
                    Cprintf(ch, ": gradually recover %d more mana", paf->modifier);
                } else if ((paf->type == gsn_berserk || paf->type == gsn_enrage)
                        && paf->location == APPLY_NONE) {
                    Cprintf(ch, ": gradually recover %d more hp", paf->modifier);	
		} else if(paf->type == gsn_chi_ei) {
		    Cprintf(ch, ": modifies counter attack by %d", paf->modifier);
                } else {
                    Cprintf(ch, ": modifies %s by %d ", affect_loc_name(paf->location), paf->modifier);

                    if (paf->duration == -1) {
                        Cprintf(ch, "permanently");
                    } else {
                        Cprintf(ch, "for %d hours", paf->duration);
                    }
                }
            }

            Cprintf(ch, "\n\r");
            paf_last = paf;
        }

        if (IS_AFFECTED(ch, AFF_HIDE)) {
            Cprintf(ch, "Spell: hide\n\r");
        }
    } else {
        if (ch == victim) {
            Cprintf(ch, "You are not affected by any spells.\n\r");
        } else {
            Cprintf(ch, "%s is not affected by any spells.\n\r", victim->name);
        }
    }

    return;
}


char *const
 day_name[] =
{
	"the Moon", "the Bull", "Deception", "Thunder", "Freedom",
	"the Great Gods", "the Sun"
};


char *const
 month_name[] =
{
	"Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
	"the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
	"the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
	"the Long Shadows", "the Ancient Darkness", "the Great Evil"
};


void
do_time(CHAR_DATA * ch, char *argument)
{
	extern char str_boot_time[];
	char *suf;
	int day;

	day = time_info.day + 1;

	if (day > 4 && day < 20)
		suf = "th";
	else if (day % 10 == 1)
		suf = "st";
	else if (day % 10 == 2)
		suf = "nd";
	else if (day % 10 == 3)
		suf = "rd";
	else
		suf = "th";

	Cprintf(ch, "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n\r", (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12, time_info.hour >= 12 ? "pm" : "am", day_name[day % 7], day, suf, month_name[time_info.month]);
	Cprintf(ch, "Redemption started up at %s\n\rThe system time is %s\n\r", str_boot_time, (char *) ctime(&current_time));

	return;
}


void
do_weather(CHAR_DATA * ch, char *argument)
{

	static char *const sky_look[4] =
	{
		"cloudless",
		"cloudy",
		"rainy",
		"lit by flashes of lightning"
	};

	if (!IS_OUTSIDE(ch))
	{
		Cprintf(ch, "You can't see the weather indoors.\n\r");
		return;
	}

	Cprintf(ch, "The sky is %s and %s.\n\r", sky_look[weather_info.sky], weather_info.change >= 0 ? "a warm southerly breeze blows" : "a cold northern gust blows");
	return;
}


void
do_help(CHAR_DATA * ch, char *argument)
{
	HELP_DATA *pHelp;
	BUFFER *output;
	bool found = FALSE;
	char argall[MAX_INPUT_LENGTH], argone[MAX_INPUT_LENGTH];
	int level;

	if (IS_NPC(ch))
		return;

	output = new_buf();

	if (argument[0] == '\0')
		argument = "summary";

	/* this parts handles help a b so that it returns help 'a b' */
	argall[0] = '\0';
	while (argument[0] != '\0')
	{
		argument = one_argument(argument, argone);
		if (argall[0] != '\0')
			strcat(argall, " ");
		strcat(argall, argone);
	}

	for (pHelp = help_first; pHelp != NULL; pHelp = pHelp->next)
	{
		level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;

		if (level > get_trust(ch))
			continue;

		if (is_name(argall, pHelp->keyword))
		{
			/* add seperator if found */
			if (found)
			{
				if (str_cmp(argall, "greeting"))
				{
					add_buf(output, "\n\r");
				}

				add_buf(output, "\n\r============================================================\n\r\n\r");
			}

			if (pHelp->level >= 0 && str_cmp(argall, "imotd"))
			{
				add_buf(output, pHelp->keyword);
				add_buf(output, "\n\r");
			}

			/*
			 * Strip leading '.' to allow initial blanks.
			 */
			if (pHelp->text[0] == '.')
				add_buf(output, pHelp->text + 1);
			else
				add_buf(output, pHelp->text);
			found = TRUE;
			/* small hack :) */
			if (ch->desc != NULL && ch->desc->connected != CON_PLAYING
				&& ch->desc->connected != CON_GEN_GROUPS)
				break;
		}
	}

	if (!found)
		Cprintf(ch, "No help on that word.\n\r");
	else
		page_to_char(buf_string(output), ch);
	free_buf(output);
}


void
do_whorem(CHAR_DATA * ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	char *tmprac;
	char *tmpclass;

	Cprintf(ch, "List of remorts/reclasses online:\n\r---------------------------------\n\r\n\r");

	for (d = descriptor_list; d != NULL; d = d->next)
	{
		if (d->character != NULL)
		{
 			if ((d->character->reclass > 0
			|| d->character->remort > 0)
			&& !IS_IMMORTAL(d->character)
			&& can_see(ch, d->character)) {
				if(d->character->remort)
					tmprac = str_dup(remort_list.race_info[((d->character->race-1) *2) + d->character->rem_sub - 1].remort_race);
				else
					tmprac = str_dup(pc_race_table[d->character->race].name);

				if(d->character->reclass)
					tmpclass = str_dup(reclass_list.class_info[d->character->reclass].re_class);
				else
					tmpclass = str_dup(class_table[d->character->charClass].name);

				Cprintf(ch, "%-16s%s %s\n\r", d->character->name, tmprac, tmpclass);
                                free_string(tmprac);
				free_string(tmpclass);
			}
		}
	}
	return;
}


/* whois command */
void
do_whois(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char *contNames[] =
	{"Terra", "Dominia"};
	BUFFER *continent[2];
	DESCRIPTOR_DATA *d;
	bool found = FALSE;
	CHAR_DATA *wch;
	int i;

	one_argument(argument, arg);

	continent[0] = new_buf();
	continent[1] = new_buf();

	if (arg[0] == '\0')
	{
		Cprintf(ch, "You must provide a name.\n\r");
		return;
	}

	for (d = descriptor_list; d != NULL; d = d->next)
	{
		if (d->character != NULL)
		{
			if (d->connected != CON_PLAYING || !can_see(ch, d->character))
				continue;

			wch = (d->original != NULL) ? d->original : d->character;

			if (!can_see(ch, wch))
				continue;

			if (!str_prefix(arg, wch->name))
			{
				int cont = (wch->in_room != NULL) ? wch->in_room->area->continent : 0;

				found = TRUE;

				char_line(wch, ch, buf);
				add_buf(continent[cont], buf);
				add_buf(continent[cont], "\n\r");
			}
		}
	}

	if (found)
	{
		for (i = 0; i < 2; i++)
		{
			if (strlen(buf_string(continent[i])))
			{
				Cprintf(ch, "{YOn {G%s{Y:{x\n\r", contNames[i]);
				Cprintf(ch, "%s\n\r", buf_string(continent[i]));
			}
		}
	}
	else
	{
		Cprintf(ch, "No one of that name is playing.\n\r");
		return;
	}

	free_buf(continent[0]);
	free_buf(continent[1]);
}

char *remort_abrev[] =
{
	"Dopple ",
	"Medusa ",
	"Forest ",
	"Drow   ",
	"Mountn ",
	"Grey   ",
	"Hill   ",
	"Cave   ",
	"   xx  ",
	"   yy  ",
	"{x{DOnyx{x   ",
	"{x{DShadow{x ",
	"{x{bSaphire{x",
	"{x{bSky{x    ",
	"{x{gEmerald{x",
	"{x{gEarth{x  ",
	"{x{rRuby{x   ",
	"{x{rFire{x   ",
	"{x{WMist{x   ",
	"{x{WPearl{x  ",
	"Sting  ",
	"Abyssal",
	"TwoHead",
	"Spectre",
	"Sentinl",
	"Hauntng",
	"{x{YSabre{x  ",
	"{x{mDark{x   ",
        "{x{bNereid{x ",
        "{x{yBefoulr{x",
	NULL
};

char *reclass_abrev[] =
{
	NULL,
	" Wizard",
	" Alchem",
	" Shaman",
	" Sorcer",
	" Mystic",
	"  Illus",
	"  Necro",
	"Diviner",
	"Heretic",
	"Exorcst",
	"  Hiero",
	" Herbal",
	"Warlord",
	"   Barb",
	"Cavlier",
	"Templar",
	" Hermit",
	"BountyH",
	"Assasin",
	"  Rogue",
	" Zealot",
        "  Psion",
        "  Ninja",
        "Samurai",
	NULL
};

char *get_remort_abrev(CHAR_DATA *ch)
{
	return remort_abrev[((ch->race-1) *2) + ch->rem_sub -1];
}

char *get_reclass_abrev(CHAR_DATA *ch)
{
	return reclass_abrev[ch->reclass];
}

void
char_line(CHAR_DATA * ch, CHAR_DATA * looker, char *text) {
    char *immTitles[] =
           {"    IMPLEMENTOR   ",
            "     SUPREMACY    ",
            "       DEITY      ",
            "        GOD       ",
            "      DEMIGOD     ",
            "       ANGEL      ",
            "      BUILDER     ",
            "      LEADER      ",
            "     RECRUITER    "};

	char *independentLeadership = "     ---------    ";
	char *clanRecruiter         = "     Recruiter    ";
	char *clanLeader            = "      Leader      ";

    char shDesc[20] = "                  ";	/* either level/race/class or immTitle */
    char rpbuf[MAX_INPUT_LENGTH];
    char *invisFlag, *hideFlag, *occularyFlag;
    char buf[MAX_STRING_LENGTH];
    char raceName[10];
    char className[10];
    char wiziShow[MAX_INPUT_LENGTH];
    char incogShow[MAX_INPUT_LENGTH];

    /* Remort/reclasses for who list coded by Tsongas April 9, 2001 */
    // And re-updated by Starcrossed same time!
    sprintf(raceName,  "%s", ch->remort  ? get_remort_abrev(ch)  : pc_race_table[ch->race].who_name);
    sprintf(className, "%s", ch->reclass ? get_reclass_abrev(ch) : class_table[ch->charClass].who_name);


    /* set up the lvl/race/cls or immTitle */
    if (ch->level <= 51) /* Non-remort/reclass case */
    {
        sprintf(shDesc, "%2d %7s %7s", ch->level, raceName, className);
    }
    else if (ch->level == 52) /* Recruiter case */
    {
        if (clan_table[ch->clan].independent)
            sprintf(shDesc, "%18s", independentLeadership);
	else
            sprintf(shDesc, "%18s", clanRecruiter);
    }
    else if (ch->level == 53) /* Leader case */
    {
	if (clan_table[ch->clan].independent)
	    sprintf(shDesc, "%18s", independentLeadership);
	else
	    sprintf(shDesc, "%18s", clanLeader);
    } else if ((ch->level >= MAX_LEVEL - 8) && (ch->level <= MAX_LEVEL)) {
        /* immortal case */
        sprintf(shDesc, "%18s", ((ch->short_descr[0]) ? ch->short_descr : immTitles[MAX_LEVEL - ch->level]));
    }

    /* rptitle */
    if (ch->rptitle && ch->rptitle[0] != '\0') {
        sprintf(rpbuf, "[%s] ", ch->rptitle);
    } else {
        rpbuf[0] = '\0';
    }

    /* invis/hide flags */
    if (IS_IMMORTAL(looker)) {
        if (IS_SET(looker->act, PLR_HOLYLIGHT)) {
            invisFlag = (IS_AFFECTED(ch, AFF_INVISIBLE)) ? "(Invis) " : "";
            hideFlag = (IS_AFFECTED(ch, AFF_HIDE)) ? "(Hiding) " : "";
            occularyFlag = (is_affected(ch, gsn_oculary)) ? "(Ocularied) " : "";
        } else {
            invisFlag = "";
            hideFlag = "";
            occularyFlag = "";
        }

        if (ch->invis_level > 0) {
            sprintf(wiziShow, "(Wizi %d) ", ch->invis_level);
        } else {
            wiziShow[0] = '\0';
        }

        if (ch->incog_level > 0) {
            sprintf(incogShow, "(Incog %d) ", ch->incog_level);
        } else {
            incogShow[0] = '\0';
        }
    } else {
        invisFlag = "";
        hideFlag = "";
        occularyFlag = "";
        wiziShow[0] = '\0';
        incogShow[0] = '\0';
    }

    /* Actual formatting of the who/whois list */
    sprintf(buf, "{x[%s] %s%s%s%s%s%s%s{x%s%s%s%s%s%s%s%s%s%s%s%s{x",
                    shDesc,
                    wiziShow,
                    incogShow,
                    invisFlag,
                    hideFlag,
                    occularyFlag,
                    clan_table[ch->clan].who_name,
                    rpbuf,
                    IS_SET(ch->comm, COMM_AFK) ? "[A] " : "",
                    IS_SET(ch->wiznet, WIZ_OLC) ? "[OLC] " : "", /* OLC flag added by Tsongas */
                    IS_SET(ch->comm, COMM_DEAF) ? "[D] " : "",
                    IS_SET(ch->comm, COMM_QUIET) ? "[Q] " : "",
                    IS_SET(ch->act, PLR_KILLER) ? "{R(K){x " : "",
                    IS_SET(ch->act, PLR_THIEF) ? "{Y(T){x " : "",
                    IS_SET(ch->comm, COMM_NOTE_WRITE) ? "{Y<Note>{x " : "",
                    IS_SET(ch->comm, COMM_NOCHANNELS) ? "{G[NoChan]{x " : "",
                    IS_SET(ch->act, PLR_FREEZE) ? "{C[Frozen]{x " : "",
                    IS_SET(ch->comm, COMM_LAG) ? "{M<LAGGED>{x " : "",
                    ch->name,
                    IS_NPC(ch) ? "" : ch->pcdata->title);

    strcpy(text, buf);
}


bool
is_range_who(CHAR_DATA * ch, CHAR_DATA * victim)
{
	if (!is_clan(ch) || !is_clan(victim))
		return FALSE;

	if ((IS_SET(victim->act, PLR_KILLER) || IS_SET(victim->act, PLR_THIEF))
		&& (ch->level - victim->level < 17))
		return TRUE;

	if (abs(victim->level - ch->level) < 9)
		return TRUE;

	return FALSE;
}


int
list_npc(int where, BUFFER * output, CHAR_DATA * looker)
{
	char buf[2000];
	CHAR_DATA *ch;
	int count = 0;

	for (ch = char_list; ch != NULL; ch = ch->next)
	{
		if (ch->on_who == TRUE && IS_NPC(ch) && ch->in_room != NULL && can_see(looker, ch))
		{
			sprintf(buf, "[%2d Quest    Mobile] %s\n\r", ch->level, ch->short_descr);
			if (ch->in_room->area->continent == where)
			{
				add_buf(output, buf);
				count++;
			}
		}
	}

	return (count);
}


/*
 * New 'who' command originally by Alander of Rivers of Mud.
 */

void
do_who(CHAR_DATA * ch, char *argument) {
    const char *continentNames[] = {"Terra", "Dominia"};

    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    int iClass;
    int iRace;
    int iClan;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    int continent;
    bool rgfClass[MAX_CLASS];
    bool rgfRace[MAX_PC_RACE];
    bool rgfClan[MAX_CLAN];
    bool fClassRestrict = FALSE;
    bool fClanRestrict = FALSE;
    bool fClan = FALSE;
    bool fNoClan = FALSE;
    bool fRaceRestrict = FALSE;
    bool fImmortalOnly = FALSE;
    bool fRange = FALSE;
    bool fGroup = FALSE;
    bool fContinent[] =
    {FALSE, FALSE};
    bool fDragon = FALSE;
    bool fLead = FALSE;

    /*
     * Set default arguments.
     */
    iLevelLower = 0;
    iLevelUpper = MAX_LEVEL;

    for (iClass = 0; iClass < MAX_CLASS; iClass++) {
        rgfClass[iClass] = FALSE;
    }

    for (iRace = 0; iRace < MAX_PC_RACE; iRace++) {
        rgfRace[iRace] = FALSE;
    }

    for (iClan = 0; iClan < MAX_CLAN; iClan++) {
        rgfClan[iClan] = FALSE;
    }

    /*
     * Parse arguments.
     */
    nNumber = 0;
    for (;;) {
        char arg[MAX_STRING_LENGTH];

        argument = one_argument(argument, arg);
        if (arg[0] == '\0') {
            break;
        }

        if (is_number(arg)) {
            switch (++nNumber) {
                case 1:
                    iLevelLower = atoi(arg);
                    break;

                case 2:
                    iLevelUpper = atoi(arg);
                    break;

                default:
                    Cprintf(ch, "Only two level numbers allowed.\n\r");
                    return;
            }
        } else {
            /*
             * Look for classes to turn on.
             */
            if (!str_prefix(arg, "immortals")) {
                fImmortalOnly = TRUE;
            } else if (!str_prefix(arg, "pkill")) {
                fRange = TRUE;
            } else if (!str_prefix(arg, "group")) {
                fGroup = TRUE;
            } else if (!str_prefix(arg, "dominia")) {
                fContinent[1] = TRUE;
            } else if (!str_prefix(arg, "terra")) {
                fContinent[0] = TRUE;
            } else if (!str_prefix(arg, "dragon")) {
                fDragon = TRUE;
            } else if (!str_prefix(arg, "lead")) {
                fLead = TRUE;
            } else {
                iClass = class_lookup(arg);
                if (iClass == -1) {
                    iRace = race_lookup(arg);

                    if (iRace == 0 || iRace >= MAX_PC_RACE) {
                        if (!str_prefix(arg, "clan")) {
                            fClan = TRUE;
                        } else if (!str_prefix(arg, "noclan")) {
                            fNoClan = TRUE;
                        } else {
                            iClan = clan_lookup(arg);
                            if (iClan) {
                                fClanRestrict = TRUE;
                                rgfClan[iClan] = TRUE;
                            } else {
                                Cprintf(ch, "That's not a valid race, class, or clan.\n\r");
                                return;
                            }
                        }
                    } else {
                        fRaceRestrict = TRUE;
                        rgfRace[iRace] = TRUE;
                    }
                } else {
                    fClassRestrict = TRUE;
                    rgfClass[iClass] = TRUE;
                }
            }
        }
    }

    /*
     * Now show matching chars.
     */
    nMatch = 0;
    buf[0] = '\0';
    output = new_buf();

    if (!(fContinent[0] || fContinent[1])) {
        fContinent[0] = fContinent[1] = TRUE;
    }

    if (fDragon) {
        rgfRace[race_lookup("hatchling")] =
            rgfRace[race_lookup("black dragon")] =
                rgfRace[race_lookup("white dragon")] =
                    rgfRace[race_lookup("green dragon")] =
                        rgfRace[race_lookup("red dragon")] =
                            rgfRace[race_lookup("blue dragon")] = TRUE;

        fRaceRestrict = TRUE;
    }

    for (continent = 0; continent < 2; continent++) {
        if (!fContinent[continent]) {
            continue;
        }

        sprintf(buf, "----==== Characters on %s ====----\n\r\n\r", continentNames[continent]);
        add_buf(output, buf);

        for (d = descriptor_list; d != NULL; d = d->next) {
            CHAR_DATA *wch;

            if (d->character == NULL) {
                continue;
            }

            /*
             * Check for match against restrictions.
             * Don't use trust as that exposes trusted mortals.
             */
            if (d->connected != CON_PLAYING) {
                if ( !(IS_IMMORTAL(ch) && IS_SET(d->character->comm, COMM_NOTE_WRITE)) ) {
                    continue;
                }
            }

            wch = (d->original != NULL) ? d->original : d->character;

            if (!can_see(ch, wch)) {
                continue;
            }

            if (wch->in_room != NULL) {
                if (wch->in_room->area->continent != continent) {
                    continue;
                }
            }

            if ((wch->level < iLevelLower)
                    || (wch->level > iLevelUpper)
                    || (fImmortalOnly && (wch->level < 55))
                    || (fLead && ((wch->level < (LEVEL_HERO + 1)) && ((get_trust(wch) != 52) && (get_trust(wch) != 53 )) ))
                    || (fClassRestrict && !rgfClass[wch->charClass])
                    || (fRaceRestrict && !rgfRace[wch->race])
                    || (fClan && !is_clan(wch))
                    || (fNoClan && is_clan(wch))
                    || (fRange && !is_range_who(ch, wch))
                    || (fGroup && ((wch->level < ch->level - 8) || (wch->level > ch->level + 8)))
                    || (fClanRestrict && !rgfClan[wch->clan])) {
                continue;
            }

            nMatch++;

            char_line(wch, ch, buf);
            add_buf(output, buf);
            add_buf(output, "\n\r");
        }

        nMatch += list_npc(continent, output, ch);
        add_buf(output, "\n\r");
    }

    sprintf(buf2, "Players found: %d\n\r", nMatch);
    add_buf(output, buf2);
    page_to_char(buf_string(output), ch);
    free_buf(output);

    return;
}


void
do_count(CHAR_DATA * ch, char *argument)
{
	int count, max_count;
	DESCRIPTOR_DATA *d;
	char buf[MAX_STRING_LENGTH];

	count = 0;

	for (d = descriptor_list; d != NULL; d = d->next)
		if (d->character != NULL)
			if (d->connected == CON_PLAYING && can_see(ch, d->character))
				count++;

	max_on = UMAX(count, max_on);

	if (max_on == count)
		sprintf(buf, "There are %d characters on, the most so far today.\n\r", count);
	else
		sprintf(buf, "There are %d characters on, the most on today was %d.\n\r", count, max_on);

	Cprintf(ch, "%s", buf);

	max_count = load_count();
	if (max_count > count)
		Cprintf(ch, "The most people on ever was %d.\n\r", max_count);
	else
	{
		Cprintf(ch, "This is a record! %d is the most people on ever!\n\r", count);
		save_count(count);
	}

	return;
}


void
do_inventory(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "You are carrying:\n\r");
	show_list_to_char(ch->carrying, ch, TRUE, TRUE);
	return;
}


void
do_equipment(CHAR_DATA * ch, char *argument) {
    OBJ_DATA *obj;
    int iWear;
    bool found;

    Cprintf(ch, "You are using:\n\r");
    found = FALSE;
    for (iWear = 0; iWear < MAX_WEAR; iWear++) {
        if ((obj = get_eq_char(ch, iWear)) == NULL) {
            /* special cases */
            if ((iWear == WEAR_HANDS
                    || iWear == WEAR_LEGS
                    || iWear == WEAR_FEET)
                    && IS_DRAGON(ch) && ch->remort) {
                Cprintf(ch, "%s(Permanent) Spikey Dragon Scales\n\r", where_name[iWear]);
                continue;
            }

            if (iWear == WEAR_FLOAT_2
                    && ch->level < 25) {
                continue;
            }

            if (iWear == WEAR_HEAD_2
                    && (ch->race != race_lookup("troll")
                            || ch->rem_sub != 1)) {
                continue;
            }

            if (iWear == WEAR_DUAL
                    && get_skill(ch, gsn_dual_wield) < 1) {
                continue;
            }

            if ((iWear == WEAR_RANGED
                    || iWear == WEAR_AMMO)
                    && get_skill(ch, gsn_marksmanship) < 1) {
                continue; 
            }

            Cprintf(ch, "%s<Nothing>\n\r", where_name[iWear]);
            continue;
        }

        Cprintf(ch, "%s", where_name[iWear]);

        if (can_see_obj(ch, obj)) {
            Cprintf(ch, "%s\n\r", format_obj_to_char(obj, ch, TRUE));
        } else {
            Cprintf(ch, "something.\n\r");
        }

        found = TRUE;
    }

    return;
}


void
do_compare(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	OBJ_DATA *obj1;
	OBJ_DATA *obj2;
	int value1;
	int value2;
	char *msg;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Compare what to what?\n\r");
		return;
	}

	if ((obj1 = get_obj_carry(ch, arg1, ch)) == NULL)
	{
		Cprintf(ch, "You do not have that item.\n\r");
		return;
	}

	if (arg2[0] == '\0')
	{
		for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
			if (obj2->wear_loc != WEAR_NONE && can_see_obj(ch, obj2) && obj1->item_type == obj2->item_type && (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0)
				break;

		if (obj2 == NULL)
		{
			Cprintf(ch, "You aren't wearing anything comparable.\n\r");
			return;
		}
	}

	else if ((obj2 = get_obj_carry(ch, arg2, ch)) == NULL)
	{
		Cprintf(ch, "You do not have that item.\n\r");
		return;
	}

	msg = NULL;
	value1 = 0;
	value2 = 0;

	if (obj1 == obj2)
		msg = "You compare $p to itself.  It looks about the same.";
	else if (obj1->item_type != obj2->item_type)
		msg = "You can't compare $p and $P.";
	else
	{
		switch (obj1->item_type)
		{
		default:
			msg = "You can't compare $p and $P.";
			break;

		case ITEM_ARMOR:
			value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
			value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
			break;

		case ITEM_WEAPON:
			if (obj1->pIndexData->new_format)
				value1 = (1 + obj1->value[2]) * obj1->value[1];
			else
				value1 = obj1->value[1] + obj1->value[2];

			if (obj2->pIndexData->new_format)
				value2 = (1 + obj2->value[2]) * obj2->value[1];
			else
				value2 = obj2->value[1] + obj2->value[2];
			break;
		}
	}

	if (msg == NULL)
	{
		if (value1 == value2)
			msg = "$p and $P look about the same.";
		else if (value1 > value2)
			msg = "$p looks better than $P.";
		else
			msg = "$p looks worse than $P.";
	}

	act(msg, ch, obj1, obj2, TO_CHAR, POS_RESTING);
	return;
}


void
do_credits(CHAR_DATA * ch, char *argument)
{
	do_help(ch, "diku");
	return;
}


void
do_where(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	bool found;

	if (is_affected(ch, gsn_loneliness))
	{
		Cprintf(ch, "You feel like you are all alone.\n\r");
		return;
	}

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		if (ch->in_room == NULL || ch->in_room->area == NULL)
			return;

		Cprintf(ch, "Current Area: %s. Level Range: %.5s\r\n", ch->in_room->area->name, &(ch->in_room->area->credits[1]));
		Cprintf(ch, "Players near you:\n\r");
		found = FALSE;

		for (victim = char_list; victim != NULL; victim = victim->next)
		{
			if (((  !IS_NPC(victim) && victim->desc != NULL && victim->desc->connected == CON_PLAYING) ||
				 (   IS_NPC(victim) &&
				      (victim->on_who == TRUE ||
					   is_affected(victim, gsn_duplicate)))) &&
				((victim->in_room != NULL	&&
				!IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)
				&& victim->in_room->area == ch->in_room->area
				&& can_see(ch, victim)
				&& !is_affected(victim, gsn_shapeshift)
				&& !is_affected(victim, gsn_concealment)
				&& !room_is_affected(victim->in_room, gsn_lair))
				||
				(  victim->in_room != NULL
			  	&& !IS_NPC(ch)
				&& IS_SET(ch->act, PLR_HOLYLIGHT)
				&& victim->in_room->area == ch->in_room->area)))
			{
				found = TRUE;
				if (room_is_affected(ch->in_room, gsn_winter_storm))
				{
					Cprintf(ch, "%-28s A raging winter storm\n\r", PERS(victim, ch));
				}
				else
				{
					if (is_affected(victim, gsn_aura))
        				{
						AFFECT_DATA *paf = NULL;

                				for (paf = victim->affected; paf != NULL; paf = paf->next)
               					{
                        				if (paf->type == gsn_aura)
                                				break;
                				}
                				if (paf != NULL)
						{
							if (!IS_NPC(victim) && victim->pcdata->ctf_flag)
							{
                						switch (paf->modifier) {
                        					case 1:
									Cprintf(ch, "%-28s ({BBlue {x[{RFlag{x]) %s\n\r", PERS(victim, ch), victim->in_room->name);
                                					break;
                        					case 2:
									Cprintf(ch, "%-28s ({RRed {x[{BFlag{x]) %s\n\r", PERS(victim, ch), victim->in_room->name);
                                					break;
                						}

							}
							else
							{
                						switch (paf->modifier) {
                        					case 1:
									Cprintf(ch, "%-28s ({BBlue{x) %s\n\r", PERS(victim, ch), victim->in_room->name);
                                					break;
                        					case 2:
									Cprintf(ch, "%-28s ({RRed{x) %s\n\r", PERS(victim, ch), victim->in_room->name);
                                					break;
                						}
							}
						}
        				}
					else
					{
						Cprintf(ch, "%-28s %s\n\r", PERS(victim, ch), victim->in_room->name);
					}
				}
			}
		}

		if (!found)
			Cprintf(ch, "None\n\r");
	}
	else
	{
		found = FALSE;
		for (victim = char_list; victim != NULL; victim = victim->next)
		{
			if ((victim->in_room != NULL
				&& is_name(arg, victim->name)
				&& victim->in_room->area == ch->in_room->area)
  			 	&&
				(( can_see(ch, victim)
				&& !IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)
				&& !is_affected(victim, gsn_shapeshift)
				&& !is_affected(victim, gsn_concealment)
				&& !room_is_affected(victim->in_room, gsn_lair))
				||
				(  !IS_NPC(ch)
				&& IS_SET(ch->act, PLR_HOLYLIGHT)
				&& ch->in_room->area == victim->in_room->area)))
			{
				found = TRUE;
					if (room_is_affected(ch->in_room, gsn_winter_storm))
					{
						Cprintf(ch, "%-28s A raging winter storm\n\r", PERS(victim, ch));
					}
					else
					{
						Cprintf(ch, "%-28s %s\n\r", PERS(victim, ch), victim->in_room->name);
					}
				break;
			}
		}
		if (!found)
			act("You didn't find any $T.", ch, NULL, arg, TO_CHAR, POS_RESTING);
	}

	return;
}


void
do_consider(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	char *msg;
	int diff;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Consider killing whom?\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They're not here.\n\r");
		return;
	}

	if (is_safe(ch, victim))
	{
		Cprintf(ch, "Don't even think about it.\n\r");
		return;
	}

	diff = victim->level - ch->level;

	if (diff <= -10)
		msg = "You can kill $N naked and weaponless.";
	else if (diff <= -5)
		msg = "$N is no match for you.";
	else if (diff <= -2)
		msg = "$N looks like an easy kill.";
	else if (diff <= 1)
		msg = "The perfect match!";
	else if (diff <= 4)
		msg = "$N says 'Do you feel lucky, punk?'.";
	else if (diff <= 9)
		msg = "$N laughs at you mercilessly.";
	else
		msg = "Death will thank you for your gift.";

	act(msg, ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}


void
set_title(CHAR_DATA * ch, char *title)
{
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
	{
		bug("Set_title: NPC.", 0);
		return;
	}

	if (title[0] != ':' && title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?')
	{
		buf[0] = ' ';
		strcpy(buf + 1, title);
	}
	else
	{
		strcpy(buf, title);
	}

	free_string(ch->pcdata->title);
	ch->pcdata->title = str_dup(buf);
	return;
}


void
do_title(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->toggles, TOGGLES_NOTITLE))
	{
		Cprintf(ch, "Your title priviledges have been revoked.\n\r");
		return;
	}

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Change your title to what?\n\r");
		return;
	}

	if (colorstrlen(argument) > 60)
		argument[60] = '\0';

	smash_tilde(argument);
	set_title(ch, argument);
	Cprintf(ch, "Ok.\n\r");
}


void
do_secret(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	if (IS_IMMORTAL(ch))
	{
		Cprintf(ch, "This command is for mortals only.\n\r");
		return;
	}

	if (ch->short_descr != NULL)
	{
		Cprintf(ch, "You have already set your secret word.\n\rAsk an immortal if you want to change it.\n\r");
		return;
	}

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Change your secret word to what?\n\r");
		return;
	}

	if (strlen(argument) > 45)
		argument[45] = '\0';

	smash_tilde(argument);
	strcpy(buf, argument);
	free_string(ch->short_descr);
	ch->short_descr = str_dup(buf);

	Cprintf(ch, "Secret word changed.\n\r");
}


void
do_description(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];

	if (argument[0] != '\0')
	{
		buf[0] = '\0';
		smash_tilde(argument);

		if (argument[0] == '-')
		{
			int len;
			bool found = FALSE;

			if (ch->description == NULL || ch->description[0] == '\0')
			{
				Cprintf(ch, "No lines left to remove.\n\r");
				return;
			}

			strcpy(buf, ch->description);

			for (len = strlen(buf); len > 0; len--)
			{
				if (buf[len] == '\r')
				{
					if (!found)	/* back it up */
					{
						if (len > 0)
							len--;
						found = TRUE;
					}
					else
						/* found the second one */
					{
						buf[len + 1] = '\0';
						free_string(ch->description);
						ch->description = str_dup(buf);
						Cprintf(ch, "Your description is:\n\r");
						Cprintf(ch, "%s", ch->description ? ch->description : "(None).\n\r");
						return;
					}
				}
			}
			buf[0] = '\0';
			free_string(ch->description);
			ch->description = str_dup(buf);
			Cprintf(ch, "Description cleared.\n\r");
			return;
		}
		if (argument[0] == '+')
		{
			if (ch->description != NULL)
				strcat(buf, ch->description);
			argument++;
			while (isspace(*argument))
				argument++;
		}

		if (strlen(buf) >= 1536)
		{
			Cprintf(ch, "Description too long.\n\r");
			return;
		}

		strcat(buf, argument);
		strcat(buf, "\n\r");
		free_string(ch->description);
		ch->description = str_dup(buf);
	}

	Cprintf(ch, "Your description is:\n\r");
	Cprintf(ch, "%s", ch->description ? ch->description : "(None).\n\r");
	return;
}


void
do_report(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_INPUT_LENGTH];

	Cprintf(ch, "You say 'I have %d/%d hp %d/%d mana %d Pkills and got Pkilled %d times.'\n\r",
			ch->hit, MAX_HP(ch),
			ch->mana, MAX_MANA(ch),
			ch->pkills, ch->deaths);


	sprintf(buf, "$n says 'I have %d/%d hp %d/%d mana %d Pkills and got Pkilled %d times.'",
			ch->hit, MAX_HP(ch),
			ch->mana, MAX_MANA(ch),
			ch->pkills, ch->deaths);

	act(buf, ch, NULL, NULL, TO_ROOM, POS_RESTING);

}


void
do_practice(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	int rating;
	int sn;
	bool Hack;

	if (IS_NPC(ch))
		return;

	if (argument[0] == '\0')
	{
		int col;

		col = 0;

		strcpy(buf, "");

		for (sn = 0; sn < MAX_SKILL; sn++)
		{
			if (skill_table[sn].name == NULL)
				break;
			if (ch->level < skill_table[sn].skill_level[ch->charClass] || ch->pcdata->learned[sn] < 1)	/* skill is not known */
				continue;

			if (sn == gsn_acid_breath
				|| sn == gsn_fire_breath
				|| sn == gsn_gas_breath
				|| sn == gsn_frost_breath
				|| sn == gsn_lightning_breath)
				sprintf(buf, "%s%-18s %3d%%  ", buf, "breath", ch->pcdata->learned[sn]);
			else
				sprintf(buf, "%s%-18s %3d%%  ", buf, skill_table[sn].name, ch->pcdata->learned[sn]);

			if (++col % 3 == 0)
			{
				Cprintf(ch, "%s\n\r", buf);
				strcpy(buf, "");
			}
		}

		if (col % 3 != 0)
			Cprintf(ch, "%s\n\r", buf);

		Cprintf(ch, "You have %d practice sessions left.\n\r", ch->practice);
	}
	else
	{
		CHAR_DATA *mob;
		int adept;

		if (!IS_AWAKE(ch))
		{
			Cprintf(ch, "In your dreams, or what?\n\r");
			return;
		}

		for (mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room)
		{
			if (IS_NPC(mob) && IS_SET(mob->act, ACT_PRACTICE))
				break;
		}

		if (mob == NULL)
		{
			Cprintf(ch, "You can't do that here.\n\r");
			return;
		}

		if (ch->practice <= 0)
		{
			Cprintf(ch, "You have no practice sessions left.\n\r");
			return;
		}
		sn = find_spell(ch, argument);
		Hack = FALSE;
		if (!str_prefix(argument, "breath"))
		{
			if (ch->race == race_lookup("black dragon"))
				sn = gsn_acid_breath;
			else if (ch->race == race_lookup("red dragon"))
				sn = gsn_fire_breath;
			else if (ch->race == race_lookup("green dragon"))
				sn = gsn_gas_breath;
			else if (ch->race == race_lookup("white dragon"))
				sn = gsn_frost_breath;
			else if (ch->race == race_lookup("blue dragon"))
				sn = gsn_lightning_breath;
			else
				sn = -1;
			Hack = TRUE;
		}
		if (sn < 0 || (!IS_NPC(ch) && (ch->level < skill_table[sn].skill_level[ch->charClass] || ch->pcdata->learned[sn] < 1 /* skill is not known */ )))
		{
			Cprintf(ch, "You can't practice that.\n\r");
			return;
		}

		adept = IS_NPC(ch) ? 100 : class_table[ch->charClass].skill_adept;
		if (!IS_NPC(ch) && adept < 90 && ch->race == race_lookup("human"))
			adept += 10;

		if (ch->pcdata->learned[sn] >= adept)
		{
			Cprintf(ch, "You are already learned at %s.\n\r", Hack ? "breath" : skill_table[sn].name);
		}
		else
		{
			rating = skill_table[sn].rating[ch->charClass];
			if (rating <= 0)
				rating = 8;
			ch->practice--;
			if(NEWBIE_REMORT_RECLASS(ch) && IS_WEAPON_SN(sn)) {
				ch->pcdata->learned[sn] = 40;
			}
			else {
				ch->pcdata->learned[sn] +=
					int_app[get_curr_stat(ch, STAT_INT)].learn / rating;
			}
			if (ch->pcdata->learned[sn] < adept)
			{
				act("You practice $T.", ch, NULL, Hack ? "breath" : skill_table[sn].name, TO_CHAR, POS_RESTING);
				act("$n practices $T.", ch, NULL, Hack ? "breath" : skill_table[sn].name, TO_ROOM, POS_RESTING);
			}
			else
			{
				ch->pcdata->learned[sn] = adept;
				act("You are now learned at $T.", ch, NULL, Hack ? "breath" : skill_table[sn].name, TO_CHAR, POS_RESTING);
				act("$n is now learned at $T.", ch, NULL, Hack ? "breath" : skill_table[sn].name, TO_ROOM, POS_RESTING);
			}
		}
	}
	return;
}


/*
 * 'Wimpy' originally by Dionysos.
 */
void
do_wimpy(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	int wimpy;

	one_argument(argument, arg);

	if (race_lookup("giant") == ch->race)
	{
		if (ch->remort > 0)
		{
			Cprintf(ch, "A big giant like you doesn't need to wimp out.\n\r");
			ch->wimpy = 0;
			return;
		}
	}

	if (arg[0] == '\0')
		wimpy = MAX_HP(ch) / 5;
	else
		wimpy = atoi(arg);

	if (wimpy < 0)
	{
		Cprintf(ch, "Your courage exceeds your wisdom.\n\r");
		return;
	}

	if (wimpy > MAX_HP(ch) / 2)
	{
		Cprintf(ch, "Such cowardice ill becomes you.\n\r");
		return;
	}

	ch->wimpy = wimpy;
	Cprintf(ch, "Wimpy set to %d hit points.\n\r", wimpy);
	return;
}


void
do_password(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char *pArg;
	char *pwdnew;
	char *p;
	char cEnd;

	if (IS_NPC(ch))
		return;

	/*
	 * Can't use one_argument here because it smashes case.
	 * So we just steal all its code.  Bleagh.
	 */
	pArg = arg1;
	while (isspace(*argument))
		argument++;

	cEnd = ' ';
	if (*argument == '\'' || *argument == '"')
		cEnd = *argument++;

	while (*argument != '\0')
	{
		if (*argument == cEnd)
		{
			argument++;
			break;
		}
		*pArg++ = *argument++;
	}
	*pArg = '\0';

	pArg = arg2;
	while (isspace(*argument))
		argument++;

	cEnd = ' ';
	if (*argument == '\'' || *argument == '"')
		cEnd = *argument++;

	while (*argument != '\0')
	{
		if (*argument == cEnd)
		{
			argument++;
			break;
		}
		*pArg++ = *argument++;
	}
	*pArg = '\0';

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: password <old> <new>.\n\r");
		return;
	}

	if (strcmp(crypt(arg1, ch->pcdata->pwd), ch->pcdata->pwd))
	{
		WAIT_STATE(ch, 40);
		Cprintf(ch, "Wrong password.  Wait 10 seconds.\n\r");
		return;
	}

	if (strlen(arg2) < 5)
	{
		Cprintf(ch, "New password must be at least five characters long.\n\r");
		return;
	}

	/*
	 * No tilde allowed because of player file format.
	 */
	pwdnew = crypt(arg2, ch->name);
	for (p = pwdnew; *p != '\0'; p++)
	{
		if (*p == '~')
		{
			Cprintf(ch, "New password not acceptable, try again.\n\r");
			return;
		}
	}

	free_string(ch->pcdata->pwd);
	ch->pcdata->pwd = str_dup(pwdnew);
	save_char_obj(ch, FALSE);
	Cprintf(ch, "Ok.\n\r");
	return;
}


void
do_lore(CHAR_DATA * ch, char *argument) {
    OBJ_DATA *obj;
    char arg[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg);

    if (*arg == '\0') {
        Cprintf(ch, "Lore what?\n\r");
        return;
    }

    obj = get_obj_carry_or_wear(ch, arg);
    if (obj == NULL) {
        Cprintf(ch, "You do not see that here.\n\r");
        return;
    }

    if (number_percent() < get_skill(ch, gsn_lore)) {
        check_improve(ch, gsn_lore, TRUE, 4);

        lore_obj(ch, obj);
    } else {
        Cprintf(ch, "Your knowledge does not extend that far.\n\r");
        check_improve(ch, gsn_lore, FALSE, 4);
    }

    return;
}

void
lore_obj(CHAR_DATA *ch, OBJ_DATA *obj) {
    AFFECT_DATA *paf;
    ROOM_INDEX_DATA *location;

    /* General Info */
    Cprintf(ch, "Object '%s' is of type %s.", obj->short_descr, item_name(obj->item_type) );

    if (obj->clan_status == CS_CLANNER) {
        Cprintf(ch, " {r[Clanner Only]{x");
    }

    if(obj->clan_status == CS_NONCLANNER) {
        Cprintf(ch, " {r[Non-Clanner Only]{x");
    }

    Cprintf(ch, "\n\r");

    Cprintf(ch, "Description: %s\n\r", obj->description);
    Cprintf(ch, "Keywords '%s'\n\r", obj->name);
    Cprintf(ch, "Weight %d lbs, Value %d silver, level is %d, Material is %s.\n\r",
                        obj->weight / 10,
                        obj->cost,
                        obj->level,
                        obj->material);
    Cprintf(ch, "Extra flags: %s\n\r", extra_bit_name(obj->extra_flags));

    /* Wear location information */
    if (IS_SET(obj->wear_flags, ITEM_WEAR_FINGER)) {
        Cprintf(ch, "Item is worn on fingers.\n\r");
    } else if (obj->item_type == ITEM_LIGHT) {
        Cprintf(ch, "Item is used as a light.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_NECK)) {
        Cprintf(ch, "Item is worn around the neck.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_BODY)) {
        Cprintf(ch, "Item is worn on torso.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_HEAD)) {
        Cprintf(ch, "Item is worn on the head.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_LEGS)) {
        Cprintf(ch, "Item is worn on the legs.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_FEET)) {
        Cprintf(ch, "Item is worn on the feet.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_HANDS)) {
        Cprintf(ch, "Item is worn on the hands.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_ARMS)) {
        Cprintf(ch, "Item is worn on the arms.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_SHIELD)) {
        Cprintf(ch, "Item is worn as shield.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_ABOUT)) {
        Cprintf(ch, "Item is worn about the body.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_WAIST)) {
        Cprintf(ch, "Item is worn around the waist.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_WRIST)) {
        Cprintf(ch, "Item is worn on the wrists.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WIELD)) {
        Cprintf(ch, "Item is wielded as a weapon.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_HOLD)) {
        Cprintf(ch, "Item is held in the hand.\n\r");
    } else if (IS_SET(obj->wear_flags, ITEM_WEAR_FLOAT)) {
        Cprintf(ch, "Item is floating magically.\n\r");
    } else {
        Cprintf(ch, "Item is not wearable.\n\r");
    }

	/* Other stuff using wear flags */
	if(obj->respawn_owner != NULL)
		Cprintf(ch, "Item belongs to %s.\n\r", obj->respawn_owner);
	if(IS_SET(obj->wear_flags, ITEM_NO_SAC))
		Cprintf(ch, "Item may not be sacrificed.\n\r");
	if(IS_SET(obj->wear_flags, ITEM_FLAG_EMBELISHED))
		Cprintf(ch, "Item has been embellished.\n\r");
	if(IS_SET(obj->wear_flags, ITEM_FLAG_REPLICATED))
		Cprintf(ch, "Item is a replication.\n\r");
	if(IS_SET(obj->wear_flags, ITEM_NORECALL))
		Cprintf(ch, "Item silences recall prayer while worn.\n\r");
	if(IS_SET(obj->wear_flags, ITEM_NOGATE))
		Cprintf(ch, "Item disrupts transportation magic while worn.\n\r");
	if(IS_SET(obj->wear_flags, ITEM_INCOMPLETE)) {
		Cprintf(ch, "Item can be used as part of a crafted item.\n\r");
	}
	if(IS_SET(obj->wear_flags, ITEM_CRAFTED)) {
		Cprintf(ch, "Item has been magically crafted.\n\r");
	}
	if(IS_SET(obj->wear_flags, ITEM_NEWBIE)) {
		Cprintf(ch, "Item can only be used by new characters.\n\r");
	}

	if(obj->item_type == ITEM_WEAPON
	&& IS_WEAPON_STAT(obj, WEAPON_INTELLIGENT)) {
		Cprintf(ch, "Item is level %d %sand has %d xp.\n\r",
			obj->extra[0] > 0 ? obj->extra[0] : 1,
			obj->extra[0] == 10 ? "(max) " : "", obj->extra[1]);
	}

	if(IS_SET(obj->wear_flags, ITEM_CHARGED) && obj->special[4] > 0) {
		Cprintf(ch, "Item contains %d/%d charges of level %d %d%% %s.\n\r",
		obj->special[2], obj->special[1], obj->special[0], obj->special[4], skill_table[obj->special[3]].name);
	}
	/* info by item types */
        switch (obj->item_type)
        {
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
        	Cprintf(ch, "Casts level %d spells of:", obj->value[0]);
                if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL)
                	Cprintf(ch, " '%s'", skill_table[obj->value[1]].name);
		if (obj->value[2] > 0 && obj->value[2] < MAX_SKILL)
                        Cprintf(ch, " '%s'", skill_table[obj->value[2]].name);
                if (obj->value[3] > 0 && obj->value[3] < MAX_SKILL)
                        Cprintf(ch, " '%s'", skill_table[obj->value[3]].name);
                if (obj->value[4] > 0 && obj->value[4] < MAX_SKILL)
                        Cprintf(ch, " '%s'", skill_table[obj->value[4]].name);
                Cprintf(ch, ".\n\r");
                break;
	case ITEM_WAND:
        case ITEM_STAFF:
        	Cprintf(ch, "Has %d/%d charges of level %d",
			obj->value[2], obj->value[1], obj->value[0]);
                if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL)
	                Cprintf(ch, " '%s'", skill_table[obj->value[3]].name);
                Cprintf(ch, ".\n\r");
                break;
	case ITEM_DRINK_CON:
                Cprintf(ch, "It holds %s-colored %s.\n\r",
			liq_table[obj->value[2]].liq_color, liq_table[obj->value[2]].liq_name);
                break;
	case ITEM_CONTAINER:
                Cprintf(ch, "Capacity: %d lbs  Per item: %d lbs flags: %s\n\r",
			obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));

                if (obj->value[4] != 100)
			Cprintf(ch, "Weight multiplier: %d%%\n\r", obj->value[4]);
                break;
        case ITEM_RECALL:
                location = get_room_index(obj->value[0]);
                if (location == NULL)
			break;
                Cprintf(ch, "Charges: %d  Max: %d  Recalls to: %s.\n\r",
			obj->value[1], obj->value[2], location->name);
                break;
	case ITEM_THROWING:
                Cprintf(ch, "Item is a missile weapon.\n\r");
                Cprintf(ch, "Damage is %dd%d (average %d).\n\r",
	                obj->value[0], obj->value[1],
                        (1 + obj->value[1]) * obj->value[0] / 2);

                Cprintf(ch, "Casts level %d '%s'\n\r",
	                obj->value[3],
                        skill_table[obj->value[4]].name);
                break;
	case ITEM_AMMO:
		Cprintf(ch, "Item is ammunition [%d remaining].\n\r",
			obj->weight / 10);


		if(obj->value[0] && obj->value[1])
			Cprintf(ch, "Damage is %dd%d (average %d).\n\r",
        			obj->value[0], obj->value[1],
        			(1 + obj->value[1]) * obj->value[0] / 2);

		if(obj->value[4])
			Cprintf(ch, "Casts level %d '%s'\n\r",
        			obj->value[3],
        			skill_table[obj->value[4]].name);
		break;
	case ITEM_CHARM:
		Cprintf(ch, "Item is used with ninjitsu skill.\n\r");
		Cprintf(ch, "Damage is %dd%d (average %d).\n\r",
			obj->value[1], obj->value[2],
 			(1 + obj->value[2]) * obj->value[1] / 2);
		Cprintf(ch, "Ninjitsu effect: %s\n\r", flag_string(vuln_flags, obj->value[4]));
		Cprintf(ch, "Deals %s damage.\n\r", flag_string(damtype_flags, obj->value[3]));
		break;
	case ITEM_WEAPON:
        	Cprintf(ch, "Weapon type is ");
               	switch (obj->value[0]) {
                case (WEAPON_EXOTIC):
                       	Cprintf(ch, "exotic.\n\r"); break;
                case (WEAPON_SWORD):
                        Cprintf(ch, "sword.\n\r"); break;
                case (WEAPON_DAGGER):
                        Cprintf(ch, "dagger.\n\r"); break;
                case (WEAPON_SPEAR):
                        Cprintf(ch, "spear.\n\r"); break;
                case (WEAPON_MACE):
                        Cprintf(ch, "mace/club.\n\r"); break;
                case (WEAPON_AXE):
                        Cprintf(ch, "axe.\n\r"); break;
		case (WEAPON_FLAIL):
                      	Cprintf(ch, "flail.\n\r"); break;
                case (WEAPON_WHIP):
			Cprintf(ch, "whip.\n\r"); break;
                case (WEAPON_POLEARM):
		        Cprintf(ch, "polearm.\n\r"); break;
		case (WEAPON_KATANA):
			Cprintf(ch, "katana.\n\r"); break;
		case (WEAPON_RANGED):
			Cprintf(ch, "ranged.\n\r"); break;
                default:
                        Cprintf(ch, "unknown.\n\r"); break;
                }


		if((paf = affect_find(obj->affected, gsn_magic_sheath)) != NULL
		&& (paf->extra == SHEATH_DICECOUNT
		|| paf->extra == SHEATH_DICETYPE)) {
			if(paf->extra == SHEATH_DICECOUNT) {
				Cprintf(ch, "Damage is %dd%d (average %d).\n\r",
        			obj->value[1]+1, obj->value[2],
        			(1 + obj->value[2]) * (obj->value[1]+1) / 2);
			}
			if(paf->extra == SHEATH_DICETYPE) {
				Cprintf(ch, "Damage is %dd%d (average %d).\n\r",
        			obj->value[1], obj->value[2]+1,
        			(2 + obj->value[2]) * obj->value[1] / 2);
			}
		}
		else {
                	Cprintf(ch, "Damage is %dd%d (average %d).\n\r",
			obj->value[1], obj->value[2],
			(1 + obj->value[2]) * obj->value[1] / 2);
		}

		/* weapon flags */
                if (obj->value[4])
                      	Cprintf(ch, "Weapons flags: %s\n\r",
				weapon_bit_name(obj->value[4]));
                        break;
        case ITEM_ARMOR:
		Cprintf(ch, "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r",
			obj->value[0], obj->value[1], obj->value[2], obj->value[3]);
		break;

	case ITEM_SHEATH:
		if(obj->value[3] == 0) {
			Cprintf(ch, "Fits a single %s class weapon.\n\r", weapon_name(obj->value[0]));
		}
		else {
			Cprintf(ch, "Custom made to fit one particular weapon.\n\r");
		}
		switch(obj->value[1]) {
		case SHEATH_FLAG:
			Cprintf(ch, "Adds %s flags when drawn.\n\r",
				weapon_bit_name(obj->value[2]));
			break;
		case SHEATH_DICETYPE:
			Cprintf(ch, "Increases type of damdice when drawn.\n\r");
			break;
		case SHEATH_DICECOUNT:
			Cprintf(ch, "Increases number of damdice when drawn.\n\r");
			break;
		case SHEATH_HITROLL:
			Cprintf(ch, "Modifies hitroll by %d when drawn.\n\r",
				obj->value[2]);
			break;
		case SHEATH_DAMROLL:
			Cprintf(ch, "Modifies damroll by %d when drawn.\n\r",
				obj->value[2]);
			break;
		case SHEATH_QUICKDRAW:
			Cprintf(ch, "Allows quickdraw attacks when drawn.\n\r");
			break;
		case SHEATH_SPELL:
			Cprintf(ch, "Casts %s when drawn.\n\r",
				skill_table[obj->value[2]].name);
			break;
		}
	}

	/* Ok, used to be more complex for including things like TO_AFFECTS
	   TO_WEAPON and so on, but on Redemption there is no way to add such
	   things using OLC (yet) so I took it out of lore obj. */

	if (!obj->enchanted) {
        	for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
                {
                	if (paf->location != APPLY_NONE && paf->modifier != 0)
                        {
                        	Cprintf(ch, "Object modifies %s by %d%s\n\r",
					affect_loc_name(paf->location), paf->modifier, IS_SET(paf->bitvector, AFF_DAYLIGHT) ? " during the day." : IS_SET(paf->bitvector, AFF_DARKLIGHT) ? " during the night." : ".");
                        }
                }
	}
	/* Note we DO have a few spells/skills such as envenom that use affects
	   in that way, so we will have to check for it on current weapon */
	for (paf = obj->affected; paf != NULL; paf = paf->next)
        {
        	if (paf->location != APPLY_NONE && paf->modifier != 0) {
			if(paf->type == -1
			|| paf->type == gsn_reserved
			|| paf->type == gsn_enchant_weapon
			|| paf->type == gsn_enchant_armor)
				Cprintf(ch, "Object");
                	else
				Cprintf(ch, "%s",
				skill_table[paf->type].name);
			Cprintf(ch, " modifies %s by %d%s",
			affect_loc_name(paf->location), paf->modifier, IS_SET(paf->bitvector, AFF_DAYLIGHT) ? " during the day" : IS_SET(paf->bitvector, AFF_DARKLIGHT) ? " during the night" : "");

                        if (paf->duration > -1)
				Cprintf(ch, " for %d hours.\n\r", paf->duration);
                        else
                                Cprintf(ch, ".\n\r");
                }

		if (paf->bitvector)
		{

			if (IS_SET(paf->bitvector, AFF_DARKLIGHT)
			|| IS_SET(paf->bitvector, AFF_DAYLIGHT))
				continue;

			Cprintf(ch, "%s ", skill_table[paf->type].name);
                	switch (paf->where) {
                        case TO_AFFECTS:
                        	Cprintf(ch, "adds %s affect",
				affect_bit_name(paf->bitvector)); break;
                        case TO_OBJECT:
                                Cprintf(ch, "adds %s object flag",
				extra_bit_name(paf->bitvector)); break;
                        case TO_WEAPON:
                                Cprintf(ch,  "adds %s weapon flag",
				weapon_bit_name(paf->bitvector)); break;
                        case TO_IMMUNE:
                                Cprintf(ch, "adds immunity to %s",
				imm_bit_name(paf->bitvector)); break;
                        case TO_RESIST:
                                Cprintf(ch, "adds resistance to %s",
	 			imm_bit_name(paf->bitvector)); break;
			case TO_VULN:
                                Cprintf(ch, "adds vulnerability to %s",
				imm_bit_name(paf->bitvector)); break;
                        default:
                                Cprintf(ch, "Unknown bit %d: %d",
				paf->where, paf->bitvector);
                        }
			if (paf->duration > -1)
				Cprintf(ch, " for %d hours.\n\r", paf->duration);
                        else
                                Cprintf(ch, ".\n\r");
		}
	}

	// Here can show some specific information for runist affects
	if((paf = affect_find(obj->affected, gsn_blade_rune)) != NULL) {
		if(paf->location == APPLY_NONE
		&& paf->modifier == BLADE_RUNE_SPEED) {
			Cprintf(ch, "blade rune strikes with great speed for %d hours.\n\r",
				paf->duration);
		}
		if(paf->location == APPLY_NONE
		&& paf->modifier == BLADE_RUNE_ACCURACY) {
			Cprintf(ch, "blade rune strikes with greater accuracy for %d hours.\n\r",
				paf->duration);
		}
		if(paf->location == APPLY_NONE
		&& paf->modifier == BLADE_RUNE_PARRYING) {
			Cprintf(ch, "blade rune deflects incoming attacks for %d hours.\n\r",
				paf->duration);
		}
	}
	if((paf = affect_find(obj->affected, gsn_burst_rune)) != NULL) {
		if(paf->modifier == gsn_acid_blast)
	        	Cprintf(ch, "burst rune deals corrosive damage for %d hours.\n\r", paf->duration);
		if(paf->modifier == gsn_ice_bolt)
	        	Cprintf(ch, "burst rune deals cold damage for %d hours.\n\r", paf->duration);
		if(paf->modifier == gsn_blast_of_rot)
	        	Cprintf(ch, "burst rune deals poison damage for %d hours.\n\r", paf->duration);
		if(paf->modifier == gsn_lightning_bolt)
			Cprintf(ch, "burst rune deals lightning damage for %d hours.\n\r", paf->duration);
		if(paf->modifier == gsn_flamestrike)
        		Cprintf(ch, "burst rune deals fire damage for %d hours.\n\r", paf->duration);
		if(paf->modifier == gsn_tsunami)
			Cprintf(ch, "burst rune deals drowning damage for %d hours.\n\r", paf->duration);
	}
	if((paf = affect_find(obj->affected, gsn_balance_rune)) != NULL) {
                Cprintf(ch, "balance rune stabilizes enchants for %d hours.\n\r",
                        paf->duration);
        }
	if((paf = affect_find(obj->affected, gsn_soul_rune)) != NULL) {
		Cprintf(ch, "soul rune object will follow you to the grave for %d hours.\n\r",
			paf->duration);
	}

	if((paf = affect_find(obj->affected, gsn_paint_power)) != NULL) {
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_ANIMATES) {
			Cprintf(ch, "power tattoo can be animated using the animate command.\n\r");
		}
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_MV_SPELLS) {
			Cprintf(ch, "power tattoo casts spells without using mana.\n\r");
		}
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_HP_REGEN) {
			Cprintf(ch, "power tattoo increases healing rate.\n\r");
		}
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_MANA_REGEN) {
			Cprintf(ch, "power tattoo increases mana recovery rate.\n\r");
		}
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_HP_PER_KILL) {
			Cprintf(ch, "power tattoo recovers hp from each death.\n\r");
		}
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_MANA_PER_KILL) {
			Cprintf(ch, "power tattoo recovers mana from each death.\n\r");
		}
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_EXTRA_DP) {
			Cprintf(ch, "power tattoo increases the favour of deities.\n\r");
		}
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_PHYSICAL_RESIST) {
			Cprintf(ch, "power tattoo reduces physical damage taken.\n\r");
		}
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_MAGICAL_RESIST) {
			Cprintf(ch, "power tattoo reduces magical damage taken.\n\r");
		}
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_LEARN_SPELL) {
			Cprintf(ch, "power tattoo grants magical knowledge of %s.\n\r",
				skill_table[paf->extra].name);
		}
		if(paf->type == gsn_paint_power
		&& paf->modifier == TATTOO_RAISES_CASTING) {
			Cprintf(ch, "power tattoo raises casting level.\n\r");
		}
	}

	if(obj_is_affected(obj, gsn_corruption))
		Cprintf(ch, "Object doesn't look quite right.\n\r");
}

char *
godName(CHAR_DATA * ch)
{
	return (ch->deity_type != 0) ? deity_table[ch->deity_type - 1].name : ((ch->in_room) && (ch->in_room->area->continent == 0)) ? "Bosco" : "Selina";
}
