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
 *        ROM 2.4 is copyright 1993-1996 Russ Taylor                       *
 *        ROM has been brought to you by the ROM consortium                *
 *            Russ Taylor (rtaylor@pacinfo.com)                            *
 *            Gabrielle Taylor (gtaylor@pacinfo.com)                       *
 *            Brian Moore (rom@rom.efn.org)                                *
 *        By using this code, you have agreed to follow the terms of the   *
 *        ROM license, in the file Rom24/doc/rom.license                   *
 ***************************************************************************/

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "merc.h"
#include "magic.h"
#include "utils.h"


extern int generate_int(unsigned char, unsigned char);

void
do_heal (CHAR_DATA * ch, char *argument)
{
   int i;
   char buf[MAX_STRING_LENGTH];
   int spell_level;
   CHAR_DATA * mob;
   char arg[MAX_INPUT_LENGTH];
   int cost;

   struct healSpellStruct
   {
      char *keyWord;
      char *spellDesc;
      int goldCost;
      SPELL_FUN *spell;
      int sn;
      char *magicWords;
   } healSpells[] =
   {
      {"light",     "cure light wounds",    10, spell_cure_light,     gsn_cure_light,     "judicandus dies"},
      {"serious",   "cure serious wounds",  15, spell_cure_serious,   gsn_cure_serious,   "judicandus gzfaujg"},
      {"critic",    "cure critical wounds", 25, spell_cure_critical,  gsn_cure_critical,  "judicandus qfuhuqar"},
      {"heal",      "healing spell",        50, spell_heal,           gsn_heal,           "pzar"},
      {"bless",     "bless",                5,  spell_bless,          gsn_bless,          "fido"},
      {"armor",     "armor",                5,  spell_armor,          gsn_armor,          "judiscandis fonda"},
      {"sanctuary", "sanctuary",            80, spell_sanctuary,      gsn_sanctuary,      "judiscandis montonia"},
      {"blind",     "cure blindness",       20, spell_cure_blindness, gsn_cure_blindness, "judicandus noselacri"},
      {"disease",   "cure disease",         15, spell_cure_disease,   gsn_cure_disease,   "judicandus eugzagz"},
      {"poison",    "cure poison",          25, spell_cure_poison,    gsn_cure_poison,    "judicandus sausabru"},
      {"uncurse",   "remove curse",         50, spell_remove_curse,   gsn_remove_curse,   "candussido judifgz"},
      {"mana",      "restore mana",         5,  NULL,                 -1,                 "energizer"},
      {"refresh",   "restore movement",     10, spell_refresh,        gsn_refresh,        "candusima"},
      {NULL,        NULL,                   0,  NULL,                 -1,                 NULL}
   };

   /* check for healer */
   for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
   {
      if (IS_NPC (mob) && IS_SET (mob->act, ACT_IS_HEALER))
         break;
   }

   if (mob == NULL)
   {
      Cprintf (ch, "You can't do that here.\n\r");
      return;
   }

   one_argument (argument, arg);

   if (arg[0] == '\0')
   {
      /* display price list updated by Gothic */
       act("$N says 'I offer the following spells:'", ch, NULL, mob, TO_CHAR, POS_RESTING);
       i = 0;
       while (healSpells[i].keyWord)
       {
          sprintf(buf, "  %s: %s", healSpells[i].keyWord, healSpells[i].spellDesc);
          Cprintf (ch, "%-32s%d gold\n\r", buf, healSpells[i].goldCost);
          i++;
       }
       Cprintf (ch, " Type heal <type> to be healed.\n\r");

       return;
   }

   i = 0;
   while ((healSpells[i].keyWord) && (str_prefix(arg, healSpells[i].keyWord)))
     i++;

   if (!healSpells[i].keyWord)
   {
      act("$N says 'Type 'heal' for a list of spells.'", ch, NULL, mob, TO_CHAR, POS_RESTING);

      return;
   }


   cost = healSpells[i].goldCost * 100;

   if (cost > (ch->gold * 100 + ch->silver))
   {
      act("$N says 'You do not have enough gold for my services.'", ch, NULL, mob, TO_CHAR, POS_RESTING);

      return;
   }

   WAIT_STATE (ch, PULSE_VIOLENCE);

   deduct_cost(ch, cost);
   act("$n utters the words '$T'.", mob, NULL, healSpells[i].magicWords, TO_ROOM, POS_RESTING);

   if (healSpells[i].spell == NULL)                /* restore mana trap...kinda hackish */
   {
      ch->mana += dice (2, 8) + mob->level / 3;
      ch->mana = UMIN (ch->mana, MAX_MANA(ch));
      Cprintf (ch, "A warm glow passes through you.\n\r");
      return;
   }

   if (healSpells[i].sn == -1)
      return;
   spell_level = generate_int(mob->level, mob->level);
   (healSpells[i].spell)(healSpells[i].sn, spell_level, mob, ch, TARGET_CHAR);
}
