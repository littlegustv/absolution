#ifndef VOTE_H
#define VOTE_H

#include "merc.h"
#include "clan.h"

struct vote_choice
{
	char description[MAX_INPUT_LENGTH];
	int tally;
};

struct vote_session
{
	char creator[20];
	char question[MAX_INPUT_LENGTH];
	struct vote_choice result[5];

	bool restriction[MAX_CLAN];
	int vote_level;
	bool active;
	char voted[2000];
} vote;
#endif
