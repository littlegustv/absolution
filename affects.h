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

#ifndef AFFECTS_H
#define AFFECTS_H

typedef struct affect_data AFFECT_DATA;

/*
 * An affect.
 */
struct affect_data {
    AFFECT_DATA *next;
    bool valid;
    int where;
    int type;
    int level;
    int duration;
    int location;
    int modifier;
    int extra;
    int clan;
    long bitvector;
};

#define IS_AFFECTED(ch, sn)   (IS_SET((ch)->affected_by, (sn)))

void affect_to_char(CHAR_DATA * ch, AFFECT_DATA * paf);
void affect_to_obj(OBJ_DATA * obj, AFFECT_DATA * paf);
void affect_to_room(ROOM_INDEX_DATA * room, AFFECT_DATA * af);
void affect_remove_room(ROOM_INDEX_DATA * room, AFFECT_DATA * af);
void affect_remove(CHAR_DATA * ch, AFFECT_DATA * paf);
void affect_remove_obj(OBJ_DATA * obj, AFFECT_DATA * paf);
void affect_strip(CHAR_DATA * ch, int sn);
bool is_affected(CHAR_DATA * ch, int sn);
bool obj_is_affected(OBJ_DATA* obj, int sn);
bool room_is_affected(ROOM_INDEX_DATA * room, int sn);
AFFECT_DATA *get_affect(CHAR_DATA *ch, int sn);
AFFECT_DATA *get_room_affect(ROOM_INDEX_DATA *room, int sn);
AFFECT_DATA *get_obj_affect(OBJ_DATA *obj, int sn);
void affect_join(CHAR_DATA * ch, AFFECT_DATA * paf);
void affect_merge(CHAR_DATA * ch, AFFECT_DATA * paf, int sn);
AFFECT_DATA *affect_find(AFFECT_DATA * paf, int sn);
void affect_check(CHAR_DATA * ch, int where, int vector);
void affect_refresh(CHAR_DATA * ch, int sn, int duration);
void affect_modify(CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd);

#endif
