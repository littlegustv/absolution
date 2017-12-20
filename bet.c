#include <ctype.h>

#include "merc.h"
#include "bet.h"

int
advatoi (const char *s)
{

   char string[MAX_INPUT_LENGTH];	/* a buffer to hold a copy of the argument */
   char *stringptr = string;	/* a pointer to the buffer so we can move around */
   char tempstring[2];		/* a small temp buffer to pass to atoi */
   int number = 0;		/* number to be returned */
   int multiplier = 0;		/* multiplier used to get the extra digits right */


   strcpy (string, s);		/* working copy */

   while (isdigit (*stringptr))	/* as long as the current character is a digit */
   {
      strncpy (tempstring, stringptr, 1);	/* copy first digit */
      number = (number * 10) + atoi (tempstring);	/* add to current number */
      stringptr++;		/* advance */
   }

   switch (UPPER (*stringptr))
   {
   case 'K':
      multiplier = 1000;
      number *= multiplier;
      stringptr++;
      break;
   case 'M':
      multiplier = 1000000;
      number *= multiplier;
      stringptr++;
      break;
   case '\0':
      break;
   default:
      return 0;			/* not k nor m nor NUL - return 0! */
   }

   while (isdigit (*stringptr) && (multiplier > 1))	/* if any digits follow k/m, add those too */
   {
      strncpy (tempstring, stringptr, 1);	/* copy first digit */
      multiplier = multiplier / 10;	/* the further we get to right, the less are the digit 'worth' */
      number = number + (atoi (tempstring) * multiplier);
      stringptr++;
   }

   if (*stringptr != '\0' && !isdigit (*stringptr))	/* a non-digit character was found, other than NUL */
      return 0;			/* If a digit is found, it means the multiplier is 1 - i.e. extra
				   digits that just have to be ignore, liked 14k4443 -> 3 is ignored */


   return (number);
}

int
parsebet (const int currentbet, const char *argument) {
	/* a variable to temporarily hold the new bet */
	int newbet = 0;

	/* a buffer to modify the bet string */
	char string[MAX_INPUT_LENGTH];

	/* a pointer we can move around */
	char *stringptr = string;

	/* make a work copy of argument */
	strcpy (string, argument);

	/* check for an empty string */
	if (*stringptr) {
		/* first char is a digit assume e.g. 433k */
		if (isdigit (*stringptr)) {
			/* parse and set newbet to that value */
			newbet = advatoi (stringptr);
		} else if (*stringptr == '+') {
			/* only + specified, assume default */
			if (strlen (stringptr) == 1) {
				/* default: add 25% */
				newbet = (currentbet * 125) / 100;
			} else {
				/* cut off the first char */
				newbet = (currentbet * (100 + atoi (++stringptr))) / 100;
			}
		} else {
			printf ("considering: * x \n\r");

			/* multiply */
			if ((*stringptr == '*') || (*stringptr == 'x')) {
				/* only x specified, assume default */
				if (strlen (stringptr) == 1) {
					/* default: twice */
					newbet = currentbet * 2;
				} else {
					/* user specified a number */
					/* cut off the first char */
					newbet = currentbet * atoi (++stringptr);
				}
			}
		}
	}

	/* return the calculated bet */
	return newbet;
}
