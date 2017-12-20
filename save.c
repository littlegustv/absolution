/* revision 1.1 - August 1 1999 - making it compilable under g++ */
/**************************************************************************
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <malloc.h>

#include "merc.h"
#include "recycle.h"
#include "clan.h"
#include "utils.h"
#include "board.h"



extern int is_quest_craft(OBJ_DATA *);
extern void update_version_weapon(CHAR_DATA *, OBJ_DATA *);
int rename(const char *oldfname, const char *newfname);

char *
print_flags(int flag)
{
	int count, pos = 0;
	static char buf[52];


	for (count = 0; count < 32; count++)
	{
		if (IS_SET(flag, 1 << count))
		{
			if (count < 26)
				buf[pos] = 'A' + count;
			else
				buf[pos] = 'a' + (count - 26);
			pos++;
		}
	}

	if (pos == 0)
	{
		buf[pos] = '0';
		pos++;
	}

	buf[pos] = '\0';

	return buf;
}


/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST        100
static OBJ_DATA *rgObjNest[MAX_NEST];



/*
 * Local functions.
 */
void fwrite_char(CHAR_DATA * ch, FILE * fp);
void fwrite_obj(CHAR_DATA * ch, OBJ_DATA * obj, FILE * fp, int iNest, int drop_items);
void fwrite_pet(CHAR_DATA * pet, FILE * fp);
void fread_char(CHAR_DATA * ch, FILE * fp);
void fread_pet(CHAR_DATA * ch, FILE * fp);
void fread_obj(CHAR_DATA * ch, FILE * fp);

void
save_count(int count)
{
	FILE *fp;

	fclose(fpReserve);

	if ((fp = fopen(TEMP_FILE, "w")) == NULL)
	{
		bug("Save_count: fopen", 0);
		return;
	}

	fprintf(fp, "%d\n", count);
	fclose(fp);
	rename(TEMP_FILE, COUNT_FILE);
	fpReserve = fopen(NULL_FILE, "r");
	return;
}


int
load_count(void)
{
	FILE *fp;
	int count;

	if ((fp = fopen(COUNT_FILE, "r")) == NULL)
	{
		bug("Error reading count file.'", 0);
		return 0;
	}

	count = fread_number(fp);
	fclose(fp);
	return count;
}


/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void
save_char_obj(CHAR_DATA * ch, int drop_items)
{
	char strsave[MAX_INPUT_LENGTH];
	FILE *fp;

	if (IS_NPC(ch))
		return;

	if (ch->desc != NULL && ch->desc->original != NULL)
		ch = ch->desc->original;

	/* create god log */
	if (IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL)
	{
		fclose(fpReserve);
		sprintf(strsave, "%s%s", GOD_DIR, capitalize(ch->name));
		if ((fp = fopen(strsave, "w")) == NULL)
		{
			bug("Save_char_obj: fopen", 0);
			perror(strsave);
		}

		fprintf(fp, "Lev %2d Trust %2d  %s%s\n",
				ch->level, get_trust(ch), ch->name, ch->pcdata->title);
		fclose(fp);
		fpReserve = fopen(NULL_FILE, "r");
	}

	fclose(fpReserve);
	sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(ch->name));
	if ((fp = fopen(TEMP_FILE, "w")) == NULL)
	{
		bug("Save_char_obj: fopen", 0);
		perror(strsave);
	}
	else
	{
		fwrite_char(ch, fp);
		if (ch->carrying != NULL)
			fwrite_obj(ch, ch->carrying, fp, 0, drop_items);
		/* save the pets */
		if (ch->pet != NULL && ch->pet->in_room == ch->in_room)
			fwrite_pet(ch->pet, fp);
		fprintf(fp, "#END\n");
	}
	fclose(fp);
	rename(TEMP_FILE, strsave);
	fpReserve = fopen(NULL_FILE, "r");
	return;
}



/*
 * Write the char.
 */
void
fwrite_char(CHAR_DATA * ch, FILE * fp) {
    AFFECT_DATA *paf;
    int sn, gn, pos;
    int i, currentRoom;

    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)) {
        for (i = 0; i < MAX_CLAN; i++) {
            if (clan_table[i].independent) {
                continue;
            }

            REMOVE_BIT(ch->wiznet, clan_table[i].join_constant);
        }
    }

    fprintf(fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER");

    fprintf(fp, "Name %s~\n", ch->name);
    fprintf(fp, "Id   %ld\n", ch->id);
    fprintf(fp, "LogO %ld\n", current_time);
    fprintf(fp, "Vers %d\n", ch->version);

    if (ch->short_descr[0] != '\0') {
        fprintf(fp, "ShD  %s~\n", ch->short_descr);
    }

    if (ch->long_descr[0] != '\0') {
        fprintf(fp, "LnD  %s~\n", ch->long_descr);
    }

    if (ch->shift_name != NULL) {
        fprintf(fp, "SNE  %s~\n", ch->shift_name);
    }

    if (ch->shift_short != NULL) {
        fprintf(fp, "SST  %s~\n", ch->shift_short);
    }

    if (ch->shift_long != NULL) {
        fprintf(fp, "SLG  %s~\n", ch->shift_long);
    }

    if (ch->description[0] != '\0') {
        fprintf(fp, "Desc %s~\n", ch->description);
    }

    if (ch->prompt != NULL || !str_cmp(ch->prompt, "<%hhp %mm %vmv> ")) {
        fprintf(fp, "Prom %s~\n", ch->prompt);
    }

    fprintf(fp, "Race %s~\n", pc_race_table[ch->race].name);

    if (ch->size != pc_race_table[ch->race].size) {
        fprintf(fp, "Size %d\n", ch->size);
    }

    if (ch->clan) {
        fprintf(fp, "Clan %s~\n", clan_table[ch->clan].name);
    }

    if (!clan_table[ch->clan].independent && ch->clan_rank) {
        fprintf(fp, "CRank %s~\n", ch->clan_rank);
    }

    if (ch->rptitle) {
        fprintf(fp, "RPTitle %s~\n", ch->rptitle);
    }

    if (ch->site != NULL) {
        fprintf(fp, "Cre  %s~\n", ch->site);
    }

    fprintf(fp, "Sex  %d\n", ch->sex);
    fprintf(fp, "Cla  %d\n", ch->charClass);
    fprintf(fp, "Levl %d\n", ch->level);
    fprintf(fp, "Lay  %d\n", ch->can_lay);

    if (ch->pcdata->spouse > 0) {
        fprintf(fp, "Spou %s~\n", ch->pcdata->spouse);
    }

    fprintf(fp, "Brth %d\n", ch->breath);

    if (ch->questpoints != 0) {
        fprintf(fp, "QuestPnts %d\n", ch->questpoints);
    }

    if (ch->bounty > 0) {
        fprintf(fp, "Bounty %d\n", ch->bounty);
    }

    if(!IS_QUESTING(ch) && ch->pcdata->quest.timer != 0) {
        fprintf(fp, "QuestNext %d\n", ch->pcdata->quest.timer);
    }

    if (ch->trust != 0) {
        fprintf(fp, "Tru  %d\n", ch->trust);
    }

    fprintf(fp, "Sec  %d\n", ch->pcdata->security);
    fprintf(fp, "Plyd %d\n", ch->played_perm);

    /* Save note board status */
    if (ch->pcdata->noteReadData) {
        NOTE_READ_DATA *pNoteRead;

        /* Save number of boards in case that number changes */
        i = 0;
        for (pNoteRead = ch->pcdata->noteReadData; pNoteRead; pNoteRead = pNoteRead->next) {
            i++;
        }

        fprintf(fp, "Boards       %d ", i);

        for (pNoteRead = ch->pcdata->noteReadData; pNoteRead; pNoteRead = pNoteRead->next) {
            fprintf(fp, "%s %ld ", pNoteRead->boardName, pNoteRead->timestamp);
        }

        fprintf(fp, "\n");
    }

    fprintf(fp, "Scro %d\n", ch->lines);

    if (ch->in_room == get_room_index(ROOM_VNUM_LIMBO) || ch->in_room == get_room_index(ROOM_VNUM_LIMBO_DOMINIA)) {
        if (ch->was_in_room != NULL) {
            currentRoom = ch->was_in_room->vnum;
        } else {
            currentRoom = ch->in_room->vnum;
        }
    } else {
        if (ch->in_room) {
            currentRoom = ch->in_room->vnum;
        } else {
            currentRoom = 3001;
        }
    }
/* original line
    fprintf(fp, "Room %d\n",
            ((ch->in_room == get_room_index(ROOM_VNUM_LIMBO)
                    || ch->in_room == get_room_index(ROOM_VNUM_LIMBO_DOMINIA))
                    && ch->was_in_room != NULL)
                    ? ch->was_in_room->vnum
                            : ch->in_room == NULL ? 3001 : ch->in_room->vnum);
*/
    fprintf(fp, "Room %d\n", currentRoom);


    fprintf(fp, "HMV  %d %d %d %d %d %d\n", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move);
    if (ch->gold > 0) {
        fprintf(fp, "Gold %ld\n", ch->gold);
    } else {
        fprintf(fp, "Gold %d\n", 0);
    }

    if (ch->silver > 0) {
        fprintf(fp, "Silv %ld\n", ch->silver);
    } else {
        fprintf(fp, "Silv %d\n", 0);
    }

    fprintf(fp, "Exp  %d\n", ch->exp);

    if (ch->deity_points > 0) {
        fprintf(fp, "DeiT %d\n", ch->deity_type);
        fprintf(fp, "DeiP %d\n", ch->deity_points);
    }

    if (ch->act != 0) {
        fprintf(fp, "Act  %s\n", print_flags(ch->act));
    }

    if (ch->affected_by != 0) {
        fprintf(fp, "AfBy %s\n", print_flags(ch->affected_by));
    }

    fprintf(fp, "Comm %s\n", print_flags(ch->comm));

    if (ch->wiznet) {
        fprintf(fp, "Wizn %s\n", print_flags(ch->wiznet));
    }

    if (ch->toggles) {
        fprintf(fp, "Togls %s\n", print_flags(ch->toggles));
    }

    if (ch->invis_level) {
        fprintf(fp, "Invi %d\n", ch->invis_level);
    }

    if (ch->incog_level) {
        fprintf(fp, "Inco %d\n", ch->incog_level);
    }

    fprintf(fp, "Pos  %d\n", ch->position == POS_FIGHTING ? POS_STANDING : ch->position);

    if (ch->level > 54) {
        fprintf(fp, "Gift %d\n", ch->gift);
    }

    if (ch->delegate > 0) {
        fprintf(fp, "Deleg %d\n", ch->delegate);
    }

    if (ch->remort != 0) {
        fprintf(fp, "Remort %d\n", ch->remort);
    }

    if (ch->rem_sub != 0) {
        fprintf(fp, "Remsub %d\n", ch->rem_sub);
    }

    if (ch->reclass != 0) {
        fprintf(fp, "Reclas %d\n", ch->reclass);
    }

    if (ch->air_supply) {
        fprintf(fp, "Air %d\n", ch->air_supply);
    }

    if (ch->craft_timer > 0) {
        fprintf(fp, "CraftTimer %d\n", ch->craft_timer);
    }

    if (ch->pass_along != 0) {
        fprintf(fp, "PasAl %d\n", ch->pass_along);
    }

    if (ch->pass_along_limit != 0) {
        fprintf(fp, "PasLimit %d\n", ch->pass_along_limit);
    }

    if (ch->patron != NULL) {
        fprintf( fp, "Pat %s~\n", ch->patron);
    }

    if (ch->vassal != NULL) {
        fprintf( fp, "Vas %s~\n", ch->vassal);
    }

    if (ch->to_pass > 0) {
        fprintf( fp, "TPas %d~\n", ch->to_pass);
    }

    if (ch->pkills != 0) {
        fprintf(fp, "Pkil %d\n", ch->pkills);
    }

    if (ch->deaths != 0) {
        fprintf(fp, "Pkds %d\n", ch->deaths);
    }

    if (ch->practice != 0) {
        fprintf(fp, "Prac %d\n", ch->practice);
    }

    if (ch->train != 0) {
        fprintf(fp, "Trai %d\n", ch->train);
    }

    if (ch->saving_throw != 0) {
        fprintf(fp, "Save  %d\n", ch->saving_throw);
    }

    fprintf(fp, "Alig  %d\n", ch->alignment);

    if (ch->hitroll != 0) {
        fprintf(fp, "Hit   %d\n", ch->hitroll);
    }

    if (ch->damroll != 0) {
        fprintf(fp, "Dam   %d\n", ch->damroll);
    }

    fprintf(fp, "ACs %d %d %d %d\n", ch->armor[0], ch->armor[1], ch->armor[2], ch->armor[3]);

    if (ch->wimpy != 0) {
        fprintf(fp, "Wimp  %d\n", ch->wimpy);
    }

    fprintf(fp, "Attr %d %d %d %d %d\n", ch->perm_stat[STAT_STR], ch->perm_stat[STAT_INT], ch->perm_stat[STAT_WIS], ch->perm_stat[STAT_DEX], ch->perm_stat[STAT_CON]);

    fprintf(fp, "AMod %d %d %d %d %d\n", ch->mod_stat[STAT_STR], ch->mod_stat[STAT_INT], ch->mod_stat[STAT_WIS], ch->mod_stat[STAT_DEX], ch->mod_stat[STAT_CON]);

    if (IS_NPC(ch)) {
        fprintf(fp, "Vnum %d\n", ch->pIndexData->vnum);
    } else {
        fprintf(fp, "Pass %s~\n", ch->pcdata->pwd);
        
        if (ch->pcdata->bamfin[0] != '\0') {
            fprintf(fp, "Bin  %s~\n", ch->pcdata->bamfin);
        }

        if (ch->pcdata->bamfout[0] != '\0') {
            fprintf(fp, "Bout %s~\n", ch->pcdata->bamfout);
        }

        fprintf(fp, "Titl %s~\n", ch->pcdata->title);
        fprintf(fp, "Pnts %d\n", ch->pcdata->points);
        fprintf(fp, "TSex %d\n", ch->pcdata->true_sex);
        fprintf(fp, "LLev %d\n", ch->pcdata->last_level);
        fprintf(fp, "HMVP %d %d %d\n", ch->max_hit, ch->max_mana, ch->max_move);
        fprintf(fp, "Cnd  %d %d %d %d\n", ch->pcdata->condition[0], ch->pcdata->condition[1], ch->pcdata->condition[2], ch->pcdata->condition[3]);

        if (ch->sliver > 0) {
            fprintf(fp, "Sliv %d\n", ch->sliver);
        }

        if (ch->outcast_timer > 0) {
            fprintf(fp, "Outc %d\n", ch->outcast_timer);
        }

        /* write alias */
        for (pos = 0; pos < MAX_ALIAS; pos++) {
            if (ch->pcdata->alias[pos] == NULL || ch->pcdata->alias_sub[pos] == NULL)
                break;

            fprintf(fp, "Alias %s %s~\n", ch->pcdata->alias[pos], ch->pcdata->alias_sub[pos]);
        }

        /* write macros */
        for (pos = 0; pos < MAX_NUM_MACROS; pos++) {
            if (ch->pcdata->macro[pos].name != NULL) {
                fprintf(fp, "Macro %s %s~\n", ch->pcdata->macro[pos].name, ch->pcdata->macro[pos].definition);
            }
        }

        if (ch->meditate_needed) {
            fprintf(fp, "MedNeed %d\n", ch->meditate_needed);
        }

        fprintf(fp, "Specialty '%s'\n", skill_table[ch->pcdata->specialty].name);

        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0) {
                fprintf(fp, "Sk %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn].name);
            }
        }

        for (gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name != NULL && ch->pcdata->group_known[gn]) {
                fprintf(fp, "Gr '%s'\n", group_table[gn].name);
            }
        }
    }

    for (paf = ch->affected; paf != NULL; paf = paf->next) {
        if (paf->type < 0 || paf->type >= MAX_SKILL) {
            continue;
        }

        if (paf->type == gsn_nature_protection
                || paf->type == gsn_living_stone
                || paf->type == gsn_alarm_rune
                || paf->type == gsn_wizards_eye
                || paf->type == gsn_animal_skins
                || paf->type == gsn_aura
                || paf->type == gsn_jail
                || paf->type == gsn_stance_shadow) {
            continue;
        }

        fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10ld\n",
                skill_table[paf->type].name,
                paf->where,
                paf->level,
                paf->duration,
                paf->modifier,
                paf->location,
                paf->bitvector
        );
    }

    fprintf(fp, "End\n\n");
    return;
}

/* write a pet */
void
fwrite_pet(CHAR_DATA * pet, FILE * fp)
{
	AFFECT_DATA *paf;

	fprintf(fp, "#PET\n");

	fprintf(fp, "Vnum %d\n", pet->pIndexData->vnum);

	fprintf(fp, "Name %s~\n", pet->name);
	fprintf(fp, "LogO %ld\n", current_time);
	if (pet->short_descr != pet->pIndexData->short_descr)
		fprintf(fp, "ShD  %s~\n", pet->short_descr);
	if (pet->long_descr != pet->pIndexData->long_descr)
		fprintf(fp, "LnD  %s~\n", pet->long_descr);
	if (pet->description != pet->pIndexData->description)
		fprintf(fp, "Desc %s~\n", pet->description);
	if (pet->race != pet->pIndexData->race)
		fprintf(fp, "Race %s~\n", race_table[pet->race].name);
	if (pet->clan)
		fprintf(fp, "Clan %s~\n", clan_table[pet->clan].name);
	fprintf(fp, "Sex  %d\n", pet->sex);
	if (pet->level != pet->pIndexData->level)
		fprintf(fp, "Levl %d\n", pet->level);
	fprintf(fp, "HMV  %d %d %d %d %d %d\n",
			pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move, pet->max_move);
	if (pet->gold > 0)
		fprintf(fp, "Gold %ld\n", pet->gold);
	if (pet->silver > 0)
		fprintf(fp, "Silv %ld\n", pet->silver);
	if (pet->exp > 0)
		fprintf(fp, "Exp  %d\n", pet->exp);
	if (pet->act != pet->pIndexData->act)
		fprintf(fp, "Act  %s\n", print_flags(pet->act));
	if (pet->affected_by != pet->pIndexData->affected_by)
		fprintf(fp, "AfBy %s\n", print_flags(pet->affected_by));
	if (pet->comm != 0)
		fprintf(fp, "Comm %s\n", print_flags(pet->comm));
	fprintf(fp, "Pos  %d\n", pet->position = POS_FIGHTING ? POS_STANDING : pet->position);
	if (pet->saving_throw != 0)
		fprintf(fp, "Save %d\n", pet->saving_throw);
	if (pet->alignment != pet->pIndexData->alignment)
		fprintf(fp, "Alig %d\n", pet->alignment);
	if (pet->hitroll != pet->pIndexData->hitroll)
		fprintf(fp, "Hit  %d\n", pet->hitroll);
	if (pet->damroll != pet->pIndexData->damage[DICE_BONUS])
		fprintf(fp, "Dam  %d\n", pet->damroll);
	fprintf(fp, "ACs  %d %d %d %d\n",
			pet->armor[0], pet->armor[1], pet->armor[2], pet->armor[3]);
	fprintf(fp, "Attr %d %d %d %d %d\n",
			pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
			pet->perm_stat[STAT_WIS], pet->perm_stat[STAT_DEX],
			pet->perm_stat[STAT_CON]);
	fprintf(fp, "AMod %d %d %d %d %d\n",
			pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
			pet->mod_stat[STAT_WIS], pet->mod_stat[STAT_DEX],
			pet->mod_stat[STAT_CON]);

	for (paf = pet->affected; paf != NULL; paf = paf->next)
	{
		if (paf->type < 0 || paf->type >= MAX_SKILL)
			continue;

		fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10ld\n",
				skill_table[paf->type].name,
		paf->where, paf->level, paf->duration, paf->modifier, paf->location,
				paf->bitvector);
	}

	fprintf(fp, "End\n");
	return;
}

/*
 * Write an object and its contents.
 */
void
fwrite_obj(CHAR_DATA * ch, OBJ_DATA * obj, FILE * fp, int iNest, int drop_items)
{
	EXTRA_DESCR_DATA *ed;
	AFFECT_DATA *paf;

	/*
	 * Slick recursion to write lists backwards,
	 *   so loading them will load in forwards order.
	 */
	if (obj->next_content != NULL)
		fwrite_obj(ch, obj->next_content, fp, iNest, drop_items);

	/*
	 * Castrate storage characters.
	 */
	if (drop_items
	&& obj->level > ch->level + 2
	&& ch->in_room != NULL) {
		Cprintf(ch, "Your high level gear falls to the ground as you vanish.\n\r");
		if(iNest > 0)
		{
			obj_from_obj(obj);
		}
		else
			obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
		return;
	}

	if (drop_items
	&& !IS_IMMORTAL(ch)
	&& ch->in_room
	&& ((obj->clan_status == CS_CLANNER && !clan_table[ch->clan].pkiller)
	|| (obj->clan_status == CS_NONCLANNER && clan_table[ch->clan].pkiller))) {
		Cprintf(ch, "Your illegal quest/craft item falls to the ground as you vanish.\n\r");
		if(iNest > 0)
			obj_from_obj(obj);
		else
			obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
		return;
	}

	if (obj->item_type == ITEM_KEY)
		return;

	fprintf(fp, "#O\n");
	fprintf(fp, "Vnum %d\n", obj->pIndexData->vnum);
	if (!obj->pIndexData->new_format)
		fprintf(fp, "Oldstyle\n");
	if (obj->enchanted)
		fprintf(fp, "Enchanted\n");
	fprintf(fp, "Nest %d\n", iNest);
	if (obj->seek != NULL)
		fprintf(fp, "Seek %s~\n", obj->seek);
	if (obj->owner != NULL)
                fprintf(fp, "Owner %s~\n", obj->owner);
        if (obj->owner_vnum != 0)
                fprintf(fp, "Onum %d\n", obj->owner_vnum);
	if (obj->respawn_owner != NULL) {
		fprintf(fp, "Respawn_Owner %s~\n", obj->respawn_owner);
	}
	// Fix unflagged clan status items
	if (is_quest_craft(obj)
	&& obj->clan_status == CS_UNSPECIFIED) {
		if(clan_table[ch->clan].pkiller)
			obj->clan_status = CS_CLANNER;
		else
			obj->clan_status = CS_NONCLANNER;
	}

	/* these data are only used if they do not match the defaults */

	if (obj->name != obj->pIndexData->name)
		fprintf(fp, "Name %s~\n", obj->name);
	if (obj->short_descr != obj->pIndexData->short_descr)
		fprintf(fp, "ShD  %s~\n", obj->short_descr);
	if (obj->description != obj->pIndexData->description)
		fprintf(fp, "Desc %s~\n", obj->description);
	if (obj->extra_flags != obj->pIndexData->extra_flags)
		fprintf(fp, "ExtF %d\n", obj->extra_flags);
	if (obj->wear_flags != obj->pIndexData->wear_flags)
		fprintf(fp, "WeaF %d\n", obj->wear_flags);
	if (obj->item_type != obj->pIndexData->item_type)
		fprintf(fp, "Ityp %d\n", obj->item_type);
	if (obj->weight != obj->pIndexData->weight)
		fprintf(fp, "Wt   %d\n", obj->weight);
	if (obj->condition != obj->pIndexData->condition)
		fprintf(fp, "Cond %d\n", obj->condition);
	if(obj->clan_status)
		fprintf(fp, "ClanType %d\n", obj->clan_status);

	/* variable data */

	fprintf(fp, "Wear %d\n", obj->wear_loc);
	if (obj->level != obj->pIndexData->level)
		fprintf(fp, "Lev  %d\n", obj->level);
	if (obj->timer != 0)
		fprintf(fp, "Time %d\n", obj->timer);
	fprintf(fp, "Cost %d\n", obj->cost);
	if (obj->value[0] != obj->pIndexData->value[0]
		|| obj->value[1] != obj->pIndexData->value[1]
		|| obj->value[2] != obj->pIndexData->value[2]
		|| obj->value[3] != obj->pIndexData->value[3]
		|| obj->value[4] != obj->pIndexData->value[4])
		fprintf(fp, "Val  %d %d %d %d %d\n",
				obj->value[0], obj->value[1], obj->value[2], obj->value[3],
				obj->value[4]);

	if (obj->special[0] != obj->pIndexData->special[0]
                || obj->special[1] != obj->pIndexData->special[1]
                || obj->special[2] != obj->pIndexData->special[2]
                || obj->special[3] != obj->pIndexData->special[3]
                || obj->special[4] != obj->pIndexData->special[4])
	                fprintf(fp, "SValue  %d %d %d %d %d\n",
                                obj->special[0], obj->special[1],
				obj->special[2], obj->special[3],
                                obj->special[4]);


	fprintf(fp, "EValue %d %d %d %d %d\n", obj->extra[0],
		obj->extra[1], obj->extra[2], obj->extra[3], obj->extra[4]);

	switch (obj->item_type)
	{
	case ITEM_POTION:
	case ITEM_SCROLL:
	case ITEM_PILL:
		if (obj->value[1] > 0)
		{
			fprintf(fp, "Spell 1 '%s'\n",
					skill_table[obj->value[1]].name);
		}

		if (obj->value[2] > 0)
		{
			fprintf(fp, "Spell 2 '%s'\n",
					skill_table[obj->value[2]].name);
		}

		if (obj->value[3] > 0)
		{
			fprintf(fp, "Spell 3 '%s'\n",
					skill_table[obj->value[3]].name);
		}

		break;

	case ITEM_STAFF:
	case ITEM_WAND:
		if (obj->value[3] > 0)
		{
			fprintf(fp, "Spell 3 '%s'\n",
					skill_table[obj->value[3]].name);
		}

		break;
	case ITEM_THROWING:
	case ITEM_AMMO:
		if (obj->value[4] > 0)
			fprintf(fp, "Spell 4 '%s'\n",
				skill_table[obj->value[4]].name);
	}

	if(IS_SET(obj->wear_flags, ITEM_CHARGED)) {
		fprintf(fp, "Charge 3 '%s'\n",
			skill_table[obj->special[3]].name);
	}

	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
		if (paf->type < 0 || paf->type >= MAX_SKILL)
			continue;

		if(paf->extra == 0) {
			fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10ld\n",
				skill_table[paf->type].name,
				paf->where,
				paf->level,
				paf->duration,
				paf->modifier,
				paf->location,
				paf->bitvector
			);
		}
		if(paf->extra != 0) {
			fprintf(fp, "AffExtra '%s' %3d %3d %3d %3d %3d %10ld %4d\n",
				skill_table[paf->type].name,
				paf->where,
				paf->level,
				paf->duration,
				paf->modifier,
				paf->location,
				paf->bitvector,
				paf->extra
			);
		}
	}


	for (ed = obj->extra_descr; ed != NULL; ed = ed->next)
	{
		fprintf(fp, "ExDe %s~ %s~\n",
				ed->keyword, ed->description);
	}

	fprintf(fp, "End\n\n");

	if (obj->contains != NULL)
		fwrite_obj(ch, obj->contains, fp, iNest + 1, drop_items);

	return;
}



/*
 * Load a char and inventory into a new ch structure.
 */
bool
load_char_obj(DESCRIPTOR_DATA * d, char *name) {
    char strsave[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *ch;
    FILE *fp;
    bool found;
    int stat;

    ch = new_char();
    ch->pcdata = new_pcdata();

    d->character = ch;
    ch->desc = d;
    ch->name = str_dup(name);
    ch->id = get_pc_id();
    ch->race = race_lookup("human");
    ch->act = PLR_NOSUMMON;
    ch->comm = COMM_COMBINE | COMM_PROMPT;
    ch->prompt = str_dup("{c<%hhp %mm %vmv>{x ");
    ch->pcdata->confirm_delete = FALSE;
    ch->pcdata->pwd = str_dup("");
    ch->pcdata->bamfin = str_dup("");
    ch->pcdata->bamfout = str_dup("");
    ch->pcdata->title = str_dup("");
    ch->pcdata->board = getDefaultBoard();

    for (stat = 0; stat < MAX_STATS; stat++) {
        ch->perm_stat[stat] = 13;
    }

    ch->pcdata->condition[COND_THIRST] = 48;
    ch->pcdata->condition[COND_FULL] = 48;
    ch->pcdata->condition[COND_HUNGER] = 48;
    ch->pcdata->security = 0;	/* OLC */
    ch->clan_rank = NULL;

    found = FALSE;
    fclose(fpReserve);

    /* decompress if .gz file exists */
    sprintf(strsave, "%s%s%s", PLAYER_DIR, capitalize(name), ".gz");
    if ((fp = fopen(strsave, "r")) != NULL) {
        fclose(fp);
        sprintf(buf, "gzip -dfq %s", strsave);
        system(buf);
    }

    sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(name));
    if ((fp = fopen(strsave, "r")) != NULL) {
        int iNest;

        for (iNest = 0; iNest < MAX_NEST; iNest++) {
            rgObjNest[iNest] = NULL;
        }

        found = TRUE;
        for (;;) {
            char letter;
            char *word;

            letter = fread_letter(fp);
            if (letter == '*') {
                fread_to_eol(fp);
                continue;
            }

            if (letter != '#') {
                bug("Load_char_obj: # not found.", 0);
                break;
            }

            word = fread_word(fp);
            if (!str_cmp(word, "PLAYER")) {
                fread_char(ch, fp);
                ch->played = ch->played_perm;

                // Special remort flags, should be elsewhere
                if (ch->race == race_lookup("dwarf") && ch->remort > 0) {
                    SET_BIT(ch->affected_by, AFF_DARK_VISION);
                }

                if (ch->race == race_lookup("elf")
                        && ch->remort
                        && ch->rem_sub == 2) {
                    SET_BIT(ch->affected_by, AFF_DARK_VISION);
                }

                if (ch->race == race_lookup("kirre")
                        && ch->remort
                        && ch->rem_sub == 2) {
                    SET_BIT(ch->affected_by, AFF_DARK_VISION);
                }

                if (ch->race == race_lookup("human")
                        && ch->remort) {
                    SET_BIT(ch->affected_by, AFF_WATERWALK);
                }

                // In case they were writing a note and lost link
                REMOVE_BIT(ch->comm, COMM_NOTE_WRITE);
            } else if (!str_cmp(word, "OBJECT")) {
                fread_obj(ch, fp);
            } else if (!str_cmp(word, "O")) {
                fread_obj(ch, fp);
            } else if (!str_cmp(word, "PET")) {
                fread_pet(ch, fp);
            } else if (!str_cmp(word, "END")) {
                break;
            } else {
                bug("Load_char_obj: bad section.", 0);
                break;
            }
        }

        fclose(fp);
    }

	fpReserve = fopen(NULL_FILE, "r");


	/* initialize race */
	if (found)
	{
		int i;

		if (ch->race == 0)
			ch->race = race_lookup("human");

		ch->size = pc_race_table[ch->race].size;
		ch->dam_type = 17;		/*punch */

		for (i = 0; i < 5; i++)
		{
			if (pc_race_table[ch->race].skills[i] == NULL)
				break;
			group_add(ch, pc_race_table[ch->race].skills[i], FALSE);
		}
		ch->affected_by = ch->affected_by | race_table[ch->race].aff;
		ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
		ch->res_flags = ch->res_flags | race_table[ch->race].res;
		ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;
		ch->form = race_table[ch->race].form;
		ch->parts = race_table[ch->race].parts;
	}


	/* RT initialize skills */

	if (found && ch->version < 2)	/* need to add the new skills */
	{
		group_add(ch, "rom basics", FALSE);
		group_add(ch, class_table[ch->charClass].base_group, FALSE);
		group_add(ch, class_table[ch->charClass].default_group, TRUE);
		ch->pcdata->learned[gsn_recall] = 50;
	}

	/* fix levels */
	if (found && ch->version < 3 && (ch->level > 35 || ch->trust > 35))
	{
		switch (ch->level)
		{
		case (40):
			ch->level = 60;
			break;				/* imp -> imp */
		case (39):
			ch->level = 58;
			break;				/* god -> supreme */
		case (38):
			ch->level = 56;
			break;				/* deity -> god */
		case (37):
			ch->level = 53;
			break;				/* angel -> demigod */
		}

		switch (ch->trust)
		{
		case (40):
			ch->trust = 60;
			break;				/* imp -> imp */
		case (39):
			ch->trust = 58;
			break;				/* god -> supreme */
		case (38):
			ch->trust = 56;
			break;				/* deity -> god */
		case (37):
			ch->trust = 53;
			break;				/* angel -> demigod */
		case (36):
			ch->trust = 51;
			break;				/* hero -> hero */
		}
	}

	/* ream gold */
	if (found && ch->version < 4)
	{
		ch->gold /= 100;
	}

	// New character version, skills go up based on level!
	// V6
	if (found && ch->version < 6)
	{
		int sn = 1;
		char buf[255];

		// Update to new version and raise skills
		sprintf(buf, "%s character version updated from %d to %d", ch->name, ch->version, 6);
		log_string("%s", buf);
		ch->version = 6;

		// All skills known raise 1% per level you have known them.
		while(1)
		{
        		if(sn == gsn_last_skill)
                		break;

        		if(ch->level < skill_table[sn].skill_level[ch->charClass]
        		|| ch->pcdata->learned[sn] == 0) {
                		sn++;
                		continue;
        		}

        		if(ch->pcdata->learned[sn] < 75) {
                		ch->pcdata->learned[sn] += (ch->level - skill_table[sn].skill_level[ch->charClass]);
				ch->pcdata->learned[sn] = UMIN(75, ch->pcdata->learned[sn]);
			}

        		sn++;
		}
	}


	if (found && ch->version < 7)
	{
        	char buf[255];

        	// Update to new version and raise skills
        	sprintf(buf, "%s character version updated from %d to %d",
                	ch->name, ch->version, 7);
        	log_string(buf);
        	ch->version = 7;

		save_char_obj(ch, FALSE);
	}


	// Fix oldschool assassins
	// Lose steal, peek and dual wield, but add death blow.
	if(ch->reclass == reclass_lookup("assassin")
	&& get_skill(ch, gsn_dual_wield) > 0
	&& get_skill(ch, gsn_death_blow) < 1) {
		ch->pcdata->learned[gsn_steal] = 0;
		ch->pcdata->learned[gsn_peek] = 0;
		ch->pcdata->learned[gsn_death_blow] = ch->pcdata->learned[gsn_dual_wield];
		ch->pcdata->learned[gsn_dual_wield] = 0;
	}

	// Fix Broken assassins
	if(ch->reclass == reclass_lookup("assassin")
	&& get_skill(ch, gsn_dual_wield) < 1
	&& get_skill(ch, gsn_death_blow) < 1)
	{
		ch->pcdata->learned[gsn_death_blow] = UMAX(ch->level - 35, 1);
		ch->pcdata->learned[gsn_steal] = 0;
		ch->pcdata->learned[gsn_peek] = 0;
	}

	// Fix broken rogues
	if(ch->reclass == reclass_lookup("rogue"))
		ch->pcdata->learned[gsn_cheat] = 0;

	// Fix broken paladins (no berserk)
	if(ch->charClass == class_lookup("paladin")
	&& ch->race != race_lookup("dwarf")) {
		ch->pcdata->learned[gsn_berserk] = 0;
	}

	// Remove sunray from templars
	if(ch->charClass == class_lookup("paladin")
	&& ch->reclass == reclass_lookup("templar")
	&& ch->pcdata->learned[gsn_sunray] > 0) {
		stat = ch->pcdata->learned[gsn_sunray];
		ch->pcdata->learned[gsn_sunray] = 0;
		ch->pcdata->learned[gsn_purify] = stat;
		ch->pcdata->learned[gsn_hallow] = stat;
	}

	return found;
}



/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                                        \
                                if ( !str_cmp( word, literal ) )        \
                                {                                        \
                                    field  = value;                        \
                                    fMatch = TRUE;                        \
                                    break;                                \
                                }

/* provided to free strings */
#if defined(KEYS)
#undef KEYS
#endif

#define KEYS( literal, field, value )                                        \
                                if ( !str_cmp( word, literal ) )        \
                                {                                        \
                                    free_string(field);                        \
                                    field  = value;                        \
                                    fMatch = TRUE;                        \
                                    break;                                \
                                }

void
fread_char(CHAR_DATA * ch, FILE * fp) {
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;
    int count = 0;
    int macro_count = 0;

    sprintf(buf, "Loading %s.", ch->name);
    log_string("%s", buf);

    for (;;) {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = FALSE;

        switch (UPPER(word[0])) {
            case '*':
                fMatch = TRUE;
                fread_to_eol(fp);
                break;

            case 'A':
                KEY("Act", ch->act, fread_flag(fp));
                KEY("AffectedBy", ch->affected_by, fread_flag(fp));
                KEY("AfBy", ch->affected_by, fread_flag(fp));
                KEY("Air", ch->air_supply, fread_number(fp));
                KEY("Alignment", ch->alignment, fread_number(fp));
                KEY("Alig", ch->alignment, fread_number(fp));

                if (!str_cmp(word, "Alia")) {
                    if (count >= MAX_ALIAS) {
                        fread_to_eol(fp);
                        fMatch = TRUE;
                        break;
                    }

                    ch->pcdata->alias[count] = str_dup(fread_word(fp));
                    ch->pcdata->alias_sub[count] = str_dup(fread_word(fp));
                    count++;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "Alias")) {
                    if (count >= MAX_ALIAS) {
                        fread_to_eol(fp);
                        fMatch = TRUE;
                        break;
                    }

                    ch->pcdata->alias[count] = str_dup(fread_word(fp));
                    ch->pcdata->alias_sub[count] = fread_string(fp);
                    count++;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "AC") || !str_cmp(word, "Armor")) {
                    fread_to_eol(fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "ACs")) {
                    int i;

                    for (i = 0; i < 4; i++) {
                        ch->armor[i] = fread_number(fp);
                    }

                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "AffD")) {
                    AFFECT_DATA *paf;
                    int sn;

                    paf = new_affect();

                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0) {
                        bug("Fread_char: unknown skill.", 0);
                    } else {
                        paf->type = sn;
                    }

                    paf->level = fread_number(fp);
                    paf->duration = fread_number(fp);
                    paf->modifier = fread_number(fp);
                    paf->location = fread_number(fp);
                    paf->bitvector = fread_number(fp);
                    paf->next = ch->affected;
                    ch->affected = paf;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "Affc")) {
                    AFFECT_DATA *paf;
                    int sn;

                    paf = new_affect();

                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0) {
                        bug("Fread_char: unknown skill.", 0);
                    } else {
                        paf->type = sn;
                    }

                    paf->where = fread_number(fp);
                    paf->level = fread_number(fp);
                    paf->duration = fread_number(fp);
                    paf->modifier = fread_number(fp);
                    paf->location = fread_number(fp);
                    paf->bitvector = fread_number(fp);
                    paf->next = ch->affected;
                    ch->affected = paf;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "AttrMod") || !str_cmp(word, "AMod")) {
                    int stat;

                    for (stat = 0; stat < MAX_STATS; stat++) {
                        ch->mod_stat[stat] = fread_number(fp);
                    }

                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "AttrPerm") || !str_cmp(word, "Attr")) {
                    int stat;

                    for (stat = 0; stat < MAX_STATS; stat++) {
                        ch->perm_stat[stat] = fread_number(fp);

                        // Update, fix stats below current racial base stats
                        if(ch->perm_stat[stat] < pc_race_table[ch->race].stats[stat]) {
                            ch->perm_stat[stat] = pc_race_table[ch->race].stats[stat];
                        }
                    }

                    fMatch = TRUE;
                    break;
                }
                break;

            case 'B':
                KEY("Bamfin", ch->pcdata->bamfin, fread_string(fp));
                KEY("Bamfout", ch->pcdata->bamfout, fread_string(fp));
                KEY("Bin", ch->pcdata->bamfin, fread_string(fp));
                KEY("Bout", ch->pcdata->bamfout, fread_string(fp));
                KEY("Brth", ch->breath, fread_number(fp));
                KEY("Bounty", ch->bounty, fread_number(fp));

                /* Read in board status */
                if (!str_cmp(word, "Boards")) {
                    int num;
                    char *boardname;
                    NOTE_READ_DATA *pNoteRead;
                    NOTE_READ_DATA *pLastNoteRead;

                    /* number of boards saved */
                    num = fread_number(fp);

                    pLastNoteRead = NULL;
                    pNoteRead = new_note_read();
                    ch->pcdata->noteReadData = pNoteRead;

                    for (; num; num--) {
                        /* for each of the board saved */
                        boardname = fread_word(fp);

                        if (!board_lookup(boardname)) {
                            /* Does board still exist ? */
                            sprintf(buf, "fread_char: %s had unknown board name: %s. Skipped.", ch->name, boardname);
                            log_string("%s", buf);
                            fread_number(fp);	/* read timestamp and skip info */
                        } else {
                            /* Save it */
                            pNoteRead->boardName = str_dup(boardname);
                            pNoteRead->timestamp = fread_number(fp);
                            pLastNoteRead = pNoteRead;
                            pNoteRead->next = new_note_read();
                            pNoteRead = pNoteRead->next;
                        }
                    }

                    if (pLastNoteRead) {
                        free_note_read(pLastNoteRead->next);
                        pLastNoteRead->next = NULL;
                    } else {
                        free_note_read(ch->pcdata->noteReadData);
                        ch->pcdata->noteReadData = NULL;
                    }

                    fMatch = TRUE;
                }
                break;


            case 'C':
                KEY("Class", ch->charClass, fread_number(fp));
                KEY("Cla", ch->charClass, fread_number(fp));
                KEY("Cre", ch->site, fread_string(fp));
                KEY("CraftTimer", ch->craft_timer, fread_number(fp));

                if (!str_cmp(word, "Clan")) {
                    char *temp_str;

                    temp_str = fread_string(fp);
                    KEY("Clan", ch->clan, clan_lookup(temp_str));
                    free_string(temp_str);
                    fMatch = TRUE;
                    break;
                }

                if(!str_cmp(word, "CRank")) {
                    KEY("CRank", ch->clan_rank, fread_string(fp));
                }

                if (!str_cmp(word, "Condition") || !str_cmp(word, "Cond")) {
                    ch->pcdata->condition[0] = fread_number(fp);
                    ch->pcdata->condition[1] = fread_number(fp);
                    ch->pcdata->condition[2] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "Cnd")) {
                    ch->pcdata->condition[0] = fread_number(fp);
                    ch->pcdata->condition[1] = fread_number(fp);
                    ch->pcdata->condition[2] = fread_number(fp);
                    ch->pcdata->condition[3] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                KEY("Comm", ch->comm, fread_flag(fp));
                break;

            case 'D':
                KEY("Damroll", ch->damroll, fread_number(fp));
                KEY("Dam", ch->damroll, fread_number(fp));
                KEY("Description", ch->description, fread_string(fp));
                KEY("Desc", ch->description, fread_string(fp));
                KEY("Deleg", ch->delegate, fread_number(fp));
                KEY("DeiT", ch->deity_type, fread_number(fp));
                KEY("DeiP", ch->deity_points, fread_number(fp));
                break;

            case 'E':
                if (!str_cmp(word, "End")) {
                    return;
                }

                KEY("Exp", ch->exp, fread_number(fp));
                break;

            case 'G':
                KEY("Gold", ch->gold, fread_number(fp));
                KEY("Gift", ch->gift, fread_number(fp));

                if (!str_cmp(word, "Group") || !str_cmp(word, "Gr")) {
                    int gn;
                    char *temp;
                    int sn;

                    temp = fread_word(fp);
                    gn = group_lookup(temp);
                    if (!str_cmp(temp, "blessing") && ch->charClass == class_lookup("cleric")) {
                        sn = gsn_holy_word;

                        if (ch->pcdata->learned[sn] == 0) {
                            ch->pcdata->learned[sn] = 1;
                        }
                    }

                    if (!str_cmp(temp, "conjuration") && ch->charClass == class_lookup("conjurer")) {
                        sn = gsn_crushing_hand;

                        if (ch->pcdata->learned[sn] == 0) {
                            ch->pcdata->learned[sn] = 1;
                        }

                        sn = gsn_clenched_fist;

                        if (ch->pcdata->learned[sn] == 0) {
                            ch->pcdata->learned[sn] = 1;
                        }
                    }

                    if (!str_cmp(temp, "enchantment") && ch->charClass == class_lookup("enchanter")) {
                        sn = gsn_feeblemind;

                        if (ch->pcdata->learned[sn] == 0) {
                            ch->pcdata->learned[sn] = 1;
                        }

                        sn = gsn_psychic_crush;

                        if (ch->pcdata->learned[sn] == 0) {
                            ch->pcdata->learned[sn] = 1;
                        }
                    }

                    /* gn    = group_lookup( fread_word( fp ) ); */
                    if (gn < 0) {
                        fprintf(stderr, "%s", temp);
                        bug("Fread_char: unknown group. ", 0);
                    } else {
                        gn_add(ch, gn);
                    }

                    fMatch = TRUE;
                }
                break;

            case 'H':
                KEY("Hitroll", ch->hitroll, fread_number(fp));
                KEY("Hit", ch->hitroll, fread_number(fp));

                if (!str_cmp(word, "HpManaMove") || !str_cmp(word, "HMV")) {
                    ch->hit = fread_number(fp);
                    ch->max_hit = fread_number(fp);
                    ch->mana = fread_number(fp);
                    ch->max_mana = fread_number(fp);
                    ch->move = fread_number(fp);
                    ch->max_move = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "HpManaMovePerm") || !str_cmp(word, "HMVP")) {
                    ch->max_hit = fread_number(fp);
                    ch->max_mana = fread_number(fp);
                    ch->max_move = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'I':
                KEY("Id", ch->id, fread_number(fp));
                KEY("InvisLevel", ch->invis_level, fread_number(fp));
                KEY("Inco", ch->incog_level, fread_number(fp));
                KEY("Invi", ch->invis_level, fread_number(fp));
                break;

            case 'L':
                KEY("LastLevel", ch->pcdata->last_level, fread_number(fp));
                KEY("LLev", ch->pcdata->last_level, fread_number(fp));
                KEY("Level", ch->level, fread_number(fp));
                KEY("Lev", ch->level, fread_number(fp));
                KEY("Levl", ch->level, fread_number(fp));
                KEY("Lay", ch->can_lay, fread_number(fp));
                KEY("LogO", ch->lastlogoff, fread_number(fp));
                KEY("LongDescr", ch->long_descr, fread_string(fp));
                KEY("LnD", ch->long_descr, fread_string(fp));
                break;

            case 'M':
                if (!str_cmp(word, "Macro")) {
                    if (macro_count >= MAX_NUM_MACROS) {
                        /* ignore macro definitions if we've already hit our
                         * maximum number of macros
                         */
                        fread_to_eol(fp);
                        fMatch = TRUE;
                        break;
                    }

                    ch->pcdata->macro[macro_count].name = str_dup(fread_word(fp));
                    ch->pcdata->macro[macro_count].definition = fread_string(fp);
                    /* all that's saved in the pfile is the macro definition,
                     * which looks like: macro command1; command2; command3...
                     * def_to_macro fills out the subs[] array.
                     */
                    def_to_macro(ch, ch->pcdata->macro[macro_count].name);
                    macro_count++;
                    fMatch = TRUE;
                    break;
                }

                KEY("MedNeed", ch->meditate_needed, fread_number(fp));

            case 'N':
                KEYS("Name", ch->name, fread_string(fp));

                if (!str_cmp(word, "Not")) {
                    fread_number(fp);
                    fread_number(fp);
                    fread_number(fp);
                    fread_number(fp);
                    fread_number(fp);
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'O':
                KEY("Outc", ch->outcast_timer, fread_number(fp));
                break;

            case 'P':
                KEY("Password", ch->pcdata->pwd, fread_string(fp));
                KEY("Pass", ch->pcdata->pwd, fread_string(fp));
                KEY("Pat", ch->patron, fread_string(fp));
                KEY("PasAl", ch->pass_along, fread_number(fp));
                KEY("PasLimit", ch->pass_along_limit, fread_number(fp));
                KEY("Pkil", ch->pkills, fread_number(fp));
                KEY("Pkds", ch->deaths, fread_number(fp));
                KEY("Played", ch->played_perm, fread_number(fp));
                KEY("Plyd", ch->played_perm, fread_number(fp));
                KEY("Points", ch->pcdata->points, fread_number(fp));
                KEY("Pnts", ch->pcdata->points, fread_number(fp));
                KEY("Position", ch->position, fread_number(fp));
                KEY("Pos", ch->position, fread_number(fp));
                KEY("Practice", ch->practice, fread_number(fp));
                KEY("Prac", ch->practice, fread_number(fp));
                KEYS("Prompt", ch->prompt, fread_string(fp));
                KEY("Prom", ch->prompt, fread_string(fp));
                break;

            case 'Q':
                KEY("QuestPnts", ch->questpoints, fread_number(fp));
                KEY("QuestNext", ch->pcdata->quest.timer, fread_number(fp));
                break;


            case 'R':
                KEY("RPTitle", ch->rptitle, fread_string(fp));
                KEY("Remort", ch->remort, fread_number(fp));
                KEY("Remsub", ch->rem_sub, fread_number(fp));
                KEY("Reclas", ch->reclass, fread_number(fp));
                if (!str_cmp(word, "Race")) {
                    char *temp_str;

                    temp_str = fread_string(fp);
                    KEY("Race", ch->race, race_lookup(temp_str));
                    free_string(temp_str);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "Room")) {
                    ch->in_room = get_room_index(fread_number(fp));

                    if (ch->in_room == NULL) {
                        log_string("Character has NULL room");
                        ch->in_room = get_room_index(ROOM_VNUM_LIMBO);
                    }

                    fMatch = TRUE;
                    break;
                }
                break;

            case 'S':
                KEY("SavingThrow", ch->saving_throw, fread_number(fp));
                KEY("Spou", ch->pcdata->spouse, fread_string( fp ) );
                KEY("Save", ch->saving_throw, fread_number(fp));
                KEY("Scro", ch->lines, fread_number(fp));
                KEY("Sex", ch->sex, fread_number(fp));
                KEY("Size", ch->size, fread_number(fp));
                KEY("ShortDescr", ch->short_descr, fread_string(fp));
                KEY("ShD", ch->short_descr, fread_string(fp));
                KEY("SNE", ch->shift_name, fread_string(fp));
                KEY("SST", ch->shift_short, fread_string(fp));
                KEY("SLG", ch->shift_long, fread_string(fp));
                KEY("Sec", ch->pcdata->security, fread_number(fp));		/* OLC */
                KEY("Silv", ch->silver, fread_number(fp));
                KEY("Sliv", ch->sliver, fread_number(fp));

                if (!str_cmp(word, "Specialty")) {
                    int sn;
                    char *temp;

                    temp = fread_word(fp);
                    sn = skill_lookup(temp);

                    if (sn < 0) {
                        fprintf(stderr, "%s", temp);
                        bug("Fread_char: unknown skill. ", 0);
                    } else {
                        ch->pcdata->specialty = sn;
                    }

                    fMatch = TRUE;
                }

                if (!str_cmp(word, "Skill") || !str_cmp(word, "Sk")) {
                    int sn;
                    int value;
                    char *temp;

                    value = fread_number(fp);
                    temp = fread_word(fp);
                    if (!str_cmp(temp, "hunting spear")) {
                        sn = gsn_lightning_spear;
                    } else {
                        sn = skill_lookup(temp);
                    }

                    if ((ch->pcdata->group_known[group_lookup("protective")] > 0
                            || ch->pcdata->group_known[group_lookup("abjuration")] > 0)
                            && get_skill(ch, gsn_protection_neutral) < 1) {
                        ch->pcdata->learned[gsn_protection_neutral] = 1;
                    }

                    if (ch->pcdata->group_known[group_lookup("weather")] > 0
                            && get_skill(ch, gsn_farsight) < 1) {
                        ch->pcdata->learned[gsn_farsight] = 1;
                    }

                    if (ch->pcdata->group_known[group_lookup("protective")] > 0
                            && get_skill(ch, gsn_know_alignment) < 1) {
                        ch->pcdata->learned[gsn_know_alignment] = 1;
                    }

		    // Update monks
                    if (!str_cmp(temp, "brawling")) 
                        sn = gsn_second_attack;
                    if (!str_cmp(temp, "chi: moonlight"))
                        sn = gsn_chi_ei;
                    if (!str_cmp(temp, "chi: whisper"))
                        sn = gsn_chi_kaze;
                    if (!str_cmp(temp, "chi: whirlwind"))
                        sn = gsn_zanshin;
                    if (!str_cmp(temp, "dragon spirit"))
                        sn = gsn_dragon_kick;
                    if (!str_cmp(temp, "dolphin spirit"))
                        sn = gsn_choke_hold;
                    if (!str_cmp(temp, "eagle spirit"))
                        sn = gsn_eagle_claw;
                    if (!str_cmp(temp, "weapon catch"))
                        sn = gsn_demon_fist;
		    if (!str_cmp(temp, "cycle")) 
			sn = gsn_shadow_walk;
                    
                    if (sn < 0) {
                        fprintf(stderr, "%s: ", temp);
                        bug("Fread_char: unknown skill. ", 0);
                    } else {
                        ch->pcdata->learned[sn] = value;
                    }

                    fMatch = TRUE;
                }
                break;

            case 'T':
                KEY("TrueSex", ch->pcdata->true_sex, fread_number(fp));
                KEY("TSex", ch->pcdata->true_sex, fread_number(fp));
                KEY("TPas", ch->to_pass, fread_number(fp));
                KEY("Trai", ch->train, fread_number(fp));
                KEY("Trust", ch->trust, fread_number(fp));
                KEY("Tru", ch->trust, fread_number(fp));
                KEY("Togls", ch->toggles, fread_flag(fp));

                if (!str_cmp(word, "Title") || !str_cmp(word, "Titl")) {
                    ch->pcdata->title = fread_string(fp);

                    if (ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ','
                        && ch->pcdata->title[0] != '!' && ch->pcdata->title[0] != '?') {
                        sprintf(buf, " %s", ch->pcdata->title);
                        free_string(ch->pcdata->title);
                        ch->pcdata->title = str_dup(buf);
                    }

                    fMatch = TRUE;
                    break;
                }
                break;

            case 'V':
                KEY("Version", ch->version, fread_number(fp));
                KEY("Vers", ch->version, fread_number(fp));
                KEY("Vas", ch->vassal, fread_string(fp));

                if (!str_cmp(word, "Vnum")) {
                    ch->pIndexData = get_mob_index(fread_number(fp));
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'W':
                KEY("Wimpy", ch->wimpy, fread_number(fp));
                KEY("Wimp", ch->wimpy, fread_number(fp));
                KEY("Wizn", ch->wiznet, fread_flag(fp));
                break;
        }

        if (!fMatch) {
            bug("Fread_char: no match.", 0);
            bug(word, 0);
            fread_to_eol(fp);
        }
    }
}

/* load a pet from the forgotten reaches */
void
fread_pet(CHAR_DATA * ch, FILE * fp)
{
	char *word;
	CHAR_DATA *pet;
	bool fMatch;
	int lastlogoff = current_time;
	int percent;

	/* first entry had BETTER be the vnum or we barf */
	word = feof(fp) ? "END" : fread_word(fp);
	if (!str_cmp(word, "Vnum"))
	{
		int vnum;

		vnum = fread_number(fp);
		if (get_mob_index(vnum) == NULL)
		{
			bug("Fread_pet: bad vnum %d.", vnum);
			pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
		}
		else
			pet = create_mobile(get_mob_index(vnum));
	}
	else
	{
		bug("Fread_pet: no vnum in file.", 0);
		pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
	}

	for (;;)
	{
		word = feof(fp) ? "END" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'A':
			KEY("Act", pet->act, fread_flag(fp));
			KEY("AfBy", pet->affected_by, fread_flag(fp));
			KEY("Alig", pet->alignment, fread_number(fp));

			if (!str_cmp(word, "ACs"))
			{
				int i;

				for (i = 0; i < 4; i++)
					pet->armor[i] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AffD"))
			{
				AFFECT_DATA *paf;
				int sn;

				paf = new_affect();

				sn = skill_lookup(fread_word(fp));
				if (sn < 0)
					bug("Fread_char: unknown skill.", 0);
				else
					paf->type = sn;

				paf->level = fread_number(fp);
				paf->duration = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->location = fread_number(fp);
				paf->bitvector = fread_number(fp);
				paf->next = pet->affected;
				pet->affected = paf;
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Affc"))
			{
				AFFECT_DATA *paf;
				int sn;

				paf = new_affect();

				sn = skill_lookup(fread_word(fp));
				if (sn < 0)
					bug("Fread_char: unknown skill.", 0);
				else
					paf->type = sn;

				paf->where = fread_number(fp);
				paf->level = fread_number(fp);
				paf->duration = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->location = fread_number(fp);
				paf->bitvector = fread_number(fp);
				paf->next = pet->affected;
				pet->affected = paf;
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AMod"))
			{
				int stat;

				for (stat = 0; stat < MAX_STATS; stat++)
					pet->mod_stat[stat] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Attr"))
			{
				int stat;

				for (stat = 0; stat < MAX_STATS; stat++)
					pet->perm_stat[stat] = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'C':
			KEY("Comm", pet->comm, fread_flag(fp));
			if (!str_cmp(word, "Clan"))
			{
				char *temp_str;

				temp_str = fread_string(fp);
				KEY("Clan", pet->clan, clan_lookup(temp_str));
				free_string(temp_str);
				fMatch = TRUE;
				break;
			}
			break;

		case 'D':
			KEY("Dam", pet->damroll, fread_number(fp));
			KEY("Desc", pet->description, fread_string(fp));
			break;

		case 'E':
			if (!str_cmp(word, "End"))
			{
				pet->leader = ch;
				pet->master = ch;
				ch->pet = pet;
				/* adjust hp mana move up  -- here for speed's sake */
				percent = (current_time - lastlogoff) * 25 / (2 * 60 * 60);

				if (percent > 0 && !IS_AFFECTED(ch, AFF_POISON)
					&& !IS_AFFECTED(ch, AFF_PLAGUE))
				{
					percent = UMIN(percent, 100);
					pet->hit += (pet->max_hit - pet->hit) * percent / 100;
					pet->mana += (pet->max_mana - pet->mana) * percent / 100;
					pet->move += (pet->max_move - pet->move) * percent / 100;
				}
				return;
			}
			KEY("Exp", pet->exp, fread_number(fp));
			break;

		case 'G':
			KEY("Gold", pet->gold, fread_number(fp));
			break;

		case 'H':
			KEY("Hit", pet->hitroll, fread_number(fp));

			if (!str_cmp(word, "HMV"))
			{
				pet->hit = fread_number(fp);
				pet->max_hit = fread_number(fp);
				pet->mana = fread_number(fp);
				pet->max_mana = fread_number(fp);
				pet->move = fread_number(fp);
				pet->max_move = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'L':
			KEY("Levl", pet->level, fread_number(fp));
			KEY("LnD", pet->long_descr, fread_string(fp));
			KEY("LogO", lastlogoff, fread_number(fp));
			break;

		case 'N':
			KEY("Name", pet->name, fread_string(fp));
			break;

		case 'P':
			KEY("Pos", pet->position, fread_number(fp));
			break;

		case 'R':
			if (!str_cmp(word, "Race"))
			{
				char *temp_str;

				temp_str = fread_string(fp);
				KEY("Race", pet->race, race_lookup(temp_str));
				free_string(temp_str);
				fMatch = TRUE;
				break;
			}
			break;

		case 'S':
			KEY("Save", pet->saving_throw, fread_number(fp));
			KEY("Sex", pet->sex, fread_number(fp));
			KEY("ShD", pet->short_descr, fread_string(fp));
			KEY("Silv", pet->silver, fread_number(fp));
			break;

			if (!fMatch)
			{
				bug("Fread_pet: no match.", 0);
				fread_to_eol(fp);
			}

		}
	}
}

extern OBJ_DATA *obj_free;

void
fread_obj(CHAR_DATA * ch, FILE * fp)
{
	OBJ_DATA *obj;
	AFFECT_DATA* paf;
	char *word;
	int iNest;
	bool fMatch;
	bool fNest;
	bool fVnum;
	bool first;
	bool new_format;			/* to prevent errors */
	bool make_new;				/* update object */
	char buf[MAX_STRING_LENGTH];

	fVnum = FALSE;
	obj = NULL;
	first = TRUE;				/* used to counter fp offset */
	new_format = FALSE;
	make_new = FALSE;

	word = feof(fp) ? "End" : fread_word(fp);
	if (!str_cmp(word, "Vnum"))
	{
		int vnum;

		first = FALSE;			/* fp will be in right place */

		vnum = fread_number(fp);
		if (get_obj_index(vnum) == NULL)
		{
			bug("Fread_obj: bad vnum %d.", vnum);
/* Delstar */
			for (;;)
			{
				if (feof(fp))
					break;
				else
				{
					word = fread_word(fp);
					if (!str_cmp(word, "End"))
						break;
				}
			}
		}
		else
		{
			obj = create_object(get_obj_index(vnum), -1);
			new_format = TRUE;
		}

	}

	if (obj == NULL)			/* either not found or old style */
	{
		obj = new_obj();
		obj->name = str_dup("");
		obj->short_descr = str_dup("");
		obj->description = str_dup("");
	}

	fNest = FALSE;
	fVnum = TRUE;
	iNest = 0;

	for (;;)
	{
		if (first)
			first = FALSE;
		else
			word = feof(fp) ? "End" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'A':
			if (!str_cmp(word, "AffD"))
			{
				int sn;

				paf = new_affect();

				sn = skill_lookup(fread_word(fp));
				if (sn < 0)
					bug("Fread_obj: unknown skill.", 0);
				else
					paf->type = sn;

				paf->level = fread_number(fp);
				paf->duration = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->location = fread_number(fp);
				paf->bitvector = fread_number(fp);
				paf->next = obj->affected;
				obj->affected = paf;
				fMatch = TRUE;
				break;
			}
			if (!str_cmp(word, "Affc"))
			{
				int sn;

				paf = new_affect();

				sn = skill_lookup(fread_word(fp));
				if (sn < 0)
					bug("Fread_obj: unknown skill.", 0);
				else
					paf->type = sn;

				paf->where = fread_number(fp);
				paf->level = fread_number(fp);
				paf->duration = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->location = fread_number(fp);
				paf->bitvector = fread_number(fp);
				paf->next = obj->affected;
				obj->affected = paf;
				fMatch = TRUE;
				break;
			}
			if (!str_cmp(word, "AffExtra"))
                        {
                                int sn;

                                paf = new_affect();

                                sn = skill_lookup(fread_word(fp));
                                if (sn < 0)
                                        bug("Fread_obj: unknown skill.", 0);
                                else
                                        paf->type = sn;

                                paf->where = fread_number(fp);
                                paf->level = fread_number(fp);
                                paf->duration = fread_number(fp);
                                paf->modifier = fread_number(fp);
                                paf->location = fread_number(fp);
                                paf->bitvector = fread_number(fp);
				paf->extra = fread_number(fp);
                                paf->next = obj->affected;
                                obj->affected = paf;
                                fMatch = TRUE;
                                break;
                        }
			break;


		case 'C':
			KEY("Cond", obj->condition, fread_number(fp));
			KEY("Cost", obj->cost, fread_number(fp));
			KEY("ClanType", obj->clan_status, fread_number(fp));

			if (!str_cmp(word, "Charge"))
			{
        			int iValue;
        			int sn;
        			char *str;

        			iValue = fread_number(fp);
        			str = fread_word(fp);
        			sn = skill_lookup(str);

				// Fix broken charged items
				if(obj->special[3] !=
				obj->pIndexData->special[3])
					sn = obj->pIndexData->special[3];

        			if (iValue < 0 || iValue > 4)
        			{
                			bug("Fread_obj: bad iValue %d.", iValue);
        			}
        			else if (sn < 0)
        			{
                			bug("Fread_obj: unknown skill.", 0);
        			}
        			else
        			{
                			obj->special[iValue] = sn;
        			}
				fMatch = TRUE;
				break;
			}
		case 'D':
			KEY("Description", obj->description, fread_string(fp));
			KEY("Desc", obj->description, fread_string(fp));
			break;

		case 'E':

			if (!str_cmp(word, "Enchanted"))
			{
				obj->enchanted = TRUE;
				fMatch = TRUE;
				break;
			}

			KEY("ExtraFlags", obj->extra_flags, fread_number(fp));
			KEY("ExtF", obj->extra_flags, fread_number(fp));

			if (!str_cmp(word, "EValue"))
                        {
                                obj->extra[0] = fread_number(fp);
                                obj->extra[1] = fread_number(fp);
                                obj->extra[2] = fread_number(fp);
                                obj->extra[3] = fread_number(fp);
                                obj->extra[4] = fread_number(fp);
                                fMatch = TRUE;
                                break;
                        }

			if (!str_cmp(word, "ExtraDescr") || !str_cmp(word, "ExDe"))
			{
				EXTRA_DESCR_DATA *ed;

				ed = new_extra_descr();

				ed->keyword = fread_string(fp);
				ed->description = fread_string(fp);
				ed->next = obj->extra_descr;
				obj->extra_descr = ed;
				fMatch = TRUE;
			}

			if (!str_cmp(word, "End"))
			{
				if (!fNest || (fVnum && obj->pIndexData == NULL))
				{
					bug("Fread_obj: incomplete object.", 0);
					extract_obj(obj);
					return;
				}
				else
				{
					if (!fVnum)
					{
						free_string(obj->name);
						free_string(obj->description);
						free_string(obj->short_descr);
						obj->next = obj_free;
						obj_free = obj;

						obj = create_object(get_obj_index(OBJ_VNUM_DUMMY), 0);
					}

					if (!new_format)
					{
						obj->next = object_list;
						object_list = obj;
						obj->pIndexData->count++;
					}

					if (!obj->pIndexData->new_format
						&& obj->item_type == ITEM_ARMOR
						&& obj->value[1] == 0)
					{
						obj->value[1] = obj->value[0];
						obj->value[2] = obj->value[0];
					}
					if (make_new)
					{
						int wear;

						wear = obj->wear_loc;
						extract_obj(obj);

						obj = create_object(obj->pIndexData, 0);
						obj->wear_loc = wear;
					}

					if (iNest == 0 || rgObjNest[iNest] == NULL)
					{
						obj_to_char(obj, ch);

					}
					else
					{
						obj_to_obj(obj, rgObjNest[iNest - 1]);
					}
					update_version_weapon(ch, obj);
					return;
				}
			}
			break;

		case 'I':
			KEY("ItemType", obj->item_type, fread_number(fp));
			KEY("Ityp", obj->item_type, fread_number(fp));
			break;

		case 'L':
			KEY("Level", obj->level, fread_number(fp));
			KEY("Lev", obj->level, fread_number(fp));
			break;

		case 'N':
			KEY("Name", obj->name, fread_string(fp));

			if (!str_cmp(word, "Nest"))
			{
				iNest = fread_number(fp);
				if (iNest < 0 || iNest >= MAX_NEST)
				{
					bug("Fread_obj: bad nest %d.", iNest);
				}
				else
				{
					rgObjNest[iNest] = obj;
					fNest = TRUE;
				}
				fMatch = TRUE;
			}
			break;

		case 'O':
			KEY("Owner", obj->owner, fread_string(fp));
			KEY("Onum", obj->owner_vnum, fread_number(fp));
			if (!str_cmp(word, "Oldstyle"))
			{
				if (obj->pIndexData != NULL && obj->pIndexData->new_format)
					make_new = TRUE;
				fMatch = TRUE;
			}
			break;
		case 'R':
			KEY("Respawn_Owner", obj->respawn_owner, fread_string(fp));
			break;
		case 'S':
			KEY("Seek", obj->seek, fread_string(fp));
			KEY("ShortDescr", obj->short_descr, fread_string(fp));
			KEY("ShD", obj->short_descr, fread_string(fp));
			if (!str_cmp(word, "SValue"))
                        {
                                obj->special[0] = fread_number(fp);
                                obj->special[1] = fread_number(fp);
                                obj->special[2] = fread_number(fp);
                                obj->special[3] = fread_number(fp);
                                obj->special[4] = fread_number(fp);
                                fMatch = TRUE;
                                break;
                        }

			if (!str_cmp(word, "Spell"))
			{
				int iValue;
				int sn;
				char *str;

				iValue = fread_number(fp);
				str = fread_word(fp);
				sn = skill_lookup(str);

				if (iValue < 0 || iValue > 4)
				{
					bug("Fread_obj: bad iValue %d.", iValue);
				}
				else if (sn < 0)
				{
					bug("Fread_obj: unknown skill.", 0);
				}
				else
				{
					obj->value[iValue] = sn;
				}
				fMatch = TRUE;
				break;
			}

			break;

		case 'T':
			KEY("Timer", obj->timer, fread_number(fp));
			KEY("Time", obj->timer, fread_number(fp));
			break;

		case 'V':
			if (!str_cmp(word, "Values") || !str_cmp(word, "Vals"))
			{
				obj->value[0] = fread_number(fp);
				obj->value[1] = fread_number(fp);
				obj->value[2] = fread_number(fp);
				obj->value[3] = fread_number(fp);
				if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
					obj->value[0] = obj->pIndexData->value[0];
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Val"))
			{
				obj->value[0] = fread_number(fp);
				obj->value[1] = fread_number(fp);
				obj->value[2] = fread_number(fp);
				obj->value[3] = fread_number(fp);
				obj->value[4] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Vnum"))
			{
				int vnum;

				vnum = fread_number(fp);
				if ((obj->pIndexData = get_obj_index(vnum)) == NULL)
					bug("Fread_obj: bad vnum %d.", vnum);
				else
					fVnum = TRUE;
				fMatch = TRUE;
				break;
			}
			break;

		case 'W':
			KEY("WearFlags", obj->wear_flags, fread_number(fp));
			KEY("WeaF", obj->wear_flags, fread_number(fp));
			KEY("WearLoc", obj->wear_loc, fread_number(fp));
			KEY("Wear", obj->wear_loc, fread_number(fp));
			KEY("Weight", obj->weight, fread_number(fp));
			KEY("Wt", obj->weight, fread_number(fp));
			break;

		}

		if (!fMatch)
		{
			sprintf(buf, "Fread_obj: no match on %s", word);
			bug(buf, 0);
			fread_to_eol(fp);
		}
	}

}


void write_clanleaders()
{
	FILE *fp;
	int i;

	fclose (fpReserve);

	if ((fp = fopen (TEMP_FILE, "w")) == NULL)
	{
		bug ("Save_count: fopen", 0);
		return;
	}

	for (i = 0; i < MAX_CLAN; i++)
	{
		if ( clan_table[i].independent == FALSE )
		{
			fprintf (fp, "Clan %s~\n", clan_table[i].name );
			if (clan_leadership[i].leader)
			{
				fprintf (fp, "Leader %s~\n", clan_leadership[i].leader );
			}
			else
			{
				fprintf (fp, "Leader none~\n");
			}

			if (clan_leadership[i].recruiter1 != NULL)
			{
				fprintf (fp, "R1 %s~\n", clan_leadership[i].recruiter1 );
			}
			else
			{
				fprintf (fp, "R1 none~\n");
			}

			if (clan_leadership[i].recruiter2 != NULL)
			{
				fprintf (fp, "R2 %s~\n", clan_leadership[i].recruiter2 );
			}
			else
			{
				fprintf (fp, "R2 none~\n");
			}
		}
	}

	fprintf (fp, "End\n");

	fclose (fp);
	rename (TEMP_FILE, CLAN_FILE);
	fpReserve = fopen (NULL_FILE, "r");
	return;
}

void read_clanleaders()
{
	FILE *fp;
	char *word;
	bool fMatch;
	bool done;
	int clan;

	if ((fp = fopen (CLAN_FILE, "r")) == NULL)
	{
		bug ("Error reading clan file, loading defaults and creating new file.'", 0);
		memset (clan_leadership, 0, sizeof( CLAN_LEADER_DATA ) * MAX_CLAN);
		for(clan = 0; clan < MAX_CLAN; clan++) {
			clan_leadership[clan].leader = NULL;
			clan_leadership[clan].recruiter1 = NULL;
			clan_leadership[clan].recruiter2 = NULL;
		}

		write_clanleaders();

		return;
	}

	memset (clan_leadership, 0, sizeof( CLAN_LEADER_DATA ) * MAX_CLAN);
	done = FALSE;

	for (;;)
	{
		word = feof (fp) ? "End" : fread_word (fp);
		fMatch = FALSE;

		switch (UPPER (word[0]))
		{
		case 'C':
			if (!str_cmp (word, "Clan") )
			{
				char* temp_str;

				temp_str = fread_string(fp);
				KEY ("Clan", clan, clan_lookup ( temp_str ));
				free_string( temp_str );
				fMatch = TRUE;
				break;
			}
		case 'L':
			if (!str_cmp (word, "Leader") )
			{
				char* temp_str;

				temp_str = fread_string(fp);
				if (!str_cmp (temp_str, "none"))
				{
					clan_leadership[clan].leader = NULL;
				}
				else
				{
					clan_leadership[clan].leader = str_dup(temp_str);
				}
				free_string( temp_str );
				fMatch = TRUE;
			}
		case 'R':
			if (!str_cmp (word, "R1") )
			{
				char* temp_str;

				temp_str = fread_string(fp);
				if (!str_cmp (temp_str, "none"))
				{
					clan_leadership[clan].recruiter1 = NULL;
				}
				else
				{
					clan_leadership[clan].recruiter1 = str_dup(temp_str);
				}
				free_string( temp_str );
				fMatch = TRUE;
			}
			if (!str_cmp (word, "R2") )
			{
				char* temp_str;

				temp_str = fread_string(fp);
				if (!str_cmp (temp_str, "none"))
				{
					clan_leadership[clan].recruiter2 = NULL;
				}
				else
				{
					clan_leadership[clan].recruiter2 = str_dup(temp_str);
				}
				free_string( temp_str );
				fMatch = TRUE;
			}
		case 'E':
			if (!str_cmp (word, "End") )
			{
				done = TRUE;
				fMatch = TRUE;
				break;
			}
		}

		if (!fMatch)
		{
			bug ("Fread_char: no match.", 0);
			bug ( word, 0 );
			fread_to_eol (fp);
		}

		if (done)
		{
			break;
		}
	}

	fclose (fp);
	return;
}
