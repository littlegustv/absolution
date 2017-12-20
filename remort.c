/* revitsion 1.1 - August 1 1999 - making it compilable under g++ */
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "merc.h"
#include "clan.h"

DECLARE_SPELL_FUN(spell_null);
DECLARE_DO_FUN(do_outfit);
extern void quit_character(CHAR_DATA *);
void undo_reclass(CHAR_DATA *);
void undo_remort(CHAR_DATA *);

/* Remort tables! */
const struct remort_data remort_list =
{
	/* class part */
	{
		{
			{"thief"},
			{"confusion", "weaponsmith", "substitution", "hunt", "bribe"}
		},

		{
			{"druid", "cleric"},
			{"call to arms", "wrath", "transferance", "substitution", "materialize"}
		},

		{
			{"mage", "conjurer", "invoker", "enchanter"},
			{"detonation", "mutate", "call to arms", "confusion", "weaponsmith"}
		},

		{
			{"paladin", "ranger", "warrior", "monk"},
			{"bashdoor", "hunt", "substitution", "drag", "tame animal", "retribution", "rescue"}
		},

		{
			{"runist"},
			{"substitution", "hunt", "materialize", "envenom", "confusion"},
		},

		{
			{NULL},
			{NULL}
		}
	},

	{
		{
			"human", "doppleganger", 1,
			{"shapeshift"},
			RES_CHARM, 0, 0
		},

		{
			"human", "medusa", 2,
			{"snake bite", "granite stare"},
			RES_POISON, 0, 0
		},

		{
			"elf", "forest elf", 1,
			{"reflection", "brew"},
			0, 0, 0
		},

		{
			"elf", "drow elf", 2,
			{"reflection", "primal rage"},
			0, 0, 0
		},

		{
			"dwarf", "mountain dwarf", 1,
			{"conversion", "wild swing"},
			0, 0, 0
		},

		{
			"dwarf", "grey dwarf", 2,
			{"turn magic", "wild swing", "enlargement"},
			0, 0, 0
		},

		{
			"giant", "hill giant", 1,
			{"beheading", "carve boulder", "block", "robustness"},
			0, 0, 0
		},

		{
			"giant", "cave giant", 2,
			{"beheading", "carve boulder", "cave bears", "animal skins"},
			0, 0, 0
		},

		{
			"hatchling", "xx", 1,
			{NULL},
			0, 0, 0
		},

		{
			"hatchling", "yy", 2,
			{NULL},
			0, 0, 0
		},

		{
			"black dragon", "onyx dragon", 1,
			{"tail attack", "cloud of poison", "retreat"},
			0, 0, 0
		},

		{
			"black dragon", "shadow dragon", 2,
			{"tail attack", "create shadow", "hide"},
			0, 0, 0
		},

		{
			"blue dragon", "sapphire dragon", 1,
			{"tail attack", "cone of fear", "gemology"},
			0, 0, 0
		},

		{
			"blue dragon", "sky dragon", 2,
			{"tail attack", "lightning spear", "presence"},
			0, 0, 0
		},

		{
			"green dragon", "emerald dragon", 1,
			{"tail attack", "wail", "loneliness"},
			0, 0, 0
		},

		{
			"green dragon", "earth dragon", 2,
			{"tail attack", "drakor", "shifting sands"},
			0, 0, 0
		},

		{
			"red dragon", "ruby dragon", 1,
			{"tail attack", "thrust", "loneliness"},
			0, 0, 0
		},

		{
			"red dragon", "fire dragon", 2,
			{"tail attack", "dragon bite", "scalemail"},
			0, 0, 0
		},

		{
			"white dragon", "mist dragon", 1,
			{"tail attack", "melior", "discordance", "misty cloak"},
			0, 0, 0
		},

		{
			"white dragon", "pearl dragon", 2,
			{"tail attack", "presence", "shedding", "aurora"},
			0, 0, 0
		},

		{
			"sliver", "stinging sliver", 1,
			{"web", "corpse drain"},
			0, 0, 0
        },

		{
			"sliver", "abyssal sliver", 2,
			{},
			0, 0, 0
		},
		{
			"troll", "two-headed troll", 1,
			{"regrowth", "bite"},
			0, 0, 0
		},

		{
			"troll", "spectral troll", 2,
			{"regrowth", "familiar", "fear"},
			0, AFF_INVISIBLE, 0
		},

		{
			"gargoyle", "sentinel gargoyle", 1,
			{"homonculus", "gore", "stone sleep", "mass protect"},
			0, 0, 0
		},

		{
            		"gargoyle", "haunting gargoyle", 2,
			{"shriek", "displace", "stone sleep", "mass protect"},
			0, 0, 0
        	},

		{
			"kirre", "sabre kirre", 1,
			{"hand to hand", "ambush", "concealment", "razor claws", "tail trip"},
			0,0,0
		},

		{
			"kirre", "dark kirre", 2,
			{"hand to hand", "quill armor", "dark feast", "razor claws", "tail trip"},
			0,0,0
		},

		{
			"marid", "nereid", 1,
			{"flood", "condensation", "water breathing", "lure", "shawl"},
			0,0,0
		},

		{
			"marid", "befouler", 2,
			{"flood", "condensation", "corruption", "dissolution"},
			0,0,0
		},

		{
			NULL, NULL, -1,
			{NULL},
			0, 0, 0
		},
	},

	{"throw", "taste", "butcher", "gravitation", "recall"}
};

/* reclass table!! */
const struct reclass_data reclass_list =
{
	{
		{
			"","",
			{NULL},
			0,0,0
		},

		{
                        "mage", "wizard",
                        {"prismatic sphere", "channel energy", "sharpen",
			"wizards eye", "beacon", "scribe", "attraction"},
                        0, 0, 0
                },

                {
                        "mage", "alchemist",
                        {"prismatic sphere", "create oil", "replicate",
			 "cryogenesis", "combine potion",
			 "embellish", "flesh golem"},
                        0, 0, 0
                },

                {
                        "conjurer", "shaman",
                        {"combine potion", "symbol", "voodoo doll", "repel",
			"cryogenesis", "flesh golem", "jinx"},
                        0, 0, 0
                },

		{
                        "conjurer", "sorceror",
                        {"combine potion", "symbol", "duplicate", "shock wave",
			"soul trap", "sunray", "scribe"},
                        0, 0, 0
                },

		{
                        "enchanter", "mystic",
                        {"remove alignment", "flag", "abandon", "attraction",
			"repel", "spell stealing", "jinx"},
                        0, 0, 0
                },

		{
                        "enchanter", "illusionist",
                        {"remove alignment", "flag", "shadow magic", "jinx",
			"oculary", "duplicate", "create door"},
                        0, 0, 0
                },

		{
                        "invoker", "necromancer",
                        {"oculary", "meteor swarm", "animate dead", "drain life",
			"skeletal warrior", "cryogenesis", "withstand death"},
                        0, 0, 0
                },

		{
                        "invoker", "diviner",
                        {"oculary", "meteor swarm", "delayed blast fireball", "true sight",
			"wizards eye", "inform", "spy"},
                        0, 0, 0
                },

		{
                        "cleric", "heretic",
                        {"drain life", "create oil", "moonbeam", "atheism", "animate dead",
			"cryogenesis", "withstand death", "nightmares"},
                        0, 0, 0
                },

		{
                        "cleric", "exorcist",
                        {"magic stone", "create oil", "life wave", "summon angel",
			"turn undead", "missionary"},
			/* silent prayer is free */
                        0, 0, 0
                },

		{
                        "druid", "hierophant",
                        {"winter storm", "animate tree", "whirlpool", "earth to mud",
			"sunray", "wildfire", "nature protection"},
                        0, 0, 0
                },

		{
                        "druid", "herbalist",
                        {"winter storm", "animate tree", "iron vigil",
			"nature protection", "plant", "shock wave", "tornado"},
                        0, 0, 0
                },

		{
                        "warrior", "warlord",
                        {"hand to hand", "dual wield", "push", "rally", "leadership",
			"gladiator"},
			/* martial arts and endurance/thicken is free */
                        0, 0, 0
                },

		{
                        "warrior", "barbarian",
                        {"hand to hand", "shield bash", "enrage", "cave in", "rip",
			"jump"},
			/* martial arts and endurance/thicken is free */
                        0, 0, 0
                },

		{
                        "paladin", "cavalier",
                        {"hand to hand", "shield bash", "charge", "fortify", "push",
			"jump"},
			/* martial arts and endurance/thicken is free */
                        0, 0, 0
                },

		{
                        "paladin", "templar",
                        {"hand to hand", "missionary", "summon angel",
			"faith", "recall", "lesser dispel", "hallow", "purify"},
			/* martial arts and endurance/thicken is free */
			/* silent prayer is free */
                        0, 0, 0
                },

		{
                        "ranger", "hermit",
                        {"hand to hand", "shield bash", "plant", "carve spear",
			"iron vigil", "kick"},
			/* martial arts and endurance/thicken is free */
			/* double kick is free */
                        0, 0, 0
                },

		{
                        "ranger", "bounty hunter",
                        {"dual wield", "iron vigil", "slide",
			"call to hunt", "hunter ball", "carve spear"},
			/* endurance/thicken is free, hunter bonus */
                        0, 0, 0
                },

		{
                        "thief", "assassin",
                        {"death blow", "ensnare", "jump", "sap",
			"spy", "slide", "push"},
                        0, 0, 0
                },

		{
                        "thief", "rogue",
			// Critical hit and Crippling Strike are free.
                        {"tumbling", "ensnare", "steal", "peek",
			"spy", "oculary"},
                        0, 0, 0
                },

		{
			"runist", "zealot",
			{"ignore wounds", "push", "symbol",
			 "zeal", "hurl", "death rune",
			 "hand to hand"},
			// Martial arts is free
			0, 0, 0
		},

		{
			"runist", "psion",
			{"jump", "push", "symbol", "iron vigil",
			 "soul blade", "fury", "trance"},
			0, 0, 0
		},

		{
			"monk", "ninja",
			{"stance: shadow", "dual wield", "shadow walk",
			 "ninjitsu", "blindfighting", "instant stand",
			 "evasion" },
			0, 0, 0
		},

		{
			"monk", "samurai",
			{"katana", "stance: kensai", "doublestrike",
                         "third eye", "quicken", "extreme damage",
                         "seppuku"},
			0, 0, 0
		},
		{
			NULL, NULL,
		  	{NULL},
		  	0,0,0
		},

	},

	{"throw", "rub"}
	/* speciality is free */
};

/*
 * used to calculate qp's needed for remorting
 *
 * returns qp's needed to remort
 */

int
remort_cp_qp(CHAR_DATA * ch)
{
	long int qp;
	int cp;

	if (IS_NPC(ch))
	{
		Cprintf(ch, "You cannot remort, your not a player!\n\r");
		return 0;
	}

	cp = ch->pcdata->points;

	// Base amount needed to remort.
	qp = 3000;

        // Modify based on cp. Less cp = more qp
	// This is a nice curve
	if(cp > 100) {
	        qp -= ( (cp - 100) * 10);
	}
	if(cp > 80) {
		qp -= ( (cp - 80) * 10);
	}
	if(cp > 60) {
		qp -= ( (cp - 60) * 10);
	}
	// Lose 10 qp per cp base
	qp -= cp * 10;

	// 880  qp base is 113 cp = 130 + 330 + 530 + 1130 = 2120
	// 1850 qp base is 85 cp  = 50 + 250 + 850 = 1150
	// 2280 qp base is 2280 qp base 66 cp  = 60 + 660 = 720
	// 2600 qp base is 40 cp  = 400

	// Modify by 500-1000 based on race/class combo.
	qp += 500 * (pc_race_table[ch->race].class_mult[ch->charClass] / 100.0);


	// Modify based on age
	qp = qp - ((get_age(ch) - 17) * 50);

	return (qp);
}

int remort_pts_level(CHAR_DATA* ch)
{
	char tmpsk[5][MAX_STRING_LENGTH];
	int totpts;
	bool defau;
	bool found;
	bool wm;
	bool tm;
	int sn;
	int sn2;
	char gotit[MAX_IN_GROUP][MAX_STRING_LENGTH];
	char gotit2[MAX_IN_GROUP][MAX_STRING_LENGTH];
	int index;

	wm = FALSE;
	tm = FALSE;
	defau = FALSE;
	found = FALSE;
	totpts = pc_race_table[ch->race].points;
	/* check for weaponsmaster, basic and default skills */
	memset(tmpsk, 0, sizeof(char) * 5 * MAX_STRING_LENGTH);
	memset(gotit, 0, sizeof(char) * MAX_IN_GROUP * MAX_STRING_LENGTH);
	memset(gotit2, 0, sizeof(char) * MAX_IN_GROUP * MAX_STRING_LENGTH);

	for (sn = 0; sn < MAX_GROUP; sn++)
	{
		if (ch->pcdata->group_known[sn] && !strcmp(group_table[sn].name, "weaponsmaster"))
			wm = TRUE;

		if (ch->pcdata->group_known[sn] && !strcmp(group_table[sn].name, "tattoo magic"))
			tm = TRUE;

		if (group_table[sn].name != NULL)
		{
			if (!strcmp(group_table[sn].name, class_table[ch->charClass].default_group))
			{
				if (ch->pcdata->group_known[sn])
					defau = TRUE;

				for (sn2 = 0; sn2 < MAX_IN_GROUP; sn2++)
				{
					if (group_table[sn].spells[sn2] != NULL)
					{
						strcpy(gotit[sn2], group_table[sn].spells[sn2]);
						if (!strcmp(group_table[sn].spells[sn2], "weaponsmaster"))
							wm = TRUE;
					}
				}
			}

			if (!strcmp(group_table[sn].name, class_table[ch->charClass].base_group))
			{
				for (sn2 = 0; sn2 < MAX_IN_GROUP; sn2++)
				{
					if (group_table[sn].spells[sn2] != NULL)
					{
						strcpy(gotit2[sn2], group_table[sn].spells[sn2]);
						if (!strcmp(group_table[sn].spells[sn2], "weaponsmaster"))
							wm = TRUE;
					}
				}
			}
		}
	}

/* PROCEED WITH REMORTING */

	/* Counts points for known skills */

	for (sn = 0; sn < 5; sn++)
	{
		if (pc_race_table[ch->race].skills[sn] == NULL)
			strcpy(tmpsk[sn], " ");
		else
			strcpy(tmpsk[sn], pc_race_table[ch->race].skills[sn]);
	}

	/* Counts points for known spell groups */

	for (sn = 0; sn < MAX_GROUP; sn++)
	{
		if (group_table[sn].name == NULL)
			break;

		found = FALSE;

		for (sn2 = 0; sn2 < MAX_IN_GROUP; sn2++)
		{
			if (gotit[sn2][0] != '\0' && defau == TRUE)
			{
				if (!strcmp(group_table[sn].name, gotit[sn2]))
					found = TRUE;
			}

			if (gotit2[sn2][0] != '\0')
			{
				if (!strcmp(group_table[sn].name, gotit2[sn2]))
					found = TRUE;
			}
		}

		/* K, check for race shit */
		for (index = 0; index < 5; index++)
		{
			if (pc_race_table[ch->race].skills[index] == NULL)
					break;

			if (!strcmp(group_table[sn].name, pc_race_table[ch->race].skills[index]))
				found = TRUE;
		}

		if (ch->pcdata->group_known[sn] &&
			found == FALSE &&
			group_table[sn].rating[ch->charClass] != -1)
		{
			totpts = totpts + group_table[sn].rating[ch->charClass];
		}
	}

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;

		found = FALSE;

		for (sn2 = 0; sn2 < MAX_IN_GROUP; sn2++)
		{
			if (gotit[sn2][0] != '\0' && defau == TRUE)
			{
				if (!strcmp(skill_table[sn].name, gotit[sn2]))
				{
					found = TRUE;
				}
			}
			if (gotit2[sn2][0] != '\0')
			{
				if (!strcmp(skill_table[sn].name, gotit2[sn2]))
				{
					found = TRUE;
				}
			}
		}

		if (wm == TRUE)
		{
			if (!strcmp(skill_table[sn].name, "axe")
				|| !strcmp(skill_table[sn].name, "dagger")
				|| !strcmp(skill_table[sn].name, "flail")
				|| !strcmp(skill_table[sn].name, "mace")
				|| !strcmp(skill_table[sn].name, "polearm")
				|| !strcmp(skill_table[sn].name, "spear")
				|| !strcmp(skill_table[sn].name, "sword")
				|| !strcmp(skill_table[sn].name, "whip"))
			{
				found = TRUE;
			}
		}

		if (tm == TRUE) {
			if(sn == gsn_paint_lesser
			|| sn == gsn_paint_greater
			|| sn == gsn_paint_power)
				found = TRUE;
		}

		/* K, check for race shit */
		for (index = 0; index < 5; index++)
		{
			if (pc_race_table[ch->race].skills[index] == NULL)
					break;

			if (!strcmp(skill_table[sn].name, pc_race_table[ch->race].skills[index]))
				found = TRUE;
		}

		// Some exceptions to the rules.
		// All kirre get hand to hand and disarm, but shouldn't pay.
		if(ch->race == race_lookup("kirre")
		&& sn == gsn_hand_to_hand) {
			found = TRUE;
		}

		if(ch->charClass == class_lookup("warrior")
		|| ch->charClass == class_lookup("ranger")
		|| ch->charClass == class_lookup("paladin")) {
			if(ch->remort
			&& sn == gsn_rescue)
				found = TRUE;
		}

		if ((skill_table[sn].skill_level[ch->charClass]) < 53
			&& skill_table[sn].spell_fun == spell_null
			&& ch->pcdata->learned[sn] > 0
			&& found == FALSE
			&& skill_table[sn].rating[ch->charClass] != -1
			&& skill_table[sn].remort == 0
			&& strcmp(skill_table[sn].name, "wands")
			&& strcmp(skill_table[sn].name, "scrolls")
			&& strcmp(skill_table[sn].name, "recall")
			&& strcmp(skill_table[sn].name, "staves")
			&& strcmp(skill_table[sn].name, "craft item")
			&& strcmp(skill_table[sn].name, tmpsk[0])
			&& strcmp(skill_table[sn].name, tmpsk[1])
			&& strcmp(skill_table[sn].name, tmpsk[2])
			&& strcmp(skill_table[sn].name, tmpsk[3])
			&& strcmp(skill_table[sn].name, tmpsk[4]))
		{
			totpts = totpts + skill_table[sn].rating[ch->charClass];
		}
	}

	return totpts;
}

void remort_reset_char(CHAR_DATA* ch, int qp, int pts)
{
	char buf[MAX_STRING_LENGTH];
	int sn;
	int wr;
	int i;
	int past;
	OBJ_DATA* obj, *obj_next;

	/* Set all known skills and spells at 1% */
	die_follower(ch);
	ch->deity_points = 0;

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;
		if (ch->level < skill_table[sn].skill_level[ch->charClass]
			|| ch->pcdata->learned[sn] < 1 /* skill is not known */ )
			continue;

		ch->pcdata->learned[sn] = 1;
	}

	ch->pcdata->learned[gsn_recall] = 50;


	if (IS_SET(ch->act, PLR_KILLER))
	{
		REMOVE_BIT(ch->act, PLR_KILLER);
	}

	if (IS_SET(ch->act, PLR_THIEF))
	{
		REMOVE_BIT(ch->act, PLR_THIEF);
	}

	for (i = 0; i < MAX_STATS; i++)
		ch->perm_stat[i] = pc_race_table[ch->race].stats[i];

	ch->perm_stat[class_table[ch->charClass].attr_prime] += 3;

	for (obj = ch->carrying; obj != NULL; obj = obj_next)
        {
                obj_next = obj->next_content;
                if(obj_is_affected(obj, gsn_paint_lesser)
                || obj_is_affected(obj, gsn_paint_greater)
                || obj_is_affected(obj, gsn_paint_power)) {
			unequip_char(ch, obj);
                        extract_obj(obj);
		}
        }

	for (wr = WEAR_LIGHT; wr < MAX_WEAR; wr++)
	{
		if ((obj = get_eq_char(ch, wr)) != NULL)
			unequip_char(ch, obj);
	}

	past = 0;
	if ((ch->level == 52) || (ch->level == 53))
		past = ch->level;
	else if ((get_trust(ch) == 52) || (get_trust(ch) == 53))
		past = ch->trust;

	ch->wimpy = 0;
	ch->level = 1;
	ch->pcdata->points = pts;
	ch->exp = exp_per_level(ch, ch->pcdata->points);
	ch->pcdata->last_level = 0;
	ch->questpoints = ch->questpoints - qp;
	ch->played_perm = 0;
	ch->played = 0;
	ch->max_mana = 100;
	ch->max_hit = 20;
	ch->max_move = 100;
	ch->train = 3;
	ch->practice = 6;
	ch->trust = past;
	sprintf(buf, "the %s",
			title_table[ch->charClass][ch->level]
			[ch->sex == SEX_FEMALE ? 1 : 0]);
	set_title(ch, buf);

	Cprintf(ch, "The gods reincarnate you into a more powerful creature!\n\r");
	
	Cprintf(ch, "You fall into a deep sleep... and awaken somewhere else.\n\r");

	if(ch->in_room->area->continent == 0) { 
		char_from_room(ch);
		char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
	}
	else {
		char_from_room(ch);
		char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL_DOMINIA));
	}

	ch->position = POS_SLEEPING;
	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
}

int remort_class_index(CHAR_DATA* ch)
{
	int i;
	int j;

	for(i = 0; remort_list.class_info[i].classes[0] != NULL; i++)
	{
		for(j = 0; remort_list.class_info[i].classes[j] != NULL; j++)
		{
			if(strcmp(class_table[ch->charClass].name, remort_list.class_info[i].classes[j]) == 0)
			{
				return i;
			}
		}
	}

	return -1;
}

int remort_race_index(CHAR_DATA* ch, int index)
{
	int i;
	int j;

	j = 0;
	for(i = 0; remort_list.race_info[i].race != NULL; i++)
	{
		if(strcmp(pc_race_table[ch->race].name, remort_list.race_info[i].race) == 0)
		{
			j++;
			if(j == index)
			{
				return i;
			}
		}
	}

	return -1;
}

int reclass_index(CHAR_DATA* ch, int index)
{
	int i;
	int j;

	j = 0;
	for (i = 0; reclass_list.class_info[i].ch_class != NULL; i++)
	{
		if (strcmp(class_table[ch->charClass].name, reclass_list.class_info[i].ch_class) == 0)
		{
			j++;
			if (j == index)
			{
				return i;
			}
		}
	}

	return -1;
}

bool is_reclass_spell(int index, int sn)
{
	int i;

	if (index < 0 || index > MAX_RECLASS)
		return FALSE;

	for(i = 0; i < MAX_REM_SKILL; i++)
	{
		if(reclass_list.class_info[index].skills[i] != NULL &&
		   reclass_list.class_info[index].skills[i][0] != '\0' &&
		   sn == skill_lookup(reclass_list.class_info[index].skills[i]))
		{
			return TRUE;
		}
	}

	return FALSE;
}

int reclass_lookup(char* arg)
{
	int i;
	for(i = 0; reclass_list.class_info[i].re_class != NULL; i++)
	{
		if(strcmp(arg, reclass_list.class_info[i].re_class) == 0)
		{
			return i;
		}
	}

	return -1;
}

void
do_remort(CHAR_DATA * ch, char *argument)
{
	char sub1[MAX_STRING_LENGTH];
	char sub2[MAX_STRING_LENGTH];
	int qp;
	int totpts;
	int sub;
	int index;
	int i;


	if (IS_NPC(ch))
	{
		Cprintf(ch, "You may not remort, you are not a player.\n\r");
		return;
	}

	if (ch->level >= 54)
	{
		Cprintf(ch, "You're an Immortal, such an action would defeat the purpose!\n\r");
		return;
	}

	if (ch->remort > 0 && strcmp(argument, "forsake"))
	{
        	Cprintf(ch, "You have already remorted.\n\r");
        	return;
	}
	else if(ch->remort > 0 && !strcmp(argument, "forsake")) {
        	// This method will undo reclass if they meet the requirements.
        	undo_remort(ch);
        	return;
	}

	qp = remort_cp_qp(ch);
	totpts = remort_pts_level(ch);

	index = remort_race_index(ch, 1);
	strcpy(sub1, index != -1 ? remort_list.race_info[index].remort_race : "");
	index = remort_race_index(ch, 2);
	strcpy(sub2, index != -1 ? remort_list.race_info[index].remort_race : "");

	if ((ch->questpoints < qp) || (ch->level < 51))
	{
		Cprintf(ch, "You will require %d quest points to remort (%d exp per level).\n\r", qp, exp_per_level(ch, totpts));
		Cprintf(ch, "Your choices are as follows:\n\r");
		Cprintf(ch, "'remort %s' or 'remort %s'\n\r", sub1, sub2);
		return;
	}


	/* GET SKILLS FROM STRUCT */
	sub = 0;
	if(strcmp(argument, sub1) == 0)
	{
		sub = 1;
	}
	else if(strcmp(argument, sub2) == 0)
	{
		sub = 2;
	}
	else
	{
		Cprintf(ch, "After you remort you will have %d CPs for a total of %d exp per level.\n\r", totpts, exp_per_level(ch, totpts));
		Cprintf(ch, "Your choices are as follows:\n\r");
		Cprintf(ch, "'remort %s' or 'remort %s'\n\r", sub1, sub2);
		return;
	}

	if(is_affected(ch, gsn_gladiator)
        || is_affected(ch, gsn_spell_stealing)) {
                Cprintf(ch, "Your temporary skills prevent your transformation.\n\r");
                return;
        }

	// Some affects don't work very well.
	affect_strip(ch, gsn_transferance);

	/* K, we are set, and got a valid remort type, lets get busy */
	/* class part */
	index = remort_class_index(ch);
	for(i = 0; i < MAX_REM_SKILL; i++)
	{
		if(remort_list.class_info[index].skills[i] != NULL &&
		   remort_list.class_info[index].skills[i][0] != '\0')
		{
			ch->pcdata->learned[skill_lookup(remort_list.class_info[index].skills[i])] = 1;
		}
	}

	/* race part */
	index = remort_race_index(ch, sub);
	for(i = 0; i < MAX_REM_SKILL; i++)
	{
		if(remort_list.race_info[index].skills[i] != NULL &&
		   remort_list.race_info[index].skills[i][0] != '\0')
		{
			ch->pcdata->learned[skill_lookup(remort_list.race_info[index].skills[i])] = 1;
		}
	}
	ch->res_flags |= remort_list.race_info[index].res_bits;
	ch->affected_by |= remort_list.race_info[index].aff_bits;

	/* all */
	for(i = 0; i < MAX_REM_SKILL; i++)
	{
		if(remort_list.skills[i] != NULL &&
		   remort_list.skills[i][0] != '\0')
		{
			ch->pcdata->learned[skill_lookup(remort_list.skills[i])] = 1;
		}
	}
	ch->rem_sub = sub;


	/* sloppy hack for slivers.. damnit, this should be done liek cloudkill! */
	if(ch->race == race_lookup("sliver"))
		ch->pcdata->learned[gsn_telepathy] = 100;

	if(ch->race == race_lookup("sliver") && sub == 2)
	{
		if (clan_table[ch->clan].pkiller)
		{
			ch->pcdata->learned[gsn_rain_of_tears] = 1;
		}
		else
		{
			ch->pcdata->learned[gsn_hailstorm] = 1;
		}
	}

	/* K, added stuff, now reset char */
	remort_reset_char(ch, qp, totpts);
	ch->remort++;
	return;
}

void
do_reclass(CHAR_DATA * ch, char *argument)
{
	char sub1[MAX_STRING_LENGTH];
	char sub2[MAX_STRING_LENGTH];
	int qp;
	int totpts;
	int index;
	int i;
	int sub;


	if (IS_NPC(ch))
	{
		Cprintf(ch, "You may not reclass, you are not a player.\n\r");
		return;
	}

        if (ch->level >= 54)
        {
                Cprintf(ch, "You're an Immortal, such an action would defeat the purpose!\n\r");
                return;
        }

	if (ch->reclass > 0 && strcmp(argument, "forsake"))
	{
		Cprintf(ch, "You have already reclassed.\n\r");
		return;
	}
	else if(ch->reclass > 0 && !strcmp(argument, "forsake")) {
		// This method will undo reclass if they meet the requirements.
		undo_reclass(ch);
		return;
	}

	qp = remort_cp_qp(ch);
	totpts = remort_pts_level(ch);

	index = reclass_index(ch, 1);

	strcpy(sub1, index != -1 ? reclass_list.class_info[index].re_class : "");
	index = reclass_index(ch, 2);
	strcpy(sub2, index != -1 ? reclass_list.class_info[index].re_class : "");

	if ((ch->questpoints < qp) || (ch->level < 51))
	{
		Cprintf(ch, "You will require %d quest points to reclass (%d exp per level).\n\r", qp, exp_per_level(ch, totpts));
		Cprintf(ch, "Your choices are as follows:\n\r");
		Cprintf(ch, "'reclass %s' or 'reclass %s'\n\r", sub1, sub2);
		return;
	}


	/* GET SKILLS FROM STRUCT */
	sub = 0;
	if(strcmp(argument, sub1) == 0)
	{
		sub = 1;
	}
	else if(strcmp(argument, sub2) == 0)
	{
		sub = 2;
	}
	else
	{
		Cprintf(ch, "After you reclass you will have %d CPs for a total of %d exp per level.\n\r", totpts, exp_per_level(ch, totpts));
		Cprintf(ch, "Your choices are as follows:\n\r");
		Cprintf(ch, "'reclass %s' or 'reclass %s'\n\r", sub1, sub2);
		return;
	}

	// Don't let them fubar their cps
	if(is_affected(ch, gsn_gladiator)
        || is_affected(ch, gsn_spell_stealing)) {
                Cprintf(ch, "Your temporary skills prevent your transformation.\n\r");
                return;
        }

	// Some affects don't work very well
	affect_strip(ch, gsn_razor_claws);
        affect_strip(ch, gsn_transferance);
        affect_strip(ch, gsn_robustness);
        affect_strip(ch, gsn_enlargement);
        affect_strip(ch, gsn_stance_turtle);

	/* K, we are set, and got a valid remort type, lets get buisy */

	/* class part */
	index = reclass_index(ch, sub);
	for(i = 0; i < MAX_REM_SKILL; i++)
	{
		if(reclass_list.class_info[index].skills[i] != NULL &&
		   reclass_list.class_info[index].skills[i][0] != '\0')
		{
			ch->pcdata->learned[skill_lookup(reclass_list.class_info[index].skills[i])] = 1;
		}
	}
	ch->res_flags |= reclass_list.class_info[index].res_bits;
	ch->affected_by |= reclass_list.class_info[index].aff_bits;

	/* all */
	for(i = 0; i < MAX_REM_SKILL; i++)
	{
		if(reclass_list.skills[i] != NULL &&
		   reclass_list.skills[i][0] != '\0')
		{
			ch->pcdata->learned[skill_lookup(reclass_list.skills[i])] = 1;
		}
	}
	ch->reclass = index;

	// Fix assassins
	if(index == reclass_lookup("assassin")) {
		ch->pcdata->learned[gsn_steal] = 0;
		ch->pcdata->learned[gsn_peek] = 0;
	}

	/* K, added stuff, now reset char */
	remort_reset_char(ch, qp, totpts);
        return;
}

void undo_reclass(CHAR_DATA *ch) {
	int index;
	int i;
	OBJ_DATA *key, *obj, *obj_next;
	int found;
	CHAR_DATA *master; // The mob required for the transformation

	found = FALSE;
	for(master = ch->in_room->people; master != NULL; master = master->next_in_room) {
		if(master->spec_fun == spec_lookup("spec_unremort")) {
			found = TRUE;
			break;
		}
	}

	if(!found) {
		Cprintf(ch, "You may only forsake your reclass in an appropriate location.\n\r");
		return;
	}
	

	// Check for the key item
	key = create_object(get_obj_index(OBJ_VNUM_UNRECLASS_ITEM), 0);
	found = FALSE;

	for(obj = ch->carrying; obj != NULL; obj = obj_next) {
		obj_next = obj->next;
		if(obj->pIndexData->vnum == key->pIndexData->vnum) {
			found = TRUE;
			break;
		}
	}

	if(!found) {
		Cprintf(ch, "Without a proper offering you can't forsake your reclass.\n\r");
		return;
	}
	
	// Don't let them fubar their cps
        if(is_affected(ch, gsn_gladiator)
        || is_affected(ch, gsn_spell_stealing)) {
            Cprintf(ch, "Your temporary skills prevent your transformation.\n\r");
            return;
        }
		
	extract_obj(obj);

	// Don't let them fubar their cps
	if(is_affected(ch, gsn_gladiator)
	|| is_affected(ch, gsn_spell_stealing)) {
		Cprintf(ch, "Your temporary skills prevent your transformation.\n\r");
		return;
	}

	// Some affects don't work very well
	affect_strip(ch, gsn_transferance);

	/* class part */
	index = ch->reclass;
	for(i = 0; i < MAX_REM_SKILL; i++)
	{
		if(reclass_list.class_info[index].skills[i] != NULL 
		&& reclass_list.class_info[index].skills[i][0] != '\0')
		{
			ch->pcdata->learned[skill_lookup(reclass_list.class_info[index].skills[i])] = 0;
		}
	}
	
	REMOVE_BIT(ch->res_flags, reclass_list.class_info[index].res_bits);
	REMOVE_BIT(ch->affected_by, reclass_list.class_info[index].aff_bits);

	/* all */
	for(i = 0; i < MAX_REM_SKILL; i++)
	{
		if(reclass_list.skills[i] != NULL 
		&& reclass_list.skills[i][0] != '\0')
		{
			ch->pcdata->learned[skill_lookup(reclass_list.skills[i])] = 0;
		}
	}

	ch->pcdata->specialty = 0;
	ch->reclass = 0;

	Cprintf(ch, "With a slight darkening of your vision, your reclass is ripped away!\n\r");

	return;
}

void undo_remort(CHAR_DATA *ch) {
        int index;
        int i;
        OBJ_DATA *key, *obj, *obj_next;
        int found;
        int sub;

        CHAR_DATA *master; // The mob required for the transformation

        found = FALSE;
        for(master = ch->in_room->people; master != NULL; master = master->next_in_room) {
                if(master->spec_fun == spec_lookup("spec_unremort")) {
                        found = TRUE;
                        break;
                }
        }
        if(!found) {
                Cprintf(ch, "You may only forsake your remort in an appropriate location.\n\r");
                return;
        }

        // Check for the key item
        key = create_object(get_obj_index(OBJ_VNUM_UNREMORT_ITEM), 0);
        found = FALSE;

        for(obj = ch->carrying; obj != NULL; obj = obj_next) {
                obj_next = obj->next;
                if(obj->pIndexData->vnum == key->pIndexData->vnum) {
                        found = TRUE;
                        break;
                }
        }
        if(!found) {
                Cprintf(ch, "Without a proper offering you can't forsake your remort.\n\r");
                return;
        }

	// Don't let them fubar their cps
        if(is_affected(ch, gsn_gladiator)
        || is_affected(ch, gsn_spell_stealing)) {
                Cprintf(ch, "Your temporary skills prevent your transformation.\n\r");
                return;
        }

        extract_obj(obj);        

        /* K, we are set, and got a valid remort type, lets get busy */
        /* class part */
        index = remort_class_index(ch);
	sub = ch->rem_sub;
        for(i = 0; i < MAX_REM_SKILL; i++)
        {
                if(remort_list.class_info[index].skills[i] != NULL &&
                   remort_list.class_info[index].skills[i][0] != '\0')
                {
                        ch->pcdata->learned[skill_lookup(remort_list.class_info[index].skills[i])] = 0;
                }
        }

 	/* race part */
        index = remort_race_index(ch, sub);
        for(i = 0; i < MAX_REM_SKILL; i++)
        {
                if(remort_list.race_info[index].skills[i] != NULL &&
                   remort_list.race_info[index].skills[i][0] != '\0')
                {
                        ch->pcdata->learned[skill_lookup(remort_list.race_info[index].skills[i])] = 0;
                }
        }

        REMOVE_BIT(ch->res_flags, remort_list.race_info[index].res_bits);
        REMOVE_BIT(ch->affected_by, remort_list.race_info[index].aff_bits);

                /* all */
        for(i = 0; i < MAX_REM_SKILL; i++)
        {
                if(remort_list.skills[i] != NULL &&
                   remort_list.skills[i][0] != '\0')
                {
                        ch->pcdata->learned[skill_lookup(remort_list.skills[i])] = 0;
                }
        }
	ch->remort = 0;
        ch->rem_sub = 0;

        /* sloppy hack for slivers.. damnit, this should be done liek cloudkill! */
        if(ch->race == race_lookup("sliver"))
                ch->pcdata->learned[gsn_telepathy] = 0;

        if(ch->race == race_lookup("sliver") && sub == 2)
        {
                ch->pcdata->learned[gsn_rain_of_tears] = 0;
                ch->pcdata->learned[gsn_hailstorm] = 0;
        }


        Cprintf(ch, "With a slight darkening of your vision, your remort is ripped away!\n\r");

        return;
}

