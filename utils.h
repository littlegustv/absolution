/*
 * utility functions
 */
#ifndef UTILS_H
#define UTILS_H

AREA_DATA* get_vnum_area(int n);
void interp_olc (CHAR_DATA *ch, char *argument);
bool IS_BUILDER(CHAR_DATA *victim, AREA_DATA *Area);
char *crypt(const char *key, const char *salt);
bool str_cmp(const char *astr, const char *bstr);
bool str_prefix(const char *astr, const char *bstr);
bool str_infix(const char *astr, const char *bstr);
bool str_suffix(const char *astr, const char *bstr);
char *capitalize(const char *str);
void sprintf_cat(char *target, const char *txt, ...);
char * capitalizeWords(const char *str);
void bug(const char *str, ...);
void log_string(const char *str, ...);
void lowercase(char *target, const char *original);
int str_len(const char *string);

extern FILE *fpArea;
extern char strArea[MAX_INPUT_LENGTH];

#endif
