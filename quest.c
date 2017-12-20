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
 *       ROM 2.4 is copyright 1993-1995 Russ Taylor                         *
 *       ROM has been brought to you by the ROM consortium                  *
 *           Russ Taylor (rtaylor@pacinfo.com)                              *
 *           Gabrielle Taylor (gtaylor@pacinfo.com)                         *
 *           Brian Moore (rom@rom.efn.org)                                  *
 *       By using this code, you have agreed to follow the terms of the     *
 *       ROM license, in the file Rom24/doc/rom.license                     *
 ***************************************************************************/

 /***************************************************************************
 *  Automated Quest code written by Vassago of MOONGATE, moongate.ams.com   *
 *  4000. Copyright (c) 1996 Ryan Addams, All Rights Reserved. Use of this  *
 *  code is allowed provided you add a credit line to the effect of:        *
 *  "Quest Code (c) 1996 Ryan Addams" to your logon screen with the rest    *
 *  of the standard diku/rom credits. If you use this or a modified version *
 *  of this code, let me know via email: moongate@moongate.ams.com. Further *
 *  updates will be posted to the rom mailing list. If you'd like to get    *
 *  the latest version of quest.c, please send a request to the above add-  *
 *  ress. Quest Code v2.00.                                                 *
 ***************************************************************************/


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "clan.h"
#include "merc.h"
#include "utils.h"


DECLARE_DO_FUN(do_say);

#define TERRA 0
#define DOMINIA 1

#define OBJ_VNUM_SELINA_BOOTS 10910
#define OBJ_VNUM_SELINA_FIRE  10911
#define OBJ_VNUM_SELINA_MIGHT 10912
#define OBJ_VNUM_SELINA_WING  10913
#define OBJ_VNUM_SELINA_BELT  10914
#define OBJ_VNUM_SELINA_RING  30827

QUEST_ITEM itemList[] =
{
	{8314, TERRA, "cloak", "The cloak of Bosco", 3000},
	{8311, TERRA, "shield", "Shield of Bosco", 2000},
	{2224, TERRA, "ring", "The Diamond Ring", 1500},
	{8312, TERRA, "dagger", "The Bosco dagger", 1000},
	{8310, TERRA, "amulet", "Amulet of Bosco", 1000},
	{11696, TERRA, "blanket", "The Blanket of Regeneration", 1000},
	{8313, TERRA, "stone", "The stone of Bosco", 850},
	{8320, TERRA, "key", "Bosco's key on a chain", 30},

	{10910, DOMINIA, "boots", "Selina's Swift Boots", 2500},
	{10911, DOMINIA, "fire", "Selina's Fire", 2500},
	{10913, DOMINIA, "might", "Selina's Might", 1500},
	{10914, DOMINIA, "wings", "Selina's Wings", 2000},
	{10912, DOMINIA, "belt", "Selina's Belt", 1000},
	{30827, DOMINIA, "ring", "Selina's Magic Ring", 50},

	{0, 0, NULL, NULL, 0}
};

/* Object vnums for object quest 'tokens'. In Moongate, the tokens are
   things like 'the Shield of Moongate', 'the Sceptre of Moongate'. These
   items are worthless and have the rot-death flag, as they are placed
   into the world when a player receives an object quest. */

#define QUEST_OBJQUEST1 8315
#define QUEST_OBJQUEST2 8316
#define QUEST_OBJQUEST3 8317
#define QUEST_OBJQUEST4 8318
#define QUEST_OBJQUEST5 8319

#define QUEST_MOB_ASSAULT_SLIVER 6469
#define QUEST_MOB_ASSAULT_TROLL 6471
#define QUEST_MOB_ASSAULT_KIRRE 6472
#define QUEST_MOB_ASSAULT_ELF 6474
#define QUEST_MOB_ASSAULT_HUMAN 6473
#define QUEST_MOB_ASSAULT_DWARF 6475
#define QUEST_MOB_ESCORT
#define QUEST_MOB_RESCUE

// Define all the assault mobs

struct assault_mob_type {
	int continent;
	int rank;
	char *race_type;
	int vnum;
	char *mob_short;
	char *mob_name;
	char *mob_long;
};

#define MAX_ASSAULT_MOBS 18

const struct assault_mob_type assault_mobs[MAX_ASSAULT_MOBS] =
{
	{ TERRA, 1, "sliver", QUEST_MOB_ASSAULT_SLIVER,
	 "a sliver blade drone", "sliver blade drone assault",
	 "A small sliver blade drone is scuttling about, following orders.\n"},

	{ TERRA, 2, "sliver", QUEST_MOB_ASSAULT_SLIVER,
	 "a sliver shock trooper", "sliver shock trooper assault",
	 "A large sliver shock trooper is here, engaged in battle.\n"},

	{ TERRA, 3, "sliver", QUEST_MOB_ASSAULT_SLIVER,
	 "a sliver assault leader", "sliver assault leader",
	 "A giant sliver assault leader is here, directing the attack.\n"},

	{ TERRA, 1, "troll", QUEST_MOB_ASSAULT_TROLL,
	 "a troll pikeman", "troll pike man assault",
	 "A troll pikeman is here, poking whoever gets in his way.\n"},

	{ TERRA, 2, "troll", QUEST_MOB_ASSAULT_TROLL,
	 "a troll blademaster", "troll blade master assault",
	 "A troll blademaster is here, slicing up the locals.\n"},

	{ TERRA, 3, "troll", QUEST_MOB_ASSAULT_TROLL,
	 "a troll warleader", "troll war leader assault",
	 "A troll warleader is here, embracing the carnage.\n"},

	{ TERRA, 1, "kirre", QUEST_MOB_ASSAULT_KIRRE,
	 "a kirre war cat", "kirre war cat assault",
	 "A kirre war cat is here, stalking prey.\n"},

	{ TERRA, 2, "kirre", QUEST_MOB_ASSAULT_KIRRE,
	 "a kirre champion", "kirre champion assault",
	 "A kirre champion is here, on the hunt.\n"},

	{ TERRA, 3, "kirre", QUEST_MOB_ASSAULT_KIRRE,
	 "a kirre sabre lord", "kirre sabre lord assault",
	 "A sabretooth kirre lord is here.\n"},

	{ DOMINIA, 1, "elf", QUEST_MOB_ASSAULT_ELF,
	 "an elven soldier", "elven elf soldier assault",
	 "An elven soldier is here, on patrol.\n"},

	{ DOMINIA, 2, "elf", QUEST_MOB_ASSAULT_ELF,
	 "an elven archer", "elven elf archer assault",
	 "An elven archer is here, taking aim.\n"},

	{ DOMINIA, 3, "elf", QUEST_MOB_ASSAULT_ELF,
	 "an elven commander", "elven commander assault",
	 "An elven commander is here, at the ready.\n"},

	{ DOMINIA, 1, "human", QUEST_MOB_ASSAULT_HUMAN,
	 "a human invader", "human invader assault",
	 "A human invader is here from far away.\n"},

	{ DOMINIA, 2, "human", QUEST_MOB_ASSAULT_HUMAN,
	 "a human bandit", "human bandit assault",
	 "A human bandit is here, blade drawn.\n"},

	{ DOMINIA, 3, "human", QUEST_MOB_ASSAULT_HUMAN,
	 "a human despot", "human despot assault",
	 "A human despot is here, ravaging the land.\n"},

	{ DOMINIA, 1, "dwarf", QUEST_MOB_ASSAULT_DWARF,
	 "a dwarven fighter", "dwarven fighter assault",
	 "A dwarven fighter is here, scowling.\n"},

	{ DOMINIA, 2, "dwarf", QUEST_MOB_ASSAULT_DWARF,
	 "a dwarven battle rager", "dwarven battle rager assault",
	 "A dwarven battle rager is here, totally berserk!\n"},

	{ DOMINIA, 3, "dwarf", QUEST_MOB_ASSAULT_DWARF,
	 "a dwarven warsmith", "dwarven war smith assault",
	 "A dwarven warsmith is here, overseeing the conquest.\n"},

};


/* external declerations */
extern int double_qp_ticks;
extern int double_xp_ticks;
extern void exp_reward(CHAR_DATA *, int, int);
extern void size_mob(CHAR_DATA * ch, CHAR_DATA * victim, int level);

/* Local functions */
void generate_quest(CHAR_DATA *, CHAR_DATA *, int);
bool chance(int num);
ROOM_INDEX_DATA *find_location(CHAR_DATA * ch, char *arg);
void test_port_promote(CHAR_DATA *);
void test_port_restore(CHAR_DATA *);
void test_port_gearmelee(CHAR_DATA *);
void test_port_gearmagic(CHAR_DATA *);
DECLARE_DO_FUN(do_restore);
DECLARE_DO_FUN(do_set);
DECLARE_DO_FUN(do_advance);
DECLARE_DO_FUN(do_maxstats);


/* CHANCE function. I use this everywhere in my code, very handy :> */
bool
chance(int num)
{
	return ((number_range(1, 100)) <= num) ? TRUE : FALSE;
}

// Display the remaining timer
void quest_time(CHAR_DATA *ch)
{
	if(IS_QUESTING(ch)) {
		Cprintf(ch, "You have %d minutes to finish and report back to the questmaster.\n\r", ch->pcdata->quest.timer);
	}
	else {
		if(ch->pcdata->quest.timer == 0) {
			Cprintf(ch, "{RYou may now quest again.{x\n\r");
		}
		else if(ch->pcdata->quest.timer == 1) {
			Cprintf(ch, "There is roughly a minute left until you can begin another quest.\n\r");
		}
		else {
			Cprintf(ch, "You have %d minutes until the beginning of your next quest.\n\r", ch->pcdata->quest.timer);
		}
	}

	return;
}

// Display the info on the current quest and target.
void quest_info(CHAR_DATA *ch)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;

	if(!IS_QUESTING(ch)) {
		Cprintf(ch, "You aren't currently on a quest.\n\r");
		return;
	}

	if(ch->pcdata->quest.type == QUEST_TYPE_ITEM) {
		obj = create_object(get_obj_index(ch->pcdata->quest.target), 0);
		obj_to_room(obj, ch->in_room);
		Cprintf(ch, "You are on a quest to retrieve %s.\n\r", obj->short_descr);
		Cprintf(ch, "Please find it and return it to the questmaster!\n\r");
		Cprintf(ch, "You may start your search in %s in %s.\n\r",
				ch->pcdata->quest.destination->name,
				ch->pcdata->quest.destination->area->name);
		extract_obj(obj);
		return;
	}
	else if(ch->pcdata->quest.type == QUEST_TYPE_VILLAIN) {
		mob = create_mobile(get_mob_index(ch->pcdata->quest.target));
		char_to_room(mob, ch->in_room);
		if(ch->pcdata->quest.progress == 0) {
			Cprintf(ch, "You are on a quest to defeat the villainous %s!\n\r",
				mob->short_descr);
			Cprintf(ch, "You may start your search in %s in %s.\n\r",
				ch->pcdata->quest.destination->name,
				ch->pcdata->quest.destination->area->name);
		}
		else {
			Cprintf(ch, "Your quest is almost finished.\n\r");
			Cprintf(ch, "Report back to the questmaster before time runs out!\n\r");
		}
		extract_char(mob, TRUE);
		return;
	}
	else if(ch->pcdata->quest.type == QUEST_TYPE_ASSAULT) {
		mob = create_mobile(get_mob_index(ch->pcdata->quest.target));
                char_to_room(mob, ch->in_room);
                Cprintf(ch, "You are currently on a quest to stop the assault on %s.\n\r", ch->pcdata->quest.destination->area->name);

		Cprintf(ch, "So far you have slain %d of the attacking %s force.\n\r", ch->pcdata->quest.progress, race_table[mob->race].name);
		Cprintf(ch, "Slay as many as you can, and report back, before time runs out.\n\r");
                extract_char(mob, TRUE);
                return;
	}
}

// Give up the current quest, don't reset timer however.
void quest_reset(CHAR_DATA *ch)
{

	if(!IS_QUESTING(ch)) {
		Cprintf(ch, "You aren't currently on a quest.\n\r");
		return;
	}

	Cprintf(ch, "You give up on the current quest.\n\r");
 	REMOVE_BIT(ch->act, PLR_QUESTING);
        ch->pcdata->quest.giver = NULL;
        ch->pcdata->quest.type = QUEST_TYPE_NONE;
        ch->pcdata->quest.target = 0;
        ch->pcdata->quest.progress = 0;
        ch->pcdata->quest.destination = NULL;
}

// Display a list of the items available for purchase
// using quest points.
void quest_list(CHAR_DATA *ch, CHAR_DATA *questmaster)
{
    int i;

    act("$n asks $N for a list of quest items.", ch, NULL, questmaster, TO_ROOM, POS_RESTING);
    act("You ask $N for a list of quest items.", ch, NULL, questmaster, TO_CHAR, POS_RESTING);

    Cprintf(ch, "Current quest items available for purchase:\n\r");

    for (i = 0; itemList[i].keyword; i++)
        if (ch->in_room->area->continent == itemList[i].continent)
            Cprintf(ch, "%-4dqp ........ %s\n\r", itemList[i].requiredQP, itemList[i].desc);

    return;
}

// Attempt to purchase the quest item indicated by the argument.
void quest_buy(CHAR_DATA *ch, CHAR_DATA *questmaster, char *argument)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH];
	int i;
	bool found = FALSE;

	if (argument[0] == '\0')
	{
		Cprintf(ch, "To buy an item, type 'QUEST BUY <item>'.\n\r");
		return;
	}

	for (i = 0; itemList[i].keyword; i++)
	{
		if (ch->in_room->area->continent != itemList[i].continent)
			continue;

		if (is_name(argument, itemList[i].keyword))
		{
			found = TRUE;

			if (ch->questpoints >= itemList[i].requiredQP)
			{
				ch->questpoints -= itemList[i].requiredQP;
				obj = create_object(get_obj_index(itemList[i].vnum), ch->level);
				// Set to correct clan status
				if(clan_table[ch->clan].pkiller)
					obj->clan_status = CS_CLANNER;
				else
					obj->clan_status = CS_NONCLANNER;

				// Set the owner for the claim system
				obj->respawn_owner = str_dup(ch->name);
			}
			else
			{
				sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.", ch->name);
				do_say(questmaster, buf);
				return;
			}
		}
	}

	if (!found)
	{
		sprintf(buf, "I don't have that item, %s.", ch->name);
		do_say(questmaster, buf);
		return;
	}

	act("$N gives $p to $n.", ch, obj, questmaster, TO_ROOM, POS_RESTING);
	act("$N gives you $p.", ch, obj, questmaster, TO_CHAR, POS_RESTING);
	obj_to_char(obj, ch);

	return;
}

// Request a new quest if your timer is ready. Will call generate_quest
// to manage the creation of the actual quest. You may request a specific
// type of quest, or don't specify one and get a random type.
void quest_request(CHAR_DATA *ch, CHAR_DATA *questmaster, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	int type_of_quest;

	// Will only apply during a quest.
	if (IS_QUESTING(ch))
	{
		sprintf(buf, "But you're already on a quest!");
		do_say(questmaster, buf);
		return;
	}

	// Will only apply if current quest is done and waiting for timer.
	if (ch->pcdata->quest.timer > 0)
	{
		sprintf(buf, "You're very brave, %s, but let someone else have a chance.", ch->name);
		do_say(questmaster, buf);
		sprintf(buf, "Come back later.");
		do_say(questmaster, buf);
		return;
	}

	// Determine the type of quest
	if(!str_prefix(argument, "item"))
		type_of_quest = QUEST_TYPE_ITEM;
	else if(!str_prefix(argument, "villain"))
		type_of_quest = QUEST_TYPE_VILLAIN;
	else if(!str_prefix(argument, "assault"))
		type_of_quest = QUEST_TYPE_ASSAULT;
	/*
	else if(!str_prefix(argument, "rescue"))
		type_of_quest = QUEST_TYPE_RESCUE;
 	else if(!str_prefix(argument, "escort"))
		type_of_quest = QUEST_TYPE_ESCORT;
	*/
	else {
		type_of_quest = number_range(QUEST_TYPE_ITEM, QUEST_TYPE_VILLAIN);
	}

	act("$n asks $N for a quest.", ch, NULL, questmaster, TO_ROOM, POS_RESTING);
	act("You ask $N for a quest.", ch, NULL, questmaster, TO_CHAR, POS_RESTING);

	sprintf(buf, "Thank you, brave %s!", ch->name);
	do_say(questmaster, buf);

	// Set up the quest
 	generate_quest(ch, questmaster, type_of_quest);

	sprintf(buf, "You have %d minutes to complete this quest.", ch->pcdata->quest.timer);
	do_say(questmaster, buf);

	return;
}


void quest_complete(CHAR_DATA *ch, CHAR_DATA *questmaster)
{
	OBJ_DATA *obj = NULL, *obj_next;
	CHAR_DATA *mob = NULL;
	char buf[MAX_STRING_LENGTH];
	int completed = FALSE;
	int gold_reward = 0;
	int xp_bonus = 0;
	int qp_reward = 0;
	int i;

	act("$n informs $N $e has completed $s quest.", ch, NULL, questmaster, TO_ROOM, POS_RESTING);
	act("You inform $N you have completed $s quest.", ch, NULL, questmaster, TO_CHAR, POS_RESTING);

	if (ch->pcdata->quest.giver != questmaster)
	{
		sprintf(buf, "I never sent you on a quest!  Perhaps you're thinking of someone else.");
		do_say(questmaster, buf);
		return;
	}

	if (IS_QUESTING(ch))
	{
		// Check the win conditions

		// For villain quests, the target mob should be
		// killed and the progress recorded.
		// For assault quests, the progress represents how
		// many kills were completed.
		// For escort quests, the progress represents having
		// talked to the NPC escort at the destination.
		if(ch->pcdata->quest.type == QUEST_TYPE_VILLAIN
		|| ch->pcdata->quest.type == QUEST_TYPE_ASSAULT
		|| ch->pcdata->quest.type == QUEST_TYPE_ESCORT) {
			if(ch->pcdata->quest.progress > 0)
				completed = TRUE;
		}
		// For item quests, you must be carrying to requested
		// item. It will be removed.
		else if(ch->pcdata->quest.type == QUEST_TYPE_ITEM)
		{
			bool obj_found = FALSE;

			for (obj = ch->carrying; obj != NULL; obj = obj_next)
			{
				obj_next = obj->next_content;

				if (obj != NULL && obj->pIndexData->vnum == ch->pcdata->quest.target)
				{
					obj_found = TRUE;
					break;
				}
			}

			if (obj_found == TRUE)
			{
				act("You hand $p to $N.", ch, obj, questmaster, TO_CHAR, POS_RESTING);
				act("$n hands $p to $N.", ch, obj, questmaster, TO_ROOM, POS_RESTING);
				extract_obj(obj);
				completed = TRUE;
			}
		}
		// For rescue quests, the requested mob must be in the room
		// with the player and questmaster.
		else if(ch->pcdata->quest.type == QUEST_TYPE_RESCUE)
		{
			bool mob_found = FALSE;

			for (mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room)
			{
				if (mob != NULL && mob->pIndexData->vnum == ch->pcdata->quest.target)
				{
					mob_found = TRUE;
					break;
				}
			}

			if (mob_found == TRUE)
			{
				extract_char(mob, FALSE);
				completed = TRUE;
			}
		}
	}
	else {
		sprintf(buf, "You aren't currently on a quest.");
		do_say(questmaster, buf);
		return;
	}

	if (completed) {
		// Assault a longer quest
		if(ch->pcdata->quest.type == QUEST_TYPE_ASSAULT) {
			// Curve the assault reward.
			int points = ch->pcdata->quest.progress;

			gold_reward = xp_bonus = qp_reward = 0;

			for(i = 0; i < points; i++) {

				if(i < 10) {
					gold_reward += number_range(15, 30);
					xp_bonus += number_range(50, 150);
					qp_reward += number_range(8, 14);
				}
				else if(i < 15) {
					gold_reward += number_range(7, 15);
					xp_bonus += number_range(25, 75);
					qp_reward += number_range(4, 7);
				}
				else {
					gold_reward += number_range(5, 10);
					xp_bonus += number_range(15, 50);
					qp_reward += number_range(3, 5);
				}
			}
		}
		else {
			gold_reward = number_range(ch->level, ch->level * 2);
			xp_bonus = number_range(100, 300);
			qp_reward = number_range(30, 50);
		}

		if(ch->pcdata->quest.type == QUEST_TYPE_ITEM) {
			gold_reward /= 2;
			xp_bonus = xp_bonus * 2 / 3;
			qp_reward -= 10;
		}

		// Below level 51 qp is only a token amount (1-10)
		if(ch->level < 51) {
			qp_reward = qp_reward * 2 / 5;
		}

		if (double_qp_ticks > 0)
			qp_reward *= 2;
		if (double_xp_ticks > 0)
			xp_bonus *= 2;

		sprintf(buf, "As your reward for this quest, I present you with %d qp, %d gold and %d experience points.",
		qp_reward, gold_reward, xp_bonus);
		do_say(questmaster, buf);

		ch->gold += gold_reward;
		ch->questpoints += qp_reward;
		exp_reward(ch, xp_bonus, FALSE);

		REMOVE_BIT(ch->act, PLR_QUESTING);
                ch->pcdata->quest.giver = NULL;
                ch->pcdata->quest.type = QUEST_TYPE_NONE;
                ch->pcdata->quest.target = 0;
                ch->pcdata->quest.progress = 0;
                ch->pcdata->quest.destination = NULL;

		return;
	}

	if (!completed)
	{
		sprintf(buf, "You haven't completed the quest yet, but there is still time!");
		do_say(questmaster, buf);
		return;
	}
}

void
do_quest(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *questman;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	
	if (IS_NPC(ch)) {
	    Cprintf(ch, "Imagine that.  Mobs trying to help.  How wonderous!\n\r");
	    return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (!str_prefix(arg1, "time"))
        {
                quest_time(ch);
                return;
        }
	else if(!str_prefix(arg1, "info"))
	{
		quest_info(ch);
		return;
	}
	else if(!str_cmp(arg1, "reset"))
	{
		quest_reset(ch);
		return;
	}

	// Checks for a character in the room with spec_questmaster set.
	for (questman = ch->in_room->people; questman != NULL; questman = questman->next_in_room)
	{
		if (!IS_NPC(questman))
			continue;

		if (questman->spec_fun == spec_lookup("spec_questmaster"))
			break;
	}

	// No questermaster was found.
	if (questman == NULL || questman->spec_fun != spec_lookup("spec_questmaster"))
	{
		Cprintf(ch, "You can't do that here.\n\r");
		return;
	}

	// Now do calls for the methods that require
        // the quest master to be present.
	if (!str_prefix(arg1, "list"))
	{
		quest_list(ch, questman);
		return;
	}
	else if (!str_prefix(arg1, "buy"))
	{
		quest_buy(ch, questman, arg2);
		return;
	}
	else if (!str_prefix(arg1, "request"))
	{
		quest_request(ch, questman, arg2);
		return;
	}
	else if (!str_prefix(arg1, "complete"))
	{
		quest_complete(ch, questman);
		return;
	}
	else if(TEST_PORT && !str_prefix(arg1, "promote")) {
		test_port_promote(ch);
		return;
	}
	else if(TEST_PORT && !str_prefix(arg1, "restore")) {
		test_port_restore(ch);
		return;
	}
	else if(TEST_PORT && !str_prefix(arg1, "gearmagic")) {
		test_port_gearmagic(ch);
		return;
	}
	else if(TEST_PORT && !str_prefix(arg1, "gearmelee")) {
		test_port_gearmelee(ch);
		return;
	}

	Cprintf(ch, "QUEST commands: POINTS INFO TIME REQUEST COMPLETE LIST BUY TALK.\n\r");
	if(TEST_PORT) {
		Cprintf(ch, "Test Port commands: PROMOTE RESTORE GEARMAGIC GEARMELEE\n\r");
	}
	Cprintf(ch, "For more information, type 'HELP QUEST'.\n\r");
	return;
}

// Display the messages for an item quest
void quest_story_item(CHAR_DATA *questman, CHAR_DATA *victim, OBJ_DATA *questitem) {
	char buf[MAX_STRING_LENGTH];
	ROOM_INDEX_DATA *room = victim->in_room;

	switch(number_range(0,3)) {
      	case 0: sprintf(buf, "Vile thieves have stolen %s from the royal treasury!", questitem->short_descr); break;
		case 1: sprintf(buf, "A group of diplomats visited last week, and now %s is missing from the capital!", questitem->short_descr); break;
		case 2: sprintf(buf, "The local ruler's bumbling aid seems to have misplaced %s!", questitem->short_descr); break;
		case 3: sprintf(buf, "A travelling caravan was robbed of %s out in the wilds!", questitem->short_descr); break;
	}

      do_say(questman, buf);

      sprintf(buf, "You may begin your search in %s for %s!", room->area->name, room->name);
      do_say(questman, buf);
      return;
}

void quest_story_villain(CHAR_DATA *questman, CHAR_DATA *victim)
{
	char buf[MAX_STRING_LENGTH];
	ROOM_INDEX_DATA *room = victim->in_room;

	switch(number_range(0,3)) {
	    case 0: sprintf(buf, "One of this land's worst foes, %s, has escaped from the dungeon!", victim->short_descr); break;
	    case 1: sprintf(buf, "A villain by the name of %s has named itself an enemy of our town.", victim->short_descr); break;
   	    case 2: sprintf(buf, "There has been an attempt on my life by %s!", victim->short_descr); break;
	    case 3: sprintf(buf, "The local people are being terrorized by %s.", victim->short_descr); break;
	}
      do_say(questman, buf);

	switch(number_range(0,3)) {
	    case 0: sprintf(buf, "Since then, %s has murdered %d people!", victim->short_descr, number_range(2, 20)); break;
	    case 1: sprintf(buf, "The local people are very scared of %s.", victim->short_descr); break;
	    case 2: sprintf(buf, "A young girl has recently gone missing, we fear the worst."); break;
	    case 3: sprintf(buf, "There is fear %s may try and start a rebellion.", victim->short_descr); break;
	}
	do_say(questman, buf);

	switch(number_range(0,3)) {
	    case 0: sprintf(buf, "The penalty for this crime is death, and I am sending you to deliver the sentence."); break;
	    case 1: sprintf(buf, "The town has chosen YOU to resolve this situation."); break;
   	    case 2: sprintf(buf, "Only you are strong enough to end this threat!"); break;
	    case 3: sprintf(buf, "I wish you the best of luck in slaying %s.", victim->short_descr); break;
	}
      do_say(questman, buf);


      sprintf(buf, "Seek %s out somewhere in the vicinity of %s!", victim->short_descr, room->name);
      do_say(questman, buf);

	sprintf(buf, "You can find that location in %s.", room->area->name);
      do_say(questman, buf);
	return;
}

void quest_story_assault
	       (CHAR_DATA *questman,
		CHAR_DATA *victim,
		int group_size,
		int index)
{
	char buf[MAX_STRING_LENGTH];

	if(group_size == 1) {
		sprintf(buf, "Although you are alone, I must ask for your help.");
	}
	else {
		sprintf(buf, "With your group of %d, nothing can stop you.", group_size);
	}
	do_say(questman, buf);

	switch(number_range(0,2)) {
	    case 0: sprintf(buf, "A group of foreign invaders has arrived!"); break;
	    case 1: sprintf(buf, "A large force has been discovered."); break;
   	    case 2: sprintf(buf, "A fierce battle has already begun!"); break;
	}
      do_say(questman, buf);

	switch(number_range(0,2)) {
	    case 0: sprintf(buf, "I have personally seen the %s troops!", assault_mobs[index].race_type); break;
	    case 1: sprintf(buf, "Word has spread that the %s army is already waging battle.", assault_mobs[index].race_type); break;
   	    case 2: sprintf(buf, "The deadly %s strike force must be stopped.", assault_mobs[index].race_type); break;
	}
      do_say(questman, buf);

	switch(number_range(0,2)) {
	    case 0: sprintf(buf, "Innocents are being slaughtered as we speak!"); break;
	    case 1: sprintf(buf, "Show the invaders no mercy."); break;
   	    case 2: sprintf(buf, "You must mount an offensive immediately!"); break;
	}
      do_say(questman, buf);

	switch(number_range(0,2)) {
	    case 0: sprintf(buf, "The area of %s is under attack.", victim->in_room->area->name); break;
	    case 1: sprintf(buf, "The enemy may be found in %s.", victim->in_room->area->name); break;
   	    case 2: sprintf(buf, "The battle for %s is in your hands.", victim->in_room->area->name); break;
	}
      do_say(questman, buf);
}

// Customize and place a single assault mob based on the criteria.
void generate_assault_mob(CHAR_DATA *ch, int level, char *race, ROOM_INDEX_DATA *room, int assault_size)
{
	ROOM_INDEX_DATA *mob_room = NULL;
	CHAR_DATA *mob = NULL;
	int i = 0;
	int rank = 1;
	int roll = number_percent();
	int room_vnum;

	// Determine the mob rank and level
	if(roll < 50) {
		rank = 1;
		level -= 2;
	}
	else if(roll < 80) {
		rank = 2;
		level += 2;
	}
	else {
		rank = 3;
		level += 4;
	}

	// Find the actual data for this class of assault mob.
	for(i = 0; i < MAX_ASSAULT_MOBS; i++) {
		if(!str_cmp(assault_mobs[i].race_type, race)
		&& assault_mobs[i].rank == rank)
			break;
	}

	// The i-th index should have all the data we need.
	mob = create_mobile(get_mob_index(assault_mobs[i].vnum));
	size_mob(ch, mob, level);
	mob->max_hit = mob->max_hit * 3 / 2;
	mob->hit = mob->max_hit;
	mob->damroll = mob->damroll + (5 * rank);

	// Scale mobs up based on assault size.
	if(assault_size > 5) {
		mob->max_hit = mob->max_hit +
			mob->max_hit * ((assault_size - 5) / 10);
		mob->hitroll = mob->hitroll +
			(assault_size - 5) * 4;
		mob->damroll = mob->damroll +
			(assault_size - 5) * 4;
	}

	if(mob->short_descr != NULL)
		free_string(mob->short_descr);
	mob->short_descr = str_dup(assault_mobs[i].mob_short);

	if(mob->name != NULL)
		free_string(mob->name);
	mob->name = str_dup(assault_mobs[i].mob_name);

	if(mob->long_descr != NULL)
		free_string(mob->long_descr);
	mob->long_descr = str_dup(assault_mobs[i].mob_long);

	mob->rot_timer = ch->pcdata->quest.timer;

	mob->spec_fun = spec_lookup("spec_assault");

	// Find a random room in the correct area
	room_vnum = number_range(room->area->min_vnum, room->area->max_vnum);
	while(1) {
		mob_room = get_room_index(room_vnum);
		if(mob_room != NULL
		&& !IS_SET(mob_room->room_flags, ROOM_NO_MOB)
		&& !IS_SET(mob_room->room_flags, ROOM_PET_SHOP)
		&& !IS_SET(mob_room->room_flags, ROOM_NO_KILL)
		&& !IS_SET(mob_room->room_flags, ROOM_PRIVATE)
		&& !IS_SET(mob_room->room_flags, ROOM_GODS_ONLY)
		&& !IS_SET(mob_room->room_flags, ROOM_SOLITARY)
		&& !IS_SET(mob_room->room_flags, ROOM_ARENA)
		&& !IS_SET(mob_room->room_flags, ROOM_FERRY))
			break;

		// Pick the next room down, since builders usually
		// start building at the lower vnums
		room_vnum--;

		// Last sanity check, what if no room is found?
		if(room_vnum < room->area->min_vnum) {
			mob_room = NULL;
			break;
		}
	}

	// There has been a big problem with the quest, don't use this mob.
	if(mob_room == NULL) {
		extract_char(mob, FALSE);
		return;
	}

	char_to_room(mob, mob_room);
}


// Create a quest and assign to the character.
void
generate_quest(CHAR_DATA *ch, CHAR_DATA *questman, int quest_type) {
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *room;
    OBJ_DATA *questitem;
    char buf[MAX_STRING_LENGTH];
    int i, mob_skip, questfound=TRUE;
    int iteration = 1;

    // In all cases we can define a quest by first finding a good
    // target location close to the player's level. We can do this
    // by doing a skip-list of mobs until we find a good candidate.
    for (victim = char_list; victim != NULL; victim = victim->next) {
        // Skip-search by 1-200 indexes each time
        if (iteration == 1)
            mob_skip = number_range(1, 200);

        if (iteration == 2)
            mob_skip = number_range(1, 50);

        if (iteration == 3)
            mob_skip = number_range(1, 6);

        if (iteration > 3) {
            victim = NULL;
            break;
        }

        for (i=0; i < mob_skip; i++) {
            victim = victim->next;

            if (victim == NULL) {
                questfound = FALSE;
                break;
            }
        }

        // We reached the end of the list... start over again.
        if (questfound == FALSE) {
            victim = char_list;
            iteration++;
            questfound = TRUE;
            continue;
        }

        // Check each potential target against the following criteria:
        if (!IS_NPC(victim)
                || IS_AFFECTED(victim, AFF_CHARM)
                || victim->in_room == NULL
                || victim->level - ch->level > 5
                || victim->level - ch->level < -12
                || (IS_NPC(victim) && IS_SET(victim->act, ACT_TRAIN))
                || (IS_NPC(victim) && IS_SET(victim->act, ACT_PRACTICE))
                || (IS_NPC(victim) && IS_SET(victim->act, ACT_IS_HEALER))
                || (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
                || (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
                || victim->in_room->area->security < 9
                || !victim->in_room->area->questable
                || ch->in_room->area->continent != victim->in_room->area->continent
                || number_percent() < 30) /* don't always pick the first valid we find. */
        {
            // Target rejected, keep searching.
            continue;
        } else {
            // Target accepted, proceed with quest generation.
            break;
        }
    }

    // This should no longer occur.
    if (victim == NULL || victim->in_room == NULL) {
        if (ch->in_room->area->continent == CONTINENT_TERRA) {
            sprintf(buf, "The people of Midgaard do not require your services right now.");
        } else {
            sprintf(buf, "King Lawrygold does not require your services at the moment.");
        }

        do_say(questman, buf);
        sprintf(buf, "Thanks for the offer, come back soon.");
        do_say(questman, buf);
        ch->pcdata->quest.timer = 3;
        return;
    }

    room = victim->in_room;

    if (quest_type == QUEST_TYPE_ITEM) {
        int objvnum = 0;

        // Decide which token they have to rescue
        switch (number_range(0, 4)) {
            case 0: objvnum = QUEST_OBJQUEST1; break;
            case 1: objvnum = QUEST_OBJQUEST2; break;
            case 2: objvnum = QUEST_OBJQUEST3; break;
            case 3: objvnum = QUEST_OBJQUEST4; break;
            case 4: objvnum = QUEST_OBJQUEST5; break;
        }

        // Place the token.
        questitem = create_object(get_obj_index(objvnum), ch->level);
        obj_to_room(questitem, room);

        // Set up the quest
        SET_BIT(ch->act, PLR_QUESTING);
        ch->pcdata->quest.giver = questman;
        ch->pcdata->quest.timer = number_range(8, 12);
        ch->pcdata->quest.type = QUEST_TYPE_ITEM;
        ch->pcdata->quest.target = questitem->pIndexData->vnum;
        ch->pcdata->quest.progress = 0;
        ch->pcdata->quest.destination = room;

        quest_story_item(questman, victim, questitem);
        return;
    }

    if (quest_type == QUEST_TYPE_VILLAIN) {
        // The villain quest requires no special set up on the mob.

        // Set up the quest for the quester.
        SET_BIT(ch->act, PLR_QUESTING);
        ch->pcdata->quest.giver = questman;
        ch->pcdata->quest.timer = number_range(8, 12);
        ch->pcdata->quest.type = QUEST_TYPE_VILLAIN;
        ch->pcdata->quest.target = victim->pIndexData->vnum;
        ch->pcdata->quest.progress = 0;
        ch->pcdata->quest.destination = room;

        quest_story_villain(questman, victim);
        return;
    }

    if (quest_type == QUEST_TYPE_ASSAULT) {
        CHAR_DATA *rch;
        int group_size = 0;
        int group_timer = number_range(15, 20);
        int assault_size;
        int index;
        int top_level = ch->level;

        // find which mob is the base type for the quest
        if (ch->in_room->area->continent == TERRA) {
            // First half of list for terra.
            index = number_range(0, (MAX_ASSAULT_MOBS / 2) - 1);
        } else {
            // Second half of list for dominia.
            index = number_range( (MAX_ASSAULT_MOBS / 2), MAX_ASSAULT_MOBS - 1);
        }

        for (rch = ch->in_room->people;rch != NULL; rch = rch->next_in_room) {
            // Assign the quest to all groupmates
            if (!IS_NPC(rch)
                    && is_same_group(ch, rch)
                    && !IS_QUESTING(rch)
                    && rch->pcdata->quest.timer == 0) {
                group_size++;

                if (rch->level > top_level) {
                    top_level = rch->level;
                }

                SET_BIT(rch->act, PLR_QUESTING);
                rch->pcdata->quest.giver = questman;
                rch->pcdata->quest.timer = group_timer;
                rch->pcdata->quest.type = QUEST_TYPE_ASSAULT;
                rch->pcdata->quest.target = assault_mobs[index].vnum;
                rch->pcdata->quest.progress = 0;
                rch->pcdata->quest.destination = room;
            }
        }

        // Create the assault
        assault_size = 3 + (2 * group_size);

        for (i = 0; i < assault_size; i++) {
            generate_assault_mob(ch, top_level, assault_mobs[index].race_type, room, assault_size);
        }

        // Now reveal the story
        quest_story_assault(questman, victim, group_size, index);
    }

    return;
}


/* Called from update_handler() by pulse_area */

void
quest_update(void)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch;

	for (d = descriptor_list; d != NULL; d = d->next)
	{
		ch = d->character;

		if(ch == NULL)
			continue;

		if(IS_NPC(ch))
			continue;

		if (ch->pcdata->quest.timer > 0)
		{
			ch->pcdata->quest.timer--;

			if (ch->pcdata->quest.timer == 0)
			{
				if(IS_QUESTING(ch)) {
					Cprintf(ch, "{RYou have failed in your quest...{x\n\r");
					REMOVE_BIT(ch->act, PLR_QUESTING);
					ch->pcdata->quest.type = QUEST_TYPE_NONE;
					ch->pcdata->quest.progress = 0;
				}
				else
					Cprintf(ch, "{RYou may now quest again.{x\n\r");
			}
			else if (IS_QUESTING(ch) && ch->pcdata->quest.timer < 3)
			{
				Cprintf(ch, "Better hurry, you're almost out of time for your quest!\n\r");
			}
		}
	}
	return;
}

void test_port_promote(CHAR_DATA *ch) {
	char buf[255];

	Cprintf(ch, "Activating test port command: PROMOTE\n\r");
	ch->trust = 51;
	sprintf(buf, "%s 51", ch->name);
	do_advance(ch, buf);
	sprintf(buf, "char %s quest 10000", ch->name);
	do_set(ch, buf);
	sprintf(buf, "%s", ch->name);
	do_maxstats(ch, buf);
	Cprintf(ch,"Promote complete.\n\r");
}

void test_port_gearmelee(CHAR_DATA *ch) {
	OBJ_DATA *obj;

	Cprintf(ch, "Activating test port command: GEARMELEE\n\r");
		obj = create_object(get_obj_index(10911), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(2224), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(2224), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(11709), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(11709), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(31153), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(6733), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(11096), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(3811), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(8311), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(10903), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(6758), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(6758), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(1838), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(19350), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(8312), 0);
        obj_to_char(obj, ch);
	obj = create_object(get_obj_index(1202), 0);
	obj_to_char(obj, ch);         
	obj = create_object(get_obj_index(2454), 0);
        obj_to_char(obj, ch);         
	obj = create_object(get_obj_index(10910), 0);
        obj_to_char(obj, ch);         
	obj = create_object(get_obj_index(18955), 0);
        obj_to_char(obj, ch);         
	obj = create_object(get_obj_index(6474), 0);
	obj_to_char(obj, ch);        
	obj = create_object(get_obj_index(11815), 0);
	obj_to_char(obj, ch);    
      	obj = create_object(get_obj_index(6776), 0);
	obj_to_char(obj, ch);     
      	obj = create_object(get_obj_index(3809), 0);
	obj_to_char(obj, ch);        
      	obj = create_object(get_obj_index(3809), 0);
	obj_to_char(obj, ch);        
      	obj = create_object(get_obj_index(6754), 0);
	obj_to_char(obj, ch);    
      	obj = create_object(get_obj_index(6470), 0);
	obj_to_char(obj, ch);        
      	obj = create_object(get_obj_index(6756), 0);
	obj_to_char(obj, ch);        
      	obj = create_object(get_obj_index(10914), 0);
	obj_to_char(obj, ch);       
      	obj = create_object(get_obj_index(3808), 0);
	obj_to_char(obj, ch);         
 	obj = create_object(get_obj_index(2982), 0);
  	obj_to_char(obj, ch);
	obj = create_object(get_obj_index(2980), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(2977), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(2961), 0);
        obj_to_char(obj, ch);
	ch->gold += 1000;

	Cprintf(ch, "Check your inventory.\n\r");
}

void test_port_gearmagic(CHAR_DATA *ch) {
	OBJ_DATA *obj;

        Cprintf(ch, "Activating test port command: GEARMAGIC\n\r");
		  obj = create_object(get_obj_index(17631), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(6757), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(6757), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(11709), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(11709), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(31153), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(6764), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(6734), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(31152), 0);
        obj_to_char(obj, ch);     
        obj = create_object(get_obj_index(11096), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(11096), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(11406), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(6758), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(6758), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(10459), 0);
        obj_to_char(obj, ch);                              
	  obj = create_object(get_obj_index(1202), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(2454), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(10910), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(18955), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(6474), 0);
        obj_to_char(obj, ch);
        obj = create_object(get_obj_index(11815), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(13427), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(6471), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(4828), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(11635), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(1826), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(502), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(502), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(11731), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(14674), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(6765), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(6766), 0);
        obj_to_char(obj, ch);        
        obj = create_object(get_obj_index(13507), 0);
        obj_to_char(obj, ch);        
	ch->gold += 1000;

        Cprintf(ch, "Check your inventory.\n\r");
}

void test_port_restore(CHAR_DATA *ch) {
	Cprintf(ch, "Activating test port command: RESTORE\n\r");
	restore_char(ch);
	Cprintf(ch, "Restored.\n\r");
}
