/* revision 1.2 - August 1 1999 - making it compilable under g++ */
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
 *        ROM 2.4 is copyright 1993-1996 Russ Taylor                       *
 *        ROM has been brought to you by the ROM consortium                *
 *            Russ Taylor (rtaylor@pacinfo.com)                            *
 *            Gabrielle Taylor (gtaylor@pacinfo.com)                       *
 *            Brian Moore (rom@rom.efn.org)                                *
 *        By using this code, you have agreed to follow the terms of the   *
 *        ROM license, in the file Rom24/doc/rom.license                   *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "clan.h"
#include "merc.h"
#include "music.h"
#include "magic.h"
#include "stats.h"
#include "recycle.h"
#include "utils.h"

#define MATERIAL_FLOATS(a) (!str_cmp(a, "air")          \
                ||  !str_cmp(a, "bamboo")               \
                ||  !str_cmp(a, "cardboard")            \
                ||  !str_cmp(a, "cloth")                \
                ||  !str_cmp(a, "cork")                 \
                ||  !str_cmp(a, "feathers")             \
                ||  !str_cmp(a, "felt")                 \
                ||  !str_cmp(a, "ice")                  \
                ||  !str_cmp(a, "lace")                 \
                ||  !str_cmp(a, "leather")              \
                ||  !str_cmp(a, "linen")                \
                ||  !str_cmp(a, "oil")                  \
                ||  !str_cmp(a, "paper")                \
                ||  !str_cmp(a, "parchment")            \
                ||  !str_cmp(a, "satin")                \
                ||  !str_cmp(a, "silk")                 \
                ||  !str_cmp(a, "sponge")               \
                ||  !str_cmp(a, "vellum")               \
                ||  !str_cmp(a, "velvet")               \
                ||  !str_cmp(a, "wood"))                \

int check_valid(CHAR_DATA * ch);

extern int double_xp_ticks;
extern int saving_throw(CHAR_DATA *, CHAR_DATA *, int sn, int level,
	int diff, int stat, int damtype);
extern int power_tattoo_count(CHAR_DATA*, int);
extern CHAR_DATA *System;
extern DECLARE_DO_FUN(do_newbie_channel);
extern void call_lightning_update();
extern void advance_craft(CHAR_DATA *, OBJ_DATA *);
extern void advance_weapon(CHAR_DATA *ch, OBJ_DATA *obj, int xp);

/* command procedures needed */
DECLARE_DO_FUN(do_look);
void hunt_victim(CHAR_DATA* ch);

/*
 * Local functions.
 */

void quest_update(void);		/* Vassago - quest.c */
void talk_auction(char *argument);
void do_save(CHAR_DATA * ch, char *argument);
void check_weather(char *buf, int diff);

int hit_gain(CHAR_DATA * ch);
int mana_gain(CHAR_DATA * ch);
int move_gain(CHAR_DATA * ch);
void mobile_update(void);
void rain_update(void);
void char_update(void);
void obj_update(void);
void aggr_update(void);
void null_room_update(void);
void room_update(void);
void spectral_update();
void tip_update();
void advance_leader(CHAR_DATA * ch);
void advance_recruiter(CHAR_DATA * ch);
void demote_leader(CHAR_DATA* ch);
void demote_recruiter(CHAR_DATA* ch);
void olcautosave(void);
void save_area_list(void);
void save_area(AREA_DATA *pArea);


/* Code by: Jim,
   -dragon growth */

bool
dragon_grow(CHAR_DATA * ch)
{

	bool no_good = TRUE;
	int i;
	int race;

	switch (dice(1, 5))
	{
	case 1:
		ch->race = race_lookup("black dragon");
		Cprintf(ch, "You feel acid course through your veins.\n\r");
		Cprintf(ch, "Black wings unfold off your back.\n\r");
		Cprintf(ch, "Saliva turned acid drips from your maw.\n\r");
		no_good = FALSE;
		break;
	case 2:
		ch->race = race_lookup("blue dragon");
		Cprintf(ch, "The sky clouds over and lightning crackles in the distance.\n\r");
		Cprintf(ch, "Energy ripples, and your entire body vibrates with each pulse.\n\r");
		Cprintf(ch, "Your scales harden into blue metallic plates.\n\r");
		no_good = FALSE;
		break;
	case 3:
		ch->race = race_lookup("green dragon");
		Cprintf(ch, "A stench crawls its way up your nasal passage.\n\r");
		Cprintf(ch, "Poison seeps through your body and clouds your eyes.\n\r");
		Cprintf(ch, "Your green tail lashes about behind you, with fearsome power.\n\r");
		no_good = FALSE;
		break;
	case 4:
		ch->race = race_lookup("red dragon");
		Cprintf(ch, "A powerful heat rolls up your back, and fills your eyes.\n\r");
		Cprintf(ch, "Red claws stab the air frantically as the burning fills your brain.\n\r");
		Cprintf(ch, "The burning subsides and your new red coat of scales clank together.\n\r");
		no_good = FALSE;
		break;
	case 5:
		ch->race = race_lookup("white dragon");
		Cprintf(ch, "White wings fold up off your body and you test the air with them.\n\r");
		Cprintf(ch, "Breath burns out of your maw, spilling the burning cold frost into the air.\n\r");
		Cprintf(ch, "Your white hind claws cause the ground to harden and freeze.\n\r");
		no_good = FALSE;
		break;
	}
	race = ch->race;
	/* re init race */
	ch->affected_by = ch->affected_by | race_table[race].aff;
	ch->imm_flags = ch->imm_flags | race_table[race].imm;
	ch->res_flags = ch->res_flags | race_table[race].res;
	ch->vuln_flags = ch->vuln_flags | race_table[race].vuln;
	ch->form = race_table[race].form;
	ch->parts = race_table[race].parts;

	/* add skills */
	for (i = 0; i < 5; i++)
	{
		if (pc_race_table[race].skills[i] == NULL)
			break;
		group_add(ch, pc_race_table[race].skills[i], FALSE);
		if (ch->pcdata->learned[skill_lookup(pc_race_table[race].skills[i])] <= 1)
			ch->pcdata->learned[skill_lookup(pc_race_table[race].skills[i])] = 1;
	}
	ch->size = pc_race_table[race].size;

	if (no_good)
		return FALSE;
	else
		return TRUE;
}

/*
 * Advancement stuff.
 */

/* the auction update - another very important part */

void
auction_update(void)
{
	char buf[MAX_STRING_LENGTH];

	/*int numb; */

	if (auction->item != NULL)
		if (--auction->pulse <= 0)	/* decrease pulse */
		{
			auction->pulse = PULSE_AUCTION;
			switch (++auction->going)	/* increase the going state */
			{
			case 1:			/* going once */
			case 2:			/* going twice */
				if (auction->bet > 0)
					sprintf(buf, "%s: going %s for %d gold.", auction->item->short_descr,
						((auction->going == 1) ? "once" : "twice"), auction->bet);
				else
					sprintf(buf, "%s: going %s (no bet received yet).", auction->item->short_descr,
						((auction->going == 1) ? "once" : "twice"));

				talk_auction(buf);
				break;
			case 3:			/* SOLD! */
				if (auction->bet > 0)
				{
					sprintf(buf, "%s sold to %s for %d gold.\n\r",
						auction->item->short_descr,
						IS_NPC(auction->buyer) ? auction->buyer->short_descr : auction->buyer->name,
						auction->bet);
					talk_auction(buf);
					obj_to_char(auction->item, auction->buyer);
					act("The auctioneer appears before you in a puff of smoke and hands you $p.",
						auction->buyer, auction->item, NULL, TO_CHAR, POS_RESTING);
					act("The auctioneer appears before $n, and hands $m $p",
						auction->buyer, auction->item, NULL, TO_ROOM, POS_RESTING);

					if(auction->seller->reclass == reclass_lookup("templar"))
					{
						auction->bet_gold = auction->bet_gold * 9/10;
						auction->bet_silver = auction->bet_silver * 9/10;
						Cprintf(auction->seller, "You tithe some of the gold.\n\r");
					}
					auction->seller->gold += auction->bet_gold;		/* give him the money */
					auction->seller->silver += auction->bet_silver;

					auction->item = NULL;	/* reset item */
				}
				else
				/* not sold */
				{
					sprintf(buf, "No bets received for %s - object has been removed.", auction->item->short_descr);
					talk_auction(buf);
					act("The auctioneer appears before you to return $p to you.",
						auction->seller, auction->item, NULL, TO_CHAR, POS_RESTING);
					act("The auctioneer appears before $n to return $p to $m.",
						auction->seller, auction->item, NULL, TO_ROOM, POS_RESTING);
					obj_to_char(auction->item, auction->seller);
					auction->item = NULL;	/* clear auction */
				}
			}
		}
}

void
advance_level(CHAR_DATA * ch, bool hide)
{
	char buf[MAX_STRING_LENGTH];
	int add_hp;
	int add_mana;
	int add_move;
	int add_prac;
	int sn = 1;
	//OBJ_DATA *obj;
	//OBJ_DATA *obj_next;

	ch->pcdata->last_level = get_hours(ch);

	if (IS_SET(ch->act, PLR_AUTOTITLE))
	{
		sprintf(buf, "the %s", title_table[ch->charClass][ch->level][ch->sex == SEX_FEMALE ? 1 : 0]);
		set_title(ch, buf);
	}

	add_hp = con_app[get_race_curr_stat(ch, STAT_CON)].hitp + number_range(
										  class_table[ch->charClass].hp_min,
										 class_table[ch->charClass].hp_max);
	add_mana = number_range(7, (2 * get_race_curr_stat(ch, STAT_INT)
								+ get_race_curr_stat(ch, STAT_WIS)) / 7);
	if (!class_table[ch->charClass].fMana)
		add_mana /= 2;
	add_move = number_range(1, (get_race_curr_stat(ch, STAT_CON)
								+ get_race_curr_stat(ch, STAT_DEX)) / 6);
	add_prac = wis_app[get_race_curr_stat(ch, STAT_WIS)].practice;

	add_hp = add_hp * 9 / 10;
	add_mana = add_mana * 9 / 10;
	add_move = add_move * 9 / 10;

	add_hp = UMAX(2, add_hp);
	add_mana = UMAX(2, add_mana);
	add_move = UMAX(6, add_move);

/* Code by: Jim
   -min mana gain */
	if (ch->charClass == class_lookup("paladin")
		|| ch->charClass == class_lookup("runist"))
		add_mana += 4;

	if (ch->charClass == class_lookup("mage")
		|| ch->charClass == class_lookup("enchanter")
		|| ch->charClass == class_lookup("invoker")
		|| ch->charClass == class_lookup("conjurer")
		|| ch->charClass == class_lookup("cleric")
		|| ch->charClass == class_lookup("druid"))
		add_mana += 6;


	if (ch->charClass == class_lookup("thief")
		||ch->charClass == class_lookup("ranger"))
		add_mana += 2;

	// Monks get bonus movement instead of mana.
	if(ch->charClass == class_lookup("monk"))
		add_move = 15 + dice(1, 8);

	ch->max_hit += add_hp;
	ch->max_mana += add_mana;
	ch->max_move += add_move;
	ch->practice += add_prac;
	ch->train += 1;

	if (!hide && check_valid(ch))
	{
		Cprintf(ch, "You gain %d hit point%s, %d mana, %d move, and %d practice%s.\n\r",
				add_hp, add_hp == 1 ? "" : "s", add_mana, add_move,
				add_prac, add_prac == 1 ? "" : "s");
	}

	// All skills known raise 1% per level.
	while(1)
	{
		if(sn == gsn_last_skill)
        		break;

		if(ch->level <= skill_table[sn].skill_level[ch->charClass]
		|| ch->pcdata->learned[sn] == 0) {
			sn++;
			continue;
		}

		if(ch->pcdata->learned[sn] < 75)
			ch->pcdata->learned[sn]++;
			 
		if(ch->race == race_lookup("human") && ch->pcdata->learned[sn] < 85)
			ch->pcdata->learned[sn]++;

		sn++;
	}


	/* Code by: Jim
	   -Dragon code */
	if (race_lookup("hatchling") == ch->race)
		if (ch->level == 10)
			if (!(dragon_grow(ch)))
			{
				ch->race = race_lookup("hatchling");
			}

	if (race_lookup("sliver") == ch->race)
	{
		if (ch->level == 10 && ch->sliver == 0)
		{
			ch->sliver = number_range(1, 25);

			Cprintf(ch, "You mature into a %s sliver!\n\r", sliver_table[ch->sliver].name);

		}
	}

	if (ch->level == 10 && ch->vassal != NULL && !ch->remort && ch->reclass)
	{
		free_string(ch->vassal);
		ch->vassal = NULL;
	}

	/*if (ch->level == 10 && !ch->remort && !ch->reclass)
	{
		for (obj = ch->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			if (obj->wear_loc == WEAR_NONE)
				continue;

			if (IS_SET(obj->wear_flags, ITEM_NEWBIE))
			{
				act("You hastily remove $p, as it is now too small.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				act("$n hastily removes $p, as it is now too small.", ch, obj, NULL, TO_ROOM, POS_RESTING);

				obj_from_char(obj);
				obj_to_char(obj, ch);
			}
		}
	}*/

	if (ch->level == 10 && IS_SET(ch->wiznet, CAN_CLAN) && ch->clan == 0)
	{
		Cprintf(ch, "You have been lonered.\n\r");
		ch->clan = clan_lookup("loner");
	}

	/* k, check for that nasty leader/recruiter stuff */
	if (ch->trust == 52 && ch->level == 51)
	{
		Cprintf(ch, "You are have gained level 52, welcome to recruiter!\n\r");
		demote_recruiter(ch);
		advance_recruiter(ch);
	}

	if (ch->trust == 53 && ch->level == 51)
	{
		Cprintf(ch, "You are have gained level 53, welcome to leader!\n\r");
		demote_leader(ch);
		advance_leader(ch);
	}

	//patron change: reset cap on patron points
	if(ch->level >= 51)
		ch->pass_along_limit = 10000;
	else
		ch->pass_along_limit = exp_per_level(ch, ch->pcdata->points) / 2;

	return;
}



void
gain_exp(CHAR_DATA * ch, int gain)
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *fch;
	OBJ_INDEX_DATA *pObjIndex;
	OBJ_DATA *obj;


	if (IS_NPC(ch) || ch->level >= LEVEL_HERO)
		return;

	// We have major problems if people gain a level using focus
        if(ch->pcdata != NULL
        && ch->pcdata->any_skill == TRUE) {
		Cprintf(ch, "The experience is consumed by the charge.\n\r");
		gain = 0;
		return;
	}

	ch->exp = UMAX(exp_per_level(ch, ch->pcdata->points), ch->exp + gain);

	while (ch->level < 51 && ch->exp >=
		   exp_per_level(ch, ch->pcdata->points) * (ch->level + 1))
	{
		Cprintf(ch, "You raise a level!!  ");
		ch->level += 1;
		log_string("%s gained level %d", ch->name, ch->level);
		sprintf(buf, "$N has attained level %d!", ch->level);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, 0);
		advance_level(ch, FALSE);
		if (IS_SET(ch->toggles, TOGGLES_SOUND))
			Cprintf(ch, "!!SOUND(sounds/wav/level*.wav V=80 P=20 T=admin)");

		for (fch = char_list; fch != NULL; fch = fch->next)
		{
			if (IS_IMMORTAL(fch) && fch->gift > 0 && fch->invis_level < ch->level && fch->incog_level < ch->level)
			{

				if ((pObjIndex = get_obj_index(fch->gift)) == NULL)
				{
					Cprintf(fch, "No gift has that vnum.\n\r");
					return;
				}
				obj = create_object(pObjIndex, 0);

				if (CAN_WEAR(obj, ITEM_TAKE))
					obj_to_char(obj, ch);
				else
					obj_to_room(obj, ch->in_room);

				Cprintf(ch, "%s gives you %s!\n\r", fch->name, obj->short_descr);
				if (fch != NULL && fch->desc != NULL)
				{
					if (fch->desc->connected != CON_NOTE_TEXT)
						Cprintf(fch, "You hand out %s to %s.\n\r", obj->short_descr, ch->name);
				}
				wiznet("$N loads $p as levelling gift.", fch, obj, WIZ_LOAD, 0, get_trust(fch));
			}
		}

		save_char_obj(ch, FALSE);
	}

	return;
}



/*
 * Regenerate hit points every tick for all characters.
 */
int
hit_gain(CHAR_DATA * ch)
{
	int tattoos = 0;
    int base_hp_healed = 0;
    int healing_rate = 0;
    int actual_hp_healed = 0;

	if (ch->in_room == NULL)
		return 0;

    // Simple formula for mobiles.
    if (IS_NPC(ch)) {
        actual_hp_healed = 5 + (ch->level * 3);
    }
    // Less simple formula for players.
    else {
        // Base recovery of 5% max hp each tick.      
        base_hp_healed = UMAX(5, MAX_HP(ch) * 5 / 100);

        // Determine a healing rate based on level and con.
        healing_rate = UMAX(1, (get_curr_stat(ch, STAT_CON) / 2) + (ch->level / 4));

        // The healing rate is modified by regeneration and stone sleep.
        if (number_percent() < get_skill(ch, gsn_regeneration)) {
            healing_rate = healing_rate * 3 / 2;

            if (ch->remort > 0) {
                healing_rate = healing_rate * 3 / 2;
	}

            if (ch->hit < MAX_HP(ch)) {
				check_improve(ch, gsn_regeneration, TRUE, 8);
		}
        }

        if (ch->position == POS_SLEEPING
        && number_percent() < get_skill(ch, gsn_stone_sleep)) {
            check_improve(ch, gsn_stone_sleep, TRUE, 8);
            healing_rate = healing_rate * 3 / 2;
		}

        // The healing rate is applied based on various factors.
        actual_hp_healed = base_hp_healed;

        // Fast Healing skill
        if (number_percent() < get_skill(ch, gsn_fast_healing)) {
            actual_hp_healed += healing_rate;

            if (ch->hit < MAX_HP(ch)) {
                check_improve(ch, gsn_fast_healing, TRUE, 8);
            }
	}

        // Build Fire skill
        if (room_is_affected(ch->in_room, gsn_build_fire)) {
            actual_hp_healed += healing_rate;
	}

        // Lair Skill
        if (room_is_affected(ch->in_room, gsn_lair)) {
            actual_hp_healed += healing_rate;
	}

        // Condensation Skill
	if (ch->in_room
	&& get_skill(ch, gsn_condensation) > 0
	&& (ch->in_room->sector_type == SECT_WATER_SWIM
	|| ch->in_room->sector_type == SECT_WATER_NOSWIM
	|| ch->in_room->sector_type == SECT_UNDERWATER)
        && number_percent() < get_skill(ch, gsn_condensation)) {
       		check_improve(ch, gsn_condensation, TRUE, 3);
        	Cprintf(ch, "You condense the water and close your wounds.\n\r");
            actual_hp_healed += healing_rate;
	}

        // HP recovery Tattoos
	if ((tattoos = power_tattoo_count(ch, TATTOO_HP_REGEN)) > 0) {
            actual_hp_healed += (healing_rate * tattoos);
	}

        // Rating of the current room (homeland counts as 300 always)
        if(is_affected(ch, gsn_homeland)) {
            actual_hp_healed += (healing_rate * 300 / 100);
        }
        else {
            actual_hp_healed += (healing_rate * ch->in_room->heal_rate / 100);
        }

        // Furniture in the current room
        if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE) {
            actual_hp_healed += (healing_rate * ch->on->value[3] / 100);
        }
    }

    // Zanshin allows rapid healing during combat (almost as good as sleeping)
    if (number_percent() < get_skill(ch, gsn_zanshin)
    && ch->position == POS_FIGHTING) {
        actual_hp_healed = actual_hp_healed * 6 / 5;
        if (ch->hit < MAX_HP(ch)) {
            check_improve(ch, gsn_zanshin, TRUE, 8);
        }
    }
    else {
        // Healing rate is then globally affected by position.
        switch (ch->position) {
            case POS_SLEEPING:
                actual_hp_healed = actual_hp_healed * 3 / 2; break;
            case POS_RESTING:
                actual_hp_healed = actual_hp_healed; break;
            default:
                actual_hp_healed = actual_hp_healed * 1 / 2; break;
        }
    }

    // Bad stuff that hurts recovery: hungry/thirst/poison/plagued/dissolved/cloudkilled
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER] == 0) {
        actual_hp_healed = actual_hp_healed / 2;
    }

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] == 0) {
        actual_hp_healed = actual_hp_healed / 2;
    }

    if (IS_AFFECTED(ch, AFF_POISON)) {
        actual_hp_healed = actual_hp_healed / 8;
    }

    if (IS_AFFECTED(ch, AFF_PLAGUE)) {
        actual_hp_healed = actual_hp_healed / 8;
}

    if (is_affected(ch, gsn_dissolution)
    || room_is_affected(ch->in_room, gsn_cloudkill)) {
        actual_hp_healed = 0;
    }

    return UMIN(actual_hp_healed, MAX_HP(ch) - ch->hit);
}

/*
 * Regenerate mana every tick for all characters.
 */
int
mana_gain(CHAR_DATA * ch)
{
	int tattoos = 0;
    int base_mana_healed = 0;
    int healing_rate = 0;
    int actual_mana_healed = 0;

	if (ch->in_room == NULL)
		return 0;

    // Simple formula for mobiles.
    if (IS_NPC(ch)) {
        actual_mana_healed = 5 + (ch->level * 3);
		}
    // Less simple formula for players.
    else {
        // Base recovery of 10% max mana each tick (half for non-casters).
        base_mana_healed = UMAX(10, MAX_MANA(ch) * 5 / 100);

        // Determine a healing rate based on level, int and wis.
        healing_rate = UMAX(2, 
            (get_curr_stat(ch, STAT_INT) / 2) + (get_curr_stat(ch, STAT_WIS) / 2) + (ch->level / 10));

        // Stone sleep boosts mana recovery rate when sleeping
        if (ch->position == POS_SLEEPING
        && number_percent() < get_skill(ch, gsn_stone_sleep)) {
            check_improve(ch, gsn_stone_sleep, TRUE, 8);
            healing_rate = healing_rate * 3 / 2;
		}

        // The healing rate is applied based on various factors.
        actual_mana_healed = base_mana_healed;

        // Meditation skill
        if (number_percent() < get_skill(ch, gsn_meditation)) {
            actual_mana_healed += healing_rate;

            if (ch->mana < MAX_MANA(ch))
		check_improve(ch, gsn_meditation, TRUE, 8);
	}

        // Oracle skill
        if (room_is_affected(ch->in_room, gsn_oracle)) {
            actual_mana_healed += healing_rate;
	}

        // Lair Skill
        if (room_is_affected(ch->in_room, gsn_lair)) {
            actual_mana_healed += healing_rate;
	}

        // Condensation Skill
	if (ch->in_room
	&& get_skill(ch, gsn_condensation) > 0
	&& (ch->in_room->sector_type == SECT_WATER_SWIM
	|| ch->in_room->sector_type == SECT_WATER_NOSWIM
	|| ch->in_room->sector_type == SECT_UNDERWATER)
        && number_percent() < get_skill(ch, gsn_condensation)) {
        	check_improve(ch, gsn_condensation, TRUE, 3);
        	Cprintf(ch, "You condense the water into mana.\n\r");
            actual_mana_healed += healing_rate;
	}

        // Mana recovery Tattoos
        if ((tattoos = power_tattoo_count(ch, TATTOO_MANA_REGEN)) > 0) {
            actual_mana_healed += (healing_rate * tattoos);
        }

        // Rating of the current room (homeland counts as 300 always)
        if(is_affected(ch, gsn_homeland)) {
            actual_mana_healed += (healing_rate * 300 / 100);
        }
        else {
            actual_mana_healed += (healing_rate * ch->in_room->mana_rate / 100);
        }

        // Furniture in the current room
        if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE) {
            actual_mana_healed += (healing_rate * ch->on->value[4] / 100);
        }
    }

    // Zanshin allows rapid healing during combat (almost as good as sleeping)
    if (number_percent() < get_skill(ch, gsn_zanshin)
    && ch->position == POS_FIGHTING) {
        actual_mana_healed = actual_mana_healed * 5 / 4;
        if (ch->mana < MAX_MANA(ch)) {
            check_improve(ch, gsn_zanshin, TRUE, 8);
        }
    }
    else {
        // Healing rate is then globally affected by position.
        switch (ch->position) {
            case POS_SLEEPING:
                actual_mana_healed = actual_mana_healed * 3 / 2; break;
            case POS_RESTING:
                actual_mana_healed = actual_mana_healed; break;
            default:
                actual_mana_healed = actual_mana_healed * 1 / 2; break;
        }
    }

    // Bad stuff that hurts recovery: hungry/thirst/poison/plagued/dissolved/cloudkilled
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER] == 0) {
        actual_mana_healed = actual_mana_healed / 2;
    }

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] == 0) {
        actual_mana_healed = actual_mana_healed / 2;
    }

    if (IS_AFFECTED(ch, AFF_POISON)) {
        actual_mana_healed = actual_mana_healed / 8;
    }

    if (IS_AFFECTED(ch, AFF_PLAGUE)) {
        actual_mana_healed = actual_mana_healed / 8;
}

    if (is_affected(ch, gsn_dissolution)
    || room_is_affected(ch->in_room, gsn_cloudkill)) {
        actual_mana_healed = 0;
    }

    return UMIN(actual_mana_healed, MAX_MANA(ch) - ch->mana);
}

/*
 * Regenerate movement every tick for all characters.
 */
int
move_gain(CHAR_DATA * ch)
{
    int base_move_healed = 0;
    int healing_rate = 0;
    int actual_move_healed = 0;


	if (ch->in_room == NULL)
		return 0;

    // Simple formula for mobiles.
    if (IS_NPC(ch)) {
        actual_move_healed = 5 + (ch->level * 3);
	}
    // Less simple formula for players.
    else {
        // Base recovery of 7% max move each tick (half for non-casters).
        base_move_healed = UMAX(20, MAX_MOVE(ch) * 8 / 100);

        // Determine a healing rate based on level and strength.
        healing_rate = UMAX(5, (get_curr_stat(ch, STAT_STR) / 2) + (ch->level / 3));

        // Stone sleep boosts mana recovery rate when sleeping
        if (ch->position == POS_SLEEPING
        && number_percent() < get_skill(ch, gsn_stone_sleep)) {
            check_improve(ch, gsn_stone_sleep, TRUE, 8);
            healing_rate = healing_rate * 3 / 2;
		}

        // The healing rate is applied based on various factors.
        actual_move_healed = base_move_healed;

        // Iron Vigil Skill
        if (number_percent() < get_skill(ch, gsn_iron_vigil)) {
            actual_move_healed += healing_rate;
            check_improve(ch, gsn_iron_vigil, TRUE, 8);
	}

        // Build Fire Skill
        if (room_is_affected(ch->in_room, gsn_build_fire)) {
            actual_move_healed += healing_rate;
	}

        // Lair Skill
        if (room_is_affected(ch->in_room, gsn_lair)) {
            actual_move_healed += healing_rate;
	}

        // Condensation Skill
	if (ch->in_room
	&& get_skill(ch, gsn_condensation) > 0
	&& (ch->in_room->sector_type == SECT_WATER_SWIM
	|| ch->in_room->sector_type == SECT_WATER_NOSWIM
	|| ch->in_room->sector_type == SECT_UNDERWATER)
        && number_percent() < get_skill(ch, gsn_condensation)) {
        	check_improve(ch, gsn_condensation, TRUE, 3);
        	Cprintf(ch, "You condense the water for stamina.\n\r");
            actual_move_healed += healing_rate;
        }

        // Rating of the current room (homeland counts as 300 always)
        if(is_affected(ch, gsn_homeland)) {
            actual_move_healed += (healing_rate * 300 / 100);
        }
        else {
            actual_move_healed += (healing_rate * ch->in_room->heal_rate / 100);
	}

        // Furniture in the current room
        if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE) {
            actual_move_healed += (healing_rate * ch->on->value[3] / 100);
        }
    }

    // Zanshin allows rapid healing during combat (almost as good as sleeping)
    if (number_percent() < get_skill(ch, gsn_zanshin)
    && ch->position == POS_FIGHTING) {
        actual_move_healed = actual_move_healed * 5 / 4;
        if (ch->mana < MAX_MANA(ch)) {
            check_improve(ch, gsn_zanshin, TRUE, 8);
        }
    }
    else {
        // Healing rate is then globally affected by position.
        switch (ch->position) {
            case POS_SLEEPING:
                actual_move_healed = actual_move_healed * 3 / 2; break;
            case POS_RESTING:
                actual_move_healed = actual_move_healed; break;
            default:
                actual_move_healed = actual_move_healed * 1 / 2; break;
        }
    }

    // Bad stuff that hurts recovery: hungry/thirst/poison/plagued/dissolved/cloudkilled
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER] == 0) {
        actual_move_healed = actual_move_healed / 2;
    }

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] == 0) {
        actual_move_healed = actual_move_healed / 2;
    }

    if (IS_AFFECTED(ch, AFF_POISON)) {
        actual_move_healed = actual_move_healed / 8;
    }

    if (IS_AFFECTED(ch, AFF_PLAGUE)) {
        actual_move_healed = actual_move_healed / 8;
    }

	if (is_affected(ch, gsn_dissolution)
    || room_is_affected(ch->in_room, gsn_cloudkill)) {
        actual_move_healed = 0;
    }

    return UMIN(actual_move_healed, MAX_MOVE(ch) - ch->move);
}


void
gain_condition(CHAR_DATA * ch, int iCond, int value)
{
	int condition;

	if (value == 0 || IS_NPC(ch) || ch->level >= LEVEL_IMMORTAL)
		return;

	condition = ch->pcdata->condition[iCond];
	if (condition == -1)
		return;
	ch->pcdata->condition[iCond] = URANGE(0, condition + value, 48);

	if (ch->pcdata->condition[iCond] == 0)
	{
		switch (iCond)
		{
		case COND_HUNGER:
			if (ch->desc)
				if (ch->desc->connected < CON_NOTE_TO
					|| ch->desc->connected > CON_NOTE_FINISH)
					Cprintf(ch, "You are hungry.\n\r");
			break;

		case COND_THIRST:
			if (ch->desc)
				if (ch->desc->connected < CON_NOTE_TO
					|| ch->desc->connected > CON_NOTE_FINISH)
					Cprintf(ch, "You are thirsty.\n\r");
			break;

		case COND_DRUNK:
			if (ch->desc)
				if (ch->desc->connected < CON_NOTE_TO
					|| ch->desc->connected > CON_NOTE_FINISH)
					if (condition != 0)
						Cprintf(ch, "You are sober.\n\r");
			break;
		}
	}

	return;
}



/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void
mobile_update(void)
{
	CHAR_DATA *ch;
	CHAR_DATA *ch_next;
	EXIT_DATA *pexit;
	int door;

	/* Examine all mobs. */
	for (ch = char_list; ch != NULL; ch = ch_next)
	{
		ch_next = ch->next;

		if ((ch->race == race_lookup("deamon"))
			|| (ch->race == race_lookup("homonculus"))
			|| (ch->race == race_lookup("tree"))
			|| (ch->race == race_lookup("undead")))
		{
			if (ch->spec_fun != 0)
			{
				(*ch->spec_fun) (ch);
			}
			continue;
		}

/*  Hunting check to remove "old hunters" - Added by Del */

		if (ch->hunt_timer > 1)
		{
			ch->hunt_timer = ch->hunt_timer - 1;

			if (ch->hunt_timer == 1)
			{
				ch->hunt_timer = 0;
				ch->hunting = NULL;
			}
		}

/*  End changes  */

		if (!IS_NPC(ch) || ch->in_room == NULL || IS_AFFECTED(ch, AFF_CHARM))
			continue;

		if (ch->in_room->area->empty && !IS_SET(ch->act, ACT_UPDATE_ALWAYS))
			continue;

		/* Examine call for special procedure */
		if (ch->spec_fun != 0)
		{
			if ((*ch->spec_fun) (ch))
				continue;
		}

		if (ch->pIndexData->pShop != NULL)	/* give him some gold */
			if ((ch->gold * 100 + ch->silver) < ch->pIndexData->wealth)
			{
				ch->gold += ch->pIndexData->wealth * number_range(1, 20) / 5000000;
				ch->silver += ch->pIndexData->wealth * number_range(1, 20) / 50000;
			}


		/*
		 * Check triggers only if mobile still in default position
		 */
		if (ch->position == ch->pIndexData->default_pos)
		{
			/* Delay */
			if (HAS_TRIGGER(ch, TRIG_DELAY)
				&& ch->mprog_delay > 0)
			{
				if (--ch->mprog_delay <= 0)
				{
					//mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_DELAY);
					continue;
				}
			}
			if (HAS_TRIGGER(ch, TRIG_RANDOM))
			{
				/*if (mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_RANDOM))
					continue;*/
			}
		}


		// Fighting thin air is not cool.
		if (ch->fighting == NULL
		&& ch->position == POS_FIGHTING)
			ch->position = POS_STANDING;

		/* That's all for sleeping / busy monster, and empty zones */
		if (ch->position != POS_STANDING)
			continue;

		/* Scavenge */
		if (IS_SET(ch->act, ACT_SCAVENGER)
			&& ch->in_room->contents != NULL
			&& number_bits(6) == 0)
		{
			OBJ_DATA *obj;
			OBJ_DATA *obj_best;
			int max;

			max = 1;
			obj_best = NULL;
			for (obj = ch->in_room->contents; obj; obj = obj->next_content)
			{
				if (CAN_WEAR(obj, ITEM_TAKE)
				&& obj->item_type != ITEM_FURNITURE
				&& can_loot(ch, obj)
				&& obj->cost > max
				&& obj->cost > 0
				&& can_see_obj(ch, obj))
				{
					obj_best = obj;
					max = obj->cost;
				}
			}

			if (obj_best)
			{
				obj_from_room(obj_best);
				obj_to_char(obj_best, ch);
				act("$n gets $p.", ch, obj_best, NULL, TO_ROOM, POS_RESTING);
			}
		}

		/* Wander */
		if (!IS_SET(ch->act, ACT_SENTINEL)
			&& number_bits(3) == 0
			&& (door = number_bits(5)) <= 5
			&& (pexit = ch->in_room->exit[door]) != NULL
			&& pexit->u1.to_room != NULL
			&& (!IS_SET(pexit->exit_info, EX_CLOSED)
				|| is_affected(ch, gsn_pass_door))
			&& !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
			&& (!IS_SET(ch->act, ACT_STAY_AREA)
				|| pexit->u1.to_room->area == ch->in_room->area)
			&& (!IS_SET(ch->act, ACT_OUTDOORS)
				|| !IS_SET(pexit->u1.to_room->room_flags, ROOM_INDOORS))
			&& !(pexit->u1.to_room->sector_type == SECT_AIR && !IS_AFFECTED(ch, AFF_FLYING))
			&& (!IS_SET(ch->act, ACT_INDOORS)
				|| IS_SET(pexit->u1.to_room->room_flags, ROOM_INDOORS)))
		{
			move_char(ch, door, FALSE);
		}
	}

	return;
}



/*
 * Update the weather.
 */
void
weather_update(void)
{
	char buf[MAX_STRING_LENGTH];
	int diff;

	buf[0] = '\0';

	switch (++time_info.hour)
	{
	case 5:
		weather_info.sunlight = SUN_LIGHT;
		strcat(buf, "The day has begun.\n\r");
		break;

	case 6:
		weather_info.sunlight = SUN_RISE;
		strcat(buf, "The sun rises in the east.\n\r");
		break;

	case 19:
		weather_info.sunlight = SUN_SET;
		strcat(buf, "The sun slowly disappears in the west.\n\r");
		break;

	case 20:
		weather_info.sunlight = SUN_DARK;
		strcat(buf, "The night has begun.\n\r");
		break;

	case 24:
		time_info.hour = 0;
		time_info.day++;
		break;
	}

	if (time_info.day >= 35)
	{
		time_info.day = 0;
		time_info.month++;
	}

	if (time_info.month >= 17)
	{
		time_info.month = 0;
		time_info.year++;
	}

	/*
	 * Weather change.
	 */
	if (time_info.month >= 9 && time_info.month <= 16)
		diff = weather_info.mmhg > 985 ? -2 : 2;
	else
		diff = weather_info.mmhg > 1015 ? -2 : 2;

	check_weather(buf, diff);

	return;
}

void
check_weather(char *buf, int diff)
{
	DESCRIPTOR_DATA *d;

	weather_info.change += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
	weather_info.change = UMAX(weather_info.change, -12);
	weather_info.change = UMIN(weather_info.change, 12);

	weather_info.mmhg += weather_info.change;
	weather_info.mmhg = UMAX(weather_info.mmhg, 960);
	weather_info.mmhg = UMIN(weather_info.mmhg, 1040);

	switch (weather_info.sky)
	{
	default:
		bug("Weather_update: bad sky %d.", weather_info.sky);
		weather_info.sky = SKY_CLOUDLESS;
		break;

	case SKY_CLOUDLESS:
		if (weather_info.mmhg < 990
			|| (weather_info.mmhg < 1010 && number_bits(2) == 0))
		{
			strcat(buf, "The sky is getting cloudy.\n\r");
			weather_info.sky = SKY_CLOUDY;
		}
		break;

	case SKY_CLOUDY:
		if (weather_info.mmhg < 970
			|| (weather_info.mmhg < 990 && number_bits(2) == 0))
		{
			strcat(buf, "It starts to rain.\n\r");
			weather_info.sky = SKY_RAINING;
		}

		if (weather_info.mmhg > 1030 && number_bits(2) == 0)
		{
			strcat(buf, "The clouds disappear.\n\r");
			weather_info.sky = SKY_CLOUDLESS;
		}
		break;

	case SKY_RAINING:
		if (weather_info.mmhg < 970 && number_bits(2) == 0)
		{
			strcat(buf, "Lightning flashes in the sky.\n\r");
			weather_info.sky = SKY_LIGHTNING;
		}

		if (weather_info.mmhg > 1030
			|| (weather_info.mmhg > 1010 && number_bits(2) == 0))
		{
			strcat(buf, "The rain stopped.\n\r");
			weather_info.sky = SKY_CLOUDY;
		}
		break;

	case SKY_LIGHTNING:
		if (weather_info.mmhg > 1010
			|| (weather_info.mmhg > 990 && number_bits(2) == 0))
		{
			strcat(buf, "The lightning has stopped.\n\r");
			weather_info.sky = SKY_RAINING;
			break;
		}
		break;
	}

	if (buf[0] != '\0')
	{
		for (d = descriptor_list; d != NULL; d = d->next)
		{
			if (d->character != NULL)
			{
				if ((d->connected == CON_PLAYING || d->original != NULL)
					&& IS_OUTSIDE(d->character)
					&& IS_AWAKE(d->character))
					Cprintf(d->character, buf);
			}
		}
	}

	return;
}


// Returns true if target has died due to drowning. Sucker.
int check_drowning(CHAR_DATA *ch, int airused)
{
	AFFECT_DATA af;

	AFFECT_DATA *affect = get_room_affect(ch->in_room, gsn_flood);
	
	if (affect != NULL) {
	    // Protect non clanners in flooded rooms.
	    if (!clan_table[ch->clan].pkiller) {
            return FALSE;
        }
	    
	    if (!affect->clan && clan_table[ch->clan].pkiller) {
	        return FALSE;
	    }
	}

	if (!IS_NPC(ch)
          && ch->in_room
          && ch->in_room->sector_type == SECT_UNDERWATER
	      && !IS_IMMORTAL(ch)
          && !IS_AFFECTED(ch, AFF_WATER_BREATHING)) {
 		ch->air_supply += airused;
        if (ch->race == race_lookup("dwarf"))
            ch->air_supply += airused;

        // You're toast
        if (ch->air_supply > get_curr_stat(ch, STAT_CON) * 3 / 2)
        {
            Cprintf(ch, "{BYour bloated corpse sinks to the bottom...{x\n\r");
            act("$n has run out air and stops thrashing!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
            ch->hit = 1;
            damage(ch, ch, 50, 0, DAM_DROWNING, FALSE, TYPE_MAGIC);
            ch->air_supply = 0;
            return TRUE;
        }
        // Drowning quickly
        else if (ch->air_supply > get_curr_stat(ch, STAT_CON)) {
            Cprintf(ch, "{RYou're drowning!{x\n\r");
            act("$n begins to suffocate and drown!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
            af.where = TO_AFFECTS;
            af.type = gsn_hurricane;
            af.level = ch->level;
            af.duration = 5;
            af.modifier = -4;
            af.location = APPLY_CON;
            af.bitvector = 0;

            affect_join(ch, &af);
        }
        else if(ch->air_supply > get_curr_stat(ch, STAT_CON) / 2) {
            Cprintf(ch, "Your air supply is about half gone.\n\r");
            af.where = TO_AFFECTS;
            af.type = gsn_hurricane;
            af.level = ch->level;
            af.duration = 5;
            af.modifier = -1;
            af.location = APPLY_CON;
            af.bitvector = 0;

            affect_join(ch, &af);
        }
    }

	return FALSE;
}

void complete_craft(CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	char oname[255];

	// Now load the item and go wild.
	obj = create_object(get_obj_index(ch->craft_target), 0);
	if(clan_table[ch->clan].pkiller)
        	obj->clan_status = CS_CLANNER;
	else
		obj->clan_status = CS_NONCLANNER;

	Cprintf(ch, "You receive %s!\n\r", obj->short_descr);
	// Exp award for crafting.
	advance_craft(ch, obj);
	sprintf(oname, "%s %s", obj->name, ch->name);
	free_string(obj->name);
	obj->name = str_dup(oname);
	obj->special[4] = UMIN(100, 2 * obj->special[0]);
	obj->respawn_owner = str_dup(ch->name);
	obj_to_char(obj, ch);
	check_improve(ch, gsn_craft_item, TRUE, 1);
	log_string("Item crafted successfully vnum %d.", obj->pIndexData->vnum);

	return;
}

/*
 * Update all chars, including mobs.
 */
void
char_update(void)
{
	CHAR_DATA *ch;
	CHAR_DATA *ch_next;
	CHAR_DATA *ch_quit;
	OBJ_DATA *obj, *obj_next;
	ROOM_INDEX_DATA *location;
	DESCRIPTOR_DATA *d2;
	int x;
	int victim_dead = FALSE;
	ch_quit = NULL;

	for (ch = char_list; ch != NULL; ch = ch_next)
	{
		AFFECT_DATA *paf;
		AFFECT_DATA *paf_next;

		ch_next = ch->next;

		if (ch->timer > 30)
			ch_quit = ch;

		if (ch->can_lay > 0)
		{
			ch->can_lay++;
			if (ch->can_lay > 5)
			{
				Cprintf(ch, "Your healing powers are restored.\n\r");
				ch->can_lay = 0;
			}
		}

		if(!IS_NPC(ch)
		&& ch->air_supply > 0
		&& ch->in_room
		&& ch->in_room->sector_type != SECT_UNDERWATER) {
			ch->air_supply = UMAX(0, ch->air_supply - dice(2, 8));
			if(ch->air_supply == 0) {
				Cprintf(ch, "You feel ready to swim again, if need be.\n\r");
			}
		}

		// Check for drowning in under water rooms
		if(check_drowning(ch, dice(1, 6)))
			return;

		if (ch->in_room != NULL)
		{
			if (IS_SET(ch->in_room->room_flags, ROOM_FERRY) && !IS_NPC(ch))
			{
				ch->ferry_timer++;
				if (ch->ferry_timer < 4)
				{
					Cprintf(ch, "A ferryman yells '%d more hours before we dock!'\n\r", 4 - ch->ferry_timer);
				}

				if (ch->ferry_timer > 3)
				{
					if (ch->in_room->area->continent != 0)
					{
						location = get_room_index(ROOM_VNUM_SHORE_TERRA);
					}
					else
					{
						location = get_room_index(ROOM_VNUM_SHORE_DOMINIA);
					}

					if (ch->fighting != NULL)
						stop_fighting(ch, TRUE);

					ch->ferry_timer = 0;
					act("$n gets off the ferry boat.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
					char_from_room(ch);
					char_to_room(ch, location);
					do_look(ch, "auto");
				}
			}
			else if (!IS_NPC(ch))
				ch->ferry_timer = 0;
		}

		if (get_age(ch) > 100)
			x = 10;
		else if (get_age(ch) > 80)
			x = 9;
		else if (get_age(ch) > 60)
			x = 8;
		else if (get_age(ch) > 50)
			x = 7;
		else if (get_age(ch) > 40)
			x = 6;
		else if (get_age(ch) > 30)
			x = 5;
		else if (get_age(ch) > 25)
			x = 4;
		else
			x = 3;

		x *= 10;

		if (ch->breath < x)
		{
			ch->breath += UMIN((x / 15), 3);
			if (ch->breath > x)
				ch->breath = x;
		}

		if (ch->desc)
			if (ch->desc->connected < CON_NOTE_TO || ch->desc->connected > CON_NOTE_FINISH)
				if (ch->pktimer == 1)
					Cprintf(ch, "You can do battle again.\n\r");

		if (ch->pktimer > 0)
			ch->pktimer--;

		// New craft code
		if(ch->craft_timer > 0) {
			ch->craft_timer--;
			if(ch->craft_timer == 0)
				Cprintf(ch, "You may now craft items again.\n\r");
		}

		if(ch->craft_timer < 0) {
			ch->craft_timer++;
			if(ch->craft_timer == 0) {
				Cprintf(ch, "Your item is complete!\n\r");
				complete_craft(ch);
			}
			else {
				Cprintf(ch, "You make progress on your crafted item.\n\r");
				if(ch->craft_timer == -3)
					Cprintf(ch, "Your item is about halfway done.\n\r");
				if(ch->craft_timer == -1)
					Cprintf(ch, "Your item is almost complete!\n\r");
			}
		}

		if (ch->in_room == NULL)
		{
			location = get_room_index(ROOM_VNUM_LIMBO);
			char_to_room(ch, location);
			if (ch->desc)
				if (ch->desc->connected < CON_NOTE_TO || ch->desc->connected > CON_NOTE_FINISH)
					Cprintf(ch, "You are now in limbo.\n\r");

			log_string("Character %s was in a NULL room", IS_NPC(ch) ? ch->short_descr : ch->name);
		}

		/* Rot some undead */
                if (ch->rot_timer > 1) {
                        ch->rot_timer--;
                }
                if (ch->rot_timer == 1) {
                        act("$n crumbles into a pile of fine ash.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			/* drop everything */
			for(obj = ch->carrying; obj != NULL; obj = obj_next) {
                        	obj_next = obj->next_content;
                        	obj_from_char(obj);
                        	if(ch->in_room != NULL)
                                	obj_to_room(obj, ch->in_room);
                	}
                        extract_char(ch, TRUE);
			continue;
                }

		if (ch->position >= POS_STUNNED)
		{
			if (!IS_SET(ch->act, ACT_PET))
			{
				/* check to see if we need to go home */
				if (IS_NPC(ch)
					&& ch->is_clone == FALSE
					&& ch->in_room->area != NULL
					&& ch->pIndexData->area != ch->in_room->area
					&& ch->fighting == NULL
					&& ch->hunting == NULL
					&& ch->spec_fun != spec_lookup("spec_selina")
					&& !is_name("assault", ch->name)
					&& !IS_AFFECTED(ch, AFF_CHARM))
				{
				    if (ch->wander_timer > 0) {
				        ch->wander_timer--;
				    } else if (number_percent() < 10) {
				        act("$n wanders on home.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
				        extract_char(ch, TRUE);
				        continue;
				    }
				}
			}
			if (ch->hit < MAX_HP(ch))
				ch->hit += hit_gain(ch);
			else
				ch->hit = MAX_HP(ch);

			if (ch->mana < MAX_MANA(ch))
				ch->mana += mana_gain(ch);
			else
				ch->mana = MAX_MANA(ch);

			if (ch->move < MAX_MOVE(ch))
				ch->move += move_gain(ch);
			else
				ch->move = MAX_MOVE(ch);
		}

		if (ch->position == POS_STUNNED)
			update_pos(ch);

		/* stone sleep is nice, but could cause you to fall asleep */
		if (((ch->position == POS_STANDING) || (ch->position == POS_RESTING))
			&& (get_skill(ch, gsn_stone_sleep) > 0))
		{
			if (((time_info.hour > 4) && (time_info.hour < 20))
				&& (number_bits(2) == 0))
			{
				if (number_percent() == 1)
				{
					Cprintf(ch, "You feel the stone calling as you fall asleep!\n\r");
					act("$n stiffens and falls asleep!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
					ch->position = POS_SLEEPING;
				}
			}
		}

		// charged and daylight items
                if (!IS_NPC(ch)
                && time_info.hour == 8) {
                        OBJ_DATA *obj = NULL;
                        AFFECT_DATA *paf = NULL;

                        for(obj = ch->carrying; obj != NULL; obj = obj->next_content) {
                                if (obj->wear_loc != WEAR_NONE) {
                                        for(paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) {
                                                if(IS_SET(paf->bitvector, AFF_DARKLIGHT))
                                                        affect_modify(ch, paf, FALSE);
                                                if(IS_SET(paf->bitvector, AFF_DAYLIGHT))
                                                        affect_modify(ch, paf, TRUE);
                                        }
                                }

				// Items recharged 1 charge per 12 ticks.
                                if(IS_SET(obj->wear_flags, ITEM_CHARGED)
                                && obj->special[2] < obj->special[1]
                                && obj->wear_loc != WEAR_NONE) {
					Cprintf(ch, "%s is warmed by the morning light.\n\r", obj->short_descr);
                                        obj->special[2]++;
				}
                        }
		}

                if (!IS_NPC(ch)
                && time_info.hour == 20) {
                        OBJ_DATA *obj = NULL;
                        AFFECT_DATA *paf = NULL;

                        for(obj = ch->carrying; obj != NULL; obj = obj->next_content) {
                                if (obj->wear_loc != WEAR_NONE) {
                                        for(paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) {
                                                if(IS_SET(paf->bitvector, AFF_DARKLIGHT))
                                                        affect_modify(ch, paf, TRUE);
                                                if(IS_SET(paf->bitvector, AFF_DAYLIGHT))
                                                        affect_modify(ch, paf, FALSE);
                                        }
                                }

				// Items recharged 1 charge per 12 ticks.
                                if(IS_SET(obj->wear_flags, ITEM_CHARGED)
                                && obj->special[2] < obj->special[1]
                                && obj->wear_loc != WEAR_NONE) {
					Cprintf(ch, "%s sparkles in the moonlight.\n\r", obj->short_descr);
                                        obj->special[2]++;
				}
                        }
		}

		if (!IS_NPC(ch) && ch->level < LEVEL_IMMORTAL)
		{
			OBJ_DATA *obj;

			if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
				&& obj->item_type == ITEM_LIGHT
				&& obj->value[2] > 0)
			{
				if (--obj->value[2] == 0 && ch->in_room != NULL)
				{
					--ch->in_room->light;
					act("$p goes out.", ch, obj, NULL, TO_ROOM, POS_RESTING);
					act("$p flickers and goes out.", ch, obj, NULL, TO_CHAR, POS_RESTING);
					extract_obj(obj);
				}
				else if (obj->value[2] <= 5 && ch->in_room != NULL)
					act("$p flickers.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			}

			if (ch->level > 53)
				ch->timer = 0;

			if (++ch->timer >= 12)
			{
				if (ch->timer > 35)

				{
					if (ch->desc != NULL)
					{
						d2 = ch->desc;
						extract_char(ch, TRUE);
						if (d2 != NULL)
							close_socket(d2);
						return;
					}
				}

				if(ch->in_room != get_room_index(ROOM_VNUM_LIMBO)
				&& ch->in_room != get_room_index(ROOM_VNUM_LIMBO_DOMINIA))
				{
					if (ch->in_room != NULL)
					{
						ch->was_in_room = ch->in_room;
						if (ch->fighting != NULL)
							stop_fighting(ch, TRUE);
						act("$n disappears into the void.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
						Cprintf(ch, "You disappear into the void.\n\r");
						if (ch->level > 1)
							save_char_obj(ch, FALSE);
						if (ch->in_room != NULL && ch->in_room->area->continent == 0)
							location = get_room_index(ROOM_VNUM_LIMBO);
						else
							location = get_room_index(ROOM_VNUM_LIMBO_DOMINIA);
						char_from_room(ch);
						char_to_room(ch, location);
					}
					else
					{
						ch->was_in_room = ch->in_room;
						if (ch->fighting != NULL)
							stop_fighting(ch, TRUE);
						act("$n disappears into the void.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
						Cprintf(ch, "You disappear into the void.\n\r");
						if (ch->level > 1)
							save_char_obj(ch, FALSE);
						if (ch->in_room != NULL && ch->in_room->area->continent == 0)
							location = get_room_index(ROOM_VNUM_LIMBO);
						else
							location = get_room_index(ROOM_VNUM_LIMBO_DOMINIA);
						char_from_room(ch);
						char_to_room(ch, location);
					}
				}
			}

			/* Mystics gotta meditate! */
			if(!IS_NPC(ch)
			&& ch->reclass == reclass_lookup("mystic")
			&& time_info.hour == 23) {
				Cprintf(ch, "{CYour need for meditation grows.{x\n\r");
				ch->meditate_needed += 3;
				// Minimum caster level of 1
				if(ch->meditate_needed > ch->level - 1)
					ch->meditate_needed = ch->level - 1;
			}

			if(ch->reclass == reclass_lookup("mystic")
			&& ch->position == POS_RESTING) {
				if(ch->meditate_needed > 0) {
					Cprintf(ch, "You are filled with inner peace as you meditate.\n\r");
					ch->meditate_needed--;
					if(ch->meditate_needed == 0)
						Cprintf(ch, "You have fulfilled your need to meditate...for now.\n\r");
				}
			}

			// Reuse of the thrust counter as a wail counter
		        if(get_skill(ch, gsn_wail) > 0
			&& ch->thrustCounter > 0) {
				ch->thrustCounter = 0;
				Cprintf(ch, "You feel ready to wail again.");
			}

			gain_condition(ch, COND_DRUNK, -1);
			gain_condition(ch, COND_FULL, ch->size > SIZE_MEDIUM ? -4 : -2);
			if(number_percent() > get_skill(ch, gsn_iron_vigil)) {
				if (race_lookup("human") == ch->race)
				{
					if (ch->remort > 0)
					{
						if (dice(1, 2) == 2)
							gain_condition(ch, COND_THIRST, -1);
					}
				}

				// Marid remort penalty
				if (ch->race == race_lookup("marid")
				&& ch->remort)
					gain_condition(ch, COND_THIRST, 0 - (dice(1, 12)));
				else
					gain_condition(ch, COND_THIRST, -1);

				gain_condition(ch, COND_HUNGER, ch->size > SIZE_MEDIUM ? -2 : -1);

				if (race_lookup("human") == ch->race)
				{
					if (ch->remort > 0)
					{
						if (dice(1, 2) == 2)
							gain_condition(ch, COND_HUNGER, -1);
					}
				}
			}
			else {
				/* Iron vigil! No more hunger */
				check_improve(ch, gsn_iron_vigil, TRUE, 4);
			}
		}

		for (paf = ch->affected; paf != NULL; paf = paf_next)
		{
			paf_next = paf->next;

			if (paf->type == gsn_crushing_hand
			|| (paf->type == gsn_razor_claws
			&& paf->modifier == 1))
				continue;

			if(paf->type == gsn_surge) {
				paf->duration--;
			}

			if (paf->duration > 0)
			{
				if (!is_affected(ch, gsn_surge))
                                {
             				paf->duration--;
                                        if (number_range(0, 4) == 0 && paf->level > 0)
                                                paf->level--;
                                }

			}
			else if (paf->duration < 0)
				;
			else
			{
				if (paf_next == NULL
					|| paf_next->type != paf->type
					|| paf_next->duration > 0)
				{
					if (paf->type > 0 && skill_table[paf->type].msg_off)
					{
						Cprintf(ch, "%s\n\r", skill_table[paf->type].msg_off);
					}
				}
				/* Call a funky end function */
				if(skill_table[paf->type].end_fun != end_null)
					skill_table[paf->type].end_fun((void*)ch, TARGET_CHAR);
				/* remove aff, AFTER.. might need it in end_func*/
				affect_remove(ch, paf);
			}
		}

		if (ch->outcast_timer > 0)
		{
			ch->outcast_timer--;
			if (ch->outcast_timer == 0)
			{
				Cprintf(ch, "You are now able to join a clan again.\n\r");
				ch->clan = clan_lookup("loner");
				ch->outcast_timer = -1;
			}
		}

		if (ch->outcast_timer < 0)
		{
			ch->outcast_timer--;
		}

		/*
		 * Careful with the damages here,
		 *   MUST NOT refer to ch after damage taken,
		 *   as it may be lethal damage (on NPC).
		 */
		// We can ignore this now, damage returns true if the
		// damage WAS lethal, so stack like a bitch.
		if (ch != NULL && ch->race == race_lookup("troll")
                && time_info.hour >= 11 && time_info.hour <= 15
                && !IS_NPC(ch)
                && ch->remort > 0
		&& ch->level < LEVEL_IMMORTAL
                && ch->in_room != NULL
		&& ch->in_room != get_room_index(ROOM_VNUM_LIMBO)
                && ch->in_room != get_room_index(ROOM_VNUM_LIMBO_DOMINIA)
		&& ch->pktimer == 0)
                {
			if (ch->in_room->sector_type != SECT_INSIDE
                        && !IS_SET(ch->in_room->room_flags, ROOM_INDOORS))
                        {
                                act("$n is struck by the power of the sun.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
                                Cprintf(ch, "You are struck by the power of the sun.\n\r");
                                victim_dead = damage(ch, ch, (int) (ch->level * 2), TYPE_UNDEFINED, DAM_LIGHT, FALSE, TYPE_MAGIC);
                        }
                }
		if (!victim_dead
		&& ch->race == race_lookup("marid")
		&& ch->remort
		&& ch->pcdata->condition[COND_THIRST] == 0
		&& ch->pktimer == 0
		&& ch->level < LEVEL_IMMORTAL
		&& ch->in_room
		&& ch->in_room != get_room_index(ROOM_VNUM_LIMBO)
		&& ch->in_room != get_room_index(ROOM_VNUM_LIMBO_DOMINIA)
		&& ch->in_room->sector_type != SECT_UNDERWATER
		&& ch->in_room->sector_type != SECT_WATER_SWIM
		&& ch->in_room->sector_type != SECT_WATER_NOSWIM) {
			Cprintf(ch, "You writhe in pain as your physical form evaporates!\n\r");
			act("$n begins to evaporate painfully.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			victim_dead = damage(ch, ch, number_range(1, ch->level * 2), TYPE_UNDEFINED, DAM_LIGHT, FALSE, TYPE_MAGIC);
		}

		if(!victim_dead
		&& ch != NULL
		&& ch->reclass == reclass_lookup("sorceror")
		&& ch->level < LEVEL_IMMORTAL
        	&& ch->mana >= MAX_MANA(ch)
		&& number_percent() < 25) {
                	Cprintf(ch, "The magical power within you flares and burns!\n\r");
			act("$n is momentarily consumed in eldrich flames.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			victim_dead = damage(ch, ch, (int)(MAX_MANA(ch) / 25), TYPE_UNDEFINED, DAM_ENERGY, FALSE, TYPE_MAGIC);
        	}
		if (!victim_dead
		&& ch != NULL
		&& is_affected(ch, gsn_plague)
		&& ch->in_room != NULL
		&& ch->in_room != get_room_index(ROOM_VNUM_LIMBO)
		&& ch->in_room != get_room_index(ROOM_VNUM_LIMBO_DOMINIA))
		{
			AFFECT_DATA *af, plague;
			CHAR_DATA *vch;
			int dam;

			act("$n writhes in agony as plague sores erupt from $s skin.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

			if (ch->desc)
				if (ch->desc->connected < CON_NOTE_TO || ch->desc->connected > CON_NOTE_FINISH)
					Cprintf(ch, "You writhe in agony from the plague.\n\r");

			af = affect_find(ch->affected, gsn_plague);
			if (af == NULL)
				return;

			if (af->level == 1)
				return;

			plague.where = TO_AFFECTS;
			plague.type = gsn_plague;
			plague.level = af->level - 1;
			plague.duration = number_range(1, af->level / 2);
			plague.location = APPLY_STR;
			plague.modifier = af->modifier;
			plague.bitvector = AFF_PLAGUE;

			for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
			{
				if (number_percent() <= 3
					&& !IS_IMMORTAL(vch)
					&& (!IS_SET(vch->act, ACT_IS_WIZI) && IS_NPC(vch))
					&& !IS_AFFECTED(vch, AFF_PLAGUE)
					&& check_immune(vch, DAM_DISEASE) != IS_IMMUNE)
				{
					if (ch->desc)
						if (ch->desc->connected < CON_NOTE_TO || ch->desc->connected > CON_NOTE_FINISH)
							Cprintf(vch, "You feel hot and feverish.\n\r");
					act("$n shivers and looks very ill.", vch, NULL, NULL, TO_ROOM, POS_RESTING);
					affect_join(vch, &plague);
				}
			}

			/* plague deals 2-5% damage now */
			dam = 1 + (ch->hit / number_range(20, 50));
			ch->mana -= dam / 2;
			ch->move -= dam / 2;
			victim_dead = damage(ch, ch, dam, gsn_plague, DAM_DISEASE, FALSE, TYPE_MAGIC);
		}
		if (!victim_dead
		&& IS_AFFECTED(ch, AFF_POISON) && ch != NULL
		&& !IS_AFFECTED(ch, AFF_SLOW)
		&& ch->in_room != NULL && ch->in_room != get_room_index(ROOM_VNUM_LIMBO)
		&& ch->in_room != get_room_index(ROOM_VNUM_LIMBO_DOMINIA))
		{
			AFFECT_DATA *poison;

			poison = affect_find(ch->affected, gsn_poison);

			if (poison != NULL)
			{
				act("$n shivers and suffers.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
				Cprintf(ch, "You shiver and suffer.\n\r");
				// Poison is 3-6% damage per tick.
				victim_dead = damage(ch, ch, 1 + (ch->hit / number_range(15, 30)), gsn_poison, DAM_POISON, FALSE, TYPE_MAGIC);
			}
		}

		else if (!victim_dead
		&& ch->position == POS_INCAP && number_range(0, 1) == 0)
		{
			damage(ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, FALSE, TYPE_MAGIC);
		}
		else if (!victim_dead
		&& ch->position == POS_MORTAL)
		{
			damage(ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, FALSE, TYPE_MAGIC);
		}
	}

	for (ch = char_list; ch != NULL; ch = ch_next)
	{
		ch_next = ch->next;

		if (ch->no_quit_timer > 0)
			ch->no_quit_timer--;

		if (ch->position == POS_SLEEPING)
		{
			if (ch->desc)
			{
				if ((ch->desc->connected < CON_NOTE_TO) || (ch->desc->connected > CON_NOTE_FINISH))
					Cprintf(ch, "\n");
			}
		}
	}
	/*
	 * Autosave and autoquit.
	 * Check that these chars still exist.
	 */
	for (ch = char_list; ch != NULL; ch = ch_next)
	{
		ch_next = ch->next;

		if (ch->desc != NULL && number_percent() < 10)
			save_char_obj(ch, FALSE);

		if (ch == ch_quit)
			quit_character(ch);
	}

	return;
}




/*
 * Update all objs.
 * This function is performance sensitive.
 */
void
obj_update(void)
{
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	AFFECT_DATA *paf, *paf_next;

	for (obj = object_list; obj != NULL; obj = obj_next)
	{
		CHAR_DATA *rch;
		char *message;

		obj_next = obj->next;

		/* go through affects and decrement */
		for (paf = obj->affected; paf != NULL; paf = paf_next)
		{
			paf_next = paf->next;
			if (paf->duration > 0)
			{
				paf->duration--;
				if (number_range(0, 4) == 0 && paf->level > 0)
					paf->level--;	/* spell strength fades with time */
			}
			else if (paf->duration < 0)
				;
			else
			{
				if (paf_next == NULL
					|| paf_next->type != paf->type
					|| paf_next->duration > 0)
				{
					if (paf->type > 0 && skill_table[paf->type].msg_obj)
					{
						if (obj->carried_by != NULL)
						{
							rch = obj->carried_by;
							act(skill_table[paf->type].msg_obj, rch, obj, NULL, TO_CHAR, POS_RESTING);
						}
						if (obj->in_room != NULL
							&& obj->in_room->people != NULL)
						{
							rch = obj->in_room->people;
							act(skill_table[paf->type].msg_obj, rch, obj, NULL, TO_ALL, POS_RESTING);
						}
					}
				}

				// Currently there are no object spells that
				// use end_funs... if there is any in the
				// Future, this is the place to add the call.

				affect_remove_obj(obj, paf);
			}
		}						/* end for */

		// Objects falling in mid-air rooms
		if (obj->in_room
		&& obj->in_room->sector_type == SECT_AIR
		&& obj->wear_flags & ITEM_TAKE
		&& obj->in_room->exit[DIR_DOWN]
		&& obj->in_room->exit[DIR_DOWN]->u1.to_room)
		{
			ROOM_INDEX_DATA *new_room = obj->in_room->exit[DIR_DOWN]->u1.to_room;
			if (obj->in_room->people)
				act("$p falls away.", obj->in_room->people, obj, NULL, TO_ALL, POS_RESTING);
			obj_from_room(obj);
			obj_to_room(obj, new_room);
			if (obj->in_room->people)
				act("$p falls into the room.", obj->in_room->people, obj, NULL, TO_ALL, POS_RESTING);
		}

		// Object underwater... some float, some sink, depends on material.
	        if (obj->in_room
	        && (obj->in_room->sector_type == SECT_WATER_SWIM
		||  obj->in_room->sector_type == SECT_WATER_NOSWIM
		||  obj->in_room->sector_type == SECT_UNDERWATER)
		&& !MATERIAL_FLOATS(obj->material)
	        && obj->wear_flags & ITEM_TAKE
	        && obj->in_room->exit[DIR_DOWN]
	        && obj->in_room->exit[DIR_DOWN]->u1.to_room)
	        {
	                ROOM_INDEX_DATA *new_room = obj->in_room->exit[DIR_DOWN]->u1.to_room;
	                if (obj->in_room->people)
        	                act("$p sinks deeper into the water.", obj->in_room->people, obj, NULL, TO_ALL, POS_RESTING);
	                obj_from_room(obj);
	                obj_to_room(obj, new_room);
	                if (obj->in_room->people)
        	                act("$p floats into the room.", obj->in_room->people, obj, NULL,TO_ALL, POS_RESTING);
	        }

	        if (obj->in_room
	        && obj->in_room->sector_type == SECT_UNDERWATER
		&& MATERIAL_FLOATS(obj->material)
	        && obj->wear_flags & ITEM_TAKE
	        && obj->in_room->exit[DIR_UP]
	        && obj->in_room->exit[DIR_UP]->u1.to_room)
	        {
	                ROOM_INDEX_DATA *new_room = obj->in_room->exit[DIR_UP]->u1.to_room;
	                if (obj->in_room->people)
	                        act("$p floats towards the surface.", obj->in_room->people, obj, NULL, TO_ALL, POS_RESTING);
	                obj_from_room(obj);
	                obj_to_room(obj, new_room);
	                if (obj->in_room->people)
	                        act("$p floats into the room from below.", obj->in_room->people, obj, NULL, TO_ALL, POS_RESTING);
	        }

		if (obj->timer <= 0 || --obj->timer > 0)
			continue;

		switch (obj->item_type)
		{
		default:
			message = "$p crumbles into dust.";
			break;
		case ITEM_FOUNTAIN:
			message = "$p dries up.";
			break;
		case ITEM_CORPSE_NPC:
			message = "$p decays into dust.";
			break;
		case ITEM_CORPSE_PC:
			message = "$p decays into dust.";
			break;
		case ITEM_FOOD:
			message = "$p decomposes.";
			break;
		case ITEM_POTION:
			message = "$p has evaporated from disuse.";
			break;
		case ITEM_PORTAL:
			message = "$p fades out of existence.";
			break;
		case ITEM_CONTAINER:
			if (CAN_WEAR(obj, ITEM_WEAR_FLOAT))
				if (obj->contains)
					message =
						"$p flickers and vanishes, spilling its contents on the floor.";
				else
					message = "$p flickers and vanishes.";
			else
				message = "$p crumbles into dust.";
			break;
		}

		if (obj->carried_by != NULL)
		{
			if (IS_NPC(obj->carried_by)
				&& obj->carried_by->pIndexData->pShop != NULL)
				obj->carried_by->silver += obj->cost / 5;
			else
			{
				act(message, obj->carried_by, obj, NULL, TO_CHAR, POS_RESTING);
				if (obj->wear_loc == WEAR_FLOAT)
					act(message, obj->carried_by, obj, NULL, TO_ROOM, POS_RESTING);
			}
		}
		else if (obj->in_room != NULL
				 && (rch = obj->in_room->people) != NULL)
		{
			if (!(obj->in_obj && obj->in_obj->pIndexData->vnum == OBJ_VNUM_PIT
				  && !CAN_WEAR(obj->in_obj, ITEM_TAKE)))
			{
				act(message, rch, obj, NULL, TO_ROOM, POS_RESTING);
				act(message, rch, obj, NULL, TO_CHAR, POS_RESTING);
			}
		}

		if ((obj->item_type == ITEM_CORPSE_PC || obj->wear_loc == WEAR_FLOAT)
			&& obj->contains)
		{						/* save the contents */
			OBJ_DATA *t_obj, *next_obj;

			for (t_obj = obj->contains; t_obj != NULL; t_obj = next_obj)
			{
				next_obj = t_obj->next_content;
				obj_from_obj(t_obj);

				if (obj->in_obj)	/* in another object */
					obj_to_obj(t_obj, obj->in_obj);
				else if (obj->carried_by)	/* carried */
					if (obj->wear_loc == WEAR_FLOAT)
						if (obj->carried_by->in_room == NULL)
							extract_obj(t_obj);
						else
							obj_to_room(t_obj, obj->carried_by->in_room);
					else
						obj_to_char(t_obj, obj->carried_by);
				else if (obj->in_room == NULL)	/* destroy it */
					extract_obj(t_obj);
				else			/* to a room */
					obj_to_room(t_obj, obj->in_room);
			}
		}

		extract_obj(obj);
	}

	return;
}



/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void
aggr_update(void)
{
	CHAR_DATA *wch;
	CHAR_DATA *wch_next;
	CHAR_DATA *ch;
	CHAR_DATA *ch_next;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	CHAR_DATA *victim;
	int i;

	for (wch = char_list; wch != NULL; wch = wch_next)
	{
		wch_next = wch->next;
		if (IS_NPC(wch)
			|| wch->level >= LEVEL_IMMORTAL
			|| wch->in_room == NULL
			|| wch->in_room->area->empty)
			continue;

		i = 0;
		for (ch = wch->in_room->people; ch != NULL; ch = ch_next)
		{
			int count;

			if (i > 200)
			{
				log_string("INFINTE loop in aggr_update");
				break;
			}

			i++;
			ch_next = ch->next_in_room;
			if (ch == ch_next)
			{
				log_string("INFINTE loop in aggr_update");
				break;
			}

			if (ch->in_room == NULL)
				continue;

			if (!IS_NPC(ch)
				|| !IS_SET(ch->act, ACT_AGGRESSIVE)
				|| IS_SET(ch->in_room->room_flags, ROOM_SAFE)
				|| IS_AFFECTED(ch, AFF_CALM)
				|| ch->fighting != NULL
				|| IS_AFFECTED(ch, AFF_CHARM)
				|| !IS_AWAKE(ch)
				|| (IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch))
				|| !can_see(ch, wch)
				|| number_bits(1) == 0)
				continue;


			/* give sneaking people a chance */
			if (IS_AFFECTED(wch, AFF_SNEAK))
			{
				if (dice(1, 6) == 4)
					continue;
			}

			/*
			 * Ok we have a 'wch' player character and a 'ch' npc aggressor.
			 * Now make the aggressor fight a RANDOM pc victim in the room,
			 *   giving each 'vch' an equal chance of selection.
			 */
			count = 0;
			victim = NULL;
			for (vch = wch->in_room->people; vch != NULL; vch = vch_next)
			{
				vch_next = vch->next_in_room;

				if (!IS_NPC(vch)
					&& vch->level < LEVEL_IMMORTAL
					&& ch->level >= vch->level - 5
					&& (!IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch))
					&& can_see(ch, vch)
					&& !is_affected(vch, gsn_cloak_of_mind))
				{
					if (number_range(0, count) == 0)
						victim = vch;
					count++;
				}
			}

			if (victim == NULL)
				continue;

			multi_hit(ch, victim, TYPE_UNDEFINED);
		}
	}

	return;
}


/*
 * Deal with mob sleep.
 */

void sleep_update(void)
{
	return;
	/*
	SLEEP_DATA *temp;// = first_sleep;
	SLEEP_DATA *temp_next;

	for(; temp != NULL; temp = temp_next)
	{
		bool delete = FALSE;

		temp_next = temp->next;

		/* checks to make sure the mob still exists 
		if(!temp->mob)
			delete = TRUE;
		/*checks to make sure the char is still in the same room as the mob 
		else if (temp->mob && temp->ch && temp->mob->in_room != temp->ch->in_room)
			delete = TRUE;

		if (delete)
		{
			if (temp->prev)
				temp->prev->next = temp->next;
			if (temp->next)
				temp->next->prev = temp->prev;
			/*if (temp == first_sleep && (temp->next == NULL || temp->prev == NULL))
				first_sleep = temp->next;
			free_sleep_data(temp);
			continue;
		}

		if(--temp->timer <= 0)
		{
			program_flow(temp->vnum, temp->prog, temp->mob, temp->ch, NULL, NULL, temp->line);

			if (temp->prev)
				temp->prev->next = temp->next;
			if (temp->next)
				temp->next->prev = temp->prev;
			/*if (temp == first_sleep && (temp->next == NULL || temp->prev == NULL))
				first_sleep = temp->next;
			free_sleep_data(temp);
		}
	}*/
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

void
update_handler(void)
{
	static int pulse_area;
	static int pulse_mobile;
	static int pulse_violence;
	static int pulse_rain;
	static int pulse_point;
	static int pulse_music;
	static int pulse_hunt;

	if (--pulse_area <= 0)
	{
		pulse_area = PULSE_AREA;
		area_update();
		olcautosave();
	}

	if (--pulse_music <= 0)
	{
		pulse_music = PULSE_MUSIC;
		song_update();
	}

	if (--pulse_mobile <= 0)
	{
		pulse_mobile = PULSE_MOBILE;
		mobile_update();
	}

	if (--pulse_violence <= 0)
	{
		pulse_violence = PULSE_VIOLENCE;
		violence_update();
	}

	if (--pulse_rain <= 0)
	{
		pulse_rain = PULSE_RAIN;
		rain_update();
	}

	if (--pulse_point <= 0)
	{
		wiznet("TICK!", NULL, NULL, WIZ_TICKS, 0, 0);
		pulse_point = PULSE_TICK;
/* number_range( PULSE_TICK / 2, 3 * PULSE_TICK / 2 ); */
		weather_update();
		char_update();
		obj_update();
		quest_update();
		room_update();
		spectral_update();
		tip_update();
		call_lightning_update();
	}

	if (pulse_hunt == 1)
	{
		CHAR_DATA* dog;

		pulse_hunt = 0;

		for (dog = char_list; dog != NULL; dog = dog->next)
		{
			if (IS_NPC(dog) &&
			    IS_AWAKE(dog) &&
			    dog->hunting != NULL &&
			    dog->pIndexData->vnum == MOB_VNUM_HUNT_DOG)
			{
				hunt_victim(dog);
			}
		}
	}
	else
	{
		pulse_hunt = 1;
	}

	null_room_update();

	auction_update();
	aggr_update();
	sleep_update();

	tail_chain();
	return;
}

void
room_update(void)
{
	AFFECT_DATA *paf;
	CHAR_DATA *rch;
	AFFECT_DATA *paf_next;
	ROOM_INDEX_DATA *room;
	long vnum;


	for (vnum = 0; vnum < 32000; vnum++)
	{
		room = get_room_index(vnum);
		if (room == NULL)
			continue;

		/* go through affects and decrement */
		for (paf = room->affected; paf != NULL; paf = paf_next)
		{
			paf_next = paf->next;
			if (paf->duration > 0)
			{
				paf->duration--;
				if (number_range(0, 4) == 0 && paf->level > 0)
					paf->level--;	/* spell strength fades with time */
			}
			else if (paf->duration < 0)
				;
			else
			{
				if (paf_next == NULL
					|| paf_next->type != paf->type
					|| paf_next->duration > 0)
				{
					if (paf->type > 0 && skill_table[paf->type].msg_off)
					{
						if (room->people != NULL)
						{
							rch = room->people;
							act(skill_table[paf->type].msg_off, rch, NULL, NULL, TO_ALL, POS_RESTING);
						}
					}
				}

				/* Call a funky end function */
				if(skill_table[paf->type].end_fun != end_null)
					skill_table[paf->type].end_fun((void*)room, TARGET_ROOM);

				affect_remove_room(room, paf);
			}
		}						/* end for */
	}
}

void
null_room_update(void)
{
	CHAR_DATA *ch;

	for (ch = char_list; ch != NULL; ch = ch->next)
	{
		if (ch->in_room == NULL)
		{
			Cprintf(ch, "Null room update!\n\r");
			if (ch->next_in_room != NULL &&
				ch->next_in_room->in_room != NULL)
			{
				/* atempt to recover */
				char_to_room(ch, ch->next_in_room->in_room);
			}
			else
			{
				char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
				if (ch->desc)
					if (ch->desc->connected < CON_NOTE_TO || ch->desc->connected > CON_NOTE_FINISH)
						Cprintf(ch, "You are now in limbo\n\r");

				log_string("Character %s was in a NULL room", IS_NPC(ch) ? ch->short_descr : ch->name);
			}
		}
	}
}

void
spectral_update() {
	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch;
	AFFECT_DATA af;

	for (d = descriptor_list; d != NULL; d = d->next) {
		ch = d->character;

		if (ch != NULL && ch->in_room != NULL
		   && ch->race == race_lookup("troll")
		   && ch->rem_sub == 2
		   && !is_affected(ch, gsn_oculary)) {

			af.where = TO_AFFECTS;
			af.type = gsn_oculary;
			af.level = ch->level;
			af.duration = -1;
			af.location = 0;
			af.modifier = 0;
			af.bitvector = 0;
			affect_to_char(ch, &af);
			Cprintf(ch, "You ripple out of existence.\n\r");
			act("$n ripples out of existence.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
	}
	return;

}

void
olcautosave(void)
{
	AREA_DATA *pArea;

	/*save_area_list();*/

	for( pArea = area_first; pArea != NULL; pArea = pArea->next )
	{
		/* Save changed areas. */
		if ( IS_SET( pArea->area_flags, AREA_CHANGED ) )
		{
			//save_area( pArea );
		}

		REMOVE_BIT( pArea->area_flags, AREA_CHANGED );
	}
}

// Get system to spit out a random type once in awhile.
void tip_update() {
	int tip = number_range(1, top_tips);
	TIP_DATA *pTip = first_tip;
	int i;

	// No tips loaded
	if(top_tips == 0)
		return;

	// Doesn't happen all the time
	if(number_percent() < 75)
		return;

	// Present a random tip
	for(i = 1; i <= tip; i++) {
		if(i == tip) {
			do_newbie_channel(System, pTip->message);
			return;
		}
		pTip = pTip->next;
	}
}

// This procedure correctly computes exp deductions such
// as patron points and int weapons, and then calls the regular
// group_gain.
void exp_reward(CHAR_DATA *ch, int xp, int show_message)
{
	int weapon_count = 0;
	OBJ_DATA *wield = NULL;
	CHAR_DATA *vch;
	int vassalpts;
	int weapon_xp;

	if(IS_NPC(ch))
		return;

	// Intelligent weapons get some xp.
        // Watch out for dual wield.
        weapon_count = 0;
        if((wield = get_eq_char(ch, WEAR_WIELD)) != NULL
        && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT))
        	weapon_count++;
	if((wield = get_eq_char(ch, WEAR_DUAL)) != NULL
        && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT))
        	weapon_count++;

	if((wield = get_eq_char(ch, WEAR_WIELD)) != NULL
        && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT)) {
		weapon_xp = xp / (5 + (5 * weapon_count));
		if(show_message)
                	Cprintf(ch, "%s receives %d experience points.\n\r", capitalize(wield->short_descr), weapon_xp);
                advance_weapon(ch, wield, weapon_xp);
	}

	if((wield = get_eq_char(ch, WEAR_DUAL)) != NULL
        && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT)) {
        	weapon_xp = xp / (5 + (5 * weapon_count));
		if(show_message)
                	Cprintf(ch, "%s receives %d experience points.\n\r", capitalize(wield->short_descr), weapon_xp);
                advance_weapon(ch, wield, weapon_xp);
	}

	// Convert some exp into patron/vassal points
        // Patron code starts here
        vassalpts = xp / 10;

	if (ch->patron != NULL && !IS_SET(ch->toggles, TOGGLES_PLEDGE))
	{
        	if ((vch = get_char_world(ch, ch->patron, TRUE)) != NULL)
                {
	                if (ch->level >= vch->level)
                        {
				if(show_message)
        	                	Cprintf(ch, "{cYou and your patron stands as equals now.{x\n\r");
                                vassalpts = 0;
                        }

                        vassalpts = vassalpts + ch->to_pass;
                        Cprintf(vch, "{cYou receive %d patron points from your vassal, %s.{x\n\r", vassalpts, ch->name);
			if(show_message)
                        	Cprintf(ch, "{cYour patron %s receives %d vassal points.{x\n\r", vch->name, vassalpts);
                        xp = xp * 9 / 10;
                        vch->pass_along = vch->pass_along + vassalpts;
                        if (ch->to_pass > 0)
			{
                        	ch->to_pass = 0;
                                /* prevents drop starts for accumulation */
                                save_char_obj(ch, FALSE);
                        }
		}
                else
                {
			if(show_message)
                		Cprintf(ch, "{cYou receive %d vassal points.{x\n\r", vassalpts);
                        /* builds the bank while patron is offline. */
                        ch->to_pass = ch->to_pass + vassalpts;
                        xp = xp * 9 / 10;
                }
	}

	if(show_message)
		Cprintf(ch, "You receive %d experience points.\n\r", xp);
        gain_exp(ch, xp);

	return;
}
