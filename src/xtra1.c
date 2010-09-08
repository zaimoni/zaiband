/* File: xtra1.c */

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
#include "game-event.h"
#include "option.h"
#include "tvalsval.h"
#include "wind_flg.h"

#include "POD.hpp"

using namespace zaiband;

/*** Screen Locations ***/

/*
 * Some screen locations for various display routines
 * Currently, row 8 and 15 are the only "blank" rows.
 * That leaves a "border" around the "stat" values.
 */

#define ROW_RACE		1
#define COL_RACE		0	/* <race name> */

#define ROW_CLASS		2
#define COL_CLASS		0	/* <class name> */

#define ROW_TITLE		3
#define COL_TITLE		0	/* <title> or <mode> */

#define ROW_LEVEL		4
#define COL_LEVEL		0	/* "LEVEL xxxxxx" */

#define ROW_EXP			5
#define COL_EXP			0	/* "EXP xxxxxxxx" */

#define ROW_GOLD		6
#define COL_GOLD		0	/* "AU xxxxxxxxx" */

#define ROW_EQUIPPY		7
#define COL_EQUIPPY		0	/* equippy chars */

#define ROW_STAT		8
#define COL_STAT		0	/* "xxx   xxxxxx" */

#define ROW_AC			15
#define COL_AC			0	/* "Cur AC xxxxx" */

#define ROW_HP			16
#define COL_HP			0

#define ROW_SP			17
#define COL_SP			0	/* "SP xxxx/xxxx" */

#define ROW_ENERGY		18
#define COL_ENERGY		0	/* "Energy: xx" */

#define ROW_INFO		19
#define COL_INFO		0	/* "xxxxxxxxxxxx" / "xxxxx" */

#define ROW_CUT			21
#define COL_CUT			0	/* <cut> */

#define ROW_STUN		22
#define COL_STUN		0	/* <stun> */

#define ROW_HUNGRY		(Term->hgt - 1)
#define COL_HUNGRY		0	/* "Weak" / "Hungry" / "Full" / "Gorged" */

#define ROW_BLIND		(Term->hgt - 1)
#define COL_BLIND		7	/* "Blind" */

#define ROW_CONFUSED	(Term->hgt - 1)
#define COL_CONFUSED	13	/* "Confused" */

#define ROW_AFRAID		(Term->hgt - 1)
#define COL_AFRAID		22	/* "Afraid" */

#define ROW_POISONED	(Term->hgt - 1)
#define COL_POISONED	29	/* "Poisoned" */

#define ROW_STATE		(Term->hgt - 1)
#define COL_STATE		38	/* <state> */

#define ROW_SPEED		(Term->hgt - 1)
#define COL_SPEED		49	/* "Slow (-NN)" or "Fast (+NN)" */

#define ROW_STUDY		(Term->hgt - 1)
#define COL_STUDY		64	/* "Study" */

#define ROW_DEPTH		(Term->hgt - 1)
#define COL_DEPTH		70	/* "Lev NNN" / "NNNN ft" */

#define ROW_OPPOSE_ELEMENTS	(Term->hgt - 1)
#define COL_OPPOSE_ELEMENTS	80	/* "Acid Elec Fire Cold Pois" */

/*
 * Converts stat num into a six-char (right justified) string
 */
void cnv_stat(int val, char *out_val)
{
	/* Above 18 */
	if (val > 18)
	{
		int bonus = (val - 18);

		if (bonus >= 100)
		{
			sprintf(out_val, "18/%03d", bonus);
		}
		else
		{
			sprintf(out_val, " 18/%02d", bonus);
		}
	}

	/* From 3 to 18 */
	else
	{
		sprintf(out_val, "    %2d", val);
	}
}



/*
 * Modify a stat value by a "modifier", return new value
 *
 * Stats go up: 3,4,...,17,18,18/10,18/20,...,18/220
 * Or even: 18/13, 18/23, 18/33, ..., 18/220
 *
 * Stats go down: 18/220, 18/210,..., 18/10, 18, 17, ..., 3
 * Or even: 18/13, 18/03, 18, 17, ..., 3
 */
s16b modify_stat_value(int value, int amount)
{
	int i;

	/* Reward */
	if (amount > 0)
	{
		/* Apply each point */
		for (i = 0; i < amount; i++)
		{
			/* One point at a time */
			if (value < 18) value++;

			/* Ten "points" at a time */
			else value += 10;
		}
	}

	/* Penalty */
	else if (amount < 0)
	{
		/* Apply each point */
		for (i = 0; i < (0 - amount); i++)
		{
			/* Ten points at a time */
			if (value >= 18+10) value -= 10;

			/* Hack -- prevent weirdness */
			else if (value > 18) value = 18;

			/* One point at a time */
			else if (value > 3) value--;
		}
	}

	/* Return new value */
	return (value);
}


/*
 * Calculate number of spells player should have, and forget,
 * or remember, spells until that number is properly reflected.
 *
 * Note that this function induces various "status" messages,
 * which must be bypasses until the character is created.
 */
static void calc_spells(void)
{
	int i, j, k, levels;
	int num_allowed, num_known;
	int percent_spells;

	const magic_type *s_ptr;

	s16b old_spells;

	const char* const p = ((p_ptr->cp_ptr->spell_book == TV_MAGIC_BOOK) ? "spell" : "prayer");


	/* Hack -- must be literate */
	if (!p_ptr->spell_book()) return;

	/* Hack -- wait for creation */
	if (!character_generated) return;

	/* Hack -- handle "xtra" mode */
	if (character_xtra) return;

	/* Save the new_spells value */
	old_spells = p_ptr->new_spells;


	/* Determine the number of spells allowed */
	levels = p_ptr->lev - p_ptr->cp_ptr->spell_first + 1;

	/* Hack -- no negative spells */
	if (levels < 0) levels = 0;

	/* Number of 1/100 spells per level */
	percent_spells = adj_mag_study[p_ptr->stat_ind[p_ptr->cp_ptr->spell_stat]];

	/* Extract total allowed spells (rounded up) */
	num_allowed = (((percent_spells * levels) + 50) / 100);

	/* Assume none known */
	num_known = 0;

	/* Count the number of spells we know */
	for (j = 0; j < PY_MAX_SPELLS; j++)
	{
		/* Count known spells */
		if (p_ptr->spell_flags[j] & PY_SPELL_LEARNED)
		{
			num_known++;
		}
	}

	/* See how many spells we must forget or may learn */
	p_ptr->new_spells = num_allowed - num_known;



	/* Forget spells which are too hard */
	for (i = PY_MAX_SPELLS - 1; i >= 0; i--)
	{
		/* Get the spell */
		j = p_ptr->spell_order[i];

		/* Skip non-spells */
		if (j >= 99) continue;

		/* Get the spell */
		s_ptr = p_ptr->spell_info(j);

		/* Skip spells we are allowed to know */
		if (s_ptr->slevel <= p_ptr->lev) continue;

		/* Is it known? */
		if (p_ptr->spell_flags[j] & PY_SPELL_LEARNED)
		{
			/* Mark as forgotten */
			p_ptr->spell_flags[j] |= PY_SPELL_FORGOTTEN;

			/* No longer known */
			p_ptr->spell_flags[j] &= ~PY_SPELL_LEARNED;

			/* Message */
			msg_format("You have forgotten the %s of %s.", p,
			           get_spell_name(p_ptr->cp_ptr->spell_book, j));

			/* One more can be learned */
			p_ptr->new_spells++;
		}
	}


	/* Forget spells if we know too many spells */
	for (i = PY_MAX_SPELLS - 1; i >= 0; i--)
	{
		/* Stop when possible */
		if (p_ptr->new_spells >= 0) break;

		/* Get the (i+1)th spell learned */
		j = p_ptr->spell_order[i];

		/* Skip unknown spells */
		if (j >= 99) continue;

		/* Forget it (if learned) */
		if (p_ptr->spell_flags[j] & PY_SPELL_LEARNED)
		{
			/* Mark as forgotten */
			p_ptr->spell_flags[j] |= PY_SPELL_FORGOTTEN;

			/* No longer known */
			p_ptr->spell_flags[j] &= ~PY_SPELL_LEARNED;

			/* Message */
			msg_format("You have forgotten the %s of %s.", p,
			           get_spell_name(p_ptr->cp_ptr->spell_book, j));

			/* One more can be learned */
			p_ptr->new_spells++;
		}
	}


	/* Check for spells to remember */
	for (i = 0; i < PY_MAX_SPELLS; i++)
	{
		/* None left to remember */
		if (p_ptr->new_spells <= 0) break;

		/* Get the next spell we learned */
		j = p_ptr->spell_order[i];

		/* Skip unknown spells */
		if (j >= 99) break;

		/* Get the spell */
		s_ptr = p_ptr->spell_info(j);

		/* Skip spells we cannot remember */
		if (s_ptr->slevel > p_ptr->lev) continue;

		/* First set of spells */
		if (p_ptr->spell_flags[j] & PY_SPELL_FORGOTTEN)
		{
			/* No longer forgotten */
			p_ptr->spell_flags[j] &= ~PY_SPELL_FORGOTTEN;

			/* Known once more */
			p_ptr->spell_flags[j] |= PY_SPELL_LEARNED;

			/* Message */
			msg_format("You have remembered the %s of %s.",
			           p, get_spell_name(p_ptr->cp_ptr->spell_book, j));

			/* One less can be learned */
			p_ptr->new_spells--;
		}
	}


	/* Assume no spells available */
	k = 0;

	/* Count spells that can be learned */
	for (j = 0; j < PY_MAX_SPELLS; j++)
	{
		/* Get the spell */
		s_ptr = p_ptr->spell_info(j);

		/* Skip spells we cannot remember */
		if (s_ptr->slevel > p_ptr->lev) continue;

		/* Skip spells we already know */
		if (p_ptr->spell_flags[j] & PY_SPELL_LEARNED)
		{
			continue;
		}

		/* Count it */
		k++;
	}

	/* Cannot learn more spells than exist */
	if (p_ptr->new_spells > k) p_ptr->new_spells = k;

	/* Spell count changed */
	if (old_spells != p_ptr->new_spells)
	{
		/* Message if needed */
		if (p_ptr->new_spells)
		{
			/* Message */
			msg_format("You can learn %d more %s%s.",
			           p_ptr->new_spells, p,
			           (p_ptr->new_spells != 1) ? "s" : "");
		}

		/* Redraw Study Status, object recall */
		p_ptr->redraw |= (PR_STUDY | PR_OBJECT);
	}
}


/*
 * Calculate maximum mana.  You do not need to know any spells.
 * Note that mana is lowered by heavy (or inappropriate) armor.
 *
 * This function induces status messages.
 */
static void calc_mana(void)
{
	/* Must be literate */
	if (!p_ptr->cp_ptr->spell_book) return;

	{	/* C-ish blocking brace */
	int msp;

	/* Determine the weight allowance */
	const int max_wgt = p_ptr->cp_ptr->spell_weight;

	int cur_wgt = 0;	/* Prepare to weigh the armor */

	/* Extract "effective" player level */
	int levels = (p_ptr->lev - p_ptr->cp_ptr->spell_first) + 1;

	const bool old_cumber_glove = p_ptr->cumber_glove;
	const bool old_cumber_armor = p_ptr->cumber_armor;

	if (0 < levels) 
 	{ 
		msp = 1; 
		msp += adj_mag_mana[p_ptr->stat_ind[p_ptr->cp_ptr->spell_stat]] * levels / 100; 
	} 
	else 
	{ 
		levels = 0; 
		msp = 0; 
	} 

	/* Process gloves for those disturbed by them */
	if (p_ptr->cp_ptr->flags & CF_CUMBER_GLOVE)
	{
		u32b f[OBJECT_FLAG_STRICT_UB];

		/* Get the gloves */
		const object_type* const o_ptr = &p_ptr->inventory[INVEN_HANDS];

		/* Assume player is not encumbered by gloves */
		p_ptr->cumber_glove = FALSE;

		if (o_ptr->k_idx)
		{	/* Examine the gloves */
			object_flags(o_ptr, f);

			/* Normal gloves hurt mage-type spells */
			if (	!(f[2] & (TR3_FREE_ACT))
				 && !((f[0] & (TR1_DEX)) && (o_ptr->pval > 0)))
			{
				p_ptr->cumber_glove = TRUE;	/* Encumbered */
				msp = (3 * msp) / 4;		/* Reduce mana */
			}
		}
	}


	/* Assume player not encumbered by armor */
	p_ptr->cumber_armor = FALSE;

	/* Weigh the armor */
	cur_wgt += p_ptr->inventory[INVEN_BODY].weight;
	cur_wgt += p_ptr->inventory[INVEN_HEAD].weight;
	cur_wgt += p_ptr->inventory[INVEN_ARM].weight;
	cur_wgt += p_ptr->inventory[INVEN_OUTER].weight;
	cur_wgt += p_ptr->inventory[INVEN_HANDS].weight;
	cur_wgt += p_ptr->inventory[INVEN_FEET].weight;

	/* Heavy armor penalizes mana */
	if (((cur_wgt - max_wgt) / 10) > 0)
	{
		p_ptr->cumber_armor = TRUE;			/* Encumbered */
		msp -= ((cur_wgt - max_wgt) / 10);	/* Reduce mana */
	}


	/* Mana can never be negative */
	if (msp < 0) msp = 0;


	/* Maximum mana has changed */
	if (p_ptr->msp != msp)
	{
		/* Save new limit */
		p_ptr->msp = msp;

		/* Enforce new limit */
		if (p_ptr->csp >= msp)
		{
			p_ptr->csp = msp;
			p_ptr->csp_frac = 0;
		}

		/* Display mana later */
		p_ptr->redraw |= (PR_MANA);
	}


	/* Hack -- handle "xtra" mode */
	if (character_xtra) return;

	/* Take note when "glove state" changes */
	if (old_cumber_glove != p_ptr->cumber_glove)
	{
		msg_print((p_ptr->cumber_glove) ? "Your covered hands feel unsuitable for spellcasting."
										: "Your hands feel more suitable for spellcasting.");
	}


	/* Take note when "armor state" changes */
	if (old_cumber_armor != p_ptr->cumber_armor)
	{
		msg_print((p_ptr->cumber_armor)	? "The weight of your armor encumbers your movement."
										: "You feel able to move more freely.");
	}
	}	/* end C-ish blocking brace */
}



/*
 * Calculate the players (maximal) hit points
 *
 * Adjust current hitpoints if necessary
 */
static void calc_hitpoints(void)
{
	/* Get "1/100th hitpoint bonus per level" value */
	const long bonus = adj_con_mhp[p_ptr->stat_ind[A_CON]];

	/* Calculate hitpoints */
	int mhp = p_ptr->player_hp[p_ptr->lev-1] + (bonus * p_ptr->lev / 100);

	/* Always have at least one hitpoint per level */
	if (mhp < p_ptr->lev + 1) mhp = p_ptr->lev + 1;

	/* New maximum hitpoints */
	if (p_ptr->mhp != mhp)
	{	
		p_ptr->mhp = mhp;			/* Save new limit */

		if (p_ptr->chp >= mhp)		/* Enforce new limit */
		{
			p_ptr->chp = mhp;
			p_ptr->chp_frac = 0;
		}
		
		p_ptr->redraw |= (PR_HP);	/* Display hitpoints (later) */
	}
}



/*
 * Extract and set the current "lite radius"
 */
static void calc_torch(void)
{
	u32b f[OBJECT_FLAG_STRICT_UB];
	int i;
	const s16b old_lite = p_ptr->cur_lite;


	/* Assume no light */
	p_ptr->cur_lite = 0;

	/* Loop through all wielded items */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		const object_type* const o_ptr = &p_ptr->inventory[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Examine actual lites */
		if (o_ptr->obj_id.tval == TV_LITE)
		{
			/* Artifact Lites provide permanent, bright, lite */
			if (o_ptr->is_artifact())
			{
				p_ptr->cur_lite += 3;
				continue;
			}
			
			if (0 < o_ptr->pval)
			{
				/* Lanterns (with fuel) provide more lite */
				if (o_ptr->obj_id.sval == SV_LITE_LANTERN)
				{
					p_ptr->cur_lite += 2;
					continue;
				}
			
				/* Torches (with fuel) provide some lite */
				if (o_ptr->obj_id.sval == SV_LITE_TORCH)
				{
					p_ptr->cur_lite += 1;
					continue;
				}
			}
		}
		else
		{
			/* Extract the flags */
			object_flags(o_ptr, f);

			/* does this item glow? */
			if (f[2] & TR3_LITE) p_ptr->cur_lite++;
		}
	}


	/* Player is glowing */
	if (p_ptr->lite) p_ptr->cur_lite++;


	/* Reduce lite when running if requested */
	if (p_ptr->running && OPTION(view_reduce_lite))
	{
		/* Reduce the lite radius if needed */
		if (p_ptr->cur_lite > 1) p_ptr->cur_lite = 1;
	}

	/* Notice changes in the "lite radius" */
	if (old_lite != p_ptr->cur_lite)
	{
		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	}
}


/*
 * Computes current weight limit.
 */
static int weight_limit(void)
{
	/* Weight limit based only on strength */
	int i = adj_str_wgt[p_ptr->stat_ind[A_STR]] * 100;

	/* Return the result */
	return (i);
}

/* synchronize against: calc_bonuses */
void player_missile_shot_analysis(const object_type& o, s16b str, s16b& num_fire, byte& ammo_tval, byte& ammo_mult)
{
	assert(o.k_idx);
	assert(TV_BOW==o.obj_id.tval);
	assert(SV_SLING==o.obj_id.sval || SV_SHORT_BOW==o.obj_id.sval || SV_LONG_BOW==o.obj_id.sval || SV_LIGHT_XBOW==o.obj_id.sval || SV_HEAVY_XBOW==o.obj_id.sval);
	u32b f[OBJECT_FLAG_STRICT_UB];
	int hold = adj_str_hold[str];	/* Obtain the "hold" value */
	int extra_shots = 0;
	int extra_might = 0;
	bool heavy_shoot = (hold < o.weight/10);

	/* Extract the item flags */
	object_flags(&o, f);

	/* Affect shots */
	if (f[0] & (TR1_SHOTS)) extra_shots = o.pval;

	/* Affect Might */
	if (f[0] & (TR1_MIGHT)) extra_might = o.pval;

	num_fire = 1;
	
	switch(o.obj_id.sval)
	{
		/* Sling and ammo */
		case SV_SLING:
		{
			ammo_tval = TV_SHOT;
			ammo_mult = 2;
			break;
		}

		/* Short Bow and Arrow */
		case SV_SHORT_BOW:
		{
			ammo_tval = TV_ARROW;
			ammo_mult = 2;
			break;
		}

		/* Long Bow and Arrow */
		case SV_LONG_BOW:
		{
			ammo_tval = TV_ARROW;
			ammo_mult = 3;
			break;
		}

		/* Light Crossbow and Bolt */
		case SV_LIGHT_XBOW:
		{
			ammo_tval = TV_BOLT;
			ammo_mult = 3;
			break;
		}

		/* Heavy Crossbow and Bolt */
		case SV_HEAVY_XBOW:
		{
			ammo_tval = TV_BOLT;
			ammo_mult = 4;
			break;
		}
		default:
		{
			assert(0 && "SV out of range");
			return;
		}
	}

	/* Apply special flags */
	if (!heavy_shoot)
	{
		num_fire += extra_shots;	/* Extra shots */
		ammo_mult += extra_might;	/* Extra might */

		/* Hack -- Rangers love Bows */
		if ((p_ptr->cp_ptr->flags & CF_EXTRA_SHOT) && (TV_ARROW == p_ptr->ammo_tval))
		{
			/* Extra shot at level 20 */
			if (p_ptr->lev >= 20) ++num_fire;

			/* Extra shot at level 40 */
			if (p_ptr->lev >= 40) ++num_fire;
		}
	}

	/* Require at least one shot */
	if (num_fire < 1) num_fire = 1;
}

/* synchronize against: calc_bonuses */
void player_melee_blow_analysis(const object_type& o, s16b str, s16b dex, s16b& num_blow)
{
	assert(o.k_idx);
	assert(o.obj_id.tval==TV_DIGGING || o.obj_id.tval==TV_HAFTED || o.obj_id.tval==TV_POLEARM || o.obj_id.tval==TV_SWORD);
	u32b f[OBJECT_FLAG_STRICT_UB];
	int hold = adj_str_hold[str];	/* Obtain the "hold" value */
	int extra_blows = 0;
	bool heavy_wield = (hold < o.weight/10);

	/* Extract the item flags */
	object_flags(&o, f);

	/* Affect shots */
	if (f[0] & (TR1_BLOWS)) extra_blows = o.pval;

	num_blow = 1;

	/*** Analyze weapon ***/

	/* Normal weapons */
	if (!heavy_wield)
	{
		/* Enforce a minimum "weight" (tenth pounds) */
		int div = MAX(o.weight, p_ptr->cp_ptr->min_weight);

		/* Get the strength vs weight */
		int str_index = (adj_str_blow[str] * p_ptr->cp_ptr->att_multiply / div);

		/* Index by dexterity */
		int dex_index = (adj_dex_blow[dex]);

		if (str_index > 11) str_index = 11;	/* Maximal value */
		if (dex_index > 11) dex_index = 11;	/* Maximal value */

		/* Use the blows table */
		num_blow = blows_table[str_index][dex_index];

		/* Maximal value */
		if (num_blow > p_ptr->cp_ptr->max_attacks) num_blow = p_ptr->cp_ptr->max_attacks;

		num_blow += extra_blows;	/* Add in the "bonus blows" */

		/* Require at least one blow */
		if (num_blow < 1) num_blow = 1;
	}
}

/*
 * Calculate the players current "state", taking into account
 * not only race/class intrinsics, but also objects being worn
 * and temporary spell effects.
 *
 * See also calc_mana() and calc_hitpoints().
 *
 * Take note of the new "speed code", in particular, a very strong
 * player will start slowing down as soon as he reaches 150 pounds,
 * but not until he reaches 450 pounds will he be half as fast as
 * a normal kobold.  This both hurts and helps the player, hurts
 * because in the old days a player could just avoid 300 pounds,
 * and helps because now carrying 300 pounds is not very painful.
 *
 * The "weapon" and "bow" do *not* add to the bonuses to hit or to
 * damage, since that would affect non-combat things.  These values
 * are actually added in later, at the appropriate place.
 *
 * This function induces various "status" messages.
 */
static void calc_bonuses(void)
{
	int i, j, hold;
	int tmp_speed = 110;

	int old_speed = p_ptr->speed;

	int old_telepathy = p_ptr->telepathy;
	int old_see_inv = p_ptr->see_inv;

	int old_dis_ac = p_ptr->dis_ac;
	int old_dis_to_a = p_ptr->dis_to_a;

	int extra_blows = 0;
	int extra_shots = 0;
	int extra_might = 0;

	int old_stat_top[A_MAX];
	int old_stat_use[A_MAX];
	int old_stat_ind[A_MAX];

	bool old_heavy_shoot = p_ptr->heavy_shoot;
	bool old_heavy_wield = p_ptr->heavy_wield;
	bool old_icky_wield = p_ptr->icky_wield;

	const object_type *o_ptr;

	u32b f[OBJECT_FLAG_STRICT_UB];


	/*** Memorize ***/

	/* Save the old stats */
	for (i = 0; i < A_MAX; i++)
	{
		old_stat_top[i] = p_ptr->stat_top[i];
		old_stat_use[i] = p_ptr->stat_use[i];
		old_stat_ind[i] = p_ptr->stat_ind[i];
	}

	/*** Reset ***/

	/* Reset player speed */
	p_ptr->speed = 110;

	/* Reset "blow" info */
	p_ptr->num_blow = 1;

	/* Reset "fire" info */
	p_ptr->num_fire = 0;
	p_ptr->ammo_mult = 0;
	p_ptr->ammo_tval = 0;

	/* Clear the stat modifiers */
	for (i = 0; i < A_MAX; i++) p_ptr->stat_add[i] = 0;

	/* Clear the Displayed/Real armor class */
	p_ptr->dis_ac = p_ptr->ac = 0;

	/* Clear the Displayed/Real Bonuses */
	p_ptr->dis_to_h = p_ptr->to_h = 0;
	p_ptr->dis_to_d = p_ptr->to_d = 0;
	p_ptr->dis_to_a = p_ptr->to_a = 0;

	/* Clear all the flags */
	p_ptr->aggravate = FALSE;
	p_ptr->teleport = FALSE;
	p_ptr->exp_drain = FALSE;
	p_ptr->bless_blade = FALSE;
	p_ptr->impact = FALSE;
	p_ptr->see_inv = FALSE;
	p_ptr->free_act = FALSE;
	p_ptr->slow_digest = FALSE;
	p_ptr->regenerate = FALSE;
	p_ptr->ffall = FALSE;
	p_ptr->hold_life = FALSE;
	p_ptr->telepathy = FALSE;
	p_ptr->lite = FALSE;
	p_ptr->sustain[A_STR] = FALSE;
	p_ptr->sustain[A_INT] = FALSE;
	p_ptr->sustain[A_WIS] = FALSE;
	p_ptr->sustain[A_DEX] = FALSE;
	p_ptr->sustain[A_CON] = FALSE;
	p_ptr->sustain[A_CHR] = FALSE;
	p_ptr->resist_acid = FALSE;
	p_ptr->resist_elec = FALSE;
	p_ptr->resist_fire = FALSE;
	p_ptr->resist_cold = FALSE;
	p_ptr->resist_pois = FALSE;
	p_ptr->resist_fear = FALSE;
	p_ptr->resist_lite = FALSE;
	p_ptr->resist_dark = FALSE;
	p_ptr->resist_blind = FALSE;
	p_ptr->resist_confu = FALSE;
	p_ptr->resist_sound = FALSE;
	p_ptr->resist_chaos = FALSE;
	p_ptr->resist_disen = FALSE;
	p_ptr->resist_shard = FALSE;
	p_ptr->resist_nexus = FALSE;
	p_ptr->resist_nethr = FALSE;
	p_ptr->immune_acid = FALSE;
	p_ptr->immune_elec = FALSE;
	p_ptr->immune_fire = FALSE;
	p_ptr->immune_cold = FALSE;


	/*** Extract race/class info ***/

	/* Base infravision (purely racial) */
	p_ptr->see_infra = p_ptr->rp_ptr->infra;

	/* Base skills */
	for(i=0;i<SKILL_MAX-2;++i)
    {
		p_ptr->skills[i] = p_ptr->rp_ptr->r_skills[i] + p_ptr->cp_ptr->c_skills[i];
    }

	/* Base skill -- combat (throwing) */
	p_ptr->skills[SKILL_TO_HIT_THROW] = p_ptr->skills[SKILL_TO_HIT_BOW];

	/* Base skill -- digging */
	p_ptr->skills[SKILL_DIGGING] = 0;

	/*** Analyze player ***/

	/* Extract the player flags */
	p_ptr->flags(f);

	/* Good flags */
	if (f[2] & (TR3_SLOW_DIGEST)) p_ptr->slow_digest = TRUE;
	if (f[2] & (TR3_FEATHER)) p_ptr->ffall = TRUE;
	if (f[2] & (TR3_LITE)) p_ptr->lite = TRUE;
	if (f[2] & (TR3_REGEN)) p_ptr->regenerate = TRUE;
	if (f[2] & (TR3_TELEPATHY)) p_ptr->telepathy = TRUE;
	if (f[2] & (TR3_SEE_INVIS)) p_ptr->see_inv = TRUE;
	if (f[2] & (TR3_FREE_ACT)) p_ptr->free_act = TRUE;
	if (f[2] & (TR3_HOLD_LIFE)) p_ptr->hold_life = TRUE;

	/* Weird flags */
	if (f[2] & (TR3_BLESSED)) p_ptr->bless_blade = TRUE;

	/* Bad flags */
	if (f[2] & (TR3_IMPACT)) p_ptr->impact = TRUE;
	if (f[2] & (TR3_AGGRAVATE)) p_ptr->aggravate = TRUE;
	if (f[2] & (TR3_TELEPORT)) p_ptr->teleport = TRUE;
	if (f[2] & (TR3_DRAIN_EXP)) p_ptr->exp_drain = TRUE;

	/* Immunity flags */
	if (f[1] & (TR2_IM_FIRE)) p_ptr->immune_fire = TRUE;
	if (f[1] & (TR2_IM_ACID)) p_ptr->immune_acid = TRUE;
	if (f[1] & (TR2_IM_COLD)) p_ptr->immune_cold = TRUE;
	if (f[1] & (TR2_IM_ELEC)) p_ptr->immune_elec = TRUE;

	/* Resistance flags */
	if (f[1] & (TR2_RES_ACID)) p_ptr->resist_acid = TRUE;
	if (f[1] & (TR2_RES_ELEC)) p_ptr->resist_elec = TRUE;
	if (f[1] & (TR2_RES_FIRE)) p_ptr->resist_fire = TRUE;
	if (f[1] & (TR2_RES_COLD)) p_ptr->resist_cold = TRUE;
	if (f[1] & (TR2_RES_POIS)) p_ptr->resist_pois = TRUE;
	if (f[1] & (TR2_RES_FEAR)) p_ptr->resist_fear = TRUE;
	if (f[1] & (TR2_RES_LITE)) p_ptr->resist_lite = TRUE;
	if (f[1] & (TR2_RES_DARK)) p_ptr->resist_dark = TRUE;
	if (f[1] & (TR2_RES_BLIND)) p_ptr->resist_blind = TRUE;
	if (f[1] & (TR2_RES_CONFU)) p_ptr->resist_confu = TRUE;
	if (f[1] & (TR2_RES_SOUND)) p_ptr->resist_sound = TRUE;
	if (f[1] & (TR2_RES_SHARD)) p_ptr->resist_shard = TRUE;
	if (f[1] & (TR2_RES_NEXUS)) p_ptr->resist_nexus = TRUE;
	if (f[1] & (TR2_RES_NETHR)) p_ptr->resist_nethr = TRUE;
	if (f[1] & (TR2_RES_CHAOS)) p_ptr->resist_chaos = TRUE;
	if (f[1] & (TR2_RES_DISEN)) p_ptr->resist_disen = TRUE;

	/* Sustain flags */
	if (f[1] & (TR2_SUST_STR)) p_ptr->sustain[A_STR] = TRUE;
	if (f[1] & (TR2_SUST_INT)) p_ptr->sustain[A_INT] = TRUE;
	if (f[1] & (TR2_SUST_WIS)) p_ptr->sustain[A_WIS] = TRUE;
	if (f[1] & (TR2_SUST_DEX)) p_ptr->sustain[A_DEX] = TRUE;
	if (f[1] & (TR2_SUST_CON)) p_ptr->sustain[A_CON] = TRUE;
	if (f[1] & (TR2_SUST_CHR)) p_ptr->sustain[A_CHR] = TRUE;


	/*** Analyze equipment ***/

	/* Scan the equipment */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
		o_ptr = &p_ptr->inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Extract the item flags */
		object_flags(o_ptr, f);

		/* Affect stats */
		if (f[0] & (TR1_STR)) p_ptr->stat_add[A_STR] += o_ptr->pval;
		if (f[0] & (TR1_INT)) p_ptr->stat_add[A_INT] += o_ptr->pval;
		if (f[0] & (TR1_WIS)) p_ptr->stat_add[A_WIS] += o_ptr->pval;
		if (f[0] & (TR1_DEX)) p_ptr->stat_add[A_DEX] += o_ptr->pval;
		if (f[0] & (TR1_CON)) p_ptr->stat_add[A_CON] += o_ptr->pval;
		if (f[0] & (TR1_CHR)) p_ptr->stat_add[A_CHR] += o_ptr->pval;

		/* Affect stealth */
		if (f[0] & (TR1_STEALTH)) p_ptr->skills[SKILL_STEALTH] += o_ptr->pval;

		/* Affect searching ability (factor of five) */
		if (f[0] & (TR1_SEARCH)) p_ptr->skills[SKILL_SEARCH] += (o_ptr->pval * 5);

		/* Affect searching frequency (factor of five) */
		if (f[0] & (TR1_SEARCH)) p_ptr->skills[SKILL_SEARCH_FREQUENCY] += (o_ptr->pval * 5);

		/* Affect infravision */
		if (f[0] & (TR1_INFRA)) p_ptr->see_infra += o_ptr->pval;

		/* Affect digging (factor of 20) */
		if (f[0] & (TR1_TUNNEL)) p_ptr->skills[SKILL_DIGGING] += (o_ptr->pval * 20);

		/* Affect speed */
		if (f[0] & (TR1_SPEED)) tmp_speed += o_ptr->pval;

		/* Affect blows */
		if (f[0] & (TR1_BLOWS)) extra_blows += o_ptr->pval;

		/* Affect shots */
		if (f[0] & (TR1_SHOTS)) extra_shots += o_ptr->pval;

		/* Affect Might */
		if (f[0] & (TR1_MIGHT)) extra_might += o_ptr->pval;

		/* Good flags */
		if (f[2] & (TR3_SLOW_DIGEST)) p_ptr->slow_digest = TRUE;
		if (f[2] & (TR3_FEATHER)) p_ptr->ffall = TRUE;
		if (f[2] & (TR3_REGEN)) p_ptr->regenerate = TRUE;
		if (f[2] & (TR3_TELEPATHY)) p_ptr->telepathy = TRUE;
		if (f[2] & (TR3_SEE_INVIS)) p_ptr->see_inv = TRUE;
		if (f[2] & (TR3_FREE_ACT)) p_ptr->free_act = TRUE;
		if (f[2] & (TR3_HOLD_LIFE)) p_ptr->hold_life = TRUE;

		/* Weird flags */
		if (f[2] & (TR3_BLESSED)) p_ptr->bless_blade = TRUE;

		/* Bad flags */
		if (f[2] & (TR3_IMPACT)) p_ptr->impact = TRUE;
		if (f[2] & (TR3_AGGRAVATE)) p_ptr->aggravate = TRUE;
		if (f[2] & (TR3_TELEPORT)) p_ptr->teleport = TRUE;
		if (f[2] & (TR3_DRAIN_EXP)) p_ptr->exp_drain = TRUE;

		/* Immunity flags */
		if (f[1] & (TR2_IM_FIRE)) p_ptr->immune_fire = TRUE;
		if (f[1] & (TR2_IM_ACID)) p_ptr->immune_acid = TRUE;
		if (f[1] & (TR2_IM_COLD)) p_ptr->immune_cold = TRUE;
		if (f[1] & (TR2_IM_ELEC)) p_ptr->immune_elec = TRUE;

		/* Resistance flags */
		if (f[1] & (TR2_RES_ACID)) p_ptr->resist_acid = TRUE;
		if (f[1] & (TR2_RES_ELEC)) p_ptr->resist_elec = TRUE;
		if (f[1] & (TR2_RES_FIRE)) p_ptr->resist_fire = TRUE;
		if (f[1] & (TR2_RES_COLD)) p_ptr->resist_cold = TRUE;
		if (f[1] & (TR2_RES_POIS)) p_ptr->resist_pois = TRUE;
		if (f[1] & (TR2_RES_FEAR)) p_ptr->resist_fear = TRUE;
		if (f[1] & (TR2_RES_LITE)) p_ptr->resist_lite = TRUE;
		if (f[1] & (TR2_RES_DARK)) p_ptr->resist_dark = TRUE;
		if (f[1] & (TR2_RES_BLIND)) p_ptr->resist_blind = TRUE;
		if (f[1] & (TR2_RES_CONFU)) p_ptr->resist_confu = TRUE;
		if (f[1] & (TR2_RES_SOUND)) p_ptr->resist_sound = TRUE;
		if (f[1] & (TR2_RES_SHARD)) p_ptr->resist_shard = TRUE;
		if (f[1] & (TR2_RES_NEXUS)) p_ptr->resist_nexus = TRUE;
		if (f[1] & (TR2_RES_NETHR)) p_ptr->resist_nethr = TRUE;
		if (f[1] & (TR2_RES_CHAOS)) p_ptr->resist_chaos = TRUE;
		if (f[1] & (TR2_RES_DISEN)) p_ptr->resist_disen = TRUE;

		/* Sustain flags */
		if (f[1] & (TR2_SUST_STR)) p_ptr->sustain[A_STR] = TRUE;
		if (f[1] & (TR2_SUST_INT)) p_ptr->sustain[A_INT] = TRUE;
		if (f[1] & (TR2_SUST_WIS)) p_ptr->sustain[A_WIS] = TRUE;
		if (f[1] & (TR2_SUST_DEX)) p_ptr->sustain[A_DEX] = TRUE;
		if (f[1] & (TR2_SUST_CON)) p_ptr->sustain[A_CON] = TRUE;
		if (f[1] & (TR2_SUST_CHR)) p_ptr->sustain[A_CHR] = TRUE;

		/* Modify the base armor class */
		p_ptr->ac += o_ptr->ac;

		/* The base armor class is always known */
		p_ptr->dis_ac += o_ptr->ac;

		/* Apply the bonuses to armor class */
		p_ptr->to_a += o_ptr->to_a;

		/* Apply the mental bonuses to armor class, if known */
		if (o_ptr->known()) p_ptr->dis_to_a += o_ptr->to_a;

		/* Hack -- do not apply "weapon" bonuses */
		if (i == INVEN_WIELD) continue;

		/* Hack -- do not apply "bow" bonuses */
		if (i == INVEN_BOW) continue;

		/* Apply the bonuses to hit/damage */
		p_ptr->to_h += o_ptr->to_h;
		p_ptr->to_d += o_ptr->to_d;

		/* Apply the mental bonuses to hit/damage, if known */
		if (o_ptr->known())
			{
			p_ptr->dis_to_h += o_ptr->to_h;
			p_ptr->dis_to_d += o_ptr->to_d;
			};
	}


	/*** Handle stats ***/

	/* Calculate stats */
	for (i = 0; i < A_MAX; i++)
	{
		int add, top, use, ind;

		/* Extract modifier */
		add = p_ptr->stat_add[i];

		/* Maximize mode */
		if (OPTION(adult_maximize))
		{
			/* Modify the stats for race/class */
			add += (p_ptr->rp_ptr->r_adj[i] + p_ptr->cp_ptr->c_adj[i]);
		}

		/* Extract the new "stat_top" value for the stat */
		top = modify_stat_value(p_ptr->stat_max[i], add);

		/* Save the new value */
		p_ptr->stat_top[i] = top;

		/* Extract the new "stat_use" value for the stat */
		use = modify_stat_value(p_ptr->stat_cur[i], add);

		/* Save the new value */
		p_ptr->stat_use[i] = use;

		/* Values: 3, 4, ..., 17 */
		if (use <= 18) ind = (use - 3);

		/* Ranges: 18/00-18/09, ..., 18/210-18/219 */
		else if (use <= 18+219) ind = (15 + (use - 18) / 10);

		/* Range: 18/220+ */
		else ind = (37);

		/* Save the new index */
		p_ptr->stat_ind[i] = ind;
	}


	/*** Temporary flags ***/

	/* Apply temporary "stun" */
	if (p_ptr->timed[TMD_STUN] > 50)
	{
		p_ptr->to_h -= 20;
		p_ptr->dis_to_h -= 20;
		p_ptr->to_d -= 20;
		p_ptr->dis_to_d -= 20;
	}
	else if (p_ptr->timed[TMD_STUN])
	{
		p_ptr->to_h -= 5;
		p_ptr->dis_to_h -= 5;
		p_ptr->to_d -= 5;
		p_ptr->dis_to_d -= 5;
	}

	/* Invulnerability */
	if (p_ptr->timed[TMD_INVULN]) p_ptr->mental_to_a_adj(100);

	/* Temporary blessing */
	if (p_ptr->timed[TMD_BLESSED])
	{
		p_ptr->mental_to_a_adj(5);
		p_ptr->mental_to_h_adj(10);
	}

	/* Temporary shield */
	if (p_ptr->timed[TMD_SHIELD]) p_ptr->mental_to_a_adj(50);

	/* Temporary "Hero" */
	if (p_ptr->timed[TMD_HERO]) p_ptr->mental_to_h_adj(12);

	/* Temporary "Berserk" */
	if (p_ptr->timed[TMD_SHERO])
	{
		p_ptr->mental_to_a_adj(-10);
		p_ptr->mental_to_h_adj(24);
	}

	/* Temporary "fast" */
	if (p_ptr->timed[TMD_FAST]) tmp_speed += 10;

	/* Temporary "slow" */
	if (p_ptr->timed[TMD_SLOW]) tmp_speed -= 10;

	/* Temporary see invisible */
	if (p_ptr->timed[TMD_SINVIS]) p_ptr->see_inv = TRUE;

	/* Temporary infravision boost */
	if (p_ptr->timed[TMD_SINFRA]) p_ptr->see_infra += 5;


	/*** Special flags ***/

	/* Hack -- Hero/Shero -> Res fear */
	if (p_ptr->timed[TMD_HERO] || p_ptr->timed[TMD_SHERO])
	{
		p_ptr->resist_fear = TRUE;
	}


	/*** Analyze weight ***/

	/* Extract the current weight (in tenth pounds) */
	j = p_ptr->total_weight;

	/* Extract the "weight limit" (in tenth pounds) */
	i = weight_limit();

	/* Apply "encumbrance" from weight */
	if (j > i / 2) tmp_speed -= ((j - (i / 2)) / (i / 10));

	/* Bloating slows the player down (a little) */
	if (p_ptr->food >= PY_FOOD_MAX) tmp_speed -= 10;

	/* Searching slows the player down */
	if (p_ptr->searching) tmp_speed -= 10;

	/* Sanity check on extreme speeds */
	if (tmp_speed < 0) tmp_speed = 0;
	if (tmp_speed > 199) tmp_speed = 199;
	p_ptr->speed = tmp_speed;

	/*** Apply modifier bonuses ***/

	/* Actual and Displayed Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->mental_to_a_adj((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->mental_to_h_adj((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
	p_ptr->mental_to_h_adj((int)(adj_str_th[p_ptr->stat_ind[A_STR]]) - 128);

	/* Actual Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->to_d += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);

	/* Displayed Modifier Bonuses (Un-inflate stat bonuses) */
	p_ptr->dis_to_d += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);


	/*** Modify skills ***/

	/* Affect Skill -- stealth (bonus one) */
	p_ptr->skills[SKILL_STEALTH] += 1;

	/* Affect Skill -- disarming (DEX and INT) */
	p_ptr->skills[SKILL_DISARM] += adj_dex_dis[p_ptr->stat_ind[A_DEX]];
	p_ptr->skills[SKILL_DISARM] += adj_int_dis[p_ptr->stat_ind[A_INT]];

	/* Affect Skill -- magic devices (INT) */
	p_ptr->skills[SKILL_DEVICE] += adj_int_dev[p_ptr->stat_ind[A_INT]];

	/* Affect Skill -- saving throw (WIS) */
	p_ptr->skills[SKILL_SAVE] += adj_wis_sav[p_ptr->stat_ind[A_WIS]];

	/* Affect Skill -- digging (STR) */
	p_ptr->skills[SKILL_DIGGING] += adj_str_dig[p_ptr->stat_ind[A_STR]];

	/* Affect Skills (Level, by Class */
	for(i=0;i<SKILL_MAX-2;++i)
    {
		p_ptr->skills[i] += (p_ptr->cp_ptr->x_skills[i] * p_ptr->lev / 10);
    }

	/* Affect Skill -- combat (throwing) (Level, by Class) */
	p_ptr->skills[SKILL_TO_HIT_THROW] += (p_ptr->cp_ptr->x_skills[SKILL_TO_HIT_BOW] * p_ptr->lev / 10);

	/* Limit Skill -- digging from 1 up */
	if (p_ptr->skills[SKILL_DIGGING] < 1) p_ptr->skills[SKILL_DIGGING] = 1;

	/* Limit Skill -- stealth from 0 to 30 */
	if (p_ptr->skills[SKILL_STEALTH] > 30) p_ptr->skills[SKILL_STEALTH] = 30;
	if (p_ptr->skills[SKILL_STEALTH] < 0) p_ptr->skills[SKILL_STEALTH] = 0;

	/* Apply Skill -- Extract noise from stealth */
	p_ptr->noise = (1L << (30 - p_ptr->skills[SKILL_STEALTH]));

	/* Obtain the "hold" value */
	hold = adj_str_hold[p_ptr->stat_ind[A_STR]];


	/*** Analyze current bow ***/

	/* Examine the "current bow" */
	o_ptr = &p_ptr->inventory[INVEN_BOW];

	/* Assume not heavy */
	p_ptr->heavy_shoot = FALSE;

	/* It is hard to carholdry a heavy bow */
	if (hold < o_ptr->weight / 10)
	{
		/* Hard to wield a heavy bow */
		p_ptr->mental_to_h_adj(2 * (hold - o_ptr->weight / 10));

		/* Heavy Bow */
		p_ptr->heavy_shoot = TRUE;
	}

	/* Analyze launcher */
	if (o_ptr->k_idx)
	{
		/* Get to shoot */
		p_ptr->num_fire = 1;

		/* Analyze the launcher */
		switch (o_ptr->obj_id.sval)
		{
			/* Sling and ammo */
			case SV_SLING:
			{
				p_ptr->ammo_tval = TV_SHOT;
				p_ptr->ammo_mult = 2;
				break;
			}

			/* Short Bow and Arrow */
			case SV_SHORT_BOW:
			{
				p_ptr->ammo_tval = TV_ARROW;
				p_ptr->ammo_mult = 2;
				break;
			}

			/* Long Bow and Arrow */
			case SV_LONG_BOW:
			{
				p_ptr->ammo_tval = TV_ARROW;
				p_ptr->ammo_mult = 3;
				break;
			}

			/* Light Crossbow and Bolt */
			case SV_LIGHT_XBOW:
			{
				p_ptr->ammo_tval = TV_BOLT;
				p_ptr->ammo_mult = 3;
				break;
			}

			/* Heavy Crossbow and Bolt */
			case SV_HEAVY_XBOW:
			{
				p_ptr->ammo_tval = TV_BOLT;
				p_ptr->ammo_mult = 4;
				break;
			}
		}

		/* Apply special flags */
		if (o_ptr->k_idx && !p_ptr->heavy_shoot)
		{
			/* Extra shots */
			p_ptr->num_fire += extra_shots;

			/* Extra might */
			p_ptr->ammo_mult += extra_might;

			/* Hack -- Rangers love Bows */
			if ((p_ptr->cp_ptr->flags & CF_EXTRA_SHOT) &&
			    (p_ptr->ammo_tval == TV_ARROW))
			{
				/* Extra shot at level 20 */
				if (p_ptr->lev >= 20) p_ptr->num_fire++;

				/* Extra shot at level 40 */
				if (p_ptr->lev >= 40) p_ptr->num_fire++;
			}
		}

		/* Require at least one shot */
		if (p_ptr->num_fire < 1) p_ptr->num_fire = 1;
	}


	/*** Analyze weapon ***/

	/* Examine the "current weapon" */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];

	/* Assume not heavy */
	p_ptr->heavy_wield = FALSE;

	/* It is hard to hold a heavy weapon */
	if (hold < o_ptr->weight / 10)
	{
		/* Hard to wield a heavy weapon */
		p_ptr->mental_to_h_adj(2 * (hold - o_ptr->weight / 10));

		/* Heavy weapon */
		p_ptr->heavy_wield = TRUE;
	}

	/* Normal weapons */
	if (o_ptr->k_idx && !p_ptr->heavy_wield)
	{
		/* Enforce a minimum "weight" (tenth pounds) */
		int div = MAX(o_ptr->weight, p_ptr->cp_ptr->min_weight);

		/* Get the strength vs weight */
		int str_index = (adj_str_blow[p_ptr->stat_ind[A_STR]] * p_ptr->cp_ptr->att_multiply / div);

		/* Index by dexterity */
		int dex_index = (adj_dex_blow[p_ptr->stat_ind[A_DEX]]);

		if (str_index > 11) str_index = 11;	/* Maximal value */
		if (dex_index > 11) dex_index = 11;	/* Maximal value */

		/* Use the blows table */
		p_ptr->num_blow = blows_table[str_index][dex_index];

		/* Maximal value */
		if (p_ptr->num_blow > p_ptr->cp_ptr->max_attacks) p_ptr->num_blow = p_ptr->cp_ptr->max_attacks;

		p_ptr->num_blow += extra_blows;	/* Add in the "bonus blows" */

		/* Require at least one blow */
		if (p_ptr->num_blow < 1) p_ptr->num_blow = 1;

		/* Boost digging skill by weapon weight */
		p_ptr->skills[SKILL_DIGGING] += (o_ptr->weight / 10);
	}

	p_ptr->icky_wield = FALSE;	/* Assume okay */

	/* Priest weapon penalty for non-blessed edged weapons */
	if ((p_ptr->cp_ptr->flags & CF_BLESS_WEAPON) && (!p_ptr->bless_blade) &&
	    ((o_ptr->obj_id.tval == TV_SWORD) || (o_ptr->obj_id.tval == TV_POLEARM)))
	{
		/* Reduce the real bonuses */
		p_ptr->to_h -= 2;
		p_ptr->to_d -= 2;

		/* Reduce the mental bonuses */
		p_ptr->dis_to_h -= 2;
		p_ptr->dis_to_d -= 2;

		/* Icky weapon */
		p_ptr->icky_wield = TRUE;
	}


	/*** Notice changes ***/

	/* Analyze stats */
	for (i = 0; i < A_MAX; i++)
	{
		/* Notice changes, redisplay stats later if changed */
		if (p_ptr->stat_top[i] != old_stat_top[i]) p_ptr->redraw |= (PR_STATS);
		if (p_ptr->stat_use[i] != old_stat_use[i]) p_ptr->redraw |= (PR_STATS);

		/* Notice changes */
		if (p_ptr->stat_ind[i] != old_stat_ind[i])
		{
			/* Change in CON affects Hitpoints */
			if (i == A_CON) p_ptr->update |= (PU_HP);

			/* Change in spell stat may affect Mana/Spells */
			if (i == p_ptr->cp_ptr->spell_stat)
			{
				p_ptr->update |= (PU_MANA | PU_SPELLS);
			}
		}
	}

	/* Telepathy Change: update monster visibility */
	if (p_ptr->telepathy != old_telepathy) p_ptr->update |= (PU_MONSTERS);

	/* See Invis Change: update monster visibility */
	if (p_ptr->see_inv != old_see_inv) p_ptr->update |= (PU_MONSTERS);

	/* Speed change: redraw speed */
	if (p_ptr->speed != old_speed) p_ptr->redraw |= (PR_SPEED);

	/* Redraw armor (if needed) */
	if ((p_ptr->dis_ac != old_dis_ac) || (p_ptr->dis_to_a != old_dis_to_a))
	{
		/* Redraw */
		p_ptr->redraw |= (PR_ARMOR);
	}

	/* Hack -- handle "xtra" mode */
	if (character_xtra) return;

	/* Take note when "heavy bow" changes */
	if (old_heavy_shoot != p_ptr->heavy_shoot)
	{
		/* Message */
		if (p_ptr->heavy_shoot)
		{
			msg_print("You have trouble wielding such a heavy bow.");
		}
		else if (p_ptr->inventory[INVEN_BOW].k_idx)
		{
			msg_print("You have no trouble wielding your bow.");
		}
		else
		{
			msg_print("You feel relieved to put down your heavy bow.");
		}
	}

	/* Take note when "heavy weapon" changes */
	if (old_heavy_wield != p_ptr->heavy_wield)
	{
		/* Message */
		if (p_ptr->heavy_wield)
		{
			msg_print("You have trouble wielding such a heavy weapon.");
		}
		else if (p_ptr->inventory[INVEN_WIELD].k_idx)
		{
			msg_print("You have no trouble wielding your weapon.");
		}
		else
		{
			msg_print("You feel relieved to put down your heavy weapon.");	
		}
	}

	/* Take note when "illegal weapon" changes */
	if (old_icky_wield != p_ptr->icky_wield)
	{
		/* Message */
		if (p_ptr->icky_wield)
		{
			msg_print("You do not feel comfortable with your weapon.");
		}
		else if (p_ptr->inventory[INVEN_WIELD].k_idx)
		{
			msg_print("You feel comfortable with your weapon.");
		}
		else
		{
			msg_print("You feel more comfortable after removing your weapon.");
		}
	}
}



/*
 * Handle "p_ptr->notice"
 */
void notice_stuff(void)
{
	/* Notice stuff */
	if (!p_ptr->notice) return;


	/* Combine the pack */
	if (p_ptr->notice & (PN_COMBINE))
	{
		p_ptr->notice &= ~(PN_COMBINE);
		combine_pack();
	}

	/* Reorder the pack */
	if (p_ptr->notice & (PN_REORDER))
	{
		p_ptr->notice &= ~(PN_REORDER);
		reorder_pack();
	}
}


/*
 * Handle "p_ptr->update"
 */
void update_stuff(void)
{
	/* Update stuff */
	if (!p_ptr->update) return;


	if (p_ptr->update & (PU_BONUS))
	{
		p_ptr->update &= ~(PU_BONUS);
		calc_bonuses();
	}

	if (p_ptr->update & (PU_TORCH))
	{
		p_ptr->update &= ~(PU_TORCH);
		calc_torch();
	}

	if (p_ptr->update & (PU_HP))
	{
		p_ptr->update &= ~(PU_HP);
		calc_hitpoints();
	}

	if (p_ptr->update & (PU_MANA))
	{
		p_ptr->update &= ~(PU_MANA);
		calc_mana();
	}

	if (p_ptr->update & (PU_SPELLS))
	{
		p_ptr->update &= ~(PU_SPELLS);
		calc_spells();
	}


	/* Character is not ready yet, no screen updates */
	if (!character_generated) return;


	/* Character is in "icky" mode, no screen updates */
	if (character_icky) return;


	if (p_ptr->update & (PU_FORGET_VIEW))
	{
		p_ptr->update &= ~(PU_FORGET_VIEW);
		forget_view();
	}

	if (p_ptr->update & (PU_UPDATE_VIEW))
	{
		p_ptr->update &= ~(PU_UPDATE_VIEW);
		update_view();
	}


	if (p_ptr->update & (PU_FORGET_FLOW))
	{
		p_ptr->update &= ~(PU_FORGET_FLOW);
		forget_flow();
	}

	if (p_ptr->update & (PU_UPDATE_FLOW))
	{
		p_ptr->update &= ~(PU_UPDATE_FLOW);
		update_flow();
	}


	if (p_ptr->update & (PU_DISTANCE))
	{
		p_ptr->update &= ~(PU_DISTANCE);
		p_ptr->update &= ~(PU_MONSTERS);
		update_monsters(TRUE);
	}

	if (p_ptr->update & (PU_MONSTERS))
	{
		p_ptr->update &= ~(PU_MONSTERS);
		update_monsters(FALSE);
	}


	if (p_ptr->update & (PU_PANEL))
	{
		p_ptr->update &= ~(PU_PANEL);
		event_signal(EVENT_PLAYERMOVED);
	}
}

/*
 * Events triggered by the various flags.
 */
static const POD_pair<u32b,game_event_type> redraw_events[] =
{
	{ PR_MISC, EVENT_RACE_CLASS },
	{ PR_TITLE, EVENT_PLAYERTITLE },
	{ PR_LEV, EVENT_PLAYERLEVEL },
	{ PR_EXP, EVENT_EXPERIENCE },
	{ PR_GOLD, EVENT_GOLD },
	{ PR_STATS, EVENT_STATS },
	{ PR_ARMOR, EVENT_AC },
	{ PR_HP, EVENT_HP },
	{ PR_MANA, EVENT_MANA },
	{ PR_HEALTH, EVENT_MONSTERHEALTH },
	{ PR_DEPTH, EVENT_DUNGEONLEVEL },
	{ PR_SPEED, EVENT_SPEED },
	{ PR_STATE, EVENT_STATE },
	{ PR_STATUS, EVENT_STATUS },
	{ PR_STUDY, EVENT_STUDYSTATUS },
//	{ PR_DTRAP, EVENT_DETECTIONSTATUS },

	{ PR_INVEN, EVENT_INVENTORY },
	{ PR_EQUIP, EVENT_EQUIPMENT },
	{ PR_MONLIST, EVENT_MONSTERLIST },
	{ PR_MONSTER, EVENT_SPEED },
	{ PR_MONSTER, EVENT_MONSTERTARGET },
	{ PR_MESSAGE, EVENT_MESSAGES }
};

/*
 * Handle "p_ptr->redraw"
 */
void redraw_stuff(void)
{
	size_t i;
	
	/* Redraw stuff */
	if (!p_ptr->redraw) return;

	/* Character is not ready yet, no screen updates */
	if (!character_generated) return;

	/* Character is in "icky" mode, no screen updates */
	if (character_icky) return;

	/* For each listed flag, send the appropriate signal to the UI */
	for (i = 0; i < N_ELEMENTS(redraw_events); i++)
	{
		if (p_ptr->redraw & redraw_events[i].first)
			event_signal(redraw_events[i].second);
	}

	/* Then the ones that require parameters to be supplied. */
	if (p_ptr->redraw & PR_MAP)
	{
		/* Mark the whole map to be redrawn */
		event_signal_point(EVENT_MAP, -1, -1);
	}

	p_ptr->redraw = 0;

	/* 
	 * Do any plotting, etc. delayed from earlier - this set of updates
	 * is over. 
	 */
	event_signal(EVENT_END);
}

/*
 * Handle "p_ptr->update" and "p_ptr->redraw"
 */
void handle_stuff(void)
{
	/* Update stuff */
	if (p_ptr->update) update_stuff();

	/* Redraw stuff */
	if (p_ptr->redraw) redraw_stuff();
}
