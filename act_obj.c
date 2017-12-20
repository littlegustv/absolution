#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "merc.h"
#include "magic.h"
#include "deity.h"
#include "clan.h"
#include "utils.h"


/* command procedures needed */
DECLARE_DO_FUN(do_split);
DECLARE_DO_FUN(do_yell);
DECLARE_DO_FUN(do_say);
DECLARE_DO_FUN(do_wake);
DECLARE_DO_FUN(do_at);

extern char *target_name;
extern int generate_int(unsigned char, unsigned char);
extern void lore_obj(CHAR_DATA *ch, OBJ_DATA *obj);
extern void size_mob(CHAR_DATA * ch, CHAR_DATA * victim, int level);
extern int power_tattoo_count(CHAR_DATA*, int);
extern AFFECT_DATA *new_affect(void);
extern int hit_lookup(char *hit);
int count_obj_list_by_name(CHAR_DATA *, char *, OBJ_DATA *);
int count_obj_carry_by_name(CHAR_DATA *, char *, CHAR_DATA *);
/*
 * Local functions.
 */
bool remove_obj(CHAR_DATA * ch, int iWear, bool fReplace);
void wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace);
CHAR_DATA *find_keeper(CHAR_DATA * ch);
int get_cost(CHAR_DATA * keeper, OBJ_DATA * obj, bool fBuy);
void obj_to_keeper(OBJ_DATA * obj, CHAR_DATA * ch);
OBJ_DATA *get_obj_keeper(CHAR_DATA * ch, CHAR_DATA * keeper, char *argument);
void talk_auction(char *argument);
bool spell_in_group(int sn, char *group);
extern AUCTION_DATA *auction;
int trap_symbol(CHAR_DATA *victim, OBJ_DATA *obj);

void paint_tattoo(CHAR_DATA *ch, OBJ_DATA *tattoo, int class, int recursive);
void finalize_tattoo(OBJ_DATA *tattoo);
int tattoo_lookup(AFFECT_DATA *);

// Clanspells
void clanspell_general(int, int, CHAR_DATA *);
DECLARE_SPELL_FUN(clanspell_paradox);

/* RT part of the corpse looting code */

bool
can_loot(CHAR_DATA * ch, OBJ_DATA * obj)
{
	CHAR_DATA *owner, *wch;

	/* Is the character immortal? */
	if (IS_IMMORTAL(ch))
		return TRUE;

	/* does the object have an owner? */
	if (!obj->owner)
		return TRUE;

	/* can always loot mobs */
	if (obj->owner_vnum)
		return TRUE;

	owner = NULL;
	for (wch = char_list; wch != NULL; wch = wch->next)
		if (!str_cmp(wch->name, obj->owner))
			owner = wch;

	/* true if the owner has logged off or is an NPC */
	if (owner == NULL)
		return TRUE;

	/* can't if it's a nonclanner trying to take a clanner corpse */
	if ((obj->item_type == ITEM_CORPSE_PC)
		&& !is_clan(ch)
		&& is_clan(owner))
		return FALSE;

	/* interferes with voodoo body parts
	if (!str_cmp(ch->name, owner->name))
		return TRUE;
	*/

	/* If the owner has no loot off, go ahead */
	if (!IS_NPC(owner) && IS_SET(owner->act, PLR_CANLOOT))
		return TRUE;

	/* Of course, can loot from your own group... */
	if (is_same_group(ch, owner))
		return TRUE;

	/* clanners can always loot from clanners */
	if (is_clan(ch) && is_clan(owner))
		return TRUE;

	return FALSE;
}

void
do_noauction(CHAR_DATA * ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_NOAUCTION))
	{
		Cprintf(ch, "Auction channel is now ON.\n\r");
		REMOVE_BIT(ch->comm, COMM_NOAUCTION);
	}
	else
	{
		Cprintf(ch, "Auction channel is now OFF.\n\r");
		SET_BIT(ch->comm, COMM_NOAUCTION);
	}
}


void
do_drag(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj;
	ROOM_INDEX_DATA *temp_room;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	int diff, skill;

	skill = get_skill(ch, gsn_drag);

	if (skill < 1)
	{
		Cprintf(ch, "Dragging? What's that?\n\r");
		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Drag what?\n\r");
		return;
	}

	if (arg2[0] == '\0')
	{
		Cprintf(ch, "Which way do you want to haul your stuff?\n\r");
		return;
	}

	obj = get_obj_list(ch, arg1, ch->in_room->contents);
	if (obj == NULL)
	{
		Cprintf(ch, "I don't see that here.\n\r");
		return;
	}

	if ((obj->item_type != ITEM_CORPSE_PC) && (obj->item_type != ITEM_CORPSE_NPC))
	{
		Cprintf(ch, "You can only drag corpses around.\n\r");
		return;
	}

        if (!can_loot(ch, obj))
	{
		Cprintf(ch, "Dragging a non-clanner's corpse seems like a BAD idea.\n\r");
		return;
	}

	if (number_percent() > skill)
	{
		Cprintf(ch, "You don't seem able to drag the corpse.\n\r");
		check_improve(ch, gsn_drag, FALSE, 4);
		return;
	}

	diff = ch->move;
	temp_room = ch->in_room;

	if (!str_prefix(arg2, "north"))
	{
		Cprintf(ch, "You drag %s to the north.\n\r", obj->short_descr);
		move_char(ch, DIR_NORTH, FALSE);
	}
	else if (!str_prefix(arg2, "south"))
	{
		Cprintf(ch, "You drag %s to the south.\n\r", obj->short_descr);
		move_char(ch, DIR_SOUTH, FALSE);
	}
	else if (!str_prefix(arg2, "east"))
	{
		Cprintf(ch, "You drag %s to the east.\n\r", obj->short_descr);
		move_char(ch, DIR_EAST, FALSE);
	}
	else if (!str_prefix(arg2, "west"))
	{
		Cprintf(ch, "You drag %s to the west.\n\r", obj->short_descr);
		move_char(ch, DIR_WEST, FALSE);
	}
	else if (!str_prefix(arg2, "up"))
	{
		Cprintf(ch, "You drag %s upwards.\n\r", obj->short_descr);
		move_char(ch, DIR_UP, FALSE);
	}
	else if (!str_prefix(arg2, "down"))
	{
		Cprintf(ch, "You drag %s downwards.\n\r", obj->short_descr);
		move_char(ch, DIR_DOWN, FALSE);
	}
	else
	{
		Cprintf(ch, "That's not a valid direction.\n\r");
		return;
	}

	if (ch->in_room != temp_room)
	{
		obj_from_room(obj);
		obj_to_room(obj, ch->in_room);

		check_improve(ch, gsn_drag, TRUE, 5);
	}

	if (ch->move > 0)
	{
		diff -= ch->move;
		ch->move -= diff;
		if (obj->item_type == ITEM_CORPSE_PC)
			ch->move -= diff;
	}

	if (ch->move < 0)
		ch->move = 0;

	return;
}


/* add gold_bet and silver_bet do auc data */

/* put an item on auction, or see the stats on the current item or bet */
void
do_auction(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj;
	char arg1[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int found = FALSE;

	if (IS_SET(ch->comm, COMM_NOAUCTION))
	{
		Cprintf(ch, "Auction channel is now ON.\n\r");
		REMOVE_BIT(ch->comm, COMM_NOAUCTION);
	}

	argument = one_argument(argument, arg1);

	if (IS_NPC(ch))            /* NPC can be extracted at any time and thus can't auction! */
		return;

	if (ch->level == 54)
	{
		Cprintf(ch, "Builders cannot use the auction system.\n\r");
		return;
	}

	if (arg1[0] == '\0')
	{
		if (auction->item != NULL)
		{
			/* show item data here */
			if (auction->bet > 0)
				sprintf(buf, "Current bid on this item is %d gold.\n\r", auction->bet);
			else
				sprintf(buf, "No bids on this item have been received.\n\r");
			Cprintf(ch, "%s", buf);
			Cprintf(ch, "Item: %s  Type: %s  Level: %d  Value: %d\n\r", auction->item->short_descr, item_name(auction->item->item_type), auction->item->level, auction->item->cost);

			Cprintf(ch, "Restrictions: ");
			if(IS_SET(auction->item->extra_flags, ITEM_WARRIOR)) {
				Cprintf(ch, "Warrior "); found = TRUE;
			}
			if(IS_SET(auction->item->extra_flags, ITEM_THIEF)) {
				Cprintf(ch, "Thief "); found = TRUE;
			}
			if(IS_SET(auction->item->extra_flags, ITEM_CLERIC)) {
				Cprintf(ch, "Cleric "); found = TRUE;
			}
			if(IS_SET(auction->item->extra_flags, ITEM_MAGE)) {
				Cprintf(ch, "Mage "); found = TRUE;
			}
			if(IS_SET(auction->item->wear_flags, ITEM_NEWBIE)) {
				Cprintf(ch, "Newbie "); found = TRUE;
			}
			if(obj_is_affected(auction->item, gsn_wizard_mark)) {
				Cprintf(ch, "Runic "); found = TRUE;
			}
			if(auction->item->clan_status == CS_CLANNER) {
				Cprintf(ch, "Clanner "); found = TRUE;
			}
			if(auction->item->clan_status == CS_NONCLANNER) {
				Cprintf(ch, "Non-Clanner "); found = TRUE;
			}
			if(!found) {
				Cprintf(ch, "None");
			}
			Cprintf(ch, "\n\r");
			return;
		}
		else
		{
			Cprintf(ch, "Auction WHAT?\n\r");
			return;
		}
	}

	if (IS_IMMORTAL(ch) && !str_cmp(arg1, "stop"))
	{
		if (auction->item == NULL)
		{
			Cprintf(ch, "There is no auction going on you can stop.\n\r");
			return;
		}
		else
			/* stop the auction */
		{
			sprintf(buf, "Sale of %s has been stopped by God. Item confiscated.", auction->item->short_descr);
			talk_auction(buf);
			obj_to_char(auction->item, ch);
			auction->item = NULL;

			return;
		}
	}

	if (!str_cmp(arg1, "bet") || !str_cmp(arg1, "bid"))
	{
		if (auction->item != NULL)
		{
			int newbet;

			/* make - perhaps - a bet now */
			if (argument[0] == '\0')
			{
				Cprintf(ch, "Bid how much?\n\r");
				return;
			}

			if(is_number(argument)) {
				newbet = atoi(argument);
			}
			else {
				Cprintf(ch, "Bid how much?\n\r");
				return;
			}
			// newbet = parsebet(auction->bet, argument);

			if (newbet < (auction->bet + 1))
			{
				Cprintf(ch, "You must at least bid 1 gold over the current bit.\n\r");
				return;
			}

			if (newbet > ch->gold + (ch->silver / 100))
			{
				Cprintf(ch, "You don't have that much money!\n\r");
				return;
			}

			/* the actual bet is OK! */
			/* stop the putzes who drop their cash! */
			/* return last bet */
			if (auction->buyer)
			{
				auction->buyer->gold += auction->bet_gold;
				auction->buyer->silver += auction->bet_silver;
			}

			auction->buyer = ch;
			auction->bet = newbet;
			auction->going = 0;
			auction->bet_gold = 0;
			auction->bet_silver = 0;
			auction->pulse = PULSE_AUCTION;     /* start the auction over again */

			sprintf(buf, "A bid of %d gold has been received on %s.\n\r", newbet, auction->item->short_descr);

			/* take new bet */
			if (auction->buyer->gold < auction->bet)
			{
				/* take all gold, and add silver */
				auction->bet_gold = auction->buyer->gold;
				auction->buyer->gold = 0;

				auction->buyer->silver -= (auction->bet - auction->bet_gold) * 100;
				auction->bet_silver = (auction->bet - auction->bet_gold) * 100;
			}
			else
			{
				auction->buyer->gold -= auction->bet;
				auction->bet_gold += auction->bet;
				auction->bet_silver = 0;
			}

			talk_auction(buf);
			return;
		}
		else
		{
			Cprintf(ch, "There isn't anything being auctioned right now.\n\r");
			return;
		}
	}

	/* finally... */

	obj = get_obj_carry(ch, arg1, ch);  /* does char have the item ? */

	if (obj == NULL)
	{
		Cprintf(ch, "You aren't carrying that.\n\r");
		return;
	}

	if (IS_SET(obj->extra_flags, ITEM_ROT_DEATH) || obj->timer > 0 || IS_SET(obj->extra_flags, ITEM_NODROP))
	{
		Cprintf(ch, "You can't auction non-permanent objects.\n\r");
		return;
	}

	if (auction->item == NULL)
		switch (obj->item_type)
		{
		default:
			Cprintf(ch, "You cannot auction this item.\n\r");
			return;

		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_STAFF:
		case ITEM_WAND:
		case ITEM_SCROLL:
		case ITEM_JEWELRY:
		case ITEM_PILL:
		case ITEM_GEM:
		case ITEM_CONTAINER:
		case ITEM_DRINK_CON:
		case ITEM_BOAT:
		case ITEM_LIGHT:
		case ITEM_TREASURE:
		case ITEM_POTION:
		case ITEM_CLOTHING:
		case ITEM_THROWING:
			obj_from_char(obj);
			auction->item = obj;
			auction->bet = 0;
			auction->buyer = NULL;
			auction->seller = ch;
			auction->pulse = PULSE_AUCTION;
			auction->going = 0;

			sprintf(buf, "A new item has been received: %s.", obj->short_descr);
			talk_auction(buf);

			return;
		}                 /* switch */
	else
	{
		act("Try again later - $p is being auctioned right now!", ch, auction->item, NULL, TO_CHAR, POS_RESTING);
		return;
	}
}


void
do_gift(CHAR_DATA * ch, char *argument)
{

	char arg1[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg1);


	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Auto-gift disabled.\n\r");
		ch->gift = 0;
		return;
	}

	if (atoi(arg1) > 0 && atoi(arg1) < 32000)
	{
		ch->gift = atoi(arg1);
		Cprintf(ch, "Levelling gift set.\n\r");
		return;
	}

	Cprintf(ch, "Vnum must bee between 1 and 32000.\n\r");
	return;

	return;
}


 /* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 @+  .------------. .------------. .------..------. SOH (South OF HEAVEN)  @
	 @ / |+- -         \|+- --        \|+-    ||      | +--------------------+ @
	 @ | ||             |:             ||     ||      | The following code is  @
	 @ | |:             |.             |:     ||      | created to function    @
	 @ | |.      _______|  _____.      |.     ||      | primarily on SOH MUD   @
	 @ | |              |      ||      |      ||      | tho it can be modified @
	 @ | |______        |      ||      |      :|      | to work on most Diku,  @
	 @ |/|+-    |       |      ||      |      .:      | Merc, Envy, ROM MUDs   @
	 @ | |:     |       |      ||      |              | +--------------------+ @
	 @ | |      |       |      ||      |       .      | All these functions    @
	 @ | |      |       |      :|      |      :|      | by whiplash@armory.com @
	 @ | |      |       |      .|      |      ||      | and may not be used    @
	 @ | |      :       |       :      |      ||      | unless credit is given @
	 @ | |              |              |      ||      | somewhere in your MUD  @
	 @ | |______________|______________:______||______| that all can see.      @
	 @ |/______________/_______________/_____//______/  (ie: HELP SOH, etc)    @
	 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
void
do_butcher(CHAR_DATA * ch, char *argument)
{

	/* Butcher skill, created by Argawal */
	/* Original Idea taken fom Carrion Fields Mud */
	/* If you have an interest in this skill, feel free */
	/* to use it in your mud if you so desire. */
	/* All I ask is that Argawal is credited with creating */
	/* this skill, as I wrote it from scratch. */

	char arg[MAX_STRING_LENGTH];
	int numst = 0, counter;
	OBJ_DATA *steak;
	OBJ_DATA *obj;

	one_argument(argument, arg);

	if (get_skill(ch, gsn_butcher) < 1)
	{
		Cprintf(ch, "Butchering is beyond your skills.\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Butcher what?\n\r");
		return;
	}

	obj = get_obj_list(ch, arg, ch->in_room->contents);
	if (obj == NULL)
	{
		Cprintf(ch, "It's not here.\n\r");
		return;
	}


	if (obj->pIndexData->vnum != OBJ_VNUM_TORN_HEART
		&& obj->pIndexData->vnum != OBJ_VNUM_SLICED_ARM
		&& obj->pIndexData->vnum != OBJ_VNUM_SLICED_LEG
		&& obj->pIndexData->vnum != OBJ_VNUM_GUTS
		&& obj->pIndexData->vnum != OBJ_VNUM_BRAINS
		&& obj->pIndexData->vnum != OBJ_VNUM_TAIL
		&& obj->pIndexData->vnum != OBJ_VNUM_WING)
	{
		Cprintf(ch, "You can only butcher body parts.\n\r");
		return;
	}

	/* updated into a tidy loop by Starcrossed!
		make some steaks baby! */
	if (number_percent() < get_skill(ch, gsn_butcher))
	{
		numst = number_range(1, 4);
		for (counter = 0; counter < numst; counter++)
		{
			steak = create_object(get_obj_index(OBJ_VNUM_STEAK), 0);
			/* if part was poisoned, steaks are too */
			steak->value[3] = obj->value[3];
			obj_to_room(steak, ch->in_room);
		}
		if (numst == 1)
			Cprintf(ch, "You butcher %s and create %d steak.\n\r", obj->short_descr, numst);
		else
			Cprintf(ch, "You butcher %s and create %d steaks.\n\r", obj->short_descr, numst);

		act("$n calmly butchers $p into some steaks.", ch, obj, NULL, TO_ROOM, POS_RESTING);

		check_improve(ch, gsn_butcher, TRUE, 4);
	}
	else
	{
		act("$n fails to butcher $p, and destroys it.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You fail to butcher $p, and destroy it.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		check_improve(ch, gsn_butcher, FALSE, 1);
	}

	extract_obj(obj);
	return;
}


void
do_taste(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj;
	int which = 0;
	int liquid = 0;
	char *flavor;
	int chance;

	flavor = "Inform Delstar this is broken.";

	if (get_skill(ch, gsn_taste) < 1)
	{
		Cprintf(ch, "Tasting? Grow some taste buds first, eh?\n\r");
		return;
	}

	if ((obj = get_obj_here(ch, argument)) == NULL)
	{
		Cprintf(ch, "You don't have that.\n\r");
		return;
	}

	chance = get_skill(ch, gsn_taste) / 2;

	if (number_percent() > chance)
	{
		Cprintf(ch, "You can't seem to taste anything.\n\r");
		check_improve(ch, gsn_taste, FALSE, 6);
		return;
	}

	switch (obj->item_type)
	{
	default:
		Cprintf(ch, "Thats not something you'd want to put in your mouth.\n\r");
		break;

	case ITEM_POTION:
		Cprintf(ch, "Potions cannot be tasted, you'll have to quaff it.\n\r");
		break;

	case ITEM_FOOD:
		act("You take a small bite of $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$n takes a small bite of $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);

		if (obj->value[3] != 0) /* poison */
		{
			which = number_range(1, 3);
			if (which == 1)
				Cprintf(ch, "A bitter taste floods your mouth!\n\r");
			else if (which == 2)
				Cprintf(ch, "An acidic taste fills your mouth!\n\r");
			else
				Cprintf(ch, "A disgusting taste fills your mouth!\n\r");
			act("$p is poisoned!", ch, obj, NULL, TO_CHAR, POS_RESTING);
			act("$n spits out a bite of poison $p!", ch, obj, NULL, TO_ROOM, POS_RESTING);
			return;
		}

		act("$p tastes fine to you.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		break;

	case ITEM_DRINK_CON:
	case ITEM_FOUNTAIN:     /* this one is a bit tougher. */
		if (obj->value[1] <= 1)
		{
			Cprintf(ch, "There's nothing in that to taste.\n\r");
			return;
		}

		liquid = obj->value[2];
		act("You take a sip of $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_CHAR, POS_RESTING);
		act("$n takes a sip of $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_ROOM, POS_RESTING);

		if (obj->value[3] != 0)
		{
			which = number_range(1, 5);
			switch (which)
			{
			case 1:
				flavor = "bitter";
				break;
			case 2:
				flavor = "acidic";
				break;
			case 3:
				flavor = "spoiled";
				break;
			case 4:
				flavor = "rotten";
				break;
			case 5:
				flavor = "tart";
				break;
			default:
				break;
			}

			Cprintf(ch, "The %s has a %s flavor to it.", liq_table[liquid].liq_name, flavor);
			return;
		}

		act("$p tastes fine to you.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		break;
	}
	return;
}


void
get_obj(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * container)
{
	/* variables for AUTOSPLIT */
	CHAR_DATA *gch;
	DESCRIPTOR_DATA *d;
	int members;
	char buffer[100];

	if (!CAN_WEAR(obj, ITEM_TAKE))
	{
		Cprintf(ch, "You can't take that.\n\r");
		return;
	}

	if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
	{
		act("$d: you can't carry that many items.", ch, NULL, obj->name, TO_CHAR, POS_RESTING);
		return;
	}

	if ((!obj->in_obj || obj->in_obj->carried_by != ch) && (get_carry_weight(ch) + get_obj_weight(obj) > can_carry_w(ch)))
	{
		act("$d: you can't carry that much weight.", ch, NULL, obj->name, TO_CHAR, POS_RESTING);
		return;
	}

	if (!can_loot(ch, obj))
	{
		act("Corpse looting is not permitted.", ch, NULL, NULL, TO_CHAR, POS_RESTING);
		return;
	}

	if (obj->in_room != NULL)
	{
		for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
			if (gch->on == obj)
			{
				act("$N appears to be using $p.", ch, obj, gch, TO_CHAR, POS_RESTING);
				return;
			}
	}

	if (container != NULL)
	{
		if (container->pIndexData->vnum == OBJ_VNUM_PIT && get_trust(ch) < obj->level)
		{
			Cprintf(ch, "You are not powerful enough to use it.\n\r");
			return;
		}

		if (container->pIndexData->vnum == OBJ_VNUM_PIT && !CAN_WEAR(container, ITEM_TAKE) && !IS_OBJ_STAT(obj, ITEM_HAD_TIMER))
			obj->timer = 0;

		act("You get $p from $P.", ch, obj, container, TO_CHAR, POS_RESTING);
		act("$n gets $p from $P.", ch, obj, container, TO_ROOM, POS_RESTING);
		REMOVE_BIT(obj->extra_flags, ITEM_HAD_TIMER);
		obj_from_obj(obj);
	}
	else
	{
		act("You get $p.", ch, obj, container, TO_CHAR, POS_RESTING);
		act("$n gets $p.", ch, obj, container, TO_ROOM, POS_RESTING);
		obj_from_room(obj);
	}

	if (obj->item_type == ITEM_MONEY)
	{
		if(ch->reclass == reclass_lookup("templar")) {
			obj->value[0] = obj->value[0] * 9/10;
			obj->value[1] = obj->value[1] * 9/10;
			Cprintf(ch, "You tithe some of the gold.\n\r");
		}
		ch->silver += obj->value[0];
		ch->gold += obj->value[1];
		if (IS_SET(ch->act, PLR_AUTOSPLIT))
		{                 /* AUTOSPLIT code */
			members = 0;
			for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
			{
				if (!IS_AFFECTED(gch, AFF_CHARM) && is_same_group(gch, ch))
					members++;
			}

			if (members > 1 && (obj->value[0] > 1 || obj->value[1]))
			{
				sprintf(buffer, "%d %d", obj->value[0], obj->value[1]);
				do_split(ch, buffer);
			}
		}

		extract_obj(obj);
	}
	else if (!IS_NPC(ch) && obj->pIndexData->vnum == OBJ_CTF_FLAG(ch->pcdata->ctf_team))
	{
		Cprintf(ch, "You found your team's flag. It has been returned to your base.\n\r");
		obj_to_room(obj, get_room_index(ROOM_CTF_FLAG(ch->pcdata->ctf_team)));
        	for (d = descriptor_list; d; d = d->next)
        	{
               		if (d->connected == CON_PLAYING && d->character->in_room != NULL && d->character->pcdata->ctf_team > 0)
                	{
				if (ch->pcdata->ctf_team == CTF_RED)
				{
                       	 		Cprintf(d->character, "The {RRed{x flag has been returned.\n\r", ch->name);
				}
				else
				{
                       	 		Cprintf(d->character, "The {BBlue{x flag has been returned.\n\r", ch->name);
				}
                	}
        	}
	}
	else if (!IS_NPC(ch) && obj->pIndexData->vnum == OBJ_CTF_FLAG(CTF_OTHER_TEAM(ch->pcdata->ctf_team)))
	{
		extract_obj(obj);
		ch->pcdata->ctf_flag = TRUE;
		Cprintf(ch, "You got the other team's flag. Quickly, bring it back to your base!\n\r");

        	for (d = descriptor_list; d; d = d->next)
        	{
               		if (d->connected == CON_PLAYING && d->character->in_room != NULL && d->character->pcdata->ctf_team > 0)
                	{
				if (ch->pcdata->ctf_team == CTF_RED)
				{
                       	 		Cprintf(d->character, "The {BBlue{x flag has been stolen.\n\r", ch->name);
				}
				else
				{
                       	 		Cprintf(d->character, "The {RRed{x flag has been stolen.\n\r", ch->name);
				}
                	}
        	}
	}
	else
	{
		obj_to_char(obj, ch);
	}

	return;
}


bool
has_bank_note(CHAR_DATA * ch)
{
	OBJ_DATA *obj;

	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	{
		if (obj->pIndexData->vnum == OBJ_VNUM_BANK_NOTE)
			return TRUE;
	}

	return FALSE;
}


OBJ_DATA *
find_bank_note(CHAR_DATA * ch, OBJ_DATA * obj)
{
	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
	{
		if (obj->pIndexData->vnum == OBJ_VNUM_BANK_NOTE)
			return obj;
	}

	return NULL;
}


void
do_atm(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	CHAR_DATA *banker;
	OBJ_DATA *obj = NULL;
	int cost = 0;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	for (banker = ch->in_room->people; banker != NULL; banker = banker->next_in_room)
		if (IS_NPC(banker) && IS_SET(banker->act, ACT_BANKER))
			break;

	if ((banker == NULL || !can_see(ch, banker)) && !IS_IMMORTAL(ch))
	{
		Cprintf(ch, "You can't do your banking here.\n\r");
		return;
	}

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "What banking do you wish to do? (open, close, get, put, balance)\n\r");
		return;
	}

	if (str_prefix(arg1, "open")
		&& str_prefix(arg1, "close")
		&& str_prefix(arg1, "get")
		&& str_prefix(arg1, "put")
		&& str_prefix(arg1, "balance")
		&& str_prefix(arg1, "withdraw")
		&& str_prefix(arg1, "deposit"))
	{
		Cprintf(ch, "The banking commands are open, close, get, put and balance.\n\r");
		return;
	}

	if (!has_bank_note(ch) && str_prefix(arg1, "open"))
	{
		Cprintf(ch, "You do not own a bank note.\n\r");
		return;
	}

	if (!str_prefix(arg1, "open"))
	{
		if ((ch->silver + 100 * ch->gold) < 1000)
		{
			Cprintf(ch, "Accounts cost 10 gold.  You don't have enough money.\n\r");
			return;
		}

		deduct_cost(ch, 1000);
		obj_to_char(create_object(get_obj_index(OBJ_VNUM_BANK_NOTE), 0), ch);
		Cprintf(ch, "Enjoy your bank note.\n\r");
		return;
	}

	obj = find_bank_note(ch, obj);

	if (!str_prefix(arg1, "close"))
	{
		ch->gold += obj->value[2] / 100;
		ch->silver += obj->value[2] - (obj->value[2] / 100) * 100;

		Cprintf(ch, "You withdraw %d Gold and %d Silver and close your account.\n\r", obj->value[2] / 100, obj->value[2] - (obj->value[2] / 100) * 100);

		extract_obj(obj);
		return;
	}

	if (!str_prefix(arg1, "get") || !str_prefix(arg1, "withdraw"))
	{
		if (arg2[0] == '\0')
		{
			Cprintf(ch, "How much do you wish to withdraw?\n\r");
			return;
		}

		cost = atoi(arg2);
		if (cost < 0)
		{
			Cprintf(ch, "Only positive numbers please.\n\r");
			return;
		}

		if (!str_prefix(arg3, "gold"))
			cost = cost * 100;

		if (obj->value[2] < cost)
		{
			Cprintf(ch, "There is not that much money on that bank note.\n\r");
			return;
		}

		obj->value[2] = obj->value[2] - cost;
		ch->gold += cost / 100;
		ch->silver += cost - (cost / 100) * 100;
		if (ch->gold < 0)
			ch->gold = 0;
		if (ch->silver < 0)
			ch->silver = 0;
		if (obj->value[2] < 0)
			obj->value[2] = 0;

		Cprintf(ch, "You withdraw %d Gold and %d Silver from your bank note.\n\r", cost / 100, cost - (cost / 100) * 100);

		return;
	}

	if (!str_prefix(arg1, "put") || !str_prefix(arg1, "deposit"))
	{
		if (arg2[0] == '\0')
		{
			Cprintf(ch, "How much do you wish to deposit?\n\r");
			return;
		}

		if (!str_prefix(arg2, "all"))
		{
			cost = ch->silver + 100 * ch->gold;
			deduct_cost(ch, cost);
			obj->value[2] = obj->value[2] + cost;
			if (ch->gold < 0)
				ch->gold = 0;
			if (ch->silver < 0)
				ch->silver = 0;
			if (obj->value[2] < 0)
				obj->value[2] = 0;

			Cprintf(ch, "You deposit %d Gold and %d Silver to your bank note.\n\r", cost / 100, cost - (cost / 100) * 100);
			return;
		}

		cost = atoi(arg2);
		if (cost < 0)
		{
			Cprintf(ch, "Only positive numbers please.\n\r");
			return;
		}

		if (!str_prefix(arg3, "gold"))
			cost = cost * 100;

		if ((ch->silver + 100 * ch->gold) < cost)
		{
			Cprintf(ch, "You do not have that much money to deposit.\n\r");
			return;
		}

		deduct_cost(ch, cost);
		obj->value[2] = obj->value[2] + cost;
		if (ch->gold < 0)
			ch->gold = 0;
		if (ch->silver < 0)
			ch->silver = 0;
		if (obj->value[2] < 0)
			obj->value[2] = 0;

		Cprintf(ch, "You deposit %d Gold and %d Silver to your bank note.\n\r", cost / 100, cost - (cost / 100) * 100);
		return;
	}

	if (!str_prefix(arg1, "balance"))
	{
		Cprintf(ch, "Your balance on this note is: %d Gold and %d Silver.\n\r", obj->value[2] / 100, obj->value[2] - (obj->value[2] / 100) * 100);
		return;
	}

	Cprintf(ch, "Banking commands have to be entered in full.\n\r");
}


void
do_get(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	char rest[MAX_INPUT_LENGTH];
	OBJ_DATA *leftover, *obj;
	OBJ_DATA *coins, *obj_next;
	OBJ_DATA *container;
	bool found;
	CHAR_DATA *owner, *wch;
	int amount;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (!str_cmp(arg2, "from"))
	{
		argument = one_argument(argument, arg2);
	}

	/* Get type. */
	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Get what?\n\r");
		return;
	}

	/* Take NNNN gold/silver */
	if (is_number(arg1))
	{
		bool silver=FALSE, gold=FALSE;

		/* How many coins do we want? */
		amount = atoi(arg1);
		if (amount <= 0 || (str_cmp(arg2, "gold") && str_cmp(arg2, "silver")))
		{
			Cprintf(ch, "Sorry, you can't do that.\n\r");
			return;
		}

		if (!str_cmp(arg2, "gold"))
			gold = TRUE;

		if (!str_cmp(arg2, "silver"))
			silver = TRUE;

		/* Get all the coins in the room */
		obj = get_obj_list(ch, arg2, ch->in_room->contents);
		if (obj == NULL || obj->item_type != ITEM_MONEY)
		{
			act("I see no coins of that sort here.", ch, NULL, arg2, TO_CHAR, POS_RESTING);
			return;
		}

		/* Now deal with the amount */
		/* Single coin */
		if((obj->pIndexData->vnum == OBJ_VNUM_SILVER_ONE && silver && amount == 1)
			|| (obj->pIndexData->vnum == OBJ_VNUM_GOLD_ONE && gold && amount == 1))
		{
			get_obj(ch, obj, NULL);
			return;
		}
		/* Some silver */
		else if(silver && obj->pIndexData->vnum == OBJ_VNUM_SILVER_SOME)
		{
			if(amount == obj->value[0])
			{
				get_obj(ch, obj, NULL);
				return;
			}

			if(amount < obj->value[0])
			{
				coins = create_money(0, amount);
				obj_to_room(coins, ch->in_room);
				get_obj(ch, coins, NULL);
				leftover = create_object(get_obj_index(obj->pIndexData->vnum), 0);
				sprintf(buf, leftover->short_descr, obj->value[0] - amount);
				free_string(leftover->short_descr);
				leftover->short_descr = str_dup(buf);
				leftover->value[0] = obj->value[0] - amount;
				leftover->value[1] = obj->value[1];
				leftover->cost = obj->value[0] - amount;
				leftover->weight = (obj->value[0] - amount) / 5;
				obj_to_room(leftover, ch->in_room);
				extract_obj(obj);
				return;
			}

			if(amount > obj->value[0])
			{
				act("There isn't that much silver here.", ch, NULL, NULL, TO_CHAR, POS_RESTING);
				return;
			}
		}
		/* Some gold */
		else if(gold && obj->pIndexData->vnum == OBJ_VNUM_GOLD_SOME)
		{
			if(amount == obj->value[1])
			{
				get_obj(ch, obj, NULL);
				return;
			}

			if(amount < obj->value[1])
			{
				coins = create_money(amount, 0);
				obj_to_room(coins, ch->in_room);
				get_obj(ch, coins, NULL);
				leftover = create_object(get_obj_index(obj->pIndexData->vnum), 0);
				sprintf(buf, leftover->short_descr, obj->value[1] - amount);
				free_string(leftover->short_descr);
				leftover->short_descr = str_dup(buf);
				leftover->value[0] = obj->value[0];
				leftover->value[1] = obj->value[1] - amount;
				leftover->cost = obj->value[1] - amount;
				leftover->weight = (obj->value[1] - amount) / 5;
				obj_to_room(leftover, ch->in_room);
				extract_obj(obj);
				return;
			}

			if(amount > obj->value[1])
			{
				act("There isn't that much gold here.", ch, NULL, NULL, TO_CHAR, POS_RESTING);
				return;
			}
		}
		else if(gold && obj->pIndexData->vnum == OBJ_VNUM_COINS)
		{
			if(amount == obj->value[1])
			{
				get_obj(ch, obj, NULL);
				return;
			}

			if(amount < obj->value[1])
			{
				coins = create_money(amount, 0);
				obj_to_room(coins, ch->in_room);
				get_obj(ch, coins, NULL);
				leftover = create_object(get_obj_index(obj->pIndexData->vnum), 0);
				sprintf(buf, leftover->short_descr, obj->value[0], obj->value[1] - amount);
				free_string(leftover->short_descr);
				leftover->short_descr = str_dup(buf);
				leftover->value[0] = obj->value[0];
				leftover->value[1] = obj->value[1] - amount;
				leftover->cost = 100 * (obj->value[1] - amount) + obj->value[0];
				leftover->weight = (obj->value[1] - amount) / 5 + obj->value[0] / 20;
				obj_to_room(leftover, ch->in_room);
				extract_obj(obj);
				return;
			}

			if(amount > obj->value[1])
			{
				act("There isn't that much gold here.", ch, NULL , NULL, TO_CHAR, POS_RESTING);
				return;
			}
		}
		else if(silver && obj->pIndexData->vnum == OBJ_VNUM_COINS)
		{
			if(amount == obj->value[0])
			{
				get_obj(ch, obj, NULL);
				return;
			}

			if(amount < obj->value[0])
			{
				coins = create_money(0, amount);
				obj_to_room(coins, ch->in_room);
				get_obj(ch, coins, NULL);
				leftover = create_object(get_obj_index(obj->pIndexData->vnum), 0);
				sprintf(buf, leftover->short_descr, obj->value[0] - amount, obj->value[1]);
				free_string(leftover->short_descr);
				leftover->short_descr = str_dup(buf);
				leftover->value[0] = obj->value[0] - amount;
				leftover->value[1] = obj->value[1];
				leftover->cost = 100 * (obj->value[1]) + (obj->value[0] - amount);
				leftover->weight = (obj->value[1]) / 5 + (obj->value[0] - amount) / 20;
				obj_to_room(leftover, ch->in_room);
				extract_obj(obj);
				return;
			}

			if(amount > obj->value[1])
			{
				act("There isn't that much gold here.", ch, NULL, NULL, TO_CHAR, POS_RESTING);
				return;
			}
		}
	}

	if (arg2[0] == '\0')
	{
		int number = mult_argument(arg1, rest);

		if (str_cmp(rest, "all") && str_prefix("all.", rest))
		{
			/* 'get obj'  or  'get x*obj' */
			int foundCount, i;
			bool stillGood = TRUE;

			if (number == 0)
			{
				Cprintf(ch, "You waste your time by doing nothing.\n\r");
				return;
			}

			if (number < 0)
			{
				Cprintf(ch, "Now, that's just stupid.\n\r");
				return;
			}

			/* Need the object to get the OBJ_INDEX_DATA for count_obj_list(),
			 * so let's start with a simple existence test.
			 */
			foundCount = count_obj_list_by_name(ch, rest, ch->in_room->contents);

			if (foundCount <= 0)
			{
				Cprintf(ch, "I see no %s here.\n\r", rest);
				return;
			}

			if (foundCount < number)
			{
				Cprintf(ch, "I don't see %d %s's here.\n\r", number, rest);
				return;
			}

			for (i = 0; stillGood && i < number; i++)
			{
				obj = get_obj_list(ch, rest, ch->in_room->contents);

				if (obj == NULL)
				{
					Cprintf(ch, "You cannot get that item.\n\r");
					stillGood = FALSE;
				}

				for (wch = char_list; wch != NULL; wch = wch->next)
				{
					if (obj->owner != NULL && (!str_cmp(wch->name, obj->owner)))
					{
						owner = wch;
					}
				}

				if (owner != NULL)
				{
					if ((IS_NPC(ch)) || (obj->item_type == ITEM_CORPSE_PC && !is_clan(ch) && is_clan(owner)))
					{
						Cprintf(ch, "You cannot take the corpse of a clan member.\n\r");
						stillGood = FALSE;
					}
				}

				if (stillGood)
				{
					get_obj(ch, obj, NULL);

					if (trap_symbol(ch, obj))
					{
						break;
					}
				}
			}
		}
		else
		{
			/* 'get all' or 'get all.obj' */
			found = FALSE;
			for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
			{
				obj_next = obj->next_content;
				if ((arg1[3] == '\0' || is_name(&rest[4], obj->name)) && can_see_obj(ch, obj))
				{
					found = TRUE;
					trap_symbol(ch, obj);
					get_obj(ch, obj, NULL);
				}
			}

			if (!found)
			{
				if (arg1[3] == '\0')
				{
					Cprintf(ch, "I see nothing here.\n\r");
				}
				else
				{
					act("I see no $T here.", ch, NULL, &rest[4], TO_CHAR, POS_RESTING);
				}
			}
		}
	}
	else
	{
		/* 'get ... container' */
		if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2))
		{
			Cprintf(ch, "You can't do that.\n\r");
			return;
		}

		if ((container = get_obj_here(ch, arg2)) == NULL)
		{
			act("I see no $T here.", ch, NULL, arg2, TO_CHAR, POS_RESTING);
			return;
		}

		switch (container->item_type)
		{
			default:
				Cprintf(ch, "That's not a container.\n\r");
				return;

			case ITEM_SHEATH:
				Cprintf(ch, "You must 'draw' to get things from your weapon sheath.\n\r");
				return;

			case ITEM_CONTAINER:
			case ITEM_CORPSE_NPC:
				break;

			case ITEM_FURNITURE:
				if (IS_SET(container->value[2], PUT_ON))
				{
					break;
				}
				else
				{
					Cprintf(ch, "You can't get things from that.\n\r");
					return;
				}

			case ITEM_CORPSE_PC:
				if (!can_loot(ch, container))
				{
					Cprintf(ch, "You can't do that.\n\r");
					return;
				}
		}

		if (container->item_type == ITEM_CONTAINER && IS_SET(container->value[1], CONT_CLOSED))
		{
			act("The $d is closed.", ch, NULL, container->name, TO_CHAR, POS_RESTING);
			return;
		}

		if (str_cmp(arg1, "all") && str_prefix("all.", arg1))
		{
			int i;
			/* 'get obj container' or 'get x*obj container' */
			int number = mult_argument(arg1, rest);
			int foundCount = count_obj_list_by_name(ch, rest, container->contains);

			if (foundCount <= 0)
			{
				Cprintf(ch, "I don't see any %s in the %s.\n\r", rest, arg2);
				return;
			}

			if (foundCount < number)
			{
				Cprintf(ch, "I don't see %d %s's in the %s.\n\r", number, rest, arg2);
				return;
			}

         if (number == 0)
         {
            Cprintf(ch, "You waste your time by doing nothing.\n\r");
            return;
         }

         if (number < 0)
         {
            Cprintf(ch, "Now, that's just stupid.\n\r");
            return;
         }

			for (i = 0; i < number; i++)
			{
				obj = get_obj_list(ch, rest, container->contains);
				get_obj(ch, obj, container);
				if (trap_symbol(ch, obj))
				{
					break;
				}
			}
		}
		else
		{
			/* 'get all container' or 'get all.obj container' */
			found = FALSE;
			for (obj = container->contains; obj != NULL; obj = obj_next)
			{
				obj_next = obj->next_content;
				if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name)) && can_see_obj(ch, obj))
				{
					found = TRUE;
					if (container->pIndexData->vnum == OBJ_VNUM_PIT && !IS_IMMORTAL(ch))
					{
						Cprintf(ch, "Don't be so greedy!\n\r");
						return;
					}

					trap_symbol(ch, obj);
					get_obj(ch, obj, container);
				}
			}

			if (!found)
			{
				if (arg1[3] == '\0')
				{
					act("I see nothing in the $T.", ch, NULL, arg2, TO_CHAR, POS_RESTING);
				}
				else
				{
					act("I see nothing like that in the $T.", ch, NULL, arg2, TO_CHAR, POS_RESTING);
				}
			}
		}
	}

	return;
}


void
do_put(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char rest[MAX_INPUT_LENGTH];
	int number, i;
	OBJ_DATA *container;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (!str_cmp(arg2, "in") || !str_cmp(arg2, "on"))
		argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Put what in what?\n\r");
		return;
	}

	if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2))
	{
		Cprintf(ch, "You can't do that.\n\r");
		return;
	}

	if ((container = get_obj_here(ch, arg2)) == NULL)
	{
		act("I see no $T here.", ch, NULL, arg2, TO_CHAR, POS_RESTING);
		return;
	}

	if (container->item_type == ITEM_CONTAINER && IS_SET(container->value[1], CONT_CLOSED))
	{
		act("The $d is closed.", ch, NULL, container->name, TO_CHAR, POS_RESTING);
		return;
	}

	if (container->item_type == ITEM_SHEATH) {
		Cprintf(ch, "You must use 'sheath' to put things into your weapon sheath.\n\r");
		return;
	}

	if (container->item_type != ITEM_CONTAINER &&
		!(container->item_type == ITEM_FURNITURE &&
		  IS_SET(container->value[2], PUT_ON)))
	{
		act("That is not a container", ch, NULL, NULL, TO_CHAR, POS_RESTING);
		return;
	}

	number = mult_argument(arg1, rest);

	if (number == 0)
	{
		Cprintf(ch, "You waste your time by doing nothing.\n\r");
		return;
	}

	if (number < 0)
	{
		Cprintf(ch, "Now, that's just stupid.\n\r");
		return;
	}

	if (str_cmp(rest, "all") && str_prefix("all.", rest))
	{
		/* 'put obj container' or 'put x*obj container' */
		int foundCount = count_obj_carry_by_name(ch, rest, ch);

		if (foundCount <= 0)
		{
			Cprintf(ch, "You do not have that item.\n\r");
			return;
		}

		if (foundCount < number)
		{
			Cprintf(ch, "You don't have %d %s's.\n\r", number, rest);
			return;
		}

		for (i = 0; i < number; i++)
		{
			obj = get_obj_carry(ch, rest, ch);

			if (obj == container)
			{
				Cprintf(ch, "You can't fold it into itself.\n\r");
				break;
			}

			if (!can_drop_obj(ch, obj))
			{
				Cprintf(ch, "You can't let go of it.\n\r");
				break;
			}

			if (container->item_type == ITEM_CONTAINER)
			{
				if (WEIGHT_MULT(obj) != 100)
				{
					Cprintf(ch, "You have a feeling that would be a bad idea.\n\r");
					break;
				}

				if (get_obj_weight(obj) + get_true_weight(container) > (container->value[0] * 10) || get_obj_weight(obj) > (container->value[3] * 10))
				{
					Cprintf(ch, "It won't fit.\n\r");
					break;
				}
			}

			if (container->pIndexData->vnum == OBJ_VNUM_PIT && !CAN_WEAR(container, ITEM_TAKE))
			{
				if (obj->timer)
					SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
				else
					obj->timer = number_range(100, 200);
			}

			obj_from_char(obj);
			obj_to_obj(obj, container);

			if ((container->item_type == ITEM_CONTAINER && IS_SET(container->value[1], CONT_PUT_ON)) ||
				(container->item_type == ITEM_FURNITURE && IS_SET(container->value[2], PUT_ON)))
			{
				act("$n puts $p on $P.", ch, obj, container, TO_ROOM, POS_RESTING);
				act("You put $p on $P.", ch, obj, container, TO_CHAR, POS_RESTING);
			}
			else
			{
				act("$n puts $p in $P.", ch, obj, container, TO_ROOM, POS_RESTING);
				act("You put $p in $P.", ch, obj, container, TO_CHAR, POS_RESTING);
			}
		}
	}
	else
	{
		/* 'put all container' or 'put all.obj container' */
		for (obj = ch->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;

			if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name))
				&& can_see_obj(ch, obj)
				&& WEIGHT_MULT(obj) == 100
				&& obj->wear_loc == WEAR_NONE
				&& obj != container
				&& can_drop_obj(ch, obj)
				&& get_obj_weight(obj) + get_true_weight(container) <= (container->value[0] * 10)
				&& get_obj_weight(obj) <= (container->value[3] * 10))
			{
				if (container->pIndexData->vnum == OBJ_VNUM_PIT && !CAN_WEAR(obj, ITEM_TAKE))
				{
					if (obj->timer)
						SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
					else
						obj->timer = number_range(100, 200);
				}

				obj_from_char(obj);
				obj_to_obj(obj, container);

				if ((container->item_type == ITEM_CONTAINER && IS_SET(container->value[1], CONT_PUT_ON)) ||
					(container->item_type == ITEM_FURNITURE && IS_SET(container->value[2], PUT_ON)))
				{
					act("$n puts $p on $P.", ch, obj, container, TO_ROOM, POS_RESTING);
					act("You put $p on $P.", ch, obj, container, TO_CHAR, POS_RESTING);
				}
				else
				{
					act("$n puts $p in $P.", ch, obj, container, TO_ROOM, POS_RESTING);
					act("You put $p in $P.", ch, obj, container, TO_CHAR, POS_RESTING);
				}
			}
		}
	}

	return;
}


void
do_drop(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	bool found;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Drop what?\n\r");
		return;
	}

	if (is_number(arg))
	{
		/* 'drop NNNN coins' */
		int amount, gold = 0, silver = 0;

		amount = atoi(arg);
		argument = one_argument(argument, arg);
		if (amount <= 0 || (str_cmp(arg, "coins") && str_cmp(arg, "coin") && str_cmp(arg, "gold") && str_cmp(arg, "silver")))
		{
			Cprintf(ch, "Sorry, you can't do that.\n\r");
			return;
		}

		if (!str_cmp(arg, "coins") || !str_cmp(arg, "coin") || !str_cmp(arg, "silver"))
		{
			if (ch->silver < amount)
			{
				Cprintf(ch, "You don't have that much silver.\n\r");
				return;
			}

			ch->silver -= amount;
			silver = amount;
		}
		else
		{
			if (ch->gold < amount)
			{
				Cprintf(ch, "You don't have that much gold.\n\r");
				return;
			}

			ch->gold -= amount;
			gold = amount;
		}

		for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;

			switch (obj->pIndexData->vnum)
			{
				case OBJ_VNUM_SILVER_ONE:
					silver += 1;
					extract_obj(obj);
					break;

				case OBJ_VNUM_GOLD_ONE:
					gold += 1;
					extract_obj(obj);
					break;

				case OBJ_VNUM_SILVER_SOME:
					silver += obj->value[0];
					extract_obj(obj);
					break;

				case OBJ_VNUM_GOLD_SOME:
					gold += obj->value[1];
					extract_obj(obj);
					break;

				case OBJ_VNUM_COINS:
					silver += obj->value[0];
					gold += obj->value[1];
					extract_obj(obj);
					break;
			}
		}

		obj_to_room(create_money(gold, silver), ch->in_room);
		act("$n drops some coins.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "OK.\n\r");
		return;
	}

	if (str_cmp(arg, "all") && str_prefix("all.", arg))
	{
		/* 'drop obj' or 'drop x*obj' */
		char rest[MAX_INPUT_LENGTH];
		int number = mult_argument(arg, rest);
		int i;
		int foundCount = count_obj_carry_by_name(ch, rest, ch);

		if (foundCount <= 0)
		{
			Cprintf(ch, "You do not have any %s.\n\r", rest);
			return;
		}

		if (foundCount < number)
		{
			Cprintf(ch, "You don't have %d %s's.\n\r", number, rest);
			return;
		}

			if (number == 0)
			{
				Cprintf(ch, "You waste your time by doing nothing.\n\r");
				return;
			}

			if (number < 0)
			{
				Cprintf(ch, "Now, that's just stupid.\n\r");
				return;
			}

		for (i = 0; i < number; i++)
		{
			obj = get_obj_carry(ch, rest, ch);

			if (!can_drop_obj(ch, obj))
			{
				Cprintf(ch, "You can't let go of it.\n\r");
				return;
			}

			obj_from_char(obj);
			obj_to_room(obj, ch->in_room);

			act("$n drops $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You drop $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			if (IS_OBJ_STAT(obj, ITEM_MELT_DROP))
			{
				act("$p dissolves into smoke.", ch, obj, NULL, TO_ROOM, POS_RESTING);
				act("$p dissolves into smoke.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				extract_obj(obj);
			}

			if (room_is_affected(ch->in_room, gsn_earth_to_mud))
			{
				act("$p plops in the mud and disapears.", ch, obj, NULL, TO_ROOM, POS_RESTING);
				act("$p plops in the mud and disapears.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				extract_obj(obj);
			}
		}
	}
	else
	{
		/* 'drop all' or 'drop all.obj' */
		found = FALSE;
		for (obj = ch->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;

			if ((arg[3] == '\0' || is_name(&arg[4], obj->name)) && can_see_obj(ch, obj) && obj->wear_loc == WEAR_NONE && can_drop_obj(ch, obj))
			{
				found = TRUE;
				obj_from_char(obj);
				obj_to_room(obj, ch->in_room);
				act("$n drops $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
				act("You drop $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);

				if (IS_OBJ_STAT(obj, ITEM_MELT_DROP))
				{
					act("$p dissolves into smoke.", ch, obj, NULL, TO_ROOM, POS_RESTING);
					act("$p dissolves into smoke.", ch, obj, NULL, TO_CHAR, POS_RESTING);
					extract_obj(obj);
				}

				if (room_is_affected(ch->in_room, gsn_earth_to_mud))
				{
					act("$p plops in the mud and disapears.", ch, obj, NULL, TO_ROOM, POS_RESTING);
					act("$p plops in the mud and disapears.", ch, obj, NULL, TO_CHAR, POS_RESTING);
					extract_obj(obj);
				}
			}
		}

		if (!found)
		{
			if (arg[3] == '\0')
				act("You are not carrying anything.", ch, NULL, arg, TO_CHAR, POS_RESTING);
			else
				act("You are not carrying any $T.", ch, NULL, &arg[4], TO_CHAR, POS_RESTING);
		}
	}

	return;
}


void
do_give(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char rest[MAX_STRING_LENGTH];
	int number, i, foundCount;
	CHAR_DATA *victim;
	OBJ_DATA *obj;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Give what to whom?\n\r");
		return;
	}


	if (is_number(arg1))
	{
		/* 'give NNNN coins victim' */
		int amount;
		bool silver;

		amount = atoi(arg1);
		if (amount <= 0 || (str_cmp(arg2, "coins") && str_cmp(arg2, "coin") && str_cmp(arg2, "gold") && str_cmp(arg2, "silver")))
		{
			Cprintf(ch, "Sorry, you can't do that.\n\r");
			return;
		}

		silver = str_cmp(arg2, "gold");

		argument = one_argument(argument, arg2);
		if (arg2[0] == '\0')
		{
			Cprintf(ch, "Give what to whom?\n\r");
			return;
		}

		if ((victim = get_char_room(ch, arg2)) == NULL)
		{
			Cprintf(ch, "They aren't here.\n\r");
			return;
		}

		if (victim == ch)
		{
			Cprintf(ch, "You can't give stuff to yourself.\n\r");
			return;
		}

		if (IS_NPC(victim)) {
			if(victim->pIndexData->pShop != NULL
			|| IS_SET(victim->act, ACT_TRAIN)
			|| victim->spec_fun == spec_lookup("spec_questmaster")
        		|| IS_SET(victim->act, ACT_PRACTICE)
        		|| IS_SET(victim->act, ACT_IS_HEALER)
        		|| IS_SET(victim->act, ACT_DEALER)) {
				Cprintf(ch, "They don't want your stuff.\n\r");
        			return;
			}
		}

		if ((!silver && ch->gold < amount) || (silver && ch->silver < amount))
		{
			Cprintf(ch, "You haven't got that much.\n\r");
			return;
		}

		if (silver)
		{

			if(get_carry_weight(victim) + amount/10 > can_carry_w(victim))
			{
				act("$N can't carry that much weight.", ch, NULL, victim, TO_CHAR, POS_RESTING);
				return;
			}

			ch->silver -= amount;

			if(victim->reclass == reclass_lookup("templar"))
			{
				Cprintf(victim, "You tithe some of the gold.\n\r");
				amount = amount * 9/10;
			}
			victim->silver += amount;
		}
		else
		{
			if (get_carry_weight(victim) + amount*2/5 > can_carry_w(victim))
 			{
 				act("$N can't carry that much weight.", ch, NULL, victim, TO_CHAR, POS_RESTING);
 				return;
			}

			ch->gold -= amount;

			if(victim->reclass == reclass_lookup("templar"))
			{
				Cprintf(victim, "You tithe some of the gold.\n\r");
				amount = amount * 9/10;
			}

			victim->gold += amount;
		}

		sprintf(buf, "$n gives you %d %s.", amount, silver ? "silver" : "gold");
		act(buf, ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$n gives $N some coins.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		sprintf(buf, "You give $N %d %s.", amount, silver ? "silver" : "gold");
		act(buf, ch, NULL, victim, TO_CHAR, POS_RESTING);

		/*
		 * Bribe trigger
		if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_BRIBE))
			mp_bribe_trigger(victim, ch, silver ? amount : amount * 100);
		 */

		if (IS_NPC(victim) && IS_SET(victim->act, ACT_IS_CHANGER))
		{
			int change;

			change = (silver ? 95 * amount / 100 / 100 : 95 * amount);

			if (!silver && change > victim->silver)
				victim->silver += change;

			if (silver && change > victim->gold)
				victim->gold += change;

			if (change < 1 && can_see(victim, ch))
			{
				act("$n tells you 'I'm sorry, you did not give me enough to change.'", victim, NULL, ch, TO_VICT, POS_RESTING);
				ch->reply = victim;
				sprintf(buf, "%d %s %s", amount, silver ? "silver" : "gold", ch->name);
				do_give(victim, buf);
			}
			else if (can_see(victim, ch))
			{
				sprintf(buf, "%d %s %s", change, silver ? "gold" : "silver", ch->name);
				do_give(victim, buf);

				if (silver)
				{
					sprintf(buf, "%d silver %s", (95 * amount / 100 - change * 100), ch->name);
					do_give(victim, buf);
				}

				act("$n tells you 'Thank you, come again.'", victim, NULL, ch, TO_VICT, POS_RESTING);
				// Stop the mob from getting over weight
				victim->silver = 0;
				victim->gold = 0;
			}
		}
		return;
	}

   number = mult_argument(arg1, rest);
	foundCount = count_obj_carry_by_name(ch, rest, ch);

	if (foundCount <= 0)
	{
		Cprintf(ch, "You do not have any %s.\n\r", rest);
		return;
	}

	if (foundCount < number)
	{
		Cprintf(ch, "You do not have %d %s's.\n\r", number, rest);
		return;
	}

	if (number == 0)
	{
		Cprintf(ch, "You waste your time by doing nothing.\n\r");
		return;
	}

	if (number < 0)
	{
		Cprintf(ch, "Now, that's just stupid.\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg2)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_NPC(victim)) {
                if(victim->pIndexData->pShop != NULL
                 || IS_SET(victim->act, ACT_TRAIN)
                 || IS_SET(victim->act, ACT_PRACTICE)
                 || IS_SET(victim->act, ACT_IS_HEALER)
                 || IS_SET(victim->act, ACT_DEALER)
                 || IS_SET(victim->act, ACT_IS_CHANGER)) {
                        Cprintf(ch, "They don't want your stuff.\n\r");
                        return;
                }
        }

   for (i = 0; i < number; i++)
   {
   	obj = get_obj_carry(ch, rest, ch);

		if (obj->wear_loc != WEAR_NONE)
		{
			Cprintf(ch, "You must remove it first.\n\r");
			return;
		}

		if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
		{
			act("$N tells you 'Sorry, you'll have to sell that.'", ch, NULL, victim, TO_CHAR, POS_RESTING);
			ch->reply = victim;
			return;
		}

		if (!can_drop_obj(ch, obj))
		{
			Cprintf(ch, "You can't let go of it.\n\r");
			return;
		}

		if (victim->carry_number + get_obj_number(obj) > can_carry_n(victim))
		{
			act("$N has $S hands full.", ch, NULL, victim, TO_CHAR, POS_RESTING);
			return;
		}

		if (get_carry_weight(victim) + get_obj_weight(obj) > can_carry_w(victim))
		{
			act("$N can't carry that much weight.", ch, NULL, victim, TO_CHAR, POS_RESTING);
			return;
		}

		if (!can_see_obj(victim, obj))
		{
			act("$N can't see it.", ch, NULL, victim, TO_CHAR, POS_RESTING);
			return;
		}

		obj_from_char(obj);
		obj_to_char(obj, victim);
		MOBtrigger = FALSE;
		act("$n gives $p to $N.", ch, obj, victim, TO_NOTVICT, POS_RESTING);
		act("$n gives you $p.", ch, obj, victim, TO_VICT, POS_RESTING);
		act("You give $p to $N.", ch, obj, victim, TO_CHAR, POS_RESTING);
	}

	if (IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 50)
		Cprintf(ch, "!!SOUND(sounds/wav/gimmee.wav V=80 P=20 T=admin)");
	MOBtrigger = TRUE;
	/*
	 * Give trigger
	if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_GIVE))
		mp_give_trigger(victim, ch, obj);
	 */

	return;
}


void
do_carve_boulder(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj;
	int skill;

	if (get_skill(ch, gsn_carve_boulder) < 1)
	{
		Cprintf(ch, "You are too clumsy to carve anything.\n\r");
		return;
	}

	skill = get_skill(ch, gsn_carve_boulder);

	if (ch->move < 50)
	{
		Cprintf(ch, "You are too tired to carve boulders.\n\r");
		return;
	}

	if (ch->in_room->sector_type != SECT_FIELD
		&& ch->in_room->sector_type != SECT_FOREST
		&& ch->in_room->sector_type != SECT_MOUNTAIN
		&& ch->in_room->sector_type != SECT_HILLS
		&& ch->in_room->sector_type != SECT_SWAMP
		&& ch->in_room->sector_type != SECT_DESERT)
	{
		Cprintf(ch, "You can't find what you need to make boulders.\n\r");
		return;
	}

	ch->move = ch->move - 50;
	if (ch->move < 0)
		ch->move = 0;

	if (number_percent() > skill)
	{
		Cprintf(ch, "Your boulder crumbles as you carve it.\n\r");
		check_improve(ch, gsn_carve_boulder, FALSE, 5);
		return;
	}

	obj = create_object(get_obj_index(OBJ_VNUM_BOULDER), 0);

	if (ch->level > 24)
	{
		obj->value[0] = 7;
		obj->value[1] = 5;
		obj->value[3] = 25;
		obj->level = 25;
	}

	if (ch->level > 39)
	{
		obj->value[0] = 8;
		obj->value[1] = 5;
		obj->value[3] = 40;
		obj->level = 40;
	}
	obj->timer = number_range(1000, 1200);
	obj_to_char(obj, ch);
	act("$n bashes the ground with $s hands and creates $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
	act("You bash the ground and create $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	check_improve(ch, gsn_carve_boulder, TRUE, 5);
	return;
}


void
do_gemology(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj, *gem;
	int skill, i;
	int count = dice(1, 4);

	if ((skill = get_skill(ch, gsn_gemology)) < 1)
	{
		Cprintf(ch, "Gemology? What's that?\n\r");
		return;
	}

	/* find out what */
	if (*argument == '\0')
	{
		Cprintf(ch, "Pawn in what gem?\n\r");
		return;
	}

	obj = get_obj_list(ch, argument, ch->carrying);

	if (obj == NULL)
	{
		Cprintf(ch, "You don't have that item.\n\r");
		return;
	}

	if (obj->item_type != ITEM_GEM)
	{
		Cprintf(ch, "You can only hawk gems.\n\r");
		return;
	}

	if (obj->cost == 0)
	{
		Cprintf(ch, "You can't exchange worthless gems.\n\r");
		return;
	}

	if (number_percent() > skill)
	{
		Cprintf(ch, "You fail to hawk your gem.\n\r");
		check_improve(ch, gsn_gemology, FALSE, 6);
		return;
	}

	for(i=0; i < count; i++) {
		if (obj->cost > 0 && obj->cost < 1000)
			gem = create_object(get_obj_index(OBJ_VNUM_GEM_A), 0);
		if (obj->cost >= 1000 && obj->cost < 5000)
			gem = create_object(get_obj_index(OBJ_VNUM_GEM_B), 0);
		if (obj->cost >= 5000)
			gem = create_object(get_obj_index(OBJ_VNUM_GEM_C), 0);

		obj_to_char(gem, ch);
		act("$n gets a gem out of $s bag and hawks it for $p.", ch, gem, NULL, TO_ROOM, POS_RESTING);
		act("You hawk a gem and get $p for it.", ch, gem, NULL, TO_CHAR, POS_RESTING);
	}

	extract_obj(obj);

	check_improve(ch, gsn_gemology, TRUE, 2);
	return;
}


void
do_shedding(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj;
	int sn, chance, skill;
	AFFECT_DATA af;

	if (get_skill(ch, gsn_shedding) < 1)
	{
		Cprintf(ch, "You couldn't shed a fur ball if you tried.\n\r");
		return;
	}

	skill = get_skill(ch, gsn_shedding) / 2;
	sn = gsn_shedding;

	if (number_percent() > skill)
	{
		Cprintf(ch, "That scale is a dud.  You toss it in the garbage.\n\r");
		check_improve(ch, gsn_shedding, FALSE, 4);
		return;
	}

	chance = dice(1, 3);

	if (chance == 1)
		obj = create_object(get_obj_index(OBJ_VNUM_SCALE_A), 0);
	if (chance == 2)
		obj = create_object(get_obj_index(OBJ_VNUM_SCALE_B), 0);
	if (chance == 3)
		obj = create_object(get_obj_index(OBJ_VNUM_SCALE_C), 0);

	obj_to_char(obj, ch);
	act("$n plucks a scale out of $s body and creates $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
	act("You pluck a scale out of your body and create $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	check_improve(ch, gsn_shedding, TRUE, 4);

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = ch->level;
	af.duration = 3;
	af.location = APPLY_AC;
	af.modifier = 10;
	af.bitvector = 0;
	affect_join(ch, &af);
	return;
}


void
do_conversion(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj;
	int total, skill, gold, silver;

	if (get_skill(ch, gsn_conversion) < 1)
	{
		Cprintf(ch, "You can't convert anything.\n\r");
		return;
	}

	/* find out what */
	if (*argument == '\0')
	{
		Cprintf(ch, "Convert what item?\n\r");
		return;
	}

	obj = get_obj_carry(ch, argument, ch);

	if (obj == NULL)
	{
		Cprintf(ch, "You are not carrying that item.\n\r");
		return;
	}

	if (CAN_WEAR(obj, ITEM_NO_SAC))
	{
		Cprintf(ch, "%s is too powerful to alter.\n\r", capitalize(obj->short_descr));
		return;
	}

	if ((skill = get_skill(ch, gsn_conversion)) < 1)
	{
		Cprintf(ch, "Conversion? What's that?\n\r");
		return;
	}

	if(obj->cost > obj->pIndexData->cost) {
		Cprintf(ch, "This appraised item resists your conversion.\n\r");
		return;
	}

	if(ch->mana < 10) {
		Cprintf(ch, "You don't have enough energy to transmute anything.\n\r");
		return;
	}
	ch->mana -= 10;

	if (number_percent() > skill)
	{
		Cprintf(ch, "You fail to convert %s into gold.\n\r", obj->short_descr);
		check_improve(ch, gsn_conversion, FALSE, 4);
		return;
	}

	total = obj->cost * 8 / 10 * skill / 100;
	gold = total / 100;
	silver = total % 100;

	Cprintf(ch, "You convert %s into %d gold and %d silver.\n\r", obj->short_descr, gold, silver);
	if(ch->reclass == reclass_lookup("templar")) {
		Cprintf(ch, "You tithe some of the gold.\n\r");
		gold = gold * 9/10;
		silver = silver * 9/10;
	}
	extract_obj(obj);
	ch->gold += gold;
	ch->silver += silver;

	check_improve(ch, gsn_conversion, TRUE, 4);
}


/* for poisoning weapons and food/drink */
void
do_envenom(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *obj;
	AFFECT_DATA af;
	int percent, skill;

	/* find out what */
	if (argument[0] == '\0')
	{
		Cprintf(ch, "Envenom what item?\n\r");
		return;
	}

	obj = get_obj_list(ch, argument, ch->carrying);

	if (obj == NULL)
	{
		Cprintf(ch, "You don't have that item.\n\r");
		return;
	}

	if ((skill = get_skill(ch, gsn_envenom)) < 1)
	{
		Cprintf(ch, "Are you crazy? You'd poison yourself!\n\r");
		return;
	}

	if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
	{
		if (IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
		{
			act("You fail to poison $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			return;
		}

		if (number_percent() < skill) /* success! */
		{
			act("$n treats $p with deadly poison.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You treat $p with deadly poison.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			if (!obj->value[3])
			{
				obj->value[3] = 1;
				check_improve(ch, gsn_envenom, TRUE, 4);
			}

			WAIT_STATE(ch, skill_table[gsn_envenom].beats);
			return;
		}

		act("You fail to poison $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		if (!obj->value[3])
			check_improve(ch, gsn_envenom, FALSE, 4);

		WAIT_STATE(ch, skill_table[gsn_envenom].beats);
		return;
	}

	if (obj->item_type == ITEM_WEAPON)
	{
		if (IS_WEAPON_STAT(obj, WEAPON_POISON))
		{
			act("$p is already envenomed.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			return;
		}

		percent = number_percent();

		if (percent < skill)
		{
			af.where = TO_WEAPON;
			af.type = gsn_poison;
			af.level = ch->level;
			af.duration = ch->level / 2;
			af.location = 0;
			af.modifier = 0;
			af.bitvector = WEAPON_POISON;
			affect_to_obj(obj, &af);

			act("$n coats $p with deadly venom.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You coat $p with venom.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			check_improve(ch, gsn_envenom, TRUE, 3);
			WAIT_STATE(ch, skill_table[gsn_envenom].beats);
			return;
		}
		else
		{
			act("You fail to envenom $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			check_improve(ch, gsn_envenom, FALSE, 3);
			WAIT_STATE(ch, skill_table[gsn_envenom].beats);
			return;
		}
	}

	act("You can't poison $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	return;
}


void
do_fill(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	OBJ_DATA *fountain;
	bool found;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Fill what?\n\r");
		return;
	}

	if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
	{
		Cprintf(ch, "You do not have that item.\n\r");
		return;
	}

	found = FALSE;
	for (fountain = ch->in_room->contents; fountain != NULL; fountain = fountain->next_content)
	{
		if (fountain->item_type == ITEM_FOUNTAIN)
		{
			found = TRUE;
			break;
		}
	}

	if (!found)
	{
		Cprintf(ch, "There is no fountain here!\n\r");
		return;
	}

	if (obj->item_type != ITEM_DRINK_CON)
	{
		Cprintf(ch, "You can't fill that.\n\r");
		return;
	}

	if (obj->value[1] != 0 && obj->value[2] != fountain->value[2])
	{
		Cprintf(ch, "There is already another liquid in it.\n\r");
		return;
	}

	if (obj->value[1] >= obj->value[0])
	{
		Cprintf(ch, "Your container is full.\n\r");
		return;
	}

	sprintf(buf, "You fill $p with %s from $P.", liq_table[fountain->value[2]].liq_name);
	act(buf, ch, obj, fountain, TO_CHAR, POS_RESTING);
	sprintf(buf, "$n fills $p with %s from $P.", liq_table[fountain->value[2]].liq_name);
	act(buf, ch, obj, fountain, TO_ROOM, POS_RESTING);
	obj->value[2] = fountain->value[2];
	obj->value[1] = obj->value[0];
	if (IS_SET(ch->toggles, TOGGLES_SOUND))
		Cprintf(ch, "!!SOUND(sounds/wav/water*.wav V=80 P=20 T=admin)");

	return;
}


void
do_pour(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
	OBJ_DATA *out, *in;
	CHAR_DATA *vch = NULL;
	int amount;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0' || argument[0] == '\0')
	{
		Cprintf(ch, "Pour what into what?\n\r");
		return;
	}

	if ((out = get_obj_carry(ch, arg, ch)) == NULL)
	{
		Cprintf(ch, "You don't have that item.\n\r");
		return;
	}

	if (out->item_type != ITEM_DRINK_CON)
	{
		Cprintf(ch, "That's not a drink container.\n\r");
		return;
	}

	if (!str_cmp(argument, "out"))
	{
		if (out->value[1] == 0)
		{
			Cprintf(ch, "It's already empty.\n\r");
			return;
		}

		out->value[1] = 0;
		out->value[3] = 0;
		sprintf(buf, "You invert $p, spilling %s all over the ground.", liq_table[out->value[2]].liq_name);
		act(buf, ch, out, NULL, TO_CHAR, POS_RESTING);

		sprintf(buf, "$n inverts $p, spilling %s all over the ground.", liq_table[out->value[2]].liq_name);
		act(buf, ch, out, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	if ((in = get_obj_here(ch, argument)) == NULL)
	{
		vch = get_char_room(ch, argument);

		if (vch == NULL)
		{
			Cprintf(ch, "Pour into what?\n\r");
			return;
		}

		in = get_eq_char(vch, WEAR_HOLD);

		if (in == NULL)
		{
			Cprintf(ch, "They aren't holding anything.");
			return;
		}
	}

	if (in->item_type != ITEM_DRINK_CON)
	{
		Cprintf(ch, "You can only pour into other drink containers.\n\r");
		return;
	}

	if (in == out)
	{
		Cprintf(ch, "You cannot change the laws of physics!\n\r");
		return;
	}

	if (in->value[1] != 0 && in->value[2] != out->value[2])
	{
		Cprintf(ch, "They don't hold the same liquid.\n\r");
		return;
	}

	if (out->value[1] == 0)
	{
		act("There's nothing in $p to pour.", ch, out, NULL, TO_CHAR, POS_RESTING);
		return;
	}

	if (in->value[1] >= in->value[0])
	{
		act("$p is already filled to the top.", ch, in, NULL, TO_CHAR, POS_RESTING);
		return;
	}

	amount = UMIN(out->value[1], in->value[0] - in->value[1]);

	in->value[1] += amount;
	out->value[1] -= amount;
	in->value[2] = out->value[2];

	if (vch == NULL)
	{
		sprintf(buf, "You pour %s from $p into $P.", liq_table[out->value[2]].liq_name);
		act(buf, ch, out, in, TO_CHAR, POS_RESTING);
		sprintf(buf, "$n pours %s from $p into $P.", liq_table[out->value[2]].liq_name);
		act(buf, ch, out, in, TO_ROOM, POS_RESTING);
	}
	else
	{
		sprintf(buf, "You pour some %s for $N.", liq_table[out->value[2]].liq_name);
		act(buf, ch, NULL, vch, TO_CHAR, POS_RESTING);
		sprintf(buf, "$n pours you some %s.", liq_table[out->value[2]].liq_name);
		act(buf, ch, NULL, vch, TO_VICT, POS_RESTING);
		sprintf(buf, "$n pours some %s for $N.", liq_table[out->value[2]].liq_name);
		act(buf, ch, NULL, vch, TO_NOTVICT, POS_RESTING);
	}
}


void
do_drink(CHAR_DATA * ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    AFFECT_DATA *paf;
    int amount;
    int liquid;
    AFFECT_DATA af;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
            if (obj->item_type == ITEM_FOUNTAIN) {
                break;
            }
        }

        if (obj == NULL) {
            Cprintf(ch, "Drink what?\n\r");
            return;
        }
    } else {
        if ((obj = get_obj_here(ch, arg)) == NULL) {
            Cprintf(ch, "You can't find it.\n\r");
            return;
        }
    }

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10) {
        Cprintf(ch, "You fail to reach your mouth.  *Hic*\n\r");
        return;
    }

    switch (obj->item_type) {
        default:
            Cprintf(ch, "You can't drink from that.\n\r");
            return;

        case ITEM_FOUNTAIN:
            if ((liquid = obj->value[2]) < 0) {
                bug("Do_drink: bad liquid number %d.", liquid);
                liquid = obj->value[2] = 0;
            }

            amount = liq_table[liquid].liq_affect[4] * 3;
            break;

        case ITEM_DRINK_CON:
            if (obj->value[1] <= 0) {
                Cprintf(ch, "It is already empty.\n\r");
                if (IS_SET(ch->toggles, TOGGLES_SOUND)) {
                    Cprintf(ch, "!!SOUND(sounds/wav/empty.wav V=80 P=20 T=admin)");
                }
                return;
            }

            if ((liquid = obj->value[2]) < 0) {
                bug("Do_drink: bad liquid number %d.", liquid);
                liquid = obj->value[2] = 0;
            }

            amount = liq_table[liquid].liq_affect[4];
            amount = UMIN(amount, obj->value[1]);
            break;
    }

    if (ch->race == race_lookup("marid")
            && ch->remort
            && liquid != LIQ_WATER) {
        Cprintf(ch, "Anything but water won't satisfy your needs.\n\r");
        return;
    }

    if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && ch->pcdata->condition[COND_FULL] > 45) {
        Cprintf(ch, "You're too full to drink more.\n\r");
        return;
    }

    act("$n drinks $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_ROOM, POS_RESTING);
    act("You drink $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_CHAR, POS_RESTING);

    gain_condition(ch, COND_DRUNK, amount * liq_table[liquid].liq_affect[COND_DRUNK] / 36);
    gain_condition(ch, COND_FULL, amount * liq_table[liquid].liq_affect[COND_FULL] / 4);
    gain_condition(ch, COND_THIRST, amount * liq_table[liquid].liq_affect[COND_THIRST] / 10);
    gain_condition(ch, COND_HUNGER, amount * liq_table[liquid].liq_affect[COND_HUNGER] / 2);

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10) {
        if(ch->race == race_lookup("dwarf") && ch->remort && ch->rem_sub == 1) {
            Cprintf(ch, "You feel drunk. YOU ARE THE DRUNKEN MASTER!! Go break some heads.\n\r");
            affect_strip(ch, gsn_drunken_master);
            af.where = TO_AFFECTS;
            af.type = gsn_drunken_master;
            af.level = ch->level;
            af.duration = ch->pcdata->condition[COND_DRUNK];
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = 0;
            affect_to_char(ch, &af);
        } else {
            Cprintf(ch, "You feel drunk.\n\r");
        }
    }

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 40) {
        Cprintf(ch, "You are full.\n\r");
    }

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 40) {
        Cprintf(ch, "Your thirst is quenched.\n\r");
    }

    // Marid remort spell corrupt does nasty things
    if (((!IS_NPC(ch) && ch->clan) || IS_NPC(ch)) && (paf = affect_find(obj->affected, gsn_corruption)) != NULL) {
        obj_cast_spell(gsn_corruption, ch->level, ch, ch, obj);
    }

    if (obj->value[3] != 0) {
        /* The drink was poisoned ! */
        AFFECT_DATA af;

        act("$n chokes and gags.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
        Cprintf(ch, "You choke and gag.\n\r");
        af.where = TO_AFFECTS;
        af.type = gsn_poison;
        af.level = number_fuzzy(amount);
        af.duration = 3 * amount;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_POISON;
        affect_join(ch, &af);
    }

    if (obj->value[0] > 0) {
        obj->value[1] -= amount;
    }

    return;
}


void
do_eat(CHAR_DATA * ch, char *argument) {
    AFFECT_DATA *paf;
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        Cprintf(ch, "Eat what?\n\r");
        return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL) {
        Cprintf(ch, "You do not have that item.\n\r");
        return;
    }

    if (obj->pIndexData->area->security < 9 && !IS_IMMORTAL(ch)) {
        Cprintf(ch, "You can't eat items from unfinished areas.\n\r");
        return;
    }

    if (!IS_IMMORTAL(ch) && ch->level < obj->level) {
        Cprintf(ch, "You aren't strong enough to eat that yet.\n\r");
        return;
    }

    if (!IS_IMMORTAL(ch)) {
        if (obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL) {
            Cprintf(ch, "That's not edible.\n\r");
            return;
        }

        if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 40) {
            Cprintf(ch, "You're too full to eat more.\n\r");
            return;
        }

        if (IS_SET(obj->wear_flags, ITEM_NEWBIE) && (ch->level > 9 || ch->remort || ch->reclass)) {
            Cprintf(ch, "You refuse to eat that, it's for weaklings!\n\r");
            return;
        }
    }

    act("$n eats $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
    act("You eat $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);

    switch (obj->item_type) {
        case ITEM_FOOD:
            if (!IS_NPC(ch)) {
                int condition;

                condition = ch->pcdata->condition[COND_HUNGER];
                gain_condition(ch, COND_FULL, obj->value[0]);
                gain_condition(ch, COND_HUNGER, obj->value[1]);

                if (condition == 0 && ch->pcdata->condition[COND_HUNGER] > 0) {
                    Cprintf(ch, "You are no longer hungry.\n\r");
                } else if (ch->pcdata->condition[COND_FULL] > 40) {
                    Cprintf(ch, "You are full.\n\r");
                }

                // Marid remort spell corrupt does nasty things
                if (((!IS_NPC(ch) && ch->clan) || IS_NPC(ch)) && (paf = affect_find(obj->affected, gsn_corruption)) != NULL) {
                    obj_cast_spell(gsn_corruption, paf->level, ch, ch, obj);
                }
            }

            if (obj->value[3] != 0) {
                /* The food was poisoned! */
                AFFECT_DATA af;

                act("$n chokes and gags.", ch, 0, 0, TO_ROOM, POS_RESTING);
                Cprintf(ch, "You choke and gag.\n\r");

                af.where = TO_AFFECTS;
                af.type = gsn_poison;
                af.level = number_fuzzy(obj->value[0]);
                af.duration = 2 * obj->value[0];
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.bitvector = AFF_POISON;
                affect_join(ch, &af);
            }
            break;

        case ITEM_PILL:
            obj_cast_spell(obj->value[1], obj->value[0], ch, ch, NULL);
            obj_cast_spell(obj->value[2], obj->value[0], ch, ch, NULL);
            obj_cast_spell(obj->value[3], obj->value[0], ch, ch, NULL);
            obj_cast_spell(obj->value[4], obj->value[0], ch, ch, NULL);
            break;
    }

    extract_obj(obj);
    return;
}


/*
 * Remove an object.
 */
bool
remove_obj(CHAR_DATA * ch, int iWear, bool fReplace)
{
	OBJ_DATA *obj;

	if ((obj = get_eq_char(ch, iWear)) == NULL)
		return TRUE;

	if (!fReplace)
		return FALSE;

	if (IS_SET(obj->extra_flags, ITEM_NOREMOVE))
	{
		act("You can't remove $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		return FALSE;
	}

	unequip_char(ch, obj);
	act("$n stops using $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
	act("You stop using $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	return TRUE;
}


/* Show_wear_skill_message
 * Display the string that corresponds to your skill with said weapon. 
 * Inputs: Skill is the skill percentage from 0 to 100.
 */
void show_wear_skill_message(CHAR_DATA *ch, OBJ_DATA *obj, int skill) {

	if (skill >= 100)
        	act("$p feels like a part of you!", ch, obj, NULL, TO_CHAR, POS_RESTING);
        else if (skill > 85)
                act("You feel quite confident with $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
        else if (skill > 70)
                act("You are skilled with $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
        else if (skill > 50)
                act("Your skill with $p is adequate.", ch, obj, NULL, TO_CHAR, POS_RESTING);
        else if (skill > 25)
                act("$p feels a little clumsy in your hands.", ch, obj, NULL, TO_CHAR, POS_RESTING);
        else if (skill > 1)
                act("You fumble and almost drop $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
        else
                act("You don't even know which end is up on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);

        return;

}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void
wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace)
{
	char buf[MAX_STRING_LENGTH];
	bool can_wear = TRUE;

	if (ch->level < obj->level)
	{
		sprintf(buf, "You must be level %d to use the $p.\n\r", obj->level);
		act(buf, ch, obj, NULL, TO_CHAR, POS_RESTING);
		act("$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		return;
	}

	if (IS_SET(obj->wear_flags, ITEM_NEWBIE) &&
		(ch->level > 9 || ch->remort || ch->reclass))
	{
		Cprintf(ch, "The equipment is too small for you to wear.\n\r");
		return;
	}

	if (obj_is_affected(obj, gsn_wizard_mark)
	|| obj_is_affected(obj, gsn_blade_rune)
	|| obj_is_affected(obj, gsn_burst_rune)) {
		if(ch->charClass != class_lookup("runist")) {
			Cprintf(ch, "Only a runist can control this item.\n\r");
			act("$n is burned by the runes on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			return;
		}
	}

	if (IS_SET(obj->extra_flags, ITEM_WARRIOR) || IS_SET(obj->extra_flags, ITEM_MAGE) || IS_SET(obj->extra_flags, ITEM_THIEF) || IS_SET(obj->extra_flags, ITEM_CLERIC))
	{
		can_wear = FALSE;

		if ((ch->charClass == class_lookup("warrior") || ch->charClass == class_lookup("paladin") || ch->charClass == class_lookup("ranger")) && IS_SET(obj->extra_flags, ITEM_WARRIOR))
			can_wear = TRUE;

		if ((ch->charClass == class_lookup("cleric") || ch->charClass == class_lookup("druid") || ch->charClass == class_lookup("monk")) && IS_SET(obj->extra_flags, ITEM_CLERIC))
			can_wear = TRUE;

		if ((ch->charClass == class_lookup("mage") || ch->charClass == class_lookup("conjurer") || ch->charClass == class_lookup("invoker") || ch->charClass == class_lookup("enchanter")) && IS_SET(obj->extra_flags, ITEM_MAGE))
			can_wear = TRUE;

		if ((ch->charClass == class_lookup("thief") || ch->charClass == class_lookup("runist")) && IS_SET(obj->extra_flags, ITEM_THIEF))
			can_wear = TRUE;
	}

	if (!can_wear)
	{
		Cprintf(ch, "Your class does not permit you to use that.\n\r");
		return;
	}

	if (obj->pIndexData->area->security < 9 && 
	    !IS_IMMORTAL(ch) &&
	    !(IS_NPC(ch) && ch->pIndexData->area->security < 9))
	{
		Cprintf(ch, "You can't wear items from unfinished areas.\n\r");
		return;
	}

	if (ch->reclass == reclass_lookup("barbarian"))
	{
		if (obj->enchanted &&
			 obj->item_type == ITEM_WEAPON
		&& !IS_WEAPON_STAT(obj, WEAPON_INTELLIGENT))
		{
			Cprintf(ch, "You would lower yourself to using a weapon enchant with magical properties?\n\r");
			return;
		}
	}

	if (is_affected(ch, gsn_razor_claws)
	&& obj->item_type == ITEM_WEAPON) {
		Cprintf(ch, "You can't wield any weapons with your claws extended.\n\r");
		return;
	}


	if (obj->item_type == ITEM_LIGHT)
	{
		if (!remove_obj(ch, WEAR_LIGHT, fReplace))
			return;
		act("$n lights $p and holds it.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You light $p and hold it.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		equip_char(ch, obj, WEAR_LIGHT);
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
	{
		if (get_eq_char(ch, WEAR_FINGER_L) != NULL
			&& get_eq_char(ch, WEAR_FINGER_R) != NULL
			&& !remove_obj(ch, WEAR_FINGER_L, fReplace)
			&& !remove_obj(ch, WEAR_FINGER_R, fReplace))
			return;

		if (get_eq_char(ch, WEAR_FINGER_L) == NULL)
		{
			act("$n wears $p on $s left finger.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You wear $p on your left finger.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			equip_char(ch, obj, WEAR_FINGER_L);
			return;
		}

		if (get_eq_char(ch, WEAR_FINGER_R) == NULL)
		{
			act("$n wears $p on $s right finger.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You wear $p on your right finger.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			equip_char(ch, obj, WEAR_FINGER_R);
			return;
		}

		bug("Wear_obj: no free finger.", 0);
		Cprintf(ch, "You already wear two rings.\n\r");
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_NECK))
	{
		if (get_eq_char(ch, WEAR_NECK_1) != NULL && get_eq_char(ch, WEAR_NECK_2) != NULL && !remove_obj(ch, WEAR_NECK_1, fReplace) && !remove_obj(ch, WEAR_NECK_2, fReplace))
			return;

		if (get_eq_char(ch, WEAR_NECK_1) == NULL)
		{
			act("$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You wear $p around your neck.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			equip_char(ch, obj, WEAR_NECK_1);
			return;
		}

		if (get_eq_char(ch, WEAR_NECK_2) == NULL)
		{
			act("$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You wear $p around your neck.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			equip_char(ch, obj, WEAR_NECK_2);
			return;
		}

		bug("Wear_obj: no free neck.", 0);
		Cprintf(ch, "You already wear two neck items.\n\r");
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_BODY))
	{
		if (!remove_obj(ch, WEAR_BODY, fReplace))
			return;
		act("$n wears $p on $s torso.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You wear $p on your torso.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		equip_char(ch, obj, WEAR_BODY);
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
	{
		if (ch->race == race_lookup("troll") && ch->remort > 0 && ch->rem_sub == 1)
		{
			if (get_eq_char(ch, WEAR_HEAD) != NULL && get_eq_char(ch, WEAR_HEAD_2) != NULL && !remove_obj(ch, WEAR_HEAD, fReplace) && !remove_obj(ch, WEAR_HEAD_2, fReplace))
				return;

			if (get_eq_char(ch, WEAR_HEAD) == NULL)
			{
				act("$n wears $p on $s left head.", ch, obj, NULL, TO_ROOM, POS_RESTING);
				act("You wear $p on your left head.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				equip_char(ch, obj, WEAR_HEAD);
				return;
			}

			if (get_eq_char(ch, WEAR_HEAD_2) == NULL)
			{
				act("$n wears $p on $s right head.", ch, obj, NULL, TO_ROOM, POS_RESTING);
				act("You wear $p on your right head.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				equip_char(ch, obj, WEAR_HEAD_2);
				return;
			}

			bug("Wear_obj: no free head.", 0);
			Cprintf(ch, "You already wear two head gear.\n\r");
			return;
		}
		else
		{
			if (!remove_obj(ch, WEAR_HEAD, fReplace))
				return;
			act("$n wears $p on $s head.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You wear $p on your head.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			equip_char(ch, obj, WEAR_HEAD);
			return;
		}
	}

	if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
	{
		if (!remove_obj(ch, WEAR_LEGS, fReplace))
			return;
		act("$n wears $p on $s legs.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You wear $p on your legs.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		equip_char(ch, obj, WEAR_LEGS);
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_FEET))
	{
		if (!remove_obj(ch, WEAR_FEET, fReplace))
			return;
		act("$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You wear $p on your feet.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		equip_char(ch, obj, WEAR_FEET);
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
	{
		if (!remove_obj(ch, WEAR_HANDS, fReplace))
			return;
		act("$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You wear $p on your hands.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		equip_char(ch, obj, WEAR_HANDS);
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
	{
		if (!remove_obj(ch, WEAR_ARMS, fReplace))
			return;
		act("$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You wear $p on your arms.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		equip_char(ch, obj, WEAR_ARMS);
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
	{
		if (!remove_obj(ch, WEAR_ABOUT, fReplace))
			return;
		act("$n wears $p on $s body.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You wear $p on your body.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		equip_char(ch, obj, WEAR_ABOUT);
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
	{
		if (!remove_obj(ch, WEAR_WAIST, fReplace))
			return;
		act("$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You wear $p about your waist.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		equip_char(ch, obj, WEAR_WAIST);
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
	{
		if (get_eq_char(ch, WEAR_WRIST_L) != NULL && get_eq_char(ch, WEAR_WRIST_R) != NULL && !remove_obj(ch, WEAR_WRIST_L, fReplace) && !remove_obj(ch, WEAR_WRIST_R, fReplace))
			return;

		if (get_eq_char(ch, WEAR_WRIST_L) == NULL)
		{
			act("$n wears $p around $s left wrist.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You wear $p around your left wrist.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			equip_char(ch, obj, WEAR_WRIST_L);
			return;
		}

		if (get_eq_char(ch, WEAR_WRIST_R) == NULL)
		{
			act("$n wears $p around $s right wrist.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You wear $p around your right wrist.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			equip_char(ch, obj, WEAR_WRIST_R);
			return;
		}

		bug("Wear_obj: no free wrist.", 0);
		Cprintf(ch, "You already wear two wrist items.\n\r");
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
	{
		OBJ_DATA *weapon;

		if (!remove_obj(ch, WEAR_SHIELD, fReplace))
			return;

		weapon = get_eq_char(ch, WEAR_WIELD);
		if ((weapon != NULL
		&& ch->size < SIZE_LARGE
		&& IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS))
		|| get_eq_char(ch, WEAR_DUAL) != NULL)
		{
			Cprintf(ch, "Your hands are tied up with your weapon!\n\r");
			return;
		}

		act("$n wears $p as a shield.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		act("You wear $p as a shield.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		equip_char(ch, obj, WEAR_SHIELD);
		return;
	}

	if (CAN_WEAR(obj, ITEM_WIELD))
	{
		// Universal checks: detonate and weight
		if (is_affected(ch, gsn_detonation))
		{
			Cprintf(ch, "Your hands are still too numb to wield a weapon.\n\r");
			return;
		}

		if (!IS_NPC(ch) && get_obj_weight(obj) > (str_app[get_curr_stat(ch, STAT_STR)].wield * 10))
		{
			Cprintf(ch, "It is too heavy for you to wield.\n\r");
			return;
		}

		if(obj->value[0] == WEAPON_RANGED) {
			if (!remove_obj(ch, WEAR_RANGED, fReplace))
                                return;

                        act("$n wears $p on $s back.", ch, obj, NULL, TO_ROOM, POS_RESTING);
                        act("You wear $p on your back.", ch, obj, NULL, TO_CHAR, POS_RESTING);

                        equip_char(ch, obj, WEAR_RANGED);

                        show_wear_skill_message(ch, obj, get_weapon_skill(ch, get_weapon_sn(ch, WEAR_WIELD)));
			return;
		}
		else if(get_skill(ch, gsn_dual_wield) > 0)
		{
			// No slots open
			if (get_eq_char(ch, WEAR_WIELD) != NULL
				&& get_eq_char(ch, WEAR_DUAL) != NULL
				&& !remove_obj(ch, WEAR_WIELD, fReplace)
				&& !remove_obj(ch, WEAR_DUAL, fReplace))
			{
				return;
			}
			// neither weapon can be two handed.
			if (get_eq_char(ch, WEAR_WIELD) == NULL)
			{
				// If you are wearing a dual weapon and your new primary is two
				// handed nyah no more.
				if((IS_SET(obj->pIndexData->value[4], WEAPON_TWO_HANDS)
				|| IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
				&& get_eq_char(ch, WEAR_DUAL) != NULL)
				{
					Cprintf(ch, "You need two hands free for that weapon.\n\r");
					return;
				}
				// Normal case: if you are medium size or less you can't wear
				// a shield and use this weapon.
				if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)
					&& ch->size < SIZE_LARGE
					&& get_eq_char(ch, WEAR_SHIELD) != NULL)
				{
					Cprintf(ch, "You need two hands free for that weapon.\n\r");
					return;
				}

				act("$n wields $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
				act("You wield $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				equip_char(ch, obj, WEAR_WIELD);

				show_wear_skill_message(ch, obj, get_weapon_skill(ch, get_weapon_sn(ch, WEAR_WIELD)));


				return;
			}
			if (get_eq_char(ch, WEAR_DUAL) == NULL)
			{
				if(IS_SET(obj->pIndexData->value[4], WEAPON_TWO_HANDS)
				|| IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)
				|| IS_WEAPON_STAT(get_eq_char(ch, WEAR_WIELD), WEAPON_TWO_HANDS)
				|| get_eq_char(ch, WEAR_SHIELD) != NULL
				|| IS_SET(get_eq_char(ch, WEAR_WIELD)->pIndexData->value[4], WEAPON_TWO_HANDS))
				{
					Cprintf(ch, "You need two hands free for that weapon.\n\r");
					return;
				}


				act("$n wields $p in their off hand.", ch, obj, NULL, TO_ROOM, POS_RESTING);
				act("You wield $p in your off hand.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				equip_char(ch, obj, WEAR_DUAL);

				show_wear_skill_message(ch, obj, get_weapon_skill(ch, get_weapon_sn(ch, WEAR_DUAL)));

				return;
			}
		}
		else
		{
			if (!remove_obj(ch, WEAR_WIELD, fReplace))
				return;

			if (!IS_NPC(ch) && ch->size < SIZE_LARGE
				&& IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)
				&& get_eq_char(ch, WEAR_SHIELD) != NULL)
			{
				Cprintf(ch, "You need two hands free for that weapon.\n\r");
				return;
			}

			act("$n wields $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You wield $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			equip_char(ch, obj, WEAR_WIELD);

			show_wear_skill_message(ch, obj, get_weapon_skill(ch, get_weapon_sn(ch, WEAR_WIELD)));

			return;
		}
		bug("Wear_obj: no free weapon slot.", 0);
		Cprintf(ch, "You already wield two weapons.\n\r");
		return;
	}

	if (CAN_WEAR(obj, ITEM_HOLD))
	{
		if(obj->item_type == ITEM_AMMO) {
			if (!remove_obj(ch, WEAR_AMMO, fReplace))
                        	return;
                	act("$n puts $p on $s belt.", ch, obj, NULL, TO_ROOM, POS_RESTING);
                	act("You put $p on your belt.", ch, obj, NULL, TO_CHAR, POS_RESTING);
                	equip_char(ch, obj, WEAR_AMMO);

			if(is_name("arrow", obj->name))
				Cprintf(ch, "This ammunition is for bows.\n\r");
			if(is_name("bolt", obj->name))
				Cprintf(ch, "This ammunition is for crossbows.\n\r");
			if(is_name("bullet", obj->name))
				Cprintf(ch, "This ammunition is for guns.\n\r");

		}
		else {
	
			if (!remove_obj(ch, WEAR_HOLD, fReplace))
                        	return;
                	act("$n holds $p in $s hand.", ch, obj, NULL, TO_ROOM, POS_RESTING);
                	act("You hold $p in your hand.", ch, obj, NULL, TO_CHAR, POS_RESTING);
                	equip_char(ch, obj, WEAR_HOLD);
		}
		return;
	}

	if (CAN_WEAR(obj, ITEM_WEAR_FLOAT))
	{
		if (ch->level < 25)
		{
			if (!remove_obj(ch, WEAR_FLOAT, fReplace))
				return;
			act("$n releases $p to float next to $m.", ch, obj, NULL, TO_ROOM, POS_RESTING);
			act("You release $p and it floats next to you.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			equip_char(ch, obj, WEAR_FLOAT);
			return;
		}
		else
		{
			if (get_eq_char(ch, WEAR_FLOAT) != NULL
				&& get_eq_char(ch, WEAR_FLOAT_2) != NULL
				&& !remove_obj(ch, WEAR_FLOAT, fReplace)
				&& !remove_obj(ch, WEAR_FLOAT_2, fReplace))
				return;

			if (get_eq_char(ch, WEAR_FLOAT) == NULL)
			{
				act("$n releases $p to float next to $m.", ch, obj, NULL, TO_ROOM, POS_RESTING);
				act("You release $p and it floats next to you.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				equip_char(ch, obj, WEAR_FLOAT);
				return;
			}

			if (get_eq_char(ch, WEAR_FLOAT_2) == NULL)
			{
				act("$n releases $p to orbit around $m.", ch, obj, NULL, TO_ROOM, POS_RESTING);
				act("You release $p and it orbits around you.", ch, obj, NULL, TO_CHAR, POS_RESTING);
				equip_char(ch, obj, WEAR_FLOAT_2);
				return;
			}

			bug("Wear_obj: no free float.", 0);
			Cprintf(ch, "You already wear two stones.\n\r");
			return;
		}
	}

	if (fReplace)
		Cprintf(ch, "You can't wear, wield, or hold that.\n\r");

	return;
}


void
do_wear(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;

	if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET))
	{
		Cprintf(ch, "Pets can't wear stuff.\n\r");
		return;
	}

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Wear, wield, or hold what?\n\r");
		return;
	}

	if (!str_cmp(arg, "all"))
	{
		OBJ_DATA *obj_next;

		for (obj = ch->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;

			if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj) &&
				!((ch->remort > 0) &&
				  (ch->race == race_lookup("black dragon") ||
					ch->race == race_lookup("blue dragon") ||
					ch->race == race_lookup("green dragon") ||
					ch->race == race_lookup("red dragon") ||
					ch->race == race_lookup("white dragon")) &&
				  (IS_SET(obj->wear_flags, ITEM_WEAR_HANDS) ||
					IS_SET(obj->wear_flags, ITEM_WEAR_FEET) ||
					IS_SET(obj->wear_flags, ITEM_WEAR_LEGS))))
				wear_obj(ch, obj, FALSE);
		}
		return;
	}
	else
	{
		if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
		{
			Cprintf(ch, "You do not have that item.\n\r");
			return;
		}

		if ((ch->remort > 0) &&
			(ch->race == race_lookup("black dragon") ||
			 ch->race == race_lookup("blue dragon") ||
			 ch->race == race_lookup("green dragon") ||
			 ch->race == race_lookup("red dragon") ||
			 ch->race == race_lookup("white dragon")) &&
			(IS_SET(obj->wear_flags, ITEM_WEAR_HANDS) ||
			 IS_SET(obj->wear_flags, ITEM_WEAR_FEET) ||
			 IS_SET(obj->wear_flags, ITEM_WEAR_LEGS)))
		{
			Cprintf(ch, "You're too big a dragon to wear all that stuff.\n\r");
			return;
		}

		wear_obj(ch, obj, TRUE);
	}

	return;
}


void
do_remove(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Remove what?\n\r");
		return;
	}

	if ((obj = get_obj_wear(ch, arg, ch)) == NULL)
	{
		Cprintf(ch, "You do not have that item.\n\r");
		return;
	}

	remove_obj(ch, obj->wear_loc, TRUE);
	return;
}


void
do_worship(CHAR_DATA * ch, char *argument) {
    AFFECT_DATA af;
    int deity_num;
    int dp;

    char command[MAX_INPUT_LENGTH];
    argument = one_argument(argument, command);

    if (ch->fighting != NULL) {
        Cprintf(ch, "You can't concentrate enough to talk to your God.\n\r");
        return;
    }

    if (ch->pktimer > 0) {
        Cprintf(ch, "Prayers mixed with nervousness lead to nothing.  You can't worship right now.\n\r");
        return;
    }

    if (command[0] == '\0') {
        Cprintf(ch, "You have %d deity points.\n\r", ch->deity_points);

        if (ch->deity_type > 0) {
            Cprintf(ch, "You are a loyal follower of %s.\n\r", deity_table[ch->deity_type - 1].name);
        } else {
            Cprintf(ch, "You are atheist.  No God presently grants you favors.\n\r");
        }

        Cprintf(ch, "You can follow the following Gods: %s %s %s %s %s none.\n\r", deity_table[0].name, deity_table[1].name, deity_table[2].name, deity_table[3].name, deity_table[4].name);
        return;
    }

    if (!str_prefix(command, "none")) {
        Cprintf(ch, "You adopt atheist ways.\n\r");
        ch->deity_type = 0;
        ch->deity_points = 0;
        return;
    }

    if (!str_prefix(command, "price") || !str_prefix(command, "cost")) {
        Cprintf(ch, "The Gods offer the following favors:\n\r\n\r");
        Cprintf(ch, "%30s%8sminor%7s  %4slesser%4s  %4sgreater\n\r", "", "", "", "", "", "");
        for (dp = 0; dp < 5; dp++) {
            Cprintf(ch, "%-10s%-20s", deity_table[dp].name, deity_table[dp].spell);
            Cprintf(ch, "%d ticks for %d DP, %d for %d DP, %d for %d DP.\n\r", deity_table[dp].ticks_a, deity_table[dp].cost_a, deity_table[dp].ticks_b, deity_table[dp].cost_b, deity_table[dp].ticks_c, deity_table[dp].cost_c);
        }
        
        Cprintf(ch, "\n\r");
        Cprintf(ch, "You can claim your favour with \"worship claim <minor|lesser|greater>\".\n\r");
        Cprintf(ch, "If you do not specify one of minor, lesser or greater, the gods will assume you want the greatest blessing possible...\n\r");
        return;
    }

    if (!str_prefix(command, "claim")) {
        DEITY_TYPE deityInfo;
        int ticks;
        int cost;
        int claimLevel;
        char *claimLevelDesc;

        if (ch->deity_type == 0) {
            Cprintf(ch, "You follow no deity! How can you expect favors?\n\r");
            return;
        }

        deityInfo = deity_table[ch->deity_type - 1];

        if (is_affected(ch, skill_lookup(deityInfo.spell))) {
            Cprintf(ch, "You are already favored by %s.\n\r", deityInfo.name);
            return;
        }

        if (ch->deity_type == deity_lookup("Bhaal")) {
            if (ch->no_quit_timer > 0 && !IS_IMMORTAL(ch)) {
                Cprintf(ch, "Calm down a bit first eh!");
                return;
            }
        }

        if (argument[0] == '\0') {
            if (deityInfo.cost_c < ch->deity_points) {
                claimLevel = 3;
            } else if (deityInfo.cost_b < ch->deity_points) {
                claimLevel = 2;
            } else {
                claimLevel = 1;
            }
        } else if (!str_prefix(argument, "greater")) {
            claimLevel = 3;
        } else if (!str_prefix(argument, "lesser")) {
            claimLevel = 2;
        } else if (!str_prefix(argument, "minor")) {
            claimLevel = 1;
        } else {
            Cprintf(ch, "%s looks at you, confused.  What are you asking for?\n\r", deityInfo.name);
            return;
        }

        switch (claimLevel) {
            case 1:
                ticks = deityInfo.ticks_a;
                cost = deityInfo.cost_a;
                claimLevelDesc = "a minor";
                break;
            case 2:
                ticks = deityInfo.ticks_b;
                cost = deityInfo.cost_b;
                claimLevelDesc = "a lesser";
                break;
            case 3:
                ticks = deityInfo.ticks_c;
                cost = deityInfo.cost_c;
                claimLevelDesc = "his greatest";
                break;
        }

        if (ch->deity_points < cost) {
            Cprintf(ch, "%s refuses.  You do not have enough points for %s favor.\n\r", deityInfo.name, claimLevelDesc);
            return;
        }

        ch->deity_points -= cost;

        af.where = TO_AFFECTS;
        af.type = skill_lookup(deityInfo.spell);
        af.level = 56;
        af.duration = ticks;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        Cprintf(ch, "%s rewards you with %s favor for your loyalty!\n\r", deityInfo.name, claimLevelDesc);

        return;
    }

    deity_num = deity_lookup(command);

    if (deity_num == 0) {
        Cprintf(ch, "There is no such God for you to worship.\n\r");
        return;
    }

    if (ch->deity_type == deity_num) {
        Cprintf(ch, "Now that's just silly.  Do you want to anger %s?\n\r", deity_table[ch->deity_type - 1].name);
        return;
    }

    if (ch->deity_type != 0) {
        Cprintf(ch, "%s is a jealous god, and will only accept those who are not pledged to another.\n\r", deity_table[deity_num - 1].name);
        return;
    }

    Cprintf(ch, "You pledge your soul as a follower of %s.\n\r", deity_table[deity_num - 1].name);
    ch->deity_type = deity_num;
    ch->deity_points = 0;
	return;
}


void
do_sacrifice(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj, *contained;
	int silver;
	int tattoos = 0;
	/* variables for AUTOSPLIT */
	CHAR_DATA *gch;
	int members;
	char buffer[100];

	one_argument(argument, arg);

	if (arg[0] == '\0' || !str_cmp(arg, ch->name))
	{
		sprintf(buf, "$n offers $mself to %s, who graciously declines.", godName(ch));
		act(buf, ch, NULL, NULL, TO_ROOM, POS_RESTING);
		Cprintf(ch, "%s appreciates your offer and may accept it later.\n\r", godName(ch));
		return;
	}

	obj = get_obj_list(ch, arg, ch->in_room->contents);
	if (obj == NULL)
	{
		Cprintf(ch, "You can't find it.\n\r");
		return;
	}

	if (obj->item_type == ITEM_CORPSE_PC)
	{
		if (obj->contains)
		{
			Cprintf(ch, "%s wouldn't like that.\n\r", godName(ch));
			return;
		}
	}

	if (obj->item_type == ITEM_CONTAINER
	|| obj->item_type == ITEM_CORPSE_PC
	|| obj->item_type == ITEM_CORPSE_NPC) {
		for(contained = obj->contains; contained != NULL; contained = contained->next_content) {
			if(CAN_WEAR(contained, ITEM_NO_SAC)) {
				Cprintf(ch, "Some of the contents of that are not acceptable to the gods.\n\r");
				return;
			}
		}
	}

	if (!CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_NO_SAC))
	{
		act("$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR, POS_RESTING);
		return;
	}

	if (obj->in_room != NULL)
	{
		for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
			if (gch->on == obj)
			{
				act("$N appears to be using $p.", ch, obj, gch, TO_CHAR, POS_RESTING);
				return;
			}
	}

	silver = UMAX(1, obj->level * 3);

	if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
		silver = UMIN(silver, obj->cost);

	// Drain corpses
	if (obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC) {
		if (get_skill(ch, gsn_corpse_drain) > number_percent())
		{
			int amount = obj->level + (ch->level / 10 * dice(1, 10));

			Cprintf(ch, "You suck the remaining breath out of your prey.\n\r");
			ch->hit += amount;

			if (ch->hit > MAX_HP(ch)) 
				ch->hit = MAX_HP(ch);

			check_improve(ch, gsn_corpse_drain, TRUE, 6);
		}
		else
			check_improve(ch, gsn_corpse_drain, FALSE, 6);
	}

	if (ch->deity_type != 0 && (obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC))
	{
		silver = obj->level + (obj->level / 10 * dice(1, 3));
		if ((tattoos = power_tattoo_count(ch, TATTOO_EXTRA_DP)) > 0) {
        		Cprintf(ch, "The gods smile upon you, thanks to your tattoo.\n\r");
        		silver = silver + (silver * (50 + (25 * (tattoos -1))) / 100);
		}
		if(obj->owner && !str_cmp(ch->name, obj->owner)) {
			Cprintf(ch, "The gods would prefer if you sacrificed someone else's corpse.\n\r");
			silver = 0;
		}
		Cprintf(ch, "You offer your victory to %s who rewards you with %d deity points.\n\r", deity_table[ch->deity_type - 1].name, silver);
		act("$n offers $p to the gods.", ch, obj, NULL, TO_ROOM, POS_RESTING);

		ch->deity_points = ch->deity_points + silver;
		extract_obj(obj);
		return;
	}

	if (silver == 1)
		Cprintf(ch, "%s gives you one silver coin for your sacrifice.\n\r", godName(ch));
	else
		Cprintf(ch, "%s gives you %d silver coins for your sacrifice.\n\r", godName(ch), silver);

	ch->silver += silver;

	if (IS_SET(ch->act, PLR_AUTOSPLIT))
	{                    /* AUTOSPLIT code */
		members = 0;
		for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
			if (is_same_group(gch, ch))
				members++;

		if (members > 1 && silver > 1)
		{
			sprintf(buffer, "%d", silver);
			do_split(ch, buffer);
		}
	}

	sprintf(buf, "$n sacrifices $p to %s", godName(ch));
	act(buf, ch, obj, NULL, TO_ROOM, POS_RESTING);
	wiznet("$N sends up $p as a burnt offering.", ch, obj, WIZ_SACCING, 0, 0);
	extract_obj(obj);
	return;
}


void
do_quaff(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Quaff what?\n\r");
		return;
	}

	if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
	{
		Cprintf(ch, "You do not have that potion.\n\r");
		return;
	}

	if (obj->item_type != ITEM_POTION)
	{
		Cprintf(ch, "You can quaff only potions.\n\r");
		return;
	}

	if (ch->level < obj->level)
	{
		Cprintf(ch, "This liquid is too powerful for you to drink.\n\r");
		return;
	}

	if (IS_SET(obj->wear_flags, ITEM_NEWBIE) &&
		(ch->level > 9 || ch->remort || ch->reclass))
	{
		Cprintf(ch, "That potion is for weaklings.\n\r");
		return;
	}

	if (obj->pIndexData->area->security < 9 && !IS_IMMORTAL(ch))
	{
		Cprintf(ch, "You can't quaff potions from unfinished areas.\n\r");
		return;
	}

	if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 45)
	{
		Cprintf(ch, "You're too full to drink anymore.\n\r");
		return;
	}

	if (!IS_NPC(ch))
	{
		ch->pcdata->condition[COND_FULL] += 1;
	}

	act("$n quaffs $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
	act("You quaff $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);

	obj_cast_spell(obj->value[1], obj->value[0], ch, ch, NULL);
	obj_cast_spell(obj->value[2], obj->value[0], ch, ch, NULL);
	obj_cast_spell(obj->value[3], obj->value[0], ch, ch, NULL);
	obj_cast_spell(obj->value[4], obj->value[0], ch, ch, NULL);

	gain_condition(ch, COND_FULL, 5);

	WAIT_STATE(ch, PULSE_VIOLENCE / 2);

	if(ch->reclass == reclass_lookup("alchemist")
	&& number_percent() < 25) {
		Cprintf(ch, "You manage to conserve the potion.\n\r");
		return;
	}
	extract_obj(obj);
	return;
}


void
do_recite(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim = NULL;
	OBJ_DATA *scroll = NULL;
	OBJ_DATA *obj = NULL;
	int index;

	argument = one_argument(argument, arg1);
	target_name = argument;
	argument = one_argument(argument, arg2);

	if ((scroll = get_obj_carry(ch, arg1, ch)) == NULL)
	{
		Cprintf(ch, "You do not have that scroll.\n\r");
		return;
	}

	if (scroll->item_type != ITEM_SCROLL)
	{
		Cprintf(ch, "You can recite only scrolls.\n\r");
		return;
	}

	if (ch->level < scroll->level)
	{
		Cprintf(ch, "This scroll is too complex for you to comprehend.\n\r");
		return;
	}

	if (IS_SET(scroll->wear_flags, ITEM_NEWBIE) &&
		(ch->level > 9 || ch->remort || ch->reclass))
	{
		Cprintf(ch, "That potion is for weaklings.\n\r");
		return;
	}

	if (scroll->pIndexData->area->security < 9 && !IS_IMMORTAL(ch))
	{
		Cprintf(ch, "You can't recite scrolls from unfinished areas.\n\r");
		return;
	}

	if (number_percent() >= 20 + get_skill(ch, gsn_scrolls) * 4 / 5)
	{
        	Cprintf(ch, "You mispronounce a syllable.\n\r");
        	check_improve(ch, gsn_scrolls, FALSE, 2);
		extract_obj(scroll);
		return;
	}

	act("$n recites $p.", ch, scroll, NULL, TO_ROOM, POS_RESTING);
	act("You recite $p.", ch, scroll, NULL, TO_CHAR, POS_RESTING);


	for(index = 1; index < 5; index++)
	{
		if(arg2[0] == '\0')
		{
			if(ch->fighting)
				victim = ch->fighting;
			else
			{
				Cprintf(ch, "Recite it against who or what?\n\r");
				return;
			}
		}
		else {
			if (scroll->value[index] == gsn_gate
			|| scroll->value[index] == gsn_spirit_link
			|| scroll->value[index] == gsn_summon
			|| scroll->value[index] == gsn_carnal_reach)
			{
				if ((victim = get_char_world(ch, arg2, FALSE)) == NULL)
				{
		               		Cprintf(ch, "You failed.\n\r");
	            	   		return;
				}
			}
			// Let locate object do its own targetting
			else if(scroll->value[1] == gsn_locate_object)
			{
        			obj = NULL;
        			victim = NULL;
			}
			else if ((victim = get_char_room(ch, arg2)) == NULL
			&& (obj = get_obj_here(ch, arg2)) == NULL)
			{
				Cprintf(ch, "You can't find it.\n\r");
				return;
			}
		}

		obj_cast_spell(scroll->value[index], scroll->value[0], ch, victim, obj);
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	if(ch->reclass == reclass_lookup("alchemist")
	&& number_percent() < 25) {
        	Cprintf(ch, "You manage to conserve the scroll.\n\r");
        	return;
	}

	check_improve(ch, gsn_scrolls, TRUE, 2);
	extract_obj(scroll);
	return;
}


void
do_brandish(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	OBJ_DATA *staff;
	int sn;

	if ((staff = get_eq_char(ch, WEAR_HOLD)) == NULL)
	{
		Cprintf(ch, "You hold nothing in your hand.\n\r");
		return;
	}

	if (staff->item_type != ITEM_STAFF)
	{
		Cprintf(ch, "You can brandish only with a staff.\n\r");
		return;
	}

	if ((sn = staff->value[3]) < 0 || sn >= MAX_SKILL || skill_table[sn].spell_fun == 0)
	{
		bug("Do_brandish: bad sn %d.", sn);
		return;
	}

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

	if (staff->value[2] > 0)
	{
		act("$n brandishes $p.", ch, staff, NULL, TO_ROOM, POS_RESTING);
		act("You brandish $p.", ch, staff, NULL, TO_CHAR, POS_RESTING);
		if (ch->level < staff->level || number_percent() >= 20 + get_skill(ch, gsn_staves) * 4 / 5)
		{
			act("You fail to invoke $p.", ch, staff, NULL, TO_CHAR, POS_RESTING);
			act("...and nothing happens.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
			check_improve(ch, gsn_staves, FALSE, 2);
		}
		else
			for (vch = ch->in_room->people; vch; vch = vch_next)
			{
				vch_next = vch->next_in_room;

				switch (skill_table[sn].target)
				{
				default:
					bug("Do_brandish: bad target for sn %d.", sn);
					return;

				case TAR_IGNORE:
					if (vch != ch)
						continue;
					break;

				case TAR_CHAR_OFFENSIVE:
					if (IS_NPC(ch) ? IS_NPC(vch) : !IS_NPC(vch))
						continue;
					break;

				case TAR_CHAR_DEFENSIVE:
					if (IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch))
						continue;
					break;

				case TAR_CHAR_SELF:
					if (vch != ch)
						continue;
					break;
				}

				obj_cast_spell(staff->value[3], staff->value[0], ch, vch, NULL);
				check_improve(ch, gsn_staves, TRUE, 2);
			}
	}

	if(ch->reclass == reclass_lookup("alchemist")
	&& number_percent() < 25) {
        	Cprintf(ch, "You manage to conserve the charge.\n\r");
        	return;
	}

	if (--staff->value[2] <= 0)
	{
		act("$n's $p blazes bright and is gone.", ch, staff, NULL, TO_ROOM, POS_RESTING);
		act("Your $p blazes bright and is gone.", ch, staff, NULL, TO_CHAR, POS_RESTING);
		extract_obj(staff);
	}

	return;
}

void
do_zap(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *wand;
	OBJ_DATA *obj;

	target_name = argument;
	one_argument(argument, arg);
	if (arg[0] == '\0' && ch->fighting == NULL)
	{
		Cprintf(ch, "Zap whom or what?\n\r");
		return;
	}

	if ((wand = get_eq_char(ch, WEAR_HOLD)) == NULL)
	{
		Cprintf(ch, "You hold nothing in your hand.\n\r");
		return;
	}

	if (wand->item_type != ITEM_WAND && wand->item_type != ITEM_ORB)
	{
		Cprintf(ch, "You can zap only with a wand or an orb.\n\r");
		return;
	}

	/*  Orb interpect coded by Del */
	if (wand->item_type == ITEM_ORB)
	{

		if (arg[0] == '\0')
		{
			Cprintf(ch, "Zap whom or what?\n\r");
			return;
		}

		if ((victim = get_char_room(ch, arg)) == NULL)
		{
			Cprintf(ch, "That person isn't here.\n\r");
			return;
		}

		WAIT_STATE(ch, PULSE_VIOLENCE);

		if (IS_NPC(victim))
		{
			Cprintf(ch, "You can't use orbs on mobiles.\n\r");
			return;
		}

		if (victim == ch)
		{
			Cprintf(ch, "You mind meld with your own brain!\n\r");
			return;
		}

		if(wand->seek != NULL)
		{
			Cprintf(ch, "This orb is already empowered by someone's aura.\n\r");
			return;
		}

		if(ch->reclass != reclass_lookup("bounty hunter")) {
			Cprintf(ch, "Only bounty hunters can use orbs.\n\r");
			return;
		}

		if(number_percent() > get_skill(ch, gsn_hunter_ball)) {
			Cprintf(ch, "You failed.\n\r");
			check_improve(ch, gsn_hunter_ball, FALSE, 2);
			return;
		}

		damage(ch, victim, 0, gsn_hunter_ball, DAM_NONE, TRUE, TYPE_SKILL);
		Cprintf(ch, "The aura of %s flows through you.\n\r", victim->name);
		Cprintf(victim, "%s has linked your aura.\n\r", ch->name);
		wand->seek = str_dup(victim->name);
		sprintf(buf, "a hunter eye carrying the aura of %s", victim->name);
		free_string(wand->short_descr);
		wand->short_descr = str_dup(buf);
		free_string(wand->name);
		sprintf(buf, "hunter eye %s", victim->name);
		wand->name = str_dup(buf);
		wand->timer = number_range(1000, 1200);
		check_improve(ch, gsn_hunter_ball, TRUE, 2);
		return;
	}

	obj = NULL;
	if (arg[0] == '\0')
	{
		if (ch->fighting != NULL)
		{
			victim = ch->fighting;
		}
		else
		{
			Cprintf(ch, "Zap whom or what?\n\r");
			return;
		}
	}
	else
	{
		if (wand->value[3] == gsn_gate
		|| wand->value[3] == gsn_spirit_link
		|| wand->value[3] == gsn_summon
		|| wand->value[3] == gsn_carnal_reach)
		{
			if ((victim = get_char_world(ch, arg, FALSE)) == NULL)
			{
				Cprintf(ch, "You failed.\n\r");
				return;
			}
		}
		// Let locate object do its own targetting
		else if(wand->value[3] == gsn_locate_object)
		{
			obj = NULL;
			victim = NULL;
		}
		else if ((victim = get_char_room(ch, arg)) == NULL
		&& (obj = get_obj_here(ch, arg)) == NULL)
		{
			Cprintf(ch, "You can't find it.\n\r");
			return;
		}
	}

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

	if (wand->value[2] > 0)
	{
		if (victim != NULL)
		{
			act("$n zaps $N with $p.", ch, wand, victim, TO_NOTVICT, POS_RESTING);
			act("You zap $N with $p.", ch, wand, victim, TO_CHAR, POS_RESTING);
			act("$n zaps you with $p.", ch, wand, victim, TO_VICT, POS_RESTING);
		}
		else if(obj != NULL)
		{
			act("$n zaps $P with $p.", ch, wand, obj, TO_ROOM, POS_RESTING);
			act("You zap $P with $p.", ch, wand, obj, TO_CHAR, POS_RESTING);
		}
		else
		{
			act("$n waves $p in the air.", ch, wand, obj, TO_ROOM, POS_RESTING);
			act("You wave $p in the air.", ch, wand, obj, TO_CHAR, POS_RESTING);
		}
		if (ch->level < wand->level || number_percent() >= 20 + get_skill(ch, gsn_wands) * 4 / 5)
		{
			act("Your efforts with $p produce only smoke and sparks.", ch, wand, NULL, TO_CHAR, POS_RESTING);
			act("$n's efforts with $p produce only smoke and sparks.", ch, wand, NULL, TO_ROOM, POS_RESTING);
			check_improve(ch, gsn_wands, FALSE, 2);
		}
		else
		{
			obj_cast_spell(wand->value[3], wand->value[0], ch, victim, obj);
			check_improve(ch, gsn_wands, TRUE, 2);
		}
	}

	if(ch->reclass == reclass_lookup("alchemist")
	&& number_percent() < 25) {
        	Cprintf(ch, "You manage to conserve the charge.\n\r");
        	return;
	}

	if (--wand->value[2] <= 0)
	{
		act("$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM, POS_RESTING);
		act("Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR, POS_RESTING);
		extract_obj(wand);
	}

	return;
}



// This is what we called FUCKED UP
void focus_call(CHAR_DATA *ch, OBJ_DATA *focus, char *command) {
	int old_skill;
	int sn;

	if(IS_NPC(ch))
		return;

	// Set up
	sn = focus->special[3];
	old_skill = ch->pcdata->learned[sn];
	ch->real_level = ch->level; /* needed to for is_safe checks */

	ch->level = focus->special[0];
	ch->pcdata->learned[sn] = focus->special[4];
	ch->pcdata->any_skill = TRUE; /* needed to get around level restrict */

	// Action! (wow, this is fucked)
	interpret(ch, command);

	// Clean up
	ch->level = ch->real_level;
	ch->pcdata->learned[focus->special[3]] = old_skill;
	ch->pcdata->any_skill = 0;
	ch->real_level = 0;

	return;
}

void
do_focus(CHAR_DATA *ch, char* argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char command[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;

	argument = one_argument(argument, arg1);
	one_argument(argument, arg2);

	// no arguments
	if(*arg1 == '\0') {
		Cprintf(ch, "Huh? Try focus <object> |target|\n\r");
		return;
	}

	// if untargetted, fire on yourself
	if(*arg2 == '\0')
		sprintf(arg2, "%s", ch->name);

	// find the item we are focusing.
	obj = get_obj_list(ch, arg1, ch->carrying);
	if(obj == NULL
	|| obj->wear_loc == WEAR_NONE) {
		Cprintf(ch, "You aren't wearing that.\n\r");
		return;
	}

	// No charges
	if(!IS_SET(obj->wear_flags, ITEM_CHARGED)) {
		Cprintf(ch, "The object stubbornly ignores your effort.\n\r");
		return;
	}

	if(obj->special[2] < 1) {
		Cprintf(ch, "You try to focus the power of %s, but there's none left.\n\r", obj->short_descr);
		return;
	}

	if(ch->reclass == reclass_lookup("alchemist")
	&& number_percent() < 25)
        	Cprintf(ch, "You manage to conserve the charge.\n\r");
	else
		obj->special[2] -= 1;

	WAIT_STATE(ch, PULSE_VIOLENCE * 2);

	// If its a spell, easy to handle.
	if(skill_table[obj->special[3]].spell_fun != spell_null)
		sprintf(command, "cast '%s' %s", skill_table[obj->special[3]].name, arg2);
	// If this is not a spell... try and use its name to call it.
	// This won't work with lots of skills, but works for most
	// targetted ones.
	else
		sprintf(command, "%s %s", skill_table[obj->special[3]].name, arg2);

	Cprintf(ch, "You concentrate on %s...\n\r", obj->short_descr);
	act("$n concentrates on their charged item...", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	focus_call(ch, obj, str_dup(command));
	return;
}


void
do_steal(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char arg4[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *container;
	OBJ_DATA *obj;
	int percent;
	int chFlagged, victimFlagged;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);
	argument = one_argument(argument, arg4);

	if (arg1[0] == '\0' || arg2[0] == '\0')
	{
		Cprintf(ch, "Steal what from whom?\n\r");
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_ARENA))
	{
		Cprintf(ch, "No stealing in the Arena buddy.\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg2)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "That's pointless.\n\r");
		return;
	}

	if (IS_IMMORTAL(victim))
	{
		Cprintf(ch, "Steal from an Immortal? Really.\n\r");
		return;
	}

	if (victim->level > ch->level + 16)
	{
		Cprintf(ch, "You can't possibly steal from someone that experienced.\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	percent = number_percent();

	if (!IS_AWAKE(victim))
		percent -= 10;

	if (!can_see(victim, ch))
		percent -= 15;

	percent -= (get_curr_stat(ch, STAT_WIS) - get_curr_stat(victim, STAT_WIS * 5/4));
	percent -= (ch->level - victim->level) * 2;

	if (ch->reclass == reclass_lookup("rogue"))
	{
		if (arg4[0] != '\0' || !str_cmp(arg3, "in") || !str_cmp(arg3, "from"))
		{
			percent += 20;
		}
	}

	chFlagged = IS_SET(ch->act, PLR_KILLER) || IS_SET(ch->act, PLR_THIEF);
	victimFlagged = IS_SET(victim->act, PLR_KILLER) || IS_SET(victim->act, PLR_THIEF);

	if (!IS_NPC(victim) && !(IS_NPC(ch)))
	{
		if (ch->level + 8 < victim->level && !chFlagged)
		{
			Cprintf(ch, "I wouldn't do that, he'll kick your ass.\n\r");
			return;
		}

		if (ch->level - 8 > victim->level && !victimFlagged)
		{
			Cprintf(ch, "Try picking on someone your own size.\n\r");
			return;
		}
	}

	if (!IS_NPC(victim) && !is_clan(ch))
	{
		Cprintf(ch, "You're not in a clan, you can't steal from players.\n\r");
		return;
	}

	if (!is_clan(victim) && is_clan(ch) && !IS_NPC(victim))
	{
		Cprintf(ch, "You can't steal from them, they not in a clan.\n\r");
		return;
	}

	if (is_affected(ch, gsn_pacifism) && !IS_NPC(victim))
	{
		affect_strip(ch, gsn_pacifism);
	}

	if (percent > get_skill(ch, gsn_steal))
	{
		/*
		 * Failure.
		 */
		Cprintf(ch, "Oops.\n\r");
		affect_strip(ch, gsn_sneak);
		affect_strip(ch, gsn_hide);
		if(!IS_NPC(victim)) {
			ch->no_quit_timer = 3;
			victim->no_quit_timer = 3;
		}
		REMOVE_BIT(ch->affected_by, AFF_SNEAK);


		if (IS_SET(ch->toggles, TOGGLES_SOUND))
			Cprintf(ch, "!!SOUND(sounds/wav/stfail*.wav V=80 P=20 T=admin)");

		act("$n tried to steal from you.\n\r", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$n tried to steal from $N.\n\r", ch, NULL, victim, TO_NOTVICT, POS_RESTING);

		if (can_see(victim, ch))
		{
			switch (number_range(0, 3))
			{
			case 0:
				sprintf(buf, "%s is a lousy thief!", ch->name);
				break;
			case 1:
				sprintf(buf, "%s couldn't rob %s way out of a paper bag!", ch->name, (ch->sex == 2) ? "her" : "his");
				break;
			case 2:
				sprintf(buf, "%s tried to rob me!", ch->name);
				break;
			case 3:
				sprintf(buf, "Keep your hands out of there, %s!", ch->name);
				break;
			}
		}
		else
		{
			sprintf(buf, "Help Help, someone is trying to rob me!");
		}

		if (!IS_AWAKE(victim))
			do_wake(victim, "");

		if (IS_AWAKE(victim))
			do_yell(victim, buf);

		if (!IS_NPC(ch))
		{
			if (IS_NPC(victim))
			{
				check_improve(ch, gsn_steal, FALSE, 2);
				multi_hit(victim, ch, TYPE_UNDEFINED);
			}
			else
			{
				check_improve(ch, gsn_steal, FALSE, 2);

				sprintf(buf, "$N tried to steal from %s.", victim->name);
				wiznet(buf, ch, NULL, WIZ_FLAGS, 0, get_trust(ch));
				log_string("%s fails to steal from %s.", ch->name, victim->name);

				if (!IS_SET(ch->act, PLR_THIEF))
				{
					SET_BIT(ch->act, PLR_THIEF);
					Cprintf(ch, "*** You are now a THIEF!! ***\n\r");
					save_char_obj(ch, FALSE);
				}
			}
		}

		WAIT_STATE(ch, skill_table[gsn_steal].beats);
		return;
	}

	if (!str_cmp(arg1, "coin") || !str_cmp(arg1, "coins") || !str_cmp(arg1, "gold") || !str_cmp(arg1, "silver"))
	{
		int gold, silver;

		gold = victim->gold * number_range(1, ch->level) / 60;
		silver = victim->silver * number_range(1, ch->level) / 60;
		if (gold <= 0 && silver <= 0)
		{
			Cprintf(ch, "You couldn't get any coins.\n\r");
			return;
		}

		if(!IS_NPC(victim)) {
			ch->no_quit_timer = 3;
			victim->no_quit_timer = 3;
		}

		ch->gold += gold;
		ch->silver += silver;
		victim->silver -= silver;
		victim->gold -= gold;

		if (silver <= 0)
			sprintf(buf, "Bingo!  You got %d gold coins.\n\r", gold);
		else if (gold <= 0)
			sprintf(buf, "Bingo!  You got %d silver coins.\n\r", silver);
		else
			sprintf(buf, "Bingo!  You got %d silver and %d gold coins.\n\r",
					silver, gold);

		Cprintf(ch, "%s", buf);

		if (IS_SET(ch->toggles, TOGGLES_SOUND))
			Cprintf(ch, "!!SOUND(sounds/wav/steal*.wav V=80 P=20 T=admin)");

		if(!IS_NPC(victim)) {
			sprintf(buf, "$N steals coins from %s.", victim->name);
			wiznet(buf, ch, NULL, WIZ_FLAGS, 0, get_trust(ch));
			log_string("%s steals coins from %s.", ch->name, victim->name);
		}
		check_improve(ch, gsn_steal, TRUE, 2);
		WAIT_STATE(ch, 12);
		return;
	}

	if (ch->reclass == reclass_lookup("rogue"))
	{
		if ((!str_cmp(arg3, "in") && arg4[0] != '\0') ||
			(!str_cmp(arg3, "from") && arg4[0] != '\0') ||
			(arg3[0] != '\0' && arg4[0] == '\0'))
		{
			if (!str_cmp(arg3, "in") || !str_cmp(arg3, "from"))
			{
				if ((container = get_obj_carry(victim, arg4, ch)) == NULL)
				{
					if ((container = get_obj_wear(victim, arg4, ch)) == NULL)
					{
						Cprintf(ch, "You can't find the container.\n\r");
						return;
					}
				}
			}
			else
			{
				if ((container = get_obj_carry(victim, arg3, ch)) == NULL)
				{
					if ((container = get_obj_wear(victim, arg3, ch)) == NULL)
					{
						Cprintf(ch, "You can't find the container.\n\r");
						return;
					}
				}
			}

			if (container->item_type != ITEM_CONTAINER &&
				container->item_type != ITEM_CORPSE_NPC &&
				container->item_type != ITEM_CORPSE_PC)
			{
				Cprintf(ch, "That is not a container.\n\r");
				return;
			}

			if (IS_SET(container->value[1], CONT_CLOSED))
			{
				Cprintf(ch, "It is closed.\n\r");
				return;
			}

			if ((obj = get_obj_list(ch, arg1, container->contains)) == NULL)
			{
				Cprintf(ch, "That is not in there.\n\r");
				return;
			}

			if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
			{
				Cprintf(ch, "You have your hands full.\n\r");
				return;
			}

			if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch))
			{
				Cprintf(ch, "You can't carry that much weight.\n\r");
				return;
			}

			obj_from_obj(obj);
			obj_to_char(obj, ch);
			trap_symbol(ch, obj);
			act("You pocket $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
			WAIT_STATE(ch, 24);

			if(!IS_NPC(victim)) {
				ch->no_quit_timer = 3;
				victim->no_quit_timer = 3;
			}

			check_improve(ch, gsn_steal, TRUE, 2);
			Cprintf(ch, "Got it!\n\r");
			if (!IS_NPC(victim)) {
			    sprintf(buf, "$N steals %s from %s.", obj->short_descr, victim->name);
				wiznet(buf, ch, NULL, WIZ_FLAGS, 0, get_trust(ch));
				log_string("%s steals %s from %s.", ch->name, obj->short_descr, victim->name);
			}
			if (IS_SET(ch->toggles, TOGGLES_SOUND))
				Cprintf(ch, "!!SOUND(sounds/wav/steal*.wav V=80 P=20 T=admin)");

			return;
		}
	}

	if ((obj = get_obj_carry(victim, arg1, ch)) == NULL)
	{
		Cprintf(ch, "You can't find it.\n\r");
		return;
	}

	if (!can_drop_obj(ch, obj) || IS_SET(obj->extra_flags, ITEM_INVENTORY))
	{
		Cprintf(ch, "You can't pry it away.\n\r");
		return;
	}

	if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
	{
		Cprintf(ch, "You have your hands full.\n\r");
		return;
	}

	if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch))
	{
		Cprintf(ch, "You can't carry that much weight.\n\r");
		return;
	}

	obj_from_char(obj);
	obj_to_char(obj, ch);
	trap_symbol(ch, obj);
	act("You pocket $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);

	WAIT_STATE(ch, 24);
	if(!IS_NPC(victim)) {
		ch->no_quit_timer = 3;
		victim->no_quit_timer = 3;
	}

	check_improve(ch, gsn_steal, TRUE, 2);
	Cprintf(ch, "Got it!\n\r");
	if(!IS_NPC(victim)) {
	    sprintf(buf, "$N steals %s from %s.", obj->short_descr, victim->name);
 		wiznet(buf, ch, NULL, WIZ_FLAGS, 0, get_trust(ch));
 		log_string("%s steals %s from %s.", ch->name, obj->short_descr, victim->name);
	}
	if (IS_SET(ch->toggles, TOGGLES_SOUND))
		Cprintf(ch, "!!SOUND(sounds/wav/steal*.wav V=80 P=20 T=admin)");

	return;
}

void
do_swap(CHAR_DATA * ch, char *argument)
{
        char buf[MAX_STRING_LENGTH];
        char arg1[MAX_INPUT_LENGTH];
        char arg2[MAX_INPUT_LENGTH];
        CHAR_DATA *victim;
        OBJ_DATA *obj, *target;
        int percent;
        int chFlagged, victimFlagged;
        int wear_loc = WEAR_NONE;

        argument = one_argument(argument, arg1);
        argument = one_argument(argument, arg2);

        if (!IS_IMMORTAL(ch) && ch->reclass != reclass_lookup("rogue"))
        {
                Cprintf(ch, "That much dexterity requires training as a rogue.\n\r");
                return;
        }
	if (arg1[0] == '\0' || arg2[0] == '\0')
        {
                Cprintf(ch, "Try swap <left/right> <victim>\n\r");
                return;
        }

        if (IS_SET(ch->in_room->room_flags, ROOM_ARENA))
        {
                Cprintf(ch, "No stealing in the arena buddy.\n\r");
                return;
        }

        if ((victim = get_char_room(ch, arg2)) == NULL)
        {
                Cprintf(ch, "They aren't here.\n\r");
                return;
        }
	if (victim == ch)
        {
                Cprintf(ch, "That's pointless.\n\r");
                return;
        }
        if (IS_IMMORTAL(victim))
        {
                Cprintf(ch, "Steal from an Immortal? Really.\n\r");
                return;
        }
        if (is_safe(ch, victim))
                return;

        percent = number_percent();

        if (!IS_AWAKE(victim))
                percent -= 10;

        if (!can_see(victim, ch))
                percent -= 15;
	percent -= (get_curr_stat(ch, STAT_WIS) - get_curr_stat(victim, STAT_WIS * 5/4));
        percent -= (ch->level - victim->level) * 2;

        chFlagged = IS_SET(ch->act, PLR_KILLER) || IS_SET(ch->act, PLR_THIEF);
        victimFlagged = IS_SET(victim->act, PLR_KILLER) || IS_SET(victim->act, PLR_THIEF);

        if (!IS_NPC(victim) && !(IS_NPC(ch)))
        {
                if (ch->level + 8 < victim->level && !chFlagged)
                {
                        Cprintf(ch, "I wouldn't do that, he'll kick your ass.\n\r");
                        return;
                }
		if (ch->level - 8 > victim->level && !victimFlagged)
                {
                        Cprintf(ch, "Try picking on someone your own size.\n\r");
                        return;
                }
        }

        if (!IS_NPC(victim) && !is_clan(ch))
        {
                Cprintf(ch, "You're not in a clan, you can't steal from players.\n\r");
                return;
        }
	if (!is_clan(victim) && is_clan(ch) && !IS_NPC(victim))
        {
                Cprintf(ch, "You can't steal from them, they not in a clan.\n\r");
                return;
        }

        if (is_affected(ch, gsn_pacifism) && !IS_NPC(victim))
        {
		affect_strip(ch, gsn_pacifism);
        }

	if (percent > get_skill(ch, gsn_steal))
        {
                /*
                 * Failure.
                 */
                Cprintf(ch, "Oops.\n\r");
                affect_strip(ch, gsn_sneak);
		affect_strip(ch, gsn_hide);
		if(!IS_NPC(victim)) {
			ch->no_quit_timer = 3;
			victim->no_quit_timer = 3;
		}

                REMOVE_BIT(ch->affected_by, AFF_SNEAK);


                if (IS_SET(ch->toggles, TOGGLES_SOUND))
                        Cprintf(ch, "!!SOUND(sounds/wav/stfail*.wav V=80 P=20 T=admin)");

                act("$n tried to steal from you.\n\r", ch, NULL, victim, TO_VICT, POS_RESTING);
                act("$n tried to steal from $N.\n\r", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		if (can_see(victim, ch))
                {
                        switch (number_range(0, 3))
                        {
                        case 0:
                                sprintf(buf, "%s is a lousy thief!", ch->name);
                                break;
                        case 1:
                                sprintf(buf, "%s couldn't rob %s way out of a paper bag!", ch->name, (ch->sex == 2) ? "her" : "his");
                                break;
                        case 2:
                                sprintf(buf, "%s tried to rob me!", ch->name);
                                break;
                        case 3:
                                sprintf(buf, "Keep your hands out of there, %s!", ch->name);
                                break;
                        }
                }
		else
                        sprintf(buf, "Help Help, someone is trying robbing me!");

                if (!IS_AWAKE(victim))
                        do_wake(victim, "");

                if (IS_AWAKE(victim))
                        do_yell(victim, buf);

                if (!IS_NPC(ch))
                {
                        if (IS_NPC(victim))
                        {
                                check_improve(ch, gsn_steal, FALSE, 2);
                                multi_hit(victim, ch, TYPE_UNDEFINED);
                        }
			else
                        {
                        	check_improve(ch, gsn_steal, FALSE, 2);
                                sprintf(buf, "$N tried to steal from %s.", victim->name);
				wiznet(buf, ch, NULL, WIZ_FLAGS, 0, get_trust(ch));

                                if (!IS_SET(ch->act, PLR_THIEF))
                                {
                                        SET_BIT(ch->act, PLR_THIEF);
                                        Cprintf(ch, "*** You are now a THIEF!! ***\n\r");
                                        save_char_obj(ch, FALSE);
                                }
                        }
                }
		WAIT_STATE(ch, skill_table[gsn_steal].beats);
                return;
        }

        /* swap <finger> <target> */
        if (!str_prefix(arg1, "left"))
        {
                obj = get_eq_char(victim, WEAR_FINGER_L);
                wear_loc = WEAR_FINGER_L;
        }
        else if (!str_prefix(arg1, "right"))
        {
                obj = get_eq_char(victim, WEAR_FINGER_R);
                wear_loc = WEAR_FINGER_R;
        }
	else
        {
                Cprintf(ch, "Choose left or right finger to swap.\n\r");
                return;
        }

        if (obj == NULL)
        {
                Cprintf(ch, "They aren't wearing anything there!\n\r");
                return;
        }


	if (obj->level > ch->level) {
		Cprintf(ch, "You must be level %d to use %s\n\r",
			obj->level, obj->short_descr);
		return;
	}

        if (!can_drop_obj(victim, obj) || IS_SET(obj->extra_flags, ITEM_INVENTORY))
        {
                Cprintf(ch, "You can't pry it away.\n\r");
                return;
        }

	if (get_eq_char(ch, wear_loc) == NULL)
        {
                Cprintf(ch, "You have nothing to swap for that.\n\r");
                return;
        }

        obj_from_char(obj);
        obj_to_char(obj, ch);
	target = obj;
        obj = get_eq_char(ch, wear_loc);
        obj_from_char(obj);
        obj_to_char(obj, victim);
        equip_char(victim, obj, wear_loc);
	equip_char(ch, target, wear_loc);

        trap_symbol(ch, obj);
        act("You swap $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);

        WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	if(!IS_NPC(victim)) {
		victim->no_quit_timer = 3;
		ch->no_quit_timer = 3;
	}

        check_improve(ch, gsn_steal, TRUE, 2);
        Cprintf(ch, "Got it!\n\r");

	if (IS_SET(ch->toggles, TOGGLES_SOUND))
                Cprintf(ch, "!!SOUND(sounds/wav/steal*.wav V=80 P=20 T=admin)");

        return;
}

void
do_gravitation(CHAR_DATA * ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	int percent;
	int chFlagged, victimFlagged;

	if (get_skill(ch, gsn_gravitation) < 1)
	{
		Cprintf(ch, "Gravitation? What's that?\n\r");
		return;
	}

	argument = one_argument(argument, arg1);

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Gravitate from whom?\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg1)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (victim == ch)
	{
		Cprintf(ch, "That's pointless.\n\r");
		return;
	}

	if (IS_IMMORTAL(victim))
	{
		Cprintf(ch, "Gravitate from an Immortal? Really.\n\r");
		return;
	}

	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, skill_table[gsn_gravitation].beats);
	percent = number_percent();

	if (!IS_AWAKE(victim))
		percent -= 10;

	if (!can_see(victim, ch))
		percent -= 15;

	if (ch->level < victim->level)
		percent += (victim->level - ch->level) * 2;

	chFlagged = IS_SET(ch->act, PLR_KILLER) || IS_SET(ch->act, PLR_THIEF);
	victimFlagged = IS_SET(victim->act, PLR_KILLER) || IS_SET(victim->act, PLR_THIEF);

	if (!IS_NPC(victim) && !(IS_NPC(ch)))
	{
		if (ch->level + 12 < victim->level && !chFlagged)
		{
			Cprintf(ch, "I wouldn't do that, he'll kick your ass.\n\r");
			return;
		}

		if (ch->level - 12 > victim->level && !victimFlagged)
		{
			Cprintf(ch, "Try picking on someone your own size.\n\r");
			return;
		}
	}

	if (get_eq_char(victim, WEAR_FLOAT_2) == NULL)
	{
		Cprintf(ch, "There is nothing for you to gravitate in.\n\r");
		return;
	}

	if (!IS_NPC(victim) && !is_clan(ch))
	{
		Cprintf(ch, "Your not in a clan, you can't gravitate from players.\n\r");
		return;
	}

	if (!is_clan(victim) && is_clan(ch) && !IS_NPC(victim))
	{
		Cprintf(ch, "You can't gravitate from them, they not in a clan.\n\r");
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_ARENA))
        {
		Cprintf(ch, "No gravitating in the arena, sorry.\n\r");
		return;
        }

	if ((percent * 4 / 5) > get_skill(ch, gsn_gravitation))
	{
		/*
		 * Failure.
		 */
		Cprintf(ch, "DOH!.\n\r");
		affect_strip(ch, gsn_sneak);
		REMOVE_BIT(ch->affected_by, AFF_SNEAK);
		ch->no_quit_timer = 3;
		victim->no_quit_timer = 3;


		if (IS_SET(ch->toggles, TOGGLES_SOUND))
			Cprintf(ch, "!!SOUND(sounds/wav/stfail*.wav V=80 P=20 T=admin)");

		act("$n tried to gravitate from you.\n\r", ch, NULL, victim, TO_VICT, POS_RESTING);
		act("$n tried to gravitate from $N.\n\r", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		switch (number_range(0, 3))
		{
		case 0:
			sprintf(buf, "%s is a lousy thief!", ch->name);
			break;
		case 1:
			sprintf(buf, "%s couldn't gravitate dust to %sself!", ch->name, (ch->sex == 2) ? "her" : "him");
			break;
		case 2:
			sprintf(buf, "%s tried to pull me in!", ch->name);
			break;
		case 3:
			sprintf(buf, "Keep your hands off my stones %s!", ch->name);
			break;
		}

		if (!IS_AWAKE(victim))
			do_wake(victim, "");

		if (IS_AWAKE(victim))
			do_yell(victim, buf);

		if (!IS_NPC(ch))
		{
			if (IS_NPC(victim))
			{
				check_improve(ch, gsn_gravitation, FALSE, 2);
				multi_hit(victim, ch, TYPE_UNDEFINED);
			}
			else
			{
				sprintf(buf, "$N tried to gravitate from %s.", victim->name);
				wiznet(buf, ch, NULL, WIZ_FLAGS, 0, get_trust(ch));

				if (!IS_SET(ch->act, PLR_THIEF))
				{
					SET_BIT(ch->act, PLR_THIEF);
					Cprintf(ch, "*** You are now a THIEF!! ***\n\r");
					save_char_obj(ch, FALSE);
				}
			}
		}

		return;
	}


	obj = get_eq_char(victim, WEAR_FLOAT_2);

	if (!can_drop_obj(ch, obj) || IS_SET(obj->extra_flags, ITEM_INVENTORY))
	{
		Cprintf(ch, "You can't pry it away.\n\r");
		return;
	}

	if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
	{
		Cprintf(ch, "You have your hands full.\n\r");
		return;
	}

	if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch))
	{
		Cprintf(ch, "You can't carry that much weight.\n\r");
		return;
	}

	unequip_char(victim, obj);
	obj_from_char(obj);
	obj_to_char(obj, ch);
	trap_symbol(ch, obj);
	act("You drag away $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
	check_improve(ch, gsn_gravitation, TRUE, 2);
	ch->no_quit_timer = 3;
	victim->no_quit_timer = 3;

	if (IS_SET(ch->toggles, TOGGLES_SOUND))
		Cprintf(ch, "!!SOUND(sounds/wav/steal*.wav V=80 P=20 T=admin)");
	return;
}


/*
 * Shopping commands.
 */
CHAR_DATA *
find_keeper(CHAR_DATA * ch)
{
	/*char buf[MAX_STRING_LENGTH]; */
	CHAR_DATA *keeper;
	SHOP_DATA *pShop;

	pShop = NULL;
	for (keeper = ch->in_room->people; keeper; keeper = keeper->next_in_room)
		if (IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL)
			break;

	if (pShop == NULL)
	{
		Cprintf(ch, "You can't do that here.\n\r");
		return NULL;
	}

	/*
	 * Shop hours.
	 */
	if (time_info.hour < pShop->open_hour)
	{
		do_say(keeper, "Sorry, I am closed. Come back later.");
		return NULL;
	}

	if (time_info.hour > pShop->close_hour)
	{
		do_say(keeper, "Sorry, I am closed. Come back tomorrow.");
		return NULL;
	}

	/*
	 * Invisible or hidden people.
	 */
	if (!can_see(keeper, ch))
	{
		do_say(keeper, "I don't trade with folks I can't see.");
		return NULL;
	}

	return keeper;
}


/* insert an object at the right spot for the keeper */
void
obj_to_keeper(OBJ_DATA * obj, CHAR_DATA * ch)
{
	OBJ_DATA *t_obj, *t_obj_next;

	/* see if any duplicates are found */
	for (t_obj = ch->carrying; t_obj != NULL; t_obj = t_obj_next)
	{
		t_obj_next = t_obj->next_content;

		if (obj->pIndexData == t_obj->pIndexData && !str_cmp(obj->short_descr, t_obj->short_descr))
		{
			/* if this is an unlimited item, destroy the new one */
			if (IS_OBJ_STAT(t_obj, ITEM_INVENTORY))
			{
				extract_obj(obj);
				return;
			}
			obj->cost = t_obj->cost;   /* keep it standard */
			break;
		}
	}

	if (t_obj == NULL)
	{
		obj->next_content = ch->carrying;
		ch->carrying = obj;
	}
	else
	{
		obj->next_content = t_obj->next_content;
		t_obj->next_content = obj;
	}

	obj->carried_by = ch;
	obj->in_room = NULL;
	obj->in_obj = NULL;
	ch->carry_number += get_obj_number(obj);
	ch->carry_weight += get_obj_weight(obj);
}


/* get an object from a shopkeeper's list */
OBJ_DATA *
get_obj_keeper(CHAR_DATA * ch, CHAR_DATA * keeper, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int number;
	int count;

	number = number_argument(argument, arg);
	count = 0;
	for (obj = keeper->carrying; obj != NULL; obj = obj->next_content)
	{
		if (obj->wear_loc == WEAR_NONE && can_see_obj(keeper, obj) && can_see_obj(ch, obj) && is_name(arg, obj->name))
		{
			if (++count == number)
				return obj;

			/* skip other objects of the same name */
			while (obj->next_content != NULL && obj->pIndexData == obj->next_content->pIndexData && !str_cmp(obj->short_descr, obj->next_content->short_descr))
				obj = obj->next_content;
		}
	}

	return NULL;
}


int
get_cost(CHAR_DATA * keeper, OBJ_DATA * obj, bool fBuy)
{
	SHOP_DATA *pShop;
	int cost;

	if (obj == NULL || (pShop = keeper->pIndexData->pShop) == NULL)
		return 0;

	if (fBuy)
		cost = obj->cost * pShop->profit_buy / 100;
	else
	{
		OBJ_DATA *obj2;
		int itype;

		cost = 0;
		for (itype = 0; itype < MAX_TRADE; itype++)
		{
			if (obj->item_type == pShop->buy_type[itype])
			{
				cost = obj->cost * pShop->profit_sell / 100;
				break;
			}
		}

		if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT))
			for (obj2 = keeper->carrying; obj2; obj2 = obj2->next_content)
			{
				if (obj->pIndexData == obj2->pIndexData && !str_cmp(obj->short_descr, obj2->short_descr))
				{
					if (IS_OBJ_STAT(obj2, ITEM_INVENTORY))
						cost /= 2;
					else
						cost = cost * 3 / 4;
				}
			}
	}

	if (obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND)
	{
		if (obj->value[1] == 0)
			cost /= 4;
		else
			cost = cost * obj->value[2] / obj->value[1];
	}

	return cost;
}


void cheat_call_guard(CHAR_DATA *ch) {
    CHAR_DATA *pet;
    char buf[MAX_INPUT_LENGTH];

    Cprintf(ch, "You are caught and the authorities have been notified!\n\r");
    pet = create_mobile(get_mob_index(MOB_VNUM_EXECUTIONER));
    size_mob(pet, pet, ch->level + 8);
    pet->hunting = ch;
    pet->hunt_timer = 30;
    /* Set descriptions */
    sprintf(buf, "heavy town patrol watchman");
    free_string(pet->name);
    pet->name = str_dup(buf);
    sprintf(buf, "A heavily-armed town watchman is here.\n\r");
    free_string(pet->long_descr);
    pet->long_descr = str_dup(buf);
    sprintf(buf, "a heavily armed town watchman");
    free_string(pet->short_descr);
    pet->short_descr = str_dup(buf);
    size_mob(pet, pet, ch->level + 8);
    pet->spec_fun = NULL;
    SET_BIT(pet->toggles, TOGGLES_NOEXP);
    char_to_room(pet, ch->in_room);
    affect_strip(ch, gsn_cheat);

    return;
}

void
do_buy(CHAR_DATA * ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    int cost, roll;
    int chance, cheated = FALSE;

    if (argument[0] == '\0') {
        Cprintf(ch, "Buy what?\n\r");
        return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)) {
        char arg[MAX_INPUT_LENGTH];
        char buf[MAX_STRING_LENGTH];
        CHAR_DATA *pet;
        ROOM_INDEX_DATA *pRoomIndexNext;
        ROOM_INDEX_DATA *in_room;

        if (IS_NPC(ch)) {
            return;
        }

        argument = one_argument(argument, arg);

        /* hack to make new thalos pets work */
        if (ch->in_room->vnum == 9621) {
            pRoomIndexNext = get_room_index(9706);
        } else {
            pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);
        }

        if (pRoomIndexNext == NULL) {
            bug("Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum);
            Cprintf(ch, "Sorry, you can't buy that here.\n\r");
            return;
        }

        in_room = ch->in_room;
        if (in_room == NULL) {
            log_string("Character in NULL room");
            Cprintf(ch, "You are in a NULL room, please report to an imm immediately\n\r");
            return;
        }

        ch->in_room = pRoomIndexNext;
        pet = get_char_room(ch, arg);
        ch->in_room = in_room;
        
        if (pet == NULL || !IS_SET(pet->act, ACT_PET)) {
            Cprintf(ch, "Sorry, you can't buy that here.\n\r");
            return;
        }

        if (ch->pet != NULL) {
            Cprintf(ch, "You already own a pet.\n\r");
            return;
        }

        cost = 10 * pet->level * pet->level;

        if ((ch->silver + 100 * ch->gold) < cost) {
            Cprintf(ch, "You can't afford it.\n\r");
            return;
        }

        if (ch->level < pet->level) {
            Cprintf(ch, "You're not powerful enough to master this pet.\n\r");
            return;
        }

        if (is_affected(ch, gsn_cheat)) {
            chance = get_skill(ch, gsn_cheat) / 5;
            chance += get_curr_stat(ch, STAT_WIS);

            if (number_percent() < chance) {
                cheated = TRUE;
                Cprintf(ch, "You get away with your pet for free!\n\r");
            } else {
                // Catastrophic Failure based on cost.
                chance = cost / 200;

                if (number_percent() > chance) {
                    Cprintf(ch, "You try to cheat the shop, but find no opportunity.\n\r");
                } else {
                    cheat_call_guard(ch);
                    return;
                }
            }

            affect_strip(ch, gsn_cheat);
        }

        roll = number_percent();
        if (!cheated && roll < get_skill(ch, gsn_haggle)) {
            cost -= cost / 2 * roll / 100;
            Cprintf(ch, "You haggle the price down to %d coins.\n\r", cost);
            check_improve(ch, gsn_haggle, TRUE, 4);
        } else if (!cheated && get_skill(ch, gsn_haggle) > 0) {
            check_improve(ch, gsn_haggle, FALSE, 4);
        }

        if (!cheated) {
            deduct_cost(ch, cost);
        }

        pet = create_mobile(pet->pIndexData);
        SET_BIT(pet->act, ACT_PET);
        SET_BIT(pet->affected_by, AFF_CHARM);
        pet->comm = COMM_NOTELL | COMM_NOSHOUT | COMM_NOCHANNELS;

        argument = one_argument(argument, arg);
        if (arg[0] != '\0') {
            sprintf(buf, "%s %s", pet->name, arg);
            free_string(pet->name);
            pet->name = str_dup(buf);
        }

        sprintf(buf, "%sA neck tag says 'I belong to %s'.\n\r", pet->description, ch->name);
        free_string(pet->description);
        pet->description = str_dup(buf);

        char_to_room(pet, ch->in_room);
        add_follower(pet, ch);
        pet->leader = ch;
        ch->pet = pet;
        Cprintf(ch, "Enjoy your pet.\n\r");
        act("$n bought $N as a pet.", ch, NULL, pet, TO_ROOM, POS_RESTING);
        return;
    } else {
        CHAR_DATA *keeper;
        OBJ_DATA *obj, *t_obj;
        char arg[MAX_INPUT_LENGTH];
        int number, count = 1;

        if ((keeper = find_keeper(ch)) == NULL) {
            return;
        }

        number = mult_argument(argument, arg);
        obj = get_obj_keeper(ch, keeper, arg);
        cost = get_cost(keeper, obj, TRUE);

        if (number < 1 || number > 100) {
            act("$n tells you 'Get real!", keeper, NULL, ch, TO_VICT, POS_RESTING);
            return;
        }

        if (cost <= 0 || !can_see_obj(ch, obj)) {
            act("$n tells you 'I don't sell that -- try 'list''.", keeper, NULL, ch, TO_VICT, POS_RESTING);
            return;
        }

        if (!IS_OBJ_STAT(obj, ITEM_INVENTORY)) {
            for (t_obj = obj->next_content; count < number && t_obj != NULL; t_obj = t_obj->next_content) {
                if (t_obj->pIndexData == obj->pIndexData && !str_cmp(t_obj->short_descr, obj->short_descr)) {
                    count++;
                } else {
                    break;
                }
            }

            if (count < number) {
                act("$n tells you 'I don't have that many in stock.", keeper, NULL, ch, TO_VICT, POS_RESTING);
                return;
            }
        }

        if ((ch->silver + ch->gold * 100) < cost * number) {
            if (number > 1) {
                act("$n tells you 'You can't afford to buy that many.", keeper, obj, ch, TO_VICT, POS_RESTING);
            } else {
                act("$n tells you 'You can't afford to buy $p'.", keeper, obj, ch, TO_VICT, POS_RESTING);
            }

            return;
        }

        if (obj->level > ch->level) {
            act("$n tells you 'You can't use $p yet'.", keeper, obj, ch, TO_VICT, POS_RESTING);
            return;
        }

        if (ch->carry_number + number * get_obj_number(obj) > can_carry_n(ch)) {
            Cprintf(ch, "You can't carry that many items.\n\r");
            return;
        }

        if (ch->carry_weight + number * get_obj_weight(obj) > can_carry_w(ch)) {
            Cprintf(ch, "You can't carry that much weight.\n\r");
            return;
        }

        if (is_affected(ch, gsn_cheat)) {
            chance = get_skill(ch, gsn_cheat) / 5;
            chance += get_curr_stat(ch, STAT_WIS);
            chance -= (2 * number);

            if (number_percent() < chance) {
                cheated = TRUE;
                Cprintf(ch, "You use sleight of hand to pocket your purchase without paying!\n\r");
            } else {
                // Catastrophic Failure based on cost.
                chance = (number * cost) / 200;

                if (number_percent() > chance) {
                    Cprintf(ch, "You try to cheat the shop,but find no opportunity.\n\r");
                } else {
                    cheat_call_guard(ch);
                    return;
                }
            }

            affect_strip(ch, gsn_cheat);
        }

        /* haggle */

        roll = number_percent();
        if (!cheated && !IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT) && roll < get_skill(ch, gsn_haggle)) {
            cost -= obj->cost / 2 * roll / 100;
            act("You haggle with $N.", ch, NULL, keeper, TO_CHAR, POS_RESTING);
            check_improve(ch, gsn_haggle, TRUE, 4);
        } else if (!cheated && get_skill(ch, gsn_haggle) > 0) {
            check_improve(ch, gsn_haggle, FALSE, 1);
        }

        if (number > 1) {
            sprintf(buf, "$n buys $p[%d].", number);
            act(buf, ch, obj, NULL, TO_ROOM, POS_RESTING);
            sprintf(buf, "You buy $p[%d] for %d silver.", number, cost * number);
            act(buf, ch, obj, NULL, TO_CHAR, POS_RESTING);
        } else {
            act("$n buys $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
            sprintf(buf, "You buy $p for %d silver.", cost);
            act(buf, ch, obj, NULL, TO_CHAR, POS_RESTING);
        }

        if (!cheated) {
            deduct_cost(ch, cost * number);
        }

        keeper->gold += cost * number / 100;
        keeper->silver += cost * number - (cost * number / 100) * 100;

        for (count = 0; count < number; count++) {
            if (IS_SET(obj->extra_flags, ITEM_INVENTORY)) {
                t_obj = create_object(obj->pIndexData, obj->level);
            } else {
                t_obj = obj;
                obj = obj->next_content;
                obj_from_char(t_obj);
            }

            if (t_obj->timer > 0 && !IS_OBJ_STAT(t_obj, ITEM_HAD_TIMER)) {
                t_obj->timer = 0;
            }

            REMOVE_BIT(t_obj->extra_flags, ITEM_HAD_TIMER);
            obj_to_char(t_obj, ch);
            if (cost < t_obj->cost) {
                t_obj->cost = cost;
            }
        }
    }
}


void
do_list(CHAR_DATA * ch, char *argument) {
    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)) {
        ROOM_INDEX_DATA *pRoomIndexNext;
        CHAR_DATA *pet;
        bool found;

        /* hack to make new thalos pets work */
        if (ch->in_room->vnum == 9621) {
            pRoomIndexNext = get_room_index(9706);
        } else {
            pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);
        }

        if (pRoomIndexNext == NULL) {
            bug("Do_list: bad pet shop at vnum %d.", ch->in_room->vnum);
            Cprintf(ch, "You can't do that here.\n\r");
            return;
        }

        found = FALSE;
        for (pet = pRoomIndexNext->people; pet; pet = pet->next_in_room) {
            if (IS_SET(pet->act, ACT_PET)) {
                if (!found) {
                    found = TRUE;
                    Cprintf(ch, "Pets for sale:\n\r");
                }

                Cprintf(ch, "[%2d] %8d - %s\n\r", pet->level, 10 * pet->level * pet->level, pet->short_descr);
            }
        }

        if (!found) {
            Cprintf(ch, "Sorry, we're out of pets right now.\n\r");
        }

        return;
    } else {
        CHAR_DATA *keeper;
        OBJ_DATA *obj;
        int cost, count;
        bool found;
        char arg[MAX_INPUT_LENGTH];

        if ((keeper = find_keeper(ch)) == NULL) {
            return;
        }

        one_argument(argument, arg);

        found = FALSE;
        for (obj = keeper->carrying; obj; obj = obj->next_content) {
            if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj) && (cost = get_cost(keeper, obj, TRUE)) > 0 && (arg[0] == '\0' || is_name(arg, obj->name))) {
                if (!found) {
                    found = TRUE;
                    Cprintf(ch, "[Lv Price Qty] Item\n\r");
                }

                if (IS_OBJ_STAT(obj, ITEM_INVENTORY)) {
                    Cprintf(ch, "[%2d %5d -- ] %s\n\r", obj->level, cost, obj->short_descr);
                } else {
                    count = 1;

                    while (obj->next_content != NULL && obj->pIndexData == obj->next_content->pIndexData && !str_cmp(obj->short_descr, obj->next_content->short_descr)) {
                        obj = obj->next_content;
                        count++;
                    }

                    Cprintf(ch, "[%2d %5d %2d ] %s\n\r", obj->level, cost, count, obj->short_descr);
                }
            }
        }

        if (!found) {
            Cprintf(ch, "You can't buy anything here.\n\r");
        }

        return;
    }
}


void
do_sell(CHAR_DATA * ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char rest[MAX_INPUT_LENGTH];
    int number, i, foundCount;
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost, roll;
    int cheated = FALSE, chance;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        Cprintf(ch, "Sell what?\n\r");
        return;
    }

    if ((keeper = find_keeper(ch)) == NULL) {
        return;
    }

    number = mult_argument(arg, rest);
    foundCount = count_obj_carry_by_name(ch, rest, ch);

    if (foundCount <= 0) {
        Cprintf(ch, "%s tells you 'You don't have any %s'.\n\r", keeper->short_descr, rest);
        return;
    }

    if (foundCount < number) {
        Cprintf(ch, "%s tells you 'You don't have %d %s's'.\n\r", keeper->short_descr, number, rest);
        return;
    }

    if (number == 0) {
        Cprintf(ch, "You waste your time by doing nothing.\n\r");
        return;
    }

    if (number < 0) {
        Cprintf(ch, "Now, that's just stupid.\n\r");
        return;
    }

    for (i = 0; i < number; i++) {
        obj = get_obj_carry(ch, rest, ch);

        if (!can_drop_obj(ch, obj)) {
            Cprintf(ch, "You can't let go of it.\n\r");
            break;
        }

        if (!can_see_obj(keeper, obj)) {
            act("$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT, POS_RESTING);
            break;
        }

        if ((cost = get_cost(keeper, obj, FALSE)) <= 0) {
            act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT, POS_RESTING);
            break;
        }

        if (cost > (keeper->silver + 100 * keeper->gold)) {
            act("$n tells you 'I'm afraid I don't have enough wealth to buy $p.", keeper, obj, ch, TO_VICT, POS_RESTING);
            break;
        }

        act("$n sells $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);

        /* haggle */
        roll = number_percent();
        if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT) && roll < get_skill(ch, gsn_haggle)) {
            Cprintf(ch, "You haggle with the shopkeeper.\n\r");
            cost += obj->cost / 2 * roll / 100;
            cost = UMIN(cost, 95 * get_cost(keeper, obj, TRUE) / 100);
            cost = UMIN(cost, (keeper->silver + 100 * keeper->gold));
            check_improve(ch, gsn_haggle, TRUE, 4);
        }

        sprintf(buf, "You sell $p for %d silver and %d gold piece%s.", cost - (cost / 100) * 100, cost / 100, cost == 1 ? "" : "s");
        act(buf, ch, obj, NULL, TO_CHAR, POS_RESTING);

        if (ch->reclass == reclass_lookup("templar")) {
            cost = cost * 9/10;
            Cprintf(ch, "You tithe some of the gold.\n\r");
        }

        ch->gold += cost / 100;
        ch->silver += cost - (cost / 100) * 100;
        deduct_cost(keeper, cost);

        if (keeper->gold < 0) {
            keeper->gold = 0;
        }

        if (keeper->silver < 0) {
            keeper->silver = 0;
        }

        if (is_affected(ch, gsn_cheat)) {
            chance = get_skill(ch, gsn_cheat) / 5;
            chance += get_curr_stat(ch, STAT_WIS);
            chance -= (number * 2);

            if (number_percent() < chance) {
                cheated = TRUE;
                Cprintf(ch, "You use sleight of hand to give the shop a fake item and keep yours!\n\r");
            } else {
                // Catastrophic Failure based on cost.
                chance = cost / 200;

                if (number_percent() > chance) {
                    Cprintf(ch, "You try to cheat the shop, but find no opportunity.\n\r");
                } else {
                    cheat_call_guard(ch);
                    return;
                }
            }

            affect_strip(ch, gsn_cheat);
        }

        if (obj->item_type == ITEM_TRASH || IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)) {
            if(!cheated) {
                extract_obj(obj);
            }
        } else if(!cheated) {
            obj_from_char(obj);

            if (obj->timer) {
                SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
            } else {
                obj->timer = number_range(50, 100);
            }

            obj_to_keeper(obj, keeper);
        }
    }

    if (i > 1 && IS_SET(ch->toggles, TOGGLES_SOUND) && number_percent() > 75) {
        Cprintf(ch, "!!SOUND(sounds/wav/gold.wav V=80 P=20 T=admin)");
    }

    return;
}


void
do_value(CHAR_DATA * ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        Cprintf(ch, "Value what?\n\r");
        return;
    }

    if ((keeper = find_keeper(ch)) == NULL) {
        return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL) {
        act("$n tells you 'You don't have that item'.", keeper, NULL, ch, TO_VICT, POS_RESTING);
        return;
    }

    if (!can_see_obj(keeper, obj)) {
        act("$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT, POS_RESTING);
        return;
    }

    if (!can_drop_obj(ch, obj)) {
        Cprintf(ch, "You can't let go of it.\n\r");
        return;
    }

    if ((cost = get_cost(keeper, obj, FALSE)) <= 0) {
        act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT, POS_RESTING);
        return;
    }

    sprintf(buf, "$n tells you 'I'll give you %d silver and %d gold coins for $p'.", cost - (cost / 100) * 100, cost / 100);
    act(buf, keeper, obj, ch, TO_VICT, POS_RESTING);

    return;
}


/* NEW class skills */
void
do_lay_hands(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	int level; 
	int dice_type;
	int healed;

	if (ch->desc == NULL)
		return;

	if (ch->position < POS_SLEEPING)
	{
		Cprintf(ch, "You can't see anything but stars!\n\r");
		return;
	}

	if (ch->position == POS_SLEEPING)
	{
		Cprintf(ch, "You can't see anything, you're sleeping!\n\r");
		return;
	}

	argument = one_argument(argument, arg);

	level = ch->level;

	if (ch->race == race_lookup("gargoyle"))
	{
		level += 5;
	}

	if ((victim = get_char_room(ch, arg)) != NULL)
	{
		if (number_percent() < get_skill(ch, gsn_lay_hands))
		{
			if (ch->can_lay != 0)
			{
				Cprintf(ch, "You are too tired.\n\r");
				return;
			}

			act("You lay your hands on $N.", ch, NULL, victim, TO_CHAR, POS_RESTING);
			act("$n lays $s hands on you!", ch, NULL, victim, TO_VICT, POS_RESTING);
			act("$n lays $s hands on $N.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
			check_improve(ch, gsn_lay_hands, TRUE, 4);

			ch->can_lay = 1;
			
			// Heals 200-500 
			dice_type = 1 + wis_app[get_curr_stat(ch, STAT_WIS)].practice;
			healed = (3 * level) + dice(level, dice_type);
			if(!is_affected(ch, gsn_dissolution))
				victim->hit += healed;
			if (victim->hit > MAX_HP(victim))
				victim->hit = MAX_HP(victim);
			update_pos(victim);
			Cprintf(victim, "A warm feeling fills your body.\n\r");
			WAIT_STATE(ch, PULSE_VIOLENCE);
			return;
		}
		else
		{
			Cprintf(ch, "You failed your attempt to lay hands!\n\r");
			check_improve(ch, gsn_lay_hands, FALSE, 4);
			WAIT_STATE(ch, PULSE_VIOLENCE);
			return;
		}
	}
	else
	{
		Cprintf(ch, "They are not here.\n\r");
		return;
	}
}



void
do_guardian(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;
	char arg[MAX_INPUT_LENGTH];
	int level = ch->level;

	if (ch->desc == NULL)
		return;

	argument = one_argument(argument, arg);

	if (ch->move < 50 || ch->mana < 50)
	{
		Cprintf(ch, "You are too tired.\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They're not here.\n\r");
		return;
	}

	if (IS_NEUTRAL(ch)) {
		Cprintf(ch, "You aren't morally committed enough to use guardian.\n\r");
		return;
	}

	if(is_affected(victim, gsn_berserk)
	|| is_affected(victim, gsn_frenzy)
	|| is_affected(victim, gsn_taunt)) {
		if(victim == ch) {
			Cprintf(ch, "You're too enraged to act as a guardian!\n\r");
			return;
		}
		else {
			Cprintf(ch, "Your target is too enraged to be guarded right now.\n\r");
			return;
		}
	}

	/* Use BITS since theres more ways than one to get protect */
        if(IS_AFFECTED(victim, AFF_PROTECT_GOOD)
        || IS_AFFECTED(victim, AFF_PROTECT_EVIL)
        || IS_AFFECTED(victim, AFF_PROTECT_NEUTRAL))
        {
                act("$N is already protected from another alignment.", ch, NULL, victim, TO_CHAR, POS_RESTING);
                return;
        }

	if (number_percent() > get_skill(ch, gsn_guardian))
	{
		Cprintf(ch, "Your guardian skill fails you.\n\r");
		check_improve(ch, gsn_guardian, FALSE, 1);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
	}

	/* Ok, the skill works. Add proper protection! */
	ch->mana -= 50;
	ch->move -= 50;

	WAIT_STATE(ch, PULSE_VIOLENCE);

	/* They already have what we're going to give them. Refresh it. */
	if (IS_GOOD(ch) && is_affected(victim, gsn_protection_evil))
	{
		Cprintf(ch, "You refresh the protection from evil.\n\r");

		if(victim != ch)
		{
			act("$N refreshed your protection from evil.", victim, NULL, ch, TO_CHAR, POS_RESTING);
		}

		affect_refresh(victim, gsn_guardian, ch->level);
		return;
	}

	if (IS_EVIL(ch) && is_affected(victim, gsn_protection_good))
	{
		Cprintf(ch, "You refresh the protection from good.\n\r");

		if(victim != ch)
		{
			act("$N refreshed your protection from evil.", victim, NULL, ch, TO_CHAR, POS_RESTING);
		}

		affect_refresh(victim, gsn_guardian, ch->level);
		return;
	}

	if(ch->race == race_lookup("gargoyle")) {
		level = level + 10;
	}

	af.where    = TO_AFFECTS;
	af.type     = gsn_guardian;
	af.level    = level;
        af.duration = 5 + ((ch == victim) ? (level / 3) : 0);
	af.location = APPLY_SAVING_SPELL;
	af.modifier = -1 * (level / 10);

	af.bitvector = IS_EVIL(ch) ? AFF_PROTECT_GOOD : AFF_PROTECT_EVIL;
	affect_to_char(victim, &af);

	af.location = APPLY_AC;
	af.modifier = -5 * (level / 5); 
	af.bitvector = 0;
	affect_to_char(victim, &af);

	af.location = APPLY_DAMAGE_REDUCE;
	af.modifier = (level / 10); 
	affect_to_char(victim, &af);

	if(IS_GOOD(ch))
	{
		act("A warm glow guards you against evil.", victim, NULL, ch, TO_CHAR, POS_RESTING);
		act("$N glows warmly with benevolence.", victim, NULL, victim, TO_NOTVICT, POS_RESTING);
	}
	if(IS_EVIL(ch))
	{
		act("A dark aura guards you against good.", victim, NULL, ch, TO_CHAR, POS_RESTING);
		act("$N begins to shine with malevolence.", victim, NULL, victim, TO_NOTVICT, POS_RESTING);
	}

	check_improve(ch, gsn_guardian, TRUE, 1);

	return;
}

void do_protection(CHAR_DATA *ch, char *argument)
{
		  do_guardian(ch, argument);
}

void
do_cure_plague(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	int garg_bonus;

	if (ch->desc == NULL)
		return;

	if (ch->position < POS_SLEEPING)
	{
		Cprintf(ch, "You can't see anything but stars!\n\r");
		return;
	}

	if (ch->position == POS_SLEEPING)
	{
		Cprintf(ch, "You can't see anything, you're sleeping!\n\r");
		return;
	}

	argument = one_argument(argument, arg);

	garg_bonus = 0;
	if (ch->race == race_lookup("gargoyle"))
	{
		garg_bonus += 25;
	}

	if ((victim = get_char_room(ch, arg)) != NULL)
	{
		if (number_percent() < (get_skill(ch, gsn_cure_plague) + garg_bonus))
		{
			if (!is_affected(victim, gsn_plague))
			{
				if (victim == ch)
					Cprintf(ch, "You aren't ill.\n\r");
				else
					act("$N doesn't appear to be diseased.", ch, NULL, victim, TO_CHAR, POS_RESTING);

				return;
			}

			check_improve(ch, gsn_cure_plague, TRUE, 4);

			Cprintf(victim, "Your sores vanish.\n\r");
			act("$n looks relieved as $s sores vanish.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			affect_strip(victim, gsn_plague);
			WAIT_STATE(ch, PULSE_VIOLENCE * 2);
			return;
		}
		else
		{
			Cprintf(ch, "You failed your attempt to cure plague!\n\r");
			check_improve(ch, gsn_cure_plague, FALSE, 4);
			WAIT_STATE(ch, PULSE_VIOLENCE);
			return;
		}
	}
	else
	{
		Cprintf(ch, "They are not here.\n\r");
		return;
	}
}


void
add_shadow(CHAR_DATA * ch, CHAR_DATA * master)
{
	if (ch->master != NULL)
	{
		bug("Add_shadow: non-null master.", 0);
		return;
	}
	if (number_percent() < get_skill(ch, gsn_shadow))
	{
		ch->master = master;
		ch->leader = NULL;

		act("You now shadow $N.", ch, NULL, master, TO_CHAR, POS_RESTING);
		check_improve(ch, gsn_shadow, TRUE, 4);
		return;
	}
	else
	{
		act("You are caught trying to shadow $N!", ch, NULL, master, TO_CHAR, POS_RESTING);
		act("$n tries to shadow you!", ch, NULL, master, TO_VICT, POS_RESTING);
		check_improve(ch, gsn_shadow, FALSE, 4);
		return;
	}
}


void
stop_shadow(CHAR_DATA * ch)
{
	if (ch->master == NULL)
	{
		bug("Stop_shadow: null master.", 0);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM))
	{
		REMOVE_BIT(ch->affected_by, AFF_CHARM);
		affect_strip(ch, gsn_charm_person);
		affect_strip(ch, gsn_tame_animal);
	}

	if (can_see(ch->master, ch) && ch->in_room != NULL)
		act("You stop shadowing $N.", ch, NULL, ch->master, TO_CHAR, POS_RESTING);

	if (ch->master->pet == ch)
		ch->master->pet = NULL;

	ch->master = NULL;
	ch->leader = NULL;
	return;
}


void
do_shadow(CHAR_DATA * ch, char *argument)
{
/* RT changed to allow unlimited following and follow the NOFOLLOW rules */
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Shadow whom?\n\r");
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL)
	{
		act("But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR, POS_RESTING);
		return;
	}

	if (victim == ch)
	{
		if (ch->master == NULL)
		{
			Cprintf(ch, "You already follow yourself.\n\r");
			return;
		}

		stop_shadow(ch);
		return;
	}

	REMOVE_BIT(ch->act, PLR_NOFOLLOW);

	if (ch->master != NULL)
		stop_shadow(ch);

	if (ch->move < 20)
	{
		Cprintf(ch, "You are too exhausted.\n\r");
		return;
	}

	ch->move -= 20;
	add_shadow(ch, victim);
	WAIT_STATE(ch, PULSE_VIOLENCE * 1);
	return;
}

void
do_trash(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj, *contained;
	CHAR_DATA *gch;

	one_argument(argument, arg);


	if (arg[0] == '\0' || !str_cmp(arg, ch->name))
	{
		Cprintf(ch, "You have to trash junk that is in the room.\n\r");
		return;
	}

	obj = get_obj_list(ch, arg, ch->in_room->contents);

	if (obj == NULL)
	{
		Cprintf(ch, "You can't find it.\n\r");
		return;
	}

	if (obj->item_type == ITEM_CORPSE_PC)
	{
		if (obj->contains)
		{
			Cprintf(ch, "Sorry, that's not trash.\n\r");
			return;
		}
	}

	if (obj->item_type == ITEM_CONTAINER) {
	        for(contained = obj->contains; contained != NULL; contained = contained->next_content) {
                        if(CAN_WEAR(contained, ITEM_NO_SAC)) {
                                Cprintf(ch, "Some of the contents of that are not acceptable trash.\n\r");
                                return;
                        }
                }
        }

	if (!CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_NO_SAC))
	{
		act("$p is not trasheable.", ch, obj, 0, TO_CHAR, POS_RESTING);
		return;
	}

	if (obj->in_room != NULL)
	{
		for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
			if (gch->on == obj)
			{
				act("$N appears to be using $p.", ch, obj, gch, TO_CHAR, POS_RESTING);
				return;
			}
	}


	sprintf(buf, "$n trashes $p.");
	act(buf, ch, obj, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You trash %s.", obj->short_descr);
	wiznet("$N trashes $p.", ch, obj, WIZ_SACCING, 0, 0);
	extract_obj(obj);
	return;
}

void
do_appraise(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *target;
	int chance, blow_up, max_increase;

	one_argument(argument, arg);

	if (get_skill(ch, gsn_appraise) < 1)
	{
		Cprintf(ch, "It's probably worth something to someone somewhere.\n\r");
		return;
	}

	if (arg[0] == '\0')
	{
		Cprintf(ch, "What are you trying to appraise?\n\r");
		return;
	}

	target = get_obj_carry(ch, arg, ch);
	if (target == NULL)
	{
		Cprintf(ch, "You are not carrying that.\n\r");
		return;
	}

	if(ch->mana < 10)
	{
		Cprintf(ch, "You don't have enough energy to appraise anything right now.\n\r");
		return;
	}
	ch->mana -= 10;

	if (IS_OBJ_STAT(target, ITEM_SELL_EXTRACT)) {
		Cprintf(ch, "You can't appraise this object.\n\r");
		return;
	}
	chance = get_skill(ch, gsn_appraise);
	chance += 85;
	chance -= (int)((float)target->cost / (float)target->pIndexData->cost * 100);
	/* curve it back so really expensive items increase less */
	if(target->pIndexData->cost < 10000)
		 max_increase = 75;
	else
		 max_increase = 75 - (target->pIndexData->cost / 700);

	max_increase = URANGE(1, max_increase, 75);
	chance = URANGE(5, chance, 85);

	/* success!! */
	if (number_percent() < chance)
	{
		target->cost = target->cost + (number_range(1, max_increase) * target->cost) / 50;
		act("You skillfully appraise $p!", ch, target, NULL, TO_CHAR, POS_RESTING);
		act("$p glitters and shines more brightly as $n appraises it.", ch, target, NULL, TO_ROOM, POS_RESTING);
		check_improve(ch, gsn_appraise, TRUE, 2);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
	}

	/* item may be destroyed if you fail. */
	blow_up = number_percent();
	if (blow_up < 20)
	{
		act("$p is wrecked by your appraisal!", ch, target, NULL, TO_CHAR, POS_RESTING);
		act("$p crumbles into dust at $n's touch.", ch, target, NULL, TO_ROOM, POS_RESTING);
		extract_obj(target);
	}
	else if(blow_up < 50)
	{
		target->cost = UMAX(1, target->cost - (number_range(1, 100) * target->cost) / 250);
		act("You botch your appraise on $p and ruin its value", ch, target, NULL, TO_CHAR, POS_RESTING);
		act("$p dulls and fades as $n appraises it.", ch, target, NULL, TO_ROOM, POS_RESTING);
	}
	else
	{
		act("You are unable to appraise the value of $p.", ch, target, NULL, TO_CHAR, POS_RESTING);
	}
	check_improve(ch, gsn_appraise, FALSE, 2);
	WAIT_STATE(ch, PULSE_VIOLENCE);
	return;
}

void do_rub_dirt(CHAR_DATA *ch, char* argument) {
	int chance;
	int sn;

	if(get_skill(ch, gsn_rub) < 1) {
		Cprintf(ch, "Rub who, what, where?\n\r");
		return;
	}

	if(!is_affected(ch, gsn_dirt_kicking)
		&& !is_affected(ch, gsn_fire_breath)) {
		Cprintf(ch, "Your eyes are working fine, so you scratch an itch instead.\n\r");
		return;
	}

	if(is_affected(ch, gsn_dirt_kicking))
		sn = gsn_dirt_kicking;
	if(is_affected(ch, gsn_fire_breath))
		sn = gsn_fire_breath;

	chance = get_skill(ch, gsn_rub) * 4 / 5;

	Cprintf(ch, "You take a moment to clear your eyes.\n\r");

	WAIT_STATE(ch, PULSE_VIOLENCE);

	if(number_percent() < chance)
	{
		affect_strip(ch, sn);
		if (skill_table[sn].msg_off)
		{
			Cprintf(ch, skill_table[sn].msg_off);
			Cprintf(ch, "\n\r");
		}
		act("$n takes a moment to clear $s eyes.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		check_improve(ch, gsn_rub, TRUE, 2);
		return;
	}

	Cprintf(ch, "But nothing happens.\n\r");
	check_improve(ch, gsn_rub, FALSE, 2);

	return;
}

int number_potion_effects(OBJ_DATA* pot)
{
	int num;

	if (pot == NULL)
		return 0;

	num = 0;
	if (pot->value[1] > 0)
		num++;
	if (pot->value[2] > 0)
		num++;
	if (pot->value[3] > 0)
		num++;

	return num;
}

void
do_combine_potion(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	char arg3[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA* vial;
	OBJ_DATA* potion1 = NULL;
	OBJ_DATA* potion2 = NULL;
	OBJ_DATA* potion3 = NULL;
	int num_affs;
	int chance;
	int new_level;
	int index;
	int cur_index;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	one_argument(argument, arg3);

	if (get_skill(ch, gsn_combine_potion) < 1)
	{
		Cprintf(ch, "Are you crazy, better leave the alchemy to someone more qualified!\n\r");
		return;
	}

	if (arg1[0] == '\0' && arg2[0] == '\0')
	{
		Cprintf(ch, "What potions are you trying to mix?\n\r");
		return;
	}

	potion1 = get_obj_carry(ch, arg1, ch);
	if (potion1 == NULL)
	{
		Cprintf(ch, "You are not carrying that.\n\r");
		return;
	}

	potion2 = get_obj_carry(ch, arg2, ch);
	if (potion2 == NULL)
	{
		Cprintf(ch, "You are not carrying that.\n\r");
		return;
	}

	if (arg3[0] != '\0')
	{
		potion3 = get_obj_carry(ch, arg3, ch);
		if (potion3 == NULL)
		{
			Cprintf(ch, "You are not carrying that.\n\r");
			return;
		}
	}

	num_affs = number_potion_effects(potion1);
	num_affs += number_potion_effects(potion2);
	if (potion3 != NULL)
		num_affs += number_potion_effects(potion3);

	if (num_affs > 3)
	{
		Cprintf(ch, "Not even you could get that much in one potion.\n\r");
		return;
	}

	vial = get_eq_char(ch, WEAR_HOLD);
	if (vial == NULL
	|| !is_name("vial empty potion _unused_", vial->name))
	{
		Cprintf(ch, "You must be holding an empty vial for mixing.\n\r");
		return;
	}

	if (!is_name("_unused_", vial->name))
	{
		Cprintf(ch, "This vial has already been used.\n\r");
		return;
	}

	if(potion1 == NULL || potion2 == NULL) {
		Cprintf(ch, "You need more potions when mixing!\n\r");
		return;
	}

	chance = get_skill(ch, gsn_combine_potion);
	if (potion1 != NULL)
		chance -= 15;
	if (potion2 != NULL)
		chance -= 15;
	if (potion3 != NULL)
		chance -= 20;

	chance += get_curr_stat(ch, STAT_INT);

	chance = URANGE(5, chance, 85);

	/* now the _hard_ part.. the mix! */
	index = 1;
	vial->value[1] = -1;
	vial->value[2] = -1;
	vial->value[3] = -1;
	/* find the 2-3 affects */
	for (cur_index = 1; cur_index <= 3; cur_index++)
	{
		if (potion1->value[cur_index] > 0)
			vial->value[index++] = potion1->value[cur_index];
	}

	for (cur_index = 1; cur_index <= 3; cur_index++)
	{
		if (potion2->value[cur_index] > 0)
			vial->value[index++] = potion2->value[cur_index];
	}

	if (potion3 != NULL)
	{
		for (cur_index = 1; cur_index <= 3; cur_index++)
		{
			if (potion3->value[cur_index] > 0)
				vial->value[index++] = potion3->value[cur_index];
		}
	}

	/* item level */
	new_level = potion1->level;
	new_level += potion2->level / 2;
	if (potion3 != NULL)
	{
		new_level += potion3->level / 2;
	}

	vial->level = UMIN(new_level, 51);

	/* KABOOM! */
	if (number_percent() > chance ||
		vial->value[1] == vial->value[2] ||
		vial->value[1] == vial->value[3] ||
		vial->value[2] == vial->value[3])
	{
		act("You botch the mix!", ch, NULL, NULL, TO_CHAR, POS_RESTING);
		act("$n botches the potion mix!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		extract_obj(vial);
		act("$p flares blindingly... and explodes!", ch, potion1, NULL, TO_ROOM, POS_RESTING);
		extract_obj(potion1);
		act("$p flares blindingly... and explodes!", ch, potion2, NULL, TO_ROOM, POS_RESTING);
		extract_obj(potion2);
		if (potion3 != NULL)
		{
			act("$p flares blindingly... and explodes!", ch, potion3, NULL, TO_ROOM, POS_RESTING);
			extract_obj(potion3);
		}

		damage(ch, ch, dice(ch->level / 2, 8), gsn_combine_potion, DAM_FIRE, TRUE, TYPE_MAGIC);

		check_improve(ch, gsn_combine_potion, FALSE, 2);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
	}

	act("You mix the potions together!", ch, NULL, NULL, TO_CHAR, POS_RESTING);
	act("$n mixes some potions together.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	/* k, now the new spell level is the average of the spell levels */
	new_level = potion1->value[0] + potion2->value[0];
	if (potion3 != NULL)
	{
		new_level += potion3->value[0];
		new_level /= 3;
	}
	else
		new_level /= 2;

	vial->value[0] = new_level;
	vial->timer = number_range(1000, 1200);

	/* item name */
	if (vial->value[3] >= 0)
	{
		sprintf(buf, "multi-layered potion %s %s %s",
			skill_table[vial->value[1]].name,
			skill_table[vial->value[2]].name,
			skill_table[vial->value[3]].name);
		free_string(vial->name);
		vial->name = str_dup(buf);

		sprintf(buf, "a multi-layered potion of %s, %s and %s",
			skill_table[vial->value[1]].name,
			skill_table[vial->value[2]].name,
			skill_table[vial->value[3]].name);
		free_string(vial->short_descr);
		vial->short_descr = str_dup(buf);

		sprintf(buf, "A multi-layered potion of %s, %s and %s.",
			skill_table[vial->value[1]].name,
			skill_table[vial->value[2]].name,
			skill_table[vial->value[3]].name);
		free_string(vial->description);
		vial->description = str_dup(buf);
	}
	else
	{
		sprintf(buf, "layered potion %s %s",
			skill_table[vial->value[1]].name,
			skill_table[vial->value[2]].name);
		free_string(vial->name);
		vial->name = str_dup(buf);

		sprintf(buf, "a layered potion of %s and %s",
			skill_table[vial->value[1]].name,
			skill_table[vial->value[2]].name);
		free_string(vial->short_descr);
		vial->short_descr = str_dup(buf);

		sprintf(buf, "A layered potion of %s and %s.",
			skill_table[vial->value[1]].name,
			skill_table[vial->value[2]].name);
		free_string(vial->description);
		vial->description = str_dup(buf);
	}

	extract_obj(potion1);
	extract_obj(potion2);
	if (potion3 != NULL)
		extract_obj(potion3);

	check_improve(ch, gsn_combine_potion, TRUE, 2);
	WAIT_STATE(ch, PULSE_VIOLENCE);
	return;
}


void
do_scribe(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA* scroll;
	int chance;
	int sn;
	int mana;
	int level;

	argument = one_argument(argument, arg1);

	if (get_skill(ch, gsn_scribe) < 1)
	{
		Cprintf(ch, "Are you crazy, you can't make scrolls.. you don't even have the right quill!\n\r");
		return;
	}

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "What exactly are you trying to scribe?\n\r");
		return;
	}

	scroll = get_eq_char(ch, WEAR_HOLD);
	if (scroll == NULL
	|| !is_name("blank scroll _unused_", scroll->name))
	{
		Cprintf(ch, "You must be holding a blank scroll for this.\n\r");
		return;
	}

	if (!is_name("_unused_", scroll->name))
	{
		Cprintf(ch, "This scroll has already been used.\n\r");
		return;
	}

	if ((sn = find_spell(ch, arg1)) < 1
		|| skill_table[sn].spell_fun == spell_null
		|| (!IS_NPC(ch) && (ch->level < skill_table[sn].skill_level[ch->charClass]
			|| ch->pcdata->learned[sn] == 0)))
	{
		Cprintf(ch, "You don't know any spells of that name.\n\r");
		return;
	}

	if (skill_table[sn].remort) {
		Cprintf(ch, "This spell is too specialized to scribe.\n\r");
		return;
	}

	if (sn == gsn_acid_breath
	 || sn == gsn_frost_breath
	 || sn == gsn_gas_breath
	 || sn == gsn_fire_breath
	 || sn == gsn_lightning_breath)
	{
		if (!IS_NPC(ch))
		{
			Cprintf(ch, "You don't know any spells of that name.\n\r");
			return;
		}
	}

	argument = one_argument(argument, arg2);
	if (is_number(arg2))
	{
		level = UMIN(ch->level, atoi(arg2));
	}
	else
	{
		level = ch->level;
	}

	if (ch->level + 2 == skill_table[sn].skill_level[ch->charClass])
		mana = 50;
	else
		mana = UMAX(skill_table[sn].min_mana,
						100 / (2 + ch->level - skill_table[sn].skill_level[ch->charClass]));

	mana += 25;

	if (!IS_NPC(ch) && ch->mana < mana)
	{
		Cprintf(ch, "You don't have enough mana.\n\r");
		return;
	}

	chance = get_skill(ch, gsn_scribe);

	/* 100% -> 50 + level difference */
	chance += ch->level - skill_table[sn].skill_level[ch->charClass];
	chance -= 20;
	chance -= 100 - ch->pcdata->learned[sn];

	chance = URANGE(5, chance, 85);

	/* KABOOM! */
	if (number_percent() > chance)
	{
		act("You botch the scroll!", ch, NULL, NULL, TO_CHAR, POS_RESTING);
		act("$n botches the scroll!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		damage(ch, ch, dice(ch->level, 4), gsn_scribe, DAM_COLD, TRUE, TYPE_MAGIC);
		extract_obj(scroll);
		ch->mana -= mana / 2;

		check_improve(ch, gsn_scribe, FALSE, 2);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
	}

	act("You scribe a new scroll!", ch, NULL, NULL, TO_CHAR, POS_RESTING);
	act("$n scribes a new scroll.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	ch->mana -= mana;

	scroll->level = level;
	scroll->value[0] = level * 4 / 5;
	scroll->value[1] = sn;
	scroll->value[2] = -1;
	scroll->value[3] = -1;
	scroll->timer = number_range(1000, 1200);

	sprintf(buf, "scroll %s",
		skill_table[sn].name);
	free_string(scroll->name);
	scroll->name = str_dup(buf);

	sprintf(buf, "a scroll of %s",
		skill_table[sn].name);
	free_string(scroll->short_descr);
	scroll->short_descr = str_dup(buf);

	sprintf(buf, "A scroll of %s.",
		skill_table[sn].name);
	free_string(scroll->description);
	scroll->description = str_dup(buf);

	check_improve(ch, gsn_scribe, TRUE, 2);
	WAIT_STATE(ch, PULSE_VIOLENCE);
	return;
}

void
do_carve_spear(CHAR_DATA * ch, char *argument) {
	 OBJ_DATA *obj;
		  int skill;

		  if (get_skill(ch, gsn_carve_spear) < 1)
		  {
					 Cprintf(ch, "You lack the skill to carve spears.\n\r");
					 return;
		  }

		  skill = get_skill(ch, gsn_carve_spear);

		  if (ch->move < 50)
		  {
					 Cprintf(ch, "You are too tired to carve spears.\n\r");
					 return;
		  }
	if (ch->in_room->sector_type != SECT_FIELD
					 && ch->in_room->sector_type != SECT_FOREST
					 && ch->in_room->sector_type != SECT_SWAMP)
		  {
					 Cprintf(ch, "There are no tree limbs to carve spears from around here.\n\r");
					 return;
		  }

		  ch->move = ch->move - 50;
		  if (ch->move < 0)
					 ch->move = 0;

		  if (number_percent() > skill)
		  {
					 Cprintf(ch, "Your spear cracks like a match.\n\r");
					 check_improve(ch, gsn_carve_spear, FALSE, 5);
					 return;
		  }

		  obj = create_object(get_obj_index(OBJ_VNUM_CARVED_SPEAR), 0);

		  obj->value[0] = ch->level / 10;
		  obj->value[1] = ch->level / 6;
		  obj->value[3] = ch->level * 4/5;
		  obj->level = ch->level * 4/5;
		  obj->timer = number_range(1000, 1200);

		  obj_to_char(obj, ch);
		  act("$n slices a tree branch and creates $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		  act("You slice a tree branch into $p.", ch, obj, NULL, TO_CHAR, POS_RESTING);
		  check_improve(ch, gsn_carve_spear, TRUE, 5);
		  return;

}

void
do_carve(CHAR_DATA * ch, char *argument)
{
		  if(!str_prefix(argument, "boulder"))
					 do_carve_boulder(ch, "");
		  else if(!str_prefix(argument, "spear"))
					 do_carve_spear(ch, "");
		  else
					 Cprintf(ch, "What are you trying to carve?\n\r");
					 return;
}

void
do_hunter_ball(CHAR_DATA* ch, char* argument)
{
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *wand;

	target_name = argument;
	one_argument(argument, arg);

	if (ch->reclass != reclass_lookup("bounty hunter"))
	{
		Cprintf(ch, "Only bounty hunters can use these.\n\r");
		return;
	}

	if (!str_cmp("create", argument))
	{
		wand = create_object(get_obj_index(OBJ_VNUM_HUNTER_BALL), 0);
		Cprintf(ch, "You conjure a magical hunting ball.\n\r");
		obj_to_char(wand, ch);
		return;
	}

	if ((wand = get_eq_char(ch, WEAR_HOLD)) == NULL)
	{
		Cprintf(ch, "You are not holding an orb in your hand.\n\r");
		return;
	}

	if (wand->item_type != ITEM_ORB)
	{
		Cprintf(ch, "You can only conjure an aura with an orb.\n\r");
		return;
	}

	if (wand->seek == NULL)
	{
		Cprintf(ch, "This orb has not been empowered with anyone's aura.\n\r");
		return;
	}

	if ((victim = get_char_world(ch, wand->seek, TRUE)) == NULL)
	{
		Cprintf(ch, "This soul cannot be reached right now.\n\r");
		return;
	}

	if (!can_see(ch, victim))
	{
		Cprintf(ch, "This soul cannot be reached right now.\n\r");
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	if (get_skill(ch, gsn_hunter_ball) < number_percent())
	{
		Cprintf(ch, "You zap the orb, but nothing happens.\n\r");
		check_improve(ch, gsn_hunter_ball, FALSE, 2);
		act("$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM, POS_RESTING);
		act("Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR, POS_RESTING);
		extract_obj(wand);
		return;
	}

	check_improve(ch, gsn_hunter_ball, TRUE, 2);

	if(!IS_AWAKE(victim)) {
		Cprintf(ch, "You see only inky darkness.\n\r");
		act("$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM, POS_RESTING);
		act("Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR, POS_RESTING);
		extract_obj(wand);
		return;
	}

	Cprintf(ch, "You look through the eyes of %s and see...\n\r", victim->name);
	sprintf(buf, "%s look", victim->name);
	do_at(ch, buf);
	act("$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM, POS_RESTING);
	act("Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR, POS_RESTING);
	extract_obj(wand);
	return;
}

// Specialized and simplified version of do_cast at a distance.
void do_voodoo(CHAR_DATA *ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim, *old_fighting;
	OBJ_DATA *obj;
	void *vo;
	int mana;
	int sn;
	int target;
	int spell_level;

	target_name = one_argument(argument, arg1);
	one_argument(target_name, arg2);

	if (ch->reclass != reclass_lookup("shaman")) {
		Cprintf(ch, "Only shamans know voodoo.\n\r");
		return;
	}

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Cast which what where?\n\r");
		return;
	}

	if ((sn = find_spell(ch, arg1)) < 1
		|| skill_table[sn].spell_fun == spell_null
		|| (!IS_NPC(ch) && (ch->level < skill_table[sn].skill_level[ch->charClass]
		|| ch->pcdata->learned[sn] == 0)))
	{
		Cprintf(ch, "You don't know any spells of that name.\n\r");
		return;
	}

	if (ch->position < skill_table[sn].minimum_position)
	{
		Cprintf(ch, "You can't concentrate enough.\n\r");
		return;
	}

	obj = get_eq_char(ch, WEAR_HOLD);
	if(obj == NULL ||
		(obj->item_type != ITEM_WAND
		&& obj->value[3] != gsn_voodoo_doll))
	{
		Cprintf(ch, "You don't have a doll to use magic on.\n\r");
		return;
	}

	if (ch->level + 2 == skill_table[sn].skill_level[ch->charClass])
		mana = 50;
	else
		mana = UMAX(skill_table[sn].min_mana,
			100 / (2 + ch->level - skill_table[sn].skill_level[ch->charClass]));

	if (!IS_NPC(ch) && ch->mana < mana)
		  {
					 Cprintf(ch, "You don't have enough mana.\n\r");
					 return;
		  }

	/* Find our victim */
	for(victim = char_list; victim != NULL; victim = victim->next)
	{
		if(IS_NPC(victim) && victim->pIndexData->vnum == obj->owner_vnum)
			break;
		if(!IS_NPC(victim) && !str_cmp(victim->name, obj->owner)
			&& obj->owner_vnum == 0)
			break;
	}

	if(victim == NULL)
	{
		Cprintf(ch, "Your doll seems to sigh, nothing happens.\n\r");
		return;
	}

	if(victim->in_room->area->continent != ch->in_room->area->continent) {
		Cprintf(ch, "Your victim is out of reach.\n\r");
		return;
	}

        if(victim->desc)
        {
                if(victim->desc->connected >= CON_NOTE_TO && victim->desc->connected <= CON_NOTE_FINISH)
                {
                        Cprintf(ch, "Your victim is writing a note, give them some peace!\n\r");
                        return;
                }
        }

	old_fighting = victim->fighting;

	if((skill_table[sn].target == TAR_CHAR_OFFENSIVE)
		|| (skill_table[sn].target == TAR_OBJ_CHAR_OFF))
	{
		if (!IS_NPC(ch))
		{
			if (is_safe(ch, victim) && victim != ch)
			{
				Cprintf(ch, "Not on that target.\n\r");
				return;
			}
			check_killer(ch, victim);
		}
		vo = (void *) victim;
		target = TARGET_CHAR;
	}
	else if((skill_table[sn].target == TAR_CHAR_DEFENSIVE)
		|| (skill_table[sn].target == TAR_OBJ_CHAR_DEF))
	{
		vo = (void *) victim;
		target = TARGET_CHAR;
	}
	else
	{
		Cprintf(ch, "You can't channel this spell through a voodoo doll.\n\r");
		return;
	}

	WAIT_STATE(ch, skill_table[sn].beats);

	if (number_percent() > get_skill(ch, sn))
	{
		Cprintf(ch, "You lost your concentration.\n\r");
		check_improve(ch, sn, FALSE, 1);
		ch->mana -= mana / 2;
	}
	else
	{
		ch->mana -= mana;

		spell_level = generate_int(ch->level, ch->level);

		if(victim != NULL)
		{
			Cprintf(ch, "Your doll glimmers and transfers the effect.\n\r");
			Cprintf(victim, "You double over in agony as you are bewitched by voodoo magic!\n\r");
			act("$n is tormented by voodoo magic.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			(*skill_table[sn].spell_fun) (sn, spell_level, ch, vo, target);
		}

		check_improve(ch, sn, TRUE, 1);
		obj->value[0]--;
		if(obj->value[0] < 1)
		{
			Cprintf(ch, "The doll crumbles into dust.\n\r");
			extract_obj(obj);
		}
		if (victim != NULL
		&& victim != ch
        	&& victim->master != ch)
        	{
                	// They died, don't start a battle
                	if(victim->pktimer > 0)
                	        return;

                	check_killer(ch, victim);

                	if(!IS_NPC(ch)
                	&& !IS_NPC(victim)) {
                	        ch->no_quit_timer = 3;
                	        victim->no_quit_timer = 3;
                	}
		}
		if(old_fighting == NULL)
			stop_fighting(victim, FALSE);
	}
}

int
trap_symbol(CHAR_DATA *victim, OBJ_DATA *obj)
{
	AFFECT_DATA *paf;
	int spell_level = 0;

	if ((paf = affect_find(obj->affected, gsn_symbol)) == NULL || !str_cmp(obj->owner, victim->name))
	{
		return FALSE;
	}

	if (victim->level < paf->level - 8)
	{
		Cprintf(victim, "You notice an odd rune but nothing happens...\n\r");
		return FALSE;
	}

	if(number_percent() > 85)
	{
		Cprintf(victim, "The symbol glows and fades.\n\r");
		affect_remove_obj(obj, paf);
		return FALSE;
	}

	Cprintf(victim, "You are struck down by a warding symbol!\n\r");

	/* fire trap */
	if (paf->modifier == 1)
	{
		spell_level = generate_int(paf->level, paf->level);
		spell_pyrotechnics(gsn_pyrotechnics, spell_level, victim, (void*)victim, TARGET_CHAR);
	}

	if (paf->modifier == 2)
	{
		spell_level = generate_int(paf->level + 2, paf->level + 2);
		spell_curse(gsn_curse, spell_level, victim, (void*)victim, TARGET_CHAR);
	}

	if (paf->modifier == 3)
	{
		spell_level = generate_int(paf->level + 2, paf->level + 2);
		spell_weaken(gsn_weaken, spell_level, victim, (void*)victim, TARGET_CHAR);
	}

	if (paf->modifier == 4)
	{
		spell_level = generate_int(paf->level, paf->level);
		spell_blast_of_rot(gsn_blast_of_rot, spell_level, victim, (void*)victim, TARGET_CHAR);
		spell_poison(gsn_poison, spell_level, victim, (void*)victim, TARGET_CHAR);
	}

	affect_remove_obj(obj, paf);

	return TRUE;
}

const struct lesser_tattoo_mod lesser_tattoo_table[] =
{
        { APPLY_WIS,	 1,  "fairy",    "wise"     },
        { APPLY_INT,     1,  "wizard",   "smart"    },
        { APPLY_CON,     1,  "stallion", "tough"    },
        { APPLY_DEX,     1,  "fox",      "agile"    },
        { APPLY_STR,     1,  "bear",     "muscular" },
        { APPLY_DAMROLL, 1,  "blade",    "powerful" },
        { APPLY_HITROLL, 1,  "eyeball",  "focused"  },
        { APPLY_HIT,     10, "sun",      "bloody"   },
        { APPLY_MANA,    10, "moon",     "glowing"  },
        { APPLY_SAVES,   -1, "shield",   "guardian" },
        { APPLY_AGE,     10, "turtle",   "aging"    },
};

const struct greater_tattoo_mod greater_tattoo_table[] =
{
        { APPLY_WIS,     1, 4,   "unicorn",   "sage"      },
        { APPLY_INT,     1, 4,   "sphinx",    "brilliant" },
        { APPLY_CON,     1, 4,   "gorgon",    "resilient" },
        { APPLY_DEX,     1, 4,   "wyvern",    "flying"    },
        { APPLY_STR,     1, 4,   "titan",     "red"       },
        { APPLY_DAMROLL, 1, 4,   "warlord",   "flaming"   },
        { APPLY_HITROLL, 1, 4,   "archer",    "precise"   },
        { APPLY_HIT,     10, 40, "gryphon",   "huge"      },
        { APPLY_MANA,    10, 40, "dragon",    "pulsating" },
        { APPLY_SAVES,   1, 5, "pentagram", "sealed"    },
        { APPLY_AGE,     10, 30, "hourglass", "ancient"   },
};

const struct power_tattoo_mod power_tattoo_table[] =
{
        {TATTOO_ANIMATES,        "golem"     },
        {TATTOO_MV_SPELLS,       "crystal"   },
        {TATTOO_HP_REGEN,        "cross"     },
        {TATTOO_MANA_REGEN,      "rose"      },
        {TATTOO_HP_PER_KILL,     "skull"     },
        {TATTOO_MANA_PER_KILL,   "vampire"   },
        {TATTOO_EXTRA_DP,        "temple"    },
        {TATTOO_PHYSICAL_RESIST, "barrier"   },
        {TATTOO_MAGICAL_RESIST,  "tower"     },
        {TATTOO_LEARN_SPELL,     "spellbook" },
        {TATTOO_RAISES_CASTING,  "staff"     },
};

void do_paint_tattoo(CHAR_DATA *ch, char *argument) {
	int class = 0;
	int chance = 0;
	int sn, i;
	int wear_dest = -1;
	OBJ_DATA *tattoo;

	if(!str_prefix(argument, "lesser")) {
		if((chance = get_skill(ch, gsn_paint_lesser)) < 1) {
			Cprintf(ch, "You don't know how to paint lesser tattoos.\n\r");
			return;
		}
		sn = gsn_paint_lesser;
		class = LESSER_TATTOO;
	}
	else if(!str_prefix(argument, "greater")) {
		if((chance = get_skill(ch, gsn_paint_greater)) < 1) {
			Cprintf(ch, "You don't know how to paint greater tattoos.\n\r");
			return;
		}
		sn = gsn_paint_greater;
		class = GREATER_TATTOO;
	}
	else if(!str_prefix(argument, "power")) {
		if((chance = get_skill(ch, gsn_paint_power)) < 1) {
			Cprintf(ch, "You don't know how to paint power tattoos.\n\r");
			return;
		}
		sn = gsn_paint_power;
		class = POWER_TATTOO;
	}
	else {
		Cprintf(ch, "Which do you want? Lesser, greater or power?\n\r");
		return;
	}

	if(get_eq_char(ch, WEAR_BODY) == NULL)
		wear_dest = ITEM_WEAR_BODY;
	else if(get_eq_char(ch, WEAR_HEAD) == NULL)
		wear_dest = ITEM_WEAR_HEAD;
	else if(get_eq_char(ch, WEAR_ARMS) == NULL)
		wear_dest = ITEM_WEAR_ARMS;
	else if(get_eq_char(ch, WEAR_WRIST_L) == NULL)
		wear_dest = ITEM_WEAR_WRIST;
	else if(get_eq_char(ch, WEAR_WRIST_R) == NULL)
		wear_dest = ITEM_WEAR_WRIST;
	else if(ch->race == race_lookup("troll")
	&& ch->remort && ch->rem_sub == 1
	&& get_eq_char(ch, WEAR_HEAD_2) == NULL)
		wear_dest = ITEM_WEAR_HEAD;
	else {
		Cprintf(ch, "You have no exposed skin to wear any tattoos right now. Try removing something.\n\r");
		return;
	}

	if(ch->mana < 50) {
		Cprintf(ch, "You don't have enough energy to paint right now.\n\r");
		return;
	}

	if(number_percent() > chance) {
		Cprintf(ch, "You make a nice design, but you fail to empower it.\n\r");
		ch->mana -= 25;
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
		check_improve(ch, sn, FALSE, 3);
		return;
	}
	check_improve(ch, sn, TRUE, 3);
	ch->mana -= 50;

	// Make a blank tattoo.
	tattoo = create_object(get_obj_index(OBJ_VNUM_NEW_TATTOO), 0);
	tattoo->level = ch->level;
	for(i=0;i<4;i++)
		tattoo->value[i] = (dice(ch->level, 4) / 12);
	tattoo->timer = (9 * ch->level) + (number_range(1, ch->level * 3));
	tattoo->wear_flags = ITEM_TAKE | wear_dest;

	if(wear_dest == ITEM_WEAR_HEAD)
		Cprintf(ch, "You carefully paint a magical tattoo on your forehead.\n\r");
	if(wear_dest == ITEM_WEAR_BODY)
		Cprintf(ch, "You carefully paint a magical tattoo on your chest.\n\r");
	if(wear_dest == ITEM_WEAR_ARMS)
		Cprintf(ch, "You carefully paint a magical tattoo on your arm.\n\r");
	if(wear_dest == ITEM_WEAR_WRIST)
		Cprintf(ch, "You carefully paint a magical tattoo on your forearm.\n\r");

	// This function will call itself recursively until the tattoo is ready.
	paint_tattoo(ch, tattoo, class, TRUE);
	finalize_tattoo(tattoo);
	obj_to_char(tattoo, ch);
	Cprintf(ch, "You have painted the following tattoo:\n\r");
	act("$n paints a tattoo on themselves.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	lore_obj(ch, tattoo);
	wear_obj(ch, tattoo, FALSE);
	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	return;
}

void paint_tattoo(CHAR_DATA *ch, OBJ_DATA *tattoo, int class, int recursive) {
        AFFECT_DATA af;
        int mod=0, trials = 0;
	int sn=-1;
	// Ok, we need to determine the number of modifiers on this tattoo.
	// Level Lesser			Greater		Power
	// 15    100			-		-
	// 20	 100/10			-		-
	// 25	 100/25			-		-
	// 30	 100/50/10		100		-
	// 35    100/100/25		100/25		10
	// 40    100/100/50/10		100/50/10	25
	// 45    100/100/100/25		100/100/25	50/10
	// 50    100/100/100/50		100/100/50	75/50

	// A really rare but nice bonus.
	if(!recursive && number_percent() < 5) {
		recursive = TRUE;
		Cprintf(ch, "{YThe tattoo sparkles brilliantly!{x\n\r");
		act("$n's tattoo sparkles brilliantly!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	// Kirre make nicer runists.
	if(ch->race == race_lookup("kirre") && recursive)
		paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);

	if(class == LESSER_TATTOO && recursive) {
		if(ch->level < 20) {
			;
		}
		else if(ch->level < 25 && number_percent() < 10) {
			paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
		}
		else if(ch->level < 30 && number_percent() < 25) {
			paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
		}
		else if(ch->level < 35) {
			if(number_percent() < 50)
				 paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
			if(number_percent() < 10)
				 paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
		}
		else if(ch->level < 40) {
			paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
			if(number_percent() < 25)
				paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
		}
		else if(ch->level < 45) {
			paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
			if(number_percent() < 50)
				paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
			if(number_percent() < 10)
				paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
		}
		else if(ch->level < 50) {
			paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
			paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
			if(number_percent() < 25)
				paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
		}
		else {
			paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
			paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
			if(number_percent() < 50)
				paint_tattoo(ch, tattoo, LESSER_TATTOO, FALSE);
		}
	}

	if(class == GREATER_TATTOO && recursive) {
		if(ch->level < 40 && number_percent() < 25) {
			paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
		}
		else if(ch->level < 45) {
			if(number_percent() < 50)
				paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
			if(number_percent() < 10)
				paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
		}
		else if(ch->level < 50) {
			paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
			if(number_percent() < 25)
				paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
		}
		else {
			paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
			if(number_percent() < 50)
				paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
		}
	}

	if(class == POWER_TATTOO && recursive) {
		if(ch->level < 40 && number_percent() < 10) {
			paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
		}
		else if(ch->level < 45 && number_percent() < 25) {
			paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
		}
		else if(ch->level < 50) {
			if(number_percent() < 50)
				paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
			if(number_percent() < 10)
				paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
		}
		else {
			if(number_percent() < 75)
				paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
			if(number_percent() < 50)
				paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
		}
	}

        mod = number_range(0, TATTOO_TYPES - 2); //age not allowed

        if(class == LESSER_TATTOO)
        af.type = gsn_paint_lesser;
        if(class == GREATER_TATTOO)
        af.type = gsn_paint_greater;
        if(class == POWER_TATTOO)
        af.type = gsn_paint_power;

        af.where = TO_AFFECTS;
        af.level = ch->level;
        af.duration = -1;
	af.extra  = 0;
        if(class == LESSER_TATTOO) {
                af.location = lesser_tattoo_table[mod].location;
                af.modifier = lesser_tattoo_table[mod].modifier;
        }
	if(class == GREATER_TATTOO) {
                af.location = lesser_tattoo_table[mod].location;
                af.modifier = number_range(
                        greater_tattoo_table[mod].min_mod,
                        greater_tattoo_table[mod].max_mod);
		if(af.location == APPLY_SAVES) {
			af.modifier = 0 - af.modifier;
		}
        }
        if(class == POWER_TATTOO) {
                af.location = APPLY_NONE;
                af.modifier = power_tattoo_table[mod].special;

		if(af.modifier == TATTOO_LEARN_SPELL) {
			// ok, we'll give 5 chances before we decide there's a problem.
			int found = FALSE;
			for(trials = 0; trials < 5; trials++) {
		                sn = number_range(gsn_reserved, gsn_axe);
				while(1) {
                                	if(sn != gsn_frost_breath
					&& sn != gsn_lightning_breath
					&& sn != gsn_fire_breath
					&& sn != gsn_gas_breath
					&& sn != gsn_acid_breath
					&& !skill_table[sn].remort
                                	&& skill_table[sn].spell_fun != spell_null
                                	&& ch->pcdata->learned[sn] == 0) {
						found = TRUE;
                                        	break;
					}
					sn++;
					if(sn >= gsn_last_skill)
						break;
				}
				if(found) {
					af.extra = sn;
					break;
				}
                        }
			if(!found) {
				Cprintf(ch, "You fail to inscribe a spell into the tattoo.\n\r");
				paint_tattoo(ch, tattoo, GREATER_TATTOO, FALSE);
				return;
			}
		}
        }
        af.bitvector = 0;
        affect_to_obj(tattoo, &af);

        return;
}

int tattoo_lookup(AFFECT_DATA *paf) {
	int i=0;

	for(i=0 ; i<TATTOO_TYPES; i++) {
		if(paf->type == gsn_paint_lesser
		&& paf->location == lesser_tattoo_table[i].location)
			return i;

		if(paf->type == gsn_paint_greater
		&& paf->location == greater_tattoo_table[i].location)
			return i;

		if(paf->type == gsn_paint_power
		&& paf->modifier == power_tattoo_table[i].special)
			return i;
	}

	return 0;
}

// This function first combies all similar bonuses
// and then uses them to name the tattoo.
void finalize_tattoo(OBJ_DATA *tattoo) {
    AFFECT_DATA *paf_old, *paf_second;
    AFFECT_DATA af, *paf;
    char b_name[MAX_INPUT_LENGTH];
    char b_short[MAX_INPUT_LENGTH];
    char *noun = NULL;
    char *adj1 = NULL;
    char *adj2 = NULL;

    for (paf_old = tattoo->affected; paf_old != NULL; paf_old = paf_old->next)
    {
	for (paf_second = tattoo->affected; paf_second != NULL; paf_second = paf_second->next)
    	{
		if (paf_old->location == paf_second->location
		&& paf_old != paf_second) {
			af.where = TO_AFFECTS;
        		af.type = UMAX(paf_old->type, paf_second->type);
        		af.level = (paf_old->level + paf_second->level) / 2;
        		af.duration = -1;
        		af.modifier = paf_old->modifier + paf_second->modifier;
        		af.bitvector = 0;
        		af.location = paf_old->location;

 			affect_remove_obj(tattoo, paf_old);
	            	affect_remove_obj(tattoo, paf_second);
           		affect_to_obj(tattoo, &af);
			paf_old = tattoo->affected;
                        paf_second = tattoo->affected;
			break;
        	}
	}
    }


	for( paf = tattoo->affected; paf != NULL; paf = paf->next) {
		// tattoo of a <adj> and <adj> <noun>
		// if more than one adjective just use "prismatic"
		if(noun == NULL) {
			if(paf->type == gsn_paint_lesser)
				noun = lesser_tattoo_table[tattoo_lookup(paf)].noun;
			if(paf->type == gsn_paint_greater)
				noun = greater_tattoo_table[tattoo_lookup(paf)].noun;
			if(paf->type == gsn_paint_power)
				noun = power_tattoo_table[tattoo_lookup(paf)].noun;
			continue;
		}

		if(adj1 == NULL) {
			if(paf->type == gsn_paint_lesser)
				adj1 = lesser_tattoo_table[tattoo_lookup(paf)].adjective;
			if(paf->type == gsn_paint_greater)
				adj1 = greater_tattoo_table[tattoo_lookup(paf)].adjective;
			continue;
		}

		if(adj2 == NULL) {
			if(paf->type == gsn_paint_lesser)
				adj2 = lesser_tattoo_table[tattoo_lookup(paf)].adjective;
			if(paf->type == gsn_paint_greater)
				adj2 = greater_tattoo_table[tattoo_lookup(paf)].adjective;
			continue;
		}

		// More than 3 affects, go crazy.
		adj1 = str_dup("prismatic");
		adj2 = NULL;
		break;
	}

	sprintf(b_name, "%s %s%s%s%s%s", tattoo->name,
		noun == NULL ? "" : noun,
		adj1 == NULL ? "" : " ",
		adj1 == NULL ? "" : adj1,
		adj2 == NULL ? "" : " ",
		adj2 == NULL ? "" : adj2);

	sprintf(b_short, "A magical tattoo of a %s%s%s%s%s",
		adj1 == NULL ? "" : adj1,
		adj2 == NULL ? "" : ", ",
		adj2 == NULL ? "" : adj2,
		(adj1 == NULL && adj2 == NULL) ? "" : " ",
		noun == NULL ? "" : noun);

	free_string(tattoo->name);
	tattoo->name = str_dup(b_name);
	free_string(tattoo->short_descr);
	tattoo->short_descr = str_dup(b_short);

    return;
}

void
do_animate_tattoo(CHAR_DATA *ch, char *argument)
{
        int tattoos = 0, i = 0;
        CHAR_DATA *mob, *victim;
        AFFECT_DATA af;
	char buf[MAX_INPUT_LENGTH];

	if(ch->fighting != NULL)
	{
	    Cprintf(ch, "You lack the concentration to animate.\n\r");
	    return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| wearing_nogate_item(ch))
    	{
            Cprintf(ch, "Not in this room.\n\r");
            return;
    	}

	// You can only animate them once!
	for (victim = char_list; victim != NULL; victim = victim->next) {
            if (victim->master == ch && IS_NPC(victim)
            && is_affected(victim, gsn_paint_power)) {
		Cprintf(ch, "Your tattoos are already animated.\n\r");
             	return;
            }
        }

        // Count the number of animate tattoos
	tattoos = power_tattoo_count(ch, TATTOO_ANIMATES);
	if(tattoos == 0) {
		Cprintf(ch, "You don't have any tattoos that can be animated.\n\r");
		return;
	}

	if(ch->mana < 30) {
		Cprintf(ch, "Your will is not strong enough right now.\n\r");
		return;
	}
	ch->mana -= 30;

	for(i = 0; i < tattoos; i++) {
 		mob = create_mobile(get_mob_index(MOB_VNUM_FIGURINE));
        	size_mob(ch, mob, ch->level);
		SET_BIT(mob->toggles, TOGGLES_NOEXP);
        	mob->hit = MAX_HP(mob);
        	mob->hitroll = ch->level / 10 + 1;
        	mob->damroll = ch->level / 4 + 5;

        	Cprintf(ch, "You touch your magical tattoo and it springs into reality!\n\r");
        	act("$n's tattoo springs into reality!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

        	/* get funky with strings! */
        	free_string(mob->name);
        	free_string(mob->short_descr);
        	free_string(mob->long_descr);

		sprintf(buf, "animated tattoo golem");
        	mob->name = str_dup(buf);

        	sprintf(buf, "A heavily tattooed golem");
        	mob->short_descr = str_dup(buf);

 		strcpy(buf, "A heavily tattooed golem glares at you.\n\r");
        	mob->long_descr = str_dup(buf);

        	char_to_room(mob, ch->in_room);
        	add_follower(mob, ch);
        	mob->leader = ch;

        	af.where = TO_AFFECTS;
    		af.type = gsn_paint_power;
       		af.level = ch->level;
	        af.duration = ch->level / 6;
        	af.location = 0;
        	af.modifier = 0;
       		af.bitvector = AFF_CHARM;
	        affect_to_char(mob, &af);

	}

	return;
}

void add_crafted_bonus(CHAR_DATA *ch, OBJ_DATA *obj)
{
	int dam = 1;
        int new_flag;
	AFFECT_DATA *paf;

	// Add a special modifier location
	if(number_percent() < 50) {
		dam = dice(1, 7);
		paf = new_affect();
                paf->where = TO_OBJECT;
                paf->type = gsn_enchant_weapon;
                paf->level = obj->level;
                paf->duration = -1;
                paf->bitvector = 0;
           	paf->modifier = 2;
		switch(dice(1, 8)) {
			case 1: paf->location = APPLY_MAX_STR; break;
			case 2: paf->location = APPLY_MAX_DEX; break;
			case 3: paf->location = APPLY_MAX_CON; break;
			case 4: paf->location = APPLY_MAX_INT; break;
			case 5: paf->location = APPLY_MAX_WIS; break;
			case 6:
				paf->location = APPLY_SPELL_DAMAGE;
				paf->modifier = 7; break;
			case 7: paf->location = APPLY_DAMAGE_REDUCE;
				paf->modifier = 5; break;
			case 8:
			        paf->location = APPLY_ATTACK_SPEED;
				paf->modifier = 10; break;
		}
		affect_to_obj(obj, paf);
		return;
	}
	else {
		while(1) {
                	dam = dice(1, 6);
                	if (dam > 6 || dam < 1)
                        	dam = 1;

                	if (dam == 1)
	                        new_flag = WEAPON_HOLY;
                	if (dam == 2)
	                        new_flag = WEAPON_UNHOLY;
                	if (dam == 3)
	                        new_flag = WEAPON_POLAR;
                	if (dam == 4)
	                        new_flag = WEAPON_DEMONIC;
                	if (dam == 5)
	                        new_flag = WEAPON_PSIONIC;
                	if (dam == 6)
	                        new_flag = WEAPON_PHASE;

			if(!IS_WEAPON_STAT(obj, new_flag)) {
                        	obj->value[4] = obj->value[4] | new_flag;
	                        return;
        	        }
		}
	}
}

void add_random_element(CHAR_DATA *ch, OBJ_DATA *obj)
{
	int dam = 1;
	int new_flag;

       	dam = dice(1, 6);
        if (dam > 6 || dam < 1)
              	dam = 1;

	if (dam == 1)
               	new_flag = hit_lookup("flame");
        else if (dam == 2)
               	new_flag = hit_lookup("chill");
        else if (dam == 3)
		new_flag = hit_lookup("slime");
	else if (dam == 4)
		new_flag = hit_lookup("shock");
	else if (dam == 5)
		new_flag = hit_lookup("pobite");
	else
		new_flag = hit_lookup("smother");

	obj->value[3] = new_flag;

	return;
}


void add_random_wflag(CHAR_DATA *ch, OBJ_DATA *obj)
{
	int dam = 1;
	int new_flag;

	while(1) {
        	dam = dice(1, 13);
                if (dam > 13 || dam < 1)
                	dam = 1;

		new_flag = -1;

		if (dam == 1)
                	new_flag = WEAPON_FLAMING;
        	else if (dam == 2)
                	new_flag = WEAPON_FROST;
        	else if (dam == 3)
			new_flag = WEAPON_VAMPIRIC;
		else if (dam == 4)
			new_flag = WEAPON_SHOCKING;
		else if (dam == 5)
			new_flag = WEAPON_CORROSIVE;
        	else if (dam == 6)
                	new_flag = WEAPON_FLOODING;
        	else if (dam == 7 && number_percent() < 50)
                	new_flag = WEAPON_POISON;
        	else if (dam == 8 && number_percent() < 50)
                	new_flag = WEAPON_INFECTED;
        	else if (dam == 9)
			new_flag = WEAPON_SOULDRAIN;
		else if (dam == 10)
			new_flag = WEAPON_VORPAL;
		else if (dam == 11 && number_percent() < 50)
			new_flag = WEAPON_TWO_HANDS;
		else if (dam == 12)
			new_flag = WEAPON_SHARP;
		else
			new_flag = WEAPON_DRAGON_SLAYER;

		if(new_flag == -1)
			continue;

		if(!IS_WEAPON_STAT(obj, new_flag)) {
			obj->value[4] = obj->value[4] | new_flag;
			break;
		}
	}

	return;
}


void
add_random_bonus(CHAR_DATA *ch, OBJ_DATA *obj, int add_bonus) {
	AFFECT_DATA *paf;
	int bonus_done = FALSE;
	int apply_location = APPLY_NONE;

	// Pick which random bonus.
	switch(number_range(1, 8)) {
		case 1: apply_location = APPLY_STR; break;
		case 2: apply_location = APPLY_DEX; break;
		case 3: apply_location = APPLY_CON; break;
		case 4: apply_location = APPLY_INT; break;
		case 5: apply_location = APPLY_WIS; break;
		case 6: apply_location = APPLY_HIT; add_bonus *= 20; break;
		case 7: apply_location = APPLY_MANA; add_bonus *= 20; break;
		case 8: apply_location = APPLY_SAVES; add_bonus = 0 - (2 * add_bonus); break;
		default: apply_location = APPLY_STR;
	}

	// Enchant the weapon, move affects to this instance so we can modify.
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

	for (paf = obj->affected; paf != NULL; paf = paf->next) {
		if (paf->location == apply_location) {
			bonus_done = TRUE;
			paf->type = gsn_enchant_weapon;
                	paf->modifier += add_bonus;
			break;
		}
	}

	if(!bonus_done && add_bonus != 0) {
		paf = new_affect();
                paf->where = TO_OBJECT;
                paf->type = gsn_enchant_weapon;
                paf->level = obj->level;
                paf->duration = -1;
                paf->location = apply_location;
                paf->modifier = add_bonus;
                paf->bitvector = 0;
                paf->next = obj->affected;
                obj->affected = paf;
	}

	return;
}



void add_hit_dam(CHAR_DATA *ch, OBJ_DATA *obj, int add_hit, int add_dam) {
	AFFECT_DATA *paf;
	int hit_done = FALSE;
	int dam_done = FALSE;

	// Enchant the weapon, move affects to this instance so we can modify.
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

	for (paf = obj->affected; paf != NULL; paf = paf->next) {
		if (paf->type != gsn_sharpen
		&& paf->type != gsn_blade_rune
		&& paf->type != gsn_magic_sheath
		&& paf->location == APPLY_HITROLL) {
			hit_done = TRUE;
			paf->type = gsn_enchant_weapon;
                	paf->modifier += add_hit;
		}

		if (paf->type != gsn_sharpen
		&& paf->type != gsn_blade_rune
		&& paf->type != gsn_magic_sheath
		&& paf->location == APPLY_DAMROLL) {
			dam_done = TRUE;
			paf->type = gsn_enchant_weapon;
                	paf->modifier += add_dam;
		}

	}

	if(!hit_done) {
		paf = new_affect();
                paf->where = TO_OBJECT;
                paf->type = gsn_enchant_weapon;
                paf->level = obj->level;
                paf->duration = -1;
                paf->location = APPLY_HITROLL;
                paf->modifier = add_hit;
                paf->bitvector = 0;
                paf->next = obj->affected;
                obj->affected = paf;
	}

	if(!dam_done) {
		paf = new_affect();
                paf->where = TO_OBJECT;
                paf->type = gsn_enchant_weapon;
                paf->level = obj->level;
                paf->duration = -1;
                paf->location = APPLY_DAMROLL;
                paf->modifier = add_dam;
                paf->bitvector = 0;
                paf->next = obj->affected;
                obj->affected = paf;
	}

	return;
}

void
update_version_weapon(CHAR_DATA* ch, OBJ_DATA* obj)
{
    int xp_extra;

    //Only update weapons
    if (obj->item_type != ITEM_WEAPON)
        return;

    //only update intelligent ones
    if (!IS_WEAPON_STAT(obj, WEAPON_INTELLIGENT))
        return;

    //only updates ones above 10k XP
    if (obj->extra[1] < 10000)
        return;

    //only update those that haven't been updated yet (stripped noun)
    if (obj->value[3] == obj->pIndexData->value[3])
        return;

    if (attack_table[obj->value[3]].damage == DAM_BASH ||
        attack_table[obj->value[3]].damage == DAM_SLASH ||
        attack_table[obj->value[3]].damage == DAM_PIERCE)
	return;

    xp_extra = UMIN(5000, obj->extra[1] - 10000);
    obj->extra[1] -= xp_extra;
    obj->extra[0] = 8;
    ch->pass_along += 10 * xp_extra;

    //strip off the damage noun
    obj->value[3] = obj->pIndexData->value[3];
    Cprintf(ch, "%s has been adjusted.\n\r", capitalize(obj->short_descr));

    //if we already have all bonuses, we're done
    if (xp_extra >= 5000)
        return;

    add_hit_dam(ch, obj, 1, 1);
    add_crafted_bonus(ch, obj);

    //we already had the level 9 bonus
    if (xp_extra >= 2000)
        return;

    add_hit_dam(ch, obj, 0, 1);
    add_random_bonus(ch, obj, 1);

}

// For increasing intelligent weapons
/*
Item levels as follows:
Exp:		Level:	Hit:	Dam:	Bonus:
0		1	0	0	none
500		2	+1	+1	none
1000		3	+1	+1	+2 random stat
2000		4	+2	+2	+random normal flag
4000		5	+2	+2	+1 random stat
6000		6	+3	+3	+random normal flag
8000		7	+3	+4	+1 random stat
10000		8	+4	+5	become elemental
12000		9	+4	+6	+1 random stat
15000		10	+5	+7	+random crafted flag
*/
void
advance_weapon(CHAR_DATA *ch, OBJ_DATA *obj, int xp)
{
	int level = obj->extra[0];
	int xptotal;
	int wear_to = WEAR_NONE;

	// Just check xp cap.
	if(obj->extra[1] > 15000)
		return;

	// If the item is currently worn, refresh its affects.
        wear_to = obj->wear_loc;
        if(wear_to != WEAR_NONE)
                unequip_char(ch, obj);

	obj->extra[1] += xp;
	xptotal = obj->extra[1];

	if(xptotal >= 500 && level < 2) {
		obj->extra[0] = 2;
		level = 2;
		Cprintf(ch, "%s has advanced to level %d!\n\r", capitalize(obj->short_descr), level);
		log_string("%s carried by %s has reached level %d", obj->short_descr, obj->carried_by->name, level);
		ch->pass_along_limit = 10000;
		add_hit_dam(ch, obj, 1, 1);
	}

	if(xptotal >= 1000 && level < 3) {
		obj->extra[0] = 3;
		level = 3;
		Cprintf(ch, "%s has advanced to level %d!\n\r", capitalize(obj->short_descr), level);
		log_string("%s carried by %s has reached level %d", obj->short_descr, obj->carried_by->name, level);
		ch->pass_along_limit = 10000;
		add_random_bonus(ch, obj, 2);
	}

	if(xptotal >= 2000 && level < 4) {
		obj->extra[0] = 4;
		level = 4;
		Cprintf(ch, "%s has advanced to level %d!\n\r", capitalize(obj->short_descr), level);

		log_string("%s carried by %s has reached level %d", obj->short_descr, obj->carried_by->name, level);
		ch->pass_along_limit = 10000;
		add_hit_dam(ch, obj, 1, 1);
		add_random_wflag(ch, obj);
	}

	if(xptotal >= 4000 && level < 5) {
		obj->extra[0] = 5;
		level = 5;
		Cprintf(ch, "%s has advanced to level %d!\n\r", capitalize(obj->short_descr), level);

		log_string("%s carried by %s has reached level %d", obj->short_descr, obj->carried_by->name, level);
		ch->pass_along_limit = 10000;
		add_random_bonus(ch, obj, 1);
	}

	if(xptotal >= 6000 && level < 6) {
		obj->extra[0] = 6;
		level = 6;
		Cprintf(ch, "%s has advanced to level %d!\n\r", capitalize(obj->short_descr), level);
		log_string("%s carried by %s has reached level %d", obj->short_descr, obj->carried_by->name, level);
		ch->pass_along_limit = 10000;
		add_hit_dam(ch, obj, 1, 1);
		add_random_wflag(ch, obj);
	}

	if(xptotal >= 8000 && level < 7) {
		obj->extra[0] = 7;
		level = 7;
		Cprintf(ch, "%s has advanced to level %d!\n\r", capitalize(obj->short_descr), level);
		log_string("%s carried by %s has reached level %d", obj->short_descr, obj->carried_by->name, level);
		ch->pass_along_limit = 10000;
		add_hit_dam(ch, obj, 0, 1);
		add_random_bonus(ch, obj, 1);
	}

	if(xptotal >= 10000 && level < 8) {
		obj->extra[0] = 8;
		level = 8;
		Cprintf(ch, "%s has advanced to level %d!\n\r", capitalize(obj->short_descr), level);
		log_string("%s carried by %s has reached level %d", obj->short_descr, obj->carried_by->name, level);
		ch->pass_along_limit = 10000;
		add_hit_dam(ch, obj, 2, 3);
		add_crafted_bonus(ch, obj);
		add_random_bonus(ch, obj, 1);
	}

	if (wear_to != WEAR_NONE)
                equip_char(ch, obj, wear_to);
}

struct clanspell_type {
	char *name;
//	{"amnesty", 7000, 0, CLAN_JUSTICE},		// single use
	int cost;
	int duration;
	int clan;
};

const struct clanspell_type clanspell_table[] =
{
	// Venari
//	{"homeland", 3000, 10, CLAN_VENARI},  		// 300 per tick
	{"dragonbane", 5000, 10, CLAN_VENARI}, 	// 333 per tick
//	{"trackless step", 7000, 20, CLAN_VENARI}, // 350 per tick
	// Kenshi
	{"brotherhood", 3000, 60, CLAN_KENSHI},	// 50 per tick
//	{"dragonblood", 5000, 15, CLAN_KENSHI},	// 333 per tick
//	{"martyr", 7000, 30, CLAN_KENSHI},	// 230 per tick
	// Kindred
//	{"hatred", 3000, 10, CLAN_KINDRED},		// 300 per tick
	{"paradox", 1000, 0, CLAN_KINDRED},		// single use
//	{"chaos belief", 7000, 0, CLAN_KINDRED},	// single use
	// Seeker
//	{"spiritual call", 3000, 0, CLAN_SEEKERS},	// single use
//	{"wise scholar", 5000, 12, CLAN_SEEKERS},	// 420 per tick
	{"harmonic aura", 3000, 20, CLAN_SEEKERS}, // 466 per tick
	// Loner/Nonclanner
	{"homeland", 3000, 30, CLAN_NONE},  		// 300 per tick
	{"homeland", 3000, 30, CLAN_LONER},
	{"homeland", 3000, 30, CLAN_OUTCAST},
	{"homeland", 3000, 30, GUILD_HELP},
	{"homeland", 3000, 30, GUILD_TR},
//	{"henchman", 5000, 0, 0},			// single use
//	{"rogue trader", 7000, 30, 0},			// 230 per tick
	{NULL, 0, 0, 0},
};

void do_pray(CHAR_DATA *ch, char *argument)
{
	char arg1[255]; // spell name
	char arg2[255]; // [optional] target name
	int i = 0;
	int sn = 0;
	int index = -1;
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if(ch->fighting) {
		Cprintf(ch, "You can't concentrate enough to summon your clan's aid.\n\r");
		return;
	}

	if(arg1[0] == '\0') {
		Cprintf(ch, "You have %d deity points.\n\r", ch->deity_points);
		Cprintf(ch, "Your choices for prayer are:\n\r");
		i = 0;
		while(1) {
			if(clanspell_table[i].name == NULL) {
				Cprintf(ch, "\n\r");
				break;
			}
			if(clanspell_table[i].clan == ch->clan)
				Cprintf(ch, "%-20sfor %d dp\n\r",
					clanspell_table[i].name,
					clanspell_table[i].cost);

			i++;
		}
		return;
	}

	i = 0;
	while(1) {
		if(clanspell_table[i].name == NULL) {
			Cprintf(ch, "You cannot call upon this favour.\n\r");
			return;
		}
		if(clanspell_table[i].clan == ch->clan
		&& !str_prefix(arg1, clanspell_table[i].name)) {
			index = i;
			break;
		}

		i++;
	}

	sn = skill_lookup(clanspell_table[index].name);

	if(ch->deity_points < clanspell_table[index].cost) {
		Cprintf(ch, "Your clan cannot grant you aid without more deity points.\n\r");
		return;
	}

	if (is_affected(ch, sn))
	{
        	Cprintf(ch, "You're already affected by that clan spell.\n\r");
        	return;
	}

	// Normal duration based clan spell, effects handled elsewhere.
	if(clanspell_table[index].duration > 0) {
        	if(ch->pktimer) {
                	Cprintf(ch, "You're too nervous to summon your clan's aid right now.\n\r");
                	return;
        	}
		ch->deity_points -= clanspell_table[index].cost;
		clanspell_general(sn, clanspell_table[index].duration, ch);
		return;
	}
	else if(!str_cmp(clanspell_table[index].name, "paradox" )) {
		victim = get_char_world(ch, arg2, FALSE);
		if(victim == NULL) {
			Cprintf(ch, "You can't find them.\n\r");
			return;
		}

		if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
 		|| wearing_nogate_item(ch))
 		{
     			Cprintf(ch, "Not in this room.\n\r");
     			return;
 		}

		if (!IS_NPC(ch))
		{
			if (is_safe(ch, victim) && victim != ch)
			{
				Cprintf(ch, "They are safe from your paradox spirit.\n\r");
				return;
			}
			check_killer(ch, victim);
		}

		ch->deity_points -= clanspell_table[index].cost;
		clanspell_paradox(gsn_paradox, ch->level, ch, victim, TARGET_CHAR);
		return;
	}
}

void
clanspell_general(int sn, int duration, CHAR_DATA * ch)
{
	AFFECT_DATA af;
	char buf[255];

	af.where = TO_AFFECTS;
	af.type = sn;
	af.level = ch->level;
	af.duration = duration;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	sprintf(buf, "%s summons the power of %s!\n\r",
		ch->name, capitalize(clan_table[ch->clan].name));

	act(buf, ch, NULL, NULL, TO_ROOM, POS_RESTING);
	Cprintf(ch, "You call upon the power of %s!\n\r",
		capitalize(clan_table[ch->clan].name));
}

void do_sheath(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *sheath = NULL;
	OBJ_DATA *obj = NULL;
	OBJ_DATA *o;
	char buf[255];

	// Default to wielded weapon.
	if(argument[0] == '\0') {
		obj = get_eq_char(ch, WEAR_WIELD);
		if(obj == NULL) {
			Cprintf(ch, "You aren't currently wearing a weapon.\n\r");
			return;
		}
	}
	else
	{
		obj = get_obj_carry(ch, argument, ch);

		if(obj == NULL) {
			Cprintf(ch, "You aren't carrying that.\n\r");
			return;
		}
	}

	// Find the sheath. Only take the first one found.
	for(o = ch->carrying; o != NULL; o = o->next_content) {
		if(o->item_type == ITEM_SHEATH
		&& o->wear_loc != WEAR_NONE) {
			sheath = o;
			// If its empty, use it, otherwise keep looking
			if(!sheath->contains)
				break;
			else
				continue;
		}
	}

	if(sheath == NULL) {
		Cprintf(ch, "You aren't wearing any sheathes.\n\r");
		return;
	}

	if(sheath->value[3] > 0
	&& obj->pIndexData->vnum != sheath->value[3]) {
		Cprintf(ch, "This sheath was specifically designed for a different weapon.\n\r");
		return;
	}

	if(obj->item_type != ITEM_WEAPON
	|| obj->value[0] != sheath->value[0]) {
		Cprintf(ch, "It won't fit in your sheath.\n\r");
		return;
	}

	if(sheath->contains != NULL) {
		Cprintf(ch, "There's already a weapon in your sheath.\n\r");
		return;
	}

	// Do it.
	if(obj->wear_loc != WEAR_NONE)
		unequip_char(ch, obj);

	obj_from_char(obj);
	obj_to_obj(obj, sheath);

	Cprintf(ch, "You take a moment and sheath %s.\n\r", obj->short_descr);
	sprintf(buf, "%s sheathes %s into %s.",
		capitalize(ch->name),
		obj->short_descr,
		sheath->short_descr);

	act(buf, ch, NULL, NULL, TO_ROOM, POS_RESTING);

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
}

void do_draw(CHAR_DATA *ch, char *argument)
{
	int mana_cost = 10;
	OBJ_DATA *obj = NULL;
	OBJ_DATA *sheath = NULL;
	OBJ_DATA *o;
	CHAR_DATA *victim = NULL;
	AFFECT_DATA af;

	// Find the sheath. Only take the first one found.
	for(o = ch->carrying; o != NULL; o = o->next_content) {
		if(o->item_type == ITEM_SHEATH
		&& o->wear_loc != WEAR_NONE) {
			sheath = o;
			// If it has a weapon, use it, otherwise keep looking
			if(sheath->contains)
        			break;
			else
        			continue;
		}
	}

	if(sheath == NULL) {
		Cprintf(ch, "You aren't wearing any sheathes.\n\r");
		return;
	}

	if(sheath->contains == NULL) {
		Cprintf(ch, "You find your sheath is currently empty.\n\r");
		return;
	}

	obj = sheath->contains;

	// Find the mana cost if its not default (15 mana)
	if(sheath->value[1] == SHEATH_QUICKDRAW)
		mana_cost = 30;
	else if(sheath->value[1] == SHEATH_SPELL)
		mana_cost = skill_table[sheath->value[2]].min_mana;
	else if(sheath->value[1] == SHEATH_NONE)
		mana_cost = 0;

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

	if(ch->mana < mana_cost) {
		Cprintf(ch, "You don't have enough mana, but you draw it anyways.\n\r");
		obj_from_obj(obj);
		obj_to_char(obj, ch);
		wear_obj(ch, obj, TRUE);
		return;
	}

	ch->mana -= mana_cost;

	// Ok, deal with all magic effects here somewhere but eventually
	Cprintf(ch, "You gracefully draw %s from its sheath!\n\r", obj->short_descr);
	act("$n gracefully draws $p!", ch, obj, ch, TO_NOTVICT, POS_RESTING);

	if(sheath->value[1] == SHEATH_FLAG) {
		act("$p glows with new light.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		af.where = TO_WEAPON;
		af.type = gsn_magic_sheath;
		af.level = obj->level;
		af.duration = 0;
		af.location = 0;
		af.modifier = 0;
		af.bitvector = sheath->value[2];
		affect_to_obj(obj, &af);
		obj_from_obj(obj);
		obj_to_char(obj, ch);
		wear_obj(ch, obj, TRUE);
		return;
	}

	af.where = TO_OBJECT;
	af.type = gsn_magic_sheath;
	af.level = obj->level;
	af.duration = 0;
	af.bitvector = 0;

	if(sheath->value[1] == SHEATH_HITROLL) {
		act("$p glows dark green.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		af.location = APPLY_HITROLL;
		af.modifier = sheath->value[2];
		affect_to_obj(obj, &af);
		return;
	}
	if(sheath->value[1] == SHEATH_DAMROLL) {
		act("$p glows dark blue.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		af.location = APPLY_DAMROLL;
		af.modifier = sheath->value[2];
		affect_to_obj(obj, &af);
	}
	if(sheath->value[1] == SHEATH_DICECOUNT) {
		act("$p glows bright yellow.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		af.location = 0;
		af.modifier = 0;
		af.extra = SHEATH_DICECOUNT;
		affect_to_obj(obj, &af);
	}
	if(sheath->value[1] == SHEATH_DICETYPE) {
		act("$p glows bright red.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		af.location = 0;
		af.modifier = 0;
		af.extra = SHEATH_DICETYPE;
		affect_to_obj(obj, &af);
	}

	obj_from_obj(obj);
	obj_to_char(obj, ch);
	wear_obj(ch, obj, TRUE);

	if(sheath->value[1] == SHEATH_QUICKDRAW) {
		if(argument[0] == '\0' && ch->fighting)
                	victim = ch->fighting;
		else
			victim = get_char_room(ch, argument);

		if(victim == NULL)
		{
        		Cprintf(ch, "They aren't here.\n\r");
        		return;
		}

		Cprintf(ch, "You attack with incredible speed!\n\r");
		act("$n draws and attacks in one swift motion!", ch, NULL, ch, TO_NOTVICT, POS_RESTING);
		if(ch->fighting == NULL)
			multi_hit(ch, victim, TYPE_UNDEFINED);
		multi_hit(ch, victim, TYPE_UNDEFINED);
		return;
	}
	if(sheath->value[1] == SHEATH_SPELL) {
		if(argument[0] == '\0')	 {
			if(ch->fighting)
				victim = ch->fighting;
			else
				victim = ch;
		}
		else if ((victim = get_char_room(ch, argument)) == NULL)
		{
        		Cprintf(ch, "They aren't here.\n\r");
       			return;
		}

		obj_cast_spell(sheath->value[2], obj->level, ch, victim, obj);
		return;
	}

	return;
}


/* Returns the cost to change the ownership of a crafted item.
   May return zero if the item is not crafted.
   Cost is calculated at 75 gold per level.
*/
int calculate_offering_tax_gold(OBJ_DATA *obj)
{
	if(IS_SET(obj->wear_flags, ITEM_CRAFTED)) {
		return obj->level * 75;
	}
	else
		return 0;
}

/* Returns the cost to change the ownership of a quest item.
   May return zero if the item is not crafted.
   Cost is calculated at 10%.
*/
int calculate_offering_tax_qp(OBJ_DATA *obj)
{
	int i = 0;
	int found = FALSE;

	// The itemList referred to is the quest items in quest.c
	while(1) {
		if(itemList[i].vnum == 0)
			break;

		if(itemList[i].vnum == obj->pIndexData->vnum) {
			found = TRUE;
			break;
		}
		i = i + 1;
	}

	if(found == TRUE)
		return itemList[i].requiredQP / 10;

	return 0;
}

/* This function is a modification of the "give" function. It is
   used to give items and change the respawn owner at the same
   time. It will transfer the item, and ownership of the item,
   to the target. Only works between two players.
*/
void do_offer(CHAR_DATA *ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char rest[MAX_STRING_LENGTH];
	int number, i, foundCount;
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	int gold_tax = 0, qp_tax = 0;
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	// No arguments
	if (arg1[0] == '\0')
	{
	        Cprintf(ch, "Give what to whom?\n\r");
	        return;
	}
	// First argument is accept, no second arg.
	else if (!str_prefix(arg1, "accept") && arg2[0] == '\0') {
		if(!IS_SET(ch->toggles, TOGGLES_PENDING_OFFER)) {
			Cprintf(ch, "No one has made you an offer yet.\n\r");
			return;
		}
		REMOVE_BIT(ch->toggles, TOGGLES_PENDING_OFFER);
		SET_BIT(ch->toggles, TOGGLES_ACCEPT_OFFER);
		Cprintf(ch, "You indicate your acceptance of the offer.\n\r");
		act("$n is willing to accept the offer.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}
	else if(!str_prefix(arg1, "refuse") && arg2[0] == '\0') {
		if(!IS_SET(ch->toggles, TOGGLES_PENDING_OFFER)) {
			Cprintf(ch, "No one has made you an offer yet.\n\r");
			return;
		}
		REMOVE_BIT(ch->toggles, TOGGLES_PENDING_OFFER);
		Cprintf(ch, "You indicate your refusal of the offer.\n\r");
		act("$n rejects the offer.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		return;
	}
	// First argument is anything else, but no second argument.
	else if(arg2[0] == '\0') {
		Cprintf(ch, "Give what to whom?\n\r");
		return;
	}

	number = mult_argument(arg1, rest);
        foundCount = count_obj_carry_by_name(ch, rest, ch);

        if (foundCount <= 0)
        {
                Cprintf(ch, "You do not have any %s.\n\r", rest);
                return;
        }

        if (foundCount < number)
        {
                Cprintf(ch, "You do not have %d %s's.\n\r", number, rest);
                return;
        }

        if (number == 0)
        {
                Cprintf(ch, "You waste your time by doing nothing.\n\r");
                return;
        }

        if (number < 0)
        {
                Cprintf(ch, "Now, that's just stupid.\n\r");
                return;
        }

        if ((victim = get_char_room(ch, arg2)) == NULL)
        {
                Cprintf(ch, "They aren't here.\n\r");
                return;
        }

        if (IS_NPC(victim)) {
		Cprintf(ch, "You can't offer items to mobs.\n\r");
		return;
	}

	if (ch == victim) {
		Cprintf(ch, "There's no point in trading items to yourself.\n\r");
		return;
	}

	if (IS_SET(victim->toggles, TOGGLES_PENDING_OFFER)) {
		Cprintf(ch, "Your offer has not been accepted yet.\n\r");
		return;
	}

	for (i = 0; i < number; i++)
	{
	        obj = get_obj_carry(ch, rest, ch);

                if (obj->wear_loc != WEAR_NONE)
                {
                        Cprintf(ch, "You must remove it first.\n\r");
                        return;
                }

		if (obj->respawn_owner == NULL) {
			Cprintf(ch, "This item belongs to no one, use the 'give' command instead.\n\r");
			return;
		}

		if (!is_name(obj->respawn_owner, ch->name)) {
			Cprintf(ch, "This item doesn't belong to you. You cannot offer it to others.\n\r");
			return;
		}

                if (!can_drop_obj(ch, obj))
                {
                        Cprintf(ch, "You can't let go of it.\n\r");
                        return;
                }

                if (victim->carry_number + get_obj_number(obj) > can_carry_n(victim))
                {
                        act("$N has $S hands full.", ch, NULL, victim, TO_CHAR,POS_RESTING);
                        return;
                }

                if (get_carry_weight(victim) + get_obj_weight(obj) > can_carry_w(victim))
                {
                        act("$N can't carry that much weight.", ch, NULL, victim, TO_CHAR, POS_RESTING);
                        return;
                }

                if (!can_see_obj(victim, obj))
                {
                        act("$N can't see it.", ch, NULL, victim, TO_CHAR, POS_RESTING);
                        return;
                }

		qp_tax = calculate_offering_tax_qp(obj);
		gold_tax = calculate_offering_tax_gold(obj);

		if(clan_table[ch->clan].pkiller != clan_table[victim->clan].pkiller) {
			qp_tax *= 2;
			gold_tax *=2;
		}

		// Start the hand shaking exchange
		if (!IS_SET(victim->toggles, TOGGLES_PENDING_OFFER)
	        &&  !IS_SET(victim->toggles, TOGGLES_ACCEPT_OFFER)) {
		        SET_BIT(victim->toggles, TOGGLES_PENDING_OFFER);
	                act("You offer $p to $N.", ch, obj, victim, TO_CHAR, POS_RESTING);
								                			act("$n offers you $p.", ch, obj, victim, TO_VICT, POS_RESTING);
										                	act("$n makes an offer to $N.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
												        // Add a message to display the tax
													Cprintf(victim, "You will need to sacrifice %d %s to gain ownership.\n\r", gold_tax > 0 ? gold_tax : qp_tax, gold_tax > 0 ? "gold" : "quest points");

		        act("Type 'offer accept' to allow the transfer of ownership.", ch, obj, victim, TO_VICT, POS_RESTING);
			act("Type 'offer refuse' to refuse the transfer.", ch, obj, victim, TO_VICT, POS_RESTING);
												        return;
												}

		// Check for requirements
		if(gold_tax > 0) {
			if(victim->gold < gold_tax) {
				Cprintf(ch, "%s is short of gold to complete the transfer. Aborted.\n\r", victim->name);
				Cprintf(victim, "You need %d gold to appease the gods. Offer aborted.\n\r", gold_tax);
				REMOVE_BIT(victim->toggles, TOGGLES_ACCEPT_OFFER);
				return;
			}
			victim->gold -= gold_tax;
		}
		else if(qp_tax > 0) {
			if(victim->questpoints < qp_tax) {
				Cprintf(ch, "%s is short of quest points to complete the tranfer. Aborted.\n\r", victim->name);
				Cprintf(victim, "You need %d quest points to appease the gods. Offer aborted.\n\r", qp_tax);
				REMOVE_BIT(victim->toggles, TOGGLES_ACCEPT_OFFER);
				return;
			}
			victim->questpoints -= qp_tax;
		}
		else {
			Cprintf(ch, "This item shouldn't have an owner.\n\r");
			return;
		}
		// Change ownership.
		free_string(obj->respawn_owner);
		obj->respawn_owner = str_dup(victim->name);
                obj_from_char(obj);
                obj_to_char(obj, victim);
                MOBtrigger = FALSE;
		if(is_clan(victim))
			obj->clan_status = CS_CLANNER;
		else
			obj->clan_status = CS_NONCLANNER;

                act("$n offers $p to $N.", ch, obj, victim, TO_NOTVICT, POS_RESTING);
                act("$n offers you $p.", ch, obj, victim, TO_VICT, POS_RESTING);
                act("You offer $p to $N.", ch, obj, victim, TO_CHAR, POS_RESTING);
		Cprintf(ch, "You give up ownership of %s.\n\r", obj->short_descr);
		Cprintf(victim, "You assume ownership of %s.\n\r", obj->short_descr);
		// Pay the tax here.
		REMOVE_BIT(victim->toggles, TOGGLES_ACCEPT_OFFER);
	}

	return;
}
/* This function is used when you want to claim ownership
   of a valid item that has no current owner.
*/
void do_claim(CHAR_DATA *ch, char *argument)
{
        OBJ_DATA *obj = NULL;

	obj = get_obj_carry(ch, argument, ch);

	if(obj == NULL) {
		Cprintf(ch, "You aren't carrying that.\n\r");
		return;
	}

	if(calculate_offering_tax_qp(obj) == 0
        && calculate_offering_tax_gold(obj) == 0) {
		Cprintf(ch, "You can only claim ownership of quest or crafted items.\n\r");
		return;
	}

	if(!IS_IMMORTAL(ch) && obj->respawn_owner != NULL) {
		Cprintf(ch, "That item is already owned by someone.\n\r");
		return;
	}

	if(IS_IMMORTAL(ch) && obj->respawn_owner != NULL) {
		free_string(obj->respawn_owner);
		obj->respawn_owner = NULL;
	}

	obj->respawn_owner = str_dup(ch->name);
	Cprintf(ch, "You claim ownership of %s.\n\r", obj->short_descr);

	return;
}

