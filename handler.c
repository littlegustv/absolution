/* revision 1.1 - August 1 1999 - making it compilable under g++ */

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
#include "magic.h"
#include "recycle.h"
#include "clan.h"
#include "utils.h"


/* command procedures needed */
DECLARE_DO_FUN(do_return);
DECLARE_DO_FUN(do_crash);
/*
 * Local functions.
 */
bool is_reclass_spell(int index, int sn);
bool is_hushed(CHAR_DATA* ch, CHAR_DATA* vict);

// Externals
extern int saving_throw(CHAR_DATA *, CHAR_DATA *, int sn, int level,
	int diff, int stat, int damtype);
extern int using_skill_tattoo(CHAR_DATA *ch, int sn);

/* friend stuff -- for NPC's mostly */
bool
is_friend(CHAR_DATA * ch, CHAR_DATA * victim)
{
	if (is_same_group(ch, victim))
		return TRUE;


	if (!IS_NPC(ch))
		return FALSE;

	if (!IS_NPC(victim))
	{
		if (IS_SET(ch->off_flags, ASSIST_PLAYERS))
			return TRUE;
		else
			return FALSE;
	}

	if (IS_AFFECTED(ch, AFF_CHARM))
		return FALSE;

	if (IS_SET(ch->off_flags, ASSIST_ALL))
		return TRUE;

	if (ch->group && ch->group == victim->group)
		return TRUE;

	if (IS_SET(ch->off_flags, ASSIST_VNUM)
		&& ch->pIndexData == victim->pIndexData)
		return TRUE;

	if (IS_SET(ch->off_flags, ASSIST_RACE) && ch->race == victim->race)
		return TRUE;

	if (IS_SET(ch->off_flags, ASSIST_ALIGN)
		&& !IS_SET(ch->act, ACT_NOALIGN) && !IS_SET(victim->act, ACT_NOALIGN)
		&& ((IS_GOOD(ch) && IS_GOOD(victim))
			|| (IS_EVIL(ch) && IS_EVIL(victim))
			|| (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))))
		return TRUE;

	return FALSE;
}

/* returns number of people on an object */
int
count_users(OBJ_DATA * obj)
{
	CHAR_DATA *fch;
	int count = 0;

	if (obj->in_room == NULL)
		return 0;

	for (fch = obj->in_room->people; fch != NULL; fch = fch->next_in_room)
		if (fch->on == obj)
			count++;

	return count;
}

/* returns material number */
int
material_lookup(const char *name)
{
	return 0;
}

/* returns race number */
int
race_lookup(const char *name)
{
	int race;

	for (race = 0; race_table[race].name != NULL; race++)
	{
		if (LOWER(name[0]) == LOWER(race_table[race].name[0])
			&& !str_prefix(name, race_table[race].name))
			return race;
	}

	return 0;
}

int
liq_lookup(const char *name)
{
	int liq;

	for (liq = 0; liq_table[liq].liq_name != NULL; liq++)
	{
		if (LOWER(name[0]) == LOWER(liq_table[liq].liq_name[0])
			&& !str_prefix(name, liq_table[liq].liq_name))
			return liq;
	}

	return -1;
}

int
weapon_lookup(const char *name)
{
	int type;

	for (type = 0; weapon_table[type].name != NULL; type++)
	{
		if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
			&& !str_prefix(name, weapon_table[type].name))
			return type;
	}

	return -1;
}

int
weapon_type(const char *name)
{
	int type;

	for (type = 0; weapon_table[type].name != NULL; type++)
	{
		if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
			&& !str_prefix(name, weapon_table[type].name))
			return weapon_table[type].type;
	}

	return WEAPON_EXOTIC;
}


int
item_lookup(const char *name)
{
	int type;

	for (type = 0; item_table[type].name != NULL; type++)
	{
		if (LOWER(name[0]) == LOWER(item_table[type].name[0])
			&& !str_prefix(name, item_table[type].name))
			return item_table[type].type;
	}

	return -1;
}

char *
item_name(int item_type)
{
	int type;

	for (type = 0; item_table[type].name != NULL; type++)
	{
		if (item_table[type].type == item_type)
		{
			return item_table[type].name;
		}
	}
	return "none";
}

char *
weapon_name(int weapon_type)
{
	int type;

	for (type = 0; weapon_table[type].name != NULL; type++)
		if (weapon_type == weapon_table[type].type)
			return weapon_table[type].name;
	return "exotic";
}

int
attack_lookup(const char *name)
{
	int att;

	for (att = 0; attack_table[att].name != NULL; att++)
	{
		if (LOWER(name[0]) == LOWER(attack_table[att].name[0])
			&& !str_prefix(name, attack_table[att].name))
			return att;
	}

	return 0;
}

/* returns a flag for wiznet */
long
wiznet_lookup(const char *name)
{
	int flag;

	for (flag = 0; wiznet_table[flag].name != NULL; flag++)
	{
		if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0])
			&& !str_prefix(name, wiznet_table[flag].name))
			return flag;
	}

	return -1;
}

/* returns class number */
int
class_lookup(const char *name)
{
	int charClass;

	for (charClass = 0; charClass < MAX_CLASS; charClass++)
	{
		if (LOWER(name[0]) == LOWER(class_table[charClass].name[0])
			&& !str_prefix(name, class_table[charClass].name))
			return charClass;
	}

	return -1;
}

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.c */

int
check_immune(CHAR_DATA * ch, int dam_type)
{
	int immune, def;
	int bit;

	immune = -1;
	def = IS_NORMAL;

	if (dam_type == DAM_NONE)
		return immune;

	if (dam_type <= 3)
	{
		if (IS_SET(ch->imm_flags, IMM_WEAPON))
			def = IS_IMMUNE;
		else if (IS_SET(ch->res_flags, RES_WEAPON))
			def = IS_RESISTANT;
		else if (IS_SET(ch->vuln_flags, VULN_WEAPON))
			def = IS_VULNERABLE;
	}
	else
		/* magical attack */
	{
		if (IS_SET(ch->imm_flags, IMM_MAGIC))
			def = IS_IMMUNE;
		else if (IS_SET(ch->res_flags, RES_MAGIC))
			def = IS_RESISTANT;
		else if (IS_SET(ch->vuln_flags, VULN_MAGIC))
			def = IS_VULNERABLE;
	}

	/* set bits to check -- VULN etc. must ALL be the same or this will fail */
	switch (dam_type)
	{
	case (DAM_BASH):
		bit = IMM_BASH;
		break;
	case (DAM_PIERCE):
		bit = IMM_PIERCE;
		break;
	case (DAM_SLASH):
		bit = IMM_SLASH;
		break;
	case (DAM_FIRE):
		bit = IMM_FIRE;
		break;
	case (DAM_COLD):
		bit = IMM_COLD;
		break;
	case (DAM_LIGHTNING):
		bit = IMM_LIGHTNING;
		break;
	case (DAM_ACID):
		bit = IMM_ACID;
		break;
	case (DAM_POISON):
		bit = IMM_POISON;
		break;
	case (DAM_NEGATIVE):
		bit = IMM_NEGATIVE;
		break;
	case (DAM_HOLY):
		bit = IMM_HOLY;
		break;
	case (DAM_ENERGY):
		bit = IMM_ENERGY;
		break;
	case (DAM_RAIN):
		bit = IMM_RAIN;
		break;
	case (DAM_MENTAL):
		bit = IMM_MENTAL;
		break;
	case (DAM_DISEASE):
		bit = IMM_DISEASE;
		break;
	case (DAM_DROWNING):
		bit = IMM_DROWNING;
		break;
	case (DAM_LIGHT):
		bit = IMM_LIGHT;
		break;
	case (DAM_CHARM):
		bit = IMM_CHARM;
		break;
	case (DAM_SOUND):
		bit = IMM_SOUND;
		break;
	case (DAM_VORPAL):
		bit = IMM_VORPAL;
		break;
	default:
		return def;
	}

	// Trying to handle case where you are vuln AND resistant == normal

	if (IS_SET(ch->imm_flags, bit))
		immune = IS_IMMUNE;
	if (IS_SET(ch->res_flags, bit) && immune != IS_IMMUNE)
		immune = IS_RESISTANT;

	// Being vuln downgrades immunity and resistance
	if (IS_SET(ch->vuln_flags, bit))
	{
		if (immune == IS_IMMUNE)
			immune = IS_RESISTANT;
		else if (immune == IS_RESISTANT)
			immune = IS_NORMAL;
		else
			immune = IS_VULNERABLE;
	}

	if (immune == -1)
		return def;
	else
		return immune;
}

bool
is_clan(CHAR_DATA * ch)
{
	return clan_table[ch->clan].pkiller;
}

bool
is_same_clan(CHAR_DATA * ch, CHAR_DATA * victim)
{
	if (clan_table[ch->clan].independent)
		return FALSE;
	else
		return (ch->clan == victim->clan);
}

/* checks mob format */
bool
is_old_mob(CHAR_DATA * ch)
{
	if (ch->pIndexData == NULL)
		return FALSE;
	else if (ch->pIndexData->new_format)
		return FALSE;
	return TRUE;
}

/* for returning skill information */
int
get_skill(CHAR_DATA * ch, int sn)
{
	int skill;
	int no_restrictions = FALSE;
	AFFECT_DATA *paf=NULL;

	// This is for exotics. Monks can use them from level 1 up.
	if (sn == -1)
	{
		if(ch->charClass == class_lookup("monk"))
			skill = 40 + (3 * ch->level);
		else
			skill = ch->level * 3;
	}

	else if (sn < -1 || sn > MAX_SKILL)
	{
		bug("Bad sn %d in get_skill.", sn);
		skill = 0;
	}

	else if (!IS_NPC(ch))
        {
                /* check for stolen spell, silly level 54 thing */
		if (is_affected(ch, gsn_spell_stealing)) {
			paf = affect_find(ch->affected, gsn_spell_stealing);
        		if(paf != NULL && paf->modifier == sn) {
				skill = ch->pcdata->learned[sn];
				no_restrictions = TRUE;
			}
		}

		// Tattoos can get passed level restrictions as well
		if (using_skill_tattoo(ch, sn)) {
			skill = ch->pcdata->learned[sn];
			no_restrictions = TRUE;
		}

		// Generally as a result of using a charged item.
		if (ch->pcdata->any_skill == TRUE) {
			skill = ch->pcdata->learned[sn];
                        no_restrictions = TRUE;
		}

		// Skill is above your level
		if (!no_restrictions
		&& ch->level < skill_table[sn].skill_level[ch->charClass])
		{
                        skill = 0;
		}
		// Normal skill percentage used
                else {
                        skill = ch->pcdata->learned[sn];
		}
        }
	else
	{
		if (skill_table[sn].spell_fun != spell_null)
			skill = 50 + 2 * ch->level;

		else if ((sn == gsn_sneak || sn == gsn_hide) && (IS_SET(ch->act, ACT_THIEF) || IS_SET(ch->affected_by, AFF_SNEAK)))
			skill = ch->level * 2 + 20;

		else if ((sn == gsn_dodge && IS_SET(ch->off_flags, OFF_DODGE))
				 || (sn == gsn_parry && IS_SET(ch->off_flags, OFF_PARRY)))
			skill = ch->level * 2;

		else if (sn == gsn_shield_block && IS_SET(ch->act, ACT_WARRIOR) )
			skill = 10 + 2 * ch->level;

		else if (sn == gsn_second_attack
			&& (IS_SET(ch->act, ACT_WARRIOR) || IS_SET(ch->act, ACT_THIEF)))
			skill = 10 + 3 * ch->level;

		else if (sn == gsn_third_attack && IS_SET(ch->act, ACT_WARRIOR))
			skill = 4 * ch->level - 40;

		else if (sn == gsn_fourth_attack && IS_SET(ch->act, ACT_WARRIOR))
			skill = 3 * ch->level - 40;

		else if (sn == gsn_hand_to_hand)
			skill = 50 + 3 * ch->level;

		else if (sn == gsn_trip && IS_SET(ch->off_flags, OFF_TRIP))
			skill = 10 + 3 * ch->level;

		else if (sn == gsn_bash && IS_SET(ch->off_flags, OFF_BASH))
			skill = 10 + 3 * ch->level;

		else if (sn == gsn_disarm
				 && (IS_SET(ch->off_flags, OFF_DISARM)
					 || IS_SET(ch->act, ACT_WARRIOR)
					 || IS_SET(ch->act, ACT_THIEF)))
			skill = 2 * ch->level;

		else if (sn == gsn_berserk && IS_SET(ch->off_flags, OFF_BERSERK))
			skill = 3 * ch->level;

		else if (sn == gsn_kick && IS_SET(ch->off_flags, OFF_KICK))
			skill = 10 + 3 * ch->level;

		else if (sn == gsn_dirt_kicking
			     && (IS_SET(ch->act, ACT_THIEF)
				  || IS_SET(ch->off_flags, OFF_KICK_DIRT)))
			skill = 10 + 3 * ch->level;

		else if (sn == gsn_backstab
			     && (IS_SET(ch->act, ACT_THIEF)
				  || IS_SET(ch->off_flags, OFF_BACKSTAB)))
			skill = 20 + 2 * ch->level;

		else if (sn == gsn_rescue
			     && (IS_SET(ch->act, ACT_WARRIOR)
				  || IS_SET(ch->off_flags, OFF_RESCUE)))
			skill = 40 + ch->level;

		else if (sn == gsn_recall)
			skill = 40 + ch->level;

		else if (sn == gsn_sword
				 || sn == gsn_dagger
				 || sn == gsn_spear
				 || sn == gsn_mace
				 || sn == gsn_axe
				 || sn == gsn_flail
				 || sn == gsn_whip
				 || sn == gsn_polearm)
			skill = 50 * ch->level / 2;

		else
			skill = 0;
	}

	/* apply reclass penalties */
	if (ch->reclass > 0)
	{
		if (ch->reclass == reclass_lookup("heretic") ||
			ch->reclass == reclass_lookup("necromancer"))
		{
			if (is_reclass_spell(ch->reclass, sn) &&
				ch->alignment > -350)
			{
				Cprintf(ch, "Your soul is too pure to use this power.\n\r");
				skill = 0;
			}
		}

		if (ch->reclass == reclass_lookup("exorcist"))
		{
			if (is_reclass_spell(ch->reclass, sn) &&
				ch->alignment < 350)
			{
				Cprintf(ch, "Your soul isn't pure enough to use this power.\n\r");
				skill = 0;
			}
		}

		if (ch->reclass == reclass_lookup("hierophant"))
		{
			if (is_reclass_spell(ch->reclass, sn) &&
				(ch->alignment > 350 || ch->alignment < -350))
			{
				Cprintf(ch, "You lack the sense of balance to use this power.\n\r");
				skill = 0;
			}
		}
	}

	skill = URANGE(0, skill, 100);

	// Pain touch disrupts all skills (but not spells)
	if (is_affected(ch, gsn_pain_touch)
	&& skill_table[sn].spell_fun == spell_null)
		skill = 9 * skill / 10;

	if (ch->daze > 0)
	{
		skill = 2 * skill / 3;
	}

	if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
		skill = 9 * skill / 10;

	if (!IS_NPC(ch)
	&& ch->race == race_lookup("dwarf")
	&& ch->remort && ch->rem_sub == 1
	&& ch->pcdata->condition[COND_DRUNK] > 10)
		skill = skill * 11 / 10;

	return URANGE(0, skill, 100);
}

/* for returning weapon information */
int
get_weapon_sn(CHAR_DATA * ch, int location)
{
	OBJ_DATA *wield = NULL;
	int sn;

	if(location == WEAR_WIELD)
		wield = get_eq_char(ch, WEAR_WIELD);
	if(location == WEAR_DUAL)
		wield = get_eq_char(ch, WEAR_DUAL);
        if(location == WEAR_RANGED)
                wield = get_eq_char(ch, WEAR_RANGED);
	if (wield == NULL || wield->item_type != ITEM_WEAPON)
		sn = gsn_hand_to_hand;
	else
		switch (wield->value[0])
		{
		default:
			sn = -1;
			break;
		case (WEAPON_SWORD):
			sn = gsn_sword;
			break;
		case (WEAPON_DAGGER):
			sn = gsn_dagger;
			break;
		case (WEAPON_SPEAR):
			sn = gsn_spear;
			break;
		case (WEAPON_MACE):
			sn = gsn_mace;
			break;
		case (WEAPON_AXE):
			sn = gsn_axe;
			break;
		case (WEAPON_FLAIL):
			sn = gsn_flail;
			break;
		case (WEAPON_WHIP):
			sn = gsn_whip;
			break;
		case (WEAPON_POLEARM):
			sn = gsn_polearm;
			break;
		case (WEAPON_KATANA):
			sn = gsn_katana;
			break;
		case (WEAPON_RANGED):
			sn = gsn_marksmanship;
			break;
		}
	return sn;
}

int
get_weapon_skill(CHAR_DATA * ch, int sn)
{
	int skill;

	/* -1 is exotic */
	if (IS_NPC(ch))
	{
		if (sn == -1)
			skill = 3 * ch->level;
		else if (sn == gsn_hand_to_hand)
			skill = 40 + 2 * ch->level;
		else
			skill = 40 + 5 * ch->level / 2;
	}

	else
	{
		// Exotics
		if (sn == -1) {
			if(ch->charClass == class_lookup("monk"))
				skill = 40 + (3 * ch->level);
			else
				skill = 3 * ch->level;
		}
		else
			skill = ch->pcdata->learned[sn];
	}

	return URANGE(0, skill, 100);
}

/* used to de-screw characters */
void
reset_char(CHAR_DATA * ch)
{
	int loc, mod, stat;
	OBJ_DATA *obj;
	AFFECT_DATA *af;
	int i;

	if (IS_NPC(ch))
		return;

/*
	{
		obj = get_eq_char(ch, loc);
		if (obj == NULL)
			continue;

		if (!obj->enchanted) 
		{
			for (af = obj->pIndexData->affected; af != NULL; af = af->next)
			{
				mod = af->modifier;
				// some affects only take place at wierd times.
                               	if(IS_SET(af->bitvector, AFF_DAYLIGHT)
                               	&& (time_info.hour < 8 || time_info.hour >= 20))
                                       	continue;

                               	if(IS_SET(af->bitvector, AFF_DARKLIGHT)
                               	&& (time_info.hour >= 8 && time_info.hour < 20))
                                       	continue;

				switch (af->location)
				{
				case APPLY_SEX:
					ch->sex -= mod;
					if (ch->sex < 0 || ch->sex > 2)
						ch->sex = IS_NPC(ch) ?
							0 :
							ch->pcdata->true_sex;
					break;
				case APPLY_AGE:
					ch->played -= (mod * 3600);
					break;
				case APPLY_MANA:
					ch->max_mana_bonus -= mod;
					break;
				case APPLY_HIT:
					ch->max_hit_bonus -= mod;
					break;
				case APPLY_MOVE:
					ch->max_move_bonus -= mod;
					break;
				}
			}
		}
		for (af = obj->affected; af != NULL; af = af->next)
		{
			mod = af->modifier;
			// some affects only take place at wierd times.
                        if(IS_SET(af->bitvector, AFF_DAYLIGHT)
                        && (time_info.hour < 8 || time_info.hour >= 20))
                                continue;

                        if(IS_SET(af->bitvector, AFF_DARKLIGHT)
                        && (time_info.hour >= 8 && time_info.hour < 20))
                                continue;

			switch (af->location)
			{
			case APPLY_AGE:
				ch->played -= (mod * 3600);
				break;
			case APPLY_SEX:
				ch->sex -= mod;
				break;
			case APPLY_MANA:
				ch->max_mana_bonus -= mod;
				break;
			case APPLY_HIT:
				ch->max_hit_bonus -= mod;
				break;
			case APPLY_MOVE:
				ch->max_move_bonus -= mod;
				break;
			}
		}
	}
	if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
	{
		if (ch->sex > 0 && ch->sex < 3)
			ch->pcdata->true_sex = ch->sex;
		else
			ch->pcdata->true_sex = 0;
	}

*/

	// now restore the character to his/her true condition 
	ch->air_supply = 0;

	for (stat = 0; stat < MAX_STATS; stat++)
		ch->mod_stat[stat] = 0;

	if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
		ch->pcdata->true_sex = 0;
	ch->sex = ch->pcdata->true_sex;
//	ch->max_hit = ch->pcdata->perm_hit;
//	ch->max_mana = ch->pcdata->perm_mana;
//	ch->max_move = ch->pcdata->perm_move;

	for (i = 0; i < 4; i++)
		ch->armor[i] = 100;

	ch->hitroll = 0;
	ch->damroll = 0;
	ch->saving_throw = 0;

	/* Adjustmnent for Damroll for remort dragons - Coded by Del */
	if (IS_DRAGON(ch) && ch->remort > 0)
		ch->damroll = ch->level / 6;

	/* now start adding back the effects */
	for (loc = 0; loc < MAX_WEAR; loc++)
	{
		obj = get_eq_char(ch, loc);
		if (obj == NULL)
			continue;
		for (i = 0; i < 4; i++)
			ch->armor[i] -= apply_ac(obj, loc, i);

		if (!obj->enchanted)
			for (af = obj->pIndexData->affected; af != NULL; af = af->next)
			{
				if(IS_SET(af->bitvector, AFF_DAYLIGHT)
                                && (time_info.hour < 8 || time_info.hour >= 20))
                                                continue;

                                if(IS_SET(af->bitvector, AFF_DARKLIGHT)
                                && (time_info.hour >= 8 && time_info.hour < 20))
                                                continue;

				mod = af->modifier;
				switch (af->location)
				{
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
				case APPLY_AGE:
					ch->played += (mod * 3600);
					break;

				case APPLY_SEX:
					ch->sex += mod;
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
					for (i = 0; i < 4; i++)
						ch->armor[i] += mod;
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
			}

		for (af = obj->affected; af != NULL; af = af->next)
		{

			if(IS_SET(af->bitvector, AFF_DAYLIGHT)
                        && (time_info.hour < 8 || time_info.hour >= 20))
                                        continue;

                        if(IS_SET(af->bitvector, AFF_DARKLIGHT)
                        && (time_info.hour >= 8 && time_info.hour < 20))
                                        continue;

			mod = af->modifier;
			switch (af->location)
			{
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
			case APPLY_AGE:
				ch->played += (mod * 3600);

			case APPLY_SEX:
				ch->sex += mod;
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
				for (i = 0; i < 4; i++)
					ch->armor[i] += mod;
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
		}
	}

	/* now add back spell effects */
	for (af = ch->affected; af != NULL; af = af->next)
	{
		mod = af->modifier;
		switch (af->location)
		{
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
		case APPLY_AGE:
			ch->played += (mod * 3600);
			break;

		case APPLY_SEX:
			ch->sex += mod;
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
			for (i = 0; i < 4; i++)
				ch->armor[i] += mod;
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
	}

	/* make sure sex is RIGHT!!!! */
	if (ch->sex < 0 || ch->sex > 2)
		ch->sex = ch->pcdata->true_sex;

	if (ch->race == race_lookup("human") && ch->remort > 0 && ch->rem_sub == 1)
		SET_BIT(ch->res_flags, RES_CHARM);
	if (ch->race == race_lookup("human") && ch->remort > 0 && ch->rem_sub == 2)
		SET_BIT(ch->res_flags, RES_POISON);


}


/*
 * Retrieve a character's trusted level for permission checking.
 */
int
get_trust(CHAR_DATA * ch)
{
	if (ch == NULL)
		return 0;

	if (ch->desc != NULL && ch->desc->original != NULL)
		ch = ch->desc->original;

	if (ch->trust)
		return ch->trust;

	if (IS_NPC(ch) && ch->level >= LEVEL_HERO)
		return LEVEL_HERO - 1;
	else
		return ch->level;
}


/*
 * Retrieve a character's age.
 */
int
get_age(CHAR_DATA * ch)
{
	return 17 + (ch->played / 72000);
}

int get_hours(CHAR_DATA* ch)
{
	return ch->played / 3600;
}

int get_perm_hours(CHAR_DATA* ch)
{
	return ch->played_perm / 3600;
}

int get_race_curr_stat(CHAR_DATA * ch, int stat)
{
        int max;

        if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
                max = 30;

        else
        {
                max = pc_race_table[ch->race].max_stats[stat] + 4;

                if (class_table[ch->charClass].attr_prime == stat)
                        max += 2;

                if (ch->race == race_lookup("human"))
                        max += 1;

                max = UMIN(max, 25);
        }

        return URANGE(3, ch->perm_stat[stat] + ch->mod_stat[stat], max);
}

/* command for retrieving stats */
int
get_curr_stat(CHAR_DATA * ch, int stat)
{
	int max;

	if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
		max = 30;

	else
	{
		max = pc_race_table[ch->race].max_stats[stat] + 4;

		if (class_table[ch->charClass].attr_prime == stat)
			max += 2;

		if (ch->race == race_lookup("human"))
			max += 1;

		max = UMIN(max, 25);
		max += ch->max_stat_bonus[stat];
                max = UMIN(max, 30);
	}

	return URANGE(3, ch->perm_stat[stat] + ch->mod_stat[stat], max);
}

// Ignores crafted eq bonuses, use this for hp/mana gains!!
int get_race_max_stat(CHAR_DATA * ch, int stat)
{
        int max = 0;

        if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
                max = 25;
        else
        {
                max = pc_race_table[ch->race].max_stats[stat] + 4;

                if (class_table[ch->charClass].attr_prime == stat)
                        max += 2;

                if (ch->race == race_lookup("human"))
                        max += 1;

                max = UMIN(max, 25);
        }

	return max;
}

/* command for retrieving stats */
int
get_max_stat(CHAR_DATA * ch, int stat)
{
	int max;

	if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
		max = 30;

	else
	{
		max = pc_race_table[ch->race].max_stats[stat] + 4;

		if (class_table[ch->charClass].attr_prime == stat)
			max += 2;

		if (ch->race == race_lookup("human"))
			max += 1;

		max = UMIN(max, 25);
		max += ch->max_stat_bonus[stat];
		max = UMIN(max, 30);

	}

	return max;
}

/* command for returning max training score */
int
get_max_train(CHAR_DATA * ch, int stat)
{
	int max;

	if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
		return 25;

	max = pc_race_table[ch->race].max_stats[stat];
	if (class_table[ch->charClass].attr_prime == stat)
	{
		if (ch->race == race_lookup("human"))
			max += 3;
		else
			max += 2;
	}

	return UMIN(max, 25);
}


int get_carry_weight(CHAR_DATA*ch)
{
	return ch->carry_weight + ch->silver/10 + ch->gold * 2 / 5;
}

/*
 * Retrieve a character's carry capacity.
 */
int
can_carry_n(CHAR_DATA * ch)
{
	if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
		return 1000;

	if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET))
		return 0;

	if (ch->reclass == reclass_lookup("warlord") ||
		ch->reclass == reclass_lookup("barbarian") ||
		ch->reclass == reclass_lookup("templar") ||
		ch->reclass == reclass_lookup("cavalier") ||
		ch->reclass == reclass_lookup("hermit") ||
		ch->reclass == reclass_lookup("bounty hunter"))
	{
		return MAX_WEAR + 2 * get_curr_stat(ch, STAT_DEX) + (ch->level * 2);
	}

	return MAX_WEAR + 2 * get_curr_stat(ch, STAT_DEX) + ch->level;
}



/*
 * Retrieve a character's carry capacity.
 */
int
can_carry_w(CHAR_DATA * ch)
{
	if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
		return 10000000;

	if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET))
		return 0;

	if (ch->reclass == reclass_lookup("warlord") ||
		ch->reclass == reclass_lookup("barbarian") ||
		ch->reclass == reclass_lookup("templar") ||
		ch->reclass == reclass_lookup("cavalier") ||
		ch->reclass == reclass_lookup("hermit") ||
		ch->reclass == reclass_lookup("bounty hunter"))
	{
		return str_app[get_curr_stat(ch, STAT_STR)].carry * 10 + ch->level * 50;
	}

	return str_app[get_curr_stat(ch, STAT_STR)].carry * 10 + ch->level * 25;
}



/*
 * See if a string is one of the names of an object.
 */

bool
is_name(char *str, char *namelist) {
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0') {
        return FALSE;
    }

    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0') {
        return FALSE;
    }

    string = str;
    /* we need ALL parts of string to match part of namelist */
    for (;;) {
        /* start parsing string */
        str = one_argument(str, part);

        if (part[0] == '\0') {
            return TRUE;
        }

        /* check to see if this is part of namelist */
        list = namelist;
        for (;;) {
            /* start parsing namelist */
            list = one_argument(list, name);
            if (name[0] == '\0') {
                /* this name was not found */
                return FALSE;
            }

            if (!str_prefix(string, name)) {
                /* full pattern match */
                return TRUE;
            }

            if (!str_prefix(part, name)) {
                break;
            }
        }
    }
}

bool
is_exact_name(char *str, char *namelist)
{
	char name[MAX_INPUT_LENGTH];

	if (namelist == NULL)
		return FALSE;

	for (;;)
	{
		namelist = one_argument(namelist, name);
		if (name[0] == '\0')
			return FALSE;
		if (!str_cmp(str, name))
			return TRUE;
	}
}


/*
 * Move a char out of a room.
 */
void
char_from_room(CHAR_DATA * ch)
{
	OBJ_DATA *obj;

	if (ch == NULL)
		return;

	if (ch->in_room == NULL)
	{
		return;
	}

	if (!IS_NPC(ch))
		--ch->in_room->area->nplayer;


	if (((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
		 && obj->item_type == ITEM_LIGHT
		 && obj->value[2] != 0
		 && ch->in_room->light > 0))
		--ch->in_room->light;


	if (ch == ch->in_room->people)
	{
		ch->in_room->people = ch->next_in_room;
	}
	else
	{
		CHAR_DATA *prev;

		for (prev = ch->in_room->people; prev; prev = prev->next_in_room)
		{
			if (prev->next_in_room == ch)
			{
				if (prev != ch->next_in_room)
				{
					prev->next_in_room = ch->next_in_room;
					break;
				}
			}
		}

		if (prev == NULL) {
			bug("Char_from_room: ch not found.", 0);
		}
	}

	if (ch->in_room->vnum != ROOM_VNUM_LIMBO &&
		ch->in_room->vnum != ROOM_VNUM_LIMBO_DOMINIA)
		ch->was_in_room = ch->in_room;

	ch->in_room = NULL;
	ch->next_in_room = NULL;
	ch->on = NULL;				/* sanity check! */

	/* see what this does */
	if (ch->position == POS_FIGHTING)
		stop_fighting(ch, TRUE);
	return;
}



/*
 * Move a char into a room.
 */
void
char_to_room(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex)
{
	OBJ_DATA *obj;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int count = 0;
	AFFECT_DATA *paff;

	// Builders, don't run away!
	if (!IS_NPC(ch) && ch->level == 54 && !IS_BUILDER(ch, pRoomIndex->area))
	{
		pRoomIndex = get_room_index(ROOM_VNUM_LIMBO);
		Cprintf(ch, "Builders cannot leave their area.\n\r");
	}
	if (pRoomIndex == NULL)
	{
		ROOM_INDEX_DATA *room;

		bug("Char_to_room: NULL.", 0);

		if ((room = get_room_index(ROOM_VNUM_TEMPLE)) != NULL)
			char_to_room(ch, room);

		return;
	}


	// Its the stupidity check.
	// If they are already in the room, don't do it again.
	if(ch->in_room == pRoomIndex)
		return;

	ch->seen = 0;
	ch->in_room = pRoomIndex;
	ch->next_in_room = pRoomIndex->people;
	pRoomIndex->people = ch;

	if (pRoomIndex == get_room_index(ROOM_VNUM_LIMBO) ||
	    pRoomIndex == get_room_index(ROOM_VNUM_LIMBO_DOMINIA))
	{
		ch->next_in_room = NULL;
		pRoomIndex->people = NULL;
	}

	if (!IS_NPC(ch))
	{
		if (ch->in_room->area->empty)
		{
			ch->in_room->area->empty = FALSE;
			ch->in_room->area->age = 0;
		}
		++ch->in_room->area->nplayer;
	}

	if (((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
		 && obj->item_type == ITEM_LIGHT
		 && obj->value[2] != 0))
	{
		++ch->in_room->light;
	}


	// Can't move while working on a craft!
	if(ch->craft_timer < 0) {
        	Cprintf(ch, "You move and interrupt your work! It's ruined!\n\r");
        	ch->craft_timer = 30;
        	ch->craft_target = 0;
	}

	// Okay, if we just withstood death, stop here....
	// Bypass all char to room skill checks
	if(ch->hit == 100
	&& is_affected(ch, gsn_withstand_death))
		return;

	if (IS_AFFECTED(ch, AFF_PLAGUE))
	{
		AFFECT_DATA *af, plague;

		for (af = ch->affected; af != NULL; af = af->next)
		{
			if (af->type == gsn_plague)
				break;
		}

		if (af == NULL)
		{
			REMOVE_BIT(ch->affected_by, AFF_PLAGUE);
			return;
		}

		if (af->level == 1)
		{
			return;
		}

		plague.where = TO_AFFECTS;
		plague.type = gsn_plague;
		plague.level = af->level - 1;
		plague.duration = number_range(1, plague.level / 2);
		plague.location = APPLY_STR;
		plague.modifier = -5;
		plague.bitvector = AFF_PLAGUE;

		for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
		{
			if (number_percent() <= 2
			&& !IS_IMMORTAL(vch)
			&& (!IS_SET(vch->act, ACT_IS_WIZI) && IS_NPC(vch))
			&& !IS_AFFECTED(vch, AFF_PLAGUE)
		 	&& check_immune(vch, DAM_DISEASE) != IS_IMMUNE)
			{
				Cprintf(vch, "You feel hot and feverish.\n\r");
				act("$n shivers and looks very ill.", vch, NULL, NULL, TO_ROOM, POS_RESTING);
				affect_join(vch, &plague);
			}
		}
	}

	if (ch->in_room->vnum != ROOM_VNUM_LIMBO
		&& ch->in_room->vnum != ROOM_VNUM_LIMBO_DOMINIA)
	{
		count = 0;

		for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
		{
			vch_next = vch->next_in_room;

			if (is_affected(vch, gsn_retribution)
				&& !IS_NPC(ch)
				&& !IS_NPC(vch)
				&& is_clan(ch)
				&& is_clan(vch)
				&& can_see(vch, ch)
				&& (vch->clan != ch->clan)
				&& vch != ch
				&& !is_safe(vch, ch)
				&& !is_safe(ch, vch)
			    && (IS_SET(ch->act, PLR_KILLER) || IS_SET(ch->act, PLR_THIEF))
				&& get_skill(vch, gsn_retribution) > 0
				&& (number_percent() < (get_skill(vch, gsn_retribution) / 2))
				&& !is_affected(ch, gsn_jump))
			{
				check_improve(vch, gsn_retribution, TRUE, 3);
				Cprintf(vch, "Your justice befalls on %s!\n\r", ch->name);
				Cprintf(ch, "The justice of %s befalls upon you!\n\r", vch->name);
				multi_hit(vch, ch, TYPE_UNDEFINED);
				break;
			}
			count++;
			if (count > 1000)
			{
				log_string("INFINTE loop in Retribution lookup [handler.c]");
				break;
			}
		}

		if (room_is_affected (ch->in_room, gsn_alarm_rune))
		{
		        for (vch = char_list; vch != NULL; vch = vch->next)
		        {
		                if ((paff = affect_find(vch->affected, gsn_alarm_rune)) != NULL
		                     && paff->modifier == ch->in_room->vnum
				     && vch != ch
				     && !IS_NPC(ch)
				     && !IS_IMMORTAL(ch)
				     && number_percent() < get_skill(vch, gsn_alarm_rune)
				     && can_see(vch, ch))
		                {
		                        act("{R$N has triggered your alarm rune!{x", vch, NULL, ch, TO_CHAR, POS_RESTING);
		                }
		        }
		}

		for (vch = char_list; vch != NULL; vch = vch->next)
		{
			if (ch != NULL && vch != NULL
			&& ch->in_room != NULL
			&& ch->was_in_room != NULL
			&& vch->in_room != NULL
			&& ch->in_room->area != ch->was_in_room->area
			&& vch->in_room->area == ch->in_room->area
			&& !IS_NPC(vch) && !IS_NPC(ch)
			&& !IS_IMMORTAL(ch)
			&& number_percent() < get_skill(vch,gsn_presence)
                        && can_see(vch, ch)
			&& vch != ch
			&& !is_hushed(vch, ch)
			&& vch->desc != NULL
			&& !(vch->desc->connected >= CON_NOTE_TO
			     && vch->desc->connected <= CON_NOTE_FINISH))
			{
				Cprintf(vch, "{GYou feel the presence of %s in the area.{x\n\r", ch->name);
				check_improve(vch, gsn_presence, TRUE, 4);
			}

		}
	}
	return;
}



/*
 * Give an obj to a char.
 */
void
obj_to_char(OBJ_DATA * obj, CHAR_DATA * ch)
{

	obj->next_content = ch->carrying;
	ch->carrying = obj;
	obj->carried_by = ch;
	obj->in_room = NULL;
	obj->in_obj = NULL;
	ch->carry_number += get_obj_number(obj);
	ch->carry_weight += get_obj_weight(obj);
}



/*
 * Take an obj from its character.
 */
void
obj_from_char(OBJ_DATA * obj)
{
	CHAR_DATA *ch;

	if ((ch = obj->carried_by) == NULL)
	{
		bug("Obj_from_char: null ch.", 0);
		return;
	}

	if (IS_SET(obj->extra_flags, ITEM_ROT_DEATH))
        {
              obj->timer = number_range(5, 10);
              REMOVE_BIT(obj->extra_flags, ITEM_ROT_DEATH);
        }

	if (obj->wear_loc != WEAR_NONE)
		unequip_char(ch, obj);

	if (ch->carrying == obj)
	{
		ch->carrying = obj->next_content;
	}
	else
	{
		OBJ_DATA *prev;

		for (prev = ch->carrying; prev != NULL; prev = prev->next_content)
		{
			if (prev->next_content == obj)
			{
				prev->next_content = obj->next_content;
				break;
			}
		}

		if (prev == NULL)
			bug("Obj_from_char: obj not in list.", 0);
	}

	obj->carried_by = NULL;
	obj->next_content = NULL;
	ch->carry_number -= get_obj_number(obj);
	ch->carry_weight -= get_obj_weight(obj);

	return;
}

/*
 * Removes an object from a character and makes it
 * unavailable for awhile.
 */
void disarm_char_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
	OBJ_DATA *last_obj = NULL;

	if(ch == NULL || obj == NULL)
		return;

	obj_from_char(obj);

	if(ch->disarmed == NULL) {
		ch->disarmed = obj;
	}
	else {
		// Get the last disarmed object.
		for(last_obj = ch->disarmed; last_obj != NULL; last_obj = last_obj->next_content) {}	

		last_obj->next_content = obj;
	}

	return;
}


/*
 * Find the ac value of an obj, including position effect.
 */
int
apply_ac(OBJ_DATA * obj, int iWear, int type)
{
	if (obj->item_type != ITEM_ARMOR)
		return 0;

	switch (iWear)
	{
	case WEAR_LIGHT:
		return obj->value[type];
	case WEAR_FINGER_L:
		return obj->value[type];
	case WEAR_FINGER_R:
		return obj->value[type];
	case WEAR_BODY:
		return 2 * obj->value[type];
	case WEAR_HEAD:
		return obj->value[type] ;
	case WEAR_LEGS:
		return obj->value[type];
	case WEAR_FEET:
		return obj->value[type];
	case WEAR_HANDS:
		return obj->value[type];
	case WEAR_ARMS:
		return obj->value[type];
	case WEAR_SHIELD:
		return obj->value[type] * 3 / 2;
	case WEAR_NECK_1:
		return obj->value[type];
	case WEAR_NECK_2:
		return obj->value[type];
	case WEAR_ABOUT:
		return obj->value[type] * 3 / 2;
	case WEAR_WAIST:
		return obj->value[type];
	case WEAR_WRIST_L:
		return obj->value[type];
	case WEAR_WRIST_R:
		return obj->value[type];
	case WEAR_HOLD:
		return obj->value[type];
	case WEAR_FLOAT:
		return obj->value[type];
	case WEAR_FLOAT_2:
		return obj->value[type];
	case WEAR_HEAD_2:
		return obj->value[type];
	}

	return 0;
}



/*
 * Find a piece of eq on a character.
 */
OBJ_DATA *
get_eq_char(CHAR_DATA * ch, int iWear)
{
	OBJ_DATA *obj;

	if (ch == NULL)
		return NULL;

	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	{
		if (obj->wear_loc == iWear)
			return obj;
	}

	return NULL;
}



/*
 * Equip a char with an obj.
 */
void
equip_char(CHAR_DATA * ch, OBJ_DATA * obj, int iWear)
{
	AFFECT_DATA *paf;
	int i;

	if (get_eq_char(ch, iWear) != NULL)
	{
		bug("Equip_char: already equipped (%d).", iWear);
		return;
	}

	if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
		|| (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
		|| (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)))
	{
		/*
		 * Thanks to Morgenes for the bug fix here!
		 */
		act("You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
		return;
	}

	for (i = 0; i < 4; i++)
		ch->armor[i] -= apply_ac(obj, iWear, i);

	obj->wear_loc = iWear;

	if (!obj->enchanted) {
		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) {
			if(paf->type == gsn_paint_power
                	&& paf->modifier == TATTOO_LEARN_SPELL) {
                        	ch->pcdata->learned[paf->extra] = UMIN(2 * ch->level, 100);
                	}
			if(paf->where != TO_RESIST
			&& paf->where != TO_IMMUNE
			&& paf->where != TO_VULN
			&& paf->location != APPLY_SPELL_AFFECT) {
				// some affects only take place at wierd times.
				if(IS_SET(paf->bitvector, AFF_DAYLIGHT)
		        	&& (time_info.hour < 8 || time_info.hour >= 20))
		                	continue;

		        	if(IS_SET(paf->bitvector, AFF_DARKLIGHT)
		        	&& (time_info.hour >= 8 && time_info.hour < 20))
		                	continue;

				affect_modify(ch, paf, TRUE);

			}

		}
	}

	for (paf = obj->affected; paf != NULL; paf = paf->next) {
		if(paf->type == gsn_paint_power
        	&& paf->modifier == TATTOO_LEARN_SPELL) {
                	ch->pcdata->learned[paf->extra] = UMIN(2 * ch->level, 100);
        	}
		if(paf->where == TO_RESIST
		|| paf->where == TO_IMMUNE
		|| paf->where == TO_VULN
		|| paf->location == APPLY_SPELL_AFFECT)
			continue;

		// some affects only take place at wierd times.
		if(IS_SET(paf->bitvector, AFF_DAYLIGHT)
        	&& (time_info.hour < 8 || time_info.hour >= 20))
                	continue;

        	if(IS_SET(paf->bitvector, AFF_DARKLIGHT)
        	&& (time_info.hour >= 8 && time_info.hour < 20))
                	continue;

		affect_modify(ch, paf, TRUE);

	}
	if (obj->item_type == ITEM_LIGHT
		&& obj->value[2] != 0
		&& ch->in_room != NULL)
		++ch->in_room->light;

	return;
}



/*
 * Unequip a char with an obj.
 */
void
unequip_char(CHAR_DATA * ch, OBJ_DATA * obj)
{
	AFFECT_DATA *paf = NULL;
	int i;
	char buf[255];

	if (obj->wear_loc == WEAR_NONE)
	{
		bug("Unequip_char: already unequipped.", 0);
		return;
	}

	for (i = 0; i < 4; i++)
		ch->armor[i] += apply_ac(obj, obj->wear_loc, i);
	obj->wear_loc = -1;

	if (!obj->enchanted)
	{
		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
		{
			if(paf->type == gsn_paint_power
                        && paf->modifier == TATTOO_LEARN_SPELL) {
                                ch->pcdata->learned[paf->extra] = 0;

				// Log a possible bug
				if(paf->extra == 0) {
					sprintf(buf, "%s had a spell tattoo with no sn. Bugged tattoo.\n\r", ch->name );
					log_string("%s", buf);
				}
                        }


			// some affects only take place at wierd times.
			if(IS_SET(paf->bitvector, AFF_DAYLIGHT)
	        	&& (time_info.hour < 8 || time_info.hour >= 20))
	                	continue;

	        	if(IS_SET(paf->bitvector, AFF_DARKLIGHT)
	        	&& (time_info.hour >= 8 && time_info.hour < 20))
	                	continue;

			affect_modify(ch, paf, FALSE);
			affect_check(ch, paf->where, paf->bitvector);
		}
	}

	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
		if(paf->type == gsn_paint_power
                && paf->modifier == TATTOO_LEARN_SPELL) {
                        ch->pcdata->learned[paf->extra] = 0;
			if(paf->extra == 0) {
				sprintf(buf, "%s had a spell tattoo with no sn. Bugged tattoo.", ch->name);
				log_string("%s", buf);
			}
                }

		// some affects only take place at wierd times.
		if(IS_SET(paf->bitvector, AFF_DAYLIGHT)
        	&& (time_info.hour < 8 || time_info.hour >= 20))
                	continue;

        	if(IS_SET(paf->bitvector, AFF_DARKLIGHT)
        	&& (time_info.hour >= 8 && time_info.hour < 20))
                	continue;

		affect_modify(ch, paf, FALSE);
		affect_check(ch, paf->where, paf->bitvector);
	}

	if (obj->item_type == ITEM_LIGHT
		&& obj->value[2] != 0
		&& ch->in_room != NULL
		&& ch->in_room->light > 0)
		--ch->in_room->light;

	return;
}



/*
 * Count occurrences of an obj in a list.
 */
int
count_obj_list(OBJ_INDEX_DATA * pObjIndex, OBJ_DATA * list)
{
	OBJ_DATA *obj;
	int nMatch;

	nMatch = 0;
	for (obj = list; obj != NULL; obj = obj->next_content)
	{
		if (obj->pIndexData == pObjIndex)
			nMatch++;
	}

	return nMatch;
}



/*
 * Count occurrences of an obj in a list by name
 * returns:
 *   0 if that object totally doesn't exist in the list
 *   x if the character can see x objects
 *   -x if there are x objects, but the character can't see any of 'em
 */
int
count_obj_list_by_name(CHAR_DATA *ch, char *argument, OBJ_DATA *list)
{
	int nMatchVisible = 0;
	int nMatchInvisible = 0;
	OBJ_DATA *obj;

	for (obj = list; obj != NULL; obj = obj->next_content)
	{
		if (is_name(argument, obj->name))
		{
			if (can_see_obj(ch, obj))
			{
				nMatchVisible++;
			}
			else
			{
				nMatchInvisible--;
			}
		}
	}

	return (nMatchVisible > 0) ? nMatchVisible : (nMatchInvisible < 0) ? nMatchInvisible : 0;
}



int
count_obj_carry_by_name(CHAR_DATA *ch, char *argument, CHAR_DATA *viewer)
{
   int nMatchVisible = 0;
   int nMatchInvisible = 0;
   OBJ_DATA *obj;

   for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
   {
      if (is_name(argument, obj->name) && obj->wear_loc == WEAR_NONE)
      {
         if (can_see_obj(viewer, obj))
         {
            nMatchVisible++;
         }
         else
         {
            nMatchInvisible--;
         }
      }
   }

   return (nMatchVisible > 0) ? nMatchVisible : (nMatchInvisible < 0) ? nMatchInvisible : 0;
}





/*
 * Move an obj out of a room.
 */
void
obj_from_room(OBJ_DATA * obj)
{
	ROOM_INDEX_DATA *in_room;
	CHAR_DATA *ch;

	if ((in_room = obj->in_room) == NULL)
	{
		bug("obj_from_room: NULL.", 0);
		return;
	}

	for (ch = in_room->people; ch != NULL; ch = ch->next_in_room)
		if (ch->on == obj)
			ch->on = NULL;

	if (obj == in_room->contents)
	{
		in_room->contents = obj->next_content;
	}
	else
	{
		OBJ_DATA *prev;

		for (prev = in_room->contents; prev; prev = prev->next_content)
		{
			if (prev->next_content == obj)
			{
				prev->next_content = obj->next_content;
				break;
			}
		}

		if (prev == NULL)
		{
			bug("Obj_from_room: obj not found.", 0);
			return;
		}
	}

	obj->in_room = NULL;
	obj->next_content = NULL;
	return;
}



/*
 * Move an obj into a room.
 */
void
obj_to_room(OBJ_DATA * obj, ROOM_INDEX_DATA * pRoomIndex)
{
	obj->next_content = pRoomIndex->contents;
	pRoomIndex->contents = obj;
	obj->in_room = pRoomIndex;
	obj->carried_by = NULL;
	obj->in_obj = NULL;
	return;
}



/*
 * Move an object into an object.
 */
void
obj_to_obj(OBJ_DATA * obj, OBJ_DATA * obj_to)
{
	obj->next_content = obj_to->contains;
	obj_to->contains = obj;
	obj->in_obj = obj_to;
	obj->in_room = NULL;
	obj->carried_by = NULL;
	if (obj_to->pIndexData->vnum == OBJ_VNUM_PIT)
		obj->cost = 0;

	for (; obj_to != NULL; obj_to = obj_to->in_obj)
	{
		if (obj_to->carried_by != NULL)
		{
			obj_to->carried_by->carry_number += get_obj_number(obj);
			obj_to->carried_by->carry_weight += get_obj_weight(obj)
				* WEIGHT_MULT(obj_to) / 100;
		}
	}

	return;
}



/*
 * Move an object out of an object.
 */
void
obj_from_obj(OBJ_DATA * obj)
{
	OBJ_DATA *obj_from;

	if ((obj_from = obj->in_obj) == NULL)
	{
		bug("Obj_from_obj: null obj_from.", 0);
		return;
	}

	if (obj == obj_from->contains)
	{
		obj_from->contains = obj->next_content;
	}
	else
	{
		OBJ_DATA *prev;

		for (prev = obj_from->contains; prev; prev = prev->next_content)
		{
			if (prev->next_content == obj)
			{
				prev->next_content = obj->next_content;
				break;
			}
		}

		if (prev == NULL)
		{
			bug("Obj_from_obj: obj not found.", 0);
			return;
		}
	}

	obj->next_content = NULL;
	obj->in_obj = NULL;

	for (; obj_from != NULL; obj_from = obj_from->in_obj)
	{
		if (obj_from->carried_by != NULL)
		{
			obj_from->carried_by->carry_number -= get_obj_number(obj);
			obj_from->carried_by->carry_weight -= get_obj_weight(obj)
				* WEIGHT_MULT(obj_from) / 100;
		}
	}

	return;
}



/*
 * Extract an obj from the world.
 */
void
extract_obj(OBJ_DATA * obj)
{
	OBJ_DATA *obj_content;
	OBJ_DATA *obj_next;

	if (obj == NULL)
		return;

	if (obj->in_room != NULL)
		obj_from_room(obj);
	else if (obj->carried_by != NULL)
		obj_from_char(obj);
	else if (obj->in_obj != NULL)
		obj_from_obj(obj);

	for (obj_content = obj->contains; obj_content; obj_content = obj_next)
	{
		obj_next = obj_content->next_content;
		extract_obj(obj_content);
	}

	if (object_list == obj)
	{
		object_list = obj->next;
	}
	else
	{
		OBJ_DATA *prev;

		for (prev = object_list; prev != NULL; prev = prev->next)
		{
			if (prev->next == obj)
			{
				prev->next = obj->next;
				break;
			}
		}

		if (prev == NULL)
		{
			bug("Extract_obj: obj %d not found.", obj->pIndexData->vnum);
			return;
		}
	}

	--obj->pIndexData->count;
	free_obj(obj);
	return;
}



/*
 * Extract a char from the world.
 */
void
extract_char(CHAR_DATA * ch, bool fPull)
{
	CHAR_DATA *wch;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	int from_room;

	if (ch->in_room == NULL)
	{
		bug("Extract_char: NULL.", 0);
		return;
	}

	nuke_pets(ch);

	if (IS_SET(ch->act, ACT_PET))
		nuke_pets(ch->master);

	if (fPull)
		die_follower(ch);

	stop_fighting(ch, TRUE);

	for (obj = ch->carrying; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;
		extract_obj(obj);
	}

	from_room = 0;
	if (ch->in_room != NULL && ch->in_room->area != NULL)
		from_room = ch->in_room->area->continent;

	char_from_room(ch);

	/* Death room is set in the clan tabe now */
	if (!fPull)
	{
		if (from_room == 0)
			char_to_room(ch, get_room_index(clan_table[ch->clan].hall));
		else
			char_to_room(ch, get_room_index(clan_table[ch->clan].hall_dominia));

		return;
	}

	if (IS_NPC(ch))
		--ch->pIndexData->count;

	if (ch->desc != NULL && ch->desc->original != NULL)
	{
		do_return(ch, "");
		ch->desc = NULL;
	}

	for (wch = char_list; wch != NULL; wch = wch->next)
	{
		if (wch->retell == ch)
			wch->retell = NULL;
		if (wch->reply == ch)
			wch->reply = NULL;
		if (ch->mprog_target == wch)
			wch->mprog_target = NULL;
	}

	if (ch == char_list)
	{
		char_list = ch->next;
	}
	else
	{
		CHAR_DATA *prev;

		for (prev = char_list; prev != NULL; prev = prev->next)
		{
			if (prev->next == ch)
			{
				prev->next = ch->next;
				break;
			}
		}

		if (prev == NULL)
		{
			bug("Extract_char: char not found.", 0);
			return;
		}
	}

	if (ch->desc != NULL)
		ch->desc->character = NULL;
	free_char(ch);
	return;
}



/*
 * Find a char in the room.
 */
/* Old version!!! searches by character! */
CHAR_DATA *
get_char_room(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *rch;
	int number;
	int count;

	number = number_argument(argument, arg);
	count = 0;

	if (ch->in_room == NULL)
		return NULL;

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
	{
		if (can_see(ch, rch)
			&& (is_name(arg, rch->name)
				|| (is_name(arg, rch->shift_name) && is_affected(rch, gsn_shapeshift))))
			++count;

		if (count == number)
			return rch;

	}

	return NULL;
}

/* New version! Checks for victim by room.
   Coded by StarX */
CHAR_DATA* get_char_from_room(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument)
{
        char arg[MAX_INPUT_LENGTH];
        CHAR_DATA *rch;
        int number;
        int count;

        number = number_argument(argument, arg);
        count = 0;

        if (room == NULL)
                return NULL;

        for (rch = room->people; rch != NULL; rch = rch->next_in_room) {
                if (can_see(ch, rch)
                    && (is_name(arg, rch->name)
                        || (is_name(arg, rch->shift_name)
			    && is_affected(rch, gsn_shapeshift))))
                        ++count;

                if (count == number)
                        return rch;
        }

        return NULL;
}



/*
 * Find a char in the world.
 */
CHAR_DATA *
get_char_world(CHAR_DATA * ch, char *argument, bool allowIntercontinental) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *wch;
    int number;
    int count;

    if ((wch = get_char_room(ch, argument)) != NULL) {
        return wch;
    }

    number = number_argument(argument, arg);
    count = 0;

    for (wch = char_list; wch != NULL; wch = wch->next) {
        if ((wch->in_room != NULL) && 
                ((wch->in_room->area->continent == ch->in_room->area->continent) || allowIntercontinental) &&
                can_see(ch, wch) &&
                (is_name(arg, wch->name) || (is_name(arg, wch->shift_name) && is_affected(wch, gsn_shapeshift)))) {
            ++count;
        }

        if (count == number) {
            return wch;
        }
    }

    return NULL;
}

/*
 * Exactly like get_char_world, except that it only checks finished areas
 */
CHAR_DATA *
get_char_world_finished_areas(CHAR_DATA * ch, char *argument, bool allowIntercontinental) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *wch;
    int number;
    int count;

    if ((wch = get_char_room(ch, argument)) != NULL) {
        return wch;
    }

    number = number_argument(argument, arg);
    count = 0;

    for (wch = char_list; wch != NULL; wch = wch->next) {
        if ((wch->in_room != NULL) && 
                ((wch->in_room->area->continent == ch->in_room->area->continent) || allowIntercontinental) &&
                (wch->in_room->area->security == 9) &&
                can_see(ch, wch) &&
                (is_name(arg, wch->name) || (is_name(arg, wch->shift_name) && is_affected(wch, gsn_shapeshift)))) {
            ++count;
        }

        if (count == number) {
            return wch;
        }
    }

    return NULL;
}

CHAR_DATA *
get_char_area(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *wch;

	wch = get_char_world(ch, argument, TRUE);
	if (wch == NULL)
		return NULL;

	if (wch->zone == ch->zone)
		return wch;
	else
		return NULL;
}




/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
OBJ_DATA *
get_obj_type(OBJ_INDEX_DATA * pObjIndex)
{
	OBJ_DATA *obj;

	for (obj = object_list; obj != NULL; obj = obj->next)
	{
		if (obj->pIndexData == pObjIndex)
			return obj;
	}

	return NULL;
}


/*
 * Find an obj in a list.
 */
OBJ_DATA *
get_obj_list(CHAR_DATA * ch, char *argument, OBJ_DATA * list)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int number;
	int count;

	number = number_argument(argument, arg);
	count = 0;
	for (obj = list; obj != NULL; obj = obj->next_content)
	{
		if (can_see_obj(ch, obj) && is_name(arg, obj->name))
		{
			if (++count == number)
				return obj;
		}
	}

	return NULL;
}


/*
 * Find an object first in a character's inventory, and then in what they're
 * wearing
 */
OBJ_DATA *
get_obj_carry_or_wear(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument(argument, arg);

    count = 0;
    for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
        if (obj->wear_loc == WEAR_NONE
                && (can_see_obj(ch, obj))
                && is_name(arg, obj->name)) {
            if (++count == number) {
                return obj;
            }
        }
    }

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
        if (obj->wear_loc != WEAR_NONE
                && (can_see_obj(ch, obj))
                && is_name(arg, obj->name)) {
            if (++count == number) {
                return obj;
            }
        }
    }

    return NULL;
}



/*
 * Find an obj in player's inventory.
 */
OBJ_DATA *
get_obj_carry(CHAR_DATA * ch, char *argument, CHAR_DATA * viewer)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int number;
	int count;

	number = number_argument(argument, arg);
	count = 0;
	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	{
		if (obj->wear_loc == WEAR_NONE
			&& (can_see_obj(viewer, obj))
			&& is_name(arg, obj->name))
		{
			if (++count == number)
				return obj;
		}
	}

	return NULL;
}

// Returns the exact number of items of the specified
// index one ch is carrying.
int get_obj_qty(CHAR_DATA *ch, OBJ_INDEX_DATA* pObjIndex) {
	OBJ_DATA *obj;
	int count = 0;

	for(obj = ch->carrying; obj != NULL; obj = obj->next_content) {
		if(obj->wear_loc == WEAR_NONE
		&& obj->pIndexData->vnum == pObjIndex->vnum) {
			count++;
		}
	}

	return count;
}


/*
 * Find an obj in player's equipment.
 */
OBJ_DATA *
get_obj_wear(CHAR_DATA * ch, char *argument, CHAR_DATA* viewer)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int number;
	int count;

	number = number_argument(argument, arg);
	count = 0;
	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	{
		if (obj->wear_loc != WEAR_NONE
			&& can_see_obj(viewer, obj)
			&& is_name(arg, obj->name))
		{
			if (++count == number)
				return obj;
		}
	}

	return NULL;
}



/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA *
get_obj_here(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj;

	obj = get_obj_list(ch, argument, ch->in_room->contents);
	if (obj != NULL)
		return obj;

	if ((obj = get_obj_carry(ch, argument, ch)) != NULL)
		return obj;

	if ((obj = get_obj_wear(ch, argument, ch)) != NULL)
		return obj;

	return NULL;
}



/*
 * Find an obj in the world.
 */
OBJ_DATA *
get_obj_world(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int number;
	int count;

	if ((obj = get_obj_here(ch, argument)) != NULL)
		return obj;

	number = number_argument(argument, arg);
	count = 0;
	for (obj = object_list; obj != NULL; obj = obj->next)
	{
		if (can_see_obj(ch, obj) && is_name(arg, obj->name))
		{
			if (++count == number)
				return obj;
		}
	}

	return NULL;
}

/* deduct cost from a character */

void
deduct_cost(CHAR_DATA * ch, int cost)
{
	int silver = 0, gold = 0;

	silver = UMIN(ch->silver, cost);

	if (silver < cost)
	{
		gold = ((cost - silver + 99) / 100);
		silver = cost - 100 * gold;
	}

	ch->gold -= gold;
	ch->silver -= silver;

	if (ch->gold < 0)
	{
		bug("deduct costs: gold %d < 0", ch->gold);
		ch->gold = 0;
	}
	if (ch->silver < 0)
	{
		bug("deduct costs: silver %d < 0", ch->silver);
		ch->silver = 0;
	}
}
/*
 * Create a 'money' obj.
 */
OBJ_DATA *
create_money(int gold, int silver)
{
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;

	if (gold < 0 || silver < 0 || (gold == 0 && silver == 0))
	{
		bug("Create_money: zero or negative money.", UMIN(gold, silver));
		gold = UMAX(1, gold);
		silver = UMAX(1, silver);
	}

	if (gold == 0 && silver == 1)
	{
		obj = create_object(get_obj_index(OBJ_VNUM_SILVER_ONE), 0);
	}
	else if (gold == 1 && silver == 0)
	{
		obj = create_object(get_obj_index(OBJ_VNUM_GOLD_ONE), 0);
	}
	else if (silver == 0)
	{
		obj = create_object(get_obj_index(OBJ_VNUM_GOLD_SOME), 0);
		sprintf(buf, obj->short_descr, gold);
		free_string(obj->short_descr);
		obj->short_descr = str_dup(buf);
		obj->value[1] = gold;
		obj->cost = gold;
		obj->weight = gold / 5;
	}
	else if (gold == 0)
	{
		obj = create_object(get_obj_index(OBJ_VNUM_SILVER_SOME), 0);
		sprintf(buf, obj->short_descr, silver);
		free_string(obj->short_descr);
		obj->short_descr = str_dup(buf);
		obj->value[0] = silver;
		obj->cost = silver;
		obj->weight = silver / 20;
	}

	else
	{
		obj = create_object(get_obj_index(OBJ_VNUM_COINS), 0);
		sprintf(buf, obj->short_descr, silver, gold);
		free_string(obj->short_descr);
		obj->short_descr = str_dup(buf);
		obj->value[0] = silver;
		obj->value[1] = gold;
		obj->cost = 100 * gold + silver;
		obj->weight = gold / 5 + silver / 20;
	}

	return obj;
}



/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int
get_obj_number(OBJ_DATA * obj)
{
	int number;

	number = 0;

	if (obj->item_type == ITEM_MONEY
	|| obj->item_type == ITEM_GEM
	|| obj->item_type == ITEM_JEWELRY
	|| obj->item_type == ITEM_FOOD)
		return 0;
	else if (obj->item_type == ITEM_CONTAINER
	|| obj->item_type == ITEM_CORPSE_NPC
	|| obj->item_type == ITEM_CORPSE_PC)
	{
		for (obj = obj->contains; obj != NULL; obj = obj->next_content)
			number += get_obj_number(obj);

		return number;
	}
	else
		return 1;

}


/*
 * Return weight of an object, including weight of contents.
 */
int
get_obj_weight(OBJ_DATA * obj)
{
	int weight;
	OBJ_DATA *tobj;

	weight = obj->weight;
	for (tobj = obj->contains; tobj != NULL; tobj = tobj->next_content)
		weight += get_obj_weight(tobj) * WEIGHT_MULT(obj) / 100;

	// This will fix problems with items that don't count as items
	// like gems and also weigh zero. If you feed 6000+ of them to
	// mobs, we will crash BADLY. This way all items have minimum
	// weight of 1 silver. No zero weight allowed!
	return UMAX(1, weight);
}

int
get_true_weight(OBJ_DATA * obj)
{
	int weight;

	weight = obj->weight;
	for (obj = obj->contains; obj != NULL; obj = obj->next_content)
		weight += get_obj_weight(obj);

	return weight;
}

/*
 * True if room is dark.
 */
bool
room_is_dark(ROOM_INDEX_DATA * pRoomIndex)
{
	if (pRoomIndex == NULL)
		return FALSE;

	if (pRoomIndex->light > 0)
		return FALSE;

	if (IS_SET(pRoomIndex->room_flags, ROOM_DARK))
		return TRUE;

	if (pRoomIndex->sector_type == SECT_INSIDE
		|| pRoomIndex->sector_type == SECT_CITY)
		return FALSE;

	if (weather_info.sunlight == SUN_SET
		|| weather_info.sunlight == SUN_DARK)
		return TRUE;

	return FALSE;
}


bool
is_room_owner(CHAR_DATA * ch, ROOM_INDEX_DATA * room)
{
	if (IS_NPC(ch))
		return FALSE;

	if (room->owner == NULL || room->owner[0] == '\0')
		return FALSE;

	return is_name(ch->name, room->owner);
}

/*
 * True if room is private.
 */
bool
room_is_private(ROOM_INDEX_DATA * pRoomIndex)
{
	CHAR_DATA *rch;
	int count;


	if (pRoomIndex->owner != NULL && pRoomIndex->owner[0] != '\0')
		return TRUE;

	count = 0;
	for (rch = pRoomIndex->people; rch != NULL; rch = rch->next_in_room)
		count++;

	if (IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE) && count >= 2)
		return TRUE;

	if (IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY) && count >= 1)
		return TRUE;

	if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY))
		return TRUE;

	return FALSE;
}

/* visibility on a room -- for entering and exits */
bool
can_see_room(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex)
{
	if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY)
		&& get_trust(ch) < MAX_LEVEL)
		return FALSE;

	if (IS_SET(pRoomIndex->room_flags, ROOM_GODS_ONLY)
		&& !IS_IMMORTAL(ch))
		return FALSE;

	if (IS_SET(pRoomIndex->room_flags, ROOM_HEROES_ONLY)
		&& !IS_IMMORTAL(ch))
		return FALSE;

	if (IS_SET(pRoomIndex->room_flags, ROOM_NEWBIES_ONLY)
		&& ch->level > 5 && !IS_IMMORTAL(ch))
		return FALSE;

	if (!IS_IMMORTAL(ch) && pRoomIndex->clan && IS_SET(pRoomIndex->room_flags, ROOM_CLAN) && ch->clan != pRoomIndex->clan)
		return FALSE;

	if (!IS_IMMORTAL(ch) && pRoomIndex->clan && clan_table[pRoomIndex->clan].pkiller != clan_table[ch->clan].pkiller)
		return FALSE;

	return TRUE;
}



/*
 * True if char can see victim.
 */
bool
can_see(CHAR_DATA * ch, CHAR_DATA * victim)
{

	if (victim == NULL || ch == NULL)
		return FALSE;

/* RT changed so that WIZ_INVIS has levels */
	if (ch == victim)
		return TRUE;

	if (get_trust(ch) < victim->invis_level)
		return FALSE;

	if (get_trust(ch) < victim->incog_level && ch->in_room != victim->in_room)
		return FALSE;

	if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
		|| (IS_NPC(ch) && IS_IMMORTAL(ch)))
		return TRUE;

	if (IS_NPC(victim)
	&& IS_SET(victim->act, ACT_IS_WIZI)
	&& !IS_IMMORTAL(ch))
		return FALSE;

	if (ch->reclass == reclass_lookup("illusionist") &&
	    number_percent() < 5)
	{
		return FALSE;
	}

	if (is_affected(ch, gsn_detect_all))
                return TRUE;

	if (victim->fighting
	&& is_affected(victim, gsn_stance_shadow)
	&& number_percent() > (is_affected(ch, gsn_true_sight) ? 20 : 10))
		return FALSE;

	if (IS_AFFECTED(ch, AFF_BLIND))
		return FALSE;

	if (ch->in_room != NULL)
	{
		if (room_is_affected(ch->in_room, gsn_darkness)
		&& !IS_AFFECTED(ch, AFF_DARK_VISION)
		&& number_percent() > (is_affected(ch, gsn_true_sight) ? 20 : 10))
			return FALSE;

		if (room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_INFRARED))
			if (!IS_AFFECTED(ch, AFF_DARK_VISION))
				return FALSE;
	}

	if (IS_AFFECTED(victim, AFF_INVISIBLE)
		&& !IS_AFFECTED(ch, AFF_DETECT_INVIS)
		&& !is_affected(ch, gsn_true_sight))
	{
		return FALSE;
	}

	/* victim is oculary'ed, ch must have detect invis, and must be within pk */
	if (is_affected(victim, gsn_oculary)
	&& !IS_AFFECTED(ch, AFF_DETECT_INVIS)
	&& !is_affected(ch, gsn_true_sight))
		return FALSE;

	if (is_affected(victim, gsn_oculary)
	&& ch->level < victim->level - 8)
		return FALSE;

	if (is_affected(victim, gsn_oculary)
	&& number_percent() < ((victim->level - ch->level) * 8) + (is_affected(ch, gsn_true_sight) ? 40 : 20))
		return FALSE;

	if (IS_AFFECTED(victim, AFF_HIDE)
		&& !IS_AFFECTED(ch, AFF_DETECT_HIDDEN)
		&& !is_affected(ch, gsn_true_sight)
		&& victim->fighting == NULL)
	{
		int chance;
		int goodHide = FALSE;

		chance = get_skill(victim, gsn_hide);
		chance += get_curr_stat(victim, STAT_DEX);
		chance -= get_curr_stat(ch, STAT_INT) * 2;
		chance -= ch->level * 2;
		chance += victim->level * 2;

		if (chance > 90)
			chance = 90;

		// Thieves always hide well (subject to size)
		if(ch->charClass == class_lookup("thief"))
			goodHide = TRUE;

		// Rangers hide well only outdoors
		if(ch->charClass == class_lookup("ranger")
		&& (ch->in_room->sector_type == SECT_FOREST
		|| ch->in_room->sector_type == SECT_HILLS
		|| ch->in_room->sector_type == SECT_MOUNTAIN
		|| ch->in_room->sector_type == SECT_SWAMP))
			goodHide = TRUE;

		// Shadow dragons hide well only at night
		// And at no size penalty
		if(ch->race == race_lookup("black dragon")
		&& ch->remort && ch->rem_sub == 2)
		{
			chance += 5;
			goodHide = TRUE;
		}

		if (!goodHide)
			chance -= get_curr_stat(ch, STAT_WIS);

		// Also, modify by size.. this means elf has the best hide.
		// Small size (1) has +5%
		// Medium size (2) has no modifier
		// Large size (3) has -5%
		// Huge size (4) has -10%
		chance -= (5 * (victim->size - 2));

		if (number_percent() < chance)
			return FALSE;
	}



	return TRUE;
}

/*
 * True if char can see obj.
 */
bool
can_see_obj(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
		return TRUE;

	if (IS_SET(obj->extra_flags, ITEM_VIS_DEATH))
		return FALSE;

	if (ch->reclass == reclass_lookup("illusionist") &&
	    number_percent() < 5)
	{
		return FALSE;
	}

	if (is_affected(ch, gsn_detect_all))
                return TRUE;

   /*
    * Original Code: Can see all potions in your inv when blind.
    *
	if (IS_AFFECTED(ch, AFF_BLIND) && obj->item_type != ITEM_POTION)
		return FALSE;
    */


   /*
    * New Code: Can see any potion/pill/scroll/wand that has cure_blind,
    *           but you will no longer be able to potions that do not
    *           have cure_blind.
    */
   if (IS_AFFECTED(ch, AFF_BLIND)) {
      int canCureBlind = 0;
      int i;
      switch (obj->item_type) {
         case ITEM_SCROLL:
         case ITEM_PILL:
            for (i = 1; i < 5; i++) {
               canCureBlind |= (obj->value[i] >= 0 && obj->value[i] < MAX_SKILL && skill_table[obj->value[i]].pgsn == &gsn_cure_blindness);
            }
	    break;
         case ITEM_WAND:
         case ITEM_STAFF:
            canCureBlind = (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL && skill_table[obj->value[3]].pgsn == &gsn_cure_blindness);
	    break;
	 case ITEM_POTION:
	    return TRUE;
      }

      if (!canCureBlind)
      	return FALSE; 
   }

	if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0)
		return TRUE;

	if (IS_SET(obj->extra_flags, ITEM_INVIS)
		&& !IS_AFFECTED(ch, AFF_DETECT_INVIS)
		&& !is_affected(ch, gsn_true_sight))
		return FALSE;

	if (room_is_affected(ch->in_room, gsn_darkness)
	   && !IS_AFFECTED(ch, AFF_DARK_VISION)
	   && number_percent() > 10)
		return FALSE;

	if (IS_OBJ_STAT(obj, ITEM_GLOW))
		return TRUE;

	if (room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_INFRARED))
		if (!IS_AFFECTED(ch, AFF_DARK_VISION))
			return FALSE;

	return TRUE;
}



/*
 * True if char can drop obj.
 */
bool
can_drop_obj(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (!IS_SET(obj->extra_flags, ITEM_NODROP))
		return TRUE;

	if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
		return TRUE;

	return FALSE;
}


/*
 * Return ascii name of extra flags vector.
 */
char *
extra_bit_name(int extra_flags)
{
	static char buf[512];

	buf[0] = '\0';
	if (extra_flags & ITEM_GLOW)
		strcat(buf, " glow");
	if (extra_flags & ITEM_HUM)
		strcat(buf, " hum");
	if (extra_flags & ITEM_DARK)
		strcat(buf, " dark");
	if (extra_flags & ITEM_LOCK)
		strcat(buf, " lock");
	if (extra_flags & ITEM_EVIL)
		strcat(buf, " evil");
	if (extra_flags & ITEM_INVIS)
		strcat(buf, " invis");
	if (extra_flags & ITEM_MAGIC)
		strcat(buf, " magic");
	if (extra_flags & ITEM_NODROP)
		strcat(buf, " nodrop");
	if (extra_flags & ITEM_BLESS)
		strcat(buf, " bless");
	if (extra_flags & ITEM_ANTI_GOOD)
		strcat(buf, " anti-good");
	if (extra_flags & ITEM_ANTI_EVIL)
		strcat(buf, " anti-evil");
	if (extra_flags & ITEM_ANTI_NEUTRAL)
		strcat(buf, " anti-neutral");
	if (extra_flags & ITEM_NOREMOVE)
		strcat(buf, " noremove");
	if (extra_flags & ITEM_INVENTORY)
		strcat(buf, " inventory");
	if (extra_flags & ITEM_NOPURGE)
		strcat(buf, " nopurge");
	if (extra_flags & ITEM_VIS_DEATH)
		strcat(buf, " vis_death");
	if (extra_flags & ITEM_ROT_DEATH)
		strcat(buf, " rot_death");
	if (extra_flags & ITEM_NOLOCATE)
		strcat(buf, " no_locate");
	if (extra_flags & ITEM_SELL_EXTRACT)
		strcat(buf, " sell_extract");
	if (extra_flags & ITEM_BURN_PROOF)
		strcat(buf, " burn_proof");
	if (extra_flags & ITEM_NOUNCURSE)
		strcat(buf, " no_uncurse");
	if (extra_flags & ITEM_WARRIOR)
		strcat(buf, " warrior");
	if (extra_flags & ITEM_MAGE)
		strcat(buf, " mage");
	if (extra_flags & ITEM_CLERIC)
		strcat(buf, " cleric");
	if (extra_flags & ITEM_THIEF)
		strcat(buf, " thief");
	if (extra_flags & ITEM_SOUND_PROOF)
		strcat(buf, " soundproof");
	return (buf[0] != '\0') ? buf + 1 : "none";
}

/* return ascii name of an act vector */
char *
act_bit_name(int act_flags)
{
	static char buf[512];

	buf[0] = '\0';

	if (IS_SET(act_flags, ACT_IS_NPC))
	{
		strcat(buf, " npc");
		if (act_flags & ACT_SENTINEL)
			strcat(buf, " sentinel");
		if (act_flags & ACT_SCAVENGER)
			strcat(buf, " scavenger");
		if (act_flags & ACT_AGGRESSIVE)
			strcat(buf, " aggressive");
		if (act_flags & ACT_STAY_AREA)
			strcat(buf, " stay_area");
		if (act_flags & ACT_WIMPY)
			strcat(buf, " wimpy");
		if (act_flags & ACT_PET)
			strcat(buf, " pet");
		if (act_flags & ACT_TRAIN)
			strcat(buf, " train");
		if (act_flags & ACT_PRACTICE)
			strcat(buf, " practice");
		if (act_flags & ACT_UNDEAD)
			strcat(buf, " undead");
		if (act_flags & ACT_CLERIC)
			strcat(buf, " cleric");
		if (act_flags & ACT_MAGE)
			strcat(buf, " mage");
		if (act_flags & ACT_THIEF)
			strcat(buf, " thief");
		if (act_flags & ACT_WARRIOR)
			strcat(buf, " warrior");
		if (act_flags & ACT_NOALIGN)
			strcat(buf, " no_align");
		if (act_flags & ACT_NOPURGE)
			strcat(buf, " no_purge");
		if (act_flags & ACT_IS_HEALER)
			strcat(buf, " healer");
		if (act_flags & ACT_IS_CHANGER)
			strcat(buf, " changer");
		if (act_flags & ACT_BANKER)
			strcat(buf, " banker");
		if (act_flags & ACT_GAIN)
			strcat(buf, " skill_train");
		if (act_flags & ACT_UPDATE_ALWAYS)
			strcat(buf, " update_always");
	}
	else
	{
		strcat(buf, " player");
		if (act_flags & PLR_PLRASSIST)
			strcat(buf, " playerassist");
		if (act_flags & PLR_PLRASSIST)
			strcat(buf, " mobassist");
		if (act_flags & PLR_AUTOEXIT)
			strcat(buf, " autoexit");
		if (act_flags & PLR_AUTOLOOT)
			strcat(buf, " autoloot");
		if (act_flags & PLR_AUTOSAC)
			strcat(buf, " autosac");
		if (act_flags & PLR_AUTOTRASH)
			strcat(buf, " autotrash");
		if (act_flags & PLR_AUTOGOLD)
			strcat(buf, " autogold");
		if (act_flags & PLR_AUTOSPLIT)
			strcat(buf, " autosplit");
		if (act_flags & PLR_HOLYLIGHT)
			strcat(buf, " holy_light");
		if (act_flags & PLR_CANLOOT)
			strcat(buf, " loot_corpse");
		if (act_flags & PLR_NOSUMMON)
			strcat(buf, " no_summon");
		if (act_flags & PLR_NOFOLLOW)
			strcat(buf, " no_follow");
		if (act_flags & PLR_AUTOTITLE)
			strcat(buf, " autotitle");
		if (act_flags & PLR_NOTIFY)
			strcat(buf, " notify");
		if (act_flags & PLR_FREEZE)
			strcat(buf, " frozen");
		if (act_flags & PLR_COLOUR)
			strcat(buf, " colour");
		if (act_flags & PLR_THIEF)
			strcat(buf, " thief");
		if (act_flags & PLR_KILLER)
			strcat(buf, " killer");
	}
	return (buf[0] != '\0') ? buf + 1 : "none";
}

char *
comm_bit_name(int comm_flags)
{
	static char buf[512];

	buf[0] = '\0';

	if (comm_flags & COMM_QUIET)
		strcat(buf, " quiet");
	if (comm_flags & COMM_DEAF)
		strcat(buf, " deaf");
	if (comm_flags & COMM_NOWIZ)
		strcat(buf, " no_wiz");
	if (comm_flags & COMM_NOGOSSIP)
		strcat(buf, " no_gossip");
	if (comm_flags & COMM_NOQUESTION)
		strcat(buf, " no_question");
	if (comm_flags & COMM_NOMUSIC)
		strcat(buf, " no_music");
	if (comm_flags & COMM_NOAUCTION)
		strcat(buf, " no_auction");
	if (comm_flags & COMM_COMPACT)
		strcat(buf, " compact");
	if (comm_flags & COMM_BRIEF)
		strcat(buf, " brief");
	if (comm_flags & COMM_PROMPT)
		strcat(buf, " prompt");
	if (comm_flags & COMM_COMBINE)
		strcat(buf, " combine");
	if (comm_flags & COMM_NOEMOTE)
		strcat(buf, " no_emote");
	if (comm_flags & COMM_NOSHOUT)
		strcat(buf, " no_shout");
	if (comm_flags & COMM_NOTELL)
		strcat(buf, " no_tell");
	if (comm_flags & COMM_NOCHANNELS)
		strcat(buf, " no_channels");
	if (comm_flags & COMM_LAG)
		strcat(buf, " lag");
	if (comm_flags & COMM_NOBEEP)
		strcat(buf, " nobeep");
	if (comm_flags & COMM_NONEWBIE)
		strcat(buf, " no_newbie_tips");

	return (buf[0] != '\0') ? buf + 1 : "none";
}

char *
imm_bit_name(int imm_flags)
{
	static char buf[512];

	buf[0] = '\0';

	if (imm_flags & IMM_SUMMON)
		strcat(buf, " summon");
	if (imm_flags & IMM_CHARM)
		strcat(buf, " charm");
	if (imm_flags & IMM_MAGIC)
		strcat(buf, " magic");
	if (imm_flags & IMM_WEAPON)
		strcat(buf, " weapon");
	if (imm_flags & IMM_BASH)
		strcat(buf, " bash");
	if (imm_flags & IMM_PIERCE)
		strcat(buf, " piercing");
	if (imm_flags & IMM_SLASH)
		strcat(buf, " slashing");
	if (imm_flags & IMM_FIRE)
		strcat(buf, " fire");
	if (imm_flags & IMM_COLD)
		strcat(buf, " cold");
	if (imm_flags & IMM_LIGHTNING)
		strcat(buf, " lightning");
	if (imm_flags & IMM_ACID)
		strcat(buf, " acid");
	if (imm_flags & IMM_POISON)
		strcat(buf, " poison");
	if (imm_flags & IMM_NEGATIVE)
		strcat(buf, " negative");
	if (imm_flags & IMM_HOLY)
		strcat(buf, " holy");
	if (imm_flags & IMM_RAIN)
		strcat(buf, " rain");
	if (imm_flags & IMM_ENERGY)
		strcat(buf, " energy");
	if (imm_flags & IMM_MENTAL)
		strcat(buf, " mental");
	if (imm_flags & IMM_DISEASE)
		strcat(buf, " disease");
	if (imm_flags & IMM_DROWNING)
		strcat(buf, " drowning");
	if (imm_flags & IMM_LIGHT)
		strcat(buf, " light");
	if (imm_flags & VULN_IRON)
		strcat(buf, " iron");
	if (imm_flags & VULN_WOOD)
		strcat(buf, " wood");
	if (imm_flags & VULN_SILVER)
		strcat(buf, " silver");
	if (imm_flags & IMM_VORPAL)
		strcat(buf, " vorpal");

	return (buf[0] != '\0') ? buf + 1 : "none";
}

char *
wear_bit_name(int wear_flags)
{
	static char buf[512];

	buf[0] = '\0';
	if (wear_flags & ITEM_TAKE)
		strcat(buf, " take");
	if (wear_flags & ITEM_WEAR_FINGER)
		strcat(buf, " finger");
	if (wear_flags & ITEM_WEAR_NECK)
		strcat(buf, " neck");
	if (wear_flags & ITEM_WEAR_BODY)
		strcat(buf, " torso");
	if (wear_flags & ITEM_WEAR_HEAD)
		strcat(buf, " head");
	if (wear_flags & ITEM_WEAR_LEGS)
		strcat(buf, " legs");
	if (wear_flags & ITEM_WEAR_FEET)
		strcat(buf, " feet");
	if (wear_flags & ITEM_WEAR_HANDS)
		strcat(buf, " hands");
	if (wear_flags & ITEM_WEAR_ARMS)
		strcat(buf, " arms");
	if (wear_flags & ITEM_WEAR_SHIELD)
		strcat(buf, " shield");
	if (wear_flags & ITEM_WEAR_ABOUT)
		strcat(buf, " body");
	if (wear_flags & ITEM_WEAR_WAIST)
		strcat(buf, " waist");
	if (wear_flags & ITEM_WEAR_WRIST)
		strcat(buf, " wrist");
	if (wear_flags & ITEM_WIELD)
		strcat(buf, " wield");
	if (wear_flags & ITEM_HOLD)
		strcat(buf, " hold");
	if (wear_flags & ITEM_NO_SAC)
		strcat(buf, " nosac");
	if (wear_flags & ITEM_WEAR_FLOAT)
		strcat(buf, " float");
	if (wear_flags & ITEM_CRAFTED)
		strcat(buf, " crafted");
	if (wear_flags & ITEM_INCOMPLETE)
		strcat(buf, " incomplete");
	if (wear_flags & ITEM_CHARGED)
		strcat(buf, " charged");
	if (wear_flags & ITEM_NEWBIE)
		strcat(buf, " newbie");

	return (buf[0] != '\0') ? buf + 1 : "none";
}

char *
form_bit_name(int form_flags)
{
	static char buf[512];

	buf[0] = '\0';
	if (form_flags & FORM_POISON)
		strcat(buf, " poison");
	else if (form_flags & FORM_EDIBLE)
		strcat(buf, " edible");
	if (form_flags & FORM_MAGICAL)
		strcat(buf, " magical");
	if (form_flags & FORM_INSTANT_DECAY)
		strcat(buf, " instant_rot");
	if (form_flags & FORM_OTHER)
		strcat(buf, " other");
	if (form_flags & FORM_ANIMAL)
		strcat(buf, " animal");
	if (form_flags & FORM_SENTIENT)
		strcat(buf, " sentient");
	if (form_flags & FORM_UNDEAD)
		strcat(buf, " undead");
	if (form_flags & FORM_CONSTRUCT)
		strcat(buf, " construct");
	if (form_flags & FORM_MIST)
		strcat(buf, " mist");
	if (form_flags & FORM_INTANGIBLE)
		strcat(buf, " intangible");
	if (form_flags & FORM_BIPED)
		strcat(buf, " biped");
	if (form_flags & FORM_CENTAUR)
		strcat(buf, " centaur");
	if (form_flags & FORM_INSECT)
		strcat(buf, " insect");
	if (form_flags & FORM_SPIDER)
		strcat(buf, " spider");
	if (form_flags & FORM_CRUSTACEAN)
		strcat(buf, " crustacean");
	if (form_flags & FORM_WORM)
		strcat(buf, " worm");
	if (form_flags & FORM_BLOB)
		strcat(buf, " blob");
	if (form_flags & FORM_MAMMAL)
		strcat(buf, " mammal");
	if (form_flags & FORM_BIRD)
		strcat(buf, " bird");
	if (form_flags & FORM_REPTILE)
		strcat(buf, " reptile");
	if (form_flags & FORM_SNAKE)
		strcat(buf, " snake");
	if (form_flags & FORM_DRAGON)
		strcat(buf, " dragon");
	if (form_flags & FORM_AMPHIBIAN)
		strcat(buf, " amphibian");
	if (form_flags & FORM_FISH)
		strcat(buf, " fish");
	if (form_flags & FORM_COLD_BLOOD)
		strcat(buf, " cold_blooded");

	return (buf[0] != '\0') ? buf + 1 : "none";
}

char *
part_bit_name(int part_flags)
{
	static char buf[512];

	buf[0] = '\0';
	if (part_flags & PART_HEAD)
		strcat(buf, " head");
	if (part_flags & PART_ARMS)
		strcat(buf, " arms");
	if (part_flags & PART_LEGS)
		strcat(buf, " legs");
	if (part_flags & PART_HEART)
		strcat(buf, " heart");
	if (part_flags & PART_BRAINS)
		strcat(buf, " brains");
	if (part_flags & PART_GUTS)
		strcat(buf, " guts");
	if (part_flags & PART_HANDS)
		strcat(buf, " hands");
	if (part_flags & PART_FEET)
		strcat(buf, " feet");
	if (part_flags & PART_FINGERS)
		strcat(buf, " fingers");
	if (part_flags & PART_EAR)
		strcat(buf, " ears");
	if (part_flags & PART_EYE)
		strcat(buf, " eyes");
	if (part_flags & PART_LONG_TONGUE)
		strcat(buf, " long_tongue");
	if (part_flags & PART_EYESTALKS)
		strcat(buf, " eyestalks");
	if (part_flags & PART_TENTACLES)
		strcat(buf, " tentacles");
	if (part_flags & PART_FINS)
		strcat(buf, " fins");
	if (part_flags & PART_WINGS)
		strcat(buf, " wings");
	if (part_flags & PART_TAIL)
		strcat(buf, " tail");
	if (part_flags & PART_CLAWS)
		strcat(buf, " claws");
	if (part_flags & PART_FANGS)
		strcat(buf, " fangs");
	if (part_flags & PART_HORNS)
		strcat(buf, " horns");
	if (part_flags & PART_SCALES)
		strcat(buf, " scales");

	return (buf[0] != '\0') ? buf + 1 : "none";
}

char *
weapon_bit_name(int weapon_flags)
{
	static char buf[512];

	buf[0] = '\0';
	if (weapon_flags & WEAPON_FLAMING)
		strcat(buf, " flaming");
	if (weapon_flags & WEAPON_FROST)
		strcat(buf, " frost");
	if (weapon_flags & WEAPON_VAMPIRIC)
		strcat(buf, " vampiric");
	if (weapon_flags & WEAPON_SHARP)
		strcat(buf, " sharp");
	if (weapon_flags & WEAPON_BLUNT)
		strcat(buf, " blunt");
	if (weapon_flags & WEAPON_DULL)
		strcat(buf, " dull");
	if (weapon_flags & WEAPON_VORPAL)
		strcat(buf, " vorpal");
	if (weapon_flags & WEAPON_TWO_HANDS)
		strcat(buf, " two-handed");
	if (weapon_flags & WEAPON_SHOCKING)
		strcat(buf, " shocking");
	if (weapon_flags & WEAPON_POISON)
		strcat(buf, " poison");
	if (weapon_flags & WEAPON_DRAGON_SLAYER)
		strcat(buf, " dragon_slaying");
	if (weapon_flags & WEAPON_CORROSIVE)
		strcat(buf, " corrosive");
	if (weapon_flags & WEAPON_FLOODING)
		strcat(buf, " flooding");
	if (weapon_flags & WEAPON_INFECTED)
		strcat(buf, " infected");
	if (weapon_flags & WEAPON_NOMUTATE)
		strcat(buf, " no_mutate");
	if (weapon_flags & WEAPON_SOULDRAIN)
		strcat(buf, " soul_drain");
	if (weapon_flags & WEAPON_HOLY)
                strcat(buf, " holy");
	if (weapon_flags & WEAPON_UNHOLY)
                strcat(buf, " unholy");
	if (weapon_flags & WEAPON_POLAR)
                strcat(buf, " polar");
	if (weapon_flags & WEAPON_PHASE)
                strcat(buf, " phasing");
	if (weapon_flags & WEAPON_ANTIMAGIC)
                strcat(buf, " antimagic");
	if (weapon_flags & WEAPON_ENTROPIC)
                strcat(buf, " entropic");
	if (weapon_flags & WEAPON_PSIONIC)
                strcat(buf, " psionic");
	if (weapon_flags & WEAPON_DEMONIC)
                strcat(buf, " demonic");
	if (weapon_flags & WEAPON_INTELLIGENT)
                strcat(buf, " intelligent");

	return (buf[0] != '\0') ? buf + 1 : "none";
}

char *
cont_bit_name(int cont_flags)
{
	static char buf[512];

	buf[0] = '\0';

	if (cont_flags & CONT_CLOSEABLE)
		strcat(buf, " closable");
	if (cont_flags & CONT_PICKPROOF)
		strcat(buf, " pickproof");
	if (cont_flags & CONT_CLOSED)
		strcat(buf, " closed");
	if (cont_flags & CONT_LOCKED)
		strcat(buf, " locked");

	return (buf[0] != '\0') ? buf + 1 : "none";
}


char *
off_bit_name(int off_flags)
{
	static char buf[512];

	buf[0] = '\0';

	if (off_flags & OFF_AREA_ATTACK)
		strcat(buf, " area attack");
	if (off_flags & OFF_BACKSTAB)
		strcat(buf, " backstab");
	if (off_flags & OFF_BASH)
		strcat(buf, " bash");
	if (off_flags & OFF_BERSERK)
		strcat(buf, " berserk");
	if (off_flags & OFF_DISARM)
		strcat(buf, " disarm");
	if (off_flags & OFF_DODGE)
		strcat(buf, " dodge");
	if (off_flags & OFF_FADE)
		strcat(buf, " fade");
	if (off_flags & OFF_FAST)
		strcat(buf, " fast");
	if (off_flags & OFF_KICK)
		strcat(buf, " kick");
	if (off_flags & OFF_KICK_DIRT)
		strcat(buf, " kick_dirt");
	if (off_flags & OFF_PARRY)
		strcat(buf, " parry");
	if (off_flags & OFF_RESCUE)
		strcat(buf, " rescue");
	if (off_flags & OFF_TAIL)
		strcat(buf, " tail");
	if (off_flags & OFF_TRIP)
		strcat(buf, " trip");
	if (off_flags & OFF_CRUSH)
		strcat(buf, " crush");
	if (off_flags & ASSIST_ALL)
		strcat(buf, " assist_all");
	if (off_flags & ASSIST_ALIGN)
		strcat(buf, " assist_align");
	if (off_flags & ASSIST_RACE)
		strcat(buf, " assist_race");
	if (off_flags & ASSIST_PLAYERS)
		strcat(buf, " assist_players");
	if (off_flags & ASSIST_GUARD)
		strcat(buf, " assist_guard");
	if (off_flags & ASSIST_VNUM)
		strcat(buf, " assist_vnum");

	return (buf[0] != '\0') ? buf + 1 : "none";
}

char * PERS(CHAR_DATA * ch, CHAR_DATA * looker) {
    // Mobs don't need to be fooled by shapeshift
    if (ch == NULL) {
        return str_dup("null");
    }

    if (can_see(looker, ch)) {
        if (IS_NPC(ch)) {
            return ch->short_descr;
        } else {
            if (is_affected(ch, gsn_shapeshift)
                    && !IS_NPC(looker)
                    && !IS_SET(looker->act, PLR_HOLYLIGHT)
                    && !is_affected(looker, gsn_detect_all)) {
                return ch->shift_short;
            } else {
                return ch->name;
            }
        }
    } else if (!IS_IMMORTAL(ch)) {
        return "Someone";
    } else {
        return "Immortal";
    }
}

int wearing_norecall_item(CHAR_DATA *ch)
{
	int i;
	OBJ_DATA *obj;

	for(i = 0; i < MAX_WEAR; i++) {
		if((obj = get_eq_char(ch, i)) != NULL
		&& IS_SET(obj->wear_flags, ITEM_NORECALL)) {

			return TRUE;
		}
	}

	return FALSE;
}

int wearing_nogate_item(CHAR_DATA *ch)
{
	int i;
	OBJ_DATA *obj;

	for(i = 0; i < MAX_WEAR; i++) {
		if((obj = get_eq_char(ch, i)) != NULL
		&& IS_SET(obj->wear_flags, ITEM_NOGATE)) {

			return TRUE;
		}
	}

	return FALSE;
}

int is_quest_craft(OBJ_DATA *obj)
{
	int i = 0;

	// Crafted items are easy
	if(IS_SET(obj->wear_flags, ITEM_CRAFTED))
		return TRUE;

	// Check the quest item table
	while(1) {
		if(itemList[i].vnum == 0)
			return FALSE;

		if(obj->pIndexData->vnum == itemList[i].vnum) {
			return TRUE;
		}
		i++;
	}

	return FALSE;
}
