#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "merc.h"
#include "magic.h"


void
do_mac_dump( CHAR_DATA *ch, char* argument )
{
   OBJ_INDEX_DATA *pObj;
   AREA_DATA *pArea;
   AFFECT_DATA * paf;
   FILE* fp;
   int vnum;

   if( argument == NULL || argument[0] == '\0' )
   {
      Cprintf (ch, "Syntax: dump <filename>\n\r");
      return;
   }

   fp = fopen( argument, "w" );
   if( fp == NULL )
   {
      Cprintf (ch, "Unable to open file for writing\n\r");
      return;
   }

   for( pArea = area_first; pArea != NULL; pArea = pArea->next )
   {
      fprintf( fp, "Area: %-29.29s (%-5d-%5d) %-12.12s\n\n",
               pArea->name,
               pArea->min_vnum,
               pArea->max_vnum,
               pArea->file_name );
      for( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
      {
         pObj = get_obj_index( vnum );
         if( pObj != NULL )
         {
            fprintf( fp, "\nName(s): '%s' Vnum: %d  Type: %s  Material: %s\n",
                     pObj->name,
                     pObj->vnum,
                     item_name(pObj->item_type),
                     pObj->material);

            fprintf( fp, "Wear bits: %s\nExtra bits: %s\n",
                     wear_bit_name(pObj->wear_flags),
                     extra_bit_name(pObj->extra_flags));

            fprintf( fp, "Weight: %d Level: %d  Cost: %d\n",
                     pObj->weight, pObj->level, pObj->cost );

            fprintf( fp, "Values: %d %d %d %d %d\n",
                     pObj->value[0], pObj->value[1],
                     pObj->value[2], pObj->value[3],
                     pObj->value[4] );

            switch (pObj->item_type)
            {
            case ITEM_SCROLL:
            case ITEM_POTION:
            case ITEM_PILL:
               fprintf( fp, "Level %d spells of:", pObj->value[0] );

               if (pObj->value[1] >= 0 && pObj->value[1] < MAX_SKILL)
               {
                  fprintf( fp, " '%s'", skill_table[pObj->value[1]].name );
               }

               if (pObj->value[2] >= 0 && pObj->value[2] < MAX_SKILL)
               {
                  fprintf( fp, " '%s'", skill_table[pObj->value[2]].name );
               }

               if (pObj->value[3] >= 0 && pObj->value[3] < MAX_SKILL)
               {
                  fprintf( fp, " '%s'", skill_table[pObj->value[3]].name );
               }

               if (pObj->value[4] >= 0 && pObj->value[4] < MAX_SKILL)
               {
                  fprintf( fp, " '%s'", skill_table[pObj->value[4]].name );
               }

               fprintf( fp, ".\n");
               break;

            case ITEM_WAND:
            case ITEM_STAFF:
               fprintf( fp, "Has %d(%d) charges of level %d",
                        pObj->value[1], pObj->value[2], pObj->value[0] );

               if( pObj->value[3] >= 0 && pObj->value[3] < MAX_SKILL)
               {
                  fprintf( fp, " '%s'", skill_table[pObj->value[3]].name );
               }

               fprintf( fp, ".\n");
               break;

            case ITEM_THROWING:
               fprintf( fp, "Dice: %d Faces: %d Damtype: %s Spell lvl: %d Spell: '%s'\n",
                        pObj->value[0],
                        pObj->value[1],
                        attack_table[pObj->value[2]].name,
                        pObj->value[3],
                        skill_table[pObj->value[4]].name);
               break;

            case ITEM_DRINK_CON:
               fprintf( fp, "It holds %s-colored %s.\n",
                        liq_table[pObj->value[2]].liq_color,
                        liq_table[pObj->value[2]].liq_name );
               break;

            case ITEM_WEAPON:
               fprintf( fp, "Weapon type is " );
               switch (pObj->value[0])
               {
               case (WEAPON_EXOTIC):
                  fprintf( fp, "exotic\n" );
                  break;
               case (WEAPON_SWORD):
                  fprintf( fp, "sword\n" );
                  break;
               case (WEAPON_DAGGER):
                  fprintf( fp, "dagger\n" );
                  break;
               case (WEAPON_SPEAR):
                  fprintf( fp, "spear/staff\n" );
                  break;
               case (WEAPON_MACE):
                  fprintf( fp, "mace/club\n" );
                  break;
               case (WEAPON_AXE):
                  fprintf( fp, "axe\n" );
                  break;
               case (WEAPON_FLAIL):
                  fprintf( fp, "flail\n" );
                  break;
               case (WEAPON_WHIP):
                  fprintf( fp, "whip\n" );
                  break;
               case (WEAPON_POLEARM):
                  fprintf( fp, "polearm\n" );
                  break;
               default:
                  fprintf( fp, "unknown\n" );
                  break;
               }
               if( pObj->new_format )
                  fprintf( fp, "Damage is %dd%d (average %d)\n",
                           pObj->value[1],
                           pObj->value[2],
                           (1 + pObj->value[2]) * pObj->value[1] / 2);
               else
                  fprintf( fp, "Damage is %d to %d (average %d)\n",
                           pObj->value[1],
                           pObj->value[2],
                           (pObj->value[1] + pObj->value[2]) / 2);

               fprintf( fp, "Damage noun is %s.\n",
                        (pObj->value[3] > 0 && pObj->value[3] < MAX_DAMAGE_MESSAGE) ?
                        attack_table[pObj->value[3]].noun : "undefined");

               if (pObj->value[4]) /* weapon flags */
               {
                  fprintf( fp, "Weapons flags: %s\n",
                           weapon_bit_name (pObj->value[4]));
               }
               break;

            case ITEM_ARMOR:
               fprintf( fp, "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n",
                        pObj->value[0], pObj->value[1],
                        pObj->value[2], pObj->value[3]);
               break;

            case ITEM_CONTAINER:
               fprintf( fp, "Capacity: %d#  Maximum weight: %d#  flags: %s\n",
                        pObj->value[0], pObj->value[3],
                        cont_bit_name (pObj->value[1]));

               if (pObj->value[4] != 100)
               {
                  fprintf( fp, "Weight multiplier: %d%%\n",
                           pObj->value[4]);
               }
               break;
            }



            for (paf = pObj->affected; paf != NULL; paf = paf->next)
            {
               fprintf (fp, "Affects %s by %d, level %d",
                        affect_loc_name (paf->location),
                        paf->modifier,
                        paf->level);
               if (paf->duration > -1)
                  fprintf (fp, ", %d hours.\n", paf->duration);
               else
                  fprintf (fp, ".\n");

               if (paf->bitvector)
               {
                  switch (paf->where)
                  {
                  case TO_AFFECTS:
                     fprintf (fp, "Adds %s affect.\n",
                              affect_bit_name (paf->bitvector));
                     break;
                  case TO_WEAPON:
                     fprintf (fp, "Adds %s weapon flags.\n",
                               weapon_bit_name (paf->bitvector));
                     break;
                  case TO_OBJECT:
                     fprintf (fp, "Adds %s pObject flag.\n",
                              extra_bit_name (paf->bitvector));
                     break;
                  case TO_IMMUNE:
                     fprintf (fp, "Adds immunity to %s.\n",
                              imm_bit_name (paf->bitvector));
                     break;
                  case TO_RESIST:
                     fprintf (fp, "Adds resistance to %s.\n",
                              imm_bit_name (paf->bitvector));
                     break;
                  case TO_VULN:
                     fprintf (fp, "Adds vulnerability to %s.\n",
                              imm_bit_name (paf->bitvector));
                     break;
                  default:
                     fprintf (fp, "Unknown bit %d: %ld\n",
                              paf->where, paf->bitvector);
                     break;
                  }
               }
            }

            for (paf = pObj->affected; paf != NULL; paf = paf->next)
            {
               fprintf (fp, "Affects %s by %d, level %d.\n",
                        affect_loc_name (paf->location),
                        paf->modifier,
                        paf->level);
               if (paf->bitvector)
               {
                  switch (paf->where)
                  {
                  case TO_AFFECTS:
                     fprintf (fp, "Adds %s affect.\n",
                              affect_bit_name (paf->bitvector));
                     break;
                  case TO_OBJECT:
                     fprintf (fp, "Adds %s pObject flag.\n",
                              extra_bit_name (paf->bitvector));
                     break;
                  case TO_IMMUNE:
                     fprintf (fp, "Adds immunity to %s.\n",
                              imm_bit_name (paf->bitvector));
                     break;
                  case TO_RESIST:
                     fprintf (fp, "Adds resistance to %s.\n",
                              imm_bit_name (paf->bitvector));
                     break;
                  case TO_VULN:
                     fprintf (fp, "Adds vulnerability to %s.\n",
                              imm_bit_name (paf->bitvector));
                     break;
                  default:
                     fprintf (fp, "Unknown bit %d: %ld\n",
                              paf->where, paf->bitvector);
                     break;
                  }
               }
            }
         }
      }
   }

   fclose( fp );
   return;
}

