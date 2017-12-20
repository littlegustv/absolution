#ifndef FLAGS_H
#define FLAGS_H

struct flag_type {
   char *name;
   bitset bit;
   bool settable;
};

int flag_lookup (const char *name, const struct flag_type *flag_table);

extern const struct flag_type act_flags[];
extern const struct flag_type plr_flags[];
extern const struct flag_type liquid_flags[];
extern const struct flag_type affect_flags[];
extern const struct flag_type off_flags[];
extern const struct flag_type damtype_flags[];
extern const struct flag_type imm_flags[];
extern const struct flag_type form_flags[];
extern const struct flag_type part_flags[];
extern const struct flag_type comm_flags[];
extern const struct flag_type area_flags[];
extern const struct flag_type sex_flags[];
extern const struct flag_type exit_flags[];
extern const struct flag_type door_resets[];
extern const struct flag_type room_flags[];
extern const struct flag_type sector_flags[];
extern const struct flag_type type_flags[];
extern const struct flag_type sheath_types[];
extern const struct flag_type extra_flags[];
extern const struct flag_type wear_flags[];
extern const struct flag_type apply_flags[];
extern const struct flag_type wear_loc_strings[];
extern const struct flag_type wear_loc_flags[];
extern const struct flag_type weapon_flags[];
extern const struct flag_type container_flags[];
extern const struct flag_type ac_type[];
extern const struct flag_type size_flags[];
extern const struct flag_type weapon_class[];
extern const struct flag_type weapon_type2[];
extern const struct flag_type res_flags[];
extern const struct flag_type vuln_flags[];
extern const struct flag_type material_type[];
extern const struct flag_type position_flags[];
extern const struct flag_type portal_flags[];
extern const struct flag_type furniture_flags[];
extern const struct flag_type mprog_flags[];

#endif
