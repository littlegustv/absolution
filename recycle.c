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
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "merc.h"
#include "recycle.h"
#include "utils.h"
#include "types.h"

extern void end_null(void* vo, int target);

/* stuff for recyling notes */
extern NOTE_DATA *note_free;

/* recycle a note */
void
free_note(NOTE_DATA * note) {
    free_string(note->sender);
    free_string(note->to_list);
    free_string(note->subject);
    free_string(note->date);
    free_string(note->text);

    note->next = note_free;
    note_free = note;
}

/* allocate memory for a new note or recycle */
NOTE_DATA *
new_note()
{
	NOTE_DATA *note;

	if (note_free)
	{
		note = note_free;
		note_free = note_free->next;
	}
	else
		note = (NOTE_DATA *) alloc_mem(sizeof(NOTE_DATA));

	/* Zero all the field - Envy does not gurantee zeroed memory */
	note->next = NULL;
	note->sender = NULL;
	note->expire = 0;
	note->to_list = NULL;
	note->subject = NULL;
	note->date = NULL;
	note->date_stamp = 0;
	note->text = NULL;

	return note;
}

/* stuff for recycling ban structures */
BAN_DATA *ban_free;

BAN_DATA *
new_ban(void)
{
	static BAN_DATA ban_zero;
	BAN_DATA *ban;

	if (ban_free == NULL)
		ban = (BAN_DATA *) alloc_perm(sizeof(*ban));
	else
	{
		ban = ban_free;
		ban_free = ban_free->next;
	}

	*ban = ban_zero;
	VALIDATE(ban);
	ban->name = &str_empty[0];
	return ban;
}

void
free_ban(BAN_DATA * ban)
{
	if (!IS_VALID(ban))
		return;

	free_string(ban->name);
	INVALIDATE(ban);

	ban->next = ban_free;
	ban_free = ban;
}



/* stuff for recycling descriptors */
DESCRIPTOR_DATA *descriptor_free;

DESCRIPTOR_DATA *
new_descriptor(void)
{
	static DESCRIPTOR_DATA d_zero;
	DESCRIPTOR_DATA *d;

	if (descriptor_free == NULL)
		d = (DESCRIPTOR_DATA *) alloc_perm(sizeof(*d));
	else
	{
		d = descriptor_free;
		descriptor_free = descriptor_free->next;
	}

	*d = d_zero;
	VALIDATE(d);
	return d;
}

void
free_descriptor(DESCRIPTOR_DATA * d)
{
	if (!IS_VALID(d))
		return;

	free_string(d->host);
	if(d->outsize)
		free_mem(d->outbuf, d->outsize);
	INVALIDATE(d);
	d->next = descriptor_free;
	descriptor_free = d;
}



/* stuff for recycling gen_data */
GEN_DATA *gen_data_free;

GEN_DATA *
new_gen_data(void)
{
	static GEN_DATA gen_zero;
	GEN_DATA *gen;

	if (gen_data_free == NULL)
		gen = (GEN_DATA *) alloc_perm(sizeof(*gen));
	else
	{
		gen = gen_data_free;
		gen_data_free = gen_data_free->next;
	}
	*gen = gen_zero;
	VALIDATE(gen);
	return gen;
}

void
free_gen_data(GEN_DATA * gen)
{
	if (!IS_VALID(gen))
		return;

	INVALIDATE(gen);

	gen->next = gen_data_free;
	gen_data_free = gen;
}



/* stuff for recycling extended descs */
EXTRA_DESCR_DATA *extra_descr_free;

EXTRA_DESCR_DATA *
new_extra_descr(void)
{
	EXTRA_DESCR_DATA *ed;

	if (extra_descr_free == NULL)
		ed = (EXTRA_DESCR_DATA *) alloc_perm(sizeof(*ed));
	else
	{
		ed = extra_descr_free;
		extra_descr_free = extra_descr_free->next;
	}

	ed->keyword = &str_empty[0];
	ed->description = &str_empty[0];
	VALIDATE(ed);
	return ed;
}

void
free_extra_descr(EXTRA_DESCR_DATA * ed)
{
	if (!IS_VALID(ed))
		return;

	free_string(ed->keyword);
	free_string(ed->description);
	INVALIDATE(ed);

	ed->next = extra_descr_free;
	extra_descr_free = ed;
}



/* stuff for recycling affects */
AFFECT_DATA *affect_free;

AFFECT_DATA *
new_affect(void) {
    static AFFECT_DATA af_zero;
    AFFECT_DATA *af;

    if (affect_free == NULL) {
        af = (AFFECT_DATA *) alloc_perm(sizeof(*af));
    } else {
        af = affect_free;
        affect_free = affect_free->next;
    }

    *af = af_zero;


    VALIDATE(af);
    return af;
}

void
free_affect(AFFECT_DATA * af) {
    if (!IS_VALID(af)) {
        return;
    }

    INVALIDATE(af);
    af->next = affect_free;
    affect_free = af;
}



/* stuff for recycling objects */
OBJ_DATA *obj_free;

OBJ_DATA *
new_obj(void)
{
	static OBJ_DATA obj_zero;
	OBJ_DATA *obj;

	if (obj_free == NULL)
		obj = (OBJ_DATA *) alloc_perm(sizeof(*obj));
	else
	{
		obj = obj_free;
		obj_free = obj_free->next;
	}

	obj->respawn_owner = NULL;
	*obj = obj_zero;
	VALIDATE(obj);

	return obj;
}

void
free_obj(OBJ_DATA * obj)
{
	AFFECT_DATA *paf, *paf_next;
	EXTRA_DESCR_DATA *ed, *ed_next;

	if (!IS_VALID(obj))
		return;

	for (paf = obj->affected; paf != NULL; paf = paf_next)
	{
		paf_next = paf->next;
		free_affect(paf);
	}
	obj->affected = NULL;

	for (ed = obj->extra_descr; ed != NULL; ed = ed_next)
	{
		ed_next = ed->next;
		free_extra_descr(ed);
	}
	obj->extra_descr = NULL;

	free_string(obj->name);
	free_string(obj->description);
	free_string(obj->short_descr);
	free_string(obj->owner);
	INVALIDATE(obj);

	obj->next = obj_free;
	obj_free = obj;
}


/* stuff for recyling characters */
CHAR_DATA *char_free;

CHAR_DATA *
new_char(void)
{
	static CHAR_DATA ch_zero;
	CHAR_DATA *ch;
	int i;

	if (char_free == NULL)
		ch = (CHAR_DATA *) alloc_perm(sizeof(*ch));
	else
	{
		ch = char_free;
		char_free = char_free->next;
	}

	*ch = ch_zero;
	VALIDATE(ch);
	ch->rptitle = &str_empty[0];
	ch->name = &str_empty[0];
	ch->short_descr = &str_empty[0];
	ch->long_descr = &str_empty[0];
	ch->shift_short = &str_empty[0];
	ch->shift_long = &str_empty[0];
	ch->shift_name = &str_empty[0];
	ch->site = &str_empty[0];
	ch->description = &str_empty[0];
	ch->prompt = &str_empty[0];
	ch->prefix = &str_empty[0];
	ch->logon = current_time;
	ch->lines = PAGELEN;
	for (i = 0; i < 4; i++)
		ch->armor[i] = 100;

	ch->position = POS_STANDING;
	ch->hit = 20;
	ch->max_hit = 20;
	ch->mana = 100;
	ch->max_mana = 100;
	ch->move = 100;
	ch->max_move = 100;
	ch->played_perm = 0;
	for (i = 0; i < MAX_STATS; i++)
	{
		ch->perm_stat[i] = 13;
		ch->mod_stat[i] = 0;
	}

	return ch;
}

void
free_char(CHAR_DATA * ch)
{
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;

	if (!IS_VALID(ch))
		return;

	if (IS_NPC(ch))
		mobile_count--;

	for (obj = ch->carrying; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;
		extract_obj(obj);
	}

	for (paf = ch->affected; paf != NULL; paf = paf_next)
	{
		paf_next = paf->next;
		if(skill_table[paf->type].end_fun != end_null)
         		skill_table[paf->type].end_fun((void*)ch, TARGET_CHAR);
		affect_remove(ch, paf);
	}

	free_string(ch->rptitle);
	free_string(ch->name);
	free_string(ch->short_descr);
	free_string(ch->long_descr);
	free_string(ch->description);
	free_string(ch->shift_short);
	free_string(ch->shift_long);
	free_string(ch->shift_name);
	free_string(ch->site);
	free_string(ch->rreply);	/* IMC v0.2 - Add this */
	free_string(ch->prompt);
	free_string(ch->prefix);

	if (ch->pcdata != NULL)
		free_pcdata(ch->pcdata);

	ch->next = char_free;
	char_free = ch;

	INVALIDATE(ch);
	return;
}



PC_DATA *pcdata_free;

PC_DATA *
new_pcdata(void)
{
	int alias;
	int macro;
	int sub;

	static PC_DATA pcdata_zero;
	PC_DATA *pcdata;

	if (pcdata_free == NULL)
		pcdata = (PC_DATA *) alloc_perm(sizeof(*pcdata));
	else
	{
		pcdata = pcdata_free;
		pcdata_free = pcdata_free->next;
	}

	*pcdata = pcdata_zero;

	for (alias = 0; alias < MAX_ALIAS; alias++)
	{
		pcdata->alias[alias] = NULL;
		pcdata->alias_sub[alias] = NULL;
	}

	for (macro = 0; macro < MAX_NUM_MACROS; macro++)
	{
		pcdata->macro[macro].name = NULL;
		pcdata->macro[macro].definition = NULL;
		for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
		{
			pcdata->macro[macro].subs[sub] = NULL;
		}
	}

	pcdata->macroInstances = NULL;
	pcdata->macroTimer = 0;
	pcdata->buffer = new_buf();


	VALIDATE(pcdata);
	return pcdata;
}

void
free_pcdata(PC_DATA * pcdata)
{
	int alias;
	int t;
	int macro;
	int sub;

	if (!IS_VALID(pcdata))
		return;

	free_string(pcdata->pwd);
	free_string(pcdata->bamfin);
	free_string(pcdata->bamfout);
	free_string(pcdata->title);
	free_buf(pcdata->buffer);

	if (pcdata->spouse != NULL)
	{
		free_string(pcdata->spouse);
	}

	for (t = 0; t < 5; t++)
	{
		free_string(pcdata->hush[t]);
		pcdata->hush[t] = NULL;
	}



	for (alias = 0; alias < MAX_ALIAS; alias++)
	{
		free_string(pcdata->alias[alias]);
		free_string(pcdata->alias_sub[alias]);
	}

	for (macro = 0; macro < MAX_NUM_MACROS; macro++)
	{
		free_string(pcdata->macro[macro].name);
		free_string(pcdata->macro[macro].definition);
		for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
		{
			free_string(pcdata->macro[macro].subs[sub]);
		}
	}

	while (pcdata->macroInstances)
	{
		free_instance(pcdata, pcdata->macroInstances);
	}

	pcdata->macroTimer = 0;


	INVALIDATE(pcdata);
	pcdata->next = pcdata_free;
	pcdata_free = pcdata;

	return;
}



/* stuff for setting ids */
long last_pc_id;
long last_mob_id;

long
get_pc_id(void)
{
	int val;

	val = (current_time <= last_pc_id) ? last_pc_id + 1 : current_time;
	last_pc_id = val;
	return val;
}

long
get_mob_id(void)
{
	last_mob_id++;
	return last_mob_id;
}



MEM_DATA *mem_data_free;

MEM_DATA *
new_mem_data(void)
{
	MEM_DATA *memory;

	if (mem_data_free == NULL)
		memory = (MEM_DATA *) alloc_mem(sizeof(*memory));
	else
	{
		memory = mem_data_free;
		mem_data_free = mem_data_free->next;
	}

	memory->next = NULL;
	memory->id = 0;
	memory->reaction = 0;
	memory->when = 0;
	VALIDATE(memory);

	return memory;
}

void
free_mem_data(MEM_DATA * memory)
{
	if (!IS_VALID(memory))
		return;

	memory->next = mem_data_free;
	mem_data_free = memory;
	INVALIDATE(memory);
}



/* procedures and constants needed for buffering */
BUFFER *buf_free;

/* buffer sizes */
const int buf_size[MAX_BUF_LIST] =
{
	16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384
};

/* local procedure for finding the next acceptable size */
/* -1 indicates out-of-boundary error */
int
get_size(int val)
{
	int i;

	for (i = 0; i < MAX_BUF_LIST; i++)
		if (buf_size[i] >= val)
		{
			return buf_size[i];
		}

	return -1;
}

BUFFER *
new_buf()
{
	BUFFER *buffer;

	if (buf_free == NULL)
		buffer = (BUFFER *) alloc_perm(sizeof(*buffer));
	else
	{
		buffer = buf_free;
		buf_free = buf_free->next;
	}

	buffer->next = NULL;
	buffer->state = BUFFER_SAFE;
	buffer->size = get_size(BASE_BUF);

	buffer->string = (char *) alloc_mem(buffer->size);
	buffer->string[0] = '\0';
	VALIDATE(buffer);

	return buffer;
}

BUFFER *
new_buf_size(int size)
{
	BUFFER *buffer;

	if (buf_free == NULL)
		buffer = (BUFFER *) alloc_perm(sizeof(*buffer));
	else
	{
		buffer = buf_free;
		buf_free = buf_free->next;
	}

	buffer->next = NULL;
	buffer->state = BUFFER_SAFE;
	buffer->size = get_size(size);
	if (buffer->size == -1)
	{
		bug("new_buf: buffer size %d too large.", size);
		exit(1);
	}
	buffer->string = (char *) alloc_mem(buffer->size);
	buffer->string[0] = '\0';
	VALIDATE(buffer);

	return buffer;
}


void
free_buf(BUFFER * buffer)
{
	if (!IS_VALID(buffer))
		return;

	free_mem(buffer->string, buffer->size);
	buffer->string = NULL;
	buffer->size = 0;
	buffer->state = BUFFER_FREED;
	INVALIDATE(buffer);

	buffer->next = buf_free;
	buf_free = buffer;
}


bool
add_buf(BUFFER * buffer, char *string)
{
	int len;
	char *oldstr;
	int oldsize;

	oldstr = buffer->string;
	oldsize = buffer->size;

	if (buffer->state == BUFFER_OVERFLOW)	/* don't waste time on bad strings! */
		return FALSE;

	len = strlen(buffer->string) + strlen(string) + 1;

	while (len >= buffer->size)	/* increase the buffer size */
	{
		buffer->size = get_size(buffer->size + 1);
		{
			if (buffer->size == -1)		/* overflow */
			{
				buffer->size = oldsize;
				buffer->state = BUFFER_OVERFLOW;
				bug("buffer overflow past size %d", buffer->size);
				return FALSE;
			}
		}
	}

	if (buffer->size != oldsize)
	{
		buffer->string = (char *) alloc_mem(buffer->size);

		strcpy(buffer->string, oldstr);
		free_mem(oldstr, oldsize);
	}

	strcat(buffer->string, string);
	return TRUE;
}


void
clear_buf(BUFFER * buffer)
{
	buffer->string[0] = '\0';
	buffer->state = BUFFER_SAFE;
}


char *
buf_string(BUFFER * buffer)
{
	return buffer->string;
}



SLEEP_DATA *sd_free;

SLEEP_DATA *new_sleep_data(void)
{
	SLEEP_DATA *sd;
	if (sd_free == NULL)
		sd = alloc_perm(sizeof(*sd));
	else
	{
		sd = sd_free;
		sd_free = sd_free->next;
	}

	sd->vnum = 0;
	sd->timer = 0;
	sd->line = 0;
	sd->prog = NULL;
	sd->mob = NULL;
	sd->ch = NULL;
	sd->next = NULL;
	sd->prev = NULL;
	VALIDATE(sd);
	return sd;
}

void free_sleep_data(SLEEP_DATA *sd)
{
	if (!IS_VALID(sd))
		return;

	INVALIDATE(sd);
	sd->next = sd_free;
	sd_free = sd;
}



RESET_DATA *reset_free;

RESET_DATA *
new_reset_data(void)
{
    RESET_DATA *pReset;

    if (!reset_free)
    {
        pReset = (RESET_DATA *) alloc_perm(sizeof(*pReset));
        top_reset++;
    }
    else
    {
        pReset = reset_free;
        reset_free = reset_free->next;
    }

    pReset->next = NULL;
    pReset->command = 'X';
    pReset->arg1 = 0;
    pReset->arg2 = 0;
    pReset->arg3 = 0;

    return pReset;
}

void
free_reset_data(RESET_DATA * pReset)
{
    pReset->next = reset_free;
    reset_free = pReset;
    return;
}



AREA_DATA *area_free;

AREA_DATA *
new_area(void)
{
    AREA_DATA *pArea;
    char buf[MAX_INPUT_LENGTH];

    if (!area_free)
    {
        pArea = (AREA_DATA *) alloc_perm(sizeof(*pArea));
        top_area++;
    }
    else
    {
        pArea = area_free;
        area_free = area_free->next;
    }

    pArea->next = NULL;
    pArea->name = str_dup("New area");
/*    pArea->recall           =   ROOM_VNUM_TEMPLE;      ROM OLC */
    pArea->area_flags = AREA_ADDED;
    pArea->security = 1;
    pArea->builders = str_dup("None");
    pArea->min_vnum = 0;
    pArea->max_vnum = 0;
    pArea->age = 0;
    pArea->nplayer = 0;
    pArea->empty = TRUE;        /* ROM patch */
    sprintf(buf, "area%d.are", pArea->vnum);
    pArea->file_name = str_dup(buf);
    pArea->vnum = top_area - 1;
    pArea->credits = str_dup("[ ALL ]");

    return pArea;
}

void
free_area(AREA_DATA * pArea)
{
    free_string(pArea->name);
    free_string(pArea->file_name);
    free_string(pArea->builders);

    pArea->next = area_free->next;
    area_free = pArea;
    return;
}



EXIT_DATA *exit_free;

EXIT_DATA *
new_exit(void)
{
    EXIT_DATA *pExit;

    if (!exit_free)
    {
        pExit = (EXIT_DATA *) alloc_perm(sizeof(*pExit));
        top_exit++;
    }
    else
    {
        pExit = exit_free;
        exit_free = exit_free->next;
    }

    pExit->u1.to_room = NULL;   /* ROM OLC */
    pExit->next = NULL;
/*  pExit->vnum         =   0;                        ROM OLC */
    pExit->exit_info = 0;
    pExit->key = 0;
    pExit->keyword = &str_empty[0];
    pExit->description = &str_empty[0];
    pExit->rs_flags = 0;

    return pExit;
}

void
free_exit(EXIT_DATA * pExit)
{
    free_string(pExit->keyword);
    free_string(pExit->description);

    pExit->next = exit_free;
    exit_free = pExit;
    return;
}



ROOM_INDEX_DATA *room_index_free;

ROOM_INDEX_DATA *
new_room_index(void)
{
    ROOM_INDEX_DATA *pRoom;
    int door;

    if (!room_index_free)
    {
        pRoom = (ROOM_INDEX_DATA *) alloc_perm(sizeof(*pRoom));
        top_room++;
    }
    else
    {
        pRoom = room_index_free;
        room_index_free = room_index_free->next;
    }

    pRoom->next = NULL;
    pRoom->people = NULL;
    pRoom->contents = NULL;
    pRoom->extra_descr = NULL;
    pRoom->area = NULL;
    pRoom->owner = str_dup("");;

    for (door = 0; door < MAX_DIR; door++)
        pRoom->exit[door] = NULL;

    pRoom->name = &str_empty[0];
    pRoom->description = &str_empty[0];
    pRoom->vnum = 0;
    pRoom->room_flags = 0;
    pRoom->light = 0;
    pRoom->sector_type = 0;
    pRoom->affected = NULL;
    pRoom->heal_rate = 100;
    pRoom->mana_rate = 100;

    return pRoom;
}

void
free_room_index(ROOM_INDEX_DATA * pRoom)
{
    int door;
    EXTRA_DESCR_DATA *pExtra;
    RESET_DATA *pReset;

    free_string(pRoom->name);
    free_string(pRoom->description);

    for (door = 0; door < MAX_DIR; door++)
    {
        if (pRoom->exit[door])
            free_exit(pRoom->exit[door]);
    }

    for (pExtra = pRoom->extra_descr; pExtra; pExtra = pExtra->next)
    {
        free_extra_descr(pExtra);
    }

    for (pReset = pRoom->reset_first; pReset; pReset = pReset->next)
    {
        free_reset_data(pReset);
    }

    pRoom->next = room_index_free;
    room_index_free = pRoom;
    return;
}



SHOP_DATA *shop_free;

SHOP_DATA *
new_shop(void)
{
    SHOP_DATA *pShop;
    int buy;

    if (!shop_free)
    {
        pShop = (SHOP_DATA *) alloc_perm(sizeof(*pShop));
        top_shop++;
    }
    else
    {
        pShop = shop_free;
        shop_free = shop_free->next;
    }

    pShop->next = NULL;
    pShop->keeper = 0;

    for (buy = 0; buy < MAX_TRADE; buy++)
        pShop->buy_type[buy] = 0;

    pShop->profit_buy = 100;
    pShop->profit_sell = 100;
    pShop->open_hour = 0;
    pShop->close_hour = 23;

    return pShop;
}

void
free_shop(SHOP_DATA * pShop)
{
    pShop->next = shop_free;
    shop_free = pShop;
    return;
}



OBJ_INDEX_DATA *obj_index_free;

OBJ_INDEX_DATA *
new_obj_index(void)
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if (!obj_index_free)
    {
        pObj = (OBJ_INDEX_DATA *) alloc_perm(sizeof(*pObj));
        top_obj_index++;
    }
    else
    {
        pObj = obj_index_free;
        obj_index_free = obj_index_free->next;
    }

    pObj->next = NULL;
    pObj->extra_descr = NULL;
    pObj->affected = NULL;
    pObj->area = NULL;
    pObj->name = str_dup("no name");
    pObj->short_descr = str_dup("(no short description)");
    pObj->description = str_dup("");
    pObj->vnum = 0;
    pObj->item_type = ITEM_TRASH;
    pObj->extra_flags = 0;
    pObj->wear_flags = 0;
    pObj->count = 0;
    pObj->weight = 0;
    pObj->cost = 0;
    pObj->material = 0;         /* ROM */
    pObj->condition = 100;      /* ROM */
    for (value = 0; value < 5; value++)     /* 5 - ROM */
        pObj->value[value] = 0;

    pObj->new_format = TRUE;    /* ROM */

    return pObj;
}

void
free_obj_index(OBJ_INDEX_DATA * pObj)
{
    EXTRA_DESCR_DATA *pExtra;
    AFFECT_DATA *pAf;

    free_string(pObj->name);
    free_string(pObj->short_descr);
    free_string(pObj->description);

    for (pAf = pObj->affected; pAf; pAf = pAf->next)
    {
        free_affect(pAf);
    }

    for (pExtra = pObj->extra_descr; pExtra; pExtra = pExtra->next)
    {
        free_extra_descr(pExtra);
    }

    pObj->next = obj_index_free;
    obj_index_free = pObj;
    return;
}



MOB_INDEX_DATA *mob_index_free;

MOB_INDEX_DATA *
new_mob_index(void)
{
    MOB_INDEX_DATA *pMob;

    if (!mob_index_free)
    {
        pMob = (MOB_INDEX_DATA *) alloc_perm(sizeof(*pMob));
        top_mob_index++;
    }
    else
    {
        pMob = mob_index_free;
        mob_index_free = mob_index_free->next;
    }

    pMob->next = NULL;
    pMob->spec_fun = NULL;
    pMob->pShop = NULL;
    pMob->area = NULL;
    pMob->player_name = str_dup("no name");
    pMob->short_descr = str_dup("(no short description)");
    pMob->long_descr = str_dup("(no long description)\n\r");
    pMob->description = &str_empty[0];
    pMob->vnum = 0;
    pMob->count = 0;
    pMob->killed = 0;
    pMob->sex = 0;
    pMob->level = 0;
    pMob->act = ACT_IS_NPC;
    pMob->affected_by = 0;
    pMob->alignment = 0;
    pMob->hitroll = 0;
    pMob->race = race_lookup("human");  /* - Hugin */
    pMob->form = 0;             /* ROM patch -- Hugin */
    pMob->parts = 0;            /* ROM patch -- Hugin */
    pMob->imm_flags = 0;        /* ROM patch -- Hugin */
    pMob->res_flags = 0;        /* ROM patch -- Hugin */
    pMob->vuln_flags = 0;       /* ROM patch -- Hugin */
    pMob->material = 0;         /* -- Hugin */
    pMob->off_flags = 0;        /* ROM patch -- Hugin */
    pMob->size = SIZE_MEDIUM;   /* ROM patch -- Hugin */
    pMob->ac[AC_PIERCE] = 0;    /* ROM patch -- Hugin */
    pMob->ac[AC_BASH] = 0;      /* ROM patch -- Hugin */
    pMob->ac[AC_SLASH] = 0;     /* ROM patch -- Hugin */
    pMob->ac[AC_EXOTIC] = 0;    /* ROM patch -- Hugin */
    pMob->hit[DICE_NUMBER] = 0; /* ROM patch -- Hugin */
    pMob->hit[DICE_TYPE] = 0;   /* ROM patch -- Hugin */
    pMob->hit[DICE_BONUS] = 0;  /* ROM patch -- Hugin */
    pMob->mana[DICE_NUMBER] = 0;    /* ROM patch -- Hugin */
    pMob->mana[DICE_TYPE] = 0;  /* ROM patch -- Hugin */
    pMob->mana[DICE_BONUS] = 0; /* ROM patch -- Hugin */
    pMob->damage[DICE_NUMBER] = 0;  /* ROM patch -- Hugin */
    pMob->damage[DICE_TYPE] = 0;    /* ROM patch -- Hugin */
    pMob->damage[DICE_NUMBER] = 0;  /* ROM patch -- Hugin */
    pMob->start_pos = POS_STANDING;     /*  -- Hugin */
    pMob->default_pos = POS_STANDING;   /*  -- Hugin */
    pMob->wealth = 0;

    pMob->new_format = TRUE;    /* ROM */

    return pMob;
}

void
free_mob_index(MOB_INDEX_DATA * pMob)
{
    free_string(pMob->player_name);
    free_string(pMob->short_descr);
    free_string(pMob->long_descr);
    free_string(pMob->description);

    free_shop(pMob->pShop);

    pMob->next = mob_index_free;
    mob_index_free = pMob;
    return;
}



BOARD_DATA *board_free;

BOARD_DATA*
new_board(void) {
    BOARD_DATA *pBoard;

    if (!board_free) {
        pBoard = (BOARD_DATA *) alloc_perm(sizeof(*pBoard));
    } else {
        pBoard = board_free;
        board_free = board_free->next;
    }

    pBoard->next = NULL;
    pBoard->name = str_dup("");
    pBoard->description = str_dup("");
    pBoard->readLevel = 0;
    pBoard->writeLevel = 2;
    pBoard->names = str_dup("all");
    pBoard->nameRestriction = DEF_NORMAL;
    pBoard->defaultExpire = 21;
    pBoard->clan = NULL;
    pBoard->isDefault = FALSE;
    pBoard->enabled = FALSE;
    pBoard->filename = NULL;

    return pBoard;
}

void
free_board(BOARD_DATA * pBoard) {
    NOTE_DATA* note;
    NOTE_DATA* nextNote;

    free_string(pBoard->name);
    free_string(pBoard->description);
    free_string(pBoard->names);
    free_string(pBoard->clan);
    free_string(pBoard->filename);

    for (note = pBoard->note_first; note; note = nextNote) {
        nextNote = note->next;

        free_note(note);
    }

    pBoard->note_first = NULL;
    pBoard->enabled = FALSE;

    pBoard->next = board_free;
    board_free = pBoard;

    return;
}



NOTE_READ_DATA *note_read_free;

NOTE_READ_DATA*
new_note_read(void) {
    NOTE_READ_DATA *pNoteRead;

    if (!note_read_free) {
        pNoteRead = (NOTE_READ_DATA *) alloc_perm(sizeof(*pNoteRead));
    } else {
        pNoteRead = note_read_free;
        note_read_free = note_read_free->next;
    }

    pNoteRead->next = NULL;
    pNoteRead->boardName = str_dup("");
    pNoteRead->timestamp = 0;

    return pNoteRead;
}

void
free_note_read(NOTE_READ_DATA *pNoteRead) {
    free_string(pNoteRead->boardName);

    pNoteRead->next = note_read_free;
    note_read_free = pNoteRead;

    return;
}


/*
MPROG_CODE *mprog_code_free;

MPROG_CODE *
new_mprog_code(void) {
    MPROG_CODE *pMProgCode;

    if (!mprog_code_free) {
        pMProgCode = (MPROG_CODE *) alloc_perm(sizeof(*pMProgCode));
    } else {
        pMProgCode = mprog_code_free;
        mprog_code_free = mprog_code_free->next;
    }

    pMProgCode->next = NULL;
    pMProgCode->approved = FALSE;
    pMProgCode->code = NULL;
    pMProgCode->vnum = 0;

    return pMProgCode;
}

void
free_mprog_code(MPROG_CODE *pMProgCode) {
    free_string(pMProgCode->code);

    pMProgCode->next = mprog_code_free;
    mprog_code_free = pMProgCode;

    return;
}



MPROG_LIST *mprog_list_free;

MPROG_LIST *
new_mprog_list(void) {
    MPROG_LIST *pMProgList;

    if (!mprog_list_free) {
        pMProgList = (MPROG_LIST *) alloc_perm(sizeof(*pMProgList));
    } else {
        pMProgList = mprog_list_free;
        mprog_list_free = pMProgList->next;
    }

    pMProgList->next = NULL;
    pMProgList->trig_type = 0;
    pMProgList->trig_phrase = NULL;
    pMProgList->vnum = 0;
    pMProgList->program = 0;

    return pMProgList;
}

void
free_mprog_list(MPROG_LIST *pMProgList) {
    free_string(pMProgList->trig_phrase);

    pMProgList->next = mprog_list_free;
    mprog_list_free = pMProgList;

    return;
}*/
