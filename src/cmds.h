#ifndef INCLUDED_CMDS_H
#define INCLUDED_CMDS_H

/* cmd.c */
extern void cmd_init(void);
extern void process_command(bool no_request);

/* cmd-obj.c */
void do_cmd_uninscribe(void);
void do_cmd_inscribe(void);
void do_cmd_observe(void);
void do_cmd_takeoff(void);
void do_cmd_wield(void);
void do_cmd_drop(void);
void do_cmd_mark_squelch(void);
void do_cmd_browse(void);
void do_cmd_study(void);
void do_cmd_cast(void);
void do_cmd_pray(void);
void do_cmd_use_staff(void);
void do_cmd_aim_wand(void);
void do_cmd_zap_rod(void);
void do_cmd_activate(void);
void do_cmd_eat_food(void);
void do_cmd_quaff_potion(void);
void do_cmd_read_scroll(void);
void do_cmd_refill(void);

/* cmd2.c */
extern void do_cmd_go_up(void);
extern void do_cmd_go_down(void);
extern void do_cmd_search(void);
extern void do_cmd_toggle_search(void);
extern void do_cmd_open(void);
extern void do_cmd_close(void);
extern void do_cmd_tunnel(void);
extern void do_cmd_disarm(void);
extern void do_cmd_bash(void);
extern void do_cmd_alter(void);
extern void do_cmd_spike(void);
extern void do_cmd_walk(void);
extern void do_cmd_jump(void);
extern void do_cmd_run(void);
extern void do_cmd_pathfind(int y, int x);
extern void do_cmd_hold(void);
extern void do_cmd_pickup(void);
extern void do_cmd_rest(void);

/* cmd3.c */
extern void do_cmd_inven(void);
extern void do_cmd_equip(void);
extern void do_cmd_destroy(void);
void refill_lamp(object_type *o_ptr, object_type *j_ptr, int item);
void refuel_torch(object_type *o_ptr, object_type *j_ptr,  int item);
extern void do_cmd_target(void);
extern void do_cmd_look(void);
extern void do_cmd_locate(void);
extern void do_cmd_query_symbol(void);
extern bool ang_sort_comp_hook(const void *u, const void *v, int a, int b);
extern void ang_sort_swap_hook(void *u, void *v, int a, int b);

/* cmd4.c */
extern void do_cmd_redraw(void);
extern void do_cmd_resize(void);
extern void do_cmd_change_name(void);
extern void do_cmd_message_one(void);
extern void do_cmd_messages(void);
extern void do_cmd_options(void);
extern void do_cmd_pref(void);
extern void do_cmd_macros(void);
extern void do_cmd_visuals(void);
extern void do_cmd_colors(void);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(void);
extern void do_cmd_load_screen(void);
extern void do_cmd_save_screen(void);
extern void do_cmd_knowledge(void);
extern void init_cmd4_c(void);

extern void do_cmd_knowledge_objects(void *obj, const char *name);


/* cmd6.c */

/* Types of item use */
typedef enum
{
	USE_TIMEOUT,
	USE_CHARGE,
	USE_SINGLE
} use_type;

void do_cmd_use(object_type *o_ptr, int item, int snd, use_type use);


#endif

