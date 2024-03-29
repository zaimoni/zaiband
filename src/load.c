/* File: load.c */

/*
 * Copyright (c) 1997 Ben Harrison, and others
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
#include "z-msg.h"
#include "z-quark.h"
#include "ego.h"
#include "option.h"
#include "tvalsval.h"
#include "store.h"

/*
 * This file loads savefiles from Angband 2.9.X.
 *
 * We attempt to prevent corrupt savefiles from inducing memory errors.
 *
 * Note that this file should not use the random number generator, the
 * object flavors, the visual attr/char mappings, or anything else which
 * is initialized *after* or *during* the "load character" function.
 *
 * This file assumes that the monster/object records are initialized
 * to zero, and the race/kind tables have been loaded correctly.  The
 * order of object stacks is currently not saved in the savefiles, but
 * the "next" pointers are saved, so all necessary knowledge is present.
 *
 * We should implement simple "savefile extenders" using some form of
 * "sized" chunks of bytes, with a {size,type,data} format, so everyone
 * can know the size, interested people can know the type, and the actual
 * data is available to the parsing routines that acknowledge the type.
 *
 * Consider changing the "globe of invulnerability" code so that it
 * takes some form of "maximum damage to protect from" in addition to
 * the existing "number of turns to protect for", and where each hit
 * by a monster will reduce the shield by that amount.  XXX XXX XXX
 */

/*
 * Oldest version number that can still be imported
 */
#define OLD_VERSION_MAJOR	3
#define OLD_VERSION_MINOR	0
#define OLD_VERSION_PATCH	9


/*! Local "savefile" pointer */
static FILE	*fff;

/*! Hack -- old "encryption" byte */
static byte	xor_byte;

/*! Hack -- simple "checksum" on the actual values */
static u32b	v_check = 0L;

/*! Hack -- simple "checksum" on the encoded bytes */
static u32b	x_check = 0L;

/*! Savefile version */
static byte sf_major;	/**< Savefile's VERSION_MAJOR */
static byte sf_minor;	/**< Savefile's VERSION_MINOR" */
static byte sf_patch;	/**< Savefile's VERSION_PATCH */
static byte sf_extra;	/**< Savefile's VERSION_EXTRA" */


/*!
 * Hack -- Show information on the screen, one line at a time.
 *
 * Avoid the top two lines, to avoid interference with "msg_print()".
 */
static void note(const char* const msg)
{
	static int y = 2;

	prt(msg, y, 0);	/* Draw the message */
	if (++y >= 24) y = 2;	/* Advance one line (wrap if needed) */
	Term_fresh();	/* Flush it */
}


/*!
 * This function determines if the version of the savefile
 * currently being read is older than version "x.y.z".
 */
static bool older_than(byte x, byte y, byte z)
{
	/* Much older, or much more recent */
	if (sf_major < x) return TRUE;
	if (sf_major > x) return FALSE;

	/* Distinctly older, or distinctly more recent */
	if (sf_minor < y) return TRUE;
	if (sf_minor > y) return FALSE;

	/* Barely older, or barely more recent */
	if (sf_patch < z) return TRUE;
	if (sf_patch > z) return FALSE;

	/* Identical versions */
	return FALSE;
}


/*! determine if an item is "wearable" (or a missile) */
static bool wearable_p(const object_type *o_ptr)
{
	/* Valid "tval" codes */
	switch (o_ptr->obj_id.tval)
	{
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_LITE:
		case TV_AMULET:
		case TV_RING:
			return TRUE;
	}

	return FALSE;	/* Nope */
}


/*!
 * The following functions are used to load the basic building blocks
 * of savefiles.  They also maintain the "checksum" info.
 */

static byte sf_get(void)
{
	/* Get a character, decode the value */
	byte c = getc(fff) & 0xFF;
	byte v = c ^ xor_byte;
	xor_byte = c;

	/* Maintain the checksum info */
	v_check += v;
	x_check += xor_byte;

	/* Return the value */
	return v;
}

static void rd_byte(byte *ip)
{
	*ip = sf_get();
}

static void rd_u16b(u16b *ip)
{
	(*ip) = sf_get();
	(*ip) |= ((u16b)(sf_get()) << 8);
}

static void rd_s16b(s16b *ip)
{
	rd_u16b((u16b*)ip);
}

static void rd_u32b(u32b *ip)
{
	(*ip) = sf_get();
	(*ip) |= ((u32b)(sf_get()) << 8);
	(*ip) |= ((u32b)(sf_get()) << 16);
	(*ip) |= ((u32b)(sf_get()) << 24);
}

static void rd_s32b(s32b *ip)
{
	rd_u32b((u32b*)ip);
}


/*! read a string */
static void rd_string(char *str, int max)
{
	int i = 0;
	byte tmp8u;

	do	{
		rd_byte(&tmp8u);	/* Read a byte */
		if (i < max) str[i] = tmp8u;	/* Collect string while legal */
		++i;
		}
	while(tmp8u);	/* continue until end of string */

	str[max-1] = '\0';	/* Terminate */
}

/*! read a coord */
static void rd(coord& loc)
{
	rd_byte(&loc.y);
	rd_byte(&loc.x);
}

/*! read a tvalsval */
static void rd(tvalsval& id)
{
	rd_byte(&id.tval);
	rd_byte(&id.sval);
}

/*! strip some bytes */
static void strip_bytes(int n)
{
	byte tmp8u;

	/* Strip the bytes */
	while (n--) rd_byte(&tmp8u);
}

/*!
 * read core agent info
 */
static void rd_agent(agent_type& src)
{
	int i;
	byte num;

	rd(src.loc);
	rd_s16b(&src.chp);
	rd_s16b(&src.mhp);
	rd_byte(&src.speed);
	rd_byte(&src.energy);	

	/* Find the number of timed effects */
	rd_byte(&num);

	if (num<=CORE_TMD_MAX)
		{	/* Read all the effects, in a loop */
		for (i = 0; i < num; i++)
			rd_s16b(&src.core_timed[i]);
		/* zero-initialize any entries not read */
		if (num<CORE_TMD_MAX) C_WIPE(src.core_timed+num,CORE_TMD_MAX-num);
		}
	else{	/* probably in trouble anyway */
		for (i = 0; i < CORE_TMD_MAX; i++)
			rd_s16b(&src.core_timed[i]);
		/* discard unused entries */
		strip_bytes(2*(num-CORE_TMD_MAX));
		note("Discarded unsupported timed effects");
		}

}
 
/*
 * Read an object
 *
 * This function attempts to "repair" old savefiles, and to extract
 * the most up to date values for various object fields.
 */
static errr rd_item(object_type *o_ptr)
{
	dice_sides old_d;
	object_kind *k_ptr;

	u32b f[OBJECT_FLAG_STRICT_UB];
	char buf[128];

	/* Location */
	rd(o_ptr->loc);

	/* Type/Subtype */
	rd(o_ptr->obj_id);

	/* tval/sval translators go here */

	/* Kind */
	o_ptr->k_idx = lookup_kind(o_ptr->obj_id);

	/* Special pval */
	rd_s16b(&o_ptr->pval);
	rd_byte(&o_ptr->pseudo);

	if (o_ptr->pseudo && INSCRIP_NULL > o_ptr->pseudo)
	{	/* XXX true discount, don't support this anymore: wipe XXX */
		o_ptr->pseudo = 0;
	}

	rd_byte(&o_ptr->number);
	rd_s16b(&o_ptr->weight);

	rd_byte(&o_ptr->name1);
	rd_byte(&o_ptr->name2);

	rd_s16b(&o_ptr->timeout);

	rd_s16b(&o_ptr->to_h);
	rd_s16b(&o_ptr->to_d);
	rd_s16b(&o_ptr->to_a);

	rd_s16b(&o_ptr->ac);

	rd_byte(&old_d.dice);
	rd_byte(&old_d.sides);

	rd_byte(&o_ptr->ident);

	rd_byte(&o_ptr->marked);

	/* Old flags */
	strip_bytes(12);

	/* Monster holding object */
	rd_s16b(&o_ptr->held_m_idx);

	/* Special powers */
	rd_byte(&o_ptr->xtra1);
	rd_byte(&o_ptr->xtra2);

	/* Inscription */
	rd_string(buf, sizeof(buf));

	/* Save the inscription */
	if (buf[0]) o_ptr->note = quark_add(buf);

	/* Hack -- notice "broken" items */
	k_ptr = object_type::k_info + o_ptr->k_idx;
	if (k_ptr->cost <= 0) o_ptr->ident |= (IDENT_BROKEN);


	/*
	 * Ensure that rods and wands get the appropriate pvals,
	 * and transfer rod charges to timeout.
	 * this test should only be passed once, the first
	 * time the file is open with ROD/WAND stacking code
	 * It could change the timeout improperly if the PVAL (time a rod
	 * takes to charge after use) is changed in object.txt.
	 * But this is nothing a little resting won't solve.
	 *
	 * -JG-
	 */
	if ((o_ptr->obj_id.tval == TV_ROD) &&
	    (o_ptr->pval - (k_ptr->pval * o_ptr->number) != 0))
	{
		o_ptr->timeout = o_ptr->pval;
		o_ptr->pval = k_ptr->pval * o_ptr->number;
	}

	/* Repair non "wearable" items */
	if (!wearable_p(o_ptr))
	{
		/* Get the correct fields */
		o_ptr->to_h = k_ptr->to_h;
		o_ptr->to_d = k_ptr->to_d;
		o_ptr->to_a = k_ptr->to_a;

		/* Get the correct fields */
		o_ptr->ac = k_ptr->ac;
		o_ptr->d = k_ptr->dice;

		/* Get the correct weight */
		o_ptr->weight = k_ptr->weight;

		/* Paranoia */
		o_ptr->name1 = o_ptr->name2 = 0;

		/* All done */
		return (0);
	}


	/* Extract the flags */
	object_flags(o_ptr, f);


	/* Paranoia */
	if (o_ptr->name1)
	{
		if (o_ptr->name1 >= z_info->a_max) return (-1);		/* Paranoia */
		if (!object_type::a_info[o_ptr->name1]._name) o_ptr->name1 = 0;	/* Verify that artifact */
	}

	/* Paranoia */
	if (o_ptr->name2)
	{
		if (o_ptr->name2 >= z_info->e_max) return (-1);		/* Paranoia */
		if (!object_type::e_info[o_ptr->name2]._name) o_ptr->name2 = 0;	/* Verify that ego-item */
	}


	/* Get the standard fields */
	o_ptr->ac = k_ptr->ac;
	o_ptr->d = k_ptr->dice;

	/* Get the standard weight */
	o_ptr->weight = k_ptr->weight;

	/* Hack -- extract the "broken" flag */
	if (o_ptr->pval < 0) o_ptr->ident |= (IDENT_BROKEN);


	/* Artifacts */
	if (o_ptr->name1)
	{	/* Obtain the artifact info */
		artifact_type *a_ptr = &object_type::a_info[o_ptr->name1];

		/* Get the new artifact "pval" */
		o_ptr->pval = a_ptr->pval;

		/* Get the new artifact fields */
		o_ptr->ac = a_ptr->ac;
		o_ptr->d = a_ptr->d;

		/* Get the new artifact weight */
		o_ptr->weight = a_ptr->weight;

		/* Hack -- extract the "broken" flag */
		if (!a_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);
	}

	/* Ego items */
	if (o_ptr->name2)
	{
		ego_item_type *e_ptr = &object_type::e_info[o_ptr->name2];	/* Obtain the ego-item info */

		/* Hack -- keep some old fields */
		if ((o_ptr->d.dice < old_d.dice) && (o_ptr->d.sides == old_d.sides))
		{
			/* Keep old boosted damage dice */
			o_ptr->d.dice = old_d.dice;
		}

		/* Hack -- extract the "broken" flag */
		if (!e_ptr->cost) o_ptr->ident |= (IDENT_BROKEN);

		/* Hack -- enforce legal pval */
		if (e_ptr->flags[0] & (TR1_PVAL_MASK))
		{
			/* Force a meaningful pval */
			if (!o_ptr->pval) o_ptr->pval = 1;
		}

		/* Mega-Hack - Enforce the special broken items */
		if ((o_ptr->name2 == EGO_BLASTED) ||
			(o_ptr->name2 == EGO_SHATTERED))
		{
			/* These were set to k_info values by preceding code */
			o_ptr->ac = 0;
			o_ptr->d.clear();
		}
	}

	/* Success */
	return (0);
}

static void wrap_mon_desc(char *desc, size_t max, const agent_type& agent, int mode)
{
	monster_desc(desc,max,&static_cast<const monster_type&>(agent),mode);
}

static agent_type::lang_aux monster_lang =
{
	"is",
	3,
	wrap_mon_desc,
	NULL	// as monster is not a viewpoint no self-desc
};


/*!
 * Read a monster
 */
static void rd_monster(monster_type *m_ptr)
{
	/* Read the monster race */
	rd_s16b(&m_ptr->r_idx);

	/* Read the other information */
	rd_agent(*m_ptr);
	m_ptr->lang = &monster_lang;
	rd_s16b(&m_ptr->csleep);
}


/*
 * Read the monster lore
 */
static void rd_lore(int r_idx)
{
	int i;
	
	monster_race *r_ptr = &monster_type::r_info[r_idx];
	monster_lore *l_ptr = &monster_type::l_list[r_idx];


	/* Count sights/deaths/kills */
	rd_s16b(&l_ptr->sights);
	rd_s16b(&l_ptr->deaths);
	rd_s16b(&l_ptr->pkills);
	rd_s16b(&l_ptr->tkills);

	/* Count wakes and ignores */
	rd_byte(&l_ptr->wake);
	rd_byte(&l_ptr->ignore);

	/* Extra stuff */
	rd_byte(&l_ptr->xtra1);
	rd_byte(&l_ptr->xtra2);

	/* Count drops */
	rd_byte(&l_ptr->drop_gold);
	rd_byte(&l_ptr->drop_item);

	/* Count spells */
	rd_byte(&l_ptr->cast_innate);
	rd_byte(&l_ptr->cast_spell);

	/* Count blows of each type */
	for (i = 0; i < MONSTER_BLOW_MAX; i++)
		rd_byte(&l_ptr->blows[i]);

	/* Memorize flags */
	for (i = 0; i < RACE_FLAG_STRICT_UB; i++)
		rd_u32b(&l_ptr->flags[i]);

	for (i = 0; i < RACE_FLAG_SPELL_STRICT_UB; i++)
		rd_u32b(&l_ptr->spell_flags[i]);

	/* Read the "Racial" monster limit per level */
	rd_byte(&r_ptr->max_num);

	/* Repair the lore flags */
	for (i = 0; i < RACE_FLAG_STRICT_UB; i++)
		l_ptr->flags[i] &= r_ptr->flags[i];

	for (i = 0; i < RACE_FLAG_SPELL_STRICT_UB; i++)
		l_ptr->spell_flags[i] &= r_ptr->spell_flags[i];
}




/*
 * Read a store
 */
static errr rd_store(int n)
{
	store_type *st_ptr = &store[n];

	int j;
	byte own, num;

	/* Read the basic info */
	rd_byte(&own);
	rd_byte(&num);

	/* Paranoia */
	if (own >= z_info->b_max)
	{
		note("Illegal store owner!");
		return (-1);
	}

	st_ptr->owner = own;

	/* Read the items */
	for (j = 0; j < num; j++)
	{
		object_type i;	/* Get local object */

		/* Wipe the object */
		WIPE(&i);

		/* Read the item */
		if (rd_item(&i))
		{
			note("Error reading item");
			return (-1);
		}

		/* Accept any valid items */
		if (st_ptr->stock_num < STORE_INVEN_MAX) st_ptr->stock[st_ptr->stock_num++] = i;
	}

	/* Success */
	return (0);
}



/*
 * Read RNG state
 */
static void rd_randomizer(void)
{
	int i;

	/* Place */
	rd_u16b(&Rand_place);

	/* State */
	for (i = 0; i < RAND_DEG; i++) rd_u32b(&Rand_state[i]);

	/* Accept */
	Rand_quick = FALSE;
}

static void flag_set(u32b mask, u32b flag, int ob, int i)
{
	/* Process saved entries */
	if (options[i].text && (mask & (1L << ob)))
	{
		op_ptr->opt[i] = (flag & (1L << ob)) ? TRUE : FALSE;
	}
}

/*
 * Read options
 *
 * Note that the normal options are stored as a set of 256 bit flags,
 * plus a set of 256 bit masks to indicate which bit flags were defined
 * at the time the savefile was created.  This will allow new options
 * to be added, and old options to be removed, at any time, without
 * hurting old savefiles.
 *
 * The window options are stored in the same way, but note that each
 * window gets 32 options, and their order is fixed by certain defines.
 */
static void rd_options(void)
{
	int i, n;

	byte b;

	u16b tmp16u;

	u32b flag[8];
	u32b mask[8];
	u32b window_flag[ANGBAND_TERM_MAX];
	u32b window_mask[ANGBAND_TERM_MAX];


	/*** Oops ***/

	/* Ignore old options */
	strip_bytes(16);


	/*** Special info */

	/* Read "delay_factor" */
	rd_byte(&b);
	op_ptr->delay_factor = b;

	/* Read "hitpoint_warn" */
	rd_byte(&b);
	op_ptr->hitpoint_warn = b;

	/* Old cheating options */
	rd_u16b(&tmp16u);


	/*** Normal Options ***/

	/* Read the option flags */
	for (n = 0; n < 8; n++) rd_u32b(&flag[n]);

	/* Read the option masks */
	for (n = 0; n < 8; n++) rd_u32b(&mask[n]);

	/* Analyze the options */
	for (i = 0; i < OPT_MAX; i++)
	{
		int os = i / 32;
		int ob = i % 32;

		flag_set(mask[os], flag[os], ob, i);
	}


	/*** Window Options ***/

	/* Read the window flags */
	for (n = 0; n < ANGBAND_TERM_MAX; n++)
	{
		rd_u32b(&window_flag[n]);
	}

	/* Read the window masks */
	for (n = 0; n < ANGBAND_TERM_MAX; n++)
	{
		rd_u32b(&window_mask[n]);
	}

	/* Analyze the options */
	for (n = 0; n < ANGBAND_TERM_MAX; n++)
	{
		/* Analyze the options */
		for (i = 0; i < 32; i++)
		{
			/* Process valid flags */
			if (window_flag_desc[i])
			{
				/* Blank invalid flags */
				if (!(window_mask[n] & (1L << i)))
				{
					window_flag[n] &= ~(1L << i);
				}
			}
		}
	}

	/* Set up the subwindows */
	subwindows_set_flags(window_flag, ANGBAND_TERM_MAX);
}


static u32b randart_version;


static errr rd_player_spells(void)
{
	int i;
	u16b tmp16u;

	/* Read the number of spells */
	rd_u16b(&tmp16u);
	if (tmp16u > PY_MAX_SPELLS)
	{
		note(format("Too many player spells (%d).", tmp16u));
		return (-1);
	}

	/* Read the spell flags */
	for (i = 0; i < tmp16u; i++) rd_byte(&p_ptr->spell_flags[i]);

	/* Read the spell order */
	for (i = 0; i < tmp16u; i++) rd_byte(&p_ptr->spell_order[i]);

	/* Success */
	return (0);
}

#define MDESC_OBJE		0x01	/* Objective (or Reflexive) */
#define MDESC_POSS		0x02	/* Possessive (or Reflexive) */
#define MDESC_IND1		0x04	/* Indefinites for hidden monsters */

static void player_desc(char *desc, size_t max, const agent_type&, int mode)
{	// second-person pronoun, always visible
	// ignore the indefinite request
	switch(mode & 0x03)
	{
	case MDESC_SUBJ: // intentional fall-through
	case MDESC_OBJE: my_strcpy(desc, "you", max); break;
	case MDESC_POSS: my_strcpy(desc, "your", max); break;
	case MDESC_REFL: my_strcpy(desc, "yourself", max); break;
	}
}

static agent_type::lang_aux player_lang =
{
	"are",
	2,
	NULL,	// don't have to think about third-person desc until multiplayer
	player_desc
};

/*
 * Read the "extra" information
 */
static errr rd_extra(void)
{
	int i;
	byte tmp8u;

	rd_agent(*p_ptr);
	p_ptr->lang = &player_lang;
	
	rd_string(op_ptr->full_name, sizeof(op_ptr->full_name));
	rd_string(p_ptr->died_from, 80);
	rd_string(p_ptr->history, 250);

	/* Player race */
	rd_byte(&tmp8u);

	/* Verify player race */
	if (tmp8u >= z_info->p_max)
	{
		note(format("Invalid player race (%d).", tmp8u));
		return (-1);
	}
	p_ptr->set_race(tmp8u);

	/* Player class */
	rd_byte(&tmp8u);

	/* Verify player class */
	if (tmp8u >= z_info->c_max)
	{
		note(format("Invalid player class (%d).", tmp8u));
		return (-1);
	}
	p_ptr->set_class(tmp8u);

	/* Player gender */
	rd_byte(&tmp8u);
	p_ptr->set_sex(tmp8u);

	/* Special Race/Class info */
	rd_byte(&p_ptr->hitdie);
	rd_byte(&p_ptr->expfact);

	/* Age/Height/Weight */
	rd_s16b(&p_ptr->age);
	rd_s16b(&p_ptr->ht);
	rd_s16b(&p_ptr->wt);

	/* Read the stat info */
	for (i = 0; i < A_MAX; i++) rd_s16b(&p_ptr->stat_max[i]);
	for (i = 0; i < A_MAX; i++) rd_s16b(&p_ptr->stat_cur[i]);

	rd_s32b(&p_ptr->au);

	rd_s32b(&p_ptr->max_exp);
	rd_s32b(&p_ptr->exp);
	rd_u16b(&p_ptr->exp_frac);

	rd_s16b(&p_ptr->lev);

	/* Verify player level */
	if ((p_ptr->lev < 1) || (p_ptr->lev > PY_MAX_LEVEL))
	{
		note(format("Invalid player level (%d).", p_ptr->lev));
		return (-1);
	}

	rd_u16b(&p_ptr->chp_frac);

	rd_s16b(&p_ptr->msp);
	rd_s16b(&p_ptr->csp);
	rd_u16b(&p_ptr->csp_frac);

	rd_s16b(&p_ptr->max_lev);
	rd_s16b(&p_ptr->max_depth);

	/* Hack -- Repair maximum player level */
	if (p_ptr->max_lev < p_ptr->lev) p_ptr->max_lev = p_ptr->lev;

	/* Hack -- Repair maximum dungeon level */
	if (p_ptr->max_depth < 0) p_ptr->max_depth = 1;

	/* More info */
	rd_s16b(&p_ptr->sc);

	{
		byte num;
		int i;

		rd_s16b(&p_ptr->food);
		rd_s16b(&p_ptr->word_recall);
		rd_s16b(&p_ptr->see_infra);
		rd_byte(&p_ptr->confusing);
		rd_byte(&p_ptr->searching);

		/* Find the number of timed effects */
		rd_byte(&num);

		if (num<=TMD_MAX)
			{	/* Read all the effects, in a loop */
			for (i = 0; i < num; i++)
				rd_s16b(&p_ptr->timed[i]);
			/* zero-initialize any entries not read */
			if (num<TMD_MAX) C_WIPE(p_ptr->timed+num,TMD_MAX-num);
			}
		else{	/* probably in trouble anyway */
			for (i = 0; i < TMD_MAX; i++)
				rd_s16b(&p_ptr->timed[i]);
			/* discard unused entries */
			strip_bytes(2*(num-TMD_MAX));
			note("Discarded unsupported timed effects");
			}
	}

	/* Read the randart version */
	rd_u32b(&randart_version);

	/* Read the randart seed */
	rd_u32b(&seed_randart);

	/* Hack -- the two "special seeds" */
	rd_u32b(&seed_flavor);
	rd_u32b(&seed_town);


	/* Special stuff */
	rd_u16b(&p_ptr->panic_save);
	rd_u16b(&p_ptr->total_winner);
	rd_u16b(&p_ptr->noscore);


	/* Read "death" */
	rd_byte(&tmp8u);
	p_ptr->is_dead = tmp8u;

	/* Read "feeling" */
	rd_byte(&tmp8u);
	feeling = tmp8u;

	/* Turn of last "feeling" */
	rd_s32b(&old_turn);

	/* Current turn */
	rd_s32b(&turn);

	return (0);
}


/*
 * Read the "extra" information
 */
static errr rd_extra2(void)
{
	int i;
	u16b tmp16u;

	/* Read the player_hp array */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > PY_MAX_LEVEL)
	{
		note(format("Too many (%u) hitpoint entries!", tmp16u));
		return (-1);
	}

	/* Read the player_hp array */
	for (i = 0; i < tmp16u; i++) rd_s16b(&p_ptr->player_hp[i]);

	/* Read the player spells */
	if (rd_player_spells()) return (-1);

	return (0);
}


/*
 * Read the random artifacts
 */
static errr rd_randarts(void)
{

	int i;
	size_t j;
	byte tmp8u;
	s16b tmp16s;
	u16b tmp16u;
	u16b artifact_count;
	s32b tmp32s;
	u32b tmp32u;


	/* Read the number of artifacts */
	rd_u16b(&artifact_count);

	/* Alive or cheating death */
	if (!p_ptr->is_dead || arg_wizard)
	{
		/* Incompatible save files */
		if (artifact_count > z_info->a_max)
		{
			note(format("Too many (%u) random artifacts!", artifact_count));
			return (-1);
		}

		/* Mark the old artifacts as "empty" */
		for (i = 0; i < z_info->a_max; i++)
		{
			artifact_type *a_ptr = &object_type::a_info[i];
			a_ptr->_name = 0;
			a_ptr->obj_id.clear();
		}

		/* Read the artifacts */
		for (i = 0; i < artifact_count; i++)
		{
			artifact_type *a_ptr = &object_type::a_info[i];

			rd(a_ptr->obj_id);
			rd_s16b(&a_ptr->pval);

			rd_s16b(&a_ptr->to_h);
			rd_s16b(&a_ptr->to_d);
			rd_s16b(&a_ptr->to_a);
			rd_s16b(&a_ptr->ac);

			rd_byte(&a_ptr->d.dice);
			rd_byte(&a_ptr->d.sides);

			rd_s16b(&a_ptr->weight);

			rd_s32b(&a_ptr->cost);

			for(j = 0; j < OBJECT_FLAG_STRICT_UB; ++j)
				rd_u32b(&a_ptr->flags[j]);

			rd_byte(&a_ptr->level);
			rd_byte(&a_ptr->rarity);

			rd_byte(&a_ptr->activation);
			rd_u16b(&a_ptr->time.base);
			rd_u16b(&a_ptr->time.range.sides);
			a_ptr->time.range.dice = 1;			/* constant 1; fix this when bumping internal version */
		}
	}
	else
	{
		/* Read the artifacts */
		for (i = 0; i < artifact_count; i++)
		{
			rd_byte(&tmp8u); /* a_ptr->tval */
			rd_byte(&tmp8u); /* a_ptr->sval */
			rd_s16b(&tmp16s); /* a_ptr->pval */

			rd_s16b(&tmp16s); /* a_ptr->to_h */
			rd_s16b(&tmp16s); /* a_ptr->to_d */
			rd_s16b(&tmp16s); /* a_ptr->to_a */
			rd_s16b(&tmp16s); /* a_ptr->ac */

			rd_byte(&tmp8u); /* a_ptr->dd */
			rd_byte(&tmp8u); /* a_ptr->ds */

			rd_s16b(&tmp16s); /* a_ptr->weight */

			rd_s32b(&tmp32s); /* a_ptr->cost */

			rd_u32b(&tmp32u); /* a_ptr->flags1 */
			rd_u32b(&tmp32u); /* a_ptr->flags2 */
			rd_u32b(&tmp32u); /* a_ptr->flags3 */

			rd_byte(&tmp8u); /* a_ptr->level */
			rd_byte(&tmp8u); /* a_ptr->rarity */

			rd_byte(&tmp8u); /* a_ptr->activation */
			rd_u16b(&tmp16u); /* a_ptr->time */
			rd_u16b(&tmp16u); /* a_ptr->randtime */
		}
	}

	/* Initialize only the randart names */
	do_randart(seed_randart, FALSE);

	return (0);
}




/*
 * Read the player inventory
 *
 * Note that the inventory is "re-sorted" later by "dungeon()".
 */
static errr rd_inventory(void)
{
	object_type object_type_body;
	object_type *i_ptr = &object_type_body;	/* Get local object */
	p_ptr->inven_cnt = 0;

	assert(p_ptr->inven_cnt_is_strict_UB_of_nonzero_k_idx() && "precondition");

	/* Read until done */
	while (1)
	{
		u16b n;

		/* Get the next item index */
		rd_u16b(&n);

		/* Nope, we reached the end */
		if (n == 0xFFFF) break;

		/* Wipe the object */
		WIPE(i_ptr);

		/* Read the item */
		if (rd_item(i_ptr))
		{
			note("Error reading item");
			return (-1);
		}

		/* Hack -- verify item */
		if (!i_ptr->k_idx) return (-1);

		/* Verify slot */
		if (n > INVEN_TOTAL) return (-1);

		/* Wield equipment */
		if ((INVEN_EQUIP_ORIGIN <= n) && (INVEN_EQUIP_STRICT_UB > n))
		{
			/* Copy object */
			p_ptr->inventory[n] = *i_ptr;

			/* Add the weight */
			p_ptr->total_weight += (i_ptr->number * i_ptr->weight);

			/* One more item */
			p_ptr->equip_cnt++;
		}

		/* Warning -- backpack is full */
		else if (p_ptr->inven_cnt == INVEN_PACK)
		{
			/* Oops */
			note("Too many items in the inventory!");

			/* Fail */
			return (-1);
		}

		/* Carry inventory */
		else
		{
			/* Copy object, one more item */
			p_ptr->inventory[p_ptr->inven_cnt++] = *i_ptr;

			/* Add the weight */
			p_ptr->total_weight += (i_ptr->number * i_ptr->weight);

			assert(p_ptr->inven_cnt_is_strict_UB_of_nonzero_k_idx() && "postcondition");
		}
	}

	/* Success */
	return (0);
}



/*! Read the saved messages */
static void rd_messages(void)
{
	size_t i;
	char buf[128];
	u16b tmp16u;

	u16b num;

	/* Total */
	rd_u16b(&num);

	/* Read the messages */
	for (i = 0; i < num; i++)
	{
		/* Read the message */
		rd_string(buf, sizeof(buf));

		/* Read the message type */
		rd_u16b(&tmp16u);

		/* Save the message */
		message_add(buf, tmp16u);
	}
}


/*
 * Read the dungeon
 *
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 *
 * Note that the size of the dungeon is now hard-coded to
 * DUNGEON_HGT by DUNGEON_WID, and any dungeon with another
 * size will be silently discarded by this routine.
 *
 * Note that dungeon objects, including objects held by monsters, are
 * placed directly into the dungeon with the = operator, which will
 * copy "iy", "ix", and "held_m_idx", leaving "next_o_idx" blank for
 * objects held by monsters, since it is not saved in the savefile.
 *
 * After loading the monsters, the objects being held by monsters are
 * linked directly into those monsters.
 */
static errr rd_dungeon(void)
{
	int i, y, x;

	s16b depth;
	s16b ymax, xmax;
	coord ploc = p_ptr->loc;

	byte count;
	byte tmp8u;

	u16b limit;


	/*** Basic info ***/

	/* Header info */
	rd_s16b(&depth);
	rd_s16b(&ymax);
	rd_s16b(&xmax);


	/* Ignore illegal dungeons */
	if ((depth < 0) || (depth >= MAX_DEPTH))
	{
		note(format("Ignoring illegal dungeon depth (%d)", depth));
		return (0);
	}

	/* Ignore illegal dungeons */
	if ((ymax != DUNGEON_HGT) || (xmax != DUNGEON_WID))
	{
		/* XXX XXX XXX */
		note(format("Ignoring illegal dungeon size (%d,%d).", ymax, xmax));
		return (0);
	}

	/* Ignore illegal dungeons */
	if ((ploc.x >= DUNGEON_WID) || (ploc.y >= DUNGEON_HGT))
	{
		note(format("Ignoring illegal player location (%d,%d).", (int)(ploc.y), (int)(ploc.x)));
		return (1);
	}


	/*** Run length decoding ***/

	/* Load the dungeon data */
	for (x = y = 0; y < DUNGEON_HGT; )
	{
		/* Grab RLE info */
		rd_byte(&count);
		rd_byte(&tmp8u);

		/* Apply the RLE info */
		for (i = count; i > 0; i--)
		{
			/* Extract "info" */
			cave_info[y][x] = tmp8u;

			/* Advance/Wrap */
			if (++x >= DUNGEON_WID)
			{
				/* Wrap */
				x = 0;

				/* Advance/Wrap */
				if (++y >= DUNGEON_HGT) break;
			}
		}
	}


	/*** Run length decoding ***/

	/* Load the dungeon data */
	for (x = y = 0; y < DUNGEON_HGT; )
	{
		/* Grab RLE info */
		rd_byte(&count);
		rd_byte(&tmp8u);

		/* Apply the RLE info */
		for (i = count; i > 0; i--)
		{
			/* Extract "feat" */
			cave_set_feat(y, x, tmp8u);

			/* Advance/Wrap */
			if (++x >= DUNGEON_WID)
			{
				/* Wrap */
				x = 0;

				/* Advance/Wrap */
				if (++y >= DUNGEON_HGT) break;
			}
		}
	}


	/*** Player ***/

	/* Load depth */
	p_ptr->depth = depth;


	/* Place player in dungeon */
	if (!player_place(ploc.y, ploc.x))
	{
		note(format("Cannot place player (%d,%d)!", ploc.y, ploc.x));
		return (-1);
	}

	/*** Objects ***/

	/* Read the item count */
	rd_u16b(&limit);

	/* Verify maximum */
	if (limit > z_info->o_max)
	{
		note(format("Too many (%d) object entries!", limit));
		return (-1);
	}

	/* Read the dungeon items */
	for (i = 1; i < limit; i++)
	{
		object_type object_type_body;
		object_type *i_ptr = &object_type_body;	/* Get the object */

		s16b o_idx;
		object_type *o_ptr;

		/* Wipe the object */
		WIPE(i_ptr);

		/* Read the item */
		if (rd_item(i_ptr))
		{
			note("Error reading item");
			return (-1);
		}

		/* Make an object */
		o_idx = o_pop();

		/* Paranoia */
		if (o_idx != i)
		{
			note(format("Cannot place object %d!", i));
			return (-1);
		}

		/* Get the object */
		o_ptr = &o_list[o_idx];

		/* Structure Copy */
		*o_ptr = *i_ptr;

		/* Dungeon floor */
		if (!i_ptr->held_m_idx)
		{
			int x = i_ptr->loc.x;
			int y = i_ptr->loc.y;

			/* ToDo: Verify coordinates */

			/* Link the object to the pile */
			o_ptr->next_o_idx = cave_o_idx[y][x];

			/* Link the floor to the object */
			cave_o_idx[y][x] = o_idx;
		}
	}


	/*** Monsters ***/

	/* Read the monster count */
	rd_u16b(&limit);

	/* Hack -- verify */
	if (limit > z_info->m_max)
	{
		note(format("Too many (%d) monster entries!", limit));
		return (-1);
	}

	/* Read the monsters */
	for (i = 1; i < limit; i++)
	{
		monster_type n;

		WIPE(&n);	/* Clear the monster */
		rd_monster(&n);	/* Read the monster */

		/* Place monster in dungeon */
		if (monster_place(n.loc, &n) != i)
		{
			note(format("Cannot place monster %d", i));
			return (-1);
		}
	}


	/*** Holding ***/

	/* Reacquire objects */
	for (i = 1; i < o_max; ++i)
	{
		object_type *o_ptr = &o_list[i];	/* Get the object */
		monster_type *m_ptr;

		/* Ignore dungeon objects */
		if (!o_ptr->held_m_idx) continue;

		/* Verify monster index */
		if (o_ptr->held_m_idx >= z_info->m_max)
		{
			note("Invalid monster index");
			return (-1);
		}

		/* Get the monster */
		m_ptr = &mon_list[o_ptr->held_m_idx];

		/* Link the object to the pile */
		o_ptr->next_o_idx = m_ptr->hold_o_idx;

		/* Link the monster to the object */
		m_ptr->hold_o_idx = i;
	}


	/*** Success ***/

	/* The dungeon is ready */
	character_dungeon = TRUE;

	/* Success */
	return (0);
}



/*
 * Actually read the savefile
 */
static errr rd_savefile_new_aux(void)
{
	int i;

	byte tmp8u;
	u16b tmp16u;

	u32b n_x_check, n_v_check;
	u32b o_x_check, o_v_check;


	/* Mention the savefile version */
	note(format("Loading a %d.%d.%d savefile...",
	            sf_major, sf_minor, sf_patch));

	/* Strip the version bytes */
	strip_bytes(4);

	/* Hack -- decrypt */
	xor_byte = sf_extra;


	/* Clear the checksums */
	v_check = 0L;
	x_check = 0L;


	rd_u32b(&sf_xtra);	/* Operating system info */
	rd_u32b(&sf_when);	/* Time of savefile creation */
	rd_u16b(&sf_lives);	/* Number of resurrections */
	rd_u16b(&sf_saves);	/* Number of times played */


	/* Read RNG state */
	rd_randomizer();
	if (arg_fiddle) note("Loaded Randomizer Info");


	/* Then the options */
	rd_options();
	if (arg_fiddle) note("Loaded Option Flags");


	/* Then the "messages" */
	rd_messages();
	if (arg_fiddle) note("Loaded Messages");


	/* Monster Memory */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > z_info->r_max)
	{
		note(format("Too many (%u) monster races!", tmp16u));
		return (-1);
	}

	/* Read the available lore records */
	for (i = 0; i < tmp16u; i++) rd_lore(i);
	if (arg_fiddle) note("Loaded Monster Memory");


	/* Object Memory */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > z_info->k_max)
	{
		note(format("Too many (%u) object kinds!", tmp16u));
		return (-1);
	}

	/* Read the object memory */
	for (i = 0; i < tmp16u; i++)
	{
		rd_byte(&tmp8u);
		p_ptr->object_awareness[i] = tmp8u & (PY_OBJECT_AWARE | PY_OBJECT_TRIED);
	}
	if (arg_fiddle) note("Loaded Object Memory");


	/* Load the Quests */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > MAX_Q_IDX)
	{
		note(format("Too many (%u) quests!", tmp16u));
		return (-1);
	}

	/* Load the Quests */
	for (i = 0; i < tmp16u; i++)
	{
		rd_byte(&tmp8u);
		q_list[i].level = tmp8u;
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
	}
	if (arg_fiddle) note("Loaded Quests");


	/* Load the Artifacts */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > z_info->a_max)
	{
		note(format("Too many (%u) artifacts!", tmp16u));
		return (-1);
	}

	/* Read the artifact flags */
	for (i = 0; i < tmp16u; i++)
	{
		rd_byte(&tmp8u);
		object_type::a_info[i].cur_num = tmp8u;
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
	}
	if (arg_fiddle) note("Loaded Artifacts");


	/* Read the extra stuff */
	if (rd_extra()) return (-1);
	if (rd_extra2()) return (-1);
	if (arg_fiddle) note("Loaded extra information");

	/* Read the inventory */
	if (rd_inventory())
	{
		note("Unable to read inventory");
		return (-1);
	}

	/* Read random artifacts */
	if (OPTION(adult_rand_artifacts))
	{
		if (rd_randarts()) return (-1);
		if (arg_fiddle) note("Loaded Random Artifacts");
	}


	/* Read the stores */
	rd_u16b(&tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		if (rd_store(i)) return (-1);
	}


	/* I'm not dead yet... */
	if (!p_ptr->is_dead)
	{
		/* Dead players have no dungeon */
		note("Restoring Dungeon...");
		if (rd_dungeon())
		{
			note("Error reading dungeon data");
			return (-1);
		}
	}


	/* Save the checksum */
	n_v_check = v_check;

	/* Read the old checksum */
	rd_u32b(&o_v_check);

	/* Verify */
	if (o_v_check != n_v_check)
	{
		note("Invalid checksum");
		return (-1);
	}

	/* Save the encoded checksum */
	n_x_check = x_check;

	/* Read the checksum */
	rd_u32b(&o_x_check);

	/* Verify */
	if (o_x_check != n_x_check)
	{
		note("Invalid encoded checksum");
		return (-1);
	}


	/* Hack -- no ghosts */
	monster_type::r_info[z_info->r_max-1].max_num = 0;

	/* Success */
	return (0);
}


/*
 * Actually read the savefile
 */
static errr rd_savefile(void)
{
	errr err;

	/* Grab permissions */
	safe_setuid_grab();

	/* The savefile is a binary file */
	fff = my_fopen(savefile, "rb");

	/* Drop permissions */
	safe_setuid_drop();

	/* Paranoia */
	if (!fff) return (-1);

	/* Call the sub-function */
	err = rd_savefile_new_aux();

	/* Check for errors */
	if (ferror(fff)) err = -1;

	/* Close the file */
	my_fclose(fff);

	/* Result */
	return (err);
}


/*
 * Attempt to Load a "savefile"
 *
 * On multi-user systems, you may only "read" a savefile if you will be
 * allowed to "write" it later, this prevents painful situations in which
 * the player loads a savefile belonging to someone else, and then is not
 * allowed to save his game when he quits.
 *
 * We return "TRUE" if the savefile was usable, and we set the global
 * flag "character_loaded" if a real, living, character was loaded.
 *
 * Note that we always try to load the "current" savefile, even if
 * there is no such file, so we must check for "empty" savefile names.
 */
bool load_player(void)
{
	int fd = -1;

	errr err = 0;

	byte vvv[4];

#ifdef VERIFY_TIMESTAMP
	struct stat	statbuf;
#endif /* VERIFY_TIMESTAMP */

	const char* what = "generic";


	/* Paranoia */
	turn = 0;

	/* Paranoia */
	p_ptr->is_dead = FALSE;


	/* Allow empty savefile name */
	if (!savefile[0]) return (TRUE);

	/* Grab permissions */
	safe_setuid_grab();

	/* Open the savefile */
	fd = fd_open(savefile, O_RDONLY);

	/* Drop permissions */
	safe_setuid_drop();

	/* No file */
	if (fd < 0)
	{
		/* Give a message */
		msg_print("Savefile does not exist.");
		message_flush();

		/* Allow this */
		return (TRUE);
	}

	/* Close the file */
	fd_close(fd);


#ifdef VERIFY_SAVEFILE

	/* Verify savefile usage */
	if (!err)
	{
		FILE *fkk;

		char temp[1024];

		/* Extract name of lock file */
		my_strcpy(temp, savefile, sizeof(temp));
		my_strcat(temp, ".lok", sizeof(temp));

		/* Grab permissions */
		safe_setuid_grab();

		/* Check for lock */
		fkk = my_fopen(temp, "r");

		/* Drop permissions */
		safe_setuid_drop();

		/* Oops, lock exists */
		if (fkk)
		{
			/* Close the file */
			my_fclose(fkk);

			/* Message */
			msg_print("Savefile is currently in use.");
			message_flush();

			/* Oops */
			return (FALSE);
		}

		/* Grab permissions */
		safe_setuid_grab();

		/* Create a lock file */
		fkk = my_fopen(temp, "w");

		/* Drop permissions */
		safe_setuid_drop();

		/* Dump a line of info */
		fprintf(fkk, "Lock file for savefile '%s'\n", savefile);

		/* Close the lock file */
		my_fclose(fkk);
	}

#endif /* VERIFY_SAVEFILE */


	/* Okay */
	if (!err)
	{
		/* Grab permissions */
		safe_setuid_grab();

		/* Open the savefile */
		fd = fd_open(savefile, O_RDONLY);

		/* Drop permissions */
		safe_setuid_drop();

		/* No file */
		if (fd < 0) err = -1;

		/* Message (below) */
		if (err) what = "Cannot open savefile";
	}

	/* Process file */
	if (!err)
	{

#ifdef VERIFY_TIMESTAMP

		/* Grab permissions */
		safe_setuid_grab();

		/* Get the timestamp */
		(void)fstat(fd, &statbuf);

		/* Drop permissions */
		safe_setuid_drop();

#endif /* VERIFY_TIMESTAMP */

		/* Read the first four bytes */
		if (fd_read(fd, (char*)(vvv), sizeof(vvv))) err = -1;

		/* What */
		if (err) what = "Cannot read savefile";

		/* Close the file */
		fd_close(fd);
	}

	/* Process file */
	if (!err)
	{
		/* Extract version */
		sf_major = vvv[0];
		sf_minor = vvv[1];
		sf_patch = vvv[2];
		sf_extra = vvv[3];

		/* Clear screen */
		Term_clear();

		if (older_than(OLD_VERSION_MAJOR, OLD_VERSION_MINOR, OLD_VERSION_PATCH))
		{
			err = -1;
			what = "Savefile is too old";
		}
		else if (!older_than(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH + 1))
		{
			err = -1;
			what = "Savefile is from the future";
		}
		else
		{
			/* Attempt to load */
			err = rd_savefile();

			/* Message (below) */
			if (err) what = "Cannot parse savefile";
		}
	}

	/* Paranoia */
	if (!err)
	{
		/* Invalid turn */
		if (!turn) err = -1;

		/* Message (below) */
		if (err) what = "Broken savefile";
	}

#ifdef VERIFY_TIMESTAMP
	/* Verify timestamp */
	if (!err && !arg_wizard)
	{
		/* Hack -- Verify the timestamp */
		if (sf_when > (statbuf.st_ctime + 100) ||
		    sf_when < (statbuf.st_ctime - 100))
		{
			/* Message */
			what = "Invalid timestamp";

			/* Oops */
			err = -1;
		}
	}
#endif /* VERIFY_TIMESTAMP */


	/* Okay */
	if (!err)
	{
		/* Give a conversion warning */
		if (((byte)VERSION_MAJOR != sf_major) ||
		    ((byte)VERSION_MINOR != sf_minor) ||
		    ((byte)VERSION_PATCH != sf_patch))
		{
			/* Message */
			msg_format("Converted a %d.%d.%d savefile.",
			           sf_major, sf_minor, sf_patch);
			message_flush();
		}

		/* Player is dead */
		if (p_ptr->is_dead)
		{
			/* Cheat death (unless the character retired) */
			if (arg_wizard)
			{
				/* A character was loaded */
				character_loaded = TRUE;

				/* Done */
				return (TRUE);
			}

			/* Forget death */
			p_ptr->is_dead = FALSE;

			/* Count lives */
			sf_lives++;

			/* Forget turns */
			turn = old_turn = 0;

			/* Done */
			return (TRUE);
		}

		/* A character was loaded */
		character_loaded = TRUE;

		/* Still alive */
		if (p_ptr->chp >= 0)
		{
			/* Reset cause of death */
			strcpy(p_ptr->died_from, "(alive and well)");
		}

		/* Success */
		return (TRUE);
	}


#ifdef VERIFY_SAVEFILE

	/* Verify savefile usage */
	if (TRUE)
	{
		char temp[1024];

		/* Extract name of lock file */
		my_strcpy(temp, savefile, sizeof(temp));
		my_strcat(temp, ".lok", sizeof(temp));

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove lock */
		fd_kill(temp);

		/* Drop permissions */
		safe_setuid_drop();
	}

#endif /* VERIFY_SAVEFILE */


	/* Message */
	msg_format("Error (%s) reading %d.%d.%d savefile.",
	           what, sf_major, sf_minor, sf_patch);
	message_flush();

	/* Oops */
	return (FALSE);
}
