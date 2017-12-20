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
 *   ROM 2.4 is copyright 1993-1996 Russ Taylor                            *
 *   ROM has been brought to you by the ROM consortium                     *
 *       Russ Taylor (rtaylor@pacinfo.com)                                 *
 *       Gabrielle Taylor (gtaylor@pacinfo.com)                            *
 *       Brian Moore (rom@rom.efn.org)                                     *
 *   By using this code, you have agreed to follow the terms of the        *
 *   ROM license, in the file Rom24/doc/rom.license                        *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/resource.h>

#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "music.h"
#include "stats.h"
#include "clan.h"
#include "utils.h"
#include "board.h"


extern int _filbuf(FILE *);
extern void do_crash(CHAR_DATA*, char*);

#if !defined(OLD_RAND)
long random();
void srandom(unsigned int);
int getpid();
time_t time(time_t * tloc);

#endif


/* externals for counting purposes */
extern OBJ_DATA *obj_free;
extern CHAR_DATA *char_free;
extern DESCRIPTOR_DATA *descriptor_free;
extern PC_DATA *pcdata_free;
extern AFFECT_DATA *affect_free;
extern AUCTION_DATA *auction;

/*
 * Globals.
 */
HELP_DATA *help_first;
HELP_DATA *help_last;

SHOP_DATA *shop_first;
SHOP_DATA *shop_last;

NOTE_DATA *note_free;
//MPROG_CODE *mprog_list;

char bug_buf[2 * MAX_INPUT_LENGTH];
CHAR_DATA *char_list;
char *help_greeting;
char log_buf[2 * MAX_INPUT_LENGTH];
KILL_DATA kill_table[MAX_LEVEL];
OBJ_DATA *object_list;
TIME_INFO_DATA time_info;
WEATHER_DATA weather_info;

int gsn_reserved;
int gsn_ambush;
int gsn_death_blow;
int gsn_atheism;
int gsn_attraction;
int gsn_abandon;
int gsn_acid_blast;
int gsn_animal_growth;
int gsn_animate_tree;
int gsn_animate_dead;
int gsn_armor;
int gsn_aura;
int gsn_aurora;
int gsn_bark_skin;
int gsn_beacon;
int gsn_blast_of_rot;
int gsn_bless;
int gsn_blindness;
int gsn_blur;
int gsn_burning_hands;
int gsn_call_creature;
int gsn_call_lightning;
int gsn_calm;
int gsn_cancellation;
int gsn_carnal_reach;
int gsn_cause_critical;
int gsn_cause_light;
int gsn_cause_serious;
int gsn_chain_lightning;
int gsn_change_sex;
int gsn_channel_energy;
int gsn_charm_person;
int gsn_chill_touch;
int gsn_clenched_fist;
int gsn_cloak_of_mind;
int gsn_cloudkill;
int gsn_colour_spray;
int gsn_concealment;
int gsn_continual_light;
int gsn_control_weather;
int gsn_condensation;
int gsn_corruption;
int gsn_create_door;
int gsn_create_food;
int gsn_create_oil;
int gsn_create_rose;
int gsn_create_spring;
int gsn_create_water;
int gsn_creeping_doom;
int gsn_crushing_hand;
int gsn_cure_blindness;
int gsn_cure_critical;
int gsn_cure_disease;
int gsn_cure_light;
int gsn_cure_poison;
int gsn_cure_serious;
int gsn_curse;
int gsn_darkness;
int gsn_delayed_blast_fireball;
int gsn_demonfire;
int gsn_denounciation;
int gsn_detect_hidden;
int gsn_detect_invis;
int gsn_detect_magic;
int gsn_detect_poison;
int gsn_destroy_rune;
int gsn_destroy_tattoo;
int gsn_dispel_magic;
int gsn_disenchant;
int gsn_dispel_evil;
int gsn_dispel_good;
int gsn_dissolution;
int gsn_drain_life;
int gsn_duplicate;
int gsn_earthquake;
int gsn_earth_to_mud;
int gsn_enchant_armor;
int gsn_enchant_weapon;
int gsn_energy_drain;
int gsn_embellish;
int gsn_nature_protection;
int gsn_faerie_fire;
int gsn_faerie_fog;
int gsn_farsight;
int gsn_feeblemind;
int gsn_figurine_spell;
int gsn_fireball;
int gsn_fireproof;
int gsn_flame_arrow;
int gsn_flamestrike;
int gsn_flesh_golem;
int gsn_flood;
int gsn_animal_skins;
int gsn_floating_disc;
int gsn_fly;
int gsn_fortify;
int gsn_frenzy;
int gsn_gate;
int gsn_giant_strength;
int gsn_harm;
int gsn_haste;
int gsn_heal;
int gsn_heat_metal;
int gsn_hell_blades;
int gsn_holy_word;
int gsn_hurricane;
int gsn_ice_bolt;
int gsn_identify;
int gsn_ignore_wounds;
int gsn_inform;
int gsn_infravision;
int gsn_invisibility;
int gsn_life_wave;
int gsn_lesser_dispel;
int gsn_lightning_bolt;
int gsn_lure;
int gsn_karma;
int gsn_know_alignment;
int gsn_jinx;
int gsn_jail;
int gsn_locate_object;
int gsn_magic_missile;
int gsn_magic_stone;
int gsn_mass_healing;
int gsn_mass_invis;
int gsn_meteor_swarm;
int gsn_missionary;
int gsn_misty_cloak;
int gsn_mirror_image;
int gsn_moonbeam;
int gsn_nexus;
int gsn_nightmares;
int gsn_oculary;
int gsn_pass_door;
int gsn_phantom_force;
int gsn_plague;
int gsn_plant;
int gsn_poison;
int gsn_portal;
int gsn_prismatic_spray;
int gsn_prismatic_sphere;
int gsn_protection_evil;
int gsn_protection_good;
int gsn_protection_neutral;
int gsn_psychic_crush;
int gsn_pyrotechnics;
int gsn_quill_armor;
int gsn_rainbow_burst;
int gsn_razor_claws;
int gsn_ray_of_truth;
int gsn_recharge;
int gsn_refresh;
int gsn_remove_alignment;
int gsn_remove_curse;
int gsn_repel;
int gsn_replicate;
int gsn_repulsion;
int gsn_rukus_magna;
int gsn_sanctuary;
int gsn_simulacrum;
int gsn_sharpen;
int gsn_shawl;
int gsn_shield;
int gsn_shocking_grasp;
int gsn_shock_wave;
int gsn_skeletal_warrior;
int gsn_sleep;
int gsn_slow;
int gsn_soul_blade;
int gsn_soul_trap;
int gsn_spell_stealing;
int gsn_spirit_link;
int gsn_stone_skin;
int gsn_summon;
int gsn_summon_angel;
int gsn_sunray;
int gsn_symbol;
int gsn_teleport;
int gsn_test_room;
int gsn_tornado;
int gsn_true_sight;
int gsn_tsunami;
int gsn_turn_undead;
int gsn_ventriloquate;
int gsn_voodoo_doll;
int gsn_weaken;
int gsn_whirlpool;
int gsn_winter_storm;
int gsn_withstand_death;
int gsn_wildfire;
int gsn_word_of_recall;
int gsn_acid_breath;
int gsn_fire_breath;
int gsn_frost_breath;
int gsn_gas_breath;
int gsn_lightning_breath;
int gsn_general_purpose;
int gsn_high_explosive;
int gsn_banish;
int gsn_blink;
int gsn_build_fire;
int gsn_call_to_arms;
int gsn_cave_bears;
int gsn_cloud_of_poison;
int gsn_cone_of_fear;
int gsn_confusion;
int gsn_create_shadow;
int gsn_cryogenesis;
int gsn_dark_feast;
int gsn_demand;
int gsn_detect_all;
int gsn_detonation;
int gsn_discordance;
int gsn_displace;
int gsn_drakor;
int gsn_enlargement;
int gsn_familiar;
int gsn_fear;
int gsn_flag;
int gsn_giant_insect;
int gsn_grandeur;
int gsn_granite_stare;
int gsn_gullivers_travel;
int gsn_hailstorm;
int gsn_hold_person;
int gsn_homonculus;
int gsn_hunt;
int gsn_lightning_spear;
int gsn_hypnosis;
int gsn_injustice;
int gsn_loneliness;
int gsn_mass_protect;
int gsn_materialize;
int gsn_melior;
int gsn_minimation;
int gsn_mutate;
int gsn_oracle;
int gsn_pacifism;
int gsn_pentagram;
int gsn_phantasm_monster;
int gsn_primal_rage;
int gsn_rain_of_tears;
int gsn_reach_elemental;
int gsn_robustness;
int gsn_scalemail;
int gsn_scramble;
int gsn_shifting_sands;
int gsn_shriek;
int gsn_soundproof;
int gsn_summon_greater;
int gsn_summon_horde;
int gsn_summon_lesser;
int gsn_summon_lord;
int gsn_surge;
int gsn_tame_animal;
int gsn_taunt;
int gsn_thirst;
int gsn_turn_magic;
int gsn_wail;
int gsn_weaponsmith;
int gsn_web;
int gsn_wizards_eye;
int gsn_wrath;
int gsn_axe;
int gsn_backstab;
int gsn_bash;
int gsn_beheading;
int gsn_berserk;
int gsn_blindfighting;
int gsn_block;
int gsn_brawling;
int gsn_brew;
int gsn_bribe;
int gsn_butcher;
int gsn_call_to_hunt;
int gsn_cave_in;
int gsn_carve_boulder;
int gsn_carve_spear;
int gsn_charge;
int gsn_cheat;
int gsn_conversion;
int gsn_corpse_drain;
int gsn_combine_potion;
int gsn_dagger;
int gsn_drunken_master;
int gsn_dirt_kicking;
int gsn_disarm;
int gsn_dodge;
int gsn_drag;
int gsn_dragon_bite;
int gsn_dual_wield;
int gsn_enhanced_damage;
int gsn_enrage;
int gsn_ensnare;
int gsn_envenom;
int gsn_gore;
int gsn_flail;
int gsn_fury;
int gsn_fourth_attack;
int gsn_gemology;
int gsn_gladiator;
int gsn_gravitation;
int gsn_hand_to_hand;
int gsn_hunter_ball;
int gsn_iron_vigil;
int gsn_jump;
int gsn_kick;
int gsn_leadership;
int gsn_living_stone;
int gsn_bite;
int gsn_mace;
int gsn_parry;
int gsn_polearm;
int gsn_presence;
int gsn_push;
int gsn_rally;
int gsn_reflection;
int gsn_rescue;
int gsn_retreat;
int gsn_retribution;
int gsn_rip;
int gsn_rub;
int gsn_sap;
int gsn_second_attack;
int gsn_shapeshift;
int gsn_shedding;
int gsn_shield_bash;
int gsn_shield_block;
int gsn_snake_bite;
int gsn_spear;
int gsn_substitution;
int gsn_sword;
int gsn_tail_attack;
int gsn_tail_trip;
int gsn_taste;
int gsn_telepathy;
int gsn_third_attack;
int gsn_throw;
int gsn_thrust;
int gsn_trance;
int gsn_transferance;
int gsn_trip;
int gsn_tumbling;
int gsn_waterwalk;
int gsn_water_breathing;
int gsn_whip;
int gsn_wild_swing;
int gsn_bashdoor;
int gsn_fast_healing;
int gsn_faith;
int gsn_haggle;
int gsn_hide;
int gsn_lore;
int gsn_meditation;
int gsn_peek;
int gsn_pick_lock;
int gsn_recall;
int gsn_regeneration;
int gsn_regrowth;
int gsn_scrolls;
int gsn_scribe;
int gsn_sneak;
int gsn_staves;
int gsn_steal;
int gsn_wands;
int gsn_cure_plague;
int gsn_guardian;
int gsn_lay_hands;
int gsn_shadow;
int gsn_shadow_magic;
int gsn_stone_sleep;
int gsn_appraise;
int gsn_lair;
int gsn_crush;
int gsn_hug;
int gsn_slide;
int gsn_spy;
int gsn_craft_item;
int gsn_paint_lesser;
int gsn_paint_greater;
int gsn_paint_power;
int gsn_alarm_rune;
int gsn_fire_rune;
int gsn_shackle_rune;
int gsn_wizard_mark;
int gsn_blade_rune;
int gsn_burst_rune;
int gsn_balance_rune;
int gsn_soul_rune;
int gsn_essence;
int gsn_last_skill;
int gsn_zeal;
int gsn_hurl;
int gsn_death_rune;
int gsn_magic_sheath;

// monks
int gsn_boost;
int gsn_chakra;
int gsn_stance_turtle;
int gsn_stance_tiger;
int gsn_stance_mantis;
int gsn_stance_shadow;
int gsn_stance_kensai;
int gsn_shoulder_throw;
int gsn_intense_damage;
int gsn_weapon_catch;
int gsn_chi_moonlight;
int gsn_chi_whirlwind;
int gsn_chi_whisper;
int gsn_dragon_kick;
int gsn_choke_hold;
int gsn_demon_fist;
int gsn_eagle_claw;
int gsn_pain_touch;

int gsn_weapon_lock;
int gsn_shadow_walk;
int gsn_chi_ei;
int gsn_chi_kaze;
int gsn_chi_gekkou;
int gsn_zanshin;

int gsn_cycle;
int gsn_ninjitsu;
int gsn_evasion;
int gsn_balance;

int gsn_katana;
int gsn_doublestrike;
int gsn_third_eye;
int gsn_quicken;
int gsn_extreme_damage;
int gsn_seppuku;

// clanspells
int gsn_homeland;
int gsn_paradox;
int gsn_dragonbane;
int gsn_harmonic_aura;
int gsn_brotherhood;

// Upgraded druids and rangers
int gsn_minor_revitalize;
int gsn_lesser_revitalize;
int gsn_greater_revitalize;
int gsn_thorn_mantle;
int gsn_leaf_shield;
int gsn_marksmanship;
int gsn_barrage;
int gsn_aiming;
int gsn_rapid_shot;
int gsn_stun;
int gsn_purify;
int gsn_hallow;

int gsn_mana_drain;

/*
 * Locals.
 */
MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
char *string_hash[MAX_KEY_HASH];

AREA_DATA *area_first;
AREA_DATA *area_last;

char *string_space;
char *top_string;
char str_empty[1];

int top_affect;
int top_area;
int top_ed;
int top_exit;
int top_help;
int top_mob_index;
int top_obj_index;
int top_reset;
int top_room;
int top_shop;
int top_vnum_room;				/* OLC */
int top_vnum_mob;				/* OLC */
int top_vnum_obj;				/* OLC */
int top_vnum_mprog;
int mobile_count = 0;
int newmobs = 0;
int newobjs = 0;


/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
#define         MAX_STRING      7000000
#define         MAX_PERM_BLOCK   131072
#define         MAX_MEM_LIST   11

void *rgFreeList[MAX_MEM_LIST];
const int rgSizeList[MAX_MEM_LIST] =
{
	16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768 - 64
};

int nAllocString;
int sAllocString;
int nAllocPerm;
int sAllocPerm;



/*
 * Semi-locals.
 */
bool fBootDb;
FILE *fpArea;
char strArea[MAX_INPUT_LENGTH];

/*
 * Local booting procedures.
 */
void init_mm(void);
void load_area(FILE * fp);
void new_load_area(FILE * fp);	/* OLC */
void load_helps(FILE * fp);
void load_editable_helps(FILE * fp);
void load_old_mob(FILE * fp);
void load_mobiles(FILE * fp);
void load_old_obj(FILE * fp);
void load_objects(FILE * fp);
void load_resets(FILE * fp);
void load_rooms(FILE * fp);
void load_shops(FILE * fp);
void load_socials(FILE * fp);
void load_specials(FILE * fp);
void load_bans(void);
void load_mobprogs(FILE * fp);
void fix_exits(void);
void fix_mobprogs(void);

/*
 * Big mama top level function.
 */
void
boot_db(void)
{
    log_string("Creating the universe...");

	help_first = NULL;
	help_last = NULL;
	help_greeting = NULL;

	/*
	 * Init some data space stuff.
	 */
	{
		if ((string_space = (char *) calloc(1, MAX_STRING)) == NULL)
		{
			bug("Boot_db: can't alloc %d string space.", MAX_STRING);
			exit(1);
		}
		top_string = string_space;
		fBootDb = TRUE;
	}

	/*
	 * Init random number generator.
	 */
	{
		init_mm();
	}

	/*
	 * Set time and weather.
	 */
	{
        long lhour, lday, lmonth;

	    log_string("Cueing the sun...");

		lhour = (current_time - 650336715)
			/ (PULSE_TICK / PULSE_PER_SECOND);
		time_info.hour = lhour % 24;
		lday = lhour / 24;
		time_info.day = lday % 35;
		lmonth = lday / 35;
		time_info.month = lmonth % 17;
		time_info.year = lmonth / 17;

		if (time_info.hour < 5)
			weather_info.sunlight = SUN_DARK;
		else if (time_info.hour < 6)
			weather_info.sunlight = SUN_RISE;
		else if (time_info.hour < 19)
			weather_info.sunlight = SUN_LIGHT;
		else if (time_info.hour < 20)
			weather_info.sunlight = SUN_SET;
		else
			weather_info.sunlight = SUN_DARK;

		log_string("Creating wind and rain...");

		weather_info.change = 0;
		weather_info.mmhg = 960;
		if (time_info.month >= 7 && time_info.month <= 12)
			weather_info.mmhg += number_range(1, 50);
		else
			weather_info.mmhg += number_range(1, 80);

		if (weather_info.mmhg <= 980)
			weather_info.sky = SKY_LIGHTNING;
		else if (weather_info.mmhg <= 1000)
			weather_info.sky = SKY_RAINING;
		else if (weather_info.mmhg <= 1020)
			weather_info.sky = SKY_CLOUDY;
		else
			weather_info.sky = SKY_CLOUDLESS;

	}

	/* this should go in right after the weather initialization in boot_db */
	log_string("Waking up auctioneers...");

	auction = (AUCTION_DATA *) malloc(sizeof(AUCTION_DATA));	/* DOH!!! */
	if (auction == NULL)
	{
		bug("malloc'ing AUCTION_DATA didn't give %d bytes", sizeof(AUCTION_DATA));
		exit(1);
	}

	auction->item = NULL;		/* nothing is being sold */


	/*
	 * Assign gsn's for skills which have them.
	 */
	{
		int sn;

        log_string("Setting up skills...");

        for (sn = 0; sn < MAX_SKILL; sn++)
		{
			if (skill_table[sn].pgsn != NULL)
				*skill_table[sn].pgsn = sn;
		}
	}

	/*
	 * Read in all the area files.
	 */
	{
		FILE *fpList;

		log_string("Creating terra firma...");

		if ((fpList = fopen(AREA_LIST, "r")) == NULL)
		{
			perror(AREA_LIST);
			exit(1);
		}

		for (;;)
		{
			strcpy(strArea, fread_word(fpList));
			if (strArea[0] == '$')
				break;

			if (strArea[0] == '-')
			{
				fpArea = stdin;
			}
			else
			{
				if ((fpArea = fopen(strArea, "r")) == NULL)
				{
					if (!str_cmp(strArea, "help_db.are")) {
						bug("help_db.are not found, creating new files.", 0);
						fpArea = fopen("help_db.are", "w");
						fprintf(fpArea, "#HELPS_2.0\n");
						fprintf(fpArea, "Builders: None~\n");
						fprintf(fpArea, "0 $~\n#$\n");
						fclose(fpArea);
						fpArea = fopen("help_db.are", "r");
					}
					else {
						perror(strArea);
						exit(1);
					}
				}
			}

			for (;;)
			{
				char *word;

				if (fread_letter(fpArea) != '#')
				{
					bug("Boot_db: # not found.", 0);
					exit(1);
				}

				word = fread_word(fpArea);

				if (word[0] == '$')
					break;
				else if (!str_cmp(word, "AREA")) {
				    load_area(fpArea);
				} else if (!str_cmp(word, "AREADATA")) {
					new_load_area(fpArea);
				} else if (!str_cmp(word, "HELPS")) {
					load_helps(fpArea);
				} else if (!str_cmp(word, "HELPS_2.0")) {
					load_editable_helps(fpArea);
				} else if (!str_cmp(word, "MOBOLD")) {
					load_old_mob(fpArea);
				} else if (!str_cmp(word, "MOBILES")) {
				    load_mobiles(fpArea);
				} else if (!str_cmp(word, "MOBPROGS")) {
				    load_mobprogs(fpArea);
				} else if (!str_cmp(word, "OBJOLD")) {
				    load_old_obj(fpArea);
				} else if (!str_cmp(word, "OBJECTS")) {
                    load_objects(fpArea);
				} else if (!str_cmp(word, "RESETS")) {
				    load_resets(fpArea);
				} else if (!str_cmp(word, "ROOMS")) {
				    load_rooms(fpArea);
				} else if (!str_cmp(word, "SHOPS")) {
				    load_shops(fpArea);
				} else if (!str_cmp(word, "SOCIALS")) {
				    load_socials(fpArea);
				} else if (!str_cmp(word, "SPECIALS")) {
				    load_specials(fpArea);
				} else {
			        char buf[MAX_STRING_LENGTH];
				    sprintf(buf, "Boot_db: bad section name: %s", word);
				    bug(buf, 0);

					exit(1);
				}
			}

			if (fpArea != stdin)
				fclose(fpArea);
			fpArea = NULL;
		}
		fclose(fpList);
	}

	/*
	 * Fix up exits.
	 * Declare db booting over.
	 * Reset all areas once.
	 * Load up the songs, notes and ban files.
	 */
	{
	    log_string("Verifying all exits go places...");
		fix_exits();

		log_string("Interpretting mobprogs...");
		fix_mobprogs();

		fBootDb = FALSE;

		log_string("Updating old objects...");
		convert_objects();

		log_string("Refresing all areas...");
		area_update();

	    log_string("Loading Board List...");
		load_boards();

		log_string("Re-saving all notes...");
		save_notes();

		log_string("Loading all banned buggers...");
		load_bans();

		log_string("Teaching the bards...");
		load_songs();
	}

	/* and lastly, load up them clan leaders! */
	log_string("Loading the clan leaders...");
	read_clanleaders();

	return;
}



/*
 * Snarf an 'area' header line.
 */
void
load_area(FILE * fp)
{
	AREA_DATA *pArea;

	pArea = (AREA_DATA *) alloc_perm(sizeof(*pArea));
/*  pArea->reset_first  = NULL;
   pArea->reset_last    = NULL; */
	pArea->file_name = fread_string(fp);

	pArea->area_flags = AREA_LOADING;	/* OLC */
	pArea->security = 9;		/* OLC *//* 9 -- Hugin */
	pArea->builders = str_dup("None");	/* OLC */
	pArea->vnum = top_area;		/* OLC */

	pArea->name = fread_string(fp);
	pArea->credits = fread_string(fp);
	pArea->min_vnum = fread_number(fp);
	pArea->max_vnum = fread_number(fp);
	pArea->age = 15;
	pArea->age_count = 15;
	pArea->nplayer = 0;
	pArea->empty = FALSE;

	if (!area_first)
		area_first = pArea;
	if (area_last)
	{
		area_last->next = pArea;
		REMOVE_BIT(area_last->area_flags, AREA_LOADING);	/* OLC */
	}
	area_last = pArea;
	pArea->next = NULL;

	top_area++;
	return;
}

/*
 * OLC
 * Use these macros to load any new area formats that you choose to
 * support on your MUD.  See the new_load_area format below for
 * a short example.
 */
#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                \
                if ( !str_cmp( word, literal ) )    \
                {                                   \
                    field  = value;                 \
                    fMatch = TRUE;                  \
                    break;                          \
                                }

#define SKEY( string, field )                       \
                if ( !str_cmp( word, string ) )     \
                {                                   \
                    free_string( field );           \
                    field = fread_string( fp );     \
                    fMatch = TRUE;                  \
                    break;                          \
                                }



/* OLC
 * Snarf an 'area' header line.   Check this format.  MUCH better.  Add fields
 * too.
 *
 * #AREAFILE
 * Name   { All } Locke    Newbie School~
 * Repop  A teacher pops in the room and says, 'Repop coming!'~
 * Recall 3001
 * End
 */
void
new_load_area(FILE * fp) {
    AREA_DATA *pArea;
    char *word;
    bool fMatch;

    pArea = (AREA_DATA *) alloc_perm(sizeof(*pArea));
    pArea->age = 15;
    pArea->age_count = 15;
    pArea->nplayer = 0;
    pArea->file_name = str_dup(strArea);
    pArea->vnum = top_area;
    pArea->name = str_dup("New Area");
    pArea->builders = str_dup("");
    pArea->security = 9;		/* 9 -- Hugin */
    pArea->min_vnum = 0;
    pArea->max_vnum = 0;
    pArea->continent = 0;
    pArea->area_flags = 0;
    pArea->questable = 1;
/*  pArea->recall       = ROOM_VNUM_TEMPLE;        ROM OLC */

    for (;;) {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = FALSE;

        switch (UPPER(word[0])) {
            case 'A':
                KEY("Age", pArea->age_count, fread_number(fp));

            case 'N':
                SKEY("Name", pArea->name);
                break;

            case 'X':
                KEY("Xcontinent", pArea->continent, fread_number(fp));
                break;

            case 'S':
                KEY("Security", pArea->security, fread_number(fp));
                break;

            case 'V':
                if (!str_cmp(word, "VNUMs")) {
                    pArea->min_vnum = fread_number(fp);
                    pArea->max_vnum = fread_number(fp);
                }
                break;

            case 'E':
                if (!str_cmp(word, "End")) {
                    fMatch = TRUE;

                    if (area_first == NULL) {
                        area_first = pArea;
                    }

                    if (area_last != NULL) {
                        area_last->next = pArea;
                    }

                    area_last = pArea;
                    pArea->next = NULL;
                    top_area++;
                    return;
                }
                break;

            case 'B':
                SKEY("Builders", pArea->builders);
                break;

            case 'C':
                SKEY("Credits", pArea->credits);
                break;

            case 'Q':
                KEY("Questable", pArea->questable, fread_number(fp));
                break;
        }
    }
}

void
load_board_list(FILE* fp) {
    BOARD_DATA *pBoard;
    BOARD_DATA *lastBoard;
    char *word;
    bool fMatch;
    bool done;

    pBoard = new_board();
    board_first = pBoard;
    lastBoard = NULL;

    done = FALSE;

    for (;!done;) {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = FALSE;

        switch (UPPER(word[0])) {
            case '$':
                done = TRUE;
                break;

            case 'C':
                SKEY("Clan", pBoard->clan);
                break;

            case 'D':
                SKEY("Description", pBoard->description)
                KEY("DefaultBoard", pBoard->isDefault, fread_number(fp));
                break;

            case 'E':
                if (!str_cmp(word, "End")) {
                    fMatch = TRUE;

                    pBoard->next = new_board();
                    lastBoard = pBoard;
                    pBoard = pBoard->next;
                }

                KEY("Expires", pBoard->defaultExpire, fread_number(fp));
                KEY("Enabled", pBoard->enabled, fread_number(fp));
                break;

            case 'F':
                if (!str_cmp(word, "Filename")) {
                    fMatch = TRUE;

                    free_string(pBoard->filename);
                    pBoard->filename = fread_string(fp);
                }
                break;

            case 'M':
                KEY("MinRead", pBoard->readLevel, fread_number(fp));
                KEY("MinWrite", pBoard->writeLevel, fread_number(fp));
                break;

            case 'N':
                if (!str_cmp(word, "Name")) {
                    fMatch = TRUE;

                    pBoard->name = fread_string(fp);
                    if (!pBoard->filename) {
                        pBoard->filename = str_dup(pBoard->name);
                    }
                }
                break;

            case 'R':
                SKEY("Recipient", pBoard->names);
                KEY("Restriction", pBoard->nameRestriction, fread_number(fp));
        }
    }

    if (!lastBoard) {
        // There are no boards defined
        free_board(board_first);
        board_first = NULL;
    } else {
        free_board(lastBoard->next);
        lastBoard->next = NULL;
    }
}

/*
 * Sets vnum range for area using OLC protection features.
 */
void
assign_area_vnum(int vnum)
{
	if (area_last->min_vnum == 0 || area_last->max_vnum == 0)
		area_last->min_vnum = area_last->max_vnum = vnum;
	if (vnum != URANGE(area_last->min_vnum, vnum, area_last->max_vnum))
	{
		if (vnum < area_last->min_vnum)
			area_last->min_vnum = vnum;
		else
			area_last->max_vnum = vnum;
	}
	return;
}

/*
 * Snarf a help section.
 */
void
load_helps(FILE * fp)
{
	HELP_DATA *pHelp;

	for (;;)
	{

		pHelp = (HELP_DATA *) alloc_perm(sizeof(*pHelp));
		pHelp->level = fread_number(fp);
		pHelp->keyword = fread_string(fp);
		if (pHelp->keyword[0] == '$')
			break;
		pHelp->text = fread_string(fp);

		if (!str_cmp(pHelp->keyword, "greeting"))
			help_greeting = pHelp->text;

		if (help_first == NULL)
			help_first = pHelp;
		if (help_last != NULL)
			help_last->next = pHelp;

		help_last = pHelp;
		pHelp->next = NULL;
		top_help++;
	}

	return;
}

// Help files version 2.0 which can be modified
// using the olc add-on HEDIT but are otherwise the same.
void
load_editable_helps(FILE * fp)
{
        HELP_DATA *pHelp;
	char *word;
	word = fread_word(fp);

	// Read builder list.
	if(str_cmp(word, "Builders:")) {
		bug("Unexpected string found on line 2 of help_db.are", 0);
		return;
	}
	word = fread_string(fp);
	if(str_cmp(word, "None"))
		heditor.builders = word;

	// Read help file entries
        for (;;)
        {

                pHelp = (HELP_DATA *) alloc_perm(sizeof(*pHelp));
                pHelp->level = fread_number(fp);
                pHelp->keyword = fread_string(fp);
                if (pHelp->keyword[0] == '$')
                        break;
                pHelp->text = fread_string(fp);

                if (!str_cmp(pHelp->keyword, "greeting"))
                        help_greeting = pHelp->text;

                if (help_first == NULL)
                        help_first = pHelp;
                if (help_last != NULL)
                        help_last->next = pHelp;

                help_last = pHelp;
                pHelp->next = NULL;
                top_help++;
        }

        return;
}


/*
 * Snarf a mob section.  old style
 */
void
load_old_mob(FILE * fp)
{
	MOB_INDEX_DATA *pMobIndex;

	/* for race updating */
	int race;
	char name[MAX_STRING_LENGTH];

	if (!area_last)				/* OLC */
	{
		bug("Load_mobiles: no #AREA seen yet.", 0);
		exit(1);
	}

	for (;;)
	{
		int vnum;
		char letter;
		int iHash;

		letter = fread_letter(fp);
		if (letter != '#')
		{
			bug("Load_mobiles: # not found.", 0);
			exit(1);
		}

		vnum = fread_number(fp);
		if (vnum == 0)
			break;

		fBootDb = FALSE;
		if (get_mob_index(vnum) != NULL)
		{
			bug("Load_mobiles: vnum %d duplicated.", vnum);
			exit(1);
		}
		fBootDb = TRUE;

		pMobIndex = (MOB_INDEX_DATA *) alloc_perm(sizeof(*pMobIndex));
		pMobIndex->vnum = vnum;
		pMobIndex->area = area_last;	/* OLC */
		pMobIndex->new_format = FALSE;
		pMobIndex->player_name = fread_string(fp);
		pMobIndex->short_descr = fread_string(fp);
		pMobIndex->long_descr = fread_string(fp);
		pMobIndex->description = fread_string(fp);

		pMobIndex->long_descr[0] = UPPER(pMobIndex->long_descr[0]);
		pMobIndex->description[0] = UPPER(pMobIndex->description[0]);

		pMobIndex->act = fread_flag(fp) | ACT_IS_NPC;
		pMobIndex->affected_by = fread_flag(fp);
		pMobIndex->pShop = NULL;
		pMobIndex->alignment = fread_number(fp);
		letter = fread_letter(fp);
		pMobIndex->level = fread_number(fp);

		/*
		 * The unused stuff is for imps who want to use the old-style
		 * stats-in-files method.
		 */
		fread_number(fp);		/* Unused */
		fread_number(fp);		/* Unused */
		fread_number(fp);		/* Unused */
		/* 'd'          */ fread_letter(fp);
		/* Unused */
		fread_number(fp);		/* Unused */
		/* '+'          */ fread_letter(fp);
		/* Unused */
		fread_number(fp);		/* Unused */
		fread_number(fp);		/* Unused */
		/* 'd'          */ fread_letter(fp);
		/* Unused */
		fread_number(fp);		/* Unused */
		/* '+'          */ fread_letter(fp);
		/* Unused */
		fread_number(fp);		/* Unused */
		pMobIndex->wealth = fread_number(fp) / 20;
		/* xp can't be used! */ fread_number(fp);
		/* Unused */
		pMobIndex->start_pos = fread_number(fp);	/* Unused */
		pMobIndex->default_pos = fread_number(fp);	/* Unused */

		if (pMobIndex->start_pos < POS_SLEEPING)
			pMobIndex->start_pos = POS_STANDING;
		if (pMobIndex->default_pos < POS_SLEEPING)
			pMobIndex->default_pos = POS_STANDING;

		/*
		 * Back to meaningful values.
		 */
		pMobIndex->sex = fread_number(fp);

		/* compute the race BS */
		one_argument(pMobIndex->player_name, name);

		if (name[0] == '\0' || (race = race_lookup(name)) == 0)
		{
			/* fill in with blanks */
			pMobIndex->race = race_lookup("human");
			pMobIndex->off_flags = OFF_DODGE | OFF_DISARM | OFF_TRIP | ASSIST_VNUM;
			pMobIndex->imm_flags = 0;
			pMobIndex->res_flags = 0;
			pMobIndex->vuln_flags = 0;
			pMobIndex->form = FORM_EDIBLE | FORM_SENTIENT | FORM_BIPED | FORM_MAMMAL;
			pMobIndex->parts = PART_HEAD | PART_ARMS | PART_LEGS | PART_HEART |
				PART_BRAINS | PART_GUTS;
		}
		else
		{
			pMobIndex->race = race;
			pMobIndex->off_flags = OFF_DODGE | OFF_DISARM | OFF_TRIP | ASSIST_RACE |
				race_table[race].off;
			pMobIndex->imm_flags = race_table[race].imm;
			pMobIndex->res_flags = race_table[race].res;
			pMobIndex->vuln_flags = race_table[race].vuln;
			pMobIndex->form = race_table[race].form;
			pMobIndex->parts = race_table[race].parts;
		}

		if (letter != 'S')
		{
			bug("Load_mobiles: vnum %d non-S.", vnum);
			exit(1);
		}

		convert_mobile(pMobIndex);	/* ROM OLC */

		iHash = vnum % MAX_KEY_HASH;
		pMobIndex->next = mob_index_hash[iHash];
		mob_index_hash[iHash] = pMobIndex;
		top_mob_index++;
		top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;	/* OLC */
		assign_area_vnum(vnum);	/* OLC */
		kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL - 1)].number++;
	}

	return;
}

/*
 * Snarf an obj section.  old style
 */
void
load_old_obj(FILE * fp)
{
	OBJ_INDEX_DATA *pObjIndex;

	if (!area_last)				/* OLC */
	{
		bug("Load_objects: no #AREA seen yet.", 0);
		exit(1);
	}

	for (;;)
	{
		int vnum;
		char letter;
		int iHash;

		letter = fread_letter(fp);
		if (letter != '#')
		{
			bug("Load_objects: # not found.", 0);
			exit(1);
		}

		vnum = fread_number(fp);
		if (vnum == 0)
			break;

		fBootDb = FALSE;
		if (get_obj_index(vnum) != NULL)
		{
			bug("Load_objects: vnum %d duplicated.", vnum);
			exit(1);
		}
		fBootDb = TRUE;

		pObjIndex = (OBJ_INDEX_DATA *) alloc_perm(sizeof(*pObjIndex));
		pObjIndex->vnum = vnum;
		pObjIndex->area = area_last;	/* OLC */
		pObjIndex->new_format = FALSE;
		pObjIndex->reset_num = 0;
		pObjIndex->name = fread_string(fp);
		pObjIndex->short_descr = fread_string(fp);
		pObjIndex->description = fread_string(fp);
		/* Action description */ fread_string(fp);

		pObjIndex->short_descr[0] = LOWER(pObjIndex->short_descr[0]);
		pObjIndex->description[0] = UPPER(pObjIndex->description[0]);
		pObjIndex->material = str_dup("");

		pObjIndex->item_type = fread_number(fp);
		pObjIndex->extra_flags = fread_flag(fp);
		pObjIndex->wear_flags = fread_flag(fp);
		pObjIndex->value[0] = fread_number(fp);
		pObjIndex->value[1] = fread_number(fp);
		pObjIndex->value[2] = fread_number(fp);
		pObjIndex->value[3] = fread_number(fp);
		pObjIndex->value[4] = 0;
		pObjIndex->level = 0;
		pObjIndex->condition = 100;
		pObjIndex->weight = fread_number(fp);
		pObjIndex->cost = fread_number(fp);		/* Unused */
		/* Cost per day */ fread_number(fp);


		if (pObjIndex->item_type == ITEM_WEAPON)
		{
			if (is_name("two", pObjIndex->name)
				|| is_name("two-handed", pObjIndex->name)
				|| is_name("claymore", pObjIndex->name))
				SET_BIT(pObjIndex->value[4], WEAPON_TWO_HANDS);
		}

		for (;;)
		{
			char letter;

			letter = fread_letter(fp);

			if (letter == 'A')
			{
				AFFECT_DATA *paf;

				paf = (AFFECT_DATA *) alloc_perm(sizeof(*paf));
				paf->where = TO_OBJECT;
				paf->type = -1;
				paf->level = 20;	/* RT temp fix */
				paf->duration = -1;
				paf->location = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->bitvector = 0;
				paf->next = pObjIndex->affected;
				pObjIndex->affected = paf;
				top_affect++;
			}

			else if (letter == 'E')
			{
				EXTRA_DESCR_DATA *ed;

				ed = (EXTRA_DESCR_DATA *) alloc_perm(sizeof(*ed));
				ed->keyword = fread_string(fp);
				ed->description = fread_string(fp);
				ed->next = pObjIndex->extra_descr;
				pObjIndex->extra_descr = ed;
				top_ed++;
			}

			else
			{
				ungetc(letter, fp);
				break;
			}
		}

		/* fix armors */
		if (pObjIndex->item_type == ITEM_ARMOR)
		{
			pObjIndex->value[1] = pObjIndex->value[0];
			pObjIndex->value[2] = pObjIndex->value[1];
		}

		/*
		 * Translate spell "slot numbers" to internal "skill numbers."
		 */
		switch (pObjIndex->item_type)
		{
		case ITEM_PILL:
		case ITEM_POTION:
		case ITEM_SCROLL:
			pObjIndex->value[1] = slot_lookup(pObjIndex->value[1]);
			pObjIndex->value[2] = slot_lookup(pObjIndex->value[2]);
			pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]);
			pObjIndex->value[4] = slot_lookup(pObjIndex->value[4]);
			break;

		case ITEM_STAFF:
		case ITEM_WAND:
			pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]);
			break;
		}

		iHash = vnum % MAX_KEY_HASH;
		pObjIndex->next = obj_index_hash[iHash];
		obj_index_hash[iHash] = pObjIndex;
		top_obj_index++;
		top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;	/* OLC */
		assign_area_vnum(vnum);	/* OLC */
	}

	return;
}

/*
 * Adds a reset to a room.  OLC
 * Similar to add_reset in olc.c
 */
void
new_reset(ROOM_INDEX_DATA * pR, RESET_DATA * pReset)
{
	RESET_DATA *pr;

	if (!pR)
		return;

	pr = pR->reset_last;

	if (!pr)
	{
		pR->reset_first = pReset;
		pR->reset_last = pReset;
	}
	else
	{
		pR->reset_last->next = pReset;
		pR->reset_last = pReset;
		pR->reset_last->next = NULL;
	}

	top_reset++;
	return;
}

/*
 * Snarf a reset section.
 */
void
load_resets(FILE * fp)
{
	RESET_DATA *pReset;
	int iLastRoom = 0;
	int iLastObj = 0;
	int percent = 0;

	if (!area_last)
	{
		bug("Load_resets: no #AREA seen yet.", 0);
		exit(1);
	}

	for (;;)
	{
		ROOM_INDEX_DATA *pRoomIndex;
		EXIT_DATA *pexit;
		char letter;

		if ((letter = fread_letter(fp)) == 'S')
			break;

		if (letter == '*')
		{
			fread_to_eol(fp);
			continue;
		}

		// Percent change for the next reset.
		if (letter == 'C')
		{
			percent = fread_number(fp);
			fread_to_eol(fp);
			continue;
		}

		pReset = (RESET_DATA *) alloc_perm(sizeof(*pReset));
		pReset->command = letter;
		/* if_flag */ fread_number(fp);
		pReset->arg1 = fread_number(fp);
		pReset->arg2 = fread_number(fp);
		pReset->arg3 = (letter == 'G' || letter == 'R')
			? 0 : fread_number(fp);
		pReset->arg4 = (letter == 'P' || letter == 'M')
			? fread_number(fp) : 0;

		// If this reset is percent based, apply the previously
		// read argument and reset.
		if(percent > 0) {
			pReset->percent = percent;
			percent = 0;
		}

		fread_to_eol(fp);

		/*
		 * Validate parameters.
		 * We're calling the index functions for the side effect.
		 */
		switch (letter)
		{
		default:
			bug("Load_resets: bad command '%c'.", letter);
			exit(1);
			break;

		case 'M':
			get_mob_index(pReset->arg1);
			if ((pRoomIndex = get_room_index(pReset->arg3)))
			{
				new_reset(pRoomIndex, pReset);
				iLastRoom = pReset->arg3;
			}
			break;

		case 'O':
			get_obj_index(pReset->arg1);
			if ((pRoomIndex = get_room_index(pReset->arg3)))
			{
				new_reset(pRoomIndex, pReset);
				iLastObj = pReset->arg3;
			}
			break;

		case 'P':
			get_obj_index(pReset->arg1);
			if ((pRoomIndex = get_room_index(iLastObj)))
			{
				new_reset(pRoomIndex, pReset);
			}
			break;

		case 'G':
		case 'E':
			get_obj_index(pReset->arg1);
			if ((pRoomIndex = get_room_index(iLastRoom)))
			{
				new_reset(pRoomIndex, pReset);
				iLastObj = iLastRoom;
			}
			break;

		case 'D':
			pRoomIndex = get_room_index(pReset->arg1);

			if (pReset->arg2 < 0
				|| pReset->arg2 > 5
				|| !pRoomIndex
				|| !(pexit = pRoomIndex->exit[pReset->arg2])
				|| !IS_SET(pexit->rs_flags, EX_ISDOOR))
			{
				bug("Load_resets: 'D': exit %d not door.", pReset->arg2);
				exit(1);
			}

			switch (pReset->arg3)
			{
			default:
				bug("Load_resets: 'D': bad 'locks': %d.", pReset->arg3);
			case 0:
				break;
			case 1:
				SET_BIT(pexit->rs_flags, EX_CLOSED);
				SET_BIT(pexit->exit_info, EX_CLOSED);
				break;
			case 2:
				SET_BIT(pexit->rs_flags, EX_CLOSED | EX_LOCKED);
				SET_BIT(pexit->exit_info, EX_CLOSED | EX_LOCKED);
				break;
			}

			break;

		case 'R':
			pRoomIndex = get_room_index(pReset->arg1);

			if (pReset->arg2 < 0 || pReset->arg2 > 6)
			{
				bug("Load_resets: 'R': bad exit %d.", pReset->arg2);
				exit(1);
			}

			if (pRoomIndex)
				new_reset(pRoomIndex, pReset);

			break;
		}

	}

	return;
}

/*
 * Snarf a room section.
 */
void
load_rooms(FILE * fp)
{
	ROOM_INDEX_DATA *pRoomIndex;

	if (area_last == NULL)
	{
		bug("Load_resets: no #AREA seen yet.", 0);
		exit(1);
	}

	for (;;)
	{
		int vnum;
		char letter;
		int door;
		int iHash;

		letter = fread_letter(fp);
		if (letter != '#')
		{
			bug("Load_rooms: # not found.", 0);
			exit(1);
		}

		vnum = fread_number(fp);
		if (vnum == 0)
			break;

		fBootDb = FALSE;
		if (get_room_index(vnum) != NULL)
		{
			bug("Load_rooms: vnum %d duplicated.", vnum);
			exit(1);
		}
		fBootDb = TRUE;

		pRoomIndex = (ROOM_INDEX_DATA *) alloc_perm(sizeof(*pRoomIndex));
		pRoomIndex->owner = str_dup("");
		pRoomIndex->people = NULL;
		pRoomIndex->contents = NULL;
		pRoomIndex->extra_descr = NULL;
		pRoomIndex->area = area_last;
		pRoomIndex->vnum = vnum;
		pRoomIndex->name = fread_string(fp);
		pRoomIndex->description = fread_string(fp);
		/* Area number */ fread_number(fp);
		pRoomIndex->room_flags = fread_flag(fp);
		/* horrible hack */
		if (3000 <= vnum && vnum < 3400)
			SET_BIT(pRoomIndex->room_flags, ROOM_LAW);
		pRoomIndex->sector_type = fread_number(fp);
		pRoomIndex->light = 0;
		for (door = 0; door <= 5; door++)
			pRoomIndex->exit[door] = NULL;

		/* defaults */
		pRoomIndex->heal_rate = 100;
		pRoomIndex->mana_rate = 100;

		for (;;)
		{
			letter = fread_letter(fp);

			if (letter == 'S')
				break;

			if (letter == 'H')	/* healing room */
				pRoomIndex->heal_rate = fread_number(fp);

			else if (letter == 'M')		/* mana room */
				pRoomIndex->mana_rate = fread_number(fp);

			else if (letter == 'X')		/* Sound file */
				pRoomIndex->sound = fread_string(fp);

			else if (letter == 'C')		/* clan */
			{
				if (pRoomIndex->clan)
				{
					bug("Load_rooms: duplicate clan fields.", 0);
					exit(1);
				}
				pRoomIndex->clan = clan_lookup(fread_string(fp));
			}


			else if (letter == 'D')
			{
				EXIT_DATA *pexit;
				int locks;

				door = fread_number(fp);
				if (door < 0 || door > 5)
				{
					bug("Fread_rooms: vnum %d has bad door number.", vnum);
					exit(1);
				}

				pexit = (EXIT_DATA *) alloc_perm(sizeof(*pexit));
				pexit->description = fread_string(fp);
				pexit->keyword = fread_string(fp);
				pexit->exit_info = 0;
				pexit->rs_flags = 0;	/* OLC */
				locks = fread_number(fp);
				pexit->key = fread_number(fp);
				pexit->u1.vnum = fread_number(fp);
				pexit->orig_door = door;	/* OLC */

				switch (locks)
				{
				case 1:
					pexit->exit_info = EX_ISDOOR;
					pexit->rs_flags = EX_ISDOOR;
					break;
				case 2:
					pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
					pexit->rs_flags = EX_ISDOOR | EX_PICKPROOF;
					break;
				case 3:
					pexit->exit_info = EX_ISDOOR | EX_NOPASS;
					pexit->rs_flags = EX_ISDOOR | EX_NOPASS;
					break;
				case 4:
					pexit->exit_info = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
					pexit->rs_flags = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
					break;
				}

				pRoomIndex->exit[door] = pexit;
				pRoomIndex->old_exit[door] = pexit;
				top_exit++;
			}
			else if (letter == 'E')
			{
				EXTRA_DESCR_DATA *ed;

				ed = (EXTRA_DESCR_DATA *) alloc_perm(sizeof(*ed));
				ed->keyword = fread_string(fp);
				ed->description = fread_string(fp);
				ed->next = pRoomIndex->extra_descr;
				pRoomIndex->extra_descr = ed;
				top_ed++;
			}

			else if (letter == 'O')
			{
				if (pRoomIndex->owner[0] != '\0')
				{
					bug("Load_rooms: duplicate owner.", 0);
					exit(1);
				}

				pRoomIndex->owner = fread_string(fp);
			}

			else
			{
				bug("Load_rooms: vnum %d has flag not 'DES'.", vnum);
				exit(1);
			}
		}

		iHash = vnum % MAX_KEY_HASH;
		pRoomIndex->next = room_index_hash[iHash];
		room_index_hash[iHash] = pRoomIndex;
		top_room++;
		top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;	/* OLC */
		assign_area_vnum(vnum);	/* OLC */
	}

	return;
}



/*
 * Snarf a shop section.
 */
void
load_shops(FILE * fp)
{
	SHOP_DATA *pShop;

	for (;;)
	{
		MOB_INDEX_DATA *pMobIndex;
		int iTrade;

		pShop = (SHOP_DATA *) alloc_perm(sizeof(*pShop));
		pShop->keeper = fread_number(fp);
		if (pShop->keeper == 0)
			break;
		for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
			pShop->buy_type[iTrade] = fread_number(fp);
		pShop->profit_buy = fread_number(fp);
		pShop->profit_sell = fread_number(fp);
		pShop->open_hour = fread_number(fp);
		pShop->close_hour = fread_number(fp);
		fread_to_eol(fp);
		pMobIndex = get_mob_index(pShop->keeper);
		pMobIndex->pShop = pShop;

		if (shop_first == NULL)
			shop_first = pShop;
		if (shop_last != NULL)
			shop_last->next = pShop;

		shop_last = pShop;
		pShop->next = NULL;
		top_shop++;
	}

	return;
}


/*
 * Snarf spec proc declarations.
 */
void
load_specials(FILE * fp)
{
	for (;;)
	{
		MOB_INDEX_DATA *pMobIndex;
		char letter;

		switch (letter = fread_letter(fp))
		{
		default:
			bug("Load_specials: letter '%c' not *MS.", letter);
			exit(1);

		case 'S':
			return;

		case '*':
			break;

		case 'M':
			pMobIndex = get_mob_index(fread_number(fp));
			pMobIndex->spec_fun = spec_lookup(fread_word(fp));
			if (pMobIndex->spec_fun == 0)
			{
				bug("Load_specials: 'M': vnum %d.", pMobIndex->vnum);
				exit(1);
			}
			break;
		}

		fread_to_eol(fp);
	}
}


/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void
fix_exits(void)
{
	ROOM_INDEX_DATA *pRoomIndex;
	EXIT_DATA *pexit;
	int iHash;
	int door;

	for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
	{
		for (pRoomIndex = room_index_hash[iHash];
			 pRoomIndex != NULL;
			 pRoomIndex = pRoomIndex->next)
		{
			bool fexit;

			fexit = FALSE;
			for (door = 0; door <= 5; door++)
			{
				if ((pexit = pRoomIndex->exit[door]) != NULL)
				{
					if (pexit->u1.vnum <= 0
						|| get_room_index(pexit->u1.vnum) == NULL)
						pexit->u1.to_room = NULL;
					else
					{
						fexit = TRUE;
						pexit->u1.to_room = get_room_index(pexit->u1.vnum);
					}
				}
			}
			if (!fexit)
				SET_BIT(pRoomIndex->room_flags, ROOM_NO_MOB);
		}
	}

/*
   for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
   {
   for (pRoomIndex = room_index_hash[iHash];
   pRoomIndex != NULL;
   pRoomIndex = pRoomIndex->next)
   {
   for (door = 0; door <= 5; door++)
   {
   if ((pexit = pRoomIndex->exit[door]) != NULL
   && (to_room = pexit->u1.to_room) != NULL
   && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
   && pexit_rev->u1.to_room != pRoomIndex
   && (pRoomIndex->vnum < 1200 || pRoomIndex->vnum > 1299))
   {
   sprintf (buf, "Fix_exits: %d:%d -> %d:%d -> %d.",
   pRoomIndex->vnum, door,
   to_room->vnum, rev_dir[door],
   (pexit_rev->u1.to_room == NULL)
   ? 0 : pexit_rev->u1.to_room->vnum);
   bug (buf, 0);
   }
   }
   }
   }
 */

	return;
}



/*
 * Load mobprogs section
 */
void
load_mobprogs(FILE * fp)
{
	return;
	/*MPROG_CODE *pMprog;
	char letter;

	if (area_last == NULL)
	{
		bug("Load_mobprogs: no #AREA seen yet.", 0);
		exit(1);
	}

	letter = fread_letter(fp);

	for (;;)
	{
		int vnum;

		if (letter != '#')
		{
			bug("Load_mobprogs: # not found.", 0);
			exit(1);
		}

		vnum = fread_number(fp);
		if (vnum == 0)
			break;

		fBootDb = FALSE;
		if (get_mprog_index(vnum) != NULL)
		{
			bug("Load_mobprogs: vnum %d duplicated.", vnum);
			exit(1);
		}
		fBootDb = TRUE;


		pMprog = new_mprog_code();
		pMprog->vnum = vnum;
		pMprog->code = fread_string(fp);

		//If it's labeled, check and see if we have approval info
		letter = fread_letter(fp);
		if (letter == 'A')
		{
			pMprog->approved = 1;
			letter = fread_letter(fp);
		}
		else if (letter == 'N')
		{
			pMprog->approved = 0;
			letter = fread_letter(fp);
		}
		else
			pMprog->approved = 0;

		if (mprog_list == NULL)
			mprog_list = pMprog;
		else
		{
			pMprog->next = mprog_list;
			mprog_list = pMprog;
		}
		if (top_vnum_mprog < vnum)
			top_vnum_mprog = vnum;

	}

	return;*/
}

/*
 *  Translate mobprog vnums pointers to real code
 */
void
fix_mobprogs(void) {
	return;
  /*  MOB_INDEX_DATA *pMobIndex;
    MPROG_LIST *list;
    MPROG_CODE *prog;
    int iHash;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
        for (pMobIndex = mob_index_hash[iHash]; pMobIndex != NULL; pMobIndex = pMobIndex->next) {
            for (list = pMobIndex->mprogs; list != NULL; list = list->next) {
                if ((prog = get_mprog_index(list->vnum)) != NULL) {
                    list->program = prog;
                } else {
                    bug("Fix_mobprogs: code vnum %d not found.", list->vnum);
                    exit(1);
                }
            }
        }
    }*/
}
/*
MPROG_CODE *
get_mprog_index(int vnum) {
    MPROG_CODE *prg;

    for (prg = mprog_list; prg; prg = prg->next) {
        if (prg->vnum == vnum) {
            return (prg);
        }
    }

    return NULL;
}*/



/*
 * Repopulate areas periodically.
 */
void
area_update(void)
{
	AREA_DATA *pArea;
	char buf[MAX_STRING_LENGTH];

	for (pArea = area_first; pArea != NULL; pArea = pArea->next)
	{

		if (++pArea->age < 3)
			continue;

		/*
		 * Check age and reset.
		 * Note: Mud School resets every 3 minutes (not 15).
		 */

/* Changed by delstar to account for area age set by OLC

   if ((!pArea->empty && (pArea->nplayer == 0 || pArea->age >= 15))
   || pArea->age >= 31)
 */

		if (pArea->age >= pArea->age_count || pArea->age >= 30)
		{
			reset_area(pArea, FALSE);
			sprintf(buf, "%s has just been reset.", pArea->name);
			wiznet(buf, NULL, NULL, WIZ_RESETS, 0, 0);

			pArea->age = 0;

/* More changes by Delstar for the age thing

   pRoomIndex = get_room_index (ROOM_VNUM_SCHOOL);
   if (pRoomIndex != NULL && pArea == pRoomIndex->area)
   pArea->age = 15 - 2;
 */
			if (pArea->nplayer == 0)
				pArea->empty = TRUE;
		}
	}

	return;
}

/* OLC
 * Reset one room.  Called by reset_area and olc.
 * 
 * manualReset is true when this is called by a player, e.g. through redit reset.
 */
void
reset_room(ROOM_INDEX_DATA *pRoom, bool manualReset)
{
	RESET_DATA *pReset;
	CHAR_DATA *pMob;
	CHAR_DATA *mob;
	OBJ_DATA *pObj;
	CHAR_DATA *LastMob = NULL;
	OBJ_DATA *LastObj = NULL;
	int iExit;
	int level = 0;
	bool last;

	if (!pRoom)
		return;

	pMob = NULL;
	last = FALSE;

	for (iExit = 0; iExit < MAX_DIR; iExit++)
	{
		EXIT_DATA *pExit;
		if ((pExit = pRoom->exit[iExit]))
		{
			pExit->exit_info = pExit->rs_flags;
			if ((pExit->u1.to_room != NULL)
				&& ((pExit = pExit->u1.to_room->exit[rev_dir[iExit]])))
			{
				/* nail the other side */
				pExit->exit_info = pExit->rs_flags;
			}
		}
	}

	for (pReset = pRoom->reset_first; pReset != NULL; pReset = pReset->next)
	{
		MOB_INDEX_DATA *pMobIndex;
		OBJ_INDEX_DATA *pObjIndex;
		OBJ_INDEX_DATA *pObjToIndex;
		ROOM_INDEX_DATA *pRoomIndex;
		char buf[MAX_STRING_LENGTH];
		int count, limit = 0;

		// Woot rare resets don't always kick in!
		if(pReset->percent > 0) {
			if(number_percent() > pReset->percent) {
				continue;
			}
		}
		switch (pReset->command)
		{
		default:
			bug("Reset_room: bad command %c.", pReset->command);
			break;

		case 'M':
			if (!(pMobIndex = get_mob_index(pReset->arg1)))
			{
				bug("Reset_room: 'M': bad vnum %d.", pReset->arg1);
				continue;
			}

			if ((pRoomIndex = get_room_index(pReset->arg3)) == NULL)
			{
				bug("Reset_area: 'R': bad vnum %d.", pReset->arg3);
				continue;
			}
			if (pMobIndex->count >= pReset->arg2)
			{
				last = FALSE;
				break;
			}
/* */
			count = 0;
			for (mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room)
				if (mob->pIndexData == pMobIndex)
				{
					count++;
					if (count >= pReset->arg4)
					{
						last = FALSE;
						break;
					}
				}

			if (count >= pReset->arg4)
				break;

			pMob = create_mobile(pMobIndex);

			/*
			 * Some more hard coding.
			 */

			/*
			 * Pet shop mobiles get ACT_PET set.
			 */
			{
				ROOM_INDEX_DATA *pRoomIndexPrev;

				pRoomIndexPrev = get_room_index(pRoom->vnum - 1);
				if (pRoomIndexPrev
					&& IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
					SET_BIT(pMob->act, ACT_PET);
			}

			char_to_room(pMob, pRoom);

			LastMob = pMob;
			level = URANGE(0, pMob->level - 2, LEVEL_HERO - 1);		/* -1 ROM */
			last = TRUE;
			break;

		case 'O':
			if (!(pObjIndex = get_obj_index(pReset->arg1)))
			{
				bug("Reset_room: 'O' 1 : bad vnum %d", pReset->arg1);
				sprintf(buf, "%d %d %d %d", pReset->arg1, pReset->arg2, pReset->arg3,
						pReset->arg4);
				bug(buf, 1);
				continue;
			}

			if (!(pRoomIndex = get_room_index(pReset->arg3)))
			{
				bug("Reset_room: 'O' 2 : bad vnum %d.", pReset->arg3);
				sprintf(buf, "%d %d %d %d", pReset->arg1, pReset->arg2, pReset->arg3,
						pReset->arg4);
				bug(buf, 1);
				continue;
			}

			if ( (pRoom->area->nplayer > 0 && !manualReset)
				|| count_obj_list(pObjIndex, pRoom->contents) > 0)
			{
				last = FALSE;
				break;
			}

			pObj = create_object(pObjIndex,		/* UMIN - ROM OLC */
								 UMIN(number_fuzzy(level), LEVEL_HERO - 1));
			pObj->cost = 0;
			obj_to_room(pObj, pRoom);
			last = TRUE;
			break;

		case 'P':
			if (!(pObjIndex = get_obj_index(pReset->arg1)))
			{
				bug("Reset_room: 'P': bad vnum %d.", pReset->arg1);
				continue;
			}

			if (!(pObjToIndex = get_obj_index(pReset->arg3)))
			{
				bug("Reset_room: 'P': bad vnum %d.", pReset->arg3);
				continue;
			}

			if (pReset->arg2 > 50)	/* old format */
				limit = 6;
			else if (pReset->arg2 == -1)	/* no limit */
				limit = 999;
			else
				limit = pReset->arg2;

			if ((LastObj = get_obj_type(pObjToIndex)) == NULL
				|| (LastObj->in_room == NULL && !last)
			 || (pObjIndex->count >= limit /* && number_range(0,4) != 0 */ )
				|| (count = count_obj_list(pObjIndex, LastObj->contains)) > pReset->arg4)
			{
				last = FALSE;
				break;
			}

			while (count < pReset->arg4)
			{
				pObj = create_object(pObjIndex, number_fuzzy(LastObj->level));
				obj_to_obj(pObj, LastObj);
				count++;
				if (pObjIndex->count >= limit)
					break;
			}

			/* fix object lock state! */
			LastObj->value[1] = LastObj->pIndexData->value[1];
			last = TRUE;
			break;

		case 'G':
		case 'E':
			if (!(pObjIndex = get_obj_index(pReset->arg1)))
			{
				bug("Reset_room: 'E' or 'G': bad vnum %d.", pReset->arg1);
				continue;
			}

			if (!last)
				break;

			if (!LastMob)
			{
				bug("Reset_room: 'E' or 'G': null mob for vnum %d.",
					pReset->arg1);
				last = FALSE;
				break;
			}

			if (LastMob->pIndexData->pShop)		/* Shop-keeper? */
			{
				int olevel = 0, i, j;

				if (!pObjIndex->new_format)
					switch (pObjIndex->item_type)
					{
					default:
						olevel = 0;
						break;
					case ITEM_PILL:
					case ITEM_POTION:
					case ITEM_SCROLL:
						olevel = 53;
						for (i = 1; i < 5; i++)
						{
							if (pObjIndex->value[i] > 0)
							{
								for (j = 0; j < MAX_CLASS; j++)
								{
									olevel = UMIN(olevel,
										   skill_table[pObjIndex->value[i]].
												  skill_level[j]);
								}
							}
						}

						olevel = UMAX(0, (olevel * 3 / 4) - 2);
						break;

					case ITEM_WAND:
						olevel = number_range(10, 20);
						break;
					case ITEM_STAFF:
						olevel = number_range(15, 25);
						break;
					case ITEM_ARMOR:
						olevel = number_range(5, 15);
						break;
						/* ROM patch weapon, treasure */
					case ITEM_WEAPON:
						olevel = number_range(5, 15);
						break;
					case ITEM_TREASURE:
						olevel = number_range(10, 20);
						break;
					}

				pObj = create_object(pObjIndex, olevel);
				SET_BIT(pObj->extra_flags, ITEM_INVENTORY);		/* ROM OLC */
			}
			else
				/* ROM OLC else version */
			{
				int limit;

				if (pReset->arg2 > 50)	/* old format */
					limit = 6;
				else if (pReset->arg2 == -1 || pReset->arg2 == 0)	/* no limit */
					limit = 999;
				else
					limit = pReset->arg2;

				if (pObjIndex->count < limit || number_range(0, 4) == 0)
				{
					pObj = create_object(pObjIndex, UMIN(number_fuzzy(level), LEVEL_HERO - 1));

				}
				else
					break;
			}


			obj_to_char(pObj, LastMob);
			if (pReset->command == 'E')
				equip_char(LastMob, pObj, pReset->arg3);
			last = TRUE;
			break;

		case 'D':
			break;

		case 'R':
			if (!(pRoomIndex = get_room_index(pReset->arg1)))
			{
				bug("Reset_room: 'R': bad vnum %d.", pReset->arg1);
				continue;
			}

			{
				EXIT_DATA *pExit;
				int d0;
				int d1;

				for (d0 = 0; d0 < pReset->arg2 - 1; d0++)
				{
					d1 = number_range(d0, pReset->arg2 - 1);
					pExit = pRoomIndex->exit[d0];
					pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
					pRoomIndex->exit[d1] = pExit;
				}
			}
			break;
		}
	}

	return;
}

/* OLC
 * Reset one area.
 */
void
reset_area(AREA_DATA * pArea, bool manualReset)
{
	ROOM_INDEX_DATA *pRoom;
	int vnum;

	for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
	{
		if ((pRoom = get_room_index(vnum)))
			reset_room(pRoom, manualReset);
	}

	return;
}

/*
 * Create an instance of a mobile.
 */
CHAR_DATA *
create_mobile(MOB_INDEX_DATA * pMobIndex)
{
	CHAR_DATA *mob;
	int i;
	AFFECT_DATA af;

	mobile_count++;

	if (pMobIndex == NULL)
	{
		bug("Create_mobile: NULL pMobIndex.", 0);
		exit(1);
	}

	mob = new_char();

	mob->pIndexData = pMobIndex;

	mob->name = str_dup(pMobIndex->player_name);	/* OLC */
	mob->short_descr = str_dup(pMobIndex->short_descr);		/* OLC */
	mob->long_descr = str_dup(pMobIndex->long_descr);	/* OLC */
	mob->description = str_dup(pMobIndex->description);		/* OLC */
	mob->id = get_mob_id();
	mob->spec_fun = pMobIndex->spec_fun;
	mob->prompt = NULL;
	mob->mprog_target = NULL;
	mob->on_who = FALSE;
	mob->is_clone = FALSE;

	if (pMobIndex->wealth == 0)
	{
		mob->silver = 0;
		mob->gold = 0;
	}
	else
	{
		long wealth;

		wealth = number_range(pMobIndex->wealth / 2, 3 * pMobIndex->wealth / 2);
		mob->gold = number_range(wealth / 200, wealth / 100);
		mob->silver = wealth - (mob->gold * 100);
	}

	if (pMobIndex->new_format)
		/* load in new style */
	{
		/* read from prototype */
		mob->group = pMobIndex->group;
		mob->act = pMobIndex->act;
		mob->comm = COMM_NOCHANNELS | COMM_NOSHOUT | COMM_NOTELL;
		mob->affected_by = pMobIndex->affected_by;
		mob->alignment = pMobIndex->alignment;
		mob->level = pMobIndex->level;
		mob->hitroll = pMobIndex->hitroll;
		mob->damroll = pMobIndex->damage[DICE_BONUS];
		mob->max_hit = dice(pMobIndex->hit[DICE_NUMBER],
							pMobIndex->hit[DICE_TYPE])
			+ pMobIndex->hit[DICE_BONUS];
		mob->hit = mob->max_hit;
		mob->max_mana = dice(pMobIndex->mana[DICE_NUMBER],
							 pMobIndex->mana[DICE_TYPE])
			+ pMobIndex->mana[DICE_BONUS];
		mob->mana = mob->max_mana;
		mob->damage[DICE_NUMBER] = pMobIndex->damage[DICE_NUMBER];
		mob->damage[DICE_TYPE] = pMobIndex->damage[DICE_TYPE];
		mob->dam_type = pMobIndex->dam_type;
		if (mob->dam_type == 0)
			switch (number_range(1, 3))
			{
			case (1):
				mob->dam_type = 3;
				break;			/* slash */
			case (2):
				mob->dam_type = 7;
				break;			/* pound */
			case (3):
				mob->dam_type = 11;
				break;			/* pierce */
			}
		for (i = 0; i < 4; i++)
			mob->armor[i] = pMobIndex->ac[i];
		mob->off_flags = pMobIndex->off_flags;
		mob->imm_flags = pMobIndex->imm_flags;
		mob->res_flags = pMobIndex->res_flags;
		mob->vuln_flags = pMobIndex->vuln_flags;
		mob->start_pos = pMobIndex->start_pos;
		mob->default_pos = pMobIndex->default_pos;
		mob->sex = pMobIndex->sex;
		if (mob->sex == 3)		/* random sex */
			mob->sex = number_range(1, 2);
		mob->clan = pMobIndex->clan;	
		mob->race = pMobIndex->race;
		mob->form = pMobIndex->form;
		mob->parts = pMobIndex->parts;
		mob->size = pMobIndex->size;
		mob->material = str_dup(pMobIndex->material);

		/* computed on the spot */

		for (i = 0; i < MAX_STATS; i++)
			mob->perm_stat[i] = UMIN(25, 11 + mob->level / 4);

		if (IS_SET(mob->act, ACT_WARRIOR))
		{
			mob->perm_stat[STAT_STR] += 3;
			mob->perm_stat[STAT_INT] -= 1;
			mob->perm_stat[STAT_CON] += 2;
		}

		if (IS_SET(mob->act, ACT_THIEF))
		{
			mob->perm_stat[STAT_DEX] += 3;
			mob->perm_stat[STAT_INT] += 1;
			mob->perm_stat[STAT_WIS] -= 1;
		}

		if (IS_SET(mob->act, ACT_CLERIC))
		{
			mob->perm_stat[STAT_WIS] += 3;
			mob->perm_stat[STAT_DEX] -= 1;
			mob->perm_stat[STAT_STR] += 1;
		}

		if (IS_SET(mob->act, ACT_MAGE))
		{
			mob->perm_stat[STAT_INT] += 3;
			mob->perm_stat[STAT_STR] -= 1;
			mob->perm_stat[STAT_DEX] += 1;
		}

		if (IS_SET(mob->off_flags, OFF_FAST))
			mob->perm_stat[STAT_DEX] += 2;

		mob->perm_stat[STAT_STR] += mob->size - SIZE_MEDIUM;
		mob->perm_stat[STAT_CON] += (mob->size - SIZE_MEDIUM) / 2;

		/* let's get some spell action */
		if (IS_AFFECTED(mob, AFF_SANCTUARY))
		{
			af.where = TO_AFFECTS;
			af.type = gsn_sanctuary;
			af.level = mob->level;
			af.duration = -1;
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.bitvector = AFF_SANCTUARY;
			affect_to_char(mob, &af);
		}

		if (IS_AFFECTED(mob, AFF_HASTE))
		{
			af.where = TO_AFFECTS;
			af.type = gsn_haste;
			af.level = mob->level;
			af.duration = -1;
			af.location = APPLY_DEX;
			af.modifier = 1 + (mob->level >= 18) + (mob->level >= 25) +
				(mob->level >= 32);
			af.bitvector = AFF_HASTE;
			affect_to_char(mob, &af);
		}

		if (IS_AFFECTED(mob, AFF_PROTECT_EVIL))
		{
			af.where = TO_AFFECTS;
			af.type = gsn_protection_evil;
			af.level = mob->level;
			af.duration = -1;
			af.location = APPLY_SAVES;
			af.modifier = -1;
			af.bitvector = AFF_PROTECT_EVIL;
			affect_to_char(mob, &af);
		}

		if (IS_AFFECTED(mob, AFF_PROTECT_GOOD))
		{
			af.where = TO_AFFECTS;
			af.type = gsn_protection_good;
			af.level = mob->level;
			af.duration = -1;
			af.location = APPLY_SAVES;
			af.modifier = -1;
			af.bitvector = AFF_PROTECT_GOOD;
			affect_to_char(mob, &af);
		}

		if (IS_AFFECTED(mob, AFF_BLIND))
                {
                        af.where = TO_AFFECTS;
                        af.type = gsn_blindness;
                        af.level = mob->level;
                        af.duration = -1;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.bitvector = AFF_BLIND;
                        affect_to_char(mob, &af);
                }

		if (IS_AFFECTED(mob, AFF_INVISIBLE))
                {
                        af.where = TO_AFFECTS;
                        af.type = gsn_invisibility;
                        af.level = mob->level;
                        af.duration = -1;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.bitvector = AFF_INVISIBLE;
                        affect_to_char(mob, &af);
                }

		if (IS_AFFECTED(mob, AFF_MINIMATION))
                {
                        af.where = TO_AFFECTS;
                        af.type = gsn_minimation;
                        af.level = mob->level;
                        af.duration = -1;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.bitvector = AFF_MINIMATION;
                        affect_to_char(mob, &af);
                }

		if (IS_AFFECTED(mob, AFF_GRANDEUR))
                {
                        af.where = TO_AFFECTS;
                        af.type = gsn_grandeur;
                        af.level = mob->level;
                        af.duration = -1;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.bitvector = AFF_GRANDEUR;
                        affect_to_char(mob, &af);
                }

		if (IS_AFFECTED(mob, AFF_FAERIE_FIRE))
                {
                        af.where = TO_AFFECTS;
                        af.type = gsn_faerie_fire;
                        af.level = mob->level;
                        af.duration = -1;
                        af.location = APPLY_NONE;
                        af.modifier = 5 * mob->level;
                        af.bitvector = AFF_FAERIE_FIRE;
                        affect_to_char(mob, &af);
                }

		if (IS_AFFECTED(mob, AFF_POISON))
                {
                        af.where = TO_AFFECTS;
                        af.type = gsn_poison;
                        af.level = mob->level;
                        af.duration = -1;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.bitvector = AFF_POISON;
                        affect_to_char(mob, &af);
                }

		if (IS_AFFECTED(mob, AFF_PLAGUE))
                {
                        af.where = TO_AFFECTS;
                        af.type = gsn_plague;
                        af.level = mob->level;
                        af.duration = -1;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.bitvector = AFF_PLAGUE;
                        affect_to_char(mob, &af);
                }

		if (IS_AFFECTED(mob, AFF_TAUNT))
                {
                        af.where = TO_AFFECTS;
                        af.type = gsn_taunt;
                        af.level = mob->level;
                        af.duration = -1;
                        af.location = APPLY_HITROLL;
                        af.modifier = 7;
                        af.bitvector = AFF_TAUNT;
                        affect_to_char(mob, &af);
			af.location = APPLY_DAMROLL;
                        affect_to_char(mob, &af);
                }

		if (IS_AFFECTED(mob, AFF_BERSERK))
                {
                        af.where = TO_AFFECTS;
                        af.type = gsn_berserk;
                        af.level = mob->level;
                        af.duration = -1;
                        af.location = APPLY_HITROLL;
                        af.modifier = mob->level / 5;
                        af.bitvector = AFF_BERSERK;
                        affect_to_char(mob, &af);
			af.location = APPLY_DAMROLL;
                        affect_to_char(mob, &af);
			af.location = APPLY_AC;
			af.modifier = (mob->level / 5) * 10;
                        affect_to_char(mob, &af);
                }

		if (IS_AFFECTED(mob, AFF_SNEAK))
                {
                        af.where = TO_AFFECTS;
                        af.type = gsn_sneak;
                        af.level = mob->level;
                        af.duration = -1;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.bitvector = 0;
                        affect_to_char(mob, &af);
                }

	}
	else
		/* read in old format and convert */
	{
		mob->act = pMobIndex->act;
		mob->affected_by = pMobIndex->affected_by;
		mob->alignment = pMobIndex->alignment;
		mob->level = pMobIndex->level;
		mob->hitroll = pMobIndex->hitroll;
		mob->damroll = 0;
		mob->max_hit = mob->level * 8 + number_range(
												mob->level * mob->level / 4,
												   mob->level * mob->level);
		mob->max_hit = (int) (mob->max_hit * 0.9);
		mob->hit = mob->max_hit;
		mob->max_mana = 100 + dice(mob->level, 10);
		mob->mana = mob->max_mana;
		switch (number_range(1, 3))
		{
		case (1):
			mob->dam_type = 3;
			break;				/* slash */
		case (2):
			mob->dam_type = 7;
			break;				/* pound */
		case (3):
			mob->dam_type = 11;
			break;				/* pierce */
		}
		for (i = 0; i < 3; i++)
			mob->armor[i] = interpolate(mob->level, 100, -100);
		mob->armor[3] = interpolate(mob->level, 100, 0);
		mob->race = pMobIndex->race;
		mob->off_flags = pMobIndex->off_flags;
		mob->imm_flags = pMobIndex->imm_flags;
		mob->res_flags = pMobIndex->res_flags;
		mob->vuln_flags = pMobIndex->vuln_flags;
		mob->start_pos = pMobIndex->start_pos;
		mob->default_pos = pMobIndex->default_pos;
		mob->sex = pMobIndex->sex;
		mob->clan = pMobIndex->clan;
		mob->form = pMobIndex->form;
		mob->parts = pMobIndex->parts;
		mob->size = SIZE_MEDIUM;
		mob->material = "";

		for (i = 0; i < MAX_STATS; i++)
			mob->perm_stat[i] = 11 + mob->level / 4;
	}

	mob->position = mob->start_pos;


	/* link the mob to the world list */
	mob->next = char_list;
	char_list = mob;
	pMobIndex->count++;
	return mob;
}

/* duplicate a mobile exactly -- except inventory */
void
clone_mobile(CHAR_DATA * parent, CHAR_DATA * clone)
{
	int i;
	AFFECT_DATA *paf;

	if (parent == NULL || clone == NULL || !IS_NPC(parent))
		return;

	/* start fixing values */
	clone->name = str_dup(parent->name);
	clone->version = parent->version;
	clone->short_descr = str_dup(parent->short_descr);
	clone->long_descr = str_dup(parent->long_descr);
	clone->description = str_dup(parent->description);
	clone->group = parent->group;
	clone->clan = parent->clan;
	clone->sex = parent->sex;
	clone->charClass = parent->charClass;
	clone->race = parent->race;
	clone->level = parent->level;
	clone->trust = 0;
	clone->timer = parent->timer;
	clone->wait = parent->wait;
	clone->hit = parent->hit;
	clone->max_hit = parent->max_hit;
	clone->mana = parent->mana;
	clone->max_mana = parent->max_mana;
	clone->move = parent->move;
	clone->max_move = parent->max_move;
	clone->gold = parent->gold;
	clone->silver = parent->silver;
	clone->exp = parent->exp;
	clone->act = parent->act;
	clone->comm = parent->comm;
	clone->imm_flags = parent->imm_flags;
	clone->res_flags = parent->res_flags;
	clone->vuln_flags = parent->vuln_flags;
	clone->invis_level = parent->invis_level;
	clone->affected_by = parent->affected_by;
	clone->position = parent->position;
	clone->practice = parent->practice;
	clone->train = parent->train;
	clone->saving_throw = parent->saving_throw;
	clone->alignment = parent->alignment;
	clone->hitroll = parent->hitroll;
	clone->damroll = parent->damroll;
	clone->wimpy = parent->wimpy;
	clone->form = parent->form;
	clone->parts = parent->parts;
	clone->size = parent->size;
	clone->material = str_dup(parent->material);
	clone->off_flags = parent->off_flags;
	clone->dam_type = parent->dam_type;
	clone->start_pos = parent->start_pos;
	clone->default_pos = parent->default_pos;
	clone->spec_fun = parent->spec_fun;

	for (i = 0; i < 4; i++)
		clone->armor[i] = parent->armor[i];

	for (i = 0; i < MAX_STATS; i++)
	{
		clone->perm_stat[i] = parent->perm_stat[i];
		clone->mod_stat[i] = parent->mod_stat[i];
	}

	for (i = 0; i < 3; i++)
		clone->damage[i] = parent->damage[i];

	/* now add the affects */
	for (paf = parent->affected; paf != NULL; paf = paf->next)
		affect_to_char(clone, paf);

}




/*
 * Create an instance of an object.
 */
OBJ_DATA *
create_object(OBJ_INDEX_DATA * pObjIndex, int level)
{
	OBJ_DATA *obj;
	int i;

	if (pObjIndex == NULL)
	{
		bug("Create_object: NULL pObjIndex.", 0);
		do_crash(System, "woops");
	}

	obj = new_obj();

	obj->pIndexData = pObjIndex;
	obj->in_room = NULL;
	obj->enchanted = FALSE;

	if (pObjIndex->new_format)
		obj->level = pObjIndex->level;
	else
		obj->level = UMAX(0, level);
	obj->wear_loc = -1;

	obj->name = str_dup(pObjIndex->name);	/* OLC */
	obj->short_descr = str_dup(pObjIndex->short_descr);	/* OLC */
	obj->description = str_dup(pObjIndex->description);	/* OLC */
	obj->material = str_dup(pObjIndex->material);
	obj->item_type = pObjIndex->item_type;
	obj->extra_flags = pObjIndex->extra_flags;
	obj->wear_flags = pObjIndex->wear_flags;
	obj->value[0] = pObjIndex->value[0];
	obj->value[1] = pObjIndex->value[1];
	obj->value[2] = pObjIndex->value[2];
	obj->value[3] = pObjIndex->value[3];
	obj->value[4] = pObjIndex->value[4];
	obj->special[0] = pObjIndex->special[0];
	obj->special[1] = pObjIndex->special[1];
	obj->special[2] = pObjIndex->special[2];
	obj->special[3] = pObjIndex->special[3];
	obj->special[4] = pObjIndex->special[4];

	obj->extra[0] = obj->extra[1] = obj->extra[2] = obj->extra[3] = obj->extra[4] = 0;

	if(pObjIndex->extra[0])
	        obj->extra[0] = pObjIndex->extra[0];
	if(pObjIndex->extra[1])
		obj->extra[1] = pObjIndex->extra[1];
	if(pObjIndex->extra[2])
        	obj->extra[2] = pObjIndex->extra[2];
	if(pObjIndex->extra[3])
        	obj->extra[3] = pObjIndex->extra[3];
	if(pObjIndex->extra[4])
        	obj->extra[4] = pObjIndex->extra[4];

	obj->weight = pObjIndex->weight;

	if (level == -1 || pObjIndex->new_format)
		obj->cost = pObjIndex->cost;
	else
		obj->cost = number_fuzzy(10)
			* number_fuzzy(level) * number_fuzzy(level);

	/*
	 * Mess with object properties.
	 */
	switch (obj->item_type)
	{
	default:
		bug("Read_object: vnum %d bad type.", pObjIndex->vnum);
		break;

	case ITEM_LIGHT:
		if (obj->value[2] == 999)
			obj->value[2] = -1;
		break;

	case ITEM_SLOT_MACHINE:
		obj->value[1] = obj->value[0] * 10;
		if (obj->value[2] > 5)
			obj->value[2] = 5;
		else if (obj->value[2] < 3)
			obj->value[2] = 3;
		if(obj->value[3] > 0)
			obj->value[3] = 1;
		else
			obj->value[3] = 0;
	case ITEM_FURNITURE:
	case ITEM_TRASH:
	case ITEM_CONTAINER:
	case ITEM_DRINK_CON:
	case ITEM_KEY:
	case ITEM_FOOD:
	case ITEM_BOAT:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	case ITEM_FOUNTAIN:
	case ITEM_MAP:
	case ITEM_CLOTHING:
	case ITEM_PORTAL:
		if (!pObjIndex->new_format)
			obj->cost /= 5;
		break;

	case ITEM_TREASURE:
	case ITEM_WARP_STONE:
	case ITEM_ROOM_KEY:
	case ITEM_GEM:
	case ITEM_JEWELRY:
	case ITEM_THROWING:
	case ITEM_RECALL:
	case ITEM_ORB:
	case ITEM_SHEATH:
	case ITEM_CHARM:
	case ITEM_AMMO:
		break;

	case ITEM_JUKEBOX:
		for (i = 0; i < 5; i++)
			obj->value[i] = -1;
		break;

	case ITEM_SCROLL:
		if (level != -1 && !pObjIndex->new_format)
			obj->value[0] = number_fuzzy(obj->value[0]);
		break;

	case ITEM_WAND:
	case ITEM_STAFF:
		if (level != -1 && !pObjIndex->new_format)
		{
			obj->value[0] = number_fuzzy(obj->value[0]);
			obj->value[1] = number_fuzzy(obj->value[1]);
			obj->value[2] = obj->value[1];
		}
		if (!pObjIndex->new_format)
			obj->cost *= 2;
		break;

	case ITEM_WEAPON:
		if (level != -1 && !pObjIndex->new_format)
		{
			obj->value[1] = number_fuzzy(number_fuzzy(1 * level / 4 + 2));
			obj->value[2] = number_fuzzy(number_fuzzy(3 * level / 4 + 6));
		}
		break;

	case ITEM_ARMOR:
		if (level != -1 && !pObjIndex->new_format)
		{
			obj->value[0] = number_fuzzy(level / 5 + 3);
			obj->value[1] = number_fuzzy(level / 5 + 3);
			obj->value[2] = number_fuzzy(level / 5 + 3);
		}
		break;

	case ITEM_POTION:
	case ITEM_PILL:
		if (level != -1 && !pObjIndex->new_format)
			obj->value[0] = number_fuzzy(number_fuzzy(obj->value[0]));
		break;

	case ITEM_MONEY:
		if (!pObjIndex->new_format)
			obj->value[0] = obj->cost;
		break;
	}

	obj->next = object_list;
	object_list = obj;
	pObjIndex->count++;

	if(IS_SET(obj->wear_flags, ITEM_CHARGED))
		obj->special[4] = UMIN(100, 2 * obj->special[0]);

	return obj;
}

/* duplicate an object exactly -- except contents */
void
clone_object(OBJ_DATA * parent, OBJ_DATA * clone)
{
	int i;
	AFFECT_DATA *paf;
	EXTRA_DESCR_DATA *ed, *ed_new;

	if (parent == NULL || clone == NULL)
		return;

	/* start fixing the object */
	clone->name = str_dup(parent->name);
	clone->short_descr = str_dup(parent->short_descr);
	clone->description = str_dup(parent->description);
	clone->item_type = parent->item_type;
	clone->extra_flags = parent->extra_flags;
	clone->wear_flags = parent->wear_flags;
	clone->weight = parent->weight;
	clone->cost = parent->cost;
	clone->level = parent->level;
	clone->condition = parent->condition;
	clone->material = str_dup(parent->material);
	clone->timer = parent->timer;

	for (i = 0; i < 5; i++)
	{
		clone->value[i] = parent->value[i];
		clone->special[i] = parent->special[i];
		clone->extra[i] = parent->extra[i];
	}
	/* affects */
	clone->enchanted = parent->enchanted;

	for (paf = parent->affected; paf != NULL; paf = paf->next)
		affect_to_obj(clone, paf);

	/* extended desc */
	for (ed = parent->extra_descr; ed != NULL; ed = ed->next)
	{
		ed_new = new_extra_descr();
		ed_new->keyword = str_dup(ed->keyword);
		ed_new->description = str_dup(ed->description);
		ed_new->next = clone->extra_descr;
		clone->extra_descr = ed_new;
	}

}



/*
 * Clear a new character.
 */
void
clear_char(CHAR_DATA * ch)
{
	static CHAR_DATA ch_zero;
	int i;

	*ch = ch_zero;
	ch->name = &str_empty[0];
	ch->short_descr = &str_empty[0];
	ch->long_descr = &str_empty[0];
	ch->description = &str_empty[0];
	ch->shift_short = &str_empty[0];
	ch->shift_long = &str_empty[0];
	ch->shift_name = &str_empty[0];
	ch->site = &str_empty[0];
	ch->prompt = &str_empty[0];
	ch->rreply = &str_empty[0];	/* IMC v0.2 - Add this */
	ch->logon = current_time;
	ch->lines = PAGELEN;

	for (i = 0; i < 4; i++)
	{
		/* Removed, now dragons get -2 ac per level.
		if (IS_DRAGON(ch) && ch->remort > 0)
		{
			ch->armor[i] = DRAGON_ARMOR;
		}
		else
			*/
		ch->armor[i] = 100;
	}

	ch->position = POS_STANDING;
	ch->hit = 20;
	ch->max_hit = 20;
	ch->mana = 100;
	ch->max_mana = 100;
	ch->move = 100;
	ch->max_move = 100;
	ch->on = NULL;
	for (i = 0; i < MAX_STATS; i++)
	{
		ch->perm_stat[i] = 13;
		ch->mod_stat[i] = 0;
	}
	return;
}

/*
 * Get an extra description from a list.
 */
char *
get_extra_descr(const char *name, EXTRA_DESCR_DATA * ed)
{
	for (; ed != NULL; ed = ed->next)
	{
		if (is_name((char *) name, ed->keyword))
			return ed->description;
	}
	return NULL;
}



/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *
get_mob_index(int vnum)
{
	MOB_INDEX_DATA *pMobIndex;

	for (pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH];
		 pMobIndex != NULL;
		 pMobIndex = pMobIndex->next)
	{
		if (pMobIndex->vnum == vnum)
			return pMobIndex;
	}

	if (fBootDb)
	{
		bug("Get_mob_index: bad vnum %d.", vnum);
		exit(1);
	}

	return NULL;
}



/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *
get_obj_index(int vnum)
{
	OBJ_INDEX_DATA *pObjIndex;

	for (pObjIndex = obj_index_hash[vnum % MAX_KEY_HASH];
		 pObjIndex != NULL;
		 pObjIndex = pObjIndex->next)
	{
		if (pObjIndex->vnum == vnum)
			return pObjIndex;
	}

	if (fBootDb)
	{
		bug("Get_obj_index: bad vnum %d.", vnum);
		exit(1);
	}

	return NULL;
}



/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *
get_room_index(int vnum)
{
	ROOM_INDEX_DATA *pRoomIndex = NULL;

	for (pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH];
		 pRoomIndex != NULL;
		 pRoomIndex = pRoomIndex->next)
	{
                if(pRoomIndex->vnum == vnum)
                        return pRoomIndex;
	}

	if (fBootDb)
	{
		bug("Get_room_index: bad vnum %d.", vnum);
		exit(1);
	}

	return NULL;
}



/*
 * Read a letter from a file.
 */
char
fread_letter(FILE * fp)
{
	char c;

	do
	{
		c = getc(fp);
	}
	while (isspace(c));

	return c;
}



/*
 * Read a number from a file.
 */
int
fread_number(FILE * fp)
{
	int number;
	bool sign;
	char c;

	do
	{
		c = getc(fp);
	}
	while (isspace(c));

	number = 0;

	sign = FALSE;
	if (c == '+')
	{
		c = getc(fp);
	}
	else if (c == '-')
	{
		sign = TRUE;
		c = getc(fp);
	}

	if (!isdigit(c))
	{
		bug("Fread_number: bad format.", 0);
		exit(1);
	}

	while (isdigit(c))
	{
		number = number * 10 + c - '0';
		c = getc(fp);
	}

	if (sign)
		number = 0 - number;

	if (c == '|')
		number += fread_number(fp);
	else if (c != ' ')
		ungetc(c, fp);

	return number;
}

long
fread_flag(FILE * fp)
{
	int number;
	char c;
	bool negative = FALSE;

	do
	{
		c = getc(fp);
	}
	while (isspace(c));

	if (c == '-')
	{
		negative = TRUE;
		c = getc(fp);
	}

	number = 0;

	if (!isdigit(c))
	{
		while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
		{
			number += flag_convert(c);
			c = getc(fp);
		}
	}

	while (isdigit(c))
	{
		number = number * 10 + c - '0';
		c = getc(fp);
	}

	if (c == '|')
		number += fread_flag(fp);

	else if (c != ' ')
		ungetc(c, fp);

	if (negative)
		return -1 * number;

	return number;
}

long
flag_convert(char letter)
{
	long bitsum = 0;
	char i;

	if ('A' <= letter && letter <= 'Z')
	{
		bitsum = 1;
		for (i = letter; i > 'A'; i--)
			bitsum *= 2;
	}
	else if ('a' <= letter && letter <= 'z')
	{
		bitsum = 67108864;		/* 2^26 */
		for (i = letter; i > 'a'; i--)
			bitsum *= 2;
	}

	return bitsum;
}



char *
fread_line(FILE * fp)
{
	char *plast;
	char c;

	plast = top_string + sizeof(char *);

	if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH])
	{
		bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
		exit(1);
	}

	/*
	 * Skip blanks.
	 * Read first char.
	 */
	do
	{
		c = getc(fp);
	}
	while (isspace(c));

	if ((*plast++ = c) == '~')
		return &str_empty[0];

	for (;;)
	{
		/*
		 * Back off the char type lookup,
		 *   it was too dirty for portability.
		 *   -- Furey
		 */

		switch (*plast = getc(fp))
		{
		default:
			plast++;
			break;

		case EOF:
			/* temp fix */
			return NULL;
			/* exit( 1 ); */
			break;

		case '\r':
			break;

		case '\n':
			plast++;
			*plast++ = '\r';
			plast++;
			{
				union
				{
					char *pc;
					char rgc[sizeof(char *)];
				}
				u1;
				unsigned int ic;
				int iHash;
				char *pHash;
				char *pHashPrev;
				char *pString;

				plast[-1] = '\0';
				iHash = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
				for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev)
				{
					for (ic = 0; ic < sizeof(char *); ic++)

						u1.rgc[ic] = pHash[ic];
					pHashPrev = u1.pc;
					pHash += sizeof(char *);

					if (top_string[sizeof(char *)] == pHash[0]
					 && !strcmp(top_string + sizeof(char *) + 1, pHash + 1))
						 return pHash;
				}

				if (fBootDb)
				{
					pString = top_string;
					top_string = plast;
					u1.pc = string_hash[iHash];
					for (ic = 0; ic < sizeof(char *); ic++)

						pString[ic] = u1.rgc[ic];
					string_hash[iHash] = pString;

					nAllocString += 1;
					sAllocString += top_string - pString;
					return pString + sizeof(char *);
				}
				else
				{
					return str_dup(top_string + sizeof(char *));
				}
			}
		}
	}
}






/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with hash pointer to prev string,
 *   hash code is simply the string length.
 *   this function takes 40% to 50% of boot-up time.
 */
char *
fread_string(FILE * fp)
{
	char *plast;
	char c;

	plast = top_string + sizeof(char *);

	if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH])
	{
		bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
		exit(1);
	}

	/*
	 * Skip blanks.
	 * Read first char.
	 */
	do
	{
		c = getc(fp);
	}
	while (isspace(c));

	if ((*plast++ = c) == '~')
		return &str_empty[0];

	for (;;)
	{
		/*
		 * Back off the char type lookup,
		 *   it was too dirty for portability.
		 *   -- Furey
		 */

		switch (*plast = getc(fp))
		{
		default:
			plast++;
			break;

		case EOF:
			/* temp fix */
			bug("Fread_string: EOF", 0);
			return NULL;
			/* exit( 1 ); */
			break;

		case '\n':
			plast++;
			*plast++ = '\r';
			break;

		case '\r':
			break;

		case '~':
			plast++;
			{
				union
				{
					char *pc;
					char rgc[sizeof(char *)];
				}
				u1;
				unsigned int ic;
				int iHash;
				char *pHash;
				char *pHashPrev;
				char *pString;

				plast[-1] = '\0';
				iHash = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
				for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev)
				{
					for (ic = 0; ic < sizeof(char *); ic++)

						u1.rgc[ic] = pHash[ic];
					pHashPrev = u1.pc;
					pHash += sizeof(char *);

					if (top_string[sizeof(char *)] == pHash[0]
					 && !strcmp(top_string + sizeof(char *) + 1, pHash + 1))
						 return pHash;
				}

				if (fBootDb)
				{
					pString = top_string;
					top_string = plast;
					u1.pc = string_hash[iHash];
					for (ic = 0; ic < sizeof(char *); ic++)

						pString[ic] = u1.rgc[ic];
					string_hash[iHash] = pString;

					nAllocString += 1;
					sAllocString += top_string - pString;
					return pString + sizeof(char *);
				}
				else
				{
					return str_dup(top_string + sizeof(char *));
				}
			}
		}
	}
}

char *
fread_string_eol(FILE * fp)
{
	static bool char_special[256 - EOF];
	char *plast;
	char c;

	if (char_special[EOF - EOF] != TRUE)
	{
		char_special[EOF - EOF] = TRUE;
		char_special['\n' - EOF] = TRUE;
		char_special['\r' - EOF] = TRUE;
	}

	plast = top_string + sizeof(char *);

	if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH])
	{
		bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
		exit(1);
	}

	/*
	 * Skip blanks.
	 * Read first char.
	 */
	do
	{
		c = getc(fp);
	}
	while (isspace(c));

	if ((*plast++ = c) == '\n')
		return &str_empty[0];

	for (;;)
	{
		if (!char_special[(*plast++ = getc(fp)) - EOF])
			continue;

		switch (plast[-1])
		{
		default:
			break;

		case EOF:
			bug("Fread_string_eol  EOF", 0);
			exit(1);
			break;

		case '\n':
		case '\r':
			{
				union
				{
					char *pc;
					char rgc[sizeof(char *)];
				}
				u1;
				unsigned int ic;
				int iHash;
				char *pHash;
				char *pHashPrev;
				char *pString;

				plast[-1] = '\0';
				iHash = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
				for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev)
				{
					for (ic = 0; ic < sizeof(char *); ic++)

						u1.rgc[ic] = pHash[ic];
					pHashPrev = u1.pc;
					pHash += sizeof(char *);

					if (top_string[sizeof(char *)] == pHash[0]
					 && !strcmp(top_string + sizeof(char *) + 1, pHash + 1))
						 return pHash;
				}

				if (fBootDb)
				{
					pString = top_string;
					top_string = plast;
					u1.pc = string_hash[iHash];
					for (ic = 0; ic < sizeof(char *); ic++)

						pString[ic] = u1.rgc[ic];
					string_hash[iHash] = pString;

					nAllocString += 1;
					sAllocString += top_string - pString;
					return pString + sizeof(char *);
				}
				else
				{
					return
						str_dup(top_string + sizeof(char *));
				}
			}
		}
	}
}



/*
 * Read to end of line (for comments).
 */
void
fread_to_eol(FILE * fp)
{
	char c;

	do
	{
		c = getc(fp);
	}
	while (c != '\n' && c != '\r');

	do
	{
		c = getc(fp);
	}
	while (c == '\n' || c == '\r');

	ungetc(c, fp);
	return;
}



/*
 * Read one word (into static buffer).
 */
char *
fread_word(FILE * fp)
{
	static char word[MAX_INPUT_LENGTH];
	char *pword;
	char cEnd;

	do
	{
		cEnd = getc(fp);
	}
	while (isspace(cEnd));

	if (cEnd == '\'' || cEnd == '"')
	{
		pword = word;
	}
	else
	{
		word[0] = cEnd;
		pword = word + 1;
		cEnd = ' ';
	}

	for (; pword < word + MAX_INPUT_LENGTH; pword++)
	{
		*pword = getc(fp);
		if (cEnd == ' ' ? isspace(*pword) : *pword == cEnd)
		{
			if (cEnd == ' ')
				ungetc(*pword, fp);
			*pword = '\0';
			return word;
		}
	}

	bug("Fread_word: word too long.", 0);
	exit(1);
	return NULL;
}

/*
 * Allocate some ordinary memory,
 *   with the expectation of freeing it someday.
 */
void *
alloc_mem(int sMem)
{
	void *pMem;
	int *magic;
	int iList;

	sMem += sizeof(*magic);

	for (iList = 0; iList < MAX_MEM_LIST; iList++)
	{
		if (sMem <= rgSizeList[iList])
			break;
	}

	if (iList == MAX_MEM_LIST)
	{
		bug("Alloc_mem: size %d too large.", sMem);
		exit(1);
	}

	if (rgFreeList[iList] == NULL)
	{
		pMem = alloc_perm(rgSizeList[iList]);
	}
	else
	{
		pMem = rgFreeList[iList];
		rgFreeList[iList] = *((void **) rgFreeList[iList]);
	}

	magic = (int *) pMem;
	*magic = MAGIC_NUM;
	pMem += sizeof(*magic);

	return pMem;
}



/*
 * Free some memory.
 * Recycle it back onto the free list for blocks of that size.
 */
void
free_mem(void *pMem, int sMem)
{
	int iList;
	int *magic;

	pMem -= sizeof(*magic);
	magic = (int *) pMem;

	if (*magic != MAGIC_NUM)
	{
		bug("Attempt to recyle invalid memory of size %d.", sMem);
		bug((char *) pMem + sizeof(*magic), 0);
		abort();
		return;
	}

	*magic = 0;
	sMem += sizeof(*magic);

	for (iList = 0; iList < MAX_MEM_LIST; iList++)
	{
		if (sMem <= rgSizeList[iList])
			break;
	}

	if (iList == MAX_MEM_LIST)
	{
		bug("Free_mem: size %d too large.", sMem);
		exit(1);
	}

	*((void **) pMem) = rgFreeList[iList];
	rgFreeList[iList] = pMem;

	return;
}


/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void *
alloc_perm(int sMem)
{
	static char *pMemPerm;
	static int iMemPerm;
	void *pMem;

	while (sMem % sizeof(long) != 0)
		 sMem++;

	if (sMem > MAX_PERM_BLOCK)
	{
		bug("Alloc_perm: %d too large.", sMem);
		exit(1);
	}

	if (pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK)
	{
		iMemPerm = 0;
		if ((pMemPerm = (char *) calloc(1, MAX_PERM_BLOCK)) == NULL)
		{
			perror("Alloc_perm");
			exit(1);
		}
	}

	pMem = pMemPerm + iMemPerm;
	iMemPerm += sMem;
	nAllocPerm += 1;
	sAllocPerm += sMem;
	return pMem;
}



/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *
str_dup(const char *str)
{
	char *str_new;

	if (str == NULL)
	{
		return NULL;
	}

	if (str[0] == '\0')
		return &str_empty[0];

	if (str >= string_space && str < top_string)
		return (char *) str;

	str_new = (char *) alloc_mem(strlen(str) + 1);
	strcpy(str_new, str);
	return str_new;
}



/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void
free_string(char *pstr)
{
	if (pstr == NULL
		|| pstr == &str_empty[0]
		|| (pstr >= string_space && pstr < top_string))
		return;

	free_mem(pstr, strlen(pstr) + 1);
	return;
}



void
do_areas(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	AREA_DATA *pArea;
	bool bBoth;
	int cont;
	int col;

	bBoth = FALSE;
	if (argument == NULL || argument[0] == '\0')
	{
		bBoth = TRUE;
	}
	else if (!str_prefix(argument, "Terra"))
	{
		cont = 0;
	}
	else if (!str_prefix(argument, "Dominia"))
	{
		cont = 1;
	}


	if (cont == 0 || bBoth == TRUE)
	{
		Cprintf(ch, "----==== Areas on Terra ====----\n\r");
		col = 0;
		for (pArea = area_first; pArea != NULL; pArea = pArea->next)
		{
			if (pArea->continent == 0 &&
			   (pArea->security == 9 || IS_IMMORTAL(ch)))
			{
				if (col == 0)
				{
					sprintf(buf, "%-39s", pArea->credits);
					col = 1;
				}
				else
				{
					sprintf(buf, " %-39s\n\r", pArea->credits);
					col = 0;
				}
				send_to_char_bw(buf, ch);
			}
		}
	}

	if (cont == 1 || bBoth == TRUE)
	{
		Cprintf(ch, "\n\r----==== Areas on Dominia ====----\n\r");
		col = 0;
		for (pArea = area_first; pArea != NULL; pArea = pArea->next)
		{
			if (pArea->continent == 1 &&
			   (pArea->security == 9 || IS_IMMORTAL(ch)))
			{
				if (col == 0)
				{
					sprintf(buf, "%-39s", pArea->credits);
					col = 1;
				}
				else
				{
					sprintf(buf, " %-39s\n\r", pArea->credits);
					col = 0;
				}
				send_to_char_bw(buf, ch);
			}
		}
	}

	return;
}



void
do_memory(CHAR_DATA * ch, char *argument)
{
	Cprintf(ch, "Affects %5d\n\r", top_affect);
	Cprintf(ch, "Areas   %5d\n\r", top_area);
	Cprintf(ch, "ExDes   %5d\n\r", top_ed);
	Cprintf(ch, "Exits   %5d\n\r", top_exit);
	Cprintf(ch, "Helps   %5d\n\r", top_help);
	Cprintf(ch, "Socials %5d\n\r", social_count);
	Cprintf(ch, "Mobs    %5d(%d new format)\n\r", top_mob_index, newmobs);
	Cprintf(ch, "(in use)%5d\n\r", mobile_count);
	Cprintf(ch, "Objs    %5d(%d new format)\n\r", top_obj_index, newobjs);
	Cprintf(ch, "Resets  %5d\n\r", top_reset);
	Cprintf(ch, "Rooms   %5d\n\r", top_room);
	Cprintf(ch, "Shops   %5d\n\r", top_shop);

	Cprintf(ch, "Strings %5d strings of %7d bytes (max %d).\n\r",
			nAllocString, sAllocString, MAX_STRING);

	Cprintf(ch, "Perms   %5d blocks  of %7d bytes.\n\r",
			nAllocPerm, sAllocPerm);

	return;
}

void
do_dump(CHAR_DATA * ch, char *argument)
{
	int count, count2, num_pcs, aff_count;
	CHAR_DATA *fch;
	MOB_INDEX_DATA *pMobIndex;
	PC_DATA *pc;
	OBJ_DATA *obj;
	OBJ_INDEX_DATA *pObjIndex;
	ROOM_INDEX_DATA *room;
	EXIT_DATA *exit;
	DESCRIPTOR_DATA *d;
	AFFECT_DATA *af;
	FILE *fp;
	int vnum, nMatch = 0;

	/* open file */
	fclose(fpReserve);
	fp = fopen("mem.dmp", "w");

	/* report use of data structures */

	num_pcs = 0;
	aff_count = 0;

	/* mobile prototypes */
	fprintf(fp, "MobProt   %4d (%8d bytes)\n",
			top_mob_index, top_mob_index * (sizeof(*pMobIndex)));

	/* mobs */
	count = 0;
	count2 = 0;
	for (fch = char_list; fch != NULL; fch = fch->next)
	{
		count++;
		if (fch->pcdata != NULL)
			num_pcs++;
		for (af = fch->affected; af != NULL; af = af->next)
			aff_count++;
	}
	for (fch = char_free; fch != NULL; fch = fch->next)
		count2++;

	fprintf(fp, "Mobs   %4d (%8d bytes), %2d free (%d bytes)\n",
			count, count * (sizeof(*fch)), count2, count2 * (sizeof(*fch)));

	/* pcdata */
	count = 0;
	for (pc = pcdata_free; pc != NULL; pc = pc->next)
		count++;

	fprintf(fp, "Pcdata   %4d (%8d bytes), %2d free (%d bytes)\n",
			num_pcs, num_pcs * (sizeof(*pc)), count, count * (sizeof(*pc)));

	/* descriptors */
	count = 0;
	count2 = 0;
	for (d = descriptor_list; d != NULL; d = d->next)
		count++;
	for (d = descriptor_free; d != NULL; d = d->next)
		count2++;

	fprintf(fp, "Descs   %4d (%8d bytes), %2d free (%d bytes)\n",
			count, count * (sizeof(*d)), count2, count2 * (sizeof(*d)));

	/* object prototypes */
	for (vnum = 0; nMatch < top_obj_index; vnum++)
		if ((pObjIndex = get_obj_index(vnum)) != NULL)
		{
			for (af = pObjIndex->affected; af != NULL; af = af->next)
				aff_count++;
			nMatch++;
		}

	fprintf(fp, "ObjProt   %4d (%8d bytes)\n",
			top_obj_index, top_obj_index * (sizeof(*pObjIndex)));


	/* objects */
	count = 0;
	count2 = 0;
	for (obj = object_list; obj != NULL; obj = obj->next)
	{
		count++;
		for (af = obj->affected; af != NULL; af = af->next)
			aff_count++;
	}
	for (obj = obj_free; obj != NULL; obj = obj->next)
		count2++;

	fprintf(fp, "Objs   %4d (%8d bytes), %2d free (%d bytes)\n",
			count, count * (sizeof(*obj)), count2, count2 * (sizeof(*obj)));

	/* affects */
	count = 0;
	for (af = affect_free; af != NULL; af = af->next)
		count++;

	fprintf(fp, "Affects   %4d (%8d bytes), %2d free (%d bytes)\n",
		aff_count, aff_count * (sizeof(*af)), count, count * (sizeof(*af)));

	/* rooms */
	fprintf(fp, "Rooms   %4d (%8d bytes)\n",
			top_room, top_room * (sizeof(*room)));

	/* exits */
	fprintf(fp, "Exits   %4d (%8d bytes)\n",
			top_exit, top_exit * (sizeof(*exit)));

	fclose(fp);

	/* start printing out mobile data */
	fp = fopen("mob.dmp", "w");

	fprintf(fp, "\nMobile Analysis\n");
	fprintf(fp, "---------------\n");
	nMatch = 0;
	for (vnum = 0; nMatch < top_mob_index; vnum++)
		if ((pMobIndex = get_mob_index(vnum)) != NULL)
		{
			nMatch++;
			fprintf(fp, "#%-4d %3d active %3d killed     %s\n",
					pMobIndex->vnum, pMobIndex->count,
					pMobIndex->killed, pMobIndex->short_descr);
		}
	fclose(fp);

	/* start printing out object data */
	fp = fopen("obj.dmp", "w");

	fprintf(fp, "\nObject Analysis\n");
	fprintf(fp, "---------------\n");
	nMatch = 0;
	for (vnum = 0; nMatch < top_obj_index; vnum++)
		if ((pObjIndex = get_obj_index(vnum)) != NULL)
		{
			nMatch++;
			fprintf(fp, "#%-4d %3d active %3d reset      %s\n",
					pObjIndex->vnum, pObjIndex->count,
					pObjIndex->reset_num, pObjIndex->short_descr);
		}

	/* close file */
	fclose(fp);
	fpReserve = fopen(NULL_FILE, "r");
}



/*
 * Stick a little fuzz on a number.
 */
int
number_fuzzy(int number)
{
	switch (number_bits(2))
	{
	case 0:
		number -= 1;
		break;
	case 3:
		number += 1;
		break;
	}

	return UMAX(1, number);
}



/*
 * Generate a random number.
 */
int
number_range(int from, int to)
{
	int power;
	int number;

	if (from == 0 && to == 0)
		return 0;

	/* to is set to one more than the difference between from and to */
	if ((to = to - from + 1) <= 1)
		return from;

	/* power is set to the smallest power of 2 greater than to */
	for (power = 2; power < to; power <<= 1)
		;

	while ((number = number_mm() & (power - 1)) >= to)
		;

	return from + number;
}



/*
 * Generate a percentile roll.
 */
int
number_percent(void)
{
	int percent;

	while ((percent = number_mm() & (128 - 1)) > 99)
		;

	return 1 + percent;
}



/*
 * Generate a random door.
 */
int
number_door(void)
{
	int door;

	while ((door = number_mm() & (8 - 1)) > 5)
		;

	return door;
}

int
number_bits(int width)
{
	return number_mm() & ((1 << width) - 1);
}




/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */

/* I noticed streaking with this random number generator, so I switched
   back to the system srandom call.  If this doesn't work for you,
   define OLD_RAND to use the old system -- Alander */

#if defined (OLD_RAND)
static int rgiState[2 + 55];

#endif

void
init_mm()
{
#if defined (OLD_RAND)
	int *piState;
	int iState;

	piState = &rgiState[2];

	piState[-2] = 55 - 55;
	piState[-1] = 55 - 24;

	piState[0] = ((int) current_time) & ((1 << 30) - 1);
	piState[1] = 1;
	for (iState = 2; iState < 55; iState++)
	{
		piState[iState] = (piState[iState - 1] + piState[iState - 2])
			& ((1 << 30) - 1);
	}
#else
	srandom(time(NULL) ^ getpid());
#endif
	return;
}



long
number_mm(void)
{
#if defined (OLD_RAND)
	int *piState;
	int iState1;
	int iState2;
	int iRand;

	piState = &rgiState[2];
	iState1 = piState[-2];
	iState2 = piState[-1];
	iRand = (piState[iState1] + piState[iState2])
		& ((1 << 30) - 1);
	piState[iState1] = iRand;
	if (++iState1 == 55)
		iState1 = 0;
	if (++iState2 == 55)
		iState2 = 0;
	piState[-2] = iState1;
	piState[-1] = iState2;
	return iRand >> 6;
#else
	return random() >> 6;
#endif
}


/*
 * Roll some dice.
 */
int
dice(int number, int size)
{
	int idice;
	int sum;

	switch (size)
	{
	case 0:
		return 0;
	case 1:
		return number;
	}

	for (idice = 0, sum = 0; idice < number; idice++)
		sum += number_range(1, size);

	return sum;
}



/*
 * Simple linear interpolation.
 */
int
interpolate(int level, int value_00, int value_32)
{
	return value_00 + level * (value_32 - value_00) / 38;
}



/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void
smash_tilde(char *str)
{
	for (; *str != '\0'; str++)
	{
		if (*str == '~')
			*str = '-';
	}

	return;
}





/*
 * Append a string to a file.
 */
void
append_file(CHAR_DATA * ch, char *file, char *str)
{
	FILE *fp;

	if (IS_NPC(ch) || str[0] == '\0')
		return;

	fclose(fpReserve);
	if ((fp = fopen(file, "a")) == NULL)
	{
		perror(file);
		Cprintf(ch, "Could not open the file!\n\r");
	}
	else
	{
		fprintf(fp, "[%5d] %s: %s\n",
				ch->in_room ? ch->in_room->vnum : 0, ch->name, str);
		fclose(fp);
	}

	fpReserve = fopen(NULL_FILE, "r");
	return;
}



/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void
tail_chain(void)
{
	return;
}
