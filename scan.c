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
#include "utils.h"


char *const distance[4] =
{
   "right there", "close by to the", "not too far", "off in the distance"
};

int scan_list (ROOM_INDEX_DATA * scan_room, CHAR_DATA * ch, int depth, int door);
void scan_char (CHAR_DATA * victim, CHAR_DATA * ch, int depth, int door);

void
start_scan (CHAR_DATA * ch, CHAR_DATA *victim, char *argument)
{
   extern char *const dir_name[];
   char arg1[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
   ROOM_INDEX_DATA *scan_room;
   EXIT_DATA *pExit;
   int door, depth;

   argument = one_argument (argument, arg1);

   if(victim == NULL
   || victim->in_room == NULL)
		return;

   scan_room = victim->in_room;

   if(arg1[0] == '\0') {
   	scan_list(scan_room, ch, 0, 0);
	return;
   }

   if (!str_cmp (arg1, "n") || !str_cmp (arg1, "north"))
      door = 0;
   else if (!str_cmp (arg1, "e") || !str_cmp (arg1, "east"))
      door = 1;
   else if (!str_cmp (arg1, "s") || !str_cmp (arg1, "south"))
      door = 2;
   else if (!str_cmp (arg1, "w") || !str_cmp (arg1, "west"))
      door = 3;
   else if (!str_cmp (arg1, "u") || !str_cmp (arg1, "up"))
      door = 4;
   else if (!str_cmp (arg1, "d") || !str_cmp (arg1, "down"))
      door = 5;
   else
   {
      Cprintf(ch, "Which way do you want to scan?\n\r");
      return;
   }

   act("You scan intently $T.", ch, NULL, dir_name[door], TO_CHAR, POS_RESTING);
   sprintf (buf, "Scanning %s you see:\n\r", dir_name[door]);

   for (depth = 1; depth < 4; depth++)
   {
      if ((pExit = scan_room->exit[door]) != NULL)
      {
         scan_room = pExit->u1.to_room;
         scan_list (pExit->u1.to_room, ch, depth, door);
      }
   }
   return;
}

int
scan_list (ROOM_INDEX_DATA * scan_room, CHAR_DATA * ch, int depth,
           int door)
{
   CHAR_DATA *rch;
   int found = FALSE;

   if (scan_room == NULL)
      return FALSE;
   for (rch = scan_room->people; rch != NULL; rch = rch->next_in_room)
   {
      if (!IS_NPC (rch) && rch->invis_level > get_trust (ch))
         continue;
      if (can_see (ch, rch)) {
         scan_char (rch, ch, depth, door);
         found = TRUE;
      }
   }
   return found;
}

void
scan_char (CHAR_DATA * victim, CHAR_DATA * ch, int depth, int door)
{
   extern char *const dir_name[];
   extern char *const distance[];

   if (!IS_NPC(victim)
   && number_percent() < get_curr_stat(victim, STAT_INT) / 2) {
      Cprintf(victim, "You get the odd feeling you are being watched.\n\r");
   }

   if (depth == 0)
   {
      Cprintf(ch, "%s, %s.\n\r", PERS(victim, ch), distance[depth]);
   }
   else
   {
      Cprintf(ch, "%s, %s %s.\n\r", PERS(victim, ch), distance[depth], dir_name[door]);
   }
   return;
}

