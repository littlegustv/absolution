#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"

#ifndef MERC_H
#define MERC_H

/* enable crypt 

#ifndef NOCRYPT
  #define _XOPEN_SOURCE
  #include <unistd.h>

  extern char *crypt(const char *key, const char *salt);
#endif
*/

#include <netinet/in.h>

#define DECLARE_DO_FUN( fun )    DO_FUN    fun
#define DECLARE_SPEC_FUN( fun )     SPEC_FUN  fun
#define DECLARE_SPELL_FUN( fun ) SPELL_FUN fun
#define DECLARE_END_FUN( fun ) END_FUN fun

/* system calls */
int unlink();
int system();

#define OLD_RAND 0

#define FALSE   0
#define TRUE    1

typedef int bool;
typedef long long bitset;

/*
 * Structure types.
 */
typedef struct area_data AREA_DATA;
typedef struct ban_data BAN_DATA;
typedef struct buf_type BUFFER;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct help_data HELP_DATA;
typedef struct kill_data KILL_DATA;
typedef struct mem_data MEM_DATA;
typedef struct mob_index_data MOB_INDEX_DATA;
typedef struct note_data NOTE_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;
typedef struct pc_data PC_DATA;

typedef struct gen_data GEN_DATA;
typedef struct reset_data RESET_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct shop_data SHOP_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct weather_data WEATHER_DATA;
typedef struct macro_data MACRO_DATA;
typedef struct instance_data INSTANCE_DATA;
typedef struct auction_data AUCTION_DATA;

#include "board.h"
#include "affects.h"

// Sets if the test port actions are on or off.
// 1 means on, 0 means off.
#define TEST_PORT        (0)

#define PULSE_AUCTION             (25 * PULSE_PER_SECOND)	/* 10 seconds */


struct auction_data
{
	OBJ_DATA *item;				/* a pointer to the item */
	CHAR_DATA *seller;			/* a pointer to the seller - which may NOT quit */
	CHAR_DATA *buyer;			/* a pointer to the buyer - which may NOT quit */
	int bet;					/* last bet - or 0 if noone has bet anything */
	int going;				/* 1,2, sold */
	int pulse;				/* how many pulses (.25 sec) until another call-out ? */
	int bet_silver;
	int bet_gold;
};

extern AUCTION_DATA *auction;


/*
 * Function types.
 */
typedef void DO_FUN(CHAR_DATA * ch, char *argument);
typedef bool SPEC_FUN(CHAR_DATA * ch);
typedef void SPELL_FUN(int sn, int level, CHAR_DATA * ch, void *vo, int target);
typedef void END_FUN(void* vo, int target);



/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH      1024
#define MAX_STRING_LENGTH 9216
#define MAX_INPUT_LENGTH  256
#define PAGELEN           22

/*
 * Colour stuff by Lope of Loping Through The MUD
 */
#define CLEAR                "[0m"		/* Resets Colour  */
#define C_RED                "[0;31m"	/* Normal Colours */
#define C_GREEN                "[0;32m"
#define C_YELLOW        "[0;33m"
#define C_BLUE                "[0;34m"
#define C_MAGENTA        "[0;35m"
#define C_CYAN                "[0;36m"
#define C_WHITE                "[0;37m"
#define C_D_GREY        "[1;30m"	/* Light Colors           */
#define C_B_RED                "[1;31m"
#define C_B_GREEN        "[1;32m"
#define C_B_YELLOW        "[1;33m"
#define C_B_BLUE        "[1;34m"
#define C_B_MAGENTA        "[1;35m"
#define C_UNDERLINE        "[4m"
#define C_B_CYAN        "[1;36m"
#define C_B_WHITE        "[1;37m"


/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_SOCIALS    300
#define MAX_SKILL      442
#define MAX_GROUP      53
#define MAX_IN_GROUP   18
#define MAX_ALIAS       5
#define MAX_CLASS      12
#define MAX_PC_RACE     16
#define MAX_DAMAGE_MESSAGE   70
#define MAX_LEVEL       60
#define LEVEL_HERO         (MAX_LEVEL - 9)
#define LEVEL_IMMORTAL        (54)

#define PULSE_PER_SECOND      4
#define PULSE_VIOLENCE       ( 3 * PULSE_PER_SECOND)
#define PULSE_RAIN     ( 10 * PULSE_PER_SECOND)
#define PULSE_MOBILE      ( 4 * PULSE_PER_SECOND)
#define PULSE_MUSIC       ( 6 * PULSE_PER_SECOND)
#define PULSE_TICK        (60 * PULSE_PER_SECOND)
#define PULSE_AREA        (60 * PULSE_PER_SECOND)

#define IMPLEMENTOR     MAX_LEVEL
#define CREATOR        (MAX_LEVEL - 1)
#define SUPREME        (MAX_LEVEL - 2)
#define DEITY          (MAX_LEVEL - 3)
#define GOD            (MAX_LEVEL - 4)
#define IMMORTAL       (MAX_LEVEL - 5)
#define DEMI           (MAX_LEVEL - 6)
#define ANGEL          (MAX_LEVEL - 6)
#define AVATAR         (MAX_LEVEL - 6)
#define HERO            LEVEL_HERO

/* macro stuff */
#define MAX_NUM_SUB_PER_MACRO (10)
#define MAX_NUM_MACROS (10)
#define MAX_NUM_COM_PER_TRACE (50)
#define MAX_NUM_MACRO_VARS (10)
#define MACRO_PAUSE (1)

// These are flags for clan_status flag on powerful equipment
#define CS_UNSPECIFIED (0)
#define CS_NONCLANNER  (1)
#define CS_CLANNER     (2)

#define GUILD_HELP_VNUM    11240
#define GUILD_BUNNIES_VNUM 11239

#define DRAGON_ARMOR  0

/*
 * Site ban structure.
 */

#define BAN_SUFFIX      A
#define BAN_PREFIX      B
#define BAN_NEWBIES     C
#define BAN_ALL         D
#define BAN_PERMIT      E
#define BAN_PERMANENT   F

struct ban_data
{
	BAN_DATA *next;
	bool valid;
	int ban_flags;
	int level;
	char *name;
};

struct struckdrunk
{
	int min_drunk_level;
	int number_of_rep;
	char *replacement[11];
};


struct buf_type
{
	BUFFER *next;
	bool valid;
	int state;				/* error state of the buffer */
	int size;				/* size in k */
	char *string;				/* buffer's string */
};



/*
 * Time and weather stuff.
 */
#define SUN_DARK       0
#define SUN_RISE       1
#define SUN_LIGHT      2
#define SUN_SET        3

#define SKY_CLOUDLESS  0
#define SKY_CLOUDY     1
#define SKY_RAINING    2
#define SKY_LIGHTNING  3

struct time_info_data
{
	int hour;
	int day;
	int month;
	int year;
};

struct weather_data
{
	int mmhg;
	int change;
	int sky;
	int sunlight;
};



/*
 * Connected state for a channel.
 */
#define CON_PLAYING               0
#define CON_GET_NAME              1
#define CON_GET_OLD_PASSWORD      2
#define CON_CONFIRM_NEW_NAME      3
#define CON_GET_NEW_PASSWORD      4
#define CON_CONFIRM_NEW_PASSWORD  5
#define CON_GET_NEW_RACE          6
#define CON_GET_NEW_SEX           7
#define CON_GET_NEW_CLASS         8
#define CON_GET_ALIGNMENT         9
#define CON_DEFAULT_CHOICE       10
#define CON_GEN_GROUPS           11
#define CON_PICK_WEAPON          12
#define CON_PICK_DEITY           13
#define CON_READ_IMOTD           14
#define CON_READ_MOTD            15
#define CON_BREAK_CONNECT        16
#define CON_CAN_CLAN             17
#define CON_GET_CONTINENT        18


/*
 * Descriptor (channel) structure.
 */
struct descriptor_data {
    DESCRIPTOR_DATA *next;
    DESCRIPTOR_DATA *snoop_by;
    CHAR_DATA *character;
    CHAR_DATA *original;
    bool valid;
    char *host;
    long addr;
    int descriptor;
    int connected;
    bool fcommand;
    char inbuf[4 * MAX_INPUT_LENGTH];
    char incomm[MAX_INPUT_LENGTH];
    char inlast[MAX_INPUT_LENGTH];
    int repeat;
    char *outbuf;
    int outsize;
    int outtop;
    char *showstr_head;
    char *showstr_point;
    int ifd;
    pid_t ipid;
    char *ident;
    int port;
    int ip;
    char iphost[MAX_STRING_LENGTH];

    MOB_INDEX_DATA *pMEdit;
    OBJ_INDEX_DATA *pOEdit;
    ROOM_INDEX_DATA *pREdit;
    AREA_DATA *pAEdit;
    MPROG_CODE *pPEdit;
    HELP_DATA *pHEdit;
    BOARD_DATA *pBEdit;
    char **pString;  /* OLC */
    int editor;      /* OLC */
    int attempt;     /* password attempt */
};



/*
 * Attribute bonus structures.
 */
struct str_app_type
{
	int tohit;
	int todam;
	int carry;
	int wield;
};

struct int_app_type
{
	int learn;
};

struct wis_app_type
{
	int practice;
};

struct dex_app_type
{
	int defensive;
};

struct con_app_type
{
	int hitp;
	int shock;
};



/*
 * TO types for act.
 */
#define TO_ROOM          0
#define TO_NOTVICT       1
#define TO_VICT          2
#define TO_CHAR          3
#define TO_ALL        4



/*
 * Help table types.
 */
struct help_data
{
	HELP_DATA *next;
	int level;
	char *keyword;
	char *text;
};



/*
 * Shop types.
 */
#define MAX_TRADE  5

struct shop_data
{
	SHOP_DATA *next;			/* Next shop in list    */
	int keeper;				/* Vnum of shop keeper mob */
	int buy_type[MAX_TRADE];	/* Item types shop will buy   */
	int profit_buy;			/* Cost multiplier for buying */
	int profit_sell;			/* Cost multiplier for selling   */
	int open_hour;			/* First opening hour      */
	int close_hour;			/* First closing hour      */
};



/*
 * Per-class stuff.
 */

#define MAX_GUILD    2
#define MAX_STATS    5
#define STAT_NONE -1
#define STAT_STR  0
#define STAT_INT  1
#define STAT_WIS  2
#define STAT_DEX  3
#define STAT_CON  4

#define SAVE_EASY 1
#define SAVE_NORMAL 2
#define SAVE_HARD 3

struct class_type
{
	char *name;					/* the full name of the class */
	char who_name[8];			/* Three-letter name for 'who'   */
	int attr_prime;			/* Prime attribute      */
	int weapon;				/* First weapon         */
	int guild[MAX_GUILD];	/* Vnum of guild rooms     */
	int skill_adept;			/* Maximum skill level     */
	int thac0_00;			/* Thac0 for level  0      */
	int thac0_32;			/* Thac0 for level 32      */
	int hp_min;				/* Min hp gained on leveling  */
	int hp_max;				/* Max hp gained on leveling  */
	bool fMana;					/* Class gains mana on level  */
	char *base_group;			/* base skills gained      */
	char *default_group;		/* default skills gained   */
};

struct item_type
{
	int type;
	char *name;
};

struct weapon_type
{
	char *name;
	int vnum;
	int type;
	int *gsn;
};

struct wiznet_type
{
	char *name;
	bitset flag;
	int level;
};

struct attack_type
{
	char *name;					/* name */
	char *noun;					/* message */
	int damage;					/* damage class */
};

struct race_type
{
	char *name;					/* call name of the race */
	bool pc_race;				/* can be chosen by pcs */
	long act;					/* act bits for the race */
	long aff;					/* aff bits for the race */
	long off;					/* off bits for the race */
	long imm;					/* imm bits for the race */
	long res;					/* res bits for the race */
	long vuln;					/* vuln bits for the race */
	long form;					/* default form flag for the race */
	long parts;					/* default parts for the race */
};

struct sliver_type
{
	char *name;					/* name of the sliver sub-race */
	int number;				/* affect number stored in the pfile */
	int location;			/* affect location */
	int amount;				/* affect amount */
};


struct pc_race_type				/* additional data for pc races */
{
	char *name;					/* MUST be in race_type */
	char who_name[12];
	int points;				/* cost in points of the race */
	int class_mult[MAX_CLASS];	/* exp multiplier for class, * 100 */
	char *skills[5];			/* bonus skills for the race */
	int stats[MAX_STATS];	/* starting stats */
	int max_stats[MAX_STATS];	/* maximum stats */
	int size;				/* aff bits for the race */
};


struct spec_type
{
	char *name;					/* special function name */
	SPEC_FUN *function;			/* the function */
};



/*
 * Data structure for notes.
 */

#define NOTE_NOTE 0
#define NOTE_IDEA 1
#define NOTE_PENALTY 2
#define NOTE_NEWS 3
#define NOTE_CHANGES 4
struct note_data
{
	NOTE_DATA *next;
	bool valid;
	int type;
	char *sender;
	char *date;
	char *to_list;
	char *subject;
	char *text;
	time_t date_stamp;
	time_t expire;
};



/* where definitions */
#define TO_AFFECTS   0
#define TO_OBJECT 1
#define TO_IMMUNE 2
#define TO_RESIST 3
#define TO_VULN      4
#define TO_WEAPON 5


/*
 * A kill structure (indexed by level).
 */
struct kill_data
{
	int number;
	int killed;
};


/* macro stuff */
struct macro_data
{
	char *name;
	char *definition;
	char *subs[MAX_NUM_SUB_PER_MACRO];
};

struct instance_data
{
	char *macroName;
	int counter;
	char *vars[MAX_NUM_MACRO_VARS];
	INSTANCE_DATA *next;
	INSTANCE_DATA *child;
};


/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_EXECUTIONER  3011
#define MOB_VNUM_FIDO         3090
#define MOB_VNUM_CITYGUARD    3060
#define MOB_VNUM_VAMPIRE      3404
#define MOB_VNUM_DEMON_MANES 1870
#define MOB_VNUM_DEMON_SPINAGON 1871
#define MOB_VNUM_DEMON_BARLGURA 1874
#define MOB_VNUM_DEMON_SUCCUBUS 1872
#define MOB_VNUM_DEMON_ABISHAI 1873
#define MOB_VNUM_DEMON_OSYLUTH 1875
#define MOB_VNUM_DEMON_CORNUGON 1877
#define MOB_VNUM_DEMON_GLABREZU 1878
#define MOB_VNUM_DEMON_VROCK 1876
#define MOB_VNUM_DEMON_GELUGON 1879
#define MOB_VNUM_DEMON_BALOR 1880
#define MOB_VNUM_DEMON_PITFIEND 1881
#define MOB_VNUM_DEMON_JUIBLEX 1882
#define MOB_VNUM_DEMON_BEELZEBUB 1883
#define MOB_VNUM_DEMON_ORCUS 1884
#define MOB_VNUM_DEMON_MEPHISTOPHILIS 1885
#define MOB_VNUM_DEMON_DEMOGORGON 1886
#define MOB_VNUM_DEMON_ASMODEUS 1887
#define MOB_VNUM_TROLL_ARM 1897
#define MOB_VNUM_TROLL_LEG 1898
#define MOB_VNUM_HOMONCULUS 1850
#define MOB_VNUM_SKEL 1851
#define MOB_VNUM_TREE 1002
#define MOB_VNUM_FIGURINE 1003
#define MOB_VNUM_ANGEL 6717
#define MOB_VNUM_FLESH_GOLEM 6718
#define MOB_VNUM_ZOMBIE 6719
#define MOB_VNUM_EYE 1004
#define MOB_VNUM_DUP 1005
#define MOB_VNUM_HUNT_DOG 1006
#define MOB_VNUM_SLIVER_SOLDIER 6720

#define OBJ_VNUM_ELEMENTAL_A       3723
#define OBJ_VNUM_ELEMENTAL_B       3724
#define OBJ_VNUM_ELEMENTAL_C       3725
#define OBJ_VNUM_ELEMENTAL_D       3726
#define OBJ_VNUM_WRATH                   3727
#define OBJ_VNUM_STEAK                   6644
#define OBJ_VNUM_BOULDER           10009
#define OBJ_VNUM_SCALE_A           10010
#define OBJ_VNUM_SCALE_B           10011
#define OBJ_VNUM_SCALE_C           10012
#define OBJ_VNUM_GEM_A             10013
#define OBJ_VNUM_GEM_B             10014
#define OBJ_VNUM_GEM_C             10015
#define OBJ_VNUM_POT_A             10025
#define OBJ_VNUM_POT_B             10026
#define OBJ_VNUM_POT_C             10027
#define OBJ_VNUM_POT_D             10029
#define OBJ_VNUM_GRANITE_STATUE    6
#define OBJ_VNUM_FIGURINE_A        3381
#define OBJ_VNUM_FIGURINE_B        31109
#define OBJ_VNUM_VIAL_A			   3382
#define OBJ_VNUM_VIAL_B            31111
#define OBJ_VNUM_SCROLL_A          3384
#define OBJ_VNUM_SCROLL_B          31112
#define OBJ_VNUM_ANGEL_SWORD       6717
#define OBJ_VNUM_CARVED_SPEAR      6719
#define OBJ_VNUM_PLANT_A		   1005
#define OBJ_VNUM_PLANT_B		   1006
#define OBJ_VNUM_PLANT_C		   1007
#define OBJ_VNUM_PLANT_D		   1008
#define OBJ_VNUM_VOODOO            6720
#define OBJ_VNUM_HUNTER_BALL	   10905
#define OBJ_VNUM_OIL_BOMB          6721
#define OBJ_VNUM_UNREMORT_1        1203
#define OBJ_VNUM_UNREMORT_2        1204
#define OBJ_VNUM_UNREMORT_3        1205
#define OBJ_VNUM_UNREMORT_ITEM     1206
#define OBJ_VNUM_UNRECLASS_1       1207
#define OBJ_VNUM_UNRECLASS_2       1208
#define OBJ_VNUM_UNRECLASS_3       1209
#define OBJ_VNUM_UNRECLASS_ITEM    1210

#define MOB_VNUM_INSECT            3725
#define MOB_VNUM_ELEMENTAL_A       3721
#define MOB_VNUM_ELEMENTAL_B       3722
#define MOB_VNUM_ELEMENTAL_C       3723
#define MOB_VNUM_ELEMENTAL_D       3724
#define MOB_VNUM_PHANTASM          3726
#define MOB_VNUM_CLONE             10004
#define MOB_VNUM_BEAR              10005
#define MOB_VNUM_FAMILIAR          10906

#define MOB_VNUM_WRATH                   3727

#define MOB_VNUM_PATROLMAN    2106
#define GROUP_VNUM_TROLLS     2100
#define GROUP_VNUM_OGRES      2101


/* RT ASCII conversions -- used so we can have letters in this file */

#define A         0x1
#define B         0x2
#define C         0x4
#define D         0x8

#define E         0x10
#define F         0x20
#define G         0x40
#define H         0x80

#define I         0x100
#define J         0x200
#define K         0x400
#define L         0x800

#define M         0x1000
#define N         0x2000
#define O         0x4000
#define P         0x8000

#define Q         0x10000
#define R         0x20000
#define S         0x40000
#define T         0x80000

#define U         0x100000
#define V         0x200000
#define W         0x400000
#define X         0x800000

#define Y         0x1000000
#define Z         0x2000000
#define aa        0x4000000     /* doubled due to conflicts */
#define bb        0x8000000

#define cc        0x10000000
#define dd        0x20000000
#define ee        0x40000000
#define ff        0x80000000

#define gg        0x100000000
#define hh        0x200000000
#define ii        0x400000000
#define jj        0x800000000

#define kk        0x1000000000
#define ll        0x2000000000
#define mm        0x4000000000
#define nn        0x8000000000

#define oo        0x10000000000
#define pp        0x20000000000
#define qq        0x40000000000
#define rr        0x80000000000


/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC         (A)	/* Auto set for mobs */
#define ACT_SENTINEL       (B)	/* Stays in one room */
#define ACT_SCAVENGER      (C)	/* Picks up objects  */
#define ACT_LEGENDARY      (E)  /* Rare monster      */
#define ACT_AGGRESSIVE     (F)	/* Attacks PC's      */
#define ACT_STAY_AREA      (G)	/* Won't leave area  */
#define ACT_WIMPY          (H)
#define ACT_PET            (I)	/* Auto set for pets */
#define ACT_TRAIN          (J)	/* Can train PC's */
#define ACT_PRACTICE       (K)	/* Can practice PC's */
#define ACT_DEALER         (L)
#define ACT_UNDEAD         (O)
#define ACT_CLERIC         (Q)
#define ACT_MAGE           (R)
#define ACT_THIEF          (S)
#define ACT_WARRIOR        (T)
#define ACT_NOALIGN        (U)
#define ACT_NOPURGE        (V)
#define ACT_OUTDOORS       (W)
#define ACT_INDOORS        (Y)
#define ACT_IS_WIZI        (Z)
#define ACT_IS_HEALER      (aa)
#define ACT_GAIN           (bb)
#define ACT_UPDATE_ALWAYS  (cc)
#define ACT_IS_CHANGER     (dd)
#define ACT_BANKER         (ee)

/* damage classes */
#define DAM_NONE                0
#define DAM_BASH                1
#define DAM_PIERCE              2
#define DAM_SLASH               3
#define DAM_FIRE                4
#define DAM_COLD                5
#define DAM_LIGHTNING           6
#define DAM_ACID                7
#define DAM_POISON              8
#define DAM_NEGATIVE            9
#define DAM_HOLY                10
#define DAM_ENERGY              11
#define DAM_MENTAL              12
#define DAM_DISEASE             13
#define DAM_DROWNING            14
#define DAM_LIGHT               15
#define DAM_OTHER               16
#define DAM_HARM                17
#define DAM_CHARM               18
#define DAM_SOUND               19
#define DAM_RAIN                20
#define DAM_VORPAL              21

// Spell damages
#define SPELL_DAMAGE_LOW        0
#define SPELL_DAMAGE_MEDIUM     1
#define SPELL_DAMAGE_HIGH       2
#define SPELL_DAMAGE_CHART_GOOD 3
#define SPELL_DAMAGE_CHART_POOR 4

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK         (A)
#define OFF_BACKSTAB            (B)
#define OFF_BASH                (C)
#define OFF_BERSERK             (D)
#define OFF_DISARM              (E)
#define OFF_DODGE               (F)
#define OFF_FADE                (G)
#define OFF_FAST                (H)
#define OFF_KICK                (I)
#define OFF_KICK_DIRT           (J)
#define OFF_PARRY               (K)
#define OFF_RESCUE              (L)
#define OFF_TAIL                (M)
#define OFF_TRIP                (N)
#define OFF_CRUSH               (O)
#define ASSIST_ALL              (P)
#define ASSIST_ALIGN            (Q)
#define ASSIST_RACE             (R)
#define ASSIST_PLAYERS          (S)
#define ASSIST_GUARD            (T)
#define ASSIST_VNUM             (U)

/* return values for check_imm */
#define IS_NORMAL          0
#define IS_IMMUNE          1
#define IS_RESISTANT       2
#define IS_VULNERABLE      3

/* IMM bits for mobs */
#define IMM_SUMMON              (A)
#define IMM_CHARM               (B)
#define IMM_MAGIC               (C)
#define IMM_WEAPON              (D)
#define IMM_BASH                (E)
#define IMM_PIERCE              (F)
#define IMM_SLASH               (G)
#define IMM_FIRE                (H)
#define IMM_COLD                (I)
#define IMM_LIGHTNING           (J)
#define IMM_ACID                (K)
#define IMM_POISON              (L)
#define IMM_NEGATIVE            (M)
#define IMM_HOLY                (N)
#define IMM_ENERGY              (O)
#define IMM_MENTAL              (P)
#define IMM_DISEASE             (Q)
#define IMM_DROWNING            (R)
#define IMM_LIGHT               (S)
#define IMM_SOUND               (T)
#define IMM_WOOD                (X)
#define IMM_SILVER              (Y)
#define IMM_IRON                (Z)
#define IMM_RAIN                (aa)
#define IMM_VORPAL              (bb)

/* RES bits for mobs */
#define RES_SUMMON              (A)
#define RES_CHARM               (B)
#define RES_MAGIC               (C)
#define RES_WEAPON              (D)
#define RES_BASH                (E)
#define RES_PIERCE              (F)
#define RES_SLASH               (G)
#define RES_FIRE                (H)
#define RES_COLD                (I)
#define RES_LIGHTNING           (J)
#define RES_ACID                (K)
#define RES_POISON              (L)
#define RES_NEGATIVE            (M)
#define RES_HOLY                (N)
#define RES_ENERGY              (O)
#define RES_MENTAL              (P)
#define RES_DISEASE             (Q)
#define RES_DROWNING            (R)
#define RES_LIGHT               (S)
#define RES_SOUND               (T)
#define RES_WOOD                (X)
#define RES_SILVER              (Y)
#define RES_IRON                (Z)
#define RES_SPELL               (aa)
#define RES_RAIN                (bb)

/* VULN bits for mobs */
#define VULN_SUMMON     (A)
#define VULN_CHARM      (B)
#define VULN_MAGIC              (C)
#define VULN_WEAPON             (D)
#define VULN_BASH               (E)
#define VULN_PIERCE             (F)
#define VULN_SLASH              (G)
#define VULN_FIRE               (H)
#define VULN_COLD               (I)
#define VULN_LIGHTNING          (J)
#define VULN_ACID               (K)
#define VULN_POISON             (L)
#define VULN_NEGATIVE           (M)
#define VULN_HOLY               (N)
#define VULN_ENERGY             (O)
#define VULN_MENTAL             (P)
#define VULN_DISEASE            (Q)
#define VULN_DROWNING           (R)
#define VULN_LIGHT      (S)
#define VULN_SOUND      (T)
#define VULN_WOOD               (X)
#define VULN_SILVER             (Y)
#define VULN_IRON    (Z)
#define VULN_RAIN               (aa)

/* body form */
#define FORM_EDIBLE             (A)
#define FORM_POISON             (B)
#define FORM_MAGICAL            (C)
#define FORM_INSTANT_DECAY      (D)
#define FORM_OTHER              (E)		/* defined by material bit */

/* actual form */
#define FORM_ANIMAL             (G)
#define FORM_SENTIENT           (H)
#define FORM_UNDEAD             (I)
#define FORM_CONSTRUCT          (J)
#define FORM_MIST               (K)
#define FORM_INTANGIBLE         (L)

#define FORM_BIPED              (M)
#define FORM_CENTAUR            (N)
#define FORM_INSECT             (O)
#define FORM_SPIDER             (P)
#define FORM_CRUSTACEAN         (Q)
#define FORM_WORM               (R)
#define FORM_BLOB    (S)

#define FORM_MAMMAL             (V)
#define FORM_BIRD               (W)
#define FORM_REPTILE            (X)
#define FORM_SNAKE              (Y)
#define FORM_DRAGON             (Z)
#define FORM_AMPHIBIAN          (aa)
#define FORM_FISH               (bb)
#define FORM_COLD_BLOOD    (cc)

/* body parts */
#define PART_HEAD               (A)
#define PART_ARMS               (B)
#define PART_LEGS               (C)
#define PART_HEART              (D)
#define PART_BRAINS             (E)
#define PART_GUTS               (F)
#define PART_HANDS              (G)
#define PART_FEET               (H)
#define PART_FINGERS            (I)
#define PART_EAR                (J)
#define PART_EYE     (K)
#define PART_LONG_TONGUE        (L)
#define PART_EYESTALKS          (M)
#define PART_TENTACLES          (N)
#define PART_FINS               (O)
#define PART_WINGS              (P)
#define PART_TAIL               (Q)
/* for combat */
#define PART_CLAWS              (U)
#define PART_FANGS              (V)
#define PART_HORNS              (W)
#define PART_SCALES             (X)
#define PART_TUSKS      (Y)


/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND          (A)
#define AFF_INVISIBLE      (B)
#define AFF_MINIMATION     (C)
#define AFF_DETECT_INVIS   (D)
#define AFF_DETECT_MAGIC   (E)
#define AFF_DETECT_HIDDEN  (F)
#define AFF_WATER_BREATHING (G)
#define AFF_SANCTUARY      (H)
#define AFF_FAERIE_FIRE    (I)
#define AFF_INFRARED       (J)
#define AFF_CURSE          (K)
#define AFF_GRANDEUR       (L)
#define AFF_POISON         (M)
#define AFF_PROTECT_EVIL   (N)
#define AFF_PROTECT_GOOD   (O)
#define AFF_SNEAK          (P)
#define AFF_HIDE           (Q)
#define AFF_SLEEP          (R)
#define AFF_CHARM          (S)
#define AFF_FLYING         (T)
#define AFF_PROTECT_NEUTRAL (U)
#define AFF_HASTE          (V)
#define AFF_CALM           (W)
#define AFF_PLAGUE         (X)
#define AFF_WATERWALK      (Y)
#define AFF_DARK_VISION    (Z)
#define AFF_BERSERK        (aa)
#define AFF_TAUNT          (bb)
#define AFF_DARKLIGHT      (cc)
#define AFF_SLOW           (dd)
#define AFF_DAYLIGHT       (ee)
#define AFF_MARTIAL_ARTS   (ff)

/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL           0
#define SEX_MALE           1
#define SEX_FEMALE            2

/* AC types */
#define AC_PIERCE       0
#define AC_BASH            1
#define AC_SLASH        2
#define AC_EXOTIC       3

/* dice */
#define DICE_NUMBER        0
#define DICE_TYPE       1
#define DICE_BONUS         2

/* size */
#define SIZE_TINY       0
#define SIZE_SMALL         1
#define SIZE_MEDIUM        2
#define SIZE_LARGE         3
#define SIZE_HUGE       4
#define SIZE_GIANT         5



/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_SILVER_ONE         1
#define OBJ_VNUM_GOLD_ONE        2
#define OBJ_VNUM_GOLD_SOME       3
#define OBJ_VNUM_SILVER_SOME        4
#define OBJ_VNUM_COINS           5

#define OBJ_VNUM_CORPSE_NPC        10
#define OBJ_VNUM_CORPSE_PC      11
#define OBJ_VNUM_SEVERED_HEAD      12
#define OBJ_VNUM_TORN_HEART        13
#define OBJ_VNUM_SLICED_ARM        14
#define OBJ_VNUM_SLICED_LEG        15
#define OBJ_VNUM_GUTS           16
#define OBJ_VNUM_BRAINS         17
#define OBJ_VNUM_TAIL		18
#define OBJ_VNUM_WING		19

#define OBJ_VNUM_MUSHROOM       20
#define OBJ_VNUM_LIGHT_BALL        21
#define OBJ_VNUM_SPRING         22
#define OBJ_VNUM_DISC           23
#define OBJ_VNUM_PORTAL         25

#define OBJ_VNUM_ROSE         1001

#define OBJ_VNUM_PIT       3010

#define OBJ_VNUM_SCHOOL_MACE     3700
#define OBJ_VNUM_SCHOOL_DAGGER      3701
#define OBJ_VNUM_SCHOOL_SWORD    3702
#define OBJ_VNUM_SCHOOL_SPEAR    3717
#define OBJ_VNUM_SCHOOL_STAFF    3718
#define OBJ_VNUM_SCHOOL_AXE      3719
#define OBJ_VNUM_SCHOOL_FLAIL    3720
#define OBJ_VNUM_SCHOOL_WHIP     3721
#define OBJ_VNUM_SCHOOL_POLEARM    3722
#define OBJ_VNUM_SCHOOL_KNUCKLES  3728
#define OBJ_VNUM_SCHOOL_KATANA    3729

#define OBJ_VNUM_MAGICAL_MACE     10016
#define OBJ_VNUM_MAGICAL_DAGGER   10017
#define OBJ_VNUM_MAGICAL_SWORD    10018
#define OBJ_VNUM_MAGICAL_SPEAR    10019
#define OBJ_VNUM_MAGICAL_STAFF    10020
#define OBJ_VNUM_MAGICAL_AXE      10021
#define OBJ_VNUM_MAGICAL_FLAIL    10022
#define OBJ_VNUM_MAGICAL_WHIP     10023
#define OBJ_VNUM_MAGICAL_POLEARM  10024
#define OBJ_VNUM_MAGICAL_KNUCKLES 10028


#define OBJ_VNUM_SCHOOL_VEST     3703
#define OBJ_VNUM_SCHOOL_SHIELD      3704
#define OBJ_VNUM_SCHOOL_BANNER     3716
#define OBJ_VNUM_MAP       3162

#define OBJ_VNUM_WHISTLE      2116
#define OBJ_VNUM_BANK_NOTE         6521




/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT            1
#define ITEM_SCROLL           2
#define ITEM_WAND          3
#define ITEM_STAFF            4
#define ITEM_WEAPON           5
#define ITEM_TREASURE            8
#define ITEM_ARMOR            9
#define ITEM_POTION          10
#define ITEM_CLOTHING           11
#define ITEM_FURNITURE          12
#define ITEM_TRASH           13
#define ITEM_CONTAINER          15
#define ITEM_DRINK_CON          17
#define ITEM_KEY          18
#define ITEM_FOOD         19
#define ITEM_MONEY           20
#define ITEM_BOAT         22
#define ITEM_CORPSE_NPC         23
#define ITEM_CORPSE_PC          24
#define ITEM_FOUNTAIN           25
#define ITEM_PILL         26
#define ITEM_PROTECT         27
#define ITEM_MAP          28
#define ITEM_PORTAL          29
#define ITEM_WARP_STONE         30
#define ITEM_ROOM_KEY           31
#define ITEM_GEM          32
#define ITEM_JEWELRY         33
#define ITEM_JUKEBOX         34
#define ITEM_THROWING             35
#define ITEM_RECALL          36
#define ITEM_SLOT_MACHINE    37
#define ITEM_ORB             38
#define ITEM_SHEATH	     39
#define ITEM_CHARM           40
#define ITEM_AMMO            41

#define SHEATH_NONE          0
#define SHEATH_FLAG	     1
#define SHEATH_DICETYPE	     2
#define SHEATH_DICECOUNT     3
#define SHEATH_HITROLL       4
#define SHEATH_DAMROLL	     5
#define SHEATH_QUICKDRAW     6
#define SHEATH_SPELL	     7

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW    (A)
#define ITEM_HUM     (B)
#define ITEM_DARK    (C)
#define ITEM_LOCK    (D)
#define ITEM_EVIL    (E)
#define ITEM_INVIS      (F)
#define ITEM_MAGIC      (G)
#define ITEM_NODROP     (H)
#define ITEM_BLESS      (I)
#define ITEM_ANTI_GOOD     (J)
#define ITEM_ANTI_EVIL     (K)
#define ITEM_ANTI_NEUTRAL  (L)
#define ITEM_NOREMOVE      (M)
#define ITEM_INVENTORY     (N)
#define ITEM_NOPURGE    (O)
#define ITEM_ROT_DEATH     (P)
#define ITEM_VIS_DEATH     (Q)
#define ITEM_NONMETAL      (S)
#define ITEM_NOLOCATE      (T)
#define ITEM_MELT_DROP     (U)
#define ITEM_HAD_TIMER     (V)
#define ITEM_SELL_EXTRACT  (W)
#define ITEM_BURN_PROOF    (Y)
#define ITEM_NOUNCURSE     (Z)
#define ITEM_WARRIOR       (aa)
#define ITEM_MAGE          (bb)
#define ITEM_THIEF         (cc)
#define ITEM_CLERIC        (dd)
#define ITEM_SOUND_PROOF   (ee)

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE    (A)
#define ITEM_WEAR_FINGER   (B)
#define ITEM_WEAR_NECK     (C)
#define ITEM_WEAR_BODY     (D)
#define ITEM_WEAR_HEAD     (E)
#define ITEM_WEAR_LEGS     (F)
#define ITEM_WEAR_FEET     (G)
#define ITEM_WEAR_HANDS    (H)
#define ITEM_WEAR_ARMS     (I)
#define ITEM_WEAR_SHIELD   (J)
#define ITEM_WEAR_ABOUT    (K)
#define ITEM_WEAR_WAIST    (L)
#define ITEM_WEAR_WRIST    (M)
#define ITEM_WIELD      (N)
#define ITEM_HOLD    (O)
#define ITEM_NO_SAC     (P)
#define ITEM_WEAR_FLOAT    (Q)
#define ITEM_CHARGED       (R)
#define ITEM_NORECALL	   (S)
#define ITEM_NOGATE	   (T)
#define ITEM_INCOMPLETE (Y)
#define ITEM_CRAFTED (Z)
#define ITEM_FLAG_EMBELISHED (aa)
#define ITEM_FLAG_REPLICATED (bb)
#define ITEM_NEWBIE (cc)


/* weapon class */
#define WEAPON_EXOTIC      0
#define WEAPON_SWORD    1
#define WEAPON_DAGGER      2
#define WEAPON_SPEAR    3
#define WEAPON_MACE     4
#define WEAPON_AXE      5
#define WEAPON_FLAIL    6
#define WEAPON_WHIP     7
#define WEAPON_POLEARM     8
#define WEAPON_KATANA   9
#define WEAPON_RANGED   10

/* weapon types */
#define WEAPON_FLAMING     (A)
#define WEAPON_FROST    (B)
#define WEAPON_VAMPIRIC    (C)
#define WEAPON_SHARP    (D)
#define WEAPON_VORPAL      (E)
#define WEAPON_TWO_HANDS   (F)
#define WEAPON_SHOCKING    (G)
#define WEAPON_POISON      (H)
#define WEAPON_DRAGON_SLAYER (I)
#define WEAPON_DULL        (J)
#define WEAPON_BLUNT       (K)
#define WEAPON_CORROSIVE   (L)
#define WEAPON_FLOODING     (M)
#define WEAPON_INFECTED    (N)
#define WEAPON_NOMUTATE    (O)
#define WEAPON_SOULDRAIN   (P)
#define WEAPON_HOLY	   (Q)
#define WEAPON_UNHOLY      (R)
#define WEAPON_POLAR       (S)
#define WEAPON_PHASE	   (T)
#define WEAPON_ANTIMAGIC   (U)
#define WEAPON_ENTROPIC    (V)
#define WEAPON_PSIONIC     (W)
#define WEAPON_DEMONIC	   (X)
#define WEAPON_INTELLIGENT (Y)

/* gate flags */
#define GATE_NORMAL_EXIT   (A)
#define GATE_NOCURSE    (B)
#define GATE_GOWITH     (C)
#define GATE_BUGGY      (D)
#define GATE_RANDOM     (E)

/* furniture flags */
#define STAND_AT     (A)
#define STAND_ON     (B)
#define STAND_IN     (C)
#define SIT_AT       (D)
#define SIT_ON       (E)
#define SIT_IN       (F)
#define REST_AT         (G)
#define REST_ON         (H)
#define REST_IN         (I)
#define SLEEP_AT     (J)
#define SLEEP_ON     (K)
#define SLEEP_IN     (L)
#define PUT_AT       (M)
#define PUT_ON       (N)
#define PUT_IN       (O)
#define PUT_INSIDE      (P)




/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE            0
#define APPLY_STR             1
#define APPLY_DEX             2
#define APPLY_INT             3
#define APPLY_WIS             4
#define APPLY_CON             5
#define APPLY_SEX             6
#define APPLY_LEVEL           8
#define APPLY_AGE             9
#define APPLY_MANA           12
#define APPLY_HIT            13
#define APPLY_MOVE           14
#define APPLY_AC             17
#define APPLY_HITROLL        18
#define APPLY_DAMROLL        19
#define APPLY_SAVES          20
#define APPLY_SAVING_PARA    20
#define APPLY_SAVING_ROD     21
#define APPLY_SAVING_PETRI   22
#define APPLY_SAVING_BREATH  23
#define APPLY_SAVING_SPELL   24
#define APPLY_SPELL_AFFECT   25
#define APPLY_DAMAGE_REDUCE  26
#define APPLY_SPELL_DAMAGE   27
#define APPLY_MAX_STR	     28
#define APPLY_MAX_DEX        29
#define APPLY_MAX_CON        30
#define APPLY_MAX_INT        31
#define APPLY_MAX_WIS        32
#define APPLY_ATTACK_SPEED   33

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE           1
#define CONT_PICKPROOF           2
#define CONT_CLOSED           4
#define CONT_LOCKED           8
#define CONT_PUT_ON          16



/* err handy const */
#define CONTINENT_TERRA		0
#define CONTINENT_DOMINIA	1

/* game constants */
#define CTF_BLUE		1
#define CTF_RED			2

/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO          2
#define ROOM_VNUM_LIMBO_DOMINIA 31189
#define ROOM_VNUM_CHAT        1200
#define ROOM_VNUM_TEMPLE      3001
#define ROOM_VNUM_DOMINIA     31000
#define ROOM_VNUM_ALTAR_DOM   31001
#define ROOM_VNUM_SHORE_TERRA 254
#define ROOM_VNUM_SHORE_DOMINIA 1900
#define ROOM_VNUM_PORTAL_DOMINIA  30812
#define ROOM_VNUM_PORTAL_TERRA 3388
#define ROOM_VNUM_ALTAR       3054
#define ROOM_VNUM_SCHOOL      17179
#define ROOM_VNUM_SCHOOL_DOMINIA 20200
#define ROOM_VNUM_BALANCE     4500
#define ROOM_VNUM_CIRCLE      4400
#define ROOM_VNUM_DEMISE      4201
#define ROOM_VNUM_HONOR       4300
#define ROOM_VNUM_SEEKERS     10804
#define ROOM_VNUM_KINDRED     10604
#define ROOM_VNUM_JUSTICE     10754
#define ROOM_VNUM_VENARI      23102
#define ROOM_VNUM_KENSHI      23401
#define ROOM_VNUM_SEEKERS_DOMINIA     23302
#define ROOM_VNUM_KINDRED_DOMINIA     23002
#define ROOM_VNUM_JUSTICE_DOMINIA     23202
#define ROOM_VNUM_VENARI_DOMINIA    10654
#define ROOM_VNUM_KENSHI_DOMINIA     1253

/* rooms used for CTF */
#define ROOM_CTF_JAIL(team) ( team == 1? 2646 : 2613)
#define ROOM_CTF_JAILER(team) ( team == 1? 2645 : 2612)
#define ROOM_CTF_FLAG(team) ( team == 1 ? 2651 : 2601)
#define ROOM_CTF_PREP(team) ( team == 1 ? 2666 : 2665)

#define OBJ_CTF_FLAG(team) (team == 1 ? 2620 : 2619)
#define CTF_SCORE(team) (team == 1 ? ctf_score_blue : ctf_score_red )
#define CTF_OTHER_TEAM(team) (team == CTF_BLUE ? CTF_RED : CTF_BLUE )
#define MOB_CTF_JAILER(team) ( team == 1 ? 2604 : 2603 )

/* room affect flags */
#define ROOM_AFF_TEST (A)
#define ROOM_AFF_FIRE (B)
#define ROOM_AFF_PENT_GOOD        (C)
#define ROOM_AFF_PENT_BAD        (D)
#define ROOM_AFF_ORACLE         (E)
#define ROOM_AFF_WEB            (F)
#define ROOM_AFF_RAIN           (G)
#define ROOM_AFF_HAIL           (H)
#define ROOM_AFF_LAIR           (I)
#define ROOM_AFF_DARKNESS       (J)
#define ROOM_AFF_CLOUDKILL_CLAN (K)
#define ROOM_AFF_CLOUDKILL_NC   (L)
#define ROOM_AFF_SHACKLE_RUNE   (M)
#define ROOM_AFF_FIRE_RUNE      (N)
#define ROOM_AFF_ALARM_RUNE     (O)

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK    (A)
#define ROOM_NO_MOB     (C)
#define ROOM_INDOORS    (D)
#define ROOM_ARENA      (E)
#define ROOM_FERRY      (F)
#define ROOM_NO_KILL     (I)
#define ROOM_PRIVATE    (J)
#define ROOM_SAFE    (K)
#define ROOM_SOLITARY      (L)
#define ROOM_PET_SHOP      (M)
#define ROOM_NO_RECALL     (N)
#define ROOM_IMP_ONLY      (O)
#define ROOM_GODS_ONLY     (P)
#define ROOM_HEROES_ONLY   (Q)
#define ROOM_NEWBIES_ONLY  (R)
#define ROOM_LAW     (S)
#define ROOM_NOWHERE    (T)
#define ROOM_NOQUIT     (U)
#define ROOM_CLAN                (bb)
#define ROOM_NO_GATE                (cc)
#define ROOM_NO_LAIR          (dd)
#define ROOM_WORKSHOP	   (ee)

 /* Directions.
    * Used in #ROOMS.
  */
#define DIR_NORTH          0
#define DIR_EAST           1
#define DIR_SOUTH          2
#define DIR_WEST           3
#define DIR_UP             4
#define DIR_DOWN           5



/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR          (A)
#define EX_CLOSED          (B)
#define EX_LOCKED          (C)
#define EX_PICKPROOF          (F)
#define EX_NOPASS          (G)
#define EX_EASY               (H)
#define EX_HARD               (I)
#define EX_INFURIATING           (J)
#define EX_NOCLOSE            (K)
#define EX_NOLOCK          (L)



/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE           0
#define SECT_CITY             1
#define SECT_FIELD            2
#define SECT_FOREST           3
#define SECT_HILLS            4
#define SECT_MOUNTAIN         5
#define SECT_WATER_SWIM       6
#define SECT_WATER_NOSWIM     7
#define SECT_UNUSED           8
#define SECT_AIR              9
#define SECT_DESERT          10
#define SECT_SWAMP           11
#define SECT_UNDERWATER      12

#define SECT_MAX             13


/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE         -1
#define WEAR_LIGHT            0
#define WEAR_FINGER_L            1
#define WEAR_FINGER_R            2
#define WEAR_NECK_1           3
#define WEAR_NECK_2           4
#define WEAR_BODY          5
#define WEAR_HEAD          6
#define WEAR_LEGS          7
#define WEAR_FEET          8
#define WEAR_HANDS            9
#define WEAR_ARMS         10
#define WEAR_SHIELD          11
#define WEAR_ABOUT           12
#define WEAR_WAIST           13
#define WEAR_WRIST_L         14
#define WEAR_WRIST_R         15
#define WEAR_WIELD           16
#define WEAR_HOLD         17
#define WEAR_FLOAT           18
#define WEAR_FLOAT_2      19
#define WEAR_HEAD_2       20
#define WEAR_DUAL          21
#define WEAR_RANGED        22
#define WEAR_AMMO          23
#define MAX_WEAR           24



/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
#define COND_DRUNK            0
#define COND_FULL          1
#define COND_THIRST           2
#define COND_HUNGER           3



/*
 * Positions.
 */
#define POS_DEAD              0
#define POS_MORTAL            1
#define POS_INCAP             2
#define POS_STUNNED           3
#define POS_SLEEPING          4
#define POS_RESTING           5
#define POS_SITTING           6
#define POS_FIGHTING          7
#define POS_STANDING          8



/*
 * ACT bits for players.
 */
#define PLR_IS_NPC      (A)		/* Don't EVER set.   */

/* RT auto flags */
#define PLR_MOBASSIST		(B)
#define PLR_PLRASSIST		(C)
#define PLR_AUTOEXIT    (D)
#define PLR_AUTOLOOT    (E)
#define PLR_AUTOSAC             (F)
#define PLR_AUTOGOLD    (G)
#define PLR_AUTOSPLIT      (H)
#define PLR_AUTOTRASH   (I)

/* RT personal flags */
#define PLR_NOTIFY	(M)
#define PLR_HOLYLIGHT   (N)
#define PLR_AUTOTITLE	(O)
#define PLR_CANLOOT     (P)
#define PLR_NOSUMMON    (Q)
#define PLR_NOFOLLOW    (R)
/* 2 bits reserved, S-T */

/* penalty flags */
#define PLR_PERMIT      (U)
#define PLR_LOG         (W)
#define PLR_DENY     (X)
#define PLR_FREEZE      (Y)
#define PLR_THIEF    (Z)
#define PLR_KILLER      (aa)

#define PLR_QUESTING  (bb)
#define PLR_NOCAN     (cc)
#define PLR_NORECALL  (dd)
#define PLR_COLOUR    (ee)


/* Toggles flags for players - New flags since all is full */

#define TOGGLES_LINKDEAD         (A)
#define TOGGLES_BLOCKING         (B)
#define TOGGLES_AUTOVALUE        (C)
#define TOGGLES_SOUND            (D)
#define TOGGLES_NOTITLE          (E)
#define TOGGLES_NOEXP            (F)
#define TOGGLES_NOBOUNTY         (G)
#define TOGGLES_PLEDGE           (H)
#define TOGGLES_PENDING_OFFER    (I)
#define TOGGLES_ACCEPT_OFFER     (J)

/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET         (A)
#define COMM_DEAF          (B)
#define COMM_NOWIZ         (C)
#define COMM_NONEWBIE      (D)
#define COMM_NOGOSSIP      (E)
#define COMM_NOQUESTION    (F)
#define COMM_NOMUSIC       (G)
#define COMM_NOCLAN        (H)
#define COMM_NOAUCTION     (I)
#define COMM_SHOUTSOFF     (J)
#define COMM_LEAD          (K)

/* display flags */
#define COMM_COMPACT       (L)
#define COMM_BRIEF         (M)
#define COMM_PROMPT        (N)
#define COMM_COMBINE       (O)
#define COMM_TELNET_GA     (P)
#define COMM_SHOW_AFFECTS  (Q)
#define COMM_NOGRATS       (R)

/* penalties */
#define COMM_NOEMOTE       (T)
#define COMM_NOSHOUT       (U)
#define COMM_NOTELL        (V)
#define COMM_NOCHANNELS    (W)
#define COMM_SNOOP_PROOF   (Y)
#define COMM_AFK           (Z)
#define COMM_LAG           (aa)
#define COMM_NOBEEP        (bb)
#define COMM_NONOTE        (cc)
#define COMM_NOCGOSS       (dd)
#define COMM_NOOOC         (ee)
#define COMM_NOTE_WRITE    (ff)


/* WIZnet flags */
#define WIZ_ON       (A)
#define WIZ_TICKS    (B)
#define WIZ_LOGINS      (C)
#define WIZ_SITES    (D)
#define WIZ_LINKS    (E)
#define WIZ_DEATHS      (F)
#define WIZ_RESETS      (G)
#define WIZ_MOBDEATHS      (H)
#define WIZ_FLAGS    (I)
#define WIZ_PENALTIES      (J)
#define WIZ_SACCING     (K)
#define WIZ_LEVELS      (L)
#define WIZ_SECURE      (M)
#define WIZ_SWITCHES    (N)
#define WIZ_SNOOPS      (O)
#define WIZ_RESTORE     (P)
#define WIZ_LOAD     (Q)
#define WIZ_NEWBIE      (R)
#define WIZ_PREFIX      (S)
#define WIZ_SPAM     (T)
#define CAN_CLAN     (U)
#define JOIN_A    (V)
#define JOIN_B    (W)
#define JOIN_C    (X)
#define JOIN_D    (Y)
#define JOIN_E    (K)
#define JOIN_F 	  (O)
#define JOIN_G    (P)
#define JOIN_H    (Q)
#define JOIN_I    (R)

#define NO_QUIT   (aa)
#define WIZ_OLC   (bb)
#define WIZ_DISP  (cc)
#define WIZ_TIMES (dd)
#define WIZ_DAMAGE (ee)
#define WIZ_NOTES (ff)

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct mob_index_data
{
	MOB_INDEX_DATA *next;
	SPEC_FUN *spec_fun;
	SHOP_DATA *pShop;
	MPROG_LIST *mprogs;
	AREA_DATA *area;			/* OLC */
	int vnum;
	int group;
	bool new_format;
	int count;
	int killed;
	char *player_name;
	char *short_descr;
	char *long_descr;
	char *description;
	long act;
	long affected_by;
	int alignment;
	int level;
	int hitroll;
	int hit[3];
	int mana[3];
	int damage[3];
	int ac[4];
	int dam_type;
	long off_flags;
	long imm_flags;
	long res_flags;
	long vuln_flags;
	int start_pos;
	int default_pos;
	int sex;
	int race;
	long wealth;
	long form;
	long parts;
	int size;
	char *material;
	long mprog_flags;
	int clan;
};



/* memory settings */
#define MEM_CUSTOMER A
#define MEM_SELLER   B
#define MEM_HOSTILE  C
#define MEM_AFRAID   D

/* memory for mobs */
struct mem_data
{
	MEM_DATA *next;
	bool valid;
	int id;
	int reaction;
	time_t when;
};

/*
 * One character (PC or NPC).
 */
struct char_data
{
	CHAR_DATA *next;
	CHAR_DATA *next_in_room;
	CHAR_DATA *master;
	CHAR_DATA *leader;
	CHAR_DATA *fighting;
	CHAR_DATA *reply;
	CHAR_DATA *retell;
	CHAR_DATA *mprog_target;
	CHAR_DATA *pet;
	MEM_DATA *memory;
	SPEC_FUN *spec_fun;
	MOB_INDEX_DATA *pIndexData;
	DESCRIPTOR_DATA *desc;
	AFFECT_DATA *affected;
	OBJ_DATA *carrying;
	OBJ_DATA *on;
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *note_room;
	ROOM_INDEX_DATA *was_in_room;
	AREA_DATA *zone;
	PC_DATA *pcdata;
	GEN_DATA *gen_data;
	NOTE_DATA *pnote;
	int last_in_room_vnum;
	bool valid;
	char *name;
	long id;
	int version;
	char *short_descr;
	char *long_descr;
	char *description;
	char *shift_name;
	char *shift_short;
	char *shift_long;
	char *site;
	char *prompt;
	char *rreply;
	char *prefix;
	char *rptitle;
	int group;
	int clan;
	int outcast_timer;
	int sex;
	int charClass;
	int race;
	int level;
	int trust;
	int gift;
	int delegate;
	int ferry_timer;
	int remort;
	int rem_sub;
	int reclass;
	int pkills;
	int deaths;
	int pktimer;
	int bounty;
	int deity_type;
	int seen;
	int deity_points;
	int can_lay;
	int breath;
	int played;
	int played_perm;
	int lines;					/* for the pager */
	time_t logon;
	int timer;
	int wait;
	int daze;
	int hit;
	int max_hit;
	int mana;
	int max_mana;
	int move;
	int max_move;
	long gold;
	long silver;
	int exp;
	bitset act;
	bitset comm;					/* RT added to pad the vector */
	bitset wiznet;				/* wiz stuff */
	bitset imm_flags;
	bitset res_flags;
	bitset vuln_flags;
	long toggles;
	int invis_level;
	int incog_level;
	bitset affected_by;
	int position;
	int practice;
	int train;
	int carry_weight;
	int carry_number;
	int saving_throw;
	int alignment;
	int hitroll;
	int damroll;
	int armor[4];
	int wimpy;
	/* stats */
	int perm_stat[MAX_STATS];
	int mod_stat[MAX_STATS];
	/* parts stuff */
	bitset form;
	bitset parts;
	int size;
	char *material;
	/* mobile stuff */
	bool is_clone;
	long off_flags;
	int damage[3];
	int dam_type;
	int start_pos;
	int default_pos;
	int mprog_delay;
	int no_quit_timer;

	int questpoints;			/* Vassago */
	int hunt_timer;
	CHAR_DATA *hunting;
	int sliver;
	int rot_timer;              /* for temp mobs like undead */
	bool on_who;
	int last_sn[2];
	int meditate_needed;
	int thrustCounter;
	int dbiteCounter;
        int sbashCounter;
	int burstCounter;
	char *clan_rank;
	int damage_reduce;
	int spell_damroll;
	int max_stat_bonus[MAX_STATS];
	int air_supply;
	char *lured_by;
	int real_level; /* Needed for charged item focus */
	int craft_timer;
	int craft_target;
	char *death_blow_target;
	char *patron;
	char *vassal;
	int pass_along;
	int pass_along_limit;
	int to_pass;
	int attack_speed;
	OBJ_DATA *disarmed;
	int charge_wait;
	int max_hit_bonus;
	int max_mana_bonus;
	int max_move_bonus;
	int lastlogoff;
	int wander_timer; // how many ticks summoned mobs wait before wandering
};

/*
 * Data for questing
 */
#define QUEST_TYPE_NONE (0)
#define QUEST_TYPE_ITEM (1)
#define QUEST_TYPE_VILLAIN (2)
#define QUEST_TYPE_ASSAULT (3)
#define QUEST_TYPE_RESCUE (4)
#define QUEST_TYPE_ESCORT (5)

struct quest_data {
    CHAR_DATA *giver;
    int timer;
    int type;
    int target;
    int progress;
    ROOM_INDEX_DATA *destination;
};

/*
 * Data which only PC's have.
 */
struct pc_data
{
	PC_DATA *next;
	BUFFER *buffer;
	bool valid;
	char *pwd;
	char *bamfin;
	char *bamfout;
	char *title;
	BOARD_DATA *board;
	NOTE_READ_DATA *noteReadData;
	NOTE_DATA *in_progress;
	char* spouse;
	int true_sex;
	int last_level;
	int specialty;
	int condition[4];
	int learned[MAX_SKILL];
	bool group_known[MAX_GROUP];
	int points;
	bool confirm_delete;
	char *alias[MAX_ALIAS];
	char *alias_sub[MAX_ALIAS];
	int security;				/* OLC *//* Builder security */

	char *hush[5];
	int olc_depth;
	int min_vnum;
	int max_vnum;
	bool double_door;
	bool brief;
	int cur_area;
	int cur_room;
	int cur_mob;
	int cur_obj;
	bool auto_walk;
	bool room_def;
	bool obj_def;
	bool mob_def;
	MACRO_DATA macro[MAX_NUM_MACROS];
	INSTANCE_DATA *macroInstances;
	int macroTimer;
	int sliver;
	int confirm_fixcp;
	int any_skill;
	bool ctf_flag;
	int ctf_team;
	struct quest_data quest;
};

/* Data for generating characters -- only used during generation */
struct gen_data
{
	GEN_DATA *next;
	bool valid;
	bool skill_chosen[MAX_SKILL];
	bool group_chosen[MAX_GROUP];
	int points_chosen;
};

extern CHAR_DATA *System;


/*
 * Liquids.
 */
#define LIQ_WATER        0

struct liq_type
{
	char *liq_name;
	char *liq_color;
	int liq_affect[5];
};



/*
 * Extra description data for a room or object.
 */
struct extra_descr_data
{
	EXTRA_DESCR_DATA *next;		/* Next in list                     */
	bool valid;
	char *keyword;				/* Keyword in look/examine          */
	char *description;			/* What to see                      */
};



/*
 * Prototype for an object.
 */
struct obj_index_data
{
	OBJ_INDEX_DATA *next;
	EXTRA_DESCR_DATA *extra_descr;
	AFFECT_DATA *affected;
	AREA_DATA *area;			/* OLC */
	bool new_format;
	char *name;
	char *short_descr;
	char *description;
	int vnum;
	int reset_num;
	char *material;
	int item_type;
	int extra_flags;
	int wear_flags;
	int level;
	int condition;
	int count;
	int weight;
	int cost;
	int value[5];
	int special[5];
	int extra[5];
};



/*
 * One object.
 */
struct obj_data
{
	OBJ_DATA *next;
	OBJ_DATA *next_content;
	OBJ_DATA *contains;
	OBJ_DATA *in_obj;
	OBJ_DATA *on;
	CHAR_DATA *carried_by;
	EXTRA_DESCR_DATA *extra_descr;
	AFFECT_DATA *affected;
	OBJ_INDEX_DATA *pIndexData;
	ROOM_INDEX_DATA *in_room;
	bool valid;
	bool enchanted;
	char *owner;
	int owner_vnum;
	char *name;
	char *short_descr;
	char *description;
	char *seek;
	int item_type;
	int extra_flags;
	int wear_flags;
	int wear_loc;
	int weight;
	int cost;
	int level;
	int condition;
	char *material;
	int timer;
	int value[5];
	int special[5];
	int extra[5];
	int clan_status;
	char *respawn_owner;
};



/*
 * Exit data.
 */
struct exit_data
{
	union
	{
		ROOM_INDEX_DATA *to_room;
		int vnum;
	}
	u1;
	int exit_info;
	int key;
	char *keyword;
	char *description;
	char *blocking;
	EXIT_DATA *next;			/* OLC */
	int rs_flags;				/* OLC */
	int orig_door;				/* OLC */

};



/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

struct quest_item
{
        int vnum;
        int continent;
        char *keyword;
        char *desc;
        int requiredQP;
};

typedef struct quest_item QUEST_ITEM;

extern QUEST_ITEM itemList[];

/*
 * Area-reset definition.
 */
struct reset_data
{
	RESET_DATA *next;
	char command;
	int arg1;
	int arg2;
	int arg3;
	int arg4;
	int percent;
};



/*
 * Area definition.
 */
struct area_data
{
	AREA_DATA *next;
	RESET_DATA *reset_first;
	RESET_DATA *reset_last;
	char *file_name;
	char *name;
	char *credits;
	int age;
	int age_count;
	int nplayer;
	int low_range;
	int high_range;
	int min_vnum;
	int max_vnum;
	int continent;
	bool empty;
	char *builders;             /* OLC *//* Listing of */
	int vnum;                   /* OLC *//* Area vnum  */
	int area_flags;             /* OLC */
	int security;               /* OLC *//* Value 1-9  */
	bool questable;             /* Whether the questmaster can use this area when assigning quests */

};



/*
 * Room type.
 */
struct room_index_data
{
	ROOM_INDEX_DATA *next;
	CHAR_DATA *people;
	OBJ_DATA *contents;
	EXTRA_DESCR_DATA *extra_descr;
	AREA_DATA *area;
	EXIT_DATA *exit[6];
	EXIT_DATA *old_exit[6];
	RESET_DATA *reset_first;	/* OLC */
	RESET_DATA *reset_last;		/* OLC */
	AFFECT_DATA *affected;
	char *name;
	char *description;
	char *owner;
	char *sound;
	int vnum;
	int room_flags;
	long affected_by;
	int light;
	int sector_type;
	int heal_rate;
	int mana_rate;
	int clan;
};



/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED               -1
#define TYPE_HIT                      1
#define TYPE_MAGIC                    2
#define TYPE_SKILL                    4
#define TYPE_SPECIALIZED              8
#define TYPE_ANONYMOUS               16
#define TYPE_SLIVER_THRUST           32

/*
 *  Target types.
 */
#define TAR_IGNORE             0
#define TAR_CHAR_OFFENSIVE     1
#define TAR_CHAR_DEFENSIVE     2
#define TAR_CHAR_SELF          3
#define TAR_OBJ_INV            4
#define TAR_OBJ_CHAR_DEF       5
#define TAR_OBJ_CHAR_OFF       6
#define TAR_OBJ_CARRYING       7

#define TARGET_CHAR         0
#define TARGET_OBJ          1
#define TARGET_ROOM         2
#define TARGET_NONE         3



/*
 * Skills include spells as a particular case.
 */
struct skill_type
{
	char *name;					/* Name of skill                                    */
	int remort;				/* Remort Level added by delstar                    */
	int skill_level[MAX_CLASS];	/* Level needed by class                            */
	int rating[MAX_CLASS];	/* How hard it is to learn                          */
	SPELL_FUN *spell_fun;		/* Spell pointer (for spells)                       */
	END_FUN *end_fun;			/* function to run after the spell/skill wears off. */
	int target;				/* Legal targets                                    */
	int minimum_position;	/* Position for caster / user                       */
	int *pgsn;				/* Pointer to associated gsn                        */
	int slot;				/* Slot for #OBJECT loading                         */
	int min_mana;			/* Minimum mana used                                */
	int beats;				/* Waiting time after use                           */
	char *noun_damage;			/* Damage message                                   */
	char *msg_off;				/* Wear off message                                 */
	char *msg_obj;				/* Wear off message for obects                      */
};

struct group_type
{
	char *name;
	int rating[MAX_CLASS];
	char *spells[MAX_IN_GROUP];
};

/*
 * MOBprog definitions
 */
#define TRIG_ACT        (A)
#define TRIG_BRIBE        (B)
#define TRIG_DEATH        (C)
#define TRIG_ENTRY        (D)
#define TRIG_FIGHT        (E)
#define TRIG_GIVE         (F)
#define TRIG_GREET        (G)
#define TRIG_GRALL        (H)
#define TRIG_KILL         (I)
#define TRIG_HPCNT        (J)
#define TRIG_RANDOM       (K)
#define TRIG_SPEECH       (L)
#define TRIG_EXIT         (M)
#define TRIG_EXALL        (N)
#define TRIG_DELAY        (O)
#define TRIG_SURR         (P)


/*
 * These are skill_lookup return values for common skills and spells.
 */
extern int gsn_reserved;
extern int gsn_death_blow;
extern int gsn_aura;
extern int gsn_aurora;
extern int gsn_ambush;
extern int gsn_atheism;
extern int gsn_attraction;
extern int gsn_abandon;
extern int gsn_acid_blast;
extern int gsn_animal_growth;
extern int gsn_animate_tree;
extern int gsn_animate_dead;
extern int gsn_armor;
extern int gsn_bark_skin;
extern int gsn_beacon;
extern int gsn_blast_of_rot;
extern int gsn_bless;
extern int gsn_blindness;
extern int gsn_blur;
extern int gsn_burning_hands;
extern int gsn_call_creature;
extern int gsn_call_lightning;
extern int gsn_calm;
extern int gsn_cancellation;
extern int gsn_carnal_reach;
extern int gsn_cause_critical;
extern int gsn_cause_light;
extern int gsn_cause_serious;
extern int gsn_chain_lightning;
extern int gsn_change_sex;
extern int gsn_channel_energy;
extern int gsn_charm_person;
extern int gsn_chill_touch;
extern int gsn_clenched_fist;
extern int gsn_cloak_of_mind;
extern int gsn_cloudkill;
extern int gsn_colour_spray;
extern int gsn_concealment;
extern int gsn_condensation;
extern int gsn_continual_light;
extern int gsn_control_weather;
extern int gsn_create_door;
extern int gsn_create_food;
extern int gsn_create_oil;
extern int gsn_create_rose;
extern int gsn_create_spring;
extern int gsn_create_water;
extern int gsn_creeping_doom;
extern int gsn_crushing_hand;
extern int gsn_cure_blindness;
extern int gsn_cure_critical;
extern int gsn_cure_disease;
extern int gsn_cure_light;
extern int gsn_cure_poison;
extern int gsn_cure_serious;
extern int gsn_curse;
extern int gsn_darkness;
extern int gsn_delayed_blast_fireball;
extern int gsn_demonfire;
extern int gsn_denounciation;
extern int gsn_destroy_rune;
extern int gsn_destroy_tattoo;
extern int gsn_detect_hidden;
extern int gsn_detect_invis;
extern int gsn_detect_magic;
extern int gsn_detect_poison;
extern int gsn_dispel_magic;
extern int gsn_disenchant;
extern int gsn_dispel_evil;
extern int gsn_dispel_good;
extern int gsn_dissolution;
extern int gsn_drain_life;
extern int gsn_duplicate;
extern int gsn_earthquake;
extern int gsn_earth_to_mud;
extern int gsn_enchant_armor;
extern int gsn_enchant_weapon;
extern int gsn_energy_drain;
extern int gsn_embellish;
extern int gsn_nature_protection;
extern int gsn_faerie_fire;
extern int gsn_faerie_fog;
extern int gsn_farsight;
extern int gsn_feeblemind;
extern int gsn_figurine_spell;
extern int gsn_fireball;
extern int gsn_fireproof;
extern int gsn_flame_arrow;
extern int gsn_flamestrike;
extern int gsn_flesh_golem;
extern int gsn_flood;
extern int gsn_animal_skins;
extern int gsn_floating_disc;
extern int gsn_fly;
extern int gsn_fortify;
extern int gsn_frenzy;
extern int gsn_gate;
extern int gsn_giant_strength;
extern int gsn_harm;
extern int gsn_haste;
extern int gsn_heal;
extern int gsn_heat_metal;
extern int gsn_hell_blades;
extern int gsn_holy_word;
extern int gsn_hurricane;
extern int gsn_ice_bolt;
extern int gsn_identify;
extern int gsn_ignore_wounds;
extern int gsn_inform;
extern int gsn_infravision;
extern int gsn_invisibility;
extern int gsn_life_wave;
extern int gsn_lesser_dispel;
extern int gsn_lightning_bolt;
extern int gsn_lure;
extern int gsn_karma;
extern int gsn_know_alignment;
extern int gsn_jail;
extern int gsn_jinx;
extern int gsn_locate_object;
extern int gsn_magic_missile;
extern int gsn_magic_stone;
extern int gsn_mana_drain;
extern int gsn_mass_healing;
extern int gsn_mass_invis;
extern int gsn_meteor_swarm;
extern int gsn_missionary;
extern int gsn_misty_cloak;
extern int gsn_mirror_image;
extern int gsn_moonbeam;
extern int gsn_nexus;
extern int gsn_nightmares;
extern int gsn_oculary;
extern int gsn_pass_door;
extern int gsn_phantom_force;
extern int gsn_plague;
extern int gsn_plant;
extern int gsn_poison;
extern int gsn_portal;
extern int gsn_prismatic_spray;
extern int gsn_prismatic_sphere;
extern int gsn_protection_evil;
extern int gsn_protection_good;
extern int gsn_protection_neutral;
extern int gsn_psychic_crush;
extern int gsn_pyrotechnics;
extern int gsn_quill_armor;
extern int gsn_rainbow_burst;
extern int gsn_ray_of_truth;
extern int gsn_recharge;
extern int gsn_refresh;
extern int gsn_remove_alignment;
extern int gsn_remove_curse;
extern int gsn_repel;
extern int gsn_replicate;
extern int gsn_repulsion;
extern int gsn_rukus_magna;
extern int gsn_sanctuary;
extern int gsn_simulacrum;
extern int gsn_sharpen;
extern int gsn_shawl;
extern int gsn_shield;
extern int gsn_shocking_grasp;
extern int gsn_shock_wave;
extern int gsn_skeletal_warrior;
extern int gsn_sleep;
extern int gsn_slow;
extern int gsn_soul_trap;
extern int gsn_spell_stealing;
extern int gsn_spirit_link;
extern int gsn_stone_skin;
extern int gsn_summon;
extern int gsn_summon_angel;
extern int gsn_sunray;
extern int gsn_symbol;
extern int gsn_teleport;
extern int gsn_test_room;
extern int gsn_tornado;
extern int gsn_true_sight;
extern int gsn_tsunami;
extern int gsn_turn_undead;
extern int gsn_ventriloquate;
extern int gsn_voodoo_doll;
extern int gsn_weaken;
extern int gsn_whirlpool;
extern int gsn_winter_storm;
extern int gsn_withstand_death;
extern int gsn_wildfire;
extern int gsn_word_of_recall;
extern int gsn_acid_breath;
extern int gsn_fire_breath;
extern int gsn_frost_breath;
extern int gsn_gas_breath;
extern int gsn_lightning_breath;
extern int gsn_general_purpose;
extern int gsn_high_explosive;
extern int gsn_banish;
extern int gsn_blink;
extern int gsn_build_fire;
extern int gsn_call_to_arms;
extern int gsn_cave_bears;
extern int gsn_cloud_of_poison;
extern int gsn_cone_of_fear;
extern int gsn_confusion;
extern int gsn_create_shadow;
extern int gsn_cryogenesis;
extern int gsn_demand;
extern int gsn_detect_all;
extern int gsn_detonation;
extern int gsn_discordance;
extern int gsn_displace;
extern int gsn_drakor;
extern int gsn_enlargement;
extern int gsn_familiar;
extern int gsn_fear;
extern int gsn_flag;
extern int gsn_giant_insect;
extern int gsn_grandeur;
extern int gsn_granite_stare;
extern int gsn_gullivers_travel;
extern int gsn_hailstorm;
extern int gsn_hold_person;
extern int gsn_homonculus;
extern int gsn_hunt;
extern int gsn_lightning_spear;
extern int gsn_hypnosis;
extern int gsn_injustice;
extern int gsn_loneliness;
extern int gsn_mass_protect;
extern int gsn_materialize;
extern int gsn_melior;
extern int gsn_minimation;
extern int gsn_mutate;
extern int gsn_oracle;
extern int gsn_pacifism;
extern int gsn_pentagram;
extern int gsn_phantasm_monster;
extern int gsn_primal_rage;
extern int gsn_rain_of_tears;
extern int gsn_reach_elemental;
extern int gsn_robustness;
extern int gsn_scalemail;
extern int gsn_scramble;
extern int gsn_shifting_sands;
extern int gsn_shriek;
extern int gsn_soundproof;
extern int gsn_summon_greater;
extern int gsn_summon_horde;
extern int gsn_summon_lesser;
extern int gsn_summon_lord;
extern int gsn_surge;
extern int gsn_tame_animal;
extern int gsn_taunt;
extern int gsn_thirst;
extern int gsn_turn_magic;
extern int gsn_wail;
extern int gsn_weaponsmith;
extern int gsn_web;
extern int gsn_wizards_eye;
extern int gsn_wrath;
extern int gsn_axe;
extern int gsn_backstab;
extern int gsn_bash;
extern int gsn_beheading;
extern int gsn_berserk;
extern int gsn_blindfighting;
extern int gsn_block;
extern int gsn_brawling;
extern int gsn_brew;
extern int gsn_bribe;
extern int gsn_butcher;
extern int gsn_call_to_hunt;
extern int gsn_cave_in;
extern int gsn_carve_boulder;
extern int gsn_carve_spear;
extern int gsn_charge;
extern int gsn_chase;
extern int gsn_cheat;
extern int gsn_conversion;
extern int gsn_corpse_drain;
extern int gsn_combine_potion;
extern int gsn_corruption;
extern int gsn_dagger;
extern int gsn_drunken_master;
extern int gsn_dark_feast;
extern int gsn_dirt_kicking;
extern int gsn_disarm;
extern int gsn_dodge;
extern int gsn_drag;
extern int gsn_dragon_bite;
extern int gsn_dual_wield;
extern int gsn_enhanced_damage;
extern int gsn_enrage;
extern int gsn_ensnare;
extern int gsn_envenom;
extern int gsn_gore;
extern int gsn_flail;
extern int gsn_fourth_attack;
extern int gsn_gemology;
extern int gsn_gladiator;
extern int gsn_gravitation;
extern int gsn_hand_to_hand;
extern int gsn_hunter_ball;
extern int gsn_iron_vigil;
extern int gsn_jump;
extern int gsn_kick;
extern int gsn_leadership;
extern int gsn_living_stone;
extern int gsn_bite;
extern int gsn_mace;
extern int gsn_parry;
extern int gsn_polearm;
extern int gsn_presence;
extern int gsn_push;
extern int gsn_rally;
extern int gsn_reflection;
extern int gsn_rescue;
extern int gsn_retreat;
extern int gsn_retribution;
extern int gsn_rip;
extern int gsn_rub;
extern int gsn_sap;
extern int gsn_second_attack;
extern int gsn_shapeshift;
extern int gsn_shedding;
extern int gsn_shield_bash;
extern int gsn_shield_block;
extern int gsn_snake_bite;
extern int gsn_spear;
extern int gsn_substitution;
extern int gsn_sword;
extern int gsn_tail_attack;
extern int gsn_tail_trip;
extern int gsn_taste;
extern int gsn_telepathy;
extern int gsn_third_attack;
extern int gsn_throw;
extern int gsn_thrust;
extern int gsn_transferance;
extern int gsn_trip;
extern int gsn_tumbling;
extern int gsn_waterwalk;
extern int gsn_water_breathing;
extern int gsn_whip;
extern int gsn_wild_swing;
extern int gsn_bashdoor;
extern int gsn_fast_healing;
extern int gsn_faith;
extern int gsn_haggle;
extern int gsn_hide;
extern int gsn_lore;
extern int gsn_meditation;
extern int gsn_peek;
extern int gsn_pick_lock;
extern int gsn_razor_claws;
extern int gsn_recall;
extern int gsn_regeneration;
extern int gsn_regrowth;
extern int gsn_scrolls;
extern int gsn_scribe;
extern int gsn_sneak;
extern int gsn_staves;
extern int gsn_steal;
extern int gsn_wands;
extern int gsn_cure_plague;
extern int gsn_guardian;
extern int gsn_lay_hands;
extern int gsn_shadow;
extern int gsn_shadow_magic;
extern int gsn_stone_sleep;
extern int gsn_appraise;
extern int gsn_lair;
extern int gsn_crush;
extern int gsn_hug;
extern int gsn_slide;
extern int gsn_spy;
extern int gsn_craft_item;
extern int gsn_paint_lesser;
extern int gsn_paint_greater;
extern int gsn_paint_power;
extern int gsn_alarm_rune;
extern int gsn_fire_rune;
extern int gsn_shackle_rune;
extern int gsn_wizard_mark;
extern int gsn_blade_rune;
extern int gsn_burst_rune;
extern int gsn_balance_rune;
extern int gsn_soul_rune;
extern int gsn_zeal;
extern int gsn_death_rune;
extern int gsn_hurl;
extern int gsn_soul_blade;
extern int gsn_fury;
extern int gsn_trance;
extern int gsn_evasion;
extern int gsn_magic_sheath;
extern int gsn_essence;
extern int gsn_last_skill;

// Monks
extern int gsn_boost;
extern int gsn_chakra;
extern int gsn_stance_turtle;
extern int gsn_stance_tiger;
extern int gsn_stance_mantis;
extern int gsn_stance_shadow;
extern int gsn_stance_kensai;
extern int gsn_shoulder_throw;
extern int gsn_intense_damage;
extern int gsn_weapon_catch; // removed
extern int gsn_pain_touch;
extern int gsn_dragon_kick;
extern int gsn_choke_hold;
extern int gsn_eagle_claw;
extern int gsn_demon_fist;
extern int gsn_chi_whisper; // removed
extern int gsn_chi_moonlight; // removed
extern int gsn_chi_whirlwind; //removed
// Monk revision
extern int gsn_chi_gekkou; // replaced whirlwind
extern int gsn_chi_kaze;  // replaced whisper
extern int gsn_chi_ei; // replaced moonlight
extern int gsn_weapon_lock; // replaced weapon catch
extern int gsn_shadow_walk; // replaced cycle
extern int gsn_zanshin;

extern int gsn_ninjitsu;
extern int gsn_evasion;
extern int gsn_cycle; // removed
extern int gsn_balance;
extern int gsn_blindfighting;

extern int gsn_katana;
extern int gsn_third_eye;
extern int gsn_quicken;
extern int gsn_doublestrike;
extern int gsn_extreme_damage;
extern int gsn_seppuku;

// clanspells
extern int gsn_homeland;
extern int gsn_paradox;
extern int gsn_dragonbane;
extern int gsn_harmonic_aura;
extern int gsn_brotherhood;

// Upgraded druids and rangers
extern int gsn_minor_revitalize;
extern int gsn_lesser_revitalize;
extern int gsn_greater_revitalize;
extern int gsn_thorn_mantle;
extern int gsn_leaf_shield;
extern int gsn_marksmanship;
extern int gsn_barrage;
extern int gsn_aiming;
extern int gsn_rapid_shot;
extern int gsn_stun;
extern int gsn_purify;
extern int gsn_hallow;

// Tattoo related stuff
#define OBJ_VNUM_NEW_TATTOO 6725
#define LESSER_TATTOO 0
#define GREATER_TATTOO 1
#define POWER_TATTOO 2
#define TATTOO_TYPES 11

#define TATTOO_ANIMATES 1
#define TATTOO_MV_SPELLS 2
#define TATTOO_HP_REGEN 3
#define TATTOO_MANA_REGEN 4
#define TATTOO_HP_PER_KILL 5
#define TATTOO_MANA_PER_KILL 6
#define TATTOO_EXTRA_DP 7
#define TATTOO_PHYSICAL_RESIST 8
#define TATTOO_MAGICAL_RESIST 9
#define TATTOO_LEARN_SPELL 10
#define TATTOO_RAISES_CASTING 11

#define BLADE_RUNE_SPEED 0
#define BLADE_RUNE_ACCURACY 1
#define BLADE_RUNE_PARRYING 2

struct lesser_tattoo_mod {
        int location;
        int modifier;
        char *noun;
        char *adjective;
};

struct greater_tattoo_mod {
        int location;
        int min_mod;
        int max_mod;
        char *noun;
        char *adjective;
};

// These will apply to none and the modifier of the
// affect will store the special power number used elsewhere.
struct power_tattoo_mod {
        int special;
        char *noun;
};

extern const struct lesser_tattoo_mod lesser_tattoo_table[];
extern const struct greater_tattoo_mod greater_tattoo_table[];
extern const struct power_tattoo_mod power_tattoo_table[];

// Data universal for the help file editor
struct hedit_type {
	char *builders;
};

struct hedit_type heditor;

// Crafted Item System definitions, by StarX. Used in act_wiz.c
typedef struct ci_entry CRAFTED_ITEM;
typedef struct ci_part CRAFTED_COMPONENT;

struct ci_part {
	int part_vnum;
	int qty;
	CRAFTED_COMPONENT *next_part;
};

struct ci_entry {
	int index;
	int ci_vnum;
	CRAFTED_COMPONENT *parts;
	CRAFTED_ITEM *next_ci;
};

CRAFTED_ITEM *crafted_items;

// Newbie tip structs
typedef struct newbie_tip TIP_DATA;

struct newbie_tip {
	int id;
	char *message;
	TIP_DATA *next;
};

TIP_DATA *first_tip;
TIP_DATA *last_tip;
int top_tips;


/*
 * Utility macros.
 */
#define IS_VALID(data)     ((data) != NULL && (data)->valid)
#define VALIDATE(data)     ((data)->valid = TRUE)
#define INVALIDATE(data)   ((data)->valid = FALSE)
#define UMIN(a, b)      ((a) < (b) ? (a) : (b))
#define UMAX(a, b)      ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)    ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)     ((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define UPPER(c)     ((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
#define IS_SET(flag, bit)  ((flag) & (bit))
#define SET_BIT(var, bit)  ((var) |= (bit))
#define REMOVE_BIT(var, bit)  ((var) &= ~(bit))



/*
 * Character macros.
 */


#define IS_QUESTING(ch)     (IS_SET((ch)->act, PLR_QUESTING))


#define IS_NPC(ch)      (IS_SET((ch)->act, ACT_IS_NPC))
//#define IS_CLAN_GOON(ch) (IS_NPC(ch) && (ch->spec_fun == spec_lookup("spec_goon_kindred") || ch->spec_fun == spec_lookup("spec_goon_seekers") || ch->spec_fun == spec_lookup("spec_goon_venari") || ch->spec_fun == spec_lookup("spec_goon_kenshi")))
#define IS_CLAN_GOON(ch) (IS_NPC(ch) && (ch->spec_fun == spec_lookup("spec_goon")))
#define IN_ANY_CLAN_HALL(ch) (in_own_hall(ch) || in_enemy_hall(ch))
#define IS_IMMORTAL(ch)    (get_trust(ch) >= LEVEL_IMMORTAL)
#define IS_HERO(ch)     (get_trust(ch) >= LEVEL_HERO)
#define IS_TRUSTED(ch,level)  (get_trust((ch)) >= (level))
#define NEWBIE_REMORT_RECLASS(ch) ((ch->remort || ch->reclass) && ch->pcdata->learned[gsn_dagger] < 40 && ch->pcdata->learned[gsn_sword] < 40 && ch->pcdata->learned[gsn_spear] < 40 && ch->pcdata->learned[gsn_polearm] < 40 && ch->pcdata->learned[gsn_axe] < 40 && ch->pcdata->learned[gsn_flail] < 40 && ch->pcdata->learned[gsn_whip] < 40 && ch->pcdata->learned[gsn_mace] < 40 && ch->pcdata->learned[gsn_hand_to_hand] < 40 && ch->pcdata->learned[gsn_katana] < 40)

#define IS_WEAPON_SN(sn) (sn == gsn_dagger || sn == gsn_sword || sn == gsn_spear || sn == gsn_polearm || sn == gsn_axe || sn == gsn_flail || sn == gsn_whip || sn == gsn_mace || sn == gsn_hand_to_hand || sn == gsn_katana)

#define MAX_HP(ch) (ch->max_hit + ch->max_hit_bonus)
#define MAX_MANA(ch) (ch->max_mana + ch->max_mana_bonus)
#define MAX_MOVE(ch) (ch->max_move + ch->max_move_bonus)

#define replace_string( pstr, nstr )     { free_string( (pstr) ); pstr=str_dup( (nstr) ); }

#define IS_NULLSTR(str)        ((str)==NULL || (str)[0]=='\0')

#define CH(d)                ((d)->original ? (d)->original : (d)->character )
char *first_arg(char *argument, char *arg_first, bool fCase);





#define GET_AGE(ch)     ((int) (17 + ((ch)->played  + current_time - (ch)->logon )/72000))

#define IS_DRAGON(ch)    ((ch->race == race_lookup ("black dragon")) || (ch->race == race_lookup ("red dragon")) || (ch->race == race_lookup ("blue dragon")) || (ch->race == race_lookup ("white dragon")) || (ch->race == race_lookup ("green dragon")))
#define IS_RANGE(ch, victim)  ((ch->level + 8 > victim->level) && (ch->level - 8 < victim->level))

#define IS_GOOD(ch)     (ch->alignment >= 350)
#define IS_EVIL(ch)     (ch->alignment <= -350)
#define IS_NEUTRAL(ch)     (!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_AWAKE(ch)    (ch->position > POS_SLEEPING)
#define GET_AC(ch,type)    ((ch)->armor[type] + ( IS_AWAKE(ch) ? dex_app[get_curr_stat(ch,STAT_DEX)].defensive : 0 ) - ( (IS_DRAGON(ch) && ch->remort > 0) ? ch->level * 5 / 2: 0 ))

#define GET_HITROLL(ch) ((ch)->hitroll+str_app[get_curr_stat(ch,STAT_STR)].tohit)
#define GET_DAMROLL(ch) ((ch)->damroll+str_app[get_curr_stat(ch,STAT_STR)].todam)

#define IS_OUTSIDE(ch)     (!IS_SET(ch->in_room->room_flags, ROOM_INDOORS) && ch->in_room->sector_type != SECT_INSIDE)

#define WAIT_STATE(ch, npulse)   ((ch)->wait = UMAX((ch)->wait, (npulse)))
#define DAZE_STATE(ch, npulse)  ((ch)->daze = UMAX((ch)->daze, (npulse)))
#define HAS_TRIGGER(ch,trig)        (IS_SET((ch)->pIndexData->mprog_flags,(trig)))

/*
 * Object macros.
 */
#define CAN_WEAR(obj, part)   (IS_SET((obj)->wear_flags,  (part)))
#define IS_OBJ_STAT(obj, stat)   (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj,stat)(IS_SET((obj)->value[4],(stat)))
#define WEIGHT_MULT(obj)   ((obj)->item_type == ITEM_CONTAINER ? (obj)->value[4] : 100)

/*
 * Structure for a social in the socials table.
 */
struct social_type
{
	char name[20];
	char *char_no_arg;
	char *others_no_arg;
	char *char_found;
	char *others_found;
	char *vict_found;
	char *char_not_found;
	char *char_auto;
	char *others_auto;
};


/* Structure for automatic mob parameters in OLC - Delstar */

struct auto_type
{
	int level;
	int hit_num;
	int hit_face;
	int hit_bonus;
	int armor_class;
	int dam_num;
	int dam_face;
	int dam_bonus;
	int mana_num;
	int mana_face;
	int mana_bonus;
	int hitroll;
};


/*
 * Global constants.
 */
extern const struct str_app_type str_app[31];
extern const struct int_app_type int_app[31];
extern const struct wis_app_type wis_app[31];
extern const struct dex_app_type dex_app[31];
extern const struct con_app_type con_app[31];

extern const struct class_type class_table[MAX_CLASS];
extern const struct weapon_type weapon_table[];
extern const struct item_type item_table[];
extern const struct auto_type auto_table[];
extern const struct wiznet_type wiznet_table[];
extern const struct attack_type attack_table[];
extern const struct race_type race_table[];
extern const struct sliver_type sliver_table[];
extern const struct pc_race_type pc_race_table[];
extern const struct spec_type spec_table[];
extern const struct liq_type liq_table[];
extern const struct skill_type skill_table[MAX_SKILL];
extern const struct group_type group_table[MAX_GROUP];
extern struct social_type social_table[MAX_SOCIALS];
extern char *const title_table[MAX_CLASS]
[MAX_LEVEL + 1]
[2];



/*
 * Global variables.
 */
extern HELP_DATA *help_first;
extern HELP_DATA *help_last;
extern SHOP_DATA *shop_first;

extern CHAR_DATA *char_list;
extern DESCRIPTOR_DATA *descriptor_list;
extern OBJ_DATA *object_list;
extern MPROG_CODE *mprog_list;
extern char bug_buf[];
extern time_t current_time;
extern bool fLogAll;
extern FILE *fpReserve;
extern KILL_DATA kill_table[];
extern char log_buf[];
extern TIME_INFO_DATA time_info;
extern WEATHER_DATA weather_info;
extern bool MOBtrigger;
extern char last_command[MAX_STRING_LENGTH];
extern int ctf_score_blue;
extern int ctf_score_red;


/*
 * OS-dependent declarations.
 * These are all very standard library functions,
 *   but some systems have incomplete or non-ansi header files.

#ifndef NOCRYPT
char *crypt(const char *key, const char *salt);
#endif
 */

/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
#if defined(NOCRYPT)
#define crypt(s1, s2)   (s1)
#endif

 */


/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */
#if defined(macintosh)
#define PLAYER_DIR   ""			/* Player files   */
#define TEMP_FILE "romtmp"
#define NULL_FILE "proto.are"	/* To reserve one stream */
#endif

#if defined(MSDOS)
#define PLAYER_DIR   ""			/* Player files */
#define TEMP_FILE "romtmp"
#define NULL_FILE "nul"			/* To reserve one stream */
#endif

#if defined(unix)
#define PLAYER_DIR      "../player/"	/* Player files */
#define GOD_DIR         "../gods/"	/* list of gods */
#define TEMP_FILE		"../player/romtmp"
#define NULL_FILE		"/dev/null"	/* To reserve one stream */
#define LAST_COMMAND    "last_command.txt"  /*For the signal handler.*/
#endif

#define AREA_LIST       "area.lst"	/* List of areas */
#define BUG_FILE        "bugs.txt"	/* For 'bug' and bug() */
#define TYPO_FILE       "typos.txt"		/* For 'typo' */
#define NOTE_FILE       "notes.not"		/* For 'notes' */
#define IDEA_FILE		"ideas.not"
#define PENALTY_FILE	"penal.not"
#define NEWS_FILE		"news.not"
#define CHANGES_FILE	"chang.not"
#define SHUTDOWN_FILE   "shutdown.txt"	/* For 'shutdown' */
#define BAN_FILE		"ban.txt"
#define MUSIC_FILE		"music.txt"
#define COUNT_FILE		"count.txt"
#define CLAN_FILE		"clanleaders.txt"



/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD  CHAR_DATA
#define MID MOB_INDEX_DATA
#define OD  OBJ_DATA
#define OID OBJ_INDEX_DATA
#define RID ROOM_INDEX_DATA
#define SF  SPEC_FUN
#define MPC MPROG_CODE

/* act_comm.c */
int check_demon_loyalty(CHAR_DATA *ch, CHAR_DATA *victim);
void check_sex(CHAR_DATA * ch);
void add_follower(CHAR_DATA * ch, CHAR_DATA * master);
void stop_follower(CHAR_DATA * ch);
void nuke_pets(CHAR_DATA * ch);
void die_follower(CHAR_DATA * ch);
bool is_same_group(CHAR_DATA * ach, CHAR_DATA * bch);
bool in_enemy_hall(CHAR_DATA * ch);
bool in_own_hall(CHAR_DATA * ch);
void quit_character(CHAR_DATA * ch);

/* act_enter.c */
RID *get_random_room(CHAR_DATA * ch);

/* act_info.c */
void set_title(CHAR_DATA * ch, char *title);
char *godName(CHAR_DATA * ch);

/* act_move.c */
void move_char(CHAR_DATA * ch, int door, bool follow);

/* act_obj.c */
bool can_loot(CHAR_DATA * ch, OBJ_DATA * obj);
void wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace);
void get_obj(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * container);
int trap_symbol(CHAR_DATA *victim, OBJ_DATA *obj);

/* act_wiz.c */
void wiznet(char *string, CHAR_DATA * ch, OBJ_DATA * obj, long flag, long flag_skip, int min_level);

/* alias.c */
void substitute_alias(DESCRIPTOR_DATA * d, char *input);

/* ban.c */
bool check_ban(char *site, int type);


/* comm.c */
void show_string(struct descriptor_data *d, char *input);
void close_socket(DESCRIPTOR_DATA * dclose);
void write_to_buffer(DESCRIPTOR_DATA * d, const char *txt);
bool write_to_descriptor(int desc, char *txt, int length);
void cprintf(CHAR_DATA * ch, bool inColour, bool showCodes, const char *txt,...);
void Cprintf(CHAR_DATA * ch, const char *txt,...);
void page_to_char(const char *txt, CHAR_DATA * ch);
void act(const char *format, CHAR_DATA * ch, const void *arg1, const void *arg2, int type, int min_pos);

 /*
  * Colour stuff by Lope of Loping Through The MUD
  */
int colour(char type, CHAR_DATA * ch, char *string);
void colourconv(char *buffer, const char *txt, CHAR_DATA * ch);
void send_to_char_bw(const char *txt, CHAR_DATA * ch);
void page_to_char_bw(const char *txt, CHAR_DATA * ch);

/* db.c */
char *print_flags(int flag);
void boot_db(void);
void area_update(void);
CD *create_mobile(MOB_INDEX_DATA * pMobIndex);
void clone_mobile(CHAR_DATA * parent, CHAR_DATA * clone);
OD *create_object(OBJ_INDEX_DATA * pObjIndex, int level);
void clone_object(OBJ_DATA * parent, OBJ_DATA * clone);
void clear_char(CHAR_DATA * ch);
char *get_extra_descr(const char *name, EXTRA_DESCR_DATA * ed);
MID *get_mob_index(int vnum);
OID *get_obj_index(int vnum);
RID *get_room_index(int vnum);
MPC *get_mprog_index(int vnum);
char fread_letter(FILE * fp);
int fread_number(FILE * fp);
long fread_flag(FILE * fp);
char *fread_string(FILE * fp);
char *fread_string_eol(FILE * fp);
void fread_to_eol(FILE * fp);
char *fread_word(FILE * fp);
long flag_convert(char letter);
void *alloc_mem(int sMem);
void *alloc_perm(int sMem);
void free_mem(void *pMem, int sMem);
char *str_dup(const char *str);
void free_string(char *pstr);
int number_fuzzy(int number);
int number_range(int from, int to);
int number_percent(void);
int number_door(void);
int number_bits(int width);
long number_mm(void);
int dice(int number, int size);
int interpolate(int level, int value_00, int value_32);
void smash_tilde(char *str);
void append_file(CHAR_DATA * ch, char *file, char *str);
void tail_chain(void);

/* effect.c */
void acid_effect(void *vo, int level, int dam, int target);
void cold_effect(void *vo, int level, int dam, int target);
void fire_effect(void *vo, int level, int dam, int target);
void poison_effect(void *vo, int level, int dam, int target);
void shock_effect(void *vo, int level, int dam, int target);
void water_effect(void *vo, int level, int dam, int target);

/* fight.c */
bool is_safe(CHAR_DATA * ch, CHAR_DATA * victim);
bool is_safe_spell(CHAR_DATA * ch, CHAR_DATA * victim, bool area);
void violence_update(void);
void rain_update(void);
void multi_hit(CHAR_DATA * ch, CHAR_DATA * victim, int dt);
bool anonymous_damage(CHAR_DATA *victim, int dam, int dt, bool lethal);
bool damage(CHAR_DATA * ch, CHAR_DATA * victim, int dam, int dt, int damClass, bool show, bool spell);
bool damage_old(CHAR_DATA * ch, CHAR_DATA * victim, int dam, int dt, int damClass, bool show, bool spell);
void update_pos(CHAR_DATA * victim);
void stop_fighting(CHAR_DATA * ch, bool fBoth);
void check_killer(CHAR_DATA * ch, CHAR_DATA * victim);
int get_main_hitroll(CHAR_DATA *);
int get_dual_hitroll(CHAR_DATA *);
int get_ranged_hitroll(CHAR_DATA *);
int get_natural_hitroll(CHAR_DATA *);
int get_main_damroll(CHAR_DATA *);
int get_dual_damroll(CHAR_DATA *);
int get_ranged_damroll(CHAR_DATA *);
int get_natural_damroll(CHAR_DATA *);

/* handler.c */
int count_users(OBJ_DATA * obj);
void deduct_cost(CHAR_DATA * ch, int cost);
void affect_enchant(OBJ_DATA * obj);
int check_immune(CHAR_DATA * ch, int dam_type);
int get_carry_weight(CHAR_DATA* ch);
char * PERS(CHAR_DATA * ch, CHAR_DATA * looker);
int count_obj_list_by_name(CHAR_DATA *, char *, OBJ_DATA *);
int wearing_norecall_item(CHAR_DATA *ch);
int wearing_nogate_item(CHAR_DATA *ch);

/* websvr.c */
void init_web(int port);
void handle_web(void);
void shutdown_web(void);

/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */
#define OBJ_VNUM_DUMMY        30

/*
 * Area flags.
 */
#define         AREA_NONE       0
#define         AREA_CHANGED    1	/* Area has been modified. */
#define         AREA_ADDED      2	/* Area has been added to. */
#define         AREA_LOADING    4	/* Used for counting in db.c */

#define MAX_DIR        6
#define NO_FLAG -99				/* Must not be used in flags or stats. */

/*
 * Global Constants
 */

/*
 * Global variables
 */
extern AREA_DATA *area_first;
extern AREA_DATA *area_last;
extern SHOP_DATA *shop_last;

extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_exit;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_reset;
extern int top_room;
extern int top_shop;

extern int top_vnum_mob;
extern int top_vnum_obj;
extern int top_vnum_room;
extern int top_vnum_mprog;

extern char str_empty[1];

extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

/* act_wiz.c */
/*
   ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg );
 */
/* db.c */
void reset_area(AREA_DATA * pArea, bool manualReset);
void reset_room(ROOM_INDEX_DATA * pRoom, bool manualReset);

/* string.c */
void string_edit(CHAR_DATA * ch, char **pString);
void string_append(CHAR_DATA * ch, char **pString);
char *string_replace(char *orig, char *old, char *replacement);
void string_add(CHAR_DATA * ch, char *argument);
char *format_string(char *oldstring /*, bool fSpace */ );
char *first_arg(char *argument, char *arg_first, bool fCase);
char *string_unpad(char *argument);
char *string_proper(char *argument);

/* olc.c */
bool run_olc_editor(DESCRIPTOR_DATA * d);
char *olc_ed_name(CHAR_DATA * ch);
char *olc_ed_vnum(CHAR_DATA * ch);

int liq_lookup(const char *name);
int material_lookup(const char *name);
int weapon_lookup(const char *name);
int weapon_type(const char *name);
char *weapon_name(int weapon_Type);
int item_lookup(const char *name);
char *item_name(int item_type);
int attack_lookup(const char *name);
int race_lookup(const char *name);
long wiznet_lookup(const char *name);
int class_lookup(const char *name);
bool is_clan(CHAR_DATA * ch);
bool is_same_clan(CHAR_DATA * ch, CHAR_DATA * victim);
bool is_old_mob(CHAR_DATA * ch);
int get_skill(CHAR_DATA * ch, int sn);
int get_weapon_sn(CHAR_DATA * ch, int location);
int get_weapon_skill(CHAR_DATA * ch, int sn);
int get_age(CHAR_DATA * ch);
int get_hours(CHAR_DATA* ch);
int get_perm_hours(CHAR_DATA* ch);
void reset_char(CHAR_DATA * ch);
int get_trust(CHAR_DATA * ch);
int get_curr_stat(CHAR_DATA * ch, int stat);
int get_max_stat(CHAR_DATA * ch, int stat);
int get_race_curr_stat(CHAR_DATA *ch, int stat);
int get_race_max_stat(CHAR_DATA *ch, int stat);
int get_max_train(CHAR_DATA * ch, int stat);
int can_carry_n(CHAR_DATA * ch);
int can_carry_w(CHAR_DATA * ch);
bool is_name(char *str, char *namelist);
bool is_name_2(char *str, char *namelist);
bool is_exact_name(char *str, char *namelist);
void char_from_room(CHAR_DATA * ch);
void char_to_room(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex);
void obj_to_char(OBJ_DATA * obj, CHAR_DATA * ch);
void obj_from_char(OBJ_DATA * obj);
int apply_ac(OBJ_DATA * obj, int iWear, int type);
OD *get_eq_char(CHAR_DATA * ch, int iWear);
void equip_char(CHAR_DATA * ch, OBJ_DATA * obj, int iWear);
void unequip_char(CHAR_DATA * ch, OBJ_DATA * obj);
int count_obj_list(OBJ_INDEX_DATA * obj, OBJ_DATA * list);
void obj_from_room(OBJ_DATA * obj);
void obj_to_room(OBJ_DATA * obj, ROOM_INDEX_DATA * pRoomIndex);
void obj_to_obj(OBJ_DATA * obj, OBJ_DATA * obj_to);
void obj_from_obj(OBJ_DATA * obj);
void extract_obj(OBJ_DATA * obj);
void extract_char(CHAR_DATA * ch, bool fPull);
CD *get_char_room(CHAR_DATA * ch, char *argument);
CD *get_char_from_room(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument);
CD *get_char_world(CHAR_DATA * ch, char *argument, int allowIntercontinental);
CD *get_char_world_finished_areas(CHAR_DATA * ch, char *argument, int allowIntercontinental);
OD *get_obj_type(OBJ_INDEX_DATA * pObjIndexData);
OD *get_obj_list(CHAR_DATA * ch, char *argument, OBJ_DATA * list);
OD *get_obj_carry(CHAR_DATA * ch, char *argument, CHAR_DATA * viewer);
OBJ_DATA *get_obj_carry_or_wear(CHAR_DATA *ch, char *argument);
int get_obj_qty(CHAR_DATA *ch, OBJ_INDEX_DATA *obj);
OD *get_obj_wear(CHAR_DATA * ch, char *argument, CHAR_DATA* viewer);
OD *get_obj_here(CHAR_DATA * ch, char *argument);
OD *get_obj_world(CHAR_DATA * ch, char *argument);
OD *create_money(int gold, int silver);
int get_obj_number(OBJ_DATA * obj);
int get_obj_weight(OBJ_DATA * obj);
int get_true_weight(OBJ_DATA * obj);
bool room_is_dark(ROOM_INDEX_DATA * pRoomIndex);
bool is_room_owner(CHAR_DATA * ch, ROOM_INDEX_DATA * room);
bool room_is_private(ROOM_INDEX_DATA * pRoomIndex);
bool can_see(CHAR_DATA * ch, CHAR_DATA * victim);
bool can_see_obj(CHAR_DATA * ch, OBJ_DATA * obj);
bool can_see_room(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex);
bool can_drop_obj(CHAR_DATA * ch, OBJ_DATA * obj);
char *affect_loc_name(int location);
char *affect_bit_name(int vector);
char *extra_bit_name(int extra_flags);
char *wear_bit_name(int wear_flags);
char *act_bit_name(int act_flags);
char *off_bit_name(int off_flags);
char *imm_bit_name(int imm_flags);
char *form_bit_name(int form_flags);
char *part_bit_name(int part_flags);
char *weapon_bit_name(int weapon_flags);
char *comm_bit_name(int comm_flags);
char *cont_bit_name(int cont_flags);


/* interp.c */
void interpret(CHAR_DATA * ch, char *argument);
bool is_number(char *arg);
int number_argument(char *argument, char *arg);
int mult_argument(char *argument, char *arg);
char *one_argument(char *argument, char *arg_first);

/* magic.c */
int find_spell(CHAR_DATA * ch, const char *name);
int mana_cost(CHAR_DATA * ch, int min_mana, int level);
int skill_lookup(const char *name);
int slot_lookup(int slot);
bool saves_spell(int level, CHAR_DATA * victim, int dam_type);
void obj_cast_spell(int sn, int level, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj);
void affect_refresh(CHAR_DATA * ch, int sn, int duration);

/* save.c */
void save_char_obj(CHAR_DATA * ch, int drop_items);
bool load_char_obj(DESCRIPTOR_DATA * d, char *name);

/* skills.c */
bool parse_gen_groups(CHAR_DATA * ch, char *argument);
void list_group_costs(CHAR_DATA * ch);
void list_group_known(CHAR_DATA * ch);
int exp_per_level(CHAR_DATA * ch, int points);
void check_improve(CHAR_DATA * ch, int sn, bool success, int multiplier);
int group_lookup(const char *name);
void gn_add(CHAR_DATA * ch, int gn);
void gn_remove(CHAR_DATA * ch, int gn);
void group_add(CHAR_DATA * ch, const char *name, bool deduct);
void group_remove(CHAR_DATA * ch, const char *name);

/* special.c */
SF *spec_lookup(const char *name);
char *spec_name(SPEC_FUN * function);

/* teleport.c */
RID *room_by_name(char *target, int level, bool error);

/* update.c */
void advance_level(CHAR_DATA * ch, bool hide);
void gain_exp(CHAR_DATA * ch, int gain);
void gain_condition(CHAR_DATA * ch, int iCond, int value);
void update_handler(void);

#undef   CD
#undef   MID
#undef   OD
#undef   OID
#undef   RID
#undef   SF
#undef AD


/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */
#define OBJ_VNUM_DUMMY        30

/*
 * Area flags.
 */
#define         AREA_NONE       0
#define         AREA_CHANGED    1	/* Area has been modified. */
#define         AREA_ADDED      2	/* Area has been added to. */
#define         AREA_LOADING    4	/* Used for counting in db.c */

#define MAX_DIR        6
#define NO_FLAG -99				/* Must not be used in flags or stats. */

/*
 * Global Constants
 */
extern char *const dir_name[];
extern const int rev_dir[];

	 /*extern const struct spec_type spec_table[];
	  */
/*
 * Global variables
 */
extern AREA_DATA *area_first;
extern AREA_DATA *area_last;
extern SHOP_DATA *shop_last;

extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_exit;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_reset;
extern int top_room;
extern int top_shop;

extern int top_vnum_mob;
extern int top_vnum_obj;
extern int top_vnum_room;
extern int top_vnum_mprog;

extern char str_empty[1];

extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

/* macro.c */
void do_macro(CHAR_DATA * ch, char *arg);
void macro_help(CHAR_DATA * ch);
void macro_list(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_define(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_undefine(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_trace(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_join(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_joinR(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_remove(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_rename(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_renameU(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_insert(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_append(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_replace(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_kill(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_running(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
void macro_status(CHAR_DATA * ch, CHAR_DATA * rch, char *arg);
int def_to_macro(CHAR_DATA * ch, char *arg);
void macro_to_def(CHAR_DATA * ch, char *art);
int macro_traceR(CHAR_DATA * ch, char *arg, int num, int *pInt, BUFFER *, int *);
void free_instance(PC_DATA * pc, INSTANCE_DATA * instance);
void add_instance(CHAR_DATA * ch, int, char *);
void spawn_macro_instace(CHAR_DATA *, INSTANCE_DATA *, int, int);
void do_next_command(CHAR_DATA *);
void substitute_macro_vars(MACRO_DATA *, INSTANCE_DATA *, char *);
bool macro_is_running(CHAR_DATA *, CHAR_DATA *, char *);
char *macro_one_argument(char *, char *);

/* act_wiz.c */
/*
   ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg );
 */

void restore_char(CHAR_DATA * ch);
bool check_parse_name(char *name);

/* string.c */
void string_edit(CHAR_DATA * ch, char **pString);
void string_append(CHAR_DATA * ch, char **pString);
char *string_replace(char *orig, char *old, char *replacement);
void string_add(CHAR_DATA * ch, char *argument);
char *format_string(char *oldstring /*, bool fSpace */ );
char *first_arg(char *argument, char *arg_first, bool fCase);
char *string_unpad(char *argument);
char *string_proper(char *argument);

/* olc.c */
bool run_olc_editor(DESCRIPTOR_DATA * d);
char *olc_ed_name(CHAR_DATA * ch);
char *olc_ed_vnum(CHAR_DATA * ch);

/* remort and stuff */
int remort_cp_qp(CHAR_DATA * ch);
int reclass_lookup(char* arg);
#define MAX_REM_SKILL 10
#define MAX_REMORT_RACE 32
#define MAX_REMORT_CLASS 6
#define MAX_REMORT_SUBCLASS 4
#define MAX_RECLASS 30

struct reclass_class_data
{
        char* ch_class;
        char* re_class;
        char* skills[MAX_REM_SKILL];
        int res_bits;
        int aff_bits;
	int unused;
};

struct reclass_data
{
        struct reclass_class_data class_info[MAX_RECLASS];
        char* skills[MAX_REM_SKILL];
};

struct remort_class_data
{
        char* classes[MAX_REMORT_SUBCLASS];
        char* skills[MAX_REM_SKILL];
};

struct remort_race_data
{
        char* race;
        char* remort_race;
        int rem_sub;
        char* skills[MAX_REM_SKILL];
        int res_bits;
        int aff_bits;
        int unused;
};

struct remort_data
{
        struct remort_class_data class_info[MAX_REMORT_CLASS];
        struct remort_race_data race_info[MAX_REMORT_RACE];
        char* skills[MAX_REM_SKILL];
};

extern const struct remort_data remort_list;
extern const struct reclass_data reclass_list;

/* end reclass/remort info */

void demote_recruiter(CHAR_DATA* ch);
void demote_leader(CHAR_DATA* ch);
void advance_recruiter(CHAR_DATA* ch);
void advance_leader(CHAR_DATA* ch);
void write_clanleaders();
void read_clanleaders();


/* stuff for mob sleep */

#endif
