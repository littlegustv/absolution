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

#ifndef MAGIC_H
#define MAGIC_H

/*
 * Spell functions.
 * Defined in magic.c.
 */
DECLARE_SPELL_FUN(spell_null);
DECLARE_END_FUN(end_null);
DECLARE_END_FUN(end_living_stone);
DECLARE_END_FUN(end_chi_ei);

DECLARE_SPELL_FUN(spell_acid_blast);
DECLARE_SPELL_FUN(spell_aura);
DECLARE_SPELL_FUN(spell_aurora);
DECLARE_SPELL_FUN(spell_armor);
DECLARE_SPELL_FUN(spell_cryo);
DECLARE_SPELL_FUN(spell_wrath);
DECLARE_SPELL_FUN(spell_familiar);
DECLARE_SPELL_FUN(spell_fear);

DECLARE_SPELL_FUN(spell_flame_arrow);
DECLARE_SPELL_FUN(spell_flood);
DECLARE_END_FUN(end_flood);
DECLARE_SPELL_FUN(spell_call_creature);
DECLARE_SPELL_FUN(spell_prismatic_spray);
DECLARE_SPELL_FUN(spell_phantom_force);
DECLARE_SPELL_FUN(spell_magic_stone);
DECLARE_SPELL_FUN(spell_denounciation);
DECLARE_SPELL_FUN(spell_quill_armor);
DECLARE_SPELL_FUN(spell_rainbow_burst);
DECLARE_SPELL_FUN(spell_rain);
DECLARE_SPELL_FUN(spell_hallow);
DECLARE_SPELL_FUN(spell_hail);
DECLARE_SPELL_FUN(spell_creeping_doom);
DECLARE_SPELL_FUN(spell_animal_growth);
DECLARE_SPELL_FUN(spell_alarm_rune);
DECLARE_END_FUN(end_alarm_rune);
DECLARE_SPELL_FUN(spell_bark_skin);
DECLARE_SPELL_FUN(spell_balance_rune);
DECLARE_SPELL_FUN(spell_burst_rune);
DECLARE_SPELL_FUN(spell_blade_rune);
DECLARE_SPELL_FUN(spell_build_fire);
DECLARE_SPELL_FUN(spell_oracle);
DECLARE_SPELL_FUN(spell_web);
DECLARE_SPELL_FUN(spell_rukus_magna);
DECLARE_SPELL_FUN(spell_blur);
DECLARE_SPELL_FUN(spell_hell_blade);
DECLARE_SPELL_FUN(spell_lightning_spear);
DECLARE_SPELL_FUN(spell_cause_discordance);
DECLARE_SPELL_FUN(spell_tame_animal);
DECLARE_SPELL_FUN(spell_materialize);

DECLARE_SPELL_FUN(spell_bless);
DECLARE_SPELL_FUN(spell_blindness);
DECLARE_SPELL_FUN(spell_burning_hands);
DECLARE_SPELL_FUN(spell_call_lightning);
DECLARE_SPELL_FUN(spell_call_to_arms);
DECLARE_SPELL_FUN(spell_calm);
DECLARE_SPELL_FUN(spell_cancellation);
DECLARE_SPELL_FUN(spell_cause_critical);
DECLARE_SPELL_FUN(spell_cause_light);
DECLARE_SPELL_FUN(spell_cause_serious);
DECLARE_SPELL_FUN(spell_change_sex);
DECLARE_SPELL_FUN(spell_chain_lightning);
DECLARE_SPELL_FUN(spell_charm_person);
DECLARE_SPELL_FUN(spell_chill_touch);
DECLARE_SPELL_FUN(spell_colour_spray);
DECLARE_SPELL_FUN(spell_concealment);
DECLARE_SPELL_FUN(spell_continual_light);
DECLARE_SPELL_FUN(spell_control_weather);
DECLARE_SPELL_FUN(spell_corruption);
DECLARE_SPELL_FUN(spell_create_food);
DECLARE_SPELL_FUN(spell_create_rose);
DECLARE_SPELL_FUN(spell_create_spring);
DECLARE_SPELL_FUN(spell_create_water);
DECLARE_SPELL_FUN(spell_cure_blindness);
DECLARE_SPELL_FUN(spell_cure_critical);
DECLARE_SPELL_FUN(spell_cure_disease);
DECLARE_SPELL_FUN(spell_cure_light);
DECLARE_SPELL_FUN(spell_cure_poison);
DECLARE_SPELL_FUN(spell_cure_serious);
DECLARE_SPELL_FUN(spell_curse);
DECLARE_SPELL_FUN(spell_death_rune);
DECLARE_SPELL_FUN(spell_demonfire);
DECLARE_SPELL_FUN(spell_detect_hidden);
DECLARE_SPELL_FUN(spell_detect_invis);
DECLARE_SPELL_FUN(spell_detect_magic);
DECLARE_SPELL_FUN(spell_detect_poison);
DECLARE_SPELL_FUN(spell_dispel_evil);
DECLARE_SPELL_FUN(spell_dispel_good);
DECLARE_SPELL_FUN(spell_dispel_magic);
DECLARE_SPELL_FUN(spell_dissolution);
DECLARE_SPELL_FUN(spell_destroy_tattoo);
DECLARE_SPELL_FUN(spell_destroy_rune);
DECLARE_SPELL_FUN(spell_drakor);
DECLARE_SPELL_FUN(spell_earthquake);
DECLARE_SPELL_FUN(spell_enchant_armor);
DECLARE_SPELL_FUN(spell_enchant_weapon);
DECLARE_SPELL_FUN(spell_energy_drain);
DECLARE_SPELL_FUN(spell_mana_drain);
DECLARE_SPELL_FUN(spell_faerie_fire);
DECLARE_SPELL_FUN(spell_faerie_fog);
DECLARE_SPELL_FUN(spell_farsight);
DECLARE_SPELL_FUN(spell_fire_rune);
DECLARE_END_FUN(end_fire_rune);
DECLARE_SPELL_FUN(spell_fireball);
DECLARE_SPELL_FUN(spell_fireproof);
DECLARE_SPELL_FUN(spell_flamestrike);
DECLARE_SPELL_FUN(spell_floating_disc);
DECLARE_SPELL_FUN(spell_fly);
DECLARE_SPELL_FUN(spell_frenzy);
DECLARE_SPELL_FUN(spell_gate);
DECLARE_SPELL_FUN(spell_giant_strength);
DECLARE_SPELL_FUN(spell_harm);
DECLARE_SPELL_FUN(spell_haste);
DECLARE_SPELL_FUN(spell_heal);
DECLARE_SPELL_FUN(spell_heat_metal);
DECLARE_SPELL_FUN(spell_holy_word);
DECLARE_SPELL_FUN(spell_identify);
DECLARE_SPELL_FUN(spell_ignore_wounds);
DECLARE_SPELL_FUN(spell_infravision);
DECLARE_SPELL_FUN(spell_invis);
DECLARE_SPELL_FUN(spell_jail);
DECLARE_END_FUN(end_jail);
DECLARE_SPELL_FUN(spell_know_alignment);
DECLARE_SPELL_FUN(spell_lightning_bolt);
DECLARE_SPELL_FUN(spell_locate_object);
DECLARE_SPELL_FUN(spell_lesser_dispel);
DECLARE_SPELL_FUN(spell_lure);
DECLARE_END_FUN(end_lure);
DECLARE_SPELL_FUN(spell_magic_missile);
DECLARE_SPELL_FUN(spell_mass_healing);
DECLARE_SPELL_FUN(spell_mass_invis);
DECLARE_SPELL_FUN(spell_mass_sanc);
DECLARE_SPELL_FUN(spell_melior);
DECLARE_SPELL_FUN(spell_mutate);
DECLARE_SPELL_FUN(spell_nexus);
DECLARE_SPELL_FUN(spell_pass_door);
DECLARE_SPELL_FUN(spell_plague);
DECLARE_SPELL_FUN(spell_poison);
DECLARE_SPELL_FUN(spell_portal);
DECLARE_SPELL_FUN(spell_protection_evil);
DECLARE_SPELL_FUN(spell_protection_good);
DECLARE_SPELL_FUN(spell_protection_neutral);
DECLARE_SPELL_FUN(spell_purify);
DECLARE_SPELL_FUN(spell_ray_of_truth);
DECLARE_SPELL_FUN(spell_recharge);
DECLARE_SPELL_FUN(spell_refresh);
DECLARE_SPELL_FUN(spell_remove_curse);
DECLARE_SPELL_FUN(spell_robustness);
DECLARE_SPELL_FUN(spell_sanctuary);
DECLARE_SPELL_FUN(spell_seppuku);
DECLARE_END_FUN(end_seppuku);
DECLARE_SPELL_FUN(spell_scalemail);
DECLARE_SPELL_FUN(spell_shackle_rune);
DECLARE_END_FUN(end_shackle_rune);
DECLARE_SPELL_FUN(spell_shadow_magic);
DECLARE_SPELL_FUN(spell_shawl);
DECLARE_SPELL_FUN(spell_shocking_grasp);
DECLARE_SPELL_FUN(spell_shield);
DECLARE_SPELL_FUN(spell_sleep);
DECLARE_END_FUN(end_sleep);
DECLARE_SPELL_FUN(spell_slow);
DECLARE_SPELL_FUN(spell_soul_blade);
DECLARE_SPELL_FUN(spell_soul_rune);
DECLARE_SPELL_FUN(spell_stone_skin);
DECLARE_SPELL_FUN(spell_summon);
DECLARE_SPELL_FUN(spell_mirror_image);
DECLARE_SPELL_FUN(spell_cloak_of_mind);
DECLARE_SPELL_FUN(spell_carnal_reach);
DECLARE_SPELL_FUN(spell_spirit_link);
DECLARE_SPELL_FUN(spell_teleport);
DECLARE_SPELL_FUN(spell_thirst);
DECLARE_SPELL_FUN(spell_ventriloquate);
DECLARE_SPELL_FUN(spell_water_breathing);
DECLARE_SPELL_FUN(spell_weaken);
DECLARE_SPELL_FUN(spell_wizard_mark);
DECLARE_SPELL_FUN(spell_word_of_recall);
DECLARE_SPELL_FUN(spell_acid_breath);
DECLARE_SPELL_FUN(spell_fire_breath);
DECLARE_SPELL_FUN(spell_frost_breath);
DECLARE_SPELL_FUN(spell_gas_breath);
DECLARE_SPELL_FUN(spell_lightning_breath);
DECLARE_SPELL_FUN(spell_general_purpose);
DECLARE_SPELL_FUN(spell_high_explosive);
DECLARE_SPELL_FUN(spell_grandeur);
DECLARE_SPELL_FUN(spell_scramble);
DECLARE_SPELL_FUN(spell_minimation);
DECLARE_SPELL_FUN(spell_loneliness);
DECLARE_SPELL_FUN(spell_confusion);
DECLARE_SPELL_FUN(spell_hypnosis);
DECLARE_SPELL_FUN(spell_demand);
DECLARE_SPELL_FUN(spell_phantasm_monster);
DECLARE_SPELL_FUN(spell_giant_insect);
DECLARE_SPELL_FUN(spell_reach_elemental);
DECLARE_SPELL_FUN(spell_taunt);
DECLARE_SPELL_FUN(spell_blink);
DECLARE_SPELL_FUN(spell_hold_person);
DECLARE_SPELL_FUN(spell_weaponsmith);
DECLARE_SPELL_FUN(spell_create_shadow);
DECLARE_SPELL_FUN(spell_cloud_of_poison);
DECLARE_SPELL_FUN(spell_wail);
DECLARE_SPELL_FUN(spell_granite_stare);
DECLARE_SPELL_FUN(spell_cave_bears);
DECLARE_SPELL_FUN(spell_enlargement);
DECLARE_END_FUN(end_enlargement);
DECLARE_SPELL_FUN(spell_detonation);
DECLARE_SPELL_FUN(spell_turn_magic);
DECLARE_SPELL_FUN(spell_primal_rage);
DECLARE_SPELL_FUN(spell_shifting_sands);
DECLARE_SPELL_FUN(spell_cone_of_fear);

DECLARE_SPELL_FUN(spell_room_test);
DECLARE_SPELL_FUN(spell_tusunami);
DECLARE_SPELL_FUN(spell_huricane);

DECLARE_SPELL_FUN(spell_summon_lesser);
DECLARE_SPELL_FUN(spell_summon_horde);
DECLARE_SPELL_FUN(spell_summon_greater);
DECLARE_SPELL_FUN(spell_summon_lord);
DECLARE_SPELL_FUN(spell_banish);
DECLARE_SPELL_FUN(spell_ice_bolt);
DECLARE_SPELL_FUN(spell_blast_of_rot);

DECLARE_SPELL_FUN(spell_crushing_hand);
DECLARE_SPELL_FUN(spell_clenched_fist);
DECLARE_SPELL_FUN(spell_feeblemind);
DECLARE_SPELL_FUN(spell_psychic_crush);
DECLARE_SPELL_FUN(spell_darkness);
DECLARE_SPELL_FUN(spell_karma);
DECLARE_SPELL_FUN(spell_cloudkill);
DECLARE_SPELL_FUN(spell_pyrotechnics);

DECLARE_SPELL_FUN(spell_displace);
DECLARE_SPELL_FUN(spell_homonculus);
DECLARE_SPELL_FUN(spell_mass_protect);
DECLARE_SPELL_FUN(spell_shriek);
DECLARE_SPELL_FUN(spell_soundproof);

DECLARE_SPELL_FUN(spell_repulsion);
DECLARE_SPELL_FUN(spell_skeletal_warrior);
DECLARE_SPELL_FUN(spell_turn_undead);
DECLARE_SPELL_FUN(spell_withstand_death);
DECLARE_SPELL_FUN(spell_animate_tree);
DECLARE_SPELL_FUN(spell_prismatic_sphere);
DECLARE_SPELL_FUN(spell_spell_stealing);
DECLARE_END_FUN(end_spell_stealing);
DECLARE_END_FUN(end_paint_power);
DECLARE_SPELL_FUN(spell_oculary);
DECLARE_SPELL_FUN(spell_soul_trap);
DECLARE_SPELL_FUN(spell_figurine_spell);
DECLARE_SPELL_FUN(spell_remove_align);
DECLARE_SPELL_FUN(spell_inform);
DECLARE_SPELL_FUN(spell_atheism);
DECLARE_SPELL_FUN(spell_attraction);
DECLARE_SPELL_FUN(spell_disenchant);
DECLARE_SPELL_FUN(spell_missionary);
DECLARE_SPELL_FUN(spell_misty_cloak);
DECLARE_SPELL_FUN(spell_flag);
DECLARE_SPELL_FUN(spell_jinx);
DECLARE_SPELL_FUN(spell_moonbeam);
DECLARE_SPELL_FUN(spell_sunray);
DECLARE_SPELL_FUN(spell_life_wave);
DECLARE_SPELL_FUN(spell_shock_wave);
DECLARE_SPELL_FUN(spell_meteor_swarm);
DECLARE_SPELL_FUN(spell_delayed_fireball);
DECLARE_END_FUN(end_delayed_fireball);
DECLARE_SPELL_FUN(spell_earth_to_mud);
DECLARE_SPELL_FUN(spell_elemental_protection);
DECLARE_END_FUN(end_elemental_protection);
DECLARE_SPELL_FUN(spell_winter_storm);
DECLARE_SPELL_FUN(spell_summon_angel);
DECLARE_SPELL_FUN(spell_flesh_golem);
DECLARE_SPELL_FUN(spell_animate_dead);
DECLARE_END_FUN(end_animate_dead);
DECLARE_SPELL_FUN(spell_wizards_eye);
DECLARE_END_FUN(end_wizards_eye);
DECLARE_SPELL_FUN(spell_embelish);
DECLARE_SPELL_FUN(spell_beacon);
DECLARE_SPELL_FUN(spell_nightmares);
DECLARE_SPELL_FUN(spell_fortify);
DECLARE_SPELL_FUN(spell_replicate);
DECLARE_SPELL_FUN(spell_plant);
DECLARE_END_FUN(end_plant);
DECLARE_SPELL_FUN(spell_repel);
DECLARE_SPELL_FUN(spell_abandon);
DECLARE_END_FUN(end_abandon);
DECLARE_SPELL_FUN(spell_duplication);
DECLARE_END_FUN(end_duplication);
DECLARE_SPELL_FUN(spell_simularcum);
DECLARE_SPELL_FUN(spell_create_door);
DECLARE_END_FUN(end_create_door);
DECLARE_SPELL_FUN(spell_tornado);
DECLARE_SPELL_FUN(spell_whirlpool);
DECLARE_SPELL_FUN(spell_true_sight);
DECLARE_SPELL_FUN(spell_voodoo);
DECLARE_SPELL_FUN(spell_sharpen);
DECLARE_SPELL_FUN(spell_wildfire);
DECLARE_SPELL_FUN(spell_symbol);
DECLARE_SPELL_FUN(spell_create_oil);
DECLARE_SPELL_FUN(spell_drain_life);
DECLARE_SPELL_FUN(spell_channel_energy);
DECLARE_SPELL_FUN(spell_animal_skins);
DECLARE_SPELL_FUN(clanspell_paradox);
DECLARE_SPELL_FUN(spell_minor_revitalize);
DECLARE_SPELL_FUN(spell_lesser_revitalize);
DECLARE_SPELL_FUN(spell_greater_revitalize);
DECLARE_SPELL_FUN(spell_thorn_mantle);
DECLARE_SPELL_FUN(spell_leaf_shield);
DECLARE_SPELL_FUN(spell_stun);

#endif
