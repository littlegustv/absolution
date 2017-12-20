/* cool, stats for RD */

#ifndef STATS_H
#define STATS_H

#define MAX_SAMPLES 400
#define MAX_STATISTICS 25
#define STAT_FILE "stats.txt"

#define STAT_TYPE_AVERAGE 1
#define STAT_TYPE_COUNT 2
#define STAT_TYPE_FREQ 3

/* basic stat structure, for int's only */
struct statistic_type
{
	char* name;
	int value[MAX_SAMPLES];
	int insert;
	int type;
};

extern struct statistic_type statistic_table[MAX_STATISTICS];

/* stat funcs */

/* returns index into stat table */
int stat_lookup(char* name);
/* returns average of stat */
float stat_average(char* name);
char* stat_count(char* name);
char* stat_freq(char* name);
/* adds a stat to the list, bumps old values */
void stat_add(char* name, int value);
/* display stats to a char */
void do_stats(CHAR_DATA* ch, char* argument);
/* read stat from file */
void stat_init();
/* write stats to file */
void stat_save();

#endif

