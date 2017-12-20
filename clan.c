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

#include "clan.h"
#include "utils.h"


#define ROOM_VNUM_HELP 11240
#define ROOM_VNUM_HELP_DOM 31193
#define ROOM_VNUM_TR 11305
#define ROOM_VNUM_TR_DOM 31168
#define ROOM_VNUM_HELLIONS 11281
#define ROOM_VNUM_HELLIONS_DOM ROOM_VNUM_ALTAR_DOM
#define ROOM_VNUM_FELLOWS 11301
#define ROOM_VNUM_FELLOWS_DOM ROOM_VNUM_ALTAR_DOM

/* for clans */
const CLAN_TYPE clan_table[MAX_CLAN] =
{
    /*  name,           who entry,      death-transfer room,    independent */
    /* independent should be FALSE if is a real clan */
   {"the guilds", "",            ROOM_VNUM_ALTAR,   ROOM_VNUM_ALTAR_DOM,       TRUE, FALSE, 0},
   {"loner",      "[  Loner ] ", ROOM_VNUM_ALTAR,   ROOM_VNUM_ALTAR_DOM,       TRUE, TRUE, 0},
   {"outcast",      "[ Outcast] ", ROOM_VNUM_ALTAR,   ROOM_VNUM_ALTAR_DOM,       TRUE, TRUE, 0},
   {"seeker",     "[ Seeker ] ", ROOM_VNUM_SEEKERS, ROOM_VNUM_SEEKERS_DOMINIA, FALSE, TRUE, JOIN_A},
   {"kindred",    "[ Kindred] ", ROOM_VNUM_KINDRED, ROOM_VNUM_KINDRED_DOMINIA, FALSE, TRUE, JOIN_B},
   {"venari",     "[ Venari ] ", ROOM_VNUM_VENARI,  ROOM_VNUM_VENARI_DOMINIA,  FALSE, TRUE, JOIN_C},
   {"kenshi",     "[ Kenshi ] ", ROOM_VNUM_KENSHI,  ROOM_VNUM_KENSHI_DOMINIA,  FALSE, TRUE, JOIN_D},
   {"dummyguild", "( A Guild) ", ROOM_VNUM_ALTAR,  ROOM_VNUM_ALTAR_DOM,    FALSE, FALSE, JOIN_E},
   {"help", "(  HELP  ) ", ROOM_VNUM_HELP, ROOM_VNUM_HELP_DOM, FALSE, FALSE, JOIN_F},
   {"treasure hunter", "(Treasure) ", ROOM_VNUM_TR, ROOM_VNUM_TR_DOM, FALSE, FALSE, JOIN_G},
   {"hellions", "(Hellions) ", ROOM_VNUM_HELLIONS, ROOM_VNUM_HELLIONS_DOM, FALSE, FALSE, JOIN_H},
   {"fellowship", "(Fellows) ", ROOM_VNUM_FELLOWS, ROOM_VNUM_FELLOWS_DOM, FALSE, FALSE, JOIN_I},
};

int
clan_lookup (const char *name)
{
   int clan;

   for (clan = 0; clan < MAX_CLAN; clan++)
   {
      if (LOWER (name[0]) == LOWER (clan_table[clan].name[0])
          && !str_prefix (name, clan_table[clan].name))
         return clan;
   }

   //loner people in justice
   if (!str_cmp(name, "justice"))
   {
   	return 1;
   }

   return 0;
}

char *
clanName(const int index) {
    return capitalizeWords( clan_table[index].name );
}

int
clan_wiznet_lookup (int clan)
{
	return clan_table[clan].join_constant;
}

