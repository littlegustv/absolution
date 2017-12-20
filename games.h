/*
Game Code v2 for ROM based muds. Robert Schultz, Sembiance  -  bert@ncinter.net
Snippets of mine can be found at http://www.ncinter.net/~bert/mud/
This file (games.h) is the header file for games.c
*/

#ifndef GAMES_H
#define GAMES_H

#define      GAME_NONE       0
#define      GAME_SLOTS      1
#define      GAME_HIGH_DICE  2

void do_slots(CHAR_DATA *ch, char *argument );
void do_high_dice(CHAR_DATA *ch, char *argument);

#endif

