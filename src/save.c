/* File: save.c */

/*
 * Copyright (c) 1997 Ben Harrison
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
#include "option.h"
#include "store.h"

/*
 * Some "local" parameters, used to help write savefiles
 */

static FILE	*fff;		/* Current save "file" */

static byte	xor_byte;	/* Simple encryption */

static u32b	v_stamp = 0L;	/* A simple "checksum" on the actual values */
static u32b	x_stamp = 0L;	/* A simple "checksum" on the encoded bytes */



/*
 * These functions place information into a savefile a byte at a time
 */

static void sf_put(byte v)
{
	/* Encode the value, write a character */
	xor_byte ^= v;
	(void)putc((int)xor_byte, fff);

	/* Maintain the checksum info */
	v_stamp += v;
	x_stamp += xor_byte;
}

static void wr_byte(byte v)
{
	sf_put(v);
}

static void wr_u16b(u16b v)
{
	sf_put((byte)(v & 0xFF));
	sf_put((byte)((v >> 8) & 0xFF));
}

static void wr_s16b(s16b v)
{
	wr_u16b((u16b)v);
}

static void wr_u32b(u32b v)
{
	sf_put((byte)(v & 0xFF));
	sf_put((byte)((v >> 8) & 0xFF));
	sf_put((byte)((v >> 16) & 0xFF));
	sf_put((byte)((v >> 24) & 0xFF));
}

static void wr_s32b(s32b v)
{
	wr_u32b((u32b)v);
}

static void wr_string(const char* str)
{
	while (*str)
	{
		wr_byte(*str);
		str++;
	}
	wr_byte(*str);
}

/*! write a coord */
static void wr(const coord& loc)
{
	wr_byte(loc.y);
	wr_byte(loc.x);
}

/*! write a tvalsval */
static void wr(const tvalsval& id)
{
	wr_byte(id.tval);
	wr_byte(id.sval);
}

/*
 * These functions write info in larger logical records
 */

/*!
 * write core agent info
 */
static void wr_agent(const agent_type& src)
{
	int i;

	wr(src.loc);
	wr_s16b(src.chp);
	wr_s16b(src.mhp);
	wr_byte(src.speed);
	wr_byte(src.energy);	

	/* Find the number of core timed effects */
	wr_byte(CORE_TMD_MAX);

	/* Read all the effects, in a loop */
	for (i = 0; i < CORE_TMD_MAX; i++)
		wr_s16b(src.core_timed[i]);
} 

/*
 * Write an "item" record
 */
static void wr_item(const object_type *o_ptr)
{
	/* Location */
	wr(o_ptr->loc);

	wr(o_ptr->obj_id);
	wr_s16b(o_ptr->pval);

	wr_byte(o_ptr->pseudo);

	wr_byte(o_ptr->number);
	wr_s16b(o_ptr->weight);

	wr_byte(o_ptr->name1);
	wr_byte(o_ptr->name2);

	wr_s16b(o_ptr->timeout);

	wr_s16b(o_ptr->to_h);
	wr_s16b(o_ptr->to_d);
	wr_s16b(o_ptr->to_a);
	wr_s16b(o_ptr->ac);
	wr_byte(o_ptr->d.dice);
	wr_byte(o_ptr->d.sides);

	wr_byte(o_ptr->ident);

	wr_byte(o_ptr->marked);

	/* Old flags */
	wr_u32b(0L);
	wr_u32b(0L);
	wr_u32b(0L);

	/* Held by monster index */
	wr_s16b(o_ptr->held_m_idx);

	/* Extra information */
	wr_byte(o_ptr->xtra1);
	wr_byte(o_ptr->xtra2);

	/* Save the inscription (if any) */
	wr_string(o_ptr->note ? quark_str(o_ptr->note) : "");
}


/*!
 * Write a "monster" record
 */
static void wr_monster(const monster_type *m_ptr)
{
	wr_s16b(m_ptr->r_idx);
	wr_agent(*m_ptr);
	wr_s16b(m_ptr->csleep);
}


/*
 * Write a "lore" record
 */
static void wr_lore(int r_idx)
{
	int i;

	monster_race *r_ptr = &monster_type::r_info[r_idx];
	monster_lore *l_ptr = &monster_type::l_list[r_idx];

	/* Count sights/deaths/kills */
	wr_s16b(l_ptr->sights);
	wr_s16b(l_ptr->deaths);
	wr_s16b(l_ptr->pkills);
	wr_s16b(l_ptr->tkills);

	/* Count wakes and ignores */
	wr_byte(l_ptr->wake);
	wr_byte(l_ptr->ignore);

	/* Extra stuff */
	wr_byte(l_ptr->xtra1);
	wr_byte(l_ptr->xtra2);

	/* Count drops */
	wr_byte(l_ptr->drop_gold);
	wr_byte(l_ptr->drop_item);

	/* Count spells */
	wr_byte(l_ptr->cast_innate);
	wr_byte(l_ptr->cast_spell);

	/* Count blows of each type */
	for (i = 0; i < MONSTER_BLOW_MAX; i++)
		wr_byte(l_ptr->blows[i]);

	/* Memorize flags */
	for (i = 0; i < RACE_FLAG_STRICT_UB; i++)
		wr_u32b(l_ptr->flags[i]);

	for (i = 0; i < RACE_FLAG_SPELL_STRICT_UB; i++)
		wr_u32b(l_ptr->spell_flags[i]);


	/* Monster limit per level */
	wr_byte(r_ptr->max_num);
}


/*
 * Write an "xtra" record
 */
static void wr_xtra(int k_idx)
{
	wr_byte(p_ptr->object_awareness[k_idx]);
}


/*
 * Write a "store" record
 */
static void wr_store(const store_type *st_ptr)
{
	int j;

	/* Save the current owner */
	wr_byte(st_ptr->owner);

	/* Save the stock size */
	wr_byte(st_ptr->stock_num);

	/* Save the stock */
	for (j = 0; j < st_ptr->stock_num; j++)
	{
		/* Save each item in stock */
		wr_item(&st_ptr->stock[j]);
	}
}


/*
 * Write RNG state
 */
static void wr_randomizer(void)
{
	int i;

	/* Place */
	wr_u16b(Rand_place);

	/* State */
	for (i = 0; i < RAND_DEG; i++) wr_u32b(Rand_state[i]);
}


/*
 * Write the "options"
 */
static void wr_options(void)
{
	int i, k;

	u32b flag[8];
	u32b mask[8];
	u32b window_flag[ANGBAND_TERM_MAX];
	u32b window_mask[ANGBAND_TERM_MAX];


	/*** Oops ***/

	/* Oops */
	for (i = 0; i < 4; i++) wr_u32b(0L);


	/*** Special Options ***/

	/* Write "delay_factor" */
	wr_byte(op_ptr->delay_factor);

	/* Write "hitpoint_warn" */
	wr_byte(op_ptr->hitpoint_warn);

	wr_u16b(0);	/* oops */


	/*** Normal options ***/

	/* Reset */
	for (i = 0; i < 8; i++)
	{
		flag[i] = 0L;
		mask[i] = 0L;
	}

	/* Analyze the options */
	for (i = 0; i < OPT_MAX; i++)
	{
		int os = i / 32;
		int ob = i % 32;

		/* Process real entries */
		if (options[i].text)
		{
			/* Set flag */
			if (op_ptr->opt[i])
			{
				/* Set */
				flag[os] |= (1L << ob);
			}

			/* Set mask */
			mask[os] |= (1L << ob);
		}
	}

	/* Dump the flags */
	for (i = 0; i < 8; i++) wr_u32b(flag[i]);

	/* Dump the masks */
	for (i = 0; i < 8; i++) wr_u32b(mask[i]);


	/*** Window options ***/

	/* Reset */
	for (i = 0; i < ANGBAND_TERM_MAX; i++)
	{
		/* Flags */
		window_flag[i] = op_ptr->window_flag[i];

		/* Mask */
		window_mask[i] = 0L;

		/* Build the mask */
		for (k = 0; k < 32; k++)
		{
			/* Set mask */
			if (window_flag_desc[k])
			{
				window_mask[i] |= (1L << k);
			}
		}
	}

	/* Dump the flags */
	for (i = 0; i < ANGBAND_TERM_MAX; i++) wr_u32b(window_flag[i]);

	/* Dump the masks */
	for (i = 0; i < ANGBAND_TERM_MAX; i++) wr_u32b(window_mask[i]);
}


/*
 * Write some "extra" info
 */
static void wr_extra(void)
{
	int i;

	wr_agent(*p_ptr);

	wr_string(op_ptr->full_name);
	wr_string(p_ptr->died_from);
	wr_string(p_ptr->history);

	/* Race/Class/Gender/Spells */
	wr_byte(p_ptr->prace);
	wr_byte(p_ptr->pclass);
	wr_byte(p_ptr->psex);

	wr_byte(p_ptr->hitdie);
	wr_byte(p_ptr->expfact);

	wr_s16b(p_ptr->age);
	wr_s16b(p_ptr->ht);
	wr_s16b(p_ptr->wt);

	/* Dump the stats (maximum and current) */
	for (i = 0; i < A_MAX; ++i) wr_s16b(p_ptr->stat_max[i]);
	for (i = 0; i < A_MAX; ++i) wr_s16b(p_ptr->stat_cur[i]);

	wr_u32b(p_ptr->au);

	wr_u32b(p_ptr->max_exp);
	wr_u32b(p_ptr->exp);
	wr_u16b(p_ptr->exp_frac);
	wr_s16b(p_ptr->lev);

	wr_u16b(p_ptr->chp_frac);

	wr_s16b(p_ptr->msp);
	wr_s16b(p_ptr->csp);
	wr_u16b(p_ptr->csp_frac);

	/* Max Player and Dungeon Levels */
	wr_s16b(p_ptr->max_lev);
	wr_s16b(p_ptr->max_depth);

	/* More info */
	wr_s16b(p_ptr->sc);

	wr_s16b(p_ptr->food);
	wr_s16b(p_ptr->word_recall);
	wr_s16b(p_ptr->see_infra);
	wr_byte(p_ptr->confusing);
	wr_byte(p_ptr->searching);

	/* Find the number of timed effects */
	wr_byte(TMD_MAX);

	/* Read all the effects, in a loop */
	for (i = 0; i < TMD_MAX; i++)
		wr_s16b(p_ptr->timed[i]);

	/* Random artifact version */
	wr_u32b(RANDART_VERSION);

	/* Random artifact seed */
	wr_u32b(seed_randart);


	/* Write the "object seeds" */
	wr_u32b(seed_flavor);
	wr_u32b(seed_town);


	/* Special stuff */
	wr_u16b(p_ptr->panic_save);
	wr_u16b(p_ptr->total_winner);
	wr_u16b(p_ptr->noscore);


	/* Write death */
	wr_byte(p_ptr->is_dead);

	/* Write feeling */
	wr_byte(feeling);

	/* Turn of last "feeling" */
	wr_s32b(old_turn);

	/* Current turn */
	wr_s32b(turn);
}

/*
 * Write some "extra" info
 */
static void wr_extra2(void)
{
	int i;

	/* Dump the "player hp" entries */
	wr_u16b(PY_MAX_LEVEL);
	for (i = 0; i < PY_MAX_LEVEL; i++) wr_s16b(p_ptr->player_hp[i]);


	/* Write spell data */
	wr_u16b(PY_MAX_SPELLS);
	for (i = 0; i < PY_MAX_SPELLS; i++) wr_byte(p_ptr->spell_flags[i]);

	/* Dump the ordered spells */
	for (i = 0; i < PY_MAX_SPELLS; i++) wr_byte(p_ptr->spell_order[i]);
}

/*
 * Dump the random artifacts
 */
static void wr_randarts(void)
{
	int i;
	size_t j;

	wr_u16b(z_info->a_max);

	for (i = 0; i < z_info->a_max; i++)
	{
		artifact_type *a_ptr = &object_type::a_info[i];

		wr(a_ptr->obj_id);
		wr_s16b(a_ptr->pval);

		wr_s16b(a_ptr->to_h);
		wr_s16b(a_ptr->to_d);
		wr_s16b(a_ptr->to_a);
		wr_s16b(a_ptr->ac);

		wr_byte(a_ptr->d.dice);
		wr_byte(a_ptr->d.sides);

		wr_s16b(a_ptr->weight);

		wr_s32b(a_ptr->cost);

		for(j = 0; j < OBJECT_FLAG_STRICT_UB; ++j)
			wr_u32b(a_ptr->flags[j]);

		wr_byte(a_ptr->level);
		wr_byte(a_ptr->rarity);

		wr_byte(a_ptr->activation);
		wr_u16b(a_ptr->time.base);
		/* discard constant 1 a_ptr->time.range.dice */
		/* fix this when bumping internal version */
		wr_u16b(a_ptr->time.range.sides);
	}
}


/*
 * The cave grid flags that get saved in the savefile
 */
#define IMPORTANT_FLAGS (CAVE_MARK | CAVE_GLOW | CAVE_ICKY | CAVE_ROOM)


/*
 * Write the current dungeon
 */
static void wr_dungeon(void)
{
	int i, y, x;

	byte tmp8u;

	byte count;
	byte prev_char;

	/*** Basic info ***/

	/* Dungeon specific info follows */
	wr_u16b(p_ptr->depth);
	wr_u16b(DUNGEON_HGT);
	wr_u16b(DUNGEON_WID);


	/*** Simple "Run-Length-Encoding" of cave ***/

	/* Note that this will induce two wasted bytes */
	count = 0;
	prev_char = 0;

	/* Dump the cave */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			/* Extract the important cave_info flags */
			tmp8u = (cave_info[y][x] & (IMPORTANT_FLAGS));

			/* If the run is broken, or too full, flush it */
			if ((tmp8u != prev_char) || (count == UCHAR_MAX))
			{
				wr_byte((byte)count);
				wr_byte((byte)prev_char);
				prev_char = tmp8u;
				count = 1;
			}

			/* Continue the run */
			else
			{
				count++;
			}
		}
	}

	/* Flush the data (if any) */
	if (count)
	{
		wr_byte(count);
		wr_byte(prev_char);
	}


	/*** Simple "Run-Length-Encoding" of cave ***/

	/* Note that this will induce two wasted bytes */
	count = 0;
	prev_char = 0;

	/* Dump the cave */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			/* Extract a byte */
			tmp8u = cave_feat[y][x];

			/* If the run is broken, or too full, flush it */
			if ((tmp8u != prev_char) || (count == UCHAR_MAX))
			{
				wr_byte(count);
				wr_byte(prev_char);
				prev_char = tmp8u;
				count = 1;
			}

			/* Continue the run */
			else
			{
				count++;
			}
		}
	}

	/* Flush the data (if any) */
	if (count)
	{
		wr_byte(count);
		wr_byte(prev_char);
	}


	/*** Compact ***/

	compact_objects(0);	/* Compact the objects */
	compact_monsters(0);	/* Compact the monsters */


	/*** Dump objects ***/

	/* Total objects */
	wr_u16b(o_max);

	/* Dump the objects */
	for (i = 1; i < o_max; i++) wr_item(o_list+i);


	/*** Dump the monsters ***/

	/* Total monsters */
	wr_u16b(mon_max);

	/* Dump the monsters */
	for (i = 1; i < mon_max; i++) wr_monster(mon_list+i);
}


/*! write the messages */
static void wr_messages(void)
{
	/* Dump the number of "messages" */
	u16b i = messages_num();
	if (OPTION(compress_savefile) && (40 < i)) i = 40;
	wr_u16b(i);

	/* Dump the messages (oldest first!) */
	while(0<i)
		{
		wr_string(message_str(--i));
		wr_u16b(message_type(i));
		}
}


/*
 * Actually write a save-file
 */
static bool wr_savefile_new(void)
{
	int i;
	u32b now = time((time_t *)0); /* Guess at the current time */

	sf_xtra = 0L; /* Note the operating system */
	sf_when = now; /* Note when the file was saved */
	sf_saves++; /* Note the number of saves */


	/*** Actually write the file ***/

	/* Dump the file header */
	xor_byte = 0;
	wr_byte(VERSION_MAJOR);
	xor_byte = 0;
	wr_byte(VERSION_MINOR);
	xor_byte = 0;
	wr_byte(VERSION_PATCH);
	xor_byte = 0;
	wr_byte(VERSION_EXTRA);


	/* Reset the checksum */
	v_stamp = 0L;
	x_stamp = 0L;

	wr_u32b(sf_xtra);	/* Operating system */
	wr_u32b(sf_when);	/* Time file last saved */
	wr_u16b(sf_lives);	/* Number of past lives */
	wr_u16b(sf_saves);	/* Number of times saved */

	wr_randomizer();	/* Write the RNG state */
	wr_options();	/* Write the boolean "options" */
	wr_messages();	/* Dump the number of "messages" */


	/* Dump the monster lore */
	wr_u16b(z_info->r_max);
	for (i = 0; i < z_info->r_max; i++) wr_lore(i);


	/* Dump the object memory */
	wr_u16b(z_info->k_max);
	for (i = 0; i < z_info->k_max; i++) wr_xtra(i);


	/* Hack -- Dump the quests */
	wr_u16b(MAX_Q_IDX);
	for (i = 0; i < MAX_Q_IDX; i++)
	{
		wr_byte(q_list[i].level);
		wr_byte(0);
		wr_byte(0);
		wr_byte(0);
	}

	/* Hack -- Dump the artifacts */
	wr_u16b(z_info->a_max);
	for (i = 0; i < z_info->a_max; i++)
	{
		artifact_type *a_ptr = &object_type::a_info[i];
		wr_byte(a_ptr->cur_num);
		wr_byte(0);
		wr_byte(0);
		wr_byte(0);
	}


	/* Write the "extra" information */
	wr_extra();
	wr_extra2();

	/* Write the inventory.*/ 
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		const object_type* const o_ptr = &p_ptr->inventory[i];

		if (!o_ptr->k_idx) continue;	/* Skip non-objects */
		wr_u16b((u16b)i);	/* Dump index */
		wr_item(o_ptr);	/* Dump object */
	}

	/* Add a sentinel */
	wr_u16b(0xFFFF);
	
	/* Write randart information */
	if (OPTION(adult_rand_artifacts)) wr_randarts();


	/* Dump the stores */
	wr_u16b(MAX_STORES);
	for (i = 0; i < MAX_STORES; i++) wr_store(&store[i]);


	/* Player is not dead, write the dungeon */
	if (!p_ptr->is_dead)
	{
		wr_dungeon();	/* Dump the dungeon */
	}


	/* Write the "value check-sum" */
	wr_u32b(v_stamp);

	/* Write the "encoded checksum" */
	wr_u32b(x_stamp);


	/* Error in save */
	if (ferror(fff) || (fflush(fff) == EOF)) return FALSE;

	/* Successful save */
	return TRUE;
}


/*
 * Medium level player saver
 *
 * XXX XXX XXX Angband 2.8.0 will use "fd" instead of "fff" if possible
 */
static bool save_player_aux(const char* const name)
{
	bool ok = FALSE;

	int fd;

	int mode = 0644;


	/* No file yet */
	fff = NULL;


	/* File type is "SAVE" */
	FILE_TYPE(FILE_TYPE_SAVE);


	/* Grab permissions */
	safe_setuid_grab();

	/* Create the savefile */
	fd = fd_make(name, mode);

	/* Drop permissions */
	safe_setuid_drop();

	/* File is okay */
	if (fd >= 0)
	{
		/* Close the "fd" */
		fd_close(fd);

		/* Grab permissions */
		safe_setuid_grab();

		/* Open the savefile */
		fff = my_fopen(name, "wb");

		/* Drop permissions */
		safe_setuid_drop();

		/* Successful open */
		if (fff)
		{
			/* Write the savefile */
			if (wr_savefile_new()) ok = TRUE;

			/* Attempt to close it */
			if (my_fclose(fff)) ok = FALSE;
		}

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove "broken" files */
		if (!ok) fd_kill(name);

		/* Drop permissions */
		safe_setuid_drop();
	}


	/* Failure */
	if (!ok) return (FALSE);

	/* Successful save */
	character_saved = TRUE;

	/* Success */
	return (TRUE);
}



/*
 * Attempt to save the player in a savefile
 */
bool save_player(void)
{
	int result = FALSE;

	char safe[1024];


	/* New savefile */
	my_strcpy(safe, savefile, sizeof(safe));
	my_strcat(safe, ".new", sizeof(safe));

#ifdef VM
	/* Hack -- support "flat directory" usage on VM/ESA */
	my_strcpy(safe, savefile, sizeof(safe));
	my_strcat(safe, "n", sizeof(safe));
#endif /* VM */

	/* Grab permissions */
	safe_setuid_grab();

	/* Remove it */
	fd_kill(safe);

	/* Drop permissions */
	safe_setuid_drop();

	/* Attempt to save the player */
	if (save_player_aux(safe))
	{
		char temp[1024];

		/* Old savefile */
		my_strcpy(temp, savefile, sizeof(temp));
		my_strcat(temp, ".old", sizeof(temp));

#ifdef VM
		/* Hack -- support "flat directory" usage on VM/ESA */
		my_strcpy(temp, savefile, sizeof(temp));
		my_strcat(temp, "o", sizeof(temp));
#endif /* VM */

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove it */
		fd_kill(temp);

		/* Preserve old savefile */
		fd_move(savefile, temp);

		/* Activate new savefile */
		fd_move(safe, savefile);

		/* Remove preserved savefile */
		fd_kill(temp);

		/* Drop permissions */
		safe_setuid_drop();

		/* Hack -- Pretend the character was loaded */
		character_loaded = TRUE;

#ifdef VERIFY_SAVEFILE

		/* Lock on savefile */
		my_strcpy(temp, savefile, sizeof(temp));
		my_strcat(temp, ".lok", sizeof(temp));

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove lock file */
		fd_kill(temp);

		/* Drop permissions */
		safe_setuid_drop();

#endif /* VERIFY_SAVEFILE */

		/* Success */
		result = TRUE;
	}


	/* Return the result */
	return (result);
}
