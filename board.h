/* Includes for board system */
/* This is version 2 of the board system, (c) 1995-96 erwin@pip.dknet.dk */

#ifndef BOARD_H
#define BOARD_H

#define CON_NOTE_TO               20
#define CON_NOTE_SUBJECT            21
#define CON_NOTE_EXPIRE               22
#define CON_NOTE_TEXT               23
#define CON_NOTE_FINISH               24

#define NOTE_DIR              "../note/" /* set it to something you like */

#define DEF_NORMAL  0 /* No forced change, but default (any string)   */
#define DEF_INCLUDE 1 /* 'names' MUST be included (only ONE name!)    */
#define DEF_EXCLUDE 2 /* 'names' must NOT be included (one name only) */

#define MAX_LINE_LENGTH 80 /* enforce a max length of 80 on text lines, reject longer lines */
                           /* This only applies in the Body of the note */

#define MAX_NOTE_TEXT (4 * MAX_STRING_LENGTH - 1000)

#define BOARD_NOTFOUND -1 /* Error code from board_lookup() and board_number */

typedef struct board_data BOARD_DATA;

/* Data about a board */
struct board_data {
    // Max 16 chars
    char *name;

    // Explanatory text, should be no more than 60 chars
    char *description;

    // minimum level to see board
    int readLevel;

    // minimum level to post notes
    int writeLevel;

    // Default recipient
    char *names;

    // Recipient restrictions (based on default recipient)
    int nameRestriction;

    // Default expiration
    int defaultExpire;

    // Which clan is the board restricted to (if any)
    char *clan;

    // Is this the default note board?
    bool isDefault;

    // Is this note board enabled?
    bool enabled;

    // The name of the file for this board
    char *filename;

    /* Non-constant data */

    // pointer to board's first note
    NOTE_DATA *note_first;

    // currently unused
    bool changed;

    // Next board in the list
    BOARD_DATA *next;
};

typedef struct note_read_data NOTE_READ_DATA;

struct note_read_data {
    char *boardName;
    time_t timestamp;
    NOTE_READ_DATA *next;
};





/* External variables */
extern BOARD_DATA *board_first;

/* Prototypes */

void finish_note (BOARD_DATA *board, NOTE_DATA *note); /* attach a note to a board */
void free_note   (NOTE_DATA *note); /* deallocate memory used by a note */
void load_boards (void); /* load all boards */
BOARD_DATA* board_lookup (const char *name); /* Find a board with that name */
BOARD_DATA* getBoard(const int boardNumber);
bool is_note_to (CHAR_DATA *ch, NOTE_DATA *note); /* is tha note to ch? */
NOTE_DATA *make_note (const char* board_name, const char *sender, const char *to, const char *subject, const int expire_days, const char *text);
void save_notes ();
BOARD_DATA* getDefaultBoard();
void load_board(BOARD_DATA *board);

/* for nanny */
void handle_con_note_to(CHAR_DATA *, char *);
void handle_con_note_subject(CHAR_DATA *, char *);
void handle_con_note_expire(CHAR_DATA *, char *);
void handle_con_note_text(CHAR_DATA *, char *);
void handle_con_note_finish(CHAR_DATA *, char *);


/* stufff &*/
void do_board( CHAR_DATA *ch, char* arg );

#endif
