/***************************************************************************
 *	Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer, 	   *
 *	Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.	   *
 *										   *
 *	Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael		   *
 *	Chastain, Michael Quan, and Mitchell Tse.				   *
 *										   *
 *	In order to use any part of this Merc Diku Mud, you must comply with	   *
 *	both the original Diku license in 'license.doc' as well the Merc	   *
 *	license in 'license.txt'.  In particular, you may not remove either of	   *
 *	these copyright notices.						   *
 *										   *
 *	Much time and thought has gone into this software and you are		   *
 *	benefitting.  We hope that you share your changes too.	What goes	   *
 *	around, comes around.							   *
 ***************************************************************************/

/***************************************************************************
 *	  ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
 *	  ROM has been brought to you by the ROM consortium		   *
 *		  Russ Taylor (rtaylor@pacinfo.com) 			   *
 *		  Gabrielle Taylor (gtaylor@pacinfo.com)		   *
 *		  Brian Moore (rom@rom.efn.org) 			   *
 *	  By using this code, you have agreed to follow the terms of the   *
 *	  ROM license, in the file Rom24/doc/rom.license		   *
 ***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "stats.h"
#include "utils.h"


bool check_social(CHAR_DATA * ch, char *command, char *argument);
char last_command[MAX_STRING_LENGTH];



/*
 * Command logging types.
 */
#define LOG_NORMAL		  0
#define LOG_ALWAYS		  1
#define LOG_NEVER		  2



/*
 * Log-all switch.
 */
bool fLogAll = FALSE;



/*
 * Command table.
 */
const struct cmd_type cmd_table[] =
{
	{"at", do_at, POS_DEAD, L4, TRUE, LOG_NORMAL, 1, FALSE},
	{"affects", do_affects, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"ambush", do_ambush, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"aiming", do_aiming, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"animate", do_animate_tattoo, POS_FIGHTING, 0, TRUE, LOG_NORMAL, TRUE},
	{"atm", do_atm, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"areas", do_areas, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"alia", do_alia, POS_DEAD, 0, TRUE, LOG_NORMAL, 0, FALSE},
	{"alias", do_alias, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
        {"assassinate", do_perform_death_blow, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"assume", do_assume, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autolist", do_autolist, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autoassist", do_autoassist, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autoplayerassist", do_playerassist, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"automobassist", do_mobassist, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autoexit", do_autoexit, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autogold", do_autogold, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autoloot", do_autoloot, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autosac", do_autosac, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autotrash", do_autotrash, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autotitle", do_autotitle, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autosplit", do_autosplit, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autovalue", do_autovalue, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"afk", do_afk, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"auction", do_auction, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"answer", do_answer, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"appraise", do_appraise, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"advance", do_advance, POS_DEAD, ML, TRUE, LOG_ALWAYS, 1, FALSE},
	{"allow", do_allow, POS_DEAD, L2, TRUE, LOG_ALWAYS, 1, FALSE},
	{"accept", do_accept, POS_DEAD, 52, TRUE, LOG_ALWAYS, 1, FALSE},
	{"addrecruite", do_addrecruite, POS_STANDING, 53, TRUE, LOG_ALWAYS, 1, FALSE},
	{"addrecruiter", do_addrecruiter, POS_STANDING, 53, TRUE, LOG_ALWAYS, 1, FALSE},
//	{"asave", do_asave, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
//	{"alist", do_alist, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
//	{"aedit", do_aedit, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},

	{"buy", do_buy, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"boost", do_boost, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"butcher", do_butcher, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"brief", do_brief, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"beep", do_beep, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"brandish", do_brandish, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"brew", do_brew, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"breathe", do_breath, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"backstab", do_backstab, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"bash", do_bash, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"barrage", do_barrage, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"bashdoor", do_bash_door, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"bs", do_backstab, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 0, TRUE},
//    {"bedit", do_bedit, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"berserk", do_berserk, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"bite", do_bite, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"bribe", do_bribe, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"bounty", do_bounty, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"board", do_board, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"block", do_block, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"blank", do_blank, POS_DEAD, 54, TRUE, LOG_NORMAL, 0, FALSE},
	{"ban", do_ban, POS_DEAD, L2, TRUE, LOG_ALWAYS, 1, FALSE},

	{"cast", do_cast, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"call to hunt", do_call_to_hunt, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"cave in", do_cave_in, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"channels", do_channels, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
        {"chakra", do_chakra, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"charge", do_charge, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"chi", do_chi, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"choke", do_choke_hold, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"clan", do_clantalk, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"clanreport", do_clan_report, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"cheat", do_cheat, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"combine potion", do_combine_potion, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"commands", do_commands, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"compare", do_compare, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"consider", do_consider, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"count", do_count, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"credits", do_credits, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"craft", do_craft, POS_RESTING, 0, FALSE, LOG_NORMAL, 1, FALSE},
	{"crash", do_crash, POS_SLEEPING, 60, FALSE, LOG_ALWAYS, 1, FALSE},
	{"colour", do_colour, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"color", do_colour, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"autocombine", do_combine, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"compact", do_compact, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"cgoss", do_cgoss, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"close", do_close, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"convert", do_conversion, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"ctf", do_ctf, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"carve boulder", do_carve, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"cure plague", do_cure_plague, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"check", do_check, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"clone", do_clone, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"claim", do_claim, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, FALSE},

	{"down", do_down, POS_STANDING, 0, TRUE, LOG_NEVER, 0, TRUE},
	{"description", do_description, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
        {"demon", do_demon_fist, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"feast", do_dark_feast, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"delet", do_delet, POS_DEAD, 0, TRUE, LOG_ALWAYS, 0, FALSE},
	{"delete", do_delete, POS_STANDING, 0, TRUE, LOG_ALWAYS, 1, FALSE},
	{"deaf", do_deaf, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"drink", do_drink, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"drop", do_drop, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"draw", do_draw, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"dirt", do_dirt, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"disarm", do_disarm, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"drag", do_drag, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"dragon", do_dragon_kick, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1 ,TRUE},
	/*{"dns", do_check_dns, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE}, */
	{"dump", do_dump, POS_DEAD, ML, TRUE, LOG_ALWAYS, 0, FALSE},
	{"deny", do_deny, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"dice", do_dice, POS_DEAD, L6, TRUE, LOG_NEVER, 1, FALSE},
	{"disconnect", do_disconnect, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"dispvnum", do_dispvnum, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"disperse", do_disperse, POS_DEAD, L4, TRUE, LOG_NORMAL, 1, FALSE},
	{"disperseroom", do_disperseroom, POS_DEAD, L3, TRUE, LOG_NORMAL, 1, FALSE},
	{"delegate", do_delegate, POS_DEAD, 1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"double", do_double, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"droprecruite", do_droprecruite, POS_STANDING, 53, TRUE, LOG_ALWAYS, 1, FALSE},
	{"droprecruiter", do_droprecruiter, POS_STANDING, 53, TRUE, LOG_ALWAYS, 1, FALSE},
        {"droppatron", do_droppatron, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"dropvassal", do_dropvassal, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"divorce", do_divorce, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},

	{"east", do_east, POS_STANDING, 0, TRUE, LOG_NEVER, 0, TRUE},
        {"eagle", do_eagle_claw, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"enrage", do_enrage, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"evade", do_evasion, POS_FIGHTING, 1, TRUE, LOG_NORMAL, 1, TRUE},
	{"trap", do_ensnare, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"exits", do_exits, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"equipment", do_equipment, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"eqoutput", do_eqoutput, POS_RESTING, 60, TRUE, LOG_NORMAL, 1, TRUE},
	{"examine", do_examine, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"emote", do_emote, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{",", do_emote, POS_RESTING, 0, TRUE, LOG_NORMAL, 0, TRUE},
	{"eat", do_eat, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"envenom", do_envenom, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"enter", do_enter, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"echo", do_recho, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
//	{"edit", do_olc, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, TRUE},

	{"fill", do_fill, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"flee", do_flee, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"follow", do_follow, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"flag", do_flag, POS_DEAD, 59, TRUE, LOG_ALWAYS, 1, FALSE},
	{"freeze", do_freeze, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"force", do_force, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
//	{"fstat", do_fstat, POS_DEAD, 56, FALSE, LOG_NORMAL, 1, FALSE},
	{"fixcp", do_fixcp, POS_RESTING, 0, FALSE, LOG_NORMAL, 1, FALSE},
	{"fury", do_fury, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
//	{"fuck", do_fuck, POS_DEAD, L3, TRUE, LOG_ALWAYS, 1, FALSE},
	{"focus", do_focus, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},

	{"get", do_get, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"goto", do_goto, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"gaze", do_granite_stare, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"group", do_group, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"guild", do_guild, POS_DEAD, L2, TRUE, LOG_ALWAYS, 1, FALSE},
	{"gossip", do_gossip, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{".", do_gossip, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 0, TRUE},
	{"grats", do_grats, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"gtell", do_gtell, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{";", do_gtell, POS_DEAD, 0, TRUE, LOG_NORMAL, 0, TRUE},
	{"gemology", do_gemology, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"give", do_give, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"gain", do_gain, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, FALSE},
//	{"game", do_game, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"gladiator", do_gladiator, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"groups", do_groups, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"gravitation", do_gravitation, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"guardian", do_protection, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"gift", do_gift, POS_DEAD, L5, TRUE, LOG_NORMAL, 1, FALSE},
	{"gecho", do_echo, POS_DEAD, L3, TRUE, LOG_ALWAYS, 1, FALSE},

	{"help", do_help, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"hush", do_hush, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"heal", do_heal, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"hold", do_wear, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"hunt", do_hunt, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"hide", do_hide, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"hal", do_hal, POS_DEAD, L1, TRUE, LOG_NORMAL, 0, FALSE},
	{"half", do_half, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"holylight", do_holylight, POS_DEAD, L5, TRUE, LOG_NORMAL, 1, FALSE},
	{"hunter ball", do_hunter_ball, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"hurl", do_hurl, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
//	{"hedit", do_hedit, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},

	{"inventory", do_inventory, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"info", do_groups, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"incognito", do_incognito, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"invis", do_invis, POS_DEAD, L6, TRUE, LOG_NORMAL, 0, FALSE},
	{"immtalk", do_immtalk, POS_DEAD, L5, TRUE, LOG_NORMAL, 1, FALSE},
	{":", do_immtalk, POS_DEAD, L5, TRUE, LOG_NORMAL, 0, FALSE},
	{"imotd", do_imotd, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
//	{"ident", do_ident, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},

	{"junk", do_sacrifice, POS_RESTING, 0, TRUE, LOG_NORMAL, 0, TRUE},
	{"join", do_join, POS_DEAD, 0, TRUE, LOG_ALWAYS, 1, FALSE},
	{"jump", do_jump, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},

	{"kill", do_kill, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"kick", do_kick, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},

	{"look", do_look, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"lookup", do_lookup, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"leadchan", do_leadchan, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"lore", do_lore, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"list", do_list, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"linkdead", do_linkdead, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"lock", do_lock, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"lay hands", do_lay_hands, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"lair", do_lair, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"load", do_load, POS_DEAD, L6, TRUE, LOG_ALWAYS, 1, FALSE},
	{"log", do_log, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"lag", do_lag, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"loner", do_loner, POS_DEAD, 1, TRUE, LOG_ALWAYS, 1, FALSE},

	{"macro", do_macro, POS_DEAD, 3, TRUE, LOG_NORMAL, 1, FALSE},
	{"mark", do_ready_death_blow, POS_STANDING, 0, TRUE, LOG_NORMAL, TRUE},
	{"music", do_music, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"motd", do_motd, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
//	{"mob", do_mob, POS_DEAD, 0, TRUE, LOG_NEVER, 0, FALSE},
	{"mobassist", do_mobassist, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"mind", do_sliver_chan, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"murde", do_murde, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 0, TRUE},
	{"mystats", do_mystats, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	/*{"menuolc", do_edit, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},*/
	{"maxstats", do_maxstats, POS_DEAD, ML, TRUE, LOG_ALWAYS, 1, FALSE},
//	{"macdump", do_mac_dump, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"mobdump", do_mobdump, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"mset", do_mset, POS_DEAD, L2, TRUE, LOG_ALWAYS, 1, FALSE},
	{"mload", do_mload, POS_DEAD, L6, TRUE, LOG_ALWAYS, 1, FALSE},
	{"memberlist", do_member_list, POS_DEAD, L1, TRUE, LOG_NORMAL, 1, FALSE},
	{"memory", do_memory, POS_DEAD, L4, TRUE, LOG_NORMAL, 1, FALSE},
	{"mwhere", do_mwhere, POS_DEAD, L6, TRUE, LOG_ALWAYS, 1, FALSE},
	{"mstat", do_mstat, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"mfind", do_mfind, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
//	{"mpdump", do_mpdump, POS_DEAD, L6, TRUE, LOG_NEVER, 1, FALSE},
//	{"mpstat", do_mpstat, POS_DEAD, L6, TRUE, LOG_NEVER, 1, FALSE},
	{"marry", do_marry, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
//	{"medit", do_medit, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},

	{"north", do_north, POS_STANDING, 0, TRUE, LOG_NEVER, 0, TRUE},
	{"nofollow", do_nofollow, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"noloot", do_noloot, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"nosummo", do_nosummo, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"nosummon", do_nosummon, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"nocancel", do_nocancel, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"norecall", do_norecall, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"noauction", do_noauction, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"note", do_note, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"newlock", do_newlock, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
        {"newbie", do_newbie_channel, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"nochannels", do_nochannels, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
	{"noemote", do_noemote, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
	{"notify", do_notify, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"notitle", do_notitle, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
	{"noshout", do_noshout, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
	{"notell", do_notell, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
	{"noquit", do_no_quit, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
	{"nosac", do_nosac, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"nonote", do_nonote, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
	{"ninjitsu", do_ninjitsu, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},

	{"order", do_order, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"ooc", do_ooc, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"]", do_ooc, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"oldscore", do_old_score, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"outfit", do_outfit, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"open", do_open, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
//	{"objdump", do_objdump, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"oset", do_oset, POS_DEAD, L2, TRUE, LOG_ALWAYS, 1, FALSE},
	{"oload", do_oload, POS_DEAD, L6, TRUE, LOG_ALWAYS, 1, FALSE},
	{"owhere", do_owhere, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"ostat", do_ostat, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"ofind", do_ofind, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
//	{"oedit", do_oedit, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},

	{"peer", do_peer, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"pain", do_pain_touch, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"paint", do_paint_tattoo, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"practice", do_practice, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"prayer", do_pray, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"play", do_play, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"playerassist", do_playerassist, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
        {"pledge", do_pledge, POS_RESTING, 0, TRUE, LOG_NEVER, 1, FALSE},

	{"password", do_password, POS_DEAD, 0, FALSE, LOG_NEVER, 1, FALSE},
	{"prompt", do_prompt, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"pmote", do_pmote, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"pose", do_pose, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"pick", do_pick, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"pour", do_pour, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"put", do_put, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"push", do_push, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"peek", do_peek, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"pentagram", do_pentagram, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"permban", do_permban, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"protect", do_protect, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"pecho", do_pecho, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"pardon", do_pardon, POS_DEAD, L3, TRUE, LOG_ALWAYS, 1, FALSE},
	{"purge", do_purge, POS_DEAD, L6, TRUE, LOG_ALWAYS, 1, FALSE},
	{"poofin", do_bamfin, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"poofout", do_bamfout, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"peace", do_peace, POS_DEAD, L5, TRUE, LOG_NORMAL, 1, FALSE},
	{"prefi", do_prefi, POS_DEAD, L6, TRUE, LOG_NORMAL, 0, FALSE},
	{"prefix", do_prefix, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"pload", do_pload, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"punload", do_punload, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"passwordreset", do_resetpassword, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
//	{"pedit", do_pedit, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},

	{"quest", do_quest, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"question", do_question, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"?", do_question, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"qui", do_qui, POS_DEAD, 0, TRUE, LOG_NORMAL, 0, FALSE},
	{"quiet", do_quiet, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"quaff", do_quaff, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"quit", do_quit, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"quicken", do_quicken, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"rest", do_rest, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"read", do_read, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"rank", do_clan_rank, POS_DEAD, 0, FALSE, LOG_NORMAL, 1, FALSE},
	{"report", do_report, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"rules", do_rules, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"reply", do_reply, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"replay", do_replay, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"recite", do_recite, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"remove", do_remove, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"retell", do_retell, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"-", do_retell, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"retribution", do_retribution, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"retreat", do_retreat, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"rescue", do_rescue, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 0, TRUE},
	{"recall", do_recall, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"/", do_recall, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 0, TRUE},
	{"rent", do_rent, POS_DEAD, 0, TRUE, LOG_NORMAL, 0, FALSE},
	{"roar", do_roar, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"reboo", do_reboo, POS_DEAD, L1, TRUE, LOG_NORMAL, 0, FALSE},
	{"reboot", do_reboot, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"rename", do_rename, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"rset", do_rset, POS_DEAD, L2, TRUE, LOG_ALWAYS, 1, FALSE},
	{"restore", do_restore, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
	{"return", do_return, POS_DEAD, L4, TRUE, LOG_NORMAL, 1, FALSE},
	{"retir", do_retir, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"retire", do_retire, POS_DEAD, 0, TRUE, LOG_ALWAYS, 1, FALSE},
	{"rstat", do_rstat, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"reclass", do_reclass, POS_STANDING, 51, TRUE, LOG_ALWAYS, 1, FALSE},
	{"remort", do_remort, POS_STANDING, 51, TRUE, LOG_ALWAYS, 1, FALSE},
//	{"resets", do_resets, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
//	{"redit", do_redit, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"rptitle", do_rptitle, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"rub", do_rub_dirt, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"rip", do_rip, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"razor", do_razor_claws, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"rally", do_rally, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},

	{"south", do_south, POS_STANDING, 0, TRUE, LOG_NEVER, 0, TRUE},
        {"shoot", do_ranged_attack, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"sit", do_sit, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"sockets", do_sockets, POS_DEAD, L4, TRUE, LOG_NORMAL, 1, FALSE},
	{"stand", do_stand, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"stance", do_stance, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"score", do_new_score, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"skills", do_skills, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"socials", do_socials, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
        {"sheath", do_sheath, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"show", do_show, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"spells", do_spells, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"story", do_story, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"scroll", do_scroll, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"sound", do_sound, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"say", do_say, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"'", do_say, POS_RESTING, 0, TRUE, LOG_NORMAL, 0, TRUE},
	{"shout", do_shout, POS_RESTING, 3, TRUE, LOG_NORMAL, 1, TRUE},
	{"speak", do_speak, POS_RESTING, 3, TRUE, LOG_NORMAL, 1, TRUE},
	{"shed", do_shedding, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"sell", do_sell, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"secret", do_secret, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"sacrifice", do_sacrifice, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"shapeshift", do_shapeshift, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"substitute", do_substitution, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"sap", do_sap, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE },
	{"save", do_save, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"sleep", do_sleep, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"slide", do_slide, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"sneak", do_sneak, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"split", do_split, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"steal", do_steal, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"shadow", do_shadow, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"shadowwalk", do_shadow_walk, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"stone", do_living_stone, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"set", do_set, POS_DEAD, L2, TRUE, LOG_ALWAYS, 1, FALSE},
	{"sset", do_sset, POS_DEAD, L2, TRUE, LOG_ALWAYS, 1, FALSE},
	{"setpass", do_setpass, POS_DEAD, ML, TRUE, LOG_ALWAYS, 1, FALSE},
	{"shutdow", do_shutdow, POS_DEAD, L1, TRUE, LOG_NORMAL, 0, FALSE},
	{"shutdown", do_shutdown, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"sla", do_sla, POS_DEAD, L3, TRUE, LOG_NORMAL, 0, FALSE},
	{"slay", do_slay, POS_DEAD, L3, TRUE, LOG_ALWAYS, 1, FALSE},
	{"site", do_site, POS_DEAD, L1, TRUE, LOG_NORMAL, 1, FALSE},
	{"snoop", do_snoop, POS_DEAD, L3, TRUE, LOG_ALWAYS, 1, FALSE},
	{"spy", do_spy, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"stare", do_granite_stare, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"string", do_string, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
        {"stat", do_stat, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
	{"swap", do_swap, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"switch", do_switch, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"slookup", do_slookup, POS_DEAD, L2, TRUE, LOG_NORMAL, 1, FALSE},
	{"smote", do_smote, POS_DEAD, L5, TRUE, LOG_NORMAL, 1, FALSE},
	{"swapleade", do_swapleade, POS_STANDING, 53, TRUE, LOG_ALWAYS, 1, FALSE},
	{"swapleader", do_swapleader, POS_STANDING, 53, TRUE, LOG_ALWAYS, 1, FALSE},
	{"specialize", do_specialize, POS_STANDING, 1, TRUE, LOG_NORMAL, 1, FALSE},
	{"spousetalk", do_spousetalk, POS_SLEEPING, 1, TRUE, LOG_NEVER, 1, FALSE},
	{"scribe", do_scribe, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"[", do_spousetalk, POS_SLEEPING, 1, TRUE, LOG_NEVER, 1, FALSE},

	{"tell", do_tell, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"team", do_teamtalk, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"time", do_time, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"title", do_title, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"take", do_get, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"trash", do_trash, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"offer", do_offer, POS_RESTING, 0, TRUE, LOG_NORMAL, 0, FALSE},
	{"taste", do_taste, POS_RESTING, 0, TRUE, LOG_NORMAL, 0, TRUE},
	{"tap", do_sacrifice, POS_RESTING, 0, TRUE, LOG_NORMAL, 0, TRUE},
	{"trip", do_trip, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"throw", do_throw, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
        {"third", do_third_eye, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"train", do_train, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"trust", do_trust, POS_DEAD, ML, TRUE, LOG_ALWAYS, 1, FALSE},
//	{"teleport", do_transfer, POS_DEAD, L4, TRUE, LOG_ALWAYS, 1, FALSE},
	{"transfer", do_transfer, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},
	{"transferance", do_transferance, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"trance", do_trance, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},

	{"up", do_up, POS_STANDING, 0, TRUE, LOG_NEVER, 0, TRUE},
	{"unlock", do_unlock, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"unalias", do_unalias, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"unhush", do_unhush, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"value", do_value, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"visible", do_visible, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"violate", do_violate, POS_DEAD, ML, TRUE, LOG_ALWAYS, 1, FALSE},
	{"vapourize", do_vapourize, POS_DEAD, L6, TRUE, LOG_ALWAYS, 1, FALSE},
	{"voodoo", do_voodoo, POS_STANDING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"vote", do_vote, POS_DEAD, 0, FALSE, LOG_NORMAL, 1, FALSE},
	{"vnum", do_vnum, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},

	{"west", do_west, POS_STANDING, 0, TRUE, LOG_NEVER, 0, TRUE},
	{"wake", do_wake, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"wail", do_wail, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"wear", do_wear, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"wizhelp", do_wizhelp, POS_DEAD, IM, TRUE, LOG_NORMAL, 1, FALSE},
	{"wield", do_wear, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"weather", do_weather, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"who", do_who, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"whois", do_whois, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"whorem", do_whorem, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"wizlist", do_wizlist, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"worth", do_worth, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"wimpy", do_wimpy, POS_DEAD, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"warp", do_warp, POS_STANDING, 0, TRUE, LOG_NEVER, 0, TRUE},
	{"where", do_where, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"worship", do_worship, POS_SLEEPING, 0, TRUE, LOG_NORMAL, 1, FALSE},
	{"workshop", do_workshop, POS_SLEEPING, 59, FALSE, LOG_NORMAL, 1, FALSE},
	{"wizlock", do_wizlock, POS_DEAD, L1, TRUE, LOG_ALWAYS, 1, FALSE},
	{"wizinvis", do_invis, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"wiznet", do_wiznet, POS_DEAD, L6, TRUE, LOG_NORMAL, 1, FALSE},
	{"whodesc", do_whodesc, POS_DEAD, L2, TRUE, LOG_NORMAL, 1, FALSE},

	{"yell", do_yell, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},

	{"zap", do_zap, POS_RESTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"zeal", do_zeal, POS_FIGHTING, 0, TRUE, LOG_NORMAL, 1, TRUE},
	{"zecho", do_zecho, POS_DEAD, L5, TRUE, LOG_ALWAYS, 1, FALSE},

	{"", NULL, POS_DEAD, 0, TRUE, LOG_NORMAL, 0, FALSE}
};

bool
can_order(char *arg)
{
	int cmd;

	for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	{
		if (arg[0] == cmd_table[cmd].name[0]
			&& !str_prefix(arg, cmd_table[cmd].name)
			&& cmd_table[cmd].can_order == TRUE)
		{
			return TRUE;
		}
	}

	for (cmd = 0; social_table[cmd].name[0] != '\0'; cmd++)
	{
		if (arg[0] == social_table[cmd].name[0]
			&& !str_prefix(arg, social_table[cmd].name))
		{
			return TRUE;
		}
	}


	return FALSE;
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void
interpret(CHAR_DATA * ch, char *argument)
{
  log_string("interpret: %s : %s", ch->name, argument);

	char command[MAX_INPUT_LENGTH];
	char logline[MAX_INPUT_LENGTH];
	char logdel[MAX_INPUT_LENGTH];
	int cmd;
	int trust;
	bool found;
	char tild_check[MAX_INPUT_LENGTH];
	int i = 0;

	/*
	 * Strip leading spaces.
	 */
	while (isspace(*argument))
		argument++;
	if (argument[0] == '\0')
		return;

    sprintf(last_command, "%s in room[%d]: %s.",
		ch->name,
        ch->in_room == NULL ? 0 : ch->in_room->vnum,
        argument);

	if(strlen(argument) > MAX_INPUT_LENGTH - 2)  {
                Cprintf(ch, "Line too long. Your command failed!\n\r");
                return;
        }

	strcpy(tild_check, argument);
	while (tild_check[i] != '\0')
	{
		if (tild_check[i] == '~')
		{
			argument = '\0';
			return;
		}
		i++;
	}
	/*
	 * No hiding.
	 */
/*    REMOVE_BIT( ch->affected_by, AFF_HIDE ); */

	/*
	 * Implement freeze command.
	 */
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_FREEZE))
	{
		Cprintf(ch, "You're totally frozen!\n\r");
		return;
	}
	if (IS_SET(ch->comm, COMM_LAG))
		WAIT_STATE(ch, PULSE_VIOLENCE);

	/*
	 * Grab the command word.
	 * Special parsing so ' can be a command,
	 *   also no spaces needed after punctuation.
	 */
	strcpy(logline, argument);
	if (!isalpha(argument[0]) && !isdigit(argument[0]))
	{
		command[0] = argument[0];
		command[1] = '\0';
		argument++;
		while (isspace(*argument))
			argument++;
	}
	else
	{
		argument = one_argument(argument, command);
	}

	/*
	 * Look for command in command table.
	 */
	found = FALSE;
	trust = get_trust(ch);
	for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	{
		if (command[0] == cmd_table[cmd].name[0]
			&& !str_prefix(command, cmd_table[cmd].name)
			&& cmd_table[cmd].level <= trust)
		{
			found = TRUE;
			break;
		}
	}
	log_string("Help! %i, %s", found, command);

	/*
	 * Log and snoop.
	 */
	if (cmd_table[cmd].log == LOG_NEVER)
		strcpy(logline, "");
	one_argument(logline, logdel);

	if (!str_cmp(logdel, "delete"))
		strcpy(logline, "delete");
	if (!str_cmp(logdel, "swapleader"))
		strcpy(logline, "swapleader");
	if (!str_cmp(logdel, "addrecruiter"))
		strcpy(logline, "addrecruiter");
	if (!str_cmp(logdel, "droprecruiter"))
		strcpy(logline, "droprecruiter");

	if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_LOG))
		|| fLogAll
		|| cmd_table[cmd].log == LOG_ALWAYS)
	{
		sprintf(log_buf, "Log %s: %s", ch->name, logline);
		wiznet(log_buf, ch, NULL, WIZ_SECURE, 0, get_trust(ch));
		log_string("%s", log_buf);
	}

	if (ch->desc != NULL && ch->desc->snoop_by != NULL)
	{
		write_to_buffer(ch->desc->snoop_by, "% ");
		write_to_buffer(ch->desc->snoop_by, logline);
		write_to_buffer(ch->desc->snoop_by, "\n\r");
	}

	if (!found)
	{
		/*
		 * Look for command in socials table.
		 */
		if (!check_social(ch, command, argument))
			Cprintf(ch, "Huh?\n\r");
		return;
	}

	/*
	 * Character not in position for command?
	 */
	if (ch->position < cmd_table[cmd].position)
	{
		switch (ch->position)
		{
		case POS_DEAD:
			Cprintf(ch, "Lie still; you are DEAD.\n\r");
			break;

		case POS_MORTAL:
		case POS_INCAP:
			Cprintf(ch, "You are hurt far too bad for that.\n\r");
			break;

		case POS_STUNNED:
			Cprintf(ch, "You are too stunned to do that.\n\r");
			break;

		case POS_SLEEPING:
			Cprintf(ch, "In your dreams, or what?\n\r");
			break;

		case POS_RESTING:
			Cprintf(ch, "Nah... You feel too relaxed...\n\r");
			break;

		case POS_SITTING:
			Cprintf(ch, "Better stand up first.\n\r");
			break;

		case POS_FIGHTING:
			Cprintf(ch, "No way!  You are still fighting!\n\r");
			break;

		}
		return;
	}

	/*
	 * Dispatch the command.
	 */
	(*cmd_table[cmd].do_fun) (ch, argument);

	tail_chain();
	return;
}



bool
check_social(CHAR_DATA * ch, char *command, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int cmd;
	bool found;

	found = FALSE;
	for (cmd = 0; social_table[cmd].name[0] != '\0'; cmd++)
	{
		if (command[0] == social_table[cmd].name[0]
			&& !str_prefix(command, social_table[cmd].name))
		{
			found = TRUE;
			break;
		}
	}

	if (!found)
		return FALSE;

	if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE))
	{
		Cprintf(ch, "You are anti-social!\n\r");
		return TRUE;
	}

	switch (ch->position)
	{
	case POS_DEAD:
		Cprintf(ch, "Lie still; you are DEAD.\n\r");
		return TRUE;

	case POS_INCAP:
	case POS_MORTAL:
		Cprintf(ch, "You are hurt far too bad for that.\n\r");
		return TRUE;

	case POS_STUNNED:
		Cprintf(ch, "You are too stunned to do that.\n\r");
		return TRUE;

	case POS_SLEEPING:
		/*
		 * I just know this is the path to a 12" 'if' statement.  :(
		 * But two players asked for it already!  -- Furey
		 */
		if (!str_cmp(social_table[cmd].name, "snore"))
			break;
		Cprintf(ch, "In your dreams, or what?\n\r");
		return TRUE;

	}

	one_argument(argument, arg);
	victim = NULL;
	if (arg[0] == '\0')
	{
		act(social_table[cmd].others_no_arg, ch, NULL, victim, TO_ROOM, POS_RESTING);
		act(social_table[cmd].char_no_arg, ch, NULL, victim, TO_CHAR, POS_RESTING);
	}
	else if ((victim = get_char_room(ch, arg)) == NULL)
	{
		Cprintf(ch, "They aren't here.\n\r");
	}
	else if (victim == ch)
	{
		act(social_table[cmd].others_auto, ch, NULL, victim, TO_ROOM, POS_RESTING);
		act(social_table[cmd].char_auto, ch, NULL, victim, TO_CHAR, POS_RESTING);
	}
	else
	{
		act(social_table[cmd].others_found, ch, NULL, victim, TO_NOTVICT, POS_RESTING);
		act(social_table[cmd].char_found, ch, NULL, victim, TO_CHAR, POS_RESTING);
		act(social_table[cmd].vict_found, ch, NULL, victim, TO_VICT, POS_RESTING);

		if (!IS_NPC(ch) && IS_NPC(victim)
			&& !IS_AFFECTED(victim, AFF_CHARM)
			&& IS_AWAKE(victim)
			&& victim->desc == NULL)
		{
			switch (number_bits(4))
			{
			case 0:

			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				act(social_table[cmd].others_found, victim, NULL, ch, TO_NOTVICT, POS_RESTING);
				act(social_table[cmd].char_found, victim, NULL, ch, TO_CHAR, POS_RESTING);
				act(social_table[cmd].vict_found, victim, NULL, ch, TO_VICT, POS_RESTING);
				break;

			case 9:
			case 10:
			case 11:
			case 12:
				act("$n slaps $N.", victim, NULL, ch, TO_NOTVICT, POS_RESTING);
				act("You slap $N.", victim, NULL, ch, TO_CHAR, POS_RESTING);
				act("$n slaps you.", victim, NULL, ch, TO_VICT, POS_RESTING);
				break;
			}
		}
	}

	return TRUE;
}



/*
 * Return true if an argument is completely numeric.
 */
bool
is_number(char *arg)
{

	if (*arg == '\0')
		return FALSE;

	if (*arg == '+' || *arg == '-')
		arg++;

	for (; *arg != '\0'; arg++)
	{
		if (!isdigit(*arg))
			return FALSE;
	}

	return TRUE;
}



/*
 * Given a string like 14.foo, return 14 and 'foo'
 */
int
number_argument(char *argument, char *arg)
{
	char *pdot;
	int number;

	for (pdot = argument; *pdot != '\0'; pdot++)
	{
		if (*pdot == '.')
		{
			*pdot = '\0';
			number = atoi(argument);
			*pdot = '.';
			strcpy(arg, pdot + 1);
			return number;
		}
	}

	strcpy(arg, argument);
	return 1;
}

/*
 * Given a string like 14*foo, return 14 and 'foo'
 */
int
mult_argument(char *argument, char *arg)
{
	char *pdot;
	int number;

	for (pdot = argument; *pdot != '\0'; pdot++)
	{
		if (*pdot == '*')
		{
			*pdot = '\0';
			number = atoi(argument);
			*pdot = '*';
			strcpy(arg, pdot + 1);
			return number;
		}
	}

	strcpy(arg, argument);
	return 1;
}



/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.
 */
char *
one_argument(char *argument, char *arg_first)
{
	char cEnd;

	if (strlen(argument) == 0)
	{
		strcpy(arg_first, "");
		return argument;
	}

	while (isspace(*argument))
		argument++;

	cEnd = ' ';
	if (*argument == '\'' || *argument == '"')
		cEnd = *argument++;

	while (*argument != '\0')
	{
		if (*argument == cEnd)
		{
			argument++;
			break;
		}
		*arg_first = LOWER(*argument);
		arg_first++;
		argument++;
	}
	*arg_first = '\0';

	while (isspace(*argument))
		argument++;

	return argument;
}

/*
 * Contributed by Alander.
 */
void
do_commands(CHAR_DATA * ch, char *argument)
{
	int cmd;
	int col;

	col = 0;
	for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	{
		if (cmd_table[cmd].level < LEVEL_HERO
			&& cmd_table[cmd].level <= get_trust(ch)
			&& cmd_table[cmd].show)
		{
			Cprintf(ch, "%-12s", cmd_table[cmd].name);
			if (++col % 6 == 0)
				Cprintf(ch, "\n\r");
		}
	}

	if (col % 6 != 0)
		Cprintf(ch, "\n\r");
	return;
}

void
do_wizhelp(CHAR_DATA * ch, char *argument)
{
	BUFFER *output[7];
	char buf[7][MAX_STRING_LENGTH];
	int cmd;
	int level;
	int lines[7] =
	{0, 0, 0, 0, 0, 0, 0};
	int col[7] =
	{0, 0, 0, 0, 0, 0, 0};

	for (cmd = 0; cmd < 7; cmd++)
	{
		output[cmd] = new_buf();
		strcpy(buf[cmd], "");
	}

	strcpy(buf[0], "{y[{cBuilder{y]{x\n\r");
	add_buf(output[0], buf[0]);
	strcpy(buf[0], "{y[{cAngel{y]{x\n\r");
	add_buf(output[1], buf[0]);
	strcpy(buf[0], "{y[{cDemigod{y]{x\n\r");
	add_buf(output[2], buf[0]);
	strcpy(buf[0], "{y[{cGod{y]{x\n\r");
	add_buf(output[3], buf[0]);
	strcpy(buf[0], "{y[{cDeity{y]{x\n\r");
	add_buf(output[4], buf[0]);
	strcpy(buf[0], "{y[{cSupremacy{y]{x\n\r");
	add_buf(output[5], buf[0]);
	strcpy(buf[0], "{y[{cImplementor{y]{x\n\r");
	add_buf(output[6], buf[0]);

	strcpy(buf[0], "");

	/* write up the buffers */
	for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
	{
		level = cmd_table[cmd].level;

		if (level < 54)
			continue;

		sprintf(buf[level - 54], "%s%-12s", buf[level - 54], cmd_table[cmd].name);
		if (++col[level - 54] % 6 == 0)
		{
			strcat(buf[level - 54], "\n\r");
			add_buf(output[level - 54], buf[level - 54]);
			strcpy(buf[level - 54], "");
			lines[level - 54]++;
		}
	}

	/* flush and lines that didn't fill up. */
	for (cmd = 0; cmd < 7; cmd++)
	{
		if (strlen(buf[cmd]) != 0)
		{
			strcat(buf[cmd], "\n\r");
			add_buf(output[cmd], buf[cmd]);
			lines[cmd]++;
		}
	}

	/* print everything out */
	for (cmd = 54; cmd <= get_trust(ch); cmd++)
	{
		if (lines[cmd - 54])
		{
			page_to_char(buf_string(output[cmd - 54]), ch);
			if (cmd != get_trust(ch))
				Cprintf(ch, "\n\r");
		}
	}

	/* free up space */
	for (cmd = 0; cmd < 7; cmd++)
		free_buf(output[cmd]);

	return;
}
