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

#ifndef RECYCLE_H
#define RECYCLE_H

/* externs */
extern char str_empty[1];
extern int mobile_count;

/* stuff for providing a crash-proof buffer */

#define MAX_BUF      16384
#define MAX_BUF_LIST 10
#define BASE_BUF     1024

/* valid states */
#define BUFFER_SAFE     0
#define BUFFER_OVERFLOW 1
#define BUFFER_FREED    2

/* note recycling */
NOTE_DATA *new_note (void);
void free_note (NOTE_DATA * note);

/* ban data recycling */
BAN_DATA *new_ban (void);
void free_ban (BAN_DATA * ban);

/* descriptor recycling */
DESCRIPTOR_DATA *new_descriptor (void);
void free_descriptor (DESCRIPTOR_DATA * d);

/* char gen data recycling */
GEN_DATA *new_gen_data (void);
void free_gen_data (GEN_DATA * gen);

/* extra descr recycling */
EXTRA_DESCR_DATA *new_extra_descr (void);
void free_extra_descr (EXTRA_DESCR_DATA * ed);

/* affect recycling */
AFFECT_DATA *new_affect (void);
void free_affect (AFFECT_DATA * af);

/* object recycling */
OBJ_DATA *new_obj (void);
void free_obj (OBJ_DATA * obj);

/* character recyling */
CHAR_DATA *new_char (void);
void free_char (CHAR_DATA * ch);
PC_DATA *new_pcdata (void);
void free_pcdata (PC_DATA * pcdata);

/* mob id and memory procedures */
long get_pc_id (void);
long get_mob_id (void);
MEM_DATA *new_mem_data (void);
void free_mem_data (MEM_DATA * memory);
MEM_DATA *find_memory (MEM_DATA * memory, long id);

/* buffer procedures */

BUFFER *new_buf (void);
BUFFER *new_buf_size (int size);
void free_buf (BUFFER * buffer);
bool add_buf (BUFFER * buffer, char *string);
void clear_buf (BUFFER * buffer);
char *buf_string (BUFFER * buffer);

/*
void free_sleep_data(SLEEP_DATA *sd);
SLEEP_DATA *new_sleep_data(void);
*/

/* Reset recycling */
RESET_DATA * new_reset_data(void);
void free_reset_data(RESET_DATA * pReset);

/* area recycling */
AREA_DATA * new_area(void);
void free_area(AREA_DATA * pArea);

/* exits recyling */
EXIT_DATA * new_exit(void);
void free_exit(EXIT_DATA * pExit);

/* room recyling */
ROOM_INDEX_DATA * new_room_index(void);
void free_room_index(ROOM_INDEX_DATA * pRoom);

/* shop recycling */
SHOP_DATA * new_shop(void);
void free_shop(SHOP_DATA * pShop);

/* object recycling */
OBJ_INDEX_DATA * new_obj_index(void);
void free_obj_index(OBJ_INDEX_DATA * pObj);

/* mob recycling */
MOB_INDEX_DATA * new_mob_index(void);
void free_mob_index(MOB_INDEX_DATA * pMob);

/* board recycling */
BOARD_DATA* new_board(void);
void free_board(BOARD_DATA * pBoard);

/* note read recycling */
NOTE_READ_DATA* new_note_read(void);
void free_note_read(NOTE_READ_DATA *pNoteRead);

/* mprog code recycling 
MPROG_CODE *new_mprog_code(void);
void free_mprog_code(MPROG_CODE *pMProgCode);

/* mprog list recycling 
MPROG_LIST *new_mprog_list(void);
void free_mprog_list(MPROG_LIST *pMProgList);
*/
#endif
