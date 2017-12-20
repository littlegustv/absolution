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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "recycle.h"


// Eats magical items, worsens AC slowly
void
acid_effect(void *vo, int level, int dam, int target)
{
	if (target == TARGET_ROOM)	/* nail objects on the floor */
	{
		ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
		OBJ_DATA *obj, *obj_next;

		for (obj = room->contents; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			acid_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR)	/* do the effect on a victim */
	{
		CHAR_DATA *victim = (CHAR_DATA *) vo;
		OBJ_DATA *obj, *obj_next;
		AFFECT_DATA af;
		int chance;

		// Typically 5% weapon hit, 25% breath.
		chance = 8 + dam / 8;

		// Burn some flesh
		if(number_percent() < chance) {
			Cprintf(victim, "Chunks of your flesh melt away, exposing vital areas!\n\r");
			act("$n's flesh burns away, revealing vital areas!", victim, NULL, NULL, TO_ROOM, POS_RESTING);

			af.where = TO_AFFECTS;
			af.type = gsn_acid_breath;
			af.level = level;
			af.duration = number_range(1, 4);
			af.modifier = 25;
			af.location = APPLY_AC;
			af.bitvector = 0;

			affect_join(victim, &af);
		}

		/* let's toast some gear */
		for (obj = victim->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			acid_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ)	/* toast an object */
	{
		OBJ_DATA *obj = (OBJ_DATA *) vo;
		OBJ_DATA *t_obj, *n_obj;
		int chance;
		char *msg;

		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
			|| IS_OBJ_STAT(obj, ITEM_NOPURGE))
			return;

		chance = (level - obj->level);


		switch (obj->item_type)
		{
		default:
			return;
		case ITEM_CONTAINER:
		case ITEM_CORPSE_PC:
		case ITEM_CORPSE_NPC:
			msg = "$p fumes and dissolves.";
			break;
		case ITEM_CLOTHING:
			msg = "$p is corroded into scrap.";
			break;
		case ITEM_STAFF:
		case ITEM_WAND:
			msg = "$p corrodes and breaks.";
			break;
		case ITEM_SCROLL:
			msg = "$p is burned into waste.";
			break;
		}

		chance = URANGE(1, chance, 6);

		if (number_percent() > chance)
			return;

		if (obj->carried_by != NULL)
			act(msg, obj->carried_by, obj, NULL, TO_ALL, POS_RESTING);
		else if (obj->in_room != NULL && obj->in_room->people != NULL)
			act(msg, obj->in_room->people, obj, NULL, TO_ALL, POS_RESTING);

		/* get rid of the object */
		if (obj->contains)		/* dump contents */
		{
			for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
			{
				n_obj = t_obj->next_content;
				obj_from_obj(t_obj);
				if (obj->in_room != NULL)
					obj_to_room(t_obj, obj->in_room);
				else if (obj->carried_by != NULL)
					obj_to_room(t_obj, obj->carried_by->in_room);
				else
				{
					extract_obj(t_obj);
					continue;
				}

				acid_effect(t_obj, level / 2, dam / 2, TARGET_OBJ);
			}
		}

		extract_obj(obj);
		return;
	}
}


// Slowly eats strength and movement, and shatters potions/drinks.
void
cold_effect(void *vo, int level, int dam, int target)
{
	if (target == TARGET_ROOM)	/* nail objects on the floor */
	{
		ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
		OBJ_DATA *obj, *obj_next;

		for (obj = room->contents; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			cold_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR)	/* whack a character */
	{
		CHAR_DATA *victim = (CHAR_DATA *) vo;
		OBJ_DATA *obj, *obj_next;
		AFFECT_DATA af;
		int chance;

		// Typically 5% weapon hit, 25% breath.
		chance = 5 + dam / 8;

		/* chill touch effect */
		if (number_percent() <= chance)
		{
			victim->move -= (dice(4, 6) + (level / 2));

			act("$n turns blue and shivers.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			act("A chill sinks deep into your bones.", victim, NULL, NULL, TO_CHAR, POS_RESTING);

			af.where = TO_AFFECTS;
			af.type = gsn_frost_breath;
			af.level = level;
			af.duration = number_range(1, 4);
			af.location = APPLY_STR;
			af.modifier = -1;
			af.bitvector = 0;

			affect_join(victim, &af);
		}

		/* let's toast some gear */
		for (obj = victim->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			cold_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ)	/* toast an object */
	{
		OBJ_DATA *obj = (OBJ_DATA *) vo;
		int chance;
		char *msg;

		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
			|| IS_OBJ_STAT(obj, ITEM_NOPURGE))
			return;

		chance = level - obj->level;

		switch (obj->item_type)
		{
		default:
			return;
		case ITEM_POTION:
			msg = "$p freezes and shatters!";
			break;
		case ITEM_DRINK_CON:
			msg = "$p freezes and shatters!";
			break;
		}

		chance = URANGE(5, chance, 20);

		if (number_percent() > chance)
			return;

		if (obj->carried_by != NULL)
			act(msg, obj->carried_by, obj, NULL, TO_ALL, POS_RESTING);
		else if (obj->in_room != NULL && obj->in_room->people != NULL)
			act(msg, obj->in_room->people, obj, NULL, TO_ALL, POS_RESTING);

		extract_obj(obj);
		return;
	}
}



void
fire_effect(void *vo, int level, int dam, int target)
{
	if (target == TARGET_ROOM)	/* nail objects on the floor */
	{
		ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
		OBJ_DATA *obj, *obj_next;

		for (obj = room->contents; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			fire_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR)	/* do the effect on a victim */
	{
		CHAR_DATA *victim = (CHAR_DATA *) vo;
		OBJ_DATA *obj, *obj_next;
		AFFECT_DATA af;
		int chance;

		// Typically 5% weapon hit, 20% breath.
		chance = 5 + dam / 10;

		/* chance of blindness */
		if (!IS_AFFECTED(victim, AFF_BLIND)
		&& number_percent() <= chance)
		{
			act("$n is blinded by smoke!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			act("Your eyes tear up from smoke...you can't see a thing!", victim, NULL, NULL, TO_CHAR, POS_RESTING);

			af.where = TO_AFFECTS;
			af.type = gsn_fire_breath;
			af.level = level;
			af.duration = URANGE(0, (level - victim->level) / 5, 2);
			af.location = APPLY_HITROLL;
			af.modifier = -4;
			af.bitvector = AFF_BLIND;

			affect_to_char(victim, &af);
		}

		/* getting thirsty */
		if (!IS_NPC(victim))
			gain_condition(victim, COND_THIRST, dam / 20);

		/* let's toast some gear! */
		for (obj = victim->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			fire_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ)	/* toast an object */
	{
		OBJ_DATA *obj = (OBJ_DATA *) vo;
		OBJ_DATA *t_obj, *n_obj;
		int chance;
		char *msg;

		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
			|| IS_OBJ_STAT(obj, ITEM_NOPURGE))
			return;

		chance = level - obj->level;

		switch (obj->item_type)
		{
		default:
			return;
		case ITEM_CONTAINER:
			msg = "$p ignites and burns!";
			break;
		case ITEM_POTION:
			msg = "$p bubbles and boils!";
			break;
		case ITEM_SCROLL:
			msg = "$p crackles and burns!";
			break;
		case ITEM_STAFF:
			msg = "$p smokes and chars!";
			break;
		case ITEM_WAND:
			msg = "$p sparks and sputters!";
			break;
		case ITEM_FOOD:
			msg = "$p blackens and crisps!";
			break;
		case ITEM_PILL:
			msg = "$p melts and drips!";
			break;
		}

		chance = URANGE(1, chance, 6);

		if (number_percent() > chance)
			return;

		if (obj->carried_by != NULL)
			act(msg, obj->carried_by, obj, NULL, TO_ALL, POS_RESTING);
		else if (obj->in_room != NULL && obj->in_room->people != NULL)
			act(msg, obj->in_room->people, obj, NULL, TO_ALL, POS_RESTING);

		if (obj->contains)
		{
			/* dump the contents */

			for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
			{
				n_obj = t_obj->next_content;
				obj_from_obj(t_obj);
				if (obj->in_room != NULL)
					obj_to_room(t_obj, obj->in_room);
				else if (obj->carried_by != NULL)
					obj_to_room(t_obj, obj->carried_by->in_room);
				else
				{
					extract_obj(t_obj);
					continue;
				}
				fire_effect(t_obj, level / 2, dam / 2, TARGET_OBJ);
			}
		}

		extract_obj(obj);
		return;
	}
}

void
poison_effect(void *vo, int level, int dam, int target)
{
	if (target == TARGET_ROOM)	/* nail objects on the floor */
	{
		ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
		OBJ_DATA *obj, *obj_next;

		for (obj = room->contents; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			poison_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR)	/* do the effect on a victim */
	{
		CHAR_DATA *victim = (CHAR_DATA *) vo;
		OBJ_DATA *obj, *obj_next;
		int chance;

		// Typically 5% weapon hit, 25% breath.
		chance = 6 + dam / 8;

		/* chance of poisoning */
		if (number_percent() <= chance)
		{
			AFFECT_DATA af;

			Cprintf(victim, "You feel poison coursing through your veins.\n\r");
			act("$n looks very ill.", victim, NULL, NULL, TO_ROOM, POS_RESTING);

			af.where = TO_AFFECTS;
			af.type = gsn_poison;
			af.level = level;
			af.duration = number_range(1, 4);
			af.location = APPLY_STR;
			af.modifier = -1;
			af.bitvector = AFF_POISON;
			affect_join(victim, &af);

			damage(victim, victim, 1 + (victim->hit / number_range(15, 30)), gsn_poison, DAM_POISON, FALSE, TYPE_MAGIC);
		}

		/* equipment */
		for (obj = victim->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			poison_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ)	/* do some poisoning */
	{
		OBJ_DATA *obj = (OBJ_DATA *) vo;
		int chance;


		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
			|| IS_OBJ_STAT(obj, ITEM_BLESS))
			return;

		chance = level - obj->level;

		switch (obj->item_type)
		{
		default:
			return;
		case ITEM_FOOD:
			break;
		case ITEM_DRINK_CON:
			if (obj->value[0] == obj->value[1])
				return;
			break;
		}

		chance = URANGE(5, chance, 25);

		if (number_percent() > chance)
			return;

		obj->value[3] = 1;
		return;
	}
}


void
shock_effect(void *vo, int level, int dam, int target)
{
	if (target == TARGET_ROOM)
	{
		ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
		OBJ_DATA *obj, *obj_next;

		for (obj = room->contents; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			shock_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR)
	{
		CHAR_DATA *victim = (CHAR_DATA *) vo;
		OBJ_DATA *obj, *obj_next;
		int chance;

		// Typically 5% weapon hit, 35% breath.
		chance = 6 + dam / 5;

		/* daze and confused? */
		if (number_percent() <= chance)
		{
			Cprintf(victim, "Your muscles stop responding.\n\r");
			act("$n jerks and twitches from the shock!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
			victim->wait += dice(1, 6);
			victim->daze += 2 * PULSE_VIOLENCE;
		}

		/* toast some gear */
		for (obj = victim->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			shock_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ)
	{
		OBJ_DATA *obj = (OBJ_DATA *) vo;
		int chance;
		char *msg;

		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
			|| IS_OBJ_STAT(obj, ITEM_NOPURGE))
			return;

		chance = level - obj->level;

		switch (obj->item_type)
		{
		default:
			return;
		case ITEM_WAND:
		case ITEM_STAFF:
			msg = "$p overloads and explodes!";
			break;
		case ITEM_JEWELRY:
			msg = "$p is fused into a worthless lump.";
		}

		chance = URANGE(1, chance, 10);

		if (number_percent() > chance)
			return;

		if (obj->carried_by != NULL)
			act(msg, obj->carried_by, obj, NULL, TO_ALL, POS_RESTING);
		else if (obj->in_room != NULL && obj->in_room->people != NULL)
			act(msg, obj->in_room->people, obj, NULL, TO_ALL, POS_RESTING);

		extract_obj(obj);
		return;
	}
}

void
water_effect(void *vo, int level, int dam, int target)
{
	if (target == TARGET_ROOM)	/* nail objects on the floor */
	{
		ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
		OBJ_DATA *obj, *obj_next;

		for (obj = room->contents; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			water_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR)	/* do the effect on a victim */
	{
		CHAR_DATA *victim = (CHAR_DATA *) vo;
		OBJ_DATA *obj, *obj_next;
		int chance;

		// Typically 5% weapon hit, 25% spell.
		chance = 5 + dam / 8;

		/* slowed by water */
		if (number_percent() <= chance)
		{
			AFFECT_DATA af;
			if (IS_AFFECTED(victim, AFF_HASTE))
			{
				Cprintf(victim, "The water clings to you, slowing you down.\n\r");
				act("$n is slowed by the water.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
				affect_strip(victim, gsn_haste);
			}
			else
			{
				Cprintf(victim, "You cough and sputter on the water.\n\r");
				act("$n coughes and chokes on the water.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
				af.where = TO_AFFECTS;
				af.type = gsn_hurricane;
				af.level = level;
				af.duration = number_range(1, 4);
				af.location = APPLY_DEX;
				af.modifier = -1;
				af.bitvector = 0;
				affect_join(victim, &af);
			}
		}

		/* let's toast some gear */
		for (obj = victim->carrying; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			water_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ)	/* toast an object */
	{
		OBJ_DATA *obj = (OBJ_DATA *) vo;
		OBJ_DATA *t_obj, *n_obj;
		int chance;
		char *msg;

		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
			|| IS_OBJ_STAT(obj, ITEM_NOPURGE)
			|| number_range(0, 4) == 0)
			return;

		chance = level - obj->level;

		switch (obj->item_type)
		{
		default:
			return;
		case ITEM_CONTAINER:
		case ITEM_DRINK_CON:
			msg = "$p fills and bursts!";
			break;
		case ITEM_POTION:
			msg = "$p dilutes and overflows.";
			break;
		case ITEM_FOOD:
		case ITEM_PILL:
			msg = "$p dissolves into an icky sludge.";
			break;
		case ITEM_SCROLL:
			msg = "$p's ink melts and runs.";
			break;
		}

		chance = URANGE(5, chance, 10);

		if (number_percent() > chance)
			return;

		if (obj->carried_by != NULL)
			act(msg, obj->carried_by, obj, NULL, TO_ALL, POS_RESTING);
		else if (obj->in_room != NULL && obj->in_room->people != NULL)
			act(msg, obj->in_room->people, obj, NULL, TO_ALL, POS_RESTING);

		/* get rid of the object */
		if (obj->contains)		/* dump contents */
		{
			for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
			{
				n_obj = t_obj->next_content;
				obj_from_obj(t_obj);
				if (obj->in_room != NULL)
					obj_to_room(t_obj, obj->in_room);
				else if (obj->carried_by != NULL)
					obj_to_room(t_obj, obj->carried_by->in_room);
				else
				{
					extract_obj(t_obj);
					continue;
				}

				water_effect(t_obj, level / 2, dam / 2, TARGET_OBJ);
			}
		}

		extract_obj(obj);
		return;
	}
}


