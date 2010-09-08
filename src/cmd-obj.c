/*
 * File: cmd-obj.c
 * Purpose: Handle objects in various ways
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2007 Andrew Sidwell
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */
#include "angband.h"
#include "z-quark.h"
#include "cmds.h"
#include "tvalsval.h"

/*** Inscriptions ***/

/* Can has inscrip pls */
static bool obj_has_inscrip(const object_type *o_ptr)
{
	return (o_ptr->note ? TRUE : FALSE);
}

/* Remove inscription */
static void obj_uninscribe(object_type *o_ptr, int item)
{
	o_ptr->note = 0;
	msg_print("Inscription removed.");

//	p_ptr->notice |= (PN_COMBINE | PN_SQUELCH);
	p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
	p_ptr->notice |= (PN_COMBINE);
}

/* Add inscription */
static void obj_inscribe(object_type *o_ptr, int item)
{
	char o_name[80];
	char tmp[80] = "";

	object_desc(o_name, sizeof(o_name), o_ptr, TRUE, ODESC_FULL);
	msg_format("Inscribing %s.", o_name);
	message_flush();

	/* Use old inscription */
	if (o_ptr->note)
		strnfmt(tmp, sizeof(tmp), "%s", quark_str(o_ptr->note));

	/* Get a new inscription (possibly empty) */
	if (get_string("Inscription: ", tmp, sizeof(tmp)))
	{
		o_ptr->note = quark_add(tmp);

//		p_ptr->notice |= (PN_COMBINE | PN_SQUELCH);
		p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
		p_ptr->notice |= (PN_COMBINE);
	}
}


/*** Examination ***/
static void obj_examine(object_type *o_ptr, int item)
{
    object_info_screen(o_ptr);
}



/*** Taking off/putting on ***/

/* Can only take off non-cursed items */
static bool obj_can_takeoff(const object_type *o_ptr)
{
	return !o_ptr->is_cursed();
}

/* Can only put on wieldable items */
static bool obj_can_wear(const object_type *o_ptr)
{
	return (wield_slot(o_ptr) >= INVEN_WIELD);
}


/* Take off an item */
static void obj_takeoff(object_type *o_ptr, int item)
{
	(void)inven_takeoff(item, 255);
	p_ptr->energy_use = 50;
}

/* Wield or wear an item */
static void obj_wear(object_type *o_ptr, int item)
{
	char o_name[80];

	/* Check the slot */
	int slot = wield_slot(o_ptr);
	assert((INVEN_PACK < slot) && (slot < INVEN_TOTAL) && "retval precondition");
	const object_type* const equip_o_ptr = &p_ptr->inventory[slot];

	/* Prevent wielding into a cursed slot */
	if (equip_o_ptr->is_cursed())
	{
		/* Message */
		object_desc(o_name, sizeof(o_name), equip_o_ptr, FALSE, ODESC_BASE);
		msg_format("The %s you are %s appears to be cursed.",
		           o_name, describe_use(slot));

		return;
	}

	/* "!t" checks for taking off */
	if (check_for_inscrip(o_ptr, "!t"))
	{
		/* Prompt */
		object_desc(o_name, sizeof(o_name), equip_o_ptr, TRUE, ODESC_FULL);

		/* Forget it */
		if (!get_check(format("Really take off %s? ", o_name))) return;
	}
	
	wield_item(o_ptr, item);
}

/* Drop an item */
static void obj_drop(object_type *o_ptr, int item)
{
	int amt;

	amt = get_quantity(NULL, o_ptr->number);
	if (amt <= 0) return;

	/* Hack -- Cannot remove cursed items */
	if ((item >= INVEN_WIELD) && o_ptr->is_cursed())
	{
		msg_print("Hmmm, it seems to be cursed.");
		return;
	}

	inven_drop(item, amt);
	p_ptr->energy_use = 50;
}

#if 0
/*** Squelch stuff ***/

/* See if one can squelch a given kind of item. */
static bool obj_can_set_squelch(const object_type *o_ptr)
{
	object_kind *k_ptr = &object_type::k_info[o_ptr->k_idx];

	if (k_ptr->squelch) return FALSE;
	if (!squelch_tval(o_ptr->tval)) return FALSE;

	/* Only allow if aware */
	return o_ptr->aware();
}

/*
 * Mark item as "squelch".
 */
static void obj_set_squelch(object_type *o_ptr, int item)
{
	k_info[o_ptr->k_idx].squelch = TRUE;
}
#endif


/*** Casting and browsing ***/

static bool obj_can_browse(const object_type *o_ptr)
{
	if (o_ptr->obj_id.tval != p_ptr->spell_book()) return FALSE;
	return TRUE;
}


static bool obj_cast_pre(void)
{
   	if (!p_ptr->spell_book())
	{
		msg_print("You cannot pray or produce magics.");
		return FALSE;
	}

	if (p_ptr->timed[TMD_BLIND] || no_lite())
	{
		msg_print("You cannot see!");
		return FALSE;
	}

	if (p_ptr->timed[TMD_CONFUSED])
	{
		msg_print("You are too confused!");
		return FALSE;
	}

	return TRUE;
}

/* A prerequisite to studying */
static bool obj_study_pre(void)
{
	if (!obj_cast_pre())
		return FALSE;

	if (!p_ptr->new_spells)
	{
		const char* const p = ((p_ptr->spell_book() == TV_MAGIC_BOOK) ? "spell" : "prayer");
		msg_format("You cannot learn any new %ss!", p);
		return FALSE;
	}

	return TRUE;
}


/* Peruse spells in a book */
static void obj_browse(object_type *o_ptr, int item)
{
	do_cmd_browse_aux(o_ptr);
}

/* Study a book to gain a new spell */
static void obj_study(object_type *o_ptr, int item)
{
	int spell;

	/* Track the object kind */
	object_kind_track(o_ptr->k_idx);
	handle_stuff();

	/* Choose a spell to study */
	spell = spell_choose_new(o_ptr);
	if (spell < 0) return;

	/* Learn the spell */
	spell_learn(spell);
	p_ptr->energy_use = 100;
}

/* Cast a spell from a book */
static void obj_cast(object_type *o_ptr, int item)
{
	int spell;
	const char* const verb = ((p_ptr->spell_book() == TV_MAGIC_BOOK) ? "cast" : "recite");

	/* Track the object kind */
	object_kind_track(o_ptr->k_idx);
	handle_stuff();

	/* Ask for a spell */
	spell = get_spell(o_ptr, verb, TRUE);
	if (spell < 0)
	{
		const char* const p = ((p_ptr->spell_book() == TV_MAGIC_BOOK) ? "spell" : "prayer");

		if (spell == -2) msg_format("You don't know any %ss in that book.", p);
		return;
	}

	/* Cast a spell */
	if (spell_cast(spell))
	    p_ptr->energy_use = 100;
}


/*** Using items the traditional way ***/

/* Determine if the player can read scrolls. */
static bool obj_read_pre(void)
{
	if (p_ptr->timed[TMD_BLIND])
	{
		msg_print("You can't see anything.");
		return FALSE;
	}

	if (no_lite())
	{
		msg_print("You have no light to read by.");
		return FALSE;
	}

	if (p_ptr->timed[TMD_CONFUSED])
	{
		msg_print("You are too confused to read!");
		return FALSE;
	}

#if 0
	if (p_ptr->timed[TMD_AMNESIA])
	{
		msg_print("You can't remember how to read!");
		return FALSE;
	}
#endif

	return TRUE;
}

/* Determine if an object is zappable */
static bool obj_can_zap(const object_type *o_ptr)
{
	const object_kind *k_ptr = &object_type::k_info[o_ptr->k_idx];
	if (o_ptr->obj_id.tval != TV_ROD) return FALSE;

	/* All still charging? */
//	if (o_ptr->number <= (o_ptr->timeout + (k_ptr->time_base - 1)) / k_ptr->time_base) return FALSE;
	if (o_ptr->timeout > (o_ptr->pval - k_ptr->pval)) return FALSE;

	/* Otherwise OK */
	return TRUE;
}

/* Determine if an object is activatable */
static bool obj_can_activate(const object_type *o_ptr)
{
	if (!o_ptr->known()) return FALSE;	/* Not known */
	if (o_ptr->timeout) return FALSE;	/* Check the recharge */
	return obj_has_activation(o_ptr);
}


/* Use a staff */
static void obj_use_staff(object_type *o_ptr, int item)
{
	do_cmd_use(o_ptr, item, MSG_USE_STAFF, USE_CHARGE);
}

/* Aim a wand */
static void obj_use_wand(object_type *o_ptr, int item)
{
	do_cmd_use(o_ptr, item, MSG_ZAP_ROD, USE_CHARGE);
}

/* Zap a rod */
static void obj_use_rod(object_type *o_ptr, int item)
{
	do_cmd_use(o_ptr, item, MSG_ZAP_ROD, USE_TIMEOUT);
}

/* Activate a wielded object */
static void obj_activate(object_type *o_ptr, int item)
{
	do_cmd_use(o_ptr, item, MSG_ACT_ARTIFACT, USE_TIMEOUT);
}

/* Eat some food */
static void obj_use_food(object_type *o_ptr, int item)
{
	do_cmd_use(o_ptr, item, MSG_EAT, USE_SINGLE);
}

/* Quaff a potion (from the pack or the floor) */
static void obj_use_potion(object_type *o_ptr, int item)
{
	do_cmd_use(o_ptr, item, MSG_QUAFF, USE_SINGLE);
}

/* Read a scroll (from the pack or floor) */
static void obj_use_scroll(object_type *o_ptr, int item)
{
	do_cmd_use(o_ptr, item, MSG_GENERIC, USE_SINGLE);
}

/*** Refuelling ***/

static bool obj_refill_pre(void)
{
   	const object_type* const o_ptr = &p_ptr->inventory[INVEN_LITE];
//	u32b f[OBJECT_FLAG_STRICT_UB];

//	object_flags(o_ptr, f);

	if (o_ptr->obj_id.tval != TV_LITE)
	{
		msg_print("You are not wielding a light.");
		return FALSE;
	}

	else if (o_ptr->obj_id.sval != SV_LITE_LANTERN && o_ptr->obj_id.sval != SV_LITE_TORCH)
//	else if (f3 & TR3_NO_FUEL)
	{
		msg_print("Your light cannot be refilled.");
		return FALSE;
	}

	return TRUE;
}

static bool obj_can_refill(const object_type *o_ptr)
{
//	u32b f[OBJECT_FLAG_STRICT_UB];
	const object_type* const j_ptr = &p_ptr->inventory[INVEN_LITE];

	/* Get flags */
//	object_flags(o_ptr, f);

	if (j_ptr->obj_id.sval == SV_LITE_LANTERN)
	{
		/* Flasks of oil are okay */
		if (o_ptr->obj_id.tval == TV_FLASK) return (TRUE);
	}

	/* Non-empty, non-everburning sources are okay */
	if ((o_ptr->obj_id == j_ptr->obj_id) &&
	    (o_ptr->timeout > 0) &&
		(o_ptr->obj_id.sval == SV_LITE_LANTERN || o_ptr->obj_id.sval == SV_LITE_TORCH))
//		!(f3 & TR3_NO_FUEL))
	{
		return (TRUE);
	}

	/* Assume not okay */
	return (FALSE);
}

static void obj_refill(object_type *o_ptr, int item)
{
	object_type* const j_ptr = &p_ptr->inventory[INVEN_LITE];
	p_ptr->energy_use = 50;

	/* It's a lamp */
	if (j_ptr->obj_id.sval == SV_LITE_LANTERN)
		refill_lamp(j_ptr, o_ptr, item);

	/* It's a torch */
	else if (j_ptr->obj_id.sval == SV_LITE_TORCH)
		refuel_torch(j_ptr, o_ptr, item);
}




/*** Handling bits ***/

/* Item "action" type */
typedef struct
{
	void (*action)(object_type *, int);
	const char *desc;

	const char *prompt;
	const char *noop;

	bool (*filter)(const object_type *o_ptr);
	int mode;
	bool (*prereq)(void);
} item_act_t;


/* All possible item actions */
static item_act_t item_actions[] =
{
	{ obj_uninscribe, "uninscribe",
	  "Un-inscribe which item? ", "You have nothing to un-inscribe.",
	  obj_has_inscrip, (USE_EQUIP | USE_INVEN | USE_FLOOR), NULL },

	{ obj_inscribe, "inscribe",
	  "Inscribe which item? ", "You have nothing to inscribe.",
	  NULL, (USE_EQUIP | USE_INVEN | USE_FLOOR), NULL },

	{ obj_examine, "examine",
	  "Examine which item? ", "You have nothing to examine.",
	  NULL, (USE_EQUIP | USE_INVEN | USE_FLOOR), NULL },

	/*** Takeoff/drop/wear ***/
	{ obj_takeoff, "takeoff",
	  "Take off which item? ", "You are not wearing anything you can take off.",
	  obj_can_takeoff, USE_EQUIP, NULL },

	{ obj_wear, "wield",
	  "Wear/Wield which item? ", "You have nothing you can wear or wield.",
	  obj_can_wear, (USE_INVEN | USE_FLOOR), NULL },

	{ obj_drop, "drop",
	  "Drop which item? ", "You have nothing to drop.",
	  NULL, (USE_EQUIP | USE_INVEN), NULL },

#if 0
	{ obj_set_squelch, "setsquelch",
	  "Squelch which item kind? ", "You have nothing you can squelch.",
	  obj_can_set_squelch, (USE_INVEN | USE_FLOOR), NULL },
#endif

	/*** Spellbooks ***/
	{ obj_browse, "browse",
	  "Browse which book? ", "You have no books that you can read.",
	  obj_can_browse, (USE_INVEN | USE_FLOOR), NULL },

	{ obj_study, "study",
	  "Study which book? ", "You have no books that you can read.",
	  obj_can_browse, (USE_INVEN | USE_FLOOR), obj_study_pre },

	{ obj_cast, "cast",
	  "Use which book? ", "You have no books that you can read.",
	  obj_can_browse, (USE_INVEN | USE_FLOOR), obj_cast_pre },

	/*** Item usage ***/
	{ obj_use_staff, "use",
	  "Use which staff? ", "You have no staff to use.",
	  o_ptr_is<TV_STAFF>, (USE_INVEN | USE_FLOOR), NULL },

	{ obj_use_wand, "aim",
      "Aim which wand? ", "You have no wand to aim.",
	  o_ptr_is<TV_WAND>, (USE_INVEN | USE_FLOOR), NULL },

	{ obj_use_rod, "zap",
      "Zap which rod? ", "You have no charged rods to zap.",
	  obj_can_zap, (USE_INVEN | USE_FLOOR), NULL },

	{ obj_activate, "activate",
      "Activate which item? ", "You have nothing to activate.",
	  obj_can_activate, USE_EQUIP, NULL },

	{ obj_use_food, "eat",
      "Eat which item? ", "You have nothing to eat.",
	  o_ptr_is<TV_FOOD>, (USE_INVEN | USE_FLOOR), NULL },

	{ obj_use_potion, "quaff",
      "Quaff which potion? ", "You have no potions to quaff.",
	  o_ptr_is<TV_POTION>, (USE_INVEN | USE_FLOOR), NULL },

	{ obj_use_scroll, "read",
      "Read which scroll? ", "You have no scrolls to read.",
	  o_ptr_is<TV_SCROLL>, (USE_INVEN | USE_FLOOR), obj_read_pre },

	{ obj_refill, "refill",
      "Refuel with what fuel source? ", "You have nothing to refuel with.",
	  obj_can_refill, (USE_INVEN | USE_FLOOR), obj_refill_pre },
};


/* List matching up to item_actions[] */
typedef enum
{
	ACTION_UNINSCRIBE = 0,
	ACTION_INSCRIBE,
	ACTION_EXAMINE,
	ACTION_TAKEOFF,
	ACTION_WIELD,
	ACTION_DROP,
//	ACTION_SET_SQUELCH,

	ACTION_BROWSE,
	ACTION_STUDY,
	ACTION_CAST,

	ACTION_USE_STAFF,
	ACTION_AIM_WAND,
	ACTION_ZAP_ROD,
	ACTION_ACTIVATE,
	ACTION_EAT_FOOD,
	ACTION_QUAFF_POTION,
	ACTION_READ_SCROLL,
	ACTION_REFILL
} item_act;



/*** Old-style noun-verb functions ***/

/* Generic "do item action" function */
static void do_item(item_act act)
{
	int item;
	object_type *o_ptr;
	const char* q;
	const char* s;

	if (item_actions[act].prereq)
	{
		if (!item_actions[act].prereq())
			return;
	}

	/* Get item */
	q = item_actions[act].prompt;
	s = item_actions[act].noop;
	item_tester_hook = item_actions[act].filter;
	if (!get_item(&item, q, s, item_actions[act].mode)) return;

	/* Get the item */
	o_ptr = get_o_ptr_from_inventory_or_floor(item);

	item_actions[act].action(o_ptr, item);
}

/* Wrappers */
void do_cmd_uninscribe(void) { do_item(ACTION_UNINSCRIBE); }
void do_cmd_inscribe(void) { do_item(ACTION_INSCRIBE); }
void do_cmd_observe(void) { do_item(ACTION_EXAMINE); }
void do_cmd_takeoff(void) { do_item(ACTION_TAKEOFF); }
void do_cmd_wield(void) { do_item(ACTION_WIELD); }
void do_cmd_drop(void) { do_item(ACTION_DROP); }
//void do_cmd_mark_squelch(void) { do_item(ACTION_SET_SQUELCH); }
void do_cmd_browse(void) { do_item(ACTION_BROWSE); }
void do_cmd_study(void) { do_item(ACTION_STUDY); }
void do_cmd_cast(void) { do_item(ACTION_CAST); }
void do_cmd_pray(void) { do_item(ACTION_CAST); }
void do_cmd_use_staff(void) { do_item(ACTION_USE_STAFF); }
void do_cmd_aim_wand(void) { do_item(ACTION_AIM_WAND); }
void do_cmd_zap_rod(void) { do_item(ACTION_ZAP_ROD); }
void do_cmd_activate(void) { do_item(ACTION_ACTIVATE); }
void do_cmd_eat_food(void) { do_item(ACTION_EAT_FOOD); }
void do_cmd_quaff_potion(void) { do_item(ACTION_QUAFF_POTION); }
void do_cmd_read_scroll(void) { do_item(ACTION_READ_SCROLL); }
void do_cmd_refill(void) { do_item(ACTION_REFILL); }
