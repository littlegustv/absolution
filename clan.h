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

#ifndef CLAN_H
#define CLAN_H

#define CLAN_NONE	0
#define CLAN_LONER	1
#define CLAN_OUTCAST	2
#define CLAN_SEEKERS	3
#define CLAN_KINDRED	4
#define CLAN_VENARI	5
#define CLAN_KENSHI	6
#define GUILD_DUMMY	7
#define GUILD_HELP	8
#define GUILD_TR	9
#define GUILD_HELLIONS	10
#define GUILD_FELLOWS	11

#define MAX_PKILL_CLAN CLAN_KENSHI
#define MIN_PKILL_CLAN CLAN_SEEKERS
#define MAX_NC_CLAN GUILD_FELLOWS
#define MIN_NC_CLAN GUILD_DUMMY
#define MAX_CLAN (MAX_NC_CLAN + 1)

int clan_lookup (const char *name);
int clan_wiznet_lookup (int clan);
char *clanName(const int index);

typedef struct clan_type CLAN_TYPE;

struct clan_type {
	char *name;
	char *who_name;
	int hall;
	int hall_dominia;
	bool independent;      /* true for loners and outcasts */
	bool pkiller;
	int join_constant;
};

struct clan_report_info {
	int clan_pkills;
	int clan_pkilled;
	char best_pkill[15];
	char worst_pkilled[15];
	int player_pkills;
	int player_pkilled;
};

struct clan_leader_data {
	char* recruiter1;
	char* recruiter2;
	char* leader;
};

typedef struct clan_leader_data CLAN_LEADER_DATA;

struct clan_report_info clan_report[MAX_CLAN];
extern const CLAN_TYPE clan_table[MAX_CLAN];
extern CLAN_LEADER_DATA clan_leadership[MAX_CLAN];


#endif
