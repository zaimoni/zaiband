/* File: spells1.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
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
#include "project.h"
#include "learn.h"
#include "option.h"
#include "raceflag.h"
#include "tvalsval.h"

#include "flow.h"

/*
 * Helper function -- return a "nearby" race for polymorphing
 *
 * Note that this function is one of the more "dangerous" ones...
 */
s16b poly_r_idx(int r_idx)
{
	monster_race *r_ptr = &monster_type::r_info[r_idx];

	int i, lev1, lev2;

	/* Hack -- Uniques never polymorph */
	if (r_ptr->flags[0] & RF0_UNIQUE) return r_idx;

	/* Allowable range of "levels" for resulting monster */
	lev1 = r_ptr->level - ((randint(20)/randint(9))+1);
	lev2 = r_ptr->level + ((randint(20)/randint(9))+1);

	/* Pick a (possibly new) non-unique race */
	for (i = 0; i < 1000; i++)
	{
		/* Pick a new race, using a level calculation */
		int r = get_mon_num((p_ptr->depth + r_ptr->level) / 2 + 5);

		/* Handle failure */
		if (!r) break;

		/* Obtain race */
		r_ptr = &monster_type::r_info[r];

		/* Ignore unique monsters */
		if (r_ptr->flags[0] & RF0_UNIQUE) continue;

		/* Ignore monsters with incompatible levels */
		if ((r_ptr->level < lev1) || (r_ptr->level > lev2)) continue;

		/* Use that index */
		r_idx = r;

		/* Done */
		break;
	}

	/* Result */
	return r_idx;
}

static void _teleport_away(coord g, int dis, int disallow_flg, int sound_code)
{
	int d, i;
	int min = dis/2;	/* Minimum distance */
	coord n;

	bool look = TRUE;

	/* Look until done */
	while (look)
	{
		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			do	{
				n.y = rand_spread(g.y, dis);
				n.x = rand_spread(g.x, dis);
				d = distance(g.y, g.x, n.y, n.x);
				if ((d >= min) && (d <= dis)) break;
				}
			while((d < min) || (d > dis));

			/* Ignore illegal locations */
			if (!in_bounds_fully(n.y, n.x)) continue;

			/* Require "empty" floor space */
			if (!cave_empty_bold(n.y, n.x)) continue;

			/* Hack -- no teleport onto glyph of warding for monsters */
			/* key from disallow-flag, as monsters are not restricted from teleporting into vaults */
			if (disallow_flg)
				{	/* player */
				if (cave_info[n.y][n.x] & disallow_flg) continue;	
				}
			else{	/* monster */
				if (FEAT_GLYPH == cave_feat[n.y][n.x]) continue;
				}
			if (!disallow_flg && (FEAT_GLYPH == cave_feat[n.y][n.x])) continue;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis *= 2;

		/* Decrease the minimum distance */
		min /= 2;
	}

	/* Sound */
	sound(sound_code);

	/* Swap the monsters */
	monster_swap(g, n);
}

/*
 * Teleport a monster, normally up to "dis" grids away.
 *
 * Attempt to move the monster at least "dis/2" grids away.
 *
 * But allow variation to prevent infinite loops.
 */
void teleport_away(const m_idx_type m_idx, int dis)
{
	monster_type *m_ptr = &mon_list[m_idx];

	/* Paranoia */
	if (!m_ptr->r_idx) return;

	_teleport_away(m_ptr->loc,dis,0,MSG_TPOTHER);
}


/*
 * Teleport the player to a location up to "dis" grids away.
 *
 * If no such spaces are readily available, the distance may increase.
 * Try very hard to move the player at least a quarter that distance.
 */
void teleport_player(int dis)
{
	/* no teleporting into vaults */
	_teleport_away(p_ptr->loc,dis,(CAVE_ICKY),MSG_TELEPORT);

	/* Handle stuff XXX XXX XXX */
	handle_stuff();
}



/*
 * Teleport player to a grid near the given location
 *
 * This function is slightly obsessive about correctness.
 * This function allows teleporting into vaults (!)
 */
void teleport_player_to(coord g)
{
	coord t = p_ptr->loc;

	int dis = 0, ctr = 0;

	/* Find a usable location */
	do	{
		/* Occasionally advance the distance */
		if (ctr++ > (4 * dis * dis + 4 * dis + 1))
		{
			ctr = 0;
			dis++;
		}

		/* Pick a nearby legal location */
		do	{
			t.y = rand_spread(g.y, dis);
			t.x = rand_spread(g.x, dis);
			}
		while(!in_bounds_fully(t.y, t.x));
		}
	while(cave_naked_bold(t.y, t.x));	/* Accept "naked" floor grids */

	/* Sound */
	sound(MSG_TELEPORT);

	/* Move player */
	monster_swap(p_ptr->loc, t);

	/* Handle stuff XXX XXX XXX */
	handle_stuff();
}



/*
 * Teleport the player one level up or down (random when legal)
 */
void teleport_player_level(void)
{
	if (OPTION(adult_ironman))
	{
		msg_print("Nothing happens.");
		return;
	}


	if (!p_ptr->depth)
	{
		message(MSG_TPLEVEL, "You sink through the floor.");

		/* New depth */
		p_ptr->depth++;
	}

	else if (is_quest(p_ptr->depth) || (p_ptr->depth >= MAX_DEPTH-1))
	{
		message(MSG_TPLEVEL, "You rise up through the ceiling.");

		/* New depth */
		p_ptr->depth--;
	}

	else if (one_in_(2))
	{
		message(MSG_TPLEVEL, "You rise up through the ceiling.");

		/* New depth */
		p_ptr->depth--;
	}

	else
	{
		message(MSG_TPLEVEL, "You sink through the floor.");

		/* New depth */
		p_ptr->depth++;

	}

	/* Leaving */
	p_ptr->leaving = true;
}






/*
 * Return a color to use for the bolt/ball spells
 */
static byte spell_color(int type)
{
	/* Analyze */
	switch (type)
	{
		case GF_MISSILE:	return (TERM_VIOLET);
		case GF_ACID:		return (TERM_SLATE);
		case GF_ELEC:		return (TERM_BLUE);
		case GF_FIRE:		return (TERM_RED);
		case GF_COLD:		return (TERM_WHITE);
		case GF_POIS:		return (TERM_GREEN);
		case GF_HOLY_ORB:	return (TERM_L_DARK);
		case GF_MANA:		return (TERM_L_DARK);
		case GF_ARROW:		return (TERM_WHITE);
		case GF_WATER:		return (TERM_SLATE);
		case GF_NETHER:		return (TERM_L_GREEN);
		case GF_CHAOS:		return (TERM_VIOLET);
		case GF_DISENCHANT:	return (TERM_VIOLET);
		case GF_NEXUS:		return (TERM_L_RED);
		case GF_CONFUSION:	return (TERM_L_UMBER);
		case GF_SOUND:		return (TERM_YELLOW);
		case GF_SHARD:		return (TERM_UMBER);
		case GF_FORCE:		return (TERM_UMBER);
		case GF_INERTIA:	return (TERM_L_WHITE);
		case GF_GRAVITY:	return (TERM_L_WHITE);
		case GF_TIME:		return (TERM_L_BLUE);
		case GF_LITE_WEAK:	return (TERM_ORANGE);
		case GF_LITE:		return (TERM_ORANGE);
		case GF_DARK_WEAK:	return (TERM_L_DARK);
		case GF_DARK:		return (TERM_L_DARK);
		case GF_PLASMA:		return (TERM_RED);
		case GF_METEOR:		return (TERM_RED);
		case GF_ICE:		return (TERM_WHITE);
	}

	/* Standard "color" */
	return (TERM_WHITE);
}



/*
 * Find the attr/char pair to use for a spell effect
 *
 * It is moving (or has moved) from (x,y) to (nx,ny).
 *
 * If the distance is not "one", we (may) return "*".
 */
static attr_char bolt_pict(int y, int x, int ny, int nx, int typ)
{
	int base;

	byte k;

	attr_char ret;

	if (!(use_graphics && (arg_graphics == GRAPHICS_DAVID_GERVAIS)))
	{
		/* No motion (*) */
		if ((ny == y) && (nx == x)) base = 0x30;

		/* Vertical (|) */
		else if (nx == x) base = 0x40;

		/* Horizontal (-) */
		else if (ny == y) base = 0x50;

		/* Diagonal (/) */
		else if ((ny-y) == (x-nx)) base = 0x60;

		/* Diagonal (\) */
		else if ((ny-y) == (nx-x)) base = 0x70;

		/* Weird (*) */
		else base = 0x30;

		/* Basic spell color */
		k = spell_color(typ);

		/* Obtain attr/char */
		ret._attr = misc_to_attr[base+k];
		ret._char = misc_to_char[base+k];
	}
	else
	{
		int add;

		/* No motion (*) */
		if ((ny == y) && (nx == x)) {base = 0x00; add = 0;}

		/* Vertical (|) */
		else if (nx == x) {base = 0x40; add = 0;}

		/* Horizontal (-) */
		else if (ny == y) {base = 0x40; add = 1;}

		/* Diagonal (/) */
		else if ((ny-y) == (x-nx)) {base = 0x40; add = 2;}

		/* Diagonal (\) */
		else if ((ny-y) == (nx-x)) {base = 0x40; add = 3;}

		/* Weird (*) */
		else {base = 0x00; add = 0;}

		if (typ >= 0x40) k = 0;
		else k = typ;

		/* Obtain attr/char */
		ret._attr = misc_to_attr[base+k];
		ret._char = misc_to_char[base+k] + add;
	}

	/* Create pict */
	return ret;
}




/*
 * Decreases players hit points and sets death flag if necessary
 *
 * Invulnerability needs to be changed into a "shield" XXX XXX XXX
 *
 * Hack -- this function allows the user to save (or quit) the game
 * when he dies, since the "You die." message is shown before setting
 * the player to "dead".
 */
void take_hit(int dam, const char* kb_str)
{
	int old_chp = p_ptr->chp;
	int warning = (p_ptr->mhp * op_ptr->hitpoint_warn / 10);


	/* Paranoia */
	if (p_ptr->is_dead) return;


	/* Disturb */
	disturb(1, 0);

	/* Mega-Hack -- Apply "invulnerability" */
	if (p_ptr->timed[TMD_INVULN] && (dam < 9000)) return;

	/* Hurt the player */
	p_ptr->chp -= dam;

	/* Display the hitpoints */
	p_ptr->redraw |= (PR_HP);

	/* Dead player */
	if (p_ptr->chp < 0)
	{
		/* Hack -- Note death */
		message(MSG_DEATH, "You die.");
		message_flush();

		/* Note cause of death */
		my_strcpy(p_ptr->died_from, kb_str, sizeof(p_ptr->died_from));

		/* No longer a winner */
		p_ptr->total_winner = FALSE;

		/* Note death */
		p_ptr->is_dead = TRUE;

		/* Leaving */
		p_ptr->leaving = TRUE;

		/* Dead */
		return;
	}

	/* Hitpoint warning */
	if (p_ptr->chp < warning)
	{
		/* Hack -- bell on first notice */
		if (old_chp > warning) bell("Low hitpoint warning!");

		/* Message */
		message(MSG_HITPOINT_WARN, "*** LOW HITPOINT WARNING! ***");
		message_flush();
	}
}





/*
 * Does a given class of objects (usually) hate acid?
 * Note that acid can either melt or corrode something.
 */
static bool hates_acid(const object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->obj_id.tval)
	{
		/* Wearable items */
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:	return TRUE;

		/* Staffs/Scrolls are wood/paper */
		case TV_STAFF:
		case TV_SCROLL:		return TRUE;

		/* Ouch */
		case TV_CHEST:		return TRUE;

		/* Junk is useless */
		case TV_SKELETON:
		case TV_BOTTLE:
		case TV_JUNK:		return TRUE;
	}

	return FALSE;
}


/*
 * Does a given object (usually) hate electricity?
 */
static bool hates_elec(const object_type *o_ptr)
{
	switch (o_ptr->obj_id.tval)
	{
		case TV_RING:
		case TV_WAND:
		case TV_ROD:	return TRUE;
	}

	return FALSE;
}


/*
 * Does a given object (usually) hate fire?
 * Hafted/Polearm weapons have wooden shafts.
 * Arrows/Bows are mostly wooden.
 */
static bool hates_fire(const object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->obj_id.tval)
	{
		/* Wearable */
		case TV_LITE:
		case TV_ARROW:
		case TV_BOW:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:		return TRUE;

		/* Books */
		case TV_MAGIC_BOOK:
		case TV_PRAYER_BOOK:	return TRUE;

		/* Chests */
		case TV_CHEST:			return TRUE;

		/* Staffs/Scrolls burn */
		case TV_STAFF:
		case TV_SCROLL:			return TRUE;
	}

	return FALSE;
}


/*
 * Does a given object (usually) hate cold?
 */
static bool hates_cold(const object_type *o_ptr)
{
	switch (o_ptr->obj_id.tval)
	{
		case TV_POTION:
		case TV_FLASK:
		case TV_BOTTLE:	return TRUE;
	}

	return FALSE;
}









/*
 * Melt something
 */
bool set_acid_destroy(const object_type *o_ptr)
{
	u32b f[OBJECT_FLAG_STRICT_UB];
	if (!hates_acid(o_ptr)) return (FALSE);
	object_flags(o_ptr, f);
	if (f[2] & (TR3_IGNORE_ACID)) return (FALSE);
	return (TRUE);
}


/*
 * Electrical damage
 */
bool set_elec_destroy(const object_type *o_ptr)
{
	u32b f[OBJECT_FLAG_STRICT_UB];
	if (!hates_elec(o_ptr)) return (FALSE);
	object_flags(o_ptr, f);
	if (f[2] & (TR3_IGNORE_ELEC)) return (FALSE);
	return (TRUE);
}


/*
 * Burn something
 */
bool set_fire_destroy(const object_type *o_ptr)
{
	u32b f[OBJECT_FLAG_STRICT_UB];
	if (!hates_fire(o_ptr)) return (FALSE);
	object_flags(o_ptr, f);
	if (f[2] & (TR3_IGNORE_FIRE)) return (FALSE);
	return (TRUE);
}


/*
 * Freeze things
 */
bool set_cold_destroy(const object_type *o_ptr)
{
	u32b f[OBJECT_FLAG_STRICT_UB];
	if (!hates_cold(o_ptr)) return (FALSE);
	object_flags(o_ptr, f);
	if (f[2] & (TR3_IGNORE_COLD)) return (FALSE);
	return (TRUE);
}




/*
 * Destroys a type of item on a given percent chance
 * Note that missiles are no longer necessarily all destroyed
 *
 * Returns number of items destroyed.
 */
static int inven_damage(o_ptr_test* typ, int perc)
{
	int i, j, amt;
	int k = 0;			/* Count the casualties */

	char o_name[80];

	assert(0 <= p_ptr->inven_cnt && INVEN_PACK >= p_ptr->inven_cnt && "precondition");
	assert(p_ptr->inven_cnt_is_strict_UB_of_nonzero_k_idx() && "precondition");
	assert(NULL != typ && "precondition");

	/* Scan through the slots backwards */
	for (i = 0; i < p_ptr->inven_cnt; ++i)
	{
		object_type* const o_ptr = &p_ptr->inventory[i];

		/* Hack -- for now, skip artifacts */
		if (o_ptr->is_artifact()) continue;

		/* Give this item slot a shot at death */
		if ((*typ)(o_ptr))
		{
			/* Scale the destruction chance up */
			int chance = perc * 100;

			/* Rods are tough */
			if (o_ptr->obj_id.tval == TV_ROD) chance /= 4;

			/* Count the casualties */
			for (amt = j = 0; j < o_ptr->number; ++j)
			{
				if (rand_int(10000) < chance) ++amt;
			}

			/* Some casualities */
			if (amt)
			{
				/* Get a description */
				object_desc(o_name, sizeof(o_name), o_ptr, FALSE, ODESC_FULL);

				/* Message */
				message_format(MSG_DESTROY, 0, "%sour %s (%c) %s destroyed!",
				           ((o_ptr->number > 1) ?
				            ((amt == o_ptr->number) ? "All of y" :
				             (amt > 1 ? "Some of y" : "One of y")) : "Y"),
				           o_name, index_to_label(i),
				           ((amt > 1) ? "were" : "was"));

				/* Hack -- If rods, wands, or staves are destroyed, the total
				 * maximum timeout or charges of the stack needs to be reduced,
				 * unless all the items are being destroyed. -LM-
				 */
				if (((o_ptr->obj_id.tval == TV_WAND) ||
				     (o_ptr->obj_id.tval == TV_STAFF) ||
				     (o_ptr->obj_id.tval == TV_ROD)) &&
				    (amt < o_ptr->number))
				{
					o_ptr->pval -= o_ptr->pval * amt / o_ptr->number;
				}

				/* Destroy "amt" items */
				inven_item_increase(i, -amt);
				inven_item_optimize(i);

				/* Count the casualties */
				k += amt;
			}
		}
	}

	/* Return the casualty count */
	return (k);
}




/*
 * Acid has hit the player, attempt to affect some armor.
 *
 * Note that the "base armor" of an object never changes.
 *
 * If any armor is damaged (or resists), the player takes less damage.
 */
static int minus_ac(void)
{
	u32b f[OBJECT_FLAG_STRICT_UB];
	char o_name[80];
	object_type* const o_ptr = &p_ptr->inventory[INVEN_BODY+rand_int(6)];

	/* reality checks for above */
	ZAIBAND_STATIC_ASSERT(INVEN_OUTER==INVEN_BODY+1);
	ZAIBAND_STATIC_ASSERT(INVEN_ARM==INVEN_BODY+2);
	ZAIBAND_STATIC_ASSERT(INVEN_HEAD==INVEN_BODY+3);
	ZAIBAND_STATIC_ASSERT(INVEN_HANDS==INVEN_BODY+4);
	ZAIBAND_STATIC_ASSERT(INVEN_FEET==INVEN_BODY+5);

	/* Nothing to damage */
	if (!o_ptr->k_idx) return (FALSE);

	/* No damage left to be done */
	if (o_ptr->ac + o_ptr->to_a <= 0) return (FALSE);


	/* Describe */
	object_desc(o_name, sizeof(o_name), o_ptr, FALSE, ODESC_BASE);

	/* Extract the flags */
	object_flags(o_ptr, f);

	/* Object resists */
	if (f[2] & (TR3_IGNORE_ACID))
	{
		msg_format("Your %s is unaffected!", o_name);

		return (TRUE);
	}

	/* Message */
	msg_format("Your %s is damaged!", o_name);

	/* Damage the item */
	o_ptr->to_a--;

	/* Calculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->redraw |= (PR_EQUIP);

	/* Item was damaged */
	return (TRUE);
}


/*
 * Hurt the player with Acid
 */
void acid_dam(int dam, const char* kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total Immunity */
	if (p_ptr->immune_acid || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->resist_acid) dam = DR<3>(dam);
	if (p_ptr->timed[TMD_OPP_ACID]) dam = DR<3>(dam);

	/* If any armor gets hit, defend the player */
	if (minus_ac()) dam = DR<2>(dam);

	/* Take damage */
	take_hit(dam, kb_str);

	/* Inventory damage */
	inven_damage(set_acid_destroy, inv);
}


/*
 * Hurt the player with electricity
 */
void elec_dam(int dam, const char* kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total immunity */
	if (p_ptr->immune_elec || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->timed[TMD_OPP_ELEC]) dam = DR<3>(dam);
	if (p_ptr->resist_elec) dam = DR<3>(dam);

	/* Take damage */
	take_hit(dam, kb_str);

	/* Inventory damage */
	inven_damage(set_elec_destroy, inv);
}




/*
 * Hurt the player with Fire
 */
void fire_dam(int dam, const char* kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Totally immune */
	if (p_ptr->immune_fire || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->resist_fire) dam = DR<3>(dam);
	if (p_ptr->timed[TMD_OPP_FIRE]) dam = DR<3>(dam);

	/* Take damage */
	take_hit(dam, kb_str);

	/* Inventory damage */
	inven_damage(set_fire_destroy, inv);
}


/*
 * Hurt the player with Cold
 */
void cold_dam(int dam, const char* kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total immunity */
	if (p_ptr->immune_cold || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->resist_cold) dam = DR<3>(dam);
	if (p_ptr->timed[TMD_OPP_COLD]) dam = DR<3>(dam);

	/* Take damage */
	take_hit(dam, kb_str);

	/* Inventory damage */
	inven_damage(set_cold_destroy, inv);
}





/*
 * Increase a stat by one randomized level
 *
 * Most code will "restore" a stat before calling this function,
 * in particular, stat potions will always restore the stat and
 * then increase the fully restored value.
 */
bool inc_stat(stat_index stat)
{
	int value, gain;

	/* Then augment the current/max stat */
	value = p_ptr->stat_cur[stat];

	/* Cannot go above 18/100 */
	if (value < 18+100)
	{
		/* Gain one (sometimes two) points */
		if (value < 18)
		{
			gain = (one_in_(4) ? 2 : 1);
			value += gain;
		}

		/* Gain 1/6 to 1/3 of distance to 18/100 */
		else if (value < 18+98)
		{
			/* Approximate gain value */
			gain = (((18+100) - value) / 2 + 3) / 2;

			/* Paranoia */
			if (gain < 1) gain = 1;

			/* Apply the bonus */
			value += randint(gain) + gain / 2;

			/* Maximal value */
			if (value > 18+99) value = 18 + 99;
		}

		/* Gain one point at a time */
		else
		{
			value++;
		}

		/* Save the new value */
		p_ptr->stat_cur[stat] = value;

		/* Bring up the maximum too */
		if (value > p_ptr->stat_max[stat])
		{
			p_ptr->stat_max[stat] = value;
		}

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Success */
		return (TRUE);
	}

	/* Nothing to gain */
	return (FALSE);
}



/*
 * Decreases a stat by an amount indended to vary from 0 to 100 percent.
 *
 * Note that "permanent" means that the *given* amount is permanent,
 * not that the new value becomes permanent.  This may not work exactly
 * as expected, due to "weirdness" in the algorithm, but in general,
 * if your stat is already drained, the "max" value will not drop all
 * the way down to the "cur" value.
 */
bool dec_stat(stat_index stat, int amount, bool permanent)
{
	int cur, max, loss, same, res = FALSE;


	/* Get the current value */
	cur = p_ptr->stat_cur[stat];
	max = p_ptr->stat_max[stat];

	/* Note when the values are identical */
	same = (cur == max);

	/* Damage "current" value */
	if (cur > 3)
	{
		/* Handle "low" values */
		if (cur <= 18)
		{
			if (amount > 90) cur--;
			if (amount > 50) cur--;
			if (amount > 20) cur--;
			cur--;
		}

		/* Handle "high" values */
		else
		{
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((cur-18) / 2 + 1) / 2 + 1);

			/* Paranoia */
			if (loss < 1) loss = 1;

			/* Randomize the loss */
			loss = ((randint(loss) + loss) * amount) / 100;

			/* Maximal loss */
			if (loss < amount/2) loss = amount/2;

			/* Lose some points */
			cur = cur - loss;

			/* Hack -- Only reduce stat to 17 sometimes */
			if (cur < 18) cur = (amount <= 20) ? 18 : 17;
		}

		/* Prevent illegal values */
		if (cur < 3) cur = 3;

		/* Something happened */
		if (cur != p_ptr->stat_cur[stat]) res = TRUE;
	}

	/* Damage "max" value */
	if (permanent && (max > 3))
	{
		/* Handle "low" values */
		if (max <= 18)
		{
			if (amount > 90) max--;
			if (amount > 50) max--;
			if (amount > 20) max--;
			max--;
		}

		/* Handle "high" values */
		else
		{
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((max-18) / 2 + 1) / 2 + 1);
			if (loss < 1) loss = 1;
			loss = ((randint(loss) + loss) * amount) / 100;
			if (loss < amount/2) loss = amount/2;

			/* Lose some points */
			max = max - loss;

			/* Hack -- Only reduce stat to 17 sometimes */
			if (max < 18) max = (amount <= 20) ? 18 : 17;
		}

		/* Hack -- keep it clean */
		if (same || (max < cur)) max = cur;

		/* Something happened */
		if (max != p_ptr->stat_max[stat]) res = TRUE;
	}

	/* Apply changes */
	if (res)
	{
		/* Actually set the stat to its new value. */
		p_ptr->stat_cur[stat] = cur;
		p_ptr->stat_max[stat] = max;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);
	}

	/* Done */
	return (res);
}


/*
 * Restore a stat.  Return TRUE only if this actually makes a difference.
 */
bool res_stat(stat_index stat)
{
	/* Restore if needed */
	if (p_ptr->stat_cur[stat] != p_ptr->stat_max[stat])
	{
		/* Restore */
		p_ptr->stat_cur[stat] = p_ptr->stat_max[stat];

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Success */
		return (TRUE);
	}

	/* Nothing to restore */
	return (FALSE);
}




/*
 * Apply disenchantment to the player's stuff
 *
 * This function is also called from the "melee" code.
 *
 * The "mode" is currently unused.
 *
 * Return "TRUE" if the player notices anything.
 */
bool apply_disenchant(int mode)
{
	static const byte disenchant_slots[8]
		 = {	INVEN_WIELD,	INVEN_BOW,	INVEN_BODY,		INVEN_OUTER,
				INVEN_ARM,		INVEN_HEAD,	INVEN_HANDS,	INVEN_FEET
				};

	char o_name[80];
	int t = 0;

	/* Get the item */
	object_type* const o_ptr = &p_ptr->inventory[disenchant_slots[rand_int(N_ELEMENTS(disenchant_slots))]];

	/* Unused parameter */
	(void)mode;

	/* No item, nothing happens */
	if (!o_ptr->k_idx) return (FALSE);


	/* Nothing to disenchant */
	if ((o_ptr->to_h <= 0) && (o_ptr->to_d <= 0) && (o_ptr->to_a <= 0))
	{
		/* Nothing to notice */
		return (FALSE);
	}


	/* Describe the object */
	object_desc(o_name, sizeof(o_name), o_ptr, FALSE, ODESC_BASE);


	/* Artifacts have 60% chance to resist */
	if (o_ptr->is_artifact() && (rand_int(100) < 60))
	{
		/* Message */
		msg_format("Your %s (%c) resist%s disenchantment!",
		           o_name, index_to_label(t),
		           ((o_ptr->number != 1) ? "" : "s"));

		/* Notice */
		return (TRUE);
	}


	/* Disenchant tohit */
	if (o_ptr->to_h > 0) o_ptr->to_h--;
	if ((o_ptr->to_h > 5) && one_in_(5)) o_ptr->to_h--;

	/* Disenchant todam */
	if (o_ptr->to_d > 0) o_ptr->to_d--;
	if ((o_ptr->to_d > 5) && one_in_(5)) o_ptr->to_d--;

	/* Disenchant toac */
	if (o_ptr->to_a > 0) o_ptr->to_a--;
	if ((o_ptr->to_a > 5) && one_in_(5)) o_ptr->to_a--;

	/* Message */
	msg_format("Your %s (%c) %s disenchanted!",
	           o_name, index_to_label(t),
	           ((o_ptr->number != 1) ? "were" : "was"));

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->redraw |= (PR_EQUIP);

	/* Notice */
	return (TRUE);
}


/*
 * Apply Nexus
 */
static void apply_nexus(coord loc)
{
	int max1, cur1, max2, cur2, ii, jj;

	switch (randint(7))
	{
		case 1: case 2: case 3:
		{
			teleport_player(200);
			break;
		}

		case 4: case 5:
		{
			teleport_player_to(loc);
			break;
		}

		case 6:
		{
			if (p_ptr->std_save())
			{
				msg_print("You resist the effects!");
				break;
			}

			/* Teleport Level */
			teleport_player_level();
			break;
		}

		case 7:
		{
			if (p_ptr->std_save())
			{
				msg_print("You resist the effects!");
				break;
			}

			msg_print("Your body starts to scramble...");

			/* Pick a pair of stats */
			ii = rand_int(A_MAX);
			jj = rand_int(A_MAX-1);
			if (ii<=jj) ++jj;

			max1 = p_ptr->stat_max[ii];
			cur1 = p_ptr->stat_cur[ii];
			max2 = p_ptr->stat_max[jj];
			cur2 = p_ptr->stat_cur[jj];

			p_ptr->stat_max[ii] = max2;
			p_ptr->stat_cur[ii] = cur2;
			p_ptr->stat_max[jj] = max1;
			p_ptr->stat_cur[jj] = cur1;

			p_ptr->update |= (PU_BONUS);

			break;
		}
	}
}





/*
 * Mega-Hack -- track "affected" monsters (see "project()" comments)
 */
static int project_m_n;
static coord project_m_g;


/*
 * We are called from "project()" to "damage" terrain features
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 *
 * Hack -- We also "see" grids which are "memorized".
 *
 * Perhaps we should affect doors and/or walls.
 */
static bool project_f(int who, int r, coord g, int dam, int typ)
{
	bool obvious = FALSE;

	/* Unused parameters */
	(void)who;
	(void)r;
	(void)dam;

#if 0 /* unused */
	/* Reduce damage by distance */
	dam = (dam + r) / (r + 1);
#endif /* 0 */

	/* Analyze the type */
	switch (typ)
	{
		/* Ignore most effects */
		case GF_ACID:
		case GF_ELEC:
		case GF_FIRE:
		case GF_COLD:
		case GF_PLASMA:
		case GF_METEOR:
		case GF_ICE:
		case GF_SHARD:
		case GF_FORCE:
		case GF_SOUND:
		case GF_MANA:
		case GF_HOLY_ORB:
		{
			break;
		}

		/* Destroy Traps (and Locks) */
		case GF_KILL_TRAP:
		{
			/* Reveal secret doors */
			if (cave_feat[g.y][g.x] == FEAT_SECRET)
			{
				place_closed_door(g.y, g.x);

				/* Check line of sight */
				if (player_has_los_bold(g.y, g.x))
				{
					obvious = TRUE;
				}
			}

			/* Destroy traps */
			if ((cave_feat[g.y][g.x] == FEAT_INVIS) ||
			    cave_feat_in_range(g.y,g.x,FEAT_TRAP_HEAD,FEAT_TRAP_TAIL))
			{
				/* Check line of sight */
				if (player_has_los_bold(g.y, g.x))
				{
					msg_print("There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Forget the trap */
				cave_info[g.y][g.x] &= ~(CAVE_MARK);

				/* Destroy the trap */
				cave_set_feat(g.y, g.x, FEAT_FLOOR);
			}

			/* Locked doors are unlocked */
			else if ((cave_feat[g.y][g.x] >= FEAT_DOOR_HEAD + 0x01) &&
			          (cave_feat[g.y][g.x] <= FEAT_DOOR_HEAD + 0x07))
			{
				/* Unlock the door */
				cave_set_feat(g.y, g.x, FEAT_DOOR_HEAD + 0x00);

				/* Check line of sound */
				if (player_has_los_bold(g.y, g.x))
				{
					msg_print("Click!");
					obvious = TRUE;
				}
			}

			break;
		}

		/* Destroy Doors (and traps) */
		case GF_KILL_DOOR:
		{
			/* Destroy all doors and traps */
			if ((cave_feat[g.y][g.x] == FEAT_OPEN) ||
			    (cave_feat[g.y][g.x] == FEAT_BROKEN) ||
			    (cave_feat[g.y][g.x] == FEAT_INVIS) ||
			    cave_feat_in_range(g.y,g.x,FEAT_TRAP_HEAD,FEAT_TRAP_TAIL) ||
			    cave_feat_in_range(g.y,g.x,FEAT_DOOR_HEAD,FEAT_DOOR_TAIL))
			{
				/* Check line of sight */
				if (player_has_los_bold(g.y, g.x))
				{
					/* Message */
					msg_print("There is a bright flash of light!");
					obvious = TRUE;

					/* Visibility change */
					if (cave_feat_in_range(g.y,g.x,FEAT_DOOR_HEAD,FEAT_DOOR_TAIL))
					{
						/* Update the visuals */
						p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
					}
				}

				/* Forget the door */
				cave_info[g.y][g.x] &= ~(CAVE_MARK);

				/* Destroy the feature */
				cave_set_feat(g.y, g.x, FEAT_FLOOR);
			}

			break;
		}

		/* Destroy walls (and doors) */
		case GF_KILL_WALL:
		{
			/* Non-walls (etc) */
			if (cave_floor_bold(g.y, g.x)) break;

			/* Permanent walls */
			if (cave_feat[g.y][g.x] >= FEAT_PERM_EXTRA) break;

			/* Granite */
			if (cave_feat[g.y][g.x] >= FEAT_WALL_EXTRA)
			{
				/* Message */
				if (cave_info[g.y][g.x] & (CAVE_MARK))
				{
					msg_print("The wall turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				cave_info[g.y][g.x] &= ~(CAVE_MARK);

				/* Destroy the wall */
				cave_set_feat(g.y, g.x, FEAT_FLOOR);
			}

			/* Quartz / Magma with treasure */
			else if (cave_feat[g.y][g.x] >= FEAT_MAGMA_H)
			{
				/* Message */
				if (cave_info[g.y][g.x] & (CAVE_MARK))
				{
					msg_print("The vein turns into mud!");
					msg_print("You have found something!");
					obvious = TRUE;
				}

				/* Forget the wall */
				cave_info[g.y][g.x] &= ~(CAVE_MARK);

				/* Destroy the wall */
				cave_set_feat(g.y, g.x, FEAT_FLOOR);

				/* Place some gold */
				place_gold(g.y, g.x, p_ptr->depth);
			}

			/* Quartz / Magma */
			else if (cave_feat[g.y][g.x] >= FEAT_MAGMA)
			{
				/* Message */
				if (cave_info[g.y][g.x] & (CAVE_MARK))
				{
					msg_print("The vein turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				cave_info[g.y][g.x] &= ~(CAVE_MARK);

				/* Destroy the wall */
				cave_set_feat(g.y, g.x, FEAT_FLOOR);
			}

			/* Rubble */
			else if (cave_feat[g.y][g.x] == FEAT_RUBBLE)
			{
				/* Message */
				if (cave_info[g.y][g.x] & (CAVE_MARK))
				{
					msg_print("The rubble turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				cave_info[g.y][g.x] &= ~(CAVE_MARK);

				/* Destroy the rubble */
				cave_set_feat(g.y, g.x, FEAT_FLOOR);

				/* Hack -- place an object */
				if (one_in_(10))
				{
					/* Found something */
					if (player_can_see_bold(g.y, g.x))
					{
						msg_print("There was something buried in the rubble!");
						obvious = TRUE;
					}

					/* Place gold */
					place_object(g.y, g.x, FALSE, FALSE, p_ptr->depth);
				}
			}

			/* Destroy doors (and secret doors) */
			else /* if (cave_feat[g.y][g.x] >= FEAT_DOOR_HEAD) */
			{
				/* Hack -- special message */
				if (cave_info[g.y][g.x] & (CAVE_MARK))
				{
					msg_print("The door turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				cave_info[g.y][g.x] &= ~(CAVE_MARK);

				/* Destroy the feature */
				cave_set_feat(g.y, g.x, FEAT_FLOOR);
			}

			/* Update the visuals */
			p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

			/* Fully update the flow */
			p_ptr->update |= (PU_FORGET_FLOW | PU_UPDATE_FLOW);

			break;
		}

		/* Make doors */
		case GF_MAKE_DOOR:
		{
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(g.y, g.x)) break;

			/* Create closed door */
			cave_set_feat(g.y, g.x, FEAT_DOOR_HEAD + 0x00);

			/* Observe */
			if (cave_info[g.y][g.x] & (CAVE_MARK)) obvious = TRUE;

			/* Update the visuals */
			p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

			break;
		}

		/* Make traps */
		case GF_MAKE_TRAP:
		{
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(g.y, g.x)) break;

			/* Place a trap */
			place_trap(g.y, g.x);

			break;
		}

		/* Lite up the grid */
		case GF_LITE_WEAK:
		case GF_LITE:
		{
			/* Turn on the light */
			cave_info[g.y][g.x] |= (CAVE_GLOW);

			/* Grid is in line of sight */
			if (player_has_los_bold(g.y, g.x))
			{
				if (!p_ptr->timed[TMD_BLIND])
				{
					/* Observe */
					obvious = TRUE;
				}

				/* Fully update the visuals */
				p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);
			}

			break;
		}

		/* Darken the grid */
		case GF_DARK_WEAK:
		case GF_DARK:
		{
			/* Turn off the light */
			cave_info[g.y][g.x] &= ~(CAVE_GLOW);

			/* Hack -- Forget "boring" grids */
			if (cave_feat[g.y][g.x] <= FEAT_INVIS)
			{
				/* Forget */
				cave_info[g.y][g.x] &= ~(CAVE_MARK);
			}

			/* Grid is in line of sight */
			if (player_has_los_bold(g.y, g.x))
			{
				/* Observe */
				obvious = TRUE;

				/* Fully update the visuals */
				p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);
			}

			/* All done */
			break;
		}
	}

	/* Return "Anything seen?" */
	return (obvious);
}



/*
 * We are called from "project()" to "damage" objects
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * Perhaps we should only SOMETIMES damage things on the ground.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * Hack -- We also "see" objects which are "memorized".
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 */
static bool project_o(int who, int r, coord g, int dam, int typ)
{
	s16b this_o_idx, next_o_idx = 0;

	bool obvious = FALSE;

	u32b f[OBJECT_FLAG_STRICT_UB];

	char o_name[80];


	/* Unused parameters */
	(void)who;
	(void)r;
	(void)dam;

#if 0 /* unused */
	/* Reduce damage by distance */
	dam = (dam + r) / (r + 1);
#endif /* 0 */


	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[g.y][g.x]; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type* const o_ptr = &o_list[this_o_idx];	/* Get the object */

		bool is_art = o_ptr->is_artifact();	/* Check for artifact */
		bool ignore = FALSE;
		bool plural = (o_ptr->number > 1);	/* Get the "plural"-ness */
		bool do_kill = FALSE;

		const char* note_kill = NULL;

		/* Get the next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Extract the flags */
		object_flags(o_ptr, f);

		/* Analyze the type */
		switch (typ)
		{
			/* Acid -- Lots of things */
			case GF_ACID:
			{
				if (hates_acid(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " melt!" : " melts!");
					if (f[2] & (TR3_IGNORE_ACID)) ignore = TRUE;
				}
				break;
			}

			/* Elec -- Rings and Wands */
			case GF_ELEC:
			{
				if (hates_elec(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " are destroyed!" : " is destroyed!");
					if (f[2] & (TR3_IGNORE_ELEC)) ignore = TRUE;
				}
				break;
			}

			/* Fire -- Flammable objects */
			case GF_FIRE:
			{
				if (hates_fire(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " burn up!" : " burns up!");
					if (f[2] & (TR3_IGNORE_FIRE)) ignore = TRUE;
				}
				break;
			}

			/* Cold -- potions and flasks */
			case GF_COLD:
			{
				if (hates_cold(o_ptr))
				{
					note_kill = (plural ? " shatter!" : " shatters!");
					do_kill = TRUE;
					if (f[2] & (TR3_IGNORE_COLD)) ignore = TRUE;
				}
				break;
			}

			/* Fire + Elec */
			case GF_PLASMA:
			{
				if (hates_fire(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " burn up!" : " burns up!");
					if (f[2] & (TR3_IGNORE_FIRE)) ignore = TRUE;
				}
				if (hates_elec(o_ptr))
				{
					ignore = FALSE;
					do_kill = TRUE;
					note_kill = (plural ? " are destroyed!" : " is destroyed!");
					if (f[2] & (TR3_IGNORE_ELEC)) ignore = TRUE;
				}
				break;
			}

			/* Fire + Cold */
			case GF_METEOR:
			{
				if (hates_fire(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " burn up!" : " burns up!");
					if (f[2] & (TR3_IGNORE_FIRE)) ignore = TRUE;
				}
				if (hates_cold(o_ptr))
				{
					ignore = FALSE;
					do_kill = TRUE;
					note_kill = (plural ? " shatter!" : " shatters!");
					if (f[2] & (TR3_IGNORE_COLD)) ignore = TRUE;
				}
				break;
			}

			/* Hack -- break potions and such */
			case GF_ICE:
			case GF_SHARD:
			case GF_FORCE:
			case GF_SOUND:
			{
				if (hates_cold(o_ptr))
				{
					note_kill = (plural ? " shatter!" : " shatters!");
					do_kill = TRUE;
				}
				break;
			}

			/* Mana -- destroys everything */
			case GF_MANA:
			{
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				break;
			}

			/* Holy Orb -- destroys cursed non-artifacts */
			case GF_HOLY_ORB:
			{
				if (o_ptr->is_cursed())
				{
					do_kill = TRUE;
					note_kill = (plural ? " are destroyed!" : " is destroyed!");
				}
				break;
			}

			/* Unlock chests */
			case GF_KILL_TRAP:
			case GF_KILL_DOOR:
			{
				/* Chests are noticed only if trapped or locked */
				if (o_ptr->obj_id.tval == TV_CHEST)
				{
					/* Disarm/Unlock traps */
					if (o_ptr->pval > 0)
					{
						/* Disarm or Unlock */
						o_ptr->pval = (0 - o_ptr->pval);

						/* Identify */
						object_known(o_ptr);

						/* Notice */
						if (o_ptr->marked)
						{
							msg_print("Click!");
							obvious = TRUE;
						}
					}
				}

				break;
			}
		}


		/* Attempt to destroy the object */
		if (do_kill)
		{
			/* Effect "observed" */
			if (o_ptr->marked)
			{
				obvious = TRUE;
				object_desc(o_name, sizeof(o_name), o_ptr, FALSE, ODESC_BASE);
			}

			/* Artifacts, and other objects, get to resist */
			if (is_art || ignore)
			{
				/* Observe the resist */
				if (o_ptr->marked)
				{
					msg_format("The %s %s unaffected!",
					           o_name, (plural ? "are" : "is"));
				}
			}

			/* Kill it */
			else
			{
				/* Describe if needed */
				if (o_ptr->marked && note_kill)
				{
					message_format(MSG_DESTROY, 0, "The %s%s", o_name, note_kill);
				}

				/* object destruction is noisy */
				apply_noise(o_ptr->loc, 1);

				/* Delete the object */
				delete_object_idx(this_o_idx);

				/* Redraw */
				lite_spot(g);
			}
		}
	}

	/* Return "Anything seen?" */
	return (obvious);
}

bool monster_type::take_confusion(int base, int zero_to_N_sub_1)
{
	if (0>=base && 0>=zero_to_N_sub_1) return false;

	const monster_race* const r_ptr = race();

	/* Confusion and Chaos breathers (and sleepers) never confuse */
	if (r_ptr->flags[2] & RF2_NO_CONF)
		{
		if (ml) lore()->flags[2] |= RF2_NO_CONF;
		return false;
		}
	if (r_ptr->spell_flags[0] & (RSF0_BR_CONF | RSF0_BR_CHAO)) return false;
	return inc_core_timed<CORE_TMD_CONFUSED>(base+rand_int(zero_to_N_sub_1));
}

bool player_type::take_confusion(int base, int zero_to_N_sub_1)
{
	if (0>=base && 0>=zero_to_N_sub_1) return false;
	if (p_ptr->resist_confu) return false; 
	return inc_core_timed<CORE_TMD_CONFUSED>(base+rand_int(zero_to_N_sub_1));
	/* XXX should update non-cheat learning for all monsters that can see the player XXX */
}


/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to a monster.
 *
 * This routine takes a "source monster" (by index) which is mostly used to
 * determine if the player is causing the damage, and a "radius" (see below),
 * which is used to decrease the power of explosions with distance, and a
 * location, via integers which are modified by certain types of attacks
 * (polymorph and teleport being the obvious ones), a default damage, which
 * is modified as needed based on various properties, and finally a "damage
 * type" (see below).
 *
 * Note that this routine can handle "no damage" attacks (like teleport) by
 * taking a "zero" damage, and can even take "parameters" to attacks (like
 * confuse) by accepting a "damage", using it to calculate the effect, and
 * then setting the damage to zero.  Note that the "damage" parameter is
 * divided by the radius, so monsters not at the "epicenter" will not take
 * as much damage (or whatever)...
 *
 * Note that "polymorph" is dangerous, since a failure in "place_monster()"'
 * may result in a dereference of an invalid pointer.  XXX XXX XXX
 *
 * Various messages are produced, and damage is applied.
 *
 * Just "casting" a substance (i.e. plasma) does not make you immune, you must
 * actually be "made" of that substance, or "breathe" big balls of it.
 *
 * We assume that "Plasma" monsters, and "Plasma" breathers, are immune
 * to plasma.
 *
 * We assume "Nether" is an evil, necromantic force, so it doesn't hurt undead,
 * and hurts evil less.  If can breath nether, then it resists it as well.
 *
 * Damage reductions use the following formulas:
 *   Note that "dam = dam * 6 / (randint(6) + 6);"
 *     gives avg damage of .655, ranging from .858 to .500
 *   Note that "dam = dam * 5 / (randint(6) + 6);"
 *     gives avg damage of .544, ranging from .714 to .417
 *   Note that "dam = dam * 4 / (randint(6) + 6);"
 *     gives avg damage of .444, ranging from .556 to .333
 *   Note that "dam = dam * 3 / (randint(6) + 6);"
 *     gives avg damage of .327, ranging from .427 to .250
 *   Note that "dam = dam * 2 / (randint(6) + 6);"
 *     gives something simple.
 *
 * In this function, "result" messages are postponed until the end, where
 * the "note" string is appended to the monster name, if not NULL.  So,
 * to make a spell have "no effect" just set "note" to NULL.  You should
 * also set "notice" to FALSE, or the player will learn what the spell does.
 *
 * We attempt to return "TRUE" if the player saw anything "useful" happen.
 */
static bool project_m(int who, int r, coord g, int dam, int typ)
{
	int tmp;

	int do_dist = 0;		/* Teleport setting (max distance) */
	int do_conf = 0;		/* Confusion setting (amount to confuse) */
	int do_stun = 0;		/* Stunning setting (amount to stun) */
	int do_sleep = 0;		/* Sleep amount (amount to sleep) */
	int do_fear = 0;		/* Fear amount (amount to fear) */
	char m_name[80];		/* Hold the monster name */
	const char* note = NULL;			/* Assume no note */
	const char* note_dies = " dies.";	/* Assume a default death */
	bool obvious = FALSE;	/* Were the effects "obvious" (if seen)? */
	bool do_poly = FALSE;	/* Polymorph setting (true or false) */

	if (!cave_floor_bold(g.y,g.x)) return FALSE;	/* Walls protect monsters */
	if (0 >= cave_m_idx[g.y][g.x]) return FALSE;	/* No monster here */
	if (cave_m_idx[g.y][g.x] == who) return FALSE;	/* Never affect projector */

	/* Obtain monster info */
	{	/* C-ish blocking brace */
	monster_type* m_ptr = &mon_list[cave_m_idx[g.y][g.x]];
	const monster_race* r_ptr = m_ptr->race();
	monster_lore* const l_ptr = m_ptr->lore();
	const bool seen = m_ptr->ml;			/* Is the monster "seen"? */
	obvious = seen;							/* Usually obvious if seen */

	dam = (dam + r) / (r + 1);							/* Reduce damage by distance */

	/* Get the monster name (BEFORE polymorphing) */
	monster_desc(m_name, sizeof(m_name), m_ptr, 0);

	/* Some monsters get "destroyed" */
	if (r_ptr->is_nonliving()) note_dies = " is destroyed.";


	/* Analyze the damage type */
	switch (typ)
	{
		case GF_MISSILE:	/* Magic Missile -- pure damage */
		case GF_ARROW:		/* Arrow -- no defense XXX */
		case GF_MANA:		/* Pure damage */
		case GF_METEOR:		/* Meteor -- powerful magic missile */
		{
			break;
		}

		case GF_ACID:		/* Acid */
		{
			if (r_ptr->flags[2] & RF2_IM_ACID)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen) l_ptr->flags[2] |= RF2_IM_ACID;
			}
			break;
		}

		case GF_ELEC:		/* Electricity */
		{
			if (r_ptr->flags[2] & RF2_IM_ELEC)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen) l_ptr->flags[2] |= RF2_IM_ELEC;
			}
			break;
		}

		case GF_FIRE:		/* Fire damage */
		{
			if (r_ptr->flags[2] & RF2_IM_FIRE)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen) l_ptr->flags[2] |= RF2_IM_FIRE;
			}
			break;
		}

		case GF_COLD:		/* Cold */
		{
			if (r_ptr->flags[2] & RF2_IM_COLD)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen) l_ptr->flags[2] |= RF2_IM_COLD;
			}
			break;
		}

		case GF_POIS:		/* Poison */
		{
			if (r_ptr->flags[2] & RF2_IM_POIS)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen) l_ptr->flags[2] |= RF2_IM_POIS;
			}
			break;
		}

		case GF_HOLY_ORB:	/* Holy Orb -- hurts Evil */
		{
			if (r_ptr->flags[2] & RF2_EVIL)
			{
				dam *= 2;
				note = " is hit hard.";
				if (seen) l_ptr->flags[2] |= RF2_EVIL;
			}
			break;
		}

		case GF_PLASMA:		/* Plasma */
		{
			if (r_ptr->flags[2] & RF2_RES_PLAS)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
				if (seen) l_ptr->flags[2] |= RF2_RES_PLAS;
			}
			break;
		}

		case GF_NETHER:		/* Nether -- see above */
		{
			if (r_ptr->flags[2] & RF2_UNDEAD)
			{
				note = " is immune.";
				dam = 0;
				if (seen) l_ptr->flags[2] |= RF2_UNDEAD;
			}
			else if (r_ptr->spell_flags[0] & RSF0_BR_NETH)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			else if (r_ptr->flags[2] & RF2_EVIL)
			{
				dam /= 2;
				note = " resists somewhat.";
				if (seen) l_ptr->flags[2] |= RF2_EVIL;
			}
			break;
		}

		case GF_WATER:		/* Water damage */
		{
			if (r_ptr->flags[2] & RF2_IM_WATER)
			{
				note = " is immune.";
				dam = 0;
				if (seen) l_ptr->flags[2] |= RF2_IM_WATER;
			}
			break;
		}

		case GF_CHAOS:		/* Chaos -- Chaos breathers resist */
		{
			do_poly = TRUE;
			do_conf = (5 + randint(11) + r) / (r + 1);
			if (r_ptr->spell_flags[0] & RSF0_BR_CHAO)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
				do_poly = FALSE;
			}
			break;
		}

		case GF_SHARD:		/* Shards -- Shard breathers resist */
		{
			if (r_ptr->spell_flags[0] & RSF0_BR_SHAR)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

		case GF_SOUND:		/* Sound -- Sound breathers resist */
		{
			do_stun = (10 + randint(15) + r) / (r + 1);
			if (r_ptr->spell_flags[0] & RSF0_BR_SOUN)
			{
				note = " resists.";
				dam *= 2; dam /= (randint(6)+6);
			}
			break;
		}

		case GF_CONFUSION:	/* Confusion */
		{
			do_conf = (10 + randint(15) + r) / (r + 1);
			if (r_ptr->spell_flags[0] & RSF0_BR_CONF)
			{
				note = " resists.";
				dam *= 2; dam /= (randint(6)+6);
			}
			else if (r_ptr->flags[2] & RF2_NO_CONF)
			{
				note = " resists somewhat.";
				dam /= 2;
			}
			break;
		}

		case GF_DISENCHANT:	/* Disenchantment */
		{
			if (r_ptr->spell_flags[0] & RF2_RES_DISE)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
				if (seen) l_ptr->flags[2] |= RF2_RES_DISE;
			}
			break;
		}

		case GF_NEXUS:		/* Nexus */
		{
			if (r_ptr->spell_flags[0] & RF2_RES_NEXUS)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
				if (seen) l_ptr->flags[2] |= RF2_RES_NEXUS;
			}
			break;
		}

		case GF_FORCE:		/* Force */
		{
			do_stun = (randint(15) + r) / (r + 1);
			if (r_ptr->spell_flags[0] & RSF0_BR_WALL)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

		case GF_INERTIA:	/* Inertia -- breathers resist */
		{
			if (r_ptr->spell_flags[0] & RSF0_BR_INER)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

		case GF_TIME:		/* Time -- breathers resist */
		{
			if (r_ptr->spell_flags[0] & RSF0_BR_TIME)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

		case GF_GRAVITY:	/* Gravity -- breathers resist */
		{
			/* Higher level monsters can resist the teleportation better */
			if (randint(127) > r_ptr->level)
				do_dist = 10;

			if (r_ptr->spell_flags[0] & RSF0_BR_GRAV)
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
				do_dist = 0;
			}
			break;
		}

		case GF_ICE:		/* Ice -- Cold + Cuts + Stun */
		{
			do_stun = (randint(15) + 1) / (r + 1);
			if (r_ptr->flags[2] & RF2_IM_COLD)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen) l_ptr->flags[2] |= RF2_IM_COLD;
			}
			break;
		}


		case GF_OLD_DRAIN:	/* Drain Life */
		{
			if ((r_ptr->flags[2] & (RF2_UNDEAD | RF2_DEMON)) ||
			    strchr("Egv", r_ptr->d._char))
			{
				if (seen) l_ptr->flags[2] |= (r_ptr->flags[2] & (RF2_UNDEAD | RF2_DEMON));

				note = " is unaffected!";
				obvious = FALSE;
				dam = 0;
			}

			break;
		}

		case GF_OLD_POLY:	/* Polymorph monster (Use "dam" as "power") */
		{
			do_poly = TRUE;	/* Attempt to polymorph (see below) */

			/* Powerful monsters can resist */
			if ((r_ptr->flags[0] & RF0_UNIQUE) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = " is unaffected!";
				do_poly = FALSE;
				obvious = FALSE;
			}

			dam = 0;	/* No "real" damage */
			break;
		}

		case GF_OLD_CLONE:	/* Clone monsters (Ignore "dam") */
		{
			m_ptr->chp = m_ptr->mhp;					/* Heal fully */
			if (!m_ptr->core_timed[CORE_TMD_FAST])
			{
				m_ptr->inc_core_timed<CORE_TMD_FAST>(randint(25) + 15);
			}
			else
			{
				m_ptr->inc_core_timed<CORE_TMD_FAST>(5);
			}

			/* Attempt to clone. */
			if (multiply_monster(cave_m_idx[g.y][g.x])) note = " spawns!";

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_OLD_HEAL:	/* Heal Monster (use "dam" as amount of healing) */
		{
			m_ptr->csleep = 0;	/* Wake up */
			
			/* Heal */
			if ((m_ptr->mhp - m_ptr->chp)>=dam)
			{
				m_ptr->chp += dam;
			}
			else
			{
				m_ptr->chp = m_ptr->mhp;
			}

			/* Redraw (later) if needed */
			if (p_ptr->health_who == cave_m_idx[g.y][g.x]) p_ptr->redraw |= (PR_HEALTH);

			/* Message */
			note = " looks healthier.";

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_OLD_SPEED:	/* Speed Monster (Ignore "dam") */
		{
			if (!m_ptr->core_timed[CORE_TMD_FAST])
			{
				m_ptr->inc_core_timed<CORE_TMD_FAST>(randint(25) + 15);
			}
			else
			{
				m_ptr->inc_core_timed<CORE_TMD_FAST>(5);
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_OLD_SLOW:	/* Slow Monster (Use "dam" as "power") */
		{
			/* Powerful monsters can resist */
			if ((r_ptr->flags[0] & RF0_UNIQUE) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = " is unaffected!";
				obvious = FALSE;
			}

			/* Normal monsters slow down */
			else
			{
				(m_ptr->inc_core_timed<CORE_TMD_SLOW>(randint(25) + 15));
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_OLD_SLEEP:	/* Sleep (Use "dam" as "power") */
		{
			/* Attempt a saving throw */
			if ((r_ptr->flags[0] & RF0_UNIQUE) ||
			    (r_ptr->flags[2] & RF2_NO_SLEEP) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Memorize a flag */
				if (r_ptr->flags[2] & RF2_NO_SLEEP)
				{
					if (seen) l_ptr->flags[2] |= RF2_NO_SLEEP;
				}

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			else
			{
				/* Go to sleep (much) later */
				note = " falls asleep!";
				do_sleep = 500;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_OLD_CONF:	/* Confusion (Use "dam" as "power") */
		{
			/* Get confused later */
			do_conf = NdS(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags[0] & RF0_UNIQUE) ||
			    (r_ptr->flags[2] & RF2_NO_CONF) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Memorize a flag */
				if (r_ptr->flags[2] & RF2_NO_CONF)
				{
					if (seen) l_ptr->flags[2] |= RF2_NO_CONF;
				}

				/* Resist */
				do_conf = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_LITE_WEAK:	/* Lite, but only hurts susceptible creatures */
		{
			/* Hurt by light */
			if (r_ptr->flags[2] & RF2_HURT_LITE)
			{
				/* Memorize the effects */
				if (seen) l_ptr->flags[2] |= RF2_HURT_LITE;

				/* Special effect */
				note = " cringes from the light!";
				note_dies = " shrivels away in the light!";
			}

			/* Normally no damage */
			else
			{
				obvious = FALSE;	/* No obvious effect */
				dam = 0;			/* No damage */
			}

			break;
		}

		case GF_LITE:	/* Lite -- opposite of Dark */
		{
			if (r_ptr->spell_flags[0] & RSF0_BR_LITE)
			{
				note = " resists.";
				dam *= 2; dam /= (randint(6)+6);
			}
			else if (r_ptr->flags[2] & RF2_HURT_LITE)
			{
				if (seen) l_ptr->flags[2] |= RF2_HURT_LITE;
				note = " cringes from the light!";
				note_dies = " shrivels away in the light!";
				dam *= 2;
			}
			break;
		}

		case GF_DARK:	/* Dark -- opposite of Lite */
		{
			if (r_ptr->spell_flags[0] & RSF0_BR_DARK)
			{
				note = " resists.";
				dam *= 2; dam /= (randint(6)+6);
			}
			break;
		}

		case GF_KILL_WALL:	/* Stone to Mud */
		{
			/* Hurt by rock remover */
			if (r_ptr->flags[2] & RF2_HURT_ROCK)
			{
				/* Memorize the effects */
				if (seen) l_ptr->flags[2] |= RF2_HURT_ROCK;

				/* Cute little message */
				note = " loses some skin!";
				note_dies = " dissolves!";
			}

			/* Usually, ignore the effects */
			else
			{
				obvious = FALSE;	/* No obvious effect */
				dam = 0;			/* No damage */
			}

			break;
		}

		case GF_AWAY_UNDEAD:	/* Teleport undead (Use "dam" as "power") */
		{
			/* Only affect undead */
			if (!(r_ptr->flags[2] & RF2_UNDEAD)) return FALSE;

			if (seen) l_ptr->flags[2] |= RF2_UNDEAD;	/* Learn about type */

			do_dist = dam;

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_AWAY_EVIL:		/* Teleport evil (Use "dam" as "power") */
		{
			/* Only affect evil */
			if (!(r_ptr->flags[2] & RF2_EVIL)) return FALSE;

			if (seen) l_ptr->flags[2] |= RF2_EVIL;	/* Learn about type */

			do_dist = dam;

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_AWAY_ALL:		/* Teleport monster (Use "dam" as "power") */
		{
			/* Prepare to teleport */
			do_dist = dam;

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_TURN_UNDEAD:	/* Turn undead (Use "dam" as "power") */
		{
			/* Only affect undead */
			if (!(r_ptr->flags[2] & RF2_UNDEAD)) return FALSE;

			if (seen) l_ptr->flags[2] |= RF2_UNDEAD;	/* Learn about type */

			/* Apply some fear */
			do_fear = NdS(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
			{
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
				do_fear = 0;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_TURN_EVIL:		/* Turn evil (Use "dam" as "power") */
		{
			/* Only affect evil */
			if (!(r_ptr->flags[2] & RF2_EVIL)) return FALSE;

			if (seen) l_ptr->flags[2] |= RF2_EVIL;	/* Learn about type */

			/* Apply some fear */
			do_fear = NdS(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
			{
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
				do_fear = 0;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_TURN_ALL:		/* Turn monster (Use "dam" as "power") */
		{
			/* Apply some fear */
			do_fear = NdS(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags[0] & RF0_UNIQUE) ||
			    (r_ptr->flags[2] & RF2_NO_FEAR) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
				do_fear = 0;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_DISP_UNDEAD:	/* Dispel undead */
		{
			/* Only affect undead */
			if (!(r_ptr->flags[2] & RF2_UNDEAD)) return FALSE;

			if (seen) l_ptr->flags[2] |= RF2_UNDEAD;	/* Learn about type */

			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";

			break;
		}

		case GF_DISP_EVIL:		/* Dispel evil */
		{
			/* Only affect evil */
			if (!(r_ptr->flags[2] & RF2_EVIL)) return FALSE;

			if (seen) l_ptr->flags[2] |= RF2_EVIL;	/* Learn about type */

			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";

			break;
		}

		case GF_DISP_ALL:		/* Dispel monster */
		{
			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";

			break;
		}


		/* Default: Irrelevant */
		default:	return FALSE;
	}


	/* Unique monsters cannot be polymorphed */
	if (r_ptr->flags[0] & RF0_UNIQUE) do_poly = FALSE;


	/* Uniques may only be killed by the player */
	if (r_ptr->flags[0] & RF0_UNIQUE)
	{
		if ((0 < who) && (dam > m_ptr->chp)) dam = m_ptr->chp;
	}


	/* Check for death */
	if (dam > m_ptr->chp)
	{
		note = note_dies;	/* Extract method of death */
	}

	/* Mega-Hack -- Handle "polymorph" -- monsters get a saving throw */
	else if (do_poly && (randint(90) > r_ptr->level))
	{
		/* Default -- assume no polymorph */
		note = " is unaffected!";

		/* Pick a "new" monster race */
		tmp = poly_r_idx(m_ptr->r_idx);

		/* Handle polymorph */
		if (tmp != m_ptr->r_idx)
		{
			if (seen) obvious = TRUE;	/* Obvious (irrational caution) */
			note = " changes!";			/* Monster polymorphs */
			dam = 0;					/* Turn off the damage */

			/* "Kill" the "old" monster */
			delete_monster_idx(cave_m_idx[g.y][g.x]);

			/* Create a new monster (no groups) */
			/* since we just freed a slot, there should be space for this monster */
			(void)place_monster_aux(g, tmp, FALSE, FALSE);

			m_ptr = &mon_list[cave_m_idx[g.y][g.x]];	/* Get new monster */
			r_ptr = m_ptr->race();						/* Get new race */
		}
	}

	/* Handle "teleport" */
	else if (do_dist)
	{
		if (seen) obvious = TRUE;			/* Obvious (irrational caution) */
		note = " disappears!";							/* Message */
		teleport_away(cave_m_idx[g.y][g.x], do_dist);	/* Teleport */
		g = m_ptr->loc;									/* get new location */
	}

	/* Sound and Impact breathers never stun */
	else if (do_stun &&
	         !(r_ptr->spell_flags[0] & (RSF0_BR_SOUN | RSF0_BR_WALL)))
	{
		/* Obvious (irrational caution) */
		if (seen) obvious = TRUE;

		m_ptr->inc_core_timed<CORE_TMD_STUN>(MIN(do_stun, 200));
	}

	/* Confusion and Chaos breathers (and sleepers) never confuse */
	else if (m_ptr->take_confusion(do_conf, 0))
	{
		/* Obvious (irrational caution) */
		if (seen) obvious = TRUE;
	}


	/* Fear */
	m_ptr->inc_core_timed<CORE_TMD_AFRAID>(do_fear);

	
	/* If another monster did the damage, hurt the monster by hand */
	if (who > 0)
	{
		/* Redraw (later) if needed */
		if (p_ptr->health_who == cave_m_idx[g.y][g.x]) p_ptr->redraw |= (PR_HEALTH);

		m_ptr->csleep = 0;	/* Wake the monster up */
		m_ptr->chp -= dam;	/* Hurt the monster */

		/* Dead monster */
		if (m_ptr->chp < 0)
		{
			/* Generate treasure, etc */
			monster_death(cave_m_idx[g.y][g.x]);

			/* Delete the monster */
			delete_monster_idx(cave_m_idx[g.y][g.x]);

			/* Give detailed messages if destroyed */
			if (note) msg_format("%^s%s", m_name, note);
		}

		/* Damaged monster */
		else
		{
			/* Give detailed messages if visible or destroyed */
			if (note && seen) msg_format("%^s%s", m_name, note);

			/* Hack -- Pain message */
			else if (dam > 0) message_pain(cave_m_idx[g.y][g.x], dam);

			/* Hack -- handle sleep */
			if (do_sleep) m_ptr->csleep = do_sleep;
		}
	}

	/* If the player did it, give him experience, check fear */
	else
	{
		bool fear = FALSE;

		/* Hurt the monster, check for fear and death */
		if (!mon_take_hit(cave_m_idx[g.y][g.x], dam, &fear, note_dies))
		{	/* monster survived */
			/* Give detailed messages if visible or destroyed */
			if (note && seen) msg_format("%^s%s", m_name, note);

			/* Hack -- Pain message */
			else if (dam > 0) message_pain(cave_m_idx[g.y][g.x], dam);

			/* Take note */
			if ((fear || do_fear) && (m_ptr->ml))
			{
				/* Message */
				message_format(MSG_FLEE, "%^s flees in terror!", m_name);
			}

			/* Hack -- handle sleep */
			if (do_sleep) m_ptr->csleep = do_sleep;
		}
	}


	update_mon(cave_m_idx[g.y][g.x], FALSE);	/* Update the monster */
	lite_spot(g);								/* Redraw the monster grid */

	/* Update monster recall window */
	if (p_ptr->monster_race_idx == m_ptr->r_idx) p_ptr->redraw |= (PR_MONSTER);
	}	/* end C-ish blocking brace */

	/* Track it */
	project_m_n++;
	project_m_g = g;


	/* Return "Anything seen?" */
	return (obvious);
}


/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to the player.
 *
 * This routine takes a "source monster" (by index), a "distance", a default
 * "damage", and a "damage type".  See "project_m()" above.
 *
 * If "rad" is non-zero, then the blast was centered elsewhere, and the damage
 * is reduced (see "project_m()" above).  This can happen if a monster breathes
 * at the player and hits a wall instead.
 *
 * We return "TRUE" if any "obvious" effects were observed.
 *
 * Actually, for historical reasons, we just assume that the effects were
 * obvious.  XXX XXX XXX
 */
static bool project_p(int who, int r, int y, int x, int dam, int typ)
{
	int k = 0;
	bool blind = p_ptr->timed[TMD_BLIND];	/* Player blind-ness */
	monster_type *m_ptr;					/* Source monster */
	char m_name[80];						/* Monster name (for attacks) */
	char killer[80];						/* Monster name (for damage) */

	/* No player here */
	if (!(cave_m_idx[y][x] < 0)) return (FALSE);

	/* Never affect projector */
	if (cave_m_idx[y][x] == who) return (FALSE);


	/* Limit maximum damage XXX XXX XXX */
	if (dam > 1600) dam = 1600;

	/* Reduce damage by distance */
	dam = (dam + r) / (r + 1);


	/* Get the source monster */
	m_ptr = &mon_list[who];

	/* Get the monster name */
	monster_desc(m_name, sizeof(m_name), m_ptr, 0);

	/* Get the monster's real name */
	monster_desc(killer, sizeof(killer), m_ptr, MDESC_SHOW | MDESC_IND2);

	/* Analyze the damage */
	switch (typ)
	{
		/* Standard damage -- hurts inventory too */
		case GF_ACID:
		{
			if (blind) msg_print("You are hit by acid!");
			acid_dam(dam, killer);
			update_smart_learn(who, DRS_RES_ACID);
			break;
		}

		/* Standard damage -- hurts inventory too */
		case GF_FIRE:
		{
			if (blind) msg_print("You are hit by fire!");
			fire_dam(dam, killer);
			update_smart_learn(who, DRS_RES_FIRE);
			break;
		}

		/* Standard damage -- hurts inventory too */
		case GF_COLD:
		{
			if (blind) msg_print("You are hit by cold!");
			cold_dam(dam, killer);
			update_smart_learn(who, DRS_RES_COLD);
			break;
		}

		/* Standard damage -- hurts inventory too */
		case GF_ELEC:
		{
			if (blind) msg_print("You are hit by lightning!");
			elec_dam(dam, killer);
			update_smart_learn(who, DRS_RES_ELEC);
			break;
		}

		/* Standard damage -- also poisons player */
		case GF_POIS:
		{
			if (blind) msg_print("You are hit by poison!");
			if (p_ptr->resist_pois) dam = DR<3>(dam);
			if (p_ptr->timed[TMD_OPP_POIS]) dam = DR<3>(dam);
			take_hit(dam, killer);
			if (!(p_ptr->resist_pois || p_ptr->timed[TMD_OPP_POIS]))
			{
				(void)p_ptr->inc_timed<TMD_POISONED>(rand_int(dam) + 10);
			}
			update_smart_learn(who, DRS_RES_POIS);
			break;
		}

		/* Standard damage */
		case GF_MISSILE:
		{
			if (blind) msg_print("You are hit by something!");
			take_hit(dam, killer);
			break;
		}

		/* Holy Orb -- Player only takes partial damage */
		case GF_HOLY_ORB:
		{
			if (blind) msg_print("You are hit by something!");
			dam /= 2;
			take_hit(dam, killer);
			break;
		}

		/* Arrow -- no dodging XXX */
		case GF_ARROW:
		{
			if (blind) msg_print("You are hit by something sharp!");
			take_hit(dam, killer);
			break;
		}

		/* Plasma -- No resist XXX */
		case GF_PLASMA:
		{
			if (blind) msg_print("You are hit by something!");
			take_hit(dam, killer);
			if (!p_ptr->resist_sound)
			{
				p_ptr->inc_core_timed<CORE_TMD_STUN>(randint((dam > 40) ? 35 : (dam * 3 / 4 + 5)));
			}
			break;
		}

		/* Nether -- drain experience */
		case GF_NETHER:
		{
			if (blind) msg_print("You are hit by something strange!");
			if (p_ptr->resist_nethr)
			{
				dam *= 6; dam /= (randint(6) + 6);
			}
			else
			{
				if (p_ptr->hold_life && !one_in_(4))
				{
					msg_print("You keep hold of your life force!");
				}
				else
				{
					s32b d = 200 + (p_ptr->exp / 100) * MON_DRAIN_LIFE;

					if (p_ptr->hold_life)
					{
						msg_print("You feel your life slipping away!");
						lose_exp(d / 10);
					}
					else
					{
						msg_print("You feel your life draining away!");
						lose_exp(d);
					}
				}
			}
			take_hit(dam, killer);
			update_smart_learn(who, DRS_RES_NETHR);
			break;
		}

		/* Water -- stun/confuse */
		case GF_WATER:
		{
			if (blind) msg_print("You are hit by something!");
			if (!p_ptr->resist_sound)
			{
				p_ptr->inc_core_timed<CORE_TMD_STUN>(randint(40));
			}
			p_ptr->take_confusion(6, 5);
			take_hit(dam, killer);
			break;
		}

		/* Chaos -- many effects */
		case GF_CHAOS:
		{
			if (blind) msg_print("You are hit by something strange!");
			if (p_ptr->resist_chaos)
			{
				dam *= 6; dam /= (randint(6) + 6);
			}
			if (!p_ptr->resist_chaos)
			{
				p_ptr->take_confusion(10, 20);
				p_ptr->inc_timed<TMD_IMAGE>(randint(10));
			}
			if (!p_ptr->resist_nethr && !p_ptr->resist_chaos)
			{
				if (p_ptr->hold_life && (rand_int(100) < 75))
				{
					msg_print("You keep hold of your life force!");
				}
				else
				{
					s32b d = 5000 + (p_ptr->exp / 100) * MON_DRAIN_LIFE;

					if (p_ptr->hold_life)
					{
						msg_print("You feel your life slipping away!");
						lose_exp(d / 10);
					}
					else
					{
						msg_print("You feel your life draining away!");
						lose_exp(d);
					}
				}
			}
			take_hit(dam, killer);
			update_smart_learn(who, DRS_RES_CHAOS);
			break;
		}

		/* Shards -- mostly cutting */
		case GF_SHARD:
		{
			if (blind) msg_print("You are hit by something sharp!");
			if (p_ptr->resist_shard)
			{
				dam *= 6; dam /= (randint(6) + 6);
			}
			else
			{
				p_ptr->inc_timed<TMD_CUT>(dam);
			}
			take_hit(dam, killer);
			update_smart_learn(who, DRS_RES_SHARD);
			break;
		}

		/* Sound -- mostly stunning */
		case GF_SOUND:
		{
			if (blind) msg_print("You are hit by something!");
			if (p_ptr->resist_sound)
			{
				dam *= 5; dam /= (randint(6) + 6);
			}
			else
			{
				p_ptr->inc_core_timed<CORE_TMD_STUN>(randint((dam > 90) ? 35 : (dam / 3 + 5)));
			}
			take_hit(dam, killer);
			update_smart_learn(who, DRS_RES_SOUND);
			break;
		}

		/* Pure confusion */
		case GF_CONFUSION:
		{
			if (blind) msg_print("You are hit by something!");
			if (p_ptr->resist_confu)
			{
				dam *= 5; dam /= (randint(6) + 6);
			}
			p_ptr->take_confusion(11, 20);
			take_hit(dam, killer);
			update_smart_learn(who, DRS_RES_CONFU);
			break;
		}

		/* Disenchantment -- see above */
		case GF_DISENCHANT:
		{
			if (blind) msg_print("You are hit by something strange!");
			if (p_ptr->resist_disen)
			{
				dam *= 6; dam /= (randint(6) + 6);
			}
			else
			{
				apply_disenchant(0);
			}
			take_hit(dam, killer);
			update_smart_learn(who, DRS_RES_DISEN);
			break;
		}

		/* Nexus -- see above */
		case GF_NEXUS:
		{
			if (blind) msg_print("You are hit by something strange!");
			if (p_ptr->resist_nexus)
			{
				dam *= 6; dam /= (randint(6) + 6);
			}
			else
			{
				apply_nexus(m_ptr->loc);
			}
			take_hit(dam, killer);
			update_smart_learn(who, DRS_RES_NEXUS);
			break;
		}

		/* Force -- mostly stun */
		case GF_FORCE:
		{
			if (blind) msg_print("You are hit by something!");
			if (!p_ptr->resist_sound)
			{
				p_ptr->inc_core_timed<CORE_TMD_STUN>(randint(20));
			}
			take_hit(dam, killer);
			break;
		}

		/* Inertia -- slowness */
		case GF_INERTIA:
		{
			if (blind) msg_print("You are hit by something strange!");
			p_ptr->inc_core_timed<CORE_TMD_SLOW>(rand_int(4) + 4);
			take_hit(dam, killer);
			break;
		}

		/* Lite -- blinding */
		case GF_LITE:
		{
			if (blind) msg_print("You are hit by something!");
			if (p_ptr->resist_lite)
			{
				dam *= 4; dam /= (randint(6) + 6);
			}
			else if (!blind && !p_ptr->resist_blind)
			{
				p_ptr->inc_timed<TMD_BLIND>(randint(5) + 2);
			}
			take_hit(dam, killer);
			update_smart_learn(who, DRS_RES_LITE);
			break;
		}

		/* Dark -- blinding */
		case GF_DARK:
		{
			if (blind) msg_print("You are hit by something!");
			if (p_ptr->resist_dark)
			{
				dam *= 4; dam /= (randint(6) + 6);
			}
			else if (!blind && !p_ptr->resist_blind)
			{
				p_ptr->inc_timed<TMD_BLIND>(randint(5) + 2);
			}
			take_hit(dam, killer);
			update_smart_learn(who, DRS_RES_DARK);
			break;
		}

		/* Time -- bolt fewer effects XXX */
		case GF_TIME:
		{
			if (blind) msg_print("You are hit by something strange!");

			switch (randint(10))
			{
				case 1: case 2: case 3: case 4: case 5:
				{
					msg_print("You feel life has clocked back.");
					lose_exp(100 + (p_ptr->exp / 100) * MON_DRAIN_LIFE);
					break;
				}

				case 6: case 7: case 8: case 9:
				{
					static const char* const time_drain[A_MAX] =	{	"strong","bright","wise","agile","hale","beautiful"	};
					k = rand_int(A_MAX);

					msg_format("You're not as %s as you used to be...", time_drain[k]);

					p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 3) / 4;
					if (p_ptr->stat_cur[k] < 3) p_ptr->stat_cur[k] = 3;
					p_ptr->update |= (PU_BONUS);
					break;
				}

				case 10:
				{
					msg_print("You're not as powerful as you used to be...");

					for (k = 0; k < A_MAX; k++)
					{
						p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 3) / 4;
						if (p_ptr->stat_cur[k] < 3) p_ptr->stat_cur[k] = 3;
					}
					p_ptr->update |= (PU_BONUS);
					break;
				}
			}
			take_hit(dam, killer);
			break;
		}

		/* Gravity -- stun plus slowness plus teleport */
		case GF_GRAVITY:
		{
			if (blind) msg_print("You are hit by something strange!");
			msg_print("Gravity warps around you.");

			/* Higher level players can resist the teleportation better */
			if (randint(127) > p_ptr->lev)
				teleport_player(5);

			p_ptr->inc_core_timed<CORE_TMD_SLOW>(rand_int(4) + 4);
			if (!p_ptr->resist_sound)
			{
				p_ptr->inc_core_timed<CORE_TMD_STUN>(randint((dam > 90) ? 35 : (dam / 3 + 5)));
			}
			take_hit(dam, killer);
			break;
		}

		/* Pure damage */
		case GF_MANA:
		{
			if (blind) msg_print("You are hit by something!");
			take_hit(dam, killer);
			break;
		}

		/* Pure damage */
		case GF_METEOR:
		{
			if (blind) msg_print("You are hit by something!");
			take_hit(dam, killer);
			break;
		}

		/* Ice -- cold plus stun plus cuts */
		case GF_ICE:
		{
			if (blind) msg_print("You are hit by something sharp!");
			cold_dam(dam, killer);
			if (!p_ptr->resist_shard)
			{
				p_ptr->inc_timed<TMD_CUT>(NdS(5, 8));
			}
			if (!p_ptr->resist_sound)
			{
				p_ptr->inc_core_timed<CORE_TMD_STUN>(randint(15));
			}
			update_smart_learn(who, DRS_RES_COLD);
			break;
		}


		/* Default */
		default:
		{
			/* No damage */
			dam = 0;
			break;
		}
	}

	disturb(1, 0);	/* Disturb */
	return TRUE;	/* Return "Anything seen?" */
}





/*
 * Generic "beam"/"bolt"/"ball" projection routine.
 *
 * Input:
 *   who: Index of "source" monster (negative for "player")
 *   rad: Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 *   y,x: Target location (or location to travel "towards")
 *   dam: Base damage roll to apply to affected monsters (or player)
 *   typ: Type of damage to apply to monsters (and objects)
 *   flg: Extra bit flags (see PROJECT_xxxx in "defines.h")
 *
 * Return:
 *   TRUE if any "effects" of the projection were observed, else FALSE
 *
 * Allows a monster (or player) to project a beam/bolt/ball of a given kind
 * towards a given location (optionally passing over the heads of interposing
 * monsters), and have it do a given amount of damage to the monsters (and
 * optionally objects) within the given radius of the final location.
 *
 * A "bolt" travels from source to target and affects only the target grid.
 * A "beam" travels from source to target, affecting all grids passed through.
 * A "ball" travels from source to the target, exploding at the target, and
 *   affecting everything within the given radius of the target location.
 *
 * Traditionally, a "bolt" does not affect anything on the ground, and does
 * not pass over the heads of interposing monsters, much like a traditional
 * missile, and will "stop" abruptly at the "target" even if no monster is
 * positioned there, while a "ball", on the other hand, passes over the heads
 * of monsters between the source and target, and affects everything except
 * the source monster which lies within the final radius, while a "beam"
 * affects every monster between the source and target, except for the casting
 * monster (or player), and rarely affects things on the ground.
 *
 * Two special flags allow us to use this function in special ways, the
 * "PROJECT_HIDE" flag allows us to perform "invisible" projections, while
 * the "PROJECT_JUMP" flag allows us to affect a specific grid, without
 * actually projecting from the source monster (or player).
 *
 * The player will only get "experience" for monsters killed by himself
 * Unique monsters can only be destroyed by attacks from the player
 *
 * Only 256 grids can be affected per projection, limiting the effective
 * "radius" of standard ball attacks to nine units (diameter nineteen).
 *
 * One can project in a given "direction" by combining PROJECT_THRU with small
 * offsets to the initial location (see "line_spell()"), or by calculating
 * "virtual targets" far away from the player.
 *
 * One can also use PROJECT_THRU to send a beam/bolt along an angled path,
 * continuing until it actually hits somethings (useful for "stone to mud").
 *
 * Bolts and Beams explode INSIDE walls, so that they can destroy doors.
 *
 * Balls must explode BEFORE hitting walls, or they would affect monsters
 * on both sides of a wall.  Some bug reports indicate that this is still
 * happening in 2.7.8 for Windows, though it appears to be impossible.
 *
 * We "pre-calculate" the blast area only in part for efficiency.
 * More importantly, this lets us do "explosions" from the "inside" out.
 * This results in a more logical distribution of "blast" treasure.
 * It also produces a better (in my opinion) animation of the explosion.
 * It could be (but is not) used to have the treasure dropped by monsters
 * in the middle of the explosion fall "outwards", and then be damaged by
 * the blast as it spreads outwards towards the treasure drop location.
 *
 * Walls and doors are included in the blast area, so that they can be
 * "burned" or "melted" in later versions.
 *
 * This algorithm is intended to maximize simplicity, not necessarily
 * efficiency, since this function is not a bottleneck in the code.
 *
 * We apply the blast effect from ground zero outwards, in several passes,
 * first affecting features, then objects, then monsters, then the player.
 * This allows walls to be removed before checking the object or monster
 * in the wall, and protects objects which are dropped by monsters killed
 * in the blast, and allows the player to see all affects before he is
 * killed or teleported away.  The semantics of this method are open to
 * various interpretations, but they seem to work well in practice.
 *
 * We process the blast area from ground-zero outwards to allow for better
 * distribution of treasure dropped by monsters, and because it provides a
 * pleasing visual effect at low cost.
 *
 * Note that the damage done by "ball" explosions decreases with distance.
 * This decrease is rapid, grids at radius "dist" take "1/dist" damage.
 *
 * Notice the "napalm" effect of "beam" weapons.  First they "project" to
 * the target, and then the damage "flows" along this beam of destruction.
 * The damage at every grid is the same as at the "center" of a "ball"
 * explosion, since the "beam" grids are treated as if they ARE at the
 * center of a "ball" explosion.
 *
 * Currently, specifying "beam" plus "ball" means that locations which are
 * covered by the initial "beam", and also covered by the final "ball", except
 * for the final grid (the epicenter of the ball), will be "hit twice", once
 * by the initial beam, and once by the exploding ball.  For the grid right
 * next to the epicenter, this results in 150% damage being done.  The center
 * does not have this problem, for the same reason the final grid in a "beam"
 * plus "bolt" does not -- it is explicitly removed.  Simply removing "beam"
 * grids which are covered by the "ball" will NOT work, as then they will
 * receive LESS damage than they should.  Do not combine "beam" with "ball".
 *
 * The array "gy[],gx[]" with current size "grids" is used to hold the
 * collected locations of all grids in the "blast area" plus "beam path".
 *
 * Note the rather complex usage of the "gm[]" array.  First, gm[0] is always
 * zero.  Second, for N>1, gm[N] is always the index (in gy[],gx[]) of the
 * first blast grid (see above) with radius "N" from the blast center.  Note
 * that only the first gm[1] grids in the blast area thus take full damage.
 * Also, note that gm[rad+1] is always equal to "grids", which is the total
 * number of blast grids.
 *
 * Note that once the projection is complete, (y2,x2) holds the final location
 * of bolts/beams, and the "epicenter" of balls.
 *
 * Note also that "rad" specifies the "inclusive" radius of projection blast,
 * so that a "rad" of "one" actually covers 5 or 9 grids, depending on the
 * implementation of the "distance" function.  Also, a bolt can be properly
 * viewed as a "ball" with a "rad" of "zero".
 *
 * Note that if no "target" is reached before the beam/bolt/ball travels the
 * maximum distance allowed (MAX_RANGE), no "blast" will be induced.  This
 * may be relevant even for bolts, since they have a "1x1" mini-blast.
 *
 * Note that for consistency, we "pretend" that the bolt actually takes "time"
 * to move from point A to point B, even if the player cannot see part of the
 * projection path.  Note that in general, the player will *always* see part
 * of the path, since it either starts at the player or ends on the player.
 *
 * Hack -- we assume that every "projection" is "self-illuminating".
 *
 * Hack -- when only a single monster is affected, we automatically track
 * (and recall) that monster, unless "PROJECT_JUMP" is used.
 *
 * Note that all projections now "explode" at their final destination, even
 * if they were being projected at a more distant destination.  This means
 * that "ball" spells will *always* explode.
 *
 * Note that we must call "handle_stuff()" after affecting terrain features
 * in the blast radius, in case the "illumination" of the grid was changed,
 * and "update_view()" and "update_monsters()" need to be called.
 */
bool project(int who, int rad, coord g, int dam, int typ, int flg)
{
	coord p_g = p_ptr->loc;
	coord g1, g2;

	int i, t, dist;

	int msec = op_ptr->delay_factor * op_ptr->delay_factor;

	bool notice = FALSE;	/* Assume the player sees nothing */
	bool visual = FALSE;	/* Assume the player has seen nothing */
	bool drawn = FALSE;		/* Assume the player has seen no blast grids */
	bool blind = p_ptr->timed[TMD_BLIND];	/* Is the player blind? */
	int path_n = 0;			/* Number of grids in the "path" */
	coord path_g[512];	/* Actual grids in the "path" */
	int grids = 0;	/* Number of grids in the "blast area" (including the "beam" path) */
	coord zap_g[256];
	byte gm[16];	/* Encoded "radius" info (see above) */

	/* Hack -- Jump to target */
	if (flg & (PROJECT_JUMP))
	{
		g1 = g;
		flg &= ~(PROJECT_JUMP);	/* Clear the flag */
	}

	/* Start at player */
	else if (who < 0)
		g1 = p_g;

	/* Start at monster */
	else if (who > 0)
		g1 = mon_list[who].loc;

	/* Oops */
	else
		g1 = g;

	/* Default "destination" */
	g2 = g;

	/* verify PROJECT_THRU flag */
	if ((flg & (PROJECT_THRU)) && g1==g2) flg &= ~(PROJECT_THRU);

	
	C_WIPE(gm+0, 16);	/* Assume there will be no blast (max radius 16) */
	g = g1;				/* Initial grid */

	/* Collect beam grids */
	if (flg & (PROJECT_BEAM)) C_ARRAY_PUSH(zap_g,N_ELEMENTS(zap_g),grids,g);

	/* Calculate the projection path */
	path_n = project_path(path_g, MAX_RANGE, g1, g2, !(flg & (PROJECT_THRU)),((flg & (PROJECT_STOP)) ? &wall_mon_stop : &wall_stop));


	/* Hack -- Handle stuff */
	handle_stuff();

	/* Project along the path */
	for (i = 0; i < path_n; ++i)
	{
		coord o = g;
		coord n = path_g[i];

		/* Hack -- Balls explode before reaching walls */
		if (!cave_floor_bold(n.y, n.x) && (rad > 0)) break;

		/* Advance */
		g = n;

		/* Collect beam grids */
		if (flg & (PROJECT_BEAM)) C_ARRAY_PUSH(zap_g,N_ELEMENTS(zap_g),grids,g);

		/* Only do visuals if requested */
		if (!blind && !(flg & (PROJECT_HIDE)))
		{
			/* Only do visuals if the player can "see" the bolt */
			if (player_has_los_bold(g.y, g.x))
			{
				/* Visual effects */
				print_rel(bolt_pict(o.y, o.x, g.y, g.x, typ), g);
				move_cursor_relative(g);

				Term_fresh();

				Term_xtra(TERM_XTRA_DELAY, msec);

				lite_spot(g);

				Term_fresh();

				/* Display "beam" grids */
				if (flg & (PROJECT_BEAM))
				{
					/* Visual effects */
					print_rel(bolt_pict(g.y, g.x, g.y, g.x, typ), g);
				}

				/* Hack -- Activate delay */
				visual = TRUE;
			}

			/* Hack -- delay anyway for consistency */
			else if (visual)
			{
				/* Delay for consistency */
				Term_xtra(TERM_XTRA_DELAY, msec);
			}
		}
	}

	g2 = g;			/* Save the "blast epicenter" */
	gm[0] = 0;		/* Start the "explosion" */
	gm[1] = grids;	/* Hack -- make sure beams get to "explode" */

	/* Explode */
	/* Hack -- remove final beam grid */
	if (flg & (PROJECT_BEAM)) grids--;

	/* Determine the blast area, work from the inside out */
	for (dist = 0; dist <= rad; dist++)
	{
		/* Scan the maximal blast area of radius "dist" */
		for (g.y = g2.y - dist; g.y <= g2.y + dist; g.y++)
		{
			for (g.x = g2.x - dist; g.x <= g2.x + dist; g.x++)
			{
				/* Ignore "illegal" locations */
				if (!in_bounds(g.y, g.x)) continue;

				/* Enforce a "circular" explosion */
				if (distance(g2.y, g2.x, g.y, g.x) != dist) continue;

				/* Ball explosions are stopped by walls */
				if (!los(g2, g)) continue;

				/* Save this grid */
				C_ARRAY_PUSH(zap_g,N_ELEMENTS(zap_g),grids,g);
			}
		}

		/* Encode some more "radius" info */
		gm[dist+1] = grids;
	}


	/* Speed -- ignore "non-explosions" */
	if (!grids) return (FALSE);


	/* Display the "blast area" if requested */
	if (!blind && !(flg & (PROJECT_HIDE)))
	{
		/* Then do the "blast", from inside out */
		for (t = 0; t <= rad; t++)
		{
			/* Dump everything with this radius */
			for (i = gm[t]; i < gm[t+1]; i++)
			{
				/* Extract the location */
				g = zap_g[i];

				/* Only do visuals if the player can "see" the blast */
				if (player_has_los_bold(g.y, g.x))
				{
					drawn = TRUE;

					/* Visual effects -- Display */
					print_rel(bolt_pict(g.y, g.x, g.y, g.x, typ), g);
				}
			}

			/* Hack -- center the cursor */
			move_cursor_relative(g2);

			/* Flush */
			Term_fresh();

			/* Delay (efficiently) */
			if (visual || drawn)
			{
				Term_xtra(TERM_XTRA_DELAY, msec);
			}
		}

		/* Flush the erasing */
		if (drawn)
		{
			/* Erase the explosion drawn above */
			for (i = 0; i < grids; i++)
			{
				/* Extract the location */
				g = zap_g[i];

				/* Hack -- Erase if needed */
				if (player_has_los_bold(g.y, g.x))
				{
					lite_spot(g);
				}
			}

			/* Hack -- center the cursor */
			move_cursor_relative(g2);

			/* Flush */
			Term_fresh();
		}
	}


	/* Check features */
	if (flg & (PROJECT_GRID))
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for features */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Affect the feature in that grid */
			if (project_f(who, dist, zap_g[i], dam, typ)) notice = TRUE;
		}
	}


	/* Update stuff if needed */
	if (p_ptr->update) update_stuff();


	/* Check objects */
	if (flg & (PROJECT_ITEM))
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for objects */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Affect the object in the grid */
			if (project_o(who, dist, zap_g[i], dam, typ)) notice = TRUE;
		}
	}


	/* Check monsters */
	if (flg & (PROJECT_KILL))
	{
		/* Mega-Hack */
		project_m_n = 0;
		project_m_g.clear();

		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for monsters */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Affect the monster in the grid */
			if (project_m(who, dist, zap_g[i], dam, typ)) notice = TRUE;
		}

		/* Player affected one monster (without "jumping") */
		if ((who < 0) && (project_m_n == 1) && !(flg & (PROJECT_JUMP)))
		{
			/* Location */
			g = project_m_g;

			/* Track if possible */
			if (cave_m_idx[g.y][g.x] > 0)
			{
				const monster_type* const m_ptr = &mon_list[cave_m_idx[g.y][g.x]];

				if (m_ptr->ml)
				{
					monster_race_track(m_ptr->r_idx);		/* auto-recall */
					health_track(cave_m_idx[g.y][g.x]);		/* auto-track */
				}
			}
		}
	}


	/* Check player */
	if (flg & (PROJECT_KILL))
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for player */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Affect the player */
			if (project_p(who, dist, zap_g[i].y, zap_g[i].x, dam, typ))
			{
				notice = TRUE;
				break;			/* Only affect the player once */
			}
		}
	}


	/* Return "something was noticed" */
	return (notice);
}
