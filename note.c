/* new code here!! */

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
#include <sys/time.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "merc.h"
#include "recycle.h"

/* globals from db.c for load_notes */
extern int _filbuf (FILE *);
extern FILE *fpArea;
extern char strArea[MAX_INPUT_LENGTH];

/* local procedures */

NOTE_DATA *note_list;
NOTE_DATA *idea_list;
NOTE_DATA *penalty_list;
NOTE_DATA *news_list;
NOTE_DATA *changes_list;

void
append_note (NOTE_DATA * pnote)
{
   FILE *fp;
   char *name;
   NOTE_DATA **list;
   NOTE_DATA *last;

   switch (pnote->type)
   {
   default:
      return;
   case NOTE_NOTE:
      name = NOTE_FILE;
      list = &note_list;
      break;
   case NOTE_IDEA:
      name = IDEA_FILE;
      list = &idea_list;
      break;
   case NOTE_PENALTY:
      name = PENALTY_FILE;
      list = &penalty_list;
      break;
   case NOTE_NEWS:
      name = NEWS_FILE;
      list = &news_list;
      break;
   case NOTE_CHANGES:
      name = CHANGES_FILE;
      list = &changes_list;
      break;
   }

   if (*list == NULL)
      *list = pnote;
   else
   {
      for (last = *list; last->next != NULL; last = last->next)
         ;
      last->next = pnote;
   }

   fclose (fpReserve);
   if ((fp = fopen (name, "a")) == NULL)
   {
      perror (name);
   }
   else
   {
      fprintf (fp, "Sender  %s~\n", pnote->sender);
      fprintf (fp, "Date    %s~\n", pnote->date);
      fprintf (fp, "Stamp   %ld\n", pnote->date_stamp);
      fprintf (fp, "To      %s~\n", pnote->to_list);
      fprintf (fp, "Subject %s~\n", pnote->subject);
      fprintf (fp, "Text\n%s~\n", pnote->text);
      fclose (fp);
   }
   fpReserve = fopen (NULL_FILE, "r");
}
