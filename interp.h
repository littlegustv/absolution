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
 *  ROM 2.4 is copyright 1993-1996 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@pacinfo.com)                                  *
 *      Gabrielle Taylor (gtaylor@pacinfo.com)                             *
 *      Brian Moore (rom@rom.efn.org)                                      *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

#ifndef INTERP_H
#define INTERP_H

/* this is a listing of all the commands and command related data */

/* for command types */
#define ML 	MAX_LEVEL			/* implementor */
#define L1	MAX_LEVEL - 1		/* creator */
#define L2	MAX_LEVEL - 2		/* supreme being */
#define L3	MAX_LEVEL - 3		/* deity */
#define L4 	MAX_LEVEL - 4		/* god */
#define L5	MAX_LEVEL - 5		/* immortal */
#define L6	MAX_LEVEL - 6		/* demigod */
#define L7	MAX_LEVEL - 7		/* angel */
#define L8	MAX_LEVEL - 8		/* avatar */
#define IM	LEVEL_IMMORTAL		/* avatar */
#define HE	LEVEL_HERO			/* hero */

#define COM_INGORE	1


/*
 * Structure for a command in the command lookup table.
 */
struct cmd_type
{
	char *const name;
	DO_FUN *do_fun;
	int position;
	int level;
	int demandable;
	int log;
	int show;
	bool can_order;
};

/* the command table itself */
extern const struct cmd_type cmd_table[];

bool can_order(char *arg);

/*
 * Command functions.
 * Defined in act_*.c (mostly).
 */
DECLARE_DO_FUN(do_macro);
DECLARE_DO_FUN(do_atm);
DECLARE_DO_FUN(do_ambush);
DECLARE_DO_FUN(do_advance);
DECLARE_DO_FUN(do_affects);
DECLARE_DO_FUN(do_afk);
DECLARE_DO_FUN(do_aiming);
DECLARE_DO_FUN(do_alia);
DECLARE_DO_FUN(do_alias);
DECLARE_DO_FUN(do_allow);
DECLARE_DO_FUN(do_animate_tattoo);
DECLARE_DO_FUN(do_answer);
DECLARE_DO_FUN(do_areas);
DECLARE_DO_FUN(do_at);
DECLARE_DO_FUN(do_auction);
DECLARE_DO_FUN(do_noauction);
DECLARE_DO_FUN(do_autovalue);
DECLARE_DO_FUN(do_autoassist);
DECLARE_DO_FUN(do_ninjitsu);
DECLARE_DO_FUN(do_mobassist);
DECLARE_DO_FUN(do_playerassist);
DECLARE_DO_FUN(do_autoexit);
DECLARE_DO_FUN(do_autogold);
DECLARE_DO_FUN(do_autolist);
DECLARE_DO_FUN(do_autoloot);
DECLARE_DO_FUN(do_autosac);
DECLARE_DO_FUN(do_autotitle);
DECLARE_DO_FUN(do_autotrash);
DECLARE_DO_FUN(do_autosplit);
DECLARE_DO_FUN(do_backstab);
DECLARE_DO_FUN(do_bamfin);
DECLARE_DO_FUN(do_bamfout);
DECLARE_DO_FUN(do_ban);
DECLARE_DO_FUN(do_bash);
DECLARE_DO_FUN(do_bash_door);
DECLARE_DO_FUN(do_barrage);
DECLARE_DO_FUN(do_berserk);
DECLARE_DO_FUN(do_blank);
DECLARE_DO_FUN(do_boost);
DECLARE_DO_FUN(do_block);
DECLARE_DO_FUN(do_bounty);
DECLARE_DO_FUN(do_brandish);
DECLARE_DO_FUN(do_brief);
DECLARE_DO_FUN(do_bribe);
DECLARE_DO_FUN(do_bug);
DECLARE_DO_FUN(do_butcher);
DECLARE_DO_FUN(do_buy);
DECLARE_DO_FUN(do_cast);
DECLARE_DO_FUN(do_craft);
DECLARE_DO_FUN(do_crash);
DECLARE_DO_FUN(do_carve);
DECLARE_DO_FUN(do_carve_boulder);
DECLARE_DO_FUN(do_chakra);
DECLARE_DO_FUN(do_changes);
DECLARE_DO_FUN(do_channels);
DECLARE_DO_FUN(do_warp);
DECLARE_DO_FUN(do_check);
DECLARE_DO_FUN(do_choke_hold);
DECLARE_DO_FUN(do_ctf);
DECLARE_DO_FUN(do_chi);
DECLARE_DO_FUN(do_clan_rank);
DECLARE_DO_FUN(do_claim);
DECLARE_DO_FUN(do_clone);
DECLARE_DO_FUN(do_close);
DECLARE_DO_FUN(do_colour);		/* Colour Command By Lope */
DECLARE_DO_FUN(do_commands);
DECLARE_DO_FUN(do_combine);
DECLARE_DO_FUN(do_compact);
DECLARE_DO_FUN(do_compare);
DECLARE_DO_FUN(do_offer);
DECLARE_DO_FUN(do_sheath);
DECLARE_DO_FUN(do_shapeshift);
DECLARE_DO_FUN(do_brew);
DECLARE_DO_FUN(do_transferance);

DECLARE_DO_FUN(do_retell);
DECLARE_DO_FUN(do_retreat);
DECLARE_DO_FUN(do_remort);
DECLARE_DO_FUN(do_reclass);
DECLARE_DO_FUN(do_rptitle);
DECLARE_DO_FUN(do_consider);
DECLARE_DO_FUN(do_count);
DECLARE_DO_FUN(do_conversion);
DECLARE_DO_FUN(do_credits);
DECLARE_DO_FUN(do_dark_feast);
DECLARE_DO_FUN(do_deaf);
DECLARE_DO_FUN(do_demon_fist);
DECLARE_DO_FUN(do_dragon_kick);
DECLARE_DO_FUN(do_eagle_claw);
DECLARE_DO_FUN(do_evasion);
DECLARE_DO_FUN(do_delet);
DECLARE_DO_FUN(do_delete);
DECLARE_DO_FUN(do_delegate);
DECLARE_DO_FUN(do_deny);
DECLARE_DO_FUN(do_description);
DECLARE_DO_FUN(do_dice);
DECLARE_DO_FUN(do_dirt);
DECLARE_DO_FUN(do_disarm);
DECLARE_DO_FUN(do_disconnect);
DECLARE_DO_FUN(do_dispvnum);
DECLARE_DO_FUN(do_down);
DECLARE_DO_FUN(do_drag);
DECLARE_DO_FUN(do_draw);
DECLARE_DO_FUN(do_drink);
DECLARE_DO_FUN(do_drop);
DECLARE_DO_FUN(do_dump);
DECLARE_DO_FUN(do_east);
DECLARE_DO_FUN(do_eat);
DECLARE_DO_FUN(do_echo);
DECLARE_DO_FUN(do_emote);
DECLARE_DO_FUN(do_enter);
DECLARE_DO_FUN(do_envenom);
DECLARE_DO_FUN(do_equipment);
DECLARE_DO_FUN(do_examine);
DECLARE_DO_FUN(do_exits);
DECLARE_DO_FUN(do_fill);
DECLARE_DO_FUN(do_fixcp);
DECLARE_DO_FUN(do_flag);
DECLARE_DO_FUN(do_flee);
DECLARE_DO_FUN(do_follow);
DECLARE_DO_FUN(do_force);
DECLARE_DO_FUN(do_freeze);
DECLARE_DO_FUN(do_fstat);
DECLARE_DO_FUN(do_gain);
DECLARE_DO_FUN(do_game);
DECLARE_DO_FUN(do_gemology);
DECLARE_DO_FUN(do_get);
DECLARE_DO_FUN(do_give);
DECLARE_DO_FUN(do_gift);
DECLARE_DO_FUN(do_gossip);
DECLARE_DO_FUN(do_newbie_channel);
DECLARE_DO_FUN(do_mind_link);
DECLARE_DO_FUN(do_sliver_chan);
DECLARE_DO_FUN(do_goto);
DECLARE_DO_FUN(do_grats);
DECLARE_DO_FUN(do_gravitation);
DECLARE_DO_FUN(do_group);
DECLARE_DO_FUN(do_groups);
DECLARE_DO_FUN(do_gtell);
DECLARE_DO_FUN(do_guild);
DECLARE_DO_FUN(do_hal);
DECLARE_DO_FUN(do_half);
DECLARE_DO_FUN(do_heal);
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_hide);
DECLARE_DO_FUN(do_holylight);
DECLARE_DO_FUN(do_hush);
DECLARE_DO_FUN(do_unhush);
DECLARE_DO_FUN(do_unseal);
DECLARE_DO_FUN(do_hunt);
DECLARE_DO_FUN(do_idea);
DECLARE_DO_FUN(do_immtalk);
DECLARE_DO_FUN(do_incognito);
DECLARE_DO_FUN(do_clantalk);
DECLARE_DO_FUN(do_clan_report);
DECLARE_DO_FUN(do_imotd);
DECLARE_DO_FUN(do_inventory);
DECLARE_DO_FUN(do_invis);
DECLARE_DO_FUN(do_kick);
DECLARE_DO_FUN(do_bite);
DECLARE_DO_FUN(do_throw);
DECLARE_DO_FUN(do_third_eye);
DECLARE_DO_FUN(do_taste);
DECLARE_DO_FUN(do_kill);
DECLARE_DO_FUN(do_killdude);
DECLARE_DO_FUN(do_list);
DECLARE_DO_FUN(do_linkdead);
DECLARE_DO_FUN(do_leadchan);
DECLARE_DO_FUN(do_load);
DECLARE_DO_FUN(do_lock);
DECLARE_DO_FUN(do_log);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_lookup);
DECLARE_DO_FUN(do_maxstats);
DECLARE_DO_FUN(do_member_list);
DECLARE_DO_FUN(do_memory);
DECLARE_DO_FUN(do_mfind);
DECLARE_DO_FUN(do_mload);
DECLARE_DO_FUN(do_mset);
DECLARE_DO_FUN(do_mstat);
DECLARE_DO_FUN(do_mwhere);
DECLARE_DO_FUN(do_motd);
DECLARE_DO_FUN(do_mob);
DECLARE_DO_FUN(do_murde);
DECLARE_DO_FUN(do_murder);
DECLARE_DO_FUN(do_mpstat);
DECLARE_DO_FUN(do_mpdump);
DECLARE_DO_FUN(do_music);
DECLARE_DO_FUN(do_newlock);
DECLARE_DO_FUN(do_news);
DECLARE_DO_FUN(do_nochannels);
DECLARE_DO_FUN(do_noemote);
DECLARE_DO_FUN(do_nofollow);
DECLARE_DO_FUN(do_noloot);
DECLARE_DO_FUN(do_nonote);
DECLARE_DO_FUN(do_north);
DECLARE_DO_FUN(do_noshout);
DECLARE_DO_FUN(do_nosummon);
DECLARE_DO_FUN(do_nosummo);
DECLARE_DO_FUN(do_note);
DECLARE_DO_FUN(do_notell);
DECLARE_DO_FUN(do_notify);
DECLARE_DO_FUN(do_notitle);
DECLARE_DO_FUN(do_nosac);
DECLARE_DO_FUN(do_objdump);
DECLARE_DO_FUN(do_ofind);
DECLARE_DO_FUN(do_oload);
DECLARE_DO_FUN(do_open);
DECLARE_DO_FUN(do_order);
DECLARE_DO_FUN(do_oset);
DECLARE_DO_FUN(do_ostat);
DECLARE_DO_FUN(do_outfit);
DECLARE_DO_FUN(do_owhere);
DECLARE_DO_FUN(do_gather);
DECLARE_DO_FUN(do_disperse);
DECLARE_DO_FUN(do_disperseroom);
DECLARE_DO_FUN(do_paint_tattoo);
DECLARE_DO_FUN(do_pain_touch);
DECLARE_DO_FUN(do_pardon);
DECLARE_DO_FUN(do_password);
DECLARE_DO_FUN(do_peace);
DECLARE_DO_FUN(do_peer);
DECLARE_DO_FUN(do_pecho);
DECLARE_DO_FUN(do_penalty);
DECLARE_DO_FUN(do_permban);
DECLARE_DO_FUN(do_pfile);
DECLARE_DO_FUN(do_pick);
DECLARE_DO_FUN(do_play);
DECLARE_DO_FUN(do_pledge);
DECLARE_DO_FUN(do_pmote);
DECLARE_DO_FUN(do_pose);
DECLARE_DO_FUN(do_pour);
DECLARE_DO_FUN(do_practice);
DECLARE_DO_FUN(do_pray);
DECLARE_DO_FUN(do_prefi);
DECLARE_DO_FUN(do_prefix);
DECLARE_DO_FUN(do_prompt);
DECLARE_DO_FUN(do_protect);
DECLARE_DO_FUN(do_purge);
DECLARE_DO_FUN(do_vapourize);
DECLARE_DO_FUN(do_put);
DECLARE_DO_FUN(do_quaff);
DECLARE_DO_FUN(do_quest);
DECLARE_DO_FUN(do_question);
DECLARE_DO_FUN(do_qui);
DECLARE_DO_FUN(do_quiet);
DECLARE_DO_FUN(do_quit);
DECLARE_DO_FUN(do_quicken);
DECLARE_DO_FUN(do_quote);
DECLARE_DO_FUN(do_read);
DECLARE_DO_FUN(do_reboo);
DECLARE_DO_FUN(do_reboot);
DECLARE_DO_FUN(do_recall);
DECLARE_DO_FUN(do_recho);
DECLARE_DO_FUN(do_recite);
DECLARE_DO_FUN(do_remove);
DECLARE_DO_FUN(do_rent);
DECLARE_DO_FUN(do_replay);
DECLARE_DO_FUN(do_reply);
DECLARE_DO_FUN(do_report);
DECLARE_DO_FUN(do_rescue);
DECLARE_DO_FUN(do_rest);
DECLARE_DO_FUN(do_restore);
DECLARE_DO_FUN(do_retir);
DECLARE_DO_FUN(do_retire);
DECLARE_DO_FUN(do_retribution);
DECLARE_DO_FUN(do_return);
DECLARE_DO_FUN(do_rset);
DECLARE_DO_FUN(do_rstat);
DECLARE_DO_FUN(do_rules);
DECLARE_DO_FUN(do_sacrifice);
DECLARE_DO_FUN(do_save);
DECLARE_DO_FUN(do_say);
DECLARE_DO_FUN(do_new_score);
DECLARE_DO_FUN(do_old_score);
DECLARE_DO_FUN(do_scroll);
DECLARE_DO_FUN(do_sell);
DECLARE_DO_FUN(do_set);
DECLARE_DO_FUN(do_setpass);
DECLARE_DO_FUN(do_shout);
DECLARE_DO_FUN(do_show);
DECLARE_DO_FUN(do_shedding);
DECLARE_DO_FUN(do_shutdow);
DECLARE_DO_FUN(do_shutdown);
DECLARE_DO_FUN(do_sit);
DECLARE_DO_FUN(do_site);
DECLARE_DO_FUN(do_skills);
DECLARE_DO_FUN(do_sla);
DECLARE_DO_FUN(do_slay);
DECLARE_DO_FUN(do_sleep);
DECLARE_DO_FUN(do_slookup);
DECLARE_DO_FUN(do_smote);
DECLARE_DO_FUN(do_sneak);
DECLARE_DO_FUN(do_snoop);
DECLARE_DO_FUN(do_socials);
DECLARE_DO_FUN(do_south);
DECLARE_DO_FUN(do_sound);
DECLARE_DO_FUN(do_sockets);
DECLARE_DO_FUN(do_speak);
DECLARE_DO_FUN(do_spells);
DECLARE_DO_FUN(do_split);
DECLARE_DO_FUN(do_sset);
DECLARE_DO_FUN(do_stand);
DECLARE_DO_FUN(do_stance);
DECLARE_DO_FUN(do_stat);
DECLARE_DO_FUN(do_steal);
DECLARE_DO_FUN(do_story);
DECLARE_DO_FUN(do_string);
DECLARE_DO_FUN(do_substitution);
DECLARE_DO_FUN(do_surrender);
DECLARE_DO_FUN(do_switch);
DECLARE_DO_FUN(do_teamtalk);
DECLARE_DO_FUN(do_tell);
DECLARE_DO_FUN(do_time);
DECLARE_DO_FUN(do_title);
DECLARE_DO_FUN(do_secret);
DECLARE_DO_FUN(do_train);
DECLARE_DO_FUN(do_transfer);
DECLARE_DO_FUN(do_transferance);
DECLARE_DO_FUN(do_trash);
DECLARE_DO_FUN(do_trip);
DECLARE_DO_FUN(do_trust);
DECLARE_DO_FUN(do_typo);
DECLARE_DO_FUN(do_unalias);
DECLARE_DO_FUN(do_unlock);
DECLARE_DO_FUN(do_unread);
DECLARE_DO_FUN(do_up);
DECLARE_DO_FUN(do_value);
DECLARE_DO_FUN(do_visible);
DECLARE_DO_FUN(do_violate);
DECLARE_DO_FUN(do_vote);
DECLARE_DO_FUN(do_vnum);
DECLARE_DO_FUN(do_wail);
DECLARE_DO_FUN(do_wake);
DECLARE_DO_FUN(do_wear);
DECLARE_DO_FUN(do_weather);
DECLARE_DO_FUN(do_west);
DECLARE_DO_FUN(do_where);
DECLARE_DO_FUN(do_who);
DECLARE_DO_FUN(do_whois);
DECLARE_DO_FUN(do_whorem);
DECLARE_DO_FUN(do_wimpy);
DECLARE_DO_FUN(do_wizhelp);
DECLARE_DO_FUN(do_wizlock);
DECLARE_DO_FUN(do_wizlist);
DECLARE_DO_FUN(do_wiznet);
DECLARE_DO_FUN(do_worth);
DECLARE_DO_FUN(do_worship);
DECLARE_DO_FUN(do_workshop);
DECLARE_DO_FUN(do_yell);
DECLARE_DO_FUN(do_focus);
DECLARE_DO_FUN(do_zap);
DECLARE_DO_FUN(do_zecho);
DECLARE_DO_FUN(do_olc);
DECLARE_DO_FUN(do_asave);
DECLARE_DO_FUN(do_alist);
DECLARE_DO_FUN(do_resets);
DECLARE_DO_FUN(do_redit);
DECLARE_DO_FUN(do_aedit);
DECLARE_DO_FUN(do_medit);
DECLARE_DO_FUN(do_oedit);
DECLARE_DO_FUN(do_pedit);
DECLARE_DO_FUN(do_hedit);
DECLARE_DO_FUN(do_bedit);
DECLARE_DO_FUN(do_lag);
DECLARE_DO_FUN(do_no_quit);
DECLARE_DO_FUN(do_fuck);
DECLARE_DO_FUN(do_beep);
DECLARE_DO_FUN(do_nocancel);
DECLARE_DO_FUN(do_norecall);
DECLARE_DO_FUN(do_peek);
DECLARE_DO_FUN(do_cgoss);
DECLARE_DO_FUN(do_lore);
DECLARE_DO_FUN(do_ooc);
DECLARE_DO_FUN(do_whodesc);
DECLARE_DO_FUN(do_loner);
DECLARE_DO_FUN(do_join);
DECLARE_DO_FUN(do_accept);
DECLARE_DO_FUN(do_lay_hands);
DECLARE_DO_FUN(do_cure_plague);
DECLARE_DO_FUN(do_protection);
DECLARE_DO_FUN(do_shadow);
DECLARE_DO_FUN(do_shadow_walk);
DECLARE_DO_FUN(do_edit);
DECLARE_DO_FUN(do_ident);
DECLARE_DO_FUN(do_roar);
DECLARE_DO_FUN(do_living_stone);

/*DECLARE_DO_FUN(do_check_dns);*/
DECLARE_DO_FUN(do_mac_dump);
DECLARE_DO_FUN(do_breath);
DECLARE_DO_FUN(do_pentagram);
DECLARE_DO_FUN(do_double);

DECLARE_DO_FUN(do_rename);
DECLARE_DO_FUN(do_appraise);
DECLARE_DO_FUN(do_lair);

DECLARE_DO_FUN(do_swapleade);
DECLARE_DO_FUN(do_droprecruite);
DECLARE_DO_FUN(do_addrecruite);
DECLARE_DO_FUN(do_swapleader);
DECLARE_DO_FUN(do_droprecruiter);
DECLARE_DO_FUN(do_addrecruiter);
DECLARE_DO_FUN(do_assume);
DECLARE_DO_FUN(do_droppatron);
DECLARE_DO_FUN(do_dropvassal);

DECLARE_DO_FUN(do_mystats);
DECLARE_DO_FUN(do_marry);
DECLARE_DO_FUN(do_divorce);
DECLARE_DO_FUN(do_spousetalk);

DECLARE_DO_FUN(do_resetpassword);
DECLARE_DO_FUN(do_pload);
DECLARE_DO_FUN(do_punload);
DECLARE_DO_FUN(do_mobdump);

DECLARE_DO_FUN(do_sap);
DECLARE_DO_FUN(do_enrage);
DECLARE_DO_FUN(do_ensnare);
DECLARE_DO_FUN(do_rub_dirt);
DECLARE_DO_FUN(do_chase);
DECLARE_DO_FUN(do_jump);
DECLARE_DO_FUN(do_combine_potion);
DECLARE_DO_FUN(do_scribe);
DECLARE_DO_FUN(do_charge);
DECLARE_DO_FUN(do_push);
DECLARE_DO_FUN(do_rip);
DECLARE_DO_FUN(do_spy);
DECLARE_DO_FUN(do_gladiator);
DECLARE_DO_FUN(do_guardian);
DECLARE_DO_FUN(do_specialize);
DECLARE_DO_FUN(do_carve_spear);
DECLARE_DO_FUN(do_razor_claws);
DECLARE_DO_FUN(do_rally);
DECLARE_DO_FUN(do_hunter_ball);
DECLARE_DO_FUN(do_cave_in);
DECLARE_DO_FUN(do_call_to_hunt);
DECLARE_DO_FUN(do_voodoo);
DECLARE_DO_FUN(do_slide);
DECLARE_DO_FUN(do_granite_stare);
DECLARE_DO_FUN(do_swap);
DECLARE_DO_FUN(do_cheat);
DECLARE_DO_FUN(do_hurl);
DECLARE_DO_FUN(do_zeal);
DECLARE_DO_FUN(do_fury);
DECLARE_DO_FUN(do_trance);
DECLARE_DO_FUN(do_ready_death_blow);
DECLARE_DO_FUN(do_perform_death_blow);
DECLARE_DO_FUN(do_ranged_attack);
DECLARE_DO_FUN(do_eqoutput);

/* End functions

 * These are run when a spell/skill wears off.
 */
DECLARE_END_FUN(end_null);
DECLARE_END_FUN(end_delayed_fireball);
DECLARE_END_FUN(end_gladiator);
DECLARE_END_FUN(end_chi_ei);

#endif
