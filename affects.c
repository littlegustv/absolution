/**************************************************************************r
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
#include <string.h>
#include <time.h>

#include "merc.h"
#include "recycle.h"
#include "magic.h"
#include "utils.h"

/* enchanted stuff for eq */
void
affect_enchant(OBJ_DATA * obj) {
    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted) {
        AFFECT_DATA *paf, *af_new;

        obj->enchanted = TRUE;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) {
            af_new = new_affect();

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where = paf->where;
            af_new->type = UMAX(0, paf->type);
            af_new->level = paf->level;
            af_new->duration = paf->duration;
            af_new->location = paf->location;
            af_new->modifier = paf->modifier;
            af_new->bitvector = paf->bitvector;
        }
    }
}

/*
 * Apply or remove an affect to a character.
 */
void
affect_modify(CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd) {
    OBJ_DATA *wield;
    int mod, i;

    mod = paf->modifier;

    if (fAdd) {
        switch (paf->where) {
            case TO_AFFECTS:
                SET_BIT(ch->affected_by, paf->bitvector);
                break;

            case TO_IMMUNE:
                SET_BIT(ch->imm_flags, paf->bitvector);
                break;

            case TO_RESIST:
                SET_BIT(ch->res_flags, paf->bitvector);
                break;

            case TO_VULN:
                SET_BIT(ch->vuln_flags, paf->bitvector);
                break;
        }
    } else {
        switch (paf->where) {
            case TO_AFFECTS:
                REMOVE_BIT(ch->affected_by, paf->bitvector);
                break;

            case TO_IMMUNE:
                REMOVE_BIT(ch->imm_flags, paf->bitvector);
                break;

            case TO_RESIST:
                REMOVE_BIT(ch->res_flags, paf->bitvector);
                break;

            case TO_VULN:
                REMOVE_BIT(ch->vuln_flags, paf->bitvector);
                break;
        }

        mod = 0 - mod;
    }

    switch (paf->location) {
        default:
            bug("Affect_modify: unknown location %d.", paf->location);
            return;

        case APPLY_NONE:
            break;

        case APPLY_STR:
            ch->mod_stat[STAT_STR] += mod;
            break;

        case APPLY_DEX:
            ch->mod_stat[STAT_DEX] += mod;
            break;

        case APPLY_INT:
            ch->mod_stat[STAT_INT] += mod;
            break;

        case APPLY_WIS:
            ch->mod_stat[STAT_WIS] += mod;
            break;

        case APPLY_CON:
            ch->mod_stat[STAT_CON] += mod;
            break;

        case APPLY_SEX:
            ch->sex += mod;
            break;

        case APPLY_AGE:
            ch->played += (mod * 3600);
            break;

        case APPLY_MANA:
            ch->max_mana_bonus += mod;
            break;

        case APPLY_HIT:
            ch->max_hit_bonus += mod;
            break;

        case APPLY_MOVE:
            ch->max_move_bonus += mod;
            break;

        case APPLY_AC:
            for (i = 0; i < 4; i++) {
                ch->armor[i] += mod;
            }

            break;

        case APPLY_HITROLL:
            ch->hitroll += mod;
            break;

        case APPLY_DAMROLL:
            ch->damroll += mod;
            break;

        case APPLY_SAVES:
            ch->saving_throw += mod;
            break;

        case APPLY_SAVING_ROD:
            ch->saving_throw += mod;
            break;

        case APPLY_SAVING_PETRI:
            ch->saving_throw += mod;
            break;

        case APPLY_SAVING_BREATH:
            ch->saving_throw += mod;
            break;

        case APPLY_SAVING_SPELL:
            ch->saving_throw += mod;
            break;

        case APPLY_SPELL_AFFECT:
            break;

        case APPLY_DAMAGE_REDUCE:
            ch->damage_reduce += mod;
            break;

        case APPLY_SPELL_DAMAGE:
            ch->spell_damroll += mod;
            break;

        case APPLY_MAX_STR:
            ch->max_stat_bonus[STAT_STR] += mod;
            break;

        case APPLY_MAX_DEX:
            ch->max_stat_bonus[STAT_DEX] += mod;
            break;

        case APPLY_MAX_CON:
            ch->max_stat_bonus[STAT_CON] += mod;
            break;

        case APPLY_MAX_INT:
            ch->max_stat_bonus[STAT_INT] += mod;
            break;

        case APPLY_MAX_WIS:
            ch->max_stat_bonus[STAT_WIS] += mod;
            break;

        case APPLY_ATTACK_SPEED:
            ch->attack_speed += mod;
            break;
    }

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
    if (!IS_NPC(ch) && (wield = get_eq_char(ch, WEAR_WIELD)) != NULL
            && get_obj_weight(wield) > (str_app[get_curr_stat(ch, STAT_STR)].wield * 10)) {
        static int depth;

        if (depth == 0) {
            depth++;
            act("You drop $p.", ch, wield, NULL, TO_CHAR, POS_RESTING);
            act("$n drops $p.", ch, wield, NULL, TO_ROOM, POS_RESTING);
            obj_from_char(wield);
            obj_to_room(wield, ch->in_room);
            depth--;
        }
    }

    return;
}


/*
 * Checks the specified affect list for an affect with the specified sn.  If
 * found, the affect is returned -- otherwise, NULL is returned.
 */
AFFECT_DATA *
affect_find(AFFECT_DATA *affectList, int sn) {
    while (affectList != NULL) {
        if (affectList->type == sn) {
            break;
        }

        affectList = affectList->next;
    }

    // affectList is either NULL, or has the proper type (== sn)
    return affectList;
}

/* fix object affects when removing one */
void
affect_check(CHAR_DATA * ch, int where, int vector)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    if (where == TO_OBJECT || where == TO_WEAPON || vector == 0) {
        return;
    }

    for (paf = ch->affected; paf != NULL; paf = paf->next) {
        if (paf->where == where && paf->bitvector == vector) {
            switch (where) {
                case TO_AFFECTS:
                    SET_BIT(ch->affected_by, vector);
                    break;

                case TO_IMMUNE:
                    SET_BIT(ch->imm_flags, vector);
                    break;

                case TO_RESIST:
                    SET_BIT(ch->res_flags, vector);
                    break;

                case TO_VULN:
                    SET_BIT(ch->vuln_flags, vector);
                    break;
            }

            return;
        }
    }

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
        if (obj->wear_loc == -1) {
            continue;
        }

        for (paf = obj->affected; paf != NULL; paf = paf->next) {
            if (paf->where == where && paf->bitvector == vector) {
                switch (where) {
                    case TO_AFFECTS:
                        SET_BIT(ch->affected_by, vector);
                        break;

                    case TO_IMMUNE:
                        SET_BIT(ch->imm_flags, vector);
                        break;

                    case TO_RESIST:
                        SET_BIT(ch->res_flags, vector);
                        break;

                    case TO_VULN:
                        SET_BIT(ch->vuln_flags, vector);
                }

                return;
            }
        }

        if (obj->enchanted) {
            continue;
        }

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) {
            if (paf->where == where && paf->bitvector == vector) {
                switch (where) {
                    case TO_AFFECTS:
                        SET_BIT(ch->affected_by, vector);
                        break;

                    case TO_IMMUNE:
                        SET_BIT(ch->imm_flags, vector);
                        break;

                    case TO_RESIST:
                        SET_BIT(ch->res_flags, vector);
                        break;

                    case TO_VULN:
                        SET_BIT(ch->vuln_flags, vector);
                        break;
                }

                return;
            }
        }
    }
}

/*
 * Give an affect to a char.
 */
void
affect_to_char(CHAR_DATA * ch, AFFECT_DATA * paf) {
    AFFECT_DATA *paf_new;
    AFFECT_DATA *caf;
    int new_duration = 0;
    int found = -1;

    paf_new = new_affect();
    *paf_new = *paf;
    VALIDATE(paf_new);

    paf_new->next = ch->affected;
    ch->affected = paf_new;

    // Remort dwarf penalty!!
    if (ch->race == race_lookup("dwarf")
            && ch->remort
            && number_percent() < 25
            && skill_table[paf_new->type].spell_fun != spell_null) {
        found = 0;
        new_duration = paf_new->duration / 2;
        paf_new->duration = new_duration;

        // Fix ALL affects of this type (bleh)
        for (caf = ch->affected; caf != NULL; caf = caf->next) {
            if (caf != paf_new
                    && caf->type == paf_new->type) {
                caf->duration = new_duration;
                found = 1;
            }
        }
    }

    // Only display message once
    if (found == 0) {
        Cprintf(ch, "Your dwarven ancestry interferes.\n\r");
    }

    affect_modify(ch, paf_new, TRUE);

    return;
}


void
affect_to_room(ROOM_INDEX_DATA * room, AFFECT_DATA * af) {
    AFFECT_DATA *af_new;

    af_new = new_affect();
    *af_new = *af;
    VALIDATE(af_new);
    af_new->next = room->affected;
    room->affected = af_new;

    /* apply bit vectors */
    if (af->bitvector) {
        SET_BIT(room->affected_by, af->bitvector);
    }
}


/* give an affect to an object */
void
affect_to_obj(OBJ_DATA * obj, AFFECT_DATA * paf) {
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new = *paf;
    VALIDATE(paf_new);
    paf_new->next = obj->affected;
    obj->affected = paf_new;

    /* apply any affect vectors to the object's extra_flags */
    if (paf->bitvector) {
        switch (paf->where) {
            case TO_OBJECT:
                SET_BIT(obj->extra_flags, paf->bitvector);
                break;

            case TO_WEAPON:
                if (obj->item_type == ITEM_WEAPON) {
                    SET_BIT(obj->value[4], paf->bitvector);
                }

                break;
        }
    }


    return;
}



/*
 * Remove an affect from a char.
 */
void
affect_remove(CHAR_DATA * ch, AFFECT_DATA * paf) {
    int where;
    int vector;

    if (ch->affected == NULL) {
        /* This isn't too bad. */
        /*bug("Affect_remove: no affect.", 0);
        */
        return;
    }

    affect_modify(ch, paf, FALSE);
    where = paf->where;
    vector = paf->bitvector;

    if (paf == ch->affected) {
        ch->affected = paf->next;
    } else {
        AFFECT_DATA *prev;

        for (prev = ch->affected; prev != NULL; prev = prev->next) {
            if (prev->next == paf) {
                prev->next = paf->next;
                break;
            }
        }

        if (prev == NULL) {
            bug("Affect_remove: cannot find paf.", 0);
            return;
        }
    }

    free_affect(paf);

    affect_check(ch, where, vector);
    return;
}


void
affect_remove_room(ROOM_INDEX_DATA * room, AFFECT_DATA * af) {
    if (room->affected == NULL) {
        bug("Affect_remove_room: no affect.", 0);
        return;
    }

    REMOVE_BIT(room->affected_by, af->bitvector);

    if (af == room->affected) {
        room->affected = af->next;
    } else {
        AFFECT_DATA *prev;

        for (prev = room->affected; prev != NULL; prev = prev->next) {
            if (prev->next == af) {
                prev->next = af->next;
                break;
            }
        }

        if (prev == NULL) {
            bug("Affect_remove_room: cannot find paf.", 0);
            return;
        }
    }

    free_affect(af);
}


void
affect_remove_obj(OBJ_DATA * obj, AFFECT_DATA * paf) {
    int where, vector;

    if (obj->affected == NULL) {
        bug("Affect_remove_object: no affect.", 0);
        return;
    }

    if (obj->carried_by != NULL && obj->wear_loc != -1) {
        affect_modify(obj->carried_by, paf, FALSE);
    }

    where = paf->where;
    vector = paf->bitvector;

    /* remove flags from the object if needed */
    if (paf->bitvector) {
        switch (paf->where) {
            case TO_OBJECT:
                REMOVE_BIT(obj->extra_flags, paf->bitvector);
                break;

            case TO_WEAPON:
                if (obj->item_type == ITEM_WEAPON) {
                    REMOVE_BIT(obj->value[4], paf->bitvector);
                }

                break;
        }
    }

    if (paf == obj->affected) {
        obj->affected = paf->next;
    } else {
        AFFECT_DATA *prev;

        for (prev = obj->affected; prev != NULL; prev = prev->next) {
            if (prev->next == paf) {
                prev->next = paf->next;
                break;
            }
        }

        if (prev == NULL) {
            bug("Affect_remove_object: cannot find paf.", 0);
            return;
        }
    }

    free_affect(paf);

    if (obj->carried_by != NULL && obj->wear_loc != -1) {
        affect_check(obj->carried_by, where, vector);
    }

    return;
}


/*
 * Strip all affects of a given sn.
 */
void
affect_strip(CHAR_DATA * ch, int sn) {
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    for (paf = ch->affected; paf != NULL; paf = paf_next) {
        paf_next = paf->next;
        if (paf->type == sn) {
            // Should not be needed, but sanity check.
            if (skill_table[sn].end_fun != end_null) {
                skill_table[sn].end_fun((void*)ch, TARGET_CHAR);
            }
            affect_remove(ch, paf);
        }
    }

    return;
}

/*
 * Return true if a char is affected by a spell.
 */
bool
is_affected(CHAR_DATA * ch, int sn) {
    return affect_find(ch->affected, sn) != NULL;
}

bool
room_is_affected(ROOM_INDEX_DATA * room, int sn) {
    return affect_find(room->affected, sn) != NULL;
}

bool
obj_is_affected(OBJ_DATA* obj, int sn) {
    return affect_find(obj->affected, sn) != NULL;
}

/*
 * Retrieve the actual effect
 */
AFFECT_DATA *
get_affect(CHAR_DATA *ch, int sn) {
    return affect_find(ch->affected, sn);
}

AFFECT_DATA *
get_room_affect(ROOM_INDEX_DATA *room, int sn) {
    return affect_find(room->affected, sn);
}

AFFECT_DATA *
get_obj_affect(OBJ_DATA *obj, int sn) {
    return affect_find(obj->affected, sn);
}

/*
 * Add or enhance an affect.
 */
void
affect_join(CHAR_DATA * ch, AFFECT_DATA * paf) {
    AFFECT_DATA *paf_old;
    bool found;

    found = FALSE;
    for (paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next) {
        if (paf_old->type == paf->type) {
            paf->level = (paf->level += paf_old->level) / 2;
            paf->duration += paf_old->duration;
            paf->modifier += paf_old->modifier;
            affect_remove(ch, paf_old);
            break;
        }
    }

    affect_to_char(ch, paf);
    return;
}

/*
Find an effect and merge the 2.
-Affect from same spell
-Affect has same location

Merge:
 Add modifier
 Reset time
*/
void
affect_merge(CHAR_DATA * ch, AFFECT_DATA * paf, int sn) {
    AFFECT_DATA *paf_old, *paf_old_next;

    for (paf_old = ch->affected; paf_old != NULL; paf_old = paf_old_next) {
        paf_old_next = paf_old->next;

        if (paf_old->location == paf->location &&
                paf_old->type == paf->type) {
            paf->modifier += paf_old->modifier;
            affect_remove(ch, paf_old);
            break;
        }
    }

    affect_to_char(ch, paf);
    return;
}

/*
 * Return ascii name of an affect location.
 */
char *
affect_loc_name(int location) {
    switch (location) {
        case APPLY_NONE:
            return "none";

        case APPLY_STR:
            return "strength";

        case APPLY_DEX:
            return "dexterity";

        case APPLY_INT:
            return "intelligence";

        case APPLY_WIS:
            return "wisdom";

        case APPLY_CON:
            return "constitution";

        case APPLY_SEX:
            return "sex";

        case APPLY_LEVEL:
            return "level";

        case APPLY_AGE:
            return "age";

        case APPLY_MANA:
            return "mana";

        case APPLY_HIT:
            return "hp";

        case APPLY_MOVE:
            return "moves";

        case APPLY_AC:
            return "armor class";

        case APPLY_HITROLL:
            return "hit roll";

        case APPLY_DAMROLL:
            return "damage roll";

        case APPLY_SAVES:
            return "saves";

        case APPLY_SAVING_ROD:
            return "save vs rod";

        case APPLY_SAVING_PETRI:
            return "save vs petrification";

        case APPLY_SAVING_BREATH:
            return "save vs breath";

        case APPLY_SAVING_SPELL:
            return "save vs spell";

        case APPLY_DAMAGE_REDUCE:
            return "damage reduction";

        case APPLY_SPELL_DAMAGE:
            return "all magic damage";

        case APPLY_MAX_STR:
            return "max strength";

        case APPLY_MAX_DEX:
            return "max dexterity";

        case APPLY_MAX_CON:
            return "max constitution";

        case APPLY_MAX_INT:
            return "max intelligence";

        case APPLY_MAX_WIS:
            return "max wisdom";

        case APPLY_SPELL_AFFECT:
            return "none";

        case APPLY_ATTACK_SPEED:
            return "attack speed";
    }

    bug("Affect_location_name: unknown location %d.", location);
    return "(unknown)";
}


/*
 * Return ascii name of an affect bit vector.
 */
char *
affect_bit_name(int vector) {
    static char buf[512];

    buf[0] = '\0';
    if (vector & AFF_BLIND) {
        strcat(buf, " blind");
    }

    if (vector & AFF_INVISIBLE) {
        strcat(buf, " invisible");
    }

    if (vector & AFF_DETECT_INVIS) {
        strcat(buf, " detect_invis");
    }

    if (vector & AFF_DETECT_MAGIC) {
        strcat(buf, " detect_magic");
    }

    if (vector & AFF_DETECT_HIDDEN) {
        strcat(buf, " detect_hidden");
    }

    if (vector & AFF_SANCTUARY) {
        strcat(buf, " sanctuary");
    }

    if (vector & AFF_FAERIE_FIRE) {
        strcat(buf, " faerie_fire");
    }

    if (vector & AFF_INFRARED) {
        strcat(buf, " infrared");
    }

    if (vector & AFF_CURSE) {
        strcat(buf, " curse");
    }

    if (vector & AFF_POISON) {
        strcat(buf, " poison");
    }

    if (vector & AFF_PROTECT_EVIL) {
        strcat(buf, " prot_evil");
    }

    if (vector & AFF_PROTECT_GOOD) {
        strcat(buf, " prot_good");
    }

    if (vector & AFF_SLEEP) {
        strcat(buf, " sleep");
    }

    if (vector & AFF_SNEAK) {
        strcat(buf, " sneak");
    }

    if (vector & AFF_HIDE) {
        strcat(buf, " hide");
    }

    if (vector & AFF_CHARM) {
        strcat(buf, " charm");
    }

    if (vector & AFF_FLYING) {
        strcat(buf, " flying");
    }

    if (vector & AFF_BERSERK) {
        strcat(buf, " berserk");
    }

    if (vector & AFF_CALM) {
        strcat(buf, " calm");
    }

    if (vector & AFF_HASTE) {
        strcat(buf, " haste");
    }

    if (vector & AFF_SLOW) {
        strcat(buf, " slow");
    }

    if (vector & AFF_PLAGUE) {
        strcat(buf, " plague");
    }

    if (vector & AFF_DARK_VISION) {
        strcat(buf, " dark_vision");
    }

    if (vector & AFF_WATER_BREATHING) {
        strcat(buf, " water_breathing");
    }

    if (vector & AFF_WATERWALK) {
        strcat(buf, " water_walk");
    }

    if (vector & AFF_GRANDEUR) {
        strcat(buf, " grandeur");
    }

    if (vector & AFF_MINIMATION) {
        strcat(buf, " minimation");
    }

    return (buf[0] != '\0') ? buf + 1 : "none";
}

void
affect_refresh(CHAR_DATA * ch, int sn, int duration) {
    AFFECT_DATA *paf;

    for (paf = ch->affected; paf != NULL; paf = paf->next) {
        if (paf->type == sn && paf->duration >= 0) {
            paf->duration = UMAX(duration, paf->duration);
        }
    }

    return;
}

