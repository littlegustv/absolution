/* revision 1.1 - August 1 1999 - making it compilable under g++ */
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "interp.h"
#include "clan.h"
#include "utils.h"


#define REFLECT_FAIL	0
#define REFLECT_BOUNCE	1
#define REFLECT_CASTER  2


/* command procedures needed */
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_wear);
DECLARE_DO_FUN(do_flee);



// The kludgy global is for spells who want more stuff from command line.
char *target_name;

// This is a bad work around for holy word frenzy to not check alignment.
int Holy_Bless = 0;

/*
 * Local functions.
 */
void say_spell(CHAR_DATA * ch, int sn);
int get_random_bit(unsigned int bits);
bool max_no_charmies(CHAR_DATA* ch);
int check_leaf_shield(CHAR_DATA *ch, CHAR_DATA *victim);

/* imported functions */
bool remove_obj(CHAR_DATA * ch, int iWear, bool fReplace);
void wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace);
void obj_to_char(OBJ_DATA * obj, CHAR_DATA * ch);
void death_cry(CHAR_DATA * ch);
void set_fighting(CHAR_DATA * ch, CHAR_DATA * victim);
void check_weather(char *buf, int diff);
void size_mob(CHAR_DATA * ch, CHAR_DATA * victim, int level);
CHAR_DATA* range_finder(CHAR_DATA*, char*, int, int*, int*, bool);
void prismatic_sphere_effect(CHAR_DATA *ch, CHAR_DATA *victim);
extern void lore_obj(CHAR_DATA *ch, OBJ_DATA *obj);
extern int power_tattoo_count(CHAR_DATA *, int);

// Use for spell functions to store lots of info in 1 int.
// Take two number 0-255 and makes them into a single int.
int generate_int(unsigned char prefix, unsigned char postfix)
{
	int result;

	result = postfix << 9;
	result |= prefix;

	return result;
}

int get_caster_level(int bytes) {

	return bytes & 0x00FF;
}

int
get_modified_level(int bytes) {
	return (bytes & 0xFF00) >> 9;
}

int
spell_damage(CHAR_DATA *ch, CHAR_DATA *victim, int level, int type, int dwarf_resist)
{
	int dam = 0;

	// Typically 59 to 99 damage at 51
	if(type == SPELL_DAMAGE_LOW) {
		dam = dice(level / 6, 6) + level;
	}
	// Typically 71 to 181 damage at 51
	// Typically 82 to 226 damage at level 60
	else if(type == SPELL_DAMAGE_MEDIUM) {
		dam = dice(level / 5, 8 + (level > 14) + (level > 29) + (level > 44)) + level + 5;
	}
	// Typically 87 to 243 damage at 51
	// For imms: 105 to 315 damage at level 60
	else if(type == SPELL_DAMAGE_HIGH) {
		dam = dice(level / 4, 10 + (level > 29) + (level > 39) + (level > 49)) + (level * 3 / 2);
	}
	// Follows the line 2x + b... however b depends on the spell.
	else if(type == SPELL_DAMAGE_CHART_GOOD) {
		dam = number_range(level * 2, (level * 2) + 5);
	}
	// Follows the line 2x + b... used by low level spells once you
	// reach a level to cap them out of main use.
	else if(type == SPELL_DAMAGE_CHART_POOR) {
		dam = number_range(level, level + 5);
	}
	else {
		Cprintf(ch, "Bad spell damage error.\n\r");
		return 0;
	}
	// Invokers add a tiny bit of damage
	if(ch->charClass == class_lookup("invoker"))
		dam = dam + (ch->level / 6);

	// Wisdom opposes con
	dam += (get_curr_stat(ch, STAT_WIS) * 3 / 2);
	dam -= (get_curr_stat(victim, STAT_CON) * 3 / 2);

	if(number_percent() < 50
	&& is_affected(victim, gsn_enrage)) {
		Cprintf(victim, "Your rage absorbs part of the magic!\n\r");
		dam = dam * 3 / 4;
	}

	// Dwarves generally take half damage.
	if(dwarf_resist == TRUE
	&& victim->race == race_lookup("dwarf"))
        	dam /= 2;

	return dam;
}

bool
max_no_charmies(CHAR_DATA* ch)
{
	CHAR_DATA* vch;
	int found;

	found = 0;

	for (vch = char_list; vch != NULL; vch = vch->next)
	{
		if (vch->master == ch && IS_NPC(vch))
			found++;
	}

	if (found >= 8)
	{
		return TRUE;
	}

	return FALSE;
}

void
size_mob(CHAR_DATA * ch, CHAR_DATA * victim, int level)
{
	int ac;
	int dam_bonus;
	int stat;

	victim->level = level;
	if (level > 25)
		victim->max_hit = number_range(((level - 20) * 100) + 50,
									   ((level - 20) * 100) + 250);
	else
		victim->max_hit = number_range(level * 20, level * 28);

	/* this value can be changed */
	stat = victim->level / 3;
	victim->perm_stat[0] += stat;
	victim->perm_stat[1] += stat;
	victim->perm_stat[2] += stat;
	victim->perm_stat[3] += stat;
	victim->perm_stat[4] += stat;

	victim->hit = victim->max_hit;
	victim->hitroll = level / 15;
	victim->damroll = level / 10;

	if (level > 46)
		ac = -18 - (level - 45);
	else if (level > 32)
		ac = 0 - (level - 32 / 3);
	else if (level > 11)
		ac = 0 - (level - 11 / 2);
	else
		ac = 10 - level;
	victim->armor[0] = number_fuzzy(ac);
	victim->armor[1] = number_fuzzy(ac);
	victim->armor[2] = number_fuzzy(ac);
	victim->armor[3] = number_fuzzy(ac * 2 / 3);

	if (level > 30)
		dam_bonus = (level / 2) - 7;
	else if (level > 15)
		dam_bonus = (level / 3) - 3;
	else if (level > 10)
		dam_bonus = (level / 3) - 2;
	else if (level > 5)
		dam_bonus = (level / 3) - 1;
	else
		dam_bonus = (level / 3);

	victim->damage[0] = 2;
	victim->damage[1] = level / 2;
	victim->damage[2] = dam_bonus;
}

void
size_obj(CHAR_DATA * ch, OBJ_DATA * obj, int level)
{
	if(obj == NULL
	||  ch == NULL)
		return;

	obj->level = level;
	obj->value[1] = 2;
	obj->value[2] = (level / 2) - 1;
}

/*
 * Lookup a skill by name.
 */
int
skill_lookup(const char *name)
{
	int sn;

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;
		if (LOWER(name[0]) == LOWER(skill_table[sn].name[0])
			&& !str_prefix(name, skill_table[sn].name))
			return sn;
	}

	return -1;
}

int
find_spell(CHAR_DATA * ch, const char *name)
{
	/* finds a spell the character can cast if possible */
	int sn, found = -1;

	if (IS_NPC(ch))
		return skill_lookup(name);

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;
		if (LOWER(name[0]) == LOWER(skill_table[sn].name[0])
			&& !str_prefix(name, skill_table[sn].name))
		{
			if (found == -1)
				found = sn;
			if (ch->level >= skill_table[sn].skill_level[ch->charClass]
				&& ch->pcdata->learned[sn] > 0)
				return sn;
		}
	}
	return found;
}



/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int
slot_lookup(int slot)
{
	extern bool fBootDb;
	int sn;

	if (slot <= 0)
		return -1;

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (slot == skill_table[sn].slot)
			return sn;
	}

	if (fBootDb)
	{
		bug("Slot_lookup: bad slot %d.", slot);
		abort();
	}

	return -1;
}



/*
 * Utter mystical words for an sn.
 */
void
say_spell(CHAR_DATA * ch, int sn)
{
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	CHAR_DATA *rch;
	char *pName;
	int iSyl;
	int length;

	struct syl_type
	{
		char *old;
		char *newSyl;
	};

	static const struct syl_type syl_table[] =
	{
		{" ", " "},
		{"ar", "abra"},
		{"au", "kada"},
		{"bless", "fido"},
		{"blind", "nose"},
		{"bur", "mosa"},
		{"cu", "judi"},
		{"de", "oculo"},
		{"en", "unso"},
		{"light", "dies"},
		{"lo", "hi"},
		{"mor", "zak"},
		{"move", "sido"},
		{"ness", "lacri"},
		{"ning", "illa"},
		{"per", "duda"},
		{"ra", "gru"},
		{"fresh", "ima"},
		{"re", "candus"},
		{"son", "sabru"},
		{"tect", "infra"},
		{"tri", "cula"},
		{"ven", "nofo"},
		{"a", "a"},
		{"b", "b"},
		{"c", "q"},
		{"d", "e"},
		{"e", "z"},
		{"f", "y"},
		{"g", "o"},
		{"h", "p"},
		{"i", "u"},
		{"j", "y"},
		{"k", "t"},
		{"l", "r"},
		{"m", "w"},
		{"n", "i"},
		{"o", "a"},
		{"p", "s"},
		{"q", "d"},
		{"r", "f"},
		{"s", "g"},
		{"t", "h"},
		{"u", "j"},
		{"v", "z"},
		{"w", "x"},
		{"x", "n"},
		{"y", "l"},
		{"z", "k"},
		{"", ""}
	};

	buf[0] = '\0';
	for (pName = skill_table[sn].name; *pName != '\0'; pName += length)
	{
		for (iSyl = 0; (length = strlen(syl_table[iSyl].old)) != 0; iSyl++)
		{
			if (!str_prefix(syl_table[iSyl].old, pName))
			{
				strcat(buf, syl_table[iSyl].newSyl);
				break;
			}
		}

		if (length == 0)
			length = 1;
	}

	sprintf(buf2, "$n utters the words, '%s'.", buf);
	sprintf(buf, "$n utters the words, '%s'.", skill_table[sn].name);

	for (rch = ch->in_room->people; rch; rch = rch->next_in_room)
	{
		if (rch != ch)
			act(ch->charClass == rch->charClass ? buf : buf2, ch, NULL, rch, TO_VICT, POS_RESTING);
	}

	return;
}

int
saving_throw(CHAR_DATA *caster, CHAR_DATA *victim, int sn, int level, int diff, int stat, int damtype)
{
	int spellpower = 0, resistance = 0, insanity = 0;

	// Basically we want caster power vs victim resistance
	spellpower = (2 * level) + 50;
	spellpower += get_curr_stat(caster, STAT_INT);
	spellpower += dice(1, 10);

	// Modify for innate spell difficulty
	if(diff == SAVE_EASY)
		spellpower = spellpower * 9 / 10;
	if(diff == SAVE_HARD)
		spellpower = spellpower * 11 / 10;

	// Second, victim resistance, should be lower (base 102)
	resistance = 2 * victim->level;
	resistance += dice(1, 10);

	// Add relevant stat to resistance
	if(stat != STAT_NONE)
		resistance += (get_curr_stat(victim, stat));
	else
		// makes stat_none slightly harder to save
		resistance += 16;

	// and now resistance and vuln
    switch (check_immune(victim, damtype))
    {
    case IS_IMMUNE:
            return TRUE;
    case IS_RESISTANT:
            resistance += 20;
            break;
    case IS_VULNERABLE:
            resistance -= 20;
            break;
    }

	// Remember saves are negative, so neg - neg == pos
	// Add a maximum of 30 from saving throws.
	resistance -= UMAX(victim->saving_throw, -30);

	// Mobs have innate saving throw bonus
	if(IS_NPC(victim)) {
		resistance += victim->level / 6;
	}

	// Adjustment: Minimum 5% chance to hit
	spellpower = spellpower - resistance;

	if(spellpower < 10)
		spellpower = 10;

	if(spellpower > 75)
		spellpower = 75;

    // Are your saves enough?
    if (number_percent() > spellpower) {
        char *name = can_see(victim, caster) ? IS_NPC(caster) ? caster->short_descr : caster->name : "someone";

        if (stat == STAT_STR)
            Cprintf(victim, "Your brute strength allows you to resist %s's magic.\n\r", name);
        if (stat == STAT_DEX)
            Cprintf(victim, "Your prowess helps you resist %s's magic.\n\r", name);
        if (stat == STAT_CON)
            Cprintf(victim, "You are too tough to be affected by %s's magic.\n\r", name);
        if (stat == STAT_INT)
            Cprintf(victim, "You see right through %s's magic.\n\r", name);
        if (stat == STAT_WIS)
            Cprintf(victim, "Your willpower overcomes %s's magic.\n\r", name);
        if (stat == STAT_NONE)
            Cprintf(victim, "You luckily manage to avoid %s's magic.\n\r", name);
        return TRUE;
    }

	// If they AREN'T enough, you might still get lucky.
	if(is_affected(victim, gsn_berserk) || IS_AFFECTED(victim, AFF_BERSERK)) 	{
		insanity += 15;
	}
	if(is_affected(victim, gsn_enrage)) {
		insanity += 15;
	}
	if(victim->race == race_lookup("dwarf")) {
		insanity += 10;
	}
	if(victim->charClass == class_lookup("paladin")) {
		insanity += 20;
	}

	if(number_percent() < insanity) {
		Cprintf(victim, "Magical forces roll right off you.\n\r",
			IS_NPC(caster) ? caster->short_descr : caster->name);
		return TRUE;
	}

	return FALSE;
}

// Try_cancel will do a very quick and dirty save to remove one affect.
bool
try_cancel(CHAR_DATA *ch, int sn, int level)
{
	int success = 0;
	AFFECT_DATA *paf;

	if((paf = affect_find(ch->affected, sn)) == NULL)
		return FALSE;

	// Examples:
	// Level 51 healer curing level 51 blind
	if(level <= 20) {
		success = (40 + level) - paf->level;
	}
	else if(level <= 40) {
		success = 60 - paf->level;
		success = success + ((level - 20) * 2);
	}
	else {
		success = 60 - paf->level;
		success = success + ((level - 20) * 2);
		success = success + ((level - 40) * 3);
	}

	success = URANGE(10, success, 90);

	return number_percent() < success;
}

/*
 * helper function to determine if a certain spell's in a certain spell
 * group. If there's an easier way, please let me know? :) - Litazia
 */
bool
spell_in_group(int sn, char *group)
{
	int gn, asn;

	gn = group_lookup(group);
	for (asn = 0; asn < MAX_IN_GROUP || group_table[gn].spells[asn] == NULL; asn++)
	{
		if (skill_table[sn].name == NULL)
			return FALSE;
		if (group_table[gn].spells[asn] == NULL)
			return FALSE;
		if (strcmp(group_table[gn].spells[asn], skill_table[sn].name) == 0)
			return TRUE;
	}
	return FALSE;
}


// Returns a constant determining the new target for the spell.
int
check_reflection(CHAR_DATA * ch, CHAR_DATA * elf)
{
	if (get_skill(elf, gsn_reflection) < 1)
	{
		return REFLECT_FAIL;
	}

	/* cool reflection now */
	if (number_percent() < get_skill(elf, gsn_reflection) / 6)
	{
		if (number_percent() > get_skill(elf, gsn_reflection) / 4)
		{
			if (IS_NPC(ch))
				Cprintf(elf, "%s's magic bounces off you!\n\r", ch->short_descr);
			else
				Cprintf(elf, "%s's magic bounces off you!\n\r", ch->name);
			Cprintf(ch, "Your magic bounces off %s!\n\r", elf->name);

			if(!IS_AWAKE(elf))
				return REFLECT_BOUNCE;

			damage(ch, elf, 0, gsn_bash, DAM_NONE, FALSE, TYPE_SKILL);
			return REFLECT_BOUNCE;
		}
		else
		{
			if (IS_NPC(ch))
				Cprintf(elf, "You reflect the spell back onto %s!\n\r", ch->short_descr);
			else
				Cprintf(elf, "You reflect the spell back onto %s!\n\r", ch->name);
			Cprintf(ch, "%s reflects your spell back onto you!\n\r", elf->name);

			if(!IS_AWAKE(elf))
                                return REFLECT_BOUNCE;

			damage(ch, elf, 0, gsn_bash, DAM_NONE, FALSE, TYPE_SKILL);
			return REFLECT_CASTER;
		}
	}

	return REFLECT_FAIL;
}

int check_leaf_shield(CHAR_DATA *ch, CHAR_DATA *victim) {
	AFFECT_DATA *paf = NULL;

	if (victim == NULL)
		return REFLECT_FAIL;

	if (ch != victim
	&& is_affected(victim, gsn_leaf_shield))
        {
                paf = affect_find(victim->affected, gsn_leaf_shield);
                act("Your leaf shield absorbs $N's magic!", victim, NULL, ch, TO_CHAR, POS_RESTING);
                act("$n's leaf shield absorbs the spell.", victim, NULL, NULL,TO_ROOM, POS_RESTING);
                if (number_percent() < 40)
                {
                        Cprintf(victim, "Your leaf shield breaks and scatters!\n\r");
                        act("$n's leaf shield breaks and scatters!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
                        affect_remove(victim, paf);
                }

                damage(ch, victim, 0, gsn_bash, DAM_NONE, FALSE, TYPE_SKILL);
		return REFLECT_BOUNCE;
        }

	return REFLECT_FAIL;
}

void
do_cast(CHAR_DATA * ch, char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    void *vo;
    int mana, tattoos = 0;
    int sn, hours;
    int target;
    // Base Caster Level
    unsigned char lvl=0;
    // Modified Level
    unsigned char modified_lvl=0;
    unsigned char ref=0;
    int tmp_lvl, spell_level=0; /* from spellstealing */
    bool grounded=FALSE, could_withstand=FALSE;
    int is_shadow = FALSE, shadow_cast = FALSE, chance = 0;

    paf = affect_find(ch->affected, gsn_spell_stealing);

    // Switched NPC's can cast spells, but others can't.
    if (IS_NPC(ch) && ch->desc == NULL) {
        return;
    }

    target_name = one_argument(argument, arg1);

    if (!str_cmp(arg1, "shadowcastxxx")) {
        is_shadow = TRUE;
        target_name = one_argument(target_name, arg1);
    }

    one_argument(target_name, arg2);

    if (arg1[0] == '\0') {
        Cprintf(ch, "Cast which what where?\n\r");
        return;
    }

    tmp_lvl = ch->level;
    sn = find_spell(ch, arg1);

    if (paf != NULL && paf->modifier == sn) {
        tmp_lvl = 60;
    }

    if (!IS_NPC(ch)
            && ch->pcdata != NULL
            && ch->pcdata->any_skill == TRUE) {
        tmp_lvl = 60;
    }

    if (sn < 1
            || skill_table[sn].spell_fun == spell_null
            || (!IS_NPC(ch) && (tmp_lvl < skill_table[sn].skill_level[ch->charClass]
                                                                      || ch->pcdata->learned[sn] == 0))) {
        Cprintf(ch, "You don't know any spells of that name.\n\r");
        return;
    }

    if (sn == gsn_acid_breath
            || sn == gsn_frost_breath
            || sn == gsn_gas_breath
            || sn == gsn_fire_breath
            || sn == gsn_lightning_breath) {
        if (!IS_NPC(ch)) {
            Cprintf(ch, "You don't know any spells of that name.\n\r");
            return;
        }
    }

    if (ch->position < skill_table[sn].minimum_position) {
        Cprintf(ch, "You can't concentrate enough.\n\r");
        return;
    }

    if (ch->level + 2 == skill_table[sn].skill_level[ch->charClass]) {
        mana = 50;
    } else {
        mana = UMAX(skill_table[sn].min_mana, 100 / (2 + ch->level - skill_table[sn].skill_level[ch->charClass]));
    }

    /* Alchemist penalty: non-combat mage! More expensive */
    if (!IS_NPC(ch) && ch->reclass == reclass_lookup("alchemist")
            && ch->fighting
            && number_percent() < 30
            && !skill_table[sn].remort) {
        Cprintf(ch, "Your hands shake a little from the stress, increasing the difficulty.\n\r");
        mana = mana + (mana / 4);
    }

    // Illusionst shadow magic
    if (!is_shadow && is_affected(ch, gsn_shadow_magic)) {
        // Less likely on expensive spells
        if (number_percent() < 30) {
            shadow_cast = TRUE;
        }
    }

    if(is_shadow) {
        mana = 0;
    }

    if (!IS_NPC(ch)
            && ch->pcdata != NULL
            && ch->pcdata->any_skill == TRUE) {
        mana = 0;
    }

    if (!IS_NPC(ch) && ch->mana < mana) {
        Cprintf(ch, "You don't have enough mana.\n\r");
        return;
    }

    // Special runist tattoo, reduce mana cost.
    tattoos = power_tattoo_count(ch, TATTOO_MV_SPELLS);
    if (tattoos > 0) {
        if (sn != gsn_refresh
                && (ch->mana < mana
                        || number_percent() < (20 + (15 * (tattoos - 1))))) {
            Cprintf(ch, "Your tattoo glows brightly, conserving your mana.\n\r");
            mana /= 2;
        } else {
            tattoos = 0;
        }
    }

    if (tattoos > 0 && ch->move < mana) {
        Cprintf(ch, "You don't have enough movement.\n\r");
        return;
    }

    /*
     * Locate targets.
     */
    victim = NULL;
    obj = NULL;
    vo = NULL;
    target = TARGET_NONE;

    switch (skill_table[sn].target) {
        default:
            bug("Do_cast: bad target for sn %d.", sn);
            return;

        case TAR_IGNORE:
            break;

        case TAR_CHAR_OFFENSIVE:
            if (arg2[0] == '\0') {
                if ((victim = ch->fighting) == NULL) {
                    Cprintf(ch, "Cast the spell on whom?\n\r");
                    return;
                }
            } else {
                if ((victim = get_char_room(ch, arg2)) == NULL) {
                    Cprintf(ch, "They aren't here.\n\r");
                    return;
                }
            }

            if (!IS_NPC(ch)) {
                if (is_safe(ch, victim) && victim != ch) {
                    Cprintf(ch, "Not on that target.\n\r");
                    return;
                }

                check_killer(ch, victim);
            }

            if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
                Cprintf(ch, "You can't do that on your own follower.\n\r");
                return;
            }

            ref =  check_reflection(ch, victim);

            if (ref == REFLECT_BOUNCE) {
                victim   = NULL;
                grounded = TRUE;
            } else if (ref == REFLECT_CASTER) {
                victim = ch;
            }

            ref = check_leaf_shield(ch, victim);

            if (ref == REFLECT_BOUNCE) {
                victim   = NULL;
                grounded = TRUE;
            }

            vo = (void *) victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_DEFENSIVE:
            if (arg2[0] == '\0') {
                victim = ch;
            } else {
                if ((victim = get_char_room(ch, arg2)) == NULL) {
                    Cprintf(ch, "They aren't here.\n\r");
                    return;
                }
            }

            vo = (void *) victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_SELF:
            if (arg2[0] != '\0' && !is_name(arg2, ch->name)) {
                Cprintf(ch, "You cannot cast this spell on another.\n\r");
                return;
            }

            vo = (void *) ch;
            victim = ch;
            target = TARGET_CHAR;
            break;

        case TAR_OBJ_INV:
            if (arg2[0] == '\0') {
                Cprintf(ch, "What should the spell be cast upon?\n\r");
                return;
            }

            if ((obj = get_obj_carry(ch, arg2, ch)) == NULL) {
                Cprintf(ch, "You are not carrying that.\n\r");
                return;
            }

            vo = (void *) obj;
            target = TARGET_OBJ;
            break;

        case TAR_OBJ_CARRYING:
            if (arg2[0] == '\0') {
                Cprintf(ch, "What should the spell be cast upon?\n\r");
                return;
            }

            if ((obj = get_obj_carry_or_wear(ch, arg2)) == NULL) {
                Cprintf(ch, "You are not carrying that.\n\r");
                return;
            }

            vo = (void *) obj;
            target = TARGET_OBJ;
            break;

        case TAR_OBJ_CHAR_OFF:
            if (arg2[0] == '\0') {
                if ((victim = ch->fighting) == NULL) {
                    Cprintf(ch, "Cast the spell on whom or what?\n\r");
                    return;
                }

                target = TARGET_CHAR;
            } else if ((victim = get_char_room(ch, arg2)) != NULL) {
                target = TARGET_CHAR;
            }

            if (target == TARGET_CHAR) {
                /* check the sanity of the attack */
                if (is_safe(ch, victim) && victim != ch) {
                    Cprintf(ch, "Not on that target.\n\r");
                    return;
                }

                if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
                    Cprintf(ch, "You can't do that on your own follower.\n\r");
                    return;
                }

                if (!IS_NPC(ch)) {
                    check_killer(ch, victim);
                }

                ref =  check_reflection(ch, victim);

                if (ref == REFLECT_BOUNCE) {
                    victim   = NULL;
                    grounded = TRUE;
                } else if(ref == REFLECT_CASTER) {
                    victim = ch;
                }

                ref = check_leaf_shield(ch, victim);

                if (ref == REFLECT_BOUNCE) {
                    victim   = NULL;
                    grounded = TRUE;
                }

                vo = (void *) victim;
            } else if ((obj = get_obj_here(ch, target_name)) != NULL) {
                vo = (void *) obj;
                target = TARGET_OBJ;
            } else {
                Cprintf(ch, "You don't see that here.\n\r");
                return;
            }

            break;

        case TAR_OBJ_CHAR_DEF:
            if (arg2[0] == '\0') {
                vo = (void *) ch;
                victim = ch;
                target = TARGET_CHAR;
            } else if ((victim = get_char_room(ch, arg2)) != NULL) {
                vo = (void *) victim;
                target = TARGET_CHAR;
            } else if ((obj = get_obj_carry(ch, arg2, ch)) != NULL) {
                vo = (void *) obj;
                target = TARGET_OBJ;
            } else {
                Cprintf(ch, "You don't see that here.\n\r");
                return;
            }

            break;
    }

    if (str_cmp(skill_table[sn].name, "ventriloquate")) {
        say_spell(ch, sn);
    }

    WAIT_STATE(ch, skill_table[sn].beats);

    if (number_percent() > get_skill(ch, sn)) {
        Cprintf(ch, "You lost your concentration.\n\r");
        check_improve(ch, sn, FALSE, 1);

        //(!tattoos ? ch->mana : ch->move) -= mana / 2;
        if (!tattoos) {
            ch->mana -= mana / 2;
        } else {
            ch->move -= mana / 2;
        }
    } else {
        if (ch->reclass == reclass_lookup("wizard")) {
            if (sn == ch->last_sn[0] && ch->last_sn[0] != 0 && sn == ch->last_sn[1] && ch->last_sn[1] != 0) {
                Cprintf(ch, "You are much too flamboyant to cast that spell again.\n\r");
                return;
            }
        }

        ch->last_sn[1] = ch->last_sn[0];
        ch->last_sn[0] = sn;

        //(!tattoos ? ch->mana : ch->move) -= mana;
        if (!tattoos) {
            ch->mana -= mana;
        } else {
            ch->move -= mana;
        }

        // Get Base Caster Level
        // Using charged item
        if (!IS_NPC(ch)
                && ch->pcdata != NULL
                && ch->pcdata->any_skill == TRUE) {
            lvl = ch->level;
        } else if (IS_NPC(ch) || class_table[ch->charClass].fMana) {
            // Mages
            lvl = ch->level;
        } else if (ch->charClass == class_lookup("paladin") && ch->reclass != reclass_lookup("cavalier")) {
            lvl = 5 * ch->level / 6;
        } else if (ch->charClass == class_lookup("ranger")) {
            lvl = 5 * ch->level / 6;
        } else if (ch->charClass == class_lookup("runist")) {
            lvl = 5 * ch->level / 6;
        } else {
            lvl = 3 * ch->level / 4;
        }

        // Remorts cast at level
        if (skill_table[sn].remort == 1) {
            lvl = ch->level;
        }

        // Find Spell Casting Modifiers
        modified_lvl = lvl;

        /* hours dependent casting level for elves
           done by Starcrossed */
        if (!skill_table[sn].remort
                && ch->race == race_lookup("elf")) {
            /* get hours! */
            hours = get_hours(ch);

            if (hours < 40) {
                modified_lvl = modified_lvl;
            } else if (hours <= 160) {
                modified_lvl++;
            } else if (hours <= 260) {
                modified_lvl += 2;
            } else if (hours <= 420) {
                modified_lvl += 3;
            } else if (hours > 420) {
                modified_lvl += 4;
            }
        }

        /* No level 59 spells for you buudddddy!! */
        if (number_percent() < get_skill(ch, gsn_faith) && skill_table[sn].remort != 1) {
            Cprintf(ch, "Faith strengthens your magic.\n\r");
            check_improve(ch, gsn_faith, TRUE, 4);
            modified_lvl += number_range(1, 6);
        }

        if((tattoos = power_tattoo_count(ch, TATTOO_RAISES_CASTING)) > 0) {
            Cprintf(ch, "Your tattoos glitter and focus your spell's power.\n\r");
            modified_lvl += (4 + (2 * (tattoos - 1)));
        }

        /* gargs get level dependent spells too, but only on
           certain groups - Litazia */
        if (ch->race == race_lookup("gargoyle")) {
            if (spell_in_group(sn, "protective")) {
                modified_lvl += 1;
            }

            if (spell_in_group(sn, "healing")) {
                modified_lvl += 2;
            }
        }

        if ((sn == gsn_sleep
                || sn == gsn_charm_person
                || sn == gsn_enchant_weapon
                || sn == gsn_enchant_armor)
                && ch->charClass == class_lookup("enchanter")) {
            modified_lvl += ((ch->level / 10) - 1);
        }

	if (ch->charClass == class_lookup("cleric") &&
	    spell_in_group(sn, "healing"))
	{
            if (ch->alignment <= -350)
	    {
                modified_lvl = modified_lvl * 9 / 10;
	    }
	    else if (ch->alignment < 350)
	    {
                modified_lvl = modified_lvl * 19 / 20;
	    }
	}

	if (ch->charClass == class_lookup("cleric") &&
	    spell_in_group(sn, "omination"))
	{
            if (ch->alignment >= 350)
	    {
                modified_lvl = modified_lvl * 14 / 15;
	    }
	    else if (ch->alignment > -350)
	    {
                modified_lvl = modified_lvl * 29 / 30;
	    }
	}

        // Set a limit on the sickness.
        if (!IS_IMMORTAL(ch) && modified_lvl > 55) {
            modified_lvl = 55;
        }

        /* diviner no more super invocations! */
        if (ch->reclass == reclass_lookup("diviner")
                && spell_in_group(sn, "invocation")) {
            modified_lvl = modified_lvl * 9 / 10;
        }

        /* herbalist half assed in some places */
        if (ch->reclass == reclass_lookup("herbalist")
                && ch->in_room != NULL
                && (ch->in_room->sector_type == SECT_CITY
                        || ch->in_room->sector_type == SECT_INSIDE)) {
            modified_lvl = modified_lvl * 9 / 10;
        }

        /* mystic gotta meditate or else! */
        if (ch->reclass == reclass_lookup("mystic")) {
            modified_lvl -= (ch->meditate_needed);
            modified_lvl = UMAX(1, modified_lvl);
        }

        /* shaman no more super transport */
        if (ch->reclass == reclass_lookup("shaman")
                && (sn == gsn_summon
                        ||  sn == gsn_gate
                        ||  sn == gsn_teleport
                        ||  sn == gsn_nexus
                        ||  sn == gsn_portal
                        ||  sn == gsn_carnal_reach
                        ||  sn == gsn_spirit_link)) {
            modified_lvl = modified_lvl * 9 / 10;
        }

        // Illusionist shadow magic
        // Spells at 51
        // Shadow spells at 42
        // Enchantments at 48
        // Shadow enchantments at 40
        if (is_shadow) {
            lvl = lvl * 5 / 6;
            modified_lvl = modified_lvl * 5 / 6;
        }

        if (victim != NULL && is_affected(victim, gsn_withstand_death)) {
            could_withstand = TRUE;
        }

        // Ok, to pass on the two bytes that make up level, we need to
        // mass em together.
        spell_level = generate_int(lvl, modified_lvl);

        // The new way to check reflect... should be harder to spam now.
        if (skill_table[sn].target == TAR_CHAR_OFFENSIVE
                || skill_table[sn].target == TAR_OBJ_CHAR_OFF) {
            if (victim != NULL
                    && ch != victim
                    && ch->clan != victim->clan
                    && get_skill(victim, gsn_reflection) > 0) {
                if (ref == REFLECT_BOUNCE
                        || ref == REFLECT_CASTER) {
                    check_improve(victim, gsn_reflection, TRUE, 2);
                } else {
                    check_improve(victim, gsn_reflection, FALSE, 2);
                }
            }
        }

        if (grounded == FALSE) {
            (*skill_table[sn].spell_fun) (sn, spell_level, ch, vo, target);
        }

        check_improve(ch, sn, TRUE, 1);

        /* they withstood death, leave them alone */

        if (victim != NULL
                && !is_affected(victim, gsn_withstand_death)
                && could_withstand
                && victim->hit == 100) {
            stop_fighting(victim, TRUE);
            return;
        }
    }

    if ((skill_table[sn].target == TAR_CHAR_OFFENSIVE
            || (skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR))
            && victim != NULL
            && victim != ch
            && victim->master != ch) {
        // They died, don't start a battle
        if(victim->pktimer > 0) {
            return;
        }

        check_killer(ch, victim);

        if (!IS_NPC(ch)
                && !IS_NPC(victim)) {
            ch->no_quit_timer = 3;
            victim->no_quit_timer = 3;
        }

        // If sleep works, don't attack.
        if (!IS_AWAKE(victim)) {
            return;
        }

        // This will let you doublecast more things
        if (!shadow_cast) {
            damage(ch, victim, 0, sn, DAM_NONE, FALSE, TYPE_MAGIC);
        }
    }

    if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 85) {
        Cprintf(ch, "!!SOUND(sounds/wav/magic*.wav V=80 P=20 T=admin)");
    }

    // Try a shadow cast
    if (shadow_cast == TRUE
            && !is_shadow
            && (chance = get_skill(ch, gsn_shadow_magic)) > 0) {
        char buf[255];
        sprintf(buf, "shadowcastxxx %s", argument);

        // Check for charm person here
        if (sn == gsn_charm_person
                && victim
                && victim->master == ch) {
            return;
        }

        Cprintf(ch, "{DA shadow of the magic fades into existence.{x\n\r");
        do_cast(ch, buf);
    }

    return;
}



/*
 * Cast spells at targets using a magical object.
 */
void
obj_cast_spell(int sn, int level, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{
	void *vo;
	int target = TARGET_NONE;
	bool grounded;
	int ref=0, spell_level;

	grounded = FALSE;

	if (sn <= 0)
		return;

	if (sn >= MAX_SKILL || skill_table[sn].spell_fun == spell_null)
	{
		bug("Obj_cast_spell: bad sn %d.", sn);
		return;
	}

	switch (skill_table[sn].target)
	{
	default:
		bug("Obj_cast_spell: bad target for sn %d.", sn);
		return;

	case TAR_IGNORE:
		vo = NULL;
		break;

	case TAR_CHAR_OFFENSIVE:
		if (victim == NULL)
			victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "You can't do that.\n\r");
			return;
		}
		if (!IS_NPC(ch))
		{
			if (is_safe(ch, victim) && victim != ch)
			{
				Cprintf(ch, "Not on that target.\n\r");
				return;
			}
			check_killer(ch, victim);
		}

		if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
		{
			Cprintf(ch, "You can't do that on your own follower.\n\r");
			return;
		}

		ref =  check_reflection(ch, victim);
		if(ref == REFLECT_BOUNCE) {
			victim   = NULL;
			grounded = TRUE;
		}
		else if(ref == REFLECT_CASTER)
			victim = ch;

		ref = check_leaf_shield(ch, victim);
		if(ref == REFLECT_BOUNCE) {
        		victim   = NULL;
        		grounded = TRUE;
		}

		vo = (void *) victim;
		target = TARGET_CHAR;
		break;

	case TAR_CHAR_DEFENSIVE:
	case TAR_CHAR_SELF:
		if (victim == NULL)
			victim = ch;
		vo = (void *) victim;
		target = TARGET_CHAR;
		break;

	case TAR_OBJ_INV:
	case TAR_OBJ_CARRYING:
		if (obj == NULL)
		{
			Cprintf(ch, "You can't do that.\n\r");
			return;
		}
		vo = (void *) obj;
		target = TARGET_OBJ;
		break;

	case TAR_OBJ_CHAR_OFF:
		if (victim == NULL && obj == NULL)
		{
			if (ch->fighting != NULL)
				victim = ch->fighting;
			else
			{
				Cprintf(ch, "You can't do that.\n\r");
				return;
			}
		}

		if (victim != NULL)
		{
			if (is_safe(ch, victim) && ch != victim)
			{
				Cprintf(ch, "Something isn't right...\n\r");
				return;
			}

			ref =  check_reflection(ch, victim);
			if(ref == REFLECT_BOUNCE) {
				victim = NULL;
				grounded = TRUE;
			}
			else if(ref == REFLECT_CASTER)
				victim = ch;

			ref = check_leaf_shield(ch, victim);
			if(ref == REFLECT_BOUNCE) {
        			victim   = NULL;
        			grounded = TRUE;
			}

			vo = (void *) victim;
			target = TARGET_CHAR;
		}
		else
		{
			vo = (void *) obj;
			target = TARGET_OBJ;
		}
		break;

	case TAR_OBJ_CHAR_DEF:
		if (victim == NULL && obj == NULL)
		{
			vo = (void *) ch;
			victim = ch;
			target = TARGET_CHAR;
		}
		else if (victim != NULL)
		{
			vo = (void *) victim;
			target = TARGET_CHAR;
		}
		else
		{
			vo = (void *) obj;
			target = TARGET_OBJ;
		}

		break;
	}

	if(skill_table[sn].target == TAR_CHAR_OFFENSIVE
        || skill_table[sn].target == TAR_OBJ_CHAR_OFF) {
		if(victim != NULL
		&& get_skill(victim, gsn_reflection) > 0
		&& ch != victim
		&& ch->clan != victim->clan) {
			if(ref == REFLECT_BOUNCE
			|| ref == REFLECT_CASTER)
				check_improve(victim, gsn_reflection, TRUE, 2);
			else
				check_improve(victim, gsn_reflection, FALSE, 2);
		}
	}

	// No modifiers
	spell_level = generate_int(level, level);

	if (grounded == FALSE)
	{
		(*skill_table[sn].spell_fun) (sn, spell_level, ch, vo, target);
	}

	if ((skill_table[sn].target == TAR_CHAR_OFFENSIVE
	|| (skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR))
		&& victim != NULL
		&& victim != ch
		&& victim->master != ch)
	{
		check_killer(ch, victim);
		// If sleep works, don't attack.
                if(!IS_AWAKE(victim))
                        return;

                damage(ch, victim, 0, sn, DAM_NONE, FALSE, TYPE_MAGIC);
	}

	return;
}


/*
 * Spell functions.
 */
void
spell_acid_blast(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam=0;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	damage(ch, victim, dam, sn, DAM_ACID, TRUE, TYPE_MAGIC);
	return;
}

void
spell_animal_growth(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{

	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your animal growth.\n\r");
		else
		{
			act("$N refreshed your animal growth.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the animal growth.\n\r");
		}
		affect_refresh(victim, sn, modified_level + 8);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 8 + modified_level;
	af.location = APPLY_STR;
	af.modifier = 3;
	af.bitvector = 0;
	affect_to_char(victim, &af);

	af.location = APPLY_CON;
	affect_to_char(victim, &af);

	act("$n looks like an animal.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(victim, "You look like an animal.\n\r");
	return;
}

void
spell_armor(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your armour spell.\n\r");
		else
		{
			act("$N refreshed your armour spell.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the armour spell.\n\r");
		}
		affect_refresh(victim, sn, 24);
		return;
	}
	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 24;
	af.modifier = -20;
	af.location = APPLY_AC;
	af.bitvector = 0;

	affect_to_char(victim, &af);

	Cprintf(victim, "You feel someone protecting you.\n\r");
	if (ch != victim)
		act("$N is protected by your magic.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_bark_skin(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your bark skin.\n\r");
		else
		{
			act("$N refreshed your bark skin.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the bark skin.\n\r");
		}
		affect_refresh(victim, sn, modified_level + 8);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 8 + modified_level;
	af.location = APPLY_AC;
	af.modifier = -20;
	af.bitvector = 0;
	affect_to_char(victim, &af);
	act("$n looks as mighty as an oak.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(victim, "You are as mighty as an oak.\n\r");
}

void
spell_blast_of_rot(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam=0, caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	damage(ch, victim, dam, sn, DAM_POISON, TRUE, TYPE_MAGIC);
	return;
}

void
spell_bless(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	AFFECT_DATA af;
	AFFECT_DATA* paf;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* deal with the object case first */
	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;
		paf = affect_find(obj->affected, gsn_bless);
		if (paf == NULL && IS_OBJ_STAT(obj, ITEM_BLESS))
		{
			act("$p is already blessed.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			return;
		}
		else if (paf != NULL)
		{
			while((paf = affect_find(obj->affected, gsn_bless)) != NULL)
				affect_remove_obj(obj, paf);
		}

		if (IS_OBJ_STAT(obj, ITEM_EVIL))
		{
			AFFECT_DATA *paf;

			if (number_percent() < 50 + ((modified_level - obj->level) * 5))
			{
				act("$p glows a pale blue.", ch, obj, NULL, TO_ALL, POS_RESTING);
				while((paf = affect_find(obj->affected, gsn_curse)) != NULL)
					affect_remove_obj(obj, paf);

				REMOVE_BIT(obj->extra_flags, ITEM_EVIL);
				return;
			}
			else
			{
				act("The evil of $p is too powerful for you to overcome.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				return;
			}
		}

		af.where = TO_OBJECT;
		af.type = sn;
		af.level = modified_level;
		af.duration = 6 + modified_level;
		af.location = APPLY_SAVES;
		af.modifier = -1;
		af.bitvector = ITEM_BLESS;
		affect_to_obj(obj, &af);
		act("$p glows with a holy aura.", ch, obj, NULL, TO_ALL, POS_RESTING);
		return;
	}

	/* character target */
	victim = (CHAR_DATA *) vo;

	if (victim->position == POS_FIGHTING)
	{
		if (victim == ch)
			Cprintf(ch, "You can't bless yourself while engaged in bloody battle!\n\r");
		else
			act("You can't bless $N, while $e is in bloody battle!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your blessing.\n\r");
		else
		{
			act("$N refreshed your blessing.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the blessing spell.\n\r");
		}
		affect_refresh(victim, sn, modified_level + 6);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 6 + modified_level;
	af.location = APPLY_HITROLL;
	af.modifier = modified_level / 8;
	af.bitvector = 0;
	affect_to_char(victim, &af);

	af.location = APPLY_SAVING_SPELL;
	af.modifier = 0 - modified_level / 8;
	affect_to_char(victim, &af);
	Cprintf(victim, "You feel righteous.\n\r");
	if (ch != victim)
		act("You grant $N the favor of your god.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_blindness(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_AFFECTED(victim, AFF_BLIND))
	{
		act("$e is already blinded.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (saving_throw(ch, victim, gsn_blindness, modified_level - 4, SAVE_NORMAL, STAT_INT, DAM_NEGATIVE))
	{
		Cprintf(ch, "Your victim retains their sight.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.location = APPLY_HITROLL;
	af.modifier = -4;
	af.duration = 1 + (modified_level / 12);
	if (IS_NPC(victim))
	  af.duration *= 2;
	af.bitvector = AFF_BLIND;

	affect_to_char(victim, &af);
	Cprintf(victim, "You are blinded!\n\r");
	act("$n appears to be blinded.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	return;
}

void
spell_blink(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{

	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	int door;
	int count = 0;
	int max_count = 0;

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
        {
                Cprintf(ch, "Not in this room.\n\r");
                return;
        }

	if(number_percent() < 20) {
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	Cprintf(ch, "You blink out of sight!\n\r");
	act("$n blinks out of sight!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	stop_fighting(ch, TRUE);
	while (count < 3 && max_count < 20)
	{
		door = dice(1, 6) - 1;
		in_room = ch->in_room;
		if ((pexit = in_room->exit[door]) == NULL
			|| (to_room = pexit->u1.to_room) == NULL
			|| !can_see_room(ch, pexit->u1.to_room)
			|| door < 0
			|| door > 5
			|| (!is_room_owner(ch, to_room) && room_is_private(to_room))
			|| (to_room->clan))
		{
			max_count++;

		}
		else
		{
			max_count++;
			count++;

			char_from_room(ch);
			char_to_room(ch, to_room);
			if (!IS_AFFECTED(ch, AFF_SNEAK)
				&& ch->invis_level < LEVEL_HERO)
				act("$n passes by.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

			if (in_room == to_room)		/* no circular follows */
				return;

		}
	}
	do_look(ch, "auto");
}

void
spell_blur(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your blur.\n\r");
		else
		{
			act("$N refreshed your blur.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the blur.\n\r");
		}
		affect_refresh(victim, sn, modified_level);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level;
	af.location = APPLY_AC;
	af.modifier = -40;
	af.bitvector = 0;
	affect_to_char(victim, &af);

	act("$n's outline turns blurry.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(victim, "You become blurry.\n\r");
	return;

}

void
spell_burning_hands(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(modified_level <= 15)
		dam = 5 + spell_damage(ch, victim, modified_level, SPELL_DAMAGE_CHART_GOOD, TRUE);
	else
		dam = 20 + spell_damage(ch, victim, modified_level, SPELL_DAMAGE_CHART_POOR, TRUE);

	damage(ch, victim, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);
	return;
}

void
spell_call_creature(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{

	CHAR_DATA *victim;
	CHAR_DATA *vch;
	AFFECT_DATA af;
	int mob_skip;
	int i, j, caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room\n\r");
		return;
	}

	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		mob_skip = dice(1, 100);
		for (i = 0; i < mob_skip; i++)
		{
			victim = victim->next;
			if (victim == NULL)
			{
				Cprintf(ch, "You failed to call a creature.\n\r");
				return;
			}
		}

		if (!IS_NPC(victim)
			|| IS_AFFECTED(victim, AFF_CHARM)
			|| IS_AFFECTED(ch, AFF_CHARM)
			|| IS_SET(victim->imm_flags, IMM_CHARM)
			|| (modified_level > 51 && victim->level < 46)
			|| (modified_level <= 51 && victim->level > modified_level + 10)
			|| (modified_level <= 51 && victim->level < modified_level - 5)
			|| victim == ch
			|| victim->in_room == NULL
			|| (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
			|| victim->fighting != NULL
			|| (IS_NPC(victim) && IS_SET(victim->act, ACT_IS_WIZI))
			|| (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
			|| (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
			|| victim->in_room->area->security < 9)
		{
			continue;
		}

		vch = victim;
		j = 0;
		for (victim = char_list; victim != NULL; victim = victim->next)
		{
			if (victim->master == ch && IS_NPC(victim))
				j++;
		}

		if (j > 8)
		{
			Cprintf(ch, "You cannot control that many creatures at once.\n\r");
			return;
		}

		victim = vch;

		char_from_room(victim);
		char_to_room(victim, ch->in_room);
		act("$n appears out of nowhere.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		if (victim->master)
			stop_follower(victim);
		add_follower(victim, ch);
		victim->leader = ch;

		af.where = TO_AFFECTS;
		af.type = gsn_charm_person;
		af.level = modified_level;
		af.duration = modified_level / 2;
		af.location = 0;
		af.modifier = 0;
		af.bitvector = AFF_CHARM;
		affect_to_char(victim, &af);
		act("Isn't $n just so nice?", ch, NULL, victim, TO_VICT, POS_RESTING);
		if (ch != victim)
			return;
	}
	Cprintf(ch, "You failed to call a creature.\n\r");

	return;
}


void
spell_call_lightning(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(is_affected(ch, sn)) {
		Cprintf(ch, "You are already surrounded by lightning.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 4;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = 0;

	affect_to_char(ch, &af);

	Cprintf(ch, "You are surrounded by an aura of crackling lightning.\n\r");
	act("$n is surrounded by an aura of crackling lightning.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	return;
}

void
call_lightning_update()
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch, *vch;
	CHAR_DATA *vch_next;
	int dam=0;
	int found = FALSE;

	for (d = descriptor_list; d != NULL; d = d->next) {
		ch = d->character;

		if(ch == NULL)
			continue;

		if(!is_affected(ch, gsn_call_lightning))
			continue;

		if (!IS_OUTSIDE(ch)
		|| weather_info.sky < SKY_RAINING)
		{
			return;
		}

		Cprintf(ch, "The lightning around you flares brightly and a huge thunderbolt strikes from the sky!\n\r");
		act("$n glows brightly and a huge thunderbolt strikes from the sky!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

		for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
			vch_next = vch->next_in_room;
			if (vch == ch)
				continue;

			if(vch->fighting != ch)
				continue;

			found = TRUE;
			dam = spell_damage(ch, vch, ch->level, SPELL_DAMAGE_HIGH, TRUE);
			damage(ch, vch, dam, gsn_call_lightning, DAM_LIGHTNING, TRUE, TYPE_MAGIC);
		}

		if(!found) {
			Cprintf(ch, "The thunderbolt strikes the ground harmlessly.\n\r");
		}

	}

	return;
}


void
spell_call_to_arms(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	int count = 0;

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (IS_NPC(victim) && victim->master == ch)
		{
			if(victim->fighting) {
				if(number_percent() > 60)
					continue;
				else
					stop_fighting(victim, TRUE);
			}

			if(ch->in_room->area->continent !=
			victim->in_room->area->continent)
				continue;

			act("$n has been called to its master.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			char_from_room(victim);
			char_to_room(victim, ch->in_room);
			act("$n has called you!", ch, NULL, victim, TO_VICT, POS_RESTING);
			count++;
		}
	}

	if (count > 0)
	{
		Cprintf(ch, "%d charmies summoned.\n\r", count);
	}
	else
		Cprintf(ch, "No one responds to your call.\n\r");

	return;
}

/* RT calm spell stops all fighting in the room */

void
spell_calm(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *vch;
	int count = 0;
	int chance;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* get sum of all mobiles in the room */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (vch->position == POS_FIGHTING &&
		    !is_safe(ch, vch))
		{
			count++;
		}
	}

	/* just to stop divide by zero */
	if (count == 0)
		count = 1;

	/* compute chance of stopping combat */
	chance = modified_level * 2 / count;
	chance = UMIN(chance, 90);

	if (IS_IMMORTAL(ch))		/* always works */
		chance = 100;

	if (number_percent() < chance)		/* hard to stop large fights */
	{
		for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
		{
			if (IS_NPC(vch) && (IS_SET(vch->imm_flags, IMM_MAGIC) ||
			    IS_SET(vch->act, ACT_UNDEAD)))
				continue;

			if (is_safe(ch, vch))
				continue;

			if (saving_throw(ch, vch, gsn_calm, caster_level + 2, SAVE_HARD, STAT_STR, DAM_NONE))
				continue;

			Cprintf(vch, "A wave of calm passes over you.\n\r");
			if(vch != ch)
				Cprintf(ch, "%s calms down and loses the will to fight.\n\r",
					IS_NPC(vch) ? vch->short_descr : vch->name);


			if (IS_AFFECTED(vch, AFF_BERSERK))
			{
				affect_strip(vch, gsn_berserk);
			}

			if (is_affected(vch, gsn_frenzy))
			{
				affect_strip(vch, gsn_frenzy);
			}

			if (vch->fighting || vch->position == POS_FIGHTING)
				stop_fighting(vch, FALSE);

			if (is_affected(vch, sn))
			{
				continue;
			}

			af.where = TO_AFFECTS;
			af.type = sn;
			af.level = modified_level;
			af.duration = modified_level / 6;
			af.location = APPLY_HITROLL;
			af.modifier = -5;
			af.bitvector = AFF_CALM;
			affect_to_char(vch, &af);

			af.location = APPLY_DAMROLL;
			affect_to_char(vch, &af);
		}
	}
}

void
spell_cancellation(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	bool found = FALSE;
	int spells_removed=0, affect_count=0;
	int caster_level, modified_level;
	int target_affects[100], selected = 0;
	int i, sp, forbidden = FALSE;
	AFFECT_DATA *paf;
	char buf_vict[255];

	// Stuff that can't be dispelled.
	struct no_cancel_tag {
        	int sn;
        	int no_cancel;
	};
	struct no_cancel_tag no_cancel_table[] =
	{
		{ gsn_aura,			TRUE  },
		{ gsn_jail,			TRUE  },
		{ gsn_floating_disc,		TRUE  },
       		{ gsn_plague,                   TRUE  },
        	{ gsn_poison,                   TRUE  },
        	{ gsn_nature_protection,        TRUE  },
        	{ gsn_nightmares,               TRUE  },
        	{ gsn_delayed_blast_fireball,   TRUE  },
		{ gsn_karma,			TRUE  },
                { gsn_fire_breath,              TRUE  },
		{ gsn_sunray,			TRUE  },
		{ gsn_wrath,			TRUE  },
		{ gsn_telepathy,		TRUE  },
		{ gsn_spell_stealing,           TRUE  },
		{ -1,				FALSE },
	};

	// Restrictions on cancellation:
	if (IS_SET(victim->act, PLR_NOCAN) && ch != victim) {
		Cprintf(victim, "You evade an attempt at cancellation.\n\r");
        	act("$N evades an attempt at cancellation.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		Cprintf(ch, "They avoid your attempt at cancellation, don't bother trying.\n\r");
		return;
	}

	if (!IS_NPC(ch) && IS_NPC(victim))
	{
		Cprintf(ch, "You failed, try dispel magic.\n\r");
		return;
	}

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);
	spells_removed = number_range(3, 9);

	for(sp = 0; sp < spells_removed; sp++) {
		// Count up our list of valid affects
		affect_count = 0;
		for(paf = victim->affected; paf != NULL; paf = paf->next) {
			i = 0;
			forbidden = FALSE;
			while(TRUE) {
				if(no_cancel_table[i].sn == -1)
					break;
				if(no_cancel_table[i].sn == paf->type
				&& no_cancel_table[i].no_cancel) {
					forbidden = TRUE;
					break;
				}
				i++;
			}
			if(!forbidden) {
				target_affects[affect_count] = paf->type;
				affect_count++;
			}
		}

		// Any more affects?
		if(affect_count == 0)
			break;

		// Pick one affect
		selected = number_range(0, affect_count);

		// Don't always work
		if(!try_cancel(ch, target_affects[selected], modified_level)) {
			spells_removed--;
			continue;
		}

		// Is it a valid affect?
		if(skill_table[target_affects[selected]].spell_fun != spell_null) {
			// Ok, remove it now.
			if(skill_table[target_affects[selected]].msg_off)
				Cprintf(victim, "%s\n\r", skill_table[target_affects[selected]].msg_off);

			sprintf(buf_vict, "%s is no longer affected by %s!",
				IS_NPC(victim) ? victim->short_descr : victim->name,
				skill_table[target_affects[selected]].name);

			if(victim != ch)
				Cprintf(ch, "%s\n\r", buf_vict);
			act(buf_vict, ch, NULL, victim, TO_NOTVICT, POS_RESTING);
			// Call end function if needed
			if(skill_table[target_affects[selected]].end_fun != end_null)
				skill_table[target_affects[selected]].end_fun((void*)ch, TARGET_CHAR);

			affect_strip(victim, target_affects[selected]);
			spells_removed--;
			found = TRUE;
		}
	}

	if (found)
		Cprintf(ch, "Ok.\n\r");
	else
		Cprintf(ch, "You failed to cancel any spells.\n\r");

	return;
}

void
spell_carnal_reach(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* cancels out the normal +13 bonus */
	if(ch->reclass == reclass_lookup("shaman"))
		modified_level -= 13;

	if ((victim = get_char_world(ch, target_name, FALSE)) == NULL
		|| victim == ch
		|| victim->in_room == NULL
		|| IS_SET(ch->in_room->room_flags, ROOM_SAFE)
		|| IS_SET(victim->in_room->room_flags, ROOM_SAFE)
		|| ch->in_room->clan
		|| victim->in_room->clan
		|| victim->level >= modified_level + 13
		|| (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL)
		|| victim->fighting != NULL
		|| (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
		|| (IS_NPC(victim) && IS_AFFECTED(victim, AFF_CHARM))
		|| (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOSUMMON))
		|| (IS_NPC(victim) && IS_AFFECTED(victim, AFF_CHARM) && victim->master != ch)
		|| (IS_NPC(victim)
		&& number_percent() < 20))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (victim->in_room->area->continent != ch->in_room->area->continent)
	{
		Cprintf(ch, "Inter-continental magic is forbidden.  You failed.\n\r");
		return;
	}
	if (victim->in_room->area->security < 9)
	{
		Cprintf(ch, "Not in unfinished areas.  Sorry.\n\r");
		return;
	}

	if (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
		REMOVE_BIT(victim->act, ACT_AGGRESSIVE);

	act("$n disappears suddenly.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	char_from_room(victim);
	char_to_room(victim, ch->in_room);
	act("$n arrives suddenly.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	act("$n has reached your carnal body!", ch, NULL, victim, TO_VICT, POS_RESTING);
	do_look(victim, "auto");
	return;

}

void
spell_cause_critical(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	damage(ch, victim, dice(3, 8) + modified_level - 6, sn, DAM_HARM, TRUE, TYPE_MAGIC);
	return;
}

void
spell_cause_discordance(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	CHAR_DATA *fch;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (ch == victim)
	{
		Cprintf(ch, "If you want to fight your own group ... use the kill command.\n\r");
		return;
	}

	act("$N tries to spread discord in your group!", victim, NULL, ch, TO_CHAR, POS_RESTING);
	act("You try to spread discord in $N's group!", ch, NULL, victim, TO_CHAR, POS_RESTING);

	for (fch = victim->in_room->people; fch != NULL; fch = fch->next_in_room)
	{
		if (fch->master == victim
			&& (fch->position > POS_RESTING)
			&& can_see(fch, victim))
		{
			if (IS_NPC(fch))
			{

				if (saving_throw(ch, fch, sn, caster_level + 2, SAVE_NORMAL, STAT_WIS, DAM_NONE))
        				continue;
				stop_fighting(fch, FALSE);
				stop_follower(fch);
				Cprintf(fch, "You scream and attack your group leader!!\n\r");
				act("$n screams and suddenly attacks YOU!", fch, NULL, victim, TO_VICT, POS_RESTING);
				act("$n screams and suddenly attacks $N!", fch, NULL, victim, TO_NOTVICT, POS_RESTING);
				multi_hit(fch, victim, TYPE_UNDEFINED);
			}
			else
			{
				stop_fighting(fch, FALSE);
				stop_follower(fch);
			}
		}
	}
	return;
}

void
spell_cause_light(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	damage(ch, victim, dice(1, 8) + modified_level / 3, sn, DAM_HARM, TRUE, TYPE_MAGIC);
	return;
}

void
spell_cause_serious(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	damage(ch, victim, dice(2, 8) + modified_level / 2, sn, DAM_HARM, TRUE, TYPE_MAGIC);
	return;
}

void
spell_cave_bears(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	AFFECT_DATA af3;
	int nombre;
	int mobcount = 0;
	int insect;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	for (victim = char_list; victim != NULL; victim = victim->next) {
		if (victim->master == ch && IS_NPC(victim)
                && victim->pIndexData->vnum == MOB_VNUM_BEAR) {
                    mobcount++;
                    if (mobcount > 3)
                    {
                            Cprintf(ch, "You have already emptied the local caves of bears.\n\r");
                            return;
                    }
		}
        }

        if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
        {
                Cprintf(ch, "Not in this room.\n\r");
                return;
        }

	nombre = dice(1, 4);
	Cprintf(ch, "You grunt: 'Come my bears!'\n\r");
	for (insect = 0; insect < nombre; insect++)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_BEAR));
		char_to_room(victim, ch->in_room);
		size_mob(ch, victim, modified_level * 4 / 5);
		SET_BIT(victim->toggles, TOGGLES_NOEXP);
		victim->max_hit = victim->max_hit * 6 / 5;
		victim->hit = victim->max_hit;
		victim->hitroll = modified_level / 5;
        	victim->damroll = modified_level / 2 + 6;
		if (victim->master)
			stop_follower(victim);
		add_follower(victim, ch);
		victim->leader = ch;

		af3.where = TO_AFFECTS;
		af3.type = sn;
		af3.level = modified_level;
		af3.duration = modified_level / 2;
		af3.location = 0;
		af3.modifier = 0;
		af3.bitvector = AFF_CHARM;
		affect_to_char(victim, &af3);

	}
}

void
spell_chain_lightning(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	CHAR_DATA *tmp_vict, *last_vict, *next_vict;
	bool found;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* first strike */
	act("A lightning bolt leaps from $n's hand and arcs to $N.", ch, NULL, victim, TO_ROOM, POS_RESTING);
	act("A lightning bolt leaps from your hand and arcs to $N.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	act("A lightning bolt leaps from $n's hand and strikes you!", ch, NULL, victim, TO_VICT, POS_RESTING);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
	damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);

	last_vict = victim;
	modified_level -= 5;					/* decrement damage */

	/* new targets */
	while (modified_level > 0)
	{
		found = FALSE;
		for (tmp_vict = ch->in_room->people;
			 tmp_vict != NULL;
			 tmp_vict = next_vict)
		{
			next_vict = tmp_vict->next_in_room;
			if (!is_safe(ch, tmp_vict) && tmp_vict != last_vict)
			{
				found = TRUE;
				last_vict = tmp_vict;
				act("The bolt arcs to $n!", tmp_vict, NULL, NULL, TO_ROOM, POS_RESTING);
				act("The bolt strikes you!", tmp_vict, NULL, NULL, TO_CHAR, POS_RESTING);
				dam = spell_damage(ch, tmp_vict, modified_level, SPELL_DAMAGE_LOW, TRUE);

				damage(ch, tmp_vict, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);

				// decrement damage
				modified_level -= 5;
			}
		}

		// no target found, hit the caster
		if (!found)
		{
			if (ch == NULL)
				return;

			if (last_vict == ch)
			{
				act("The bolt seems to have fizzled out.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
				act("The bolt grounds out through your body.", ch, NULL, NULL, TO_CHAR, POS_RESTING);
				return;
			}

			last_vict = ch;
			act("The bolt arcs to $n...whoops!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(ch, "You are struck by your own lightning!\n\r");
			dam = spell_damage(ch, ch, modified_level, SPELL_DAMAGE_LOW, TRUE);

			damage(ch, ch, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);

			if (ch == NULL)
				return;
		}
		/* now go back and find more targets */
	}
}


void
spell_change_sex(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You've already been changed.\n\r");
		else
			act("$N has already had $s(?) sex changed.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_STR, DAM_NONE))
		return;

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 6;
	af.location = APPLY_SEX;
	do
	{
		af.modifier = number_range(0, 2) - victim->sex;
	}
	while (af.modifier == 0);
	af.bitvector = 0;
	Cprintf(victim, "You feel different.\n\r");
	act("$n doesn't look like $mself anymore...", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	affect_to_char(victim, &af);
	return;
}



void
spell_charm_person(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	CHAR_DATA *vch;
	AFFECT_DATA af;
	int chance;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_safe(ch, victim))
		return;

	if (victim->fighting != NULL)
	{
		Cprintf(ch, "You better wait till they are done what they are doing first.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You like yourself even better!\n\r");
		return;
	}

	/* no more sickness please mommy */
	if (victim->level - ch->level > 9) {
		Cprintf(ch, "They are too mighty for you to control, for now.\n\r");
		return;
	}

	if (IS_AFFECTED(victim, AFF_CHARM)
	|| IS_AFFECTED(ch, AFF_CHARM)
	|| IS_SET(victim->imm_flags, IMM_CHARM)
	|| saving_throw(ch, victim, sn, modified_level + 3, SAVE_NORMAL, STAT_WIS, DAM_CHARM)
	|| victim->hunting != NULL) {
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	chance = 100;

	for (vch = char_list; vch != NULL; vch = vch->next)
	{
		if (vch->master == ch && IS_NPC(vch))
			chance = chance - 6;
	}

	if (number_percent() > chance)
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
		REMOVE_BIT(victim->act, ACT_AGGRESSIVE);

	if (victim->master)
		stop_follower(victim);
	add_follower(victim, ch);
	victim->leader = ch;

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 4;
	if (IS_NPC(victim))
	  af.duration *= 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(victim, &af);
	act("Isn't $n just so nice?", ch, NULL, victim, TO_VICT, POS_RESTING);
	if (ch != victim)
		act("$N looks at you with adoring eyes.", ch, NULL, victim, TO_CHAR, POS_RESTING);

	if (!IS_NPC(victim)
        && ch != victim) {
                ch->no_quit_timer = 3;
                victim->no_quit_timer = 3;
        }

	return;
}

void
spell_chill_touch(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int dam, saved;
	int caster_level, modified_level;
	static const int dam_each[] =
	{
		0,  0,  6,  7,  8,  9,  12, 13, 13, 13,
		14, 14, 14, 15, 15, 15, 16, 16, 16, 17,
		17, 17, 18, 18, 18, 19, 19, 19, 20, 20,
		20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
		24, 24, 24, 25, 25, 25, 26, 26, 26, 27,
		28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
	};

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	level = modified_level - number_range(1, 4);
	level = UMIN(level, 50);

	dam = number_range(dam_each[level] / 2, dam_each[level]);

	if (race_lookup("dwarf") == victim->race)
		dam /= 2;

	saved = saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_CON, DAM_COLD);

	if(!saved) {
		act("$n turns blue and shivers.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = modified_level;
		af.duration = number_range(3, 8);
		if (IS_NPC(victim))
	  		af.duration *= 2;
		af.location = APPLY_STR;
		af.modifier = -1;
		af.bitvector = 0;
		affect_join(victim, &af);
	}

	damage(ch, victim, dam, sn, DAM_COLD, TRUE, TYPE_MAGIC);
	return;
}

void
spell_cloak_of_mind(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your cloaking.\n\r");
		else
		{
			act("$N refreshed your cloaking.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the cloaking spell.\n\r");
		}
		affect_refresh(victim, sn, modified_level / 3);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 3;
	af.modifier = 0;
	af.bitvector = 0;

	af.location = APPLY_NONE;
	affect_to_char(victim, &af);

	act("$n cloaks $mself from mobiles.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(victim, "You cloak yourself from the wrath of mobiles.\n\r");
	return;

}

void
spell_cloud_of_poison(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		Cprintf(ch, "You refresh your cloud of poison.\n\r");
		affect_refresh(victim, sn, modified_level / 3);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 3;
	af.modifier = 0;
	af.bitvector = 0;

	af.location = APPLY_NONE;
	affect_to_char(victim, &af);

	act("$n conjures a cloud of poison.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(victim, "You conjure a cloud of poison.\n\r");
	return;
}

void
spell_colour_spray(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam, saved;
    int caster_level, modified_level;

    caster_level = get_caster_level(level);
    modified_level = get_modified_level(level);

    if (modified_level <= 25) {
        dam = 5 + spell_damage(ch, victim, modified_level, SPELL_DAMAGE_CHART_GOOD, TRUE);
    } else {
        dam = 30 + spell_damage(ch, victim, modified_level, SPELL_DAMAGE_CHART_POOR, TRUE);
    }

    saved = saving_throw(ch, victim, sn, modified_level, SAVE_HARD, STAT_INT, DAM_LIGHT);

    if (!saved) {
        spell_blindness(gsn_blindness, level, ch, (void *) victim, TARGET_CHAR);
    }

    damage(ch, victim, dam, sn, DAM_LIGHT, TRUE, TYPE_MAGIC);

    return;
}

void
spell_cone_of_fear(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *vch, *vch_next;
        AFFECT_DATA af;
        int caster_level, modified_level;
	int dam = 0;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
                vch_next = vch->next_in_room;

                // Mobs only hit those they are fighting
                if (IS_NPC(ch)
                && vch->fighting != ch)
                        continue;

                if (IS_NPC(vch)
                && ch == vch->master)
                        continue;

                if (IS_NPC(ch)
                && vch == ch->master)
                        continue;

                if (vch == ch)
                        continue;

                if (is_safe(ch, vch))
                        continue;

		dam = spell_damage(ch, vch, modified_level, SPELL_DAMAGE_HIGH, TRUE);
                damage(ch, vch, dam, sn, DAM_CHARM, TRUE, TYPE_MAGIC);

		if(!saving_throw(ch, vch, sn, modified_level, SAVE_NORMAL, STAT_WIS, DAM_MENTAL)) {
			Cprintf(vch, "You are filled with fear so terrible you back away and cower!\n\r");
			Cprintf(ch, "%s backs away from you and cowers in fear!\n\r", PERS(vch, ch));
			act("$N is filled with terror and backs away from $n!!", ch, NULL, vch, TO_NOTVICT, POS_RESTING);
        		af.where = TO_AFFECTS;
        		af.type = sn;
        		af.level = modified_level;
        		af.location = APPLY_STR;
        		af.modifier = -2;
		        af.duration = 3;
		        af.bitvector = 0;
		        affect_join(vch, &af);

		        do_flee(vch, "");
		        do_flee(vch, "");
		        do_flee(vch, "");
		        do_flee(vch, "");

			WAIT_STATE(vch, number_range(20, 30));
		}
		else {
			act("$n grits $e teeth and resists the fear.", vch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
	}

	return;
}

void
spell_confusion(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, gsn_confusion))
	{
		if (victim == ch)
			Cprintf(ch, "You can't be more confused than you presently are.\n\r");
		else
			act("$N can't be any more confused.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (saving_throw(ch, victim, sn, modified_level, SAVE_NORMAL, STAT_INT, DAM_NEGATIVE))
	{
		Cprintf(ch, "They don't seem to be affected.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 10 + 1;
	if (IS_NPC(victim))
		af.duration *= 2;
	af.bitvector = 0;
	af.modifier = -4;
	af.location = APPLY_HITROLL;
	affect_to_char(victim, &af);

	Cprintf(victim, "You feel so confused, the walls seem to spin.\n\r");
	if (ch != victim)
		act("$N suddenly looks pretty confused.", ch, NULL, victim, TO_CHAR, POS_RESTING);

	return;
}

void
spell_continual_light(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *light;
	AFFECT_DATA *paf;
	int chance;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (target_name[0] != '\0')	/* do a glow on some object */
	{
		light = get_obj_carry(ch, target_name, ch);

		if (light == NULL)
		{
			Cprintf(ch, "You don't see that here.\n\r");
			return;
		}

		if (IS_OBJ_STAT(light, ITEM_GLOW))
		{
			act("$p is already glowing.", ch, light, NULL, TO_CHAR, POS_RESTING);
			return;
		}

		SET_BIT(light->extra_flags, ITEM_GLOW);
		act("$p glows with a white light.", ch, light, NULL, TO_ALL, POS_RESTING);
		return;
	}

	/* try and break the darkness spell on the room! */
	if (room_is_affected(ch->in_room, gsn_darkness))
	{
		paf = affect_find(ch->in_room->affected, gsn_darkness);
		chance = 50 + (modified_level * 3) - (paf->level * 3);
		if (number_percent() > chance)
		{
			act("$n twiddles $s thumbs but $s light is snuffed out by the darkness.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			act("You twiddle your thumbs but the ball of light is snuffed out by the darkness.", ch, NULL, NULL, TO_CHAR, POS_RESTING);
			return;
		}
		else
		{
			act("$n twiddles $s thumbs and the darkness melts away!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			act("You twiddle your thumbs and your light overpowers the darkness!", ch, NULL, NULL, TO_CHAR, POS_RESTING);
			affect_remove_room(ch->in_room, paf);
			return;
		}
	}

	light = create_object(get_obj_index(OBJ_VNUM_LIGHT_BALL), 0);
	if(ch->carry_number < can_carry_n(ch))
	{
		obj_to_char(light, ch);
		act("$n twiddles $s thumbs and $p appears.", ch, light, NULL, TO_ROOM, POS_RESTING);
		act("You twiddle your thumbs and $p appears.", ch, light, NULL, TO_CHAR, POS_RESTING);
	}
	else
	{
		obj_to_room(light, ch->in_room);
		act("$n twiddles $s thumbs and $p drops to the ground.", ch, light, NULL, TO_ROOM, POS_RESTING);
		act("You twiddle your thumbs and drop $p on the ground.", ch, light, NULL, TO_CHAR, POS_RESTING);
	}
	return;
}

void
spell_control_weather(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	char buf[MAX_STRING_LENGTH];
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	buf[0] = '\0';

	if (!str_cmp(target_name, "better"))
	{
		Cprintf(ch, "You attempt to make the weather {Wbetter{x!\n\r");
		weather_info.change += modified_level * 2;
		check_weather(buf, 2);
	}
	else if (!str_cmp(target_name, "worse"))
	{
		Cprintf(ch, "You attempt to make the weather {rworse{x!\n\r");
		weather_info.change -= modified_level * 2;
		check_weather(buf, -2);
	}
	else
		Cprintf(ch, "Do you want it to get better or worse?\n\r");

	return;
}

void
spell_create_food(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *mushroom;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	mushroom = create_object(get_obj_index(OBJ_VNUM_MUSHROOM), 0);
	mushroom->value[0] = modified_level / 2;
	mushroom->value[1] = modified_level;
	mushroom->timer = number_range(1000, 1200);
	obj_to_room(mushroom, ch->in_room);
	act("$p suddenly appears.", ch, mushroom, NULL, TO_ROOM, POS_RESTING);
	act("$p suddenly appears.", ch, mushroom, NULL, TO_CHAR, POS_RESTING);
	return;
}

void
spell_create_rose(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *rose;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	rose = create_object(get_obj_index(OBJ_VNUM_ROSE), 0);
	rose->timer = number_range(1000, 1200);
	if((ch->carry_number < can_carry_n(ch)) && ((get_carry_weight(ch) + get_obj_weight(rose)) <= can_carry_w(ch)))
	{
		act("$n has created a beautiful red rose.", ch, rose, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You create a beautiful red rose.\n\r");
		obj_to_char(rose, ch);
	}
	else
	{
		act("$n creates a beautful red rose and drops it to the ground.", ch, rose, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You create a beautful red rose and drop it to the ground.\n\r");
		obj_to_room(rose, ch->in_room);
	}

	return;
}

void
spell_create_shadow(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = NULL;
	AFFECT_DATA af2;
	char buf[MAX_STRING_LENGTH];
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
        {
                Cprintf(ch, "Not in this room.\n\r");
                return;
        }

	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->master == ch && IS_NPC(victim))
		{
			Cprintf(ch, "You can't clone yourself in front of your charmies! Who would they follow?\n\r");
			return;
		}
	}

	victim = create_mobile(get_mob_index(MOB_VNUM_CLONE));
	size_mob(ch, victim, modified_level * 9 / 10);
	SET_BIT(victim->toggles, TOGGLES_NOEXP);

	Cprintf(ch, "A duplicate leaps from your shadows!\n\r");
	char_to_room(victim, ch->in_room);
	if (victim->master)
		stop_follower(victim);
	add_follower(victim, ch);
	victim->leader = ch;

	af2.where = TO_AFFECTS;
	af2.type = sn;
	af2.level = modified_level;
	af2.duration = modified_level / 2;
	af2.location = 0;
	af2.modifier = 0;
	af2.bitvector = AFF_CHARM;
	affect_to_char(victim, &af2);

	sprintf(buf, "clone %szzz", ch->name);
	/*  strcpy(victim->name, buf); */
	free_string(victim->name);
	victim->name = str_dup(buf);

	sprintf(buf, "%s%s is here.\n\r", ch->name, ch->pcdata->title);
/*   strcpy(victim->long_descr, buf); */
	free_string(victim->long_descr);
	victim->long_descr = str_dup(buf);

	victim->sex = ch->sex;

	sprintf(buf, "%s", ch->name);
/*   strcpy(victim->short_descr,buf); */
	free_string(victim->short_descr);
	victim->short_descr = str_dup(buf);

	return;
}

void
spell_create_spring(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *spring;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	spring = create_object(get_obj_index(OBJ_VNUM_SPRING), 0);
	spring->timer = modified_level;
	obj_to_room(spring, ch->in_room);
	act("$p flows from the ground.", ch, spring, NULL, TO_ROOM, POS_RESTING);
	act("$p flows from the ground.", ch, spring, NULL, TO_CHAR, POS_RESTING);
	return;
}

void
spell_create_water(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int water;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (obj->item_type != ITEM_DRINK_CON)
	{
		Cprintf(ch, "It is unable to hold water.\n\r");
		return;
	}

	if (obj->value[2] != LIQ_WATER && obj->value[1] != 0)
	{
		Cprintf(ch, "It contains some other liquid.\n\r");
		return;
	}

	water = UMIN(
					modified_level * (weather_info.sky >= SKY_RAINING ? 4 : 2),
					obj->value[0] - obj->value[1]
		);

	if (water > 0)
	{
		obj->value[2] = LIQ_WATER;
		obj->value[1] += water;
		if (!is_name("water", obj->name))
		{
			char buf[MAX_STRING_LENGTH];

			sprintf(buf, "%s water", obj->name);
			free_string(obj->name);
			obj->name = str_dup(buf);
		}
		act("$p is filled.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	}

	return;
}

void
spell_creeping_doom(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int caster_level, modified_level;
	int dam, saved;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	Cprintf(ch, "The sounds of doom echo in your mind!\n\r");
	act("$n calls forth the sounds of doom.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	for (vch = char_list; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next;

		if (vch->in_room == NULL)
			continue;

		if (IS_NPC(vch)
		&& ch == vch->master)
			continue;

		if (vch->in_room == ch->in_room)
		{
			if (vch != ch && !is_safe(ch, vch))
			{
				dam =  spell_damage(ch, vch, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
				saved = saving_throw(ch, vch, sn, caster_level, SAVE_HARD, STAT_CON, DAM_POISON);

				damage(ch, vch, dam, sn, DAM_POISON, TRUE, TYPE_MAGIC);

				if(!saved)
					poison_effect(vch, modified_level, dice(modified_level, 3), target);

				continue;
			}
		}

		if (vch->in_room->area == ch->in_room->area)
			Cprintf(vch, "The sounds of doom echo in your area.\n\r");
	}

	return;
}

void
spell_cryo(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	obj = (OBJ_DATA *) vo;
	if (obj->timer == -1)
	{
		Cprintf(ch, "It's already as cold as it can get\n\r");
		return;
	}

	if (obj->item_type != ITEM_FOOD &&
		obj->item_type != ITEM_CORPSE_PC &&
		obj->item_type != ITEM_CORPSE_NPC &&
		obj->pIndexData->vnum != OBJ_VNUM_SEVERED_HEAD)
	{
		Cprintf(ch, "You cannot freeze this object\n\r");
		return;
	}

	obj->timer = -1;
	act("$P is frozen solid.", ch, NULL, obj, TO_CHAR, POS_RESTING);
	return;

}

void
spell_cure_blindness(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (!is_affected(victim, gsn_blindness))
	{
		if (victim == ch)
			Cprintf(ch, "You aren't blind.\n\r");
		else
			act("$N doesn't appear to be blinded.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (try_cancel(victim, gsn_blindness, modified_level))
	{
		affect_strip(victim, gsn_blindness);
		Cprintf(victim, "Your vision returns!\n\r");
		act("$n is no longer blinded.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	else
		Cprintf(ch, "Spell failed.\n\r");
}



void
spell_cure_critical(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int heal;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	heal = dice(3, 8) + modified_level - 6;
	if(!is_affected(victim, gsn_dissolution))
		victim->hit = UMIN(victim->hit + heal, MAX_HP(victim));
	update_pos(victim);
	Cprintf(victim, "You feel better!\n\r");
	if (ch != victim)
		Cprintf(ch, "Ok.\n\r");
	return;
}

/* RT added to cure plague */
void
spell_cure_disease(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (!is_affected(victim, gsn_plague))
	{
		if (victim == ch)
			Cprintf(ch, "You aren't ill.\n\r");
		else
			act("$N doesn't appear to be diseased.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (try_cancel(victim, gsn_plague, modified_level))
	{
		affect_strip(victim, gsn_plague);
		Cprintf(victim, "Your sores vanish.\n\r");
		act("$n looks relieved as $s sores vanish.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	else
		Cprintf(ch, "Spell failed.\n\r");
}

void
spell_cure_light(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int heal;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	heal = dice(1, 8) + modified_level / 3;
	if(!is_affected(victim, gsn_dissolution))
		victim->hit = UMIN(victim->hit + heal, MAX_HP(victim));
	update_pos(victim);
	Cprintf(victim, "You feel better!\n\r");
	if (ch != victim)
		Cprintf(ch, "Ok.\n\r");
	return;
}

void
spell_cure_poison(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (!is_affected(victim, gsn_poison))
	{
		if (victim == ch)
			Cprintf(ch, "You aren't poisoned.\n\r");
		else
			act("$N doesn't appear to be poisoned.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (try_cancel(victim, gsn_poison, modified_level))
	{
		affect_strip(victim, gsn_poison);
		Cprintf(victim, "A warm feeling runs through your body.\n\r");
		act("$n looks much better.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	else
		Cprintf(ch, "Spell failed.\n\r");
}

void
spell_cure_serious(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int heal;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	heal = dice(2, 8) + modified_level / 2;
	if(!is_affected(victim, gsn_dissolution))
		victim->hit = UMIN(victim->hit + heal, MAX_HP(victim));
	update_pos(victim);
	Cprintf(victim, "You feel better!\n\r");
	if (ch != victim)
		Cprintf(ch, "Ok.\n\r");
	return;
}

void
spell_curse(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* deal with the object case first */
	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;
		if (IS_OBJ_STAT(obj, ITEM_EVIL))
		{
			act("$p is already filled with evil.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			return;
		}

		if (IS_OBJ_STAT(obj, ITEM_BLESS))
		{
			AFFECT_DATA *paf;

			if (number_percent() < 50 + ((modified_level - obj->level) * 5))
			{
				act("$p glows with a red aura.", ch, obj, NULL, TO_ALL, POS_RESTING);
				while((paf = affect_find(obj->affected, gsn_bless)) != NULL)
					affect_remove_obj(obj, paf);

				REMOVE_BIT(obj->extra_flags, ITEM_BLESS);
				return;
			}
			else
			{
				act("The holy aura of $p is too powerful for you to overcome.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				return;
			}
		}

		af.where = TO_OBJECT;
		af.type = sn;
		af.level = modified_level;
		af.duration = 1 + (modified_level / 6);
		af.location = APPLY_SAVES;
		af.modifier = 1;
		af.bitvector = ITEM_EVIL;
		affect_to_obj(obj, &af);

		act("$p glows with a malevolent aura.", ch, obj, NULL, TO_ALL, POS_RESTING);

		if (obj->wear_loc != WEAR_NONE)
			ch->saving_throw += 1;
		return;
	}

	/* character curses */
	victim = (CHAR_DATA *) vo;
	if (IS_AFFECTED(victim, AFF_CURSE)) {
		Cprintf(ch, "They are already cursed.\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_WIS, DAM_NEGATIVE))
	{
		Cprintf(ch, "They avoid your spell.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 1 + (modified_level / 6);
	if (IS_NPC(victim))
		af.duration *= 2;
	af.location = APPLY_HITROLL;
	af.modifier = -1 * (modified_level / 8);
	if(ch->charClass == class_lookup("cleric")) {
		af.modifier -= 4;
	}
	af.bitvector = AFF_CURSE;
	affect_to_char(victim, &af);

	af.location = APPLY_SAVING_SPELL;
	af.modifier = -af.modifier;
	affect_to_char(victim, &af);

	Cprintf(victim, "You feel unclean.\n\r");

	if (ch != victim)
		act("$N looks very uncomfortable.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_demand(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_STRING_LENGTH];
	char *arg2;
	char arg3[MAX_STRING_LENGTH];
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	arg2 = one_argument(target_name, arg);
	one_argument(arg2, arg3);

	if (arg[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Force whom to do what?\n\r");
		return;
	}

	if (!can_order(arg3))
	{
		Cprintf(ch, "That will not be done.\n\r");
		return;
	}

	sprintf(buf, "$n demands that you to '%s'.", arg2);

	if (victim == ch)
	{
		Cprintf(ch, "Aye aye, right away!\n\r");
		return;
	}

	if (IS_IMMORTAL(victim))
	{
		Cprintf(ch, "Do it yourself!\n\r");
		return;
	}

	if (saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_WIS, DAM_CHARM))
	{
		return;
	}

	act(buf, ch, NULL, victim, TO_VICT, POS_RESTING);
	interpret(victim, arg2);

	act("You put $N under your control.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_demonfire(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;
	int saved = FALSE;
	AFFECT_DATA af;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	saved = saving_throw(ch, victim, sn, modified_level, SAVE_NORMAL, STAT_WIS, DAM_NEGATIVE);

	if (!IS_NPC(ch) && !IS_EVIL(ch))
	{
		victim = ch;
		saved = FALSE;
		Cprintf(ch, "The demons turn upon you!\n\r");
	}

	ch->alignment = UMAX(-1000, ch->alignment - 50);

	if (victim != ch)
	{
		act("$n calls forth the demons of Hell upon $N!", ch, NULL, victim, TO_ROOM, POS_RESTING);
		act("$n has assailed you with the demons of Hell!", ch, NULL, victim, TO_VICT, POS_RESTING);
		Cprintf(ch, "You conjure forth the demons of hell!\n\r");
	}

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	if(saved)
		dam = dam / 2;

	damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE, TYPE_MAGIC);

	if(!IS_AFFECTED(victim, AFF_CURSE) && !saved) {
        	af.where = TO_AFFECTS;
        	af.type = gsn_curse;
        	af.level = modified_level;
        	af.duration = 1 + (modified_level / 6);
		if (IS_NPC(victim))
			af.duration *= 2;
        	af.location = APPLY_HITROLL;
        	af.modifier = -1 * (modified_level / 8);
        	af.bitvector = AFF_CURSE;
        	affect_to_char(victim, &af);

        	af.location = APPLY_SAVING_SPELL;
        	af.modifier = modified_level / 8;
        	affect_to_char(victim, &af);

        	Cprintf(victim, "You feel unclean.\n\r");

        	if (ch != victim)
                	act("$N looks very uncomfortable.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	}

	return;
}

void
spell_denounciation(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int caster_level, modified_level;

	victim = (CHAR_DATA *) vo;
	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, gsn_denounciation)) {
		Cprintf(ch, "They are already denounced.\n\r");
		return;
	}

	if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_INT, DAM_NEGATIVE))
	{
		Cprintf(ch, "You fail to denounce that person.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.bitvector = 0;
	af.duration = 1 + (modified_level / 6);
	if (IS_NPC(victim))
		af.duration *= 2;
	af.location = APPLY_DAMROLL;
	af.modifier = -1 * (modified_level / 10);
	affect_to_char(victim, &af);

	af.location = APPLY_CON;
	affect_to_char(victim, &af);

	Cprintf(victim, "You have been denounced.\n\r");
	Cprintf(ch, "You cast down %s!\n\r", IS_NPC(victim) ? victim->short_descr : victim->name);
	return;
}

void
spell_detect_hidden(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_AFFECTED(victim, AFF_DETECT_HIDDEN))
	{
		Cprintf(ch, "You refresh the spell.\n\r");
		affect_refresh(victim, sn, modified_level);
		return;
	}
	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_DETECT_HIDDEN;
	affect_to_char(victim, &af);
	Cprintf(victim, "Your awareness improves.\n\r");
	if (ch != victim)
		Cprintf(ch, "Ok.\n\r");
	return;
}

void
spell_detect_invis(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_AFFECTED(victim, AFF_DETECT_INVIS))
	{
		Cprintf(ch, "You refresh your detect invisibility spell.\n\r");
		affect_refresh(victim, sn, modified_level);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = AFF_DETECT_INVIS;
	affect_to_char(victim, &af);
	Cprintf(victim, "Your eyes tingle.\n\r");
	if (ch != victim)
		Cprintf(ch, "Ok.\n\r");
	return;
}

void
spell_detect_magic(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;
	AFFECT_DATA *paf, *paf_last = NULL;
        char buf[MAX_STRING_LENGTH];

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	act("Your eyes begin to glow as you study $N's magical aura.", ch, NULL, victim, TO_CHAR, POS_RESTING);

	if (victim->affected != NULL || IS_AFFECTED(victim, AFF_HIDE) || victim->affected != NULL)
        {
		act("$E is currently affected by:", ch, NULL, victim, TO_CHAR, POS_RESTING);
                for (paf = victim->affected; paf != NULL; paf = paf->next)
                {
                        if (paf->where == TO_VULN || paf->where == TO_RESIST)
                                continue;
                        if (paf_last != NULL && paf->type == paf_last->type)
                                if (ch->level >= 20)
                                        sprintf(buf, "                         ");
                                else
                                        continue;
                        else
                                sprintf(buf, "Spell: %-18s", skill_table[paf->type].name);
                        Cprintf(ch, "%s", buf);

                        Cprintf(ch, ": modifies %s by %d ", affect_loc_name(paf->location), paf->modifier);
                        if (paf->duration == -1)
                                sprintf(buf, "permanently");
                        else
                                sprintf(buf, "for %d hours", paf->duration);
                        Cprintf(ch, "%s\n\r", buf);
                        paf_last = paf;
                }
		Cprintf(ch, "\n\r");

                if (IS_AFFECTED(victim, AFF_HIDE))
                        Cprintf(ch, "Spell: hide\n\r");
        }
        else
		act("$E is not affected by any spells.", ch, NULL, victim, TO_CHAR, POS_RESTING);

        return;
}

void
spell_detect_poison(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;

	if (obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD)
	{
		if (obj->value[3] != 0)
			Cprintf(ch, "You smell poisonous fumes.\n\r");
		else
			Cprintf(ch, "It looks delicious.\n\r");
	}
	else
	{
		Cprintf(ch, "It doesn't look poisoned.\n\r");
	}

	return;
}

void
spell_detonation(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj;
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;
	int dam;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, gsn_detonation))
	{
		Cprintf(ch, "Your opponent's hands are already burning, but you try again anyways!\n\r");
	}


	/* if wielding two weapons, lose one */
	if (get_eq_char(victim, WEAR_WIELD) != NULL
		&& get_eq_char(victim, WEAR_DUAL) != NULL)
	{
		if (number_percent() < 50)
			obj = get_eq_char(victim, WEAR_WIELD);
		else
			obj = get_eq_char(victim, WEAR_DUAL);
	}
	else if (get_eq_char(victim, WEAR_WIELD) == NULL
			 && get_eq_char(victim, WEAR_DUAL) != NULL)
		obj = get_eq_char(victim, WEAR_DUAL);
	else
		obj = get_eq_char(victim, WEAR_WIELD);

	if (obj == NULL)
	{
		Cprintf(ch, "Your opponent is not wielding a weapon!\n\r");
		return;
	}

	if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_STR, DAM_FIRE))
	{
		act("$N's weapon vibrates for a moment.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		act("$n tries to detonate your weapon but it holds on!", ch, NULL, victim, TO_VICT, POS_RESTING);
		return;
	}
	else
	{
		act("$N releases $S weapon as it bursts into flame!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		act("$n detonation causes you to release your weapon!", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$n causes $N's weapon to burst into flames!", ch, NULL, victim, TO_ROOM, POS_RESTING);

		obj_from_char(obj);
		obj_to_char(obj, victim);

		dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_HIGH, TRUE);

        	damage(ch, victim, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);

		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = modified_level;
		af.duration = 0;
		af.modifier = 0;
		af.bitvector = 0;
		af.location = APPLY_NONE;
		affect_to_char(victim, &af);

		return;
	}
	return;
}

void
spell_dispel_evil(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam, saved;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (!IS_NPC(ch) && IS_EVIL(ch)) {
		Cprintf(ch, "The magic turns on you!\n\r");
		victim = ch;
	}

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_LOW, TRUE);
	dam = dam * 11 / 10;
	saved = saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_CON, DAM_HOLY);

	damage(ch, victim, dam, sn, DAM_HOLY, TRUE, TYPE_MAGIC);

	// Dispel their protection sometimes.
	if (!saved
	&& (is_affected(victim, gsn_protection_good)
	|| is_affected(victim, gsn_protection_evil)
	|| is_affected(victim, gsn_protection_neutral))) {
		Cprintf(ch, "Your victim's protection flickers and fades.\n\r");
		Cprintf(victim, "You feel your protection weaken and fade.\n\r");
		affect_strip(victim, gsn_protection_good);
		affect_strip(victim, gsn_protection_evil);
		affect_strip(victim, gsn_protection_neutral);
	}

	return;
}

void
spell_dispel_good(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam, saved;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (!IS_NPC(ch) && IS_GOOD(ch)) {
		Cprintf(ch, "The magic turns on you!\n\r");
		victim = ch;
	}

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_LOW, TRUE);
	dam = dam * 11 / 10;
	saved = saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_CON, DAM_NEGATIVE);

	damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE, TYPE_MAGIC);

	// Dispel their protection sometimes.
	if (!saved
	&& (is_affected(victim, gsn_protection_good)
        || is_affected(victim, gsn_protection_evil)
        || is_affected(victim, gsn_protection_neutral))) {
		Cprintf(ch, "Your victim's protection flickers and fades.\n\r");
		Cprintf(victim, "You feel your protection weaken and fade.\n\r");
		affect_strip(victim, gsn_protection_evil);
		affect_strip(victim, gsn_protection_good);
		affect_strip(victim, gsn_protection_neutral);
	}

	return;
}

void
spell_dispel_magic(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	bool found = FALSE;
	int spells_removed=0, affect_count=0;
	int caster_level, modified_level;
	int target_affects[100], selected = 0;
	int i, sp, forbidden = FALSE;
	AFFECT_DATA *paf;
	char buf_vict[255];

	// Stuff that can't be dispelled.
	struct no_dispel_tag {
        	int sn;
        	int no_dispel;
	};
	struct no_dispel_tag no_dispel_table[] =
	{
		{ gsn_aura,			TRUE  },
		{ gsn_jail,			TRUE  },
		{ gsn_floating_disc,		TRUE  },
       		{ gsn_plague,                   TRUE  },
        	{ gsn_poison,                   TRUE  },
        	{ gsn_nature_protection,        TRUE  },
        	{ gsn_nightmares,               TRUE  },
        	{ gsn_delayed_blast_fireball,   TRUE  },
		{ gsn_karma,			TRUE  },
		{ gsn_fire_breath,              TRUE  },
		{ gsn_sunray, 			TRUE  },
		{ gsn_wrath,			TRUE  },
		{ gsn_telepathy,		TRUE  },
		{ gsn_spell_stealing,           TRUE  },
		{ -1,				FALSE },
	};

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);
	spells_removed = number_range(1, 6);

	if(saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_WIS, DAM_NONE)) {
		Cprintf(victim, "You feel a brief tingling sensation.\n\r");
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	for(sp = 0; sp < spells_removed; sp++) {
		// Count up our list of valid affects
		affect_count = 0;
		for(paf = victim->affected; paf != NULL; paf = paf->next) {
			i = 0;
			forbidden = FALSE;
			while(TRUE) {
				if(no_dispel_table[i].sn == -1)
					break;
				if(no_dispel_table[i].sn == paf->type
				&& no_dispel_table[i].no_dispel) {
					forbidden = TRUE;
					break;
				}
				i++;
			}
			if(!forbidden) {
				target_affects[affect_count] = paf->type;
				affect_count++;
			}
		}
		// Any more affects?
		if(affect_count == 0)
			break;

		// Pick one affect
		selected = number_range(0, affect_count - 1);

		// Is it a valid affect?
		if(skill_table[target_affects[selected]].spell_fun != spell_null) {
			// Ok, remove it now.
			if(skill_table[target_affects[selected]].msg_off)
				Cprintf(victim, "%s\n\r", skill_table[target_affects[selected]].msg_off);

			sprintf(buf_vict, "%s is no longer affected by %s!",
				IS_NPC(victim) ? victim->short_descr : victim->name,
				skill_table[target_affects[selected]].name);

			if(victim != ch)
				Cprintf(ch, "%s\n\r", buf_vict);
			act(buf_vict, ch, NULL, victim, TO_NOTVICT, POS_RESTING);

			if(skill_table[target_affects[selected]].end_fun != end_null)
				skill_table[target_affects[selected]].end_fun((void*)victim, TARGET_CHAR);

			affect_strip(victim, target_affects[selected]);
			spells_removed--;
			found = TRUE;
		}
	}

	if (found)
		Cprintf(ch, "Ok.\n\r");
	else
	{
		Cprintf(victim, "You feel a brief tingling sensation.\n\r");
		Cprintf(ch, "You failed.\n\r");
	}

	return;
}

/*
 * displace is shamelessly ripped off of gate, so if you make ANY
 * modifications to gate, you gotta make them to displace too!
 */
void
spell_displace(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	bool gate_pet;
	ROOM_INDEX_DATA *room;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if ((victim = get_char_world(ch, target_name, FALSE)) == NULL
		|| victim == ch
		|| victim->in_room == NULL
//		|| (victim->in_room->area != ch->in_room->area)
		|| (!IS_NPC(victim) && IS_SET(ch->act, PLR_NOSUMMON))
		|| ch->in_room->clan
		|| victim->in_room->clan
		|| !can_see_room(ch, victim->in_room)
		|| IS_SET(victim->in_room->room_flags, ROOM_SAFE)
		|| IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
		|| IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
		|| IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
		|| IS_SET(victim->in_room->room_flags, ROOM_LAW)
		|| IS_SET(victim->in_room->room_flags, ROOM_NO_GATE)
		|| wearing_nogate_item(ch)
		|| IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
/* so that you can't displace in clan halls =P */
		|| (ch->in_room->clan)
		|| victim->level >= modified_level + 6
		|| (is_clan(victim) && !is_same_clan(ch, victim))
		|| (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
		|| (IS_NPC(victim) && IS_AFFECTED(victim, AFF_CHARM) && victim->master != ch)
		|| (IS_NPC(victim)
		&& number_percent() < 20))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (victim->in_room->area->continent != ch->in_room->area->continent)
        {
                Cprintf(ch, "Inter-continental magic is forbidden.  You failed.\n\r");
                return;
        }

        if (victim->in_room->area->security < 9)
        {
                Cprintf(ch, "Not in unfinished areas.  Sorry.\n\r");
                return;
        }

	if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
		gate_pet = TRUE;
	else
		gate_pet = FALSE;

	if (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
                REMOVE_BIT(victim->act, ACT_AGGRESSIVE);

	act("$n spirals and twists, and untwists into $N!", ch, victim, victim, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You warp yourself into another part of the world!\n\r");
	room = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, victim->in_room);
	char_from_room(victim);
	char_to_room(victim, room);

	act("$N spirals and twists, and untwists into $n!", ch, victim, NULL, TO_ROOM, POS_RESTING);
	do_look(ch, "auto");

	if (gate_pet)
	{
		act("$n warps and twists into nothingness!", ch->pet, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch->pet, "You warp through a dimensional hole!\n\r");
		char_from_room(ch->pet);
		char_to_room(ch->pet, ch->in_room);
		act("$n warps and untwists from nothingness!", ch->pet, NULL, NULL, TO_ROOM, POS_RESTING);
		do_look(ch->pet, "auto");
	}

}

void
spell_drakor(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (saving_throw(ch, victim, sn, caster_level + 2, SAVE_HARD, STAT_INT, DAM_NEGATIVE))
	{
		Cprintf(ch, "You failed to tip the scales of evil.\n\r");
		return;
	}

	if (victim->alignment == -1000)
	{
		Cprintf(ch, "They are already as evil as they can get.\n\r");
		return;
	}

	victim->alignment = victim->alignment - 250;

	if (victim->alignment < -1000)
		victim->alignment = -1000;


	Cprintf(victim, "You feel yourself becoming an evil person.\n\r");
	if (ch != victim)
		act("$N is becoming an evil person.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_earthquake(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int caster_level, modified_level;
	int dam = 0;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (ch->in_room->sector_type == SECT_AIR)
	{
		Cprintf(ch, "Riiiight. Where's the ground, buddy?\n\r");
		return;
	}

	dam = dice(modified_level / 6, 6) + modified_level * 3 / 4;

	Cprintf(ch, "The earth trembles beneath your feet!\n\r");
	act("$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	for (vch = char_list; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next;

		if (vch->in_room == NULL)
			continue;

		if (vch->in_room == ch->in_room)
		{

			if (ch == vch->master
			&& IS_NPC(vch))
				continue;

			if (IS_NPC(ch)
			&& vch->fighting != ch)
				continue;

			if (vch != ch && !is_safe(ch, vch))
			{
				if (IS_AFFECTED(vch, AFF_FLYING))
					damage(ch, vch, 0, sn, DAM_BASH, TRUE, TYPE_MAGIC);
				else
					damage(ch, vch, modified_level + dice(2, 8), sn, DAM_BASH, TRUE, TYPE_MAGIC);
				continue;
			}
		}

		if (vch->in_room->area == ch->in_room->area)
			Cprintf(vch, "The earth trembles and shivers.\n\r");
	}

	return;
}

void
spell_enchant_armor(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA *paf;
	int result, fail;
	int ac_bonus, added;
	bool ac_found = FALSE;
	int dam = 0;
	int caster_level, modified_level;
	int protected = FALSE;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (obj->item_type != ITEM_ARMOR)
	{
		Cprintf(ch, "That isn't armor!\n\r");
		return;
	}

	if (obj->wear_loc != -1)
	{
		Cprintf(ch, "The item must be carried to be enchanted.\n\r");
		return;
	}

	/* this means they have no bonus */
	ac_bonus = 0;
	fail = 25;					/* base 25% chance of failure */

	/* find the bonuses */

	if (!obj->enchanted)
		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
		{
			if (paf->location == APPLY_AC)
			{
				ac_bonus = paf->modifier;
				ac_found = TRUE;
				fail += 5 * (ac_bonus * ac_bonus);
			}

			else				/* things get a little harder */
				fail += 20;
		}

	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
		if (paf->location == APPLY_AC)
		{
			ac_bonus = paf->modifier;
			ac_found = TRUE;
			fail += 5 * (ac_bonus * ac_bonus);
		}

		else					/* things get a little harder */
			fail += 20;
	}

	/* apply other modifiers */
	fail -= modified_level;

	if (IS_OBJ_STAT(obj, ITEM_BLESS))
		fail -= 15;
	if (IS_OBJ_STAT(obj, ITEM_GLOW))
		fail -= 5;
	if(obj_is_affected(obj, gsn_balance_rune)) {
                protected = TRUE;
		fail /= 2;
	}
	fail = URANGE(5, fail, 85);

	result = number_percent();

	/* the moment of truth */
	if (result < (fail / 5) && !IS_IMMORTAL(ch))	/* item destroyed */
	{
		if(protected) {
        		act("$p begins to glow violently but the explosion is stabilized.",
               			ch, obj, NULL, TO_ALL, POS_RESTING);
        		paf = affect_find(obj->affected, gsn_balance_rune);
        		if(paf != NULL)
                	affect_remove_obj(obj, paf);
        		return;
		}
		else {
			act("$p flares blindingly... and explodes!", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$p flares blindingly... and explodes!", ch, obj, NULL, TO_ROOM, POS_RESTING);
			extract_obj(obj);
			dam = dice(ch->level, 6);
			damage(ch, ch, dam, sn, DAM_SLASH, TRUE, TYPE_MAGIC);
			return;
		}
	}

	if (result < (fail / 3) && !IS_IMMORTAL(ch))	/* item disenchanted */
	{
		if(protected) {
		        act("$p begins to glow violently but the fade is stabilized.",
                	ch, obj, NULL, TO_ALL, POS_RESTING);
        		paf = affect_find(obj->affected, gsn_balance_rune);
        		if(paf != NULL)
                		affect_remove_obj(obj, paf);
        		return;
		}
		else {
			AFFECT_DATA *paf_next;

			act("$p glows brightly, then fades...oops.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$p glows brightly, then fades.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			obj->enchanted = TRUE;

			/* remove all affects */
			for (paf = obj->affected; paf != NULL; paf = paf_next)
			{
				paf_next = paf->next;
				affect_remove_obj(obj, paf);
			}
			obj->affected = NULL;
			/* clear all flags */
			obj->extra_flags = 0;
			return;
		}
	}

	if (result <= fail && !IS_IMMORTAL(ch))		/* failed, no bad result */
	{
		Cprintf(ch, "Nothing seemed to happen.\n\r");
		return;
	}

	/* okay, move all the old flags into new vectors if we have to */
	if (!obj->enchanted)
	{
		AFFECT_DATA *af_new;

		obj->enchanted = TRUE;

		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
		{
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

	if (result <= (90 - modified_level / 5) && !IS_IMMORTAL(ch))		/* success! */
	{
		act("$p shimmers with a gold aura.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$p shimmers with a gold aura.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		SET_BIT(obj->extra_flags, ITEM_MAGIC);
		added = -1;
	}

	else
		/* exceptional enchant */
	{
		act("$p glows a brilliant gold!", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$p glows a brilliant gold!", ch, obj, NULL, TO_ROOM, POS_RESTING);
		SET_BIT(obj->extra_flags, ITEM_MAGIC);
		SET_BIT(obj->extra_flags, ITEM_GLOW);
		added = -2;
	}

	/* now add the enchantments */

	if (obj->level < 54)
		obj->level = obj->level + 1;

	if (ac_found)
	{
		for (paf = obj->affected; paf != NULL; paf = paf->next)
		{
			if (paf->location == APPLY_AC)
			{
				paf->type = sn;
				paf->modifier += added;
				paf->level = UMAX(paf->level, modified_level);
			}
		}
	}
	else
		/* add a new affect */
	{
		paf = new_affect();

		paf->where = TO_OBJECT;
		paf->type = sn;
		paf->level = modified_level;
		paf->duration = -1;
		paf->location = APPLY_AC;
		paf->modifier = added;
		paf->bitvector = 0;
		paf->next = obj->affected;
		obj->affected = paf;
	}

}

void
spell_enchant_weapon(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA *paf;
	int result, fail;
	int hit_bonus, dam_bonus, added;
	bool hit_found = FALSE, dam_found = FALSE;
	int dam = 0;
	int caster_level, modified_level;
	int protected = FALSE;


	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (obj->item_type != ITEM_WEAPON)
	{
		Cprintf(ch, "That isn't a weapon.\n\r");
		return;
	}

	if (obj->wear_loc != -1)
	{
		Cprintf(ch, "The item must be carried to be enchanted.\n\r");
		return;
	}

	/* this means they have no bonus */
	hit_bonus = 0;
	dam_bonus = 0;
	fail = 25;		/* base 25% chance of failure */

	/* find the bonuses */
	if (!obj->enchanted) {
		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
		{
			if (paf->type != gsn_sharpen
			&& paf->type != gsn_blade_rune
			&& paf->type != gsn_magic_sheath
			&& paf->type != gsn_bless
			&& paf->location == APPLY_HITROLL)
			{
				hit_bonus = paf->modifier;
				hit_found = TRUE;
				fail += 2 * (hit_bonus * hit_bonus);
			}

			else if (paf->type != gsn_sharpen
			&& paf->type != gsn_blade_rune
			&& paf->type != gsn_magic_sheath
			&& paf->type != gsn_curse
			&& paf->location == APPLY_DAMROLL)
			{
				dam_bonus = paf->modifier;
				dam_found = TRUE;
				fail += 2 * (dam_bonus * dam_bonus);
			}

			else
				fail += 25;
		}
	}

	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
		if (paf->type != gsn_sharpen
		&& paf->type != gsn_blade_rune
		&& paf->type != gsn_magic_sheath
		&& paf->type != gsn_bless
		&& paf->location == APPLY_HITROLL)
		{
			hit_bonus = paf->modifier;
			hit_found = TRUE;
			fail += 2 * (hit_bonus * hit_bonus);
		}
		else if (paf->type != gsn_sharpen
		&& paf->type != gsn_blade_rune
		&& paf->type != gsn_magic_sheath
		&& paf->type != gsn_curse
		&& paf->location == APPLY_DAMROLL)
		{
			dam_bonus = paf->modifier;
			dam_found = TRUE;
			fail += 2 * (dam_bonus * dam_bonus);
		}
		else	/* things get a little harder */
			fail += 25;
	}

	/* apply other modifiers */
	fail -= 3 * modified_level / 2;

	if (IS_OBJ_STAT(obj, ITEM_BLESS))
		fail -= 15;
	if (IS_OBJ_STAT(obj, ITEM_GLOW))
		fail -= 5;
	if(obj_is_affected(obj, gsn_balance_rune)) {
                protected = TRUE;
		fail /= 2;
	}

	fail = URANGE(5, fail, 95);

	result = number_percent();

	/* the moment of truth */
	if (result < (fail / 5) && !IS_IMMORTAL(ch))	/* item destroyed */
	{
		if(protected) {
			act("$p begins to glow violently but the explosion is stabilized.",
				ch, obj, NULL, TO_ALL, POS_RESTING);
			paf = affect_find(obj->affected, gsn_balance_rune);
			if(paf != NULL)
				affect_remove_obj(obj, paf);
			return;
		}
		else {
			act("$p shivers violently and explodes!", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$p shivers violently and explodes!", ch, obj, NULL, TO_ROOM, POS_RESTING);
			if (IS_SET(ch->toggles, TOGGLES_SOUND))
				Cprintf(ch, "!!SOUND(sounds/wav/enchblow.wav V=80 P=20 T=admin)");
			dam = dice(ch->level, 6);
			damage(ch, ch, dam, sn, DAM_SLASH, TRUE, TYPE_MAGIC);

			extract_obj(obj);
			return;
		}
	}

	if (result < (fail / 2) && !IS_IMMORTAL(ch))	/* item disenchanted */
	{
		AFFECT_DATA *paf_next;

		if(protected) {
       			act("$p begins to glow violently but the fade is stabilized.",
	                	ch, obj, NULL, TO_ALL, POS_RESTING);
        		paf = affect_find(obj->affected, gsn_balance_rune);
        		if(paf != NULL)
                		affect_remove_obj(obj, paf);
			return;
		}
		else {
			act("$p glows brightly, then fades...oops.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$p glows brightly, then fades.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			obj->enchanted = TRUE;

			/* remove all affects */
			for (paf = obj->affected; paf != NULL; paf = paf_next)
			{
				paf_next = paf->next;
				affect_remove_obj(obj, paf);
			}

			/* clear all flags */
			obj->extra_flags = 0;
			return;
		}
	}
	if (result <= fail && !IS_IMMORTAL(ch))		/* failed, no bad result */
	{
		Cprintf(ch, "Nothing seemed to happen.\n\r");
		return;
	}

	/* okay, move all the old flags into new vectors if we have to */
	if (!obj->enchanted)
	{
		AFFECT_DATA *af_new;

		obj->enchanted = TRUE;

		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
		{
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

	if (result <= (100 - modified_level / 5) && !IS_IMMORTAL(ch))	/* success! */
	{
		act("$p glows blue.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$p glows blue.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		SET_BIT(obj->extra_flags, ITEM_MAGIC);
		added = 1;
	}

	else
		/* exceptional enchant */
	{
		act("$p glows a brilliant blue!", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$p glows a brilliant blue!", ch, obj, NULL, TO_ROOM, POS_RESTING);
		SET_BIT(obj->extra_flags, ITEM_MAGIC);
		SET_BIT(obj->extra_flags, ITEM_GLOW);
		added = 2;
	}

	/* now add the enchantments */

	if (obj->level < 54)
		obj->level = obj->level + 1;

	if (dam_found)
	{
		for (paf = obj->affected; paf != NULL; paf = paf->next)
		{
			if (paf->type != gsn_sharpen
			&& paf->type != gsn_magic_sheath
			&& paf->type != gsn_blade_rune
			&& paf->type != gsn_curse
			&& paf->location == APPLY_DAMROLL)
			{
				paf->type = sn;
				paf->modifier += added;
				paf->level = UMAX(paf->level, modified_level);
				if (paf->modifier > 4)
					SET_BIT(obj->extra_flags, ITEM_HUM);
			}
		}
	}
	else
		/* add a new affect */
	{
		paf = new_affect();

		paf->where = TO_OBJECT;
		paf->type = sn;
		paf->level = modified_level;
		paf->duration = -1;
		paf->location = APPLY_DAMROLL;
		paf->modifier = added;
		paf->bitvector = 0;
		paf->next = obj->affected;
		obj->affected = paf;
	}

	if (hit_found)
	{
		for (paf = obj->affected; paf != NULL; paf = paf->next)
		{
			if (paf->type != gsn_sharpen
			&& paf->type != gsn_blade_rune
			&& paf->type != gsn_magic_sheath
			&& paf->type != gsn_bless
			&& paf->location == APPLY_HITROLL)
			{
				paf->type = sn;
				paf->modifier += added;
				paf->level = UMAX(paf->level, modified_level);
				if (paf->modifier > 4)
					SET_BIT(obj->extra_flags, ITEM_HUM);
			}
		}
	}
	else
		/* add a new affect */
	{
		paf = new_affect();

		paf->type = sn;
		paf->level = modified_level;
		paf->duration = -1;
		paf->location = APPLY_HITROLL;
		paf->modifier = added;
		paf->bitvector = 0;
		paf->next = obj->affected;
		obj->affected = paf;
	}

}

/*
 * Drain MV, HP.
 * Caster gains MANA.
 */
void
spell_energy_drain(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (victim != ch)
		ch->alignment = UMAX(-1000, ch->alignment - 50);

	if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_CON, DAM_NEGATIVE))
	{
		Cprintf(victim, "You feel a momentary chill.\n\r");
		return;
	}

	if (victim->level <= 2)
	{
		dam = ch->hit + 1;
	}
	else
	{
		dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
		gain_exp(victim, 0 - dam);

		if(victim->move > 0) {
			victim->move -= (dam * 2 / 3);
			if (victim->move < 0)
				victim->move = 0;
		}
	}

	Cprintf(victim, "You feel your energy slipping away!\n\r");
	Cprintf(ch, "Wow....what a rush!\n\r");
	damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE, TYPE_MAGIC);

	return;
}

/*
 * Drain MANA, HP.
 * Caster gains MANA.
 */
void
spell_mana_drain(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (victim != ch)
		ch->alignment = UMAX(-1000, ch->alignment - 50);

	if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_CON, DAM_NEGATIVE))
	{
		Cprintf(victim, "You feel a momentary chill.\n\r");
		return;
	}

	if (victim->level <= 2)
	{
		dam = ch->hit + 1;
	}
	else
	{
		dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
		gain_exp(victim, 0 - dam);

		if(victim->mana > 0) {
			victim->mana -= (dam * 2 / 3);
			ch->mana += (dam / 2);
			if (victim->mana < 0)
				victim->mana = 0;
		}
	}

	Cprintf(victim, "You feel your mana slipping away!\n\r");
	Cprintf(ch, "Wow....what a rush!\n\r");
	damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE, TYPE_MAGIC);

	return;
}

void
spell_enlargement(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		Cprintf(ch, "You refresh your enlargement spell.\n\r");
		affect_refresh(victim, sn, modified_level / 5);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 5;
	af.modifier = (modified_level * 4);
	af.location = APPLY_HIT;
	af.bitvector = 0;

	affect_to_char(victim, &af);

	af.location = APPLY_SAVES;
	af.modifier = -(modified_level / 10);
	affect_to_char(victim, &af);

	af.location = APPLY_DEX;
	af.modifier = -(modified_level / 12);
	affect_to_char(victim, &af);

	if(!is_affected(victim, gsn_dissolution))
		victim->hit = victim->hit + (modified_level * 4);

	victim->size++;

	Cprintf(victim, "You feel yourself getting BIGGER!\n\r");
	return;
}

void end_enlargement(void *vo, int target) {
        CHAR_DATA *ch = (CHAR_DATA*)vo;

        ch->size--;
}

void
spell_fireball(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;

		// Mobs only hit those they are fighting
		if (IS_NPC(ch)
		&& vch->fighting != ch)
			continue;

		if (IS_NPC(vch)
		&& ch == vch->master)
			continue;

		if (IS_NPC(ch)
		&& vch == ch->master)
			continue;

		if (vch == ch)
			continue;

		if (is_safe(ch, vch))
                        continue;

		dam = 10 + spell_damage(ch, vch, modified_level, SPELL_DAMAGE_CHART_GOOD, TRUE);
		damage(ch, vch, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);
	}

	return;
}

void
spell_huricane(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;

		if (is_safe(ch, vch))
			continue;

		if (vch == ch)
			continue;

		if(IS_NPC(vch)
		&& ch == vch->master)
			continue;

		dam = 10 + spell_damage(ch, vch, modified_level, SPELL_DAMAGE_CHART_GOOD, FALSE);
		damage(ch, vch, dam, sn, DAM_DROWNING, TRUE, TYPE_MAGIC);
	}

	return;
}

void
spell_tusunami(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, FALSE);
	damage(ch, victim, dam, sn, DAM_DROWNING, TRUE, TYPE_MAGIC);
	return;
}

void
spell_fireproof(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA af;
	AFFECT_DATA *paf;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	paf = affect_find(obj->affected, gsn_fireproof);
	if (paf == NULL && IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
	{
		act("$p is already protected from burning.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		return;
	}
	else if (paf != NULL)
	{
		affect_remove_obj(obj, paf);
	}

	af.where = TO_OBJECT;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = ITEM_BURN_PROOF;

	affect_to_obj(obj, &af);

	act("You protect $p from fire.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	act("$p is surrounded by a protective aura.", ch, obj, NULL, TO_ROOM, POS_RESTING);
}

void
spell_flamestrike(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	damage(ch, victim, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);
	return;
}

void
spell_faerie_fire(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;
	int amount = 0;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_INT, DAM_FIRE))
	{
		Cprintf(ch, "They evade your outline attempt.\n\r");
		return;
	}

	if (IS_AFFECTED(victim, AFF_HIDE))
	{
		REMOVE_BIT(victim->affected_by, AFF_HIDE);
	}

	if (IS_AFFECTED(victim, AFF_FAERIE_FIRE))
	{
		Cprintf(ch, "They're already outlined!\n\r");
		return;
	}

	// Reduce their ac by 50% of current value.
	amount = ((-GET_AC(victim, AC_EXOTIC)) + 100) / 2;
	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 1 + (modified_level / 6);
	if (IS_NPC(victim))
		af.duration *= 2;
	af.location = APPLY_AC;
	af.modifier = amount;
	af.bitvector = AFF_FAERIE_FIRE;
	affect_to_char(victim, &af);

	Cprintf(victim, "You are surrounded by a pink outline.\n\r");
	act("$n is surrounded by a pink outline.", victim, NULL, NULL, TO_ROOM, POS_RESTING);

	return;
}

void
spell_faerie_fog(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *ich;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	act("$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You conjure a cloud of purple smoke.\n\r");

	for (ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room)
	{
		if (IS_IMMORTAL(ich))
			continue;

		if (is_safe(ch, ich))
			continue;

		if (ich == ch || saving_throw(ch, ich, caster_level, sn, SAVE_HARD, STAT_DEX, DAM_NONE))
			continue;

		affect_strip(ich, gsn_oculary);
		affect_strip(ich, gsn_invisibility);
		affect_strip(ich, gsn_mass_invis);
		affect_strip(ich, gsn_sneak);
		REMOVE_BIT(ich->affected_by, AFF_HIDE);
		REMOVE_BIT(ich->affected_by, AFF_INVISIBLE);
		REMOVE_BIT(ich->affected_by, AFF_SNEAK);
		act("$n is revealed!", ich, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ich, "You are revealed!\n\r");
	}

	return;
}

void
spell_familiar(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	Cprintf(ch, "You invoke a familiar!'\n\r");
	victim = create_mobile(get_mob_index(MOB_VNUM_FAMILIAR));
	char_to_room(victim, ch->in_room);
	size_mob(ch, victim, modified_level / 3);
	SET_BIT(victim->toggles, TOGGLES_NOEXP);
	victim->spec_fun = spec_lookup("spec_familiar");
	return;
}

void
spell_fear(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
    	AFFECT_DATA af;
    	int dam, saved;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

    	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_LOW, TRUE);

	damage(ch, victim, dam, sn, DAM_MENTAL, TRUE, TYPE_MAGIC);

	saved = saving_throw(ch, victim, sn, caster_level + 2, SAVE_HARD, STAT_INT, DAM_MENTAL);

	if (saved) {
		if (ch == victim) {
			Cprintf(ch, "You feel momentarily fearful, but it passes.\n\r");
			return;

		}
        	else {
            		act("$N seems to be unfazed.", ch, NULL, victim, TO_CHAR, POS_RESTING);
			return;
		}
	}

	if(!saved) {
		af.where = TO_AFFECTS;
       		af.type = sn;
        	af.level = modified_level;
        	af.duration = 3;
		if (IS_NPC(victim))
			af.duration *= 2;
        	af.location = APPLY_DAMROLL;
        	af.modifier = dice(1, 5) * -1;
        	af.bitvector = 0;
        	affect_join(victim, &af);

        	Cprintf(victim, "The fears of the world rest on your shoulders.\n\r");
        	act("$n feels the fears of the world on $s shoulders.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}

   	return;
}

void
spell_flame_arrow(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam1, dam2;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam1 = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_LOW, TRUE);
	dam2 = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_LOW, TRUE);

	damage(ch, victim, dam1, sn, DAM_PIERCE, TRUE, TYPE_SKILL);
	damage(ch, victim, dam2, sn, DAM_FIRE,   TRUE, TYPE_MAGIC);

	return;
}

void
spell_floating_disc(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *disc, *floating;
	int caster_level, modified_level;
	AFFECT_DATA af;
	int duration = 0;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	floating = get_eq_char(ch, WEAR_FLOAT);
	if (floating != NULL && IS_OBJ_STAT(floating, ITEM_NOREMOVE))
	{
		act("You can't remove $p.", ch, floating, NULL, TO_CHAR, POS_RESTING);
		return;
	}

	duration = ch->level * 2 - number_range(0, ch->level / 2);

	disc = create_object(get_obj_index(OBJ_VNUM_DISC), 0);
	disc->value[0] = ch->level * 10;	/* 10 pounds per level capacity */
	disc->value[3] = ch->level * 5;		/* 5 pounds per level max per item */
	disc->timer = duration;

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = duration;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	act("$n has created a floating black disc.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You create a floating disc.\n\r");
	obj_to_char(disc, ch);
	wear_obj(ch, disc, TRUE);
	return;
}

void
spell_fly(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_AFFECTED(victim, AFF_FLYING))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your flight.\n\r");
		else
		{
			act("$N refreshed your flight.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the flight spell.\n\r");
		}
		affect_refresh(victim, sn, modified_level);
		return;
	}
	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level + 3;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_FLYING;
	affect_to_char(victim, &af);
	Cprintf(victim, "Your feet rise off the ground.\n\r");
	act("$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	return;
}

/* RT clerical berserking spell */

void
spell_frenzy(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, gsn_berserk)
	|| IS_AFFECTED(victim, AFF_BERSERK)
	|| is_affected(victim, gsn_taunt))
	{
		if (victim == ch)
			Cprintf(ch, "You are already in a frenzy.\n\r");
		else
			act("$N is already in a frenzy.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}
	if (is_affected(victim, gsn_guardian)) {
		if (victim == ch) 
			Cprintf(ch, "You're too busy defending to frenzy.\n\r");
		else
			act("$N is too busy defending to frenzy.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (is_affected(victim, gsn_calm))
	{
		if (victim == ch)
			Cprintf(ch, "Why don't you just relax for a while?\n\r");
		else
			act("$N doesn't look like $e wants to fight anymore.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (Holy_Bless != 1)
	{
		if ((IS_GOOD(ch) && !IS_GOOD(victim)) ||
			(IS_NEUTRAL(ch) && !IS_NEUTRAL(victim)) ||
			(IS_EVIL(ch) && !IS_EVIL(victim)))
		{
			act("Your god doesn't seem to like $N", ch, NULL, victim, TO_CHAR, POS_RESTING);
			return;
		}
	}

	if (is_affected(victim, sn))
	{
		if (victim == ch)
		{
			Cprintf(ch, "You refresh your fanatical rage!\n\r");
		}
		else
		{
			act("$N refreshed your fanatical rage.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the frenzy.\n\r");
		}
		affect_refresh(victim, sn, modified_level / 3);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 3;
	af.modifier = modified_level / 6;
	af.bitvector = 0;

	af.location = APPLY_HITROLL;
	affect_to_char(victim, &af);

	af.location = APPLY_DAMROLL;
	affect_to_char(victim, &af);

	af.modifier = 10 * (modified_level / 10);
	af.location = APPLY_AC;
	affect_to_char(victim, &af);

	Cprintf(victim, "You are filled with holy wrath!\n\r");
	act("$n gets a wild look in $s eyes!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
}

/* RT ROM-style gate */

void
spell_gate(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
    CHAR_DATA *victim = NULL;
    ROOM_INDEX_DATA *in_room = NULL;
    ROOM_INDEX_DATA *to_room = NULL;
    ROOM_INDEX_DATA *rand_room = NULL;
    CHAR_DATA* slider = NULL;
    bool gate_pet, was_fighting=FALSE;
    int caster_level, modified_level;

    caster_level = get_caster_level(level);
    modified_level = get_modified_level(level);

    if ((victim = get_char_world_finished_areas(ch, target_name, FALSE)) == NULL
            || victim == ch
            || victim->in_room == NULL
            || !can_see_room(ch, victim->in_room)
	    || victim->in_room->clan
            || IS_SET(victim->in_room->room_flags, ROOM_SAFE)
            || IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
            || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
            || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
            || IS_SET(victim->in_room->room_flags, ROOM_LAW)
            || IS_SET(victim->in_room->room_flags, ROOM_NO_GATE)
            || wearing_nogate_item(ch)
            || IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
            || victim->level >= modified_level + 3
            || (is_clan(victim) && !is_same_clan(ch, victim))
            || (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
            || (IS_NPC(victim) && IS_AFFECTED(victim, AFF_CHARM) && victim->master != ch)
            || (IS_NPC(victim)
                    && number_percent() < 20)) {
        Cprintf(ch, "You failed.\n\r");
        return;
    }

    if (victim->in_room->area->continent != ch->in_room->area->continent) {
        Cprintf(ch, "Inter-continental magic is forbidden.  You failed.\n\r");
        return;
    }

    // Gate works, but you lose your stealth.
    REMOVE_BIT(ch->affected_by, AFF_HIDE);

    if (ch->pet != NULL && ch->in_room == ch->pet->in_room) {
        gate_pet = TRUE;
    } else {
        gate_pet = FALSE;
    }

    act("$n steps through a gate and vanishes.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
    Cprintf(ch, "You step through a gate and vanish.\n\r");

    if (ch->fighting && is_affected(ch->fighting, gsn_slide)) {
        slider = ch->fighting;
        was_fighting = TRUE;
    } else {
        for(slider = ch->in_room->people; slider != NULL; slider = slider->next_in_room) {
            if (is_same_group(ch, slider) && is_affected(slider, gsn_slide))
                break;
        }
    }

    in_room = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, victim->in_room);
    to_room = ch->in_room;

    act("$n has arrived through a gate.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

    if (slider && number_percent() < get_skill(slider, gsn_slide) && slider != ch) {
        act("$n slides through the gate and vanishes.", slider, NULL, NULL, TO_ROOM, POS_RESTING);
        Cprintf(slider, "You slide through the gate and vanish.\n\r");

        /* 15% chance of being cut loose */
        if (number_percent() < 15) {
            Cprintf(slider, "The world spins!\n\r");
            rand_room = get_random_room(slider);

            while ( IS_SET(rand_room->room_flags, ROOM_NO_GATE) ||
                    rand_room->area->continent != slider->in_room->area->continent
                    || rand_room->area->security < 9) {
                rand_room = get_random_room(slider);
            }

            char_from_room(slider);
            char_to_room(slider, rand_room);
            act("A gate opens and $n slides into the room.", slider, NULL, NULL, TO_ROOM, POS_RESTING);
            check_improve(slider, gsn_slide, FALSE, 2);
            do_look(slider, "auto");
        } else {
            char_from_room(slider);
            char_to_room(slider, victim->in_room);
            act("A gate opens and $n slides into the room.", slider, NULL, NULL, TO_ROOM, POS_RESTING);
            check_improve(slider, gsn_slide, TRUE, 2);
            do_look(slider, "auto");

            if (was_fighting) {
                set_fighting(slider, ch);
            }
        }
    }

    do_look(ch, "auto");

    if (gate_pet) {
        act("$n steps through a gate and vanishes.", ch->pet, NULL, NULL, TO_ROOM, POS_RESTING);
        Cprintf(ch->pet, "You step through a gate and vanish.\n\r");
        char_from_room(ch->pet);
        char_to_room(ch->pet, victim->in_room);
        act("$n has arrived through a gate.", ch->pet, NULL, NULL, TO_ROOM, POS_RESTING);
        do_look(ch->pet, "auto");
    }
}

void
spell_giant_insect(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	AFFECT_DATA af3;
	int nombre;
	int insect;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	if (max_no_charmies(ch))
	{
		Cprintf(ch, "You can't call more insects.\n\r");
		return;
	}

	nombre = dice(1, 3);
	Cprintf(ch, "You chant: 'Come insects! Come!'\n\r");
	for (insect = 0; insect < nombre; insect++)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_INSECT));
		char_to_room(victim, ch->in_room);
		size_mob(ch, victim, modified_level * 4 / 5);
		if (victim->master)
			stop_follower(victim);
		add_follower(victim, ch);
		victim->leader = ch;

		af3.where = TO_AFFECTS;
		af3.type = sn;
		af3.level = modified_level;
		af3.duration = modified_level / 2;
		af3.location = 0;
		af3.modifier = 0;
		af3.bitvector = AFF_CHARM;
		affect_to_char(victim, &af3);
	}
}

void
spell_giant_strength(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your strength!\n\r");
		else
		{
			act("$N refreshed your strength.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the giant strength.\n\r");
		}
		affect_refresh(victim, sn, modified_level);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level;
	af.location = APPLY_STR;
	af.modifier = 1 + (modified_level >= 18) + (modified_level >= 25) + (modified_level >= 32) + (modified_level >= 52);
	af.bitvector = 0;
	affect_to_char(victim, &af);
	Cprintf(victim, "Your muscles surge with heightened power!\n\r");
	act("$n's muscles surge with heightened power.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	return;
}

void
spell_grandeur(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_AFFECTED(victim, AFF_MINIMATION))
	{
		Cprintf(ch, "You can't look weak and strong at the same time.\n\r");
		return;
	}

	if (is_affected(victim, sn))
	{
		Cprintf(ch, "You refresh your grandeur spell.\n\r");
		affect_refresh(victim, sn, number_fuzzy(modified_level / 4));
		return;
	}
	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = number_fuzzy(modified_level / 4);
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = AFF_GRANDEUR;
	affect_to_char(victim, &af);
	Cprintf(victim, "You now appear invincible!\n\r");
	if (ch != victim)
		act("$N suddenly looks alot tougher.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_harm(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (saving_throw(ch, victim, sn, modified_level, SAVE_HARD, STAT_NONE, DAM_HARM)) {
		Cprintf(ch, "Your foe comes to no harm.\n\r");
		return;
	}

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
	dam += victim->hit / 50;

	if(dam > 250)
		dam = 250;

	damage(ch, victim, dam, sn, DAM_HARM, TRUE, TYPE_MAGIC);
	return;
}

/* RT haste spell */

void
spell_haste(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your haste.\n\r");
		else
		{
			act("$N refreshed your haste.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the haste spell.\n\r");
		}
		affect_refresh(victim, sn, modified_level / 3);
		return;
	}

	if (IS_AFFECTED(victim, AFF_HASTE)
		|| IS_SET(victim->off_flags, OFF_FAST))
	{
		if (victim == ch)
			Cprintf(ch, "You can't move any faster!\n\r");
		else
			act("$N is already moving as fast as $E can.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (IS_AFFECTED(victim, AFF_SLOW))
	{
		if (!try_cancel(victim, gsn_slow, modified_level))
		{
			if (victim != ch)
				Cprintf(ch, "Spell failed.\n\r");
			Cprintf(victim, "You feel momentarily faster.\n\r");
			return;
		}
		affect_strip(victim, gsn_slow);
		Cprintf(victim, "You feel yourself speed up.\n\r");
		act("$n is moving less slowly.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 3;
	af.location = APPLY_DEX;
	af.modifier = 1 + (modified_level >= 18) + (modified_level >= 25) + (modified_level >= 32) + (modified_level >= 52);
	af.bitvector = AFF_HASTE;
	affect_to_char(victim, &af);
	Cprintf(victim, "You feel yourself moving more quickly.\n\r");
	act("$n is moving more quickly.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	return;
}



void
spell_heal(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
        int heal_amount;

	heal_amount = 100;

	if (ch->charClass == class_lookup("cleric") &&
	    ch->alignment <= -350)
	    heal_amount = 90;
	else if (ch->charClass == class_lookup("cleric") &&
	    ch->alignment < 350)
	    heal_amount = 95;
	
	

	if(!is_affected(victim, gsn_dissolution))
		victim->hit = UMIN(victim->hit + heal_amount, MAX_HP(victim));

	update_pos(victim);
	Cprintf(victim, "A warm feeling fills your body.\n\r");
	if (ch != victim)
		Cprintf(ch, "Ok.\n\r");
	return;
}

void
spell_heat_metal(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj_lose, *obj_next;
	int dam = 0;
	bool fail = TRUE;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (!saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_CON, DAM_FIRE)
		&& !IS_SET(victim->imm_flags, IMM_FIRE))
	{
		for (obj_lose = victim->carrying;
			 obj_lose != NULL;
			 obj_lose = obj_next)
		{
			obj_next = obj_lose->next_content;
				if( number_percent() < 20
				&& !IS_OBJ_STAT(obj_lose, ITEM_NONMETAL)
				&& !IS_OBJ_STAT(obj_lose, ITEM_BURN_PROOF))
			{
				switch (obj_lose->item_type)
				{
				case ITEM_ARMOR:
					if (obj_lose->wear_loc != -1)	/* remove the item */
					{
						if (can_drop_obj(victim, obj_lose)
							&& (obj_lose->weight / 10) <
							number_range(1, 2 * get_curr_stat(victim, STAT_DEX))
							&& remove_obj(victim, obj_lose->wear_loc, TRUE))
						{
							act("$n yelps and throws $p to the ground!", victim, obj_lose, NULL, TO_ROOM, POS_RESTING);
							act("You remove and drop $p before it burns you.", victim, obj_lose, NULL, TO_CHAR, POS_RESTING);
							dam += number_range(1, obj_lose->level);
							obj_from_char(obj_lose);
							obj_to_room(obj_lose, victim->in_room);
							fail = FALSE;
						}
						else
							/* stuck on the body! ouch! */
						{
							act("Your skin is seared by $p!", victim, obj_lose, NULL, TO_CHAR, POS_RESTING);
							dam += obj_lose->level;
							fail = FALSE;
						}

					}
					else
						/* drop it if we can */
					{
						if (can_drop_obj(victim, obj_lose))
						{
							act("$n yelps and throws $p to the ground!", victim, obj_lose, NULL, TO_ROOM, POS_RESTING);
							act("You and drop $p before it burns you.", victim, obj_lose, NULL, TO_CHAR, POS_RESTING);
							dam += number_range(1, obj_lose->level);
							obj_from_char(obj_lose);
							obj_to_room(obj_lose, victim->in_room);
							fail = FALSE;
						}
						else
							/* cannot drop */
						{
							act("Your skin is seared by $p!", victim, obj_lose, NULL, TO_CHAR, POS_RESTING);
							dam += obj_lose->level;
							fail = FALSE;
						}
					}
					break;
				case ITEM_WEAPON:
					if (obj_lose->wear_loc != -1)	/* try to drop it */
					{
						if (can_drop_obj(victim, obj_lose)
							&& remove_obj(victim, obj_lose->wear_loc, TRUE))
						{
							act("$n is burned by $p, and throws it to the ground.", victim, obj_lose, NULL, TO_ROOM, POS_RESTING);
							Cprintf(victim, "You throw your red-hot weapon to the ground!\n\r");
							dam += number_range(1, obj_lose->level);
							obj_from_char(obj_lose);
							obj_to_room(obj_lose, victim->in_room);
							fail = FALSE;
						}
						else
							/* YOWCH! */
						{
							Cprintf(victim, "Your weapon sears your flesh!\n\r");
							dam += obj_lose->level;
							fail = FALSE;
						}
					}
					else
						/* drop it if we can */
					{
						if (can_drop_obj(victim, obj_lose))
						{
							act("$n throws a burning hot $p to the ground!", victim, obj_lose, NULL, TO_ROOM, POS_RESTING);
							act("You and drop $p before it burns you.", victim, obj_lose, NULL, TO_CHAR, POS_RESTING);
							dam += (number_range(1, obj_lose->level) / 5);
							obj_from_char(obj_lose);
							obj_to_room(obj_lose, victim->in_room);
							fail = FALSE;
						}
						else
							/* cannot drop */
						{
							act("Your skin is seared by $p!", victim, obj_lose, NULL, TO_CHAR, POS_RESTING);
							dam += obj_lose->level;
							fail = FALSE;
						}
					}
					break;
				}
			}
		}
	}
	if (fail)
	{
		Cprintf(ch, "Your spell had no effect.\n\r");
		Cprintf(victim, "You feel momentarily warmer.\n\r");
	}
	else
		/* damage! */
	{
		damage(ch, victim, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);
	}
}

void
spell_hell_blade(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_HIGH, TRUE);

	damage(ch, victim, dam, sn, DAM_SLASH, TRUE, TYPE_MAGIC);

	return;

}

void
spell_hold_person(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_STR, DAM_CHARM))
	{
		Cprintf(ch, "Your victim doesn't even flinch!\n\r");
		return;
	}

	Cprintf(ch, "Your victim seems paralyzed!\n\r");
	Cprintf(victim, "You are completely paralyzed for a few moments!\n\r");
	WAIT_STATE(victim, PULSE_VIOLENCE * dice(1, 4));
	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	return;

}

/* RT really nasty high-level attack spell */
void
spell_holy_word(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;
	int deity_number;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	deity_number = ch->deity_type;
	if (deity_number == 0)
	{
		Cprintf(ch, "Atheists cannot call upon divine power!\n\r");
		return;
	}

	Holy_Bless = 1;
	act("$n utters a word of divine power!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You utter a word of divine power.\n\r");

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;
		if (!IS_NPC(vch))
		{
			if (vch->deity_type == deity_number ||
				(vch->clan == ch->clan && is_clan(ch)) ||
				is_same_group(ch, vch))
			{
				Cprintf(vch, "You feel more powerful.\n\r");
				spell_frenzy(gsn_frenzy, level, ch, (void *) vch, TARGET_CHAR);
				spell_bless(gsn_bless, level, ch, (void *) vch, TARGET_CHAR);
				spell_heal(gsn_heal, level, ch, (void *) vch, TARGET_CHAR);
			}
			else
			{
				if (!is_safe(ch, vch))
				{
					spell_curse(gsn_curse, level, ch, (void *) vch, TARGET_CHAR);
					spell_heat_metal(gsn_heat_metal, level, ch, (void *) vch, TARGET_CHAR);
					Cprintf(vch, "You are struck down!\n\r");
					dam = spell_damage(ch, vch, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
					damage(ch, vch, dam, sn, DAM_ENERGY, TRUE, TYPE_MAGIC);
				}
			}
		}
		else
		{
			if (is_same_group(ch, vch))
			{
				Cprintf(vch, "You feel more powerful.\n\r");
				spell_frenzy(gsn_frenzy, level, ch, (void *) vch, TARGET_CHAR);
				spell_bless(gsn_bless, level, ch, (void *) vch, TARGET_CHAR);
				spell_heal(gsn_heal, level, ch, (void *) vch, TARGET_CHAR);
			}
			else
			{
				if (!is_safe(ch, vch))
				{
					spell_curse(gsn_curse, level, ch, (void *) vch, TARGET_CHAR);
					spell_heat_metal(gsn_heat_metal, level, ch, (void *) vch, TARGET_CHAR);
					Cprintf(vch, "You are struck down!\n\r");
					dam = spell_damage(ch, vch, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
					damage(ch, vch, dam, sn, DAM_ENERGY, TRUE, TYPE_MAGIC);
				}
			}
		}
	}

	Cprintf(ch, "You feel drained.\n\r");
	ch->move /= 2;

	Holy_Bless = 0;
}

void
spell_homonculus(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;
	char buf[MAX_STRING_LENGTH];
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if ((victim->master == ch) && (IS_NPC(victim)))
		{
			Cprintf(ch, "Homonculi like being the centre of attention. Get rid of your buddies.\n\r");
			return;
		}
	}

	Cprintf(ch, "You form a small being in your image!\n\r");
	act("$n creates a small being in $s image!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	victim = create_mobile(get_mob_index(MOB_VNUM_HOMONCULUS));

	sprintf(buf, "homonculus %sabc Mini-%s", ch->name, ch->name);
	free_string(victim->name);
	victim->name = str_dup(buf);

	sprintf(buf, "Mini-%s", ch->name);
	free_string(victim->short_descr);
	victim->short_descr = str_dup(buf);

	sprintf(buf, "A miniature version of %s stands here.\n\r", ch->name);
	free_string(victim->long_descr);
	victim->long_descr = str_dup(buf);

	char_to_room(victim, ch->in_room);
	victim->sex = ch->sex;
	size_mob(ch, victim, caster_level * 5 / 6);

	add_follower(victim, ch);
	victim->leader = ch;

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(victim, &af);

	act("{yYou say '$e's beautiful. I shall call $m... Mini-Me.'{x", ch, NULL, NULL, TO_CHAR, POS_RESTING);
	act("{y$n says '$e's beautiful. I shall call $m... Mini-Me.'{x", ch, NULL, NULL, TO_ROOM, POS_RESTING);

}

void
spell_lightning_spear(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	CHAR_DATA *vict_was = NULL, *ch_was = NULL;
	int dam;
	int door, distance;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);
	if(target_name[0] == '\0' && ch->fighting)
		victim = ch->fighting;
	else
		victim = range_finder(ch, target_name, 2, &door, &distance, FALSE);

	if (victim == NULL)
	{
		Cprintf(ch, "You conjure a magical spear that falls on the ground and vanishes.\n\r");
		act("$n conjures a magical spear that falls on the ground and vanishes.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You can't find it.\n\r");
		return;
	}

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_HIGH, TRUE);

	if (!IS_NPC(ch))
	{
		if (is_safe(ch, victim)
			|| (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim))
			victim = ch;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You conjure a magical spear that suddenly strikes YOU!\n\r");
		act("$n is struck by the magical spear $e just conjured!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	else if (ch->in_room == victim->in_room)
	{
		act("$n conjures a magical spear that strikes $N.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		act("$n conjures a magical spear that strikes you!", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("You conjures a magical spear that strikes $N.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	}
	else
	{
		act("$n conjures a magical spear that suddenly flies away.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		act("You conjures a magical spear that suddenly flies away.", ch, NULL, NULL, TO_CHAR, POS_RESTING);
		act("$n is suddenly struck by a flying magical spear!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		act("You are suddenly struck by a flying magical spear.", victim, NULL, NULL, TO_CHAR, POS_RESTING);
	}

	if(ch->fighting)
		ch_was = ch->fighting;
	if(victim->fighting)
		vict_was = victim->fighting;

	check_killer(ch, victim);
	if(!IS_NPC(ch)
	&& !IS_NPC(victim)) {
		ch->no_quit_timer = 3;
		victim->no_quit_timer = 3;
	}

	damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);

	// Should stop counter attacks.
	if(distance > 0 && ch_was == NULL)
	{
		stop_fighting(ch, FALSE);
	}
	if(distance > 0 && vict_was == NULL)
	{
		stop_fighting(victim, FALSE);
	}

	return;
}

void
spell_hypnosis(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{

	CHAR_DATA *victim = (CHAR_DATA *) vo;
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_STRING_LENGTH];
	char *arg2;
	char arg3[MAX_STRING_LENGTH];
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(ch->charClass == class_lookup("thief"))
		caster_level = ch->level;

	arg2 = one_argument(target_name, arg);
	one_argument(arg2, arg3);

	if (arg[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Force whom to do what?\n\r");
		return;
	}

	if (!can_order(arg3))
	{
		Cprintf(ch, "That will not be done.\n\r");
		return;
	}

	sprintf(buf, "$n hypnotizes you to '%s'.", arg2);

	if (victim == ch)
	{
		Cprintf(ch, "Aye aye, right away!\n\r");
		return;
	}

	if (IS_IMMORTAL(victim))
	{
		Cprintf(ch, "Do it yourself!\n\r");
		return;
	}

	if (saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_WIS, DAM_CHARM))
	{
		return;
	}

	act(buf, ch, NULL, victim, TO_VICT, POS_RESTING);
	interpret(victim, arg2);

	act("You hypnotize $N.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;

}

void
spell_ice_bolt(int sn, int level, CHAR_DATA * ch, void *vo,
			   int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	damage(ch, victim, dam, sn, DAM_COLD, TRUE, TYPE_MAGIC);
	return;
}

void
spell_identify(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;

	lore_obj(ch, obj);
	return;
}

void
spell_infravision(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_AFFECTED(victim, AFF_INFRARED))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your infravision.\n\r");
		else
			act("You refresh $N's infravision.\n\r", ch, NULL, victim, TO_CHAR, POS_RESTING);
		affect_refresh(victim, sn, modified_level * 2);
		return;
	}

	act("$n's eyes glow red.\n\r", victim, NULL, NULL, TO_ROOM, POS_RESTING);

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 2 * modified_level;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_INFRARED;
	affect_to_char(victim, &af);
	Cprintf(victim, "Your eyes glow red.\n\r");
	return;
}

void
spell_invis(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* object invisibility */
	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;

		if (IS_OBJ_STAT(obj, ITEM_INVIS))
		{
			act("$p is already invisible.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			return;
		}

		af.where = TO_OBJECT;
		af.type = sn;
		af.level = modified_level;
		af.duration = modified_level + 12;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = ITEM_INVIS;
		affect_to_obj(obj, &af);

		act("$p fades out of sight.", ch, obj, NULL, TO_ALL, POS_RESTING);
		return;
	}

	/* character invisibility */
	victim = (CHAR_DATA *) vo;

	if (IS_AFFECTED(victim, AFF_INVISIBLE))
	{
		Cprintf(ch, "You refresh the invisibility.\n\r");
		affect_refresh(victim, sn, modified_level + 12);
		return;
	}

	act("$n fades out of existence.", victim, NULL, NULL, TO_ROOM, POS_RESTING);

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level + 12;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_INVISIBLE;
	affect_to_char(victim, &af);
	Cprintf(victim, "You fade out of existence.\n\r");
	return;
}

void
spell_karma(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int value;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim != ch)
		{
			Cprintf(ch, "Their karma is already being influenced.\n\r");
			return;
		}
		else
		{
			Cprintf(ch, "Your karma will not allow it yet!\n\r");
			return;
		}
	}

	Cprintf(ch, "You twist the powers of fate...\n\r");

	/* apply the good effect */
	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 3;
	af.bitvector = 0;

	value = number_range(1, 8);
	switch (value)
	{
	case 1:
		af.location = APPLY_STR;
		af.modifier = ch->level / 12;
		Cprintf(victim, "Your karma grows stronger!\n\r");
		act("$n's karma grows stronger!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		break;
	case 2:
		af.location = APPLY_DEX;
		af.modifier = ch->level / 12;
		Cprintf(victim, "Your karma moves faster!\n\r");
		act("$n's karma moves faster!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		break;
	case 3:
		af.location = APPLY_CON;
		af.modifier = ch->level / 12;
		Cprintf(victim, "Your karma gets tougher!\n\r");
		act("$n's karma gets tougher!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		break;
	case 4:
		af.location = APPLY_INT;
		af.modifier = ch->level / 12;
		Cprintf(victim, "Your karma looks smarter!\n\r");
		act("$n's karma looks smarter!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		break;
	case 5:
		af.location = APPLY_WIS;
		af.modifier = ch->level / 12;
		Cprintf(victim, "Your karma grows more enlightened!\n\r");
		act("$n's karma grows more enlightened!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		break;
	case 6:
		af.location = APPLY_SAVES;
		af.modifier = 0 - (ch->level / 10);
		Cprintf(victim, "Your karma is now protected!\n\r");
		act("$n's karma is now protected!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		break;
	case 7:
		af.location = APPLY_HITROLL;
		af.modifier = ch->level / 10;
		Cprintf(victim, "Your karma is more accurate!\n\r");
		act("$n's karma is more accurate!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		break;
	case 8:
		af.location = APPLY_DAMROLL;
		af.modifier = ch->level / 10;
		Cprintf(victim, "Your karma is more deadly!\n\r");
		act("$n's karma is more deadly!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		break;
	}

	affect_to_char(victim, &af);

}

void
spell_know_alignment(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	char *msg;
	int ap;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	ap = victim->alignment;

	if (ap > 700)
		msg = "$N has a pure and good aura.";
	else if (ap > 350)
		msg = "$N is of excellent moral character.";
	else if (ap > 100)
		msg = "$N is often kind and thoughtful.";
	else if (ap > -100)
		msg = "$N doesn't have a firm moral commitment.";
	else if (ap > -350)
		msg = "$N lies to $S friends.";
	else if (ap > -700)
		msg = "$N is a black-hearted murderer.";
	else
		msg = "$N is the embodiment of pure evil.";

	act(msg, ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_lightning_bolt(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);
	return;
}

void
spell_locate_object(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	char buf[MAX_INPUT_LENGTH];
	BUFFER *buffer;
	OBJ_DATA *obj;
	OBJ_DATA *in_obj;
	bool found;
	int number = 0, max_found;

	found = FALSE;
	number = 0;
	max_found = IS_IMMORTAL(ch) ? 200 : 2 * level;

	buffer = new_buf();

	for (obj = object_list; obj != NULL; obj = obj->next)
	{
		if (!can_see_obj(ch, obj) || !is_name(target_name, obj->name)
		  || IS_OBJ_STAT(obj, ITEM_NOLOCATE) || number_percent() > 2 * level
			|| ch->level < obj->level)
			continue;

		found = TRUE;
		number++;

		for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj)
			;

		if (in_obj->carried_by != NULL && can_see(ch, in_obj->carried_by))
		{
			sprintf(buf, "one is carried by %s\n\r",
					PERS(in_obj->carried_by, ch));
		}
		else
		{
			if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
				sprintf(buf, "one is in %s [Room %d]\n\r",
						in_obj->in_room->name, in_obj->in_room->vnum);
			else
				sprintf(buf, "one is in %s\n\r",
						in_obj->in_room == NULL
						? "somewhere" : in_obj->in_room->name);
		}

		buf[0] = UPPER(buf[0]);
		add_buf(buffer, buf);

		if (number >= max_found)
			break;
	}

	if (!found)
		Cprintf(ch, "Nothing like that in heaven or earth.\n\r");
	else
		page_to_char(buf_string(buffer), ch);

	free_buf(buffer);

	return;
}

void
spell_loneliness(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, gsn_loneliness))
	{
		if (victim == ch)
			Cprintf(ch, "You can't be lonelier than you presently are.\n\r");
		else
			act("$N can't be any lonelier.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_INT, DAM_CHARM))
	{
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 4;
	if (IS_NPC(victim))
		af.duration *= 2;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = 0;
	affect_to_char(victim, &af);
	Cprintf(victim, "You feel so lonely all of a sudden.\n\r");
	if (ch != victim)
		act("$N suddenly looks sooooo lonely.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_magic_missile(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;
	int victim_dead = FALSE;
	int count;
	int total = 0;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	total = (modified_level > 0) + (modified_level > 4) + (modified_level > 14);

	for(count = 0; count < total; count ++)
	{
		dam = dice(1, 10) + modified_level / 2;
		victim_dead = damage(ch, victim, dam, sn, DAM_ENERGY, TRUE, TYPE_MAGIC);
		if(victim_dead)
			return;
	}

	return;
}

void
spell_magic_stone(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
	char buf[MAX_STRING_LENGTH];
    int dam, stones, i, hits=0;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

    	dam = spell_damage(ch, victim, caster_level, SPELL_DAMAGE_HIGH, TRUE);

	stones = dice(1, 6);

	Cprintf(ch, "You conjure %d enchanted stones which attack %s!\n\r", stones, IS_NPC(victim) ? victim->short_descr : victim->name );
	Cprintf(victim, "%s conjures %d enchanted stones!\n\r", ch->name, stones);

	damage(ch, victim, dam, sn, DAM_BASH, TRUE, TYPE_MAGIC);

	for(i=0;i<stones;i++)
	{
		if(number_percent() > get_curr_stat(victim, STAT_DEX))
			hits++;
	}

	DAZE_STATE(victim, 6 * hits);

	if(hits > 0)
	{
		sprintf(buf, "%s is struck down by %d magic stones!", IS_NPC(victim) ? victim->short_descr : victim->name, hits );
		act(buf, victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	else {
		sprintf(buf, "%s dodges all the magic stones.", IS_NPC(victim) ? victim->short_descr : victim->name);
		act(buf, victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	if(hits >= 4) {
		sprintf(buf, "%s looks dazed and stunned.", IS_NPC(victim) ? victim->short_descr : victim->name );
		act(buf, victim, NULL, NULL, TO_ROOM, POS_RESTING);
		WAIT_STATE(victim, dice(3, 6));
	}
        return;
}

void
spell_mass_healing(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *gch;

	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
	{
		if ((IS_NPC(ch) && IS_NPC(gch)) ||
			(!IS_NPC(ch) && !IS_NPC(gch)))
		{
			spell_heal(gsn_heal, level, ch, (void *) gch, TARGET_CHAR);
			spell_refresh(gsn_refresh, level, ch, (void *) gch, TARGET_CHAR);
		}
	}
}


void
spell_mass_invis(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	CHAR_DATA *gch;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
	{
		if (!is_same_group(gch, ch) || IS_AFFECTED(gch, AFF_INVISIBLE))
			continue;

		act("$n slowly fades out of existence.", gch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(gch, "You slowly fade out of existence.\n\r");

		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = modified_level / 2;
		af.duration = 24;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_INVISIBLE;
		affect_to_char(gch, &af);
	}
	Cprintf(ch, "Ok.\n\r");

	return;
}

void
spell_mass_protect(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(is_affected(ch, gsn_mass_protect)) {
		Cprintf(ch, "You refresh the mass protection.\n\r");
		affect_refresh(ch, sn, modified_level / 2);
		return;
	}

	act("$n looks rather secure.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You feel a wave of security wash over you.\n\r");

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level / 2;
	af.duration = modified_level / 2;
	af.location = APPLY_AC;
	af.modifier = (modified_level / 15) * -10;
	af.bitvector = 0;
	affect_to_char(ch, &af);
	return;
}


void
spell_materialize(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* deal with the object case first */
	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;
		if (!IS_OBJ_STAT(obj, ITEM_INVIS))
		{
			act("$p is already visible.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			return;
		}
		REMOVE_BIT(obj->extra_flags, ITEM_INVIS);
		act("$p suddenly comes into view.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		return;
	}
	Cprintf(ch, "You don't see that here.\n\r");
	return;
}

void
spell_melior(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (saving_throw(ch, victim, sn, caster_level + 2, SAVE_HARD, STAT_INT, DAM_ENERGY))
	{
		Cprintf(ch, "You failed to tip the scales of good.\n\r");
		return;
	}

	if (victim->alignment == 1000)
	{
		Cprintf(ch, "They are already as good as they can get.\n\r");
		return;
	}

	victim->alignment = victim->alignment + 250;

	if (victim->alignment > 1000)
		victim->alignment = 1000;


	Cprintf(victim, "You feel yourself becoming a better person.\n\r");
	if (ch != victim)
		act("$N is becoming a better person.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_minimation(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_AFFECTED(victim, AFF_GRANDEUR))
	{
		Cprintf(ch, "You can't look weak and strong at the same time.\n\r");
		return;
	}


	if (is_affected(victim, sn))
	{
		affect_refresh(victim, sn, modified_level / 4);
		Cprintf(ch, "You refresh your minimation spell.\n\r");
		return;
	}
	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 4;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = AFF_MINIMATION;
	affect_to_char(victim, &af);
	Cprintf(victim, "You're not quite dead yet!\n\r");
	if (ch != victim)
		act("$N suddenly looks almost dead.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_mirror_image(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh your mirror images.\n\r");
		else
		{
			act("$N refreshed your mirror images.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the mirror image spell.\n\r");
		}
		affect_refresh(victim, sn, modified_level / 3);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 3;
	af.modifier = 0;
	af.bitvector = 0;

	af.location = APPLY_NONE;
	affect_to_char(victim, &af);

	act("$n creates a mirror image.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(victim, "You create a mirror image of yourself.\n\r");
	return;

}

void
spell_mutate(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA *paf;
	int result, fail;
	int hit_bonus, dam_bonus;
	bool hit_found = FALSE, dam_found = FALSE;
	int dam = 0;
	int new_flag;
	char arg[MAX_INPUT_LENGTH];
	char *arg2;
	char arg3[MAX_INPUT_LENGTH];
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	arg2 = one_argument(target_name, arg);
	one_argument(arg2, arg3);

	if (obj->item_type != ITEM_WEAPON)
	{
		Cprintf(ch, "That isn't a weapon.\n\r");
		return;
	}

	if (obj->wear_loc != -1)
	{
		Cprintf(ch, "The item must be carried to be enchanted.\n\r");
		return;
	}

	/* new: make sure we don't mutate a temporary flag - Litazia */
	/* also for no_mutate */
	if ((affect_find(obj->affected, gsn_poison) != NULL) ||
		(affect_find(obj->affected, gsn_flag) != NULL) ||
		(affect_find(obj->affected, gsn_blade_rune) != NULL) ||
		IS_WEAPON_STAT(obj, WEAPON_NOMUTATE) ||
		IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)
		// don't allow mutate of crafted flags
		|| IS_WEAPON_STAT(obj, WEAPON_HOLY)
		|| IS_WEAPON_STAT(obj, WEAPON_UNHOLY)
		|| IS_WEAPON_STAT(obj, WEAPON_POLAR)
		|| IS_WEAPON_STAT(obj, WEAPON_DEMONIC)
		|| IS_WEAPON_STAT(obj, WEAPON_PSIONIC)
		|| IS_WEAPON_STAT(obj, WEAPON_PHASE)
		|| IS_WEAPON_STAT(obj, WEAPON_INTELLIGENT)
		|| IS_WEAPON_STAT(obj, WEAPON_ENTROPIC)
		|| IS_WEAPON_STAT(obj, WEAPON_ANTIMAGIC))

	{
		act("$p cleverly resists being mutated.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		return;
	}

	if (obj->value[4] == 0)
	{
		Cprintf(ch, "The item must already have a weapon flag in order to be mutated.\n\r");
		return;
	}

	/* this means they have no bonus */
	hit_bonus = 0;
	dam_bonus = 0;
	fail = 25;					/* base 25% chance of failure */

	/* find the bonuses */

	if (!obj->enchanted) {
		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
		{
			if (paf->location == APPLY_HITROLL)
			{
				hit_bonus = paf->modifier;
				hit_found = TRUE;
				fail += 2 * (hit_bonus * hit_bonus);
			}

			else if (paf->location == APPLY_DAMROLL)
			{
				dam_bonus = paf->modifier;
				dam_found = TRUE;
				fail += 2 * (dam_bonus * dam_bonus);
			}

			else				/* things get a little harder */
				fail += 25;
		}
	}

	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
		if (paf->location == APPLY_HITROLL)
		{
			hit_bonus = paf->modifier;
			hit_found = TRUE;
			fail += 2 * (hit_bonus * hit_bonus);
		}

		else if (paf->location == APPLY_DAMROLL)
		{
			dam_bonus = paf->modifier;
			dam_found = TRUE;
			fail += 2 * (dam_bonus * dam_bonus);
		}

		else					/* things get a little harder */
			fail += 25;
	}

	/* apply other modifiers */
	fail -= 3 * modified_level / 2;

	if (IS_OBJ_STAT(obj, ITEM_BLESS))
		fail -= 15;

	if (IS_OBJ_STAT(obj, ITEM_GLOW))
		fail -= 5;

	fail = URANGE(5, fail, 95);

	result = number_percent();

	/* the moment of truth */
	if (result < (fail / 5) && !IS_IMMORTAL(ch))	/* item destroyed */
	{
		act("$p shivers violently and explodes!", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$p shivers violently and explodes!", ch, obj, NULL, TO_ROOM, POS_RESTING);
		dam = dice(ch->level, 6);
		damage(ch, ch, dam, sn, DAM_SLASH, TRUE, TYPE_MAGIC);

		extract_obj(obj);
		return;
	}

	// Give them their wish sometimes...
	if(number_percent() < 35) {
		Cprintf(ch, "Your intense concentration is rewarded...\n\r");

		if(!str_prefix(arg3, "flaming"))
			dam = 1;
		else if(!str_prefix(arg3, "frost"))
			dam = 2;
		else if(!str_prefix(arg3, "vampiric"))
			dam = 3;
		else if(!str_prefix(arg3, "dull"))
			dam = 4;
		else if(!str_prefix(arg3, "shocking"))
			dam = 5;
		else if(!str_prefix(arg3, "vorpal"))
			dam = 6;
		else if(!str_prefix(arg3, "corrosive"))
			dam = 7;
		else if(!str_prefix(arg3, "flooding"))
			dam = 8;
		else if(!str_prefix(arg3, "poison"))
			dam = 9;
		else if(!str_prefix(arg3, "infected"))
			dam = 10;
		else if(!str_prefix(arg3, "soul drain"))
			dam = 11;
		else if(!str_prefix(arg3, "sharp"))
			dam = 12;
		else {
			Cprintf(ch, "Or would have been, if you'd specified a flag.\n\r");
			dam = number_range(1, 12);
		}
	}
	else {
		Cprintf(ch, "You fail to control the weapon's mutation.\n\r");
		dam = number_range(1, 12);
	}

	if (dam == 1) {
		new_flag = WEAPON_FLAMING;
		Cprintf(ch, "Flames burst forth from the weapon.\n\r");
	}
	else if (dam == 2) {
		new_flag = WEAPON_FROST;
		Cprintf(ch, "Icicles develop on the weapon.\n\r");
	}
	else if (dam == 3) {
		new_flag = WEAPON_VAMPIRIC;
		Cprintf(ch, "The weapon becomes dark and menacing.\n\r");
	}
	else if (dam == 4)
	{
		/* Have mercy on gargoyles */
		if (attack_table[obj->value[3]].damage == DAM_POISON)
		{
			Cprintf(ch, "Your mutate destroys the magic properties of the weapon!\n\r");
			obj->value[4] = 0;
			return;
		}
		new_flag = WEAPON_DULL;
		Cprintf(ch, "The weapon's edge dulls.\n\r");
	}
	else if (dam == 5) {
		new_flag = WEAPON_SHOCKING;
		Cprintf(ch, "The weapon sparks and crackles.\n\r");
	}
	else if (dam == 6) {
		new_flag = WEAPON_VORPAL;
		Cprintf(ch, "The weapons edge hones to a keen edge.\n\r");
	}
	else if (dam == 7) {
		new_flag = WEAPON_CORROSIVE;
		Cprintf(ch, "Corrosive fumes rise from the weapon.\n\r");
	}
	else if (dam == 8) {
		new_flag = WEAPON_FLOODING;
		Cprintf(ch, "The weapon becomes heavy and wet.\n\r");
	}
	else if (dam == 9) {
		new_flag = WEAPON_POISON;
		Cprintf(ch, "The weapon emits toxic vapors.\n\r");
	}
	else if (dam == 10) {
		new_flag = WEAPON_INFECTED;
		Cprintf(ch, "The weapon gains a stench of sewers.\n\r");
	}
	else if (dam == 11) {
		new_flag = WEAPON_SOULDRAIN;
		Cprintf(ch, "The weapon begins to howl out for blood...\n\r");
	}
	else if (dam == 12)
	{
		/* hold on, no sharp, no elemental damage */
		if (!str_cmp(obj->material, "iron") ||
			attack_table[obj->value[3]].damage == DAM_FIRE ||
			attack_table[obj->value[3]].damage == DAM_COLD ||
			attack_table[obj->value[3]].damage == DAM_ACID ||
			attack_table[obj->value[3]].damage == DAM_LIGHTNING ||
			attack_table[obj->value[3]].damage == DAM_DROWNING ||
			attack_table[obj->value[3]].damage == DAM_POISON)
		{
			Cprintf(ch, "Your mutate destroys the magical properties of the weapon!\n\r");
			obj->value[4] = 0;
			return;
		}
		else
		{
			new_flag = WEAPON_SHARP;
			Cprintf(ch, "The weapons edge hones to a bloodthirsty sharpness.\n\r");
		}
	}

	act("$p mutates into a new lethal instrument!", ch, obj, NULL, TO_CHAR, POS_RESTING);
	act("$p mutates into a new lethal instrument!", ch, obj, NULL, TO_ROOM, POS_RESTING);
	obj->value[4] = new_flag;

	return;
}

/* helper function for use in mutate */
int
get_random_bit(unsigned int bits)
{
	unsigned int valid_bits[32];
	int index;
	int count;
	unsigned int cur_bit;

	count = 0;
	for (cur_bit = 1; cur_bit <= 0xFFFF; cur_bit *= 2)
	{
		if (IS_SET(bits, cur_bit))
		{
			valid_bits[count] = cur_bit;
			count++;
		}
	}

	if (count == 0)
		return 0;

	index = number_range(0, count - 1);

	return valid_bits[index];
}

void
spell_null(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	Cprintf(ch, "That's not a spell!\n\r");
	return;
}

void
spell_pass_door(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, gsn_pass_door))
	{
		Cprintf(ch, "You refresh the passdoor.\n\r");
		affect_refresh(victim, sn, modified_level / 4);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 4;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_char(victim, &af);
	act("$n turns translucent.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(victim, "You turn translucent.\n\r");
	return;
}

void
spell_phantasm_monster(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	AFFECT_DATA af2;
	int mobcount;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	mobcount = 0;
	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->master == ch && IS_NPC(victim))
		{
			mobcount++;
			if (mobcount > 2)
			{
				Cprintf(ch, "Monsters do not like the presence of so many charmies.\n\r");
				return;
			}
		}
	}

	victim = create_mobile(get_mob_index(MOB_VNUM_PHANTASM));
	Cprintf(ch, "You call forth phantasmic forces!\n\r");
	char_to_room(victim, ch->in_room);
	size_mob(ch, victim, modified_level * 3 / 4);
	SET_BIT(victim->toggles, TOGGLES_NOEXP);
	if (victim->master)
		stop_follower(victim);
	add_follower(victim, ch);

	victim->leader = ch;
	af2.where = TO_AFFECTS;
	af2.type = sn;
	af2.level = modified_level;
	af2.duration = modified_level / 2;
	af2.location = 0;
	af2.modifier = 0;
	af2.bitvector = AFF_CHARM;
	affect_to_char(victim, &af2);

	act("Isn't $n just so nice?", ch, NULL, victim, TO_VICT, POS_RESTING);
}

void
spell_phantom_force(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	damage(ch, victim, dam, sn, DAM_COLD, TRUE, TYPE_MAGIC);
	shock_effect(victim, modified_level, dam, target);
	return;
}

/* RT plague spell, very nasty */

void
spell_plague(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;
	int dam;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_CON, DAM_DISEASE) ||
		(IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)))
	{
		if (ch == victim)
			Cprintf(ch, "You feel momentarily ill, but it passes.\n\r");
		else
			act("$N seems to be unaffected.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 1 + (modified_level / 6);
	if (IS_NPC(victim))
		af.duration *= 2;
	af.location = APPLY_STR;
	af.modifier = 0 - (modified_level / 10);
	af.bitvector = AFF_PLAGUE;
	affect_join(victim, &af);

	Cprintf(victim, "You scream in agony as plague sores erupt from your skin.\n\r");
	act("$n screams in agony as plague sores erupt from $s skin.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	/* plague deals 2-5% damage now */
	dam = 1 + (victim->hit / number_range(20, 50));
	victim->mana -= dam / 2;
	victim->move -= dam / 2;
	damage(ch, victim, dam, gsn_plague, DAM_DISEASE, FALSE, TYPE_MAGIC);

}

void
spell_poison(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;

		if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
		{
			if (IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
			{
				act("Your spell fails to corrupt $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				return;
			}
			obj->value[3] = 1;
			act("$p is infused with poisonous vapors.", ch, obj, NULL, TO_ALL, POS_RESTING);
			return;
		}

		if (obj->item_type == ITEM_WEAPON)
		{
			if (IS_WEAPON_STAT(obj, WEAPON_POISON))
			{
				act("$p is already envenomed.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				return;
			}

			if(obj->value[4] == WEAPON_FLAMING ||
                        obj->value[4] == WEAPON_FROST ||
                        obj->value[4] == WEAPON_VAMPIRIC ||
                        obj->value[4] == WEAPON_SHARP ||
                        obj->value[4] == WEAPON_VORPAL ||
                        obj->value[4] == WEAPON_TWO_HANDS ||
                        obj->value[4] == WEAPON_SHOCKING ||
                        obj->value[4] == WEAPON_SHARP ||
                        obj->value[4] == WEAPON_POISON ||
                        obj->value[4] == WEAPON_DRAGON_SLAYER ||
                        obj->value[4] == WEAPON_DULL ||
                        obj->value[4] == WEAPON_BLUNT ||
                        obj->value[4] == WEAPON_CORROSIVE ||
                        obj->value[4] == WEAPON_FLOODING ||
                        obj->value[4] == WEAPON_INFECTED ||
                        obj->value[4] == 0) {
			       			af.where = TO_WEAPON;
                        	af.type = sn;
                        	af.level = obj->level;
                        	af.duration = number_range(2, 4);
                        	af.location = 0;
                        	af.modifier = 0;
                        	af.bitvector = WEAPON_POISON;
                        	affect_to_obj(obj, &af);

                        	act("$p is coated with deadly venom.", ch, obj, NULL, TO_ALL, POS_RESTING);
                        	return;
			}
			else {
				Cprintf(ch, "This weapon is too strongly enchanted for you to poison.\n\r");
				return;
			}
		}

		act("You can't poison $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		return;
	}

	victim = (CHAR_DATA *) vo;
	if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_CON, DAM_POISON))
	{
		act("$n turns slightly green, but it passes.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(victim, "You feel momentarily ill, but it passes.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 1 + (modified_level / 6);
	if (IS_NPC(victim))
		af.duration *= 2;
	af.location = APPLY_STR;
	af.modifier = 0 - number_range(1, 2);
	af.bitvector = AFF_POISON;
	affect_join(victim, &af);

	Cprintf(victim, "You feel very sick.\n\r");
	act("$n looks very ill.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	// Poison is 3-6% damage
	damage(ch, victim, 1 + (victim->hit / number_range(15, 30)), gsn_poison, DAM_POISON, FALSE, TYPE_MAGIC);
}

void
spell_primal_rage(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		Cprintf(ch, "You refresh your rage.\n\r");
		affect_refresh(victim, sn, modified_level / 4);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 4;
	af.modifier = modified_level / 10;
	af.location = APPLY_HITROLL;
	af.bitvector = 0;

	affect_to_char(victim, &af);

	af.location = APPLY_DAMROLL;
	affect_to_char(victim, &af);

	Cprintf(victim, "You feel yourself getting enraged!\n\r");
	if (ch != victim)
		act("$N is getting enraged!", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_prismatic_spray(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = 10 + spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, FALSE);

	if (check_immune(victim, DAM_COLD) == IS_VULNERABLE)
	{
		Cprintf(ch, "Icy winds stream from your hands!\n\r");
		Cprintf(victim, "Icy winds rip into you!\n\r");
		damage(ch, victim, dam, sn, DAM_COLD, TRUE, TYPE_MAGIC);
		return;
	}

	if (check_immune(victim, DAM_FIRE) == IS_VULNERABLE)
	{
		Cprintf(ch, "Roaring flames stream from your hands!\n\r");
		Cprintf(victim, "A roaring flame engulfs you!\n\r");
		damage(ch, victim, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);
		return;
	}

	if (check_immune(victim, DAM_DROWNING) == IS_VULNERABLE)
	{
		Cprintf(ch, "A jet stream of water flies from your hands!\n\r");
		Cprintf(victim, "A jet stream of water surrounds you!\n\r");
		damage(ch, victim, dam, sn, DAM_DROWNING, TRUE, TYPE_MAGIC);
		return;
	}

	if (check_immune(victim, DAM_ACID) == IS_VULNERABLE)
	{
		Cprintf(ch, "Potent acids squirt from your hands!\n\r");
		Cprintf(victim, "Potent acids dissolve your flesh!\n\r");
		damage(ch, victim, dam, sn, DAM_ACID, TRUE, TYPE_MAGIC);
		return;
	}

	if (check_immune(victim, DAM_LIGHTNING) == IS_VULNERABLE)
	{
		Cprintf(ch, "White lightning bolts fly from your hands!\n\r");
		Cprintf(victim, "White lightning bolts fly toward you!\n\r");
		damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);
		return;
	}

	if (check_immune(victim, DAM_POISON) == IS_VULNERABLE)
	{
		Cprintf(ch, "Corrosive venom flies from your hands!\n\r");
		Cprintf(victim, "Corrosive venom penetrates your body!\n\r");
		damage(ch, victim, dam, sn, DAM_POISON, TRUE, TYPE_MAGIC);
		return;
	}

	Cprintf(ch, "A blast of force flies from your hands!\n\r");
	Cprintf(victim, "A blast of force crushes you!\n\r");
	damage(ch, victim, dam, sn, DAM_BASH, TRUE, TYPE_MAGIC);

	return;

}

void
spell_protection_evil(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, gsn_protection_evil))
        {
                Cprintf(ch, "You refresh the protection from evil.\n\r");
                affect_refresh(victim, sn, 24);
                return;
        }
	if (is_affected(victim, gsn_protection_good)
	|| is_affected(victim, gsn_protection_neutral)
	|| IS_AFFECTED(victim, AFF_PROTECT_EVIL)
	|| IS_AFFECTED(victim, AFF_PROTECT_GOOD)
	|| IS_AFFECTED(victim, AFF_PROTECT_NEUTRAL))
	{
		if (victim == ch)
			Cprintf(ch, "You are already protected.\n\r");
		else
			act("$N is already protected.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 24;
	af.location = APPLY_SAVING_SPELL;
	af.modifier = -1;
	af.bitvector = AFF_PROTECT_EVIL;
	affect_to_char(victim, &af);
	Cprintf(victim, "You feel holy and pure.\n\r");
	if (ch != victim)
		act("$N is protected from evil.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_protection_good(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, gsn_protection_good))
	{
		Cprintf(ch, "You refresh the protection from good.\n\r");
		affect_refresh(victim, sn, 24);
		return;
	}
	if (is_affected(victim, gsn_protection_evil)
        || is_affected(victim, gsn_protection_neutral)
	|| IS_AFFECTED(victim, AFF_PROTECT_EVIL)
	|| IS_AFFECTED(victim, AFF_PROTECT_GOOD)
	|| IS_AFFECTED(victim, AFF_PROTECT_NEUTRAL))
	{
		if (victim == ch)
			Cprintf(ch, "You are already protected.\n\r");
		else
			act("$N is already protected.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 24;
	af.location = APPLY_SAVING_SPELL;
	af.modifier = -1;
	af.bitvector = AFF_PROTECT_GOOD;
	affect_to_char(victim, &af);
	Cprintf(victim, "You feel aligned with darkness.\n\r");
	if (ch != victim)
		act("$N is protected from good.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_protection_neutral(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;
        int caster_level, modified_level;

        caster_level = get_caster_level(level);
        modified_level = get_modified_level(level);

        if (is_affected(victim, gsn_protection_neutral))
        {
                Cprintf(ch, "You refresh the protection from neutral.\n\r");
                affect_refresh(victim, sn, 24);
                return;
        }
	if (is_affected(victim, gsn_protection_evil)
	|| is_affected(victim, gsn_protection_good)
	|| IS_AFFECTED(victim, AFF_PROTECT_EVIL)
	|| IS_AFFECTED(victim, AFF_PROTECT_GOOD)
	|| IS_AFFECTED(victim, AFF_PROTECT_NEUTRAL))
        {
                if (victim == ch)
                        Cprintf(ch, "You are already protected.\n\r");
                else
                        act("$N is already protected.", ch, NULL, victim, TO_CHAR, POS_RESTING);
                return;
        }

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 24;
        af.location = APPLY_SAVING_SPELL;
        af.modifier = -1;
        af.bitvector = AFF_PROTECT_NEUTRAL;
        affect_to_char(victim, &af);
        Cprintf(victim, "You feel aligned with twilight.\n\r");
        if (ch != victim)
                act("$N is protected from neutral.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        return;
}

void
spell_pyrotechnics(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	damage(ch, victim, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);
	return;
}

void
spell_rainbow_burst(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{

	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int elenum;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	elenum = dice(1, 5);

	Cprintf(victim, "The power of the rainbow falls upon you!\n\r");
	Cprintf(ch, "You call forth the power of the rainbow!\n\r");

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_HIGH, TRUE);

	if (elenum == 1)
	{
		Cprintf(ch, "Red radiates from the rainbow!.\n\r");
		damage(ch, victim, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);
	}

	if (elenum == 2)
	{
		Cprintf(ch, "Green radiates from the rainbow!.\n\r");
		damage(ch, victim, dam, sn, DAM_ACID, TRUE, TYPE_MAGIC);
	}

	if (elenum == 3)
	{
		Cprintf(ch, "Black radiates from the rainbow!.\n\r");
		damage(ch, victim, dam, sn, DAM_POISON, TRUE, TYPE_MAGIC);
	}

	if (elenum == 4)
	{
		Cprintf(ch, "Blue radiates from the rainbow!.\n\r");
		damage(ch, victim, dam, sn, DAM_COLD, TRUE, TYPE_MAGIC);
	}

	if (elenum == 5)
	{
		Cprintf(ch, "White radiates from the rainbow!.\n\r");
		damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);
	}
	return;
}

void
spell_ray_of_truth(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;
	int saved = FALSE;
	AFFECT_DATA af;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	ch->alignment = UMIN(1000, ch->alignment + 50);

	saved = saving_throw(ch, victim, sn, modified_level, SAVE_NORMAL, STAT_WIS, DAM_HOLY);

	if (IS_EVIL(ch))
	{
		victim = ch;
		saved = FALSE;
		Cprintf(ch, "The energy explodes inside you!\n\r");
	}

	if (victim != ch)
	{
		act("$n raises $s hand, and a blinding ray of light shoots forth!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You raise your hand and a blinding ray of light shoots forth!\n\r");
	}

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	if(saved)
		dam = dam / 2;

	damage(ch, victim, dam, sn, DAM_HOLY, TRUE, TYPE_MAGIC);

	if(!IS_AFFECTED(victim, AFF_BLIND) && !saved) {
		af.where = TO_AFFECTS;
        	af.type = gsn_blindness;
        	af.level = modified_level;
        	af.location = APPLY_HITROLL;
        	af.modifier = -4;
        	af.duration = 1 + (modified_level / 12);
		if (IS_NPC(victim))
			af.duration *= 2;
        	af.bitvector = AFF_BLIND;

        	affect_to_char(victim, &af);
        	Cprintf(victim, "You are blinded!\n\r");
        	act("$n appears to be blinded.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	return;
}

void
spell_reach_elemental(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = NULL;
	CHAR_DATA *victim = NULL;
	AFFECT_DATA af2;
	int elenum;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room\n\r");
		return;
	}

	if (max_no_charmies(ch))
	{
		Cprintf(ch, "You can't reach more elementals.\n\r");
		return;
	}

	elenum = dice(1, 4);

	if (elenum == 1)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_ELEMENTAL_A));
		obj = create_object(get_obj_index(OBJ_VNUM_ELEMENTAL_A), victim->level);
	}
	if (elenum == 2)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_ELEMENTAL_B));
		obj = create_object(get_obj_index(OBJ_VNUM_ELEMENTAL_B), victim->level);
	}
	if (elenum == 3)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_ELEMENTAL_C));
		obj = create_object(get_obj_index(OBJ_VNUM_ELEMENTAL_C), victim->level);
	}
	if (elenum == 4)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_ELEMENTAL_D));
		obj = create_object(get_obj_index(OBJ_VNUM_ELEMENTAL_D), victim->level);
	}
	size_mob(ch, victim, modified_level);
	size_obj(ch, obj, modified_level * 7 / 10);
	SET_BIT(victim->toggles, TOGGLES_NOEXP);

	obj_to_char(obj, victim);
	equip_char(victim, obj, WEAR_WIELD);

	Cprintf(ch, "You call forth elemental forces!\n\r");
	char_to_room(victim, ch->in_room);
	if (victim->master)
		stop_follower(victim);
	add_follower(victim, ch);
	victim->leader = ch;

	af2.where = TO_AFFECTS;
	af2.type = sn;
	af2.level = modified_level;
	af2.duration = modified_level / 2;
	af2.location = 0;
	af2.modifier = 0;
	af2.bitvector = AFF_CHARM;
	affect_to_char(victim, &af2);
	act("Isn't $n just so nice?", ch, NULL, victim, TO_VICT, POS_RESTING);
}

void replenish_charged_item(CHAR_DATA *ch, OBJ_DATA *obj, int modified_level)
{
	int percent;
	int chance;

	// Spell level matters
        if (obj->special[0] >= modified_level)
        {
                Cprintf(ch, "Your skills are not great enough for that.\n\r");
                return;
        }

        if (obj->special[1] == 0)
        {
                Cprintf(ch, "That item cannot be recharged any more.\n\r");
                return;
        }

        chance = 2 * modified_level;
        chance -= obj->special[0];

	percent = number_percent();

	if(ch->reclass != reclass_lookup("alchemist")
	&& percent < 5) {
		act("$p glows softly, but then fades!", ch, obj, NULL, TO_CHAR, POS_RESTING);
                act("$p glows softly, but then fades!", ch, obj, NULL, TO_ROOM, POS_RESTING);
		obj->special[1]--;
	}
	else if(percent < chance) {
		act("$p glows softly.", ch, obj, NULL, TO_CHAR, POS_RESTING);
                act("$p glows softly.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		obj->special[2]++;
	}
	else {
		Cprintf(ch, "Nothing happened.\n\r");
	}

}


void
spell_recharge(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance, percent;
	int caster_level, modified_level;

	// Okay, wands suck my ass:
	// v0 = level of spell casted by wands
	// v1 = maximum charge of wand
	// v2 = current charges left
	// v3 = spell sn

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);


	if(IS_SET(obj->wear_flags, ITEM_CHARGED)
        && obj->special[2] < obj->special[1]) {
                replenish_charged_item(ch, obj, modified_level);
                return;
        }

	if (obj->item_type != ITEM_WAND && obj->item_type != ITEM_STAFF)
	{
		Cprintf(ch, "That item does not carry charges.\n\r");
		return;
	}

	// Spell level matters
	if (obj->value[0] >= modified_level)
	{
		Cprintf(ch, "Your skills are not great enough for that.\n\r");
		return;
	}

	if (obj->value[1] <= 0)
	{
		Cprintf(ch, "That item cannot be recharged any more.\n\r");
		return;
	}

	if (obj->value[2] >= obj->value[1] / 2) {
		Cprintf(ch, "You may only recharge items after half the charges have been drained.\n\r");
		return;
	}

	chance = 40 + 2 * modified_level;
	chance -= obj->value[0];	/* harder to do high-level spells */
	chance -= (obj->value[1] - obj->value[2]) * 3;

	chance = UMAX(modified_level / 2, chance);

	percent = number_percent();

	if (percent < chance / 2)
	{
		act("$p glows softly.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$p glows softly.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		if(ch->reclass == reclass_lookup("alchemist"))
			obj->value[1]--;
		else
			obj->value[1] = obj->value[1] / 2;

		obj->value[2] = UMAX(obj->value[1], obj->value[2]);
		return;
	}
	else if (percent <= chance)
	{
		int chargeback, chargemax;

		act("$p glows softly.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$p glows softly.", ch, obj, NULL, TO_ROOM, POS_RESTING);

		if(ch->reclass == reclass_lookup("alchemist"))
        		obj->value[1]--;
		else
        		obj->value[1] = obj->value[1] / 2;

		chargemax = obj->value[1] - obj->value[2];

		if (chargemax > 0)
			chargeback = UMAX(1, chargemax * percent / 100);
		else
			chargeback = 0;

		obj->value[2] += chargeback;
		return;
	}
	else if (percent <= UMIN(95, 3 * chance / 2))
	{
		Cprintf(ch, "Nothing seems to happen.\n\r");
		if (obj->value[1] > 1)
			obj->value[1]--;
		return;
	}
	else
	{
		act("$p glows brightly and explodes!", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$p glows brightly and explodes!", ch, obj, NULL, TO_ROOM, POS_RESTING);
		extract_obj(obj);
	}
}

void
spell_refresh(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	victim->move = UMIN(victim->move + modified_level, MAX_MOVE(victim));
	if (MAX_MOVE(victim) == victim->move)
		Cprintf(victim, "You feel fully refreshed!\n\r");
	else
		Cprintf(victim, "You feel less tired.\n\r");
	if (ch != victim)
		Cprintf(ch, "Ok.\n\r");
	return;
}

void
spell_remove_curse(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	bool found = FALSE;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* do object cases first */
	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;

		if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
		{
			if (!IS_OBJ_STAT(obj, ITEM_NOUNCURSE)
				&& number_percent() < 50 + ((modified_level - obj->level) * 5))
			{
				REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
				REMOVE_BIT(obj->extra_flags, ITEM_NOREMOVE);
				act("$p glows blue.", ch, obj, NULL, TO_ALL, POS_RESTING);
				return;
			}

			act("The curse on $p is beyond your power.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			return;
		}
		act("There doesn't seem to be a curse on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		return;
	}

	/* characters */
	victim = (CHAR_DATA *) vo;

	if (try_cancel(victim, gsn_curse, modified_level))
	{
		Cprintf(victim, "You feel better.\n\r");
		affect_strip(victim, gsn_curse);
		act("$n looks more relaxed.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	for (obj = victim->carrying; (obj != NULL && !found); obj = obj->next_content)
	{
		if ((IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
			&& !IS_OBJ_STAT(obj, ITEM_NOUNCURSE))
		{						/* attempt to remove curse */
			if (number_percent() < 50 + ((modified_level - obj->level) * 5))
			{
				found = TRUE;
				REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
				REMOVE_BIT(obj->extra_flags, ITEM_NOREMOVE);
				act("Your $p glows blue.", victim, obj, NULL, TO_CHAR, POS_RESTING);
				act("$n's $p glows blue.", victim, obj, NULL, TO_ROOM, POS_RESTING);
				return;
			}
		}
	}
	Cprintf(ch, "This curse is beyond your powers.\n\r");
}

void
spell_robustness(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		Cprintf(ch, "You refresh your robustness.\n\r");
		affect_refresh(victim, sn, modified_level / 5);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = caster_level;
	af.duration = modified_level / 5;
	af.modifier = modified_level * 3;
	af.location = APPLY_HIT;
	af.bitvector = 0;
	affect_to_char(victim, &af);

	af.modifier = modified_level;
	af.location = APPLY_AC;
	affect_to_char(victim, &af);

	if (victim->fighting == NULL)
	{
		if(!is_affected(victim, gsn_dissolution))
			victim->hit = victim->hit + modified_level * 3;
	}

	Cprintf(victim, "You feel someone beefing you up.\n\r");
	return;
}

void
spell_rukus_magna(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);
	dam = dam * 9 / 10;
	damage(ch, victim, dam, sn, DAM_SOUND, TRUE, TYPE_MAGIC);
	return;
}

void
spell_sanctuary(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_AFFECTED(victim, AFF_SANCTUARY))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh sanctuary.\n\r");
		else
		{
			act("$N refreshed your sanctuary.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the sanctuary spell.\n\r");
		}
		affect_refresh(victim, sn, modified_level / 6);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 6;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_SANCTUARY;
	affect_to_char(victim, &af);
	act("$n is surrounded by a white aura.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(victim, "You are surrounded by a white aura.\n\r");
	return;
}

void
spell_scalemail(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		Cprintf(ch, "You refresh your scalemail.\n\r");
		affect_refresh(victim, sn, modified_level / 2);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 2;
	af.modifier = -20 + (-10 * (ch->level > 25)) + (-10 * (ch->level > 40));
	af.location = APPLY_AC;
	af.bitvector = 0;

	affect_to_char(victim, &af);

	Cprintf(victim, "Your scales reinforce into a tight mail.\n\r");
	if (ch != victim)
		act("$N's mail reinforce into a tight mail.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_scramble(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You already can't understand anything.\n\r");
		else
			act("$N already can't understand a thing.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_WIS, DAM_CHARM))
	{
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 5;
	if (IS_NPC(victim))
		af.duration *= 2;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = 0;
	affect_to_char(victim, &af);
	Cprintf(victim, "Your mother tongue now eludes you.\n\r");

	if (ch != victim)
		act("$N can no longer understand anything.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_shield(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You refresh the shielding.\n\r");
		else
		{
			act("$N refreshed your shielding.", victim, NULL, ch, TO_CHAR, POS_RESTING);
			Cprintf(ch, "You refresh the shield spell.\n\r");
		}
		affect_refresh(victim, sn, modified_level + 8);
		return;
	}
	if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 50)
		Cprintf(ch, "!!SOUND(sounds/wav/shield.wav V=80 P=20 T=admin)");

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 8 + modified_level;
	af.location = APPLY_AC;
	af.modifier = -20;
	af.bitvector = 0;
	affect_to_char(victim, &af);
	act("$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(victim, "You are surrounded by a force shield.\n\r");
	return;
}

void
spell_shifting_sands(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn))
	{
		if (victim == ch)
			Cprintf(ch, "You are as shifted as you can be.\n\r");
		else
			act("$N is already on shifting ground.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_STR, DAM_BASH))
	{
		return;
	}

	if(IS_AFFECTED(victim, AFF_FLYING)) {
		Cprintf(ch, "The shifting sands pull %s to ground!\n\r", PERS(victim, ch));
		if (ch != victim)
			Cprintf(victim, "You feel the shifting sands pull you to the ground.\n\r");
		affect_strip(victim, gsn_fly);
		REMOVE_BIT(victim->affected_by, AFF_FLYING);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 10;
	if (IS_NPC(victim))
		af.duration *= 2;
	af.modifier = -1 * modified_level / 10;
	af.location = APPLY_HITROLL;
	af.bitvector = 0;

	affect_to_char(victim, &af);
	af.location = APPLY_DAMROLL;
	affect_to_char(victim, &af);

	Cprintf(victim, "You feel the ground shifting under your feet.\n\r");
	if (ch != victim)
		act("The ground is shifting under $N's feet. ", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_shriek(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj_aff, *obj_next, *t_obj, *n_obj;
	int dam;
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	Cprintf(ch, "You pierce the air with a resounding shriek!\n\r");
	act("$n shatters the silence with a resounding shriek!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	dam = spell_damage(ch, victim, modified_level, SPELL_DAMAGE_MEDIUM, TRUE);

	if(!saving_throw(ch, victim, gsn_shriek, caster_level + 1, SAVE_HARD, STAT_WIS, DAM_SOUND))
	{
	/* we now check if anything starts humming or breaking */
	for (obj_aff = victim->carrying; obj_aff != NULL; obj_aff = obj_next)
	{
		obj_next = obj_aff->next_content;
		if(!IS_OBJ_STAT(obj_aff, ITEM_SOUND_PROOF))
		{
			if (!IS_OBJ_STAT(obj_aff, ITEM_HUM))
			{
				if (number_percent() <= 5)
				{
					act("You feel $p start to vibrate.", obj_aff->carried_by, obj_aff, NULL, TO_CHAR, POS_RESTING);
					act("$p begins to vibrate uncontrollably.", obj_aff->carried_by, obj_aff, NULL, TO_ROOM, POS_RESTING);
					SET_BIT(obj_aff->extra_flags, ITEM_HUM);
				}
			}
			else
			{
				if (number_percent() <= 3)
				{
					for (t_obj = obj_aff->contains; t_obj != NULL; t_obj = n_obj)
					{
						n_obj = t_obj->next_content;
						obj_from_obj(t_obj);
						if (obj_aff->in_room != NULL)
							obj_to_room(t_obj, obj_aff->in_room);
						else if (obj_aff->carried_by != NULL)
							obj_to_room(t_obj, obj_aff->carried_by->in_room);
						else
						{
							extract_obj(t_obj);
							continue;
						}
					}
					act("$p vibrates and explodes!", obj_aff->carried_by, obj_aff, NULL, TO_ALL, POS_RESTING);
					extract_obj(obj_aff);
				}
			}
		}
	}
	}
	damage(ch, victim, dam, sn, DAM_SOUND, TRUE, TYPE_MAGIC);
	return;

}

void
spell_shocking_grasp(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;
	int dam;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(modified_level <= 20)
                dam = 5 + spell_damage(ch, victim, modified_level, SPELL_DAMAGE_CHART_GOOD, TRUE);
        else
                dam = 25 + spell_damage(ch, victim, modified_level, SPELL_DAMAGE_CHART_POOR, TRUE);

	damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);
	return;
}

void
spell_sleep(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_AFFECTED(victim, AFF_SLEEP)
	|| (modified_level + 3) < victim->level)
		return;

	if (saving_throw(ch, victim, sn, modified_level, SAVE_NORMAL, STAT_WIS, DAM_CHARM))
	{
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 1 + modified_level / 10;
	if (IS_NPC(victim))
		af.duration *= 2;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_SLEEP;
	affect_to_char(victim, &af);

	if (IS_AWAKE(victim))
	{
		Cprintf(victim, "You feel very sleepy ..... zzzzzz.\n\r");
		act("$n goes to sleep.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		victim->position = POS_SLEEPING;
		if (IS_AFFECTED(victim, AFF_HIDE))
			REMOVE_BIT(victim->affected_by, AFF_HIDE);
	}
	return;
}

void end_sleep(void *vo, int target) {
	CHAR_DATA *ch = (CHAR_DATA*)vo;

	if(!IS_NPC(ch))
		return;

	ch->position = POS_STANDING;
	act("$n wakes and stands up.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
spell_slow(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn) || IS_AFFECTED(victim, AFF_SLOW))
	{
		if (victim == ch)
			Cprintf(ch, "You can't move any slower!\n\r");
		else
			act("$N can't get any slower than that.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (ch != victim)
	{
		if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_DEX, DAM_NONE))
		{
			if (victim != ch)
			{
				Cprintf(ch, "Nothing seemed to happen.\n\r");
			}
			Cprintf(victim, "You feel momentarily lethargic.\n\r");
			return;
		}
	}

	if (IS_AFFECTED(victim, AFF_HASTE))
	{
		if (!try_cancel(victim, gsn_haste, modified_level))
		{
			if (victim != ch)
				Cprintf(ch, "Spell failed.\n\r");
			Cprintf(victim, "You feel momentarily slower.\n\r");
			return;
		}
		affect_strip(victim, gsn_haste);
		Cprintf(victim, "You feel yourself slow down.\n\r");
		act("$n is moving less quickly.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}


	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 1 + (modified_level / 6);
	if (IS_NPC(victim))
		af.duration *= 2;
	af.location = APPLY_DEX;
	af.modifier = -1 - (modified_level >= 18) - (modified_level >= 25) - (modified_level >= 32);
	af.bitvector = AFF_SLOW;
	affect_to_char(victim, &af);
	Cprintf(victim, "You feel yourself slowing d o w n...\n\r");
	act("$n starts to move in slow motion.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	return;
}

void
spell_soundproof(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA af, *paf;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

    paf = affect_find(obj->affected, gsn_soundproof);
    if (paf == NULL && IS_OBJ_STAT(obj, ITEM_SOUND_PROOF))
    {
            act("$p is already protected from sound waves.", ch, obj, NULL, TO_CHAR, POS_RESTING);
            return;
    }
    else if (paf != NULL)
    {
            affect_remove_obj(obj, paf);
    }

	af.where = TO_OBJECT;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = ITEM_SOUND_PROOF;

	affect_to_obj(obj, &af);

	act("You protect $p from sound waves.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	act("$n protects $p with a silencing aura.", ch, obj, NULL, TO_ROOM, POS_RESTING);
}

void
spell_spirit_link(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *to_room;
	bool gate_pet;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	/* this cancels out the normal +13 bonus */
	if (ch->reclass == reclass_lookup("shaman"))
		modified_level -= 10;

	if ((victim = get_char_world(ch, target_name, FALSE)) == NULL
		|| victim == ch
		|| victim->in_room == NULL
		|| !can_see_room(ch, victim->in_room)
		|| IS_SET(victim->in_room->room_flags, ROOM_SAFE)
		|| IS_SET(victim->in_room->room_flags, ROOM_LAW)
		|| IS_SET(victim->in_room->room_flags, ROOM_NO_GATE)
		|| victim->level >= modified_level + 13
		|| (is_clan(victim) && !is_same_clan(ch, victim))
		|| (IS_NPC(victim) && IS_AFFECTED(victim, AFF_CHARM) && victim->master != ch)
		|| (IS_NPC(victim)
		&& number_percent() < 20))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (victim->in_room->area->continent != ch->in_room->area->continent)
	{
		Cprintf(ch, "Inter-continental magic is forbidden.  You failed.\n\r");
		return;
	}

	if (victim->in_room->area->security < 9)
	{
		Cprintf(ch, "Not in unfinished areas.  Sorry.\n\r");
		return;
	}

	if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
		gate_pet = TRUE;
	else
		gate_pet = FALSE;

	act("$n reaches a spirit and dematerializes.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You reach a spirit and dematerialize.\n\r");
	in_room = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, victim->in_room);
	to_room = ch->in_room;

	act("$n reaches the spirits in this room and materializes.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	do_look(ch, "auto");

	if (gate_pet)
	{
		act("$n reaches a spirit and dematerializes.", ch->pet, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch->pet, "You reach a spirit and dematerialize.\n\r");
		char_from_room(ch->pet);
		char_to_room(ch->pet, victim->in_room);
		act("$n reaches the spirits in this room and materializes.", ch->pet, NULL, NULL, TO_ROOM, POS_RESTING);
		do_look(ch->pet, "auto");
	}


}

void
spell_stone_skin(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(ch, sn))
	{
		Cprintf(ch, "You refresh the stone skin.\n\r");
		affect_refresh(victim, sn, modified_level);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level;
	af.location = APPLY_AC;
	af.modifier = -40;
	af.bitvector = 0;
	affect_to_char(victim, &af);
	act("$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(victim, "Your skin turns to stone.\n\r");
	return;
}

void
spell_summon(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
    CHAR_DATA *victim;
    CHAR_DATA *rch;
    ROOM_INDEX_DATA *rand_room, *was_in_room;
    int mana;
    int caster_level, modified_level;

    caster_level = get_caster_level(level);
    modified_level = get_modified_level(level);

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE) || wearing_nogate_item(ch)) {
        Cprintf(ch, "Not in this room.\n\r");
        return;
    }

    if ((victim = get_char_world_finished_areas(ch, target_name, FALSE)) == NULL
            || victim == ch
            || victim->in_room == NULL
	    || ch->in_room->clan
            || IS_SET(ch->in_room->room_flags, ROOM_SAFE)
            || IS_SET(victim->in_room->room_flags, ROOM_SAFE)
            || IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
            || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
            || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
            || victim->level >= modified_level + 3
            || (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL)
            || victim->fighting != NULL
            || (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
            || (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
            || (IS_NPC(victim) && IS_AFFECTED(victim, AFF_CHARM))
            || (IS_NPC(victim)
                    && number_percent() < 20)) {
        Cprintf(ch, "You failed.\n\r");
        return;
    }

    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOSUMMON)) {
        Cprintf(ch, "Target is not summonable.\n\r");
        Cprintf(victim, "Someone attempted to summon you.\n\r");
        return;
    }

    if (victim->in_room->area->continent != ch->in_room->area->continent) {
        Cprintf(ch, "Inter-continental magic is forbidden.  You failed.\n\r");

        if (ch->level + 2 == skill_table[sn].skill_level[ch->charClass]) {
            mana = 25;
        } else {
            mana = UMAX(skill_table[sn].min_mana / 2, 50 / (2 + ch->level - skill_table[sn].skill_level[ch->charClass]));
        }

        ch->mana += mana;
        return;
    }

    if (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE)) {
        REMOVE_BIT(victim->act, ACT_AGGRESSIVE);
    }

    act("$n disappears suddenly.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
    was_in_room = victim->in_room;
    char_from_room(victim);
    char_to_room(victim, ch->in_room);
    act("$n arrives suddenly.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
    act("$n has summoned you!", ch, NULL, victim, TO_VICT, POS_RESTING);
    do_look(victim, "auto");

    if (IS_NPC(victim)) {
        victim->wander_timer = 1;
    }

    for (rch = was_in_room->people; rch != NULL; rch = rch->next_in_room) {
        if (is_same_group(victim, rch) && is_affected(rch, gsn_slide)) {
            if (number_percent() < get_skill(rch, gsn_slide)) {
                act("$n creates a small gate and follows.", rch, NULL, NULL, TO_ROOM, POS_RESTING);
                Cprintf(rch, "You create your own gate and follow.\n\r");
                /* 15% chance of being cut loose */
                if (number_percent() < 15) {
                    Cprintf(rch, "The world spins!\n\r");
                    rand_room = get_random_room(rch);

                    while ( IS_SET(rand_room->room_flags, ROOM_NO_GATE) ||
                            rand_room->area->continent != rch->in_room->area->continent
                            || rand_room->area->security < 9) {
                        rand_room = get_random_room(rch);
                    }

                    char_from_room(rch);
                    char_to_room(rch, rand_room);
                    act("A gate opens and $n slides into the room.", rch, NULL, NULL, TO_ROOM, POS_RESTING);
                    check_improve(rch, gsn_slide, FALSE, 2);
                    do_look(rch, "auto");
                } else {
                    char_from_room(rch);
                    char_to_room(rch, victim->in_room);
                    act("A gate opens and $n slides into the room.", rch, NULL, NULL, TO_ROOM, POS_RESTING);
                    check_improve(rch, gsn_slide, TRUE, 2);
                    do_look(rch, "auto");
                }
            }
        }
    }

    return;
}

void
spell_tame_animal(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	CHAR_DATA *vch, *rch;
	AFFECT_DATA af;
	int i, diff;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_safe(ch, victim))
		return;

	if (victim == ch)
	{
		Cprintf(ch, "You indeed are your own best friend.\n\r");
		return;
	}

	if (IS_AFFECTED(victim, AFF_CHARM)
	|| IS_AFFECTED(ch, AFF_CHARM)
	|| ch->level < victim->level - 5
	|| IS_SET(victim->imm_flags, IMM_CHARM)) {
		Cprintf(ch, "You cannot tame them.\n\r");
		return;
        }
	/* Would be nicer to check if victim is not sentient and not mammal
	   or something like that :P */

	if (IS_NPC(victim)
		&& victim->race != race_lookup("bat")
		&& victim->race != race_lookup("bear")
		&& victim->race != race_lookup("cat")
		&& victim->race != race_lookup("dog")
		&& victim->race != race_lookup("fido")
		&& victim->race != race_lookup("fox")
		&& victim->race != race_lookup("lizard")
		&& victim->race != race_lookup("pig")
		&& victim->race != race_lookup("rabbit")
		&& victim->race != race_lookup("snake")
		&& victim->race != race_lookup("song bird")
		&& victim->race != race_lookup("water fowl")
		&& victim->race != race_lookup("wolf"))
	{
		Cprintf(ch, "Try this spell on an animal!\n\r");
		return;
	}

	if (!IS_NPC(victim) && !is_affected(victim, gsn_animal_growth))
	{
		Cprintf(ch, "Try this spell on someone who has animal growth!\n\r");
		return;
	}

	if(!IS_NPC(victim) && ch->reclass == reclass_lookup("hermit"))
	{
		for (rch = char_list; rch != NULL; rch = rch->next)
        	{
        		if (is_same_group(ch, rch)
                	&& !IS_NPC(rch)
                	&& rch != ch) {
				Cprintf(ch, "Hermits prefer to fight alone, thanks.");
                		return;
			}
        	}
	}

	if(IS_NPC(victim))
		diff = SAVE_HARD;
	else
		diff = SAVE_NORMAL;

	if (saving_throw(ch, victim, sn, caster_level + 3, diff, STAT_STR, DAM_CHARM))
	{
		Cprintf(ch, "Your target has resisted your attempts at taming them.\n\r");
		return;
	}

	i = 0;
	vch = victim;
	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->master == ch
		&& IS_NPC(victim)
		&& is_affected(victim, gsn_tame_animal))
			i++;
	}

	if (i > 6)
	{
		Cprintf(ch, "You cannot control that many animals at once.\n\r");
		return;
	}

	victim = vch;

	if (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
		REMOVE_BIT(victim->act, ACT_AGGRESSIVE);

	if (victim->master)
		stop_follower(victim);
	add_follower(victim, ch);
	victim->leader = ch;

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 4;
	if (IS_NPC(victim))
		af.duration *= 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(victim, &af);
	if (ch != victim)
		act("$N will now follow and serve you.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	return;
}

void
spell_taunt(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (ch->charClass == class_lookup("thief"))
	{
		modified_level = ch->level;
	}

	if (is_affected(victim, gsn_taunt) )
	{
		if(ch == victim)
			Cprintf(ch, "You refresh your taunt.\n\r");
		else {
			Cprintf(ch, "You refresh their taunting.\n\r");
			Cprintf(victim, "Your taunt is refreshed.\n\r");
		}
		affect_refresh(victim, sn, 4);
		return;
	}

	if (is_affected(victim, gsn_calm))
	{
		if (victim == ch)
			Cprintf(ch, "Why don't you just relax for a while?\n\r");
		else
			act("$N doesn't look like $e wants to fight anymore.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (is_affected(victim, gsn_frenzy)
	|| IS_AFFECTED(victim, AFF_BERSERK)
	|| is_affected(victim, gsn_berserk))
	{
		if (victim == ch)
		{
			Cprintf(ch, "You are already enraged!\n\r");
			return;
		}
	}
	if (is_affected(victim, gsn_guardian)) {
		if(victim == ch)
		{
			Cprintf(ch, "You're too busy guarding to be taunted.\n\r");
			return;
		}
	}

	if (victim != ch
	&& saving_throw(ch, victim, sn, modified_level, SAVE_NORMAL, STAT_WIS, DAM_CHARM))
	{
		Cprintf(victim, "You try to ignore the demon's vile taunting!\n\r");
		return;
	}

	if (victim == ch)
	{
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = modified_level;
		af.duration = 4;
		af.modifier = 7;
		af.bitvector = AFF_TAUNT;
		af.location = APPLY_HITROLL;
		affect_to_char(victim, &af);
		af.location = APPLY_DAMROLL;
		affect_to_char(victim, &af);
	}
	else
	{
		af.where = TO_AFFECTS;
		af.type = sn;
                af.level = modified_level;
		af.duration = 4;
		af.modifier = 0;
		af.location = APPLY_DAMROLL;
		af.bitvector = AFF_TAUNT;
                affect_to_char(victim, &af);
	}

	Cprintf(victim, "You radiate from being taunted by the demons!\n\r");
	act("$n is taunted by the demons!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
spell_teleport(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	ROOM_INDEX_DATA *pRoomIndex;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(number_percent() < 20) {
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (victim->in_room == NULL
		|| IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
		|| (victim != ch && IS_SET(victim->imm_flags, IMM_SUMMON))
		|| (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
		|| (!IS_NPC(ch) && victim->fighting != NULL)
		|| (victim != ch
		&& saving_throw(ch, victim, sn, caster_level, SAVE_HARD, STAT_WIS, DAM_NONE)))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (!IS_NPC(ch) && is_clan(victim) && !is_clan(ch))
	{
		Cprintf(ch, "Stay out of clan business.\n\r");
		return;
	}

	pRoomIndex = get_random_room(victim);
	while (IS_SET(pRoomIndex->room_flags, ROOM_NO_GATE)
		   || pRoomIndex->area->continent != ch->in_room->area->continent)
	{
		pRoomIndex = get_random_room(victim);
	}

	if (pRoomIndex->area->security < 9)
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	if (victim != ch)
		Cprintf(victim, "You have been teleported!\n\r");

	act("$n vanishes!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	char_from_room(victim);
	char_to_room(victim, pRoomIndex);
	act("$n slowly fades into existence.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	do_look(victim, "auto");
	return;
}

void
spell_thirst(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	if (IS_NPC(victim)) {
		Cprintf(ch, "Mobs don't get thirsty. Nice try.\n\r");
		return;
	}

	if (victim->pcdata->condition[COND_FULL] < 1)
	{
		Cprintf(ch, "You can't make em more thirsty.\n\r");
		return;
	}

	Cprintf(ch, "You feel more thirsty!\n\r");
	Cprintf(ch, "May your drought be upon em.\n\r");
	victim->pcdata->condition[COND_FULL] = victim->pcdata->condition[COND_FULL] - 5;
	if (victim->pcdata->condition[COND_FULL] < 0)
		victim->pcdata->condition[COND_FULL] = 0;
}

void
spell_turn_magic(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	CHAR_DATA *vch;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (!IS_NPC(victim))
	{
		Cprintf(ch, "You can only cast turn magic on mobs, not players.\n\r");
		return;
	}

	if (victim->master == NULL
	|| !IS_AFFECTED(victim, AFF_CHARM))
	{
		Cprintf(ch, "You can only steal already charmed mobs.\n\r");
		return;
	}

	if (is_safe(ch, victim->master))
	{
		Cprintf(ch, "You can't attack this charmie's master.\n\r");
		return;
	}

	if (saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_NONE, DAM_CHARM))
	{
		Cprintf(ch, "Your turn magic attempt fails.\n\r");
		act("$n tried to steal a charmie from you but failed.", ch, NULL, victim->master, TO_VICT, POS_RESTING);
		return;
	}

	vch = victim->master;
	stop_follower(victim);
	add_follower(victim, ch);
	victim->master = ch;
	victim->leader = ch;

	af.where = TO_AFFECTS;
	af.type = gsn_charm_person;
	af.level = modified_level;
	af.duration = modified_level / 10;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(victim, &af);

	check_killer(ch, vch);
	act("$n steals a charmie from you!", ch, NULL, vch, TO_VICT, POS_RESTING);
	act("You steal a charmie from $N!", ch, NULL, vch, TO_CHAR, POS_RESTING);
	return;

}

void
spell_ventriloquate(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	char buf1[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	char speaker[MAX_INPUT_LENGTH];
	CHAR_DATA *vch;
	CHAR_DATA *vict;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	target_name = one_argument(target_name, speaker);
	vict = get_char_room(ch, speaker);
	if (vict == NULL)
	{
		sprintf(buf1, "{y%s says '%s'{x\n\r", speaker, target_name);
		buf1[3] = UPPER(buf1[3]);
		sprintf(buf2, "Someone makes %s say '%s'\n\r", speaker, target_name);
	}
	else
	{
		sprintf(buf1, "{y%s says '%s'{x\n\r",
				IS_NPC(vict) ? vict->short_descr : vict->name, target_name);
		sprintf(buf2, "Someone makes %s say '%s'\n\r",
				IS_NPC(vict) ? vict->short_descr : vict->name, target_name);
	}

	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (!is_exact_name(speaker, vch->name) && IS_AWAKE(vch))
			Cprintf(vch, saving_throw(ch, vch, sn, caster_level, SAVE_NORMAL, STAT_WIS, DAM_NONE) ? buf2 : buf1);
	}

	return;
}


bool
check_wrath(CHAR_DATA * ch)
{
	CHAR_DATA *victim;
	bool result;

	result = FALSE;

	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->hunting == ch)
			result = TRUE;
	}

	return result;
}

void
spell_wrath(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	int targetClan;
	char targetClanName[30];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA* victim;
	OBJ_DATA *obj;
	DESCRIPTOR_DATA *d;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	target_name = one_argument(target_name, buf);

	if (is_affected(ch, gsn_wrath)) {
		Cprintf(ch, "The wrath spirits cannot be summoned again so soon.\n\r");
		return;
	}

	if (!is_clan(ch))
	{
		Cprintf(ch, "Stay out of clan affairs.\n\r");
		return;
	}

	if (IN_ANY_CLAN_HALL(ch))
	{
		Cprintf(ch, "You can't use this in a clan hall.\n\r");
		return;
	}

	if (buf[0] == '\0')
	{
		Cprintf(ch, "You must specify a clan!\n\r");
		return;
	}

	targetClan = clan_lookup(buf);
	strcpy(targetClanName, clan_table[targetClan].name);
	if (!clan_table[targetClan].pkiller 
	|| clan_table[targetClan].independent)
	{
		Cprintf(ch, "No such clan.  Sorry.\n\r");
		return;
	}

	for (d = descriptor_list; d != NULL; d = d->next)
	{
		if (d->character != NULL
			&& d->connected == CON_PLAYING
			&& d->character != ch
			&& d->character->in_room != NULL
			&& d->character->clan == targetClan
			&& !check_wrath(d->character))
		{
			victim = create_mobile(get_mob_index(MOB_VNUM_WRATH));
			char_to_room(victim, ch->in_room);
			obj = create_object(get_obj_index(OBJ_VNUM_WRATH), victim->level);
			size_mob(d->character, victim, d->character->level + 8);
			size_obj(d->character, obj, d->character->level);
			obj_to_char(obj, victim);
			equip_char(victim, obj, WEAR_WIELD);
			victim->hunting = d->character;
			victim->hunt_timer = 30;
			SET_BIT(victim->toggles, TOGGLES_NOEXP);
			Cprintf(d->character, "A wrath falls upon your clan!\n\r");
		}
	}

	Cprintf(ch, "Your wrath is released on clan %s.\n\r", targetClanName);

	af.where = TO_AFFECTS;
        af.type = sn;
        af.level = modified_level;
        af.duration = 5;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = 0;
        affect_to_char(ch, &af);

	return;
}


void
spell_weaken(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (is_affected(victim, sn)) {
		Cprintf(ch, "They are already feeble and wimpy.\n\r");
		return;
	}

	if(saving_throw(ch, victim, sn, caster_level, SAVE_NORMAL, STAT_STR, DAM_NONE))
	{
		Cprintf(ch, "They resist your attempt to weaken them.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = 1 + (modified_level / 6);
	if (IS_NPC(victim))
		af.duration *= 2;
	af.location = APPLY_STR;
	af.modifier = -1 * (modified_level / 5);
	af.bitvector = 0;
	affect_to_char(victim, &af);
	Cprintf(victim, "You feel your strength slip away.\n\r");
	act("$n looks tired and weak.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	return;
}

void
spell_weaponsmith(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	OBJ_DATA *obj = NULL;
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	if (IS_NPC(victim))
	{
		obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_SWORD), victim->level);
	}
	else
	{
		if (victim->charClass == class_lookup("cleric"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_FLAIL), victim->level);
		else if (victim->charClass == class_lookup("mage"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_DAGGER), victim->level);
		else if (victim->charClass == class_lookup("warrior"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_SWORD), victim->level);
		else if (victim->charClass == class_lookup("paladin"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_MACE), victim->level);
		else if (victim->charClass == class_lookup("conjurer"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_DAGGER), victim->level);
		else if (victim->charClass == class_lookup("druid"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_AXE), victim->level);
		else if (victim->charClass == class_lookup("enchanter"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_DAGGER), victim->level);
		else if (victim->charClass == class_lookup("invoker"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_DAGGER), victim->level);
		else if (victim->charClass == class_lookup("ranger"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_AXE), victim->level);
		else if (victim->charClass == class_lookup("thief"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_DAGGER), victim->level);
		else if (victim->charClass == class_lookup("runist"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_SPEAR), victim->level);
		else if (victim->charClass == class_lookup("monk"))
			obj = create_object(get_obj_index(OBJ_VNUM_MAGICAL_KNUCKLES), victim->level);
	}
	size_obj(ch, obj, victim->level);
	obj_to_char(obj, ch);
	if (get_eq_char(ch, WEAR_WIELD) == NULL && ch == victim)
	{
		wear_obj(ch, obj, TRUE);
	}

	Cprintf(ch, "You create %s for %s!\n\r", obj->short_descr, victim->name);
	act("$n has created a beautiful weapon!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	return;

}

/* RT recall spell is back */
void
spell_word_of_recall(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	ROOM_INDEX_DATA *location;
	int RECALL_ROOM, chance;
	AFFECT_DATA *paf;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (!is_clan(ch) && is_clan(victim))
	{
		Cprintf(ch, "Don't mess with clan affairs.  Spell failed.\n\r");
		return;
	}

	if (IS_NPC(victim))
		return;

	if (IS_AFFECTED(victim, AFF_TAUNT)
		|| is_affected(victim, gsn_taunt))
	{
		paf = affect_find(victim->affected, gsn_taunt);
		chance = paf->level + 10;
		if (number_percent() < chance)
		{
			Cprintf(victim, "You are taunted and cannot leave the fight!\n\r");
			act("$n is taunted and cannot leave the fight!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			return;
		}
	}

	if (ch->in_room != NULL && ch->in_room->area->continent == 0)
		RECALL_ROOM = ROOM_VNUM_TEMPLE;
	else
		RECALL_ROOM = ROOM_VNUM_DOMINIA;

	if ((location = get_room_index(RECALL_ROOM)) == NULL)
	{
		Cprintf(victim, "You are completely lost.\n\r");
		return;
	}

	if (IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
	|| wearing_norecall_item(ch)
	|| IS_AFFECTED(victim, AFF_CURSE))
	{
		Cprintf(victim, "Spell failed.\n\r");
		return;
	}

	if (ch != victim
	&& saving_throw(ch, victim, sn, modified_level, SAVE_NORMAL, STAT_WIS, DAM_CHARM))
	{
		Cprintf(ch, "Failed.\n\r");
		return;
	}

	if (victim->fighting != NULL)
		stop_fighting(victim, TRUE);

	ch->move /= 2;
	act("$n disappears.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	char_from_room(victim);
	char_to_room(victim, location);
	act("$n appears in the room.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	do_look(victim, "auto");
}

/*
 * NPC spells.
 */


void
spell_wail(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	act("$n lets out a loud shriek at $N.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	act("$n lets out a loud shriek at you.", ch, NULL, victim, TO_VICT, POS_RESTING);
	act("You let out a loud shriek at $N.", ch, NULL, victim, TO_CHAR, POS_RESTING);

	// Base damage 61 to 151
	dam = dice(modified_level / 5, 10) + modified_level * 3 / 2;

	// Adds 3% for hp. (Typical +50 for PC, 150 for mobs).
	dam += ch->hit * 2 / 100;

	if (race_lookup("dwarf") == victim->race)
		dam /= 2;

	if (saving_throw(ch, victim, sn, modified_level, SAVE_HARD, STAT_CON, DAM_SOUND))
		;
	else if (!is_affected(victim, sn))
	{
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = level;
		af.duration = 3;
		if (IS_NPC(victim))
			af.duration *= 2;
		af.location = APPLY_DEX;
		af.modifier = -3;
		af.bitvector = 0;
		affect_to_char(victim, &af);

		af.location = APPLY_SAVING_SPELL;
		af.modifier = modified_level / 8;
		affect_to_char(victim, &af);
		Cprintf(victim, "You can't hear anything!\n\r");
		Cprintf(ch, "Your opponent is now deaf!\n\r");
	}
	damage(ch, victim, dam, sn, DAM_SOUND, TRUE, TYPE_MAGIC);

	return;
}


void
spell_acid_breath(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 60)
		Cprintf(ch, "!!SOUND(sounds/wav/dragon*.wav V=80 P=20 T=admin)");

	act("$n spits acid at $N.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	act("$n spits a stream of corrosive acid at you.", ch, NULL, victim, TO_VICT, POS_RESTING);
	act("You spit acid at $N.", ch, NULL, victim, TO_CHAR, POS_RESTING);

	// Base damage 61 to 151
	dam = dice(modified_level / 5, 10) + modified_level * 3 / 2;

	// Adds 3% for hp. (Typical +50 for PC, 150 for mobs).
	dam += ch->hit * 2 / 100;

	damage(ch, victim, dam, sn, DAM_ACID, TRUE, TYPE_MAGIC);
	acid_effect(victim, caster_level, dam, TARGET_CHAR);
}


void
spell_fire_breath(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    int caster_level, modified_level;
    int dam;

    caster_level = get_caster_level(level);
    modified_level = get_modified_level(level);

    if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 60) {
        Cprintf(ch, "!!SOUND(sounds/wav/dragon*.wav V=80 P=20 T=admin)");
    }

    act("$n breathes out a searing cone of flame!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
    act("$n breathes roaring flames all over you!", ch, NULL, victim, TO_VICT, POS_RESTING);
    act("You breath out a cone of roaring flame!", ch, NULL, NULL, TO_CHAR, POS_RESTING);

    // Base damage 61 to 151
    dam = dice(modified_level / 5, 10) + modified_level * 3 / 2;

    // Adds 3% for hp. (Typical +50 for PC, 150 for mobs).
    dam += ch->hit * 2 / 100;

    if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 60) {
        Cprintf(ch, "!!SOUND(sounds/wav/dragon*.wav V=80 P=20 T=admin)");
    }

    fire_effect(victim->in_room, caster_level, dam, TARGET_ROOM);
    for (vch = victim->in_room->people; vch != NULL; vch = vch_next) {
        vch_next = vch->next_in_room;

        // Safe people
        if (is_safe(ch, vch)) {
            continue;
        }

        // Don't hit same group
        if (is_same_group(ch, vch)) {
            continue;
        }

        // Mobs only hit combatants
        if (IS_NPC(ch) && vch->fighting != ch) {
            continue;
        }

        // Don't hit wizi imms that you're not fighting
        if (vch->invis_level && vch->fighting != ch) {
            continue;
        }

        damage(ch, vch, dam, sn, DAM_FIRE, TRUE, TYPE_MAGIC);
        fire_effect(vch, caster_level, dam, TARGET_CHAR);
    }
}

void
spell_frost_breath(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    int caster_level, modified_level;
    int dam;

    caster_level = get_caster_level(level);
    modified_level = get_modified_level(level);

    if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 60) {
        Cprintf(ch, "!!SOUND(sounds/wav/dragon*.wav V=80 P=20 T=admin)");
    }

    act("$n breathes out a freezing cone of frost!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
    act("$n breathes a freezing cone of frost over you!", ch, NULL, victim, TO_VICT, POS_RESTING);
    act("You breath out a cone of frost.", ch, NULL, NULL, TO_CHAR, POS_RESTING);

    // Base damage 61 to 151
    dam = dice(modified_level / 5, 10) + modified_level * 3 / 2;

    // Adds 3% for hp. (Typical +50 for PC, 150 for mobs).
    dam += ch->hit * 2 / 100;

    if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 60) {
        Cprintf(ch, "!!SOUND(sounds/wav/dragon*.wav V=80 P=20 T=admin)");
    }

    cold_effect(victim->in_room, caster_level, dam, TARGET_ROOM);
    for (vch = victim->in_room->people; vch != NULL; vch = vch_next) {
        vch_next = vch->next_in_room;

        // Safe people
        if (is_safe(ch, vch)) {
            continue;
        }

        // Don't hit same group
        if (is_same_group(ch, vch)) {
            continue;
        }

        // Mobs only hit combatants
        if (IS_NPC(ch) && vch->fighting != ch) {
            continue;
        }

        // Don't hit wizi imms that you're not fighting
        if (vch->invis_level && vch->fighting != ch) {
            continue;
        }

        damage(ch, vch, dam, sn, DAM_COLD, TRUE, TYPE_MAGIC);
        cold_effect(vch, caster_level, dam, TARGET_CHAR);
    }
}


void
spell_gas_breath(int sn, int level, CHAR_DATA * ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int caster_level, modified_level;
    int dam;

    caster_level = get_caster_level(level);
    modified_level = get_modified_level(level);

    if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 60) {
        Cprintf(ch, "!!SOUND(sounds/wav/dragon*.wav V=80 P=20 T=admin)");
    }

    act("$n breathes out a cloud of poisonous gas!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
    act("You breath out a cloud of poisonous gas.", ch, NULL, NULL, TO_CHAR, POS_RESTING);

    // Base damage 61 to 151
    dam = dice(modified_level / 5, 10) + modified_level * 3 / 2;

    // Adds 3% for hp. (Typical +50 for PC, 150 for mobs).
    dam += ch->hit * 2 / 100;

    if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 60) {
        Cprintf(ch, "!!SOUND(sounds/wav/dragon*.wav V=80 P=20 T=admin)");
    }

    poison_effect(victim->in_room, caster_level, dam, TARGET_ROOM);

    for (vch = victim->in_room->people; vch != NULL; vch = vch_next) {
        vch_next = vch->next_in_room;

        // Safe people
        if (is_safe(ch, vch)) {
            continue;
        }

        // Don't hit same group
        if (is_same_group(ch, vch)) {
            continue;
        }

        // Mobs only hit combatants
        if (IS_NPC(ch) && vch->fighting != ch) {
            continue;
        }

        // Don't hit wizi imms that you're not fighting
        if (vch->invis_level && vch->fighting != ch) {
            continue;
        }

        poison_effect(vch, caster_level, dam, TARGET_CHAR);
        damage(ch, vch, dam, sn, DAM_POISON, TRUE, TYPE_MAGIC);
    }
}

void
spell_lightning_breath(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;
	int dam;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 60)
		Cprintf(ch, "!!SOUND(sounds/wav/dragon*.wav V=80 P=20 T=admin)");

	act("$n breathes a bolt of lightning at $N.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	act("$n breathes a bolt of lightning at you!", ch, NULL, victim, TO_VICT, POS_RESTING);
	act("You breath a bolt of lightning at $N.", ch, NULL, victim, TO_CHAR, POS_RESTING);

	// Base damage 61 to 151
	dam = dice(modified_level / 5, 10) + modified_level * 3 / 2;

	// Adds 3% for hp. (Typical +50 for PC, 150 for mobs).
	dam += ch->hit * 2 / 100;

	damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE, TYPE_MAGIC);
	shock_effect(victim, caster_level, dam, TARGET_CHAR);
}

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void
spell_general_purpose(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;
	int dam;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = number_range(25, 100);
	if (race_lookup("dwarf") == victim->race)
		dam /= 2;

//	if (saving_throw(ch, victim, sn, modified_level, SAVE_HARD, STAT_STR, DAM_PIERCE))
//		dam = dam * 3 / 4;

	damage(ch, victim, dam, sn, DAM_PIERCE, TRUE, TYPE_MAGIC);
	return;
}

void
spell_high_explosive(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int caster_level, modified_level;
	int dam;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	dam = number_range(30, 120);
	if (race_lookup("dwarf") == victim->race)
		dam /= 2;

//	if (saving_throw(ch, victim, sn, modified_level, SAVE_HARD, STAT_STR, DAM_PIERCE))
//		dam = dam * 3 / 4;

	damage(ch, victim, dam, sn, DAM_PIERCE, TRUE, TYPE_MAGIC);
	return;
}

void
spell_skeletal_warrior(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(max_no_charmies(ch))
	{
		Cprintf(ch, "No more legions of the dead appear.\n\r");
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
	{
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	victim = create_mobile(get_mob_index(MOB_VNUM_SKEL));
	char_to_room(victim, ch->in_room);
	size_mob(ch, victim, modified_level * 5 / 6);
	victim->hitroll = modified_level / 8;
	victim->damroll = modified_level / 2 + 2;

	act("A pile of bones materialize and click together to form a skeletal warrior.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You summon a pile of bones which click together and form a skeletal warrior to do your bidding.\n\r");
	if (victim->master)
		stop_follower(victim);
	add_follower(victim, ch);
	victim->leader = ch;

	victim->rot_timer = modified_level / 6 + number_range(5, 10);

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = modified_level;
	af.duration = modified_level / 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = AFF_CHARM;
	affect_to_char(victim, &af);
}


