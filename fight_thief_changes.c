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
 *  ROM 2.4 is copyright 1993-1996 Russ Taylor                              *
 *  ROM has been brought to you by the ROM consortium                       *
 *      Russ Taylor (rtaylor@pacinfo.com)                                   *
 *      Gabrielle Taylor (gtaylor@pacinfo.com)                              *
 *      Brian Moore (rom@rom.efn.org)                                       *
 *  By using this code, you have agreed to follow the terms of the          *
 *  ROM license, in the file Rom24/doc/rom.license                          *
 ***************************************************************************/

 /***************************************************************************
 *  This combat system has been 90% redone from scratch by StarCrossed.     *
 *  Damage is more variable, among MANY other changes. See the functions    *
 *  for more details.                                                       *
 *                                                                          *
 *  Done March 2001 by Starcrossed: winterwolf7@yahoo.com                   *
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "magic.h"
#include "clan.h"
#include "flags.h"
#include "utils.h"


#define TYPE_SLIVER_THRUST    32
#define DT_SLIVER_THRUST      1234
#define GET_WARRIOR_AR(a)     (20 + a)
#define GET_THIEF_AR(a)       (15 + a)
#define GET_CLERIC_AR(a)      (20 + a)
#define GET_MAGE_AR(a)        (15 + (a * 9 / 10))
#define GET_MOB_AR(a)         (15 + (a * 3 / 2))
#define HAND_TO_HAND_MINIMUM  0
#define HAND_TO_HAND_MAXIMUM  1
#define MAX_DAMAGE_MESSAGES   24

/* global for disarm weapon name */
char disarmed_weapon[20];

// Many DO functions for skills and commands
DECLARE_DO_FUN(do_backstab);
DECLARE_DO_FUN(do_emote);
DECLARE_DO_FUN(do_berserk);
DECLARE_DO_FUN(do_enrage);
DECLARE_DO_FUN(do_bash);
DECLARE_DO_FUN(do_trip);
DECLARE_DO_FUN(do_throw);
DECLARE_DO_FUN(do_dirt);
DECLARE_DO_FUN(do_flee);
DECLARE_DO_FUN(do_kick);
DECLARE_DO_FUN(do_bite);
DECLARE_DO_FUN(do_bribe);
DECLARE_DO_FUN(do_bounty);
DECLARE_DO_FUN(do_disarm);
DECLARE_DO_FUN(do_get);
DECLARE_DO_FUN(do_recall);
DECLARE_DO_FUN(do_yell);
DECLARE_DO_FUN(do_sacrifice);
DECLARE_DO_FUN(do_trash);
DECLARE_DO_FUN(do_sap);
DECLARE_DO_FUN(do_chase);
DECLARE_DO_FUN(do_charge);
DECLARE_DO_FUN(do_push);
DECLARE_DO_FUN(do_rip);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_granite_stare);
DECLARE_DO_FUN(do_cheat);

// Utility functions
void do_murder(CHAR_DATA * ch, char *argument);
void check_assist(CHAR_DATA * ch, CHAR_DATA * victim);
void check_killer(CHAR_DATA * ch, CHAR_DATA * victim);
void wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace);
void death_cry(CHAR_DATA * ch);
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim);
int xp_compute(CHAR_DATA * gch, CHAR_DATA * victim, int total_levels, int xp_modifier);
bool is_safe(CHAR_DATA * ch, CHAR_DATA * victim);
void make_corpse(CHAR_DATA * ch);
void raw_kill(CHAR_DATA * victim);
void set_fighting(CHAR_DATA * ch, CHAR_DATA * victim);
void hunt_victim(CHAR_DATA * ch);
void do_save(CHAR_DATA * ch, char *argument);
void size_mob(CHAR_DATA * ch, CHAR_DATA * victim, int level);
CHAR_DATA *range_finder(CHAR_DATA * ch, char *vname, int range, int *where, int *distance, bool across_area);
int hit_lookup(char *hit);
int power_tattoo_count(CHAR_DATA *ch, int mod);
int using_skill_tattoo(CHAR_DATA *ch, int sn);
int get_natural_hitroll(CHAR_DATA *);
int get_natural_damroll(CHAR_DATA *);
int get_attack_speed(CHAR_DATA *);
void save_clan_report();
int spell_damage(CHAR_DATA *, CHAR_DATA *, int, int, int);
void check_lure(CHAR_DATA  *source);
void handle_special_drops(CHAR_DATA *victim);

// Core Combat Functions:
void check_assist(CHAR_DATA * ch, CHAR_DATA * victim);
void check_killer(CHAR_DATA * ch, CHAR_DATA * victim);
void multi_hit(CHAR_DATA * ch, CHAR_DATA * victim, int dt);
void mob_hit(CHAR_DATA * ch, CHAR_DATA * victim, int dt);
int weapon_hit(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA * wield, int dt);
int damage(CHAR_DATA * ch, CHAR_DATA * victim, int dam, int dt, int dam_type, bool show, int special);
void dam_message(CHAR_DATA *ch, CHAR_DATA *victim, int damage, int dt, bool immune, int special);
int handle_death(CHAR_DATA *ch, CHAR_DATA *victim, int special);
void handle_sacrifice(CHAR_DATA *ch);

// Skill related functions
bool check_dodge(CHAR_DATA * ch, CHAR_DATA * victim);
bool check_parry(CHAR_DATA * ch, CHAR_DATA * victim);
bool check_tumbling(CHAR_DATA * ch, CHAR_DATA * victim);
bool check_shield_block(CHAR_DATA * ch, CHAR_DATA * victim);
void disarm(CHAR_DATA * ch, CHAR_DATA * victim);
void double_kick(CHAR_DATA *ch, CHAR_DATA *victim);
void prismatic_sphere_effect(CHAR_DATA *ch, CHAR_DATA *victim);
void check_wild_swing(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, int dt);
int check_dragon_tail(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, int dt);
void check_kirre_tail_trip(CHAR_DATA *ch, CHAR_DATA *victim);
int check_sliver_thrust(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, int dt);
int check_beheading(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, int dt);
int check_dragon_thrust(CHAR_DATA *ch, CHAR_DATA *victim);
int check_dragon_bite(CHAR_DATA *ch, CHAR_DATA *victim);
int check_shield_bash(CHAR_DATA *ch, CHAR_DATA *victim);
int check_chi_ei(CHAR_DATA *ch, CHAR_DATA *victim);

// External functions
extern int double_xp_ticks;
extern int generate_int(unsigned char, unsigned char);
extern int get_caster_level(int);
extern int get_modified_level(int);
extern int saving_throw(CHAR_DATA*, CHAR_DATA*, int sn, int level, int diff, int stat, int damtype);
extern void meteor_blast(CHAR_DATA*, ROOM_INDEX_DATA*, int);
extern void advance_weapon(CHAR_DATA *ch, OBJ_DATA *obj, int xp);
extern int deity_lookup(char *);
extern int check_drowning(CHAR_DATA *, int);
extern bool try_cancel(CHAR_DATA *ch, int sn, int level);
extern int calculate_offering_tax_qp(OBJ_DATA *);
extern int calculate_offering_tax_gold(OBJ_DATA *);


struct damage_message_type {
	int minimum;
	char *prefix;
	char *postfix;
	char *punctuation;
};

// "Your <prefix> slash <postfix> victim<punctuation>"
const struct damage_message_type normal_message_table[] =
{
	{ 0,   "clumsy",     "misses",			"."	},
	{ 4,   "clumsy",     "bruises",			"."	},
	{ 8,   "wobbly",     "scrapes",			"."	},
	{ 12,  "wobbly",     "scratches",      "."	},
	{ 16,  "amateur",    "lightly wounds", "."	},
	{ 20,  "amateur",    "injures",			"."	},
	{ 24,  "competent",  "harms",			", creating a bruise."},
	{ 28,  "competent",  "thrashes",			", leaving marks!"},
	{ 32,  "skillful",   "mauls",			"!"	},
	{ 36,  "skillful",   "maims",			"!"	},
	{ 40,  "cunning",    "decimates",		", the wound bleeds!"},
	{ 44,  "cunning",    "devastates",		", hitting organs!"},
	{ 48,  "calculated", "mutilates",		", shredding flesh!"},
	{ 52,  "calculated", "cripples",			", leaving GAPING holes!"},
	{ 60,  "calm",       "DISEMBOWELS",		", guts spill out!"},
	{ 68,  "calm",       "DISMEMBERS",		", blood sprays forth!"},
	{ 76,  "furious",    "ANNIHILATES!",		", revealing bones!"},
	{ 84,  "furious",    "OBLITERATES!",		", rending organs!"},
	{ 92,  "frenzied",   "EVISCERATES!!",		", severing arteries!"},
	{ 100, "frenzied",   "DESTROYS!!",		", shattering bones!"},
	{ 110, "barbaric",   "MASSACRES!!!",		", gore splatters everywhere!"},
	{ 120, "fierce",     "!ERADICATES!",		", leaving little remaining!"},
	{ 130, "deadly",     "!DECAPITATES!",	", scrambling some brains!"},
	{ 149, "legendary",  "{r!!SHATTERS!!{x",	" into tiny pieces!"},
	{ 150, "ultimate",   "inflicts {RUNSPEAKABLE{x damage to", "!!"},
};

// "Your acid blast <prefix> victim<postfix><punctuation>"
const struct damage_message_type magic_message_table[] =
{
	{	0,	"misses",		"",			"."},
	{	4,	"scratches",		"",			"."},
	{	8,	"grazes",		" slightly",		"."},
	{	12,	"hits",			" squarely",		"."},
	{	16,	"injures",		"",			"."},
	{	20,	"wounds",		" badly",		"."},
	{	25, 	"mauls",		"",			"!"},
	{	30, 	"maims",		"",			"!"},
	{	35,	"decimates",		"",			"!"},
	{	40, 	"devastates",		", leaving a hole",	"!"},
	{	45,	"MUTILATES",		" severely",		"!"},
	{	50,	"DISEMBOWELS",		", causing bleeding",	"!"},
	{	55, 	"DISMEMBERS",		", blood flows",	"!"},
	{	60, 	"MANGLES",		", bringing screams",	"!"},
	{	65, "** DEMOLISHES **",		" with skill",		"!"},
	{	70, "*** CRIPPLES ***",		" for life",		"!"},
	{	75, "*= WRECKS =*",		"",                    	"!"},
	{	80, "=*= BLASTS =*=",		", charring flesh",	"!"},
	{	90, "=== ANNIHILATES ===",	"",			"!"},
	{	100,"=== OBLITERATES ===", 	"",			"!"},
	{	110, ">> DESTROYS <<",		" almost completely",	"!"},
	{	120, ">>> MASSACRES <<<",	" like a lamb at slaughter", "!"},
	{	130, "<! VAPORIZES !>", 	" completely",		"!"},
	{	149, "<<< ERADICATES >>>", 	"",			"!"},
	{	150, "does {RUNSPEAKABLE{x things to", "",		"!!"},
};

/* Weather update used for spells 'rain of tears' and 'hailstorm'
   Coded by Del
 */
/* also added another area room spell for green dragons - CloudKill
   shares this function nicely.
   Coded by Starcrossed, sept 15, 1999
 */

int race_sliver;

void init_rainupdate()
{
	race_sliver = race_lookup("sliver");
}

void
rain_update(void)
{
	CHAR_DATA *ch;
	AFFECT_DATA *paf;
	ROOM_INDEX_DATA *pRoomIndex;
	int done = FALSE;
	int dam = 0;

	for (ch = char_list; ch != NULL; ch = ch->next)
	{
		if (ch->in_room != NULL)
		{
			/* sudden death check on both spells */
			if (!IS_NPC(ch) && ch->pktimer > 0)
				continue;

			// Don't do anything to "safe" mobs
			if (IS_NPC(ch)
			&& (ch->pIndexData->pShop != NULL
			|| IS_SET(ch->act, ACT_TRAIN)
        		|| IS_SET(ch->act, ACT_PRACTICE)
        		|| IS_SET(ch->act, ACT_IS_HEALER)
        		|| IS_SET(ch->act, ACT_DEALER)
        		|| IS_SET(ch->act, ACT_IS_CHANGER)
        		|| ch->spec_fun == spec_lookup("spec_portal_keeper")))
				continue;

			// Using up your air fighting under water.
			if(number_percent() < 20
			&& ch->fighting
			&& check_drowning(ch, 1))
				continue;

			// Wizi mobs shouldn't be affected by these.
			if(IS_NPC(ch)
			&& IS_SET(ch->act,ACT_IS_WIZI))
				continue;

			/* whirlpool vortices */
			if (room_is_affected(ch->in_room, gsn_whirlpool))
			{
				paf = affect_find(ch->in_room->affected, gsn_whirlpool);
				if ((paf != NULL)
				&& !saving_throw(ch, ch, gsn_whirlpool, paf->level, SAVE_NORMAL, STAT_STR, DAM_DROWNING)
				&& !IS_SET(ch->imm_flags, IMM_SUMMON)
				&& (!IS_CLAN_GOON(ch)))
				{
					pRoomIndex = get_random_room(ch);
					while (IS_SET(pRoomIndex->room_flags, ROOM_NO_GATE)
						   || pRoomIndex->area->continent != ch->in_room->area->continent
						   || pRoomIndex->area->security < 9)
					{
						pRoomIndex = get_random_room(ch);
					}
					Cprintf(ch, "You are suddenly pulled into the whirlpool!\n\r");
					act("$n is pulled into the whirlpool and vanishes!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
					char_from_room(ch);
					char_to_room(ch, pRoomIndex);
					do_look(ch, "auto");
					continue;
				}
			}

			/* tornado vortices */
			if (room_is_affected(ch->in_room, gsn_tornado))
			{
				paf = affect_find(ch->in_room->affected, gsn_tornado);
				if ((paf != NULL)
				&& !saving_throw(ch, ch, gsn_tornado, paf->level, SAVE_NORMAL, STAT_STR, DAM_LIGHTNING)
				&& !IS_SET(ch->imm_flags, IMM_SUMMON)
				&& (!IS_CLAN_GOON(ch)))
				{
					pRoomIndex = get_random_room(ch);
					while (IS_SET(pRoomIndex->room_flags, ROOM_NO_GATE)
						   || pRoomIndex->area->continent != ch->in_room->area->continent
						   || pRoomIndex->area->security < 9)
					{
						pRoomIndex = get_random_room(ch);
					}
					Cprintf(ch, "You are suddenly pulled into the tornado!\n\r");
					act("$n is pulled into the tornado and vanishes!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
					char_from_room(ch);
					char_to_room(ch, pRoomIndex);
					do_look(ch, "auto");
					continue;
				}
			}

			/* add on for poison needle trap */
			if (is_affected(ch, gsn_ensnare))
			{
				paf = affect_find(ch->affected, gsn_ensnare);
				if (paf != NULL)
				{
					Cprintf(ch, "The poison burns through your body!\n\r");
					damage(ch, ch, number_range(paf->level / 2, paf->level * 3 / 2), gsn_ensnare, DAM_POISON, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);
				}
			}

			/* cloud kill spell on clanners only! */
			done = FALSE;

			if (ch->in_room
			&& !ch->pktimer
			&& room_is_affected(ch->in_room, gsn_cloudkill)
			&& (IS_NPC(ch) || is_clan(ch)))
			{
				/* get a handle on the affect */
				paf = affect_find(ch->in_room->affected, gsn_cloudkill);

				if (IS_NPC(ch)
				&& ch->master != NULL
				&& !is_clan(ch->master))
					done = TRUE;

				if (IS_NPC(ch)
				&& ch->leader != NULL
				&& !is_clan(ch->leader))
					done = TRUE;

				if (check_immune(ch, DAM_POISON) == IS_RESISTANT
				|| check_immune(ch, DAM_POISON) == IS_IMMUNE
				|| check_immune(ch, DAM_RAIN) == IS_IMMUNE)
					done = TRUE;

				/* check clanner status */
				if ((IS_NPC(ch) && !done)
				  || (paf->bitvector ==  ROOM_AFF_CLOUDKILL_CLAN
				   && is_clan(ch)
				   && ch->level >= paf->level - 8
				   && ch->level <= paf->level + 8
				   && !done))
				{
					Cprintf(ch, "You cough and choke as the caustic gas burns your lungs!\n\r");

					dam = number_range(1, ch->level);
					damage(ch, ch, dam, gsn_cloudkill, DAM_POISON, TRUE, TYPE_MAGIC);
					poison_effect(ch, paf->level, dam, TARGET_CHAR);
				}
			}
			/* non-clanner version */
			if (ch->in_room
			&& !ch->pktimer
			&& room_is_affected(ch->in_room, gsn_hailstorm)
			&& IS_NPC(ch)
			&& ch->race != race_lookup("sliver"))
			{
				damage(ch, ch, dice(ch->level, 6), gsn_hailstorm, DAM_RAIN, TRUE, TYPE_MAGIC);
				Cprintf(ch, "The hailstorm knocks you senseless!\n\r");
			}

			done = FALSE;

			/* clanner version */
			if (ch->in_room
			&& !ch->pktimer
			&& room_is_affected(ch->in_room, gsn_rain_of_tears))
			{
				paf = affect_find(ch->in_room->affected, gsn_rain_of_tears);

				if (ch->race == race_lookup("sliver"))
					done = TRUE;

				if (!IS_NPC(ch)
				&& !is_clan(ch))
					done = TRUE;

				if (IS_NPC(ch)
				&& ch->master != NULL
				&& !is_clan(ch->master))
					done = TRUE;

				if (IS_NPC(ch)
				&& ch->leader != NULL
				&& !is_clan(ch->leader))
					done = TRUE;

				if(paf == NULL)
					done = TRUE;

                                if((IS_NPC(ch) && !done)
				|| (!IS_NPC(ch)
				&& ch->level >= paf->level - 8
                                && ch->level <= paf->level + 8
				&& !done))
				{
					dam = spell_damage(ch, ch, paf->level, SPELL_DAMAGE_MEDIUM, TRUE);

					damage(ch, ch, dam, gsn_rain_of_tears, DAM_RAIN, TRUE, TYPE_MAGIC|TYPE_ANONYMOUS);
				}
			}
		}
	}

	return;
}


void sliver_rain_penalty(CHAR_DATA *ch)
{
	AFFECT_DATA af;

	if(IS_AFFECTED(ch, AFF_SLOW)
	|| is_affected(ch, gsn_slow))
		return;

	if(ch->race == race_lookup("sliver")
		&& ch->remort
		&& IS_OUTSIDE(ch)
		&& (weather_info.sky == SKY_RAINING
		|| weather_info.sky == SKY_LIGHTNING)
		&& (number_percent() <= 4))
	{
		Cprintf(ch, "The rain gathers in pools on you, hampering your efforts.\n\r");
		act("$n is hampered by the effect of rain.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

		/* Remove that haste */
		if(is_affected(ch, gsn_haste))
		{
			affect_strip(ch, gsn_haste);
			Cprintf(ch, "You feel yourself slow down.\n\r");
			act("$n slows down.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
		else
		{
			af.where = TO_AFFECTS;
			af.type = gsn_slow;
			af.level = ch->level;
			af.duration = 0;
			af.modifier = -3;
			af.location = APPLY_DEX;
			af.bitvector = AFF_SLOW;
			affect_to_char(ch, &af);
			Cprintf(ch, "You feel yourself slowing d o w n...\n\r");
			act("$n starts to move in slow motion.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
	}

	return;
}

// Hurt someone who is bleeding.
void razor_claws_effect(CHAR_DATA *victim)
{
        AFFECT_DATA *paf = NULL, *clawsaf = NULL;
        int dam = 0;

        for(paf = victim->affected; paf != NULL; paf = paf->next) {
                if(paf != NULL
                && paf->type == gsn_razor_claws
                && paf->location == APPLY_NONE) {
			clawsaf = paf;
			dam = number_range(paf->level / 2, paf->level);
			break;
                }

        }

        if(clawsaf == NULL)
                return;

	// Don't kill them with razor claws.
	if(victim->hit <= dam)
		return;

	Cprintf(victim, "Your wounds continue to bleed!\n\r");
        act("$n continues to bleed.",victim, NULL, victim, TO_ROOM, POS_RESTING);
	damage(victim, victim, dam, gsn_razor_claws, DAM_SLASH, TRUE, TYPE_SKILL | TYPE_ANONYMOUS);

       	if(clawsaf->duration == 0) {
               	Cprintf(victim, "A wound closes and the bleeding stops.\n\r");
               	affect_remove(victim, clawsaf);
		return;
       	}
       	clawsaf->duration--;

        return;
}

void choke_hold_effect(CHAR_DATA *victim) {
	AFFECT_DATA *paf;
	int dam = 0;

	paf = affect_find(victim->affected, gsn_choke_hold);

	if(paf == NULL)
		return;

	if(paf->duration > 0) {
		dam = paf->level + dice(2, paf->level / 2);
		if(dam >= victim->hit)
        		dam = victim->hit - 1;

		damage(victim, victim, dam, gsn_choke_hold,
        		DAM_DROWNING, TRUE, TYPE_SKILL | TYPE_ANONYMOUS);

		dam = number_range(1, paf->level);
		if(dam >= victim->move)
			dam = victim->move - 1;

		victim->move -= dam;

		// Has no effect if duration is zero.
		if(paf->duration == 1) {
			affect_strip(victim, gsn_choke_hold);
		}
		else if(paf->duration > 1) {
			paf->duration--;
		}
	}
}


// Hurt someone who is being crunched
void crushing_hand_effect(CHAR_DATA *victim)
{
	AFFECT_DATA *paf;
	int dam = 0;

	paf = affect_find(victim->affected, gsn_crushing_hand);

	if(paf == NULL)
		return;

	switch (paf->duration) {
		case 4:
			Cprintf(victim, "The huge hand starts squeezing you!\n\r");
			act("A massive hand crunches down on $n.",victim, NULL, NULL, TO_ROOM, POS_RESTING);
			dam = dice((paf->level / 4 + 2), 6);
			if(dam >= victim->hit)
				dam = victim->hit - 1;
			damage(victim, victim, dam, gsn_crushing_hand,
				DAM_BASH, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);
			break;
		case 3:
			Cprintf(victim, "The huge hand squeezes the air from your lungs with a painful crunch!\n\r");
			act("$n starts to turn blue from the hand's crushing force!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			dam = dice((paf->level / 3) + 2, 6);
			if(dam >= victim->hit)
				dam = victim->hit - 1;
			damage(victim, victim, dam, gsn_crushing_hand,
				DAM_BASH, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);
			break;
		case 2:
			Cprintf(victim, "You hear popping and snapping all over your body as your bones start to buckle!\n\r");
			Cprintf(victim, "You scream as the hand tightens even more!\n\r");
			act("$n screams as the massive hand starts breaking bones.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			dam = dice((paf->level / 2) + 2, 6);
			if(dam >= victim->hit)
				dam = victim->hit - 1;
			damage(victim, victim, dam, gsn_crushing_hand,
				DAM_BASH, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);
			break;
		case 1:
			Cprintf(victim, "The pressure on your body starts to give and you can draw air into your lungs again.\n\r");
			act("A massive hand slowly starts to let up on $n.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			dam = dice((paf->level / 4) + 2, 6);
			if(dam >= victim->hit)
				dam = victim->hit - 1;
			damage(victim, victim, dam, gsn_crushing_hand,
				DAM_BASH, TRUE, TYPE_MAGIC | TYPE_ANONYMOUS);
			break;
		default:
			Cprintf(victim, "The huge hand that was crushing you flickers and fades away!\n\r");
			act("A massive hand flickers and fades away as $n is released.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			affect_strip(victim, gsn_crushing_hand);
			return;
	}
	paf->duration--;

	return;
}

// Recover some hp every 3 seconds.
void revitalize_effect(CHAR_DATA *ch)
{
	AFFECT_DATA *paf = NULL;
	AFFECT_DATA *paf_next = NULL;

	// Check for revitalize spells and apply.
	if((paf = affect_find(ch->affected, gsn_minor_revitalize)) != NULL) {

		if(ch->hit < MAX_HP(ch)
		&& !is_affected(ch, gsn_dissolution))
			ch->hit += 1;

		paf->modifier -= 1;

		if(paf->modifier <= 0) {
			Cprintf(ch, "You no longer feel as revitalized.\n\r");
			affect_strip(ch, gsn_minor_revitalize);
		}
	}

	// Check for revitalize spells and apply.
	if((paf = affect_find(ch->affected, gsn_lesser_revitalize)) != NULL) {

		if(ch->hit < MAX_HP(ch)
		&& !is_affected(ch, gsn_dissolution))
			ch->hit += 3;

		paf->modifier -= 3;

		if(paf->modifier <= 0) {
			Cprintf(ch, "You no longer feel as revitalized.\n\r");
			affect_strip(ch, gsn_lesser_revitalize);
		}
	}

	// Check for revitalize spells and apply.
	if((paf = affect_find(ch->affected, gsn_greater_revitalize)) != NULL) {

		if(ch->hit < MAX_HP(ch)
		&& !is_affected(ch, gsn_dissolution))
			ch->hit += 6;

		paf->modifier -= 6;

		if(paf->modifier <= 0) {
			Cprintf(ch, "You no longer feel as revitalized.\n\r");
			affect_strip(ch, gsn_greater_revitalize);
		}
	}

	// Check for berserk affect and apply.
	for(paf = ch->affected; paf != NULL; paf = paf_next) {
		paf_next = paf->next;
		if(paf->type == gsn_berserk
		&& paf->location == APPLY_NONE) {
			if(ch->hit < MAX_HP(ch)
			&& !is_affected(ch, gsn_dissolution))
				ch->hit += 4;

			paf->modifier -= 4;

			if(paf->modifier <= 0) 
				affect_remove(ch, paf);
		}

		if(paf->type == gsn_enrage
		&& paf->location == APPLY_NONE) {
        		if(ch->hit < MAX_HP(ch)
        		&& !is_affected(ch, gsn_dissolution))
                		ch->hit += 4;

        		paf->modifier -= 4;

        		if(paf->modifier <= 0)
                		affect_remove(ch, paf);
		}
	}

	// Aurora for pearl dragons has a similar effect on mana
        if((paf = affect_find(ch->affected, gsn_aurora)) != NULL) {

                if(ch->mana < MAX_MANA(ch))
                        ch->mana += 5;

                paf->modifier -= 5;

                if(paf->modifier <= 0) {
                        Cprintf(ch, "Your shimmering aurora fades.\n\r");
                        affect_strip(ch, gsn_aurora);
                }
        }
	return;
}

void
violence_update(void)
{
	CHAR_DATA *ch;
	CHAR_DATA *victim;

	for (ch = char_list; ch != NULL; ch = ch->next)
	{
		// Idiot checks
		if (ch == NULL)
			break;
		if (ch->in_room == NULL)
			continue;

		// Hunting mobs move once per combat round.
		if (IS_NPC(ch)
			&& ch->fighting == NULL
			&& IS_AWAKE(ch)
			&& ch->hunting != NULL
			&& ch->pIndexData->vnum != MOB_VNUM_HUNT_DOG)
		{
			hunt_victim(ch);
			continue;
		}

		if(is_affected(ch, gsn_lure))
			check_lure(ch);

		// Should never be called unless we have a dumb crash.
		if(!IS_NPC(ch)
		&& ch->position == POS_DEAD) {
			// Kill them again properly.
			handle_death(ch, ch, TYPE_ANONYMOUS);
		}

		revitalize_effect(ch);
		razor_claws_effect(ch);

		if(is_affected(ch, gsn_crushing_hand))
			crushing_hand_effect(ch);

		if(is_affected(ch, gsn_choke_hold))
			choke_hold_effect(ch); 

		if(ch->fighting == NULL
		&& IS_NPC(ch))
			ch->wait = 0;

		if(ch->charge_wait > 0)
			ch->charge_wait--;

		if ((victim = ch->fighting) == NULL || ch->in_room == NULL)
			continue;

		// Deal a round of attacks.
		if (IS_AWAKE(ch) && ch->in_room == victim->in_room) {
			multi_hit(ch, victim, TYPE_UNDEFINED);
			if(is_affected(ch, gsn_boost)) {
				Cprintf(ch, "Your attacks are no longer boosted.\n\r");
				affect_strip(ch, gsn_boost);
			}
		}
		else {
			stop_fighting(ch, FALSE);
		}

		if (IS_NPC(ch))
		{
/*			if (HAS_TRIGGER(ch, TRIG_FIGHT))
				mp_percent_trigger(ch, victim, NULL, NULL, TRIG_FIGHT);
			if (HAS_TRIGGER(ch, TRIG_HPCNT))
				mp_hprct_trigger(ch, victim);
*/				
		}

		/*
		 * Fun for the whole family!
		 */
		check_assist(ch, victim);
	}

	return;
}

/* Auto assist check
   Fixed up by StarX */

void
check_assist(CHAR_DATA * ch, CHAR_DATA * victim)
{
	CHAR_DATA *rch;

	/* Notes:
	   ch is the aggressor
	   victim is the defender
	   rch is a guy in the same room
	   vch is a guy in the victims group, including the victim
	*/
	if (ch->in_room == NULL || victim->in_room == NULL)
		return;

	if (ch->in_room != victim->in_room)
		return;

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
	{
		/* assisting yourself is bad */
		if (rch == ch)
			continue;

		if (!IS_AWAKE(rch) || rch->fighting != NULL)
		{
			continue;
		}

		/* Mob in room with assist_players will jump in based on level */
		if (!IS_NPC(ch) && IS_NPC(rch)
			&& IS_SET(rch->off_flags, ASSIST_PLAYERS)
			&& rch->level + 6 > victim->level
			&& can_see(rch, ch))
		{
			do_emote(rch, "screams and attacks!");
			if (IS_SET(ch->toggles, TOGGLES_SOUND))
				Cprintf(ch, "!!SOUND(sounds/wav/scratak*.wav V=80 P=20 T=admin)");
			multi_hit(rch, victim, TYPE_UNDEFINED);
			continue;
		}

		/* Player in group with assists set */
		if (!IS_NPC(rch)
			&& is_same_group(ch, rch)
			&& !is_safe(rch, victim))
		{
			if (IS_NPC(victim) && IS_SET(rch->act, PLR_MOBASSIST))
			{
				multi_hit(rch, victim, TYPE_UNDEFINED);
				continue;
			}
			else if (!IS_NPC(victim) && IS_SET(rch->act, PLR_PLRASSIST))
			{
				multi_hit(rch, victim, TYPE_UNDEFINED);
				continue;
			}
			/* We don't want to help! */
			continue;
		}

		/* Charmies assist group mates */
		if ((IS_AFFECTED(rch, AFF_CHARM)
			 || IS_AFFECTED(ch, AFF_CHARM))
			&& is_same_group(ch, rch)
			&& !is_safe(rch, victim))
		{
			multi_hit(rch, victim, TYPE_UNDEFINED);
			continue;
		}

		/* Lastly check for odd mob flags */
		if (IS_NPC(rch) && !IS_AFFECTED(ch, AFF_CHARM)
			&& !IS_AFFECTED(rch, AFF_CHARM)
			&& can_see(rch, victim)
			&& (IS_SET(rch->off_flags, ASSIST_ALL)
		   	|| (rch->race == ch->race && IS_SET(rch->off_flags, ASSIST_RACE))
			|| (IS_NPC(rch) && IS_SET(rch->off_flags, ASSIST_ALIGN)
				&& ((IS_GOOD(rch) && IS_GOOD(ch))
				|| (IS_EVIL(rch) && IS_EVIL(ch))
				|| (IS_NEUTRAL(rch) && IS_NEUTRAL(ch))))
			|| (rch->pIndexData == ch->pIndexData
				&& IS_SET(rch->off_flags, ASSIST_VNUM))))
		{
			CHAR_DATA *vch, *target = NULL;
			int number = 0;

			if (number_bits(1) == 0)
				continue;


			/* Attack a random person in the victim's group */
			if (victim->in_room == NULL)
				return;

			for (vch = victim->in_room->people; vch != NULL; vch = vch->next_in_room)
			{
				if (can_see(rch, vch)
					&& is_same_group(vch, victim)
					&& number_range(0, number) == 0)
				{
					target = vch;
					number++;
				}
			}
			if (target != NULL)
			{
				do_emote(rch, "screams and attacks!");
				if (IS_SET(ch->toggles, TOGGLES_SOUND))
					Cprintf(ch, "!!SOUND(sounds/wav/scratak*.wav V=80 P=20 T=admin)");
				multi_hit(rch, target, TYPE_UNDEFINED);
			}
		}
	}
}

/* helper function for dual wield */
OBJ_DATA *
swap(CHAR_DATA * ch, OBJ_DATA * current)
{
	if (current == get_eq_char(ch, WEAR_WIELD))
		return get_eq_char(ch, WEAR_DUAL);
	return get_eq_char(ch, WEAR_WIELD);
}

/*
 * Multi_hit: Do one group of attacks.
 * Returns TRUE if victim died.
 */
void
multi_hit(CHAR_DATA * ch, CHAR_DATA * victim, int dt)
{
	int chance;
	int heal;
	int dam;
	AFFECT_DATA *paf;
	OBJ_DATA *wield;
	bool dual_wielding = 0, victim_dead=FALSE;
	int attack_speed = 0;

	// recover some daze and wait.
	if (ch->desc == NULL)
		ch->wait = UMAX(0, ch->wait - PULSE_VIOLENCE);

	if (ch->desc == NULL)
		ch->daze = UMAX(0, ch->daze - PULSE_VIOLENCE);

	// Also in damage.
	if(!IS_NPC(ch)
	&& !IS_NPC(victim)
	&& ch != victim) {
		ch->no_quit_timer = 3;
		victim->no_quit_timer = 3;
	}

	// no attacks for stunnies -- just a check
	if (ch->position < POS_RESTING)
		return;

	// Kirre ambush message.
	if(is_affected(ch, gsn_ambush)) {
		Cprintf(ch, "{gYou launch a fierce ambush by pouncing out of the shadows!{x\n\r");
		act("$n launches a surprise attack against you!", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$n launches a surprise attack against $N!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	}

	// Ninja can recover from stun instantly.
	if(ch->daze > 0
	&& number_percent() < get_skill(ch, gsn_balance) / 2) {
		Cprintf(ch, "You recover your balance and keep fighting!\n\r");
		ch->daze = 0;
		check_improve(ch, gsn_balance, TRUE, 3);
	}

	// Learn monk fighting styles
	if (is_affected(ch, gsn_stance_turtle))
		check_improve(ch, gsn_stance_turtle, TRUE, 5);
	if (is_affected(ch, gsn_stance_tiger))
		check_improve(ch, gsn_stance_tiger, TRUE, 5);
	if (is_affected(ch, gsn_stance_mantis))
		check_improve(ch, gsn_stance_mantis, TRUE, 5);
	if (is_affected(ch, gsn_stance_shadow))
		check_improve(ch, gsn_stance_shadow, TRUE, 5);
	if (is_affected(ch, gsn_stance_kensai))
		check_improve(ch, gsn_stance_kensai, TRUE, 5);
	if (IS_AFFECTED(ch, AFF_BLIND) && get_skill(ch, gsn_blindfighting) > 0)
		check_improve(ch, gsn_blindfighting, TRUE, 3);


	// Umm remort slivers shouldn't fight in the rain
	if (ch->race == race_lookup("sliver"))
		sliver_rain_penalty(ch);

	if (is_affected(victim, gsn_cloud_of_poison)) {
		poison_effect(ch, victim->level, 0, TARGET_CHAR);
	}

	if (is_affected(ch, gsn_confusion) && number_percent() <= 15) {
		Cprintf(ch, "You hesitate out of confusion. Who are you fighting again??\n\r");
		act("$N seems to hesitate out of confusion.", ch, NULL, ch, TO_NOTVICT, POS_RESTING);
		return;
	}

	if (is_affected(ch, gsn_cone_of_fear)) {
		if (number_percent() < 25) {
			Cprintf(ch, "You grit your fear, gather your courage, fighting off the fear!\n\r");
			affect_strip(ch, gsn_cone_of_fear);
		} else {
			Cprintf(ch, "You are too paralyzed with fear to risk attacking!\n\r");
			return;
		}
	}

	if (is_affected(victim, gsn_mirror_image))
	{
		paf = affect_find(victim->affected, gsn_mirror_image);
		act("Your mirror image takes $N's hit!", victim, NULL, ch, TO_CHAR, POS_RESTING);
		act("$n's mirror image absorbs the shock.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		// Mage breaks around 30%, Thieves around 50%
		if (number_percent() < 80 - paf->level)
		{
			Cprintf(victim, "Your mirror image shatters to pieces!\n\r");
			act("$n's mirror image shatters to pieces!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			affect_remove(victim, paf);
		}
		if (ch->fighting == NULL)
			set_fighting(ch, victim);
		if (victim->fighting == NULL)
			set_fighting(victim, ch);
		return;
	}

	if (is_affected(victim, gsn_prismatic_sphere))
		prismatic_sphere_effect(ch, victim);

	if (is_affected(ch, gsn_shapeshift) && (ch->fighting != NULL))
	{
		Cprintf(ch, "You shift back to your original shape to fight.\n\r");
		act("$n assumes $s normal shape to fight.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		affect_strip(ch, gsn_shapeshift);
	}

	if (IS_NPC(ch))
	{
		mob_hit(ch, victim, dt);
		return;
	}

	// find out if both weapons will strike this round */
	if(get_eq_char(ch, WEAR_SHIELD) == NULL 
	&& get_skill(ch, gsn_dual_wield) > 0)
		dual_wielding = 1;

	wield = get_eq_char(ch, WEAR_WIELD);

	// No hand to hand offhand if using two handed.
	if (wield != NULL && IS_WEAPON_STAT(wield->pIndexData, WEAPON_TWO_HANDS))
		dual_wielding = 0;

	// No weapons doesn't count as dual wield
	if (wield == NULL && get_eq_char(ch, WEAR_DUAL) == NULL)
		dual_wielding = 0;

	// Determine if the character has attack speed modifiers
	attack_speed = get_attack_speed(ch);

	// Start round with counter attacks.
	victim_dead = check_dragon_thrust(ch, victim);
	if (victim_dead)
		return;

	victim_dead = check_dragon_bite(ch, victim);
	if (victim_dead)
		return;

	victim_dead = check_shield_bash(ch, victim);
	if (victim_dead)
		return;

	victim_dead = check_chi_ei(ch, victim);
	if (victim_dead)
        	return;

	// First basic hit
	victim_dead = weapon_hit(ch, victim, wield, dt);
	if (victim_dead)
		return;

	if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 95)
		Cprintf(ch, "!!SOUND(sounds/wav/fight*.wav V=80 P=20 T=admin)");

	// Hasted!
	if(is_affected(ch, gsn_haste)) {
		if (dual_wielding)
                	wield = swap(ch, wield);
		victim_dead = weapon_hit(ch, victim, wield, dt);
		if(victim_dead)
			return;
	}

	// Do second attack
	chance = (get_skill(ch, gsn_second_attack) / 2) + attack_speed;
	if (IS_AFFECTED(ch, AFF_SLOW))
		chance /= 2;

	if (number_percent() < chance) {
		if (dual_wielding)
                        wield = swap(ch, wield);
		victim_dead = weapon_hit(ch, victim, wield, dt);
		check_improve(ch, gsn_second_attack, TRUE, 5);
		if (victim_dead)
			return;
	}

	chance = (get_skill(ch, gsn_third_attack) / 2) + attack_speed;
	if (IS_AFFECTED(ch, AFF_SLOW))
		chance /= 4;

	if (number_percent() < chance)
	{
		if (dual_wielding)
                        wield = swap(ch, wield);
		victim_dead = weapon_hit(ch, victim, wield, dt);
		check_improve(ch, gsn_third_attack, TRUE, 6);
		if(victim_dead)
			return;
	}

	chance = (get_skill(ch, gsn_fourth_attack) / 2) + attack_speed;
	if (IS_AFFECTED(ch, AFF_SLOW))
		chance /= 4;

	if (number_percent() < chance) {
		if (dual_wielding)
                        wield = swap(ch, wield);
		victim_dead = weapon_hit(ch, victim, wield, dt);
		check_improve(ch, gsn_fourth_attack, TRUE, 6);
		if(victim_dead)
			return;
	}

	chance = (get_skill(ch, gsn_dual_wield) / 2) + attack_speed;
	if (!dual_wielding)
		chance = 0;

	if (number_percent() < chance)
	{
		if (dual_wielding)
                        wield = swap(ch, wield);
		victim_dead = weapon_hit(ch, victim, wield, dt);
		check_improve(ch, gsn_dual_wield, TRUE, 6);
		if(victim_dead)
			return;
	}

	chance = (get_skill(ch, gsn_dual_wield) / 4) + attack_speed;
	if (!dual_wielding 
	|| ch->level < 45
	|| IS_AFFECTED(ch, AFF_SLOW))
		chance = 0;

	if (number_percent() < chance)
	{
        	if (dual_wielding)
                	wield = swap(ch, wield);
        	victim_dead = weapon_hit(ch, victim, wield, dt);
        	check_improve(ch, gsn_dual_wield, TRUE, 6);
        	if(victim_dead)
                	return;
	}

	if(victim_dead)
		return;

	// Any other mods should check primary weapon only
	wield = get_eq_char(ch, WEAR_WIELD);

	if(wield != NULL
	&& ch->charClass == class_lookup("runist")
	&& obj_is_affected(wield, gsn_blade_rune)) {
		paf = affect_find(wield->affected, gsn_blade_rune);
		if(paf != NULL
		&& paf->location == APPLY_NONE
		&& paf->modifier == BLADE_RUNE_SPEED
		&& number_percent() < 50) {
			Cprintf(ch, "The blade rune on your weapon glows brightly!\n\r");
			victim_dead = weapon_hit(ch, victim, wield, dt);
			// Don't make allow this to give more bursts
			if(ch->burstCounter)
				ch->burstCounter--;
		}
	}

	if (victim_dead || ch->in_room != victim->in_room)
		return;

	victim_dead = check_beheading(ch, victim, wield, dt);
	if (victim_dead || ch->in_room != victim->in_room)
		return;

	victim_dead = check_sliver_thrust(ch, victim, wield, DT_SLIVER_THRUST);
	if (victim_dead || ch->in_room != victim->in_room)
		return;

	victim_dead = check_dragon_tail(ch, victim, wield, gsn_tail_attack);
	if (victim_dead || ch->in_room != victim->in_room)
		return;

	check_kirre_tail_trip(ch, victim);

	chance = get_skill(ch, gsn_gore) / 4;
	if (IS_AFFECTED(ch, AFF_SLOW))
		chance = 0;

	if (number_percent() < chance)
	{
		Cprintf(ch, "You thrust your horns into your enemy!\n\r");
		dam = number_range(ch->level / 2, ch->level * 3 / 2);
		victim_dead = damage(ch, victim, dam, gsn_gore, DAM_PIERCE, TRUE, TYPE_SKILL);
		check_improve(ch, gsn_gore, TRUE, 6);
		if (victim_dead)
			return;
	}

	chance = get_skill(ch, gsn_snake_bite) / 4;
	if (IS_AFFECTED(ch, AFF_SLOW))
		chance = 0;

	if (number_percent() < chance)
	{
		Cprintf(ch, "The snakes on your head hiss at your victim!\n\r");
		dam = number_range(ch->level / 2, ch->level * 3 / 2);

		victim_dead = damage(ch, victim, dam, gsn_snake_bite, DAM_POISON, TRUE, TYPE_MAGIC);
		if (victim_dead)
			return;
		poison_effect(victim, ch->level, ch->level / 2, TARGET_CHAR);
		check_improve(ch, gsn_snake_bite, TRUE, 6);
	}

	if (number_percent() < 30 && is_affected(ch, gsn_ambush)) {
		Cprintf(ch, "{gYou lose the element of surprise.{x\n\r");
		affect_strip(ch, gsn_ambush);
		return;
	}

	// NOTE: Wildswing HAS to be last damage in the round.
	check_wild_swing(ch, victim, wield, dt);

	if (get_skill(ch, gsn_regeneration) > 0 && !is_affected(ch, gsn_dissolution))
	{
		if (number_percent() < get_skill(ch, gsn_regeneration) / 2)
		{
			heal = UMAX(1, dice(1, MAX_HP(ch) / 75));
			if(ch->hit + heal <= MAX_HP(ch)) { 
				ch->hit += heal;

				if (ch->remort > 0)
				{
				   heal = UMAX(1, dice(1, ch->hit / 100));
				   if(ch->hit + heal <= MAX_HP(ch))
				   	ch->hit += heal;
				}
			}
			update_pos(ch);
		}
	}
	return;
}

/* procedure for all mobile attacks */
void
mob_hit(CHAR_DATA * ch, CHAR_DATA * victim, int dt)
{
	int chance, number, dam;
	CHAR_DATA *vch;
	OBJ_DATA *wield = get_eq_char(ch, WEAR_WIELD);
	int victim_dead = FALSE;

	victim_dead = weapon_hit(ch, victim, wield, dt);

	if(victim_dead)
		return;

	if (IS_AFFECTED(ch, AFF_HASTE)
	|| (IS_SET(ch->off_flags, OFF_FAST)
	&& !IS_AFFECTED(ch, AFF_SLOW)))
		victim_dead = weapon_hit(ch, victim, wield, dt);

	if(victim_dead)
		return;

	chance = get_skill(ch, gsn_second_attack) / 2;

	if (IS_AFFECTED(ch, AFF_SLOW) && !IS_SET(ch->off_flags, OFF_FAST))
		chance /= 2;

	if (number_percent() < chance)
	{
		victim_dead = weapon_hit(ch, victim, wield, dt);
		if(victim_dead)
			return;
	}

	chance = get_skill(ch, gsn_third_attack) / 4;

	if (IS_AFFECTED(ch, AFF_SLOW) && !IS_SET(ch->off_flags, OFF_FAST))
		chance = 0;

	if (number_percent() < chance)
	{
		victim_dead = weapon_hit(ch, victim, wield, dt);
		if(victim_dead)
			return;
	}

	/* coolness with moonbeam and undead */
	if ((ch->race == race_lookup("undead") ||
		 ch->race == race_lookup("deamon")) &&
		is_affected(ch, gsn_moonbeam))
	{
		victim_dead = weapon_hit(ch, victim, wield, dt);
		if(victim_dead)
			return;
	}

	if (ch->wait > 0)
		return;

        // Don't do special moves if victim is dead or fled
        // Feb 06, 2006: Also stop if victim is stunned/dying.
	if(victim_dead
        || ch->in_room != victim->in_room
        || victim->position <= POS_STUNNED)
		return;

	/* now for the skills */
	number = number_range(0, 8);

	switch (number)
	{
	case (0):
		if (IS_SET(ch->off_flags, OFF_BASH))
			do_bash(ch, "");
		break;
	case (1):
		if (IS_SET(ch->off_flags, OFF_BERSERK) && !IS_AFFECTED(ch, AFF_BERSERK))
			do_berserk(ch, "");
		break;
	case (2):
		if (IS_SET(ch->off_flags, OFF_DISARM)
			|| IS_SET(ch->act, ACT_WARRIOR)
			|| IS_SET(ch->act, ACT_THIEF))
		{
			do_disarm(ch, "");
		}
		break;
	case (3):
		if (IS_SET(ch->off_flags, OFF_KICK))
			do_kick(ch, "");
		break;
	case (4):
		if (IS_SET(ch->off_flags, OFF_KICK_DIRT))
			do_dirt(ch, "");
		break;
	case (5):
		if (IS_SET(ch->off_flags, OFF_TAIL))
		{
			victim_dead = weapon_hit(ch, victim, wield, dt);
			if(victim_dead)
				return;
		}
		break;
	case (6):
		if (IS_SET(ch->off_flags, OFF_TRIP))
			do_trip(ch, "");
		break;
	case (7):
		if (IS_SET(ch->off_flags, OFF_CRUSH))
		{
			number = number_range(1, 2);
			switch (number)
			{
				case 1:
					dam = number_range(2, ch->level * 2);
					Cprintf(victim, "You are brutally crushed by %s!\n\r", PERS(ch, victim));
					act("$n is crushed mercilessly!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
					damage(ch, victim, dam, gsn_hug, DAM_BASH, TRUE, TYPE_SKILL);
					break;
				case 2:
					dam = number_range(2, ch->level);
					Cprintf(victim, "You can't move as %s begins to crush the life from you!\n\r", PERS(ch, victim));
					act("$n can't move as they are crushed!", victim, NULL, NULL,TO_ROOM, POS_RESTING);
					damage(ch, victim, dam, gsn_crush, DAM_BASH, TRUE, TYPE_SKILL);
					DAZE_STATE(victim, 4 * PULSE_VIOLENCE);
					victim->position = POS_RESTING;
					break;
				default:
					Cprintf(ch, "Error in crush.\n\r");
					Cprintf(victim, "Error occured in crush.\n\r");
					break;
			}
		}
		break;
	case (8):
		if (IS_SET(ch->off_flags, OFF_BACKSTAB))
		{
			do_backstab(ch, "");
		}
	}

	// Area attack has to be last damage in the round.
	if (IS_SET(ch->off_flags, OFF_AREA_ATTACK))
	{
		for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
		{
			if (vch->fighting == ch)
				weapon_hit(ch, vch, wield, dt);
		}
	}
	return;
}

// Number of non-stacking weapon flags on a weapon
int
count_weapon_flags(OBJ_DATA *wield) {
	int count = 0;

	if (IS_WEAPON_STAT(wield, WEAPON_FLAMING))
		count++;
	if (IS_WEAPON_STAT(wield, WEAPON_FROST))
		count++;
	if (IS_WEAPON_STAT(wield, WEAPON_SHOCKING))
		count++;
	if (IS_WEAPON_STAT(wield, WEAPON_FLOODING))
		count++;
	if (IS_WEAPON_STAT(wield, WEAPON_VAMPIRIC))
		count++;
	if (IS_WEAPON_STAT(wield, WEAPON_SOULDRAIN))
		count++;
	if (IS_WEAPON_STAT(wield, WEAPON_CORROSIVE))
		count++;

	return count;
}

int check_martial_arts(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int victim_dead = FALSE;

	if (ch->reclass == reclass_lookup("hermit")
	 || ch->reclass == reclass_lookup("warlord")
	 || ch->reclass == reclass_lookup("barbarian")
	 || ch->reclass == reclass_lookup("cavalier")
	 || ch->reclass == reclass_lookup("templar")
	 || ch->reclass == reclass_lookup("zealot")) {
		if (number_percent() <= 5) {
			Cprintf(victim, "The force of the blow knocks you to the ground!\n\r");
			act("$n is knocked to the ground by the force of $N's attack!", victim, ch, ch, TO_ROOM, POS_RESTING);
			victim_dead = damage(ch, victim, dice(1 ,6), 0, DAM_BASH, FALSE, TYPE_MAGIC);
			
			if (!victim_dead) {
				victim->daze += 2 * PULSE_VIOLENCE;
			}
		}
	}
	else if (ch->charClass == class_lookup("monk")
	&& get_eq_char(ch, WEAR_WIELD) == NULL 
	&& get_eq_char(ch, WEAR_DUAL) == NULL
	&& number_percent() < 10) {
		Cprintf(victim, "The force of the blow leaves you dazed!\n\r");
		act("$n looks stunned after $N's attack!", victim, ch, ch, TO_ROOM, POS_RESTING);
		victim_dead = damage(ch, victim, dice(1 ,6) + ch->level, 0, DAM_BASH, FALSE, TYPE_MAGIC);

		if (!victim_dead) {
			victim->daze += 3 * PULSE_VIOLENCE;
			victim->wait += dice(2, 8);
		}
	}

	return victim_dead;
}


// Returns true if victim died
int
handle_weapon_flags(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, int dt)
{
	int dam=0;
	int victim_dead=FALSE;
	int flag_count = 0, flag_chance = 110;
	AFFECT_DATA af, *paf = NULL;
	int markup = 0;

	if(wield == NULL)
		dt = gsn_hand_to_hand;

	// Handle the hth flags.
	if(dt == gsn_hand_to_hand)
	{
		if (ch->race == race_lookup("sliver") || ch->race == race_lookup("green dragon"))
		{
			AFFECT_DATA af;

			if(number_percent() <= 5)
		{
				Cprintf(victim, "You feel poison coursing through your veins.\n\r");

				if (ch->race == race_lookup("green dragon"))
					act("$n is stricken ill by $N's poisonous attack.", victim, ch, ch, TO_ROOM, POS_RESTING);
				else
					act("$n is poisoned by the venom on $N's mandibles.", victim, ch, ch, TO_ROOM, POS_RESTING);

				af.where = TO_AFFECTS;
				af.type = gsn_poison;
				af.level = ch->level * 3 / 4;
				af.duration = dice(1, 4);
				af.location = APPLY_STR;
				af.modifier = -1;
				af.bitvector = AFF_POISON;
				affect_join(victim, &af);

				// Poison is 3-6% damage
				victim_dead = damage(ch, victim, 1 + (victim->hit / number_range(15, 30)), gsn_poison, DAM_POISON, FALSE, TYPE_MAGIC);
			}
		}
		else if (ch->race == race_lookup("white dragon"))
		{
			dam = number_range(1, (ch->level / 7) + 1);
			Cprintf(victim, "You are frozen by the ice cold touch.\n\r");
			act("$n is frozen by $N's ice cold touch.", victim, ch, ch, TO_ROOM, POS_RESTING);
			cold_effect(victim, ch->level, dam, TARGET_CHAR);
			victim_dead = damage(ch, victim, dam, 0, DAM_COLD, FALSE, TYPE_MAGIC);
		}
		else if (ch->race == race_lookup("black dragon"))
		{
			dam = number_range(1, (ch->level / 7) + 1);
			Cprintf(victim, "Your flesh is corroded by potent acid!\n\r");
			act("$n's flesh is corroded by $N's potent acid!", victim, ch, ch, TO_ROOM, POS_RESTING);
			acid_effect(victim, ch->level, dam, TARGET_CHAR);
			victim_dead = damage(ch, victim, dam, 0, DAM_ACID, FALSE, TYPE_MAGIC);
		}
		else if (ch->race == race_lookup("red dragon"))
		{
			dam = number_range(1, (ch->level / 7) + 1);
			Cprintf(victim, "You are burned by the flames!\n\r");
			act("$n is burned by $N's flames!", victim, ch, ch, TO_ROOM, POS_RESTING);
			fire_effect((void *) victim, ch->level, dam, TARGET_CHAR);
			victim_dead = damage(ch, victim, dam, 0, DAM_FIRE, FALSE, TYPE_MAGIC);
		}
		else if (ch->race == race_lookup("blue dragon"))
		{
			dam = number_range(1, (ch->level / 7) + 1);
			Cprintf(victim, "You are struck by crackling lightning!\n\r");
			act("$n is shocked by $N's crackling lightning!", victim, ch, ch, TO_ROOM, POS_RESTING);
			shock_effect(victim, ch->level, dam, TARGET_CHAR);
			victim_dead = damage(ch, victim, dam, 0, DAM_LIGHTNING, FALSE, TYPE_MAGIC);
		}
		else if(ch->race == race_lookup("marid"))
		{
			dam = number_range(1, (ch->level / 7) + 1);
			Cprintf(victim, "You are enveloped in water and start to drown!\n\r");
			act("$n is enveloped in water by $N!", victim, ch, ch, TO_ROOM, POS_RESTING);
			water_effect(victim, ch->level, dam, TARGET_CHAR);
			victim_dead = damage(ch, victim, dam, 0, DAM_DROWNING, FALSE, TYPE_MAGIC);
		}
	}

	// Don't do anything else if they died.
	if (victim_dead)
		return TRUE;

	if (is_affected(ch, gsn_razor_claws)) {
		for(paf = ch->affected; paf != NULL; paf = paf->next) {
			if (paf != NULL && paf->type == gsn_razor_claws && paf->location == APPLY_DAMROLL) {
				break;
			}
		}

		if (paf != NULL && number_percent() <= 7) {
			Cprintf(ch, "Your claws leave a horrible gash!\n\r");
			Cprintf(victim, "Blood begins to spurt from the wound!\n\r");
	  		af.where = TO_AFFECTS;
			af.type = gsn_razor_claws;
			af.level = ch->level;
			af.duration = 3;
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.bitvector = 0;
			affect_to_char(victim, &af);
		}
	}

	// Check for knockdown and stun on attacks
	victim_dead = check_martial_arts(ch, victim);
			
	if(victim_dead)
		return TRUE;

	// Note: Thorn mantle is added to all attacks, armed and not.
	if (is_affected(ch, gsn_thorn_mantle))
	{
		// Adds between 1-6 and 1-10 damage.
		dam = number_range(1, ch->level / 6 + 2);
		act("$n is pierced by thorns.", victim, wield, NULL, TO_ROOM, POS_RESTING);
		act("You are pierced by thorns!", victim, wield, NULL, TO_CHAR, POS_RESTING);
		victim_dead = damage(ch, victim, dam, 0, DAM_PIERCE, FALSE, TYPE_MAGIC);
	}

	if (victim_dead)
		return TRUE;

	flag_count = 0;
	flag_chance = 100;

	if (wield != NULL)
		flag_count = count_weapon_flags(wield);

	if (flag_count > 0)
		flag_chance = (flag_chance / flag_count) + 20;

	// Ranged attacks are fewer so each flag gas more effect
	if(dt == gsn_marksmanship) {
		markup = wield->level / 3;
	}

	// Do regular weapon flags:
	/* but do we have a funky weapon? */
	if (wield != NULL && wield->item_type == ITEM_WEAPON)
	{
		if (IS_WEAPON_STAT(wield, WEAPON_POISON))
		{
			AFFECT_DATA af;

			if (number_percent() <= (5 + (markup / 3)))
			{
				Cprintf(victim, "You feel poison coursing through your veins.\n\r");
				act("$n is poisoned by the venom on $p.", victim, wield, NULL, TO_ROOM, POS_RESTING);

				af.where = TO_AFFECTS;
				af.type = gsn_poison;
				af.level = wield->level * 3 / 4;
				af.duration = dice(1, 4);
				af.location = APPLY_STR;
				af.modifier = -1;
				af.bitvector = AFF_POISON;
				affect_join(victim, &af);

				// Poison is 3-6% damage per tick.
				victim_dead = damage(ch, victim, 1 + (victim->hit / number_range(15, 30)), gsn_poison, DAM_POISON, FALSE, TYPE_MAGIC);
			}
		}

		if (victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_DEMONIC))
		{
			AFFECT_DATA af;

			if (number_percent() <= (8 + (markup / 3)))
			{
				act("$p calls forth the demons of Hell upon $n!", victim, wield, NULL, TO_ROOM, POS_RESTING);
				act("$p has assailed you with the demons of Hell!", ch, wield, victim, TO_VICT, POS_RESTING);
				dam = number_range(wield->level / 2, wield->level * 3 / 2);
				victim_dead = damage(ch, victim, dam, gsn_demonfire, DAM_NEGATIVE, TRUE, TYPE_MAGIC);

				if (!victim_dead && !is_affected(victim, gsn_curse)) {
					af.where = TO_AFFECTS;
					af.type = gsn_curse;
					af.level = wield->level;
					af.duration = 0;
					af.location = APPLY_NONE;
					af.modifier = 0;
					af.bitvector = AFF_CURSE;
					affect_to_char(victim, &af);

       					Cprintf(victim, "You feel unclean.\n\r");
				}
			}

		}

		if (victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_PSIONIC))
		{
			AFFECT_DATA af;

			if (number_percent() <= (8 + (markup / 3)))
			{
				act("$p emits a blast of psionic force towards $n!", victim, wield, NULL, TO_ROOM, POS_RESTING);
				act("$p shatters your mental defenses!", ch, wield, victim, TO_VICT, POS_RESTING);
				dam = number_range(wield->level / 2, wield->level * 3 / 2);
				victim_dead = damage(ch, victim, dam, gsn_psychic_crush, DAM_CHARM, TRUE, TYPE_MAGIC);

				if (!victim_dead) {
					af.where = TO_AFFECTS;
					af.type = gsn_psychic_crush;
					af.level = wield->level;
					af.duration = wield->level / 8;
					af.location = APPLY_SAVING_SPELL;
					af.modifier = wield->level / 12;
					af.bitvector = 0;
					affect_merge(victim, &af, gsn_psychic_crush);
					af.modifier = -2;
					af.location = APPLY_INT;
					affect_merge(victim, &af, gsn_psychic_crush);
					af.modifier = -2;
					af.location = APPLY_WIS;
					affect_merge(victim, &af, gsn_psychic_crush);
				}
			}
		}

		if(victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_ENTROPIC))
		{
			dam = number_range(1, wield->level / 12);
			dam += (markup / 2);
			act("$p draws strength from you!", ch, wield, NULL, TO_CHAR, POS_RESTING);
			victim_dead = damage(ch, ch, dam, 0, DAM_NEGATIVE, FALSE, TYPE_MAGIC);
		}

		if (victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_ANTIMAGIC))
		{
			dam = number_range(1, wield->level / 16);
			dam += (markup / 2);
			if(ch->mana < dam) {
				act("{R$p begins feasting upon your soul!{x", ch, wield, NULL, TO_CHAR, POS_RESTING);
				victim_dead = damage(ch, ch, dam * 10, 0, DAM_NEGATIVE, FALSE, TYPE_MAGIC);
			} else {
				act("$p draws magic from you!", ch, wield, NULL, TO_CHAR, POS_RESTING);
				ch->mana -= dam;
			}
		}

		if (victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_VAMPIRIC) && number_percent() < flag_chance)
		{
			dam = number_range(1, wield->level / 7 + 1);
			dam += (markup);
			act("$p draws life from $n.", victim, wield, NULL, TO_ROOM, POS_RESTING);
			act("You feel $p drawing your life away.", victim, wield, NULL, TO_CHAR, POS_RESTING);
			victim_dead = damage(ch, victim, dam, 0, DAM_NEGATIVE, FALSE, TYPE_MAGIC);
			ch->alignment = UMAX(-1000, ch->alignment - 1);
			if(!is_affected(ch, gsn_dissolution))
				ch->hit += UMAX(1, number_range(1, wield->level / 12) + (markup / 2));
		}

		if (victim_dead)
			return TRUE;

		// Soul drain converts their hp into mana instead.
		if (IS_WEAPON_STAT(wield, WEAPON_SOULDRAIN) && number_percent() < flag_chance)
		{
			dam = number_range(1, wield->level / 7 + 1);
			dam += markup;
			act("$p greedily devours $n's life force.", victim, wield, NULL, TO_ROOM, POS_RESTING);
			act("Your soul cries in pain from $p's touch.", victim, wield, NULL, TO_CHAR, POS_RESTING);
			victim_dead = damage(ch, victim, dam, 0, DAM_NEGATIVE, FALSE, TYPE_MAGIC);
			ch->alignment = UMAX(-1000, ch->alignment - 1);
			if (!is_affected(ch, gsn_dissolution))
				ch->mana += UMAX(1, number_range(1, wield->level / 16) + (markup / 3));
		}

		if (victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_FLAMING) && number_percent() < flag_chance)
		{
			dam = number_range(1, wield->level / 7 + 1);
			dam += markup;
			act("$n is burned by $p.", victim, wield, NULL, TO_ROOM, POS_RESTING);
			act("$p sears your flesh.", victim, wield, NULL, TO_CHAR, POS_RESTING);
			fire_effect((void *) victim, wield->level, dam, TARGET_CHAR);
			victim_dead = damage(ch, victim, dam, 0, DAM_FIRE, FALSE, TYPE_MAGIC);
		}

		if (victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_FROST) && number_percent() < flag_chance)
		{
			dam = number_range(1, wield->level / 7 + 1);
			dam += markup;
			act("$p freezes $n.", victim, wield, NULL, TO_ROOM, POS_RESTING);
			act("The cold touch of $p surrounds you with ice.", victim, wield, NULL, TO_CHAR, POS_RESTING);
			cold_effect(victim, wield->level, dam, TARGET_CHAR);
			victim_dead = damage(ch, victim, dam, 0, DAM_COLD, FALSE, TYPE_MAGIC);
		}

		if (victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_SHOCKING) && number_percent() < flag_chance)
		{
			dam = number_range(1, wield->level / 7 + 2);
			dam += markup;
			act("$n is struck by lightning from $p.", victim, wield, NULL, TO_ROOM, POS_RESTING);
			act("You are shocked by $p.", victim, wield, NULL, TO_CHAR, POS_RESTING);
			shock_effect(victim, wield->level, dam, TARGET_CHAR);
			victim_dead = damage(ch, victim, dam, 0, DAM_LIGHTNING, FALSE, TYPE_MAGIC);
		}

		if (victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_CORROSIVE) && number_percent() < flag_chance)
		{
			dam = number_range(1, wield->level / 7 + 1);
			dam += markup;
			act("$n's flesh is dissolved by $p.", victim, wield, NULL, TO_ROOM, POS_RESTING);
			act("Your flesh is dissolved by $p.", victim, wield, NULL, TO_CHAR, POS_RESTING);
			acid_effect(victim, wield->level, dam, TARGET_CHAR);
			victim_dead = damage(ch, victim, dam, 0, DAM_ACID, FALSE, TYPE_MAGIC);

		}

		if (victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_FLOODING) && number_percent() < flag_chance)
		{
			dam = number_range(1, wield->level / 7 + 1);
			dam += markup;
			act("$n is smothered in water from $p.", victim, wield, NULL, TO_ROOM, POS_RESTING);
			act("You are smothered in water from $p.", victim, wield, NULL, TO_CHAR, POS_RESTING);
			water_effect(victim, wield->level, dam, TARGET_CHAR);
			victim_dead = damage(ch, victim, dam, 0, DAM_DROWNING, FALSE, TYPE_MAGIC);
		}

		if (victim_dead)
			return TRUE;

		if (IS_WEAPON_STAT(wield, WEAPON_INFECTED))
		{
			AFFECT_DATA af;

			if (number_percent() <= (5 + (markup / 3)))
			{
				Cprintf(victim, "A disgusting disease enters your bloodstream.\n\r");
				act("$n is infected with a nasty disease by $p.", victim, wield, NULL, TO_ROOM, POS_RESTING);

				af.where = TO_AFFECTS;
				af.type = gsn_plague;
				af.level = wield->level * 3 / 4;
				af.duration = dice(1, 4);
				af.location = APPLY_STR;
				af.modifier = -1;
				af.bitvector = AFF_PLAGUE;
				affect_join(victim, &af);

				/* plague deals 2-5% damage now */
				dam = 1 + (victim->hit / number_range(20, 50));
				victim->mana -= dam / 2;
				victim->move -= dam / 2;
				victim_dead = damage(ch, victim, dam, gsn_plague, DAM_DISEASE, FALSE, TYPE_MAGIC);
			}
		}

		if (victim_dead)
			return TRUE;

		/* Vorpal flag revisited:
		   Slays mobs if ur lucky.
		   Coded by Starcrossed. */
		if (IS_WEAPON_STAT(wield, WEAPON_VORPAL) && !IS_SET(victim->imm_flags, IMM_VORPAL))
		{
			if (IS_NPC(victim) && number_range(1, 180 - markup) == 1)
			{
				victim->hit = 0;
				act("$n is utterly {RSLAIN{x by $p!!!", victim, wield, NULL, TO_ROOM, POS_RESTING);
				victim_dead = damage(ch, victim, 10, 0, DAM_NONE, FALSE, TYPE_MAGIC);
			}
		}

		if (victim_dead)
			return TRUE;

	}

	return FALSE;
}

// Returns the dt for a dragon's elemental hand to hand.
int
dragon_hand_to_hand_special(CHAR_DATA *ch)
{
	if(ch->race == race_lookup("blue dragon")) {
		return hit_lookup("shbite");
	}
	else if(ch->race == race_lookup("red dragon")) {
		return hit_lookup("flbite");
	}
	else if(ch->race == race_lookup("white dragon")) {
		return hit_lookup("frbite");
	}
	else if(ch->race == race_lookup("green dragon")) {
		return hit_lookup("pobite");
	}
	else if(ch->race == race_lookup("black dragon")) {
		return hit_lookup("acbite");
	}
	else
		return hit_lookup("hit");
}

// returns the minimum or maximum damage for hand to hand by race.
int
get_hand_to_hand_damage(CHAR_DATA *ch, int option)
{
	/* Samples:
	51 monk        15 to 47 (31)
	51 sliver	9 to 39 (24)
	51 dragon	9 to 39 (24)
	51 elf		6 to 31 (18)
	51 human	6 to 31 (18)
	51 giant	6 to 31 (18)
	51 kirre        8 to 35 (21)
	*/

	if (option == HAND_TO_HAND_MINIMUM) {
		if (ch->charClass == class_lookup("monk"))
			return (ch->level / 5) + 5;
		else if (ch->race == race_lookup("sliver"))
			return (ch->level / 6) + 1;
		else if (IS_DRAGON(ch))
			return (ch->level / 6) + 1;
		else if (ch->race == race_lookup("kirre"))
			return (ch->level / 7) + 1;
		else
			return (ch->level / 10) + 1;
	}

	if (option == HAND_TO_HAND_MAXIMUM) {
		if (ch->charClass == class_lookup("monk"))
			return (ch->level * 5 / 6) + 5;
		else if (ch->race == race_lookup("sliver"))
			return (ch->level * 3 / 4) + 1;
		else if (IS_DRAGON(ch))
			return (ch->level * 3 / 4) + 1;
		else if (ch->race == race_lookup("kirre"))
			return (ch->level * 2 / 3) + 1;
		else
			return (ch->level * 3 / 5) + 1;
	}

	return 0;
}

// Return dt by looking up a damage type based on a string.
int
hit_lookup(char *hit)
{
	int index;
	for (index = 0; index < MAX_DAMAGE_MESSAGE; index++) {
		if (!str_cmp(hit, attack_table[index].name))
			return index;
	}

	return -1;
}

// Returns the base percent chance to hit not counting armor class.
int
get_attack_rating(CHAR_DATA *ch, OBJ_DATA *wield)
{
	int ar = 0;
	AFFECT_DATA *paf = NULL;

	if (IS_NPC(ch)) {
		return GET_MOB_AR(ch->level) + GET_HITROLL(ch);
	}
	else if (!IS_NPC(ch)) {
		if (ch->charClass <= class_lookup("invoker"))
			ar = GET_MAGE_AR(ch->level);
		else if (ch->charClass <= class_lookup("druid"))
			ar = GET_CLERIC_AR(ch->level);
		else if (ch->charClass <= class_lookup("ranger"))
			ar = GET_WARRIOR_AR(ch->level);
		else if (ch->charClass <= class_lookup("runist"))
			ar = GET_THIEF_AR(ch->level);
		else if (ch->charClass <= class_lookup("monk"))
			ar = GET_WARRIOR_AR(ch->level);

		// Swords ignore some AC.
	        if (wield != NULL &&  wield->item_type == ITEM_WEAPON &&  (wield->value[0] == WEAPON_SWORD || wield->value[0] == WEAPON_KATANA))
                	ar = ar + 5;

		if (wield != NULL && (paf = affect_find(wield->affected, gsn_blade_rune)) != NULL && paf->location == APPLY_HITROLL)
			ar += wield->level;

		if (wield == get_eq_char(ch, WEAR_WIELD))
			return ar + get_main_hitroll(ch);
		else if (wield == get_eq_char(ch, WEAR_DUAL))
			return ar + get_dual_hitroll(ch);
		else if (wield == get_eq_char(ch, WEAR_RANGED))
			return ar + get_ranged_hitroll(ch);
		else
			return ar + get_natural_hitroll(ch);
	}

	return -1;
}

// Returns the percentage chance your AC protects you.
int
get_defense_rating(CHAR_DATA *victim, int dam_type)
{
	int ac_modifier = 0;

	switch (dam_type) {
	case (DAM_PIERCE):
		ac_modifier = GET_AC(victim, AC_PIERCE); break;
	case (DAM_BASH):
		ac_modifier = GET_AC(victim, AC_BASH); break;
	case (DAM_SLASH):
		ac_modifier = GET_AC(victim, AC_SLASH); break;
	default:
		ac_modifier = GET_AC(victim, AC_EXOTIC); break;
	};

	// Make it positive
	ac_modifier = 0 - ac_modifier;
	ac_modifier = (ac_modifier - 100) / 5;

	if (victim->position < POS_FIGHTING)
		ac_modifier -= 10;

	if (victim->position < POS_RESTING)
		ac_modifier -= 20;

	return ac_modifier;
}


// Determine if the character has attack speed modifiers
int get_attack_speed(CHAR_DATA *ch)
{
	// Base attack speed of zero, plus gear.
	int attack_speed = ch->attack_speed;
	OBJ_DATA *wield = get_eq_char(ch, WEAR_WIELD);
	OBJ_DATA *dual = get_eq_char(ch, WEAR_DUAL);;

	// Size modifier
	if(ch->size < SIZE_MEDIUM) {
        	attack_speed += 5;
	}

	if(ch->size > SIZE_MEDIUM) {
		attack_speed -= 5;
	}

	if(wield != NULL && wield->item_type == ITEM_WEAPON) {
		// Daggers are fast
		if(wield->value[0] == WEAPON_DAGGER)
        		attack_speed += 10;

		// Twohanders are slow
		if(IS_WEAPON_STAT(wield, WEAPON_TWO_HANDS)) 
	        	attack_speed -= 10;
	}

	if(dual != NULL && dual->item_type == ITEM_WEAPON) {
		// Daggers are fast
		if(dual->value[0] == WEAPON_DAGGER)
        		attack_speed += 5;
	}

	return attack_speed;
}


// Returns the PERCENTAGE increase in damage due to race alone.
int
get_race_damage_modifiers(CHAR_DATA* ch, OBJ_DATA *wield) {
	int bonus = 0;

	if (IS_NPC(ch))
		return 0;

	if (wield == NULL || wield->item_type != ITEM_WEAPON)
		return 0;

	if (ch->race == race_lookup("dwarf") 
	&& wield->value[0] == weapon_lookup("axe"))
		bonus = 20;

	if (ch->race == race_lookup("troll") 
	&& wield->value[0] == weapon_lookup("axe"))
		bonus = 20;

	if (ch->race == race_lookup("giant")) 
		bonus = 20;

	if (ch->race == race_lookup("elf") 
	&& wield->value[0] == weapon_lookup("sword"))
		bonus = 20;

	if (ch->race == race_lookup("gargoyle") 
	&& wield->value[0] == weapon_lookup("polearm"))
		bonus = 20;

	if (ch->race == race_lookup("kirre") 
	&& wield->value[0] == weapon_lookup("whip"))
		bonus = 20;

	return bonus;
}

// Returns the percentage damage increase due to class/reclass alone.
int
get_class_damage_modifiers(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield) {
	DESCRIPTOR_DATA *d;
	CHAR_DATA *rch;
	int bonus = 0, war_group = FALSE;
	AFFECT_DATA *paf = NULL;
	int targets = 0;

	if(IS_NPC(ch))
		return 0;

	if(ch->charClass == class_lookup("monk")
	&& is_affected(ch, gsn_stance_tiger)) {
		// Small bonus with exotics
		if(wield != NULL
	        && wield->value[0] == weapon_lookup("exotic"))
			bonus = 20;
		// Also bonus with hand to hand
		else if(wield == NULL)
			bonus = 20;
	}

	// Defensive stance lowers damage
	if(is_affected(ch, gsn_stance_mantis))
		bonus = -75;

	// Runists use pole weapons
        if(ch->charClass == class_lookup("runist")
	&& wield != NULL
	&& (wield->value[0] == weapon_lookup("spear")
	|| wield->value[0] == weapon_lookup("polearm")))
		bonus = 10;

	// Ranger size bonus
	// Rangers of all kinds pwn giants
	if (ch->charClass == class_lookup("ranger")
	&& (ch->size < victim->size))
		bonus = 10 * victim->size;

	// Paladin alignment bonus
	if (is_affected(ch, gsn_guardian)
	&& IS_AFFECTED(ch, AFF_PROTECT_EVIL) && IS_EVIL(victim))
		bonus += 30;

	if (is_affected(ch, gsn_guardian)
	&& IS_AFFECTED(ch, AFF_PROTECT_GOOD) && IS_GOOD(victim))
		bonus += 30;

	// Bounty hunters
	if (!IS_NPC(ch) && ch->reclass == reclass_lookup("bounty hunter")) {
		if (!IS_NPC(victim)) {
			if (victim->bounty < victim->level * 10)
				bonus += -40;
		}
	}

	// Warlords
	// If ch is a warlord
	if(ch->reclass == reclass_lookup("warlord")) {
		war_group = FALSE;
		for (d = descriptor_list; d != NULL; d = d->next) {
			rch = d->character;

			if(is_same_group(ch, rch)
			&& !IS_NPC(rch)
			&& rch != ch) {
				war_group = TRUE;
				break;
			}
		}
		// Warlord alone
		if(!war_group) {
			bonus -= 30;
		}
	}
	// Grouped with a warlord? Nice bonus.
	for (d = descriptor_list; d != NULL; d = d->next) {
		rch = d->character;
		war_group = FALSE;

		if (is_same_group(ch, rch)
		&& !IS_NPC(rch)) 
			war_group = TRUE;

		if (war_group
		&& number_percent() < get_skill(rch, gsn_leadership)) {
			check_improve(rch, gsn_leadership, TRUE, 4);
       			bonus += 30;
			break;
		}
	}

	// Zealots with zeal
 	if(is_affected(ch, gsn_zeal)) {
		for(rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
			if(rch->fighting == ch)
				targets++;
		}
		if(targets > 5)
			targets = 5;
		bonus += (20 * targets);
		bonus += (100 - (ch->hit * 100 / MAX_HP(ch))) * 4 / 5;
	}

	// Psions with fury
	if((paf = affect_find(ch->affected, gsn_fury)) != NULL) {
		bonus += paf->modifier * 10;
	}

	// Psion penalty, need for mana!
	if(ch->reclass == reclass_lookup("psion")) {
		bonus -= (100 - (ch->mana * 100 / MAX_MANA(ch))) * 3 / 4;
	}

	// Venari clan spell.
	if(IS_DRAGON(victim)
	&& is_affected(ch, gsn_dragonbane)) {
		bonus += 40;
	}

	// Gargoyles against fliers
	if(ch->race == race_lookup("gargoyle")
	&& IS_AFFECTED(victim, AFF_FLYING)) {
		bonus += 20;
	}

	return bonus;
}

// Returns the bonus from weapon flags which add directly to damage.
// Note the flags are done order of increasing effect, so only the largest
// bonus will be returned. Flags DO NOT STACK!
int
get_flag_damage_modifiers(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, int dam_type)
{
	int bonus = 0;

	if (wield == NULL || wield->item_type != ITEM_WEAPON)
		return 0;

	// Big two handers add damage on every hit
	if (IS_WEAPON_STAT(wield, WEAPON_TWO_HANDS)) {
		// Large folks without shield
		if (ch->size > SIZE_LARGE && get_eq_char(ch, WEAR_SHIELD) == NULL)
			bonus = 70;
		// Normal folks without shield
		else if (get_eq_char(ch, WEAR_SHIELD) == NULL)
			bonus = 50;
		// Large folks with shield and big weapon
		else
			bonus = 30;
	}

	if (IS_SET(victim->vuln_flags, VULN_IRON))
	{
		if (!str_cmp(wield->material, "iron"))
			bonus = UMAX(bonus, 50);
	}

	// Sharp works occasionally, but NOT on gargoyles.
	if (IS_WEAPON_STAT(wield, WEAPON_SHARP) && victim->race != race_lookup("gargoyle")) {
		if (number_percent() <= 50)
			bonus = UMAX(bonus, 50);
	}

	// Dull slays gargoyles.
	if (IS_WEAPON_STAT(wield, WEAPON_DULL) && victim->race == race_lookup("gargoyle")) {
		bonus = UMAX(bonus, 70);
	}

	// Sharp is crap against gargoyles.
	if (IS_WEAPON_STAT(wield, WEAPON_SHARP) && victim->race == race_lookup("gargoyle")) {
		bonus = -80;
	}

	if (IS_WEAPON_STAT(wield, WEAPON_DULL) && victim->race != race_lookup("gargoyle")) {
		bonus = -80;
	}

	if (IS_WEAPON_STAT(wield, WEAPON_BLUNT)) {
		bonus = -80;
	}

	/* dragon slaying */
	if(IS_WEAPON_STAT(wield, WEAPON_DRAGON_SLAYER) && IS_DRAGON(victim))
		bonus = UMAX(bonus, 70);

	// Check for vuln
        switch (check_immune(victim, dam_type)) {
                case (IS_RESISTANT):
                        bonus = -80;
			break;
                case (IS_VULNERABLE):
                        bonus = UMAX(bonus, 50);
        }

	// Special crafted weapon modifiers... they STACK
        // so beware
	if(IS_WEAPON_STAT(wield, WEAPON_HOLY)
	&& IS_EVIL(victim))
		bonus += 60;

	if(IS_WEAPON_STAT(wield, WEAPON_UNHOLY)
	&& IS_GOOD(victim))
		bonus += 60;

	if(IS_WEAPON_STAT(wield, WEAPON_POLAR)
	&& IS_NEUTRAL(victim))
		bonus += 60;

	return bonus;
}


int
get_weapon_hit_bonus(OBJ_DATA *weapon) {
	AFFECT_DATA *paf = NULL;
	int bonus = 0;

	if (weapon == NULL)
		return 0;

	for (paf = weapon->affected;paf != NULL; paf = paf->next) {
		if (paf->location == APPLY_HITROLL)
			bonus += paf->modifier;
	}

	if (!weapon->enchanted) {
		for (paf = weapon->pIndexData->affected;paf != NULL; paf = paf->next) {
			if (paf->location == APPLY_HITROLL)
				bonus += paf->modifier;
		}
	}

	return bonus;
}

int
get_weapon_dam_bonus(OBJ_DATA *weapon) {
	AFFECT_DATA *paf = NULL;
	int bonus = 0;

	if (weapon == NULL)
		return 0;

	for (paf = weapon->affected;paf != NULL; paf = paf->next) {
		if (paf->location == APPLY_DAMROLL)
			bonus += paf->modifier;
	}

	if (!weapon->enchanted) {
		for (paf = weapon->pIndexData->affected;paf != NULL; paf = paf->next) {
			if (paf->location == APPLY_DAMROLL)
				bonus += paf->modifier;
		}
	}
	
	return bonus;
}

int
get_main_hitroll(CHAR_DATA *ch) {
	return GET_HITROLL(ch) 
		- get_weapon_hit_bonus(get_eq_char(ch, WEAR_DUAL))
		- get_weapon_hit_bonus(get_eq_char(ch, WEAR_RANGED))
                - get_weapon_hit_bonus(get_eq_char(ch, WEAR_AMMO));
}

int
get_dual_hitroll(CHAR_DATA *ch) {
	return GET_HITROLL(ch) 
		- get_weapon_hit_bonus(get_eq_char(ch, WEAR_WIELD))
		- get_weapon_hit_bonus(get_eq_char(ch, WEAR_RANGED))
		- get_weapon_hit_bonus(get_eq_char(ch, WEAR_AMMO));
}

int
get_ranged_hitroll(CHAR_DATA *ch) {
	return ch->hitroll+str_app[get_curr_stat(ch,STAT_DEX)].tohit 
        	- get_weapon_hit_bonus(get_eq_char(ch, WEAR_WIELD))
        	- get_weapon_hit_bonus(get_eq_char(ch, WEAR_DUAL));
}

int
get_main_damroll(CHAR_DATA *ch) {
	return GET_DAMROLL(ch) 
		- get_weapon_dam_bonus(get_eq_char(ch, WEAR_DUAL))
		- get_weapon_dam_bonus(get_eq_char(ch, WEAR_RANGED))
		- get_weapon_dam_bonus(get_eq_char(ch, WEAR_AMMO));
}

int
get_dual_damroll(CHAR_DATA *ch) {
	return GET_DAMROLL(ch) 
		- get_weapon_dam_bonus(get_eq_char(ch, WEAR_WIELD))
		- get_weapon_dam_bonus(get_eq_char(ch, WEAR_RANGED))
		- get_weapon_dam_bonus(get_eq_char(ch, WEAR_AMMO));
}

int
get_ranged_damroll(CHAR_DATA *ch) {
	return ch->damroll+str_app[get_curr_stat(ch,STAT_DEX)].todam
        	- get_weapon_dam_bonus(get_eq_char(ch, WEAR_WIELD))
        	- get_weapon_dam_bonus(get_eq_char(ch, WEAR_DUAL));
}

int
get_natural_damroll(CHAR_DATA *ch) {
	return GET_DAMROLL(ch)
		- get_weapon_dam_bonus(get_eq_char(ch, WEAR_WIELD))
		- get_weapon_dam_bonus(get_eq_char(ch, WEAR_DUAL))
		- get_weapon_dam_bonus(get_eq_char(ch, WEAR_RANGED))
		- get_weapon_dam_bonus(get_eq_char(ch, WEAR_AMMO));
}

int
get_natural_hitroll(CHAR_DATA *ch) {
        return GET_HITROLL(ch)
                - get_weapon_hit_bonus(get_eq_char(ch, WEAR_WIELD))
                - get_weapon_hit_bonus(get_eq_char(ch, WEAR_DUAL))
		- get_weapon_hit_bonus(get_eq_char(ch, WEAR_RANGED))
		- get_weapon_hit_bonus(get_eq_char(ch, WEAR_AMMO));
}

/*
 * Weapon_hit
 * One_hit: Hit one guy once.
 * Returns TRUE if target died.
 */
int
weapon_hit(CHAR_DATA *ch, CHAR_DATA *victim,
		   OBJ_DATA * wield,	// Can be null for hand to hand to attacks
		   int dt) 				// Skill number if applicable
{
	int weapon_min=0, weapon_max=0, dam_min=0, dam_max=0;
	int special=0, sn=-1, dam_type=-1;
	int skill=0, hit_chance=0, defense_chance=0;
	int spec_chance=0, damroll_used=0;
	int dam=0, victim_dead=FALSE;
	int blood_dam=0;
	// Store the dt elsewhere since we change it
	int skill_type = 0;
	// use this much damroll... tweak it carefully
	float damroll_multiple = 0.6;
	AFFECT_DATA *paf = NULL;
	int phased = FALSE;
	int kensai = FALSE;
	int critical_chance = 0;

	if(dt == gsn_backstab
	|| dt == gsn_beheading)
		skill_type = dt;

	if(skill_type == 0
	&& wield != NULL
	&& obj_is_affected(wield, gsn_burst_rune)
	&& ch->burstCounter == 8)
		skill_type = gsn_burst_rune;

	// just in case
	if (victim == ch || ch == NULL || victim == NULL
	|| victim->position == POS_DEAD)
		return FALSE;

	// stop wierd attacks
	if (ch->in_room != victim->in_room
	&& (wield == NULL
	|| (wield != NULL && wield->item_type != ITEM_THROWING))) {
		return FALSE;
	}

	// Set up the type of attack (claw, slash, blast, etc)
	special = TYPE_HIT;

	if (wield != NULL && wield->item_type == ITEM_WEAPON)
		dt = wield->value[3];
	else if (wield != NULL && wield->item_type == ITEM_THROWING)
		dt = wield->value[2];
	// Mobs/Hand to Hand
	else
		dt = ch->dam_type;

	// Set up the type of damage (bash, slash, pierce, fire, holy, etc)
	if (wield != NULL)
	{
		if (wield->item_type == ITEM_WEAPON)
			dam_type = attack_table[wield->value[3]].damage;
		else if (wield->item_type == ITEM_THROWING)
			dam_type = attack_table[wield->value[2]].damage;
	}
	else
		dam_type = attack_table[ch->dam_type].damage;

	// Better safe than sorry.
	if (dam_type == -1)
		dam_type = DAM_BASH;

	// get the weapon skill
	if (wield == get_eq_char(ch, WEAR_WIELD))
	{
		sn = get_weapon_sn(ch, WEAR_WIELD);
		skill = get_skill(ch, sn);
	}
	else if (wield == get_eq_char(ch, WEAR_DUAL))
	{
		sn = get_weapon_sn(ch, WEAR_DUAL);
		skill = get_skill(ch, sn);
	}
	else if (dt == gsn_hurl) {
		sn = gsn_hurl;
		skill = get_skill(ch, gsn_hurl);
	}
	else if (wield != NULL && wield->item_type == ITEM_THROWING)
	{
		sn = gsn_throw;
		skill = get_skill(ch, gsn_throw);
	}
	else {
		sn = gsn_hand_to_hand;
		skill = get_skill(ch, gsn_hand_to_hand);
	}

	// This is a dumb hack so bash doesn't lower melee damage... much.
	if(sn != gsn_throw) {
		skill += 25;
		skill = URANGE(25, skill, 100);
	}

	// Revise the dt and dam_type if they are using hth, by race.
	if (sn == gsn_hand_to_hand)
	{
		if (ch->race == race_lookup("sliver"))
		{
			dam_type = DAM_PIERCE;
			dt = hit_lookup("thrust");
		}
		else if (IS_DRAGON(ch))
		{
			dam_type = DAM_SLASH;
			dt = hit_lookup("claw");
		}
		else if (ch->race == race_lookup("giant"))
		{
			dam_type = DAM_BASH;
			dt = hit_lookup("crush");
		}
		else if (ch->race == race_lookup("kirre"))
		{
			dam_type = DAM_SLASH;
			dt = hit_lookup("scratch");
		}
		else if (ch->race == race_lookup("marid"))
		{
			dam_type = DAM_BASH;
			dt = hit_lookup("entangle");
		}
		// Martial Arts for some reclasses
		if(ch->reclass == reclass_lookup("hermit")
		|| ch->reclass == reclass_lookup("barbarian")
		|| ch->reclass == reclass_lookup("warlord")
		|| ch->reclass == reclass_lookup("cavalier")
		|| ch->reclass == reclass_lookup("templar")
		|| ch->reclass == reclass_lookup("zealot"))
		{
			int randhit = number_range(1, 6);
			switch (randhit)
			{
			case 1:
				dt = hit_lookup("punch"); break;
			case 2:
				dt = hit_lookup("crush"); break;
			case 3:
				dt = hit_lookup("stomp"); break;
			case 4:
				dt = hit_lookup("strangle"); break;
			case 5:
				dt = hit_lookup("chop"); break;
			case 6:
				dt = hit_lookup("smash"); break;
			}
		}
		// Dragon's bite overrides.
		if(IS_DRAGON(ch)
		&& number_percent() < 15) {
			dt = dragon_hand_to_hand_special(ch);
			dam_type = attack_table[dt].damage;
		}

		// Monks fighting styles override race
		if(is_affected(ch, gsn_stance_turtle)) {
			dt = hit_lookup("crush");
			dam_type = DAM_BASH;
		}
		else if(is_affected(ch, gsn_stance_tiger)) {
			dt = hit_lookup("claw");
			dam_type = DAM_SLASH;
		}
		else if(is_affected(ch, gsn_stance_mantis)) {
			dt = hit_lookup("thrust");
			dam_type = DAM_PIERCE;
		}
	}

	// Calculate chance to avoid by AC
	hit_chance = get_attack_rating(ch, wield);
	hit_chance = ((7 * hit_chance) + ((3 * hit_chance * skill) / 100.0)) / 10;
	defense_chance = get_defense_rating(victim, dam_type);
	hit_chance -= defense_chance;

	// Blindess and stuff
	if(!can_see(ch, victim)
	&& number_percent() > get_skill(ch, gsn_blindfighting))
		hit_chance -= 20;

	// Backstabs hit more
	if(skill_type == gsn_backstab)
		hit_chance += ch->level / 2;
	if(skill_type == gsn_burst_rune)
		hit_chance += ch->level / 5;

	// Max chance to hit
	hit_chance = URANGE(5, hit_chance, 95);

	if (skill_type == 0 && wield != NULL && obj_is_affected(wield, gsn_burst_rune))
		ch->burstCounter++;

	if (wield != NULL && IS_WEAPON_STAT(wield, WEAPON_PHASE) && number_percent() < 15) {
		Cprintf(ch, "Your weapon phases right through their defenses!\n\r");
		phased = TRUE;
	}

	if (is_affected(ch, gsn_stance_kensai) && number_percent() < 20) {
		phased = TRUE;
		kensai = TRUE;
	}

	if (is_affected(ch, gsn_ambush) && number_percent() < 40) {
		kensai = TRUE;
	}

	// This should have been here a long time ago.
	check_killer(ch, victim);

	// Defenses... checked on normal hits only.
	if(skill_type == 0) {
		if(!phased && check_parry(ch, victim))
			return FALSE;
		if(!kensai && check_dodge(ch, victim))
			return FALSE;
		if(!phased && check_shield_block(ch, victim))
			return FALSE;
		if(!kensai && check_tumbling(ch, victim))
			return FALSE;
	}

	// Big thrills here
	if(number_percent() > hit_chance)
	{
       		// Missed, sucker.
		if(skill_type > 0)
			damage(ch, victim, 0, skill_type, dam_type, TRUE, TYPE_SKILL);
		else
			damage(ch, victim, 0, dt, dam_type, TRUE, special);
		tail_chain();
		return FALSE;
	}

	// Ultimate ninja evasion! Always works against normal hits.
	if((paf = affect_find(victim->affected, gsn_evasion)) != NULL 
	&& skill_type == 0
	&& number_percent() < 50) {
		if(paf->duration > 0) {
			Cprintf(victim, "You evade the attack.\n\r");
			act("$N evades your attack.", ch, NULL, victim, TO_CHAR, POS_RESTING);
			paf->duration--;
			return FALSE;
		}
		else {
			Cprintf(victim, "You evade the attack, but your abilities return to normal.\n\r");
			act("$N evades your attack.", ch, NULL, victim, TO_CHAR, POS_RESTING);
			affect_strip(victim, gsn_evasion);
			return FALSE;
		}
	}
	// You hit! Let's calculate damage.
	if (IS_NPC(ch) && wield == NULL) {
		weapon_min = ch->damage[DICE_NUMBER];
		weapon_max = ch->damage[DICE_NUMBER] * ch->damage[DICE_TYPE];
		dam_min = weapon_min;
		dam_max = weapon_max;
	}
	else
	{
		if (sn != -1)
			check_improve(ch, sn, TRUE, 5);
		if (wield != NULL)
		{
			if (wield->item_type == ITEM_WEAPON) {
				if((paf = affect_find(wield->affected, gsn_magic_sheath)) != NULL
				&& paf->extra == SHEATH_DICECOUNT) {
					weapon_min = wield->value[1] + 1;
					weapon_max = (wield->value[1] + 1) * wield->value[2];
				}
				else if((paf = affect_find(wield->affected, gsn_magic_sheath)) != NULL
				&& paf->extra == SHEATH_DICETYPE) {
					weapon_min = wield->value[1];
					weapon_max = wield->value[1] * (wield->value[2] + 1);
				}
				else {
					weapon_min = wield->value[1];
					weapon_max = wield->value[1] * wield->value[2];
				}
			}
			else if (wield->item_type == ITEM_THROWING) {
				weapon_min = wield->value[0];
				weapon_max = wield->value[0] * wield->value[1];
			}

			// Base damage
			dam_min = weapon_min;
			dam_max = weapon_max;

			// Weapon flags which modify damage directly:
			dam_min += (weapon_min * get_flag_damage_modifiers(ch, victim, wield, dam_type)) / 100;
			dam_max += (weapon_max * get_flag_damage_modifiers(ch, victim, wield, dam_type)) / 100;
		}
		else
		{
			// Resolve hand to hand damage here.
			weapon_min = get_hand_to_hand_damage(ch, HAND_TO_HAND_MINIMUM);
			weapon_max = get_hand_to_hand_damage(ch, HAND_TO_HAND_MAXIMUM);

			// Base damage
			dam_min = weapon_min;
			dam_max = weapon_max;
		}
	}

	/*
	 * damage bonuses
	 */

	// Damroll
	if(wield == get_eq_char(ch, WEAR_DUAL))
		damroll_used = get_dual_damroll(ch);
	else if(wield == get_eq_char(ch, WEAR_WIELD))
		damroll_used = get_main_damroll(ch);
	else
		damroll_used = get_natural_damroll(ch);

	dam_min += (int)(damroll_used * damroll_multiple);
	dam_max += (int)(damroll_used * damroll_multiple);

	// Race modifiers
	dam_min += (weapon_min * get_race_damage_modifiers(ch, wield)) / 100.0;
	dam_max += (weapon_max * get_race_damage_modifiers(ch, wield)) / 100.0;

	// Class modifiers
	dam_min += (weapon_min * get_class_damage_modifiers(ch, victim, wield)) / 100.0;
	dam_max += (weapon_max * get_class_damage_modifiers(ch, victim, wield)) / 100.0;

	if (number_percent() < get_skill(ch, gsn_enhanced_damage))
	{
		check_improve(ch, gsn_enhanced_damage, TRUE, 2);

		// Axes inflict more enhanced damage.
		if (wield != NULL &&  wield->item_type == ITEM_WEAPON &&  wield->value[0] == WEAPON_AXE)
			dam_min += ch->level / 8;

		dam_min += ch->level / 6;
		if (dam_min > dam_max)
			dam_max = dam_min + 1;
	}

	if (number_percent() < get_skill(ch, gsn_intense_damage))
	{
		check_improve(ch, gsn_intense_damage, TRUE, 4);

		dam_min += ch->level / 5;
		if (dam_min > dam_max)
			dam_max = dam_min + 1;
	}

	if (number_percent() < get_skill(ch, gsn_extreme_damage))
	{
		check_improve(ch, gsn_extreme_damage, TRUE, 4);

		dam_min += ch->level / 5;
		if (dam_min > dam_max)
			dam_max = dam_min + 1;
	}

	if (victim->position < POS_FIGHTING)
		dam_max = dam_max * 3 / 2;

	if (skill_type == gsn_backstab && wield != NULL)
	{
		special = TYPE_SKILL;
		if (wield->value[0] == WEAPON_DAGGER) {
			dam_min += weapon_max + number_range(ch->level / 3, ch->level);
			dam_max += weapon_max + number_range(ch->level / 3, ch->level);
		}
		else {
			dam_min += weapon_max + number_range(1, ch->level / 2);
			dam_max += weapon_max + number_range(1, ch->level / 2);
		}
	}

	// Beheading hurts.
	if (skill_type == gsn_beheading) {
		special = TYPE_SKILL;
		dam_min = dam_min * 4 / 3;
		dam_max = dam_max + (dam_min * 1 / 3);
	}

	if (is_affected(ch, gsn_ambush)) {
		dam_min += weapon_min;
		dam_max += weapon_max;
	}

	if (is_affected(ch, gsn_seppuku)) {
		blood_dam = ch->hit / 10; 

		victim_dead = damage(ch, ch, blood_dam, gsn_seppuku, dam_type
, FALSE, TYPE_MAGIC);
		if(victim_dead) {
			return TRUE;
		}
		dam_min += blood_dam;
		dam_max += blood_dam;
		affect_strip(ch, gsn_seppuku);
	}

	critical_chance = 0;

	// Rogue critical hits
	if (ch->reclass == reclass_lookup("rogue"))
		critical_chance += 6;

	// Vorpal critical hits
	if (wield != NULL && IS_WEAPON_STAT(wield, WEAPON_VORPAL))
		critical_chance += 6;

	if (number_percent() <= critical_chance) {
		Cprintf(ch, "You score a {YCRITICAL HIT!{x\n\r");
		dam_min += ch->level;
		dam_max += ch->level;
	}

	if(!IS_NPC(victim)
	&& ch->race == race_lookup("dwarf")
	&& ch->remort && ch->rem_sub == 1
	&& ch->pcdata->condition[COND_DRUNK] > 10) {
		dam_min += ch->level / 5;
	}

	if (skill_type == gsn_burst_rune) {
		paf = affect_find(wield->affected, gsn_burst_rune);
		dam = spell_damage(ch, victim, wield->level, SPELL_DAMAGE_HIGH, TRUE);
		ch->burstCounter = 0;
		special = TYPE_MAGIC;
		if (paf != NULL) {
			if (paf->modifier == gsn_acid_blast) {
				Cprintf(ch, "Your attack explodes into corrosive acid!\n\r");
				dam_type = DAM_ACID;
			}

			if (paf->modifier == gsn_ice_bolt) {
				Cprintf(ch, "The air is tinged with frost as you strike!\n\r");
				dam_type = DAM_COLD;
			}

			if (paf->modifier == gsn_blast_of_rot) {
				Cprintf(ch, "Your weapon discharges a virulent spray!\n\r");
				dam_type = DAM_POISON;
			}

			if (paf->modifier == gsn_lightning_bolt) {
				Cprintf(ch, "You strike with the force of a thunder bolt!\n\r");
				dam_type = DAM_LIGHTNING;
			}

			if (paf->modifier == gsn_flamestrike) {
				Cprintf(ch, "A blast of flames explodes from your weapon!\n\r");
             			dam_type = DAM_FIRE;
			}

			if (paf->modifier == gsn_tsunami) {
				Cprintf(ch, "Your weapon carries the strength of the tides!\n\r");
				dam_type = DAM_DROWNING;
				// fix for halfing in spell_damage
				if (victim->race == race_lookup("dwarf"))
					dam *= 2;
			}
		}
	}

	// Specialize adds 12 damage occasionally.
	if (!IS_NPC(ch) && ch->pcdata->specialty == sn) {
		spec_chance = (ch->pcdata->learned[sn] - 100);
		if (spec_chance > 0 && number_percent() < spec_chance) {
			special |= TYPE_SPECIALIZED;
			dam_min += ch->level / 4;
			dam_max += ch->level / 4;
		}
    	}

 	if (skill_type != gsn_burst_rune)
	{
		dam = (number_range(dam_min, dam_max) * skill) / 100;

		// Gargoyle mass protection helps weapons
		if (is_affected(victim, gsn_mass_protect))
			dam = dam * 5 / 6;
	}

	if (dam <= 0)
		dam = 1;

	if (skill_type > 0)
		victim_dead = damage(ch, victim, dam, skill_type, dam_type, TRUE, special);
	else {
		victim_dead = damage(ch, victim, dam, dt, dam_type, TRUE, special);
	}

	// Guy died, stop hurting him
	if (victim_dead)
		return victim_dead;

	// Doublestrike for samurai with katana
	if(number_percent() < get_skill(ch, gsn_doublestrike) / 10
	&& wield != NULL
	&& wield->item_type == ITEM_WEAPON
	&& wield->value[0] == WEAPON_KATANA) {
		victim_dead = damage(ch, victim, dam, gsn_doublestrike, dam_type, TRUE, TYPE_SKILL);
		check_improve(ch, gsn_doublestrike, TRUE, 5);
		if(victim_dead)
			return victim_dead;
	}

	// Guy is alive, but we stop here for throwing items.
	if (wield != NULL && wield->item_type == ITEM_THROWING)
		return victim_dead;

	victim_dead = handle_weapon_flags(ch, victim, wield, dt);

	if(victim_dead)
		return victim_dead;

	// Maces can knock people over
        if(wield != NULL
        && wield->item_type == ITEM_WEAPON
        && wield->value[0] == WEAPON_MACE
	&& number_percent() < 5) {
		Cprintf(ch, "A crushing blow drives them to their knees!\n\r");
		Cprintf(victim, "A crushing blow knocks you down!\n\r");
		victim->position = POS_RESTING;
	}

	if (is_affected(ch, gsn_pacifism) && !IS_NPC(victim))
	{
		affect_strip(ch, gsn_pacifism);
	}

	tail_chain();
	return victim_dead;
}

void
troll_regrowth(CHAR_DATA * ch)
{

	CHAR_DATA *victim;
	AFFECT_DATA af2;
	int mobcount;

	mobcount = 0;
	for (victim = char_list; victim != NULL; victim = victim->next)
	{
		if (victim->master == ch && IS_NPC(victim))
		{
			if (victim->pIndexData == get_mob_index(MOB_VNUM_TROLL_ARM)
				|| victim->pIndexData == get_mob_index(MOB_VNUM_TROLL_LEG))
				mobcount++;

			if (mobcount > 5)
			{
				return;
			}
		}
	}

	if (number_percent() > 50)
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_TROLL_LEG));
	}
	else
	{
		victim = create_mobile(get_mob_index(MOB_VNUM_TROLL_ARM));
	}

	Cprintf(ch, "Your chopped off limb joins the battle!!\n\r");
	check_improve(ch, gsn_regrowth, TRUE, 2);
	act("$n's chopped off limb joins the fight!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	char_to_room(victim, ch->in_room);
	size_mob(ch, victim, ch->level * 6 / 7);
	SET_BIT(victim->toggles, TOGGLES_NOEXP);
	if (victim->master)
	{
		stop_follower(victim);
	}
	add_follower(victim, ch);

	victim->leader = ch;
	af2.where = TO_AFFECTS;
	af2.type = gsn_regrowth;
	af2.level = ch->level;
	af2.duration = ch->level;
	af2.location = 0;
	af2.modifier = 0;
	af2.bitvector = AFF_CHARM;
	affect_to_char(victim, &af2);
}

// Support function for damage
void
strip_noncombat_spells(CHAR_DATA *ch, CHAR_DATA *victim)
{
	AFFECT_DATA *paf = NULL;

	// If you hurt your charm, they stop following you.
 	if (victim->master == ch)
		stop_follower(victim);

	// If you somehow manage to hurt your master (area spell) break charm.
	if (ch->master == victim)
		stop_follower(ch);

	// Some spells wear off after combat starts
	// Invis person attacks
	if (IS_AFFECTED(ch, AFF_INVISIBLE)) {
		affect_strip(ch, gsn_invisibility);
		affect_strip(ch, gsn_mass_invis);
		REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
		act("$n fades into existence.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	// Invis person gets attacked
	if (IS_AFFECTED(victim, AFF_INVISIBLE)) {
		affect_strip(victim, gsn_invisibility);
		affect_strip(victim, gsn_mass_invis);
		REMOVE_BIT(victim->affected_by, AFF_INVISIBLE);
		act("$n fades into existence.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	if (is_affected(ch, gsn_oculary) && (ch->race != race_lookup("troll") || ch->rem_sub != 2)) {
		affect_strip(ch, gsn_oculary);
		act("$n ripples into existence.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	if (is_affected(victim, gsn_oculary) && (victim->race != race_lookup("troll") || victim->rem_sub != 2)) {
		affect_strip(victim, gsn_oculary);
		act("$n ripples into existence.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	if (IS_AFFECTED(ch, AFF_HIDE)) {
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
		act("$n is no longer hidden.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	if (IS_AFFECTED(victim, AFF_HIDE)) {
		REMOVE_BIT(victim->affected_by, AFF_HIDE);
		act("$n is no longer hidden.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	if ((is_affected(ch, gsn_cloak_of_mind)) && (ch->fighting != NULL)) {
		affect_strip(ch, gsn_cloak_of_mind);
		act("$n is no longer invisible to mobiles.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	if ((is_affected(victim, gsn_cloak_of_mind)) && (victim->fighting != NULL)) {
		affect_strip(victim, gsn_cloak_of_mind);
		act("$n is no longer invisible to mobiles.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}
}

/*
 * Inflict Damage, reduced by affects and resistances
 * Strips spells that don't mix when damage is done
 * (example invis, cloak of mind)
 * Handles change of position, the death of special charmies
 * (example eyes, homonoculus)
 * Calls handle_death to deal with other stuff.
 * Returns true is victim died.
 *
 * Note: to do anonymous damage, set a character to damage themselves, but
 * set the special to TYPE_ANONYMOUS. This is needed for when damage is dealt
 * without a player source, so no one can claim a pk.
 */

int
damage(CHAR_DATA * ch, CHAR_DATA * victim,
	int dam,       // Point of damage
	int dt,        // Skill that dealt damage
	int dam_type,  // Type of damage (bash, fire, holy, etc)
	bool show,     // Display message true/false
	int special)   // Bits for other information
	               // TYPE_HIT          normal attack
                              // TYPE_MAGIC        magical damage  (acid blast)
                              // TYPE_SKILL        skill damage (ex snake bite)
                              // TYPE_SPECIALIZED  specialized hit
                              // TYPE_ANONYMOUS    anonymous damage
{
	CHAR_DATA *fch;
	char buf[MAX_INPUT_LENGTH];
	bool immune;
	int in_arena;
	int tattoos = 0;
	int chance = 0;
	int hours = 0;
	int dam_reduction = 0;
	AFFECT_DATA *paf = NULL;

	// Some idiot checks...
	if ( ch == NULL ||
	     victim == NULL ||
	     ch->in_room == NULL ||
	     victim->in_room == NULL ||
	     victim->position == POS_DEAD) {
		return TRUE;
	}

	// Check for stupidity too.
	if (dam > 1000 && !IS_IMMORTAL(ch) && !IS_NPC(ch)) {
	        if (dt != TYPE_UNDEFINED)
			sprintf(buf, "Damage: %d: %s dealt more than 1000 damage!", dam, skill_table[dt].name);
		else
			sprintf(buf, "Damage: %d: Undefined type dealt more than 1000 damage!", dam);
		bug(buf, 0);
		dam = 1;
		Cprintf(ch, "Don't even try and do that much damage.\n\r");
	}

	// If we missed, burst rune is reset
	if (dam == 0 && dt == gsn_burst_rune) {
		ch->burstCounter = 0;
	}

	// Also in multi hit.
	if (!IS_NPC(ch) && !IS_NPC(victim) && ch != victim) {
		ch->no_quit_timer = 3;
		victim->no_quit_timer = 3;
	}

	if (ch != victim && is_affected(ch, gsn_pacifism) && !IS_NPC(victim)) {
		affect_strip(ch, gsn_pacifism);
	}

	// Check for a legal target
	if (victim != ch) {
		if (is_safe(ch, victim)) {
			return TRUE;
		}

		check_killer(ch, victim);

		// You hurt them? They will return fire!
		if (victim->position > POS_STUNNED) {
			if (victim->fighting == NULL) {
				set_fighting(victim, ch);
			}
			
			if (victim->timer <= 4) {
				victim->position = POS_FIGHTING;

				if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_KILL)) {
					//mp_percent_trigger(victim, ch, NULL, NULL, TRIG_KILL);
				}
			}
		}

		// Start the battle
		if (victim->position > POS_STUNNED) {
			if (ch->fighting == NULL) {
				set_fighting(ch, victim);
			}
		}
	}

	// Strip spells that don't combine with fighting
	// Invis, Cloak of Mind, Hide, Charm, etc
	strip_noncombat_spells(ch, victim);

	// General damage reduction, save your ass.
	if (dam > 1 && !IS_NPC(victim) && victim->pcdata->condition[COND_DRUNK] > 10) {
		dam = dam * 5 / 6;
	}


	if (dt == gsn_poison && room_is_affected(victim->in_room, gsn_cloudkill)) {
		dam = dam * 5 / 4;
	}

	if (IS_SET(special, TYPE_MAGIC) && ch->spell_damroll > 0) {
		dam += UMIN(10, ch->spell_damroll);
	}

    // Marid damage reduction
    if (victim->race == race_lookup("marid")) {
        hours = get_hours(victim);
        if (hours >= 320) {
            dam_reduction += 5;
        } else if (hours >= 220) {
            dam_reduction += 4;
        } else if (hours >= 140) {
            dam_reduction += 3;
        } else if (hours >= 75) {
            dam_reduction += 2;
        } else if (hours >= 25) {
            dam_reduction += 1;
        }
    }

	if (dam > 0 && victim->damage_reduce != 0) {
		dam_reduction += victim->damage_reduce;
	}

	// Damage reduction stats cap at 10.
	if(dam > 0) {
		dam -= UMIN(10, dam_reduction);
	}

	if (dam > 1 && IS_AFFECTED(victim, AFF_SANCTUARY)) {
		dam /= 2;
	}

	if (dam > 1 && (
	        ( is_affected(victim, gsn_protection_evil) && IS_EVIL(ch) ) ||
	        ( is_affected(victim, gsn_protection_good) && IS_GOOD(ch) ) ||
	        ( is_affected(victim, gsn_protection_neutral) && IS_NEUTRAL(ch) )
	      )) {
		dam -= dam / 5;
	}

	if (dam > 1 && !is_clan(victim) && is_affected(victim, gsn_pacifism)) {
		dam -= dam / 4;
	}

	if (IS_SET(special, TYPE_SKILL) || IS_SET(special, TYPE_HIT)) {
		if ((tattoos = power_tattoo_count(victim, TATTOO_PHYSICAL_RESIST)) > 0) {
			dam = (dam * (100 - (10 + (5 * (tattoos - 1)))) / 100.0);
		}
	}

	if (IS_SET(special, TYPE_MAGIC)) {
		if((tattoos = power_tattoo_count(victim, TATTOO_MAGICAL_RESIST)) > 0) {
			dam = (dam * (100 - (10 + (5 * (tattoos - 1)))) / 100.0);
		}

		if((paf = affect_find(victim->affected, gsn_trance)) != NULL) {
			dam = (dam * (100 - (-5 * paf->modifier)) / 100.0);
		}
	}

	immune = FALSE;

	// Weapons will check vulns in weapon_hit.
	switch (check_immune(victim, dam_type)) {
		case (IS_IMMUNE):
			immune = TRUE;
			dam = 0;
			break;
			
		case (IS_RESISTANT):
			if (IS_SET(special, TYPE_MAGIC) || IS_SET(special, TYPE_SKILL)) {
				dam = dam * 2 / 3;
			}
			break;
			
		case (IS_VULNERABLE):
			if (IS_SET(special, TYPE_MAGIC) || IS_SET(special, TYPE_SKILL)) {
				dam = dam * 4 / 3;
			}
	}

	if ( is_affected(victim, gsn_ignore_wounds) &&
	     victim != ch &&
	     victim->fighting != ch &&
	     dam > 0 &&
	     number_percent() < get_skill(victim, gsn_ignore_wounds) / 2) {
		Cprintf(victim, "You ignore the wounds inflicted by %s.\n\r", IS_NPC(ch) ? ch->short_descr : ch->name);
		Cprintf(ch, "Your wounds don't seem to effect %s!\n\r", victim->name);
		act("$n ignores the wounds inflicted by $N.", victim, NULL, ch, TO_NOTVICT, POS_RESTING);
		return FALSE;
	}

	if (show) {
		dam_message(ch, victim, dam, dt, immune, special);
	}

	// Show damage on wiznet to immortals always
	if ( !show &&
	    IS_SET(ch->wiznet, WIZ_DAMAGE) &&
	    dam > 0 &&
	    ch != victim) {
		Cprintf(ch, "{r(Dealt %d damage){x\n\r", dam);
	}
	
	if ( !show &&
	    IS_SET(victim->wiznet, WIZ_DAMAGE) &&
	    dam > 0) {
		Cprintf(ch, "{r(Received %d damage){x\n\r", dam);
	}

	if (dam <= 0) {
		return FALSE;
	}

	// Bring the pain!
	victim->hit -= dam;

	// But not for immortals.
	if (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1) {
		victim->hit = 1;
	}

	// Store up damage taken for a counter attack when using chi ei
	paf = affect_find(victim->affected, gsn_chi_ei);
        if (paf != NULL) {
		paf->modifier += (dam / 3);
		if(paf->modifier > victim->level * 5)
			paf->modifier = victim->level * 5;
        }


	// Marid elemental essence, add some effects
	// Gets VERY nasty with weapon flags but more so with spells
	if( (chance = get_skill(ch, gsn_essence)) > 0 &&
	     IS_SET(special, TYPE_MAGIC) &&
	     dam > 0 &&
	     ch != victim &&
	     victim->hit > 0 &&
	     number_percent() < chance) {
		check_improve(ch, gsn_essence, 2, TRUE);
		if (dam_type == DAM_ACID) {
			acid_effect(victim, ch->level, dam * 2, TARGET_CHAR);
		}
		
		if (dam_type == DAM_FIRE) {
			fire_effect(victim, ch->level, dam * 2, TARGET_CHAR);
		}
		
		if (dam_type == DAM_COLD) {
			cold_effect(victim, ch->level, dam * 2, TARGET_CHAR);
		}
		
		if (dam_type == DAM_LIGHTNING) {
			shock_effect(victim, ch->level, dam * 2, TARGET_CHAR);
		}
		
		if (dam_type == DAM_DROWNING) {
			water_effect(victim, ch->level, dam * 2, TARGET_CHAR);
		}
		
		if (dam_type == DAM_POISON) {
			poison_effect(victim, ch->level, dam * 2, TARGET_CHAR);
		}
	}

	// Spawn some happy severed body parts
	if ( victim->race == race_lookup("troll") &&
	     get_skill(victim, gsn_regrowth) > number_percent() &&
	     !in_own_hall(victim) &&
	     dam > 100) {
		troll_regrowth(victim);
	}

	if ( is_affected(victim, gsn_quill_armor) &&
	     ch != victim &&
	     number_percent() < 25 &&
	     (IS_SET(special, TYPE_HIT) || IS_SET(special, TYPE_SKILL))) {
		Cprintf(ch, "You are needled by quills as you strike!\n\r");
		Cprintf(victim, "Your attacker gets impaled by your quills!\n\r");
		// If you get hit for 30 dam, does 5d10+7(34)
		dam = dice(ch->level / 10, ch->level / 5) + (dam / 4);
		damage(victim, ch, dam, gsn_quill_armor, DAM_PIERCE, TRUE, TYPE_SKILL);
	}

	// hang on.. withstand that death!
	if (victim->hit <= 0 && is_affected(victim, gsn_withstand_death)) {
		ROOM_INDEX_DATA *room = NULL;

		// stop the fight, fake the death, "heal", and lose the spell
		stop_fighting(victim, TRUE);
		act("$n is DEAD!!", victim, 0, 0, TO_ROOM, POS_RESTING);
		if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 66) {
			Cprintf(ch, "!!SOUND(sounds/wav/cry*.wav V=80 P=20 T=admin)");
		}
		Cprintf(ch, "You receive %d experience points.\n\r", number_range(0, 60));
		death_cry(victim);
		Cprintf(victim, "You tip your hat to the grim reaper and cheat death!\n\r");
		room = victim->in_room;
		char_from_room(victim);
		victim->position = POS_STANDING;
		victim->hit = 100;
		char_to_room(victim, room);
		affect_strip(victim, gsn_withstand_death);
		return TRUE;
	}

	update_pos(victim);
	switch (victim->position) {
		case POS_MORTAL:
			act("$n is mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(victim, "You are mortally wounded, and will die soon, if not aided.\n\r");
			break;
		case POS_INCAP:
			act("$n is incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(victim, "You are incapacitated and will slowly die, if not aided.\n\r");
			break;
		case POS_STUNNED:
			act("$n is stunned, but will probably recover.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			Cprintf(victim, "You are stunned, but will probably recover.\n\r");
			break;
		case POS_DEAD:
			// The death of especially linked mobiles is bad :P
			if (is_affected(victim, gsn_homonculus) && victim->master != NULL) {
				act("You double over in pain as $N dies!", victim->master, NULL, victim, TO_CHAR, POS_RESTING);
				act("$n doubles over in pain!", victim->master, NULL, NULL, TO_ROOM, POS_RESTING);
				victim->master->hit = (victim->master->hit - (2 * victim->level) < 1) ? 1 : (victim->master->hit - (2 * victim->level));
			}

			if (IS_NPC(victim) && is_affected(victim, gsn_duplicate)) {
				CHAR_DATA *aff_char;
				
				for (aff_char = char_list; aff_char != NULL; aff_char = aff_char->next) {
					if ( !IS_NPC(aff_char) &&
					     is_affected(aff_char, gsn_duplicate) &&
					     !str_cmp(aff_char->name, victim->short_descr)) {
						Cprintf(aff_char, "You feel weak in the knees as your duplicate expires.\n\r");
					}
				}
			}

			// If a CTF Jailer dies, free everyone who was in that jail.
			if ( IS_NPC(victim) &&
			     (victim->pIndexData->vnum == MOB_CTF_JAILER(CTF_RED) ||
			      victim->pIndexData->vnum == MOB_CTF_JAILER(CTF_BLUE))) {
				ROOM_INDEX_DATA *jail;
				CHAR_DATA *ctf;
				CHAR_DATA *ctf_next;

				if (victim->pIndexData->vnum == MOB_CTF_JAILER(CTF_BLUE)) {
					jail = get_room_index(ROOM_CTF_JAIL(CTF_BLUE));
					Cprintf(ch, "You free everyone in the blue jail!\n\r");
	
					for (fch = char_list; fch != NULL; fch = fch->next) {
						if (!IS_NPC(fch) && fch->pcdata->ctf_team > 0) {
							Cprintf(fch, "A sudden CRACK of chains breaking comes from {BBlue{x's jail.\n\r");
						}
					}

					for (ctf = jail->people; ctf != NULL; ctf = ctf_next) {
						ctf_next = ctf->next_in_room;
						end_jail((void*)ctf, TARGET_CHAR);
						affect_strip(ctf, gsn_jail);
					}
				}

				if (victim->pIndexData->vnum == MOB_CTF_JAILER(CTF_RED)) {
					jail = get_room_index(ROOM_CTF_JAIL(CTF_RED));
					Cprintf(ch, "You free everyone in the red jail!\n\r");
					
					for (fch = char_list; fch != NULL; fch = fch->next) {
						if (!IS_NPC(fch) && fch->pcdata->ctf_team > 0) {
							Cprintf(fch, "A sudden CRACK of chains breaking comes from {RRed{x's jail.\n\r");
						}
					}

					for (ctf = jail->people; ctf != NULL; ctf = ctf_next) {
						ctf_next = ctf->next_in_room;
						end_jail((void*)ctf, TARGET_CHAR);
						affect_strip(ctf, gsn_jail);
					}
				}
			}



			// A friendly duel ends in a restore
			if ( ch->in_room != NULL &&
			     (IS_SET(ch->in_room->room_flags, ROOM_ARENA) || IS_SET(ch->in_room->room_flags, ROOM_FERRY)) &&
			     !IS_NPC(victim)) {
				in_arena = TRUE;
				stop_fighting(victim, TRUE);
				stop_fighting(ch, TRUE);
			
				// No restores on the ferry
				if (!IS_SET(ch->in_room->room_flags, ROOM_FERRY)) {
					restore_char(victim);
				} else {
					victim->hit = 1;
				}

				if ( IS_SET(ch->in_room->room_flags, ROOM_ARENA) &&
				     !IS_NPC(victim) &&
				     victim->pcdata->ctf_team > 0) {
					OBJ_DATA *flag;
					AFFECT_DATA af;

					if (victim->pcdata->ctf_flag) {
						flag = create_object(get_obj_index(OBJ_CTF_FLAG(CTF_OTHER_TEAM(victim->pcdata->ctf_team))), 0);
						obj_to_room(flag, victim->in_room);
						victim->pcdata->ctf_flag = FALSE;
						act("$n drops the flag!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
						Cprintf(victim, "You drop the flag!\n\r");
					}

					af.where = TO_AFFECTS;
					af.type = gsn_jail;
					af.level = victim->level;
					af.location = APPLY_NONE;
					af.modifier = victim->pcdata->ctf_team;
					af.duration = 2;
					af.bitvector = 0;
					affect_to_char(victim, &af);

					char_from_room(victim);
					char_to_room(victim, get_room_index(ROOM_CTF_JAIL(CTF_OTHER_TEAM(victim->pcdata->ctf_team))));

					do_look(victim, "auto");
					Cprintf(victim, "You have lost this battle. You must wait in jail until you're freed.\n\r");
				} else if (ch->in_room->area->continent == 0) {
					char_from_room(victim);
					char_to_room(victim, get_room_index(clan_table[victim->clan].hall));
					Cprintf(victim, "You have lost this battle. You wander home in shame.\n\r");
				} else {
					char_from_room(victim);
					char_to_room(victim, get_room_index(clan_table[victim->clan].hall_dominia));
					Cprintf(victim, "You have lost this battle. You wander home in shame.\n\r");
				}
			
				Cprintf(ch, "You have won this battle! May the Gods be with you for the next one.\n\r");
				victim->position = POS_STANDING;
				sprintf(log_buf, "%s defeated by %s at %s", victim->name, (IS_NPC(ch) ? ch->short_descr : ch->name), ch->in_room->name);
				wiznet(log_buf, NULL, NULL, WIZ_DEATHS, 0, get_trust(ch));

				sprintf(log_buf, "%s eliminated by %s!\n\r", IS_NPC(victim) ? victim->short_descr : victim->name, IS_NPC(ch) ? ch->short_descr : ch->name);

				// Show to competitors
				for (fch = char_list; fch != NULL; fch = fch->next) {
					if (IS_SET(fch->in_room->room_flags, ROOM_ARENA)) {
						Cprintf(fch, "%s", log_buf);
					}
				}
				
				die_follower(victim);
				return TRUE;
			}

			// Dead, very dead.
			act("$n is DEAD!!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 66) {
				Cprintf(ch, "!!SOUND(sounds/wav/cry*.wav V=80 P=20 T=admin)");
			}

			Cprintf(victim, "You have been KILLED!!\n\r\n\r");
			if (IS_SET(victim->toggles, TOGGLES_SOUND)) {
				Cprintf(victim, "!!SOUND(sounds/wav/death*.wav V=80 P=20 T=admin)");
			}

			victim->pktimer = 3;
			die_follower(victim);
			for (fch = char_list; fch != NULL; fch = fch->next)	{
				if (fch->hunting == victim && fch != victim && IS_NPC(fch)) {
					fch->hunting = NULL;
				}
			}
		
			break;
		default:
			if (dam > MAX_HP(victim) / 8) {
				Cprintf(victim, "That really did HURT!\n\r");
			}
			
			if (victim->hit < MAX_HP(victim) / 4) {
				Cprintf(victim, "You sure are BLEEDING!\n\r");
			}
			
			break;
	}

	// Handle xp awards, autosac, autotrash and corpse creating
	if (handle_death(ch, victim, special) == TRUE) {
		return TRUE;
	}

	// Sleep spells and extremely wounded folks.
	if (!IS_AWAKE(victim)) {
		stop_fighting(victim, FALSE);
	}

	if (victim == ch) {
		return FALSE;
	}

	// Take care of link dead people.
	if (!(IS_SET(victim->act, PLR_NORECALL))) {
		if (!IS_NPC(victim) && victim->desc == NULL) {
			if (number_range(0, victim->wait) == 0) {
				if (!IS_SET(ch->act, PLR_NORECALL) && victim != ch) {
					do_recall(victim, "");
					if (!victim->fighting) {
						return TRUE;
					}
				}
			}
		}
	}

	// Wimp out?
	if (IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2) {
		if ( IS_SET(victim->act, ACT_WIMPY) &&
		     number_bits(2) == 0 &&
		     victim->hit < MAX_HP(victim) / 5 &&
		     victim != ch) {
			do_flee(victim, "");

			// If they fled, stop combat
			if (!victim->fighting) {
				return TRUE;
			}
		} else if ( IS_AFFECTED(victim, AFF_CHARM) &&
		            victim->master != NULL &&
		            victim->master->in_room != victim->in_room &&
		            victim != ch) {
			do_flee(victim, "");
			if (!victim->fighting) {
				return TRUE;
			}
		}
	}

	if ( !IS_NPC(victim) &&
	     victim->hit > 0 &&
	     victim->hit <= victim->wimpy &&
	     victim->wait < PULSE_VIOLENCE / 2 &&
	     victim != ch) {
		do_flee(victim, "");
		if (!victim->fighting) {
			return TRUE;
		}
	}

	tail_chain();
	return FALSE;
}

void
compute_clan_report(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int ch_clan = ch->clan - MIN_PKILL_CLAN;
	int victim_clan = victim->clan - MIN_PKILL_CLAN;

	if(ch == victim)
		return;

	if (clan_table[ch->clan].independent || clan_table[victim->clan].independent || ch->clan == victim->clan)
		return;

	clan_report[ch_clan].clan_pkills++;
	clan_report[victim_clan].clan_pkilled++;

	if(ch->pkills > clan_report[ch_clan].player_pkills)
	{
		sprintf(clan_report[ch_clan].best_pkill, "%s", ch->name);
		clan_report[ch_clan].player_pkills = ch->pkills;
	}

	if(ch->deaths > clan_report[ch_clan].player_pkilled)
        {
                sprintf(clan_report[ch_clan].worst_pkilled, "%s", ch->name);
                clan_report[ch_clan].player_pkilled = ch->deaths;
        }

	if(victim->pkills > clan_report[victim_clan].player_pkills)
        {
                sprintf(clan_report[victim_clan].best_pkill, "%s", victim->name);
                clan_report[victim_clan].player_pkills = victim->pkills;
        }

	if(victim->deaths > clan_report[victim_clan].player_pkilled)
	{
		sprintf(clan_report[victim_clan].worst_pkilled, "%s", victim->name);
		clan_report[victim_clan].player_pkilled = victim->deaths;
	}

	save_clan_report();
}

/* This function returns TRUE if the object is a quest
   or crafted item, and false otherwise. It won't mention
   if it has an owner, just that it would be allowed.
*/
int
obj_owner_allowed(OBJ_DATA *obj) {
	if(calculate_offering_tax_qp(obj) > 0)
		return TRUE;
	if(calculate_offering_tax_gold(obj) > 0)
		return TRUE;
	return FALSE;
}

/* This function takes all respawning items and places them into
   a temporary storage, so that they can be returned to the character
   after they die.
*/
void
gather_all_respawning_items(CHAR_DATA *victim, OBJ_DATA *storage) {
    OBJ_DATA *obj, *obj_next;
    OBJ_DATA *obj_in, *obj_in_next;
    int obj_respawned;

    for (obj = victim->carrying; obj != NULL; obj = obj_next) {
        obj_next = obj->next_content;

	if (obj == storage)
	    continue;

        obj_respawned = FALSE;
        // Grab all tattoos (these are worn)
        if (obj_is_affected(obj, gsn_paint_lesser)
                || obj_is_affected(obj, gsn_paint_greater)
                || obj_is_affected(obj, gsn_paint_power)) {
            unequip_char(victim, obj);
            obj_from_char(obj);
            obj_to_obj(obj, storage);
            continue;
        }

        // Check 1st level of containers for soul runes and ownership
        for (obj_in = obj->contains; obj_in != NULL; obj_in = obj_in_next) {
            obj_in_next = obj_in->next_content;

            if (obj_is_affected(obj_in, gsn_soul_rune)) {
                Cprintf(victim, "%s accompanies you to the next life.\n\r", obj_in->short_descr);
                obj_from_obj(obj_in);
                obj_to_obj(obj_in, storage);
                continue;
            } else if(obj_owner_allowed(obj_in) && obj_in->respawn_owner == NULL) {
                // Unclaimed valid items respawn always
                obj_from_obj(obj_in);
                obj_to_obj(obj_in, storage);
                continue;
            } else if(obj_owner_allowed(obj_in) && !str_cmp(obj_in->respawn_owner, victim->name)) {
                // Claimed valid items only respawn with owner
                obj_from_obj(obj_in);
                obj_to_obj(obj_in, storage);
                continue;
            } else if(!obj_owner_allowed(obj_in) && obj_in->item_type != ITEM_TRASH && obj_in->item_type != ITEM_POTION && obj_in->item_type != ITEM_PILL) {
                // Only the following can be looted: claimed items claimed to someone else, anything that's trash, potions or pills
                obj_from_obj(obj_in);
                obj_to_obj(obj_in, storage);
                continue;
            }
        }

        // Check inventory for soul runes ownership.
        if (obj_is_affected(obj, gsn_soul_rune)) {
            Cprintf(victim, "%s accompanies you to the next life.\n\r", obj->short_descr);
            obj_from_char(obj);
            obj_to_obj(obj, storage);
            obj_respawned = TRUE;
        } else if (obj_owner_allowed(obj) && obj->respawn_owner == NULL) {
            // Unclaimed valid items respawn always
            obj_from_char(obj);
            obj_to_obj(obj, storage);
            obj_respawned = TRUE;
        } else if (obj_owner_allowed(obj) && !str_cmp(obj->respawn_owner, victim->name)) {
            // Claimed valid items only respawn with owner
            obj_from_char(obj);
            obj_to_obj(obj, storage);
            obj_respawned = TRUE;
        } else if(!obj_owner_allowed(obj) && obj->item_type != ITEM_TRASH && obj->item_type != ITEM_POTION && obj->item_type != ITEM_PILL) {
            // Only the following can be looted: claimed items claimed to someone else, anything that's trash, potions or pills
            obj_from_char(obj);
            obj_to_obj(obj, storage);
            obj_respawned = TRUE;
        }

        if (obj->contains && obj_respawned) {
            // Empty claimed containers
            for(obj_in = obj->contains; obj_in != NULL; obj_in = obj_in_next) {
                obj_in_next = obj_in->next_content;

                obj_from_obj(obj_in);
                obj_to_char(obj_in, victim);
            }

            // start over
            obj = victim->carrying;
        }
    }
}


/* Handles player and mob deathes
 * Bounties
 * Updating logs and global messages
 * MP Trigger for death
 * Calls rawkill to create a corpse
 * Calls handle_sacrifice to deal with corpse
 *
 * For anonymous death and so on.
 */
int
handle_death(CHAR_DATA *ch, CHAR_DATA *victim, int special) {
    CHAR_DATA *vch;
    bool anonymous=FALSE;
    char buf[255];
    OBJ_DATA *ferrethole = NULL, *obj = NULL;
    OBJ_DATA *obj_next = NULL;
    ROOM_INDEX_DATA *dead_room;
    int tattoos = 0, death_rune = FALSE, i = 0;
    int pktimer = 0, wear_loc = -1;
    CHAR_DATA *rch;

    if(IS_SET(special, TYPE_ANONYMOUS)) {
        anonymous = TRUE;
    }

    // Still kickin!
    if (victim->position != POS_DEAD) {
        return FALSE;
    }

    group_gain(ch, victim);

    if (ch->clan == 0
            && is_affected(ch, gsn_injustice)) {
        Cprintf(ch, "Carnas blesses you with %d gold and %d silver.\n\r", victim->level / 6, victim->level * 8);
        ch->gold += victim->level / 6;
        ch->silver += victim->level * 8;
    }

    // Cool runist tattoos
    if ((tattoos = power_tattoo_count(ch, TATTOO_HP_PER_KILL)) > 0) {
        Cprintf(ch, "Your tattoos glow red, absorbing life energy.\n\r");

        if (!is_affected(ch, gsn_dissolution)) {
            ch->hit = UMIN(MAX_HP(ch), ch->hit + victim->level + (victim->level / 2 * (tattoos -1)));
        }
    }

    if ((tattoos = power_tattoo_count(ch, TATTOO_MANA_PER_KILL)) > 0) {
        Cprintf(ch, "Your tattoos glow blue, absorbing magical energy.\n\r");

        if(!is_affected(ch, gsn_dissolution)) {
            ch->mana = UMIN(MAX_MANA(ch), ch->mana + victim->level + (victim->level / 2 * (tattoos -1)));
        }
    }

    // Handle player death
    if (!IS_NPC(victim) && !anonymous) {
        log_string("%s killed by %s at %d", victim->name, (IS_NPC(ch) ? ch->short_descr : ch->name), ch->in_room->vnum);

        if (!IS_NPC(ch) && !IS_NPC(victim) && ch != victim) {
            int reward = number_range(1, 4 * ch->level);
            ch->pkills++;
            victim->deaths++;
            Cprintf(ch, "The authorities increase the bounty on your head by %d.\n\r", reward);
            ch->bounty += reward;
            compute_clan_report(ch, victim);
        }

        if (victim->bounty > 0
                && victim->clan != ch->clan
                && !IS_NPC(ch)) {
            Cprintf(ch, "You collect a bounty of %d gold for %s's head!\n\r", victim->bounty, victim->name);
            Cprintf(victim, "You no longer have a price on your head.\n\r");

            if (ch->reclass == reclass_lookup("templar")) {
                victim->bounty = victim->bounty * 9 / 10;
                Cprintf(ch, "You tithe some of the gold.\n\r");
            }

            ch->gold = ch->gold + victim->bounty;
            victim->bounty = 0;
        }

        // Dying penalty: 2/3 way back to previous level.
        if (victim->exp > exp_per_level(victim, victim->pcdata->points) * victim->level) {
            gain_exp(victim, ((exp_per_level(victim, victim->pcdata->points) * victim->level - victim->exp) / 4));
        }
    }

    if (!IS_NPC(victim) && anonymous) {
        log_string("%s killed anonymously at %d", victim->name, victim->in_room->vnum);

        // Dying penalty: 2/3 way back to previous level.
        if (victim->exp > exp_per_level(victim, victim->pcdata->points) * victim->level) {
            gain_exp(victim, ((exp_per_level(victim, victim->pcdata->points) * victim->level - victim->exp) / 4));
        }
    }

    // Log the pitiful death
    if (!anonymous) { 
        sprintf(log_buf, "%s suffers defeat at the hands of %s at %s [room %d]",
                (IS_NPC(victim) ? victim->short_descr : victim->name),
                (IS_NPC(ch) ? ch->short_descr : ch->name),
                ch->in_room->name, ch->in_room->vnum);
    } else {
        sprintf(log_buf, "%s was killed anonymously at %s [room %d]",
                (IS_NPC(victim) ? victim->short_descr : victim->name),
                ch->in_room->name, ch->in_room->vnum);
    }

    if (IS_NPC(victim)) {
        wiznet(log_buf, NULL, NULL, WIZ_MOBDEATHS, 0, 0);
    } else {
        wiznet(log_buf, NULL, NULL, WIZ_DEATHS, 0, get_trust(ch));
    }

    // Share the good news with the world.
    if(!anonymous) {
        sprintf(buf, "{r%s{x suffers {Ddefeat{x at the hands of {W%s{x.\n\r",
                (IS_NPC(victim) ? victim->short_descr : victim->name),
                (IS_NPC(ch) ? ch->short_descr : ch->name));
    } else {
        sprintf(buf, "{r%s{x was slain by powerful magic.\n\r",
                (IS_NPC(victim) ? victim->short_descr : victim->name));
    }

    // Only show kills made by players to all clanners
    if (!IS_NPC(ch) && !IS_NPC(victim) && is_clan(victim)) {
        for(vch = char_list;vch != NULL; vch = vch->next) {
            if (!IS_NPC(vch) && !IS_SET(vch->comm, COMM_NOCGOSS)) {
                Cprintf(vch, "%s", buf);
            }
        }
    }

    // Death trigger
    if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_DEATH))	{
        victim->position = POS_STANDING;
        //mp_percent_trigger(victim, ch, NULL, NULL, TRIG_DEATH);
    }

    // Eq that follows to the next life for players
    if (!IS_NPC(victim)) {
        // This is some real dumb shit, but needed.
        ferrethole = create_object(get_obj_index(OBJ_VNUM_DISC), 0);
        ferrethole->value[0] = 20000;
        ferrethole->value[3] = 20000;

        obj_to_char(ferrethole, victim);
        gather_all_respawning_items(victim, ferrethole);
        obj_from_char(ferrethole);
    }

    // Toggle this now since mobs don't exist after raw-kill
    dead_room = victim->in_room;
    if (is_affected(victim, gsn_death_rune)) {
        death_rune = TRUE;
    }

    // Special quest item drops
    handle_special_drops(victim);

    raw_kill(victim);

    // FOX presents: When PCs Explode
    if (death_rune && !IS_NPC(victim)) {
        pktimer = victim->pktimer;
        victim->pktimer = 0;
        Cprintf(ch, "A rune of death suddenly explodes!\n\r");
        act("A rune of death suddenly explodes!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

        for(i = 0; i < victim->level / 6; i++) {
            meteor_blast(victim, dead_room, victim->level);
        }

        victim->pktimer = pktimer;
        stop_fighting(victim, FALSE);

	raw_kill(victim);
        for (rch = dead_room->people; rch != NULL; rch = rch->next_in_room) {
            if (rch->fighting == victim) {
                stop_fighting(rch, FALSE);
            }
        }
    }

    // Mobs with death rune operate more simple
    if (death_rune && IS_NPC(victim)) {
        Cprintf(ch, "A rune of death suddenly explodes!\n\r");
        act("A rune of death suddenly explodes!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

        for (i = 0; i < ch->level / 8; i++) {
            meteor_blast(ch, dead_room, ch->level);
        }
    }

    handle_sacrifice(ch);

    if (!IS_NPC(victim)) {
        obj_to_char(ferrethole, victim);

        for (obj = ferrethole->contains; obj != NULL; obj = obj_next) {
            obj_next = obj->next_content;
            obj_from_obj(obj);
            obj_to_char(obj, victim);

            if(obj_is_affected(obj, gsn_paint_lesser)
                    || obj_is_affected(obj, gsn_paint_greater)
                    || obj_is_affected(obj, gsn_paint_power)) {
                if (IS_SET(obj->wear_flags, ITEM_WEAR_HEAD)) {
                    if(get_eq_char(victim, WEAR_HEAD) != NULL) {
                        wear_loc = WEAR_HEAD_2;
                    } else {
                        wear_loc = WEAR_HEAD;
                    }
                }

                if (IS_SET(obj->wear_flags, ITEM_WEAR_BODY)) {
                    wear_loc = WEAR_BODY;
                }

                if (IS_SET(obj->wear_flags, ITEM_WEAR_ARMS)) {
                    wear_loc = WEAR_ARMS;
                }

                if (IS_SET(obj->wear_flags, ITEM_WEAR_WRIST)) {
                    if (get_eq_char(victim, WEAR_WRIST_L) == NULL) {
                        wear_loc = WEAR_WRIST_L;
                    } else {
                        wear_loc = WEAR_WRIST_R;
                    }
                }

                equip_char(victim, obj, wear_loc);
            }
        }

        obj_from_char(ferrethole);
        extract_obj(ferrethole);
    }

    // dump the flags
    if (ch != victim && !IS_NPC(ch) && !is_same_clan(ch, victim)) {
        REMOVE_BIT(victim->act, PLR_KILLER);
        REMOVE_BIT(victim->act, PLR_THIEF);
    }

    save_char_obj(victim, FALSE);

    return TRUE;
}

/*
 * Handles the special rare drops from undead and demons which can
 * be used to unremort.
 */
void handle_special_drops(CHAR_DATA *victim) {
	int chance;
	OBJ_DATA *obj = NULL;

	if(!IS_NPC(victim)) {
		return;
	}

	if(victim->race == race_lookup("deamon")) {
		if(IS_SET(victim->toggles, TOGGLES_NOEXP)) {
			return;
		}
		chance = 1 + ((victim->level - 40) / 5);
		if(number_percent() <= chance) {
			switch(dice(1, 3)) {
			case 1:
				obj = create_object(get_obj_index(OBJ_VNUM_UNREMORT_1), 0);
				break;
			case 2:
				obj = create_object(get_obj_index(OBJ_VNUM_UNREMORT_2), 0);
				break;
			case 3:
				obj = create_object(get_obj_index(OBJ_VNUM_UNREMORT_3), 0);
				break;

			}
		}
	}

	if(victim->race == race_lookup("undead")) {
		if(IS_SET(victim->toggles, TOGGLES_NOEXP)) {
			return;
		}
		chance = 1 + ((victim->level - 40) / 5);
		if(number_percent() <= chance) {
			switch(dice(1, 3)) {
			case 1:
				obj = create_object(get_obj_index(OBJ_VNUM_UNRECLASS_1), 0);
				break;
			case 2:
				obj = create_object(get_obj_index(OBJ_VNUM_UNRECLASS_2), 0);
				break;
			case 3:
				obj = create_object(get_obj_index(OBJ_VNUM_UNRECLASS_3), 0);
			break;
			}
		}
	}

	if(obj != NULL) {
		obj_to_char(obj, victim);
	}
}

void
handle_sacrifice(CHAR_DATA *ch)
{
	OBJ_DATA *corpse;

	// Dumped a mob corpse in front of you.
	if (ch->in_room != NULL
	&& (corpse = get_obj_list(ch, "corpse", ch->in_room->contents)) != NULL
	&& corpse->item_type == ITEM_CORPSE_NPC
	&& can_see_obj(ch, corpse))	{
		OBJ_DATA *coins;

		// Normal autoloot
		if (!IS_NPC(ch)
		&& IS_SET(ch->act, PLR_AUTOLOOT)
		&& corpse && corpse->contains)
			do_get(ch, "all corpse");

		// Autoloot charmie kills too
		if (IS_AFFECTED(ch, AFF_CHARM) && ch->leader
		&& corpse
		&& corpse->contains
		&& !IS_NPC(ch->leader)
		&& IS_SET(ch->leader->act, PLR_AUTOLOOT))
			do_get(ch->leader, "all corpse");

		// Normal autogold
		if (!IS_NPC(ch)
		&& IS_SET(ch->act, PLR_AUTOGOLD)
		&& corpse && corpse->contains &&
		!IS_SET(ch->act, PLR_AUTOLOOT))	{
			if ((coins = get_obj_list(ch, "gcash", corpse->contains)) != NULL) {

				do_get(ch, "all.gcash corpse");
			}
		}

		// Autogold charmie kills too
		if (IS_AFFECTED(ch, AFF_CHARM) && ch->leader
		&& corpse
		&& corpse->contains
		&& !IS_NPC(ch->leader)
		&& !IS_SET(ch->leader->act, PLR_AUTOLOOT)
		&& IS_SET(ch->leader->act, PLR_AUTOGOLD)) {
			if ((coins = get_obj_list(ch->leader, "gcash", corpse->contains)) != NULL)
				do_get(ch->leader, "all.gcash corpse");
		}

		// Normal autosac and autotrash
		if (!IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOSAC)) {
			if (IS_SET(ch->act, PLR_AUTOLOOT) && corpse && corpse->contains)
				return;
		 	// Don't sac corpses with stuff in em
			else
				do_sacrifice(ch, "corpse");
		}
		else if (!IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOTRASH)) {
			if (IS_SET(ch->act, PLR_AUTOLOOT) && corpse && corpse->contains)
				return;
			// Don't trash corpses with stuff in em
			else
				do_trash(ch, "corpse");
		}

		// Autosac and Autotrash when your charmie kills stuff too.
		if (IS_AFFECTED(ch, AFF_CHARM)
		&& ch->leader
		&& !IS_NPC(ch->leader)
		&& IS_SET(ch->leader->act, PLR_AUTOSAC)) {
			if (IS_SET(ch->leader->act, PLR_AUTOLOOT) && corpse && corpse->contains)
				return;
			// Don't sac corpses with stuff in em
			else
				do_sacrifice(ch->leader, "corpse");
		}
		else if (IS_AFFECTED(ch, AFF_CHARM)
		&& ch->leader
		&& !IS_NPC(ch->leader)
		&& IS_SET(ch->leader->act, PLR_AUTOTRASH)) {
			if (IS_SET(ch->leader->act, PLR_AUTOLOOT) && corpse && corpse->contains)
				return;
				// Don't trash corpses with stuff in em
			else
				do_trash(ch->leader, "corpse");
		}
	}
	return;
}

void
check_wild_swing(CHAR_DATA *ch, CHAR_DATA *victim,
					OBJ_DATA *wield, int dt)
{
	CHAR_DATA *vch;
	bool char_hit = FALSE;
	int chance;

	chance = get_skill(ch, gsn_wild_swing);
	if (IS_AFFECTED(ch, AFF_SLOW))
		chance = 0;

	if (number_percent() < chance)
	{
		int people = 0;
		for(vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
			if(vch->fighting == ch)
				people++;
		}

		if(people > 1) {
			Cprintf(ch, "Your attacks dissolve into a blur of motion!\n\r");
			for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
			{
				if (vch->fighting == ch) {
					weapon_hit(ch, vch, wield, dt);
					Cprintf(vch, "%s takes a wild swing at you!\n\r", ch->name);
					char_hit = TRUE;
				}
			}
		}
		if (char_hit != 0)
			check_improve(ch, gsn_wild_swing, TRUE, 1);
	}

	return;
}

int
check_dragon_tail(CHAR_DATA *ch, CHAR_DATA *victim,
					OBJ_DATA *wield, int dt)
{
	int victim_dead = FALSE;
	int chance, dam_min, dam_max;
	int dam = 0;

	chance = get_skill(ch, gsn_tail_attack) / 4;

	if (IS_AFFECTED(ch, AFF_SLOW))
		chance = 0;

	if (number_percent() < chance)
	{
		dam_min = get_hand_to_hand_damage(ch, HAND_TO_HAND_MINIMUM);
		dam_max = get_hand_to_hand_damage(ch, HAND_TO_HAND_MAXIMUM);
		dam_min += ch->level * 3 / 4 ;
		dam_max += ch->level * 3 / 4;

		Cprintf(ch, "Your tail lashes out furiously!\n\r");
		if(number_percent() < 5 + get_curr_stat(victim, STAT_DEX)) {
			dam = 0;
		}
		else {
			dam = number_range(dam_min, dam_max);
		}
		victim_dead = damage(ch, victim, dam,
				gsn_tail_attack, DAM_PIERCE, TRUE, TYPE_SKILL);
		if(victim_dead)
			return TRUE;

		// Apply hand to hand flags to tail attack.
		if(dam > 0)
			victim_dead = handle_weapon_flags(ch, victim, NULL, gsn_hand_to_hand);

		check_improve(ch, gsn_tail_attack, TRUE, 6);
	}

	return victim_dead;
}

void
check_kirre_tail_trip(CHAR_DATA *ch, CHAR_DATA *victim) {
	int chance = 0;

	chance = get_skill(ch, gsn_tail_trip) / 5;

 	if (IS_AFFECTED(ch, AFF_SLOW))
                chance = 0;

	if(chance < 1)
		return;

	if(number_percent() < chance) {
		act("$n deftly trips $N with their tail.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		Cprintf(ch, "You use your tail to sweep %s's feet out from under them.\n\r", PERS(victim, ch));
		Cprintf(ch, "%s falls to the ground in a heap!\n\r", PERS(victim, ch));
		Cprintf(victim, "%s's tail sweeps your feet out from under you!\n\r", ch->name);

		DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
		WAIT_STATE(victim, dice(2, 8));

		check_improve(ch, gsn_tail_trip, TRUE, 2);
		return;
	}

	return;
}

int
check_sliver_thrust(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, int dt)
{
	int victim_dead = FALSE;
	int dam, chance;

	if (ch->race == race_lookup("sliver"))
	{
		chance = 10 + ch->level / 3;
		if (number_percent() < chance)
		{
			// Remort slivers bite nicely
			Cprintf(ch, "Your mandibles latch onto your victim!\n\r");
			if(!ch->remort) {
				if(check_parry(ch, victim))
                       			return FALSE;
                		if(check_dodge(ch, victim))
				return FALSE;
			if(check_shield_block(ch, victim))
				return FALSE;
                		if(check_tumbling(ch, victim))
				return FALSE;
			}
			dam = dice(ch->level / 6, 6) + (ch->level / 3) + 1;
			victim_dead = damage(ch, victim, dam, hit_lookup("pobite"),
							DAM_POISON, TRUE, TYPE_HIT);
		}
	}

	return victim_dead;
}

int
check_beheading(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, int dt)
{
	int victim_dead = FALSE;
	int chance;

	chance = get_skill(ch, gsn_beheading) / 4;
	if (get_eq_char(ch, WEAR_WIELD) == NULL)
		chance = 0;

	if (number_percent() < chance)
	{
		if (number_percent() > 75)
		{
			Cprintf(ch, "You {Rviolently{x slash your foes throat!!\n\r");
			dt = gsn_beheading;
		}
		else
		{
			Cprintf(ch, "You take a swing at your foes head!\n\r");
		}
		victim_dead = weapon_hit(ch, victim, wield, dt);
		if(ch->burstCounter)
        		ch->burstCounter--;
		check_improve(ch, gsn_beheading, TRUE, 4);
	}

	return victim_dead;
}

// The counter will count up as they parry, after 10 points,
// they will counter.
int
check_dragon_thrust(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int dam;
	int victim_dead=FALSE;

	if(IS_NPC(ch))
		return FALSE;

	if(ch->thrustCounter < 10)
		return FALSE;

	if (IS_AFFECTED(ch, AFF_SLOW))
		return FALSE;

	Cprintf(ch, "The swing of your parry lashes at your victim!\n\r");

	ch->thrustCounter = 0;
	dam = dice(ch->level / 8, 6) + (ch->level * 3 / 4);
    victim_dead = damage(ch, victim, dam, gsn_thrust, DAM_SLASH, TRUE, TYPE_SKILL);

    check_improve(ch, gsn_thrust, TRUE, 2);

	return victim_dead;
}

// The counter will count up as they dodge, after 10 points,
// they will counter.
int
check_dragon_bite(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int dam;
	int victim_dead=FALSE;

	if(IS_NPC(ch))
		return FALSE;

	if(ch->dbiteCounter < 10)
		return FALSE;

	if (IS_AFFECTED(ch, AFF_SLOW))
		return FALSE;

	Cprintf(ch, "You hiss and lash out, biting your opponent!\n\r");

	ch->dbiteCounter = 0;
	dam = dice(ch->level / 8, 6) + (ch->level * 3 / 4);
    victim_dead = damage(ch, victim, dam, gsn_dragon_bite, DAM_SLASH, TRUE, TYPE_SKILL);

    check_improve(ch, gsn_dragon_bite, TRUE, 2);

	return victim_dead;
}

int
check_shield_bash(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int dam;
	int victim_dead=FALSE;

	if(IS_NPC(ch))
		return FALSE;

	if(ch->sbashCounter < 10)
		return FALSE;

	if (IS_AFFECTED(ch, AFF_SLOW))
		return FALSE;

	if (get_eq_char(ch, WEAR_SHIELD) == NULL)
		return FALSE;

	act("You knock your opponent back with $p!!", ch, get_eq_char(ch, WEAR_SHIELD), victim, TO_CHAR, POS_RESTING);

	ch->sbashCounter = 0;
	victim->daze += 2 * PULSE_VIOLENCE;

	dam = dice(ch->level / 8, 4) + (ch->level / 2);
    	victim_dead = damage(ch, victim, dam, gsn_shield_bash, DAM_BASH, TRUE, TYPE_SKILL);

    	check_improve(ch, gsn_shield_bash, TRUE, 2);

	return victim_dead;
}

bool
is_safe(CHAR_DATA * ch, CHAR_DATA * victim)
{
	CHAR_DATA *rch;

	if (victim->in_room == NULL || ch->in_room == NULL)
		return TRUE;

	if (ch->in_room == get_room_index(ROOM_VNUM_LIMBO))
		return TRUE;

	if (ch->in_room == get_room_index(ROOM_VNUM_LIMBO_DOMINIA))
		return TRUE;

	if (IS_SET(victim->in_room->room_flags, ROOM_NO_KILL))
		return TRUE;

	if (!IS_NPC(ch) && is_affected(victim, gsn_pacifism)) {
		Cprintf(ch, "%s is protected by the spirit of Bhaal.\n\r", victim->name);
		return TRUE;
	}

	// Stops charmies from attacking people who are pacified
	if (IS_NPC(ch) && ch->master && is_affected(victim, gsn_pacifism)) {
		return TRUE;
	}

	// Stop from ordering charmies while pacified
	if (IS_NPC(ch)
	&& ch->master
	&& is_affected(ch->master, gsn_pacifism)
	&& !IS_NPC(victim))
		return TRUE;

	// Stop charmies from killing other charmies
	if (IS_NPC(ch) && ch->master
	&&  IS_NPC(victim) && victim->master
	&&  ch->master != victim->master)
		return TRUE;

	// New effect: pacifism and harmonic aura also stop you from attacking
	if(!IS_NPC(ch)
	&& !IS_NPC(victim)
	&& is_affected(ch, gsn_pacifism)) {
		Cprintf(ch, "The spirit of Bhaal prevents you from attacking.\n\r");
		return TRUE;
	}

	for(rch = victim->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if(!IS_NPC(ch)
		&& !IS_NPC(rch)
		&& !IS_NPC(victim)
		&& (rch != victim)
		&& (rch != ch)
		&& is_affected(rch, gsn_harmonic_aura)
		&& (is_same_group(rch, victim) || is_same_group(ch, rch))) {
			Cprintf(ch, "You are filled with a sense of harmony and fail to strike down %s.\n\r", victim->name);
			return TRUE;
		}
	}


	if (!IS_NPC(victim) && is_affected(victim, gsn_shapeshift) && (victim->level - ch->level > 8))
	{
		Cprintf(ch, "A high level shapeshifter is a tricky thing to kill. Better think again.\n\r");
		return TRUE;
	}

	if (ch->pktimer > 0)
	{
		Cprintf(ch, "Give yourself a chance to breathe before fighting again.\n\r");
		return TRUE;
	}

	if (victim->fighting == ch || victim == ch)
		return FALSE;

	if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL)
		return FALSE;

	// Non clanner imms are safe in tourney rooms
	if (IS_SET(ch->in_room->room_flags, ROOM_ARENA)
	&& !IS_NPC(victim)
	&& IS_IMMORTAL(victim)
	&& !is_clan(victim))
		return TRUE;

	// Everyone else is game in tourney rooms.
	if (IS_SET(ch->in_room->room_flags, ROOM_ARENA)
        && !IS_NPC(victim))
                return FALSE;

	if (victim->fighting != NULL)
	{
		if (IS_NPC(victim))
		{
		        if (victim->fighting->spec_fun == spec_lookup("spec_goon"))
				return FALSE;
		}
	}


	/* killing mobiles */
	if (IS_NPC(victim))
	{
		/* no fighting wizi mobs */
		if (IS_NPC(victim)
		&& IS_SET(victim->act, ACT_IS_WIZI))
		{
			return TRUE;
		}

		/* safe room? */
		if (IS_SET(victim->in_room->room_flags, ROOM_SAFE))
		{
			Cprintf(ch, "Not in this room.\n\r");
			return TRUE;
		}

		if (victim->pIndexData->pShop != NULL)
		{
			Cprintf(ch, "The shopkeeper wouldn't like that.\n\r");
			return TRUE;
		}

		/* no killing healers, trainers, etc */
		if (IS_SET(victim->act, ACT_TRAIN)
			|| IS_SET(victim->act, ACT_PRACTICE)
			|| IS_SET(victim->act, ACT_IS_HEALER)
			|| IS_SET(victim->act, ACT_DEALER)
			|| IS_SET(victim->act, ACT_IS_CHANGER)
			|| victim->spec_fun == spec_lookup("spec_portal_keeper"))
		{
			if (ch->in_room != NULL && ch->in_room->area->continent == 0)
				Cprintf(ch, "I don't think Bosco would approve.\n\r");
			else
				Cprintf(ch, "I don't think Selina would approve.\n\r");
			return TRUE;
		}

		if (!IS_NPC(ch))
		{
			/* no pets */
			if (IS_SET(victim->act, ACT_PET))
			{
				act("But $N looks so cute and cuddly...", ch, NULL, victim, TO_CHAR, POS_RESTING);
				return TRUE;
			}

			/* no charmed creatures unless owner */
			if (IS_AFFECTED(victim, AFF_CHARM) && ch != victim->master
				&& victim->master != NULL)
			{
				Cprintf(ch, "You don't own that monster.\n\r");
				return TRUE;
			}
		}
	}
	/* killing players */
	else
	{
		/* NPC doing the killing */
		if (IS_NPC(ch))
		{
			/* safe room check */
			if (IS_SET(victim->in_room->room_flags, ROOM_SAFE))
			{
				Cprintf(ch, "Not in this room.\n\r");
				return TRUE;
			}


			if (ch->master != NULL
				&& ch->master->fighting != victim
				&& !IS_NPC(victim))
			{
				if (!is_clan(victim) || !is_clan(ch->master) || !IS_RANGE(ch->master, victim))
				{
					Cprintf(ch, "Players are your friends!\n\r");
					return TRUE;
				}
			}
		}

		/* player doing the killing */
		else
		{
			if (!is_clan(ch))
			{
				Cprintf(ch, "Join a clan if you want to kill players.\n\r");
				return TRUE;
			}

			/* timer to avoid kill spam by players */
			if (victim->pktimer > 0)
			{
				Cprintf(ch, "Give that person a chance to recover would'ja?\n\r");
				return TRUE;
			}

			// Killer/Thief flag extends to 16 levels.
			if (IS_SET(victim->act, PLR_KILLER)
			|| IS_SET(victim->act, PLR_THIEF)) {

				if(ch->level > victim->level + 16) {
					Cprintf(ch, "Pick on someone your own size.\n\r");
					return TRUE;
				}
				else
					return FALSE;
			}

			if (!is_clan(victim))
			{
				Cprintf(ch, "They aren't in a clan, leave them alone.\n\r");
				return TRUE;
			}

			if (ch->pcdata->any_skill == TRUE
			&& ch->real_level > victim->level + 8)
			{
				Cprintf(ch, "Pick on someone your own size.\n\r");
				return TRUE;
			}

			if (ch->level > victim->level + 8)
			{
				Cprintf(ch, "Pick on someone your own size.\n\r");
				return TRUE;
			}
		}
	}
	return FALSE;
}

/*
 * See if an attack justifies a KILLER flag.
 */
void
check_killer(CHAR_DATA * ch, CHAR_DATA * victim)
{
	char buf[MAX_STRING_LENGTH];

	if (!is_clan(ch))
	{
		return;
	}

	/* check to see if injustice is present */
	if (is_affected(ch, gsn_injustice) ||
		is_affected(victim, gsn_injustice))
	{
		return;
	}

	/* check for pc twink attacking charmie */
	if (IS_AFFECTED(victim, AFF_CHARM)
		&& victim->master == ch
		&& ch != victim
		&& victim->master != NULL)
	{
		affect_strip(victim, gsn_charm_person);
		affect_strip(victim, gsn_tame_animal);
	}

	/*
	 * Follow charm thread to responsible character.
	 * Attacking someone's charmed char is hostile!
	 */

	while (IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL)
		victim = victim->master;

	if (is_affected(ch, gsn_injustice) ||
		is_affected(victim, gsn_injustice))
		return;
//  Commented to try and get killers with throw
	/*  Del's changes to avoid charmie attack leading to killer flag following death of victim */
//	if (victim->in_room != ch->in_room)
//		return;

/* lets check somethign obvius */

	if (ch == victim)
		return;

	/*
	 * NPC's are fair game.
	 * So are killers and thieves.
	 */
	if (IS_NPC(victim)
		|| IS_SET(victim->act, PLR_KILLER)
		|| IS_SET(victim->act, PLR_THIEF)
		|| IS_SET(ch->in_room->room_flags, ROOM_ARENA))
		return;

	/*
	 * Charm-o-rama.
	 */
	if (IS_SET(ch->affected_by, AFF_CHARM))
	{
		if (ch->master == NULL)
		{
			char buf[MAX_STRING_LENGTH];

			sprintf(buf, "Check_killer: %s bad AFF_CHARM",
					IS_NPC(ch) ? ch->short_descr : ch->name);
			bug(buf, 0);
			affect_strip(ch, gsn_charm_person);
			affect_strip(ch, gsn_tame_animal);
			REMOVE_BIT(ch->affected_by, AFF_CHARM);
			return;
		}
		if (ch->master == victim)
			return;

		if (is_affected(ch->master, gsn_injustice))
			return;

		if (!IS_SET(ch->master->act, PLR_KILLER) && ch->master->in_room == victim->in_room)
		{
			Cprintf(ch->master, "*** You are now a KILLER!! ***\n\r");
			SET_BIT(ch->master->act, PLR_KILLER);
		}

		return;
	}

	/*
	 * NPC's are cool of course (as long as not charmed).
	 * Hitting yourself is cool too (bleeding).
	 * So is being immortal (Alander's idea).
	 * And current killers stay as they are.
	 */
	if (IS_NPC(ch)
		|| ch == victim
		|| ch->level >= LEVEL_IMMORTAL
		|| !is_clan(ch)
		|| IS_SET(ch->act, PLR_KILLER))
		return;

	if (victim->master == NULL && IS_AFFECTED(victim, AFF_CHARM) && IS_NPC(victim))
		return;

	if (victim->fighting != NULL)
	{
		if (ch->clan == victim->fighting->clan 
		&& victim->fighting->spec_fun == spec_lookup("spec_goon"))
			return;
	}


	Cprintf(ch, "*** You are now a KILLER!! ***\n\r");
	SET_BIT(ch->act, PLR_KILLER);
	
	sprintf(buf, "$N is attempting to murder %s", victim->name);
	wiznet(buf, ch, NULL, WIZ_FLAGS, 0, get_trust(ch));
	log_string("%s becomes a grievous KILLER.", ch->name);
	save_char_obj(ch, FALSE);
	return;
}

bool
check_parry(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int chance;
	int spec_chance=0;
	OBJ_DATA *victweapon, *chweapon, *wield;
	AFFECT_DATA *paf = NULL;

	// Make sure combat, flags, prismatic are set properly.
	if (ch->in_room == victim->in_room)
		damage(ch, victim, 0, gsn_parry, DAM_NONE, FALSE, TYPE_SKILL);

    	if (!IS_AWAKE(victim))
		return FALSE;

	if ((wield = get_eq_char(victim, WEAR_WIELD)) != NULL) {
       		if (victim->charClass == class_lookup("runist")
		&& (paf = affect_find(wield->affected, gsn_blade_rune)) != NULL) 		{
                	if(paf->location == APPLY_NONE
                	&& paf->modifier == BLADE_RUNE_PARRYING
			&& number_percent() < 15) {
				Cprintf(victim, "Your weapon flashes out and a blade rune deflects the attack.\n\r");
				act("$N's weapon flashes out and a rune stops your attack.", ch, NULL, victim, TO_CHAR, POS_RESTING);
                        	return TRUE;
                	}
        	}
	}

	// Monks get an extra parry with their combat style
	if(is_affected(victim, gsn_stance_mantis)
	&& number_percent() < get_skill(victim, gsn_stance_mantis) / 3) {
		Cprintf(victim, "You guard against the attack.\n\r");
		act("$N guards against your attack.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return TRUE;
	}

	if (get_skill(victim, gsn_parry) < 1)
		return FALSE;

	// Base: 40% chance to parry
	chance = UMIN(get_skill(victim, gsn_parry), 100) * 2 / 5;

	if(!IS_NPC(victim) && victim->pcdata->specialty == gsn_parry)
                spec_chance = (victim->pcdata->learned[gsn_parry] - 100) / 2;

	// Monks always get full parry open handed.
	if(get_eq_char(victim, WEAR_WIELD) == NULL
	&& get_eq_char(victim, WEAR_DUAL)  == NULL
        && victim->charClass == class_lookup("monk"))
		chance = chance;
	// If you have martial arts or are of a natural weapon race
	else if(get_eq_char(victim, WEAR_WIELD) == NULL
	&& get_eq_char(victim, WEAR_DUAL) == NULL
	&& (victim->race == race_lookup("sliver")
	|| victim->race == race_lookup("black dragon")
	|| victim->race == race_lookup("blue dragon")
    	|| victim->race == race_lookup("green dragon")
    	|| victim->race == race_lookup("red dragon")
    	|| victim->race == race_lookup("white dragon")
    	|| victim->reclass == reclass_lookup("hermit")
    	|| victim->reclass == reclass_lookup("warlord")
    	|| victim->reclass == reclass_lookup("barbarian")
    	|| victim->reclass == reclass_lookup("cavalier")
    	|| victim->reclass == reclass_lookup("templar")
	|| victim->reclass == reclass_lookup("zealot")))
	{
		// If you don't specialize, 90% normal odds.
		if(!IS_NPC(victim)
		&& victim->pcdata->specialty != gsn_hand_to_hand)
			chance = chance * 3 / 4;
		// If you specialize, full normal odds.
		else
			chance = chance;
	}
	// If you don't have martial arts training
    	else if(get_eq_char(victim, WEAR_WIELD) == NULL
	&& get_eq_char(victim, WEAR_DUAL) == NULL) {
		// Specialized hth allows 3 / 4 chance
		if(!IS_NPC(victim)
                && victim->pcdata->specialty == gsn_hand_to_hand)
                        chance = chance * 3 / 4;
		// Razor claws allows some chance
                else if(!IS_NPC(victim)
                && (paf = affect_find(victim->affected, gsn_razor_claws))
                && paf->location == APPLY_DAMROLL) {
                        chance = chance * 3 / 4;
                }
		else
			return FALSE;
	}

	// The weapon the attacker is using
        chweapon = get_eq_char(ch, WEAR_WIELD);
	// The weapon the parrier using
	victweapon = get_eq_char(victim, WEAR_WIELD);

	// Blind people suck even more... 22%
    	if (!can_see(victim, ch)
	&& number_percent() > get_skill(victim, gsn_blindfighting))
		chance = chance * 3 / 4;

	// Third eye ignores all hand to hand and blindess penalties
	// and even gives a small bonus.
	if (!can_see(victim, ch)
	&& is_affected(victim, gsn_third_eye)) {
		chance = (get_skill(ch, gsn_parry) / 2);
	}

	// Daggers are harder to parry with
	if (victweapon != NULL
	&&  victweapon->item_type == ITEM_WEAPON
	&&  victweapon->value[0] == WEAPON_DAGGER
	&&  victim->charClass != class_lookup("thief"))
        	chance = chance * 3 / 4;

	// You can parry a little better with staves
	// Or katanas
	if(victweapon != NULL
	&& victweapon->item_type == ITEM_WEAPON
	&& (victweapon->value[0] == WEAPON_SPEAR
	|| victweapon->value[0] == WEAPON_KATANA))
        	chance = chance + 8;

	if(chweapon != NULL
        && chweapon->item_type == ITEM_WEAPON
        && chweapon->value[0] == WEAPON_POLEARM)
               chance = chance - 8;

	// wielding two weapons gives a small bonus
	if (get_eq_char(victim, WEAR_WIELD) != NULL
	&& get_eq_char(victim, WEAR_DUAL) != NULL)
		chance += 12;

	if (number_percent() < chance + victim->level - ch->level)
    	{
		act("You parry $n's attack.", ch, NULL, victim, TO_VICT, POS_RESTING);
       		act("$N parries your attack.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        	check_improve(victim, gsn_parry, TRUE, 6);

		if(number_percent() < get_skill(victim, gsn_thrust))
			victim->thrustCounter += number_range(1, 4);

   		return TRUE;
	}

	if(spec_chance > 0 && number_percent() < spec_chance)
	{
		act("You easily stop $n's attack.", ch, NULL, victim, TO_VICT, POS_RESTING);
        	act("$N easily stops your attack.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        	check_improve(victim, gsn_parry, TRUE, 6);

		if(number_percent() < get_skill(victim, gsn_thrust))
			victim->thrustCounter += number_range(1, 2);

		return TRUE;
	}

	return FALSE;
}

/*
 * Check for shield block.
 */
bool
check_shield_block(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int chance;
	int spec_chance=0;
	OBJ_DATA *shield = NULL, *chweapon = NULL;

	if (ch->in_room == victim->in_room)
        	damage(ch, victim, 0, gsn_shield_block, DAM_NONE, FALSE, TYPE_SKILL);

	if (!IS_AWAKE(victim))
		return FALSE;

	if (get_skill(victim, gsn_shield_block) < 1)
		return FALSE;

	chance = (UMIN(get_skill(victim, gsn_shield_block), 100) / 4);

	if(!IS_NPC(victim) && victim->pcdata->specialty == gsn_shield_block)
		spec_chance = (victim->pcdata->learned[gsn_shield_block] - 100) / 2;

	if ((shield = get_eq_char(victim, WEAR_SHIELD)) == NULL)
		return FALSE;

	if (victim->race == race_lookup("dwarf"))
		chance += victim->level / 10;

	chance += shield->level / 5;

	// Flails are harder to block
        chweapon = get_eq_char(ch, WEAR_WIELD);

        if(chweapon != NULL
        &&  chweapon->item_type == ITEM_WEAPON
        &&  chweapon->value[0] == WEAPON_FLAIL)
                        chance = chance * 2 / 3;

	if (!can_see(victim, ch)
	&& number_percent() > get_skill(victim, gsn_blindfighting))
		chance = chance * 3 / 4;

	if (number_percent() < chance + victim->level - ch->level)
  	{
		act("You block $n's attack with $p.", ch, get_eq_char(victim, WEAR_SHIELD), victim, TO_VICT, POS_RESTING);
		act("$N blocks your attack with $p.", ch, get_eq_char(victim, WEAR_SHIELD), victim, TO_CHAR, POS_RESTING);
		check_improve(victim, gsn_shield_block, TRUE, 6);

		if(number_percent() < get_skill(victim, gsn_shield_bash))
			victim->sbashCounter += dice(2, 4);

		return TRUE;
    }

	if(spec_chance > 0 && number_percent() < spec_chance)
	{
        act("You quickly deflect $n's attack with $p.", ch, get_eq_char(victim, WEAR_SHIELD), victim, TO_VICT, POS_RESTING);
		act("$N quickly deflects your attack with $p.", ch, get_eq_char(victim, WEAR_SHIELD), victim, TO_CHAR, POS_RESTING);
       	check_improve(victim, gsn_shield_block, TRUE, 6);

		if(number_percent() < get_skill(victim, gsn_shield_bash))
			victim->sbashCounter += dice(1, 6);

		return TRUE;
	}

    return FALSE;
}

/*
 * Check for dodge.
 */
bool
check_dodge(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int chance;
	int spec_chance=0;
	OBJ_DATA *wield = NULL;
	AFFECT_DATA *paf = NULL;

	if (ch->in_room == victim->in_room)
	        damage(ch, victim, 0, gsn_dodge, DAM_NONE, FALSE, TYPE_SKILL);

	if (!IS_AWAKE(victim))
		return FALSE;

	// Blade runes can't be dodged.
	if ((wield = get_eq_char(ch, WEAR_WIELD)) != NULL) {
		if(ch->charClass == class_lookup("runist")
		&& (paf = affect_find(wield->affected, gsn_blade_rune)) != NULL) 		{
			if(paf->location == APPLY_NONE
			&& paf->modifier == BLADE_RUNE_ACCURACY
			&& number_percent() < 50) {
				return FALSE;
			}
		}
	}

	if (get_skill(victim, gsn_dodge) < 1)
		return FALSE;

	// 40%
	chance = UMIN(get_skill(victim, gsn_dodge), 100) * 2 / 5;

	if(!IS_NPC(victim) && victim->pcdata->specialty == gsn_dodge)
		spec_chance = (victim->pcdata->learned[gsn_dodge] - 100) / 2;

	if (!can_see(victim, ch)
	&& number_percent() > get_skill(victim, gsn_blindfighting))
		chance = chance * 3 / 4;

	if (is_affected(victim, gsn_stance_tiger))
		chance = chance / 2;

	// 5%
	chance += (get_curr_stat(victim, STAT_DEX) / 5);
	// Small vs Medium 2%
	// Small vs Large  4%
	// Small vs Huge   8%
	chance += (ch->size - victim->size) * 2;
	// Levels helps
	chance += (victim->level - ch->level);

	// Exotics help you dodge
	// Exotics help monks a lot, to make up for no shield.
	if ((wield = get_eq_char(victim, WEAR_WIELD)) != NULL && wield->item_type == ITEM_WEAPON && wield->value[0] == WEAPON_EXOTIC) {
		chance += 5;
	}

	if ((wield = get_eq_char(victim, WEAR_DUAL)) != NULL && wield->item_type == ITEM_WEAPON && wield->value[0] == WEAPON_EXOTIC) {
        		chance += 5;
	}

	if (number_percent() < chance)
	{
	        act("You dodge $n's attack.", ch, NULL, victim, TO_VICT, POS_RESTING);
        	act("$N dodges your attack.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        	check_improve(victim, gsn_dodge, TRUE, 6);

			if(number_percent() < get_skill(victim, gsn_dragon_bite))
				victim->dbiteCounter += number_range(1, 4);

			return TRUE;
	}
	if(spec_chance > 0 && number_percent() < spec_chance)
	{
		act("You gracefully evade $n's attack.", ch, NULL, victim, TO_VICT, POS_RESTING);
        	act("$N gracefully evades your attack.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        	check_improve(victim, gsn_dodge, TRUE, 6);
		if(number_percent() < get_skill(victim, gsn_dragon_bite))
				victim->dbiteCounter += number_range(1, 2);
		return TRUE;
	}
	return FALSE;
}


/*
 * Set position of a victim.
 */
void
update_pos(CHAR_DATA * victim)
{
	if (victim->hit > 0)
	{
		if (victim->position <= POS_STUNNED)
			victim->position = POS_STANDING;
		return;
	}

	if (IS_NPC(victim) && victim->hit < 1)
	{
		victim->position = POS_DEAD;
		return;
	}

	if (victim->hit <= -11)
	{
		victim->position = POS_DEAD;
		return;
	}

	if (victim->hit <= -6)
		victim->position = POS_MORTAL;
	else if (victim->hit <= -3)
		victim->position = POS_INCAP;
	else
		victim->position = POS_STUNNED;

	return;
}



/*
 * Start fights.
 */
void
set_fighting(CHAR_DATA * ch, CHAR_DATA * victim)
{
	if (ch->fighting != NULL)
	{
		bug("Set_fighting: already fighting", 0);
		return;
	}

	if (IS_AFFECTED(ch, AFF_SLEEP))
		affect_strip(ch, gsn_sleep);

	ch->fighting = victim;
	ch->position = POS_FIGHTING;

	if (!IS_NPC(ch) && !IS_NPC(victim))
	{
	    log_string("%s attacked %s.", ch->name, victim->name);
	}

	return;
}



/*
 * Stop fights.
 */
void
stop_fighting(CHAR_DATA * ch, bool fBoth)
{
	CHAR_DATA *fch;

	for (fch = char_list; fch != NULL; fch = fch->next)
	{
		if (fch == ch || (fBoth && fch->fighting == ch))
		{
			fch->fighting = NULL;
			fch->position = IS_NPC(fch) ? fch->default_pos : POS_STANDING;
			update_pos(fch);
		}
	}

	return;
}



/*
 * Make a corpse out of a character.
 */
void
make_corpse(CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *corpse;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	char *name;

	if (IS_NPC(ch))
	{
		name = ch->short_descr;
		corpse = create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
		corpse->timer = number_range(3, 6);
		corpse->owner_vnum = ch->pIndexData->vnum;
		if (ch->gold > 0)
		{
			obj_to_obj(create_money(ch->gold, ch->silver), corpse);
			ch->gold = 0;
			ch->silver = 0;
		}
		corpse->cost = 0;
	}
	else
	{
		name = ch->name;
		corpse = create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
		corpse->timer = number_range(25, 40);
		REMOVE_BIT(ch->act, PLR_CANLOOT);
		corpse->owner = str_dup(ch->name);
		if (ch->gold > 1 || ch->silver > 1)
		{
			obj_to_obj(create_money(ch->gold / 2, ch->silver / 2), corpse);
			ch->gold -= ch->gold / 2;
			ch->silver -= ch->silver / 2;
		}
		corpse->cost = 0;
	}

	corpse->level = ch->level;

	sprintf(buf, corpse->short_descr, name);
	free_string(corpse->short_descr);
	corpse->short_descr = str_dup(buf);

	sprintf(buf, corpse->description, name);
	free_string(corpse->description);
	corpse->description = str_dup(buf);

	for (obj = ch->carrying; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;

		obj_from_char(obj);

		if (obj->item_type == ITEM_POTION)
			obj->timer = number_range(500, 1000);
		if (obj->item_type == ITEM_SCROLL)
			obj->timer = number_range(1000, 2500);
		if (IS_SET(obj->extra_flags, ITEM_ROT_DEATH))
		{
			obj->timer = number_range(5, 10);
			REMOVE_BIT(obj->extra_flags, ITEM_ROT_DEATH);
		}
		REMOVE_BIT(obj->extra_flags, ITEM_VIS_DEATH);

		if (IS_SET(obj->extra_flags, ITEM_INVENTORY))
			extract_obj(obj);
		else
			obj_to_obj(obj, corpse);
	}

	obj_to_room(corpse, ch->in_room);
	return;
}



/*
 * Improved Death_cry contributed by Diavolo.
 */
void
death_cry(CHAR_DATA * ch)
{
	ROOM_INDEX_DATA *was_in_room;
	char *msg;
	int door;
	int vnum;
	int arena = 0;

	vnum = 0;
	msg = "You hear $n's death cry.";

	if(IS_SET(ch->in_room->room_flags, ROOM_ARENA))
		arena = 1;

	switch (number_range(0, 9))
	{
	case 0:
		msg = "$n hits the ground ... DEAD.";
		break;
	case 1:
		if (ch->material == 0)
		{
			msg = "$n splatters blood on your armor.";
			break;
		}
	case 2:
		if (IS_SET(ch->parts, PART_GUTS))
		{
			msg = "$n spills $s guts all over the floor.";
			vnum = OBJ_VNUM_GUTS;
		}
		break;
	case 3:
		if (IS_SET(ch->parts, PART_HEAD))
		{
			msg = "$n's severed head plops on the ground.";
			vnum = OBJ_VNUM_SEVERED_HEAD;
		}
		break;
	case 4:
		if (IS_SET(ch->parts, PART_HEART))
		{
			msg = "$n's heart is torn from $s chest.";
			vnum = OBJ_VNUM_TORN_HEART;
		}
		break;
	case 5:
		if (IS_SET(ch->parts, PART_ARMS))
		{
			msg = "$n's arm is sliced from $s dead body.";
			vnum = OBJ_VNUM_SLICED_ARM;
		}
		break;
	case 6:
		if (IS_SET(ch->parts, PART_LEGS))
		{
			msg = "$n's leg is sliced from $s dead body.";
			vnum = OBJ_VNUM_SLICED_LEG;
		}
		break;
	case 7:
		if (IS_SET(ch->parts, PART_BRAINS))
		{
			msg = "$n's head is shattered, and $s brains splash all over you.";
			vnum = OBJ_VNUM_BRAINS;
		}
		break;
	case 8:
		if (IS_SET(ch->parts, PART_WINGS))
		{
			msg = "$n's wing is severed from $s body.";
			vnum = OBJ_VNUM_WING;
		}
		break;
	case 9:
		if (IS_SET(ch->parts, PART_TAIL))
		{
			msg = "$n's tail twitches as it is severed.";
			vnum = OBJ_VNUM_TAIL;
		}
		break;
	}

	if(arena == 1)
		msg = "$n hits the ground ... DEAD.";

	act(msg, ch, NULL, NULL, TO_ROOM, POS_RESTING);

	if (vnum != 0 && arena == 0)
	{
		char buf[MAX_STRING_LENGTH];
		OBJ_DATA *obj;
		char *name;

		name = IS_NPC(ch) ? ch->short_descr : ch->name;
		obj = create_object(get_obj_index(vnum), 0);
		obj->timer = number_range(4, 7);
		if (IS_NPC(ch))
		{
			obj->owner_vnum = ch->pIndexData->vnum;
		}
		obj->owner = str_dup(name);

		sprintf(buf, obj->short_descr, name);
		free_string(obj->short_descr);
		obj->short_descr = str_dup(buf);

		sprintf(buf, obj->description, name);
		free_string(obj->description);
		obj->description = str_dup(buf);

		/* Add person's name to object for easier sorting - Tsongas */
                sprintf(buf, obj->name, ch->name);
                free_string(obj->name);
                obj->name = str_dup(buf);

		if (obj->item_type == ITEM_FOOD)
		{
			if (IS_SET(ch->form, FORM_POISON))
				obj->value[3] = 1;
			else if (!IS_SET(ch->form, FORM_EDIBLE))
				obj->item_type = ITEM_TRASH;
		}

		obj_to_room(obj, ch->in_room);
	}

	if (IS_NPC(ch))
		msg = "You hear something's death cry.";
	else
		msg = "You hear someone's death cry.";

	was_in_room = ch->in_room;
	for (door = 0; door <= 5; door++)
	{
		EXIT_DATA *pexit;

		if ((pexit = was_in_room->exit[door]) != NULL
			&& pexit->u1.to_room != NULL
			&& pexit->u1.to_room != was_in_room)
		{
			ch->in_room = pexit->u1.to_room;
			act(msg, ch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
	}
	ch->in_room = was_in_room;

	return;
}



void
raw_kill(CHAR_DATA * victim)
{
	int i;
	OBJ_DATA *obj, *obj_next;

	stop_fighting(victim, TRUE);

	if (victim->race != race_lookup("undead"))
	{
		death_cry(victim);
		make_corpse(victim);
	}
	else {
		for(obj = victim->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			obj_from_char(obj);
			if(victim->in_room != NULL)
				obj_to_room(obj, victim->in_room);
		}
		act("$n crumbles into a pile of thin dust.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	if (IS_NPC(victim))
	{
		victim->pIndexData->killed++;
		kill_table[URANGE(0, victim->level, MAX_LEVEL - 1)].killed++;
		extract_char(victim, TRUE);
		return;
	}

	extract_char(victim, FALSE);
	while (victim->affected) {
		/* shit, messy, but needed */
		if(skill_table[victim->affected->type].end_fun != end_null)
			skill_table[victim->affected->type].end_fun((void*)victim, TARGET_CHAR);
		affect_remove(victim, victim->affected);
	}
	victim->affected_by = race_table[victim->race].aff;
	if (victim->race == race_lookup("dwarf")
	&& victim->remort > 0)
		SET_BIT(victim->affected_by, AFF_DARK_VISION);
        if (victim->race == race_lookup("elf")
        && victim->remort
        && victim->rem_sub == 2)
                SET_BIT(victim->affected_by, AFF_DARK_VISION);
        if (victim->race == race_lookup("kirre")
        && victim->remort
        && victim->rem_sub == 2)
                SET_BIT(victim->affected_by, AFF_DARK_VISION);
        if (victim->race == race_lookup("human")
        && victim->remort)
                SET_BIT(victim->affected_by, AFF_WATERWALK);

	for (i = 0; i < 4; i++)
		victim->armor[i] = 100;
	victim->position = POS_RESTING;
	victim->hit = UMAX(1, victim->hit);
	victim->mana = UMAX(1, victim->mana);
	victim->move = UMAX(1, victim->move);

	return;
}



void
group_gain(CHAR_DATA * ch, CHAR_DATA * victim)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *gch, *vch;
	CHAR_DATA *lch;
	int xp;
	int members;
	int group_levels;
	int npc_members;
	int pc_members;
	int weapon_xp = 0;
	int weapon_count;
	int vassalpts;
	int xp_modifier;
	int found = FALSE;

	/*
	 * Monsters don't get kill xp's or alignment changes.
	 * P-killing doesn't help either.
	 * Dying of mortal wounds or poison doesn't give xp to anyone!
	 */
	if (victim == ch)
		return;

	members = 0;
	pc_members = 0;
	npc_members = 0;
	group_levels = 0;
	xp_modifier = URANGE(80, xp_modifier, 120);
	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
	{
		if (is_same_group(gch, ch))
		{
			if (IS_NPC(gch))
			{
				npc_members++;
			}
			else
			{
				if (is_same_clan(ch, gch) ||
				    (is_clan(ch) && is_clan(gch)))
					xp_modifier += 5;
				else
					xp_modifier -= 5;

				pc_members++;
			}

			// Don't nuke higher level pc experience
			if (IS_NPC(gch))
				group_levels += UMIN(gch->level / 2, 51);
			else
				group_levels += UMIN(gch->level, 51);

		}
	}

	members = pc_members + ((npc_members - 1) / 2);

	/* common case: bribed mob kills another mob! */
	if (members == 0)
	{
		members = 1;
		group_levels = UMIN(ch->level, 51);
	}

	lch = (ch->leader != NULL) ? ch->leader : ch;

	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
	{
		OBJ_DATA *wield;
		OBJ_DATA *obj;
		OBJ_DATA *obj_next;

		if (!is_same_group(gch, ch) || IS_NPC(gch))
			continue;

		/* cheasy charm trick */
		if ((is_affected(gch, gsn_brotherhood) || is_affected(lch, gsn_brotherhood)) && (gch->level - lch->level > 12 || lch->level - gch->level > 12)) {
			Cprintf(gch, "You're not the right level for this group.\n\r");
			continue;
		}

		if (!is_affected(gch, gsn_brotherhood) && !is_affected(lch, gsn_brotherhood) && (gch->level - lch->level > 8 || lch->level - gch->level > 8)) {
			Cprintf(gch, "You're not the right level for this group.\n\r");
			continue;
		}

		// Mark progress on quests
		if (!IS_NPC(gch) && IS_QUESTING(gch) && IS_NPC(victim))
		{
			if (gch->pcdata->quest.type == QUEST_TYPE_VILLAIN && gch->pcdata->quest.target == victim->pIndexData->vnum)
			{
				Cprintf(gch, "You have almost completed your QUEST!\n\r");
				Cprintf(gch, "Return to the questmaster before your time runs out!\n\r");
				gch->pcdata->quest.progress++;
			}
			else if (gch->pcdata->quest.type == QUEST_TYPE_ASSAULT && gch->pcdata->quest.target == victim->pIndexData->vnum)
			{
				gch->pcdata->quest.progress++;
				Cprintf(gch, "GOOD! You have defeated %d foes during your current quest.\n\r", gch->pcdata->quest.progress);
				Cprintf(gch, "Keep going, but return to the questmaster before your time runs out!\n\r");
			}
		}

		xp = xp_compute(gch, victim, group_levels, xp_modifier);

		if(IS_SET(victim->toggles, TOGGLES_NOEXP)) {
			Cprintf(gch, "You receive no experience points.\n\r");
			xp = 0;
			continue;
		}

		if (!IS_NPC(victim)) {
			xp = 0;
			continue;
		}

		if(gch->in_room->area->security < 9) {
			Cprintf(ch, "You can't earn exp in unfinished areas.\n\r");
			xp = 0;
			continue;
		}

		// Intelligent weapons get some xp.
		// Watch out for dual wield.
		weapon_count = 0;
		if ((wield = get_eq_char(gch, WEAR_WIELD)) != NULL && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT))
			weapon_count++;
		if ((wield = get_eq_char(gch, WEAR_DUAL)) != NULL && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT))
			weapon_count++;
		if((wield = get_eq_char(gch, WEAR_RANGED)) != NULL
                && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT))
                        weapon_count++;                          

		if ((wield = get_eq_char(gch, WEAR_WIELD)) != NULL && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT)) {
			weapon_xp = xp / (5 + (5 * weapon_count));
			Cprintf(gch, "%s receives %d experience points.\n\r", capitalize(wield->short_descr), weapon_xp);

			advance_weapon(gch, wield, weapon_xp);
		}
		
		if ((wield = get_eq_char(gch, WEAR_DUAL)) != NULL && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT)) {
			weapon_xp = xp / (5 + (5 * weapon_count));
			Cprintf(gch, "%s receives %d experience points.\n\r", capitalize(wield->short_descr), weapon_xp);
			advance_weapon(gch, wield, weapon_xp);
		}
		if((wield = get_eq_char(gch, WEAR_RANGED)) != NULL
                && IS_WEAPON_STAT(wield, WEAPON_INTELLIGENT)) {
                        weapon_xp = xp / (5 + (5 * weapon_count));
                        Cprintf(gch, "%s receives %d experience points.\n\r", capitalize(wield->short_descr), weapon_xp);
                        advance_weapon(gch, wield, weapon_xp);
                }                           

		// Convert some exp into patron/vassal points
		// Patron code starts here
		vassalpts = xp / 10;

		// Ch got the kill
		// Gch is the vassal
		// Vch is his patron
		if (gch->patron != NULL && !IS_SET(gch->toggles, TOGGLES_PLEDGE))
		{
			found = FALSE;

			for(d = descriptor_list; d != NULL; d = d->next)
			{
				vch = d->character;
				if(vch == NULL)
					continue;

				if (!str_cmp(vch->name, gch->patron))
				{
					found = TRUE;
					if (gch->level >= vch->level)
					{
                          	     			Cprintf(gch, "{cYou and your patron stands as equals now.{x\n\r");
						vassalpts = 0;
					}

					vassalpts = vassalpts + gch->to_pass;
					Cprintf(vch, "{cYou receive %d patron points from your vassal, %s.{x\n\r", vassalpts, gch->name);
					Cprintf(gch, "{cYour patron %s receives %d vassal points.{x\n\r", vch->name, vassalpts);

					xp = xp * 9 / 10;
					vch->pass_along = vch->pass_along + vassalpts;
				
					if (gch->to_pass > 0)
					{
						gch->to_pass = 0;
						save_char_obj(gch, FALSE);
					}

					break;
             	   		}

			}

			if(found == FALSE)
             	   	{
				Cprintf(gch, "{cYou receive %d vassal points.{x\n\r", vassalpts);
				/* builds the bank while patron is offline. */
				gch->to_pass = gch->to_pass + vassalpts;
				xp = xp * 9 / 10;
                		}
		}

		/* patron change end */
		
		if (double_xp_ticks > 0) {
			xp *= 2;
		}
		
		Cprintf(gch, "You receive %d experience points.\n\r", xp);
			
		gain_exp(gch, xp);

		for (obj = gch->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			if (obj->wear_loc == WEAR_NONE)
				continue;

			if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(gch))
				|| (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(gch))
				|| (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(gch)))
			{
				act("You are zapped by $p.", gch, obj, NULL, TO_CHAR, POS_RESTING);
				act("$n is zapped by $p.", gch, obj, NULL, TO_ROOM, POS_RESTING);
				obj_from_char(obj);

				if (IS_OBJ_STAT(obj, ITEM_NODROP))
					obj_to_char(obj, gch);
				else
					obj_to_room(obj, gch->in_room);
			}
		}
	}

	return;
}



/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int
xp_compute(CHAR_DATA * gch, CHAR_DATA * victim, int total_levels, int xp_modifier)
{
	int xp, base_exp;
	int align, level_range;
	int change;

	level_range = victim->level - (UMIN(51, gch->level));

	/* compute the base exp */
	switch (level_range)
	{
	default:
		base_exp = 0;
		break;
	case -9:
		base_exp = 1;
		break;
	case -8:
		base_exp = 2;
		break;
	case -7:
		base_exp = 5;
		break;
	case -6:
		base_exp = 9;
		break;
	case -5:
		base_exp = 11;
		break;
	case -4:
		base_exp = 22;
		break;
	case -3:
		base_exp = 33;
		break;
	case -2:
		base_exp = 50;
		break;
	case -1:
		base_exp = 66;
		break;
	case 0:
		base_exp = 83;
		break;
	case 1:
		base_exp = 99;
		break;
	case 2:
		base_exp = 120;
		break;
	case 3:
		base_exp = 141;
		break;
	case 4:
		base_exp = 162;
		break;
        case 5:
		base_exp = 180;
		break;
	}

	if (level_range > 5)
		base_exp = 180 + 12 * (level_range - 5);

	/* do alignment computations */

	align = victim->alignment - gch->alignment;

	if (IS_SET(victim->act, ACT_NOALIGN))
	{
		/* no change */
	}

	else if (align > 500)		/* monster is more good than slayer */
	{
		change = (align - 500) * base_exp / 500 * gch->level / total_levels;
		change = UMAX(1, change);
		gch->alignment = UMAX(-1000, gch->alignment - change);
	}

	else if (align < -500)		/* monster is more evil than slayer */
	{
		change = (-1 * align - 500) * base_exp / 500 * gch->level / total_levels;
		change = UMAX(1, change);
		gch->alignment = UMIN(1000, gch->alignment + change);
	}

	else
		/* improve this someday */
	{
		change = gch->alignment * base_exp / 500 * gch->level / total_levels;
		gch->alignment -= change;
	}

	/* calculate exp multiplier */
	if (IS_SET(victim->act, ACT_NOALIGN))
		xp = base_exp;

	else if (gch->alignment > 500)	/* for goodie two shoes */
	{
		if (victim->alignment < -750)
			xp = (base_exp * 4) / 3;

		else if (victim->alignment < -500)
			xp = (base_exp * 5) / 4;

		else if (victim->alignment > 750)
			xp = base_exp / 4;

		else if (victim->alignment > 500)
			xp = base_exp / 2;

		else if (victim->alignment > 250)
			xp = (base_exp * 3) / 4;

		else
			xp = base_exp;
	}

	else if (gch->alignment < -500)		/* for baddies */
	{
		if (victim->alignment > 750)
			xp = (base_exp * 5) / 4;

		else if (victim->alignment > 500)
			xp = (base_exp * 11) / 10;

		else if (victim->alignment < -750)
			xp = base_exp / 2;

		else if (victim->alignment < -500)
			xp = (base_exp * 3) / 4;

		else if (victim->alignment < -250)
			xp = (base_exp * 9) / 10;

		else
			xp = base_exp;
	}

	else if (gch->alignment > 200)	/* a little good */
	{

		if (victim->alignment < -500)
			xp = (base_exp * 6) / 5;

		else if (victim->alignment > 750)
			xp = base_exp / 2;

		else if (victim->alignment > 0)
			xp = (base_exp * 3) / 4;

		else
			xp = base_exp;
	}

	else if (gch->alignment < -200)		/* a little bad */
	{
		if (victim->alignment > 500)
			xp = (base_exp * 6) / 5;

		else if (victim->alignment < -750)
			xp = base_exp / 2;

		else if (victim->alignment < 0)
			xp = (base_exp * 3) / 4;

		else
			xp = base_exp;
	}

	else
		/* neutral */
	{

		if (victim->alignment > 500 || victim->alignment < -500)
			xp = (base_exp * 4) / 3;

		else if (victim->alignment < 200 && victim->alignment > -200)
			xp = base_exp / 2;

		else
			xp = base_exp;
	}

	// more exp at the low levels
	if (gch->level < 6)
		xp = 10 * xp / (gch->level + 4);

/*
	// less at high
	if (gch->level > 35)
		xp = 15 * xp / (gch->level - 25);
*/

	/* reduce for playing time for non-humans */
	if (gch->race != race_lookup("human"))
	{
		/*
		time_per_level = 4 * get_hours(gch) / gch->level;

		time_per_level = URANGE(4, time_per_level, 8);
		if (gch->level < 10)	
			time_per_level = UMAX(time_per_level, (10 - gch->level));
		*/
		xp = xp;
	}
	else
	{
		xp = xp * 14 / 12;
	}

	/* randomize the rewards */
	xp = number_range(xp, xp * 5 / 4);

	// Brotherhood gives a small xp bonus when grouped
	if(is_affected(gch, gsn_brotherhood))
		total_levels = (total_levels - gch->level) / 5;
	else
		total_levels = (total_levels - gch->level) / 4;

	total_levels += gch->level;
	xp = xp * gch->level / (UMAX(1, total_levels));

	xp = xp * xp_modifier / 100;

	return xp;
}

int
get_msg_index(int dam, const struct damage_message_type *table)
{
	int i;

	for(i=0;i<=MAX_DAMAGE_MESSAGES;i++) {
		if(dam <= table[i].minimum)
			return i;
	}

	if(dam >= table[MAX_DAMAGE_MESSAGES].minimum)
		return MAX_DAMAGE_MESSAGES;

	return 0;
}

void
dam_message(CHAR_DATA *ch,      // Dealt the damage
            CHAR_DATA *victim,  // Took the damage
            int dam,            // How badly
            int dt,             // What dealt it
            bool immune,        // Or not.
            int special)        // For lots of cases.
{
	int index;
	char buf_you[256], buf_vict[256], buf_room[256], buf_self[256];
	char buf_wiznet[256];

	sprintf(buf_wiznet, " {r(%d){x", dam);

	if(IS_SET(special, TYPE_MAGIC)) {
		index = get_msg_index(dam, magic_message_table);

		// Shown to everyone in the room
		sprintf(buf_room, "%s %s %s $N%s%s",
			(IS_SET(special, TYPE_ANONYMOUS)) ? "The" : "$n's",
			skill_table[dt].noun_damage,
			(immune) ?  "has no effect on" : magic_message_table[index].prefix,
			(immune) ? "" : magic_message_table[index].postfix,
			(immune) ? "." : magic_message_table[index].punctuation);

		// Shown to attacker
		sprintf(buf_you, "%s %s %s $N%s%s%s",
			(IS_SET(special, TYPE_ANONYMOUS)) ? "The" : "Your",
			skill_table[dt].noun_damage,
			(immune) ?  "has no effect on" : magic_message_table[index].prefix,
			(immune) ? "" : magic_message_table[index].postfix,
			(immune) ? "." : magic_message_table[index].punctuation,
			IS_SET(ch->wiznet, WIZ_DAMAGE) ? buf_wiznet : "");

		// Shown to defender
		sprintf(buf_vict, "%s %s %s you%s%s%s",
			(IS_SET(special, TYPE_ANONYMOUS)) ? "The" : "$n's",
			skill_table[dt].noun_damage,
			(immune) ?  "has no effect on" : magic_message_table[index].prefix,
			(immune) ? "" : magic_message_table[index].postfix,
			(immune) ? "." : magic_message_table[index].punctuation,
			IS_SET(victim->wiznet, WIZ_DAMAGE) ? buf_wiznet : "");

		sprintf(buf_self, "Your %s %s you%s%s%s",
			skill_table[dt].noun_damage,
			(immune) ?  "has no effect on" : magic_message_table[index].prefix,
			(immune) ? "" : magic_message_table[index].postfix,
			(immune) ? "." : magic_message_table[index].punctuation,
			IS_SET(ch->wiznet, WIZ_DAMAGE) ? buf_wiznet : "");
	}
	else if(IS_SET(special, TYPE_HIT)) {
		index = get_msg_index(dam, normal_message_table);

		sprintf(buf_room, "$n's %s %s %s $N%s",
			(IS_SET(special, TYPE_SPECIALIZED)) ?
				"specialized" : normal_message_table[index].prefix,
			attack_table[dt].noun,
			(immune) ? "doesn't even bruise" : normal_message_table[index].postfix,
			(immune) ? "." : normal_message_table[index].punctuation);

		sprintf(buf_you, "Your %s %s %s $N%s%s",
			(IS_SET(special, TYPE_SPECIALIZED)) ?
				"specialized" : normal_message_table[index].prefix,
			attack_table[dt].noun,
			(immune) ? "doesn't even bruise" : normal_message_table[index].postfix,
			(immune) ? "." : normal_message_table[index].punctuation,
			IS_SET(ch->wiznet, WIZ_DAMAGE) ? buf_wiznet : "");

		sprintf(buf_vict, "$n's %s %s %s you%s%s",
			(IS_SET(special, TYPE_SPECIALIZED)) ?
				"specialized" : normal_message_table[index].prefix,
			attack_table[dt].noun,
			(immune) ? "doesn't even bruise" : normal_message_table[index].postfix,
			(immune) ? "." : normal_message_table[index].punctuation,
			IS_SET(victim->wiznet, WIZ_DAMAGE) ? buf_wiznet : "");

		sprintf(buf_self, "Your %s %s %s you%s%s",
			(IS_SET(special, TYPE_SPECIALIZED)) ?
				"specialized" : normal_message_table[index].prefix,
			attack_table[dt].noun,
			(immune) ? "doesn't even bruise" : normal_message_table[index].postfix,
			(immune) ? "." : normal_message_table[index].punctuation,
			IS_SET(ch->wiznet, WIZ_DAMAGE) ? buf_wiznet : "");
	}
	else if(IS_SET(special, TYPE_SKILL)) {
		index = get_msg_index(dam, normal_message_table);

		sprintf(buf_room, "$n's %s %s %s $N%s",
			normal_message_table[index].prefix,
			skill_table[dt].noun_damage,
			(immune) ? "doesn't even bruise" : normal_message_table[index].postfix,
			(immune) ? "." : normal_message_table[index].punctuation);

		sprintf(buf_you, "Your %s %s %s $N%s%s",
			normal_message_table[index].prefix,
			skill_table[dt].noun_damage,
			(immune) ? "doesn't even bruise" : normal_message_table[index].postfix,
			(immune) ? "." : normal_message_table[index].punctuation,
			IS_SET(ch->wiznet, WIZ_DAMAGE) ? buf_wiznet : "");

		sprintf(buf_vict, "$n's %s %s %s you%s%s",
				normal_message_table[index].prefix,
			skill_table[dt].noun_damage,
			(immune) ? "doesn't even bruise" : normal_message_table[index].postfix,
			(immune) ? "." : normal_message_table[index].punctuation,
			IS_SET(victim->wiznet, WIZ_DAMAGE) ? buf_wiznet : "");

		sprintf(buf_self, "Your %s %s %s you%s%s",
			normal_message_table[index].prefix,
			skill_table[dt].noun_damage,
			(immune) ? "doesn't even bruise" : normal_message_table[index].postfix,
			(immune) ? "." : normal_message_table[index].punctuation,
			IS_SET(ch->wiznet, WIZ_DAMAGE) ? buf_wiznet : "");
	}

	// Show the message.
	if (ch == victim) {
		act(buf_room, ch, NULL, victim, TO_ROOM, POS_RESTING);
		act(buf_self, ch, NULL, victim, TO_CHAR, POS_RESTING);
	}
	else
	{
		act(buf_room, ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		act(buf_you, ch, NULL, victim, TO_CHAR, POS_RESTING);
		act(buf_vict, ch, NULL, victim, TO_VICT, POS_RESTING);
	}

	return;
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void
disarm(CHAR_DATA * ch, CHAR_DATA * victim)
{
	OBJ_DATA *obj;
	bool offhand = FALSE;

	/* if wielding two weapons, lose one */
	if (get_eq_char(victim, WEAR_WIELD) != NULL
		&& get_eq_char(victim, WEAR_DUAL) != NULL)
	{
		if (number_percent() < 50)
		{
			obj = get_eq_char(victim, WEAR_WIELD);
		}
		else
		{
			obj = get_eq_char(victim, WEAR_DUAL);
			offhand = TRUE;
		}
	}
	else if (get_eq_char(victim, WEAR_WIELD) == NULL
			 && get_eq_char(victim, WEAR_DUAL) != NULL)
	{
		obj = get_eq_char(victim, WEAR_DUAL);
		offhand = TRUE;
	}
	else
		obj = get_eq_char(victim, WEAR_WIELD);

	if (obj == NULL)
		return;

	if (IS_OBJ_STAT(obj, ITEM_NOREMOVE))
	{
		act("$S weapon won't budge!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		act("$n tries to disarm you, but your weapon won't budge!", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		return;
	}

	if (IS_SET(obj->extra_flags, ITEM_ROT_DEATH))
        {
                obj->timer = number_range(5, 10);
                REMOVE_BIT(obj->extra_flags, ITEM_ROT_DEATH);
        }

	if (!offhand)
	{
		act("$n {rDISARMS{x your main weapon and sends it flying!", ch, NULL, victim, TO_VICT, POS_RESTING);
	}
	else
	{
		act("$n {rDISARMS{x your second weapon and sends it flying!", ch, NULL, victim, TO_VICT, POS_RESTING);
	}
	act("You disarm $N!", ch, NULL, victim, TO_CHAR, POS_RESTING);
	act("$n disarms $N!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);

	obj_from_char(obj);
	if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_INVENTORY))
	{
		obj_to_char(obj, victim);
	}
	else if(IS_WEAPON_STAT(obj, WEAPON_INTELLIGENT)) {
		Cprintf(victim, "Your weapon flips back into your possession!\n\r");
		obj_to_char(obj, victim);
	}
	else if(get_skill(ch, gsn_weapon_catch) > 0) {
		if(number_percent() < get_skill(ch, gsn_weapon_catch) / 4
		&& can_see_obj(ch, obj)) {
			act("You deftly catch $p!", ch, obj, victim, TO_CHAR, POS_RESTING);
			act("$n deftly catches $p!", ch, obj, victim, TO_ROOM, POS_RESTING);
			obj_to_char(obj, ch);
			check_improve(ch, gsn_weapon_catch, TRUE, 1);
		}
		else {
			check_improve(ch, gsn_weapon_catch, FALSE, 1);
			obj_to_room(obj, victim->in_room);
                	strcpy(disarmed_weapon, obj->name);

                	if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim, obj))
                        	get_obj(victim, obj, NULL);
		}
	}
	else
	{
		obj_to_room(obj, victim->in_room);
		strcpy(disarmed_weapon, obj->name);

		if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim, obj))
			get_obj(victim, obj, NULL);
	}

	return;
}

void
do_berserk(CHAR_DATA * ch, char *argument)
{
	int chance;

	if ((chance = get_skill(ch, gsn_berserk)) == 0
		|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_BERSERK))
		|| is_affected(ch, gsn_taunt)
		|| (!IS_NPC(ch)
		&& ch->level < skill_table[gsn_berserk].skill_level[ch->charClass]))
	{
		Cprintf(ch, "You turn red in the face, but nothing happens.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_BERSERK) || is_affected(ch, gsn_berserk)
		|| IS_AFFECTED(ch, AFF_TAUNT)
		|| is_affected(ch, gsn_frenzy))
	{
		Cprintf(ch, "You get a little madder.\n\r");
		return;
	}
	if (is_affected(ch, gsn_guardian)) {
                Cprintf(ch, "You're too busy defending to frenzy.\n\r");
                return;
        }
	if (IS_AFFECTED(ch, AFF_CALM))
	{
		Cprintf(ch, "You're feeling too mellow to go berserk.\n\r");
		return;
	}

	if (ch->mana < 50 || ch->move < 50)
	{
		Cprintf(ch, "You can't get up enough energy.\n\r");
		return;
	}

	// Base 4 / 5 skill chance.
	chance = chance * 4 / 5;
	
	/* modifiers */
	if (ch->race == race_lookup("dwarf"))
		chance += 20;

	/* fighting */
	if (ch->position == POS_FIGHTING)
		chance += 10;

	if (number_percent() < chance)
	{
		AFFECT_DATA af;

		WAIT_STATE(ch, PULSE_VIOLENCE);
		ch->mana -= 50;
		ch->move -= 50;

		/* heal a little damage */
		if(!is_affected(ch, gsn_dissolution)) {
			ch->hit += ch->level * 3;
			ch->hit = UMIN(ch->hit, MAX_HP(ch));
		}

		Cprintf(ch, "Your pulse races as you are consumed by rage!\n\r");
		act("$n gets a wild look in $s eyes.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		check_improve(ch, gsn_berserk, TRUE, 2);

		af.where = TO_AFFECTS;
		af.type = gsn_berserk;
		af.level = ch->level;
		af.duration = ch->level / 6;
		af.modifier = UMAX(1, ch->level / 5);
		af.bitvector = AFF_BERSERK;

		af.location = APPLY_HITROLL;
		affect_to_char(ch, &af);

		af.location = APPLY_DAMROLL;
		affect_to_char(ch, &af);

		af.modifier = UMAX(10, 5 * (ch->level / 5));
		if(ch->race == race_lookup("dwarf"))
			af.modifier /= 2;
		af.location = APPLY_AC;
		affect_to_char(ch, &af);

		af.location = APPLY_NONE;
		af.modifier = ch->level * 3; 
		affect_to_char(ch, &af);
	}

	else
	{
		WAIT_STATE(ch, PULSE_VIOLENCE);
		ch->move -= 25;

		Cprintf(ch, "Your pulse speeds up, but only wear yourself out.\n\r");
		check_improve(ch, gsn_berserk, FALSE, 2);
	}
}

void
do_living_stone(CHAR_DATA * ch, char *argument)
{
	int chance;

	if ((chance = get_skill(ch, gsn_living_stone)) == 0
		|| (IS_NPC(ch)
	&& ch->level < skill_table[gsn_living_stone].skill_level[ch->charClass]))
	{
		Cprintf(ch, "You adopt a rather statuesque persona.\n\r");
		return;
	}

	if (is_affected(ch, gsn_living_stone))
	{
		Cprintf(ch, "If you were any more rocklike, you'd be a foyer centerpiece.\n\r");
		return;
	}

	if ((ch->mana < 30) || (ch->move < 30))
	{
		Cprintf(ch, "You can't muster up the energy to become stone.\n\r");
		return;
	}

	if (number_percent() < chance)
	{
		AFFECT_DATA af;

		WAIT_STATE(ch, PULSE_VIOLENCE);
		ch->mana -= 30;
		ch->move -= 30;
		Cprintf(ch, "You feel your flesh harden as you become living rock!\n\r");
		act("$n's flesh stiffens and fades to gray.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		check_improve(ch, gsn_living_stone, TRUE, 6);

		/* affects of becoming stone */
		af.where = TO_AFFECTS;
		af.type = gsn_living_stone;
		af.level = ch->level;
		af.duration = number_fuzzy(ch->level / 3);
		af.bitvector = 0;

		/* affect to armor */
		af.modifier = UMAX(5, 5 * (ch->level / 10)) * -1;
		af.location = APPLY_AC;
		affect_to_char(ch, &af);

		/* affect to damroll */
		af.modifier = UMAX(1, ch->level / 7);
		af.location = APPLY_DAMROLL;
		affect_to_char(ch, &af);

		/* affect to resistences */
		af.where = TO_RESIST;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = RES_SLASH;
		affect_to_char(ch, &af);

		/*REMOVE_BIT(ch->affected_by, AFF_FLYING); */
	}
	else
	{
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
		ch->mana -= 15;
		ch->move -= 15;
		Cprintf(ch, "Your muscles stiffen, but then relax.\n\r");
		check_improve(ch, gsn_living_stone, FALSE, 4);
	}
}

void
end_living_stone(void *vo, int target)
{
/*  CHAR_DATA* ch = (CHAR_DATA*) vo;

   SET_BIT(ch->affected_by, AFF_FLYING); */
}

void
do_bash(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;
	int dam = 0;

	one_argument(argument, arg);

	if ((chance = get_skill(ch, gsn_bash)) == 0
		|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_BASH))
		|| (!IS_NPC(ch)
			&& ch->level < skill_table[gsn_bash].skill_level[ch->charClass]))
	{
		Cprintf(ch, "Bashing? What's that?\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "But you aren't fighting anyone!\n\r");
			return;
		}
	}

	else if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim->position < POS_FIGHTING)
	{
		act("You'll have to let $M get back up first.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You try to bash your brains out, but fail.\n\r");
		return;
	}

	if (is_safe(ch, victim)) {
		Cprintf(ch, "You can't bash them. Leave them alone.\n\r");
		return;
	}

	if (IS_NPC(victim) &&
		victim->fighting != NULL &&
		!is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		act("But $N is your friend!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	/* modifiers */
	/* size  and weight */
	chance += ch->carry_weight / 500;
	chance -= victim->carry_weight / 500;

	if (victim->race != race_lookup("elf"))
	{
		if (ch->size < victim->size)
			chance += (ch->size - victim->size) * 12;
		else
			chance += (ch->size - victim->size) * 5;
	}
	else if (victim->race == race_lookup("elf"))
	{
		/* elven resistance to bash!! */
		chance = chance * 5 / 12;
	}

	/* stats */
	chance += get_curr_stat(ch, STAT_STR);
	chance -= (get_curr_stat(victim, STAT_DEX) * 4) / 3;

	/* speed */
	if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
		chance += 10;
	if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
		chance -= 20;

	/* level */
	chance += (ch->level - victim->level) * 2;

	check_killer(ch, victim);

	/* tumbling out of the way!! */
	if (check_tumbling(ch, victim))
	{
		damage(ch, victim, 0, gsn_bash, DAM_BASH, FALSE, TYPE_SKILL);
		check_improve(ch, gsn_bash, FALSE, 1);
		WAIT_STATE(ch, skill_table[gsn_bash].beats);
		DAZE_STATE(ch, 2 * PULSE_VIOLENCE);
		return;
	}

	// Monk Shoulder Throw to counter bash... very nasty
	if (number_percent() < get_skill(victim, gsn_shoulder_throw) / 2) {
		dam = victim->level + dice(4, get_curr_stat(victim, STAT_STR));
		check_improve(victim, gsn_shoulder_throw, TRUE, 1);
		check_improve(ch, gsn_bash, FALSE, 1);
		WAIT_STATE(ch, skill_table[gsn_bash].beats);
		DAZE_STATE(ch, 3 * PULSE_VIOLENCE);
		ch->position = POS_RESTING;
		Cprintf(victim, "You counter %s's bash and slam them into the ground!\n\r", IS_NPC(ch) ? ch->short_descr : ch->name);
		act("$n intercepts $N's bash and slams $M into the ground!", victim, NULL, ch, TO_ROOM, POS_RESTING);
		damage(victim, ch, dam, gsn_shoulder_throw, DAM_BASH, TRUE, TYPE_SKILL);
		return;
	}
	else
		check_improve(victim, gsn_shoulder_throw, FALSE, 1);

	/* now the attack */
	if (number_percent() < chance)
	{

		act("$n sends you sprawling with a powerful bash!", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("You slam into $N, and send $M flying!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		act("$n sends $N sprawling with a powerful bash.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		check_improve(ch, gsn_bash, TRUE, 1);

		//		Small	Medium	Large	Huge
		// level 10	2-14	3-18	4-22	6-30
		// level 25	2-29	3-33	4-37	6-45
		// level 51	2-55	3-59	4-63	6-71
		dam = dice(ch->size, 4) + number_range(1, ch->level);
		DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
		WAIT_STATE(ch, skill_table[gsn_bash].beats);
		victim->position = POS_RESTING;
		damage(ch, victim, dam, gsn_bash, DAM_BASH, TRUE, TYPE_SKILL);
		WAIT_STATE(victim, dice(2, 8));
	}
	else
	{
		damage(ch, victim, 0, gsn_bash, DAM_BASH, FALSE, TYPE_SKILL);
		act("You fall flat on your face!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 50)
			Cprintf(ch, "!!SOUND(sounds/wav/fall*.wav V=80 P=20 T=admin)");
		act("$n falls flat on $s face.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		act("You evade $n's bash, causing $m to fall flat on $s face.", ch, NULL, victim, TO_VICT, POS_RESTING);
		check_improve(ch, gsn_bash, FALSE, 1);
		ch->position = POS_RESTING;
		DAZE_STATE(ch, 2 * PULSE_VIOLENCE);
		WAIT_STATE(ch, skill_table[gsn_bash].beats);
	}
}


void
do_transferance(CHAR_DATA * ch, char *argument)
{
	AFFECT_DATA af;
	int diff1, diff2;
	int chance;

	if (IS_NPC(ch))
		return;

	if (get_skill(ch, gsn_transferance) < 1)
	{
		Cprintf(ch, "You transfer nothing into nothing.\n\r");
		return;
	}

	if (is_affected(ch, gsn_transferance))
	{
		Cprintf(ch, "You are already in transferance!\n\r");
		return;
	}

	if (ch->hit < 10)
	{
		Cprintf(ch, "You are too hurt to transfer!\n\r");
		return;
	}

	if (ch->mana < 10)
	{
		Cprintf(ch, "You are too weak to transfer!\n\r");
		return;
	}

	chance = get_skill(ch, gsn_transferance);
	if (number_percent() > chance)
	{
		Cprintf(ch, "You fail in your attempt to transfer energy.\n\r");
		check_improve(ch, gsn_transferance, FALSE, 2);
		return;
	}

	diff1 = MAX_HP(ch) - MAX_MANA(ch);
	diff2 = ch->mana;

	Cprintf(ch, "You channelize your energies!\n\r");

	af.where = TO_AFFECTS;
	af.type = gsn_transferance;
	af.level = ch->level;
	af.duration = 5;
	af.modifier = -1 * diff1;
	af.bitvector = 0;
	af.location = APPLY_HIT;
	affect_to_char(ch, &af);

	af.location = APPLY_MANA;
	af.modifier = diff1;
	affect_to_char(ch, &af);

	check_improve(ch, gsn_transferance, TRUE, 2);

	ch->mana = ch->hit;
	ch->hit = diff2;

	return;
}

void
do_shapeshift(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;
	char buf[MAX_STRING_LENGTH];
	int chance;


	if (IS_NPC(ch))
		return;


	if (get_skill(ch, gsn_shapeshift) < 1)
	{
		Cprintf(ch, "You change into your own self!\n\r");
		return;
	}

	if (!str_cmp(argument, "none") || !str_cmp(argument, "self"))
	{
		affect_strip(ch, gsn_shapeshift);
		return;
	}

	if ((victim = get_char_room(ch, argument)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (!IS_NPC(victim))
	{
		Cprintf(ch, "You can't shapeshift into a player.\n\r");
		return;
	}

	chance = get_skill(ch, gsn_shapeshift);
	if (number_percent() > chance)
	{
		Cprintf(ch, "Your poor attempt at shape shifting fails miserably.\n\r");
		check_improve(ch, gsn_shapeshift, FALSE, 2);
		return;
	}

	free_string(ch->shift_short);
	free_string(ch->shift_long);
	free_string(ch->shift_name);

	sprintf(buf, "%s %sxxx", ch->name, victim->name);
	ch->shift_name = str_dup(buf);
	ch->shift_short = str_dup(victim->short_descr);
	ch->shift_long = str_dup(victim->long_descr);

	if (is_affected(ch, gsn_shapeshift))
	{
		Cprintf(ch, "You mutate from one shift to another!\n\r");
		return;
	}


	Cprintf(ch, "You assume a different form.\n\r");

	af.where = TO_AFFECTS;
	af.type = gsn_shapeshift;
	af.level = ch->level;
	af.duration = 20;
	af.modifier = 0;
	af.bitvector = 0;
	af.location = APPLY_NONE;
	affect_to_char(ch, &af);

	check_improve(ch, gsn_shapeshift, TRUE, 4);
	return;
}


void
do_retribution(CHAR_DATA * ch, char *argument)
{
	AFFECT_DATA af;
	int chance;

	if (IS_NPC(ch))
		return;

	if (ch->move < 10) {
		Cprintf(ch, "You are too tired to call upon justice.\n\r");
		return;
	}
	ch->move -= 10;

	if (get_skill(ch, gsn_retribution) < 1)
	{
		Cprintf(ch, "You have no sense of justice.\n\r");
		return;
	}

	chance = get_skill(ch, gsn_retribution);
	if (number_percent() > chance)
	{
		Cprintf(ch, "You fail in your attempt at justice.\n\r");
		check_improve(ch, gsn_retribution, FALSE, 4);
		return;
	}

	check_improve(ch, gsn_retribution, TRUE, 4);

	if (is_affected(ch, gsn_retribution)) {
		Cprintf(ch, "Your sense of justice returns to normal.\n\r");
		affect_strip(ch, gsn_retribution);
		return;
	}
	af.where = TO_AFFECTS;
	af.type = gsn_retribution;
	af.level = ch->level;
	af.duration = 5;
	af.modifier = 0;
	af.bitvector = 0;
	af.location = APPLY_NONE;
	affect_to_char(ch, &af);
	Cprintf(ch, "You feel the force of justice fill your veins.\n\r");
	return;
}


void
do_brew(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *terran, *dominian, *reagent[3], *obj;
    	int chance;
	int number_potions;
    	int i, j, k;

	struct potion_info {
		char *potion_name;
		int potion_vnum;
		char *reagent[3][2];
	};

	const struct potion_info brew_table[] = {
		{"dazzling", OBJ_VNUM_POT_A,
			{ {"toadstool", "cotton candy"},
			{"autumn leaf", "yellow sandstone"},
			{"bat wings", "dead leaf"} },
		},

		{"infuriating", OBJ_VNUM_POT_B,
			{ {"algae inviting", "bottle of rum"},
			{"defeathered chicken", "haggis"},
			{"powder astral", "rotted corpse bat"} },
		},

		{"illusionary", OBJ_VNUM_POT_C,
			{ {"powder black", "poison bundle dried herbs"},
			{"piece slime", "bran muffin"},
			{"rat tooth", "pile sunflower seeds"} },
		},

                {"invigorating", OBJ_VNUM_POT_D,
                        { {"pouch leather", "big bag sugar"},
                          {"robin egg", "dummy fluff"},
                          {"ichorous panacea potion", "bottle slime mold juice"},
                        },
		},
	};

	if (get_skill(ch, gsn_brew) < 1) {
		Cprintf(ch, "Learn how to cook first eh?\n\r");
		return;
	}

	if (argument[0] == '\0') {
		Cprintf(ch, "Brew which potion? Choices are 'dazzling' 'illusionary' 'infuriating' 'invigorating'\n\r");
		return;
	}

	if(ch->mana < 50) {
		Cprintf(ch, "You don't have enough mana to brew anything right now.\n\r");
		return;
	}

	chance = get_skill(ch, gsn_brew);
	number_potions = dice(1, 4);
	WAIT_STATE(ch, PULSE_VIOLENCE);

 	for(i=0; i<4; i++) {
		if( !str_prefix(argument, brew_table[i].potion_name) ) {
			/* check for 3 reagents */
			for(j=0; j<3; j++) {
				terran = get_obj_list(ch, brew_table[i].reagent[j][0], ch->carrying);
				dominian = get_obj_list(ch, brew_table[i].reagent[j][1], ch->carrying);
				if(terran == NULL && dominian == NULL) {
					Cprintf(ch, "You need %s or %s to brew the %s potion.\n\r",
					brew_table[i].reagent[j][0], brew_table[i].reagent[j][1],
					brew_table[i].potion_name);
					return;
				}
				if(terran)
					reagent[j] = terran;
				else if(dominian)
					reagent[j] = dominian;
			}
			/* If we make it this far, they have 3 reagents */
			extract_obj(reagent[0]);
			extract_obj(reagent[1]);
			extract_obj(reagent[2]);

	     		if (number_percent() > chance) {
				ch->mana -= 25;
				Cprintf(ch, "You fail to brew anything useful.\n\r");
				check_improve(ch, gsn_brew, FALSE, 2);
        	        	return;
        		}

			ch->mana -= 50;

			/* Okey, give them their potions now! */
			for (k=0; k<number_potions; k++) {
        			obj = create_object(get_obj_index(brew_table[i].potion_vnum), 0);
                		obj->value[0] = get_skill(ch, gsn_brew) / 2;
                		act("$n brews $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
                		act("You brew $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				obj->timer = number_range(1000, 1200);
                		obj_to_char(obj, ch);
                		check_improve(ch, gsn_brew, TRUE, 2);
        		}
			return;
		}
	}
	Cprintf(ch, "You don't know how to brew that kind of potion.\n\r");
	Cprintf(ch, "Choices are 'dazzling' 'illusionary' 'infuriating' 'invigorating'\n\r");
    return;
}

void
do_retreat(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	AFFECT_DATA *paf;
	int chance;

	if ((victim = ch->fighting) == NULL)
	{
		if (ch->position == POS_FIGHTING)
			ch->position = POS_STANDING;
		Cprintf(ch, "You aren't fighting anyone.\n\r");
		return;
	}

	if(ch->move < 30) {
		Cprintf(ch, "You don't have the stamina to retreat right now!\n\r");
		return;
	}

	ch->move -= 15;

	if (IS_AFFECTED(ch, AFF_TAUNT) || is_affected(ch, gsn_taunt))
	{
		paf = affect_find(ch->affected, gsn_taunt);
		chance = paf->level + 10;
		if (number_percent() < chance)
		{

			Cprintf(ch, "You are taunted and cannot leave the fight!\n\r");
			act("$n is taunted and cannot leave the fight!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			return;
		}
	}

	if (number_percent() > (get_skill(ch, gsn_retreat) * 3 / 4))
	{
		Cprintf(ch, "PANIC! You couldn't retreat!\n\r");
		return;
	}

	ch->move -= 15;

	Cprintf(ch, "You retreat from the fight, hoping the ruse worked!\n\r");
	act("$n has fled!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	stop_fighting(ch, TRUE);
	WAIT_STATE(ch, 6);
	check_improve(ch, gsn_retreat, TRUE, 1);

	return;
}

void
do_dirt(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;

	one_argument(argument, arg);

	if (get_skill(ch, gsn_dirt_kicking) < 1) {
		Cprintf(ch, "You get your feet dirty.\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "But you aren't in combat!\n\r");
			return;
		}
	}
	else if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_AFFECTED(victim, AFF_BLIND))
	{
		act("$E's already been blinded.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "Very funny.\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (IS_NPC(victim) &&
		victim->fighting != NULL &&
		!is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		act("But $N is such a good friend!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (IS_NPC(ch))
	{
		chance = (ch->level * 3 / 2) + 10;
	}

	// Base chance:
	chance = get_skill(ch, gsn_dirt_kicking);

	/* modifiers */
	if (ch->race == race_lookup("elf"))
		chance += 5;

	/* dexterity */
	chance += get_curr_stat(ch, STAT_DEX);
	chance -= 2 * get_curr_stat(victim, STAT_DEX);

	/* speed  */
	if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
		chance += 10;
	if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
		chance -= 20;

	/* level */
	chance += (ch->level - victim->level) * 2;

	/* terrain */
	switch (ch->in_room->sector_type)
	{
	case (SECT_INSIDE):
		chance -= 20;
		break;
	case (SECT_CITY):
		chance -= 10;
		break;
	case (SECT_FIELD):
		chance += 5;
		break;
	case (SECT_FOREST):
		break;
	case (SECT_HILLS):
		break;
	case (SECT_MOUNTAIN):
		chance -= 10;
		break;
	case (SECT_WATER_SWIM):
		chance = 0;
		break;
	case (SECT_SWAMP):
		chance = 0;
		break;
	case (SECT_WATER_NOSWIM):
		chance = 0;
		break;
	case (SECT_AIR):
		chance = 0;
		break;
	case (SECT_DESERT):
		chance += 10;
		break;
	}

	if (chance == 0)
	{
		Cprintf(ch, "There isn't any dirt to kick.\n\r");
		return;
	}

	/* now the attack */
	if (number_percent() < chance)
	{
		AFFECT_DATA af;

		act("$n is blinded by the dirt in $s eyes!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		act("$n kicks dirt in your eyes!", ch, NULL, victim, TO_VICT, POS_RESTING);
		Cprintf(victim, "You can't see a thing!\n\r");
		check_improve(ch, gsn_dirt_kicking, TRUE, 2);
		WAIT_STATE(ch, skill_table[gsn_dirt_kicking].beats);

		af.where = TO_AFFECTS;
		af.type = gsn_dirt_kicking;
		af.level = ch->level;
		af.duration = 0;
		af.location = APPLY_HITROLL;
		af.modifier = -4;
		af.bitvector = AFF_BLIND;

		affect_to_char(victim, &af);
		damage(ch, victim, number_range(2, 5), gsn_dirt_kicking, DAM_NONE, FALSE, TYPE_SKILL);
	}
	else
	{
		damage(ch, victim, 0, gsn_dirt_kicking, DAM_NONE, TRUE, TYPE_SKILL);
		check_improve(ch, gsn_dirt_kicking, FALSE, 2);
		WAIT_STATE(ch, skill_table[gsn_dirt_kicking].beats);
	}
	check_killer(ch, victim);
}

/* Helper function for do_throw */
int
reverse_direction(int dir)
{
	if (dir == DIR_NORTH)
		return DIR_SOUTH;
	if (dir == DIR_SOUTH)
		return DIR_NORTH;
	if (dir == DIR_EAST)
		return DIR_WEST;
	if (dir == DIR_WEST)
		return DIR_EAST;
	if (dir == DIR_UP);
		return DIR_DOWN;
	if (dir == DIR_DOWN);
		return DIR_UP;
	return -1;
}

/* Throw revamped by StarCrossed */
void
do_throw(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim = NULL, *old_fighting = NULL;
	OBJ_DATA *obj, *obj_next;
	char buf[256];
	int door;
	int chance, distance = 0;

	if (get_skill(ch, gsn_throw) < 1)
	{
		Cprintf(ch, "You throw like a ninny! give it up before someone sees you.\n\r");
		return;
	}

	one_argument(argument, arg);

	if (arg[0] == '\0' && ch->fighting == NULL)
	{
		Cprintf(ch, "Throw on whom or what?\n\r");
		return;
	}

	if ((obj = get_eq_char(ch, WEAR_HOLD)) == NULL)
	{
		Cprintf(ch, "You hold nothing in your hand.\n\r");
		return;
	}

	if (obj->item_type != ITEM_THROWING)
	{
		Cprintf(ch, "You can throw only a throwing weapon.\n\r");
		return;
	}

	if (ch->fighting != NULL)
		victim = ch->fighting;
	else
		/* finds victim within x rooms. Stores direction
		   in 'door' and distance by reference. */
		/* does not work across areas */
		victim = range_finder(ch, arg, 1, &door, &distance, FALSE);

	if (victim == NULL)
	{
		Cprintf(ch, "You can't find them.\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	/* make sure that you can't use throw to break charm */
	if (ch->master == victim)
	{
		Cprintf(ch, "And hurt your beloved master?!\n\r");
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	chance = get_skill(ch, gsn_throw) - 10 * distance;
	old_fighting = victim->fighting;

	if (number_percent() > chance)
	{
		Cprintf(ch, "You fumble your throw.\n\r");
		check_improve(ch, gsn_throw, FALSE, 2);
		return;
	}

	if (!IS_NPC(victim) && ch != victim) {
		ch->no_quit_timer = 3;
		victim->no_quit_timer = 3;
	}

	// set up message in advance
	sprintf(buf, "You have no more of %s.\n\r", obj->short_descr);

	act("$n throws $p on $N.", ch, obj, victim, TO_NOTVICT, POS_RESTING);
	act("You throw $p on $N.", ch, obj, victim, TO_CHAR, POS_RESTING);
	act("$n throws $p on you.", ch, obj, victim, TO_VICT, POS_RESTING);

	check_killer(ch, victim);

	weapon_hit(ch, victim, obj, gsn_throw);

	if (obj->value[3] != 0 && obj->value[4] != 0 && victim->position > POS_DEAD)
	{
		obj_cast_spell(obj->value[4], obj->value[3], ch, victim, obj);
		if (distance > 0 && old_fighting == NULL)
		{
			stop_fighting(victim, TRUE);
		}
		check_improve(ch, gsn_throw, TRUE, 2);
	}

	extract_obj(obj);

	/* reload same item vnum if they have it */
	for (obj_next = ch->carrying; obj_next != NULL;
		 obj_next = obj_next->next_content)
	{
		if (obj_next->pIndexData->vnum == obj->pIndexData->vnum)
			break;
	}

	if (obj_next == NULL)
		Cprintf(ch, "%s", buf);
	else
		wear_obj(ch, obj_next, TRUE);
	door = reverse_direction(door);

	/* Don't tell them if they're surprised or same room */
	if (!can_see(victim, ch) || distance == 0)
	{
		return;
	}

	switch (door)
	{
	case 0:
		Cprintf(victim, "The throw came from NORTH!\n\r");
		break;
	case 1:
		Cprintf(victim, "The throw came from EAST!\n\r");
		break;
	case 2:
		Cprintf(victim, "The throw came from SOUTH!\n\r");
		break;
	case 3:
		Cprintf(victim, "The throw came from WEST!\n\r");
		break;
	case 4:
		Cprintf(victim, "The throw came from UP!\n\r");
		break;
	case 5:
		Cprintf(victim, "The throw came from DOWN!\n\r");
		break;
	default:
		Cprintf(victim, "Throw error: bad direction\n\r");
		break;
	}
	return;
}

/* Hurl: Man, what a cut and paste of throw for Zealots. */
void
do_hurl(CHAR_DATA * ch, char *argument)
{
        char arg[MAX_INPUT_LENGTH];
        CHAR_DATA *victim = NULL, *old_fighting = NULL;
        OBJ_DATA *obj, *obj_next;
        char buf[256];
        int door;
        int chance, distance = 0;

        if (get_skill(ch, gsn_hurl) < 1)
        {
                Cprintf(ch, "Hurl? Do you mean throw something or lose your lunch?\n\r");
                return;
        }

        one_argument(argument, arg);

        if ((obj = get_eq_char(ch, WEAR_HOLD)) == NULL)
        {
                Cprintf(ch, "You hold nothing in your hand.\n\r");
                return;
        }

        if (arg[0] == '\0' && ch->fighting == NULL)
        {
                Cprintf(ch, "Hurl it at whom or what?\n\r");
                return;
        }

        if (obj->item_type != ITEM_THROWING)
        {
                Cprintf(ch, "You can throw only a throwing weapon.\n\r");
                return;
        }

        if (ch->fighting != NULL)
                victim = ch->fighting;
        else
                /* finds victim within x rooms. Stores direction
                   in 'door' and distance by reference. */
                /* does not work across areas */
                victim = range_finder(ch, arg, 2, &door, &distance, FALSE);
        if (victim == NULL)
        {
                Cprintf(ch, "You can't find them.\n\r");
                return;
        }

        if (is_safe(ch, victim))
                return;

        /* make sure that you can't use throw to break charm */
        if (ch->master == victim)
        {
                Cprintf(ch, "And hurt your beloved master?!\n\r");
                return;
        }

        WAIT_STATE(ch, PULSE_VIOLENCE);

        chance = get_skill(ch, gsn_hurl) - 10 * distance;
        old_fighting = victim->fighting;

        if (number_percent() > chance)
        {
                Cprintf(ch, "You fail to hurl anything at them.\n\r");
                check_improve(ch, gsn_hurl, FALSE, 2);
                return;
        }

        if (!IS_NPC(victim)
        && ch != victim) {
                ch->no_quit_timer = 3;
                victim->no_quit_timer = 3;
        }

        // set up message in advance
        sprintf(buf, "You have no more of %s.\n\r", obj->short_descr);

        act("$n hurls $p at $N!", ch, obj, victim, TO_NOTVICT, POS_RESTING);
        act("You hurl $p at $N!", ch, obj, victim, TO_CHAR, POS_RESTING);
        act("$n hurls $p straight at you!", ch, obj, victim, TO_VICT, POS_RESTING);

        check_killer(ch, victim);
        weapon_hit(ch, victim, obj, gsn_hurl);

        if (obj->value[3] != 0
                && obj->value[4] != 0
                && victim->position > POS_DEAD)
        {
                obj_cast_spell(obj->value[4], obj->value[3], ch, victim, obj);
                if (distance > 0 && old_fighting == NULL)
                {
                        stop_fighting(victim, TRUE);
                }
                check_improve(ch, gsn_hurl, TRUE, 2);
        }

        extract_obj(obj);

        /* reload same item vnum if they have it */
        for (obj_next = ch->carrying; obj_next != NULL;
                 obj_next = obj_next->next_content)
        {
                if (obj_next->pIndexData->vnum == obj->pIndexData->vnum)
                        break;
        }
        if (obj_next == NULL)
                Cprintf(ch, "%s", buf);
        else
                wear_obj(ch, obj_next, TRUE);
        door = reverse_direction(door);

        /* Don't tell them if they're surprised or same room */
        if (!can_see(victim, ch) || distance == 0)
        {
                return;
        }

        switch (door)
        {
        case 0:
                Cprintf(victim, "The projectile came from NORTH!\n\r");
                break;
        case 1:
                Cprintf(victim, "The projectile came from EAST!\n\r");
                break;
        case 2:
                Cprintf(victim, "The projectile came from SOUTH!\n\r");
                break;
        case 3:
                Cprintf(victim, "The projectile came from WEST!\n\r");
                break;
        case 4:
                Cprintf(victim, "The projectile came from UP!\n\r");
                break;
        case 5:
                Cprintf(victim, "The projectile came from DOWN!\n\r");
                break;
        default:
                Cprintf(victim, "Throw error: bad direction\n\r");
                break;
        }
        return;
}


void
do_trip(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;

	one_argument(argument, arg);

	if ((chance = get_skill(ch, gsn_trip)) < 1
		|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_TRIP))
		|| (!IS_NPC(ch)
			&& ch->level < skill_table[gsn_trip].skill_level[ch->charClass]))
	{
		Cprintf(ch, "Tripping?  What's that?\n\r");
		return;
	}


	if (arg[0] == '\0')
	{
		victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "But you aren't fighting anyone!\n\r");
			return;
		}
	}

	else if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	// Harder to trip people who are flying
	if (IS_AFFECTED(victim, AFF_FLYING))
	{
		chance -= 10;

		if(!IS_AFFECTED(ch, AFF_FLYING)) {
			act("$S feet aren't on the ground.", ch, NULL, victim, TO_CHAR, POS_RESTING);
			return;
		}
	}
	else if (IS_AFFECTED(ch, AFF_FLYING))
		chance += 10;

	if (victim->position < POS_FIGHTING)
	{
		act("$N is already down.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You fall flat on your face!\n\r");
		WAIT_STATE(ch, PULSE_VIOLENCE);
		act("$n trips over $s own feet!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		victim->position = POS_RESTING;
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	/* modifiers */

	/* dex */
	chance += get_curr_stat(ch, STAT_DEX);
	chance -= get_curr_stat(victim, STAT_DEX) * 3 / 2;

	/* race */
	if(ch->race == race_lookup("kirre"))
		chance += 10;

	/* speed */
	if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
		chance += 10;
	if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
		chance -= 20;

	/* level */
	chance += (ch->level - victim->level) * 2;

	/* tumbling reclass skill */
	if (check_tumbling(ch, victim))
	{
		damage(ch, victim, 0, gsn_trip, DAM_BASH, TRUE, TYPE_SKILL);
		check_improve(ch, gsn_trip, FALSE, 1);
		WAIT_STATE(ch, skill_table[gsn_trip].beats);
		return;
	}

	/* now the attack */
	if (number_percent() < chance)
	{
		act("$n trips you and you go down!", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("You trip $N and $N goes down!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		act("$n trips $N, sending $M to the ground.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		check_improve(ch, gsn_trip, TRUE, 1);

		/* Tripping self doesn't stun you as much */
		if(ch != victim)
		{
			DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
		}

		WAIT_STATE(ch, skill_table[gsn_trip].beats);
		victim->position = POS_RESTING;
		damage(ch, victim, number_range(2, 5 * victim->size + 2), gsn_trip, DAM_BASH, TRUE, TYPE_SKILL);
	}
	else
	{
		damage(ch, victim, 0, gsn_trip, DAM_BASH, TRUE, TYPE_SKILL);
		WAIT_STATE(ch, skill_table[gsn_trip].beats * 2 / 3);
		check_improve(ch, gsn_trip, FALSE, 1);
	}
	check_killer(ch, victim);
}

void
do_kill(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	/*AFFECT_DATA af; */

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Kill whom?\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}
	if (!IS_NPC(victim))
	{
		do_murder(ch, argument);
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You hit yourself.  Ouch!\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (victim->fighting != NULL && !is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (ch->position == POS_FIGHTING)
	{
		Cprintf(ch, "You do the best you can!\n\r");
		return;
	}

	/* NO quit code */

	WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
	check_killer(ch, victim);
	multi_hit(ch, victim, TYPE_UNDEFINED);
	return;
}



void
do_murde(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "If you want to MURDER, spell it out.\n\r");
	return;
}



void
do_murder(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim, *vch;

	/*AFFECT_DATA af; */

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Murder whom?\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "Suicide is a mortal sin.\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	/* try and fix charm killer bug with order */
	vch = ch;
	while (IS_AFFECTED(vch, AFF_CHARM) && vch != NULL)
		vch = vch->master;

	if (ch->position == POS_FIGHTING)
	{
		Cprintf(ch, "You do the best you can!\n\r");
		return;
	}


	WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
	if (IS_NPC(ch))
		sprintf(buf, "Help! I am being attacked by %s!", ch->short_descr);
	else
		sprintf(buf, "Help!  I am being attacked by %s!", ch->name);
	do_yell(victim, buf);
	check_killer(vch, victim);
	multi_hit(ch, victim, TYPE_UNDEFINED);
	return;
}

void
do_block(CHAR_DATA * ch, char *argument)
{
	int dir;
	int chance;
	AFFECT_DATA af;

	if (get_skill(ch, gsn_block) < 1)
	{
		Cprintf(ch, "You can't block anybody's way.");
		return;
	}

	if (is_affected(ch, gsn_block))
	{
		Cprintf(ch, "You are already blocking an exit!\n\r");
		return;
	}

	if (ch->position < POS_STANDING)
	{
		Cprintf(ch, "You can't do that sitting down.\n\r");
		return;
	}


	if (argument[0] == '\0')
	{
		Cprintf(ch, "Block which direction?\n\r");
		return;
	}

	if (!str_prefix(argument, "north"))
		dir = DIR_NORTH;
	else if (!str_prefix(argument, "south"))
		dir = DIR_SOUTH;
	else if (!str_prefix(argument, "east"))
		dir = DIR_EAST;
	else if (!str_prefix(argument, "west"))
		dir = DIR_WEST;
	else if (!str_prefix(argument, "up"))
		dir = DIR_UP;
	else if (!str_prefix(argument, "down"))
		dir = DIR_DOWN;
	else
	{
		Cprintf(ch, "There are no such direction in this game.\n\r");
		return;
	}

	if (ch->in_room->exit[dir] == NULL)
	{
		Cprintf(ch, "There is no exit in that direction.\n\r");
		return;
	}

	if(ch->move < 50)
	{
		Cprintf(ch, "You don't have enough energy to stop anyone.\n\r");
		return;
	}
	ch->move -= 50;

	chance = get_skill(ch, gsn_block);
	if (number_percent() > chance)
	{
		Cprintf(ch, "You fail in your attempt to block an exit.\n\r");
		check_improve(ch, gsn_block, FALSE, 2);
		return;
	}

	Cprintf(ch, "You cover the exit.\n\r");
	act("$n sets up for a block.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	check_improve(ch, gsn_block, TRUE, 2);

	af.type = gsn_block;
	af.level = ch->level;
	af.where = TO_AFFECTS;
	af.location = APPLY_NONE;
	af.modifier = dir;
	af.duration = 5;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	return;
}


void
do_backstab(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *weapon;
	int victim_dead = FALSE;
	AFFECT_DATA *paf = NULL, af;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "But you aren't fighting anyone!\n\r");
			return;
		}
	}

	else if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "How can you sneak up on yourself?\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		Cprintf(ch, "How dare you rebel against your Master!\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (IS_NPC(victim) &&
		victim->fighting != NULL &&
		!is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	if (get_eq_char(ch, WEAR_WIELD) == NULL
		&& get_eq_char(ch, WEAR_DUAL) == NULL)
	{
		Cprintf(ch, "You need to wield a weapon to backstab.\n\r");
		return;
	}

	if (victim->hit < MAX_HP(victim) / 2)
	{
		act("$N is hurt and suspicious ... you can't sneak up.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	check_killer(ch, victim);
	WAIT_STATE(ch, skill_table[gsn_backstab].beats);
	if (number_percent() < get_skill(ch, gsn_backstab)
		|| (get_skill(ch, gsn_backstab) >= 2 && !IS_AWAKE(victim)))
	{
		check_improve(ch, gsn_backstab, TRUE, 1);


		// Kind of a kludge to get mirror image against backstab again
		if (is_affected(victim, gsn_mirror_image))
		{
			paf = affect_find(victim->affected, gsn_mirror_image);
			act("Your mirror image takes $N's hit!", victim, NULL, ch, TO_CHAR, POS_RESTING);
			act("$n's mirror image absorbs the shock.", victim, NULL, NULL,TO_ROOM, POS_RESTING);
			if (number_percent() < 70)
			{
				Cprintf(victim, "Your mirror image shatters to pieces!\n\r");
				act("$n's mirror image shatters to pieces!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
				affect_remove(victim, paf);
			}
			if (ch->fighting == NULL)
				set_fighting(ch, victim);
			if (victim->fighting == NULL)
				set_fighting(victim, ch);
			return;
		}

		// Different weapons based on handedness:
		if ((weapon = get_eq_char(ch, WEAR_WIELD)) != NULL)
			victim_dead = weapon_hit(ch, victim, weapon, gsn_backstab);
		else if ((weapon = get_eq_char(ch, WEAR_DUAL)) != NULL)
			victim_dead = weapon_hit(ch, victim, weapon, gsn_backstab);

		if (victim_dead)
			return;

		// Rogue crippling strike
		if(ch->reclass == reclass_lookup("rogue")) {

			// Max out at -10 DR
			paf = affect_find(victim->affected, gsn_backstab);
			if(paf
			&& paf->modifier < -10)
				return;

			af.where = TO_AFFECTS;
			af.type = gsn_backstab;
			af.level = ch->level;
			af.duration = 2;
			af.modifier = -2;
			af.bitvector = 0;
			af.location = APPLY_DAMAGE_REDUCE;

			affect_join(victim, &af);

			Cprintf(victim, "You wince in pain from the crippling strike!\n\r");
			act("$n winces in pain from the crippling strike!", victim, NULL, NULL, TO_ROOM, POS_RESTING);

			return;

		}

		// Try second backstab.
		if ((IS_AFFECTED(ch, AFF_HASTE) || is_affected(ch, gsn_haste))) {
			if ((weapon = get_eq_char(ch, WEAR_WIELD)) != NULL)
				victim_dead = weapon_hit(ch, victim, weapon, gsn_backstab);
			else if ((weapon = get_eq_char(ch, WEAR_DUAL)) != NULL)
				victim_dead = weapon_hit(ch, victim, weapon, gsn_backstab);
		}
	}
	else
	{
		check_improve(ch, gsn_backstab, FALSE, 1);
		damage(ch, victim, 0, gsn_backstab, DAM_NONE, TRUE, TYPE_SKILL);
	}

	return;
}



void
do_flee(CHAR_DATA * ch, char *argument)
{
	ROOM_INDEX_DATA *was_in;
	ROOM_INDEX_DATA *now_in;
	CHAR_DATA *victim;
	int attempt, chance;
	AFFECT_DATA *paf;

	if ((victim = ch->fighting) == NULL)
	{
		if (ch->position == POS_FIGHTING)
			ch->position = POS_STANDING;
		Cprintf(ch, "You aren't fighting anyone.\n\r");
		return;
	}


	if (IS_AFFECTED(ch, AFF_TAUNT) || is_affected(ch, gsn_taunt))
	{
		paf = affect_find(ch->affected, gsn_taunt);
		if (paf != NULL)
			chance = paf->level + 10;
		else
			chance = 0;
		if (number_percent() < chance)
		{
			Cprintf(ch, "You are taunted and cannot leave the fight!\n\r");
			act("$n is taunted and cannot leave the fight!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			return;
		}
	}

	if(!IS_NPC(ch)
	&& ch->reclass == reclass_lookup("zealot")
	&& number_percent() < 50 - (ch->hit * 50 / MAX_HP(ch))) {
		Cprintf(ch, "In your zeal for blood you refuse to run away.\n\r");
		return;
	}

	if (room_is_affected(ch->in_room, gsn_earth_to_mud))
	{
		if (number_percent() < 40)
		{
			Cprintf(ch, "You attempt to flee and get stuck in the mud!\n\r");
			return;
		}
	}

	was_in = ch->in_room;
	for (attempt = 0; attempt < 6; attempt++)
	{
		EXIT_DATA *pexit;
		int door;

		door = number_door();
		if ((pexit = was_in->exit[door]) == 0
			|| pexit->u1.to_room == NULL
			|| (IS_SET(pexit->exit_info, EX_CLOSED) && !is_affected(ch, gsn_pass_door))
			|| number_range(0, ch->daze) != 0
			|| (IS_NPC(ch)
				&& IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)))
			continue;


		act("$n has fled!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		move_char(ch, door, FALSE);
		// New small delay to prevent bugs!
		WAIT_STATE(ch, PULSE_VIOLENCE / 2);
		WAIT_STATE(victim, PULSE_VIOLENCE / 2);

		if ((now_in = ch->in_room) == was_in)
			continue;

		if (!IS_NPC(ch))
		{
			Cprintf(ch, "You flee from combat!\n\r");
			ch->charge_wait = 5;
			if ((IS_AFFECTED(ch, AFF_SNEAK)))
				Cprintf(ch, "You snuck away safely.\n\r");
			else
			{
				Cprintf(ch, "You lost 10 exp.\n\r");
				gain_exp(ch, -10);
			}
		}

		if (IS_SET(ch->toggles, TOGGLES_SOUND))
			Cprintf(ch, "!!SOUND(sounds/wav/flee*.wav V=80 P=20 T=admin)");

		stop_fighting(ch, TRUE);

		return;
	}

	Cprintf(ch, "PANIC! You couldn't escape!\n\r");
	return;
}



void
do_rescue(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *fch;
	int chance;
	int base;
	bool found = FALSE, success = FALSE;

	one_argument(argument, arg);

	if ((base = get_skill(ch, gsn_rescue)) < 1)
	{
		Cprintf(ch, "Rescuing? What's that?\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Rescue whom?\n\r");
		return;
	}
	else if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "What about fleeing instead?\n\r");
		return;
	}

	if (IS_NPC(victim) && !is_same_group(ch, victim))
	{
		Cprintf(ch, "Doesn't need your help!\n\r");
		return;
	}

	if (ch->fighting == victim)
	{
		Cprintf(ch, "Too late.\n\r");
		return;
	}

	if (victim->fighting == NULL)
	{
		Cprintf(ch, "That person is not fighting right now.\n\r");
		return;
	}

	for (fch = ch->in_room->people; fch != NULL; fch = fch->next_in_room)
	{
		if (fch->fighting != victim)
		{
			continue;
		}

		/* allow, if your in the same group OR ( same clan AND !safe ) */
		/* But, reverse the logic BLARGH!!! a goto would be cleaner! */
		if (is_safe(ch, fch))
		{
			continue;
		}

		if (IS_NPC(fch) &&
			fch->fighting != NULL &&
			!is_same_group(ch, fch->fighting))
		{
			continue;
		}

		if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == fch)
		{
			continue;
		}

		found = TRUE;
		chance = base;
		/* race */
		if (ch->race == race_lookup("giant"))
		{
			chance += 20;
		}

		/* dex */
		chance += get_curr_stat(ch, STAT_DEX);
		chance -= 2 * get_curr_stat(fch, STAT_DEX);

		/* speed  */
		if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
			chance += 10;
		if (IS_SET(fch->off_flags, OFF_FAST) || IS_AFFECTED(fch, AFF_HASTE))
			chance -= 25;

		/* level */
		chance += (ch->level - fch->level) * 2;

		if (number_percent() > chance)
		{
			continue;
		}

		success = TRUE;
		stop_fighting(fch, FALSE);
		check_killer(ch, fch);
		if (ch->fighting == NULL)
		{
			set_fighting(ch, fch);
		}
		set_fighting(fch, ch);
	}

	if (!found)
	{
		Cprintf(ch, "Doesn't need your help!\n\r");
		return;
	}

	WAIT_STATE(ch, skill_table[gsn_rescue].beats);

	if (!success)
	{
		Cprintf(ch, "You fail the rescue.\n\r");
		check_improve(ch, gsn_rescue, FALSE, 1);
		return;
	}

	act("You rescue $N!", ch, NULL, victim, TO_CHAR, POS_RESTING);
	act("$n rescues you!", ch, NULL, victim, TO_VICT, POS_RESTING);
	act("$n rescues $N!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	check_improve(ch, gsn_rescue, TRUE, 1);

	return;
}


void
do_substitution(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;

	one_argument(argument, arg);

	if ((chance = get_skill(ch, gsn_substitution)) < 1)
	{
		Cprintf(ch, "Substitution? What's that?\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Kill whom?\n\r");
		return;
	}
	else if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "How can you substitute yourself for yourself?\n\r");
		return;
	}

	if (victim == ch->fighting)
	{
		Cprintf(ch, "You're already fighting them!\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (IS_NPC(victim) &&
		victim->fighting != NULL &&
		!is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	check_killer(ch, victim);

	WAIT_STATE(ch, skill_table[gsn_substitution].beats);

	/* dex */
	chance += get_curr_stat(ch, STAT_DEX);
	chance -= 2 * get_curr_stat(victim, STAT_DEX);

	/* speed  */
	if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
		chance += 10;
	if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
		chance -= 25;

	/* level */
	chance += (ch->level - victim->level) * 2;

	if (number_percent() < chance)
	{
		Cprintf(ch, "You switch foes!\n\r");
		Cprintf(victim, "The focus of the fight is now on you!\n\r");
		check_improve(ch, gsn_substitution, TRUE, 5);
		ch->fighting = victim;
	}
	else
	{
		Cprintf(ch, "You are too tangled up to switch foes.\n\r");
		check_improve(ch, gsn_substitution, FALSE, 5);
		return;
	}

	return;
}


void
do_bite(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	AFFECT_DATA af;
	int dam;
	int skill;
	int level;

	one_argument(argument, arg);

	level = ch->level;

	if (!IS_NPC(ch) && get_skill(ch, gsn_bite) < 1)
	{
		Cprintf(ch, "You don't have the teeth for that kind of work.\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "You are not fighting anyone.\n\r");
			return;
		}
	}
	else
	{
	 	if ((victim = get_char_room(ch,arg)) == NULL)
		{
			Cprintf(ch, "They're not here.\n\r");
			return;
		}
	}

	if (victim == ch)
	{
		Cprintf(ch, "You bite yourself. Mm, yummy :)\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	WAIT_STATE(ch, skill_table[gsn_bite].beats);
	skill = get_skill(ch, gsn_bite);
	if (skill > number_percent())
	{
		dam = dice(2, ch->level);
		damage(ch, victim, dam, gsn_bite, DAM_DISEASE, TRUE, TYPE_SKILL);
		if (!saving_throw(ch, victim, gsn_plague, level, SAVE_EASY, STAT_CON, DAM_DISEASE))
		{
			af.where = TO_AFFECTS;
			af.type = gsn_plague;
			af.level = level / 2;
			af.duration = level / 4;
			af.location = APPLY_STR;
			af.modifier = 0 - level / 10;
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
		check_improve(ch, gsn_bite, TRUE, 1);
	}
	else
	{
		damage(ch, victim, 0, gsn_bite, DAM_DISEASE, TRUE, TYPE_SKILL);
		check_improve(ch, gsn_bite, FALSE, 1);
	}
	check_killer(ch, victim);
	return;
}

void
do_kick(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int dam;
	int skill;
	int dice_type= 0;
	int dice_count = 0;
	one_argument(argument, arg);

	if (!IS_NPC(ch) && (ch->level < skill_table[gsn_kick].skill_level[ch->charClass] || get_skill(ch, gsn_kick) < 1))
	{
		Cprintf(ch, "You better leave the martial arts to fighters.\n\r");
		return;
	}

	if (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_KICK))
		return;

	if (arg[0] == '\0')
	{
		victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "You are not fighting anyone.\n\r");
			return;
		}
	}
	else
	{
	 	if ((victim = get_char_room(ch,arg)) == NULL)
		{
			Cprintf(ch, "They're not here.\n\r");
			return;
		}
	}

	if (is_safe(ch, victim))
		return;

	if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You kick yourself in the ankle. OW!\n\r");
		WAIT_STATE(ch, skill_table[gsn_kick].beats);
		act("$n kicks $mself in the ankle!",ch,NULL,NULL,TO_ROOM,POS_RESTING);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if(ch->reclass == reclass_lookup("hermit")) {
		double_kick(ch, victim);
		return;
	}

	check_killer(ch, victim);
	WAIT_STATE(ch, skill_table[gsn_kick].beats);
	skill = get_skill(ch, gsn_kick);

	if (skill > number_percent())
	{
		// Elf: 10d9 (50)
		// Human: 10d10 (55)
		// Sliver: 10d11 (60)
		// Dragon: 10d12 (65)
		// Giant:  10d13 (70)
		dice_count = (ch->level / 5);
		dice_type  = (get_curr_stat(ch, STAT_STR) / 3) + ch->size + 1;
		dam = dice(dice_count, dice_type);
		damage(ch, victim, dam, gsn_kick, DAM_BASH, TRUE, TYPE_SKILL);
		check_improve(ch, gsn_kick, TRUE, 1);
	}
	else
	{
		damage(ch, victim, 0, gsn_kick, DAM_BASH, TRUE, TYPE_SKILL);
		check_improve(ch, gsn_kick, FALSE, 1);
	}

	return;
}

void
double_kick(CHAR_DATA *ch, CHAR_DATA *victim) {
	int count=1, chance, dam;
	int victim_dead = FALSE;
	int dice_type = 0;
	int dice_count = 0;

	WAIT_STATE(ch, skill_table[gsn_kick].beats);
	chance = get_skill(ch, gsn_kick);

	/* regular single kick */
	if (chance > number_percent())
    	{
		dice_count = (ch->level / 5);
		dice_type  = (get_curr_stat(ch, STAT_STR) / 3) + ch->size + 1;
		dam = dice(dice_count, dice_type);

		victim_dead = damage(ch, victim, dam, gsn_kick, DAM_NONE, TRUE, TYPE_SKILL);
		check_improve(ch, gsn_kick, TRUE, 1);
	}
	else
	{
		victim_dead = damage(ch, victim, 0, gsn_kick, DAM_BASH, TRUE, TYPE_SKILL);
		check_improve(ch, gsn_kick, FALSE, 1);
	}
	check_killer(ch, victim);

	if (victim_dead)
		return;

	/* some extra kicks */
	while(1) {
		if(count >= 3) {
			break;
		}

		if(victim_dead)
			return;

		chance = chance / 2;
		if(number_percent() < chance) {
			count++;
			Cprintf(ch, "With great agility you land another kick!\n\r");
			dam = dice(dice_count, dice_type) * (float)(chance / 100.0);
			victim_dead = damage(ch, victim, dam, gsn_kick, DAM_BASH, TRUE, TYPE_SKILL);
			check_improve(ch, gsn_kick, TRUE, 1);
		}
		else
			break;
	}
	return;
}

void
do_bounty(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int amount;
	DESCRIPTOR_DATA *d;
	bool found;

	found = FALSE;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);


	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Syntax: bounty <player> <gold>\n\rSyntax: bounty list\n\rSyntax: bounty <on/off>\n\r");
		return;
	}

	if (!str_prefix(arg1, "off"))
	{
		SET_BIT(ch->toggles, TOGGLES_NOBOUNTY);
		Cprintf(ch, "You will no longer see when bounties are placed.\n\r");
		return;
	}
	else if(!str_prefix(arg1, "on"))
	{
		REMOVE_BIT(ch->toggles, TOGGLES_NOBOUNTY);
		Cprintf(ch, "You will now see when bounties are placed.\n\r");
		return;
	}
	else if (!str_prefix(arg1, "list"))
	{

		Cprintf(ch, "   Bounty      Clan      Player\n\r");
		Cprintf(ch, "   ------      ----      ------\n\r");

		for (d = descriptor_list; d != NULL; d = d->next)
		{
			if (d->character != NULL)
			{
				if (d->character->bounty > 0 && !IS_IMMORTAL(d->character)
				    && can_see(ch, d->character) && is_clan(d->character))
				{
					found = TRUE;
					Cprintf(ch, "%5d gold  %10s  %-16s\n\r", d->character->bounty, clan_table[d->character->clan].who_name, d->character->name);
				}
			}
		}

		if (found == FALSE)
			Cprintf(ch, "No bounty found for any player online.\n\r");

		return;
	}


	if (arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: bounty <player> <gold>\n\rSyntax: bounty list\n\r");
		return;
	}

	if (!is_number(arg2))
	{
		Cprintf(ch, "Syntax: bounty <player> <gold>\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL || !can_see(ch, victim))
	{
		Cprintf(ch, "There is no such player online.\n\r");
		return;
	}

	if (ch->reclass == reclass_lookup("bounty hunter"))
	{
		Cprintf(ch, "You collect bounties for a living, not place them.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "You can't put a price on your own head!\n\r");
		return;
	}

	if (IS_IMMORTAL(ch)) {
		Cprintf(ch, "Immortals are not allowed to place bounties.\n\r");
		return;
	}

	if (IS_IMMORTAL(victim))
	{
		Cprintf(ch, "You can't put a price on an immortal's head!\n\r");
		return;
	}

	if (IS_NPC(ch))
		return;

	if (IS_NPC(victim))
	{
		Cprintf(ch, "You can't place a bounty on a mobile.\n\r");
		return;
	}

	if ((!is_clan(ch) || !is_clan(victim)) && !IS_IMMORTAL(ch))
	{
		Cprintf(ch, "Stay out of clan affairs.\n\r");
		return;
	}


	amount = atoi(arg2);

	if (ch->gold < amount || amount < 0)
	{
		Cprintf(ch, "You can't offer more money than you have.\n\r");
		return;
	}

	if(amount < victim->level)
	{
		Cprintf(ch, "You can place a minimum bounty of %d on %s.\n\r",
			victim->level, victim->name);
		return;
	}

	/* Ok, lets do it. */

	ch->gold = ch->gold - amount;
	victim->bounty = victim->bounty + amount;
	Cprintf(ch, "You offer %d gold for %s's head, raising the bounty to %d gold.\n\r", amount, victim->name, victim->bounty);

	if(!IS_SET(ch->toggles, TOGGLES_NOBOUNTY))
		Cprintf(victim, "%s places %d gold on your head, raising the bounty to %d gold.\n\r", ch->name, amount, victim->bounty);

	for (d = descriptor_list; d != NULL; d = d->next)
	{
		if (d->character != NULL)
		{
			if (is_clan(d->character)
				&& !IS_NPC(d->character)
				&& (d->character != ch)
				&& (d->character != victim)
				&& !IS_SET(d->character->toggles, TOGGLES_NOBOUNTY))
			{
				Cprintf(d->character, "%s just placed %d gold on %s's head, raising the bounty to %d gold.\n\r", ch->name, amount, victim->name, victim->bounty);
			}
		}
	}

}

void
do_bribe(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *mob;
	int chance, cost;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (!IS_NPC(ch) && get_skill(ch, gsn_bribe) < 1)
	{
		Cprintf(ch, "You couldn't even bribe a politician.\n\r");
		return;
	}

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Syntax: bribe <mobile> <target>\n\r");
		return;
	}

	if ((mob = get_char_room(ch, arg1)) == NULL)
	{
		Cprintf(ch, "You don't see them here to bribe.\n\r");
		return;
	}

	if (!IS_NPC(mob))
	{
		Cprintf(ch, "You can only bribe mobiles.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg2, TRUE)) == NULL)
	{
		Cprintf(ch, "That's not a valid target.\n\r");
		return;
	}

	if(is_safe(ch, victim)) {
		Cprintf(ch, "Yeah, right. They're safe from your intrigues. For now...\n\r");
		return;
	}

	if(!IS_NPC(victim) && mob->level > victim->level + 5) {
		Cprintf(ch, "That's not very fair to your victim.\n\r");
		return;
	}

	if (mob == victim)
	{
		Cprintf(ch, "Very funny. Not.\n\r");
		return;
	}

		cost = UMAX(1, mob->level * 4 / 3);

        if (ch->gold < cost)
        {
                Cprintf(ch, "They want more money than you have.\n\r");
                return;
        }
        else
                ch->gold -= cost;


		if (IS_AFFECTED(mob, AFF_CHARM)
        || IS_AFFECTED(ch, AFF_CHARM)
        || mob->level > ch->level + 5
        || IS_SET(mob->imm_flags, IMM_CHARM))
        {
				chance = (2 * ch->level) - mob->level;
                if (chance > number_percent())
                   	Cprintf(ch, "You can't bribe them, but they take your money anyway.\n\r");
                else
                {
                	Cprintf(ch, "You can't bribe them, but they return your money.\n\r");
                        ch->gold = ch->gold + cost;
                }
      			return;
        }

		chance = get_skill(ch, gsn_bribe) / 2;
		if (number_percent() < chance)
        {
			chance = (2 * ch->level) - mob->level;
			if (chance > number_percent())
				Cprintf(ch, "You fail to bribe them, they pocket your offer.\n\r");
            else
            {
				Cprintf(ch, "You fail to bribe them, but they return your money.\n\r");
                ch->gold = ch->gold + cost;
            }
            check_improve(ch, gsn_bribe, FALSE, 1);
            return;
        }

        check_improve(ch, gsn_bribe, TRUE, 1);

        mob->hunting = victim;
        mob->hunt_timer = 30;
        Cprintf(ch, "%s lashes out: 'My wrath will befall upon %s! I swear it!'\n\r", mob->short_descr, victim->name);

        return;
}

void
do_disarm(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj, *wpn;
	int chance, hth, ch_weapon, vict_weapon, ch_vict_weapon;
	int wpn_sn, vict_sn;

	hth = UMIN(get_skill(ch, gsn_hand_to_hand), 100);

	if ((chance = get_skill(ch, gsn_disarm)) == 0)
	{
		Cprintf(ch, "You do not know how to disarm opponents.\n\r");
		return;
	}

	if ((wpn = get_eq_char(ch, WEAR_WIELD)) != NULL)
		wpn_sn = get_weapon_sn(ch, WEAR_WIELD);
	else if (get_eq_char(ch, WEAR_DUAL) != NULL) {
		wpn_sn = get_weapon_sn(ch, WEAR_DUAL);
		wpn = get_eq_char(ch, WEAR_DUAL);
	}
	else if (get_eq_char(ch, WEAR_WIELD) == NULL
		&& get_eq_char(ch, WEAR_DUAL) == NULL) {
		wpn = NULL;
		wpn_sn = gsn_hand_to_hand;
	}

	if ((wpn == NULL && hth == 0)
	|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_DISARM)))
	{
		Cprintf(ch, "You must wield a weapon to disarm.\n\r");
		return;
	}

	if ((victim = ch->fighting) == NULL)
	{
		Cprintf(ch, "You are not fighting anyone.\n\r");
		return;
	}

	/* if wielding two weapons, lose one */
	if (get_eq_char(victim, WEAR_WIELD) != NULL
		&& get_eq_char(victim, WEAR_DUAL) != NULL)
	{
		if (number_percent() < 50) {
			obj = get_eq_char(victim, WEAR_WIELD);
			vict_sn = get_weapon_sn(victim, WEAR_WIELD);
		}
		else {
			obj = get_eq_char(victim, WEAR_DUAL);
			vict_sn = get_weapon_sn(victim, WEAR_DUAL);
		}
	}
	else if (get_eq_char(victim, WEAR_WIELD) == NULL
			 && get_eq_char(victim, WEAR_DUAL) != NULL) {
		obj = get_eq_char(victim, WEAR_DUAL);
		vict_sn = get_weapon_sn(victim, WEAR_DUAL);
	}
	else {
		obj = get_eq_char(victim, WEAR_WIELD);
		vict_sn = get_weapon_sn(ch, WEAR_WIELD);
	}

	if (obj == NULL)
	{
		Cprintf(ch, "Your opponent is not wielding a weapon.\n\r");
		return;
	}

	/* find weapon skills */
	ch_weapon = UMIN(get_skill(ch, wpn_sn), 100);
	vict_weapon = UMIN(get_weapon_skill(victim, vict_sn), 100);
	ch_vict_weapon = UMIN(get_weapon_skill(ch, vict_sn), 100);

	// Basic chance
	chance = chance / 2;

	// Modified chance
	// Monks unmodified chance to disarm
	if(!IS_NPC(ch)
	&& wpn == NULL
	&& ch->charClass == class_lookup("monk"))
		chance = chance;
	// Only specialists can disarm with hand to hand.
	else if(!IS_NPC(ch)
	&& wpn == NULL
	&& ch->pcdata->specialty != gsn_hand_to_hand)
		chance = chance * hth / 150;
	else if(!IS_NPC(ch)
	&& wpn == NULL
	&& ch->pcdata->specialty == gsn_hand_to_hand)
		chance = chance * hth / 100;
	// Use your weapon skill as well
	else if(wpn != NULL)
		chance = chance * ch_weapon / 100;

	// Your skill with their weapon
	if(ch->charClass != class_lookup("monk"))
		chance += (ch_vict_weapon - vict_weapon) / 2;

	/* dex vs. strength */
	chance += get_curr_stat(ch, STAT_DEX);
	chance -= get_curr_stat(victim, STAT_STR);

	/* level */
	chance += (ch->level - victim->level) * 2;

	// Weapon type
	// Whips disarm easier
 	if(wpn != NULL
        &&  wpn->item_type == ITEM_WEAPON
        &&  wpn->value[0] == WEAPON_WHIP)
                        chance = chance + 5;

	if(get_eq_char(victim, WEAR_SHIELD) == NULL
	&& IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)) {
		chance = chance / 3;
	}

	/* and now the attack */
	if (number_percent() < chance)
	{
		WAIT_STATE(ch, skill_table[gsn_disarm].beats);
		disarm(ch, victim);
		check_improve(ch, gsn_disarm, TRUE, 1);
	}
	else
	{
		WAIT_STATE(ch, skill_table[gsn_disarm].beats);
		act("You fail to disarm $N.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		act("$n tries to disarm you, but fails.", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		check_improve(ch, gsn_disarm, FALSE, 1);
	}
	check_killer(ch, victim);
	return;
}

/* New dragon skill for blacks: Lair
   coded by Starcrossed, sept 10, 1999
 */
void
do_lair(CHAR_DATA * ch, char *argument)
{
	AFFECT_DATA af;

	if (get_skill(ch, gsn_lair) < 1)
	{
		Cprintf(ch, "Why build a lair when you could sleep in a bed?\n\r");
		return;
	}

	if (is_affected(ch, gsn_lair))
	{
		Cprintf(ch, "You can only have one lair at a time!\n\r");
		return;
	}

	if (room_is_affected(ch->in_room, gsn_lair))
	{
		Cprintf(ch, "This room is already claimed as a lair. Find your own!\n\r");
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_LAIR))
	{
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	if (number_percent() > get_skill(ch, gsn_lair))
	{
		Cprintf(ch, "This place just isn't right for you.\n\r");
		WAIT_STATE(ch, PULSE_VIOLENCE);
		check_improve(ch, gsn_lair, FALSE, 1);
		return;
	}

	/* now apply lair effect to room and character */
	af.where = TO_AFFECTS;
	af.type = gsn_lair;
	af.level = ch->level;
	af.duration = 5;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	af.bitvector = ROOM_AFF_LAIR;
	affect_to_room(ch->in_room, &af);

	Cprintf(ch, "Welcome to your new lair!\n\r");
	act("$n has claimed this room as their lair.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	WAIT_STATE(ch, PULSE_VIOLENCE);
	check_improve(ch, gsn_lair, TRUE, 1);
}

void
do_sla(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "If you want to SLAY, spell it out.\n\r");
	return;
}



void
do_slay(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		Cprintf(ch, "Slay whom?\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They are not here.\n\r");
		return;
	}

	if (ch == victim)
	{
		Cprintf(ch, "Suicide is a mortal sin.\n\r");
		return;
	}

	if (!IS_NPC(victim) && victim->level >= get_trust(ch))
	{
		Cprintf(ch, "You failed.\n\r");
		return;
	}

	act("You slay $M in cold blood!", ch, NULL, victim, TO_CHAR, POS_RESTING);
	act("$n slays you in cold blood!", ch, NULL, victim, TO_VICT, POS_RESTING);
	act("$n slays $N in cold blood!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	raw_kill(victim);
	return;
}

void
do_breath(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	int sn, chance;
	int spell_level;

	if (ch->race == race_lookup("black dragon"))
	{
		sn = gsn_acid_breath;
	}
	else if (ch->race == race_lookup("blue dragon"))
	{
		sn = gsn_lightning_breath;
	}
	else if (ch->race == race_lookup("red dragon"))
	{
		sn = gsn_fire_breath;
	}
	else if (ch->race == race_lookup("white dragon"))
	{
		sn = gsn_frost_breath;
	}
	else if (ch->race == race_lookup("green dragon"))
	{
		sn = gsn_gas_breath;
	}
	else
	{
		Cprintf(ch, "Only dragons can breathe.\n\r");
		return;
	}

	if((chance = get_skill(ch, sn)) == 0)
	{
		Cprintf(ch, "You huff and puff, but your breath is annoying rather than damaging.\n\r");
		return;
	}

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "But you aren't fighting anyone!\n\r");
			return;
		}
	}
	else if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (ch->master == victim)
	{
		Cprintf(ch, "Not on your master!!\n\r");
		return;
	}

	if (IS_NPC(victim) &&
		victim->fighting != NULL &&
		!is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

    if (ch->breath < 10) {
        Cprintf(ch, "You can't seem to muster the wind for another breath.\n\r");
        return;
    }

    /* now the attack */
    WAIT_STATE(ch, skill_table[sn].beats);

    if (number_percent() > chance) {
        Cprintf(ch, "You can't seem to muster the strength.\n\r");
        check_improve(ch, sn, FALSE, 1);
        damage(ch, victim, 0, sn, DAM_POISON, TRUE, TYPE_MAGIC);
    } else {
        spell_level = generate_int(ch->level, ch->level);
        (*skill_table[sn].spell_fun) (sn, spell_level, ch, (void *) victim, TARGET_CHAR);
        check_improve(ch, sn, TRUE, 1);
    }

    ch->breath -= 10;
    check_killer(ch, victim);

    return;
}

void
do_sap(CHAR_DATA * ch, char *argument)
{
	int chance, level;
	OBJ_DATA *wield;
	CHAR_DATA *victim = get_char_room(ch, argument);
	bool victim_dead = FALSE;

	if ((chance = get_skill(ch, gsn_sap)) == 0)
	{
		Cprintf(ch, "And how would you like to do that?\n\r");
		return;
	}

	if (victim == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (!(IS_AWAKE(victim)))
	{
		Cprintf(ch, "They are already out cold.\n\r");
		return;
	}

	if (IS_NPC(victim)
		&& victim->fighting != NULL
		&& !is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "Look at the pretty stars! {Y* {W* {Y* {W*{x\n\r");
		return;
	}

	if (victim->fighting)
	{
		Cprintf(ch, "They're busy right now.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		act("But $N is such a good friend!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	level = ch->level;

	// odd modifiers
	if (!can_see(victim, ch))
	{
		level += 2;
	}

	// blunt weapons knock em out easier
	wield = get_eq_char(ch, WEAR_WIELD);
	if (wield != NULL && attack_table[wield->value[3]].damage == DAM_BASH)
	{
		level += 2;
	}

	check_killer(ch, victim);

	if (number_percent() < chance
	&& !saving_throw(ch, victim, gsn_sap, level, SAVE_HARD, STAT_CON, DAM_BASH))
	{
		AFFECT_DATA af;

		act("$n suddenly slumps to the ground, unconscious!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(victim, "The world swims before your eyes as you collapse...\n\r");

		victim_dead = damage(ch, victim, number_range(ch->level /2, ch->level), gsn_sap, DAM_BASH, FALSE, TYPE_SKILL);
		check_improve(ch, gsn_sap, TRUE, 1);
		WAIT_STATE(ch, skill_table[gsn_sap].beats);

		if (!victim_dead) {
			af.where = TO_AFFECTS;
			af.type = gsn_sleep;
			af.level = ch->level;
			af.duration = 0;
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.bitvector = AFF_SLEEP;

			affect_to_char(victim, &af);

			stop_fighting(victim, TRUE);
			stop_fighting(ch, TRUE);
			victim->position = POS_SLEEPING;
		}
	}
	else
	{
		damage(ch, victim, 0, gsn_sap, DAM_BASH, TRUE, TYPE_SKILL);
		WAIT_STATE(ch, skill_table[gsn_sap].beats * 2);
		check_improve(ch, gsn_sap, FALSE, 1);
	}

}

bool
check_tumbling(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int chance, type;

	if (!IS_AWAKE(victim))
		return FALSE;

	if(ch->in_room == victim->in_room)
	{
		if(ch->fighting == NULL)
			set_fighting(ch, victim);
		if(victim->fighting == NULL)
			set_fighting(victim, ch);
	}

	if (get_skill(victim, gsn_tumbling) < 1)
		return FALSE;

	chance = get_skill(victim, gsn_tumbling) / 5;

	if (!can_see(victim, ch) && number_percent() > get_skill(victim, gsn_blindfighting))
	{
		chance = chance * 3 / 4;
	}

	type = number_range(1, 4);

	if (number_percent() >= chance + victim->level - ch->level)
		return FALSE;

	switch (type)
	{
	case 1:
		act("You somersault away from $n's attack.", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$N somersaults away from your attack!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		break;
	case 2:
		act("You nimbly duck beneath $n's attack.", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$N nimbly ducks underneath your attack!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		break;
	case 3:
		act("You run between $n's legs, foiling his attack!", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$N runs between your legs, causing you to miss.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		break;
	case 4:
		act("You perform a back flip to avoid $n's attack!", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$N performs a back flip to evade your attack!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		break;
	default:
		return FALSE;
	}
	check_improve(victim, gsn_tumbling, TRUE, 6);

	return TRUE;
}

void
do_enrage(CHAR_DATA * ch, char *argument)
{
	int chance;
	AFFECT_DATA af;

	if ((chance = get_skill(ch, gsn_enrage)) == 0)
	{
		Cprintf(ch, "You huff and you puff, but you ain't got the stuff.\n\r");
		return;
	}

	if (is_affected(ch, gsn_enrage))
	{
		Cprintf(ch, "C'mon! How much more can you be enraged?\n\r");
		return;
	}

	if (ch->mana < 100 || ch->move < 100)
	{
		Cprintf(ch, "You can't seem to get the adrenalin rushing.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CALM))
	{
		Cprintf(ch, "Wouldn't you rather pick flowers? You feel so peaceful...\n\r");
		return;
	}

	/* modifiers */
	if (ch->race == race_lookup("dwarf"))
		chance += 10;

	/* fighting */
	if (ch->position == POS_FIGHTING)
		chance += 10;

	if (number_percent() <= chance)
	{
		Cprintf(ch, "You begin to froth at the mouth as a red haze covers your eyes!!!\n\r");
		Cprintf(ch, "You feel very {RANGRY!!!!!!{x\n\r");

		act("$n is blinded by {RRAGE!{x", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		ch->mana -= 100;
		ch->move -= 100;

		/* heal a little damage */
		if(!is_affected(ch, gsn_dissolution)) {
        		ch->hit += ch->level * 3;
        		ch->hit = UMIN(ch->hit, MAX_HP(ch));
		}

		check_improve(ch, gsn_enrage, TRUE, 2);

		af.where = TO_AFFECTS;
		af.type = gsn_enrage;
		af.level = ch->level;
		af.duration = ch->level / 6;

		af.modifier = UMAX(1, ch->level / 5);
		af.bitvector = 0;

		af.location = APPLY_HITROLL;
		affect_to_char(ch, &af);

		af.location = APPLY_DAMROLL;
		affect_to_char(ch, &af);

		af.modifier = UMAX(10, 5 * (ch->level / 5));
		if(ch->race == race_lookup("dwarf"))
        		af.modifier /= 2;
		af.location = APPLY_AC;
		affect_to_char(ch, &af);

		af.location = APPLY_NONE;
		af.modifier = ch->level * 3;
		affect_to_char(ch, &af);

		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

		return;
	}

	Cprintf(ch, "You can't find enough reason to be enraged right now.\n\r");
	ch->mana -= 50;
	ch->move -= 50;
	check_improve(ch, gsn_enrage, FALSE, 2);
	WAIT_STATE(ch, PULSE_VIOLENCE);
	return;
}

void
do_slide(CHAR_DATA * ch, char *argument)
{
	AFFECT_DATA af;
	int chance;

	if (get_skill(ch, gsn_slide) < 1)
	{
		Cprintf(ch, "You slide down a nearby hill. What fun!\n\r");
		return;
	}

	chance = get_skill(ch, gsn_slide);

	if (number_percent() < chance)
	{
		if(is_affected(ch, gsn_slide))
		{
			Cprintf(ch, "Your reflexes are already sharpened.\n\r");
			return;
		}

		af.where = TO_AFFECTS;
		af.type = gsn_slide;
		af.level = ch->level;
		af.duration = 5;
		af.modifier = 0;
		af.bitvector = 0;
		af.location = APPLY_NONE;

		affect_to_char(ch, &af);
		Cprintf(ch, "You focus on becoming fast enough to chase your prey through portals!\n\r");
		check_improve(ch, gsn_slide, TRUE, 2);
		return;
	}
	Cprintf(ch, "You fail to focus on portal sliding.\n\r");
	check_improve(ch, gsn_slide, FALSE, 2);
}

/* Helper function: this function finds a victim is within a certain
   range from the character, in any single direction only. */
CHAR_DATA *
range_finder(CHAR_DATA * ch, char *vname, int range, int *direction, int *distance, bool across_area)
{
	EXIT_DATA *pexit;
	ROOM_INDEX_DATA *base_room, *one_room;
	CHAR_DATA *victim;
	int i, j;

	base_room = ch->in_room;
	one_room = base_room;

	// for starters, are they in the same room?
	if ((victim = get_char_room(ch, vname)) != NULL)
	{
		*direction = -1;
		*distance = 0;
		return victim;
	}

	// Try each direction
	for (i = 0; i < 6; i++)
	{
		// keep searching in this direction up to range -1
		one_room = base_room;
		for (j = 0; j < range; j++)
		{
			pexit = one_room->exit[i];
			if (pexit != NULL
				&& pexit->u1.to_room != NULL
				&& !IS_SET(pexit->exit_info, EX_CLOSED))
			{
				one_room = pexit->u1.to_room;
				// Maybe not across areas...
				if (across_area == FALSE
					&& base_room->area != one_room->area)
					break;

				// so there is an exit. Did we find our victim?
				victim = get_char_from_room(ch, one_room, vname);
				*direction = i;
				*distance = j + 1;
				if (victim != NULL)
					return victim;
			}
		}
	}

	// Can return null if char was not found
	return victim;
}

void
do_charge(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int door, dist;
	int chance = 0;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Kill whom?\n\r");
		return;
	}

	if ((victim = range_finder(ch, arg, 1, &door, &dist, TRUE)) == NULL)
	{
		Cprintf(ch, "They aren't anywhere near you.\n\r");
		return;
	}

	if (get_char_room(ch, arg) != NULL)
	{
		Cprintf(ch, "You need more room to charge them.\n\r");
		return;
	}

	if( get_skill(ch, gsn_charge) == 0)
	{
		Cprintf(ch, "You don't know how to charge opponents.\n\r");
		return;
	}

	if( ch->charge_wait > 0)
	{
		Cprintf(ch, "You lack the valour to charge after fleeing.\n\r");
		return;
	}

	if (ch->move < 25 || ch->mana < 10) {
		Cprintf(ch, "You are too weary to charge into battle.\n\r");
		return;
	}
	ch->move -= 25;
	ch->mana -= 10;

	if (is_safe(ch, victim)) {
		Cprintf(ch, "You can't attack them at all.\n\r");
                return;
	}

	chance = get_skill(ch, gsn_charge) * 2 / 3;
	chance += get_main_hitroll(ch);

	if (number_percent() > chance)
	{
		Cprintf(ch, "Your charge lacks conviction.\n\r");
		check_improve(ch, gsn_charge, FALSE, 1);
		move_char(ch, door, FALSE);
		return;
	}

	/* Ok, now we rush them! */
	act("$n yells a battle cry and charges into battle!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You yell a battle cry and charge bravely into battle!\n\r");
	move_char(ch, door, FALSE);
	act("$n charges fearlessly at $N!", ch, victim, victim, TO_NOTVICT, POS_RESTING);
	Cprintf(victim, "%s charges straight at you!\n\r", ch->name);

	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	check_improve(ch, gsn_charge, TRUE, 1);
	check_killer(ch, victim);

	DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
	multi_hit(ch, victim, TYPE_UNDEFINED);
	return;
}

void
do_push(CHAR_DATA * ch, char *argument)
{
	int dir, chance;
	CHAR_DATA *victim;
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;

	if(get_skill(ch, gsn_push) < 1) {
		Cprintf(ch, "You can't shove people around yet.\n\r");
		return;
	}

	if (ch->fighting == NULL)
	{
		Cprintf(ch, "You aren't fighting anyone right now.\n\r");
		return;
	}

	victim = ch->fighting;

	if (victim->spec_fun == spec_lookup("spec_executioner")
		|| IS_CLAN_GOON(victim))
	{
		Cprintf(ch, "You can't shove this guy at all.\n\r");
		return;
	}

	if (!str_prefix(argument, "north"))
		dir = DIR_NORTH;
	else if (!str_prefix(argument, "south"))
		dir = DIR_SOUTH;
	else if (!str_prefix(argument, "east"))
		dir = DIR_EAST;
	else if (!str_prefix(argument, "west"))
		dir = DIR_WEST;
	else if (!str_prefix(argument, "up"))
		dir = DIR_UP;
	else if (!str_prefix(argument, "down"))
		dir = DIR_DOWN;
	else
	{
		Cprintf(ch, "There are no such direction in this game.\n\r");
		return;
	}

	if (ch->in_room->exit[dir] == NULL)
	{
		Cprintf(ch, "There is no exit in that direction.\n\r");
		return;
	}

	in_room = ch->in_room;
	pexit = in_room->exit[dir];
	to_room = pexit->u1.to_room;

	if (IS_SET(to_room->room_flags, ROOM_CLAN)
		&& (to_room->clan))
	{
		Cprintf(ch, "You can't push someone into an usher room!\n\r");
		return;
	}

	chance = (get_skill(ch, gsn_push) * 3 / 4) + 1;
	chance += (get_curr_stat(ch, STAT_STR) - get_curr_stat(victim, STAT_STR)) * 2;
	chance += (ch->level - victim->level) * 2;

	if(ch->size < victim->size)
		chance = chance * 3 / 4;

	if (number_percent() > chance)
	{
		Cprintf(ch, "Your foe resists your efforts.\n\r");
		Cprintf(victim, "You resist %s as he tries to push you.\n\r", ch->name);
		act("$n starts to push $N but fails.", ch, victim, victim, TO_NOTVICT, POS_RESTING);
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
		check_improve(ch, gsn_push, FALSE, 1);
		return;
	}

	// Give them a push!
	if (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
		REMOVE_BIT(victim->act, ACT_AGGRESSIVE);

	Cprintf(ch, "You grab ahold of your foe and move the battle elsewhere.\n\r");
	Cprintf(victim, "%s pushes you! You stumble backwards into another room.\n\r", ch->name);
	act("In the heat of battle $n pushes $N into another room!", ch, victim, victim, TO_NOTVICT, POS_RESTING);
	/* stops oddness charmies fighting across rooms */

	stop_fighting(ch, TRUE);
	move_char(victim, dir, TRUE);
	move_char(ch, dir, FALSE);
	check_improve(ch, gsn_push, TRUE, 1);
	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
	set_fighting(ch, victim);
	set_fighting(victim, ch);
	return;
}

void
do_rip(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	int chance;

	if ((chance = get_skill(ch, gsn_rip)) < 1)
	{
		Cprintf(ch, "Ask nicely, you don't just grab it!\n\r");
		return;
	}

	if (ch->fighting == NULL)
	{
		Cprintf(ch, "You are not fighting anyone.\n\r");
		return;
	}

	victim = ch->fighting;

	if ((obj = get_eq_char(victim, WEAR_HOLD)) == NULL)
	{
		Cprintf(ch, "They aren't holding anything you can grab.\n\r");
		return;
	}

	// Pit strengthes against eachother.
	chance /= 2;
	chance -= get_curr_stat(victim, STAT_STR);
	chance += get_curr_stat(ch, STAT_STR);

	// Easier if the grabber has a hand free
	if (get_eq_char(ch, WEAR_HOLD) == NULL)
		chance += 10;

	// Also modify by level
	chance += (ch->level - victim->level) * 2;

	if (number_percent() > chance)
	{
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
		act("You make a grab for the item held by $N, but miss.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		act("$n tries grab $p, but fails.", ch, obj, victim, TO_VICT, POS_RESTING);
		act("$n tries to rip $p from $N's hands, but fails.", ch, obj, victim, TO_NOTVICT, POS_RESTING);
		check_improve(ch, gsn_rip, FALSE, 1);
		return;
	}

	if (IS_OBJ_STAT(obj, ITEM_NOREMOVE))
	{
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
		act("$S $p won't budge!", ch, obj, victim, TO_CHAR, POS_RESTING);
		act("$n tries to grab $p, but it won't come loose!", ch, obj, victim, TO_VICT, POS_RESTING);
		act("$n tries to rip $p from $N's hands, but fails.", ch, obj, victim, TO_NOTVICT, POS_RESTING);
		return;
	}

	if (IS_SET(obj->extra_flags, ITEM_ROT_DEATH))
	{
		obj->timer = number_range(5, 10);
		REMOVE_BIT(obj->extra_flags, ITEM_ROT_DEATH);
	}

	act("$n {rRIPS{x $p loose from your grip!", ch, obj, victim, TO_VICT, POS_RESTING);
	act("You rip $p loose from $N's hands!", ch, obj, victim, TO_CHAR, POS_RESTING);
	act("$n rips $p from $N's hands!", ch, obj, victim, TO_NOTVICT, POS_RESTING);
	obj_from_char(obj);

	if(number_percent() < 35) {
		Cprintf(ch, "They manage to catch it before it hits the ground though.\n\r");
		obj_to_char(obj, victim);
	}
	else {
		Cprintf(ch, "It skitters across the floor!\n\r");
		if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_INVENTORY))
			obj_to_char(obj, victim);
		else
			obj_to_room(obj, victim->in_room);
	}

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	check_improve(ch, gsn_rip, TRUE, 1);
	return;
}

void
do_gladiator(CHAR_DATA * ch, char *argument)
{
	int chance;
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	int wep;
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	victim = get_char_room(ch, arg1);

	if ((chance = get_skill(ch, gsn_gladiator)) == 0)
	{
		Cprintf(ch, "And how would you like to do that?\n\r");
		return;
	}

	if (victim == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

   if ( IS_NPC(victim) ) {
      Cprintf(ch, "You don't want to waste your time training mobs.\n\r");
      return;
   }

	if (arg2[0] == '\0')
	{
		Cprintf(ch, "You must select a weapon type.\n\r'sword', 'dagger', 'axe', 'mace', 'polearm', 'whip', 'flail', 'spear'\n\r");
		return;
	}

	if (!str_prefix(arg2, "sword"))
	{
		wep = gsn_sword;
	}
	else if (!str_prefix(arg2, "dagger"))
	{
		wep = gsn_dagger;
	}
	else if (!str_prefix(arg2, "axe"))
	{
		wep = gsn_axe;
	}
	else if (!str_prefix(arg2, "mace"))
	{
		wep = gsn_mace;
	}
	else if (!str_prefix(arg2, "flail"))
	{
		wep = gsn_flail;
	}
	else if (!str_prefix(arg2, "whip"))
	{
		wep = gsn_whip;
	}
	else if (!str_prefix(arg2, "spear"))
	{
		wep = gsn_spear;
	}
	else if (!str_prefix(arg2, "polearm"))
	{
		wep = gsn_polearm;
	}
	else
	{
		Cprintf(ch, "No such weapon type exists.\n\r");
		return;
	}

	if (get_skill(ch, wep) == 0)
	{
		Cprintf(ch, "You don't know that weapon!\n\r");
		return;
	}

	if (get_skill(victim, wep) != 0)
	{
		Cprintf(ch, "They already know how to use that!\n\r");
		return;
	}

	if (is_affected(victim, gsn_gladiator))
	{
		Cprintf(ch, "They have already been instructed in the arts of one weapon, give them a chance.\n\r");
		return;
	}

	if (number_percent() < chance)
	{
		AFFECT_DATA af;

		Cprintf(ch, "You instruct them in the use of that weapon.\n\r");
		Cprintf(victim, "You have been instructed in the use of a new weapon.\n\r");

		check_improve(ch, gsn_gladiator, TRUE, 1);
		WAIT_STATE(ch, skill_table[gsn_gladiator].beats);

		af.where = TO_AFFECTS;
		af.type = gsn_gladiator;
		af.level = ch->level;
		af.duration = ch->level * 5;
		af.location = APPLY_NONE;
		af.modifier = wep;
		af.bitvector = 0;
		affect_to_char(victim, &af);

		victim->pcdata->learned[wep] = UMIN(get_skill(ch, wep), 100);
	}
	else
	{
		Cprintf(ch, "You fail to instruct them.\n\r");
		WAIT_STATE(ch, skill_table[gsn_gladiator].beats * 2);
		check_improve(ch, gsn_gladiator, FALSE, 1);
	}

}

void
end_gladiator(void *vo)
{
	CHAR_DATA *ch = (CHAR_DATA *) vo;
	AFFECT_DATA *paf;

	// No mobs
	if(IS_NPC(ch))
		return;

	paf = affect_find(ch->affected, gsn_gladiator);
	if (paf != NULL)
	{
		ch->pcdata->learned[paf->modifier] = 0;
	}
}

void
do_specialize(CHAR_DATA * ch, char *argument)
{
	AFFECT_DATA *af_glad;
	char arg[MAX_STRING_LENGTH];
	int sn;

	one_argument(argument, arg);

	if (IS_NPC(ch) || ch->pcdata == NULL)
	{
		Cprintf(ch, "NPC's can't specialize.\n\r");
		return;
	}

	if (ch->reclass <= 0)
	{
		Cprintf(ch, "Only reclasses can specialize.\n\r");
		return;
	}

	if (ch->level < 25)
	{
		Cprintf(ch, "You must be level 25 to specialize.\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		Cprintf(ch, "You are currently specializing with '%s'\n\r",
				ch->pcdata->specialty == 0 ? "none" :
				skill_table[ch->pcdata->specialty].name);
		return;
	}

	sn = skill_lookup(arg);
	if (sn == ch->pcdata->specialty)
	{
		Cprintf(ch, "You are already specializing with that.\n\r");
		return;
	}

	if (sn != gsn_sword &&
		sn != gsn_dagger &&
		sn != gsn_polearm &&
		sn != gsn_whip &&
		sn != gsn_mace &&
		sn != gsn_axe &&
		sn != gsn_spear &&
		sn != gsn_dodge &&
		sn != gsn_parry &&
		sn != gsn_flail &&
		sn != gsn_shield_block &&
		sn != gsn_hand_to_hand &&
		sn != gsn_katana &&
		sn != gsn_marksmanship)
	{
		Cprintf(ch, "You cannot specialize with that skill yet, this may be coded in the future.\n\r");
		Cprintf(ch, "Valid skills: sword, dagger, flail, polearm, whip, mace, axe, spear, katana, marksmanship, dodge, parry, shield block or hand to hand.\n\r");
		return;
	}

	if (get_skill(ch, sn) == 0) {
 		Cprintf(ch, "You don't know that skill.\n\r");
		return;
	}

	af_glad =affect_find(ch->affected, gsn_gladiator);
	if(af_glad != NULL)
	{
		if(af_glad->modifier == sn)
		{
			Cprintf(ch, "You cannot specialize in a weapon you are not completely familiar with.\n\r");
			return;
		}
	}

	if (ch->pcdata->specialty != 0)
	{
		Cprintf(ch, "You transfer your specialization to %s from %s.\n\r",
				skill_table[sn].name,
				skill_table[ch->pcdata->specialty].name);

		ch->pcdata->learned[ch->pcdata->specialty] =
			(ch->pcdata->learned[ch->pcdata->specialty] / 2) + 20;

	}

	ch->pcdata->specialty = sn;

	Cprintf(ch, "You now specialize with '%s'\n\r",
				ch->pcdata->specialty == 0 ? "none" :
				skill_table[ch->pcdata->specialty].name);
	return;
}

void
do_rally(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *gch;
	int chance;
	int members;
	bool fail;

	one_argument(argument, arg);

	if (get_skill(ch, gsn_rally) < 1)
	{
		Cprintf(ch, "Rally? What's that?\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Kill whom?\n\r");
		return;
	}
	else if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "That probably isn't very smart?\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (IS_NPC(victim) &&
		victim->fighting != NULL &&
		!is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
	{
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if(ch->move < 10) {
		Cprintf(ch, "You're too tired to rally your troops.\n\r");
		return;
	}

	check_killer(ch, victim);
	WAIT_STATE(ch, PULSE_VIOLENCE * 2);
	ch->move -= 10;

	members = 0;
	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
	{
		if (is_same_group(gch, ch))
		{
			if(gch == victim) {
				Cprintf(ch, "Not against your own group mates.\n\r");
				return;
			}
			members++;
		}
	}

	if (members < 2)
	{
		Cprintf(ch, "You need more people to effectively rally.\n\r");
		return;
	}

	Cprintf(ch, "You rally your group to defeat %s!\n\r", IS_NPC(victim) ? victim->short_descr : victim->name);
	act("$n takes command and rallies against $N!", ch, NULL, victim, TO_ROOM, POS_RESTING);

	fail = TRUE;
	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
	{
		if (is_same_group(gch, ch) || gch == ch)
		{
			chance = get_skill(ch, gsn_rally);
			chance += (gch->level - victim->level);

			if (number_percent() < chance)
			{
				gch->fighting = victim;
				
				weapon_hit(gch, victim, get_eq_char(gch, WEAR_WIELD), TYPE_UNDEFINED);
				fail = FALSE;
			}
		}
	}

	if (fail == FALSE)
	{
		check_improve(ch, gsn_rally, TRUE, 4);
	}
	else
	{
		check_improve(ch, gsn_rally, FALSE, 4);
		return;
	}

	return;
}

void
do_cave_in(CHAR_DATA * ch, char *argument)
{
	AFFECT_DATA af;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;
	int chance;

	if (get_skill(ch, gsn_cave_in) < 1)
	{
		Cprintf(ch, "Look out below. Or not.\n\r");
		return;
	}

	if (IS_OUTSIDE(ch))
	{
		Cprintf(ch, "There is no ceiling to collapse here.\n\r");
		return;
	}

	if(room_is_affected(ch->in_room, gsn_cave_in)) {
		Cprintf(ch, "This ceiling has already been torn down.\n\r");
		return;
	}

	if(ch->move < 75) {
		Cprintf(ch, "You don't have the strength left to bring down the house.\n\r");
		return;
	}

	WAIT_STATE(ch, skill_table[gsn_cave_in].beats);
	ch->move -= 75;

	chance = get_skill(ch, gsn_cave_in);
	if (number_percent() > chance)
	{
		Cprintf(ch, "You fail in your attempt to cave in the room.\n\r");
		check_improve(ch, gsn_cave_in, FALSE, 2);
		return;
	}

	Cprintf(ch, "You bring down the house!\n\r");
	check_improve(ch, gsn_cave_in, TRUE, 1);

	/* area attack */
	dam = dice(ch->level, 5);

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;
		if (vch != ch && !is_safe(ch, vch))
		{
			damage(ch, vch, dam, gsn_cave_in, DAM_BASH, TRUE, TYPE_MAGIC);
		}
	}

	/* chance of block exit */
	af.where = TO_AFFECTS;
	af.type = gsn_cave_in;
	af.level = ch->level;
	af.duration = ch->level / 4;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_room(ch->in_room, &af);
}

void
do_call_to_hunt(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *mob;
	CHAR_DATA *vch;
	int chance;

	argument = one_argument(argument, arg1);

	if (get_skill(ch, gsn_call_to_hunt) < 1)
	{
		Cprintf(ch, "You couldn't hunt an elephant in a flat field.\n\r");
		return;
	}

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Hunt who?\n\r");
		return;
	}

	if ((victim = get_char_world(ch, arg1, TRUE)) == NULL)
	{
		Cprintf(ch, "Hunt who?\n\r");
		return;
	}

	if (!is_clan(ch) && is_clan(victim))
	{
		Cprintf(ch, "You must be in a clan to organize a hit on a clanner.\n\r");
		return;
	}

	if (!IS_NPC(victim) && !is_clan(victim))
	{
		Cprintf(ch, "You can't organize a hit on a non-clanner.\n\r");
		return;
	}

	if (ch == victim)
	{
		Cprintf(ch, "Very funny. Not.\n\r");
		return;
	}


	WAIT_STATE(ch, skill_table[gsn_call_to_hunt].beats);
	if(ch->move < 75) {
		Cprintf(ch, "You are too tired to make the effort.\n\r");
		return;
	}
	ch->move -= 75;
	chance = get_skill(ch, gsn_call_to_hunt) * 5 / 6;
	if (number_percent() > chance)
	{
		Cprintf(ch, "You can't seem to find your hunting dog.\n\r");
		check_improve(ch, gsn_call_to_hunt, FALSE, 2);
		return;
	}

	Cprintf(ch, "You call your loved hunting dog.\n\r");
	check_improve(ch, gsn_call_to_hunt, TRUE, 2);
	/* kill old dogs first */
	for (vch = char_list; vch != NULL; vch = vch->next)
	{
		if (IS_NPC(vch) &&
			vch->pIndexData->vnum == MOB_VNUM_HUNT_DOG &&
			vch->master == ch)
		{
			extract_char(vch, TRUE);
			break;
		}
	}

	mob = create_mobile(get_mob_index(MOB_VNUM_HUNT_DOG));
	char_to_room(mob, ch->in_room);
	mob->hunting = victim;
	mob->hunt_timer = 30;
	if (ch->master)
		stop_follower(ch);
	if (mob->master)
		stop_follower(mob);
	add_follower(ch, mob);
	mob->master = ch;
	act("$n calls a huge hunting dog!\n\r", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	return;
}

void
do_granite_stare(CHAR_DATA *ch, char* argument) {
	int chance;
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *statue;
	CHAR_DATA *victim;

    if ((chance = get_skill(ch, gsn_granite_stare)) == 0)
    {
		Cprintf(ch, "You glare at them coldly, but what did you expect?\n\r");
        return;
	}

	if(*argument == '\0')
		victim = ch->fighting;
	else
		victim = get_char_room(ch, argument);

	if (victim == NULL)
    {
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (!IS_NPC(victim))
        {
                Cprintf(ch, "This only works on mobiles.\n\r");
                return;
        }

	/* eyes are unkillable */
	if (IS_NPC(victim) && (victim->pIndexData->vnum == MOB_VNUM_EYE))
	{
		Cprintf(ch, "You stare at the eye, which stares back at you.\n\r");
		return;
	}

	/* goon attacks anyone staring at it.. this is to prevent
	   cheapass hall raids */
	if (IS_CLAN_GOON(victim))
	{
		Cprintf(ch, "%s doesn't like the way you look!\n\r",victim->short_descr);
		damage(ch, victim, 0, gsn_granite_stare, DAM_NONE, FALSE, FALSE);
		return;
	}

	if (victim->pIndexData->pShop != NULL) {
		Cprintf(ch, "%s doesn't like the way you're looking at them!\n\r", victim->name);
		return;
	}

	if (ch->pktimer > 0)
	{
			Cprintf(ch, "Give yourself a chance to breathe before fighting again.\n\r");
			return;
    }

	chance = get_skill(ch, gsn_granite_stare);
	WAIT_STATE(ch, PULSE_VIOLENCE);
	affect_strip(ch, gsn_pacifism);

	if(victim->master) {
		// Clanners can't gaze nonclanner charmies.
		if(!is_clan(victim->master) && is_clan(ch)) {
			Cprintf(ch, "That mob is owned by a non-clanner, leave em alone.\n\r");
			return;
		}
		if(is_clan(victim->master) && !is_clan(ch)) {
			Cprintf(ch, "That mob is owned by a clanner, leave em alone.\n\r");
			return;
		}
		check_killer(ch, victim->master);
	}
	else
		check_killer(ch, victim);

	if(saving_throw(ch, victim, gsn_granite_stare, ch->level, SAVE_HARD, STAT_INT, DAM_MENTAL)) {
		Cprintf(ch, "They resist your gaze attack.\n\r");
		check_improve(ch, gsn_granite_stare, FALSE, 3);
		damage(ch, victim, 0, gsn_granite_stare, DAM_NONE, FALSE, TYPE_MAGIC);
		return;
	}

	if (number_percent() > chance)
	{
			Cprintf(ch, "%s starts to solidify but shakes it off.\n\r", victim->short_descr);
			check_improve(ch, gsn_granite_stare, FALSE, 3);
			damage(ch, victim, 0, gsn_granite_stare, DAM_NONE, FALSE, TYPE_MAGIC);
            return;
    }
    else
    {
		Cprintf(ch, "%s turns to stone and solidifies before you!\n\r", victim->short_descr);
		act("$n turns you to stone and solidifies you!", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$n turns $N to stone!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);

		damage(ch, victim, 0, gsn_granite_stare, DAM_NONE, FALSE, FALSE);
		stop_fighting(victim, TRUE);
		death_cry(victim);

		victim->pIndexData->killed++;
		kill_table[URANGE(0, victim->level, MAX_LEVEL - 1)].killed++;

		statue = create_object(get_obj_index(OBJ_VNUM_GRANITE_STATUE), 0);
		free_string(statue->description);
		sprintf(buf, "A granite statue of %s is here.", victim->short_descr);
		statue->description = str_dup(buf);
		statue->timer = ch->level / 2;
		obj_to_room(statue, victim->in_room);
		extract_char(victim, TRUE);
		check_improve(ch, gsn_granite_stare, TRUE, 3);
        	return;
	}
}

void
do_cheat(CHAR_DATA * ch, char *argument)
{
        AFFECT_DATA af;
        int chance;

        if (get_skill(ch, gsn_cheat) < 1)
        {
                Cprintf(ch, "Huh?\n\r");
                return;
        }

        chance = get_skill(ch, gsn_cheat);

	if(is_affected(ch, gsn_cheat) ) {
		Cprintf(ch, "You'll have to wait awhile before cheating again.\n\r");
		return;
	}

        if (number_percent() < chance)
        {
   		af.where = TO_AFFECTS;
                af.type = gsn_cheat;
                af.level = ch->level;
                af.duration = 5;
                af.modifier = 0;
                af.bitvector = 0;
                af.location = APPLY_NONE;

                affect_to_char(ch, &af);
		WAIT_STATE(ch, PULSE_VIOLENCE);
                Cprintf(ch, "You enhance your skills at sleight of hand.\n\r");
		check_improve(ch, gsn_cheat, TRUE, 4);
                return;
        }
	WAIT_STATE(ch, PULSE_VIOLENCE);
        Cprintf(ch, "You fail to increase your sleight of hand.\n\r");
	check_improve(ch, gsn_cheat, FALSE, 4);
}

/* basically pick a starting spot on the table and go through,
   looping as needed, until all shards are used up. */
// Triggers one shard to hit attacker (ch)
// Effect randomly ends
void prismatic_sphere_effect(CHAR_DATA *ch, CHAR_DATA *victim) {
	int colour;
	char buf[MAX_STRING_LENGTH];
	int spell_level;

	struct prismatic_shard {
		char *colour_msg;
		int spell_sn;
		SPELL_FUN *spell_fun;
	};

	const struct prismatic_shard prismatic_sphere[] =
	{
		{"{yorange{x", gsn_blindness,    spell_blindness    },
		{"{Yyellow{x", gsn_plague,       spell_plague       },
		{"{Mviolet{x", gsn_energy_drain, spell_energy_drain },
		{"{Dblack{x",  gsn_curse,        spell_curse        },
		{"{ggreen{x",  gsn_blast_of_rot, spell_blast_of_rot },
		{"{bblue{x",   gsn_ice_bolt,     spell_ice_bolt     },
		{"{mindigo{x", gsn_feeblemind,   spell_feeblemind   },
		{"{Rred{x",    gsn_pyrotechnics, spell_pyrotechnics },
		{"",           0,                spell_null         },
	};

	colour = number_range(0,7);

	if(colour < 4)
		spell_level = generate_int(victim->level, victim->level);
	else
		spell_level = generate_int(victim->level * 4 / 5,
					   victim->level * 4 / 5);

	sprintf(buf, "An %s shard breaks off the prismatic sphere!", prismatic_sphere[colour].colour_msg);
	act(buf, ch, NULL, NULL, TO_ALL, POS_RESTING);

	if(number_percent() < 35) {
		act("The prismatic sphere shatters!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
        	affect_strip(victim, gsn_prismatic_sphere);
	}
	prismatic_sphere[colour].spell_fun(prismatic_sphere[colour].spell_sn, spell_level, victim, ch, TARGET_CHAR);

	return;
}

// Display combat information
void do_fstat(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	int skill = 0, AR = 0, DR = 0;
	int spellpower = 0, resistance = 0;
	int level, insanity = 0;
	int sn;

	if(argument == '\0')
		victim = ch;
	else
		victim = get_char_room(ch, argument);

	if(victim == NULL)
		victim = get_char_world(ch, argument, TRUE);

	if(victim == NULL) {
		Cprintf(ch, "FSTAT: Target not found.\n\r");
		return;
	}

	Cprintf(ch, "%s combat stats:\n\r--------------------\n\r",
		IS_NPC(victim) ? victim->short_descr : victim->name);

	if(get_eq_char(victim, WEAR_WIELD) == NULL)
		skill = get_skill(victim, gsn_hand_to_hand);
	else {
		sn = get_weapon_sn(victim, WEAR_WIELD);
		skill = get_skill(victim, sn);
	}

	AR = get_attack_rating(victim, get_eq_char(ch, WEAR_WIELD));
	Cprintf(ch, "Primary Attack Rate: %f%% hits (Base %f%%)\n\r",
	((8 * AR) + ((2 * AR * skill) / 100.0)) / 10, AR);

	DR = (get_defense_rating(victim, DAM_SLASH) + get_defense_rating(victim, DAM_PIERCE)
		+ get_defense_rating(victim, DAM_BASH) + get_defense_rating(victim, DAM_FIRE)) / 4;

	Cprintf(ch, "Average Defense Rating: %d%% stopped by AC\n\r", DR);

	if(victim->charClass <= class_lookup("druid"))
                level = victim->level;
        else if(victim->charClass == class_lookup("paladin")
        && victim->reclass != reclass_lookup("cavalier"))
                level = victim->level * 5 / 6;
        else if(victim->charClass == class_lookup("ranger"))
                level = victim->level * 5 / 6;
        else
                level = victim->level * 3 / 4;

        spellpower = (2 * level) + 50;
        spellpower += get_curr_stat(victim, STAT_INT) / 2;

        Cprintf(ch, "Magical casting power with 100%% spell:\n\r");
        Cprintf(ch, "Easy: %d\tNormal: %d\tHard: %d\n\r",
                spellpower * 9 / 10,
                spellpower,
                spellpower * 6 / 5);

        resistance = 2 * victim->level;
        resistance += (get_curr_stat(victim, STAT_STR)
                        + get_curr_stat(victim, STAT_DEX)
                        + get_curr_stat(victim, STAT_CON)
                        + get_curr_stat(victim, STAT_INT)
                        + get_curr_stat(victim, STAT_WIS)) / 5;

        resistance -= UMAX(victim->saving_throw, -30);

        Cprintf(ch, "Average spell resistance: %d\n\r", resistance);

        if(is_affected(victim, gsn_berserk) || IS_AFFECTED(victim, AFF_BERSERK)) {
                insanity += victim->level / 5;
        }
        if(is_affected(victim, gsn_enrage)) {
                insanity += victim->level / 5;
        }
        if(victim->race == race_lookup("dwarf")) {
                insanity += victim->level / 10;
        }
   	if(victim->charClass == class_lookup("paladin")) {
                insanity += victim->level / 10;
        }

        if(insanity > 0) {
                Cprintf(ch, "Spell immunity: %d%%\n\r", insanity);
        }
}

void
do_dark_feast(CHAR_DATA *ch, char *arg) {
	CHAR_DATA *victim = NULL;
	int healed, chance = 0;

	if((chance = get_skill(ch, gsn_dark_feast)) < 1) {
		Cprintf(ch, "That's disgusting! What kind of creature are you?\n\r");
		return;
	}

	chance = (chance * 5 / 6) + 1;

	if (arg[0] == '\0')
        {
                victim = ch->fighting;
                if (victim == NULL)
                {
                        Cprintf(ch, "You are not fighting anyone.\n\r");
                        return;
                }
        }
        else
        {
                if ((victim = get_char_room(ch,arg)) == NULL)
                {
                        Cprintf(ch, "They're not here.\n\r");
                        return;
                }
        }

	if(victim == ch) {
		Cprintf(ch, "How could you feast on yourself?\n\r");
		return;
	}

	if(is_safe(ch, victim)) {
		Cprintf(ch, "Their blood won't satisfy your needs.\n\r");
		return;
	}

	healed = (120 - (((float)victim->hit / (float)MAX_HP(victim)) * 100));

	if(ch->move < healed / 2) {
		Cprintf(ch, "You lack the energy to nourish yourself on the blood of the fallen.\n\r");
		return;
	}
	check_killer(ch, victim);

	damage(ch, victim, 0, gsn_dark_feast, DAM_NONE, FALSE, TYPE_SKILL);

	if(number_percent() > chance) {
		ch->move -= healed / 4;
		Cprintf(ch, "They evade your attempt to feed on them.\n\r");
		Cprintf(victim, "You evade %s's attempt to feed off of you!\n\r", ch->name);
		WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		check_improve(ch, gsn_dark_feast, FALSE, 2);
		return;
	}

	ch->move -= healed / 2;
	healed = number_range(healed / 2, healed);
	if(!is_affected(ch, gsn_dissolution))
		ch->hit = UMIN(MAX_HP(ch), ch->hit + healed);

	act("$n latches onto $N with six claws.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	act("$n starts drinking {Rblood!{x", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	Cprintf(ch, "You latch onto your victim and begin to feed on their {RBLOOD!{x\n\r");
	Cprintf(victim, "%s latches onto you with their claws.\n\r", ch->name);
	Cprintf(victim, "You feel faint as they begin feeding on your {RBLOOD{x!\n\r");

	WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
	check_improve(ch, gsn_dark_feast, TRUE, 2);
	return;
}

void
do_razor_claws(CHAR_DATA *ch, char *argument) {
	AFFECT_DATA af;
	int chance = 0;

	if((chance = get_skill(ch, gsn_razor_claws)) < 1) {
		Cprintf(ch, "You don't have the claws for this.\n\r");
		return;
	}

	if(ch->move < 5) {
		Cprintf(ch, "You can't focus on your claws right now.\n\r");
		return;
	}

	ch->move -= 5;
	WAIT_STATE(ch, PULSE_VIOLENCE);

	if(is_affected(ch, gsn_razor_claws)) {
		if(number_percent() > chance) {
			Cprintf(ch, "You fidget with your claws, but can't seem to retract them right now.\n\r");
			check_improve(ch, gsn_razor_claws, FALSE, 2);
			return;
		}

		act("$n retracts their razor sharp claws.", ch, NULL, ch, TO_ROOM, POS_RESTING);
		Cprintf(ch, "You retract your razor sharp claws, freeing up your grip.\n\r");
		check_improve(ch, gsn_razor_claws, TRUE, 2);
		affect_strip(ch, gsn_razor_claws);
		return;
	}

	if (get_eq_char(ch, WEAR_WIELD) != NULL
	|| get_eq_char(ch, WEAR_DUAL) != NULL) {
		Cprintf(ch, "Your paws are wielding weapons right now. Put them away if you need your claws.\n\r");
		return;
	}

	if(number_percent() > chance) {
		Cprintf(ch, "You fidget with your claws, but can't seem to extend them right now.\n\r");
		check_improve(ch, gsn_razor_claws, FALSE, 2);
		return;
	}

	act("$n extends their razor sharp claws.", ch, NULL, ch, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You extend your razor sharp claws.\n\r");
	check_improve(ch, gsn_razor_claws, TRUE, 2);

	af.where = TO_AFFECTS;
	af.type = gsn_razor_claws;
	af.level = ch->level;
	af.duration = ch->level / 3;
	af.modifier = ch->level / 10;
	af.bitvector = 0;
        af.location = APPLY_DAMROLL;
	affect_to_char(ch, &af);

	return;
}

void
do_ambush(CHAR_DATA * ch, char *argument)
{
        AFFECT_DATA af;
        int chance;

        if (get_skill(ch, gsn_ambush) < 1)
        {
                Cprintf(ch, "You're already doing your best.\n\r");
                return;
        }


        if (is_affected(ch, gsn_ambush))
        {
                Cprintf(ch, "You're already ready to pounce!\n\r");
                return;
        }

	if (ch->move < 30) {
		Cprintf(ch, "You're too tired to ambush anyone right now.\n\r");
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

 	chance = get_skill(ch, gsn_ambush);
        if (number_percent() > chance)
        {
		ch->move -= 15;
                Cprintf(ch, "You can't find a good place to ambush from right now.\n\r");
                check_improve(ch, gsn_ambush, FALSE, 2);
                return;
        }

	ch->move -= 30;
	Cprintf(ch, "You crouch low and get ready to pounce!\n\r");
	act("$n gets ready to launch a surprise attack.", ch, NULL, ch, TO_ROOM, POS_RESTING);
        check_improve(ch, gsn_ambush, TRUE, 2);

        af.where = TO_AFFECTS;
        af.type = gsn_ambush;
        af.level = ch->level;
        af.duration = 5;
        af.modifier = 0;
        af.bitvector = 0;
        af.location = APPLY_NONE;
        affect_to_char(ch, &af);

        return;
}

// Mod is which power you want to count
// ie TATTOO_MV_SPELLS
int
power_tattoo_count(CHAR_DATA *ch, int mod) {
	int count = 0;
	int i = 0;
	OBJ_DATA *obj = NULL;
	AFFECT_DATA *paf = NULL;

	for(i=0; i<MAX_WEAR - 1; i++) {
		obj = get_eq_char(ch, i);
		if(obj != NULL) {
			if((paf = affect_find(obj->affected, gsn_paint_power)) != NULL) {
				if(paf->modifier == mod)
				count++;
			}
		}
	}

	return count;
}

// Returns true if the character is wearing a tattoo that allows this
// skill/spell to be used.
int
using_skill_tattoo(CHAR_DATA *ch, int sn) {
	int i = 0;
	OBJ_DATA *obj = NULL;
	AFFECT_DATA *paf = NULL;

	for(i=0; i<MAX_WEAR - 1; i++) {
        	obj = get_eq_char(ch, i);
	        if(obj != NULL) {
                	if((paf = affect_find(obj->affected, gsn_paint_power)) != NULL) {
                        	if(paf->modifier == TATTOO_LEARN_SPELL
				&& paf->extra == sn)
                	        	return TRUE;
			}
                }
        }

	return FALSE;
}


void
do_zeal(CHAR_DATA * ch, char *argument)
{
        AFFECT_DATA af;
        int chance;

        if (IS_NPC(ch))
                return;

        if (get_skill(ch, gsn_zeal) < 1)
        {
                Cprintf(ch, "Being injured doesn't make you feel particularly powerful.\n\r");
                return;
        }

        if (is_affected(ch, gsn_zeal))
        {
                Cprintf(ch, "Your agony is already being converted into zealous wrath.\n\r");
                return;
        }

	if(ch->move < 10
	|| ch->mana < 10) {
		Cprintf(ch, "You are too tired to invoke a zealous rage right now.\n\r");
		return;
	}

	ch->move -= 10;
	ch->mana -= 10;

        chance = get_skill(ch, gsn_zeal);
        if (number_percent() > chance)
        {
                Cprintf(ch, "You fail to focus your zealous wrath.\n\r");
                check_improve(ch, gsn_zeal, FALSE, 2);
                return;
        }

        af.where = TO_AFFECTS;
        af.type = gsn_zeal;
        af.level = ch->level;
        af.duration = 5;
        af.modifier = 0;
        af.bitvector = 0;
        af.location = APPLY_NONE;
        affect_to_char(ch, &af);
        check_improve(ch, gsn_zeal, TRUE, 2);
        Cprintf(ch, "You begin to channel pain into a zealous wrath!\n\r");
        return;
}

void
do_fury(CHAR_DATA * ch, char *argument)
{
        AFFECT_DATA af, *paf;
        int chance;

        if (IS_NPC(ch))
                return;

        if (get_skill(ch, gsn_fury) < 1)
        {
                Cprintf(ch, "You can't control your fury this way.\n\r");
                return;
        }

	if (ch->move < 15) {
		Cprintf(ch, "You don't have enough energy left to focus your fury.\n\r");
		return;
	}

	if (ch->fighting == NULL) {
		Cprintf(ch, "You need to be in combat to focus your fury.\n\r");
		return;
	}

        if ((paf = affect_find(ch->affected, gsn_fury)) != NULL)
        {
		switch(paf->modifier)
		{
			case 1:
				chance = get_skill(ch, gsn_fury);
        			if (number_percent() > chance)
        			{
                			Cprintf(ch, "You fail to increase your fury.\n\r");
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
                			check_improve(ch, gsn_fury, FALSE, 2);
                			return;
        			}
				ch->move -= 15;
				paf->duration = 5;
				paf->modifier = 2;
				Cprintf(ch, "Your fury grows!\n\r");
				Cprintf(ch, "Your eyes begin to glow dimly.\n\r");
				act("$n's eyes begin to glow dimly.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
  				check_improve(ch, gsn_fury, TRUE, 2);
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				return;
			case 2:
				chance = get_skill(ch, gsn_fury);
        			if (number_percent() > chance)
        			{
                			Cprintf(ch, "You fail to increase your fury.\n\r");
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
                			check_improve(ch, gsn_fury, FALSE, 2);
                			return;
        			}
				ch->move -= 15;
				paf->duration = 5;
				paf->modifier = 3;
				Cprintf(ch, "Your fury grows!\n\r");
				Cprintf(ch, "Your eyes begin to glow brightly!\n\r");
				act("$n's eyes begin to glow with fury!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
  				check_improve(ch, gsn_fury, TRUE, 2);
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				return;
			case 3:
				chance = get_skill(ch, gsn_fury);
        			if (number_percent() > chance)
        			{
                			Cprintf(ch, "You fail to increase your fury.\n\r");
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
                			check_improve(ch, gsn_fury, FALSE, 2);
                			return;
        			}
				ch->move -= 15;
				paf->duration = 5;
				paf->modifier = 4;
				Cprintf(ch, "Your fury grows!\n\r");
				Cprintf(ch, "A faint glow surrounds your whole body.\n\r");
				act("$n's whole body seems to glow faintly with fury.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
  				check_improve(ch, gsn_fury, TRUE, 2);
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				return;
			case 4:
				chance = get_skill(ch, gsn_fury);
        			if (number_percent() > chance)
        			{
                			Cprintf(ch, "You fail to increase your fury.\n\r");
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
                			check_improve(ch, gsn_fury, FALSE, 2);
                			return;
        			}
				ch->move -= 15;
				paf->duration = 5;
				paf->modifier = 5;
				Cprintf(ch, "Your fury grows!\n\r");
				Cprintf(ch, "Your whole body glows brightly!\n\r");
				act("$n's whole body glows brightly with fury!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
  				check_improve(ch, gsn_fury, TRUE, 2);
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				return;
			case 5:
				Cprintf(ch, "Your fury is already at maximum.\n\r");
				return;
		}

        }


 	chance = get_skill(ch, gsn_fury);
        if (number_percent() > chance)
        {
                Cprintf(ch, "You fail to focus your fury.\n\r");
		WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
                check_improve(ch, gsn_fury, FALSE, 2);
                return;
        }
	ch->move -= 15;
        af.where = TO_AFFECTS;
        af.type = gsn_fury;
        af.level = ch->level;
        af.duration = 5;
        af.modifier = 1;
        af.bitvector = 0;
        af.location = APPLY_NONE;
        affect_to_char(ch, &af);
        check_improve(ch, gsn_fury, TRUE, 2);
        Cprintf(ch, "You focus your fury!\n\r");
	Cprintf(ch, "Your eyes begin to glow faintly.\n\r");
	act("$n's eyes begin to glow faintly.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
        return;
}


void
do_trance(CHAR_DATA * ch, char *argument)
{
        AFFECT_DATA af, *paf;
        int chance;

        if (IS_NPC(ch))
                return;

        if (get_skill(ch, gsn_trance) < 1)
        {
                Cprintf(ch, "You don't know the difference between a trance and a light nap.\n\r");
                return;
        }

	if (ch->fighting == NULL) {
		Cprintf(ch, "You need to be in combat to enter this kind of trance.\n\r");
		return;
	}

	if (ch->mana < 15) {
		Cprintf(ch, "You don't have enough energy left to enter a trance.\n\r");
		return;
	}


        af.where = TO_AFFECTS;
        af.type = gsn_trance;
        af.level = ch->level;
        af.duration = 5;
        af.bitvector = 0;
        af.location = APPLY_SAVES;

        if ((paf = affect_find(ch->affected, gsn_trance)) != NULL)
        {
		switch(paf->modifier)
		{
			case -1:
				chance = get_skill(ch, gsn_trance);
        			if (number_percent() > chance)
        			{
                			Cprintf(ch, "You fail to deepen your trance.\n\r");
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
                			check_improve(ch, gsn_trance, FALSE, 2);
                			return;
        			}
				ch->mana -= 15;
				af.modifier = -2;
				affect_remove(ch, paf);
				affect_to_char(ch, &af);
				Cprintf(ch, "Your trance deepens!\n\r");
				Cprintf(ch, "You close your eyes and continue to fight.\n\r");
				act("$n begins to fight with their eyes closed.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
  				check_improve(ch, gsn_trance, TRUE, 2);
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				return;
			case -2:
				chance = get_skill(ch, gsn_trance);
        			if (number_percent() > chance)
        			{
                			Cprintf(ch, "You fail to deepen your trance.\n\r");
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
                			check_improve(ch, gsn_trance, FALSE, 2);
                			return;
        			}
				ch->mana -= 15;
				af.modifier = -3;
				affect_remove(ch, paf);
				affect_to_char(ch, &af);
				Cprintf(ch, "Your trance deepens!\n\r");
				Cprintf(ch, "Your feet are no longer touching the ground.\n\r");
				act("$n begins to move as if they aren't touching the ground anymore!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
  				check_improve(ch, gsn_trance, TRUE, 2);
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				return;
			case -3:
				chance = get_skill(ch, gsn_trance);
        			if (number_percent() > chance)
        			{
                			Cprintf(ch, "You fail to deepen your trance.\n\r");
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
                			check_improve(ch, gsn_trance, FALSE, 2);
                			return;
        			}
				ch->mana -= 15;
				af.modifier = -4;
				affect_remove(ch, paf);
				affect_to_char(ch, &af);
				Cprintf(ch, "Your trance deepens!\n\r");
				Cprintf(ch, "Your movements begin to leave a blurry trail.\n\r");
				act("$n's movements begin to leave a blurry trail.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
  				check_improve(ch, gsn_trance, TRUE, 2);
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				return;
			case -4:
				chance = get_skill(ch, gsn_trance);
        			if (number_percent() > chance)
        			{
                			Cprintf(ch, "You fail to increase your trance.\n\r");
					WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
                			check_improve(ch, gsn_trance, FALSE, 2);
                			return;
        			}
				ch->mana -= 15;
				af.modifier = -5;
				affect_remove(ch, paf);
				affect_to_char(ch, &af);
				Cprintf(ch, "Your trance deepens!\n\r");
				Cprintf(ch, "Your body phases out of touch with reality!\n\r");
				act("$n' phases out of touch with reality!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
  				check_improve(ch, gsn_trance, TRUE, 2);
				WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
				return;
			case -5:
				Cprintf(ch, "Your trance is already at maximum.\n\r");
				return;
		}

        }


 	chance = get_skill(ch, gsn_trance);
        if (number_percent() > chance)
        {
                Cprintf(ch, "You fail to enter a trance.\n\r");
		WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
                check_improve(ch, gsn_trance, FALSE, 2);
                return;
        }
        af.modifier = -1;
	ch->mana -= 15;
        affect_to_char(ch, &af);
        check_improve(ch, gsn_trance, TRUE, 2);
        Cprintf(ch, "Your mind enters a trance, protecting you from magical damage!\n\r");
	act("$n enters a trance to guard against magic.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	WAIT_STATE(ch, 1 * PULSE_VIOLENCE);

        return;
}

void
do_death_blow(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int victim_dead = FALSE;
	int chance = 0;
	int dam = 0;
	
    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
            victim = ch->fighting;
            if (victim == NULL)
            {
                    Cprintf(ch, "But you aren't fighting anyone!\n\r");
                    return;
            }
    }
    else if ((victim = get_char_room(ch, arg)) == NULL)
    {
            Cprintf(ch, "They aren't here.\n\r");
            return;
    }

    if (victim == ch)
    {
            Cprintf(ch, "How can you assassinate up on yourself?\n\r");
            return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
            act("But $N is such a good friend!", ch, NULL, victim, TO_CHAR, POS_RESTING);
            return;
    }

    if (is_safe(ch, victim)) {
		return;
	}
   
    if (IS_NPC(victim) &&
            victim->fighting != NULL &&
            !is_same_group(ch, victim->fighting))
    {
            Cprintf(ch, "Kill stealing is not permitted.\n\r");
            return;
    }

    if (get_eq_char(ch, WEAR_WIELD) == NULL && get_eq_char(ch, WEAR_DUAL) == NULL)
	{
		Cprintf(ch, "You need to wield a weapon to assassinate anyone.\n\r");
		return;
	}

    if(is_affected(ch, gsn_death_blow)) {
	    Cprintf(ch, "You're not ready to deal death again just yet.\n\r");
	    return;
    }

    if (ch->mana < 50) {
		Cprintf(ch, "You don't have the energy to assasinate anyone.\n\r");
		return;
	}

    af.where = TO_AFFECTS;
    af.type = gsn_death_blow;
    af.level = ch->level;
    af.duration = 5;
    af.modifier = 0;
    af.bitvector = 0;
    af.location = APPLY_NONE;
    affect_to_char(ch, &af);

	chance = get_skill(ch, gsn_death_blow) * 9 / 10;
	if(number_percent() > chance) {
		check_improve(ch, gsn_death_blow, FALSE, 1);
		ch->mana -= 25;
		damage(ch, victim, 0, gsn_death_blow, DAM_VORPAL, TRUE, TYPE_SKILL);
		return;
	}

	ch->mana -= 50;
	check_improve(ch, gsn_death_blow, TRUE, 1);

	/* Now for big nasty effect. Different mobs and PCs. */
	Cprintf(ch, "You use your knowledge to strike in exactly the right place!\n\r");
	act("$n grins evilly and unleashes a precision death blow!",
		ch, NULL, NULL, TO_ROOM, POS_RESTING);

	WAIT_STATE(ch, PULSE_VIOLENCE);
	
	dam = (ch->level * 4 + dice(get_curr_stat(ch, STAT_INT), 6));
	victim_dead = damage(ch, victim, dam, gsn_death_blow, DAM_VORPAL, TRUE, TYPE_SKILL);
	if(victim_dead)
		return;

    // On mobs possible slay
	if(IS_NPC(victim)) {
		if(number_percent() <= 10 && !IS_SET(victim->imm_flags, IMM_VORPAL)) {
			victim->hit = 0;
			act("$n is utterly {RSLAIN{x by $N!!!", victim, NULL, ch, TO_ROOM, POS_RESTING);
			damage(ch, victim, 10, 0, DAM_NONE, FALSE, TYPE_MAGIC);
		}
	}
    // On players possible paralyze
    else {
        if(number_percent() <= 35) {
	        act("$n's teeters unsteadily for a moment before collapsing from the shock...",
		        victim, NULL, NULL, TO_ROOM, POS_RESTING);

            WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
	        DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
        }
    }
}

void
do_boost(CHAR_DATA *ch, char *argument)
{
	AFFECT_DATA af;

	if (get_skill(ch, gsn_boost) < 1)
	{
		Cprintf(ch, "You grunt fiercely, but you just sound funny.\n\r");
		return;
	}
	if(!ch->fighting) {
		Cprintf(ch, "But you aren't fighting anyone!\n\r");
		return;
	}

	if(ch->move < 10) {
		Cprintf(ch, "You are too tired to increase your power.\n\r");
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	if(number_percent() > get_skill(ch, gsn_boost)) {
		ch->move -= 5;
		check_improve(ch, gsn_boost, FALSE, 4);
		Cprintf(ch, "You fail to increase your power.\n\r");
		return;
	}

	af.where = TO_AFFECTS;
	af.type = gsn_boost;
	af.level = ch->level;
	af.duration = 1;
	af.modifier = 9 + (ch->level / 3);
	af.location = APPLY_HITROLL;
	af.bitvector = 0;

	affect_to_char(ch, &af);

	af.location = APPLY_DAMROLL;
	affect_to_char(ch, &af);

	ch->move -= 10;
	check_improve(ch, gsn_boost, TRUE, 4);

	Cprintf(ch, "With a powerful kiai, you attack with increased power!\n\r");
	act("Shouting a powerful kiai, $n attacks with renewed force!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	return;
}

void
do_chakra(CHAR_DATA *ch, char *argument)
{
	int healed;

	if(ch->can_lay != 0) {
		Cprintf(ch, "You are too tired.\n\r");
		return;
	}

	if(number_percent() < get_skill(ch, gsn_chakra)) {
		ch->can_lay = 1;
		check_improve(ch, gsn_chakra, TRUE, 4);
		act("Your chakras glow warmly and heal your wounds!", ch, NULL, NULL, TO_CHAR, POS_RESTING);
		act("$n's chakras glow brightly as $s wounds close.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		healed = 16 * (get_curr_stat(ch, STAT_CON));
		healed = healed * ch->level / 51.0;
		if(!is_affected(ch, gsn_dissolution))
        		ch->hit += healed;
		if (ch->hit > MAX_HP(ch))
		        ch->hit = MAX_HP(ch);
		update_pos(ch);
		Cprintf(ch, "A warm feeling fills your body.\n\r");
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
	}
	else {
		check_improve(ch, gsn_chakra, FALSE, 4);
		act("You fail to control your chakras. Nothing happens.", ch, NULL, NULL, TO_CHAR, POS_RESTING);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
	}

	return;
}

void
do_dragon_kick(CHAR_DATA *ch, char *argument) 
{
	CHAR_DATA *victim = NULL;
	char arg[MAX_INPUT_LENGTH];
	int strikes = 0;
	AFFECT_DATA af;
	int victim_dead = FALSE;
	int hit_chance = 0;
	int i = 0;
	int dam = 0;

	one_argument(argument, arg);

	if(get_skill(ch, gsn_dragon_kick) < 1) {
		Cprintf(ch, "Your puny limbs cannot be used in this way.\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "You are not fighting anyone.\n\r");
		return;
		}
	}
	else
	{
		if ((victim = get_char_room(ch,arg)) == NULL)
		{
			Cprintf(ch, "They're not here.\n\r");
		return;
		}
	}

	if (is_safe(ch, victim))
	return;

	if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch, victim->fighting))
{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
        	return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
        {
                act("But $N is such a good friend!", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	if (IS_AFFECTED(ch, AFF_MARTIAL_ARTS))
	{
		Cprintf(ch, "You need to rest a moment before launching another martial arts attack.\n\r");
		return;
	}

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

	// Determine chance to hit:
	hit_chance = get_attack_rating(ch, NULL);
	hit_chance = (hit_chance * get_skill(ch, gsn_dragon_kick) / 100.0);
	hit_chance -= get_defense_rating(victim, DAM_FIRE);

	strikes = number_range(2, 4);

	for(i = 0; i < strikes; i++) {

		if(number_percent() > hit_chance) {
			damage(ch, victim, 0, gsn_dragon_kick, DAM_FIRE, TRUE, TYPE_SKILL);
			continue;
}

		dam = (ch->level / 2) + dice(ch->level / 10, ch->level / 5 + 5);
		victim_dead = damage(ch, victim, dam, gsn_dragon_kick, DAM_FIRE, TRUE, TYPE_SKILL);

		if(victim_dead)
			break;

		if(number_percent() < 25) {
			DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
			WAIT_STATE(victim, dice(2, 8));
			act("$n's dragon kick knocks you to the ground!", ch, NULL, victim, TO_VICT, POS_RESTING);
	        	act("Your dragon kick knocks $N to the ground!", ch, NULL, victim, TO_CHAR, POS_RESTING);
        		act("$n dragon kick knocks $N to the ground.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
			check_improve(ch, gsn_dragon_kick, TRUE, 3);	
	}

	}

	af.where = TO_AFFECTS;
	af.type = gsn_dragon_kick;
	af.level = ch->level;
	af.duration = 0;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_MARTIAL_ARTS;
	affect_to_char(ch, &af);
}

void
do_eagle_claw(CHAR_DATA *ch, char *argument)
{
        CHAR_DATA *victim = NULL;
	char arg[MAX_INPUT_LENGTH];
        int strikes = 0;
	AFFECT_DATA af;
        int victim_dead = FALSE;
        int hit_chance = 0;
        int i = 0;
        int dam = 0;

        one_argument(argument, arg);

        if(get_skill(ch, gsn_eagle_claw) < 1) {
                Cprintf(ch, "Your puny limbs cannot be used in this way.\n\r");
                return;
		}

        if (arg[0] == '\0')
        {
                victim = ch->fighting;
                if (victim == NULL)
                {
                        Cprintf(ch, "You are not fighting anyone.\n\r");
                        return;
		}
		}
        else
        {
                if ((victim = get_char_room(ch,arg)) == NULL)
                {
                        Cprintf(ch, "They're not here.\n\r");
                        return;
		}
		}

        if (is_safe(ch, victim))
		return;

        if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch, victim->fighting))
        {
                Cprintf(ch, "Kill stealing is not permitted.\n\r");
                return;
	}

        if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
        {
                act("But $N is such a good friend!", ch, NULL, victim, TO_CHAR,POS_RESTING);
		return;
	}

        if (IS_AFFECTED(ch, AFF_MARTIAL_ARTS))
        {
                Cprintf(ch, "You need to rest a moment before launching another martial arts attack.\n\r");
			return;
		}

        WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

        // Determine chance to hit:
        hit_chance = get_attack_rating(ch, NULL);
        hit_chance = (hit_chance * get_skill(ch, gsn_eagle_claw) / 100.0);
        hit_chance -= get_defense_rating(victim, DAM_LIGHTNING);

        strikes = number_range(2, 4);
        for(i = 0; i < strikes; i++) {
                if(number_percent() < hit_chance) {
    			dam += (ch->level / 2); 
                	dam += dice(ch->level / 10, ch->level / 5 + 5);

	}
		}

	victim_dead = damage(ch, victim, dam, gsn_eagle_claw, DAM_LIGHTNING, TRUE, TYPE_SKILL);

        if(!victim_dead) {

		if(number_percent() < 50) {

                        act("$n's eagle claw shatters your defenses!", ch, NULL, victim, TO_VICT, POS_RESTING);
                        act("Your eagle claw shatters $N's defenses!", ch, NULL, victim, TO_CHAR, POS_RESTING);

                        act("$n eagle claw shatters $N's defenses!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
                        check_improve(ch, gsn_eagle_claw, TRUE, 3);

		af.where = TO_AFFECTS;
		        af.type = gsn_eagle_claw;
		af.level = ch->level;
		        af.duration = 0;
		af.location = APPLY_AC;
		        af.modifier = ch->level * 3;
		af.bitvector = 0;
		        affect_to_char(victim, &af);
	}

		}

		af.where = TO_AFFECTS;
        af.type = gsn_eagle_claw;
		af.level = ch->level;
        af.duration = 0;
        af.location = APPLY_NONE;
		af.modifier = 0;
        af.bitvector = AFF_MARTIAL_ARTS;
		affect_to_char(ch, &af);
	}

void
do_choke_hold(CHAR_DATA *ch, char *argument)
{
        CHAR_DATA *victim = NULL;
	char arg[MAX_INPUT_LENGTH];
        int strikes = 0;
	AFFECT_DATA af;
        int hit_chance = 0;
        int i = 0;
	int hits = 0;

	one_argument(argument, arg);

        if(get_skill(ch, gsn_choke_hold) < 1) {
                Cprintf(ch, "Your puny limbs cannot be used in this way.\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "You are not fighting anyone.\n\r");
			return;
		}
	}
	else
	{
		if ((victim = get_char_room(ch,arg)) == NULL)
		{
			Cprintf(ch, "They're not here.\n\r");
			return;
		}
	}

	if (is_safe(ch, victim))
		return;

        if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch, 

victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}
	
	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
        {
                act("But $N is such a good friend!", ch, NULL, victim, 

TO_CHAR,POS_RESTING);
		return;
	}

        if (IS_AFFECTED(ch, AFF_MARTIAL_ARTS))
	{
                Cprintf(ch, "You need to rest a moment before launching another martial arts attack.\n\r");
		return;
	}

        WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

        // Determine chance to hit:
        hit_chance = get_attack_rating(ch, NULL);
        hit_chance = (hit_chance * get_skill(ch, gsn_choke_hold) / 100.0);
        hit_chance -= get_defense_rating(victim, DAM_DROWNING);

        strikes = number_range(2, 4);
        for(i = 0; i < strikes; i++) {
                if(number_percent() < hit_chance) {
    			hits++;
                }                
	}

	act("$n begins to choke the life from you!", ch, NULL, victim, TO_VICT, 

POS_RESTING);
        act("You begin to choke the life out of $N!", ch, NULL, victim, TO_CHAR, 

POS_RESTING);

        act("$n puts $N into a choke hold!", ch, NULL, victim, TO_NOTVICT, POS_RESTING);

        damage(ch, victim, 0, gsn_choke_hold, DAM_DROWNING, FALSE, TYPE_SKILL);
        
	check_improve(ch, gsn_choke_hold, TRUE, 3);

	// Start choking victim
	af.where = TO_AFFECTS;
	af.type = gsn_choke_hold;
	af.level = ch->level;
	af.duration = hits;
	af.location = APPLY_HITROLL;
	af.modifier = -6;
	af.bitvector = 0;
	affect_to_char(victim, &af);
        
	// Put wait timer on monk
        af.duration = 0;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_MARTIAL_ARTS;

        affect_to_char(ch, &af);
}

void
do_demon_fist(CHAR_DATA *ch, char *argument)
{
        CHAR_DATA *victim = NULL;
        char arg[MAX_INPUT_LENGTH];
        int strikes = 0;
        AFFECT_DATA af;
        int hit_chance = 0;
        int i = 0;
	int dam = 0;
	int victim_dead = FALSE;
	int hits = 0;

        one_argument(argument, arg);

        if(get_skill(ch, gsn_demon_fist) < 1) {
                Cprintf(ch, "Your puny limbs cannot be used in this way.\n\r");
                return;
        }

        if (arg[0] == '\0')
        {
                victim = ch->fighting;
                if (victim == NULL)
                {
                        Cprintf(ch, "You are not fighting anyone.\n\r");
                        return;
                }
        }
        else
        {
                if ((victim = get_char_room(ch,arg)) == NULL)
                {
                        Cprintf(ch, "They're not here.\n\r");
                        return;
                }
        }

        if (is_safe(ch, victim))
                return;

        if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch, victim->fighting))
        {
                Cprintf(ch, "Kill stealing is not permitted.\n\r");
                return;
        }

        if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
        {
                act("But $N is such a good friend!", ch, NULL, victim, TO_CHAR,POS_RESTING);
                return;
        }

        if (IS_AFFECTED(ch, AFF_MARTIAL_ARTS))
        {
                Cprintf(ch, "You need to rest a moment before launching another martial arts attack.\n\r");
                return;
        }

        WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

        // Determine chance to hit:
        hit_chance = get_attack_rating(ch, NULL);
        hit_chance = (hit_chance * get_skill(ch, gsn_demon_fist) / 100.0);
        hit_chance -= get_defense_rating(victim, DAM_NEGATIVE);

        strikes = number_range(2, 4);
        for(i = 0; i < strikes; i++) {
                if(number_percent() < hit_chance) {
			dam += (ch->level / 2);
		        dam += dice(ch->level / 10, ch->level / 5 + 5);
    			hits++;
                }                
        }

	victim_dead = damage(ch, victim, dam, gsn_demon_fist, DAM_NEGATIVE, TRUE, TYPE_SKILL);

	if(!victim_dead && number_percent() < hits * 20) {
		act("$n's demonic fist makes you very uncomfortable.", ch, NULL, victim, TO_VICT, POS_RESTING);
        	act("$N looks very uncomfortable.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        	act("$N looks very uncomfortable.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
        
		check_improve(ch, gsn_demon_fist, TRUE, 3);

		af.where = TO_AFFECTS;
		af.type = gsn_curse;
		af.level = ch->level;
		af.duration = 0;
		af.location = APPLY_HITROLL;
		af.modifier = -5;
		af.bitvector = AFF_CURSE;
		affect_to_char(victim, &af);
        }
	// Put wait timer on monk
	af.type = gsn_demon_fist;
        af.duration = 0;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_MARTIAL_ARTS;

        affect_to_char(ch, &af);
}

void
do_shadow_walk(CHAR_DATA * ch, char *argument)
{
	AFFECT_DATA af;
	int chance;

	if (IS_NPC(ch))
		return;

	if (get_skill(ch, gsn_shadow_walk) < 1)
	{
		Cprintf(ch, "You're too light hearted to shadow walk.\n\r");
		return;
	}

	if (is_affected(ch, gsn_shadow_walk))
	{
		Cprintf(ch, "You are already shadow walking.\n\r");
		return;
	}

	if (ch->move < 50) {
		Cprintf(ch, "You are too tired to shadow walk right now.\n\r");
		return;
	}

	ch->move -= 50;
	WAIT_STATE(ch, PULSE_VIOLENCE / 2);

	chance = get_skill(ch, gsn_shadow_walk);
	if (number_percent() > chance)
	{
		Cprintf(ch, "You fail at shadow walking.\n\r");
		check_improve(ch, gsn_shadow_walk, FALSE, 2);
		return;
	}

	Cprintf(ch, "Your body darkens as you begin to shadow walk.\n\r");
	check_improve(ch, gsn_shadow_walk, TRUE, 2);

	af.where = TO_AFFECTS;
	af.type = gsn_shadow_walk;
	af.level = ch->level;
	af.duration = 0;
	af.modifier = 0;
	af.bitvector = 0;
	af.location = APPLY_NONE;
	affect_to_char(ch, &af);
	
	return;
}

void
do_stance(CHAR_DATA *ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	AFFECT_DATA af;
	int found = FALSE;
	int previous_stance_sn = 0;
	int modifier = 0;

	argument = one_argument(argument, arg);

	if(arg[0] == '\0') {
		Cprintf(ch, "You may use the following stances:\n\r");
		if(get_skill(ch, gsn_stance_turtle) > 0) {
			Cprintf(ch, "Turtle: Tough stance, hand to hand crushes, increases hp and resistance.\n\r"); found = TRUE;
		}
		if(get_skill(ch, gsn_stance_tiger) >  0) {
			Cprintf(ch, "Tiger:  Offense stance, hand to hand claws, increases hit/dam, dodge rate down.\n\r"); found = TRUE;
		}
		if(get_skill(ch, gsn_stance_mantis) > 0) {
			Cprintf(ch, "Mantis: Defense stance, hand to hand thrusts, defense up, attack down.\n\r"); found = TRUE;
		}
		if(get_skill(ch, gsn_stance_shadow) > 0) {
			Cprintf(ch, "Shadow: Elusive stance, unseen while in combat, but vulnerable to magic.\n\r"); found = TRUE;
		}
		if(get_skill(ch, gsn_stance_kensai) > 0) {
			Cprintf(ch, "Kensai: Accurate stance, strike more often, but unable to flee.\n\r"); found = TRUE;
		}
		if(found == FALSE)
			Cprintf(ch, "None.\n\r");

		return;
	}

	if(ch->move < 50) {
		Cprintf(ch, "You are too tired to assume a new stance.\n\r");
		return;
	}

	// Remove the current stance, but remember which one it was.
	// Using the same stance repeatedly will strengthen it.
	if(is_affected(ch, gsn_stance_turtle)) {
		previous_stance_sn = gsn_stance_turtle;
		affect_strip(ch, gsn_stance_turtle);
	}
	if(is_affected(ch, gsn_stance_tiger)) {
		previous_stance_sn = gsn_stance_tiger;
                affect_strip(ch, gsn_stance_tiger);
	}
	if(is_affected(ch, gsn_stance_mantis)) {
		previous_stance_sn = gsn_stance_mantis;
		affect_strip(ch, gsn_stance_mantis);
	}
	if(is_affected(ch, gsn_stance_shadow)) {
		previous_stance_sn = gsn_stance_shadow;
		affect_strip(ch, gsn_stance_shadow);
	}
	if(is_affected(ch, gsn_stance_kensai)) {
		previous_stance_sn = gsn_stance_kensai;
		affect_strip(ch, gsn_stance_kensai);
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	if(!str_prefix(arg, "turtle")) {
		if(get_skill(ch, gsn_stance_turtle) < 1) {
			Cprintf(ch, "You don't know that fighting stance. Use no arguments to see a list.\n\r");
			return;
		}

		ch->move -= 50;

		af.where = TO_AFFECTS;
		af.type = gsn_stance_turtle;
		af.level = ch->level;
		af.duration = 24;
		af.modifier = 2 * get_skill(ch, gsn_stance_turtle);
		af.location = APPLY_HIT;
		af.bitvector = 0;

		affect_to_char(ch, &af);

		modifier = number_range(1, 5);
		if(previous_stance_sn == gsn_stance_turtle) {
			modifier++;
		}	
		af.modifier = UMIN(modifier, 5);
		af.location = APPLY_DAMAGE_REDUCE;

		affect_to_char(ch, &af);

		Cprintf(ch, "You cross your arms and assume a new fighting stance!\n\r");
		Cprintf(ch, "You harden your fists and enhance your vigor! Stance: Turtle!\n\r");
		act("$n crosses $s arms and assumes a different fighting stance.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		act("$n reveals Stance: Turtle.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	else if(!str_prefix(arg, "tiger")) {
		if(get_skill(ch, gsn_stance_tiger) < 1) {
			Cprintf(ch, "You don't know that fighting stance. Use no arguments to see a list.\n\r");
			return;
		}
		ch->move -= 50;

		af.where = TO_AFFECTS;
		af.type = gsn_stance_tiger;
		af.level = ch->level;
		af.duration = 24;

		modifier = number_range(get_skill(ch, gsn_stance_tiger) / 12,
                             get_skill(ch, gsn_stance_tiger) / 6);
                if(previous_stance_sn == gsn_stance_tiger) {
			modifier = modifier + 3;
		}
		af.modifier = UMIN(modifier, 16);
		af.location = APPLY_HITROLL;
		af.bitvector = 0;

		affect_to_char(ch, &af);

		af.location = APPLY_DAMROLL;
		affect_to_char(ch, &af);

		Cprintf(ch, "You cross your arms and assume a new fighting stance!\n\r");
		Cprintf(ch, "Palms raised like talons, your ferocity is enhanced! Stance: Tiger!\n\r");
		act("$n crosses $s arms and assumes a different fighting stance.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		act("$n reveals Stance: Tiger.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	else if(!str_prefix(arg, "mantis")) {
		if(get_skill(ch, gsn_stance_mantis) < 1) {
			Cprintf(ch, "You don't know that fighting stance. Use no arguments to see a list.\n\r");
			return;
		}
		ch->move -= 50;

		af.where = TO_AFFECTS;
		af.type = gsn_stance_mantis;
		af.level = ch->level;
		af.duration = 24;
		af.modifier = 0 - get_skill(ch, gsn_stance_mantis) / 2;
		af.location = APPLY_AC;
		af.bitvector = 0;

		affect_to_char(ch, &af);

		Cprintf(ch, "You cross your arms and assume a new fighting stance!\n\r");
		Cprintf(ch, "You position your arms to block any attacks! Stance: Mantis!\n\r");
		act("$n crosses $s arms and assumes a different fighting stance.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		act("$n reveals Stance: Mantis.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	else if(!str_prefix(arg, "shadow")) {
		if(get_skill(ch, gsn_stance_shadow) < 1) {
			Cprintf(ch, "You don't know that fighting stance. Use no arguments to see a list.\n\r");
			return;
		}
		if(IS_AFFECTED(ch, AFF_CHARM)) {
			Cprintf(ch, "You can't escape into the shadows while charmed.\n\r");
			return;
		}
		ch->move -= 50;

		af.where = TO_AFFECTS;
		af.type = gsn_stance_shadow;
		af.level = ch->level;
		af.duration = 24;
		af.modifier = 10;
		af.location = APPLY_SAVING_SPELL;
		af.bitvector = 0;

		affect_to_char(ch, &af);

		Cprintf(ch, "You cross your arms and assume a new fighting stance!\n\r");
		Cprintf(ch, "You fade away and strike from the shadows! Stance: Shadow!\n\r");
		act("$n crosses $s arms and assumes a different fighting stance.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		act("$n vanishes into the shadows! Stance: Shadow.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

		die_follower(ch);

		return;
	}

	else if(!str_prefix(arg, "kensai")) {
		if(get_skill(ch, gsn_stance_kensai) < 1) {
			Cprintf(ch, "You don't know that fighting stance. Use no arguments to see a list.\n\r");
			return;
		}
		ch->move -= 50;

		af.where = TO_AFFECTS;
		af.type = gsn_stance_kensai;
		af.level = ch->level;
		af.duration = 24;
		af.modifier = get_skill(ch, gsn_stance_kensai) / 10;
		af.location = APPLY_HITROLL;
		af.bitvector = 0;

		affect_to_char(ch, &af);

		af.location = APPLY_ATTACK_SPEED;
		af.modifier = 0 - (get_skill(ch, gsn_stance_kensai) / 10);

		affect_to_char(ch, &af);

		Cprintf(ch, "You cross your arms and assume a new fighting stance!\n\r");
		Cprintf(ch, "You lock onto your target intensely. Stance: Kensai!\n\r");
		act("$n crosses $s arms and assumes a different fighting stance.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		act("$n locks onto $s target. Stance: Kensai.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	else {
		Cprintf(ch, "You don't know that fighting stance. Use no arguments to see a list.\n\r");
		return;
	}
	}

void
do_pain_touch(CHAR_DATA *ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim = NULL;
	AFFECT_DATA af;
	int chance;

	one_argument(argument, arg);

	chance = get_skill(ch, gsn_pain_touch);

	if(chance < 1) {
		Cprintf(ch, "You touch your foe vigorously, but are you causing pain or pleasure?\n\r");
		return;
}

	if (arg[0] == '\0')
	{
		victim = ch->fighting;
		if (victim == NULL)
		{
			Cprintf(ch, "You are not fighting anyone.\n\r");
			return;
		}
	}
	else
	{
		if ((victim = get_char_room(ch,arg)) == NULL)
		{
			Cprintf(ch, "They're not here.\n\r");
		return;
	}
	}

	if (is_safe(ch, victim))
		return;

	if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch, victim->fighting))
	{
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
        {
                act("But $N is such a good friend!", ch, NULL, victim, TO_CHAR, POS_RESTING);
                return;
        }                          

	if (victim == ch)
	{
		Cprintf(ch, "That's a little too masochistic for you.\n\r");
		return;
	}

	if (is_affected(victim, gsn_pain_touch))
	{
		act("$e is already crippled by pain.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		damage(ch, victim, 0, gsn_pain_touch, DAM_NONE, FALSE, TYPE_SKILL);
		return;
	}

	if(number_percent() > get_skill(ch, gsn_pain_touch)) {
                Cprintf(ch, "You fail to find a pressure point.\n\r");
                damage(ch, victim, 0, gsn_pain_touch, DAM_NONE, FALSE, TYPE_SKILL);
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                check_improve(ch, gsn_pain_touch, FALSE, 3);
                return;
        }                              

	check_killer(ch, victim);
	WAIT_STATE(ch, PULSE_VIOLENCE * 2);
	damage(ch, victim, 0, gsn_pain_touch, DAM_NONE, FALSE, TYPE_SKILL);

	if (saving_throw(ch, victim, gsn_pain_touch, ch->level + 4, SAVE_NORMAL, STAT_CON, DAM_PIERCE))
	{
		Cprintf(ch, "You attempt to induce pain, but failed.\n\r");
		Cprintf(victim, "Your nerves tingle as %s touches you, but nothing happens.\n\r", ch->name);
		act("$n touches $N vigorously, but nothing happens.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		check_improve(ch, gsn_pain_touch, FALSE, 3);
			return;
		}

		af.where = TO_AFFECTS;
	af.type = gsn_pain_touch;
		af.level = ch->level;
	af.location = APPLY_HITROLL;
	af.modifier = -4;
	af.duration = 1 + (ch->level / 8);
		af.bitvector = 0;

	affect_to_char(victim, &af);
	Cprintf(victim, "You are crippled by pain as %s strikes your nerves!\n\r", ch->name);
	act("$n is suddenly crippled by pain!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	check_improve(ch, gsn_pain_touch, TRUE, 3);
	return;
}

void
chi_ei(CHAR_DATA *ch) 
{
	AFFECT_DATA af;
	AFFECT_DATA *paf = NULL;

	if(get_skill(ch, gsn_chi_ei) < 1) {
		Cprintf(ch, "The ability to control chi is beyond you.\n\r");
		return;
	}

	if(ch->move < 30) {
		Cprintf(ch, "You don't have enough energy.\n\r");
		return;
}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	if(number_percent() > get_skill(ch, gsn_chi_ei)) {
		Cprintf(ch, "You fail to gather the necessary chi.\n\r");
		check_improve(ch, gsn_chi_ei, FALSE, 2);
		ch->move -= 15;
		return;
	}

	ch->move -= 30;

	if(is_affected(ch, gsn_chi_kaze)) {
		affect_strip(ch, gsn_chi_kaze);
	}

	paf = affect_find(ch->affected, gsn_chi_ei);

	// If already affected, strengthen the effect instead.
 	if(paf != NULL)  {
		act("You focus your chi even more intensely.",
                    ch, NULL, NULL, TO_CHAR, POS_RESTING);
		act("$n focuses ever more intensely.", ch, NULL, NULL,
                    TO_ROOM, POS_RESTING);
		check_improve(ch, gsn_chi_ei, TRUE, 2);

		// The "charge" is raised by 15% and a bit
		// Max charge is 5 * level.
		paf->modifier = UMIN
			((paf->modifier * 8 / 7) + dice(1, ch->level / 2), 
			 ch->level * 5);
		return;
	}

        act("You begin to chant and gather chi.", ch, NULL, NULL, TO_CHAR, POS_RESTING);
        act("$n begins to chant quietly.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	check_improve(ch, gsn_chi_ei, TRUE, 2);

	af.where = TO_AFFECTS;
        af.type = gsn_chi_ei;
        af.level = ch->level;
        af.duration = 0;
        af.location = APPLY_NONE;
        af.modifier = 20 + dice(2, ch->level);
        af.bitvector = 0;
        affect_to_char(ch, &af);
}

int
check_chi_ei(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int victim_dead = FALSE;
    AFFECT_DATA *paf = NULL;
    int chance = 0;

    if(IS_NPC(ch))
        return FALSE;

    paf = affect_find(ch->affected, gsn_chi_ei);
    if(paf == NULL)
        return FALSE;

    chance = paf->modifier / 10;
    if(number_percent() > chance)
        return FALSE;

    if (IS_AFFECTED(ch, AFF_SLOW))
        return FALSE;

    act("$n focuses $s chi in a vengeful strike!", ch, NULL, victim, TO_VICT, POS_RESTING);
    act("You unleash your chi against $N in vengeance!", ch, NULL, victim, TO_CHAR, POS_RESTING);
    act("$n focuses $s chi vengefully.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);

    check_improve(ch, gsn_chi_ei, TRUE, 3);
    victim_dead = damage(ch, victim, paf->modifier, gsn_chi_ei, DAM_NONE, TRUE, TYPE_SKILL);

    // The mirrored soul may wear off if overused.
    chance = paf->modifier / 5;
    if(number_percent() < chance) {
        Cprintf(ch, "You lose control and your chi dissipates.\n\r");
        affect_strip(ch, gsn_chi_ei);
    }
    return victim_dead;
}

void
chi_kaze(CHAR_DATA *ch)
        {
    AFFECT_DATA af;

    if(get_skill(ch, gsn_chi_kaze) < 1) {
        Cprintf(ch, "You try to control the wind but end up breaking it.\n\r");
                return;
        }

    if(ch->move < 75) {
        Cprintf(ch, "You don't have enough energy.\n\r");
                return;
        }

    if(is_affected(ch, gsn_chi_kaze)) {
        Cprintf(ch, "You are already moving like the wind!\n\r");
                return;
        }

    if(number_percent() > get_skill(ch, gsn_chi_kaze)) {
        Cprintf(ch, "You fail to gather the necessary chi.\n\r");
        WAIT_STATE(ch, PULSE_VIOLENCE);
        ch->move -= 37;
        check_improve(ch, gsn_chi_kaze, FALSE, 2);
	return;
}


    if(is_affected(ch, gsn_chi_ei)) {
	affect_strip(ch, gsn_chi_ei);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE);
    ch->move -= 75;
    check_improve(ch, gsn_chi_kaze, TRUE, 2);

    af.where = TO_AFFECTS;
    af.type = gsn_chi_kaze;
    af.level = ch->level;
    af.duration = 0;
    af.modifier = (ch->level / 5);
    af.location = APPLY_ATTACK_SPEED;
    af.bitvector = 0;

    affect_to_char(ch, &af);

    Cprintf(ch, "The wind picks up speed and begin to howl around you!\n\rYou begin to move like the wind!\n\r");
    act("$n picks up speed as the winds begin to howl.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

		return;
	}

void
do_chi(CHAR_DATA *ch, char *argument)
{
	char command[MAX_INPUT_LENGTH];
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, command);
	one_argument(argument, arg);

	// Check command.
	if(str_prefix(command, "ei")
	&& str_prefix(command, "kaze")) {
		Cprintf(ch, "Choose a valid chi ability.\n\r");
		return;
	}

	if(!str_prefix(command, "ei"))
		chi_ei(ch);
	else if (!str_prefix(command, "kaze"))
		chi_kaze(ch);

	return;
}

// Here we start some of the reclass skills

// Ninjitsu
// The way these work is by using up charms of different types.
// Each charm is a weapon item sold in various shops. Some charms
// could even be crafted. The v4 special field determines the
// vuln bits to set on the target... and looks wacky in lore_obj.
// Ninjitsu can miss.
// Syntax: ninjitsu 'object name' (target)

void
do_ninjitsu(CHAR_DATA *ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	AFFECT_DATA af;
	OBJ_DATA *charm;
	int chance = 0;
	int dam = 0;
	int i;
	int cost;
	int need_correct = FALSE;
	long corrected = 0;

	argument = one_argument(argument, arg1);
	one_argument(argument, arg2);

	if(get_skill(ch, gsn_ninjitsu) < 1) {
		Cprintf(ch, "You sing 'kung fu fighting' but nothing happens.\n\r");
		return;
	}

	cost = number_range(3, 7);
	if(ch->gold < 7) {
		Cprintf(ch, "Your purse is too light to use ninjitsu.\n\r");
		return;
	}
	ch->gold -= cost;

	// Get object from inventory
	charm = get_obj_carry(ch, arg1, ch);
	if(charm == NULL) {
		Cprintf(ch, "You aren't carrying that.\n\r");
		return;
	}
	if(charm->item_type != ITEM_CHARM) {
		Cprintf(ch, "That object is not a ninjitsu charm.\n\r");
		return;
	}
	if(charm->level > ch->level) {
		Cprintf(ch, "You are too low level to use this charm.\n\r");
		return;
	}

	// Get and check victim.
        if (arg2[0] == '\0')
        {
                victim = ch->fighting;
                if (victim == NULL)
                {
                        Cprintf(ch, "But you aren't fighting anyone!\n\r");
                        return;
                }
        }
        else if ((victim = get_char_room(ch, arg2)) == NULL)
        {
                Cprintf(ch, "They aren't here.\n\r");
                return;
        }

	// Check chance for success.
	chance = get_skill(ch, gsn_ninjitsu) / 2;
	chance += (get_curr_stat(ch, STAT_INT) * 2);
	chance += (ch->level - victim->level) * 2;

	WAIT_STATE(ch, PULSE_VIOLENCE);

	if(number_percent() > chance) {
		damage(ch, victim, 0, gsn_ninjitsu, DAM_NONE, TRUE, TYPE_MAGIC);
		check_improve(ch, gsn_ninjitsu, FALSE, 1);
		return;
	}

	dam = dice(charm->value[1], charm->value[2]);
	dam += (get_curr_stat(ch, STAT_WIS) * 4);

	damage(ch, victim, dam, gsn_ninjitsu, charm->value[3], TRUE, TYPE_MAGIC);
	check_improve(ch, gsn_ninjitsu, TRUE, 1);

	// If they are already vuln to some of the things, fix them
	// So they are not applied twice.
	// Also... don't counteract immunities!
	if(victim->imm_flags & charm->value[4]
	&& victim->vuln_flags & charm->value[4]) {
		need_correct = TRUE;
		corrected = charm->value[4] ^
        		(victim->imm_flags & charm->value[4]) ^
			(victim->vuln_flags & charm->value[4]);
	}
	else if(victim->vuln_flags & charm->value[4]) {
		need_correct = TRUE;
		corrected = charm->value[4] ^
			(victim->vuln_flags & charm->value[4]);
	}
	else if(victim->imm_flags & charm->value[4]) {
		need_correct = TRUE;
		corrected = charm->value[4] ^
			(victim->imm_flags & charm->value[4]);
	}

	// Set the vuln bit
	af.where = TO_VULN;
	af.type = gsn_ninjitsu;
	af.level = ch->level;
	af.duration = 1;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = (need_correct) ? corrected : charm->value[4];

	if((need_correct && corrected > 0)
	|| !need_correct)
		affect_to_char(victim, &af);

	// For reference
	af.where = TO_AFFECTS;
	af.modifier = (need_correct) ? corrected : charm->value[4];
	af.bitvector = 0;

	if((need_correct && corrected > 0)
	|| !need_correct)
		affect_to_char(victim, &af);

	// Now display the proper message
	i = 0;
	while(1) {
		if(vuln_flags[i].bit == 0)
			break;

		if((!need_correct && vuln_flags[i].bit & charm->value[4])
		|| (need_correct  && vuln_flags[i].bit & corrected)) {
			Cprintf(ch, "%s is now weak against %s!\n\r",
				IS_NPC(victim) ? victim->short_descr : victim->name,
				vuln_flags[i].name);
			Cprintf(victim, "You are marked with a weakness against %s!\n\r", vuln_flags[i].name);
			sprintf(buf, "$N is marked with a weakness against %s!",
				vuln_flags[i].name);
			act(buf, ch, NULL, victim, TO_ROOM, POS_RESTING);
		}
		i++;
	}

	// Determine chance for charm to break.
	chance = 100 - get_skill(ch, gsn_ninjitsu); 
	if(number_percent() < chance) {
		Cprintf(ch, "You charm is reduced to ashes.\n\r");
	extract_obj(charm);
	}	
	return;
}

// The way this works is each hit reduces the duration
// until it is zero, then strips the effect.
void
do_evasion(CHAR_DATA *ch, char *argument)
{
	AFFECT_DATA af;

	if(is_affected(ch, gsn_evasion)) {
		Cprintf(ch, "Your evasion is already enhanced.\n\r");
		return;
	}

	if(ch->move < 50) {
		Cprintf(ch, "You are too tired to evade anymore attacks.\n\r");
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE * 2);

	if(number_percent() > get_skill(ch, gsn_evasion)) {
		Cprintf(ch, "You fail to enhance your evasion.\n\r");
		ch->move -= 25;
		check_improve(ch, gsn_evasion, FALSE, 2);
		return;
	}

	ch->move -= 50;
	check_improve(ch, gsn_evasion, TRUE, 2);

	af.where = TO_AFFECTS;
        af.type = gsn_evasion;
        af.level = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.duration = 3;
        af.bitvector = 0;

        affect_to_char(ch, &af);

        Cprintf(ch, "You enhance your evasion.\n\r");
        act("$n enhances $s evasion.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
do_quicken(CHAR_DATA *ch, char *argument)
{
	AFFECT_DATA af;
	CHAR_DATA *victim = NULL;

	if(is_affected(ch, gsn_haste)) {
		Cprintf(ch, "You are already moving as fast as you can.\n\r");
		return;
	}

	if(ch->move < 20) {
		Cprintf(ch, "You are too tired to quicken anything.\n\r");
		return;
	}

	victim = ch->fighting;
        if (victim == NULL)
        {
                Cprintf(ch, "You are not fighting anyone.\n\r");
                return;
        }

	WAIT_STATE(ch, PULSE_VIOLENCE);

	if(number_percent() > get_skill(ch, gsn_quicken)) {
		Cprintf(ch, "Your reflexes fail you.\n\r");
		check_improve(ch, gsn_quicken, FALSE, 1);
		return;
	}

	ch->move -= 20;
	check_improve(ch, gsn_quicken, TRUE, 1);

        if (saving_throw(ch, victim, gsn_quicken, ch->level, SAVE_HARD, STAT_NONE, DAM_NONE))
	{
		act("$N stops your quicken.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	Cprintf(ch, "You use your reflexes to summon a burst of speed!\n\r");
	act("$n creates a burst of speed!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	// Slow down the victim
	if(is_affected(victim, gsn_haste)) {
		affect_strip(victim, gsn_haste);
                Cprintf(victim, "You feel yourself slow down.\n\r");
                act("$n is moving less quickly.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}
	else if(is_affected(victim, gsn_slow)) {
		act("$N can't get any slower than that.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	}
	else {
		af.where = TO_AFFECTS;
	        af.type = gsn_slow;
	        af.level = ch->level;
        	af.duration = ch->level / 10;
	        af.location = APPLY_DEX;
        	af.modifier = (-1 * (ch->level / 12));
	        af.bitvector = AFF_SLOW;

		affect_to_char(victim, &af);

	        Cprintf(victim, "You feel yourself slowing d o w n...\n\r");
	        act("$n starts to move in slow motion.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	// Speed up the samurai
	if(is_affected(ch, gsn_haste)) {
		Cprintf(ch, "You can't move any faster.\n\r");
	}
	else if(is_affected(ch, gsn_slow)) {
		Cprintf(ch, "You feel yourself speed up.\n\r");
                act("$n is moving less slowly.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		affect_strip(ch, gsn_slow);
	}
	else {
		af.where = TO_AFFECTS;
	        af.type = gsn_haste;
	        af.level = ch->level;
	        af.location = APPLY_DEX;
        	af.modifier = ch->level / 12;
	        af.duration = ch->level / 6;
        	af.bitvector = AFF_HASTE;

	        affect_to_char(ch, &af);
		Cprintf(ch, "You feel yourself moving more quickly.\n\r");
        	act("$n is moving more quickly.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	return;
}

void
do_third_eye(CHAR_DATA *ch, char *argument)
{
	AFFECT_DATA af;
	CHAR_DATA *victim = NULL;

	if (!IS_AFFECTED(ch, AFF_BLIND)) {
		Cprintf(ch, "Your eyes are working just fine.\n\r");
		return;
	}

	if(is_affected(ch, gsn_third_eye)) {
		Cprintf(ch, "You can already see using your third eye.\n\r");
		return;;
	}

	if(ch->move < 50) {
		Cprintf(ch, "You are too tired to use your third eye.\n\r");
		return;
	}

	victim = ch->fighting;
        if (victim == NULL)
        {
                Cprintf(ch, "You are not fighting anyone.\n\r");
                return;
        }

	WAIT_STATE(ch, PULSE_VIOLENCE);

	if(number_percent() > get_skill(ch, gsn_third_eye) / 2) {
		Cprintf(ch, "You fail to awaken your third eye.\n\r");
		check_improve(ch, gsn_third_eye, FALSE, 1);
		return;
	}

	ch->move -= 50;
	check_improve(ch, gsn_third_eye, TRUE, 2);

	af.where = TO_AFFECTS;
        af.type = gsn_third_eye;
        af.level = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.duration = 5;
        af.bitvector = 0;

        affect_to_char(ch, &af);

        Cprintf(ch, "A third eye on your forehead shines brightly!\n\r");
        act("A third eye appears on $s forehead.", ch, NULL, victim, TO_ROOM, POS_RESTING);
}


// Fires 3-6 shots at once, but at reduce accuracy and damage.
void
fire_barrage(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *bow, OBJ_DATA *ammo, int door, int distance)
{
	CHAR_DATA *old_victim_fighting = NULL;
	CHAR_DATA *old_ch_fighting = NULL;
	int barrage_size = 0;
	int i, chance = 0;
	int dam = 0;
	int victim_dead = FALSE;
	int missed = FALSE, show_damage = TRUE;
	int parameter = TYPE_HIT;
	int spell_level;
	int spec_chance = 0;
	int old_victim_position;
	int old_ch_position;

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
        old_victim_fighting = victim->fighting;
	old_victim_position = victim->position;
	old_ch_fighting = ch->fighting;
	old_ch_position = ch->position;
	barrage_size = number_range(3, 6);

        if ((victim->spec_fun == spec_lookup("spec_executioner")
        || IS_CLAN_GOON(victim))
        && victim->fighting == NULL)
        {
                Cprintf(ch, "Your barrage fails.\n\r");
                act("$n easily deflects a ranged attack.\n\r", victim, NULL, NULL, TO_ROOM, POS_RESTING);
                return;
        }

        if (is_safe(ch, victim))
                return;

        /* make sure that you can't use shoot to break charm */
        if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
        {
                Cprintf(ch, "And hurt your beloved master?!\n\r");
                return;
        }

	if (IS_NPC(victim) &&
                victim->fighting != NULL &&
                !is_same_group(ch, victim->fighting))
        {
                Cprintf(ch, "Kill stealing is not permitted.\n\r");
                return;
        }

        if((ammo->weight / 5) < barrage_size)
                barrage_size = ammo->weight / 5;

        ammo->weight = ammo->weight - (2 * barrage_size);
        if(ammo->weight < 2) {
                act("Your $p has been used up.", ch, ammo, NULL, TO_CHAR, POS_RESTING);
                extract_obj(ammo);
        }

        // Determine the chance to hit.
	chance = get_skill(ch, gsn_marksmanship) / 2;
        chance += get_curr_stat(ch, STAT_DEX) / 2;
        chance += get_ranged_hitroll(ch);
        chance += (victim->size - ch->size) * 5;
        chance -= distance * 10;
	if(chance > 90)
		chance = 90;

	for(i = 0; i < barrage_size; i++) {
	        dam = dice(bow->value[1], bow->value[2]);
	        dam += dice(ammo->value[0], ammo->value[1]);
		dam += (dam * get_flag_damage_modifiers(ch, victim, bow, attack_table[bow->value[3]].damage)) / 100;

	        dam += get_ranged_damroll(ch) * 0.7;
	        dam += get_ranged_hitroll(ch) * 0.7;
	        dam = dam * get_skill(ch, gsn_marksmanship) / 100.0;

		spell_level = ammo->value[3];

	        if (number_percent() > chance) {
	                dam = 0;
			check_improve(ch, gsn_marksmanship, FALSE, 2);
	                missed = TRUE;
	        }
		else {
			check_improve(ch, gsn_marksmanship, TRUE, 2);
		}

	        // Special case, magical ammo doesn't do physical damage.
	        if (ammo->value[0] < 1 || ammo->value[1] < 1) {
	                dam = 0;
	                show_damage = FALSE;
	        }

		// Specialize shot
		if(!IS_NPC(ch)
		&& ch->pcdata->specialty == gsn_marksmanship) {

        		spec_chance = ch->pcdata->learned[gsn_marksmanship] - 100;
        		if(number_percent() < spec_chance) {
                		parameter |= TYPE_SPECIALIZED;
                		if(show_damage)
                        		dam = dam + (ch->level / 3);
                		else
                        		spell_level += 4;

        		}
		}

		if(show_damage) {
	        	victim_dead = damage(ch, victim, dam,
	                        ammo->value[2],
	                        attack_table[ammo->value[2]].damage,
	                        show_damage, parameter);
		}
	        if (ammo->value[3] != 0
	        && ammo->value[4] != 0
	        && !victim_dead
	        && !missed) {
	                obj_cast_spell(ammo->value[4], spell_level, ch, victim, ammo);
        	}

		update_pos(victim);

	        if(victim->position == POS_DEAD)
	                victim_dead = TRUE;

		if(victim_dead)
			break;
		
		if(!missed) {
			victim_dead = handle_weapon_flags(ch, victim, bow, gsn_marksmanship); 
		}

		if(victim_dead)
			break;
	}

	ch->fighting = old_ch_fighting;
	ch->position = old_ch_position;

	if(victim_dead)
		return;

	if(victim->position > POS_SLEEPING) {
		victim->fighting = old_victim_fighting;
		victim->position = old_victim_position;
	}
	else {
		victim->fighting = NULL;
		victim->position = POS_STANDING;
	}

	if (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
	        REMOVE_BIT(victim->act, ACT_AGGRESSIVE);

        if (!victim_dead
	&& missed
	&& number_percent() < 25
        && IS_NPC(victim)
        && victim->fighting == NULL) {
		Cprintf(ch, "You've been spotted by %s!\n\r", victim->short_descr);
                victim->hunting = ch;
                victim->hunt_timer = 10;
        }

        door = reverse_direction(door);

        /* Don't tell them if they're surprised or same room */
        if (!can_see(victim, ch) || distance == 0)
        {
                return;
        }

        switch (door)
        {
        case 0:
                Cprintf(victim, "The barrage came from NORTH!\n\r");
                break;
        case 1:
                Cprintf(victim, "The barrage came from EAST!\n\r");
                break;
        case 2:
                Cprintf(victim, "The barrage came from SOUTH!\n\r");
                break;
        case 3:
                Cprintf(victim, "The barrage came from WEST!\n\r");
                break;
        case 4:
                Cprintf(victim, "The barrage came from UP!\n\r");
                break;
        case 5:
                Cprintf(victim, "The barrage came from DOWN!\n\r");
                break;
        default:
                Cprintf(victim, "Throw error: bad direction\n\r");
                break;
        }
        return;
}


// Fires a single shot, generally high damage and accuracy.
void
do_ranged_attack(CHAR_DATA * ch, char *argument)
{
        char arg[MAX_INPUT_LENGTH];
        CHAR_DATA *victim = NULL, *old_victim_fighting = NULL;
	CHAR_DATA *old_ch_fighting = NULL;
        OBJ_DATA *bow = NULL;
	OBJ_DATA *ammo = NULL;
        int door;
	int max_range = 0;
        int chance, distance = 0;
	int dam = 0;
	int victim_dead = FALSE;
	int missed = FALSE, show_damage = TRUE;
	int parameter = TYPE_HIT;
	int spec_chance = 0;
	int spell_level;
	int old_victim_position, old_ch_position;

        one_argument(argument, arg);

        if (arg[0] == '\0' && ch->fighting == NULL)
        {
                Cprintf(ch, "Shoot at whom or what?\n\r");
                return;
        }

        if ((ammo = get_eq_char(ch, WEAR_AMMO)) == NULL)
        {
                Cprintf(ch, "You aren't holding any ammunition.\n\r");
                return;
        }

        if (ammo->item_type != ITEM_AMMO)
        {
                Cprintf(ch, "You aren't holding any ammunition.\n\r");
                return;
        }

	if ((bow = get_eq_char(ch, WEAR_RANGED)) == NULL)
        {
                Cprintf(ch, "You aren't wearing a ranged weapon.\n\r");
                return;
        }

        if(bow->item_type != ITEM_WEAPON
	|| bow->value[0] != WEAPON_RANGED)
        {
                Cprintf(ch, "You need a ranged weapon to shoot!\n\r");
                return;
        }

	// Kind of hokey, but match ammo/weapon by name.
	if((is_name("bow", bow->name) && !is_name("arrow", ammo->name))
	|| (is_name("crossbow", bow->name) && !is_name("bolt", ammo->name))
	|| (is_name("gun", bow->name) && !is_name("bullet", ammo->name))) {
		Cprintf(ch, "Your ammunition isn't compatible with that weapon.\n\r");
		return;
	}

	max_range = bow->extra[2];

        victim = range_finder(ch, arg, max_range, &door, &distance, FALSE);

        if (victim == NULL)
        {
                Cprintf(ch, "You can't find them.\n\r");
                return;
        }

	// Switch to the special barrage function for multiple shots.
	if (is_affected(ch, gsn_barrage)) {
		fire_barrage(ch, victim, bow, ammo, door, distance);
		affect_strip(ch, gsn_barrage);
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);
	old_victim_fighting = victim->fighting;
	old_victim_position = victim->position;
	old_ch_fighting = ch->fighting;
	old_ch_position = ch->position;

	if ((victim->spec_fun == spec_lookup("spec_executioner")
        || IS_CLAN_GOON(victim))
	&& victim->fighting == NULL)
        {
		Cprintf(ch, "Your ranged attack fails.\n\r");
		act("$n easily deflects a ranged attack.\n\r", victim, NULL, NULL, TO_ROOM, POS_RESTING);
                return;
	}

        if (is_safe(ch, victim))
                return;

        /* make sure that you can't use shoot to break charm */
        if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
        {
                Cprintf(ch, "And hurt your beloved master?!\n\r");
                return;
        }

	if (IS_NPC(victim) &&
                victim->fighting != NULL &&
                !is_same_group(ch, victim->fighting))
        {
                Cprintf(ch, "Kill stealing is not permitted.\n\r");
                return;
        }

	ammo->weight = ammo->weight - 2;
        if(ammo->weight < 2) {
                act("Your $p has been used up.", ch, ammo, NULL, TO_CHAR, POS_RESTING);
                extract_obj(ammo);
        }

	// Determine the chance to hit.
	chance = get_skill(ch, gsn_marksmanship) / 2;
	chance += get_curr_stat(ch, STAT_DEX) / 2;
	chance += get_ranged_hitroll(ch);
	chance += (victim->size - ch->size) * 5;
	chance -= distance * 10;
	if(chance > 90)
		chance = 90;

	dam = dice(bow->value[1], bow->value[2]);
	dam += dice(ammo->value[0], ammo->value[1]);
	dam += (dam * get_flag_damage_modifiers(ch, victim, bow, attack_table[bow->value[3]].damage)) / 100;

	dam += get_ranged_damroll(ch) * 0.7;
	dam += get_ranged_hitroll(ch) * 0.7;
	dam = dam * get_skill(ch, gsn_marksmanship) / 100.0;
	spell_level = ammo->value[3];

	if (number_percent() > chance) {
		dam = 0;
		check_improve(ch, gsn_marksmanship, FALSE, 2);
		missed = TRUE;
	}
	else {
		check_improve(ch, gsn_marksmanship, TRUE, 2);
	}

	// Special case, magical ammo doesn't do physical damage.
	if (ammo->value[0] < 1 || ammo->value[1] < 1) {
		dam = 0;
		show_damage = FALSE;
	}

	// Specialize shot
	if(!IS_NPC(ch)
	&& ch->pcdata->specialty == gsn_marksmanship) {

		spec_chance = ch->pcdata->learned[gsn_marksmanship] - 100;
		if(number_percent() < spec_chance) {
			parameter |= TYPE_SPECIALIZED;
			if(show_damage)
				dam = dam + (ch->level / 3);
			else
				spell_level += 6;

		}
	}

	if(show_damage) {
		victim_dead = damage(ch, victim, dam,
			ammo->value[2],
			attack_table[ammo->value[2]].damage,
			show_damage, parameter);
	}

        if (ammo->value[3] != 0
        && ammo->value[4] != 0
        && !victim_dead
	&& !missed) {
                obj_cast_spell(ammo->value[4], spell_level, ch, victim, ammo);
	}

	update_pos(victim);

	if(victim->position == POS_DEAD)
		return;

	if(!missed) {
		victim_dead = handle_weapon_flags(ch, victim, bow, gsn_marksmanship); 
	}

	ch->fighting = old_ch_fighting;
	ch->position = old_ch_position;

	if(victim_dead)
		return;

	if(victim->position > POS_SLEEPING) {
		victim->fighting = old_victim_fighting;
		victim->position = old_victim_position;
	}
	else {
		victim->fighting = NULL;
		victim->position = POS_STANDING;
	}

	if (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
	        REMOVE_BIT(victim->act, ACT_AGGRESSIVE);

	if (!victim_dead
	&& missed
	&& number_percent() < 25
	&& IS_NPC(victim)
	&& victim->fighting == NULL) {
		Cprintf(ch, "You've been spotted by %s!\n\r", victim->short_descr);
		victim->hunting = ch;
		victim->hunt_timer = 10;
	}

        door = reverse_direction(door);

        /* Don't tell them if they're surprised or same room */
        if (!can_see(victim, ch) || distance == 0)
        {
                return;
        }

        switch (door)
        {
        case 0:
                Cprintf(victim, "The shot came from NORTH!\n\r");
                break;
        case 1:
                Cprintf(victim, "The shot came from EAST!\n\r");
                break;
        case 2:
                Cprintf(victim, "The shot came from SOUTH!\n\r");
                break;
        case 3:
                Cprintf(victim, "The shot came from WEST!\n\r");
                break;
        case 4:
                Cprintf(victim, "The shot came from UP!\n\r");
                break;
        case 5:
                Cprintf(victim, "The shot came from DOWN!\n\r");
                break;
        default:
                Cprintf(victim, "Throw error: bad direction\n\r");
                break;
        }
        return;
}

void
do_aiming(CHAR_DATA * ch, char *argument)
{
	int chance;

        if ((chance = get_skill(ch, gsn_aiming)) == 0)
        {
                Cprintf(ch, "Your aim sucks and nothing is going to change it.\n\r");
                return;
        }

	if (ch->mana < 50)
	{
        	Cprintf(ch, "You can't get up enough energy.\n\r");
	        return;
	}

        /* modifiers */
        if (ch->race == race_lookup("elf"))
                chance += 10;

	if (is_affected(ch, gsn_aiming)) {
		Cprintf(ch, "Your aim is already focused.\n\r");
		return;
	}

	chance = chance * 7 / 8;

        if (number_percent() < chance)
        {
                AFFECT_DATA af;

                WAIT_STATE(ch, PULSE_VIOLENCE);
                ch->mana -= 50;
                ch->move /= 2;

                Cprintf(ch, "Your eyes sharpen and your aim improves!\n\r");
                act("$n gets a focused look in $s eyes.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
                check_improve(ch, gsn_aiming, TRUE, 5);

                af.where = TO_AFFECTS;
                af.type = gsn_aiming;
                af.level = ch->level;
                af.duration = ch->level / 6;
                af.modifier = UMAX(1, ch->level / 4);
                af.bitvector = 0;

                af.location = APPLY_HITROLL;
                affect_to_char(ch, &af);

		af.modifier = 0 - (af.modifier / 2);
		af.location = APPLY_DAMROLL;
                affect_to_char(ch, &af);
        }
        else
        {
                WAIT_STATE(ch, PULSE_VIOLENCE);
                ch->mana -= 25;
                ch->move = ch->move * 3 / 4;

                Cprintf(ch, "Your try to improve your aim, but nothing happens.\n\r");
                check_improve(ch, gsn_aiming, FALSE, 4);
        }
}

void
do_barrage(CHAR_DATA * ch, char *argument)
{
	int chance;

        if ((chance = get_skill(ch, gsn_barrage)) == 0)
        {
                Cprintf(ch, "The best you manage is a barrage of used tissues.\n\r");
                return;
        }

	if (is_affected(ch, gsn_barrage)) {
		Cprintf(ch, "Your barrage is already prepared.\n\r");
		return;
	}

	if (ch->mana < 50)
	{
        	Cprintf(ch, "You can't get up enough energy.\n\r");
	        return;
	}

	chance = chance * 7 / 8;

        if (number_percent() < chance)
        {
                AFFECT_DATA af;

                WAIT_STATE(ch, PULSE_VIOLENCE);
                ch->mana -= 50;

                Cprintf(ch, "You ready a deadly barrage!\n\r");
                act("$n readies a deadly barrage.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
                check_improve(ch, gsn_barrage, TRUE, 5);

                af.where = TO_AFFECTS;
                af.type = gsn_barrage;
                af.level = ch->level;
                af.duration = 5;
                af.modifier = -10;
                af.bitvector = 0;
                af.location = APPLY_HITROLL;
                affect_to_char(ch, &af);
        }
        else
        {
                WAIT_STATE(ch, PULSE_VIOLENCE);
                ch->mana -= 25;

                Cprintf(ch, "You fail to ready a barrage.\n\r");
                check_improve(ch, gsn_barrage, FALSE, 4);
        }
}


void
do_wail(CHAR_DATA *ch, char *argument) {
	CHAR_DATA *victim;
	int chance, dam;
	AFFECT_DATA af;

	if (get_skill(ch, gsn_wail) < 1) {
		Cprintf(ch, "You wail like a baby, but it's only hurting their feelings.\n\r");
		return;
	}

	if (argument[0] == '\0') {
		victim = ch->fighting;
		if (victim == NULL) {
			Cprintf(ch, "You are not fighting anyone.\n\r");
			return;
		}
	} else {
		if ((victim = get_char_room(ch,argument)) == NULL) {
			Cprintf(ch, "They're not here.\n\r");
			return;
		}
	}

	if (is_safe(ch, victim))
		return;

	if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch, victim->fighting)) {
		Cprintf(ch, "Kill stealing is not permitted.\n\r");
		return;
	}

	if (ch->thrustCounter > 0) {
		Cprintf(ch, "You haven't recovered from your last wail yet.\n\r");
		return;
	}

	if(ch->move < 50) {
		Cprintf(ch, "You're too tired to wail again right now.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR, POS_RESTING);
		return;
	}

	chance = get_skill(ch, gsn_wail);

	if (number_percent() > chance) {
		Cprintf(ch, "You try to wail, but it comes out as more of a squawk.\n\r");
		damage(ch, victim, 0, gsn_wail, DAM_SOUND, FALSE, TYPE_MAGIC);
		check_improve(ch, gsn_wail, FALSE, 1);
		WAIT_STATE(ch, skill_table[gsn_wail].beats);
		ch->move -= 25;
		return;
	}

	check_improve(ch, gsn_wail, TRUE, 1);
	check_killer(ch, victim);
	WAIT_STATE(ch, PULSE_VIOLENCE);
	ch->move -= 50;

	act("$n lets out a loud shriek at $N.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
	act("$n lets out a loud shriek at you.", ch, NULL, victim, TO_VICT, POS_RESTING);
	act("You let out a loud shriek at $N.", ch, NULL, victim, TO_CHAR, POS_RESTING);

	// Base damage 61 to 151
	dam = dice(ch->level / 5, 10) + ch->level * 3 / 2;

	// Adds 3% for hp. (Typical +50 for PC, 150 for mobs).
	dam += ch->hit * 3 / 100;

	if (saving_throw(ch, victim, gsn_wail, ch->level, SAVE_HARD, STAT_CON, DAM_SOUND))
		;
	else if (!is_affected(victim, gsn_wail))
	{
		af.where = TO_AFFECTS;
		af.type = gsn_wail;
		af.level = ch->level;
		af.duration = 3;
		af.location = APPLY_DEX;
		af.modifier = -4;
		af.bitvector = 0;
		affect_to_char(victim, &af);

		af.location = APPLY_SAVING_SPELL;
		af.modifier = ch->level / 6;
		affect_to_char(victim, &af);
		Cprintf(victim, "You can't hear anything!\n\r");
		act("$N looks pale and shaken.", ch, NULL, victim, TO_CHAR, POS_RESTING);
	}
        
	damage(ch, victim, dam, gsn_wail, DAM_SOUND, TRUE, TYPE_MAGIC);
	// Set a timer so we can only wail once per tick.
	
	ch->thrustCounter = 1;
	return;
}
