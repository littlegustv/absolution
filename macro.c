/*                                                                            *
 * *                                                                        * *
 * * Soth's Macro Stuff.  v 1.0                                             * *
 * * written for Redemption MUD, early '97                                  * *
 * * copyright and all that legal garbage. :-p                              * *
 * *                                                                        * *
 *                                                                            */

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "merc.h"
#include "recycle.h"
#include "utils.h"


/* do_macro is the launch point for any of the 'macro ' commands.
 */
void
do_macro (CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *rch;
   char       arg[MAX_INPUT_LENGTH];

   smash_tilde(argument);

   /* ensure that 'ch' is the body that the messages go to
    * and 'rch' is the soul that the commands affect.
    */

   rch = (ch->desc) ? ((ch->desc->original) ? ch->desc->original : ch) : ch;

   if (!IS_NPC(rch))
   {
      argument = macro_one_argument(argument, arg);

      /* if they entered 'macro' then they want help */
      if (strlen(arg) == 0)
      {
         macro_help(ch);
      }
      else if (str_prefix(arg, "list") == 0)
      {
         macro_list(ch, rch, argument);
      }
      else if (str_prefix(arg, "define") == 0)
      {
         macro_define(ch, rch, argument);
      }
      else if (str_prefix(arg, "undefine") == 0)
      {
         macro_undefine(ch, rch, argument);
      }
      else if (str_prefix(arg, "trace") == 0)
      {
         macro_trace(ch, rch, argument);
      }
      else if (str_prefix(arg, "join") == 0)
      {
         macro_join(ch, rch, argument);
      }
      else if (str_prefix(arg, "remove") == 0)
      {
         macro_remove(ch, rch, argument);
      }
      else if (str_prefix(arg, "rename") == 0)
      {
         macro_rename(ch, rch, argument);
      }
      else if (str_prefix(arg, "renameU") == 0)
      {
         macro_renameU(ch, rch, argument);
      }
      else if (str_prefix(arg, "insert") == 0)
      {
         macro_insert(ch, rch, argument);
      }
      else if (str_prefix(arg, "append") == 0)
      {
         macro_append(ch, rch, argument);
      }
      else if (str_prefix(arg, "replace") == 0)
      {
         macro_replace(ch, rch, argument);
      }
      else if (str_prefix(arg, "kill") == 0)
      {
         macro_kill(ch, rch, argument);
      }
      else if (str_prefix(arg, "running") == 0)
      {
         macro_running(ch, rch, argument);
      }
      else if (str_prefix(arg, "status") == 0)
      {
         macro_status(ch, rch, argument);
      }
      else
      {
         Cprintf(ch, "Eh, watsat?  Try 'macro'. *hrumph*\n\r");
      }
   }

   return;
}

void
macro_help(CHAR_DATA *ch)
{
   Cprintf(ch, "macro list      :     list your macros\n\r");
   Cprintf(ch, "macro define    :     define a macro\n\r");
   Cprintf(ch, "macro undefine  :     undefine a macro\n\r");
   Cprintf(ch, "macro trace     :     trace execution path\n\r");
   Cprintf(ch, "macro join      :     catenate two macros\n\r");
   Cprintf(ch, "macro remove    :     remove command from macro\n\r");
   Cprintf(ch, "macro rename    :     rename a macro\n\r");
   Cprintf(ch, "macro renameU   :     rename with update\n\r");
   Cprintf(ch, "macro insert    :     insert a command into a macro\n\r");
   Cprintf(ch, "macro append    :     append a command to a macro\n\r");
   Cprintf(ch, "macro replace   :     replace commands in a macro\n\r");
   Cprintf(ch, "macro kill      :     kill currently running macro\n\r");
   Cprintf(ch, "macro running   :     list all running macros\n\r");
   Cprintf(ch, "macro status    :     stats on macro\n\r");

   return;
}

void
macro_list(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   bool more = TRUE;
   int  pos;

   if (rch->pcdata->macro[0].name == NULL)
   {
      Cprintf(ch, "You have no macros defined.\n\r");
   }
   else
   {
      Cprintf(ch, "Your current macros are:\n\r");

      for (pos = 0; (pos < MAX_NUM_MACROS) && (more); pos++)
      {
         if (rch->pcdata->macro[pos].name != NULL)
         {
            Cprintf(ch, "    %s:  %s\n\r", rch->pcdata->macro[pos].name,
                                            rch->pcdata->macro[pos].definition);
         }
         else
         {
            more = FALSE;
         }
      }
   }

   return;
}

void
macro_define(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   int   num;
   int   pos;
   int   nonSeperator = 0;
   char *temp;
   char  arg[MAX_INPUT_LENGTH];

   argument = macro_one_argument(argument, arg);

   temp = argument;

   /* first make sure that they aren't trying for an empty definition
    */
   while (*temp)
   {
      if ((*temp != ';') && (*temp != ' '))
      {
         nonSeperator++;
      }

      temp++;
   }

   if ((strlen(arg) == 0) || (nonSeperator == 0))
   {
      Cprintf(ch, "macro define <name> <commands>\n\r");
      return;
   }

   /* next, make sure you aren't trying to redefine a currently
    * running macro.
    */
   if (macro_is_running(ch, rch, arg))
   {
      return;
   }

   if ((!str_cmp("all", arg)) || (!str_cmp("macro", arg)))
   {
      /* can't define 'all' and 'macro' as macros
       */
      Cprintf(ch, "Sorry, that word is reserved.\n\r");
   }
   else if (str_prefix("macro", arg) == 0)
   {
      /* nor can we have a macro name for which "macro" would be
       * short form.
       */
      Cprintf(ch, "Sorry, that word can't be macro'd.\n\r");
   }
   else
   {
      for (pos = 0; pos < MAX_NUM_MACROS; pos++)
      {
         if (rch->pcdata->macro[pos].name == NULL)
         {
            break;
         }

         if (!str_cmp(arg, rch->pcdata->macro[pos].name))
         {
            /* the macro name is already defined, so this is
             * a redefinition.
             */
            free_string(rch->pcdata->macro[pos].definition);
            rch->pcdata->macro[pos].definition = str_dup(argument);

            /* the dual call because it cleans up the definition
             * (gets rid of extra ;'s and spaces....)
             */
            num = def_to_macro(rch, arg);
            macro_to_def(rch, arg);

            Cprintf(ch, "'%s' is now redefined as '%s'.\n\r", arg, rch->pcdata->macro[pos].definition);

            if (num > MAX_NUM_SUB_PER_MACRO)
            {
               Cprintf(ch, "Only the first %d commands were used.\n\r",
                           MAX_NUM_SUB_PER_MACRO);
            }

            return;
         }
      }

      /* defining a new macro
       */
      if (pos >= MAX_NUM_MACROS)
      {
         Cprintf(ch, "Sorry, you have reached your macro limit.\n\r");
         return;
      }

      rch->pcdata->macro[pos].name = str_dup(arg);
      rch->pcdata->macro[pos].definition = str_dup(argument);

      num = def_to_macro(rch, arg);
      macro_to_def(rch, arg);

      Cprintf(ch, "'%s' is now defined as '%s'.\n\r",
                   arg, rch->pcdata->macro[pos].definition);

      if (num > MAX_NUM_SUB_PER_MACRO)
      {
         Cprintf(ch, "Only the first %d commands were used.\n\r",
                      MAX_NUM_SUB_PER_MACRO);
      }
   }

   return;
}

void macro_undefine(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   char macroName[MAX_INPUT_LENGTH];
   int  pos;
   int  sub;
   bool found = FALSE;

   argument = macro_one_argument(argument, macroName);

   if (strlen(macroName) == 0)
   {
      Cprintf(ch, "macro undefine <name>\n\r");
      Cprintf(ch, "macro undefine all\n\r");
      return;
   }

   /* check if there are any running macros
    */
   if (rch->pcdata->macroInstances != NULL)
   {
      Cprintf(ch, "Please kill all running macros first.\n\r");
      return;
   }

   if (!str_cmp(macroName, "all"))
   {
      for (pos = 0; pos < MAX_NUM_MACROS; pos++)
      {
         if (rch->pcdata->macro[pos].name == NULL)
         {
            break;
         }

         free_string(rch->pcdata->macro[pos].name);
         rch->pcdata->macro[pos].name = NULL;
         free_string(rch->pcdata->macro[pos].definition);
         rch->pcdata->macro[pos].definition = NULL;

         for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
         {
            free_string(rch->pcdata->macro[pos].subs[sub]);
            rch->pcdata->macro[pos].subs[sub] = NULL;
         }
      }
      Cprintf(ch, "All macros undefined.\n\r");
   }
   else
   {
      /* make sure that the macro isn't running */
      if (macro_is_running(ch, rch, macroName))
      {
         return;
      }

      for (pos = 0; pos < MAX_NUM_MACROS; pos++)
      {
         if (rch->pcdata->macro[pos].name == NULL)
         {
            break;
         }

         if (found)
         {
            rch->pcdata->macro[pos-1].name = rch->pcdata->macro[pos].name;
            rch->pcdata->macro[pos-1].definition =
                                            rch->pcdata->macro[pos].definition;
            for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
            {
               rch->pcdata->macro[pos-1].subs[sub] =
                                             rch->pcdata->macro[pos].subs[sub];
            }

            rch->pcdata->macro[pos].name = NULL;
            rch->pcdata->macro[pos].definition = NULL;
            for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
            {
               rch->pcdata->macro[pos].subs[sub] = NULL;
            }

            continue;
         }

         if (!str_cmp(macroName, rch->pcdata->macro[pos].name))
         {
            Cprintf(ch, "Macro undefined.\n\r");
            free_string(rch->pcdata->macro[pos].name);
            free_string(rch->pcdata->macro[pos].definition);
            for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
            {
               free_string(rch->pcdata->macro[pos].subs[sub]);
            }

            rch->pcdata->macro[pos].name = NULL;
            rch->pcdata->macro[pos].definition = NULL;
            for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
            {
               rch->pcdata->macro[pos].subs[sub] = NULL;
            }
            found = TRUE;
         }
      }

      if (!found)
      {
         Cprintf(ch, "That macro is not defined.\n\r");
      }
   }

   return;
}

void macro_trace(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   char    macroName[MAX_INPUT_LENGTH];
   char    buf[MAX_STRING_LENGTH];
   int     pos;
   bool    found = FALSE;
   int     used[MAX_NUM_MACROS] = {0};
   BUFFER *output;
   int     comCount = 0;

   macro_one_argument(argument, macroName);

   if (strlen(macroName) == 0)
   {
      Cprintf(ch, "macro trace <name>\n\r");
      return;
   }

   for (pos = 0; pos < MAX_NUM_MACROS; pos++)
   {
      if ((rch->pcdata->macro[pos].name == NULL) || (found))
      {
         break;
      }

      if (!str_cmp(rch->pcdata->macro[pos].name, macroName))
      {
         found = TRUE;
      }
   }

   if (found)
   {
      output = new_buf();
      sprintf(buf, "%s\n\r", macroName);
      add_buf(output, buf);
      macro_traceR( rch, macroName, 1, used, output, &comCount);
      page_to_char(buf_string(output), ch);
      free_buf(output);
   }
   else
   {
      Cprintf(ch, "'%s' is not defined.\n\r", macroName);
   }

   return;
}

int macro_traceR(CHAR_DATA *rch, char *macroName, int stage, int *used, BUFFER *output, int *comCount)
{
   bool recursive = FALSE;
   char buf[MAX_STRING_LENGTH];
   char indent[MAX_STRING_LENGTH];
   char subCommand[MAX_INPUT_LENGTH];
   int  pos;
   int  newPos;
   int  sub;
   int  steps;
   bool found;

   strcpy(indent, "");
   for (steps = 0; steps < stage; steps++)
   {
      strcat(indent, "  ");
   }

   for (pos = 0; pos < MAX_NUM_MACROS; pos++)
   {
      if (!str_cmp(rch->pcdata->macro[pos].name, macroName))
      {
         break;
      }
   }

   *(used + pos) = 1;

   for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
   {
      found = FALSE;
      if ((rch->pcdata->macro[pos].subs[sub] == NULL) || (recursive))
      {
         break;
      }

      *comCount = *comCount + 1;

      if (*comCount > MAX_NUM_COM_PER_TRACE)
      {
         sprintf(buf, "First %d commands traced.\n\r", *comCount - 1);
         add_buf(output, buf);
         recursive = TRUE;
         break;
      }

      for (newPos = 0; newPos < MAX_NUM_MACROS; newPos++)
      {
         if (rch->pcdata->macro[newPos].name == NULL)
         {
            break;
         }

         macro_one_argument(rch->pcdata->macro[pos].subs[sub], subCommand);

         if (!str_cmp(rch->pcdata->macro[newPos].name, subCommand))
         {
            found = TRUE;
            break;
         }
      }

      sprintf(buf, "%s(%s %d): %s\n\r", indent, macroName, sub + 1,
                                        rch->pcdata->macro[pos].subs[sub]);
      add_buf(output, buf);

      if (found)
      {
         if (*(used + newPos))
         {
            add_buf(output, "\n\r");
            add_buf(output, "And this is where it gets recursive.\n\r");
            recursive = TRUE;
            break;
         }

         add_buf(output, "\n\r");
         recursive = macro_traceR(rch, subCommand, stage + 1, used, output,
                                                                      comCount);
         if ( (!recursive) && (sub + 1 < MAX_NUM_SUB_PER_MACRO) &&
              (rch->pcdata->macro[pos].subs[sub + 1] != NULL)        )
         {
            add_buf(output, "\n\r");
         }
      }
   }

   *(used + pos) = 0;

   return (recursive);
}

void macro_join(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   int  pos1;
   int  pos2;
   int  sub1;
   int  sub2;
   int  num_commands1 = 0;
   int  num_commands2 = 0;
   int  num_new_commands;
   char macro1_name[MAX_INPUT_LENGTH];
   char macro2_name[MAX_INPUT_LENGTH];
   bool found = FALSE;

   argument = macro_one_argument(argument, macro1_name);
   argument = macro_one_argument(argument, macro2_name);

   if ((strlen(macro1_name) == 0) || (strlen(macro2_name) == 0))
   {
      Cprintf(ch, "macro join <macro1> <macro2>\n\r");
      return;
   }

   if (!str_cmp(macro1_name, macro2_name))
   {
      Cprintf(ch, "Cannot join macro to itself.\n\r");
      return;
   }

   /* make sure neither of the two macros are running
    */
   if (macro_is_running(ch, rch, macro1_name) ||
       macro_is_running(ch, rch, macro2_name)   )
   {
      return;
   }

   for (pos1 = 0; pos1 < MAX_NUM_MACROS; pos1++)
   {
      if (rch->pcdata->macro[pos1].name == NULL)
      {
         break;
      }

      if (!str_cmp(rch->pcdata->macro[pos1].name, macro1_name))
      {
         found = TRUE;
         break;
      }
   }

   if (!found)
   {
      Cprintf(ch, "Macro '%s' not defined.\n\r", macro1_name);
      return;
   }

   found = FALSE;

   for (pos2 = 0; pos2 < MAX_NUM_MACROS; pos2++)
   {
      if (rch->pcdata->macro[pos2].name == NULL)
      {
         break;
      }

      if (!str_cmp(rch->pcdata->macro[pos2].name, macro2_name))
      {
         found = TRUE;
         break;
      }
   }

   if (!found)
   {
      Cprintf(ch, "Macro '%s' not defined.\n\r", macro2_name);
      return;
   }

   for (sub1 = MAX_NUM_SUB_PER_MACRO - 1; sub1 > 0; sub1--)
   {
      if (rch->pcdata->macro[pos1].subs[sub1] == NULL)
      {
         num_commands1 = sub1;
      }
      else
      {
         break;
      }
   }

   for (sub2 = MAX_NUM_SUB_PER_MACRO - 1; sub2 > 0; sub2--)
   {
      if (rch->pcdata->macro[pos2].subs[sub2] == NULL)
      {
         num_commands2 = sub2;
      }
      else
      {
         break;
      }
   }

   num_new_commands = num_commands1 + num_commands2;

   if (num_new_commands > MAX_NUM_SUB_PER_MACRO)
   {
      Cprintf(ch, "Joining the two macros would excede the %d command "
                   "limit.\n\r", MAX_NUM_SUB_PER_MACRO);
      return;
   }

   sub1 = num_commands1;

   for (sub2 = 0; sub2 < MAX_NUM_SUB_PER_MACRO; sub2++)
   {
      if (rch->pcdata->macro[pos2].subs[sub2] == NULL)
      {
         break;
      }

      rch->pcdata->macro[pos1].subs[sub1] = rch->pcdata->macro[pos2].subs[sub2];
      sub1++;
   }

   free_string(rch->pcdata->macro[pos2].name);
   free_string(rch->pcdata->macro[pos2].definition);

   rch->pcdata->macro[pos2].name = NULL;
   rch->pcdata->macro[pos2].definition = NULL;
   for (sub1 = 0; sub1 < MAX_NUM_SUB_PER_MACRO; sub1++)
   {
      rch->pcdata->macro[pos2].subs[sub1] = NULL;
   }

   for (pos1 = pos2 + 1; pos1 < MAX_NUM_MACROS; pos1++)
   {
      if (rch->pcdata->macro[pos1].name == NULL)
      {
         break;
      }

      rch->pcdata->macro[pos1 - 1].name = rch->pcdata->macro[pos1].name;
      rch->pcdata->macro[pos1 - 1].definition =
                                           rch->pcdata->macro[pos1].definition;
      for (sub1 = 0; sub1 < MAX_NUM_SUB_PER_MACRO; sub1++)
      {
         rch->pcdata->macro[pos1 - 1].subs[sub1] =
                                           rch->pcdata->macro[pos1].subs[sub1];
      }

      rch->pcdata->macro[pos1].name = NULL;
      rch->pcdata->macro[pos1].definition = NULL;
      for (sub1 = 0; sub1 < MAX_NUM_SUB_PER_MACRO; sub1++)
      {
         rch->pcdata->macro[pos1].subs[sub1] = NULL;
      }
   }

   macro_to_def(rch, macro1_name);

   Cprintf(ch, "Macros joined.\n\r");

   return;
}

void macro_remove(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   char macroName[MAX_INPUT_LENGTH];
   char first_in_range[MAX_INPUT_LENGTH];
   char last_in_range[MAX_INPUT_LENGTH];
   int  first = -1;
   int  last = -1;
   int  pos;
   int  sub;
   bool found = FALSE;

   argument = macro_one_argument(argument, macroName);

   if (strlen(macroName) == 0)
   {
      Cprintf(ch, "macro remove <name> <x> <y>\n\r");
      Cprintf(ch, "macro remove <name> <x>\n\r");
      Cprintf(ch, "both <x> and <y> can be either 'first' or 'last'.\n\r");
      return;
   }

   /* make sure the macro isn't running
    */
   if (macro_is_running(ch, rch, macroName))
   {
      return;
   }

   for (pos = 0; pos < MAX_NUM_MACROS; pos++)
   {
      if (!str_cmp(rch->pcdata->macro[pos].name, macroName))
      {
         found = 1;
         break;
      }
   }

   if (found)
   {
      argument = macro_one_argument(argument, first_in_range);
      argument = macro_one_argument(argument, last_in_range);

      if (strlen(first_in_range) == 0)
      {
         Cprintf(ch, "macro remove <name> <x> <y>\n\r");
         Cprintf(ch, "macro remove <name> <x>\n\r");
         Cprintf(ch, "both <x> and <y> can also be either 'first' or "
                      "'last'.\n\r");
         return;
      }

      if (!str_cmp(first_in_range, "first"))
      {
         first = 1;
      }
      else if (!str_cmp(first_in_range, "last"))
      {
         if (rch->pcdata->macro[pos].subs[MAX_NUM_SUB_PER_MACRO - 1] != NULL)
         {
            first = MAX_NUM_SUB_PER_MACRO;
         }
         else
         {
            for (sub = MAX_NUM_SUB_PER_MACRO; sub > 0; sub--)
            {
               if (rch->pcdata->macro[pos].subs[sub] == NULL)
               {
                  first = sub;
               }
            }
         }
      }

      if (!str_cmp(last_in_range, "first"))
      {
         last = 1;
      }
      else if (!str_cmp(last_in_range, "last"))
      {
         if (rch->pcdata->macro[pos].subs[MAX_NUM_SUB_PER_MACRO - 1] != NULL)
         {
            last = MAX_NUM_SUB_PER_MACRO;
         }
         else
         {
            for (sub = MAX_NUM_SUB_PER_MACRO; sub > 0; sub--)
            {
               if (rch->pcdata->macro[pos].subs[sub] == NULL)
               {
                  last = sub;
               }
            }
         }
      }

      if (first == -1)
      {
         first = atoi(first_in_range);
      }

      if (last == -1)
      {
         if (strlen(last_in_range) != 0)
         {
            last = atoi(last_in_range);
         }
         else
         {
            last = first;
         }
      }

      if (last < first)
      {
         Cprintf(ch, "Invalid range.\n\r");
         return;
      }

      if ((first == 0) || (last == 0))
      {
         Cprintf(ch, "Only numbers from 1 to %d, 'first', and 'last' "
                      "can be used for range.\n\r", MAX_NUM_SUB_PER_MACRO);
         return;
      }

      if ((first > MAX_NUM_SUB_PER_MACRO) || (last > MAX_NUM_SUB_PER_MACRO))
      {
         Cprintf(ch, "Only %d commands in macro.\n\r", MAX_NUM_SUB_PER_MACRO);
         return;
      }

      if ((first == 1) && (rch->pcdata->macro[pos].subs[last] == NULL))
      {
         Cprintf(ch, "Please use 'macro undefine' to undefine macros.\n\r");
         return;
      }

      for (sub = first - 1; sub < last; sub++)
      {
         if (rch->pcdata->macro[pos].subs[sub] == NULL)
         {
            break;
         }

         free_string(rch->pcdata->macro[pos].subs[sub]);
         rch->pcdata->macro[pos].subs[sub] = NULL;
      }

      first = 0;
      for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
      {
         if (rch->pcdata->macro[pos].subs[sub] != NULL)
         {
            rch->pcdata->macro[pos].subs[first] =
                                              rch->pcdata->macro[pos].subs[sub];
            first++;
         }
      }

      for (sub = first; sub < MAX_NUM_SUB_PER_MACRO; sub++)
      {
         rch->pcdata->macro[pos].subs[sub] = NULL;
      }

      macro_to_def(rch, rch->pcdata->macro[pos].name);

      if (first == last)
      {
         Cprintf(ch, "Command removed from macro.\n\r");
      }
      else
      {
         Cprintf(ch, "Commands removed from macro.\n\r");
      }
   }
   else
   {
      Cprintf(ch, "The macro '%s' is not defined.\n\r", macroName);
   }

   return;
 }


void macro_rename(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   char oldName[MAX_INPUT_LENGTH];
   char newName[MAX_INPUT_LENGTH];
   int  pos;
   bool found = FALSE;

   argument = macro_one_argument(argument, oldName);
   argument = macro_one_argument(argument, newName);

   if ((strlen(oldName) == 0) || (strlen(newName) == 0))
   {
      Cprintf(ch, "macro rename <old name> <new name>\n\r");
      return;
   }

   /* make sure that the macro isn't running */
   if (macro_is_running(ch, rch, oldName))
   {
      return;
   }

   for (pos = 0; pos < MAX_NUM_MACROS; pos++)
   {
      if (rch->pcdata->macro[pos].name == NULL)
      {
         break;
      }

      if (!str_cmp(rch->pcdata->macro[pos].name, newName))
      {
         found = TRUE;
         break;
      }
   }

   if (!found)
   {
      for (pos = 0; pos < MAX_NUM_MACROS; pos++)
      {
         if ((rch->pcdata->macro[pos].name == NULL) || (found))
         {
            break;
         }

         if (!str_cmp(oldName, rch->pcdata->macro[pos].name))
         {
            Cprintf(ch, "Macro '%s' renamed to '%s'.\n\r", oldName, newName);

            free_string(rch->pcdata->macro[pos].name);
            rch->pcdata->macro[pos].name = str_dup(newName);

            found = 1;
         }
      }

      if (!found)
      {
        Cprintf(ch, "That macro is not defined.\n\r");
      }
   }
   else
   {
      Cprintf(ch, "The macro '%s' already exists.\n\r", newName);
   }

   return;
}


void macro_renameU(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   int   pos;
   int   replacements;
   int   sub;
   char  oldName[MAX_INPUT_LENGTH];
   char  newName[MAX_INPUT_LENGTH];
   char  subCommand[MAX_INPUT_LENGTH];
   char *restOfSub;
   bool  found = FALSE;

   argument = macro_one_argument(argument, oldName);
   argument = macro_one_argument(argument, newName);

   if ((strlen(oldName) == 0) || (strlen(newName) == 0))
   {
      Cprintf(ch, "macro renameU <old name> <new name>\n\r");
      return;
   }

   /* make sure the macro isn't running */
   if (macro_is_running(ch, rch, oldName))
   {
      return;
   }

   for (pos = 0; pos < MAX_NUM_MACROS; pos++)
   {
      if (rch->pcdata->macro[pos].name == NULL)
      {
         break;
      }

      if (!str_cmp(rch->pcdata->macro[pos].name, newName))
      {
         found = TRUE;
         break;
      }
   }

   if (!found)
   {
      for (pos = 0; pos < MAX_NUM_MACROS; pos++)
      {
         if (rch->pcdata->macro[pos].name == NULL)
         {
            break;
         }

         if (!str_cmp(oldName, rch->pcdata->macro[pos].name))
         {
            Cprintf(ch, "Macro '%s' renamed to '%s'.\n\r", oldName, newName);

            free_string(rch->pcdata->macro[pos].name);
            rch->pcdata->macro[pos].name = str_dup(newName);

            found = TRUE;
            break;
         }
      }

      if (!found)
      {
         Cprintf(ch, "That macro is not defined.\n\r");
      }
      else
      {
         for (pos = 0; pos < MAX_NUM_MACROS; pos++)
         {
            replacements = 0;

            if (rch->pcdata->macro[pos].name == NULL)
            {
               break;
            }

            for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
            {
               if (rch->pcdata->macro[pos].subs[sub] == NULL)
               {
                  break;
               }

               restOfSub = macro_one_argument(rch->pcdata->macro[pos].subs[sub],
                                                                    subCommand);

               if (!str_cmp(subCommand, oldName))
               {
                  sprintf(subCommand, "%s %s", newName, restOfSub);
                  free_string(rch->pcdata->macro[pos].subs[sub]);
                  rch->pcdata->macro[pos].subs[sub] = str_dup(subCommand);
                  replacements++;
               }
            }

            if (replacements == 1)
            {
               Cprintf(ch, "Macro '%s' updated once.\n\r",
                                                  rch->pcdata->macro[pos].name);
            }
            else if (replacements == 2)
            {
               Cprintf(ch, "Macro '%s' updated twice.\n\r",
                                                  rch->pcdata->macro[pos].name);
            }
            else if (replacements)
            {
               Cprintf(ch, "Macro '%s' updated %d times.\n\r",
                                    rch->pcdata->macro[pos].name, replacements);
            }
            macro_to_def(rch, rch->pcdata->macro[pos].name);
            def_to_macro(rch, rch->pcdata->macro[pos].name);
            macro_to_def(rch, rch->pcdata->macro[pos].name);
         }
      }
   }
   else
   {
      Cprintf(ch, "The macro '%s' already exists.\n\r", newName);
   }

   return;
}

void macro_insert(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   int   pos;
   int   sub;
   int   loc = 0;
   int   num_new_commands = 0;
   int   num_current_commands = 0;
   int   num_proposed_commands;
   int   temp_sub;
   char  macroName[MAX_INPUT_LENGTH];
   char  new_commands[MAX_INPUT_LENGTH];
   char  temp[MAX_INPUT_LENGTH];
   char  location[MAX_INPUT_LENGTH];
   char *command;
   char *temp_command_list[MAX_NUM_SUB_PER_MACRO] = {NULL};
   bool  found = FALSE;

   argument = macro_one_argument(argument, macroName);
   argument = macro_one_argument(argument, location);
   argument = macro_one_argument(argument, new_commands);

   if ((strlen(macroName) == 0) || (strlen(location) == 0))
   {
      Cprintf(ch, "macro insert <name> <x> <commands>\n\r");
      Cprintf(ch, "'first' and 'last' are valid values for <x>.\n\r");
      Cprintf(ch, "if <x> is omitted, then the commands are inserted\n\r");
      Cprintf(ch, "into the beginning of the macro.\n\r");
      return;
   }

   /* make sure the macro isn't running */
   if (macro_is_running(ch, rch, macroName))
   {
      return;
   }

   if (strlen(new_commands) == 0)
   {
      strcpy(new_commands, location);
      strcpy(location, "first");
   }

   for (pos = 0; pos < MAX_NUM_MACROS; pos++)
   {
      if (rch->pcdata->macro[pos].name == NULL)
      {
         break;
      }

      if (!str_cmp(rch->pcdata->macro[pos].name, macroName))
      {
         found = TRUE;
         break;
      }
   }

   if (!found)
   {
      Cprintf(ch, "That macro is not defined.\n\r");
      return;
   }

   if (!str_cmp(location, "first"))
   {
      loc = 1;
   }
   else if (!str_cmp(location, "last"))
   {
      if (rch->pcdata->macro[pos].subs[MAX_NUM_SUB_PER_MACRO - 1] != NULL)
      {
         loc = MAX_NUM_SUB_PER_MACRO;
      }
      else
      {
         for (sub = MAX_NUM_SUB_PER_MACRO - 1; sub > 0; sub--)
         {
            if (rch->pcdata->macro[pos].subs[sub] == NULL)
            {
               loc = sub;
            }
         }
      }
   }
   else
   {
      loc = atoi(location);

      if ( (loc <= 0) || (loc > MAX_NUM_SUB_PER_MACRO) )
      {
         Cprintf(ch, "Only numbers from 1 to %d, 'first' and 'last' "
                      "can be used for location.\n\r", MAX_NUM_SUB_PER_MACRO);
         return;
      }
   }

   strcpy(temp, new_commands);

   command = strtok(temp, ";");
   while (command)
   {
      num_new_commands++;
      command = strtok(NULL, ";");
   }

   for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
   {
      if (rch->pcdata->macro[pos].subs[sub] != NULL)
      {
         num_current_commands++;
      }
      else
      {
         break;
      }
   }

   if (loc > num_current_commands)
   {
      loc = num_current_commands;
   }

   num_proposed_commands = num_current_commands + num_new_commands;

   if (num_proposed_commands > MAX_NUM_SUB_PER_MACRO)
   {
      Cprintf(ch, "Adding the new commands would excede the %d command "
                   "limit.\n\r", MAX_NUM_SUB_PER_MACRO);
      return;
   }

   strcpy(temp, new_commands);

   for (sub = 0; sub < loc - 1; sub++)
   {
      temp_command_list[sub] = rch->pcdata->macro[pos].subs[sub];
   }

   command = strtok(temp, ";");

   while (command)
   {
      temp_command_list[sub] = str_dup(command);
      sub++;
      command = strtok(NULL, ";");
   }

   for (temp_sub = loc - 1; temp_sub < MAX_NUM_SUB_PER_MACRO; temp_sub++)
   {
      if (sub < MAX_NUM_SUB_PER_MACRO)
      {
         temp_command_list[sub] = rch->pcdata->macro[pos].subs[temp_sub];
         sub++;
      }
      else
      {
         break;
      }
   }

   for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
   {
      rch->pcdata->macro[pos].subs[sub] = temp_command_list[sub];
   }

   macro_to_def(rch, macroName);

   Cprintf(ch, "Commands inserted.\n\r");

   return;
}

void macro_append(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   int  pos;
   int  sub;
   int  loc = 0;
   int  num_new_commands = 0;
   int  num_current_commands = 0;
   int  num_proposed_commands;
   int  temp_sub;
   char macroName[MAX_INPUT_LENGTH];
   char new_commands[MAX_INPUT_LENGTH];
   char temp[MAX_INPUT_LENGTH];
   char location[MAX_INPUT_LENGTH];
   char *command;
   char *temp_command_list[MAX_NUM_SUB_PER_MACRO] = {NULL};
   bool found = FALSE;

   argument = macro_one_argument(argument, macroName);
   argument = macro_one_argument(argument, location);
   argument = macro_one_argument(argument, new_commands);

   if ((strlen(macroName) == 0) || (strlen(location) == 0))
   {
      Cprintf(ch, "macro append <name> <x> <commands>\n\r");
      Cprintf(ch, "macro append <name> <commands>\n\r");
      Cprintf(ch, "'first' and 'last' are valid values for <x>.\n\r");
      Cprintf(ch, "if <x> isn't included, then the commands are "
                   "appended\n\r");
      Cprintf(ch, "to the end of the macro.\n\r");
      return;
   }

   /* make sure that the macro isn't already running */
   if (macro_is_running(ch, rch, macroName))
   {
      return;
   }


   if (strlen(new_commands) == 0)
   {
      strcpy(new_commands, location);
      strcpy(location, "last");
   }

   for (pos = 0; pos < MAX_NUM_MACROS; pos++)
   {
      if (rch->pcdata->macro[pos].name == NULL)
      {
         break;
      }

      if (!str_cmp(rch->pcdata->macro[pos].name, macroName))
      {
         found = TRUE;
         break;
      }
   }

   if (!found)
   {
      Cprintf(ch, "That macro is not defined.\n\r");
      return;
   }

   if (!str_cmp(location, "first"))
   {
      loc = 1;
   }
   else if (!str_cmp(location, "last"))
   {
      if (rch->pcdata->macro[pos].subs[MAX_NUM_SUB_PER_MACRO - 1] != NULL)
      {
         loc = MAX_NUM_SUB_PER_MACRO;
      }
      else
      {
         for (sub = MAX_NUM_SUB_PER_MACRO - 1; sub > 0; sub--)
         {
            if (rch->pcdata->macro[pos].subs[sub] == NULL)
            {
               loc = sub;
            }
         }
      }
   }
   else
   {
      loc = atoi(location);

      if ( (loc <= 0) || (loc > MAX_NUM_SUB_PER_MACRO) )
      {
         Cprintf(ch, "Only numbers from 1 to %d, 'first' and 'last' "
                      "can be used for location.\n\r", MAX_NUM_SUB_PER_MACRO);
         return;
      }
   }

   strcpy(temp, new_commands);

   command = strtok(temp, ";");
   while (command)
   {
      num_new_commands++;
      command = strtok(NULL, ";");
   }

   for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
   {
      if (rch->pcdata->macro[pos].subs[sub] != NULL)
      {
         num_current_commands++;
      }
      else
      {
         break;
      }
   }

   if (loc > num_current_commands)
   {
      loc = num_current_commands;
   }

   num_proposed_commands = num_current_commands + num_new_commands;

   if (num_proposed_commands > MAX_NUM_SUB_PER_MACRO)
   {
      Cprintf(ch, "Adding the new commands would excede the %d command "
                   "limit.\n\r", MAX_NUM_SUB_PER_MACRO);
      return;
   }

   strcpy(temp, new_commands);

   for (sub = 0; sub < loc; sub++)
   {
      temp_command_list[sub] = rch->pcdata->macro[pos].subs[sub];
   }

   command = strtok(temp, ";");

   while (command)
   {
      temp_command_list[sub] = str_dup(command);
      sub++;
      command = strtok(NULL, ";");
   }

   for (temp_sub = loc; temp_sub < MAX_NUM_SUB_PER_MACRO; temp_sub++)
   {
      if (sub < MAX_NUM_SUB_PER_MACRO)
      {
         temp_command_list[sub] = rch->pcdata->macro[pos].subs[temp_sub];
         sub++;
      }
      else
      {
         break;
      }
   }

   for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
   {
      rch->pcdata->macro[pos].subs[sub] = temp_command_list[sub];
   }

   macro_to_def(rch, macroName);

   Cprintf(ch, "Commands appended.\n\r");

   return;
}

void macro_replace(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   bool  found = FALSE;
   char  macroName[MAX_INPUT_LENGTH];
   char  first_in_range[MAX_INPUT_LENGTH];
   char  last_in_range[MAX_INPUT_LENGTH];
   char  new_commands[MAX_INPUT_LENGTH];
   char  temp[MAX_INPUT_LENGTH];
   char *command;
   char *temp_command_list[MAX_NUM_SUB_PER_MACRO] = {NULL};
   int   num_new_commands = 0;
   int   num_current_commands = 1;
   int   proposed_num_commands;
   int   pos;
   int   sub;
   int   temp_sub;
   int   last = -1;
   int   first = -1;

   argument = macro_one_argument(argument, macroName);

   if (strlen(macroName) == 0)
   {
      Cprintf(ch, "macro replace <name> <x> <command(s)>\n\r");
      Cprintf(ch, "macro replace <name> <x> <y> <command(s)>\n\r");
      Cprintf(ch, "both <x> and <y> can also be 'first' and 'last'.\n\r");
      return;
   }

   /* make sure that the macro isn't running */
   if (macro_is_running(ch, rch, macroName))
   {
      return;
   }

   for (pos = 0; pos < MAX_NUM_MACROS; pos++)
   {
      if (rch->pcdata->macro[pos].name == NULL)
      {
         break;
      }

      if (!str_cmp(rch->pcdata->macro[pos].name, macroName))
      {
         found = TRUE;
         break;
      }
   }

   if (!found)
   {
      Cprintf(ch, "That macro is not defined.\n\r");
      return;
   }

   argument = macro_one_argument(argument, first_in_range);
   if (strlen(first_in_range) == 0)
   {
      Cprintf(ch, "macro replace <name> <x> <command(s)>\n\r");
      Cprintf(ch, "macro replace <name> <x> <y> <command(s)>\n\r");
      Cprintf(ch, "both <x> and <y> can also be 'first' and 'last'.\n\r");
      return;
   }

   argument = macro_one_argument(argument, last_in_range);
   if (strlen(last_in_range) == 0)
   {
      Cprintf(ch, "macro replace <name> <x> <command(s)>\n\r");
      Cprintf(ch, "macro replace <name> <x> <y> <command(s)>\n\r");
      Cprintf(ch, "both <x> and <y> can also be 'first' and 'last'.\n\r");
      return;
   }

   argument = macro_one_argument(argument, new_commands);
   if (strlen(new_commands) == 0)
   {
      strcpy(new_commands, last_in_range);
      strcpy(last_in_range, "");
   }

   if (!str_cmp(first_in_range, "first"))
   {
      first = 1;
   }
   else if (!str_cmp(first_in_range, "last"))
   {
      if (rch->pcdata->macro[pos].subs[MAX_NUM_SUB_PER_MACRO - 1] != NULL)
      {
         first = MAX_NUM_SUB_PER_MACRO;
      }
      else
      {
         for (sub = MAX_NUM_SUB_PER_MACRO - 1; sub > 0; sub--)
         {
            if (rch->pcdata->macro[pos].subs[sub] == NULL)
            {
               first = sub;
            }
            else
            {
               break;
            }
         }
      }
   }
   else
   {
      first = atoi(first_in_range);

      if ((first == 0) || (first > MAX_NUM_SUB_PER_MACRO))
      {
         Cprintf(ch, "Only numbers from 1 to %d, 'first', and 'last' "
                      "can be used for range.\n\r", MAX_NUM_SUB_PER_MACRO);
         return;
      }
   }

   if (strlen(last_in_range) == 0)
   {
      last = first;
   }
   else
   {
      if (!str_cmp(last_in_range, "first"))
      {
         last = 1;
      }
      else if (!str_cmp(last_in_range, "last"))
      {
         if (rch->pcdata->macro[pos].subs[MAX_NUM_SUB_PER_MACRO - 1] != NULL)
         {
            last = MAX_NUM_SUB_PER_MACRO;
         }
         else
         {
            for (sub = MAX_NUM_SUB_PER_MACRO - 1; sub > 0; sub--)
            {
               if (rch->pcdata->macro[pos].subs[sub] == NULL)
               {
                  last = sub;
               }
               else
               {
                  break;
               }
            }
         }
      }
      else
      {
         last = atoi(last_in_range);

         if ((last == 0) || (last > MAX_NUM_SUB_PER_MACRO))
         {
            Cprintf(ch, "Only numbers from 1 to %d, 'first', and 'last' "
                         "can be used for range.\n\r", MAX_NUM_SUB_PER_MACRO);
            return;
         }
      }
   }

   if (first > last)
   {
      Cprintf(ch, "Invalid range.\n\r");
      return;
   }

   strcpy(temp, new_commands);

   command = strtok(temp, ";");

   while (command)
   {
      num_new_commands++;
      command = strtok(NULL, ";");
   }

   sub = 1;

   while (rch->pcdata->macro[pos].subs[sub] != NULL)
   {
      sub++;
      num_current_commands++;
   }

   if (last > num_current_commands)
   {
      last = num_current_commands;
   }

   proposed_num_commands = num_current_commands + num_new_commands;
   proposed_num_commands -= (last - first + 1);

   if (proposed_num_commands > MAX_NUM_SUB_PER_MACRO)
   {
      Cprintf(ch, "That would set the number of commands for the macro"
                   " over the %d command limit.\n\r", MAX_NUM_SUB_PER_MACRO);
      return;
   }

   for (sub = 0; sub < first - 1; sub++)
   {
      temp_command_list[sub] = rch->pcdata->macro[pos].subs[sub];
   }

   command = strtok(new_commands, ";");
   while (command)
   {
      temp_command_list[sub] = str_dup(command);
      command = strtok(NULL, ";");
      sub++;
   }

   for (temp_sub = last; temp_sub < MAX_NUM_SUB_PER_MACRO; temp_sub++)
   {
      if (sub < MAX_NUM_SUB_PER_MACRO)
      {
         temp_command_list[sub] = rch->pcdata->macro[pos].subs[temp_sub];
         sub++;
      }
      else
      {
         break;
      }
   }

   for (sub = first - 1; sub < last; sub++)
   {
      free_string(rch->pcdata->macro[pos].subs[sub]);
   }

   for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
   {
      rch->pcdata->macro[pos].subs[sub] = temp_command_list[sub];
   }

   macro_to_def(rch, macroName);

   Cprintf(ch, "Commands replaced.\n\r");

   return;
}


void macro_kill(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   char           macroName[MAX_INPUT_LENGTH];
   bool           found = FALSE;
   INSTANCE_DATA *instance;

   argument = macro_one_argument(argument, macroName);

   if (strlen(macroName) == 0)
   {
      Cprintf(ch, "macro kill <name>\n\r");
      Cprintf(ch, "macro kill all\n\r");
      return;
   }

   instance = rch->pcdata->macroInstances;

   if (instance == NULL)
   {
      Cprintf(ch, "You don't have any macros running.\n\r");
      return;
   }

   if (!str_cmp(macroName, "all"))
   {
      while (instance)
      {
         Cprintf(ch, "'%s' been stopped.\n\r", instance->macroName);
         free_instance(rch->pcdata, instance);
         instance = rch->pcdata->macroInstances;
      }

      return;
   }

   if (instance == instance->next)
   {
      if (!str_cmp(instance->macroName, macroName))
      {
         found = TRUE;
      }
   }
   else
   {
      instance = rch->pcdata->macroInstances;
      do
      {
         if (!str_cmp(instance->macroName, macroName))
         {
            found = TRUE;
            break;
         }

         instance = instance->next;
      } while (instance != rch->pcdata->macroInstances);
   }

   if (found)
   {
      Cprintf(ch, "'%s' has been stopped.\n\r", macroName);
      free_instance(rch->pcdata, instance);
   }
   else
   {
      Cprintf(ch, "'%s' is not currently running.\n\r", macroName);
   }

   return;
}


void macro_running(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   INSTANCE_DATA *instance;

   instance = rch->pcdata->macroInstances;

   if (instance == NULL)
   {
      Cprintf(ch, "You have no running macroes.\n\r");
   }
   else
   {
      Cprintf(ch, "You're currently running macros are:\n\r");
      while (instance->next != rch->pcdata->macroInstances)
      {
         Cprintf(ch, "    %s\n\r", instance->macroName);
         instance = instance->next;
      }

      Cprintf(ch, "    %s\n\r", instance->macroName);
   }

   return;
}


void
macro_status(CHAR_DATA *ch, CHAR_DATA *rch, char *argument)
{
   Cprintf(ch, "You stat a macro. *NARF!*\n\r");
   Cprintf(ch, "macro status <name>\n\r");
   Cprintf(ch, "macro status all\n\r");
}

int
def_to_macro(CHAR_DATA *ch, char *macro)
{
   int   pos = 0;
   int   command_count = 0;
   int   sub = 0;
   char *command;
   char  definition[MAX_STRING_LENGTH];
 /*  bool  Found = FALSE; */

   while( str_cmp(ch->pcdata->macro[pos].name, macro))
   {
      pos++;
   }

   for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
   {
      free_string(ch->pcdata->macro[pos].subs[sub]);
      ch->pcdata->macro[pos].subs[sub] = NULL;
   }

   sub = 0;

   strcpy(definition, ch->pcdata->macro[pos].definition);

   command = strtok(definition, ";");

   while ((command) && (sub < MAX_NUM_SUB_PER_MACRO))
   {
      while (*command == ' ')
      {
         command++;
      }

      while (*(command + strlen(command) - 1) == ' ')
      {
         *(command + strlen(command) - 1) = '\0';
      }

      if (*command != '\0')
      {
         ch->pcdata->macro[pos].subs[sub] = str_dup(command);
         command_count++;
         sub++;
      }
      command = strtok(NULL, ";");
   }

   if (command)
   {
      while (command)
      {
         command = strtok(NULL, ";");
         command_count++;
      }
   }

   return command_count;
}

void
macro_to_def(CHAR_DATA *ch, char *macro)
{
   int  pos = 0;
   int  sub;
   char buf[MAX_STRING_LENGTH];

   strcpy(buf, "");

   while (str_cmp(ch->pcdata->macro[pos].name, macro))
   {
      pos++;
   }

   if (ch->pcdata->macro[pos].definition)
   {
      free_string(ch->pcdata->macro[pos].definition);
   }

   for (sub = 0; sub < MAX_NUM_SUB_PER_MACRO; sub++)
   {
      if (ch->pcdata->macro[pos].subs[sub] == NULL)
      {
         break;
      }

      if (strlen(buf) > 0)
      {
         strcat(buf, "; ");
      }

      strcat(buf, ch->pcdata->macro[pos].subs[sub]);
   }

   ch->pcdata->macro[pos].definition = str_dup(buf);

   return;
}

/*
 * and this is instance manipulation stuff
 */

INSTANCE_DATA *freeInstances = NULL;

void add_instance(CHAR_DATA *ch, int macroNum, char *restOfLine)
{
   int            varNum;
   INSTANCE_DATA *instance;
   INSTANCE_DATA *temp_instance;
   char           nextArg[MAX_STRING_LENGTH];

   if (freeInstances == NULL)
   {
      instance = (INSTANCE_DATA*)alloc_perm(sizeof(*instance));
   }
   else
   {
      instance = freeInstances;
      freeInstances = freeInstances->next;
   }

   instance->counter = 0;
   instance->macroName = str_dup(ch->pcdata->macro[macroNum].name);
   instance->child = NULL;

   for (varNum = 0; varNum < MAX_NUM_MACRO_VARS; varNum++)
   {
      restOfLine = macro_one_argument(restOfLine, nextArg);
      if (*nextArg)
      {
         instance->vars[varNum] = str_dup(nextArg);
      }
      else
      {
         instance->vars[varNum] = str_dup("");
      }
   }

   /* and now to insert the instance into our list... */
   if (!ch->pcdata->macroInstances)
   {
      instance->next = instance;
      ch->pcdata->macroInstances = instance;
   }
   else if (ch->pcdata->macroInstances == ch->pcdata->macroInstances->next)
   {
      instance->next = ch->pcdata->macroInstances;
      ch->pcdata->macroInstances->next = instance;
   }
   else
   {
      instance->next = ch->pcdata->macroInstances;

      temp_instance = ch->pcdata->macroInstances->next;
      while (temp_instance->next != instance->next)
      {
         temp_instance = temp_instance->next;
      }

      temp_instance->next = instance;
   }
}

void free_instance(PC_DATA *pcdata, INSTANCE_DATA *instance)
{
   INSTANCE_DATA *temp_instance;
   int            varNum;

   /* clear all of the instance's children */
   if (instance->child)
   {
      free_instance(pcdata, instance->child);
      instance->child = NULL;
   }

   /* seperate the instance from the wheel if that is where it lives. */

   /* check if this instance is the only instance in existance */
   if ((pcdata->macroInstances == instance) && (instance->next == instance))
   {
      pcdata->macroInstances = NULL;
   }
   else
   {
      /* check to see if it's even on the wheel */
      if (instance->next)
      {
         temp_instance = instance;
         while (temp_instance->next != instance)
         {
            temp_instance = temp_instance->next;
         }

         temp_instance->next = instance->next;

         /* if it is on the wheel, make sure that we don't
          * remove and forget the hook from pcdata to macroInstances.
          */
         if (pcdata->macroInstances == instance)
         {
            pcdata->macroInstances = instance->next;
         }
      }
   }

   free_string(instance->macroName);
   instance->macroName = NULL;
   instance->next = NULL;
   for (varNum = 0; varNum < MAX_NUM_MACRO_VARS; varNum++)
   {
      if (!instance->vars[varNum])
      {
         break;
      }

      free_string(instance->vars[varNum]);
      instance->vars[varNum] = NULL;
   }

   instance->next = freeInstances;
   freeInstances = instance;
}

void spawn_macro_instance(CHAR_DATA *ch, INSTANCE_DATA *parentInstance, int macro, int parentMacro)
{
   int            varNum;
   int            parentVarNum;
   char          *restOfLine;
   char           parentArgs[MAX_STRING_LENGTH];
   char           nextVariable[MAX_STRING_LENGTH];
   INSTANCE_DATA *instance;

   if (freeInstances == NULL)
   {
      instance = (INSTANCE_DATA*)alloc_perm(sizeof(*instance));
   }
   else
   {
      instance = freeInstances;
      freeInstances = freeInstances->next;
   }

   instance->counter = 0;
   instance->next = NULL;
   instance->child = NULL;
   instance->macroName = str_dup(ch->pcdata->macro[macro].name);

   /* now for variables
    * any variable values specified in the macro definition take
    * precedence over variable values assigned on the command line
    */

   strcpy(parentArgs, ch->pcdata->macro[parentMacro].subs[parentInstance->counter]);
   restOfLine = parentArgs;
   restOfLine = macro_one_argument(parentArgs, nextVariable);

   /* the first argument will be the name of the macro, so dump it. */
   restOfLine = macro_one_argument(restOfLine, nextVariable);
   parentVarNum = 0;
   while (strlen(nextVariable) != 0)
   {
      if ((nextVariable[0] == '%') && (isdigit(nextVariable[1])) && (nextVariable[2] == '\0'))
      {
         instance->vars[parentVarNum] = str_dup(parentInstance->vars[nextVariable[1] - '0']);
      }
      else
      {
         instance->vars[parentVarNum] = str_dup(nextVariable);
      }
      parentVarNum++;
      restOfLine = macro_one_argument(restOfLine, nextVariable);
   }
   for (varNum = parentVarNum; varNum < MAX_NUM_MACRO_VARS; varNum++)
   {
      if (parentInstance->vars[varNum - parentVarNum] == NULL)
      {
         break;
      }

      instance->vars[varNum] = str_dup(parentInstance->vars[varNum]);
   }

   parentInstance->child = instance;

   return;
}

void do_next_command(CHAR_DATA *ch)
{
   INSTANCE_DATA *childInstance;
   INSTANCE_DATA *parentInstance = NULL;
   int            macro;
   int            temp;
   bool           isMacro;
   char           buf[MAX_STRING_LENGTH];

   if ((ch == NULL) || (ch->pcdata->macroInstances == NULL))
   {
      return;
   }

   /* check if there is a command to run
    */
   if (!ch->pcdata->macroInstances)
   {
      ch->pcdata->macroTimer = 0;
      return;
   }

   /* check if enough time has passed since the last macro command was run
    */
   ch->pcdata->macroTimer++;
   ch->pcdata->macroTimer %= MACRO_PAUSE;

   if ((ch->pcdata->macroTimer) != 0)
   {
      return;
   }

   childInstance = ch->pcdata->macroInstances;
   while (childInstance->child)
   {
      parentInstance = childInstance;
      childInstance = parentInstance->child;
   }

   /* find which macro number we're trying to run
    */
   for (macro = 0; macro < MAX_NUM_MACROS; macro++)
   {
      if (!str_cmp(ch->pcdata->macro[macro].name, childInstance->macroName))
      {
         break;
      }
   }

   /* check if we've completed this macro already
    * if so, get rid of this macro and do a command from
    * the parent macro
    */
   if (ch->pcdata->macro[macro].subs[childInstance->counter] == NULL || (childInstance->counter == MAX_NUM_SUB_PER_MACRO - 1))
   {
      free_instance(ch->pcdata, childInstance);
      if (parentInstance)
      {
         parentInstance->child = NULL;
      }

      do_next_command(ch);
      return;
   }

   isMacro = FALSE;

   /* check if the next command to be run is another macro
    */
   for (temp = 0; temp < MAX_NUM_MACROS; temp++)
   {
      if (ch->pcdata->macro[temp].name == NULL)
      {
         break;
      }

      macro_one_argument(ch->pcdata->macro[macro].subs[childInstance->counter], buf);

      if (!str_cmp(ch->pcdata->macro[temp].name, buf))
      {
         isMacro = TRUE;
         break;
      }
   }

   if (isMacro)
   {
      /* here's where we beat all those bastards who want to run us out
       * of memory with a definition like 'a' = 'b', 'b' = 'a'
       *
       * check if the macro to be called is already on this call list
       * if it is, kill everything under that instance...
       */
      childInstance = ch->pcdata->macroInstances;
      do
      {
         if (!str_cmp(childInstance->macroName, ch->pcdata->macro[temp].name))
         {
            if (childInstance->child)
            {
               free_instance(ch->pcdata, childInstance->child);
               childInstance->child = NULL;
               childInstance->counter = 0;
            }

            break;
         }

         if (childInstance->child)
         {
            childInstance = childInstance->child;
         }
      } while (childInstance->child);

      spawn_macro_instance(ch, childInstance, temp, macro);

      childInstance->counter++;
      ch->pcdata->macroInstances = ch->pcdata->macroInstances->next;
   }
   else
   {
      substitute_macro_vars(&(ch->pcdata->macro[macro]), childInstance, buf);
      Cprintf(ch, "{{Macro: %s[%d]: \"%s\"}\n\r", ch->pcdata->macro[macro].name, childInstance->counter + 1, buf);

      childInstance->counter++;
      ch->pcdata->macroInstances = ch->pcdata->macroInstances->next;

      interpret(ch, buf);
   }

   return;
}

void
substitute_macro_vars(MACRO_DATA *macro, INSTANCE_DATA *instance, char *buf)
{
   char *sub;
   int   subPos;
   int   bufPos;

   sub = macro->subs[instance->counter];

   subPos = 0;
   bufPos = 0;

   while (*(sub + subPos))
   {
      if (*(sub + subPos) != '%')
      {
         *(buf + bufPos) = *(sub + subPos);
         *(buf + bufPos + 1) = '\0';
         bufPos++;
      }
      else
      {
         subPos++;
         if ((*(sub + subPos) < '0') || (*(sub + subPos) > '9'))
         {
            *(buf + bufPos) = *(sub + subPos);
            *(buf + bufPos + 1) = '\0';
            bufPos++;
         }
         else
         {
            strcat(buf, instance->vars[*(sub + subPos) - '0']);
            bufPos = strlen(buf);
         }
      }

      subPos++;
   }

   return;
}

bool macro_is_running(CHAR_DATA *ch, CHAR_DATA *rch, char *name)
{
   INSTANCE_DATA *childInstance;
   INSTANCE_DATA *parentInstance;
   bool           isRunning;

   isRunning = FALSE;

   if (!rch->pcdata->macroInstances)
   {
      return isRunning;
   }

   parentInstance = rch->pcdata->macroInstances;

   if (parentInstance == parentInstance->next)
   {
      if (!str_cmp(parentInstance->macroName, name))
      {
         Cprintf(ch, "'%s' is currently running.\n\r", name);
         isRunning = TRUE;
      }
      else
      {
         childInstance = parentInstance->child;
         while ((childInstance) && (!isRunning))
         {
            if (!str_cmp(childInstance->macroName, name))
            {
               Cprintf(ch, "'%s' is currently being run by '%s'.\n\r", name, parentInstance->macroName);
               isRunning = TRUE;
            }

            childInstance = childInstance->child;
         }
      }
   }
   else
   {
      do
      {
         if (!str_cmp(parentInstance->macroName, name))
         {
            Cprintf(ch, "'%s' is currently running.\n\r", name);
            isRunning = TRUE;
         }
         else
         {
            childInstance = parentInstance->child;
            while ((childInstance) && (!isRunning))
            {
               if (!str_cmp(childInstance->macroName, name))
               {
                  Cprintf(ch, "'%s' is currently being run by '%s'.\n\r", name, parentInstance->macroName);
                  isRunning = TRUE;
               }

               childInstance = childInstance->child;
            }
         }

         parentInstance = parentInstance->next;
      } while ((parentInstance != ch->pcdata->macroInstances->next) && (!isRunning));
   }

   return isRunning;
}

/* macro_one_argument() is does EXACTLY the same thing as
 * one_argument(), except it leaves case alone.
 */
char *macro_one_argument(char *argument, char *arg_first)
{
   char cEnd;

   if (strlen(argument) == 0)
   {
      strcpy(arg_first, "");
      return argument;
   }

   while (isspace(*argument))
   {
      argument++;
   }

   cEnd = ' ';

   if (*argument == '\'' || *argument == '"')
   {
      cEnd = *argument;
      argument++;
   }

   while (*argument != '\0')
   {
      if (*argument == cEnd)
      {
         argument++;
         break;
      }

      *arg_first = *argument;
      arg_first++;
      argument++;
   }

   *arg_first = '\0';

   while (isspace(*argument))
   {
      argument++;
   }

   return argument;
}

