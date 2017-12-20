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
 *  ROM 2.4 is copyright 1993-1996 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@pacinfo.com)                                  *
 *      Gabrielle Taylor (gtaylor@pacinfo.com)                             *
 *      Brian Moore (rom@rom.efn.org)                                      *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

#include "merc.h"
#include "deity.h"
#include "utils.h"



/* Deity code: name, deity_num, spell, ticks_a, ticks_b, ticks_c, cost_a, cost_b, cost_c */

const struct deity_type deity_table[] =
{
   {"Gabriel",     1, "surge",              15, 30, 45, 3000, 5000, 7000},
   {"Carnas",      2, "injustice",          10, 15, 20, 2500, 4500, 6000},
   {"Taelisan",    3, "gullivers travel",   15, 25, 40, 2000, 3000, 4000},
   {"Sky",         4, "detect all",         10, 20, 30, 2500, 4500, 6000},
   {"Bhaal"    ,   5, "pacifism",           10, 15, 20, 2500, 4500, 6500},
   {NULL,0,NULL,0,0,0,0,0,0}
};

int
deity_lookup (const char *name)
{
   int clan;

   for (clan = 0; clan < 5; clan++)
   {
      if (LOWER (name[0]) == LOWER (deity_table[clan].name[0])
          && !str_cmp (name, deity_table[clan].name))
         return deity_table[clan].deity;
   }

   return 0;
}
