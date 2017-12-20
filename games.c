/*
   01234567890123456789012345678901234567890123456789012345678901234567890123456789
   Game Code v2 for ROM based muds. Robert Schultz, Sembiance  -  bert@ncinter.net
   Snippets of mine can be found at http://www.ncinter.net/~bert/mud/
   This file (games.c) contains all the game functions.
 */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "merc.h"
#include "recycle.h"
#include "games.h"
#include "utils.h"


void
do_game(CHAR_DATA * ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	int whichGame;

	argument = one_argument(argument, arg1);

	if (arg1[0] == '\0')
	{
		Cprintf(ch, "Type 'help games' for more information on games.\n\r");
		return;
	}

	if (IS_NPC(ch))
	{
		Cprintf(ch, "Sorry, only player characters may play games.\n\r");
		return;
	}

	if (ch->level < 10) {
		Cprintf(ch, "You must be at least level 10 to gamble.\n\r");
		return;
	}

	if(ch->move < 1) {
		Cprintf(ch, "You've gambled for long enough.\n\r");
		return;
	}

	ch->move -= 1;

	if (!str_prefix(arg1, "slot") || !str_prefix(arg1, "slots"))
		whichGame = GAME_SLOTS;
	else if (!str_prefix(arg1, "highdice"))
		whichGame = GAME_HIGH_DICE;
	else
		whichGame = GAME_NONE;

	switch (whichGame)
	{
	case GAME_SLOTS:
		do_slots(ch, argument);
		break;
	case GAME_HIGH_DICE:
		do_high_dice(ch, argument);
		break;
	default:
		Cprintf(ch, "Thats not a game. Type 'help games' for a list.\n\r");
		break;
	}


	return;
}

void
do_slots(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *slotMachine;
	char arg[MAX_INPUT_LENGTH];
	int counter, winArray[11];
	int cost, jackpot, bars, winnings = 0, numberMatched = 0;
	int bar1 = 0, bar2 = 0, bar3 = 0, bar4 = 0, bar5 = 0;
	int limit;
	bool partial, won = FALSE, wonJackpot = FALSE, foundSlot;

	char *bar_messages[] =
	{
		"<------------>",
		"{YGold Coin{x",		/* 1 */
		"{RLock Pick{x",
		"{MSembiance{x",		/* 3 */
		"{cCityguard{x",
		"{CElf Sword{x",		/* 5 */
		"{yAn Orange{x",
		"{rFly Spell{x",
		"{GElemental{x",
		"{WDualWield{x",
		"{BMudSchool{x",		/* 10 */
	};

	argument = one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		Cprintf(ch, "Syntax: game slots <which machine>\n\r");
		return;
	}

	foundSlot = FALSE;

	for (slotMachine = ch->in_room->contents; slotMachine != NULL; slotMachine = slotMachine->next_content)
	{
		if ((slotMachine->item_type == ITEM_SLOT_MACHINE) && (can_see_obj(ch, slotMachine)))
		{
			if (is_name(arg, slotMachine->name))
			{
				foundSlot = TRUE;
				break;
			}
			else
			{
				foundSlot = FALSE;
			}
		}
	}

	if (foundSlot == FALSE)
	{
		Cprintf(ch, "That slot machine is not here.\n\r");
		return;
	}

	bar1 = bar2 = bar3 = bar4 = bar5 = 0;
	// The cost to play in gold
	cost    = slotMachine->value[0];
	// The current jackpot for the machine
	jackpot = slotMachine->value[1];
	// How many bars the machine has 3 or 5 currently
	bars    = slotMachine->value[2];
	// If you can win a partial jackpot for matching some bars
	partial = slotMachine->value[3];
	// The limit after which the jackpot doesn't increase
	limit   = slotMachine->value[4];

	if (cost <= 0)
	{
		Cprintf(ch, "This slot machine seems to be broken.\n\r");
		return;
	}

	if (cost > ch->gold)
	{
		Cprintf(ch, "This slot machine costs %d gold to play.\n\r", cost);
		return;
	}

	ch->gold -= cost;
	slotMachine->value[1] += cost;

	bar1 = number_range(1, 10);
	bar2 = number_range(1, 10);
	bar3 = number_range(1, 10);
	if (bars == 5)
	{
		bar4 = number_range(1, 10);
		bar5 = number_range(1, 10);
	}

	if (bars == 3)
	{
		Cprintf(ch, "{g////------------{MSlot Machine{g------------\\\\\\\\{x\n\r");
		Cprintf(ch, "{g|{C{{}{g|{x  %s  %s  %s  {h|{C{{}{g|{x\n\r", bar_messages[bar1],
				bar_messages[bar2], bar_messages[bar3]);
		Cprintf(ch, "{g\\\\\\\\------------------------------------////{x\n\r");
	}
	else
	{
		Cprintf(ch, "{g////-----------------------{MSlot Machine{g----------------------\\\\\\\\{x\n\r");
		Cprintf(ch, "{g|{C{{}{g|{x  %s  %s  %s  %s  %s  {g|{C{{}{g|{x\n\r", bar_messages[bar1],
				bar_messages[bar2], bar_messages[bar3], bar_messages[bar4], bar_messages[bar5]);
		Cprintf(ch, "{g\\\\\\\\---------------------------------------------------------////{x\n\r");
	}

	// Complete win! They get jackpot and machine is reset.
	if (bars == 3)
	{
		if ((bar1 == bar2) && (bar2 == bar3))
		{
			winnings = jackpot;
			won = TRUE;
			slotMachine->value[1] = cost * 11;
			wonJackpot = TRUE;
		}
	}
	else if (bars == 5)
	{
		if((bar1 == bar2) && (bar2 == bar3)
		&& (bar3 == bar4) && (bar4 == bar5))
		{
			winnings = jackpot;
			won = TRUE;
			slotMachine->value[1] = cost * 11;
			wonJackpot = TRUE;
		}
	}

	// Partial win! Win a prize based on how many bars you matched.
	if (!won && partial)
	{
		// Only possible to match a pair
		if (bars == 3)
		{
			if(bar1 == bar2
			|| bar1 == bar3
			|| bar2 == bar3)
			{
				winnings += cost;
				won = TRUE;
				numberMatched++;
			}

		}
		else if (bars == 5)
		{
			// An array to check for matches
			for (counter = 0; counter < 11; counter++)
        			winArray[counter] = 0;

			winArray[bar1]++;
			winArray[bar2]++;
			winArray[bar3]++;
			winArray[bar4]++;
			winArray[bar5]++;

			for (counter = 0; counter < 11; counter++)
			{
				if (winArray[counter] > 1)
					numberMatched += winArray[counter];
			}

			if (numberMatched > 0)
			{
				winnings += cost * (numberMatched - 1);
				slotMachine->value[1] += cost;
			}

			if (winnings > 0)
				won = TRUE;
		}
	}

	ch->gold += winnings;

	// If there is a "limit" on the prize won, restrict them to it.
	// This is called the "freeze" of the machine.
	if(limit > 0
	&& slotMachine->value[1] > limit)
        	slotMachine->value[1] = limit;

	// And in any case we hardcode a limit of 250 times the cost to play.
	// If it costs 10 to play, machine is fixed at 2500 max.
	if (slotMachine->value[1] > cost * 250)
        	slotMachine->value[1] = cost * 250;


	if (won && wonJackpot)
	{
		Cprintf(ch, "You won the jackpot worth %d gold!! The jackpot now stands at %d gold.\n\r",
				winnings, slotMachine->value[1]);
	}
	if (won && !wonJackpot)
	{
		Cprintf(ch, "You matched %d bars and won %d gold! The jackpot is now worth %d gold.\n\r",
				numberMatched, winnings, slotMachine->value[1]);
	}
	if (!won)
	{
		Cprintf(ch, "Sorry you didn't win anything. The jackpot is now worth %d gold.\n\r",
				slotMachine->value[1]);
	}

	WAIT_STATE(ch, PULSE_VIOLENCE / 2);
	return;
}

void
do_high_dice(CHAR_DATA * ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *dealer;
	int die, dealerDice, playerDice;
	int bet;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0' || !is_number(arg))
	{
		Cprintf(ch, "Syntax is: game highdice <bet>\n\r");
		return;
	}

	bet = atoi(arg);

	if (bet < 10)
	{
		Cprintf(ch, "Minimum bet is 10 gold coins.\n\r");
		return;
	}

	if (bet > 500)
	{
		Cprintf(ch, "Maximum bet is 500 gold coins.\n\r");
		return;
	}

	for (dealer = ch->in_room->people; dealer; dealer = dealer->next_in_room)
	{
		if (IS_NPC(dealer) && IS_SET(dealer->act, ACT_DEALER) && can_see(ch, dealer))
			break;
	}

	if (dealer == NULL)
	{
		Cprintf(ch, "You do not see any dice dealer here.\n\r");
		return;
	}

	if (bet > ch->gold)
	{
		Cprintf(ch, "You can not afford to bet that much!\n\r");
		return;
	}

	dealerDice = 0;
	playerDice = 0;


	die = number_range(1, 6);
	dealerDice += die;
	die = number_range(1, 6);
	dealerDice += die;

	die = number_range(1, 6);
	playerDice += die;
	die = number_range(1, 6);
	playerDice += die;

	Cprintf(ch, "{c%s{g rolled two dice with a total of {W%d!{x\n\r", dealer->short_descr,
			dealerDice);
	Cprintf(ch, "{gYou rolled two dice with a total of {W%d!{x\n\r", playerDice);

	if (dealerDice > playerDice)
	{
		Cprintf(ch, "{RYou lost! {c%s{g takes your bet of {y%d gold{g.{x\n\r",
				dealer->short_descr, bet);
		ch->gold -= bet;
	}

	if (dealerDice < playerDice)
	{
		Cprintf(ch, "{GYou won! {c%s {ggives you your winnings of {y%d gold{g.{x\n\r",
				dealer->short_descr, bet);
		ch->gold += bet;
	}

	if (dealerDice == playerDice)
	{
		Cprintf(ch, "{RYou lost! {gThe dealer always wins in a tie. You lose {y%d gold{g.{x\n\r",
				bet);
		ch->gold -= bet;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE / 2);
	return;
}
