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
 **************************************************************************/

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "merc.h"
#include "clan.h"
#include "recycle.h"
#include "interp.h"
#include "utils.h"


/* command procedures needed */
DECLARE_DO_FUN(do_quit);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_kill);

bool in_enemy_hall(CHAR_DATA *);
bool tell_private(CHAR_DATA *ch, CHAR_DATA *victim, char *argument);
extern void raw_kill(CHAR_DATA *);

/* RT code to delete yourself */
void
do_delet(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "You must type the full command to delete yourself.\n\r");
}


bool
is_hushed(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int t;

	if (IS_NPC(ch) || !ch->pcdata)
		return FALSE;

	if (is_affected(ch, gsn_wail) && !IS_IMMORTAL(ch))
		return TRUE;

	for (t = 0; t < 5; t++)
		if (ch->pcdata->hush[t])
			if (!str_cmp(ch->pcdata->hush[t], victim->name))
				return TRUE;

	return FALSE;
}


/* Add the following two function definitions to interp.c as well */
void
do_hush(CHAR_DATA * ch, char *argument) {
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    int count, max, t;
    DESCRIPTOR_DATA *d;
    bool found;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch) || !ch->pcdata) {
        Cprintf(ch, "Only players can hush.\n\r");
        return;
    }

    if (ch->level < 5) {
        Cprintf(ch, "You must be higher level to use this command.\n\r");
        return;
    }

    one_argument(argument, arg);

    if (!arg[0]) {
        found = FALSE;

        for (t = 0; t < 5; t++) {
            if (ch->pcdata->hush[t]) {
                if (!found) {
                    found = TRUE;
                    Cprintf(ch, "Hush list:\n\r");
                }

                Cprintf(ch, "  %s\n\r", capitalize(ch->pcdata->hush[t]));
            }
        }

        if (!found) {
            Cprintf(ch, "No characters hushed.\n\r");
        }

        return;
    }

    victim = get_char_world(ch, arg, TRUE);

    if (victim == NULL || IS_NPC(victim)) {
        Cprintf(ch, "Player not found.\n\r");
        return;
    }

    if (victim->level > 54) {
        Cprintf(ch, "You failed.\n\r");
        return;
    }

    found = FALSE;

    for (t = 0; t < 5; t++) {
        if (ch->pcdata->hush[t] == NULL) {
            found = TRUE;
            break;
        }
    }

    if (!found) {
        Cprintf(ch, "You've already reached your max hush limit.\n\r");
        return;
    }

    ch->pcdata->hush[t] = str_dup(victim->name);
    Cprintf(ch, "Ok.\n\r");

    max = 0;
    count = 0;

    for (d = descriptor_list; d != NULL; d = d->next) {
        CHAR_DATA *fch;

        fch = d->original ? d->original : d->character;

        if (d->connected == CON_PLAYING) {
            max++;

            if (is_hushed(fch, victim)) {
                count++;
            }
        }
    }

    if (max > 10 && count > max / 2) {
        if (!IS_SET(victim->comm, COMM_NOCHANNELS)) {
            Cprintf(victim, "By popular consensus, your channels have been revoked.\n\r");
            SET_BIT(victim->comm, COMM_NOCHANNELS);

            sprintf(buf, "%s has been nochanneled by hush.", capitalize(victim->name));
            log_string("%s", buf);
            wiznet(buf, NULL, NULL, WIZ_PENALTIES, 0, get_trust(victim));

            make_note("Penalties", "Mud", "all", capitalize(victim->name), 30, "Nochanneled by hush") ;
        }

        return;
    }
}


void
do_unhush(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	int t;

	if (IS_NPC(ch) || !ch->pcdata)
	{
		Cprintf(ch, "Only players can unhush.\n\r");
		return;
	}

	one_argument(argument, arg);

	if (!arg[0])
	{
		Cprintf(ch, "Who do you wish to unhush?");
		return;
	}

	for (t = 0; t < 5; t++)
	{
		if (ch->pcdata->hush[t] && !str_prefix(arg, ch->pcdata->hush[t]))
		{
			free_string(ch->pcdata->hush[t]);
			ch->pcdata->hush[t] = NULL;
			Cprintf(ch, "Ok.\n\r");
			return;
		}
	}

	Cprintf(ch, "That player is not hushed.");
}


void
talk_auction(char *argument)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *original;


	for (d = descriptor_list; d != NULL; d = d->next)
	{
		if (d->character != NULL)
		{
			original = d->original ? d->original : d->character;	/* if switched */
			if ((d->connected == CON_PLAYING) && !IS_SET(d->character->comm, COMM_NOAUCTION))
				Cprintf(d->character, "{MAUCTIONED by %s:{x %s\n\r", auction->seller->name, argument);
		}
	}
}


void
do_retir(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "You must type the command in full.  Syntax: retire <password>\n\r");
	return;
}


void
do_retire(CHAR_DATA * ch, char *argument)
{
	int sn;
	OBJ_DATA *obj;
	OBJ_DATA *obj_in;

	if (IS_NPC(ch))
		return;

	if (clan_table[ch->clan].pkiller == 0)
	{
		Cprintf(ch, "You are not in a clan.\n\r");
		return;
	}

	if (IS_SET(ch->act, PLR_KILLER) || IS_SET(ch->act, PLR_THIEF))
	{
		Cprintf(ch, "You cannot retire with a killer or a thief.\n\r");
		return;
	}

	if (ch->level > 25)
	{
		Cprintf(ch, "You had your chance to think about it before. Sorry.\n\r");
		return;
	}

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Syntax: retire <password>\n\r");
		return;
	}

	if (!strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd))
	{
		stop_fighting(ch, TRUE);
		ch->clan = 0;
		REMOVE_BIT(ch->wiznet, CAN_CLAN);	//for when we R/R
		act("$n disappears.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		char_from_room(ch);
		char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
		act("$n appears in the room.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		do_look(ch, "auto");
		ch->bounty = 0;

		for (sn = 0; sn < MAX_SKILL; sn++)
		{
			if (skill_table[sn].name == NULL)
				break;

			if (ch->level < skill_table[sn].skill_level[ch->charClass] || ch->pcdata->learned[sn] < 1 /* skill is not known */ )
				continue;

			ch->pcdata->learned[sn] = (ch->pcdata->learned[sn]) / 2;

			if (ch->pcdata->learned[sn] == 0)
				ch->pcdata->learned[sn] = 1;

			// Klunky fix for new clan status flags
			for(obj = ch->carrying; obj != NULL; obj = obj->next_content) {
				if(obj->contains) {
					for(obj_in = obj->contains; obj_in != NULL; obj_in = obj_in->next_content) {
						if(obj_in->clan_status == CS_CLANNER)
							obj_in->clan_status = CS_NONCLANNER;
					}
				}

				if(obj->clan_status == CS_CLANNER)
					obj->clan_status = CS_NONCLANNER;
			}


		}

		Cprintf(ch, "You are now a non-clanner.\n\r");
		return;
	}
	else
	{
		Cprintf(ch, "Wrong Password.  Syntax: retire <password>\n\r");
		return;
	}
}


void
do_delete(CHAR_DATA * ch, char *argument)
{
	char strsave[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;

	if (ch->level == 53 || ch->trust == 53)
	{
		Cprintf(ch, "Please pass on leadership of your clan first.\n\r");
		return;
	}

	/* is confermed once */
	if (ch->pcdata->confirm_delete)
	{
		if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd))
		{
			Cprintf(ch, "Delete status removed.\n\r");
			ch->pcdata->confirm_delete = FALSE;
			return;
		}
		else
		{
			if (ch->level == 52 || ch->trust == 52)
			{
				demote_recruiter(ch);
			}

			sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(ch->name));
			wiznet("Ashes to ashes, dust to dust. $N turns $Mself into line noise.", ch, NULL, 0, 0, 0);
			stop_fighting(ch, TRUE);
			quit_character(ch);
			unlink(strsave);
			return;
		}
	}

	if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd))
	{
		Cprintf(ch, "Wrong password.\n\r");
		return;
	}

	Cprintf(ch, "Type delete again to confirm this command.\n\r");
	Cprintf(ch, "WARNING: this command is irreversible.\n\r");
	Cprintf(ch, "Typing delete with an argument will undo delete status.\n\r");
	ch->pcdata->confirm_delete = TRUE;
	wiznet("$N is contemplating deletion.", ch, NULL, 0, 0, get_trust(ch));
}


/* RT code to display channel status */
void
do_channels(CHAR_DATA * ch, char *argument)
{
	/* lists all channels and their status */
	Cprintf(ch, "   channel     status\n\r");
	Cprintf(ch, "---------------------\n\r");

	Cprintf(ch, "gossip         %s\n\r", (!IS_SET(ch->comm, COMM_NOGOSSIP)) ? "ON" : "OFF");
	Cprintf(ch, "cgossip        %s\n\r", (!IS_SET(ch->comm, COMM_NOCGOSS)) ? "ON" : "OFF");
	Cprintf(ch, "OOC            %s\n\r", (!IS_SET(ch->comm, COMM_NOOOC)) ? "ON" : "OFF");
	Cprintf(ch, "music          %s\n\r", (!IS_SET(ch->comm, COMM_NOMUSIC)) ? "ON" : "OFF");
	Cprintf(ch, "Q/A            %s\n\r", (!IS_SET(ch->comm, COMM_NOQUESTION)) ? "ON" : "OFF");
	Cprintf(ch, "Auction        %s\n\r", (!IS_SET(ch->comm, COMM_NOAUCTION)) ? "ON" : "OFF");
	Cprintf(ch, "grats          %s\n\r", (!IS_SET(ch->comm, COMM_NOGRATS)) ? "ON" : "OFF");

	if (IS_IMMORTAL(ch))
		Cprintf(ch, "god channel    %s\n\r", (!IS_SET(ch->comm, COMM_NOWIZ)) ? "ON" : "OFF");

	Cprintf(ch, "shouts         %s\n\r", (!IS_SET(ch->comm, COMM_SHOUTSOFF)) ? "ON" : "OFF");
	Cprintf(ch, "tells          %s\n\r", (!IS_SET(ch->comm, COMM_DEAF)) ? "ON" : "OFF");
	Cprintf(ch, "newbie hints   %s\n\r", (!IS_SET(ch->comm, COMM_NONEWBIE)) ? "ON" : "OFF");
	Cprintf(ch, "quiet mode     %s\n\r", (IS_SET(ch->comm, COMM_QUIET)) ? "ON" : "OFF");
	Cprintf(ch, "bounties       %s\n\r", (IS_SET(ch->toggles, TOGGLES_NOBOUNTY)) ? "OFF" : "ON");

	if (IS_SET(ch->comm, COMM_AFK))
		Cprintf(ch, "You are AFK.\n\r");

	if (IS_SET(ch->comm, COMM_SNOOP_PROOF))
		Cprintf(ch, "You are immune to snooping.\n\r");

	if (ch->lines != PAGELEN)
	{
		if (ch->lines)
			Cprintf(ch, "You display %d lines of scroll.\n\r", ch->lines + 2);
		else
			Cprintf(ch, "Scroll buffering is off.\n\r");
	}

	if (ch->prompt != NULL) {
	    cprintf(ch, FALSE, TRUE, "Your current prompt is: %s\n\r", ch->prompt);
		Cprintf(ch, "                        %s\n\r", ch->prompt);
	}

	if (IS_SET(ch->comm, COMM_NOSHOUT))
		Cprintf(ch, "You cannot shout.\n\r");

	if (IS_SET(ch->comm, COMM_NOTELL))
		Cprintf(ch, "You cannot use tell.\n\r");

	if (IS_SET(ch->comm, COMM_NOCHANNELS))
		Cprintf(ch, "You cannot use channels.\n\r");

	if (IS_SET(ch->comm, COMM_NOEMOTE))
		Cprintf(ch, "You cannot show emotions.\n\r");

}


/* RT deaf blocks out all shouts */
void
reverse(char *s)
{
	/*               ABCDEFGHIJKLMNOPQRSTUVWXYZ      abcdefghijklmnopqrstuvwxyz */
	char *replace = "WBCDYFGHPJKXMKAPQEULSVWXYZ      wbcdyfghpjkxmkapqeulsvwxyz";
	char *let = s;

	while ((*let != '\0') && (let - s < 300))
	{
		if (isalpha(*let))
			*let = *(replace + *let - 'A');

		let++;
	}

	return;
}


char *
makedrunk(char *string, CHAR_DATA * ch)
{
	/* This structure defines all changes for a character */
	struct struckdrunk drunk[] =
	{
		{3, 10,
		 {"a", "a", "a", "A", "aa", "ah", "Ah", "ao", "aw", "oa", "ahhhh"}},
		{8, 5,
		 {"b", "b", "b", "B", "B", "vb"}},
		{3, 5,
		 {"c", "c", "C", "cj", "sj", "zj"}},
		{5, 2,
		 {"d", "d", "D"}},
		{3, 3,
		 {"e", "e", "eh", "E"}},
		{4, 5,
		 {"f", "f", "ff", "fff", "fFf", "F"}},
		{8, 2,
		 {"g", "g", "G"}},
		{9, 6,
		 {"h", "h", "hh", "hhh", "Hhh", "HhH", "H"}},
		{7, 6,
		 {"i", "i", "Iii", "ii", "iI", "Ii", "I"}},
		{9, 5,
		 {"j", "j", "jj", "Jj", "jJ", "J"}},
		{7, 2,
		 {"k", "k", "K"}},
		{3, 2,
		 {"l", "l", "L"}},
		{5, 8,
		 {"m", "m", "mm", "mmm", "mmmm", "mmmmm", "MmM", "mM", "M"}},
		{6, 6,
		 {"n", "n", "nn", "Nn", "nnn", "nNn", "N"}},
		{3, 6,
		 {"o", "o", "ooo", "ao", "aOoo", "Ooo", "ooOo"}},
		{3, 2,
		 {"p", "p", "P"}},
		{5, 5,
		 {"q", "q", "Q", "ku", "ququ", "kukeleku"}},
		{4, 2,
		 {"r", "r", "R"}},
		{2, 5,
		 {"s", "ss", "zzZzssZ", "ZSssS", "sSzzsss", "sSss"}},
		{5, 2,
		 {"t", "t", "T"}},
		{3, 6,
		 {"u", "u", "uh", "Uh", "Uhuhhuh", "uhU", "uhhu"}},
		{4, 2,
		 {"v", "v", "V"}},
		{4, 2,
		 {"w", "w", "W"}},
		{5, 6,
		 {"x", "x", "X", "ks", "iks", "kz", "xz"}},
		{3, 2,
		 {"y", "y", "Y"}},
		{2, 9,
	 {"z", "z", "ZzzZz", "Zzz", "Zsszzsz", "szz", "sZZz", "ZSz", "zZ", "Z"}}
	};

	char buf[1024];
	char temp;
	int pos = 0;
	int drunklevel;
	int randomnum;

	/* Check how drunk a person is... */
	if (IS_NPC(ch))
		drunklevel = 0;
	else
		drunklevel = ch->pcdata->condition[COND_DRUNK];

	if (drunklevel > 0)
	{
		do
		{
			temp = toupper(*string);
			if ((temp >= 'A') && (temp <= 'Z'))
			{
				if (drunklevel > drunk[temp - 'A'].min_drunk_level)
				{
					randomnum = number_range(0, drunk[temp - 'A'].number_of_rep);
					strcpy(&buf[pos], drunk[temp - 'A'].replacement[randomnum]);
					pos += strlen(drunk[temp - 'A'].replacement[randomnum]);
				}
				else
					buf[pos++] = *string;
			}
			else
			{
				if ((temp >= '0') && (temp <= '9'))
				{
					temp = '0' + number_range(0, 9);
					buf[pos++] = temp;
				}
				else
					buf[pos++] = *string;
			}
		}
		while (*string++);
		buf[pos] = '\0';		/* Mark end of the string... */

		if(strlen(buf) > MAX_INPUT_LENGTH) {
               		buf[MAX_INPUT_LENGTH] = '\0';
        	}

		strcpy(string, buf);

		return (string);
	}

	// Catch this bug early on :P
	if(strlen(string) > MAX_INPUT_LENGTH) {
		string[MAX_INPUT_LENGTH] = '\0';
	}
	return (string);
}


void
do_deaf(CHAR_DATA * ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_DEAF))
	{
		Cprintf(ch, "You can now hear tells again.\n\r");
		REMOVE_BIT(ch->comm, COMM_DEAF);
	}
	else
	{
		Cprintf(ch, "From now on, you won't hear tells.\n\r");
		SET_BIT(ch->comm, COMM_DEAF);
	}
}


/* RT quiet blocks out all communication */
void
do_quiet(CHAR_DATA * ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_QUIET))
	{
		Cprintf(ch, "Quiet mode removed.\n\r");
		REMOVE_BIT(ch->comm, COMM_QUIET);
	}
	else
	{
		Cprintf(ch, "From now on, you will only hear says and emotes.\n\r");
		SET_BIT(ch->comm, COMM_QUIET);
	}
}


/* afk command */
void
do_afk(CHAR_DATA * ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_AFK))
	{
		if (ch->desc)
		{
			if (ch->desc->connected < CON_NOTE_TO || ch->desc->connected > CON_NOTE_FINISH)
				Cprintf(ch, "AFK mode removed. Type 'replay' to see tells.\n\r");
		}

		REMOVE_BIT(ch->comm, COMM_AFK);
	}
	else
	{
		if (ch->desc)
		{
			if (ch->desc->connected < CON_NOTE_TO || ch->desc->connected > CON_NOTE_FINISH)
				Cprintf(ch, "You are now in AFK mode.\n\r");
		}

		SET_BIT(ch->comm, COMM_AFK);
	}
}


void
do_replay(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
	{
		Cprintf(ch, "You can't replay.\n\r");
		return;
	}

	if (buf_string(ch->pcdata->buffer)[0] == '\0')
	{
		Cprintf(ch, "You have no tells to replay.\n\r");
		return;
	}

	page_to_char(buf_string(ch->pcdata->buffer), ch);
	clear_buf(ch->pcdata->buffer);
}


void
do_beep(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;

	if (IS_SET(ch->comm, COMM_NOTELL))
	{
		Cprintf(ch, "Your beep didn't get through.\n\r");
		return;
	}

	if (IS_SET(ch->comm, COMM_QUIET))
	{
		Cprintf(ch, "You must turn off quiet mode first.\n\r");
		return;
	}

	if (argument[0] == '\0')
	{
		if (IS_SET(ch->comm, COMM_NOBEEP))
		{
			REMOVE_BIT(ch->comm, COMM_NOBEEP);
			Cprintf(ch, "You now accept beeps.\n\r");
		}
		else
		{
			SET_BIT(ch->comm, COMM_NOBEEP);
			Cprintf(ch, "You no longer accept beeps.\n\r");
		}
		return;
	}

	if (IS_SET(ch->comm, COMM_NOBEEP))
	{
		Cprintf(ch, "You have to turn on the beep channel first.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, argument, TRUE)) == NULL)
	{
		Cprintf(ch, "Nobody like that.\n\r");
		return;
	}

	if (IS_NPC(victim))
	{
		Cprintf(ch, "You can only beep players.\n\r");
		return;
	}

	if (IS_SET(victim->comm, COMM_NOBEEP))
	{
		Cprintf(ch, "%s is not receiving beeps.\n\r", victim->name);
		return;
	}

	if (is_hushed(victim, ch))
	{
		Cprintf(ch, "They don't want to hear from you, beeps or otherwise.\n\r");
		return;
	}

	Cprintf(ch, "\a{rYou beep to %s.{x\n\r", victim->name);
	Cprintf(victim, "\a{r%s beeps you.{x\n\r", PERS(ch, victim));

	return;
}


/* clan channels */
void
do_cgoss(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_NOCGOSS)) {
            Cprintf(ch, "Clan gossip channel is now ON\n\r");
            REMOVE_BIT(ch->comm, COMM_NOCGOSS);
        } else {
            Cprintf(ch, "Clan gossip channel is now OFF\n\r");
            SET_BIT(ch->comm, COMM_NOCGOSS);
        }

        return;
    }

    if (!is_clan(ch) && !IS_IMMORTAL(ch)) {
        Cprintf(ch, "You aren't in a clan.\n\r");
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOCGOSS);

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{gYou clan gossip '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !IS_SET(victim->comm, COMM_NOCGOSS) && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch)) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{g%s clan gossips '%s'{x\n\r", PERS(ch, victim), argument);
                    } else {
                        Cprintf(victim, "%s clan gossips something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{g%s clan gossips [{rscrambled{g] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{g%s clan gossips '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


void
do_sliver_chan(CHAR_DATA * ch, char *argument) {
    CHAR_DATA *victim;
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (ch->race != race_lookup("sliver")) {
        Cprintf(ch, "You are not one with the hive.\n\r");
        return;
    }

    if (argument[0] == '\0') {
        Cprintf(ch, "Without a message, the hive cannot voice your thoughts.\n\r");
        return;
    }

    if (!strcmp(argument, "who")) {
        for (victim = char_list; victim != NULL; victim = victim->next) {
            if (!IS_NPC(victim)
                    && race_lookup("sliver") == victim->race
                    && victim->sliver > 0
                    && victim->level < 54
                    && can_see(ch, victim)) {
                Cprintf(ch, "%s is a %s sliver.\n\r", victim->name, sliver_table[victim->sliver].name);
            }
        }

        return;
    }

    if (!strcmp(argument, "link")) {
        do_mind_link(ch, "");
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        return;
    }

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{gYou mind link {c'%s'{g with the sliver hive.{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !is_hushed(victim, ch) && d->character->race == race_lookup("sliver")) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{g%s sends {c'%s'{g to the hive.{x\n\r", PERS(ch, victim), argument);
                    } else {
                        Cprintf(victim, "%s's mind link sounds like line noise.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{g%s sends [{rscrambled{g] {c'%s'{g to the hive.{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{g%s sends {c'%s'{g to the hive.{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


void
do_ooc(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_NOOOC)) {
            Cprintf(ch, "OOC channel is now ON.\n\r");
            REMOVE_BIT(ch->comm, COMM_NOOOC);
        } else {
            Cprintf(ch, "OOC channel is now OFF.\n\r");
            SET_BIT(ch->comm, COMM_NOOOC);
        }

        return;
    }

    if (IS_SET(ch->comm, COMM_QUIET)) {
        Cprintf(ch, "You must turn off quiet mode first.\n\r");
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOOOC);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{mYou ooc '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !IS_SET(victim->comm, COMM_NOOOC) && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch)) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{m%s ooc's '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "%s ooc's something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{m%s ooc's [{rscrambled{m] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{m%s ooc's '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


void
do_leadchan(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (ch->level < 52 && get_trust(ch) < 52) {
        Cprintf(ch, "This channel is reserved for leaders and recruiters. Sorry.\n\r");
        return;
    }

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_LEAD)) {
            Cprintf(ch, "Leader channel is now ON.\n\r");
            REMOVE_BIT(ch->comm, COMM_LEAD);
        } else {
            Cprintf(ch, "Leader channel is now OFF.\n\r");
            SET_BIT(ch->comm, COMM_LEAD);
        }

        return;
    }

    if (IS_SET(ch->comm, COMM_QUIET)) {
        Cprintf(ch, "You must turn off quiet mode first.\n\r");
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        return;
    }

    REMOVE_BIT(ch->comm, COMM_LEAD);
    argument = makedrunk(argument, ch);

    strcpy(original, argument);
    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{MYou lead {Y'%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !IS_SET(victim->comm, COMM_LEAD) && (get_trust(victim) > 51 || victim->level > 51) && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch)) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{MLeader: {Y%s says '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{MLeader: {Y%s says something incomprehensible to you.{x\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{MLeader: {Y%s says [{rscramble{Y] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{MLeader: {Y%s says '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


/* RT chat replaced with ROM gossip */
void
do_gossip(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_NOGOSSIP)) {
            Cprintf(ch, "Gossip channel is now ON.\n\r");
            REMOVE_BIT(ch->comm, COMM_NOGOSSIP);
        } else {
            Cprintf(ch, "Gossip channel is now OFF.\n\r");
            SET_BIT(ch->comm, COMM_NOGOSSIP);
        }

        return;
    }

    if (IS_SET(ch->comm, COMM_QUIET)) {
        Cprintf(ch, "You must turn off quiet mode first.\n\r");
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOGOSSIP);

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{cYou gossip '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !IS_SET(victim->comm, COMM_NOGOSSIP) && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch)) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{c%s gossips '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "%s gossips something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{c%s gossips [{rscrambled{c] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{c%s gossips '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


void
do_grats(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_NOGRATS)) {
            Cprintf(ch, "Grats channel is now ON.\n\r");
            REMOVE_BIT(ch->comm, COMM_NOGRATS);
        } else {
            Cprintf(ch, "Grats channel is now OFF.\n\r");
            SET_BIT(ch->comm, COMM_NOGRATS);
        }

        return;
    }

    if (IS_SET(ch->comm, COMM_QUIET)) {
        Cprintf(ch, "You must turn off quiet mode first.\n\r");
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOGRATS);

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{gYou grats '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !IS_SET(victim->comm, COMM_NOGRATS) && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch)) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{g%s grats '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "%s grats something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{g%s grats [{rscrambled{g] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{g%s grats '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


void
do_question(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_NOQUESTION)) {
            Cprintf(ch, "Q/A channel is now ON.\n\r");
            REMOVE_BIT(ch->comm, COMM_NOQUESTION);
        } else {
            Cprintf(ch, "Q/A channel is now OFF.\n\r");
            SET_BIT(ch->comm, COMM_NOQUESTION);
        }

        return;
    }

    if (IS_SET(ch->comm, COMM_QUIET)) {
        Cprintf(ch, "You must turn off quiet mode first.\n\r");
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        return;
    }

    argument = makedrunk(argument, ch);

    REMOVE_BIT(ch->comm, COMM_NOQUESTION);
    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{gYou question '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !IS_SET(victim->comm, COMM_NOQUESTION) && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch)) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{g%s questions '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "%s questions something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{g%s questions [{rscrambled{g] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{g%s questions '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        } 
    }
}


/* RT answer channel - uses same line as questions */
void
do_answer(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_NOQUESTION)) {
            Cprintf(ch, "Q/A channel is now ON.\n\r");
            REMOVE_BIT(ch->comm, COMM_NOQUESTION);
        } else {
            Cprintf(ch, "Q/A channel is now OFF.\n\r");
            SET_BIT(ch->comm, COMM_NOQUESTION);
        }

        return;
    }

    if (IS_SET(ch->comm, COMM_QUIET)) {
        Cprintf(ch, "You must turn off quiet mode first.\n\r");
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOQUESTION);

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{gYou answer '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !IS_SET(victim->comm, COMM_NOQUESTION) && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch)) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{g%s answers '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "%s answers something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{g%s answers [{rscrambled{g] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{g%s answers '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


void
do_music(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_NOMUSIC)) {
            Cprintf(ch, "Music channel is now ON.\n\r");
            REMOVE_BIT(ch->comm, COMM_NOMUSIC);
        } else {
            Cprintf(ch, "Music channel is now OFF.\n\r");
            SET_BIT(ch->comm, COMM_NOMUSIC);
        }

        return;
    }

    if (IS_SET(ch->comm, COMM_QUIET)) {
        Cprintf(ch, "You must turn off quiet mode first.\n\r");
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOMUSIC);

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{cYou MUSIC: '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !IS_SET(victim->comm, COMM_NOMUSIC) && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch)) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{c%s MUSIC: '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "%s musics something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{c%s MUSIC: [{rscrambled{c] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{c%s MUSIC: '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


/* clan channels */
void
do_clantalk(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (clan_table[ch->clan].independent) {
        Cprintf(ch, "You aren't in a clan.\n\r");
        return;
    }

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_NOCLAN)) {
            Cprintf(ch, "Clan channel is now ON\n\r");
            REMOVE_BIT(ch->comm, COMM_NOCLAN);
        } else {
            Cprintf(ch, "Clan channel is now OFF\n\r");
            SET_BIT(ch->comm, COMM_NOCLAN);
        }

        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOCLAN);

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{yYou clan '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !IS_SET(victim->comm, COMM_NOCLAN) && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch)) {
                if (is_same_clan(ch, victim)) {
                    if (is_affected(victim, gsn_scramble)) {
                        if (IS_IMMORTAL(ch)) {
                            Cprintf(victim, "{y%s clans '%s'{x\n\r", PERS(ch, victim), argument);
                        } else {
                            Cprintf(victim, "%s clans something incomprehensible to you.\n\r", PERS(ch, victim));
                        }
                    } else {
                        if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                            Cprintf(victim, "{y%s clans [{rscrambled{y] '%s'{x\n\r", PERS(ch, victim), original);
                        } else {
                            Cprintf(victim, "{y%s clans '%s'{x\n\r", PERS(ch, victim), argument);
                        }
                    }
                } else {
                    AFFECT_DATA* paf = affect_find(d->character->affected, gsn_spy);

                    if (paf != NULL && paf->modifier == ch->clan) {
                        if (is_affected(victim, gsn_scramble)) {
                            Cprintf(victim, "{MYou spy %s clanning something incomprehensible to you.{x\n\r", PERS(ch, victim));
                        } else {
                            Cprintf(victim, "{MYou spy %s clanning '%s'{x\n\r", PERS(ch, victim), argument);
                        }
                    }
                }
            }
        }
    }
}

void
do_teamtalk(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    AFFECT_DATA *paf = affect_find(ch->affected, gsn_aura);

    if (paf == NULL) {
        Cprintf(ch, "You aren't on a team.\n\r");
        return;
    }
    
    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{GYou team {y'%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            AFFECT_DATA *pvictaf = affect_find(victim->affected, gsn_aura);
            if (d->connected == CON_PLAYING && !IS_NPC(victim) && pvictaf != NULL && paf->modifier == pvictaf->modifier) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{G%s teams {y'%s'{x\n\r", PERS(ch, victim), argument);
                    } else {
                        Cprintf(victim, "%s teams something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{G%s teams [{rscrambled{y] {y'%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{G%s teams {y'%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


void
do_immtalk(CHAR_DATA * ch, char *argument) {
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_NOWIZ)) {
            Cprintf(ch, "Immortal channel is now ON\n\r");
            REMOVE_BIT(ch->comm, COMM_NOWIZ);
        } else {
            Cprintf(ch, "Immortal channel is now OFF\n\r");
            SET_BIT(ch->comm, COMM_NOWIZ);
        }

        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOWIZ);

    Cprintf(ch, "{G%s: {y%s{x\n\r", ((IS_NPC(ch)) ? ch->short_descr : ch->name), argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && IS_IMMORTAL(victim) && victim->level > 54 && victim != ch && !IS_SET(victim->comm, COMM_NOWIZ)) {
                Cprintf(victim, "{G%s: {y%s{x\n\r", PERS(ch, victim), argument);
            }
        }
    }
}


void
do_say(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *mob;

    if (argument[0] == '\0') {
        Cprintf(ch, "Say what?\n\r");
        return;
    }

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{yYou say '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;
            
            if (d->connected == CON_PLAYING && victim != ch && victim->in_room == ch->in_room && !is_hushed(victim, ch) && victim->position >= POS_RESTING) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{y%s says '%s'{x\n\r", PERS(ch, victim), argument);
                    } else {
                        Cprintf(victim, "%s says something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{y%s says [{rscrambled{y] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{y%s says '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }

    for (mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room) {
        if (IS_NPC(mob) && mob != ch && HAS_TRIGGER(mob, TRIG_SPEECH) && mob->position == mob->pIndexData->default_pos) {
            //mp_act_trigger(argument, mob, ch, NULL, NULL, TRIG_SPEECH);
        }
    }
}


void
do_speak(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (ch->remort < 1 || ch->rem_sub != 1 || race_lookup("troll") != ch->race) {
        Cprintf(ch, "You don't have a second mouth to stick your foot into.  Sorry.\n\r");
        return;
    }

    if (argument[0] == '\0') {
        Cprintf(ch, "Say what through your second head?\n\r");
        return;
    }

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{yYour second head says '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;
            
            if (d->connected == CON_PLAYING && victim != ch && victim->in_room == ch->in_room && !is_hushed(victim, ch) && victim->position >= POS_RESTING) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{y%s's second head says '%s'{x\n\r", PERS(ch, victim), argument);
                    } else {
                        Cprintf(victim, "%s's second head says something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{y%s's second head says [{rscrambled{y] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{y%s's second head says '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }

    if (IS_NPC(ch) || !IS_NPC(ch)) {
        CHAR_DATA *mob;
        for (mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room) {
            if (IS_NPC(mob) && mob != ch && HAS_TRIGGER(mob, TRIG_SPEECH) && mob->position == mob->pIndexData->default_pos) {
                //mp_act_trigger(argument, mob, ch, NULL, NULL, TRIG_SPEECH);
            }
        }
    }
}


void
do_shout(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_SHOUTSOFF)) {
            Cprintf(ch, "You can hear shouts again.\n\r");
            REMOVE_BIT(ch->comm, COMM_SHOUTSOFF);
        } else {
            Cprintf(ch, "You will no longer hear shouts.\n\r");
            SET_BIT(ch->comm, COMM_SHOUTSOFF);
        }

        return;
    }

    if (IS_SET(ch->comm, COMM_NOSHOUT) || IS_SET(ch->comm, COMM_NOCHANNELS)) {
        Cprintf(ch, "You can't shout.\n\r");
        return;
    }

    REMOVE_BIT(ch->comm, COMM_SHOUTSOFF);

    WAIT_STATE(ch, 12);

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{rYou shout '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->character != NULL) {
            CHAR_DATA *victim = d->character;

            if (d->connected == CON_PLAYING && victim != ch && !IS_SET(victim->comm, COMM_SHOUTSOFF) && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch) && victim->position >= POS_RESTING) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{r%s shouts '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "%s shouts something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{r%s shouts [{Rscrambled{r] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{r%s shouts '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


void
do_tell(CHAR_DATA * ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        Cprintf(ch, "Tell whom what?\n\r");
        return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    victim = get_char_world(ch, arg, TRUE);
    if (victim != NULL && (IS_NPC(victim) && victim->in_room != ch->in_room)) {
        victim = NULL;
    }

    if ( tell_private(ch, victim, argument) ) {
        if ((IS_NPC(ch) || !IS_NPC(ch)) && IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_SPEECH) && victim != ch) {
            //mp_act_trigger(argument, victim, ch, NULL, NULL, TRIG_SPEECH);
        }

        victim->reply = ch;
        ch->retell = victim;
    }
}


void
do_reply(CHAR_DATA * ch, char *argument) {
    CHAR_DATA *victim = ch->reply;

    if ( tell_private(ch, victim, argument) ) {
        victim->reply = ch;
    }
}


void
do_retell(CHAR_DATA *ch, char *argument) {
    tell_private(ch, ch->retell, argument);
}

bool
tell_private(CHAR_DATA *ch, CHAR_DATA *victim, char *argument) {
    char original[MAX_STRING_LENGTH];

    if (IS_SET(ch->comm, COMM_NOTELL)) {
        Cprintf(ch, "Your message didn't get through.\n\r");
        return FALSE;
    }

    if (IS_SET(ch->comm, COMM_QUIET)) {
        Cprintf(ch, "You must turn off quiet mode first.\n\r");
        return FALSE;
    }

    if (IS_SET(ch->comm, COMM_DEAF)) {
        Cprintf(ch, "You must turn off deaf mode first.\n\r");
        return FALSE;
    }

    if (victim == NULL) {
        Cprintf(ch, "They aren't here.\n\r");
        return FALSE;
    }

    if (argument[0] == '\0') {
        Cprintf(ch, "What do you want to tell %s?\n\r", victim->name);
        return FALSE;
    }

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    if (victim->desc == NULL && !IS_NPC(victim)) {
        char buf[MAX_STRING_LENGTH];

        act("$N seems to have misplaced $S link...try again later.", ch, NULL, victim, TO_CHAR, POS_DEAD);

        if (IS_IMMORTAL(ch)) {
            sprintf(buf, "%s tells you '%s'\n\r", PERS(ch, victim), original);
        } else if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
            sprintf(buf, "%s tells you [scrambled] '%s'\n\r", PERS(ch, victim), original);
        } else {
            sprintf(buf, "%s tells you '%s'\n\r", PERS(ch, victim), argument);
        }

        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer, buf);

        return FALSE;
    }

    if (!IS_IMMORTAL(ch) && !IS_AWAKE(victim)) {
        act("$E can't hear you.", ch, 0, victim, TO_CHAR, POS_DEAD);
        return FALSE;
    }

    /*
     * Imms can always tell/reply/retell morts, regardless of quiet/deaf.
     * If an Imm goes quiet or deaf, he won't receive tell/reply/retell
     */
    if ((IS_SET(victim->comm, COMM_QUIET) || IS_SET(victim->comm, COMM_DEAF)) && (!IS_IMMORTAL(ch) || IS_IMMORTAL(victim))) {
        act("$E is not receiving tells.", ch, 0, victim, TO_CHAR, POS_DEAD);
        return FALSE;
    }

    if (!IS_IMMORTAL(victim) && !IS_AWAKE(ch)) {
        Cprintf(ch, "In your dreams, or what?\n\r");
        return FALSE;
    }

    if (IS_SET(victim->comm, COMM_AFK)) {
        char buf[MAX_STRING_LENGTH];

        if (IS_NPC(victim)) {
            act("$E is AFK, and not receiving tells.", ch, NULL, victim, TO_CHAR, POS_DEAD);
            return FALSE;
        }

        act("$E is AFK, but your tell will go through when $E returns.", ch, NULL, victim, TO_CHAR, POS_DEAD);

        if (IS_IMMORTAL(ch)) {
            sprintf(buf, "%s tells you '%s'\n\r", PERS(ch, victim), original);
        } else if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
            sprintf(buf, "%s tells you [scrambled] '%s'\n\r", PERS(ch, victim), original);
        } else {
            sprintf(buf, "%s tells you '%s'\n\r", PERS(ch, victim), argument);
        }

        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer, buf);
        return FALSE;
    }

    if (is_hushed(victim, ch)) {
        Cprintf(ch, "This person doesn't want to hear from you.\n\r");
        return FALSE;
    }

    Cprintf(ch, "{mYou tell %s {y'%s'{x\n\r", PERS(victim, ch), argument);

    if ( is_affected(victim, gsn_scramble) ) {
        if ( IS_IMMORTAL(ch) ) {
            Cprintf(victim, "{m%s tells you {y'%s'{x\n\r", PERS(ch, victim), original);
        } else {
            Cprintf(victim, "%s tells you something incomprehensible.\n\r", PERS(ch, victim));
        }
    } else {
        if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
            Cprintf(victim, "{m%s tells you [{rscrambled{m] {y'%s'{x\n\r", PERS(ch, victim), original);
        } else {
            Cprintf(victim, "{m%s tells you {y'%s'{x\n\r", PERS(ch, victim), argument);
        }
    }

    if (!IS_SET(victim->comm, COMM_NOBEEP)) {
        Cprintf(victim, "\a");
    }

    return TRUE;
}

void
do_yell(CHAR_DATA * ch, char *argument) {
    char original[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (IS_SET(ch->comm, COMM_NOSHOUT)) {
        Cprintf(ch, "You can't yell.\n\r");
        return;
    }

    if (argument[0] == '\0') {
        Cprintf(ch, "Yell what?\n\r");
        return;
    }

    argument = makedrunk(argument, ch);

    strcpy(original, argument);

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    Cprintf(ch, "{RYou yell '%s'{x\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        CHAR_DATA *victim = d->character;
        if (victim != NULL) {
            if (d->connected == CON_PLAYING && victim != ch && victim->in_room != NULL && victim->in_room->area == ch->in_room->area && !IS_SET(victim->comm, COMM_QUIET) && !is_hushed(victim, ch) && victim->position >= POS_RESTING) {
                if (is_affected(victim, gsn_scramble)) {
                    if (IS_IMMORTAL(ch)) {
                        Cprintf(victim, "{R%s yells '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "%s yells something incomprehensible to you.\n\r", PERS(ch, victim));
                    }
                } else {
                    if (IS_IMMORTAL(victim) && is_affected(ch, gsn_scramble)) {
                        Cprintf(victim, "{R%s yells [{rscrambled{R] '%s'{x\n\r", PERS(ch, victim), original);
                    } else {
                        Cprintf(victim, "{R%s yells '%s'{x\n\r", PERS(ch, victim), argument);
                    }
                }
            }
        }
    }
}


void
do_emote(CHAR_DATA * ch, char *argument) {
    DESCRIPTOR_DATA *d;

    if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE)) {
        Cprintf(ch, "You can't show your emotions.\n\r");
        return;
    }

    if (argument[0] == '\0') {
        Cprintf(ch, "Emote what?\n\r");
        return;
    }

    if (is_affected(ch, gsn_scramble)) {
        reverse(argument);
    }

    MOBtrigger = FALSE;

    Cprintf(ch, "%s %s\n\r", PERS(ch, ch), argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        CHAR_DATA *victim = d->character;
        if (victim != NULL) {
            if (d->connected == CON_PLAYING && victim != ch && victim->in_room == ch->in_room && victim->position >= POS_RESTING) {
                if (is_affected(victim, gsn_scramble)) {
                    Cprintf(victim, "%s does something incomprehensible to you.\n\r", PERS(ch, victim));
                } else {
                    Cprintf(victim, "%s %s\n\r", PERS(ch, victim), argument);
                }
            }
        }
    }

    MOBtrigger = TRUE;
    return;
}


void
do_pmote(CHAR_DATA * ch, char *argument)
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

	MOBtrigger = FALSE;
	act("$n $t", ch, argument, NULL, TO_CHAR, POS_RESTING);
	MOBtrigger = TRUE;

	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (vch->desc == NULL || vch == ch)
			continue;

		if ((letter = strstr(argument, vch->name)) == NULL)
		{
			act("$N $t", vch, argument, ch, TO_CHAR, POS_RESTING);
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

		MOBtrigger = FALSE;
		act("$N $t", vch, temp, ch, TO_CHAR, POS_RESTING);
		MOBtrigger = TRUE;
	}

	return;
}


/* pose tables, new.. THANK you Litazia! */
struct pose_table_type
{
	char *message[2 * MAX_CLASS];
};

const struct pose_table_type pose_table[] =
{
	{
		{
			"A blue bolt of energy spirals down your body.",
			"A blue bolt of energy spirals down $n's body.",
			"You create a pen and paper and write something down.",
			"$n creates a pen and paper and writes something down.",
			"A ball bounces around you of its own accord.",
			"A ball bounces around $n of its own accord.",
			"A little flame ignites your thumb.",
			"A little flame ignites $n's thumb.",
			"You glow with an inner holiness.",
			"$n glows with an inner holiness.",
			"You release a fox from a trap.",
			"$n releases a fox from a trap.",
			"You rip off your shirt to show your bulging muscles!",
			"$n rips off $s shirt to show $s bulging muscles!",
			"You swear to uphold the order of Bosco.",
			"$n swears to uphold the order of Bosco.",
			"A sparrow lands on your shoulder.",
			"A sparrow lands on $n's shoulder.",
			"You perform a small card trick for the masses.",
			"$n performs a small card trick in front of you.",
			"You sharpen your quill and glance about nervously.",
			"$n sharpens a quill and glances about nervously.",
		}}
	,
	{
		{
			"You turn into a cute litte mouse, then return to your normal shape.",
			"$n turns into a cute little mouse, then returns to $s normal shape.",
			"A small whirlwind rises up before you.",
			"A small whirlwind rises up before $n.",
			"You touch a pillar, and it moves sideways.",
			"$n touches a pillar, and it moves sideways.",
			"Light streams forth from your palm.",
			"Light streams forth from $n's palm.",
			"You nonchalantly turn water into wine.",
			"$n nonchalantly turns water into wine.",
			"The leaves on the trees turn to face you.",
			"The leaves on the trees turn to face $n.",
			"You tear a chunk of wood in half.",
			"$n tears a chunk of wood in half.",
			"You meditate on finding the Holy Grail.",
			"$n meditates on finding the Holy Grail.",
			"You beat a bloodhound in a tracking race.",
			"$n beats a bloodhound in a tracking race.",
			"You write two letters, one with each hand.",
			"$n writes two letters, one with each hand.",
			"You pull out a piece of parchment and read it out loud.",
			"$n pulls out a piece of parchment and reads it out loud.",
		}}
	,
	{
		{
			"Blue sparks fly from your fingers.",
			"Blue sparks fly from $n's fingers.",
			"A cloud of smoke surrounds you.",
			"A cloud of smoke surrounds $n.",
			"The floor glows when you jump.",
			"The floor glows when $n jumps.",
			"Icicles form on your arm.",
			"Icicles form on $n's arm.",
			"A halo appears over your head.",
			"A halo appears over $n's head.",
			"You calm a wild stallion.",
			"$n calms a wild stallion.",
			"You have a sparring match with yourself.",
			"$n has a sparring match with $mself.",
			"You kneel at an altar, and light shines upon you.",
			"$n kneels at an altar, and light shines upon $m.",
			"You determine that a rare Booga-Booga plant is before you.",
			"$n determines that a rare Booga-Booga plant is before $m.",
			"You nimbly tie yourself into a granny knot, and pull yourself apart.",
			"$n nimbly ties $mself into a granny knot, and pulls $mself apart.",
			"You sit down and begin to doodle in a book.",
			"$n sits down and begins to doodle in a book.",
		}}
	,
	{
		{
			"Your eyes shine red light on all you look upon.",
			"$n's eyes shine red light on all $M looks upon.",
			"You make a waterfall pour down from your hands.",
			"$n makes a waterfall pour down from $s hands.",
			"You pet a statue, and it speaks! WOOF!",
			"$n pets a statue, and it speaks! WOOF!",
			"A stream of sparks rains down on you.",
			"A stream of sparks rains down on $n.",
			"You recite meaningful words of wisdom.",
			"$n recites meaningful words of wisdom.",
			"You walk through the woods, but everyone else gets stuck on a bush.",
			"$n walks through the woods, but you get stuck on a bush.",
			"You snap an iron bar in two with your bare hands.",
			"$n snaps an iron bar in two with $s bare hands.",
			"A child hugs your leg and says 'My hero!'",
			"A child hugs $n's leg and says 'My hero!'",
			"The vines lift to let you pass.",
			"The vines lift to let $n pass.",
			"You juggle with daggers, poison darts, and caltrops.",
			"$n juggles with daggers, poison darts, and caltrops.",
			"You draw a portrait of yourself on the ground.",
			"$n draws a portrait of $mself on the ground.",
		}}
	,
	{
		{
			"An unidentified slimy green monster appears before you and grins.",
			"An unidentified slimy green monster appears before $n and grins.",
			"You summon a fido in front of you, which licks your face.",
			"$n summons a fido in front of $m, which licks $s face.",
			"You make everyone's armor jump up and down.",
			"Your armor jumps up and down. Thank $n for that.",
			"A mouth appears and talks to you.",
			"A mouth appears and talks to $n.",
			"Deep in prayer, you levitate.",
			"Deep in prayer, $n levitates.",
			"You intone, 'Only YOU can prevent forest fires!'",
			"$n intones, 'Only YOU can prevent forest fires!'",
			"You eat some rations, food, packaging, and all.",
			"$n eats some rations, food, packaging, and all.",
			"A magnificent war horse comes when you call it.",
			"A magnificent war horse comes when $n calls it.",
			"You chat with a dryad.",
			"$n chats with a dryad.",
			"You steal the underwear off every person in the room.",
			"You notice that $n got hold of your underwear. Um...",
			"You scribe a glowing rune onto a nearby wall. Grafiti!",
			"$n scribes a glowing rune onto a nearby wall. Grafiti!",
		}}
	,
	{
		{
			"You turn everybody into a little pink fido.",
			"You are turned into a little pink fido by $n.",
			"A bridge now spans the chasm you wish to cross.",
			"A bridge now spans the chasm $n wishes to cross.",
			"Your skin shimmers with the breeze.",
			"$n's skin shimmers with the breeze.",
			"Spectral bells ring when you walk.",
			"Spectral bells ring when $n walks.",
			"A messenger from Bosco consults you.",
			"A messenger from Bosco consults $n.",
			"You cleave through an orc with one blow! Oh, the blood!",
			"$n cleaves through an orc with one blow! Oh, the blood!",
			"You outrun a greyhound in a race.",
			"$n outruns a greyhound in a race.",
			"You look around for evil in the room.",
			"$n looks around for evil in the room.",
			"You concentrate, and turn into a fido.",
			"$n concentrates, and turns into a fido.",
			"The (loaded?) dice roll ... and you win again.",
			"The (loaded?) dice roll ... and $n wins again.",
			"You draw a rune in the air with your weapon.",
			"$n draws a rune in the air with their weapon.",
		}}
	,
	{
		{
			"Various parts of your body glow with an eerie light.",
			"Various parts of $n's body glow with an eerie light.",
			"You make a brick wall rise up from the ground.",
			"$n makes a brick wall rise up from the ground.",
			"A gargoyle animates and dances before you!",
			"A gargoyle animates and dances before $n!",
			"A curtain of sparkling energy surrounds you.",
			"A curtain of sparkling energy surrounds $n.",
			"You speak with the voice of a great being.",
			"$n speaks with the voice of a great being.",
			"A big black bear follows you around.",
			"A big black bear follows $n around.",
			"You push over a convenient brick wall.",
			"$n pushes over a convenient brick wall.",
			"Tadah! You cure everyone's warts!",
			"Tadah! $n cures your warts!",
			"You deftly cut mistletoe with a golden sickle.",
			"$n deftly cuts mistletoe with a golden sickle.",
			"You count the money in everyone's pockets.",
			"Check your money, $n is counting it.",
			"Your weapon emits a low humming sound.",
			"$n's weapon emits a low humming sound.",
		}}
	,
	{
		{
			"You toss a lightning bolt from one hand to the other.",
			"$n tosses a lightning bolt from one hand to the other.",
			"A griffin lands and prepares to be your mount.",
			"A griffin lands and prepares to be $n's mount.",
			"A large hungry tiger becomes your pussy cat.",
			"A large hungry tiger becomes $n's pussy cat.",
			"You make a ring of fire surround everyone in the room.",
			"$n makes a ring of fire surround you.",
			"A choir of angels sings of your deeds.",
			"A choir of angels sings of $n's deeds.",
			"You chase away marauding goblins from your forest.",
			"$n chases away marauding goblins from $s forest.",
			"You lift a granite boulder above your head.",
			"$n lifts a granite boulder above $s head.",
			"Evil beings drift away at the sight of you.",
			"Evil beings drift away at the sight of $n.",
			"You persuade greedy loggers from cutting down trees.",
			"$n persuades greedy loggers from cutting down trees.",
			"You balance a pocket knife on your tongue.",
			"$n balances a pocket knife on your tongue.",
			"You roll up your sleeves and show your tattoos to everyone.",
			"$n rolls up their sleeves and shows you their tattoos.",

		}}
	,
	{
		{
			"The light flickers as you chant in magical languages.",
			"The light flickers as $n chants in magical languages.",
			"A secret door to your house opens, and you enter.",
			"A secret door to $n's house opens, and $M enters.",
			"You frown, and everyone becomes depressed.",
			"$n frowns, and you become depressed.",
			"Hands appear and shake your and everyone else's hands.",
			"Hands appear and shake $n's and your hands.",
			"Everyone floats skyward as you pray.",
			"You float skyward as $n prays.",
			"You determine somebody killed your favorite falcon.",
			"$n figures somebody -- maybe you -- killed $s favorite falcon.",
			"Oomph!  You crush a granite boulder into fine dust.",
			"Oomph!  $n crushes a granite boulder into fine dust.",
			"You guard the Temple of Bosco from evil-doers... like your friends(?).",
			"$n guards the Temple of Bosco from evil-doers... like yourself(?).",
			"Vines entangle everyone's feet, for they have harmed your forest!",
			"Vines entangle your feet, for you have harmed $n's forest!",
			"You produce a coin from everyone's ear. Tadah, magic!",
			"$n produces a coin from your ear. How DOES $M do that?",
			"Your tattoos begin to glow fiercely.",
			"$n's tattoos glow brightly."
		}}
	,
	{
		{
			"You take off your head and put it into a box.",
			"$n takes off $s head and puts it into a box.",
			"A lush banquet table full of food appears before you.",
			"A lush banquet table full of food appears before $n.",
			"You tap a rusty knife, and it becomes a magnificent dagger.",
			"$n taps a rusty knife, and it becomes a magnificent dagger.",
			"You drop a magic cage on everyone.",
			"$n drops a magic cage on you.",
			"You feed 200 starving people with a loaf of bread.",
			"$n feeds 200 starving people with a loaf of bread.",
			"A pack of wolves kneels in awe before you.",
			"A pack of wolves kneels in awe before $n.",
			"You hold a giant sword in each hand.",
			"$n holds a giant sword in each hand.",
			"The might of Bosco helps you slay the bad dragon fiend.",
			"The might of Bosco helps $n to slay the bad dragon fiend.",
			"A flock of birds sings sweet songs to you.",
			"A flock of birds sings sweet songs to $n.",
			"You step behind your shadow.",
			"$n steps behind $s shadow.",
			"You brandish a sheaf of scrolls and use it like a shield!",
			"$n brandishes a sheaf of scrolls like shield!",
		}}
	,
	{
		{
			"A small dragon comes to do your bidding.",
			"A small dragon comes to do $n's bidding.",
			"You go swimming in your own personal river.",
			"$n goes swimming in $s own personal river.",
			"You say 'klutz' and everyone drops their stuff.",
			"$n says 'klutz' and you drop your stuff.",
			"A magnificent sword forms in your hand.",
			"A magnificent sword forms in $n's hand.",
			"The sun shines on you while it rains on everyone else.",
			"The sun shines on $n while you get wet.",
			"You stop everyone from torching a tree.",
			"$n stops you from torching a tree.",
			"GROUP HUG! Everyone manages to fit into your arms.",
			"GROUP HUG! You are squeezed into $n's arms.",
			"Holiness permeates you... you are such a good person.",
			"You detect holiness from $n... $M is such a stuck-up.",
			"A shillelagh forms in your hand.",
			"A shillelagh forms in $n's hand.",
			"Your eyes dance with greed when you see everyone's treasure.",
			"$n's eyes dance with greed at the sight of your treasure."
			"Your weapon hovers in the air while you read from a scroll.",
			"$n's weapon hovers in midair while they read from a scroll.",
		}}
	,
	{
		{
			"The surrounding color scheme changes to suit your taste.",
			"The surrounding color scheme changes to suit $n's taste.",
			"A fire elemental rises up and bows before you.",
			"A fire elemental rises up and bows before $n.",
			"The wall becomes a mirror, and you groom yourself.",
			"The wall becomes a mirror, and $n grooms $mself.",
			"Smoke pours from your feet, and you fly upwards!",
			"Smoke pours from $n's feet, and $M flies upwards!",
			"With a wave of your hand, the ocean parts before you.",
			"With a wave of $s hand, the ocean parts before $n.",
			"You stealthily follow everyone.",
			"You can't shake the feeling that $n is following you.",
			"You punch a nearby tree, and uproot it.",
			"$n punches a nearby tree, and uproots it.",
			"You gallantly give away all your gold to the poor.",
			"$n gallantly gives away all $s gold to the poor.",
			"With a wave of your hand, the seedling becomes a mighty oak.",
			"With a wave of $n's hand, the seedling becomes a mighty oak.",
			"You deftly steal everyone's weapon.",
			"Your weapon disappears from your hands and appears on $n!"
			"You balance your weapon on one finger and spin in a circle!",
			"$n balances their weapon on one finger and spins around in a circle!",
		}}
	,
	{
		{
			"Your body explodes in a fit of magical energy.",
			"$n's body explodes in a fit of magical energy.",
			"Stairs reveal themselves, and you descend them.",
			"Stairs reveal themselves, and $n descends them.",
			"Rocks hover when you snap your fingers.",
			"Rocks hover when $n snaps $s fingers.",
			"A stream of fire jets forth from your finger.",
			"A stream of fire jets forth from $n's finger.",
			"A minor god kneels to you.",
			"A minor god kneels to $n.",
			"Giants avoid the forest when you step out.",
			"Giants avoid the forest when $n steps out.",
			"You pound the ground, and it cracks in two!",
			"$n pounds the ground, and it cracks in two!",
			"Bosco blesses your sword while you pray.",
			"Bosco blesses $n's sword while $M prays.",
			"Many, many insects come when you call.",
			"Many, many insects come when $n calls.",
			"You deftly pick a thiefproof lock.",
			"$n deftly picks a thiefproof lock.",
			"The runes on your weapon flare brightly.",
			"$n's weapon is covered in runes which glow brightly.",
		}}
	,
	{
		{
			"You summon the Nameless Demon in a pentagram.",
			"$n summons the Nameless Demon in a pentagram.",
			"A huge dragon lands and nods at you, waiting.",
			"A huge dragon lands and nods at $n, waiting.",
			"The king's guard lets you pass with no ID. Isn't he nice?",
			"The king's guard lets $n pass with no ID. Wait a sec...",
			"You make it rain lightning bolts.",
			"Watch it, $n is making it rain lightning bolts.",
			"A nearby Burning Bush speaks to you.",
			"A nearby Burning Bush speaks to $n.",
			"A werewolf promises his services to you.",
			"A werewolf promises his services to $n.",
			"You decapitate a statue with one swipe of your sword.",
			"$n decapitates a statue with one swipe of $s sword.",
			"You hold the Holy Grail up, finally your quest is at an end!",
			"$n holds the Holy Grail up, finally $s quest is at an end!",
			"The little pussy cat becomes an enormous pussy cat when you pet it.",
			"The little pussy cat becomes an enormous pussy cat when $n pets it.",
			"You slip sneezing powder onto everyone's clothes.",
			"Why is $n playing with sneezing powder? AAAH-CHOO!.",
			"You begin writing runes which completely blanket the room!",
			"$n begins writing runes which completely cover the floor.",
		}}
	,
	{
		{
			"The furniture animate at your call.",
			"The furniture animates at $n's call.",
			"A demon appears before you in a pentagram.",
			"A demon appears before $n in a pentagram.",
			"A golem rips its way out of the ground and bows before you.",
			"A golem rips its way out of the ground and bows before $n.",
			"You will the land to explode into flame.",
			"$n wills the land to explode into flame.",
			"Hieroglyphics on the wall turn and bow to you.",
			"Hieroglyphics on the wall turn and bow to $n.",
			"You cry out, and forest critters attack those who defile the woods!",
			"$n cries out, and forest critters attack those who defile the woods!",
			"You triumphantly lead soldiers into battle.",
			"$n triumphantly leads soldiers into battle.",
			"The plague leaves everyone's body when you raise your hand.",
			"The plague leaves your body when $n raises $s hand.",
			"An enormous wall of thorns forms, protecting you from everyone.",
			"An enormous wall of thorns forms, protecting $n from you.",
			"You steal lots from everybody, and then fall under the weight.",
			"Your load seems much lighter, while $n falls under $s weight.",
			"You paint a tattoo on a cat as it passes by.",
			"$n paints a tattoo on a cat as it walks by. Poor Kitty!",
		}}
	,
	{
		{
			"You shoot a volley of fireballs from each finger. Hot stuff!",
			"$n shoots a volley of fireballs from each finger. Hot stuff!",
			"You will a magnificent tower up from the ground.",
			"$n wills a magnificent tower up from the ground.",
			"A ferocious dragon falls asleep before you.",
			"A ferocious dragon falls asleep before $n.",
			"A spectral dragon forms and coils around you.",
			"A spectral dragon forms and coils around $n.",
			"Followers erect a church in your honor.",
			"Followers erect a church in $n's honor.",
			"You raise a hand, and lull hungry lions to sleep.",
			"$n raises a hand, and lulls hungry lions to sleep.",
			"You rid the surroundings of balrogs with your war cry.",
			"$n rids the surroundings of balrogs with $s war cry.",
			"A holy light shines from your mighty sword.",
			"A holy light shines from $n's mighty sword,",
			"You yell, and lots of icky things bite everyone!",
			"$n yells, and lots of icky things bite you!",
			"You duck into the shadows... and vanish!",
			"$n ducks into the shadows... and vanishes!",
			"You draw a door on the wall and step through it.",
			"$n draws a door on the wall and steps through it.",
		}}
	,
	{
		{
			"Time goes backwards when you snap your fingers.",
			"Time goes backwards when $n snaps $s fingers.",
			"A continent appears in the sky above you.",
			"A continent appears in the sky above $n.",
			"You create a wonderful demon-slaying sword out of a fido tooth.",
			"$n creates a wonderful demon-slaying sword out of a fido tooth.",
			"You throw lightning bolts outwards at all who oppose you.",
			"$n throws lightning bolts outwards at YOU!!!",
			"The great god Bosco adopts you as a grandchild.",
			"The great god Bosco adopts $n as a grandchild.",
			"The animals come out of their holes to pay homage to you.",
			"The animals come out of their holes to pay homage to $n.",
			"You lift a castle with one hand.",
			"$n lifts a castle with one hand.",
			"You force the Great Evil Thing into leaving your presence.",
			"$n forces the Great Evil Thing into leaving $s presence.",
			"You consecrate a Sacred Glade to protect.",
			"$n consecrates a Sacred Glade to protect.",
			"Your gang of thieves surrounds everyone in the room.",
			"You are suddenly surrounded by $n's gang of thieves.",
			"You are rendered invulnerable by a wall of magical symbols.",
			"$n is rendered invulnerable by a wall of magical symbols.",
		}}
};


void
do_pose(CHAR_DATA * ch, char *argument)
{
	int level;
	int pose;
	int i;

	if (IS_NPC(ch))
		return;

	level = UMIN(ch->level, (int) (sizeof(pose_table) / sizeof(pose_table[0]) - 1));
	pose = number_range(0, level);

	i = ch->charClass;

	if (i == class_lookup("druid"))
		i = class_lookup("ranger");
	else if (i == class_lookup("ranger"))
		i = class_lookup("druid");

	i *= 2;

	act(pose_table[pose].message[i], ch, NULL, NULL, TO_CHAR, POS_RESTING);
	act(pose_table[pose].message[i + 1], ch, NULL, NULL, TO_ROOM, POS_RESTING);

	return;
}


void
do_bug(CHAR_DATA * ch, char *argument)
{
	append_file(ch, BUG_FILE, argument);
	Cprintf(ch, "Bug logged.\n\r");
	return;
}


void
do_typo(CHAR_DATA * ch, char *argument)
{
	append_file(ch, TYPO_FILE, argument);
	Cprintf(ch, "Typo logged.\n\r");
	return;
}


void
do_rent(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "There is no rent here.  Just save and quit.\n\r");
	return;
}


void
do_qui(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "If you want to QUIT, you have to spell it out.\n\r");
	return;
}


void
do_quit(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (get_trust(ch) < LEVEL_IMMORTAL && in_enemy_hall(ch))
	{
		Cprintf(ch, "You cannot quit into a rival clan hall.\n\r");
		return;
	}

	if (ch->position == POS_FIGHTING)
	{
		Cprintf(ch, "No way! You are fighting.\n\r");
		return;
	}

	if (ch->position < POS_STUNNED)
	{
		Cprintf(ch, "You're not DEAD yet.\n\r");
		return;
	}

	if (auction->item != NULL && ((ch == auction->buyer) || (ch == auction->seller)))
	{
		Cprintf(ch, "Wait until you have sold/bought the item on auction.\n\r");
		return;
	}

	if (ch->craft_timer < 0) {
		Cprintf(ch, "Please finish crafting before you quit.\n\r");
		return;
	}

	/* stops pc form quitiing in given room */
	if (IS_SET(ch->in_room->room_flags, ROOM_NOQUIT))
	{
		Cprintf(ch, "You cannot quit in this area!\n\r");
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_FERRY)
            && !IS_IMMORTAL(ch))
	{
		Cprintf(ch, "Enjoy the sea breeze a bit longer!\n\r");
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_ARENA)
            && !IS_IMMORTAL(ch))
	{
		Cprintf(ch, "You have to kill or be killed here!\n\r");
		return;
	}

	if (IS_SET(ch->wiznet, NO_QUIT))
	{
		Cprintf(ch, "You cannot quit, stick around and listen!\n\r");
		return;
	}

	if (ch->no_quit_timer > 0 && !IS_IMMORTAL(ch))
	{
		Cprintf(ch, "Calm down a bit first eh!\n\r");
		return;
	}

	quit_character(ch);
}


void
quit_character(CHAR_DATA * ch) {
    DESCRIPTOR_DATA *d, *d_next;
    AFFECT_DATA *af;
    int id;

    /* quitting during quest will reset automatically */
    if (IS_QUESTING(ch)) {
        REMOVE_BIT(ch->act, PLR_QUESTING);
        ch->pcdata->quest.giver = NULL;
        ch->pcdata->quest.type = QUEST_TYPE_NONE;
        ch->pcdata->quest.progress = 0;
        ch->pcdata->quest.target = 0;
        ch->pcdata->quest.destination = NULL;
    }

    Cprintf(ch, "Alas, all good things must come to an end.\n\r");
    act("$n has left the game.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
    for (d = descriptor_list; d != NULL; d = d_next) {
        d_next = d->next;
        if (d->character == ch) {
            break;
        }
    }

    if (d != NULL) {
        sprintf(log_buf, "%s@%s has quit.", ch->name, d->host);
    } else {
        sprintf(log_buf, "%s@No Site has quit.", ch->name);
    }

    log_string("%s", log_buf);

    wiznet(log_buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));
    wiznet("$N rejoins the real world.", ch, NULL, WIZ_LOGINS, 0, get_trust(ch));

    // Wierd affects that can stack unless we do this.
    for (af = ch->affected; af != NULL; af = af->next) {
        if (af->location == APPLY_AGE)
            affect_remove(ch, af);
    }

    // Prevents wierd bug where your master becomes
    // the next person whom you follow.
    affect_strip(ch, gsn_charm_person);
    affect_strip(ch, gsn_tame_animal);

    save_char_obj(ch, TRUE);
    id = ch->id;

    /* free note that might be there */
    if (ch->pcdata->in_progress)
        free_note(ch->pcdata->in_progress);

    d = ch->desc;
    extract_char(ch, TRUE);
    if (d != NULL)
        close_socket(d);

    /* toast evil cheating bastards */
    for (d = descriptor_list; d != NULL; d = d_next) {
        CHAR_DATA *tch;

        d_next = d->next;
        tch = d->original ? d->original : d->character;
        if (tch && tch->id == id) {
            extract_char(tch, TRUE);
            close_socket(d);
        }
    }
}

/*
 * Colour setting and unsetting, way cool, Lope Oct '94
 */
void
do_colour(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_STRING_LENGTH];

	argument = one_argument(argument, arg);

	if (!*arg)
	{
		if (!IS_SET(ch->act, PLR_COLOUR))
		{
			SET_BIT(ch->act, PLR_COLOUR);
			Cprintf(ch, "{bC{ro{yl{co{mu{gr{x is now {rON{x, Way Cool!\n\r");
		}
		else
		{
			send_to_char_bw("Colour is now OFF, <sigh>\n\r", ch);
			REMOVE_BIT(ch->act, PLR_COLOUR);
		}
		return;
	}
	else
	{
		send_to_char_bw("Colour Configuration is unavailable in this\n\r", ch);
		send_to_char_bw("version of colour, sorry\n\r", ch);
	}

	return;
}


void
do_save(CHAR_DATA * ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (ch->in_room == NULL)
		char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));

	if (get_trust(ch) < LEVEL_IMMORTAL && in_enemy_hall(ch))
	{
		Cprintf(ch, "You cannot save into a rival clan hall.\n\r");
		return;
	}

	save_char_obj(ch, FALSE);
	Cprintf(ch, "Saving. Remember that Redemption has automatic saving.\n\r");
	WAIT_STATE(ch, 4);
	return;
}


void
do_follow(CHAR_DATA * ch, char *argument)
{
/* RT changed to allow unlimited following and follow the NOFOLLOW rules */
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Follow whom?\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL)
	{
		act("But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR, POS_RESTING);
		return;
	}

	if (victim == ch)
	{
		if (ch->master == NULL)
		{
			Cprintf(ch, "You already follow yourself.\n\r");
			return;
		}

		stop_follower(ch);
		return;
	}

	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOFOLLOW) && !IS_IMMORTAL(ch))
	{
		act("$N doesn't seem to want any followers.\n\r", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	REMOVE_BIT(ch->act, PLR_NOFOLLOW);

	if (ch->master != NULL)
	{
		stop_follower(ch);
	}

	add_follower(ch, victim);
	return;
}


void
add_follower(CHAR_DATA * ch, CHAR_DATA * master)
{
	if (ch->master != NULL)
	{
		bug("Add_follower: non-null master.", 0);
		return;
	}

	ch->master = master;
	ch->leader = NULL;

	if (can_see(master, ch))
		act("$n now follows you.", ch, NULL, master, TO_VICT, POS_RESTING);

	act("You now follow $N.", ch, NULL, master, TO_CHAR, POS_RESTING);

	return;
}


void
stop_follower(CHAR_DATA * ch)
{
	OBJ_DATA *obj, *obj_next;
	if (ch->master == NULL)
	{
		bug("Stop_follower: null master.", 0);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM))
	{
		REMOVE_BIT(ch->affected_by, AFF_CHARM);
		affect_strip(ch, gsn_charm_person);
		affect_strip(ch, gsn_tame_animal);
	}

	if (can_see(ch->master, ch) && ch->in_room != NULL)
	{
		act("$n stops following you.", ch, NULL, ch->master, TO_VICT, POS_RESTING);
		act("You stop following $N.", ch, NULL, ch->master, TO_CHAR, POS_RESTING);
	}

	if (ch->master->pet == ch)
		ch->master->pet = NULL;

	ch->master = NULL;
        ch->leader = NULL;

	if (is_affected(ch, gsn_paint_power)) {
		act("$n melts back into a pool of ink.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		stop_fighting(ch, TRUE);
		for(obj = ch->carrying; obj != NULL; obj = obj_next) {
                                obj_next = obj->next_content;
                                obj_from_char(obj);
                                if(ch->in_room != NULL)
                                        obj_to_room(obj, ch->in_room);
                }
		char_from_room(ch);
		char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
	}

	return;
}


/* nukes charmed monsters and pets */
void
nuke_pets(CHAR_DATA * ch)
{
	CHAR_DATA *pet;

	/* IS_NULL check added byu Delstar */

	if (ch != NULL)
	{
		if ((pet = ch->pet) != NULL)
		{
			stop_follower(pet);
			ch->pet = NULL;
			if (pet->in_room != NULL)
				act("$N slowly fades away.", ch, NULL, pet, TO_NOTVICT, POS_RESTING);
			extract_char(pet, TRUE);

		}
	}
}


void
die_follower(CHAR_DATA * ch)
{
	CHAR_DATA *fch;
	CHAR_DATA *fch_next;

	ch->leader = NULL;

	for (fch = char_list; fch != NULL; fch = fch_next)
	{
		fch_next = fch->next;

		if (fch->master == ch && IS_SET(fch->act, ACT_PET))
			nuke_pets(ch);

		if (fch->master == ch)
			stop_follower(fch);

		if (fch->leader == ch)
			fch->leader = fch;
	}

	return;
}

// Returns true if demon is still loyal
// Returns false if they attack!
int check_demon_loyalty(CHAR_DATA *ch, CHAR_DATA *victim) {
        AFFECT_DATA *paf = NULL;

        if(ch == NULL
        || ch->in_room == NULL)
                return TRUE;

        paf = affect_find(ch->in_room->affected, gsn_pentagram);

        // Get one charmed demon
        if(victim->race == race_lookup("deamon")
        && victim->master == ch
        && IS_AFFECTED(victim, AFF_CHARM))
        {
                // If room has no pentagram, they'll attack.
                if (paf == NULL)
                {
                        Cprintf(ch, "You lose control of %s!\n\r", victim->short_descr);
                        act("$n loses control of $N!", ch, NULL, victim, TO_ROOM, POS_RESTING);
                        affect_strip(victim, gsn_summon_lord);
                        stop_follower(victim);
                        do_say(victim, "Your will cannot contain me any longer! DIE!");
                        multi_hit(victim, ch, TYPE_UNDEFINED);
                        return FALSE;
                }
                // Pentagram is great against mob kinds of demons.
                if(paf != NULL
                && !is_affected(victim, gsn_summon_lord)
                && number_percent() < 8)
                {
                        Cprintf(ch, "Your pentagram fails to contain %s!\n\r", victim->short_descr);
                        act("$n's pentagram fails to contain $N!", ch, NULL, victim, TO_ROOM, POS_RESTING);
                        affect_strip(victim, gsn_summon_lesser);
                        affect_strip(victim, gsn_summon_greater);
                        stop_follower(victim);
                        do_say(victim, "Your will cannot contain me any longer! DIE!");
                        multi_hit(victim, ch, TYPE_UNDEFINED);
                        return FALSE;
                }
                // Pentagram isn't so great against lords.
                if(paf != NULL
                && is_affected(victim, gsn_summon_lord)
                && number_percent() < 20)
                {
                        Cprintf(ch, "Your pentagram is shattered by %s!\n\r", victim->short_descr);
                        act("$n's pentagram is shattered by $N!", ch, NULL, victim, TO_ROOM, POS_RESTING);
                        affect_strip(victim, gsn_summon_lord);
                        affect_remove_room(ch->in_room, paf);
                        stop_follower(victim);
                        do_yell(victim, "Pathetic mortal, your soul is MINE!");
                        multi_hit(victim, ch, TYPE_UNDEFINED);
                        return FALSE;
                }
        }

        return TRUE;
}

void
do_order(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *och;
	CHAR_DATA *och_next;
	bool found;
	int no_group;
	int chance;

	// Idiot checks
	if(ch == NULL
	|| ch->in_room == NULL)
		return;

	argument = one_argument(argument, arg);
	one_argument(argument, arg2);

	if (ch->pktimer > 0 && ch->fighting == NULL)
	{
		Cprintf(ch, "You're too nervous to boss anyone around right now. Wait a bit.\n\r");
		return;
	}

	if (!can_order(arg2))
	{
		Cprintf(ch, "That will NOT be done.\n\r");
		return;
	}

	if (arg[0] == '\0' || argument[0] == '\0')
	{
		Cprintf(ch, "Order whom to do what?\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM))
	{
		Cprintf(ch, "You feel like taking, not giving, orders.\n\r");
		return;
	}

	if (!str_cmp(arg, "all"))
	{
		/* count group members */
		no_group = 0;
		for (och = ch->in_room->people; och != NULL; och = och_next)
		{
			och_next = och->next_in_room;

			if (IS_AFFECTED(och, AFF_CHARM) && och->master == ch)
			{
				no_group++;
			}
		}

		found = FALSE;
		for (och = ch->in_room->people; och != NULL; och = och_next)
		{
			och_next = och->next_in_room;

			if (IS_AFFECTED(och, AFF_CHARM) && och->master == ch)
			{
				if(check_demon_loyalty(ch, och) == FALSE)
                			return;

				if(och->wait > 0) {
        				Cprintf(ch, "They are too busy to heed your order right now.\n\r");
					found = TRUE;
        				continue;
				}
				found = TRUE;
				Cprintf(och, "%s orders you to '%s'.\n\r", ch->name, argument);

				/* oky, now for the save, which gets worse with more charmies
				   one charmy is always cool tho. */
				chance = 120;
				chance -= (no_group * 2) * 10;
				chance = UMAX(10, chance);

				if(chance < number_percent())
				{
					/* k, they saved msg? */
					act("Your control over $N weakens as you stretch your powers.", ch, NULL, och, TO_CHAR, POS_RESTING);
					act("$n seems unable to muster the concentration to contol $N.", ch, NULL, och, TO_ROOM, POS_RESTING);
					continue;
				}

				interpret(och, argument);
			}

		}
	}
	else
	{
		if ((victim = get_char_room(ch, arg)) == NULL)
		{
			Cprintf(ch, "They aren't here.\n\r");
			return;
		}

		if (victim == ch)
		{
			Cprintf(ch, "Aye aye, right away!\n\r");
			return;
		}

		if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch || (IS_IMMORTAL(victim) && victim->trust >= ch->trust))
		{
			Cprintf(ch, "Do it yourself!\n\r");
			return;
		}

		if(check_demon_loyalty(ch, victim) == FALSE)
                	return;

		if(victim->wait > 0) {
        		Cprintf(ch, "They are too busy to heed your order right now.\n\r");
        		return;
		}

		found = TRUE;
		Cprintf(victim, "%s orders you to '%s'.\n\r", ch->name, argument);
		interpret(victim, argument);
	}


	if (found)
	{
		WAIT_STATE(ch, PULSE_VIOLENCE);
		Cprintf(ch, "Ok.\n\r");
	}
	else
	{
		Cprintf(ch, "You have no followers here.\n\r");
	}

	return;
}

void apply_sliver_bonus(CHAR_DATA *ch, CHAR_DATA *victim) {
	AFFECT_DATA af;

	if(ch->sliver < 0)
		return;

	af.where = TO_AFFECTS;
        af.type = gsn_telepathy;
        af.level = ch->level;
        af.duration = 24;
        af.modifier = sliver_table[ch->sliver].amount;
        af.location = sliver_table[ch->sliver].location;
        af.bitvector = 0;

	affect_to_char(victim, &af);

	if(ch == victim) {
		Cprintf(ch, "You unleash the power of your own mind!\n\r");
		return;
	}

	Cprintf(victim, "The power of %s's mind is unleashed!\n\r", ch->name);
}

// Redo all the bonuses for all slivers in the group at once.
void
do_mind_link(CHAR_DATA *ch, char *argument)
{
	int found = FALSE;
	CHAR_DATA *vch = NULL;
	CHAR_DATA *rch = NULL;

	if(ch->race != race_lookup("sliver")) {
		Cprintf(ch, "You don't know how to join minds.\n\r");
		return;
	}

	act("$n glows briefly as $e establishes a mind link.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	for(vch = char_list; vch != NULL; vch = vch->next) {
		if(vch->race == race_lookup("sliver")
		&& is_same_group(ch, vch))
		{
			affect_strip(vch, gsn_telepathy);

			for(rch = char_list; rch != NULL; rch = rch->next) {
				if(rch->remort
				&& rch == vch) {
					found = TRUE;
					apply_sliver_bonus(rch, rch);
					continue;
				}
				if(is_same_group(vch, rch)
				&& rch->race == race_lookup("sliver")
				&& rch != vch) {
					found = TRUE;
					apply_sliver_bonus(rch, vch);
				}
			}
		}
	}

	if(!found) {
		Cprintf(ch, "You can't find anyone with whom to mind link.\n\r");
		return;
	}
}


void
do_group(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *gch;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		CHAR_DATA *gch;
		CHAR_DATA *leader;

		leader = (ch->leader != NULL) ? ch->leader : ch;
		Cprintf(ch, "%s's group:\n\r", PERS(leader, ch));

		for (gch = char_list; gch != NULL; gch = gch->next)
			if (is_same_group(gch, ch))
				Cprintf(ch, "[%2d %s] %-16s %4d/%4d hp %4d/%4d mana %4d/%4d mv %5d tnl\n\r",
						gch->level,
				 IS_NPC(gch) ? "Mob" : class_table[gch->charClass].who_name,
						PERS(gch, ch),
						gch->hit,
						MAX_HP(gch),
						gch->mana,
						MAX_MANA(gch),
						gch->move,
						MAX_MOVE(gch),
						IS_NPC(gch) ? 0 : (gch->level + 1) * exp_per_level(gch, gch->pcdata->points) - gch->exp);

		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (ch == victim)
	{
		Cprintf(ch, "You can't group yourself.\n\r");
		return;
	}

	if ((race_lookup("elf") == ch->race) && ch->remort > 0)
		if ((victim->alignment - ch->alignment > 250) || (ch->alignment - victim->alignment > 250))
		{
			Cprintf(ch, "You can't group with people that different from you.\n\r");
			return;
		}

	if(is_affected(victim, gsn_stance_shadow)
	|| is_affected(ch, gsn_stance_shadow)) {
		Cprintf(ch, "It is impossible to group with a shadow.\n\r");
		return;
	}

	if ((race_lookup("elf") == victim->race) && victim->remort > 0)
		if ((victim->alignment - ch->alignment > 250) || (ch->alignment - victim->alignment > 250))
		{
			Cprintf(ch, "You can't group with remort elves that different from you.\n\r");
			return;
		}

	for (gch = char_list; gch != NULL; gch = gch->next) {
                        if (!IS_NPC(gch)
			&& is_same_group(ch, gch)
			&& !IS_NPC(victim)
			&& ((gch->reclass == reclass_lookup("hermit")
			    && gch != ch)
			|| (ch->reclass == reclass_lookup("hermit")
			    && gch != victim
			    && gch != ch)
			|| (victim->reclass == reclass_lookup("hermit")
			    && gch != victim
			    && gch != ch)))
                        {
                                Cprintf(ch, "Hermits will only travel with one companion at a time.\n\r");
                                return;
                        }

        }

	if (ch->master != NULL || (ch->leader != NULL && ch->leader != ch))
	{
		Cprintf(ch, "But you are following someone else!\n\r");
		return;
	}

	if (victim->master != ch && ch != victim)
	{
		act("$N isn't following you.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
		return;
	}

	if (IS_AFFECTED(victim, AFF_CHARM))
	{
		Cprintf(ch, "You can't remove charmed mobs from your group.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM))
	{
		act("You like your master too much to leave $m!", ch, NULL, victim, TO_VICT, POS_SLEEPING);
		return;
	}

	if (is_same_group(victim, ch) && ch != victim)
	{
		victim->leader = NULL;
		act("$n removes $N from $s group.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		act("$n removes you from $s group.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
		act("You remove $N from your group.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
		return;
	}

	// Clanspell for Kenshi: Brotherhood
	if ((is_affected(ch, gsn_brotherhood)
		|| is_affected(victim, gsn_brotherhood))
	&& (victim->level - ch->level > 12 || ch->level - victim->level > 12))
	{
		Cprintf(ch, "This player is out of your group range.\n\r");
		return;
	}
	else if (!is_affected(ch, gsn_brotherhood)
	&& !is_affected(victim, gsn_brotherhood)
	&& (victim->level > ch->level + 8 || victim->level < ch->level - 8))
	{
		Cprintf(ch, "This player is out of your group range.\n\r");
		return;
	}

	victim->leader = ch;

	act("$N joins $n's group.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	act("You join $n's group.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
	act("$N joins your group.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);

	if(get_skill(ch, gsn_leadership) > 0) {
		Cprintf(ch, "You use your leadership skills to empower the group.\n\r");
		Cprintf(victim, "You are empowered by %s's leadership.\n\r", ch->name);
	}
	if(get_skill(victim, gsn_leadership) > 0) {
		Cprintf(victim, "You use your leadership skills to empower the group.\n\r");
		Cprintf(ch, "You are empowered by %s's leadership.\n\r", victim->name);
	}

	return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void
do_split(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *gch;
	int members;
	int amount_gold = 0, amount_silver = 0;
	int share_gold, share_silver;
	int extra_gold, extra_silver;

	argument = one_argument(argument, arg1);
	one_argument(argument, arg2);

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Split how much?\n\r");
		return;
	}

	amount_silver = atoi(arg1);

	if (arg2[0] != '\0')
		amount_gold = atoi(arg2);

	if (amount_gold < 0 || amount_silver < 0)
	{
		Cprintf(ch, "Your group wouldn't like that.\n\r");
		return;
	}

	if (amount_gold == 0 && amount_silver == 0)
	{
		Cprintf(ch, "You hand out zero coins, but no one notices.\n\r");
		return;
	}

	if (ch->gold < amount_gold || ch->silver < amount_silver)
	{
		Cprintf(ch, "You don't have that much to split.\n\r");
		return;
	}

	members = 0;
	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
	{
		if (!IS_NPC(gch)
		&& is_same_group(gch, ch)
		&& !IS_AFFECTED(gch, AFF_CHARM))
			members++;
	}

	if (members < 2)
	{
		Cprintf(ch, "Just keep it all.\n\r");
		return;
	}

	share_silver = amount_silver / members;
	extra_silver = amount_silver % members;

	share_gold = amount_gold / members;
	extra_gold = amount_gold % members;

	if (share_gold == 0 && share_silver == 0)
	{
		Cprintf(ch, "Don't even bother, cheapskate.\n\r");
		return;
	}

	ch->silver -= amount_silver;
	ch->silver += share_silver + extra_silver;
	ch->gold -= amount_gold;
	ch->gold += share_gold + extra_gold;

	if (share_silver > 0)
		Cprintf(ch, "You split %d silver coins. Your share is %d silver.\n\r", amount_silver, share_silver + extra_silver);

	if (share_gold > 0)
		Cprintf(ch, "You split %d gold coins. Your share is %d gold.\n\r", amount_gold, share_gold + extra_gold);

	if (share_gold == 0)
		sprintf(buf, "$n splits %d silver coins. Your share is %d silver.", amount_silver, share_silver);
	else if (share_silver == 0)
		sprintf(buf, "$n splits %d gold coins. Your share is %d gold.", amount_gold, share_gold);
	else
		sprintf(buf, "$n splits %d silver and %d gold coins, giving you %d silver and %d gold.\n\r", amount_silver, amount_gold, share_silver, share_gold);

	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
		if (gch != ch && is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM))
		{
			act(buf, ch, NULL, gch, TO_VICT, POS_RESTING);
			gch->gold += share_gold;
			gch->silver += share_silver;
		}

	return;
}


void
do_gtell(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *gch;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Tell your group what?\n\r");
		return;
	}

	if (IS_SET(ch->comm, COMM_NOTELL))
	{
		Cprintf(ch, "Your message didn't get through!\n\r");
		return;
	}

	argument = makedrunk(argument, ch);

	if (is_affected(ch, gsn_scramble))
	{
		reverse(argument);
	}

	for (gch = char_list; gch != NULL; gch = gch->next)
	{
		if (gch == ch)
			act("{YYou tell the group '$t'{x", ch, argument, gch, TO_CHAR, POS_SLEEPING);

		if (is_same_group(gch, ch))
			act("{Y$n tells the group '$t'{x", ch, argument, gch, TO_VICT, POS_SLEEPING);
	}

	return;
}


/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool
is_same_group(CHAR_DATA * ach, CHAR_DATA * bch)
{
	if (ach == NULL || bch == NULL)
		return FALSE;

	if (ach->leader != NULL)
		ach = ach->leader;
	if (bch->leader != NULL)
		bch = bch->leader;
	return ach == bch;
}


bool
in_enemy_hall(CHAR_DATA * ch)
{
	int clan;

	if (!is_clan(ch))
		return FALSE;

	for (clan = MIN_PKILL_CLAN; clan <= MAX_PKILL_CLAN; clan++)
	{
		if (ch->in_room->clan == clan && ch->clan != clan)
			return TRUE;
	}

	return FALSE;
}

bool
in_own_hall(CHAR_DATA * ch)
{
	int clan;

	if (!is_clan(ch))
		return FALSE;

	for (clan = MIN_PKILL_CLAN; clan <= MAX_PKILL_CLAN; clan++)
{
		if (ch->in_room->clan == clan && ch->clan == clan)
			return TRUE;
	}

	return FALSE;
}

void
do_marry(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *victim2;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: marry <char1> <char2>\n\r");
		return;
	}
	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
	{
		Cprintf(ch, "The first person mentioned isn't playing.\n\r");
		return;
	}
	if ((victim2 = get_char_world(ch, arg2, TRUE)) == NULL)
	{
		Cprintf(ch, "The second person mentioned isn't playing.\n\r");
		return;
	}
	if (IS_NPC(victim) || IS_NPC(victim2))
	{
		Cprintf(ch, "I don't think they want to be married to the Mob.\n\r");
		return;
	}
	if (victim == victim2) {
		Cprintf(ch, "...they already are life partners with themselves!\n\r");
		return;
	}
	if (victim->pcdata->spouse > 0 || victim2->pcdata->spouse > 0)
	{
		Cprintf(ch, "They are already married!\n\r");
		return;
	}

	if (victim->level < 10 || victim2->level < 10)
	{
		Cprintf(ch, "They are not of the proper level to marry.\n\r");
		return;
	}

	Cprintf(ch, "You pronounce them man and wife!\n\r");
	Cprintf(victim, "You say the big 'I do.'\n\r");
	Cprintf(victim2, "You say the big 'I do.'\n\r");
	victim->pcdata->spouse = str_dup(victim2->name);
	victim2->pcdata->spouse = str_dup(victim->name);
	return;
}

void
do_divorce(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *victim2;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Syntax: divorce <char1> <char2>\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
	{
		Cprintf(ch, "The first person mentioned isn't playing.\n\r");
		return;
	}

	if (arg2[0] == '\0')
	{
		if (victim->pcdata->spouse != NULL)
			Cprintf(ch, "Their spouse, %s, needs to be the second person named.\n\r", victim->pcdata->spouse);
		else
			Cprintf(ch, "That person isn't even married!\n\r");
		return;
	}

	if ((victim2 = get_char_world(ch, arg2, TRUE)) == NULL)
	{
		Cprintf(ch, "The second person mentioned isn't playing.\n\r");
		return;
	}

	if(victim == victim2) {
		Cprintf(ch, "How could they be married to themselves?\n\r");
		return;
	}

	if (IS_NPC(victim) || IS_NPC(victim2))
	{
		Cprintf(ch, "I don't think they're married to the Mob...\n\r");
		return;
	}

	if (str_cmp(victim->pcdata->spouse, victim2->name))
	{
		Cprintf(ch, "They aren't even married!!\n\r");
		return;
	}

	Cprintf(ch, "You hand them their papers.\n\r");
	Cprintf(victim, "Your divorce is final.\n\r");
	Cprintf(victim2, "Your divorce is final.\n\r");
	free_string(victim->pcdata->spouse);
	free_string(victim2->pcdata->spouse);
	victim->pcdata->spouse = NULL;
	victim2->pcdata->spouse = NULL;
	return;
}

void do_spousetalk(CHAR_DATA * ch, char *argument)
{
	DESCRIPTOR_DATA* d;
	CHAR_DATA* victim;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "What do you wish to tell your other half?\n\r");
		return;
	}
	else if (IS_NPC(ch) || ch->pcdata->spouse == NULL)
	{
		Cprintf(ch, "And which spouse was that? After all you aren't hooked up yet.\n\r");
		return;
	}
	else
	{
		for (d = descriptor_list; d != NULL; d = d->next)
		{
			victim = d->original ? d->original : d->character;

			if (d->connected == CON_PLAYING &&
				d->character != ch &&
				!str_cmp(d->character->name, ch->pcdata->spouse))
			{
				Cprintf(ch, "You say to %s, '{c%s{x'\n\r", ch->pcdata->spouse, argument);
				act("$n says to you, '{c$t{x'", ch, argument, d->character, TO_VICT, POS_SLEEPING);
				return;
			}
		}

		Cprintf(ch, "Your spouse is not here.\n\r");
	}

	return;
}

void do_spy(CHAR_DATA* ch, char* argument)
{
	int clan;
	AFFECT_DATA af;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "Which clan do you wish to spy on?\n\r");
		return;
	}

	if(!str_cmp(argument, "none")) {
		Cprintf(ch, "You stop spying.\n\r");
		affect_strip(ch, gsn_spy);
		return;
	}

	if ((clan = clan_lookup(argument)) == 0)
	{
		Cprintf(ch, "No such clan exists.\n\r");
		return;
	}

	if (clan_table[clan].independent)
	{
		Cprintf(ch, "They don't even have a clan channel.\n\r");
		return;
	}

	if (clan == ch->clan)
	{
		Cprintf(ch, "You can't spy on your own clan.\n\r");
		return;
	}

	if (get_skill(ch, gsn_spy) < 1)
	{
		Cprintf(ch, "You don't have a clue how to spy on another clan.\n\r");
		return;
	}

	if (get_skill(ch, gsn_spy) < number_percent())
	{
		Cprintf(ch, "You fail at spying on them.\n\r");
		check_improve(ch, gsn_spy, FALSE, 2);
		return;
	}

	if (is_affected(ch, gsn_spy))
	{
		affect_strip(ch, gsn_spy);
	}

	check_improve(ch, gsn_spy, TRUE, 2);
	af.where = TO_AFFECTS;
	af.type = gsn_spy;
	af.level = ch->level;
	af.duration = ch->level / 2;
	af.location = 0;
	af.modifier = clan;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	Cprintf(ch, "You start spying on them.\n\r");
}


void
do_newbie_channel(CHAR_DATA * ch, char *argument)
{
        DESCRIPTOR_DATA *d;

        if (argument[0] == '\0')
        {
                if (IS_SET(ch->comm, COMM_NONEWBIE))
                {
                        Cprintf(ch, "Newbie channel is now ON.\n\r");
                        REMOVE_BIT(ch->comm, COMM_NONEWBIE);
                }
                else
                {
                        Cprintf(ch, "Newbie channel is now OFF.\n\r");
                        SET_BIT(ch->comm, COMM_NONEWBIE);
                }

                return;
        }
	if (IS_SET(ch->comm, COMM_QUIET))
	{
        	Cprintf(ch, "You must turn off quiet mode first.\n\r");
	        return;
	}

	if (IS_SET(ch->comm, COMM_NOCHANNELS))
	{
	        Cprintf(ch, "The gods have revoked your channel priviliges.\n\r");
        	return;
	}

	REMOVE_BIT(ch->comm, COMM_NONEWBIE);

	argument = makedrunk(argument, ch);

	if (is_affected(ch, gsn_scramble))
        	reverse(argument);

	Cprintf(ch, "{MYou newbie '%s'{x\n\r", argument);

        for (d = descriptor_list; d != NULL; d = d->next)
	{
                if (d->character != NULL)
                {
                        CHAR_DATA *victim;

                        victim = d->original ? d->original : d->character;

                        if (d->connected == CON_PLAYING && d->character != ch
			&& !IS_SET(victim->comm, COMM_QUIET)
			&& !is_hushed(victim, ch)
			&& !IS_SET(victim->comm, COMM_NONEWBIE))
                        {
				if (is_affected(victim, gsn_scramble))
                                        Cprintf(victim, "%s newbies something in comprehensible to you.\n\r", PERS(ch, victim));
                                else
                                        Cprintf(victim, "{M%s newbies '%s'{x\n\r", PERS(ch, victim), argument);
                        }
                }
	}
}

