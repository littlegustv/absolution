#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>


#include "merc.h"
#include "utils.h"

/*
 * Compare strings, case insensitive.
 * Return TRUE if different
 *   (compatibility with historical functions).
 */
bool
str_cmp(const char *astr, const char *bstr) {
    if (astr == NULL && bstr == NULL) {
        bug("str_cmp: null astr and null bstr", 0);
        return TRUE;
    }

    if (astr == NULL) {
        bug("str_cmp: null astr, bstr is %s", bstr);
        return TRUE;
    }

    if (bstr == NULL) {
        bug("str_cmp: astr is %s, null bstr.", astr);
        return TRUE;
    }

    for (; *astr || *bstr; astr++, bstr++) {
        if (LOWER(*astr) != LOWER(*bstr)) {
            return TRUE;
        }
    }

    return FALSE;
}

char *crypt(const char *key, const char *salt) {
    return salt;
}


bool IS_BUILDER(CHAR_DATA *victim, AREA_DATA *Area) {
    return TRUE;
}

void interp_olc (CHAR_DATA *ch, char *argument) {
    return;
}

AREA_DATA* get_vnum_area(int n) {
    return NULL;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool
str_prefix(const char *astr, const char *bstr) {
    if (astr == NULL) {
        bug("str_prefix: null astr.");
        return TRUE;
    }

    if (bstr == NULL) {
        bug("str_prefix: null bstr.");
        return TRUE;
    }

    if (astr[0] == '\0' && bstr[0] != '\0') {
        return TRUE;
    }

    if (astr[0] != '\0' && bstr[0] == '\0') {
        return TRUE;
    }

    for (; *astr; astr++, bstr++) {
        if (LOWER(*astr) != LOWER(*bstr)) {
            return TRUE;
        }
    }

    return FALSE;
}



/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool
str_infix(const char *astr, const char *bstr) {
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ((c0 = LOWER(astr[0])) == '\0') {
        return FALSE;
    }

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for (ichar = 0; ichar <= sstr2 - sstr1; ichar++) {
        if (c0 == LOWER(bstr[ichar]) && !str_prefix(astr, bstr + ichar)) {
            return FALSE;
        }
    }

    return TRUE;
}



/*
 * Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool
str_suffix(const char *astr, const char *bstr) {
    int sstr1;
    int sstr2;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    if (sstr1 <= sstr2 && !str_cmp(astr, bstr + sstr2 - sstr1)) {
        return FALSE;
    } else {
        return TRUE;
    }
}



/*
 * Returns an initial-capped string.
 */
char *
capitalize(const char *str) {
    static char strcap[MAX_STRING_LENGTH];
    int i;

    for (i = 0; str[i] != '\0'; i++) {
        strcap[i] = LOWER(str[i]);
    }

    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);

    return strcap;
}

/*
 * A combination of sprintf and strcat -- it takes a format string and arguments
 * like sprintf, but, instead of over-writing the target string, it appends
 * to the end of it.
 */
void
sprintf_cat(char *target, const char *txt, ...) {
    int startIndex;
    va_list ap;
    va_start(ap, txt);

    startIndex = strlen(target);

    vsprintf(target + startIndex, txt, ap);

    va_end(ap);
}

/*
 * Returns a string where every "word" has been capitalized.
 */
char *
capitalizeWords(const char *str) {
    static char strcap[MAX_STRING_LENGTH];
    int i;
    bool newWord = TRUE;

    for (i = 0; str[i] != '\0'; i++) {
        if (isalpha(str[i])) {
            if (newWord) {
                strcap[i] = UPPER(str[i]);
                newWord = FALSE;
            } else {
                strcap[i] = LOWER(str[i]);
            }
        } else {
            strcap[i] = str[i];
            newWord = TRUE;
        }
    }

    strcap[i] = '\0';

    return strcap;
}

/*
 * Reports a bug.
 */
void
bug(const char *str, ...) {
    char buf[MAX_STRING_LENGTH];
    int startIndex;
    va_list ap;

    va_start(ap, str);

    if (fpArea != NULL) {
        int iLine;
        int iChar;

        if (fpArea == stdin) {
            iLine = 0;
        } else {
            iChar = ftell(fpArea);
            fseek(fpArea, 0, 0);

            for (iLine = 0; ftell(fpArea) < iChar; iLine++) {
                while (getc(fpArea) != '\n')
                    ;
            }

            fseek(fpArea, iChar, 0);
        }

        sprintf(buf, "[*****] FILE: %s LINE: %d", strArea, iLine);
        log_string("%s", buf);
    }

    strcpy(buf, "[*****] BUG: ");

    startIndex = strlen(buf);

    vsprintf(buf + startIndex, str, ap);

    va_end(ap);

    log_string("%s", buf);

    return;
}

/*
 * Writes a string to the log.
 */
void
log_string(const char *str, ...) {
    va_list ap;
    char *strtime;
    char buf[MAX_STRING_LENGTH];
    time_t logTime;

    va_start(ap, str);

    logTime = time(NULL);

    strtime = ctime(&logTime);
    strtime[strlen(strtime) - 1] = '\0';

    sprintf(buf, "%s :: %s\n", strtime, str);
    vfprintf(stderr, buf, ap);

    va_end(ap);

    return;
}

// Convert a string to lower case
void
lowercase(char *target, const char *original) {
    int idx;

    if (!original) {
        return;
    }

    idx = 0;
    for (; *original; original++) {
        *target = tolower(*original);
        target++;
    }

    *target = '\0';
}

// a color-code aware version of str_len
int
str_len(const char *string) {
    int length = 0;

    for (; *string; string++) {
        if (*string == '{') {
            string++;
            if (*string == '{') {
                length++;
            } else if (!*string) {
                // Prevent an over-run on a dangling '{'
                length ++;

                break;
            }
        } else {
            length++;
        }
    }

    return length;
}
