/* File: wizard2.c */

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
#include "option.h"
#include "tvalsval.h"

#ifdef ALLOW_DEBUG

/*
 * Hack -- quick debugging hook
 */
static void do_cmd_wiz_hack_ben(void)
{
	unsigned int relay_flow_depth;

	for (relay_flow_depth = 0; relay_flow_depth < MONSTER_FLOW_DEPTH; ++relay_flow_depth)
	{
		/* Update map */
		coord t;

		/* Scan the current panel */
		for (t.y = Term->offset_y; t.y < Term->offset_y + SCREEN_HGT; t.y++)
		{
			for (t.x = Term->offset_x; t.x < Term->offset_x + SCREEN_WID; t.x++)
			{
				if (!in_bounds_fully(t.y, t.x)) continue;

				/* Display proper cost */
				if (cave_cost[t.y][t.x] != relay_flow_depth) continue;

				/* Reliability in yellow */
				const attr_char n = {(cave_when[t.y][t.x] == cave_when[p_ptr->loc.y][p_ptr->loc.x] ? TERM_YELLOW : TERM_RED),
						(t==p_ptr->loc ? '@' : cave_floor_bold(t.y, t.x) ? '*' : '#')};


				print_rel(n,t);
			}
		}

		/* Prompt */
		prt(format("Depth %d: ", relay_flow_depth), 0, 0);

		/* Get key */
		if (inkey() == ESCAPE) break;

		/* Redraw map */
		prt_map();
	}

	/* Done */
	prt("", 0, 0);

	/* Redraw map */
	prt_map();
}



/*
 * Output a long int in binary format.
 */
static void prt_binary(u32b flags, int row, int col)
{
	int i;
	u32b bitmask;

	/* Scan the flags */
	for (i = bitmask = 1; i <= 32; i++, bitmask *= 2)
	{
		/* Dump set bits */
		if (flags & bitmask)
		{
			Term_putch(col++, row, TERM_BLUE, '*');
		}

		/* Dump unset bits */
		else
		{
			Term_putch(col++, row, TERM_WHITE, '-');
		}
	}
}


/*
 * Hack -- Teleport to the target
 */
static void do_cmd_wiz_bamf(void)
{
	/* Must have a target */
	if (target_okay())
	{
		/* Teleport to the target */
		teleport_player_to(p_ptr->target);
	}
}



/*
 * Aux function for "do_cmd_wiz_change()"
 */
static void do_cmd_wiz_change_aux(void)
{
	int i;

	int tmp_int;

	long tmp_long;

	char tmp_val[160];

	char ppp[80];


	/* Query the stats */
	for (i = 0; i < A_MAX; i++)
	{
		/* Prompt */
		strnfmt(ppp, sizeof(ppp), "%s (3-118): ", stat_names[i]);

		/* Default */
		sprintf(tmp_val, "%d", p_ptr->stat_max[i]);

		/* Query */
		if (!get_string(ppp, tmp_val, 4)) return;

		/* Extract */
		tmp_int = atoi(tmp_val);

		/* Verify */
		if (tmp_int > 18+100) tmp_int = 18+100;
		else if (tmp_int < 3) tmp_int = 3;

		/* Save it */
		p_ptr->stat_cur[i] = p_ptr->stat_max[i] = tmp_int;
	}


	/* Default */
	sprintf(tmp_val, "%ld", (long)(p_ptr->au));

	/* Query */
	if (!get_string("Gold: ", tmp_val, 10)) return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0) tmp_long = 0L;

	/* Save */
	p_ptr->au = tmp_long;


	/* Default */
	sprintf(tmp_val, "%ld", (long)(p_ptr->exp));

	/* Query */
	if (!get_string("Experience: ", tmp_val, 10)) return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0) tmp_long = 0L;

	/* Save */
	p_ptr->exp = tmp_long;

	/* Update */
	check_experience();

	/* Default */
	sprintf(tmp_val, "%ld", (long)(p_ptr->max_exp));

	/* Query */
	if (!get_string("Max Exp: ", tmp_val, 10)) return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0) tmp_long = 0L;

	/* Save */
	p_ptr->max_exp = tmp_long;

	/* Update */
	check_experience();
}


/*
 * Change various "permanent" player variables.
 */
static void do_cmd_wiz_change(void)
{
	/* Interact */
	do_cmd_wiz_change_aux();

	/* Redraw everything */
	do_cmd_redraw();
}


/*
 * Wizard routines for creating objects and modifying them
 *
 * This has been rewritten to make the whole procedure
 * of debugging objects much easier and more comfortable.
 *
 * Here are the low-level functions
 *
 * - wiz_display_item()
 *     display an item's debug-info
 * - wiz_create_itemtype()
 *     specify tval and sval (type and subtype of object)
 * - wiz_tweak_item()
 *     specify pval, +AC, +tohit, +todam
 *     Note that the wizard can leave this function anytime,
 *     thus accepting the default-values for the remaining values.
 *     pval comes first now, since it is most important.
 * - wiz_reroll_item()
 *     apply some magic to the item or turn it into an artifact.
 * - wiz_roll_item()
 *     Get some statistics about the rarity of an item:
 *     We create a lot of fake items and see if they are of the
 *     same type (tval and sval), then we compare pval and +AC.
 *     If the fake-item is better or equal it is counted.
 *     Note that cursed items that are better or equal (absolute values)
 *     are counted, too.
 *     HINT: This is *very* useful for balancing the game!
 * - wiz_quantity_item()
 *     change the quantity of an item, but be sane about it.
 *
 * And now the high-level functions
 * - do_cmd_wiz_play()
 *     play with an existing object
 * - wiz_create_item()
 *     create a new object
 *
 * Note -- You do not have to specify "pval" and other item-properties
 * directly. Just apply magic until you are satisfied with the item.
 *
 * Note -- For some items (such as wands, staffs, some rings, etc), you
 * must apply magic, or you will get "broken" or "uncharged" objects.
 *
 * Note -- Redefining artifacts via "do_cmd_wiz_play()" may destroy
 * the artifact.  Be careful.
 *
 * Hack -- this function will allow you to create multiple artifacts.
 * This "feature" may induce crashes or other nasty effects.
 */


/*
 * Display an item's properties
 */
static void wiz_display_item(const object_type *o_ptr)
{
	int j = 0;

	u32b f[OBJECT_FLAG_STRICT_UB];

	char buf[256];


	/* Extract the flags */
	object_flags(o_ptr, f);

	/* Clear screen */
	Term_clear();

	/* Describe fully */
	object_desc_spoil(buf, sizeof(buf), o_ptr, TRUE, ODESC_FULL);

	prt(buf, 2, j);

	prt(format("kind = %-5d  level = %-4d  tval = %-5d  sval = %-5d",
	           o_ptr->k_idx, o_ptr->level(),
	           o_ptr->obj_id.tval, o_ptr->obj_id.sval), 4, j);

	prt(format("number = %-3d  wgt = %-6d  ac = %-5d    damage = %dd%d",
	           o_ptr->number, o_ptr->weight,
	           o_ptr->ac, (int)(o_ptr->d.dice), int(o_ptr->d.sides)), 5, j);

	prt(format("pval = %-5d  toac = %-5d  tohit = %-4d  todam = %-4d",
	           o_ptr->pval, o_ptr->to_a, o_ptr->to_h, o_ptr->to_d), 6, j);

	prt(format("name1 = %-4d  name2 = %-4d  cost = %ld",
	           o_ptr->name1, o_ptr->name2, (long)object_value(o_ptr)), 7, j);

	prt(format("ident = %04x  timeout = %-d",
	           o_ptr->ident, o_ptr->timeout), 8, j);

	prt("+------------FLAGS1------------+", 10, j);
	prt("AFFECT..........SLAY.......BRAND", 11, j);
	prt("                ae      xxxpaefc", 12, j);
	prt("siwdcc  ssidsasmnvudotgddduoclio", 13, j);
	prt("tnieoh  trnipthgiinmrrnrrmniierl", 14, j);
	prt("rtsxna..lcfgdkttmldncltggndsdced", 15, j);
	prt_binary(f[0], 16, j);

	prt("+------------FLAGS2------------+", 17, j);
	prt("SUST........IMM.RESIST.........", 18, j);
	prt("            afecaefcpfldbc s n  ", 19, j);
	prt("siwdcc      cilocliooeialoshnecd", 20, j);
	prt("tnieoh      irelierliatrnnnrethi", 21, j);
	prt("rtsxna......decddcedsrekdfddxhss", 22, j);
	prt_binary(f[1], 23, j);

	prt("+------------FLAGS3------------+", 10, j+32);
	prt("s   ts h     tadiiii   aiehs  hp", 11, j+32);
	prt("lf  eefo     egrgggg  bcnaih  vr", 12, j+32);
	prt("we  lerl    ilgannnn  ltssdo  ym", 13, j+32);
	prt("da reied    merirrrr  eityew ccc", 14, j+32);
	prt("itlepnel    ppanaefc  svaktm uuu", 15, j+32);
	prt("ghigavai    aoveclio  saanyo rrr", 16, j+32);
	prt("seteticf    craxierl  etropd sss", 17, j+32);
	prt("trenhste    tttpdced  detwes eee", 18, j+32);
	prt_binary(f[2], 19, j+32);
}


/*
 * A structure to hold a tval and its description
 */
typedef struct tval_desc
{
	int tval;
	const char* const desc;
} tval_desc;

/*
 * A list of tvals and their textual names
 */
static const tval_desc tvals[] =
{
	{ TV_SWORD,             "Sword"                },
	{ TV_POLEARM,           "Polearm"              },
	{ TV_HAFTED,            "Hafted Weapon"        },
	{ TV_BOW,               "Bow"                  },
	{ TV_ARROW,             "Arrows"               },
	{ TV_BOLT,              "Bolts"                },
	{ TV_SHOT,              "Shots"                },
	{ TV_SHIELD,            "Shield"               },
	{ TV_CROWN,             "Crown"                },
	{ TV_HELM,              "Helm"                 },
	{ TV_GLOVES,            "Gloves"               },
	{ TV_BOOTS,             "Boots"                },
	{ TV_CLOAK,             "Cloak"                },
	{ TV_DRAG_ARMOR,        "Dragon Scale Mail"    },
	{ TV_HARD_ARMOR,        "Hard Armor"           },
	{ TV_SOFT_ARMOR,        "Soft Armor"           },
	{ TV_RING,              "Ring"                 },
	{ TV_AMULET,            "Amulet"               },
	{ TV_LITE,              "Light"                },
	{ TV_POTION,            "Potion"               },
	{ TV_SCROLL,            "Scroll"               },
	{ TV_WAND,              "Wand"                 },
	{ TV_STAFF,             "Staff"                },
	{ TV_ROD,               "Rod"                  },
	{ TV_PRAYER_BOOK,       "Priest Book"          },
	{ TV_MAGIC_BOOK,        "Magic Book"           },
	{ TV_SPIKE,             "Spikes"               },
	{ TV_DIGGING,           "Digger"               },
	{ TV_CHEST,             "Chest"                },
	{ TV_FOOD,              "Food"                 },
	{ TV_FLASK,             "Flask"                },
	{ TV_SKELETON,          "Skeletons"            },
	{ TV_BOTTLE,            "Empty bottle"         },
	{ TV_JUNK,              "Junk"                 },
	{ 0,                    NULL                   }
};


/*
 * Strip an "object name" into a buffer
 */
static void strip_name(char *buf, int k_idx)
{
	char *t;

	object_kind *k_ptr = &object_type::k_info[k_idx];

	const char* str = k_ptr->name();

	/* If not aware, use flavor */ 
	if (!OPTION(adult_know) && !p_ptr->aware(k_idx) && k_ptr->flavor) 
		str = k_ptr->flavor_text(); 

	/* Skip past leading characters */
	while ((*str == ' ') || (*str == '&')) str++;

	/* Copy useful chars */
	for (t = buf; *str; str++)
	{
		/* Pluralizer for irregular plurals */
		/* Useful for languages where adjective changes for plural */
		if (*str == '|')
		{
			/* Process singular part */
			for (str++; *str != '|'; str++) *t++ = *str;

			/* Process plural part */
			for (str++; *str != '|'; str++) ;
		}

		/* English plural indicator can simply be skipped */
		else if (*str != '~') *t++ = *str;
	}

	/* Terminate the new name */
	*t = '\0';
}


/*
 * Get an object kind for creation (or zero)
 *
 * List up to 60 choices in three columns
 */
static int wiz_create_itemtype(void)
{
	int i, num, max_num;
	int col, row;
	int tval;

	const char* tval_desc;
	char ch;

	int choice[60];
	static const char choice_name[] = "abcdefghijklmnopqrst"
	                                  "ABCDEFGHIJKLMNOPQRST"
	                                  "0123456789:;<=>?@%&*";
	const char *cp;

	char buf[160];


	/* Clear screen */
	Term_clear();

	/* Print all tval's and their descriptions */
	for (num = 0; (num < 60) && tvals[num].tval; num++)
	{
		row = 2 + (num % 20);
		col = 30 * (num / 20);
		ch  = choice_name[num];
		prt(format("[%c] %s", ch, tvals[num].desc), row, col);
	}

	/* We need to know the maximal possible tval_index */
	max_num = num;

	/* Choose! */
	if (!get_com("Get what type of object? ", &ch)) return (0);

	/* Analyze choice */
	num = -1;
	if ((cp = strchr(choice_name, ch)) != NULL)
		num = cp - choice_name;

	/* Bail out if choice is illegal */
	if ((num < 0) || (num >= max_num)) return (0);

	/* Base object type chosen, fill in tval */
	tval = tvals[num].tval;
	tval_desc = tvals[num].desc;


	/*** And now we go for k_idx ***/

	/* Clear screen */
	Term_clear();

	/* We have to search the whole itemlist. */
	for (num = 0, i = 1; (num < N_ELEMENTS(choice)) && (i < z_info->k_max); i++)
	{
		object_kind *k_ptr = &object_type::k_info[i];

		/* Analyze matching items */
		if (k_ptr->obj_id.tval == tval)
		{
			/* Hack -- Skip instant artifacts */
			if (k_ptr->flags[2] & (TR3_INSTA_ART)) continue;

			/* Prepare it */
			row = 2 + (num % 20);
			col = 30 * (num / 20);
			ch  = choice_name[num];

			/* Get the "name" of object "i" */
			strip_name(buf, i);

			/* Print it */
			prt(format("[%c] %s", ch, buf), row, col);

			/* Remember the object index */
			choice[num++] = i;
		}
	}

	/* Me need to know the maximal possible remembered object_index */
	max_num = num;

	/* Choose! */
	if (!get_com(format("What Kind of %s? ", tval_desc), &ch)) return (0);

	/* Analyze choice */
	num = -1;
	if ((cp = strchr(choice_name, ch)) != NULL)
		num = cp - choice_name;

	/* Bail out if choice is "illegal" */
	if ((num < 0) || (num >= max_num)) return (0);

	/* And return successful */
	return (choice[num]);
}


/*
 * Tweak an item
 */
static void wiz_tweak_item(object_type *o_ptr)
{
	const char* p;
	char tmp_val[80];


	/* Hack -- leave artifacts alone */
	if (o_ptr->is_artifact()) return;

	p = "Enter new 'pval' setting: ";
	sprintf(tmp_val, "%d", o_ptr->pval);
	if (!get_string(p, tmp_val, 6)) return;
	o_ptr->pval = atoi(tmp_val);
	wiz_display_item(o_ptr);

	p = "Enter new 'to_a' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_a);
	if (!get_string(p, tmp_val, 6)) return;
	o_ptr->to_a = atoi(tmp_val);
	wiz_display_item(o_ptr);

	p = "Enter new 'to_h' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_h);
	if (!get_string(p, tmp_val, 6)) return;
	o_ptr->to_h = atoi(tmp_val);
	wiz_display_item(o_ptr);

	p = "Enter new 'to_d' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_d);
	if (!get_string(p, tmp_val, 6)) return;
	o_ptr->to_d = atoi(tmp_val);
	wiz_display_item(o_ptr);
}


/*
 * Apply magic to an item or turn it into an artifact. -Bernd-
 */
static void wiz_reroll_item(object_type *o_ptr)
{
	object_type object_type_body;
	object_type *i_ptr = &object_type_body;	/* Get local object */

	char ch;

	bool changed = FALSE;


	/* Hack -- leave artifacts alone */
	if (o_ptr->is_artifact()) return;

	*i_ptr = *o_ptr;

	/* Main loop. Ask for magification and artifactification */
	while (TRUE)
	{
		/* Display full item debug information */
		wiz_display_item(i_ptr);

		/* Ask wizard what to do. */
		if (!get_com("[a]ccept, [n]ormal, [g]ood, [e]xcellent? ", &ch))
			break;

		/* Create/change it! */
		if (ch == 'A' || ch == 'a')
		{
			changed = TRUE;
			break;
		}

		/* Apply normal magic, but first clear object */
		else if (ch == 'n' || ch == 'N')
		{
			object_prep(i_ptr, o_ptr->k_idx);
			apply_magic(i_ptr, p_ptr->depth, FALSE, FALSE, FALSE);
		}

		/* Apply good magic, but first clear object */
		else if (ch == 'g' || ch == 'g')
		{
			object_prep(i_ptr, o_ptr->k_idx);
			apply_magic(i_ptr, p_ptr->depth, FALSE, TRUE, FALSE);
		}

		/* Apply great magic, but first clear object */
		else if (ch == 'e' || ch == 'e')
		{
			object_prep(i_ptr, o_ptr->k_idx);
			apply_magic(i_ptr, p_ptr->depth, FALSE, TRUE, TRUE);
		}
	}


	/* Notice change */
	if (changed)
	{
		/* Restore the position information */
		i_ptr->loc = o_ptr->loc;
		i_ptr->next_o_idx = o_ptr->next_o_idx;
		i_ptr->marked = o_ptr->marked;

		/* Apply changes */
		*o_ptr = *i_ptr;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* Window stuff */
		p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
	}
}



/*
 * Maximum number of rolls
 */
#define TEST_ROLL 100000


/*
 * Try to create an item again. Output some statistics.    -Bernd-
 *
 * The statistics are correct now.  We acquire a clean grid, and then
 * repeatedly place an object in this grid, copying it into an item
 * holder, and then deleting the object.  We fiddle with the artifact
 * counter flags to prevent weirdness.  We use the items to collect
 * statistics on item creation relative to the initial item.
 */
static void wiz_statistics(object_type *o_ptr)
{
	long i, matches, better, worse, other;

	char ch;
	const char* quality;

	bool good, great;

	object_type object_type_body;
	object_type *i_ptr = &object_type_body;	/* Get local object */

	const char* const q = "Rolls: %ld, Matches: %ld, Better: %ld, Worse: %ld, Other: %ld";


	/* Mega-Hack -- allow multiple artifacts XXX XXX XXX */
	if (o_ptr->is_artifact()) object_type::a_info[o_ptr->name1].cur_num = 0;


	/* Interact */
	while (TRUE)
	{
		const char* const pmt = "Roll for [n]ormal, [g]ood, or [e]xcellent treasure? ";

		/* Display item */
		wiz_display_item(o_ptr);

		/* Get choices */
		if (!get_com(pmt, &ch)) break;

		if (ch == 'n' || ch == 'N')
		{
			good = FALSE;
			great = FALSE;
			quality = "normal";
		}
		else if (ch == 'g' || ch == 'G')
		{
			good = TRUE;
			great = FALSE;
			quality = "good";
		}
		else if (ch == 'e' || ch == 'E')
		{
			good = TRUE;
			great = TRUE;
			quality = "excellent";
		}
		else
		{
#if 0 /* unused */
			good = FALSE;
			great = FALSE;
#endif /* unused */
			break;
		}

		/* Let us know what we are doing */
		msg_format("Creating a lot of %s items. Base level = %d.",
		           quality, p_ptr->depth);
		message_flush();

		/* Set counters to zero */
		matches = better = worse = other = 0;

		/* Let's rock and roll */
		for (i = 0; i <= TEST_ROLL; i++)
		{
			/* Output every few rolls */
			if ((i < 100) || (i % 100 == 0))
			{
				/* Do not wait */
				inkey_scan = TRUE;

				/* Allow interupt */
				if (inkey())
				{
					/* Flush */
					flush();

					/* Stop rolling */
					break;
				}

				/* Dump the stats */
				prt(format(q, i, matches, better, worse, other), 0, 0);
				Term_fresh();
			}


			/* Wipe the object */
			WIPE(i_ptr);

			/* Create an object */
			make_object(i_ptr, good, great, p_ptr->depth);


			/* Mega-Hack -- allow multiple artifacts XXX XXX XXX */
			if (i_ptr->is_artifact()) object_type::a_info[i_ptr->name1].cur_num = 0;


			/* Test for the same tval and sval. */
			if (o_ptr->obj_id!=i_ptr->obj_id) continue;

			/* Check for match */
			if ((i_ptr->pval == o_ptr->pval) &&
			    (i_ptr->to_a == o_ptr->to_a) &&
			    (i_ptr->to_h == o_ptr->to_h) &&
			    (i_ptr->to_d == o_ptr->to_d))
			{
				matches++;
			}

			/* Check for better */
			else if ((i_ptr->pval >= o_ptr->pval) &&
			         (i_ptr->to_a >= o_ptr->to_a) &&
			         (i_ptr->to_h >= o_ptr->to_h) &&
			         (i_ptr->to_d >= o_ptr->to_d))
			{
				better++;
			}

			/* Check for worse */
			else if ((i_ptr->pval <= o_ptr->pval) &&
			         (i_ptr->to_a <= o_ptr->to_a) &&
			         (i_ptr->to_h <= o_ptr->to_h) &&
			         (i_ptr->to_d <= o_ptr->to_d))
			{
				worse++;
			}

			/* Assume different */
			else
			{
				other++;
			}
		}

		/* Final dump */
		msg_format(q, i, matches, better, worse, other);
		message_flush();
	}


	/* Hack -- Normally only make a single artifact */
	if (o_ptr->is_artifact()) object_type::a_info[o_ptr->name1].cur_num = 1;
}


/*
 * Change the quantity of a the item
 */
static void wiz_quantity_item(object_type *o_ptr, bool carried)
{
	int tmp_int;

	char tmp_val[3];


	/* Never duplicate artifacts */
	if (o_ptr->is_artifact()) return;


	/* Default */
	sprintf(tmp_val, "%d", o_ptr->number);

	/* Query */
	if (get_string("Quantity: ", tmp_val, 3))
	{
		/* Extract */
		tmp_int = atoi(tmp_val);

		/* Paranoia */
		if (tmp_int < 1) tmp_int = 1;
		if (tmp_int > 99) tmp_int = 99;

		/* Adjust total weight being carried */
		if (carried)
		{
			/* Remove the weight of the old number of objects */
			p_ptr->total_weight -= (o_ptr->number * o_ptr->weight);

			/* Add the weight of the new number of objects */
			p_ptr->total_weight += (tmp_int * o_ptr->weight);
		}

		/* Adjust charge for rods */
		if (o_ptr->obj_id.tval == TV_ROD)
		{
			o_ptr->pval = (o_ptr->pval / o_ptr->number) * tmp_int;
		}

		/* Accept modifications */
		o_ptr->number = tmp_int;
	}
}



/*
 * Play with an item. Options include:
 *   - Output statistics (via wiz_roll_item)
 *   - Reroll item (via wiz_reroll_item)
 *   - Change properties (via wiz_tweak_item)
 *   - Change the number of items (via wiz_quantity_item)
 */
static void do_cmd_wiz_play(void)
{
	int item;

	object_type object_type_body;
	object_type *i_ptr = &object_type_body;	/* Get local object */
	object_type *o_ptr;

	char ch;

	const char* const q = "Play with which object? ";
	const char* const s = "You have nothing to play with.";

	bool changed = FALSE;


	/* Get an item */
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return;

	/* Get the object */
	o_ptr = get_o_ptr_from_inventory_or_floor(item);

	screen_save();

	*i_ptr = *o_ptr;

	/* The main loop */
	while (TRUE)
	{
		/* Display the item */
		wiz_display_item(i_ptr);

		/* Get choice */
		if (!get_com("[a]ccept [s]tatistics [r]eroll [t]weak [q]uantity? ", &ch))
			break;

		if (ch == 'A' || ch == 'a')
		{
			changed = TRUE;
			break;
		}

		if (ch == 's' || ch == 'S')
		{
			wiz_statistics(i_ptr);
		}

		if (ch == 'r' || ch == 'r')
		{
			wiz_reroll_item(i_ptr);
		}

		if (ch == 't' || ch == 'T')
		{
			wiz_tweak_item(i_ptr);
		}

		if (ch == 'q' || ch == 'Q')
		{
			bool carried = (item >= 0) ? TRUE : FALSE;
			wiz_quantity_item(i_ptr, carried);
		}
	}

	screen_load();

	/* Accept change */
	if (changed)
	{
		msg_print("Changes accepted.");

		/* Change */
		*o_ptr = *i_ptr;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* Window stuff */
		p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
	}

	/* Ignore change */
	else
	{
		msg_print("Changes ignored.");
	}
}


/*
 * Wizard routine for creating objects
 *
 * Note that wizards cannot create objects on top of other objects.
 *
 * Hack -- this routine always makes a "dungeon object", and applies
 * magic to it, and attempts to decline cursed items. XXX XXX XXX
 */
static void wiz_create_item(void)
{
	object_type object_type_body;
	object_type *i_ptr = &object_type_body;	/* Get local object */

	int k_idx;


	/* Save screen */
	screen_save();

	/* Get object base type */
	k_idx = wiz_create_itemtype();

	/* Load screen */
	screen_load();


	/* Return if failed */
	if (!k_idx) return;

	/* Create the item */
	object_prep(i_ptr, k_idx);

	/* Apply magic (no messages, no artifacts) */
	apply_magic(i_ptr, p_ptr->depth, FALSE, FALSE, FALSE);

	/* Drop the object from heaven */
	drop_near(i_ptr, -1, p_ptr->loc);

	/* All done */
	msg_print("Allocated.");
}


/*
 * Create the artifact with the specified number
 */
static void wiz_create_artifact(int a_idx)
{
	object_type object_type_body;
	object_type *i_ptr = &object_type_body;	/* Get local object */
	int k_idx;

	artifact_type *a_ptr = &object_type::a_info[a_idx];

	/* Ignore "empty" artifacts */
	if (!a_ptr->_name) return;

	/* Wipe the object */
	WIPE(i_ptr);

	/* Acquire the "kind" index */
	k_idx = lookup_kind(a_ptr->obj_id);

	/* Oops */
	if (!k_idx) return;

	/* Create the artifact */
	object_prep(i_ptr, k_idx);

	/* Save the name */
	i_ptr->name1 = a_idx;

	/* Extract the fields */
	i_ptr->pval = a_ptr->pval;
	i_ptr->ac = a_ptr->ac;
	i_ptr->d = a_ptr->d;
	i_ptr->to_a = a_ptr->to_a;
	i_ptr->to_h = a_ptr->to_h;
	i_ptr->to_d = a_ptr->to_d;
	i_ptr->weight = a_ptr->weight;

	/* Drop the artifact from heaven */
	drop_near(i_ptr, -1, p_ptr->loc);

	/* All done */
	msg_print("Allocated.");
}


/*
 * Cure everything instantly
 */
static void do_cmd_wiz_cure_all(void)
{
	/* Remove curses */
	remove_all_curse();

	/* Restore stats */
	res_stat(A_STR);
	res_stat(A_INT);
	res_stat(A_WIS);
	res_stat(A_CON);
	res_stat(A_DEX);
	res_stat(A_CHR);

	/* Restore the level */
	restore_level();

	/* Heal the player */
	p_ptr->chp = p_ptr->mhp;
	p_ptr->chp_frac = 0;

	/* Restore mana */
	p_ptr->csp = p_ptr->msp;
	p_ptr->csp_frac = 0;

	/* Cure stuff */
	p_ptr->clear_timed<TMD_BLIND>();
	p_ptr->clear_core_timed<CORE_TMD_CONFUSED>();
	p_ptr->clear_timed<TMD_POISONED>();
	p_ptr->clear_core_timed<CORE_TMD_AFRAID>();
	p_ptr->clear_timed<TMD_PARALYZED>();
	p_ptr->clear_timed<TMD_IMAGE>();
	p_ptr->clear_core_timed<CORE_TMD_STUN>();
	p_ptr->clear_timed<TMD_CUT>();
	p_ptr->clear_core_timed<CORE_TMD_SLOW>();

	/* No longer hungry */
	set_food(PY_FOOD_MAX - 1);

	/* Redraw everything */
	do_cmd_redraw();
}


/*
 * Go to any level
 */
static void do_cmd_wiz_jump(void)
{
	/* Ask for level */
	if (p_ptr->command_arg <= 0)
	{
		char ppp[80];
		char tmp_val[160];

		/* Prompt */
		sprintf(ppp, "Jump to level (0-%d): ", MAX_DEPTH-1);

		/* Default */
		sprintf(tmp_val, "%d", p_ptr->depth);

		/* Ask for a level */
		if (!get_string(ppp, tmp_val, 11)) return;

		/* Extract request */
		p_ptr->command_arg = atoi(tmp_val);
	}

	/* Paranoia */
	if (p_ptr->command_arg < 0) p_ptr->command_arg = 0;

	/* Paranoia */
	if (p_ptr->command_arg > MAX_DEPTH - 1) p_ptr->command_arg = MAX_DEPTH - 1;

	/* Accept request */
	msg_format("You jump to dungeon level %d.", p_ptr->command_arg);

	/* New depth */
	p_ptr->depth = p_ptr->command_arg;

	/* Leaving */
	p_ptr->leaving = TRUE;
}


/*
 * Become aware of a lot of objects
 */
static void do_cmd_wiz_learn(void)
{
	int i;

	object_type object_type_body;
	object_type *i_ptr = &object_type_body;	/* Get local object */

	/* Scan every object */
	for (i = 1; i < z_info->k_max; i++)
	{
		object_kind *k_ptr = &object_type::k_info[i];

		/* Induce awareness */
		if (k_ptr->level <= p_ptr->command_arg)
		{
			/* Prepare object */
			object_prep(i_ptr, i);

			/* Awareness */
			p_ptr->be_aware(*i_ptr);
		}
	}
}


/*
 * Hack -- Rerate Hitpoints
 */
static void do_cmd_rerate(void)
{
	int min_value, max_value, i, percent;

	min_value = (PY_MAX_LEVEL * 3 * (p_ptr->hitdie - 1)) / 8;
	min_value += PY_MAX_LEVEL;

	max_value = (PY_MAX_LEVEL * 5 * (p_ptr->hitdie - 1)) / 8;
	max_value += PY_MAX_LEVEL;

	p_ptr->player_hp[0] = p_ptr->hitdie;

	/* Rerate */
	while (1)
	{
		/* Collect values */
		for (i = 1; i < PY_MAX_LEVEL; i++)
		{
			p_ptr->player_hp[i] = randint(p_ptr->hitdie);
			p_ptr->player_hp[i] += p_ptr->player_hp[i - 1];
		}

		/* Legal values */
		if ((p_ptr->player_hp[PY_MAX_LEVEL - 1] >= min_value) &&
		    (p_ptr->player_hp[PY_MAX_LEVEL - 1] <= max_value)) break;
	}

	percent = (int)(((long)p_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) /
	                (p_ptr->hitdie + ((PY_MAX_LEVEL - 1) * p_ptr->hitdie)));

	/* Update and redraw hitpoints */
	p_ptr->update |= (PU_HP);
	p_ptr->redraw |= (PR_HP);

	/* Handle stuff */
	handle_stuff();

	/* Message */
	msg_format("Current Life Rating is %d/100.", percent);
}


/*
 * Summon some creatures
 */
static void do_cmd_wiz_summon(int num)
{
	int i;

	for (i = 0; i < num; i++)
	{
		(void)summon_specific(p_ptr->loc, p_ptr->depth, 0);
	}
}


/*
 * Summon a creature of the specified type
 *
 * This function is rather dangerous XXX XXX XXX
 */
static void do_cmd_wiz_named(int r_idx, bool slp)
{
	coord g;

	int i;

	/* Paranoia */
	if (!r_idx) return;
	if (r_idx >= z_info->r_max-1) return;

	/* Try 10 times */
	for (i = 0; i < 10; i++)
	{
		int d = 1;

		/* Pick a location */
		scatter(g, p_ptr->loc, d, 0);

		/* Require empty grids */
		if (!cave_empty_bold(g.y, g.x)) continue;

		/* Place it (allow groups) */
		if (place_monster_aux(g, r_idx, slp, TRUE)) break;
	}
}



/*
 * Hack -- Delete all nearby monsters
 */
static void do_cmd_wiz_zap(int d)
{
	int i;

	/* Banish everyone nearby */
	for (i = 1; i < mon_max; i++)
	{
		monster_type *m_ptr = &mon_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > d) continue;

		/* Delete the monster */
		delete_monster_idx(i);
	}

	/* Update monster list window */
	p_ptr->redraw |= PR_MONLIST;
}


/*
 * Un-hide all monsters
 */
static void do_cmd_wiz_unhide(int d)
{
	int i;

	/* Process monsters */
	for (i = 1; i < mon_max; i++)
	{
		monster_type *m_ptr = &mon_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > d) continue;

		/* Optimize -- Repair flags */
		repair_mflag_mark = repair_mflag_show = TRUE;

		/* Detect the monster */
		m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

		/* Update the monster */
		update_mon(i, FALSE);
	}
}

/*
 * Query the dungeon
 */
static void do_cmd_wiz_query(void)
{
	u16b wiz_mask = 0;
	coord t;
	char cmd;

	/* Get a "debug command" */
	if (!get_com("Debug Command Query: ", &cmd)) return;

	/* Extract a flag */
	switch (cmd)
	{
		case '0': wiz_mask = (1 << 0); break;
		case '1': wiz_mask = (1 << 1); break;
		case '2': wiz_mask = (1 << 2); break;
		case '3': wiz_mask = (1 << 3); break;
		case '4': wiz_mask = (1 << 4); break;
		case '5': wiz_mask = (1 << 5); break;
		case '6': wiz_mask = (1 << 6); break;
		case '7': wiz_mask = (1 << 7); break;

		case 'm': wiz_mask |= (CAVE_MARK); break;
		case 'g': wiz_mask |= (CAVE_GLOW); break;
		case 'r': wiz_mask |= (CAVE_ROOM); break;
		case 'i': wiz_mask |= (CAVE_ICKY); break;
		case 's': wiz_mask |= (CAVE_SEEN); break;
		case 'v': wiz_mask |= (CAVE_VIEW); break;
		case 't': wiz_mask |= (CAVE_TEMP); break;
		case 'w': wiz_mask |= (CAVE_WALL); break;
	}

	/* Scan the current panel */
	for (t.y = Term->offset_y; t.y < Term->offset_y + SCREEN_HGT; t.y++)
	{
		for (t.x = Term->offset_x; t.x < Term->offset_x + SCREEN_WID; t.x++)
		{
			if (!in_bounds_fully(t.y, t.x)) continue;

			/* Given mask, show only those grids */
			if (wiz_mask && !(cave_info[t.y][t.x] & wiz_mask)) continue;

			/* Given no mask, show unknown grids */
			if (!wiz_mask && (cave_info[t.y][t.x] & (CAVE_MARK))) continue;

			/* Color */
			const attr_char n = {(cave_floor_bold(t.y, t.x) ? TERM_YELLOW :  TERM_RED), 
					  	(t==p_ptr->loc ? '@' : cave_floor_bold(t.y, t.x) ? '*' : '#')};

			print_rel(n,t);
		}
	}

	/* Get keypress */
	msg_print("Press any key.");
	message_flush();

	/* Redraw map */
	prt_map();
}


/*
 * Ask for and parse a "debug command"
 *
 * The "p_ptr->command_arg" may have been set.
 */
void do_cmd_debug(void)
{
	char cmd;


	/* Get a "debug command" */
	if (!get_com("Debug Command: ", &cmd)) return;

	/* Analyze the command */
	switch (cmd)
	{
		/* Ignore */
		case ESCAPE:
		case ' ':
		case '\n':
		case '\r':
		{
			break;
		}

#ifdef ALLOW_SPOILERS

		/* Hack -- Generate Spoilers */
		case '"':
		{
			do_cmd_spoilers();
			break;
		}

#endif


		/* Hack -- Help */
		case '?':
		{
			do_cmd_help();
			break;
		}

		/* Cure all maladies */
		case 'a':
		{
			do_cmd_wiz_cure_all();
			break;
		}

		/* Teleport to target */
		case 'b':
		{
			do_cmd_wiz_bamf();
			break;
		}

		/* Create any object */
		case 'c':
		{
			wiz_create_item();
			break;
		}

		/* Create an artifact */
		case 'C':
		{
			wiz_create_artifact(p_ptr->command_arg);
			break;
		}

		/* Detect everything */
		case 'd':
		{
			detect_all();
			break;
		}

		/* Edit character */
		case 'e':
		{
			do_cmd_wiz_change();
			break;
		}

		/* View item info */
		case 'f':
		{
			(void)identify_fully();
			break;
		}

		/* Good Objects */
		case 'g':
		{
			if (p_ptr->command_arg <= 0) p_ptr->command_arg = 1;
			acquirement(p_ptr->loc, p_ptr->command_arg, FALSE, p_ptr->depth);
			break;
		}

		/* Hitpoint rerating */
		case 'h':
		{
			do_cmd_rerate();
			break;
		}

		/* Identify */
		case 'i':
		{
			(void)ident_spell();
			break;
		}

		/* Go up or down in the dungeon */
		case 'j':
		{
			do_cmd_wiz_jump();
			break;
		}

		/* Self-Knowledge */
		case 'k':
		{
			self_knowledge();
			break;
		}

		/* Learn about objects */
		case 'l':
		{
			do_cmd_wiz_learn();
			break;
		}

		/* Magic Mapping */
		case 'm':
		{
			map_area();
			break;
		}

		/* Summon Named Monster */
		case 'n':
		{
			do_cmd_wiz_named(p_ptr->command_arg, TRUE);
			break;
		}

		/* Object playing routines */
		case 'o':
		{
			do_cmd_wiz_play();
			break;
		}

		/* Phase Door */
		case 'p':
		{
			teleport_player(10);
			break;
		}

		/* Query the dungeon */
		case 'q':
		{
			do_cmd_wiz_query();
			break;
		}

		/* Summon Random Monster(s) */
		case 's':
		{
			if (p_ptr->command_arg <= 0) p_ptr->command_arg = 1;
			do_cmd_wiz_summon(p_ptr->command_arg);
			break;
		}

		/* Teleport */
		case 't':
		{
			teleport_player(100);
			break;
		}

		/* Un-hide all monsters */
		case 'u':
		{
			if (p_ptr->command_arg <= 0) p_ptr->command_arg = 255;
			do_cmd_wiz_unhide(p_ptr->command_arg);
			break;
		}

		/* Very Good Objects */
		case 'v':
		{
			if (p_ptr->command_arg <= 0) p_ptr->command_arg = 1;
			acquirement(p_ptr->loc, p_ptr->command_arg, TRUE, p_ptr->depth);
			break;
		}

		/* Wizard Light the Level */
		case 'w':
		{
			wiz_lite();
			break;
		}

		/* Increase Experience */
		case 'x':
		{
			if (p_ptr->command_arg)
			{
				gain_exp(p_ptr->command_arg);
			}
			else
			{
				gain_exp(p_ptr->exp + 1);
			}
			break;
		}

		/* Zap Monsters (Banishment) */
		case 'z':
		{
			if (p_ptr->command_arg <= 0) p_ptr->command_arg = MAX_SIGHT;
			do_cmd_wiz_zap(p_ptr->command_arg);
			break;
		}

		/* Hack */
		case '_':
		{
			do_cmd_wiz_hack_ben();
			break;
		}

		/* Oops */
		default:
		{
			msg_print("That is not a valid debug command.");
			break;
		}
	}
}


#else

#ifdef MACINTOSH
static int i = 0;
#endif

#endif


