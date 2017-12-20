/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
 ***************************************************************************/

/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@pacinfo.com)				   *
*	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
*	    Brian Moore (rom@rom.efn.org)				   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "utils.h"


// We need this stupid crap to stop people from crashing us
long string_length(char *string) {
        char* i;
        int k = 0;

        for(i = string, k = 0; string[k] != '\n'; i++) {
                k++;
        }

        return (long)(i - string);
}

/* does aliasing and other fun stuff */
void
substitute_alias (DESCRIPTOR_DATA * d, char *argument)
{
   CHAR_DATA *ch;
   char buf[MAX_STRING_LENGTH], prefix[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
   char *point;
   bool alias_found = FALSE;
   int alias;
   int macro;
   bool isMacro;
   bool macroRunning = FALSE;
   INSTANCE_DATA *macroInstance;
   char *firstInstanceName;
   char *restOfLine;

/*log_string("sub alias"); */
   ch = d->original ? d->original : d->character;

   /* check for prefix */
   if (ch->prefix[0] != '\0' && str_prefix ("prefix", argument))
   {
      if (strlen (ch->prefix) + strlen (argument) > MAX_INPUT_LENGTH)
	 Cprintf(ch, "Line to long, prefix not processed.\r\n");
      else
      {
	 sprintf (prefix, "%s %s", ch->prefix, argument);
	 argument = prefix;
      }
   }				/*
				   log_string("prefix checked"); */
   if (IS_NPC (ch)
       || ((ch->pcdata->alias[0] == NULL)
       && ( ch->pcdata->macro[0].name == NULL ) )
       || !str_prefix ("alias", argument)
       || !str_prefix ("una", argument)
       || !str_prefix ("prefix", argument))
   {
      if (IS_SET (d->character->wiznet, WIZ_OLC))
      {
	 interp_olc (d->character, argument);
      }
      else
      {
	 interpret (d->character, argument);
      }
      return;
   }



   strcpy (buf, argument);
   for (alias = 0; alias < MAX_ALIAS; alias++)	/* go through the aliases */
   {
      if (ch->pcdata->alias[alias] == NULL)
	 break;

      if (!str_prefix (ch->pcdata->alias[alias], argument))
      {
	 point = one_argument (argument, name);
	 if (!strcmp (ch->pcdata->alias[alias], name))
	 {
	    alias_found = TRUE;
	    buf[0] = '\0';
	    strcat (buf, ch->pcdata->alias_sub[alias]);
	    strcat (buf, " ");
	    strcat (buf, point);
	    break;
	 }
	 if (strlen (buf) > MAX_INPUT_LENGTH)
	 {
	    Cprintf(ch, "Alias substitution too long. Truncated.\r\n");
	    buf[MAX_INPUT_LENGTH - 1] = '\0';
	 }
      }
   }
   if (alias_found)
   {
      interpret (d->character, buf);
      return;
   }

   /* macro */
    isMacro = FALSE;
/* Commented out June 2001 Due to crashing on channels */
    restOfLine = one_argument(argument, name);
    for (macro = 0; macro < MAX_NUM_MACROS; macro++)
      {
        if (ch->pcdata->macro[macro].name == NULL)
          {
            break;
          }

        if (strcmp(ch->pcdata->macro[macro].name, name) == 0)
          {
            isMacro = TRUE;
            macroRunning = FALSE;

            // to make life easier, only one instance of any macro
            //  can be running AS THE TOP LEVEL MACRO.  for example:
            //  assume the following macro definitions:
            //  macro1 = command1; command2; macro2; command3
            //  macro2 = command4; command5
            //  now, you can call macro1 once, and you can call macro2 once,
            //  and only once, from the game prompt.  macro1 is unrestricted
            //  in how often it calls macro2, and you can call macro2 from
            //  the game prompt even if macro1 is currently running macro2.

            macroInstance = ch->pcdata->macroInstances;
            if (macroInstance)
              {
                firstInstanceName = str_dup(macroInstance->macroName);
                while ((strcmp(macroInstance->macroName, name             )) &&
                       (strcmp(macroInstance->macroName, firstInstanceName))   )
                  {
                    macroInstance = macroInstance->next;
                  }

                if (strcmp(macroInstance->macroName, name) == 0)
                  {
                    macroRunning = TRUE;
                    break;
                  }
              }

            break;
          }
      }

    if (isMacro)
      {
        if (macroRunning)
          {
            Cprintf(ch, "'%s' is already running.\n\r", name);
          }
        else
          {
            add_instance(ch, macro, restOfLine);
          }
      }

  if( !isMacro )
  {
   if (IS_SET (d->character->wiznet, WIZ_OLC))
      interp_olc (d->character, argument);
   else
      interpret (d->character, argument);
  }
}

void
do_alia (CHAR_DATA * ch, char *argument)
{
   Cprintf(ch, "I'm sorry, alias must be entered in full.\n\r");
   return;
}

void
do_alias (CHAR_DATA * ch, char *argument)
{
   CHAR_DATA *rch;
   char arg[MAX_INPUT_LENGTH];
   int pos;

   if (ch->desc == NULL)
      rch = ch;
   else
      rch = ch->desc->original ? ch->desc->original : ch;

   if (IS_NPC (rch))
      return;

   argument = one_argument (argument, arg);

   smash_tilde (argument);
   smash_tilde (arg);

   if (arg[0] == '\0')
   {

      if (rch->pcdata->alias[0] == NULL)
      {
	 Cprintf(ch, "You have no aliases defined.\n\r");
	 return;
      }
      Cprintf(ch, "Your current aliases are:\n\r");

      for (pos = 0; pos < MAX_ALIAS; pos++)
      {
	 if (rch->pcdata->alias[pos] == NULL || rch->pcdata->alias_sub[pos] == NULL)
	    break;

	 Cprintf(ch, "    %s:  %s\n\r", rch->pcdata->alias[pos], rch->pcdata->alias_sub[pos]);
      }
      return;
   }

   if (!str_prefix ("una", arg) || !str_cmp ("alias", arg) || !str_prefix ("prompt", arg))
   {
      Cprintf(ch, "Sorry, that word is reserved.\n\r");
      return;
   }

   /* stop the nasty bug that comes up with people doing something like:
    * alias a prompt ....
    *
    * A kluge, I know, but for now it's good enough.
   {
      char kluge1[MAX_INPUT_LENGTH], kluge2[MAX_INPUT_LENGTH];

      strcpy(kluge1, argument);
      one_argument(kluge1, kluge2);

      if (!str_prefix(kluge2, "prompt"))
      {
         Cprintf(ch, "Sorry, but you're not allowed to alias that word.  I'm too lazy to fix the bug.\n\r");
         return;
      }

   }

    */
   if (argument[0] == '\0')
   {
      for (pos = 0; pos < MAX_ALIAS; pos++)
      {
	 if (rch->pcdata->alias[pos] == NULL
	     || rch->pcdata->alias_sub[pos] == NULL)
	    break;

	 if (!str_cmp (arg, rch->pcdata->alias[pos]))
	 {
	    Cprintf(ch, "%s aliases to '%s'.\n\r", rch->pcdata->alias[pos], rch->pcdata->alias_sub[pos]);
	    return;
	 }
      }

      Cprintf(ch, "That alias is not defined.\n\r");
      return;
   }

   if (!str_prefix (argument, "delete") || !str_cmp (argument, "remort") || !str_prefix (argument, "prefix"))
   {
      Cprintf(ch, "That shall not be done!\n\r");
      return;
   }

   for (pos = 0; pos < MAX_ALIAS; pos++)
   {
      if (rch->pcdata->alias[pos] == NULL)
	 break;

      if (!str_cmp (arg, rch->pcdata->alias[pos]))	/* redefine an alias */
      {
	 free_string (rch->pcdata->alias_sub[pos]);
	 rch->pcdata->alias_sub[pos] = str_dup (argument);
	 Cprintf(ch, "%s is now realiased to '%s'.\n\r", arg, argument);
	 return;
      }
   }

   if (pos >= MAX_ALIAS)
   {
      Cprintf(ch, "Sorry, you have reached the alias limit.\n\r");
      return;
   }

   /* make a new alias */

   rch->pcdata->alias[pos] = str_dup (arg);
   rch->pcdata->alias_sub[pos] = str_dup (argument);
   Cprintf(ch, "%s is now aliased to '%s'.\n\r", arg, argument);
}


void
do_unalias (CHAR_DATA * ch, char *argument)
{
   CHAR_DATA *rch;
   char arg[MAX_INPUT_LENGTH];
   int pos;
   bool found = FALSE;

   if (ch->desc == NULL)
      rch = ch;
   else
      rch = ch->desc->original ? ch->desc->original : ch;

   if (IS_NPC (rch))
      return;

   argument = one_argument (argument, arg);

   if (*arg == '\0')
   {
      Cprintf(ch, "Unalias what?\n\r");
      return;
   }

   for (pos = 0; pos < MAX_ALIAS; pos++)
   {
      if (rch->pcdata->alias[pos] == NULL)
	 break;

      if (found)
      {
	 rch->pcdata->alias[pos - 1] = rch->pcdata->alias[pos];
	 rch->pcdata->alias_sub[pos - 1] = rch->pcdata->alias_sub[pos];
	 rch->pcdata->alias[pos] = NULL;
	 rch->pcdata->alias_sub[pos] = NULL;
	 continue;
      }

      if (!strcmp (arg, rch->pcdata->alias[pos]))
      {
	 Cprintf(ch, "Alias removed.\n\r");
	 free_string (rch->pcdata->alias[pos]);
	 free_string (rch->pcdata->alias_sub[pos]);
	 rch->pcdata->alias[pos] = NULL;
	 rch->pcdata->alias_sub[pos] = NULL;
	 found = TRUE;
      }
   }

   if (!found)
      Cprintf(ch, "No alias of that name to remove.\n\r");
}
