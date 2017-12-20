#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "utils.h"
#include "stats.h"

struct statistic_type statistic_table[MAX_STATISTICS] =
{
	{"av_hit", {0}, 0, STAT_TYPE_COUNT},
	{"av_dam_weapon", {0}, 0, STAT_TYPE_AVERAGE},
	{"av_dam_spell", {0}, 0, STAT_TYPE_AVERAGE},
	{"av_dam_spellvuln", {0}, 0, STAT_TYPE_AVERAGE},
	{"av_dam_dslayer", {0}, 0, STAT_TYPE_AVERAGE},
	{"av_dam_iron", {0}, 0, STAT_TYPE_AVERAGE},
	{"av_dam_weapvuln", {0}, 0, STAT_TYPE_AVERAGE},
	{"behead_frequency", {0}, 0, STAT_TYPE_FREQ},
	{"av_behead_damage", {0}, 0, STAT_TYPE_AVERAGE},
	{"tail_frequency", {0}, 0, STAT_TYPE_FREQ}, /* 10 */
	{"av_tail_damage", {0}, 0, STAT_TYPE_AVERAGE},
	{"snake_bite_frequency", {0}, 0, STAT_TYPE_FREQ},
	{"av_snake_bite_damage", {0}, 0, STAT_TYPE_AVERAGE},
	{"thrust_frequency", {0}, 0, STAT_TYPE_FREQ},
	{"av_thrust_damage", {0}, 0, STAT_TYPE_AVERAGE}, /* 15 */
	{"bite_frequency", {0}, 0, STAT_TYPE_FREQ},
	{"av_bite_damage", {0}, 0, STAT_TYPE_AVERAGE},
	{"gore_frequency", {0}, 0, STAT_TYPE_FREQ},
	{"av_gore_damage", {0}, 0, STAT_TYPE_AVERAGE},

	{NULL}
};

int stat_lookup(char* name)
{
	int i;

	for (i = 0; i < MAX_STATISTICS; i++)
	{
		if (statistic_table[i].name == NULL ||
			statistic_table[i].name[0] == '\0')
		{
			break;
		}

		if (!str_cmp(statistic_table[i].name, name))
		{
			return i;
		}
	}

	return -1;
}

float stat_average(char* name)
{
	int i;
	int j;
	int total;
	int top;

	i = stat_lookup(name);

	if (i == -1)
	{
		return 0;
	}

	total = 0;
	top = 2;
	for (j = 0; j < MAX_SAMPLES; j++)
	{
		if (statistic_table[i].value[j] != 0)
			top = j;
	}

	for (j = 0; j < top + 1; j++)
	{
		total += statistic_table[i].value[j];
	}

	return (float)total / top + 1;
}

char* stat_count(char* name)
{
	int i;
	int j;
	static char buf[MAX_STRING_LENGTH];
	int ar[2];

	i = stat_lookup(name);

	if (i == -1)
	{
		return 0;
	}

	buf[0] = '\0';
	memset(ar, 0, sizeof(int) * 2);
	for (j = 0; j < MAX_SAMPLES; j++)
	{
		ar[statistic_table[i].value[j]]++;
	}

	for (i = 0; i < 2; i++)
	{
		sprintf(buf, "%s %s %%%f(%d)", buf,
			i == 0 ? "miss" : "hit" ,
			((float)ar[i] / MAX_SAMPLES) * 100,
			ar[i]);
	}

	return buf;
}

char* stat_freq(char* name)
{
	int i;
	int j;
	static char buf[MAX_STRING_LENGTH];
	int ar[2];

	i = stat_lookup(name);

	if (i == -1)
	{
		return 0;
	}

	buf[0] = '\0';
	memset(ar, 0, sizeof(int) * 2);
	for (j = 0; j < MAX_SAMPLES; j++)
	{
		ar[statistic_table[i].value[j]]++;
	}

	for (i = 0; i < 2; i++)
	{
		sprintf(buf, "%s %s %d", buf,
			i == 0 ? "miss" : "hit" ,
			ar[i]);
	}

	return buf;
}

void stat_add(char* name, int value)
{
	int i;

	i = stat_lookup(name);

	if (i == -1)
	{
		return;
	}

	statistic_table[i].value[statistic_table[i].insert++] = value;
	if (statistic_table[i].insert >= MAX_SAMPLES)
	{
		statistic_table[i].insert = 0;
	}
}

void do_stats(CHAR_DATA* ch, char* argument)
{
	bool all;
	int i;

	all = FALSE;

	if (argument[0] == '\0' ||
		!str_cmp(argument, "all"))
	{
		all = TRUE;
	}

	Cprintf(ch, "Statistic           Average/Count\n\r");
	Cprintf(ch, "---------------------------------\n\r");

	for (i = 0; i < MAX_STATISTICS; i++)
	{
		if (statistic_table[i].name == NULL ||
			statistic_table[i].name[0] == '\0')
		{
			break;
		}

		if (!str_prefix(argument, statistic_table[i].name) ||
			all == TRUE)
		{
			if (statistic_table[i].type == STAT_TYPE_AVERAGE)
			{
				Cprintf(ch, "%-25s%f\n\r",
					statistic_table[i].name,
					stat_average(statistic_table[i].name));
			}
			else if (statistic_table[i].type == STAT_TYPE_COUNT)
			{
				Cprintf(ch, "%-25s%-s\n\r",
					statistic_table[i].name,
					stat_count(statistic_table[i].name));
			}
			else if (statistic_table[i].type == STAT_TYPE_FREQ)
			{
				Cprintf(ch, "%-25s%-s\n\r",
					statistic_table[i].name,
					stat_freq(statistic_table[i].name));
			}
		}
	}
}

void stat_init()
{
	FILE *fp;
	int i;
	int j;
	char* word;

	fclose(fpReserve);

	if ((fp = fopen(STAT_FILE, "r")) == NULL)
	{
		bug("Error reading stat file.'", 0);
		return;
	}

	while (TRUE)
	{
		word = fread_string(fp);
		if (!str_cmp(word, "END") || feof(fp))
			break;

		i = stat_lookup(word);
		if (i != -1)
		{
			statistic_table[i].insert = fread_number(fp);

			for (j = 0; j < MAX_SAMPLES; j++)
			{
				statistic_table[i].value[j] = fread_number(fp);
                                bug("Last Stat read: %d ", statistic_table[i].value[j] );
			}
		}
		else
		{
			/* recover, skip next 2 lines */
			fread_to_eol(fp);
			fread_to_eol(fp);
		}
	}
 bug("stat file exited normally.'", 0);

	fclose(fp);

	fpReserve = fopen(NULL_FILE, "r");
}

void stat_save()
{
	FILE *fp;
	int i;
	int j;

	fclose(fpReserve);

	if ((fp = fopen(TEMP_FILE, "w")) == NULL)
	{
		bug("Save_stats: fopen", 0);
		return;
	}

	for (i = 0; i < MAX_STATISTICS; i++)
	{
		if (statistic_table[i].name == NULL ||
			statistic_table[i].name[0] == '\0')
		{
			break;
		}

		fprintf(fp, "%s~\n\r", statistic_table[i].name);
		fprintf(fp, "%d\n\r", statistic_table[i].insert);

		for (j = 0; j < MAX_SAMPLES; j++)
		{
			fprintf(fp, "%d ", statistic_table[i].value[j]);
		}
		fprintf(fp, "\n\r");
	}

	fprintf(fp, "END~\n\r");

	fclose(fp);
	rename(TEMP_FILE, STAT_FILE);

	fpReserve = fopen(NULL_FILE, "r");
	return;
}
