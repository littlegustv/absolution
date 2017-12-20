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
*   ROM 2.4 is copyright 1993-1996 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor (rtaylor@pacinfo.com)                                  *
*       Gabrielle Taylor (gtaylor@pacinfo.com)                             *
*       Brian Moore (rom@rom.efn.org)                                      *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
*   Code updated by; Gothic, to be singing bards.                          *
*                    NOTE: The jukes in Midgaard.are mud be renamed        *
***************************************************************************/

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "merc.h"
#include "music.h"
#include "recycle.h"
#include "utils.h"


int channel_songs[MAX_GLOBAL + 1];
struct song_data song_table[MAX_SONGS];

void
song_update(void)
{
	OBJ_DATA *obj;
	CHAR_DATA *victim;
	ROOM_INDEX_DATA *room;
	DESCRIPTOR_DATA *d;
	char buf[MAX_STRING_LENGTH];
	char *line;
	int i;

	/* do the global song, if any */
	if (channel_songs[1] >= MAX_SONGS)
		channel_songs[1] = -1;

	if (channel_songs[1] > -1)
	{
		if (channel_songs[0] >= MAX_LINES
			|| channel_songs[0] >= song_table[channel_songs[1]].lines)
		{
			channel_songs[0] = -1;

			/* advance songs */
			for (i = 1; i < MAX_GLOBAL; i++)
				channel_songs[i] = channel_songs[i + 1];
			channel_songs[MAX_GLOBAL] = -1;
		}
		else
		{
			if (channel_songs[0] < 0)
			{
				sprintf(buf, "A bard MUSIC: %s, %s",
						song_table[channel_songs[1]].group,
						song_table[channel_songs[1]].name);
				channel_songs[0] = 0;
			}
			else
			{
				sprintf(buf, "A bard MUSIC: '%s'",
					 song_table[channel_songs[1]].lyrics[channel_songs[0]]);
				channel_songs[0]++;
			}

			for (d = descriptor_list; d != NULL; d = d->next)
			{
				victim = d->original ? d->original : d->character;

				if (d->connected == CON_PLAYING &&
					!IS_SET(victim->comm, COMM_NOMUSIC) &&
					!IS_SET(victim->comm, COMM_QUIET))
					act("$t", d->character, buf, NULL, TO_CHAR, POS_SLEEPING);
			}
		}
	}

	for (obj = object_list; obj != NULL; obj = obj->next)
	{
		if (obj->item_type != ITEM_JUKEBOX || obj->value[1] < 0)
			continue;

		if (obj->value[1] >= MAX_SONGS)
		{
			obj->value[1] = -1;
			continue;
		}

		/* find which room to play in */

		if ((room = obj->in_room) == NULL)
		{
			if (obj->carried_by == NULL)
				continue;
			else if ((room = obj->carried_by->in_room) == NULL)
				continue;
		}

		if (obj->value[0] < 0)
		{
			sprintf(buf, "$p starts playing %s, %s.",
			song_table[obj->value[1]].group, song_table[obj->value[1]].name);
			if (room->people != NULL)
				act(buf, room->people, obj, NULL, TO_ALL, POS_RESTING);
			obj->value[0] = 0;
			continue;
		}
		else
		{
			if (obj->value[0] >= MAX_LINES
				|| obj->value[0] >= song_table[obj->value[1]].lines)
			{

				obj->value[0] = -1;

				/* scroll songs forward */
				obj->value[1] = obj->value[2];
				obj->value[2] = obj->value[3];
				obj->value[3] = obj->value[4];
				obj->value[4] = -1;
				continue;
			}

			line = song_table[obj->value[1]].lyrics[obj->value[0]];
			obj->value[0]++;
		}

		sprintf(buf, "$p MUSIC: '%s'", line);
		if (room->people != NULL)
			act(buf, room->people, obj, NULL, TO_ALL, POS_RESTING);
	}
}



void
load_songs(void)
{
	FILE *fp;
	int count = 0, lines, i;
	char letter;

	/* reset global */
	for (i = 0; i <= MAX_GLOBAL; i++)
		channel_songs[i] = -1;

	if ((fp = fopen(MUSIC_FILE, "r")) == NULL)
	{
		bug("The bard tells you 'I don't feel like singing right now.'", 0);
		fclose(fp);
		return;
	}

	for (count = 0; count < MAX_SONGS; count++)
	{
		letter = fread_letter(fp);
		if (letter == '#')
		{
			if (count < MAX_SONGS)
				song_table[count].name = NULL;
			fclose(fp);
			return;
		}
		else
			ungetc(letter, fp);

		song_table[count].group = fread_string(fp);
		song_table[count].name = fread_string(fp);

		/* read lyrics */
		lines = 0;

		for (;;)
		{
			letter = fread_letter(fp);

			if (letter == '~')
			{
				song_table[count].lines = lines;
				break;
			}
			else
				ungetc(letter, fp);

			if (lines >= MAX_LINES)
			{
				bug("The bard tells you 'That song has many lines in a song.  My limit is  %d.", MAX_LINES);
				break;
			}

			song_table[count].lyrics[lines] = fread_string_eol(fp);
			lines++;
		}
	}
}

void
do_play(CHAR_DATA * ch, char *argument)
{
	OBJ_DATA *juke;
	CHAR_DATA *victim;
	DESCRIPTOR_DATA *d;
	char buf[MAX_STRING_LENGTH];
	char *str, arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int song, i;
	bool global = FALSE;
	bool ded = FALSE;
	int amt = 0;

	str = one_argument(argument, arg);

	for (juke = ch->in_room->contents; juke != NULL; juke = juke->next_content)
	{
		if (juke->item_type == ITEM_JUKEBOX && can_see_obj(ch, juke))
			break;
	}

	if (argument[0] == '\0')
	{
		Cprintf(ch, "{yThe bard says 'Sing what? I believe I missed what you said.'{x\n\r");
		return;
	}

	if (juke == NULL)
	{
		Cprintf(ch, "{yThe bard says 'I can't play that right now.'{x\n\r");
		return;
	}

	if (!str_cmp(arg, "list"))
	{
		BUFFER *buffer;
		int col = 0;
		bool artist = FALSE, match = FALSE;

		buffer = new_buf();
		argument = str;
		argument = one_argument(argument, arg);

		if (!str_cmp(arg, "artist"))
			artist = TRUE;

		if (argument[0] != '\0')
			match = TRUE;

		sprintf(buf, "%s says 'I know the following songs:'\n\r", capitalize(juke->short_descr));
		add_buf(buffer, buf);

		for (i = 0; i < MAX_SONGS; i++)
		{
			if (song_table[i].name == NULL)
				break;

			if (artist && (!match || !str_prefix(argument, song_table[i].group)))
				sprintf(buf, "%-39s %-39s\n\r", song_table[i].group, song_table[i].name);
			else if (!artist)
				sprintf(buf, "%-35s ", song_table[i].name);
			else
				continue;

			add_buf(buffer, buf);
			if (!artist && ++col % 2 == 0)
				add_buf(buffer, "\n\r");
		}

		if (!artist && col % 2 != 0)
			add_buf(buffer, "\n\r");

		page_to_char(buf_string(buffer), ch);
		free_buf(buffer);
		return;
	}

	if (!str_cmp(arg, "loud"))
	{
		if (ch->gold < 30)
		{
			Cprintf(ch, "It Costs 30 Gold to play loud.  You don't have that much.\n\r");
			return;
		}
		else
		{
			Cprintf(ch, "You pay the bard 30 Gold to play loud.\n\r");
			amt = 30;
			argument = str;
			global = TRUE;
		}
	}

	if (!str_cmp(arg, "dedicate"))
	{
		if (ch->gold < 50)
		{
			Cprintf(ch, "It Costs 50 Gold dedicate a song.  You don't have that much.\n\r");
			return;
		}

		argument = one_argument(str, arg2);

		if ((victim = get_char_world(ch, arg2, TRUE)) == NULL)
		{
			Cprintf(ch, "You can't dedicate songs to people who are not here!\n\r");
			return;
		}

		Cprintf(ch, "You pay the bard 50 Gold and dedicate the next song to %s.\n\r", victim->name);
		amt = 50;
		ded = TRUE;
		global = TRUE;
	}

	if (argument[0] == '\0')
	{
		Cprintf(ch, "The bard says 'Want me to sing what? Where?'\n\r");
		return;
	}

	if ((global && channel_songs[MAX_GLOBAL] > -1) ||
            (!global && juke->value[4] > -1))
	{
		Cprintf(ch, "The bard says 'I have all the requests I can remember right now.\n\r");
		return;
	}

	for (song = 0; song < MAX_SONGS; song++)
	{
		if (song_table[song].name == NULL)
		{
			Cprintf(ch, "The bard says 'I don't know that song, sorry.'\n\r");
			return;
		}
		if (!str_prefix(argument, song_table[song].name))
			break;
	}

	if (song >= MAX_SONGS)
	{
		Cprintf(ch, "The bards says 'I don't know that song, sorry.'\n\r");
		return;
	}

	Cprintf(ch, "The bard nods at you, and smiles.\n\r");

	ch->gold -= UMAX(0, amt);

	if (ded)
	{
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->connected == CON_PLAYING && !IS_SET(ch->comm, COMM_NOMUSIC))
			{
				Cprintf(d->character, "A Bard MUSIC: The next song is dedicated to %s by %s.\n\r", victim->name, ch->name);
			}
		}
	}

	if (global)
	{
		for (i = 1; i <= MAX_GLOBAL; i++)
			if (channel_songs[i] < 0)
			{
				if (i == 1)
					channel_songs[0] = -1;
				channel_songs[i] = song;
				return;
			}
	}
	else
	{
		for (i = 1; i < 5; i++)
			if (juke->value[i] < 0)
			{
				if (i == 1)
					juke->value[0] = -1;
				juke->value[i] = song;
				return;
			}
	}
}
