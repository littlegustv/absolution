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
#include "flags.h"
#include "utils.h"



int
flag_lookup (const char *name, const struct flag_type *flag_table) {
	int flag;

	for (flag = 0; flag_table[flag].name != NULL && flag_table[flag].name[0] != '\0'; flag++) {
		if ( LOWER (name[0]) == LOWER (flag_table[flag].name[0]) && !str_prefix(name, flag_table[flag].name) ) {
			return flag_table[flag].bit;
		}
	}

	return NO_FLAG;
}

/* various flag tables */
const struct flag_type act_flags[] = {
	{"npc",           A,  FALSE},
	{"sentinel",      B,  TRUE},
	{"scavenger",     C,  TRUE},
	{"legendary",     E,  TRUE},
	{"aggressive",    F,  TRUE},
	{"stay_area",     G,  TRUE},
	{"wimpy",         H,  TRUE},
	{"pet",           I,  TRUE},
	{"train",         J,  TRUE},
	{"practice",      K,  TRUE},
	{"dealer",        L,  TRUE},
	{"undead",        O,  TRUE},
	{"cleric",        Q,  TRUE},
	{"mage",          R,  TRUE},
	{"thief",         S,  TRUE},
	{"warrior",       T,  TRUE},
	{"noalign",       U,  TRUE},
	{"nopurge",       V,  TRUE},
	{"outdoors",      W,  TRUE},
	{"indoors",       Y,  TRUE},
	{"wizi",          Z,  TRUE},
	{"healer",        aa, TRUE},
	{"gain",          bb, TRUE},
	{"update_always", cc, TRUE},
	{"changer",       dd, TRUE},
	{"banker",        ee, TRUE},
	{NULL,            0,  FALSE}
};

const struct flag_type plr_flags[] = {
	{"npc",        A,  FALSE},
	{"autoassist", C,  FALSE},
	{"autoexit",   D,  FALSE},
	{"autoloot",   E,  FALSE},
	{"autosac",    F,  FALSE},
	{"autogold",   G,  FALSE},
	{"autosplit",  H,  FALSE},
	{"holylight",  N,  FALSE},
	{"can_loot",   P,  FALSE},
	{"nosummon",   Q,  FALSE},
	{"nofollow",   R,  FALSE},
	{"colour",     T,  FALSE},
	{"permit",     U,  TRUE},
	{"log",        W,  FALSE},
	{"deny",       X,  FALSE},
	{"freeze",     Y,  FALSE},
	{"thief",      Z,  FALSE},
	{"killer",     aa, FALSE},
	{NULL,         0,  0}
};

const struct flag_type liquid_flags[] = {
	{"water",            0,  TRUE},
	{"beer",             1,  TRUE},
	{"wine",             2,  TRUE},
	{"ale",              3,  TRUE},
	{"dark-ale",         4,  TRUE},
	{"whisky",           5,  TRUE},
	{"lemonade",         6,  TRUE},
	{"firebreather",     7,  TRUE},
	{"local-specialty",  8,  TRUE},
	{"slime-mold-juice", 9,  TRUE},
	{"milk",             10, TRUE},
	{"tea",              11, TRUE},
	{"coffee",           12, TRUE},
	{"blood",            13, TRUE},
	{"salt-water",       14, TRUE},
	{"cola",             15, TRUE},
	{"",                 0,  0}
};


const struct flag_type affect_flags[] = {
	{"blind",         A,  TRUE},
	{"invisible",     B,  TRUE},
	{"minimation",    C,  TRUE},
	{"detect_invis",  D,  TRUE},
	{"detect_magic",  E,  TRUE},
	{"detect_hidden", F,  TRUE},
	{"scramble",      G,  TRUE},
	{"sanctuary",     H,  TRUE},
	{"faerie_fire",   I,  TRUE},
	{"infrared",      J,  TRUE},
	{"curse",         K,  TRUE},
	{"grandeur",      L,  TRUE},
	{"poison",        M,  TRUE},
	{"protect_evil",  N,  TRUE},
	{"protect_good",  O,  TRUE},
	{"sneak",         P,  TRUE},
	{"hide",          Q,  TRUE},
	{"sleep",         R,  TRUE},
	{"charm",         S,  TRUE},
	{"flying",        T,  TRUE},
	{"pass_door",     U,  TRUE},
	{"haste",         V,  TRUE},
	{"calm",          W,  TRUE},
	{"plague",        X,  TRUE},
	{"weaken",        Y,  TRUE},
	{"dark_vision",   Z,  TRUE},
	{"berserk",       aa, TRUE},
	{"taunt",         bb, TRUE},
	{"darklight",     cc, TRUE},
	{"slow",          dd, TRUE},
	{"daylight",      ee, TRUE},
	{NULL,            0,  0}
};

const struct flag_type off_flags[] = {
	{"area_attack", A, TRUE},
	{"backstab", B, TRUE},
	{"bash", C, TRUE},
	{"berserk", D, TRUE},
	{"disarm", E, TRUE},
	{"dodge", F, TRUE},
	{"fade", G, TRUE},
	{"fast", H, TRUE},
	{"kick", I, TRUE},
	{"dirt_kick", J, TRUE},
	{"parry", K, TRUE},
	{"rescue", L, TRUE},
	{"tail", M, TRUE},
	{"trip", N, TRUE},
	{"crush", O, TRUE},
	{"assist_all", P, TRUE},
	{"assist_align", Q, TRUE},
	{"assist_race", R, TRUE},
	{"assist_players", S, TRUE},
	{"assist_guard", T, TRUE},
	{"assist_vnum", U, TRUE},
	{NULL, 0, 0}
};

const struct flag_type damtype_flags[] =
{
	{"bash", DAM_BASH, TRUE},
	{"pierce", DAM_PIERCE, TRUE},
	{"slash", DAM_SLASH, TRUE},
	{"fire", DAM_FIRE, TRUE},
	{"cold", DAM_COLD, TRUE},
	{"light", DAM_LIGHT, TRUE},
	{"lightning", DAM_LIGHTNING, TRUE},
	{"acid", DAM_ACID, TRUE},
	{"poison", DAM_POISON, TRUE},
	{"negative", DAM_NEGATIVE, TRUE},
	{"holy", DAM_HOLY, TRUE},
	{"energy", DAM_ENERGY, TRUE},
	{"disease", DAM_DISEASE, TRUE},
	{"drowning", DAM_DROWNING, TRUE},
	{"charm", DAM_CHARM, TRUE},
	{"sound", DAM_SOUND, TRUE},
	{"none", DAM_NONE, TRUE},
	{NULL, 0, 0},
};

const struct flag_type imm_flags[] =
{
   {"summon", A, TRUE},
   {"charm", B, TRUE},
   {"magic", C, TRUE},
   {"weapon", D, TRUE},
   {"bash", E, TRUE},
   {"pierce", F, TRUE},
   {"slash", G, TRUE},
   {"fire", H, TRUE},
   {"cold", I, TRUE},
   {"light", J, TRUE},
   {"lightning", S, TRUE},
   {"acid", K, TRUE},
   {"poison", L, TRUE},
   {"negative", M, TRUE},
   {"holy", N, TRUE},
   {"energy", O, TRUE},
   {"mental", P, TRUE},
   {"disease", Q, TRUE},
   {"drowning", R, TRUE},
   {"sound", T, TRUE},
   {"wood", X, TRUE},
   {"silver", Y, TRUE},
   {"iron", Z, TRUE},
   {"rain", aa, TRUE},
   {"vorpal", bb, TRUE},
   {NULL, 0, 0}
};

const struct flag_type form_flags[] =
{
   {"edible", FORM_EDIBLE, TRUE},
   {"poison", FORM_POISON, TRUE},
   {"magical", FORM_MAGICAL, TRUE},
   {"instant_decay", FORM_INSTANT_DECAY, TRUE},
   {"other", FORM_OTHER, TRUE},
   {"animal", FORM_ANIMAL, TRUE},
   {"sentient", FORM_SENTIENT, TRUE},
   {"undead", FORM_UNDEAD, TRUE},
   {"construct", FORM_CONSTRUCT, TRUE},
   {"mist", FORM_MIST, TRUE},
   {"intangible", FORM_INTANGIBLE, TRUE},
   {"biped", FORM_BIPED, TRUE},
   {"centaur", FORM_CENTAUR, TRUE},
   {"insect", FORM_INSECT, TRUE},
   {"spider", FORM_SPIDER, TRUE},
   {"crustacean", FORM_CRUSTACEAN, TRUE},
   {"worm", FORM_WORM, TRUE},
   {"blob", FORM_BLOB, TRUE},
   {"mammal", FORM_MAMMAL, TRUE},
   {"bird", FORM_BIRD, TRUE},
   {"reptile", FORM_REPTILE, TRUE},
   {"snake", FORM_SNAKE, TRUE},
   {"dragon", FORM_DRAGON, TRUE},
   {"amphibian", FORM_AMPHIBIAN, TRUE},
   {"fish", FORM_FISH, TRUE},
   {"cold_blood", FORM_COLD_BLOOD, TRUE},
   {NULL, 0, 0}
};

const struct flag_type part_flags[] =
{
   {"head", PART_HEAD, TRUE},
   {"arms", PART_ARMS, TRUE},
   {"legs", PART_LEGS, TRUE},
   {"heart", PART_HEART, TRUE},
   {"brains", PART_BRAINS, TRUE},
   {"guts", PART_GUTS, TRUE},
   {"hands", PART_HANDS, TRUE},
   {"feet", PART_FEET, TRUE},
   {"fingers", PART_FINGERS, TRUE},
   {"ear", PART_EAR, TRUE},
   {"eye", PART_EYE, TRUE},
   {"long_tongue", PART_LONG_TONGUE, TRUE},
   {"eyestalks", PART_EYESTALKS, TRUE},
   {"tentacles", PART_TENTACLES, TRUE},
   {"fins", PART_FINS, TRUE},
   {"wings", PART_WINGS, TRUE},
   {"tail", PART_TAIL, TRUE},
   {"claws", PART_CLAWS, TRUE},
   {"fangs", PART_FANGS, TRUE},
   {"horns", PART_HORNS, TRUE},
   {"scales", PART_SCALES, TRUE},
   {"tusks", PART_TUSKS, TRUE},
   {NULL, 0, 0}
};

const struct flag_type comm_flags[] =
{
   {"quiet", COMM_QUIET, TRUE},
   {"deaf", COMM_DEAF, TRUE},
   {"nowiz", COMM_NOWIZ, TRUE},
   {"no_newbie_tips", COMM_NONEWBIE, TRUE},
   {"nogossip", COMM_NOGOSSIP, TRUE},
   {"noquestion", COMM_NOQUESTION, TRUE},
   {"nomusic", COMM_NOMUSIC, TRUE},
   {"noclan", COMM_NOCLAN, TRUE},
   {"noauction", COMM_NOAUCTION, TRUE},
   {"shoutsoff", COMM_SHOUTSOFF, TRUE},
   {"lead", COMM_LEAD, TRUE},
   {"compact", COMM_COMPACT, TRUE},
   {"brief", COMM_BRIEF, TRUE},
   {"prompt", COMM_PROMPT, TRUE},
   {"combine", COMM_COMBINE, TRUE},
   {"telnet_ga", COMM_TELNET_GA, TRUE},
   {"show_affects", COMM_SHOW_AFFECTS, TRUE},
   {"nograts", COMM_NOGRATS, TRUE},
   {"noemote", COMM_NOEMOTE, FALSE},
   {"noshout", COMM_NOSHOUT, FALSE},
   {"notell", COMM_NOTELL, FALSE},
   {"nochannels", COMM_NOCHANNELS, FALSE},
   {"snoop_proof", COMM_SNOOP_PROOF, FALSE},
   {"afk", COMM_AFK, TRUE},
   {"lag", COMM_LAG, TRUE},
   {"no_beep", COMM_NOBEEP, TRUE},
   {"no_note", COMM_NONOTE, TRUE},
   {"no_cgos", COMM_NOCGOSS, TRUE},
   {"no_ooc", COMM_NOOOC, TRUE},
   {"note_write", COMM_NOTE_WRITE, FALSE},
   {NULL, 0, 0}
};

const struct flag_type area_flags[] =
{
   {"none", AREA_NONE, FALSE},
   {"changed", AREA_CHANGED, TRUE},
   {"added", AREA_ADDED, TRUE},
   {"loading", AREA_LOADING, FALSE},
   {NULL, 0, 0}
};

const struct flag_type sex_flags[] =
{
   {"male", SEX_MALE, TRUE},
   {"female", SEX_FEMALE, TRUE},
   {"neutral", SEX_NEUTRAL, TRUE},
   {"random", 3, TRUE},                /* ROM */
   {"none", SEX_NEUTRAL, TRUE},
   {NULL, 0, 0}
};

const struct flag_type exit_flags[] =
{
   {"door", EX_ISDOOR, TRUE},
   {"closed", EX_CLOSED, TRUE},
   {"locked", EX_LOCKED, TRUE},
   {"pickproof", EX_PICKPROOF, TRUE},
   {"nopass", EX_NOPASS, TRUE},
   {"easy", EX_EASY, TRUE},
   {"hard", EX_HARD, TRUE},
   {"infuriating", EX_INFURIATING, TRUE},
   {"noclose", EX_NOCLOSE, TRUE},
   {"nolock", EX_NOLOCK, TRUE},
   {NULL, 0, 0}
};

const struct flag_type door_resets[] =
{
   {"open and unlocked", 0, TRUE},
   {"closed and unlocked", 1, TRUE},
   {"closed and locked", 2, TRUE},
   {NULL, 0, 0}
};

const struct flag_type room_flags[] =
{
   {"dark", ROOM_DARK, TRUE},
   {"no_mob", ROOM_NO_MOB, TRUE},
   {"indoors", ROOM_INDOORS, TRUE},
   {"private", ROOM_PRIVATE, TRUE},
   {"safe", ROOM_SAFE, TRUE},
   {"solitary", ROOM_SOLITARY, TRUE},
   {"arena", ROOM_ARENA, TRUE},
   {"ferry", ROOM_FERRY, TRUE},
   {"pet_shop", ROOM_PET_SHOP, TRUE},
   {"no_recall", ROOM_NO_RECALL, TRUE},
   {"imp_only", ROOM_IMP_ONLY, TRUE},
   {"gods_only", ROOM_GODS_ONLY, TRUE},
   {"heroes_only", ROOM_HEROES_ONLY, TRUE},
   {"newbies_only", ROOM_NEWBIES_ONLY, TRUE},
   {"law", ROOM_LAW, TRUE},
   {"nowhere", ROOM_NOWHERE, TRUE},
   {"noquit", ROOM_NOQUIT, TRUE},
   {"rmclan", ROOM_CLAN, TRUE},
   {"no_gate", ROOM_NO_GATE, TRUE },
   {"no_kill", ROOM_NO_KILL, TRUE },
   {"no_lair", ROOM_NO_LAIR, TRUE },
   {"workshop", ROOM_WORKSHOP, TRUE },
   {NULL, 0, 0}
};

const struct flag_type sector_flags[] =
{
   {"city", SECT_CITY, TRUE},
   {"field", SECT_FIELD, TRUE},
   {"forest", SECT_FOREST, TRUE},
   {"hills", SECT_HILLS, TRUE},
   {"swamp", SECT_SWAMP, TRUE},
   {"mountain", SECT_MOUNTAIN, TRUE},
   {"swim", SECT_WATER_SWIM, TRUE},
   {"noswim", SECT_WATER_NOSWIM, TRUE},
   {"unused", SECT_UNUSED, TRUE},
   {"air", SECT_AIR, TRUE},
   {"desert", SECT_DESERT, TRUE},
   {"inside", SECT_INSIDE, TRUE},
   {"underwater", SECT_UNDERWATER, TRUE},
   {NULL, 0, 0},
   {NULL, -1, 0}
};

const struct flag_type type_flags[] =
{
   {"light", ITEM_LIGHT, TRUE},
   {"scroll", ITEM_SCROLL, TRUE},
   {"wand", ITEM_WAND, TRUE},
   {"staff", ITEM_STAFF, TRUE},
   {"weapon", ITEM_WEAPON, TRUE},
   {"treasure", ITEM_TREASURE, TRUE},
   {"armor", ITEM_ARMOR, TRUE},
   {"potion", ITEM_POTION, TRUE},
   {"furniture", ITEM_FURNITURE, TRUE},
   {"trash", ITEM_TRASH, TRUE},
   {"container", ITEM_CONTAINER, TRUE},
   {"drinkcontainer", ITEM_DRINK_CON, TRUE},
   {"key", ITEM_KEY, TRUE},
   {"food", ITEM_FOOD, TRUE},
   {"money", ITEM_MONEY, TRUE},
   {"boat", ITEM_BOAT, TRUE},
   {"npccorpse", ITEM_CORPSE_NPC, TRUE},
   {"pc corpse", ITEM_CORPSE_PC, FALSE},
   {"fountain", ITEM_FOUNTAIN, TRUE},
   {"pill", ITEM_PILL, TRUE},
   {"protect", ITEM_PROTECT, TRUE},
   {"map", ITEM_MAP, TRUE},
   {"portal", ITEM_PORTAL, TRUE},
   {"warpstone", ITEM_WARP_STONE, TRUE},
   {"roomkey", ITEM_ROOM_KEY, TRUE},
   {"gem", ITEM_GEM, TRUE},
   {"jewelry", ITEM_JEWELRY, TRUE},
   {"jukebox", ITEM_JUKEBOX, TRUE},
   {"throwing", ITEM_THROWING, TRUE},
   {"clothing", ITEM_CLOTHING, TRUE },
   {"recall_crystal", ITEM_RECALL, TRUE },
   {"slotmachine", ITEM_SLOT_MACHINE, TRUE},
   {"orb", ITEM_ORB, TRUE},
   {"sheath", ITEM_SHEATH, TRUE},
   {"charm", ITEM_CHARM, TRUE},
   {"ammunition", ITEM_AMMO, TRUE},
   {NULL, 0, 0}
};

const struct flag_type sheath_types[] =
{
   {"none", SHEATH_NONE, TRUE},
   {"weapon flag", SHEATH_FLAG, TRUE},
   {"dice type", SHEATH_DICETYPE, TRUE},
   {"dice count", SHEATH_DICECOUNT, TRUE},
   {"hitroll", SHEATH_DICECOUNT, TRUE},
   {"damroll", SHEATH_DAMROLL, TRUE},
   {"quickdraw", SHEATH_QUICKDRAW, TRUE},
   {"spell", SHEATH_SPELL, TRUE},
   {NULL, 0, 0},
};

const struct flag_type extra_flags[] =
{
   {"glow", ITEM_GLOW, TRUE},
   {"hum", ITEM_HUM, TRUE},
   {"dark", ITEM_DARK, TRUE},
   {"lock", ITEM_LOCK, TRUE},
   {"evil", ITEM_EVIL, TRUE},
   {"invis", ITEM_INVIS, TRUE},
   {"magic", ITEM_MAGIC, TRUE},
   {"nodrop", ITEM_NODROP, TRUE},
   {"bless", ITEM_BLESS, TRUE},
   {"antigood", ITEM_ANTI_GOOD, TRUE},
   {"antievil", ITEM_ANTI_EVIL, TRUE},
   {"antineutral", ITEM_ANTI_NEUTRAL, TRUE},
   {"noremove", ITEM_NOREMOVE, TRUE},
   {"inventory", ITEM_INVENTORY, TRUE},
   {"nopurge", ITEM_NOPURGE, TRUE},
   {"rotdeath", ITEM_ROT_DEATH, TRUE},
   {"visdeath", ITEM_VIS_DEATH, TRUE},
   {"nonmetal", ITEM_NONMETAL, TRUE},
   {"meltdrop", ITEM_MELT_DROP, TRUE},
   {"hadtimer", ITEM_HAD_TIMER, TRUE},
   {"nolocate", ITEM_NOLOCATE, TRUE},
   {"sellextract", ITEM_SELL_EXTRACT, TRUE},
   {"burnproof", ITEM_BURN_PROOF, TRUE},
   {"nouncurse", ITEM_NOUNCURSE, TRUE},
   {"warrior", ITEM_WARRIOR, TRUE},
   {"mage", ITEM_MAGE, TRUE},
   {"thief", ITEM_THIEF, TRUE},
   {"cleric", ITEM_CLERIC, TRUE},
   {"soundproof", ITEM_SOUND_PROOF, TRUE},
   {NULL, 0, 0}
};

const struct flag_type wear_flags[] =
{
   {"take", ITEM_TAKE, TRUE},
   {"finger", ITEM_WEAR_FINGER, TRUE},
   {"neck", ITEM_WEAR_NECK, TRUE},
   {"body", ITEM_WEAR_BODY, TRUE},
   {"head", ITEM_WEAR_HEAD, TRUE},
   {"legs", ITEM_WEAR_LEGS, TRUE},
   {"feet", ITEM_WEAR_FEET, TRUE},
   {"hands", ITEM_WEAR_HANDS, TRUE},
   {"arms", ITEM_WEAR_ARMS, TRUE},
   {"shield", ITEM_WEAR_SHIELD, TRUE},
   {"about", ITEM_WEAR_ABOUT, TRUE},
   {"waist", ITEM_WEAR_WAIST, TRUE},
   {"wrist", ITEM_WEAR_WRIST, TRUE},
   {"wield", ITEM_WIELD, TRUE},
   {"hold", ITEM_HOLD, TRUE},
   {"nosac", ITEM_NO_SAC, TRUE},
   {"wearfloat", ITEM_WEAR_FLOAT, TRUE},
   {"incomplete", ITEM_INCOMPLETE, TRUE},
   {"crafted", ITEM_CRAFTED, TRUE},
   {"charged", ITEM_CHARGED, TRUE},
   {"newbie", ITEM_NEWBIE, TRUE},
   {"norecall", ITEM_NORECALL, TRUE},
   {"nogate", ITEM_NOGATE, TRUE},
/*    {   "twohands",            ITEM_TWO_HANDS,         TRUE    }, */
   {NULL, 0, 0}
};

/*
 * Used when adding an affect to tell where it goes.
 * See addaffect and delaffect in act_olc.c
 */
const struct flag_type apply_flags[] =
{
   {"none", APPLY_NONE, TRUE},
   {"strength", APPLY_STR, TRUE},
   {"dexterity", APPLY_DEX, TRUE},
   {"intelligence", APPLY_INT, TRUE},
   {"wisdom", APPLY_WIS, TRUE},
   {"constitution", APPLY_CON, TRUE},
   {"sex", APPLY_SEX, TRUE},
   {"level", APPLY_LEVEL, TRUE},
   {"age", APPLY_AGE, TRUE},
   {"mana", APPLY_MANA, TRUE},
   {"hp", APPLY_HIT, TRUE},
   {"move", APPLY_MOVE, TRUE},
   {"ac", APPLY_AC, TRUE},
   {"hitroll", APPLY_HITROLL, TRUE},
   {"damroll", APPLY_DAMROLL, TRUE},
   {"saves", APPLY_SAVES, TRUE},
   {"savingpara", APPLY_SAVING_PARA, TRUE},
   {"savingrod", APPLY_SAVING_ROD, TRUE},
   {"savingpetri", APPLY_SAVING_PETRI, TRUE},
   {"savingbreath", APPLY_SAVING_BREATH, TRUE},
   {"savingspell", APPLY_SAVING_SPELL, TRUE},
   {"spellaffect", APPLY_SPELL_AFFECT, FALSE},
   {"damreduce", APPLY_DAMAGE_REDUCE, TRUE},
   {"spelldam", APPLY_SPELL_DAMAGE, TRUE},
   {"maxstr", APPLY_MAX_STR, TRUE},
   {"maxdex", APPLY_MAX_DEX, TRUE},
   {"maxcon", APPLY_MAX_CON, TRUE},
   {"maxint", APPLY_MAX_INT, TRUE},
   {"maxwis", APPLY_MAX_WIS, TRUE},
   {"attackspeed", APPLY_ATTACK_SPEED, TRUE},
   {NULL, 0, 0},
   {NULL, -1, 0}
};

/*
 * What is seen.
 */
const struct flag_type wear_loc_strings[] =
{
   {"in the inventory", WEAR_NONE, TRUE},
   {"as a light", WEAR_LIGHT, TRUE},
   {"on the left finger", WEAR_FINGER_L, TRUE},
   {"on the right finger", WEAR_FINGER_R, TRUE},
   {"around the neck (1)", WEAR_NECK_1, TRUE},
   {"around the neck (2)", WEAR_NECK_2, TRUE},
   {"on the body", WEAR_BODY, TRUE},
   {"over the head", WEAR_HEAD, TRUE},
   {"on the legs", WEAR_LEGS, TRUE},
   {"on the feet", WEAR_FEET, TRUE},
   {"on the hands", WEAR_HANDS, TRUE},
   {"on the arms", WEAR_ARMS, TRUE},
   {"as a shield", WEAR_SHIELD, TRUE},
   {"about the shoulders", WEAR_ABOUT, TRUE},
   {"around the waist", WEAR_WAIST, TRUE},
   {"on the left wrist", WEAR_WRIST_L, TRUE},
   {"on the right wrist", WEAR_WRIST_R, TRUE},
   {"wielded", WEAR_WIELD, TRUE},
   {"held in the hands", WEAR_HOLD, TRUE},
   {"floating nearby", WEAR_FLOAT, TRUE},
   {"ranged", WEAR_RANGED, TRUE},
   {"quivered", WEAR_AMMO, TRUE},
   {NULL, 0, 0}
};

const struct flag_type wear_loc_flags[] =
{
   {"none", WEAR_NONE, TRUE},
   {"light", WEAR_LIGHT, TRUE},
   {"lfinger", WEAR_FINGER_L, TRUE},
   {"rfinger", WEAR_FINGER_R, TRUE},
   {"neck1", WEAR_NECK_1, TRUE},
   {"neck2", WEAR_NECK_2, TRUE},
   {"body", WEAR_BODY, TRUE},
   {"head", WEAR_HEAD, TRUE},
   {"legs", WEAR_LEGS, TRUE},
   {"feet", WEAR_FEET, TRUE},
   {"hands", WEAR_HANDS, TRUE},
   {"arms", WEAR_ARMS, TRUE},
   {"shield", WEAR_SHIELD, TRUE},
   {"about", WEAR_ABOUT, TRUE},
   {"waist", WEAR_WAIST, TRUE},
   {"lwrist", WEAR_WRIST_L, TRUE},
   {"rwrist", WEAR_WRIST_R, TRUE},
   {"wielded", WEAR_WIELD, TRUE},
   {"hold", WEAR_HOLD, TRUE},
   {"floating", WEAR_FLOAT, TRUE},
   {NULL, 0, 0}
};

const struct flag_type weapon_flags[] =
{

   {"hit", 0, TRUE},                /*  0 */
   {"slice", 1, TRUE},
   {"stab", 2, TRUE},
   {"slash", 3, TRUE},
   {"whip", 4, TRUE},
   {"claw", 5, TRUE},                /*  5 */
   {"blast", 6, TRUE},
   {"pound", 7, TRUE},
   {"crush", 8, TRUE},
   {"grep", 9, TRUE},
   {"bite", 10, TRUE},                /* 10 */
   {"pierce", 11, TRUE},
   {"suction", 12, TRUE},
   {"beating", 13, TRUE},
   {"digestion", 14, TRUE},
   {"charge", 15, TRUE},        /* 15 */
   {"slap", 16, TRUE},
   {"punch", 17, TRUE},
   {"wrath", 18, TRUE},
   {"magic", 19, TRUE},
   {"divinepower", 20, TRUE},        /* 20 */
   {"cleave", 21, TRUE},
   {"scratch", 22, TRUE},
   {"peckpierce", 23, TRUE},
   {"peckbash", 24, TRUE},
   {"chop", 25, TRUE},                /* 25 */
   {"sting", 26, TRUE},
   {"smash", 27, TRUE},
   {"shockingbite", 28, TRUE},
   {"flamingbite", 29, TRUE},
   {"freezingbite", 30, TRUE},        /* 30 */
   {"acidicbite", 31, TRUE},
   {"chomp", 32, TRUE},
   {"lifedrain", 33, TRUE},
   {"thrust", 34, TRUE},
   {"slime", 35, TRUE},
   {"shock", 36, TRUE},
   {"thwack", 37, TRUE},
   {"flame", 38, TRUE},
   {"chill", 39, TRUE},
   {"impale", 40, TRUE},
   {"dicing", 41, TRUE},
   {"skewer", 42, TRUE},
   {"clobber", 43, TRUE},
   {"smite", 44, TRUE},
   {"grind", 45, TRUE},
   {"gutting", 46, TRUE},
   {"gore", 47, TRUE},
   {"splatter", 48, TRUE},
   {"skinning", 49, TRUE},
   {"saw", 50, TRUE},
   {"tear", 51, TRUE},
   {"rip", 52, TRUE},
   {"stomp", 53, TRUE},
   {"squeeze", 54, TRUE},
   {"chew", 55, TRUE},
   {"slaughter", 56, TRUE},
   {"strangle", 57, TRUE},
   {"entangle", 58, TRUE},
   {"force", 59, TRUE},
   {"firing", 60, TRUE},
   {"bolt", 61, TRUE},
   {"knock", 62, TRUE},
   {"smother", 63, TRUE},
   {"pobite", 64, TRUE},
   {NULL, 0, 0},
   {NULL, -1, 0}
};

const struct flag_type container_flags[] =
{
   {"closeable", 1, TRUE},
   {"pickproof", 2, TRUE},
   {"closed", 4, TRUE},
   {"locked", 8, TRUE},
   {"puton", 16, TRUE},
   {NULL, 0, 0}
};

/*****************************************************************************
                      ROM - specific tables:
 ****************************************************************************/

const struct flag_type ac_type[] =
{
   {"pierce", AC_PIERCE, TRUE},
   {"bash", AC_BASH, TRUE},
   {"slash", AC_SLASH, TRUE},
   {"exotic", AC_EXOTIC, TRUE},
   {NULL, 0, 0}
};

const struct flag_type size_flags[] =
{
   {"tiny", SIZE_TINY, TRUE},
   {"small", SIZE_SMALL, TRUE},
   {"medium", SIZE_MEDIUM, TRUE},
   {"large", SIZE_LARGE, TRUE},
   {"huge", SIZE_HUGE, TRUE},
   {"giant", SIZE_GIANT, TRUE},
   {NULL, 0, 0},
   {NULL, -1, -1}
};

const struct flag_type weapon_class[] =
{
   {"exotic", 0, TRUE},
   {"sword", 1, TRUE},
   {"dagger", 2, TRUE},
   {"spear", 3, TRUE},
   {"mace", 4, TRUE},
   {"axe", 5, TRUE},
   {"flail", 6, TRUE},
   {"whip", 7, TRUE},
   {"polearm", 8, TRUE},
   {"katana", 9, TRUE},
   {"ranged", 10, TRUE},
   {NULL, 0, 0},
   {NULL, -1, 0}
};

const struct flag_type weapon_type2[] =
{
   {"flaming", WEAPON_FLAMING, TRUE},
   {"frost", WEAPON_FROST, TRUE},
   {"vampiric", WEAPON_VAMPIRIC, TRUE},
   {"sharp", WEAPON_SHARP, TRUE},
   {"vorpal", WEAPON_VORPAL, TRUE},
   {"twohands", WEAPON_TWO_HANDS, TRUE},
   {"shocking", WEAPON_SHOCKING, TRUE},
   {"poison", WEAPON_POISON, TRUE},
   {"drag_slay", WEAPON_DRAGON_SLAYER, TRUE},
   {"blunt", WEAPON_BLUNT, TRUE},
   {"dull", WEAPON_DULL, TRUE},
   {"corrosive", WEAPON_CORROSIVE, TRUE},
   {"flooding", WEAPON_FLOODING, TRUE},
   {"infected", WEAPON_INFECTED, TRUE},
   {"no_mutate", WEAPON_NOMUTATE, TRUE},
   {"soul_drain", WEAPON_SOULDRAIN, TRUE},
   {"holy", WEAPON_HOLY, TRUE},
   {"unholy", WEAPON_UNHOLY, TRUE},
   {"polar", WEAPON_POLAR, TRUE},
   {"phase", WEAPON_PHASE, TRUE},
   {"antimagic", WEAPON_ANTIMAGIC, TRUE},
   {"entropic", WEAPON_ENTROPIC, TRUE},
   {"psionic", WEAPON_PSIONIC, TRUE},
   {"demonic", WEAPON_DEMONIC, TRUE},
   {"intelligent", WEAPON_INTELLIGENT, TRUE},
   {"none", 0, TRUE},
   {NULL, 0, 0}
};

const struct flag_type res_flags[] =
{
   {"summon", RES_SUMMON, TRUE},
   {"charm", RES_CHARM, TRUE},
   {"magic", RES_MAGIC, TRUE},
   {"weapon", RES_WEAPON, TRUE},
   {"bash", RES_BASH, TRUE},
   {"pierce", RES_PIERCE, TRUE},
   {"slash", RES_SLASH, TRUE},
   {"fire", RES_FIRE, TRUE},
   {"cold", RES_COLD, TRUE},
   {"light", RES_LIGHT, TRUE},
   {"lightning", RES_LIGHTNING, TRUE},
   {"acid", RES_ACID, TRUE},
   {"poison", RES_POISON, TRUE},
   {"negative", RES_NEGATIVE, TRUE},
   {"holy", RES_HOLY, TRUE},
   {"energy", RES_ENERGY, TRUE},
   {"mental", RES_MENTAL, TRUE},
   {"disease", RES_DISEASE, TRUE},
   {"drowning", RES_DROWNING, TRUE},
   {"sound", RES_SOUND, TRUE},
   {"wood", RES_WOOD, TRUE},
   {"silver", RES_SILVER, TRUE},
   {"iron", RES_IRON, TRUE},
   {"rain", RES_RAIN, TRUE},
   {NULL, 0, 0}
};

const struct flag_type vuln_flags[] =
{
   {"summon", VULN_SUMMON, TRUE},
   {"charm", VULN_CHARM, TRUE},
   {"magic", VULN_MAGIC, TRUE},
   {"weapon", VULN_WEAPON, TRUE},
   {"bash", VULN_BASH, TRUE},
   {"pierce", VULN_PIERCE, TRUE},
   {"slash", VULN_SLASH, TRUE},
   {"fire", VULN_FIRE, TRUE},
   {"cold", VULN_COLD, TRUE},
   {"light", VULN_LIGHT, TRUE},
   {"lightning", VULN_LIGHTNING, TRUE},
   {"acid", VULN_ACID, TRUE},
   {"poison", VULN_POISON, TRUE},
   {"negative", VULN_NEGATIVE, TRUE},
   {"holy", VULN_HOLY, TRUE},
   {"energy", VULN_ENERGY, TRUE},
   {"mental", VULN_MENTAL, TRUE},
   {"disease", VULN_DISEASE, TRUE},
   {"drowning", VULN_DROWNING, TRUE},
   {"sound", VULN_SOUND, TRUE},
   {"wood", VULN_WOOD, TRUE},
   {"silver", VULN_SILVER, TRUE},
   {"iron", VULN_IRON, TRUE},
   {"rain", VULN_RAIN, TRUE},
   {NULL, 0, 0}
};

const struct flag_type material_type[] =        /* not yet implemented */
{
   {"acid", 1, TRUE},
   {"adamantite", 2, TRUE},
   {"air", 3, TRUE},
   {"ash", 4, TRUE},
   {"balm", 5, TRUE},
   {"bamboo", 6, TRUE},
   {"bone", 7, TRUE},
   {"brass", 8, TRUE},
   {"bronze", 9, TRUE},
   {"canvas", 10, TRUE},
   {"cardboard", 11, TRUE},
   {"clay", 12, TRUE},
   {"cloth", 13, TRUE},
   {"coal", 14, TRUE},
   {"copper", 15, TRUE},
   {"coral", 16, TRUE},
   {"cork", 17, TRUE},
   {"cream", 18, TRUE},
   {"crystal", 19, TRUE},
   {"diamond", 20, TRUE},
   {"earth", 21, TRUE},
   {"ebony", 22, TRUE},
   {"electrum", 23, TRUE},
   {"enamel", 24, TRUE},
   {"energy", 25, TRUE},
   {"feathers", 26, TRUE},
   {"felt", 27, TRUE},
   {"fire", 28, TRUE},
   {"flint", 29, TRUE},
   {"fur", 30, TRUE},
   {"glass", 31, TRUE},
   {"gold", 32, TRUE},
   {"granite", 33, TRUE},
   {"hemp", 34, TRUE},
   {"ice", 35, TRUE},
   {"iron", 36, TRUE},
   {"ivory", 37, TRUE},
   {"jelly", 38, TRUE},
   {"lace", 39, TRUE},
   {"lead", 40, TRUE},
   {"leather", 41, TRUE},
   {"linen", 42, TRUE},
   {"marble", 43, TRUE},
   {"metal", 44, TRUE},
   {"mithril", 45, TRUE},
   {"oil", 46, TRUE},
   {"paper", 47, TRUE},
   {"parchment", 48, TRUE},
   {"pewter", 49, TRUE},
   {"plastic", 50, TRUE},
   {"platinum", 51, TRUE},
   {"porcelain", 52, TRUE},
   {"quartz", 53, TRUE},
   {"rubber", 54, TRUE},
   {"sandstone", 55, TRUE},
   {"satin", 56, TRUE},
   {"shell", 57, TRUE},
   {"silk", 58, TRUE},
   {"silver", 59, TRUE},
   {"slime", 60, TRUE},
   {"snakeskin", 61, TRUE},
   {"sponge", 62, TRUE},
   {"steel", 63, TRUE},
   {"stone", 64, TRUE},
   {"tin", 65, TRUE},
   {"vellum", 66, TRUE},
   {"velvet", 67, TRUE},
   {"water", 68, TRUE},
   {"wax", 69, TRUE},
   {"webbing", 70, TRUE},
   {"wire", 71, TRUE},
   {"wood", 72, TRUE},
   {"wool", 73, TRUE},
   {"food", 74, TRUE },
   {"gem", 75, TRUE},
   {"none", 0, TRUE},
   {NULL, 0,}
};

const struct flag_type position_flags[] =
{
   {"dead", POS_DEAD, FALSE},
   {"mortal", POS_MORTAL, FALSE},
   {"incap", POS_INCAP, FALSE},
   {"stunned", POS_STUNNED, FALSE},
   {"sleeping", POS_SLEEPING, TRUE},
   {"resting", POS_RESTING, TRUE},
   {"sitting", POS_SITTING, TRUE},
   {"fighting", POS_FIGHTING, FALSE},
   {"standing", POS_STANDING, TRUE},
   {NULL, 0, 0},
   {NULL, -1, -1}
};

const struct flag_type portal_flags[] =
{
   {"normal_exit", GATE_NORMAL_EXIT, TRUE},
   {"no_curse", GATE_NOCURSE, TRUE},
   {"go_with", GATE_GOWITH, TRUE},
   {"buggy", GATE_BUGGY, TRUE},
   {"random", GATE_RANDOM, TRUE},
   {"none", 0, TRUE},
   {NULL, 0, 0}
};

const struct flag_type furniture_flags[] =
{
   {"stand_at", STAND_AT, TRUE},
   {"stand_on", STAND_ON, TRUE},
   {"stand_in", STAND_IN, TRUE},
   {"sit_at", SIT_AT, TRUE},
   {"sit_on", SIT_ON, TRUE},
   {"sit_in", SIT_IN, TRUE},
   {"rest_at", REST_AT, TRUE},
   {"rest_on", REST_ON, TRUE},
   {"rest_in", REST_IN, TRUE},
   {"sleep_at", SLEEP_AT, TRUE},
   {"sleep_on", SLEEP_ON, TRUE},
   {"sleep_in", SLEEP_IN, TRUE},
   {"put_at", PUT_AT, TRUE},
   {"put_on", PUT_ON, TRUE},
   {"put_in", PUT_IN, TRUE},
   {"put_inside", PUT_INSIDE, TRUE},
   {"none", 0, TRUE},
   {NULL, 0, 0},
   {NULL, -1, -1}
};

const struct flag_type mprog_flags[] =
{
    {"act",    TRIG_ACT,    TRUE},
    {"bribe",  TRIG_BRIBE,  TRUE},
    {"death",  TRIG_DEATH,  TRUE},
    {"entry",  TRIG_ENTRY,  TRUE},
    {"fight",  TRIG_FIGHT,  TRUE},
    {"give",   TRIG_GIVE,   TRUE},
    {"greet",  TRIG_GREET,  TRUE},
    {"grall",  TRIG_GRALL,  TRUE},
    {"kill",   TRIG_KILL,   TRUE},
    {"hpcnt",  TRIG_HPCNT,  TRUE},
    {"random", TRIG_RANDOM, TRUE},
    {"speech", TRIG_SPEECH, TRUE},
    {"exit",   TRIG_EXIT,   TRUE},
    {"exall",  TRIG_EXALL,  TRUE},
    {"delay",  TRIG_DELAY,  TRUE},
    {"surr",   TRIG_SURR,   TRUE},
    {NULL,     0,           0}
};
