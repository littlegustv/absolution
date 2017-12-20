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
#include "merc.h"

DECLARE_DO_FUN(do_say);

#define TERRA 0
#define DOMINIA 1

#define OBJ_VNUM_SELINA_BOOTS 10910
#define OBJ_VNUM_SELINA_FIRE  10911
#define OBJ_VNUM_SELINA_MIGHT 10912
#define OBJ_VNUM_SELINA_WING  10913
#define OBJ_VNUM_SELINA_BELT  10914

QUEST_ITEM itemList[] =
{
	{8314, TERRA, "cloak", "The cloak of Bosco", 3000},
	{8311, TERRA, "shield", "Shield of Bosco", 2000},
	{2224, TERRA, "ring", "The Diamond Ring", 1500},
	{8312, TERRA, "dagger", "The Bosco dagger", 1000},
	{8310, TERRA, "amulet", "Amulet of Bosco", 1000},
	{11696, TERRA, "blanket", "The Blanket of Regeneration", 1000},
	{8313, TERRA, "stone", "The stone of Bosco", 850},

	{10910, DOMINIA, "boots", "Selina's Swift Boots", 3000},
	{10911, DOMINIA, "fire", "Selina's Fire", 2500},
	{10913, DOMINIA, "might", "Selina's Might", 1500},
	{10914, DOMINIA, "wings", "Selina's Wings", 2000},
	{10912, DOMINIA, "belt", "Selina's Belt", 1000},

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

/* external declerations */
extern int double_qp_ticks;

/* Local functions */
void generate_quest(CHAR_DATA * ch, CHAR_DATA * questman);
void quest_update();
bool chance(int num);
ROOM_INDEX_DATA *find_location(CHAR_DATA * ch, char *arg);


/* CHANCE function. I use this everywhere in my code, very handy :> */
bool
chance(int num)
{
	return ((number_range(1, 100)) <= num) ? TRUE : FALSE;
}

/* The main quest function */

void
do_quest(CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *questman;
	OBJ_DATA *obj = NULL, *obj_next;
	OBJ_INDEX_DATA *questinfoobj;
	MOB_INDEX_DATA *questinfo;
	char buf[MAX_STRING_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	int i;
	bool found = FALSE;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (!strcmp(arg1, "info"))
	{
		if (IS_SET(ch->act, PLR_QUESTOR))
		{
			if (ch->questmob == -1 && ch->questgiver->short_descr != NULL)
				Cprintf(ch, "Your quest is ALMOST complete!\n\rGet back to %s before your time runs out!\n\r", ch->questgiver->short_descr);
			else if (ch->questobj > 0)
			{
				questinfoobj = get_obj_index(ch->questobj);

				if (questinfoobj != NULL) {
					Cprintf(ch, "You are on a quest to recover the fabled %s!\n\r", questinfoobj->name);
					Cprintf(ch, "According to rumour, it can be found in %s.\n\r", ch->questroom);
				}
				else
					Cprintf(ch, "You aren't currently on a quest.\n\r");
			}
			else if (ch->questmob > 0)
			{
				questinfo = get_mob_index(ch->questmob);

				if (questinfo != NULL) {
					Cprintf(ch, "You are on a quest to slay the dreaded %s!\n\r", questinfo->short_descr);
					Cprintf(ch, "According to rumour, they can be found in %s.\n\r", ch->questroom);
				}
				else
					Cprintf(ch, "You aren't currently on a quest.\n\r");
			}
		}
		else
			Cprintf(ch, "You aren't currently on a quest.\n\r");

		return;
	}
	else if (!strcmp(arg1, "points"))
	{
		Cprintf(ch, "You have %d quest points.\n\r", ch->questpoints);
		return;
	}
	else if (!strcmp(arg1, "time"))
	{
		if (!IS_SET(ch->act, PLR_QUESTOR))
		{
			Cprintf(ch, "You aren't currently on a quest.\n\r");

			if (ch->nextquest > 1)
				Cprintf(ch, "There are %d minutes remaining until you can go on another quest.\n\r", ch->nextquest);
			else if (ch->nextquest == 1)
				Cprintf(ch, "There is less than a minute remaining until you can go on another quest.\n\r");
			else
				Cprintf(ch, "{RYou can now quest again!{x\n\r");
		}
		else if (ch->countdown > 0)
			Cprintf(ch, "Time left for current quest: %d\n\r", ch->countdown);

		return;
	}

	/* Checks for a character in the room with spec_questmaster set. This special
	 * procedure must be defined in special.c. You could instead use an
	 * ACT_QUESTMASTER flag instead of a special procedure.
	 */

	for (questman = ch->in_room->people; questman != NULL; questman = questman->next_in_room)
	{
		if (!IS_NPC(questman))
			continue;

		if (questman->spec_fun == spec_lookup("spec_questmaster"))
			break;
	}

	if (questman == NULL || questman->spec_fun != spec_lookup("spec_questmaster"))
	{
		Cprintf(ch, "You can't do that here.\n\r");
		return;
	}

	if (questman->fighting != NULL)
	{
		Cprintf(ch, "Wait until the fighting stops.\n\r");
		return;
	}

	ch->questgiver = questman;

	/* And, of course, you will need to change the following lines for YOUR
	 * quest item information. Quest items on Moongate are unbalanced, very
	 * very nice items, and no one has one yet, because it takes awhile to
	 * build up quest points :> Make the item worth their while.
	 */

	if (!strcmp(arg1, "list"))
	{
		act("$n asks $N for a list of quest items.", ch, NULL, questman, TO_ROOM, POS_RESTING);
		act("You ask $N for a list of quest items.", ch, NULL, questman, TO_CHAR, POS_RESTING);

		Cprintf(ch, "Current Quest Items available for Purchase:\n\r");

		for (i = 0; itemList[i].keyword; i++)
			if (ch->in_room->area->continent == itemList[i].continent)
				Cprintf(ch, "%-4dqp ........ %s\n\r", itemList[i].requiredQP, itemList[i].desc);

		return;
	}
	else if (!str_prefix(arg1, "buy"))
	{
		if (arg2[0] == '\0')
		{
			Cprintf(ch, "To buy an item, type 'QUEST BUY <item>'.\n\r");
			return;
		}

		for (i = 0; itemList[i].keyword; i++)
		{
			if (ch->in_room->area->continent != itemList[i].continent)
				continue;

			if (is_name(arg2, itemList[i].keyword))
			{
				found = TRUE;

				if (ch->questpoints >= itemList[i].requiredQP)
				{
					ch->questpoints -= itemList[i].requiredQP;
					obj = create_object(get_obj_index(itemList[i].vnum), ch->level);
					if(ch->clan > 0)
						obj->clan_status = CS_CLANNER;
					else
						obj->clan_status = CS_NONCLANNER;
					obj->respawn_owner = str_dup(ch->name);
				}
				else
				{
					sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.", ch->name);
					do_say(questman, buf);
					return;
				}
			}
		}

		if (!found)
		{
			sprintf(buf, "I don't have that item, %s.", ch->name);
			do_say(questman, buf);
			return;
		}

		act("$N gives $p to $n.", ch, obj, questman, TO_ROOM, POS_RESTING);
		act("$N gives you $p.", ch, obj, questman, TO_CHAR, POS_RESTING);
		obj_to_char(obj, ch);

		return;
	}
	else if (!str_prefix(arg1, "request"))
	{
		act("$n asks $N for a quest.", ch, NULL, questman, TO_ROOM, POS_RESTING);
		act("You ask $N for a quest.", ch, NULL, questman, TO_CHAR, POS_RESTING);

		if (IS_SET(ch->act, PLR_QUESTOR))
		{
			sprintf(buf, "But you're already on a quest!");
			do_say(questman, buf);
			return;
		}

		if (ch->nextquest > 0)
		{
			sprintf(buf, "You're very brave, %s, but let someone else have a chance.", ch->name);
			do_say(questman, buf);
			sprintf(buf, "Come back later.");
			do_say(questman, buf);
			return;
		}

		sprintf(buf, "Thank you, brave %s!", ch->name);
		do_say(questman, buf);

		generate_quest(ch, questman);

		if (ch->questmob > 0 || ch->questobj > 0)
		{
			ch->countdown = number_range(10, 30);
			SET_BIT(ch->act, PLR_QUESTOR);
			sprintf(buf, "You have %d minutes to complete this quest.", ch->countdown);
			do_say(questman, buf);
		}

		return;
	}
	else if (!str_prefix(arg1, "reset"))
	{
		if (IS_SET(ch->act, PLR_QUESTOR))
			REMOVE_BIT(ch->act, PLR_QUESTOR);
		ch->questgiver = NULL;
		ch->nextquest = 10;
		ch->countdown = 0;
		ch->questobj = 0;
		ch->questmob = 0;
		free_string(ch->questroom);

		Cprintf(ch, "Quest reset.  You will be able to quest again in 10 minutes.\n\r");

		return;
	}
	else if (!str_prefix(arg1, "complete"))
	{
		act("$n informs $N $e has completed $s quest.", ch, NULL, questman, TO_ROOM, POS_RESTING);
		act("You inform $N you have completed $s quest.", ch, NULL, questman, TO_CHAR, POS_RESTING);

		if (ch->questgiver != questman)
		{
			sprintf(buf, "I never sent you on a quest!  Perhaps you're thinking of someone else.");
			do_say(questman, buf);
			return;
		}

		if (IS_SET(ch->act, PLR_QUESTOR))
		{
			if (ch->questmob == -1 && ch->countdown > 0)
			{
				int reward, pointreward;

				reward = number_range(ch->level - 10, ch->level + 10);

				if (reward < 1)
					reward = 1;

				pointreward = number_range(25, 40);

				if (double_qp_ticks > 0)
					pointreward = pointreward * 2;


				sprintf(buf, "Congratulations on completing your quest!");
				do_say(questman, buf);

				if (ch->level > 50)
					sprintf(buf, "As a reward, I am giving you %d quest points, and %d gold.", pointreward, reward);
				else
					sprintf(buf, "As a reward, I am giving you %d gold.", reward);

				do_say(questman, buf);

				REMOVE_BIT(ch->act, PLR_QUESTOR);
				ch->questgiver = NULL;
				ch->countdown = 0;
				ch->questmob = 0;
				ch->questobj = 0;
				ch->nextquest = 10;
				ch->gold += reward;
				if (ch->level > 50)
					ch->questpoints += pointreward;

				return;
			}
			else if (ch->questobj > 0 && ch->countdown > 0)
			{
				bool obj_found = FALSE;

				for (obj = ch->carrying; obj != NULL; obj = obj_next)
				{
					obj_next = obj->next_content;

					if (obj != NULL && obj->pIndexData->vnum == ch->questobj)
					{
						obj_found = TRUE;
						break;
					}
				}

				if (obj_found == TRUE)
				{
					int reward, pointreward;

					reward = number_range(ch->level - 10, ch->level + 10);

					if (reward < 1)
						reward = 1;
					pointreward = number_range(25, 40);

					if (double_qp_ticks > 0)
						pointreward = pointreward * 2;


					act("You hand $p to $N.", ch, obj, questman, TO_CHAR, POS_RESTING);
					act("$n hands $p to $N.", ch, obj, questman, TO_ROOM, POS_RESTING);

					sprintf(buf, "Congratulations on completing your quest!");
					do_say(questman, buf);

					if (ch->level > 50)
						sprintf(buf, "As a reward, I am giving you %d quest points, and %d gold.", pointreward, reward);
					else
						sprintf(buf, "As a reward, I am giving you %d gold.", reward);

					do_say(questman, buf);

					REMOVE_BIT(ch->act, PLR_QUESTOR);
					ch->questgiver = NULL;
					ch->countdown = 0;
					ch->questmob = 0;
					ch->questobj = 0;
					ch->nextquest = 10;
					ch->gold += reward;

					if (ch->level > 50)
						ch->questpoints += pointreward;

					extract_obj(obj);
					return;
				}
				else
				{
					sprintf(buf, "You haven't completed the quest yet, but there is still time!");
					do_say(questman, buf);
					return;
				}

				return;
			}
			else if ((ch->questmob > 0 || ch->questobj > 0) && ch->countdown > 0)
			{
				sprintf(buf, "You haven't completed the quest yet, but there is still time!");
				do_say(questman, buf);
				return;
			}
		}

		if (ch->nextquest > 0)
			sprintf(buf, "But you didn't complete your quest in time!");
		else
			sprintf(buf, "You have to REQUEST a quest first, %s.", ch->name);

		do_say(questman, buf);
		return;
	}

	Cprintf(ch, "QUEST commands: POINTS INFO TIME REQUEST COMPLETE LIST BUY RESET.\n\r");
	Cprintf(ch, "For more information, type 'HELP QUEST'.\n\r");
	return;
}

/* Redone by Starcrossed, May 25, Y2K... WAAAAAY overdue. */

void
generate_quest(CHAR_DATA *ch, CHAR_DATA *questman)
{
        CHAR_DATA *victim;
        ROOM_INDEX_DATA *room;
        OBJ_DATA *questitem;
        char buf[MAX_STRING_LENGTH];
        int i, mob_skip, questfound=TRUE;

	/* Get me a random mob around their level */
	for (victim = char_list; victim != NULL; victim = victim->next)
        {
		/* skip a few to randomize through areas a bit more */
		if(ch->in_room->area->continent == CONTINENT_TERRA)
                	mob_skip = dice(10, 18);
		else
			mob_skip = dice(10, 12);

                for (i=0; i<mob_skip; i++) {
			victim = victim->next;
                        if (victim == NULL) {
                                questfound = FALSE;
				break;
                        }
                }
		/* run out of mobs */
		if (questfound == FALSE)
			break;

		/* check for bad mob */
		if (!IS_NPC(victim)
                || IS_AFFECTED(victim, AFF_CHARM)
                || victim->in_room == NULL
		|| victim->level - ch->level > 10
		|| victim->level - ch->level < -14
		|| (IS_NPC(victim) && IS_SET(victim->act, ACT_TRAIN))
		|| (IS_NPC(victim) && IS_SET(victim->act, ACT_PRACTICE))
		|| (IS_NPC(victim) && IS_SET(victim->act, ACT_IS_HEALER))
                || (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
                || (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
                || victim->in_room->area->security < 9
		|| ch->in_room->area->continent != victim->in_room->area->continent
		|| number_percent() < 40)
                {
                        continue;
                }
		else
			break;
	}

	/* do we have a victim? */
	if(questfound == FALSE || victim == NULL || victim->in_room == NULL)
	{
		if(ch->in_room->area->continent == CONTINENT_TERRA)
			sprintf(buf, "The people of Midgaard do not require your services right now.");
                else
			sprintf(buf, "King Lawrygold does not require your services at the moment.");
		do_say(questman, buf);
                sprintf(buf, "Thanks for the offer, come back soon.");
                do_say(questman, buf);
                ch->nextquest = 3;
                return;
        }

	room = victim->in_room;
	ch->questroom = str_dup(room->name);

	/* sometimes we have an item quest */
	if(number_percent() < 40) {
		int objvnum = 0;

                switch (number_range(0, 4)) {
                    case 0: objvnum = QUEST_OBJQUEST1; break;
                    case 1: objvnum = QUEST_OBJQUEST2; break;
                    case 2: objvnum = QUEST_OBJQUEST3; break;
		    case 3: objvnum = QUEST_OBJQUEST4; break;
                    case 4: objvnum = QUEST_OBJQUEST5; break;
                }

                questitem = create_object(get_obj_index(objvnum), ch->level);
                obj_to_room(questitem, room);
                ch->questobj = questitem->pIndexData->vnum;

		/* generate amusing message */
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

	/* Quest to kill a mob */
	/* Make an amusing tale */
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

	ch->questmob = victim->pIndexData->vnum;

        return;
}


/* Called from update_handler() by pulse_area */

void
quest_update(void)
{
	CHAR_DATA *ch, *ch_next;

	for (ch = char_list; ch != NULL; ch = ch_next)
	{
		ch_next = ch->next;

		if (IS_NPC(ch))
			continue;

		if (ch->nextquest > 0)
		{
			ch->nextquest--;

			if (ch->nextquest == 0)
			{
				Cprintf(ch, "{RYou may now quest again.{x\n\r");
				return;
			}
		}
		else if (IS_SET(ch->act, PLR_QUESTOR))
		{
			if (--ch->countdown <= 0)
			{
				ch->nextquest = 10;
				Cprintf(ch, "You have run out of time for your quest!\n\rYou may quest again in %d minutes.\n\r", ch->nextquest);

				REMOVE_BIT(ch->act, PLR_QUESTOR);
				ch->questgiver = NULL;
				ch->countdown = 0;
				ch->questmob = 0;
			}
			if (ch->countdown > 0 && ch->countdown < 6)
			{
				Cprintf(ch, "Better hurry, you're almost out of time for your quest!\n\r");
				return;
			}
		}
	}
	return;
}
