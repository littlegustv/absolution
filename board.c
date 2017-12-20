/* revision 1.2 - August 1 1999 - making it compilable under g++ */
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


#include <sys/types.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "recycle.h"
#include "utils.h"
#include "db.h"
#include "clan.h"


void do_help( CHAR_DATA *ch, char* arg );
void do_afk( CHAR_DATA *ch, char* arg );
void note_forward ( CHAR_DATA *ch, char *arg );
void note_reply ( CHAR_DATA *ch, char *arg );

/*

 Note Board system, (c) 1995-96 Erwin S. Andreasen, erwin@pip.dknet.dk
 =====================================================================

 Basically, the notes are split up into several boards. The boards do not
 exist physically, they can be read anywhere and in any position.

 Each of the note boards has its own file. Each of the boards can have its own
 "rights": who can read/write.

 Each character has an extra field added, namele the timestamp of the last note
 read by him/her on a certain board.

 The note entering system is changed too, making it more interactive. When
 entering a note, a character is put AFK and into a special CON_ state.
 Everything typed goes into the note.

 For the immortals it is possible to purge notes based on age. An Archive
 options is available which moves the notes older than X days into a special
 board. The file of this board should then be moved into some other directory
 during e.g. the startup script and perhaps renamed depending on date.

 Note that write_level MUST be >= read_level or else there will be strange
 output in certain functions.

 Board DEFAULT_BOARD must be at least readable by *everyone*.

*/

#define L_SUP (MAX_LEVEL - 1) /* if not already defined */

/*
 * Global Board list
 */
BOARD_DATA *board_first;

/* The prompt that the character is
 given after finishing a note with ~ or END */
const char * szFinishPrompt = "(C)ontinue, (V)iew, (P)ost or (F)orget it?";

long last_note_stamp = 0; /* To generate unique timestamps on notes */

#define BOARD_NOACCESS -1

static bool next_board (CHAR_DATA *ch);




/* append this note to the given file */
static void
append_note(FILE *fp, NOTE_DATA *note) {
    fprintf (fp, "Sender  %s~\n", note->sender);
    fprintf (fp, "Date    %s~\n", note->date);
    fprintf (fp, "Stamp   %ld\n", note->date_stamp);
    fprintf (fp, "Expire  %ld\n", note->expire);
    fprintf (fp, "To      %s~\n", note->to_list);
    fprintf (fp, "Subject %s~\n", note->subject);
    fprintf (fp, "Text\n%s~\n\n", note->text);
}

/* Save a note in a given board */
void
finish_note(BOARD_DATA *board, NOTE_DATA *note) {
    FILE *fp;
    NOTE_DATA *p;
    char filename[200];

    /* The following is done in order to generate unique date_stamps */

    if (last_note_stamp >= current_time) {
        note->date_stamp = ++last_note_stamp;
    } else {
        note->date_stamp = current_time;
        last_note_stamp = current_time;
    }

    /* are there any notes in there now? */
    if (board->note_first) {
        for (p = board->note_first; p->next; p = p->next )
            ; /* empty */

        p->next = note;
    } else {
        /* nope. empty list. */
        board->note_first = note;
    }

    /* append note to note file */
    sprintf (filename, "%s%s", NOTE_DIR, board->name);

    fp = fopen (filename, "a");
    if (!fp) {
        bug ("Could not open note file \"%s\" in append mode", filename);
        board->changed = TRUE; /* set it to TRUE hope it will be OK later? */
        return;
    }

    append_note(fp, note);
    fclose (fp);
}

/* Find a board number based on  a string */
BOARD_DATA*
board_lookup(const char *name) {
    BOARD_DATA *pBoard = board_first;

    for (pBoard = board_first; pBoard; pBoard = pBoard->next) {
        if (!str_prefix(name, pBoard->name) && pBoard->enabled) {
            break;
        }
    }

    return pBoard;
}

/*
 * This won't return NULL -- if there isn't a noteReadData entry for the
 * specified boardName, then a new one is create (and set with a time of 0)
 */
NOTE_READ_DATA*
get_last_note_read(CHAR_DATA *ch, char *boardName) {
    NOTE_READ_DATA *pNoteRead;

    for (pNoteRead = ch->pcdata->noteReadData; pNoteRead; pNoteRead = pNoteRead->next) {
        if (!str_cmp(pNoteRead->boardName, boardName)) {
            return pNoteRead;
        }

        if (!pNoteRead->next) {
            // We've hit the end of our list, and haven't found a match
            pNoteRead->next = new_note_read();
            pNoteRead->next->boardName = str_dup(boardName);
        }
    }

    // There wasn't any note read data for the player
    pNoteRead = new_note_read();
    pNoteRead->boardName = str_dup(boardName);
    
    ch->pcdata->noteReadData = pNoteRead;

    return pNoteRead;
}

// 1-based
BOARD_DATA*
getBoard(int boardNumber) {
    int i;
    BOARD_DATA *pBoard;

    if (boardNumber < 1) {
        return NULL;
    }

    pBoard = board_first;

    for (i = 1; i < boardNumber && pBoard; i++) {
        pBoard = pBoard->next;
    }

    return pBoard;
}

BOARD_DATA*
getDefaultBoard() {
    BOARD_DATA *pBoard;

    for (pBoard = board_first; pBoard; pBoard = pBoard->next) {
        if (pBoard->isDefault && pBoard->enabled) {
            break;
        }
    }

    return pBoard;
}

bool
ensureValidBoard(CHAR_DATA *ch) {
    if (!ch->pcdata->board) {
        // No default board defined when character logged in
        Cprintf(ch, "Please select a note board.\n\r");

        return FALSE;
    }

    if (!ch->pcdata->board->enabled) {
        Cprintf(ch, "Your current note board has been disabled.\n\r");
        Cprintf(ch, "Please select a different note board.\n\r");

        return FALSE;
    }

    return TRUE;
}

/* Remove list from the list. Do not free note */
static void
unlink_note(BOARD_DATA *board, NOTE_DATA *note) {
    NOTE_DATA *p;

    if (board->note_first == note) {
        board->note_first = note->next;
    } else {
        for (p = board->note_first; p && p->next != note; p = p->next);
        if (!p) {
            bug ("unlink_note: could not find note.");
        } else {
            p->next = note->next;
        }
    }
}

/* Find the nth note on a board. Return NULL if ch has no access to that note */
static NOTE_DATA*
find_note(CHAR_DATA *ch, BOARD_DATA *board, int num) {
    int count = 0;
    NOTE_DATA *p;

    for (p = board->note_first; p ; p = p->next) {
        if (++count == num) {
            break;
        }
    }

    if ( (count == num) && is_note_to (ch, p)) {
        return p;
    } else {
        return NULL;
    }
}

/* save a single board */
static void
save_board (BOARD_DATA *board) {
    FILE *fp;
    char filename[200];
    NOTE_DATA *note;

    sprintf (filename, "%s%s", NOTE_DIR, board->name);

    fp = fopen (filename, "w");
    if (!fp) {
        bug ("Error writing to: %s", filename);
    } else {
        for (note = board->note_first; note ; note = note->next) {
            append_note(fp, note);
        }

        fclose (fp);
    }
}

/* Show one note to a character */
static void
show_note_to_char (CHAR_DATA *ch, NOTE_DATA *note, int num) {
    char buf[4 * MAX_STRING_LENGTH];

    /* Ugly colors ? */
    sprintf(buf, "[%4d] %s: %s\n\r", num, note->sender, note->subject);
    sprintf_cat(buf, "       Date: %s\n\r", note->date);
    sprintf_cat(buf, "       To:   %s\n\r", note->to_list);
    sprintf_cat(buf, "================================================================================\n\r");
    sprintf_cat(buf, "%s\n\r", note->text);
    sprintf_cat( buf, "\n\r" );

    page_to_char (buf,ch);
}

/* Save changed boards */
void
save_notes() {
    BOARD_DATA *pBoard;

    for (pBoard = board_first; pBoard; pBoard = pBoard->next) {
        if (pBoard->changed) {
            /* only save changed boards */
            save_board (pBoard);
        }
    }
}

/* Load a single board */
// TODO: Static?
void
load_board(BOARD_DATA *board) {
    FILE *fp, *fp_archive;
    NOTE_DATA *last_note;
    char filename[200];
    char archive_name[200];
    NOTE_DATA *pnote;

    sprintf (filename, "%s%s", NOTE_DIR, board->filename);

    fp = fopen (filename, "r");

    /* Silently return */
    if (!fp) {
        return;
    }

    sprintf (archive_name, "%s%s.old", NOTE_DIR, board->filename);

    fp_archive = fopen (archive_name, "a");
    if (!fp_archive) {
        bug ("Could not open archive board \"%s\" for appending.", archive_name);
    }

    last_note = NULL;

    for (;;) {
        char letter;

        do {
            letter = getc( fp );

            if ( feof(fp) ) {
                fclose( fp );
                fclose (fp_archive);

                return;
            }
        }
        while ( isspace(letter) );

        ungetc( letter, fp );

        pnote = new_note();

        if (str_cmp( fread_word(fp), "sender" )) {
            break;
        }
        pnote->sender = fread_string(fp);

        if (str_cmp( fread_word(fp), "date" )) {
            break;
        }
        pnote->date = fread_string(fp);

        if (str_cmp( fread_word(fp), "stamp" )) {
            break;
        }
        pnote->date_stamp = fread_number(fp);

        if (str_cmp( fread_word(fp), "expire" )) {
            break;
        }
        pnote->expire = fread_number(fp);

        if (str_cmp( fread_word(fp), "to" )) {
            break;
        }
        pnote->to_list = fread_string(fp);

        if  (str_cmp( fread_word(fp), "subject" )) {
            break;
        }
        pnote->subject = fread_string(fp);

        if (str_cmp( fread_word(fp), "text" )) {
            break;
        }
        pnote->text = fread_string(fp);

        pnote->next = NULL; /* jic */

        /* Should this note be archived right now ? */

        if (pnote->expire < current_time) {
            if (fp_archive) {
                append_note(fp_archive, pnote);
            }

            free_note(pnote);
            board->changed = TRUE;
            continue;
        }


        if (board->note_first == NULL) {
            board->note_first = pnote;
        } else {
            last_note->next = pnote;
        }

        last_note = pnote;
    }

    free_note(pnote);
    bug( "Load_notes: bad key word.");
}

/* Initialize structures. Load all boards. */
void
load_boards() {
    char filename[200];
    FILE *fp;
    BOARD_DATA *pBoard;

    sprintf(filename, "%s%s", NOTE_DIR, "index");

    fclose(fpReserve);

    if (!(fp = fopen(filename, "r"))) {
        bug("load_boards: fopen: %s: %s", filename, strerror(errno));
        fpReserve = fopen(NULL_FILE, "r");

        return;
    }

    load_board_list(fp);

    fclose(fp);
    fpReserve = fopen(NULL_FILE, "r");

    for (pBoard = board_first; pBoard; pBoard = pBoard->next) {
        load_board(pBoard);
    }
}

/* Returns TRUE if the specified note is address to ch */
bool
is_note_to(CHAR_DATA *ch, NOTE_DATA *note) {
    char *toList;
    char nextInToList[MAX_STRING_LENGTH];

    if (!str_cmp(ch->name, note->sender)) {
        return TRUE;
    }

    toList = note->to_list;

    while (TRUE) {
        toList = one_argument(toList, nextInToList);

        if (nextInToList[0] == '\0') {
            break;
        }

        if (!str_cmp("all", nextInToList)) {
            return TRUE;
        }

        if ( (get_trust(ch) > 54) && (
                !str_prefix("imm", nextInToList) ||
                !str_prefix("god", nextInToList)) ) {
            return TRUE;
        }

        if ((get_trust(ch) == MAX_LEVEL) && !str_prefix("imp", nextInToList)) {
            return TRUE;
        }

        if (!str_cmp(ch->name, nextInToList)) {
            return TRUE;
        }

        /* Allow a note to e.g. 40 to send to characters level 40 and above */
        if (is_number(nextInToList) && get_trust(ch) >= atoi(nextInToList)) {
            return TRUE;
        }

        if ( !str_cmp("clan", note->to_list) && is_clan(ch) ) {
            return TRUE;
        }
    }

    return FALSE;
}

bool
canAccess(CHAR_DATA *ch, BOARD_DATA *board) {
    if (board->readLevel > get_trust(ch)) {
        return FALSE;
    }

    if (board->clan
            && str_cmp(board->clan, clanName(ch->clan))
            && get_trust(ch) < 55 ) {
        return FALSE;
    }

    if (!board->enabled) {
        return FALSE;
    }

    return TRUE;
}

/* Return the number of unread notes 'ch' has in 'board' */
int
unread_notes(CHAR_DATA *ch, BOARD_DATA *board) {
    NOTE_DATA *note;
    NOTE_READ_DATA *pNoteRead;
    time_t last_read;
    int count = 0;

    if (!canAccess(ch, board)) {
        return 0;
    }

    pNoteRead = get_last_note_read(ch, board->name);

    last_read = pNoteRead->timestamp;

    for (note = board->note_first; note; note = note->next) {
        if (is_note_to(ch, note) && ((long)last_read < (long)note->date_stamp)) {
            count++;
        }
    }

    return count;
}

bool checkCanWriteNote(CHAR_DATA *ch) {
    if (IS_NPC(ch)) { 
        /* NPC cannot post notes */
        return FALSE;
    }

    if (!ensureValidBoard(ch)) {
        return FALSE;
    }

    if (ch->position == POS_FIGHTING) {
        Cprintf(ch, "No way! You are fighting.\n\r");
        return FALSE;
    }

    if (ch->position < POS_STUNNED) {
        Cprintf(ch, "You're not DEAD yet.\n\r");
        return FALSE;
    }

    if (auction->item != NULL && ((ch == auction->buyer) || (ch == auction->seller))) {
        Cprintf(ch, "Wait until you have sold/bought the item on auction.\n\r");
        return FALSE;
    }

    if (ch->no_quit_timer > 0 && !IS_IMMORTAL (ch)) {
        Cprintf(ch, "You are too nervous to write a note!\n\r");
        return FALSE;
    }

    if (IS_SET(ch->comm, COMM_NONOTE)) {
        Cprintf(ch, "Your note writing ability has been revoked.\n\r");
        return FALSE;
    }

    /* stops pc form writing notes in given room */
    if (IS_SET (ch->in_room->room_flags, ROOM_FERRY)) {
        Cprintf(ch, "You cannot write notes in the ferry!");
        return FALSE;
    }

    /* stops pc form quitiing in given room */
    if (IS_SET (ch->in_room->room_flags, ROOM_NOQUIT)) {
        Cprintf(ch, "You cannot write notes in this area!");
        return FALSE;
    }

    if (IS_SET (ch->wiznet, NO_QUIT)) {
        Cprintf(ch, "You cannot write notes yet, stick around and listen!");
        return FALSE;
    }

    if (ch->no_quit_timer > 0 && !IS_IMMORTAL (ch)) {
        Cprintf(ch, "You're too nervous to write. Calm down a bit first eh!");
        return FALSE;
    }

    if (!IS_IMMORTAL(ch) && in_enemy_hall(ch)) {
        Cprintf(ch, "You cannot write notes in a rival clan hall.\n\r");
        return FALSE;
    }

    if (get_trust(ch) < ch->pcdata->board->writeLevel) {
        Cprintf(ch, "You cannot post notes on this board.\n\r");
        return FALSE;
    }

    if (ch->craft_timer < 0) {
        Cprintf(ch, "You must wait until you are done crafting your item.\n\r");
        return FALSE;
    }

    return TRUE;
}

/*
 * COMMANDS
 */

/* Start writing a note */
static void
do_nwrite (CHAR_DATA *ch, char *argument) {
    char *strtime;
    ROOM_INDEX_DATA *location;
    char buf[MAX_STRING_LENGTH];

    if (!checkCanWriteNote(ch)) {
        return;
    }

    SET_BIT(ch->comm, COMM_NOTE_WRITE);

    /* continue previous note, if any text was written*/
    if (ch->pcdata->in_progress && (!ch->pcdata->in_progress->text)) {
        Cprintf(ch, "Note in progress cancelled because you did not manage to write any text \n\rbefore losing link.\n\r\n\r");
        free_note (ch->pcdata->in_progress);
        ch->pcdata->in_progress = NULL;
    }


    if (!ch->pcdata->in_progress) {
        ch->pcdata->in_progress = new_note();
        ch->pcdata->in_progress->sender = str_dup (ch->name);

        /* convert to ascii. ctime returns a string which last character is \n, so remove that */
        strtime = ctime (&current_time);
        strtime[strlen(strtime)-1] = '\0';

        ch->pcdata->in_progress->date = str_dup (strtime);
    }

    /* Kill any running macros - added by Tsongas 11/15/2002 */
    if (ch->pcdata->macroInstances != NULL) {
        macro_kill(ch, ch, "all");
    }

    act("$n starts writing a note." , ch, NULL, NULL, TO_ROOM, POS_RESTING);
    sprintf(buf, "%s starts writing a note.", ch->name);
    wiznet(buf, NULL, NULL, WIZ_NOTES, 0, 0);
    do_afk(ch, "");

    if ( ch->in_room == NULL ) {
        char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
    }

    ch->note_room = ch->in_room;
    if (ch->in_room->area->continent == 0) {
        location = get_room_index( ROOM_VNUM_LIMBO );
    } else {
        location = get_room_index( ROOM_VNUM_LIMBO_DOMINIA );
    }

    char_from_room( ch );
    char_to_room(ch, location);

    /* Begin writing the note ! */
    Cprintf(ch, "You are now %s a new note on the %s board.\n\r\n\r", ch->pcdata->in_progress->text ? "continuing" : "posting", ch->pcdata->board->name);

    Cprintf(ch, "From:    %s\n\r\n\r", ch->name);

    if (!ch->pcdata->in_progress->text) {
        /* Are we continuing an old note or not? */
        switch (ch->pcdata->board->nameRestriction) {
            case DEF_NORMAL:
                Cprintf(ch, "If you press Return, default recipient %s will be chosen.\n\r", ch->pcdata->board->names);
                break;

            case DEF_INCLUDE:
                Cprintf(ch, "The recipient list MUST include %s. If not, it will be added automatically.\n\r", ch->pcdata->board->names);
                break;

            case DEF_EXCLUDE:
                Cprintf(ch, "The recipient of this note must NOT include: %s.", ch->pcdata->board->names);
                break;
        }

        Cprintf(ch, "\n\rTo:      ");

        ch->desc->connected = CON_NOTE_TO;
        ch->pcdata->in_progress->text = str_dup("");
        /* nanny takes over from here */
    } else {
        /* we are continuing, print out all the fields and the note so far*/
        Cprintf(ch, "To:      %s\n\r", ch->pcdata->in_progress->to_list);
        Cprintf(ch, "Expires: %s\n\r", ctime(&ch->pcdata->in_progress->expire));
        Cprintf(ch, "Subject: %s\n\r", ch->pcdata->in_progress->subject);
        Cprintf(ch, "Your note so far:\n\r");
        Cprintf(ch, "%s", ch->pcdata->in_progress->text);

        Cprintf(ch, "\n\rEnter text. Type END on an empty line to end note.\n\r");
        Cprintf(ch, "================================================================================\n\r");

        ch->desc->connected = CON_NOTE_TEXT;
    }
}

/* Read next note in current group. If no more notes, go to next board */
static void
do_nread (CHAR_DATA *ch, char *argument) {
    NOTE_DATA *p;
    int count = 0, number;
    NOTE_READ_DATA *pNoteRead;
    time_t *last_note;

    if (!ensureValidBoard(ch)) {
        return;
    }

    pNoteRead = get_last_note_read(ch, ch->pcdata->board->name);

    last_note = &pNoteRead->timestamp;

    if (!str_cmp(argument, "again")) {
        /* read last note again */
    } else if (is_number (argument)) {
        number = atoi(argument);

        for (p = ch->pcdata->board->note_first; p; p = p->next) {
            if (++count == number) {
                break;
            }
        }

        if (!p || !is_note_to(ch, p)) {
            Cprintf(ch, "No such note.\n\r");
        } else {
            show_note_to_char (ch,p,count);
            *last_note =  UMAX (*last_note, p->date_stamp);
        }
    } else {
        /* just next one */
        count = 1;
        for (p = ch->pcdata->board->note_first; p ; p = p->next, count++) {
            if ((p->date_stamp > *last_note) && is_note_to(ch,p)) {
                show_note_to_char (ch,p,count);
                /* Advance if new note is newer than the currently newest for that char */
                *last_note =  UMAX (*last_note, p->date_stamp);

                return;
            }
        }

        if (next_board(ch)) {
            Cprintf(ch, "No new notes in this board, changed to next board, {M%s{x.\n\r", ch->pcdata->board->name);
            do_nread(ch, "");
        } else {
            Cprintf(ch, "No new notes in this board.\n\r");
            Cprintf(ch, "There are no more boards, returning to first board.\n\r");
            do_board(ch, "1");
        }
    }
}

/* Remove a note */
static void
do_nremove (CHAR_DATA *ch, char *argument) {
    NOTE_DATA *p;

    if (!ensureValidBoard(ch)) {
        return;
    }

    if (!is_number(argument)) {
        Cprintf(ch, "Remove which note?\n\r");
        return;
    }

    p = find_note(ch, ch->pcdata->board, atoi(argument));

    if (!p) {
        Cprintf(ch, "No such note.\n\r");
        return;
    }

    if (str_cmp(ch->name,p->sender) && (get_trust(ch) < (MAX_LEVEL - 1))) {
        Cprintf(ch, "You are not authorized to remove this note.\n\r");
        return;
    }

    unlink_note (ch->pcdata->board,p);
    free_note (p);
    Cprintf(ch, "Note removed!\n\r");

    save_board(ch->pcdata->board); /* save the board */
}


/* List all notes or if argument given, list N of the last notes */
/* Shows REAL note numbers! */
static void
do_nlist (CHAR_DATA *ch, char *argument) {
    int count= 0, show = 0, num = 0, has_shown = 0;
    time_t last_note;
    NOTE_DATA *p;
    char to[MAX_STRING_LENGTH];
    char subject[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH *100];
    NOTE_READ_DATA *pNoteRead;

    if (!ensureValidBoard(ch)) {
        return;
    }

    /* first, count the number of notes */
    if (is_number(argument)) {
        show = atoi(argument);

        for (p = ch->pcdata->board->note_first; p; p = p->next) {
            if (is_note_to(ch,p)) {
                count++;
            }
        }
    }

    Cprintf(ch, "Notes on this board:\n\r");
    Cprintf(ch, "Num>   From          To                    Subject\n\r");

    pNoteRead = get_last_note_read(ch, ch->pcdata->board->name);
    last_note = pNoteRead->timestamp;

    buf[0] = '\0';
    for (p = ch->pcdata->board->note_first; p; p = p->next) {
        num++;
        if (is_note_to(ch,p)) {
            has_shown++; /* note that we want to see X VISIBLE note, not just last X */
            if (!show || ((count-show) < has_shown)) {
                sprintf(to, "%s", p->to_list);
                if (strlen(to) > 20) {
                    sprintf(to + 17, "{D...{x");
                }

                sprintf(subject, "%s", p->subject);
                if (strlen(subject) > 36) {
                    sprintf(subject + 33, "{D...{x");
                }

                sprintf_cat(buf, "%3d> %c %-13s %-20s  %s\n\r", num, last_note < p->date_stamp ? '*' : ' ', p->sender, to, subject);
            }
        }
    }

    strcat(buf, "\n\r");
    page_to_char(buf, ch);
}

/* catch up with some notes */
static void
do_ncatchup (CHAR_DATA *ch, char *argument) {
    NOTE_DATA *p;
    NOTE_READ_DATA *pNoteRead;

    if (!ensureValidBoard(ch)) {
        return;
    }

    /* Find last note */
    for (p = ch->pcdata->board->note_first; p && p->next; p = p->next);

    if (!p) {
        Cprintf(ch, "Alas, there are no notes in that board.\n\r");
    } else {
        pNoteRead = get_last_note_read(ch, ch->pcdata->board->name);
        pNoteRead->timestamp = p->date_stamp;
        Cprintf(ch, "All messages skipped.\n\r");
    }
}

static void
do_note_notify (BOARD_DATA *board, NOTE_DATA *note, CHAR_DATA *ch) {
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d; d = d->next) {
        if ((d->connected == CON_PLAYING)
                && (IS_SET(d->character->act, PLR_NOTIFY))
                && canAccess(d->character, board)
                && (is_note_to(d->character, note))) {
            Cprintf(d->character, "{CNew note posted by %s on board %s.{x\n\r", (can_see(d->character, ch) ? ch->name : "someone"), board->name);
        }
    }
}

/* catch up with all boards */
static void
do_nupdate (CHAR_DATA *ch, char *argument) {
    NOTE_DATA *p;
    BOARD_DATA *pBoard;

    for (pBoard = board_first; pBoard; pBoard = pBoard->next) {
        if (canAccess(ch, pBoard)) {
            NOTE_READ_DATA *pNoteRead = get_last_note_read(ch, pBoard->name);

            for (p = pBoard->note_first; p && p->next; p = p->next);

            if (p) {
                pNoteRead->timestamp = p->date_stamp;
            }
        }
    }

    Cprintf(ch, "You are now up to date on all boards.\n\r");
}


/* Dispatch function for backwards compatibility */
void
do_note (CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];

    if (IS_NPC(ch)) {
        return;
    }

    argument = one_argument (argument, arg);

    if ((!arg[0]) || (!str_prefix(arg, "read"))) { 
        /* 'note' or 'note read X' */
        do_nread (ch, argument);
    } else if (!str_prefix (arg, "list")) {
        do_nlist (ch, argument);
    } else if (!str_prefix (arg, "write")) {
        do_nwrite (ch, argument);
    } else if (!str_prefix (arg, "remove")) {
        do_nremove (ch, argument);
    } else if (!str_prefix (arg, "update")) {
        do_nupdate (ch, argument);
    } else if (!str_prefix (arg, "catchup")) {
        do_ncatchup (ch, argument);
    } else if (!str_prefix (arg, "forward")) {
        note_forward (ch, argument);
    } else if (!str_prefix (arg, "reply")) {
        note_reply (ch, argument);
    } else {
        Cprintf(ch, "I don't understand '%s', please read 'help note'.\n\r", arg);
    }
}

/* Show all accessible boards with their numbers of unread messages OR
   change board. New board name can be given as a number or as a name (e.g.
    board personal or board 4 */
void
do_board (CHAR_DATA *ch, char *argument) {
    int count, number;
    BOARD_DATA *pBoard;

    if (IS_NPC(ch)) {
        return;
    }

    count = 0;
    for (pBoard = board_first; pBoard; pBoard = pBoard->next) {
        if (canAccess(ch, pBoard)) {
            count++;
        }
    }

    if (count == 0) {
        Cprintf(ch, "There are no note boards currently available.\n\r");
        return;
    }

    if (!argument[0]) {
        /* show boards */
        count = 1;
        Cprintf(ch, "Num             Name Unread Description\n\r");
        Cprintf(ch, "=== ================ ====== ===========\n\r");
        for (pBoard = board_first; pBoard; pBoard = pBoard->next) {
            if (canAccess(ch, pBoard)) {
                int unread = unread_notes(ch, pBoard);

                Cprintf(ch, "%2d> %16s [{%c%4d{x] %s\n\r", count, pBoard->name, unread ? 'r' : 'g', unread, pBoard->description);
                count++;
            }
        }

        if (!ch->pcdata->board) {
            // There wasn't a default board defined when the character logged on

            ch->pcdata->board = getDefaultBoard();
        }

        if (!ch->pcdata->board) {
            Cprintf(ch, "\n\rYou currently don't have a board selected.\n\r");
            return;
        }

        Cprintf(ch, "\n\rYou current board is %s.\n\r", ch->pcdata->board->name);

        /* Inform of rights */
        if (ch->pcdata->board->readLevel > get_trust(ch)) {
            Cprintf(ch, "You cannot read or write notes on this board.\n\r");
        } else if (ch->pcdata->board->writeLevel > get_trust(ch)) {
            Cprintf(ch, "You can only read notes from this board.\n\r");
        } else {
            Cprintf(ch, "You can both read and write on this board.\n\r");
        }

        return;
    }

    /* Change board based on its number */
    if (is_number(argument)) {
        count = 0;
        number = atoi(argument);

        for (pBoard = board_first; pBoard; pBoard = pBoard->next) {
            if (canAccess(ch, pBoard)) {
                if (++count == number) {
                    break;
                }
            }
        }

        if (count == number) {
            ch->pcdata->board = pBoard;

            Cprintf(ch, "Current board changed to %s.  ", pBoard->name);
            if (get_trust(ch) < pBoard->writeLevel) {
                Cprintf(ch, "You can only read here.\n\r");
            } else {
                Cprintf(ch, "You can both read and write here.\n\r");
            }
        } else {
            Cprintf(ch, "No such board.\n\r");
        }

        return;
    }

    /* Non-number given, find board with that name */
    pBoard = board_lookup(argument);

    /* Does ch have access to this board? */
    if (!pBoard || !canAccess(ch, pBoard)) {
        Cprintf(ch, "No such board.\n\r");
        return;
    }

    ch->pcdata->board = pBoard;
    Cprintf(ch, "Current board changed to %s.  ", pBoard->name);
    if (get_trust(ch) < pBoard->writeLevel) {
        Cprintf(ch, "You can only read here.\n\r");
    } else {
        Cprintf(ch, "You can both read and write here.\n\r");
    }
}

NOTE_DATA*
make_note(const char* board_name, const char *sender, const char *to, const char *subject, const int expire_days, const char *text) {
    BOARD_DATA *board;
    NOTE_DATA *note;
    char *strtime;

    board = board_lookup(board_name);

    if (!board) {
        bug ("make_note: board not found");
        return NULL;
    }

    if (strlen(text) > MAX_NOTE_TEXT) {
        bug ("make_note: text too long (%d bytes)", strlen(text));
        return NULL;
    }

    note = new_note(); /* allocate new note */

    note->sender = str_dup (sender);
    note->to_list = str_dup(to);
    note->subject = str_dup (subject);
    note->expire = current_time + expire_days * 60 * 60 * 24;
    note->text = str_dup (text);

    /* convert to ascii. ctime returns a string which last character is \n, so remove that */
    strtime = ctime (&current_time);
    strtime[strlen(strtime)-1] = '\0';

    note->date = str_dup (strtime);

    finish_note(board, note);

    return note;
}

int
getCount(const char *txt) {
    char buf[5];
    const char *end;

    if (!txt) {
        return 0;
    }

    if (*txt == ':' && *(txt + 1) == ' ') {
        return 1;
    }

    if (*txt != '[') {
        return 0;
    }

    txt++;
    end = txt + 1;

    while (isdigit(*end) && end - txt < 5) {
        end++;
    }

    if (*end != ']') {
        // hit end of string, or non-digit non-], or more than 5 digits
        return 0;
    }

    if (*(end + 1) != ':' || *(end + 2) != ' ') {
        /*
         * final bit of tom-foolery.  Don't pass on "Re[123]whatever" or
         * Re[123]:whatever (i.e. both the colon and the space are needed)
         */
        return 0;
    }

    buf[0] = '\0';
    strncat(buf, txt, end - txt);

    return atoi(buf);
}

/*
 * Does an in-place prefix of the given subject.
 * is used for anything that'll take the form of
 * Xxxx: (and, subsequently, Xxxx[dd]:), e.g. Re and Fwd
 */
void
prefixSubject(const char *prefix, char *subject) {
    char buf[MAX_STRING_LENGTH];

    if (str_prefix(prefix, subject)) {
        sprintf(buf, "%s: %s", prefix, subject);
    } else {
        // Subject is potentially already prefixed... let's take advantage of that
        int replyCount = getCount(subject + strlen(prefix));

        if (!replyCount) {
            sprintf(buf, "%s: %s", prefix, subject);
        } else {
            char *subjectStart = subject + strlen(prefix);

            while (*subjectStart != ':') {
                subjectStart++;
            }

            subjectStart += 2;

            sprintf(buf, "%s[%d]: %s", prefix, replyCount + 1, subjectStart);
        }
    }

    sprintf(subject, "%s", buf);
}

/* Note forwarding by Tsongas 08/17/2001 */
/* Syntax: note forward <number> <to list> */
void
note_forward(CHAR_DATA *ch, char *argument) {
    NOTE_DATA *p;
    int note;
    char arg1[MAX_INPUT_LENGTH];
    char subject[MAX_INPUT_LENGTH];

    if (!ensureValidBoard(ch)) {
        return;
    }

    argument = one_argument(argument, arg1); /* note number */

    /* Check to make sure note is a number */
    if (!is_number(arg1)) {
        Cprintf(ch, "You need to enter a note number!\n\r");
        Cprintf(ch, "Syntax: note forward <note number> <to>\n\r");
        return;
    } else {
        /* Note exist and can they see it? */
        note = atoi(arg1);

        p = find_note(ch, ch->pcdata->board, note);

        if(!p) {
            Cprintf(ch, "No such note!\n\r");
            return;
        }
    }

    /* No sending to everyone with a number */
    if (is_number(argument)) {
        Cprintf(ch, "You need to enter a recipient, not a number!\n\r");
        Cprintf(ch, "Syntax: note forward <note number> <to>\n\r");
        return;
    }

    /* We have someone to send it to? */
    if (argument == NULL || strlen(argument) < 3) {
        Cprintf(ch, "You need to enter a recipient!\n\r");
        Cprintf(ch, "Syntax: note forward <note number> <to>\n\r");
        return;
    }

    /* Not to everyone? */
    if (!str_prefix (argument, "all")) {
        Cprintf(ch, "You can't forward to everyone!\n\r");
        return;
    }

    /* Set Subject name with p's information */
    subject[0] = 0;
    strcpy(subject, p->subject);

    prefixSubject("Fwd", subject);

    if (strlen(subject) > 60) {
        Cprintf(ch, "Subject too long, perhaps it's time to start a new note.\n\r");
        return;
    }

    p = make_note(ch->pcdata->board->name, ch->name, argument, subject, 30, p->text);

    do_note_notify(ch->pcdata->board, p, ch);
}

/* Note reply by Tsongas 04/01/2002 */
/* Syntax: note reply <number> [all] */
void
note_reply(CHAR_DATA *ch, char *argument) {
    // Used to get original note data
    NOTE_DATA *p;
    // Used for player location for note write
    ROOM_INDEX_DATA *location;
    // Used for one_argument functions
    char arg1[MAX_INPUT_LENGTH];
    // New quoted text on each line
    char quoted[MAX_STRING_LENGTH];
    // New subject (Re: old)
    char subject[MAX_INPUT_LENGTH];
    // Used for whole unedited to list
    char temp_to[MAX_INPUT_LENGTH];
    // Used to store new, edited to list (compact)
    char to[MAX_INPUT_LENGTH];
    // Used to store ctime
    char *strtime;
    // Used to turn temp_to into a char*
    char *tp = NULL;
    // Loop counter
    int i = 0;
    // Note number
    int note;
    // Length of original note
    int quotelength = 0;

    argument = one_argument(argument, arg1);

    /* Check for all standard note writing conditions */
    if (!checkCanWriteNote(ch)) {
        return;
    }

    if (!is_number(arg1)) {
        Cprintf(ch, "You need to enter a note number!\n\r");
        Cprintf(ch, "Syntax: note reply <note number> [all]\n\r");
        return;
    }

    note = atoi(arg1);

    p = find_note(ch, ch->pcdata->board, note);

    if (!p) {
        Cprintf(ch, "No such note!\n\r");
        return;
    }

    /* Set subject and check to make sure it's not too long */
    subject[0] = 0;
    strcpy(subject, p->subject);

    prefixSubject("Re", subject);

    if (strlen(subject) > 60) {
        Cprintf(ch, "Subject too long, perhaps it's time to start a new note.\n\r");
        return;
    }

    /* Reserve space for new note */
    if (!ch->pcdata->in_progress) {
        ch->pcdata->in_progress = new_note();
        ch->pcdata->in_progress->sender = str_dup (ch->name);

        /* convert to ascii. ctime returns a string which last character is \n, so remove that */
        strtime = ctime (&current_time);
        strtime[strlen(strtime)-1] = '\0';

        ch->pcdata->in_progress->date = str_dup (strtime);
    }

    /* Kill any running macros - added by Tsongas 11/15/2002 */
    if (ch->pcdata->macroInstances != NULL) {
        macro_kill(ch, ch, "all");
    }

    /* Set AFK and send them to proper location */
    act("$n starts writing a note." , ch, NULL, NULL, TO_ROOM, POS_RESTING);
    do_afk( ch, "" );
    if ( ch->in_room == NULL ) {
        char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
    }

    ch->note_room = ch->in_room;
    if (ch->in_room->area->continent == 0) {
        location = get_room_index( ROOM_VNUM_LIMBO );
    } else {
        location = get_room_index( ROOM_VNUM_LIMBO_DOMINIA );
    }

    char_from_room( ch );
    char_to_room(ch, location);

    /* Begin writing the note ! */
    Cprintf(ch, "You are now replying to a note on the %s board.\n\r\n\r", ch->pcdata->board->name);
    Cprintf(ch, "From:    %s\n\r", ch->name);

    ch->pcdata->in_progress->expire = current_time + ch->pcdata->board->defaultExpire * 24L * 3600L;
    ch->pcdata->in_progress->subject = str_dup (subject);

    /* Set internet style quoting */
    quotelength = strlen(p->text);
    quoted[0] = '>';
    quoted[1] = '\0';
    for (i = 0; i < quotelength; i++) {
        if (p->text[i] == '\r') {
            sprintf(quoted, "%s%c>", quoted, p->text[i]);
        } else {
            sprintf(quoted, "%s%c", quoted, p->text[i]);
        }
    }

    quoted[strlen(quoted) - 1] = '\n';
    sprintf(quoted, "%s\r%c", quoted, '\0');

    ch->pcdata->in_progress->text = str_dup(quoted);

    /* Create new to listing. Make sure it doesn't have duplicate names. */
    if (!str_prefix(argument, "all")) {
        sprintf(temp_to, "%s %s", p->to_list, p->sender);
        tp = temp_to;
        // To get rid of the horrid space (check below)
        to[0] = '\0';
        while (1) {
            tp = one_argument(tp, arg1);
            if (arg1[0] == '\0') {
                break;
            }

            if (str_cmp(ch->name, arg1) && to[0] == '\0') {
                sprintf(to, "%s", arg1);
            } else if(str_cmp(ch->name, arg1) && !is_name(arg1, to)) {
                sprintf(to, "%s %s", to, arg1);
            }
        }

        ch->pcdata->in_progress->to_list = str_dup (to);
    } else {
        ch->pcdata->in_progress->to_list = str_dup (p->sender);
    }

    /* Print information, and send user to nanny. She's a good lady. */
    Cprintf(ch, "To:      %s\n\r", ch->pcdata->in_progress->to_list);
    Cprintf(ch, "Expires: %s", ctime(&ch->pcdata->in_progress->expire));
    Cprintf(ch, "Subject: %s\n\r", ch->pcdata->in_progress->subject);
    Cprintf(ch, "\n\r");
    Cprintf(ch, "Enter text. Type END on an empty line to end note.\n\r");
    Cprintf(ch, "================================================================================\n\r");
    Cprintf(ch, "%s", ch->pcdata->in_progress->text);

    ch->desc->connected = CON_NOTE_TEXT;
}

/* tries to change to the next accessible board */
static bool
next_board(CHAR_DATA *ch) {
    BOARD_DATA *pBoard;

    for (pBoard = ch->pcdata->board->next; pBoard; pBoard = pBoard->next) {
        if (canAccess(ch, pBoard)) {
            ch->pcdata->board = pBoard;
            return TRUE;
        }
    }

    return FALSE;
}

void
handle_con_note_to (CHAR_DATA *ch, char * argument) {
    char buf [MAX_INPUT_LENGTH];

    if (!ch->pcdata->in_progress) {
        ch->desc->connected = CON_PLAYING;
        bug ("nanny: In CON_NOTE_TO, but no note in progress");
        return;
    }

    strcpy (buf, argument);
    smash_tilde (buf); /* change ~ to - as we save this field as a string later */

    switch (ch->pcdata->board->nameRestriction) {
        /* default field */
        case DEF_NORMAL:
            /* empty string? */
            if (!buf[0]) {
                ch->pcdata->in_progress->to_list = str_dup (ch->pcdata->board->names);
                Cprintf(ch, "Assumed default recipient: %s\n\r", ch->pcdata->board->names);
            } else {
                ch->pcdata->in_progress->to_list = str_dup (buf);
            }

            break;

        /* forced default */
        case DEF_INCLUDE:
            if (!is_name (ch->pcdata->board->names, buf)) {
                strcat (buf, " ");
                strcat (buf, ch->pcdata->board->names);
                ch->pcdata->in_progress->to_list = str_dup(buf);

                sprintf(buf, "\n\rYou did not specify %s as recipient, so it was automatically added.\n\r", ch->pcdata->board->names);
                Cprintf(ch, "New To :  %s\n\r", ch->pcdata->in_progress->to_list);
            } else {
                ch->pcdata->in_progress->to_list = str_dup (buf);
            }
            break;

        /* forced exclude */
        case DEF_EXCLUDE:
            if (is_name (ch->pcdata->board->names, buf)) {
                Cprintf(ch, "You are not allowed to send notes to %s on this board. Try again.\n\r", ch->pcdata->board->names);
                Cprintf(ch, "To:      ");
                return; /* return from nanny, not changing to the next state! */
            } else {
                ch->pcdata->in_progress->to_list = str_dup (buf);
            }
            break;
    }

    Cprintf(ch, "\n\rSubject: ");
    ch->desc->connected = CON_NOTE_SUBJECT;
}

void
handle_con_note_subject (CHAR_DATA *ch, char * argument) {
    char buf [MAX_INPUT_LENGTH];

    if (!ch->pcdata->in_progress) {
        ch->desc->connected = CON_PLAYING;
        bug("nanny: In CON_NOTE_SUBJECT, but no note in progress");
        return;
    }

    strcpy (buf, argument);
    smash_tilde (buf); /* change ~ to - as we save this field as a string later */

    /* Do not allow empty subjects */
    if (!buf[0]) {
        Cprintf(ch, "Please find a meaningful subject!\n\r");
        Cprintf(ch, "Subject: ");
    } else  if (strlen(buf)>60) {
        Cprintf(ch, "No, no. This is just the Subject. You're not writing the note yet. Twit.\n\r");
        Cprintf(ch, "Subject: ");
    } else {
        /* advance to next stage */
        ch->pcdata->in_progress->subject = str_dup(buf);
        if (IS_IMMORTAL(ch)) {
            /* immortals get to choose number of expire days */
            Cprintf(ch, "\n\r");
            Cprintf(ch, "How many days do you want this note to expire in?\n\r");
            Cprintf(ch, "Press Enter for default value for this board, %d days.\n\r", ch->pcdata->board->defaultExpire);
            Cprintf(ch, "Expire:  ");
            ch->desc->connected = CON_NOTE_EXPIRE;
        } else {
            ch->pcdata->in_progress->expire = current_time + ch->pcdata->board->defaultExpire * 24L * 3600L;
            Cprintf(ch, "This note will expire %s\r",ctime(&ch->pcdata->in_progress->expire));
            Cprintf(ch, "\n\r");
            Cprintf(ch, "Enter text. Type END on an empty line to end note.\n\r");
            Cprintf(ch, "================================================================================\n\r");
            ch->pcdata->in_progress->text = str_dup("");
            ch->desc->connected = CON_NOTE_TEXT;
        }
    }
}

void
handle_con_note_expire(CHAR_DATA *ch, char * argument) {
    char buf[MAX_STRING_LENGTH];
    time_t expire;
    int days;

    if (!ch->pcdata->in_progress) {
        ch->desc->connected = CON_PLAYING;
        bug("nanny: In CON_NOTE_EXPIRE, but no note in progress");
        return;
    }

    /* Numeric argument. no tilde smashing */
    strcpy (buf, argument);
    if (!buf[0]) {
        /* assume default expire */
        days = ch->pcdata->board->defaultExpire;
    } else {
        /* use this expire */
        if (!is_number(buf)) {
            Cprintf(ch, "Write the number of days!\n\r");
            Cprintf(ch, "Expire:  ");
            return;
        } else {
            days = atoi (buf);
            if (days <= 0) {
                Cprintf(ch, "This is a positive MUD. Use positive numbers only! :)\n\r");
                Cprintf(ch, "Expire:  ");
                return;
            }
        }
    }

    expire = current_time + (days * 24L * 3600L); /* 24 hours, 3600 seconds */

    ch->pcdata->in_progress->expire = expire;

    /* note that ctime returns XXX\n so we only need to add an \r */

    Cprintf(ch, "\n\r");
    Cprintf(ch, "Enter text. Type END on an empty line to end note.\n\r");
    Cprintf(ch, "================================================================================\n\r");
    ch->pcdata->in_progress->text = str_dup("");
    ch->desc->connected = CON_NOTE_TEXT;
}



void
handle_con_note_text (CHAR_DATA *ch, char * argument) {
    char buf[MAX_STRING_LENGTH];
    char letter[4*MAX_STRING_LENGTH];

    if (!ch->pcdata->in_progress) {
        ch->desc->connected = CON_PLAYING;
        bug("nanny: In CON_NOTE_TEXT, but no note in progress");
        return;
    }

    /* First, check for EndOfNote marker */
    strcpy (buf, argument);
    if ((!str_cmp(buf, "~")) || (!str_cmp(buf, "END"))) {
        Cprintf(ch, "\n\r\n\r");
        Cprintf(ch, szFinishPrompt);
        Cprintf(ch, "\n\r");
        ch->desc->connected = CON_NOTE_FINISH;
        return;
    }

    smash_tilde (buf); /* smash it now */

    /* Check for too long lines. Do not allow lines longer than 80 chars */

    if (strlen (buf) > MAX_LINE_LENGTH) {
        Cprintf(ch, "Too long line rejected. Do NOT go over 80 characters!\n\r");
        return;
    }

    /* Not end of note. Copy current text into temp buffer, add new line, and copy back */

    if (ch->pcdata->in_progress->text && (strlen(ch->pcdata->in_progress->text) + strlen (buf)) > MAX_NOTE_TEXT) {
        Cprintf(ch, "Note too long, aborted!\n\r");

        free_note (ch->pcdata->in_progress);
        ch->pcdata->in_progress = NULL;
        ch->desc->connected = CON_PLAYING;
        return;
    }

    /* How would the system react to strcpy( , NULL) ? */
    if (ch->pcdata->in_progress->text) {
        strcpy (letter, ch->pcdata->in_progress->text);
        free_string (ch->pcdata->in_progress->text);
        ch->pcdata->in_progress->text = NULL; /* be sure we don't free it twice */
    } else {
        strcpy (letter, "");
    }

    /* Add new line to the buffer */
    strcat (letter, buf);
    strcat (letter, "\n\r"); /* new line. \r first to make note files better readable */

    /* allocate dynamically */
    ch->pcdata->in_progress->text = str_dup (letter);
}

void
handle_con_note_finish (CHAR_DATA *ch, char * argument) {
    char buf[MAX_STRING_LENGTH];

    if (!ch->pcdata->in_progress) {
        ch->desc->connected = CON_PLAYING;
        bug("nanny: In CON_NOTE_FINISH, but no note in progress");
        return;
    }

    switch (tolower(argument[0])) {
        case 'c': /* keep writing */
        Cprintf(ch, "Continuing note...\n\r",0);
            ch->desc->connected = CON_NOTE_TEXT;
            break;

        case 'v': /* view note so far */
            if (ch->pcdata->in_progress->text) {
                Cprintf(ch, "Text of your note so far:\n\r");
                Cprintf(ch, "%s", ch->pcdata->in_progress->text);
            } else {
                Cprintf(ch, "You haven't written a thing!\n\r\n\r");
            }

            Cprintf(ch, szFinishPrompt);
            Cprintf(ch, "\n\r");
            break;

        case 'p': /* post note */
            finish_note (ch->pcdata->board, ch->pcdata->in_progress);
            Cprintf(ch, "Note posted.\n\r");
            ch->desc->connected = CON_PLAYING;
            /* remove AFK status */
            do_note_notify(ch->pcdata->board, ch->pcdata->in_progress, ch);
            ch->pcdata->in_progress = NULL;
            do_afk( ch, "" );
            char_from_room( ch );
            char_to_room( ch, ch->note_room );
            ch->note_room = NULL;
            act("$n finishes $s note." , ch, NULL, NULL, TO_ROOM, POS_RESTING);
            sprintf(buf, "%s has finished writing a note.", ch->name);
            wiznet(buf, NULL, NULL, WIZ_NOTES, 0, 0);
            REMOVE_BIT(ch->comm, COMM_NOTE_WRITE);
            break;

        case 'f':
            Cprintf(ch, "Note cancelled!\n\r");
            free_note (ch->pcdata->in_progress);
            ch->pcdata->in_progress = NULL;
            ch->desc->connected = CON_PLAYING;
            /* remove afk status */
            do_afk( ch, "" );
            char_from_room( ch );
            char_to_room( ch, ch->note_room );
            ch->note_room = NULL;
            sprintf(buf, "%s has finished writing a note.", ch->name);
            wiznet(buf, NULL, NULL, WIZ_NOTES, 0, 0);
            REMOVE_BIT(ch->comm, COMM_NOTE_WRITE);
            break;

        default: /* invalid response */
            Cprintf(ch, "Huh? Valid answers are:\n\r\n\r");
            Cprintf(ch, szFinishPrompt);
            Cprintf(ch, "\n\r");
    }
}
