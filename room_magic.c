#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "magic.h"
#include "utils.h"


extern char* target_name;
extern int get_caster_level(int);
extern int get_modified_level(int);

void
spell_room_test( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = modified_level;
    af.duration  = 2;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = ROOM_AFF_TEST;
    affect_to_room ( ch->in_room, &af );

    Cprintf(ch, "The room is now affected!\n\r");

   return;
}


void
spell_build_fire( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (room_is_affected (ch->in_room, gsn_build_fire))
	{
		Cprintf(ch, "There already is a nice fire here.\n\r");
		return;
	}

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = modified_level;
    af.duration  = (ch->level / 6) + 3;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = ROOM_AFF_FIRE;
    affect_to_room ( ch->in_room, &af );

    Cprintf(ch, "You create a hearty camp fire.\n\r");
    act("$n has created a beautiful camp fire.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	return;
}

void
spell_oracle( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (room_is_affected (ch->in_room, gsn_oracle))
	{
		Cprintf(ch, "There already is an oracle in this room.\n\r");
		return;
	}

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = modified_level;
    af.duration  = (ch->level / 6) + 3;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = ROOM_AFF_ORACLE;
    affect_to_room ( ch->in_room, &af );

    Cprintf(ch, "You create an oracle of benefit.\n\r");
    act("$n has created an oracle of benefit.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

   return;
}

void
spell_web( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (room_is_affected (ch->in_room, gsn_web))
	{
		Cprintf(ch, "There already is a woven web in this room.\n\r");
		return;
	}

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = modified_level;
    af.duration  = (ch->level / 10) + 3;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = ROOM_AFF_WEB;
    affect_to_room ( ch->in_room, &af );

    Cprintf(ch, "You weave a sticky web all around the room.\n\r");
    act("$n weaves a sticky web all over the room.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

   return;
}

void
spell_rain( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (room_is_affected (ch->in_room, gsn_rain_of_tears))
	{
		Cprintf(ch, "It's already raining buckets in here!\n\r");
		return;
	}

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = modified_level;
    af.duration  = (ch->level / 12) + 1;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = ROOM_AFF_RAIN;
    affect_to_room ( ch->in_room, &af );

    Cprintf(ch, "You call forth a rain of tears.\n\r");
    act("$n calls forth a rain of tears.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

   return;
}

void
spell_hail( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (room_is_affected (ch->in_room, gsn_hailstorm))
	{
	 Cprintf(ch, "There already is a hailstorm this room.\n\r");
	 return;
	}

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = modified_level;
    af.duration  = (ch->level / 10) + 1;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = ROOM_AFF_HAIL;
    affect_to_room ( ch->in_room, &af );

    Cprintf(ch, "You conjure hail from the heavens.\n\r");
    act("$n conjures a hailstorm from the heavens.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

   return;
}

/* new dragon spell Darkness for blacks.
   coded by Starcrossed, sept 10, 1999
*/
void
spell_darkness( int sn, int level, CHAR_DATA *ch, void *vo, int target ) {
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

    if (room_is_affected (ch->in_room, gsn_darkness))
    {
            Cprintf(ch, "It can't get any darker than this!\n\r");
            return;
    }

    if(number_percent() < 20) {
	Cprintf(ch, "Your darkness fails to appear.\n\r");
	return;
    }

    Cprintf(ch, "You plunge the room into absolute darkness!\n\r");
    act("$n plunges the room into total darkness!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = modified_level;
    af.duration  = (ch->level / 12) + 1;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = ROOM_AFF_DARKNESS;
    affect_to_room ( ch->in_room, &af );

    return;
}

void
spell_cloudkill( int sn, int level, CHAR_DATA *ch, void *vo, int target ) {
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (room_is_affected (ch->in_room, gsn_cloudkill))
	{
        Cprintf(ch, "This room is already covered in a toxic cloud!\n\r");
        return;
	}

	Cprintf(ch, "You belch forth a cloud of toxic gas to cover the room!\n\r");
	act("$n emits a greenish cloud of poison gas!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = ch->level;
	af.duration  = (ch->level / 16) + 1;
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	if( is_clan(ch) )
		af.bitvector = ROOM_AFF_CLOUDKILL_CLAN;
	else
		af.bitvector = ROOM_AFF_CLOUDKILL_NC;
	affect_to_room ( ch->in_room, &af );

	return;
}


void
spell_earth_to_mud( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (room_is_affected (ch->in_room, gsn_earth_to_mud))
	{
		Cprintf(ch, "This room is sloppy and muddy enough, don't you think?\n\r");
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = modified_level;
	af.duration  = (ch->level / 12);
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = 0;
	affect_to_room ( ch->in_room, &af );

	Cprintf(ch, "You call a down pour and turn the ground into a muddy slop.\n\r");
	act("$n calls a down pour and turns the ground into a muddy slop.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	return;
}

void
winter_chill(CHAR_DATA *ch, ROOM_INDEX_DATA* room) {
	CHAR_DATA *rch = NULL;

	cold_effect(room, ch->level, ch->level * 8, TARGET_ROOM);

	for(rch = room->people; rch != NULL; rch = rch->next_in_room) {
		if(ch == rch)
			continue;
		if(IS_NPC(rch)
		&& rch->pIndexData->pShop != NULL)
			continue;
		cold_effect(rch, ch->level, ch->level * 8, TARGET_CHAR);
	}
	return;
}

void
spell_winter_storm( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	AFFECT_DATA af;
	int exit;
	EXIT_DATA* room;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (room_is_affected (ch->in_room, gsn_winter_storm))
	{
		Cprintf(ch, "This area is already covered in snow.\n\r");
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = modified_level;
	af.duration  = ch->level / 5;
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = 0;
	affect_to_room(ch->in_room, &af);

	Cprintf(ch, "You call a raging winter storm to blanket the area!\n\r");
	act("$n calls a raging winter storm!", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	// Chill people
	winter_chill(ch, ch->in_room);

	/* go out each exit */
	for (exit = 0; exit < 6; exit++)
	{
		/* If you're outside a clan hall, storm won't go inside (Tsongas 10/19/2001) */
		room = ch->in_room->exit[exit];
		if (room != NULL &&
		    room->u1.to_room != NULL &&
		    !(!IS_SET(ch->in_room->room_flags, ROOM_CLAN) && room->u1.to_room->clan))
		{
			affect_to_room(room->u1.to_room, &af);
			winter_chill(ch, room->u1.to_room);
		}
	}

	return;
}

void
spell_plant( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (room_is_affected (ch->in_room, gsn_plant))
	{
		Cprintf(ch, "This room is already full of magical plants.\n\r");
		return;
	}

	if (ch->in_room->sector_type == SECT_INSIDE)
	{
		Cprintf(ch, "You can't plant things indoors!");
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = modified_level;
	af.duration  = 2;
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = 0;
	affect_to_room(ch->in_room, &af);

	Cprintf(ch, "You drop some magical seeds on the ground and nuture their growth.\n\r");
	act("$n plants some magical seeds", ch, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
end_plant(void *vo, int target)
{
	ROOM_INDEX_DATA* room = (ROOM_INDEX_DATA*)vo;
	OBJ_DATA* obj;
	int count;

	for (count = dice(1,4); count > 0; count--)
	{
		switch (dice(1,4))
		{
		case 1:
			obj = create_object(get_obj_index(OBJ_VNUM_PLANT_A), 0);
			break;
		case 2:
			obj = create_object(get_obj_index(OBJ_VNUM_PLANT_B), 0);
			break;
		case 3:
			obj = create_object(get_obj_index(OBJ_VNUM_PLANT_C), 0);
			break;
		case 4:
		default:
			obj = create_object(get_obj_index(OBJ_VNUM_PLANT_D), 0);
			break;
		}

		obj_to_room(obj, room);
	}
}

void
spell_repel( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);


	if (room_is_affected (ch->in_room, gsn_repel))
	{
		Cprintf(ch, "This room is already protected.\n\r");
		return;
	}

	af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = modified_level;
    af.duration  = modified_level / 8;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    Cprintf(ch, "You protect this room against intruders.\n\r");
    act("$n protects this room from intruders.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
spell_abandon( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (room_is_affected (ch->in_room, gsn_abandon))
	{
		Cprintf(ch, "This room is already protected.\n\r");
		return;
	}

	if(IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
	|| IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)) {
		Cprintf(ch, "Not in this room.\n\r");
		return;
	}

	af.where     = TO_AFFECTS;
    	af.type      = sn;
    	af.level     = modified_level;
    	af.duration  = ch->level / 6;
    	af.modifier  = 0;
    	af.location  = APPLY_NONE;
    	af.bitvector = 0;
    	affect_to_room(ch->in_room, &af);

	SET_BIT(ch->in_room->room_flags, ROOM_NO_GATE);
	SET_BIT(ch->in_room->room_flags, ROOM_NO_RECALL);

	Cprintf(ch, "You create a space-time anchor to prevent magical transit.\n\r");
    	act("$n creates a space-time anchor to prevent magical transit.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
}


void
end_abandon(void* vo, int target)
{
	ROOM_INDEX_DATA* room = (ROOM_INDEX_DATA*) vo;
	REMOVE_BIT(room->room_flags, ROOM_NO_GATE);
	REMOVE_BIT(room->room_flags, ROOM_NO_RECALL);
}

void
spell_create_door(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
	int caster_level, modified_level;
	EXIT_DATA* exit;
	ROOM_INDEX_DATA* room;
	AFFECT_DATA af;
	int door;
	char dir[MAX_STRING_LENGTH];

	one_argument(target_name, dir);
	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if (dir[0] == '\0')
	{
		Cprintf(ch, "Which direction or door?\n\r");
		return;
	}

	room = ch->in_room;
	if (room == NULL)
	{
		Cprintf(ch, "Somethen's fissshy!\n\r");
		return;
	}

	if (IS_SET(room->room_flags, ROOM_LAW))
	{
		Cprintf(ch, "Not here.\n\r");
		return;
	}

	exit = NULL;
	if (!str_prefix(dir, "north"))
	{
		exit = room->exit[DIR_NORTH];
		door = DIR_NORTH;
	}
	else if (!str_prefix(dir, "east"))
	{
		exit = room->exit[DIR_EAST];
		door = DIR_EAST;
	}
	else if (!str_prefix(dir, "south"))
	{
		exit = room->exit[DIR_SOUTH];
		door = DIR_SOUTH;
	}
	else if (!str_prefix(dir, "west"))
	{
		exit = room->exit[DIR_WEST];
		door = DIR_WEST;
	}
	else if (!str_prefix(dir, "up"))
	{
		exit = room->exit[DIR_UP];
		door = DIR_UP;
	}
	else if (!str_prefix(dir, "down"))
	{
		exit = room->exit[DIR_DOWN];
		door = DIR_DOWN;
	}
	else
	{
		for (door = 0; door <= 5; door++)
		{
			if (room->exit[door] != NULL &&
				IS_SET(room->exit[door]->exit_info, EX_ISDOOR) &&
				room->exit[door]->keyword != NULL &&
				is_name(dir, room->exit[door]->keyword))
			{
				exit = room->exit[door];
				break;
			}
		}

		if (exit == NULL)
		{
			Cprintf(ch, "No such door\n\r");
			return;
		}
	}

	if (exit == NULL)
	{
		Cprintf(ch, "There is no exist in that direction.\n\r");
		return;
	}

	if (IS_SET(exit->exit_info, EX_ISDOOR))
	{
		Cprintf(ch, "There is already a door there.\n\r");
		return;
	}

	SET_BIT(exit->exit_info, EX_ISDOOR);
	SET_BIT(exit->exit_info, EX_CLOSED);
	SET_BIT(exit->exit_info, EX_NOPASS);

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = modified_level;
	af.duration  = number_range(2, 4);
	af.modifier  = door;
	af.location  = APPLY_NONE;
	af.bitvector = 0;
	affect_to_room(ch->in_room, &af);

	Cprintf(ch, "You create a magical door!\n\r");
	act("$n creates a magical door!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
}

void
end_create_door(void* vo, int target)
{
	ROOM_INDEX_DATA* room = (ROOM_INDEX_DATA*) vo;
	EXIT_DATA* exit;
	AFFECT_DATA* paf;

	paf = affect_find(room->affected, gsn_create_door);
	exit = room->exit[paf->modifier];
	REMOVE_BIT(exit->exit_info, EX_CLOSED);
	REMOVE_BIT(exit->exit_info, EX_NOPASS);
	REMOVE_BIT(exit->exit_info, EX_ISDOOR);
}

void
spell_tornado( int sn, int level, CHAR_DATA *ch, void *vo, int target ) {
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(ch->in_room->sector_type == SECT_WATER_SWIM
	|| ch->in_room->sector_type == SECT_WATER_NOSWIM
	|| ch->in_room->sector_type == SECT_INSIDE) {
		Cprintf(ch, "The winds cannot gather here.\n\r");
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = modified_level;
	af.duration  = modified_level / 12;
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = 0;
	affect_to_room (ch->in_room, &af);

	Cprintf(ch, "The winds begin to howl and rage, forming a mighty tornado!\n\r");
	act("The winds begin to howl and rage, forming a tornado!", NULL, NULL, NULL, TO_ROOM, POS_RESTING);

	return;
}

void
spell_whirlpool( int sn, int level, CHAR_DATA *ch, void *vo, int target ) {
	AFFECT_DATA af;
	int caster_level, modified_level;

	caster_level = get_caster_level(level);
	modified_level = get_modified_level(level);

	if(ch->in_room->sector_type != SECT_WATER_SWIM
	&& ch->in_room->sector_type != SECT_WATER_NOSWIM
	&& ch->in_room->sector_type != SECT_UNDERWATER
	&& ch->in_room->sector_type != SECT_SWAMP) {
		Cprintf(ch, "There isn't enough water to create a whirlpool here.\n\r");
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = modified_level;
	af.duration  = modified_level / 12;
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = 0;
	affect_to_room (ch->in_room, &af);

	Cprintf(ch, "The water begins to churn, forming a frothing whirlpool!\n\r");
	act("The water begins to churn, forming a frothing whirlpool!", NULL, NULL, NULL, TO_ROOM, POS_RESTING);

	return;
}
