/* File: defines.h */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */


/*
 * Do not edit this file unless you know *exactly* what you are doing.
 *
 * Some of the values in this file were chosen to preserve game balance,
 * while others are hard-coded based on the format of old save-files, the
 * definition of arrays in various places, mathematical properties, fast
 * computation, storage limits, or the format of external text files.
 *
 * Changing some of these values will induce crashes or memory errors or
 * savefile mis-reads.  Most of the comments in this file are meant as
 * reminders, not complete descriptions, and even a complete knowledge
 * of the source may not be sufficient to fully understand the effects
 * of changing certain definitions.
 *
 * Lastly, note that the code does not always use the symbolic constants
 * below, and sometimes uses various hard-coded values that may not even
 * be defined in this file, but which may be related to definitions here.
 * This is of course bad programming practice, but nobody is perfect...
 *
 * For example, there are MANY things that depend on the screen being
 * 80x24, with the top line used for messages, the bottom line being
 * used for status, and exactly 22 lines used to show the dungeon.
 * Just because your screen can hold 46 lines does not mean that the
 * game will work if you try to use 44 lines to show the dungeon.
 *
 * You have been warned.
 */


/*
 * Name of the version/variant
 */
#define VERSION_NAME "Zaiband"


/*
 * Current version string
 */
#define VERSION_STRING	"3.0.10 alpha"


/*
 * Current version numbers
 */
#define VERSION_MAJOR	3
#define VERSION_MINOR	0
#define VERSION_PATCH	9
#define VERSION_EXTRA	0

/*
 * Version of random artifact code.
 */
#define RANDART_VERSION	62

/*
 * Number of grids in each block (vertically)
 * Probably hard-coded to 11, see "generate.c"
 */
#define BLOCK_HGT	11

/*
 * Number of grids in each block (horizontally)
 * Probably hard-coded to 11, see "generate.c"
 */
#define BLOCK_WID	11


/*
 * Number of grids in each panel (vertically)
 * Must be a multiple of BLOCK_HGT
 */
#define PANEL_HGT	11

/*
 * Number of grids in each panel (horizontally)
 * Must be a multiple of BLOCK_WID
 */
#define PANEL_WID	(use_bigtile ? 16 : 33)

#define ROW_MAP			1
#define COL_MAP			13


/*
 * Number of grids in each screen (vertically)
 * Must be a multiple of PANEL_HGT (at least 2x)
 */
#define SCREEN_HGT	(Term->hgt - ROW_MAP - 1)

/*
 * Number of grids in each screen (horizontally)
 * Must be a multiple of PANEL_WID (at least 2x)
 */
#define SCREEN_WID	((Term->wid - COL_MAP - 1) / (use_bigtile ? 2 : 1))


/*
 * Number of grids in each dungeon (horizontally)
 * Must be a multiple of SCREEN_HGT
 * Must be less or equal to 256
 */
#define DUNGEON_HGT		66

/*
 * Number of grids in each dungeon (vertically)
 * Must be a multiple of SCREEN_WID
 * Must be less or equal to 256
 */
#define DUNGEON_WID		198

#define TOWN_WID 66
#define TOWN_HGT 22


/*
 * Maximum amount of Angband windows.
 */
#define ANGBAND_TERM_MAX 8


/*
 * Player sex constants (hard-coded by save-files, arrays, etc)
 */
enum player_gender	{
					SEX_FEMALE = 0,
					SEX_MALE
					};

/*
 * Maximum number of player "sex" types (see "table.c", etc)
 */
#define MAX_SEXES            SEX_MALE+1


/*
 * Maximum amount of starting equipment
 */
#define MAX_START_ITEMS	4


/*
 * Number of tval/min-sval/max-sval slots per ego_item
 */
#define EGO_TVALS_MAX 3


/*
 * Hack -- Maximum number of quests
 */
#define MAX_Q_IDX	4

/*
 * Maximum number of high scores in the high score file
 */
#define MAX_HISCORES	100


/**
 * Maximum dungeon level.  The player can never reach this level
 * in the dungeon, and this value is used for various calculations
 * involving object and monster creation.  It must be at least 100.
 * Setting it below 128 may prevent the creation of some objects.
 */
#define MAX_DEPTH	128


/*
 * Maximum size of the "view" array (see "cave.c")
 * Note that the "view radius" will NEVER exceed 20, and even if the "view"
 * was octagonal, we would never require more than 1520 entries in the array.
 */
#define VIEW_MAX 1536

/*
 * Maximum size of the "temp" array (see "cave.c")
 * Note that we must be as large as "VIEW_MAX" for proper functioning
 * of the "update_view()" function, and we must also be as large as the
 * largest illuminatable room, but no room is larger than 800 grids.  We
 * must also be large enough to allow "good enough" use as a circular queue,
 * to calculate monster flow, but note that the flow code is "paranoid".
 */
#define TEMP_MAX 1536


/*
 * Misc constants
 */
#define TOWN_DAWN		10000	/* Number of turns from dawn to dawn XXX */
#define MON_DRAIN_LIFE	2		/* Percent of player exp drained per hit */
#define USE_DEVICE      3		/* x> Harder devices x< Easier devices */

/*
 * Refueling constants
 */
#define FUEL_TORCH	5000	/* Maximum amount of fuel in a torch */
#define FUEL_LAMP	15000   /* Maximum amount of fuel in a lantern */


/*
 * More maximum values
 */
#define MAX_SIGHT	20	/* Maximum view distance */
#define MAX_RANGE	18	/* Maximum range (spells, etc) */



/*
 * Player constants
 */
#define PY_MAX_EXP	99999999L	/* Maximum exp */
#define PY_MAX_GOLD	999999999L	/* Maximum gold */
#define PY_MAX_LEVEL	50		/* Maximum level */

/*
 * Player "food" crucial values
 */
#define PY_FOOD_UPPER   20000	/* Upper limit on food counter */
#define PY_FOOD_MAX		15000	/* Food value (Bloated) */
#define PY_FOOD_FULL	10000	/* Food value (Normal) */
#define PY_FOOD_ALERT	2000	/* Food value (Hungry) */
#define PY_FOOD_WEAK	1000	/* Food value (Weak) */
#define PY_FOOD_FAINT	500		/* Food value (Fainting) */
#define PY_FOOD_STARVE	100		/* Food value (Starving) */

/*
 * Flags for player_type.object_awareness[]
 */
#define PY_OBJECT_AWARE 0x01
#define PY_OBJECT_TRIED 0x02 

/*
 * Maximum number of players spells
 */
#define PY_MAX_SPELLS 64

/*
 * Number of spells per book
 */
#define SPELLS_PER_BOOK 9


/*
 * Flags for player_type.spell_flags[]
 */
#define PY_SPELL_LEARNED    0x01 /* Spell has been learned */
#define PY_SPELL_WORKED     0x02 /* Spell has been successfully tried */
#define PY_SPELL_FORGOTTEN  0x04 /* Spell has been forgotten */


/*
 * Maximum number of "normal" pack slots, and the index of the "overflow"
 * slot, which can hold an item, but only temporarily, since it causes the
 * pack to "overflow", dropping the "last" item onto the ground.  Since this
 * value is used as an actual slot, it must be less than "INVEN_WIELD" (below).
 * Note that "INVEN_PACK" is probably hard-coded by its use in savefiles, and
 * by the fact that the screen can only show 23 items plus a one-line prompt.
 */
#define INVEN_ORIGIN	0
#define INVEN_STRICT_UB	24
#define INVEN_PACK		(INVEN_STRICT_UB-INVEN_ORIGIN-1)
#define INVEN_OVERFLOW	(INVEN_STRICT_UB-1)

/*
 * Indexes used for various "equipment" slots (hard-coded by savefiles, etc).
 */
#define INVEN_EQUIP_ORIGIN 24
#define INVEN_WIELD		(INVEN_EQUIP_ORIGIN+0)
#define INVEN_BOW       (INVEN_EQUIP_ORIGIN+1)
#define INVEN_LEFT      (INVEN_EQUIP_ORIGIN+2)
#define INVEN_RIGHT     (INVEN_EQUIP_ORIGIN+3)
#define INVEN_NECK      (INVEN_EQUIP_ORIGIN+4)
#define INVEN_LITE      (INVEN_EQUIP_ORIGIN+5)
#define INVEN_BODY      (INVEN_EQUIP_ORIGIN+6)
#define INVEN_OUTER     (INVEN_EQUIP_ORIGIN+7)
#define INVEN_ARM       (INVEN_EQUIP_ORIGIN+8)
#define INVEN_HEAD      (INVEN_EQUIP_ORIGIN+9)
#define INVEN_HANDS     (INVEN_EQUIP_ORIGIN+10)
#define INVEN_FEET      (INVEN_EQUIP_ORIGIN+11)
#define INVEN_EQUIP_STRICT_UB (INVEN_EQUIP_ORIGIN+12)

/*
 * Total number of inventory slots (hard-coded).
 * Should be signed because of negative-slot convention.
 */
#define INVEN_TOTAL	36


/*
 * Maximum number of objects allowed in a single dungeon grid.
 *
 * The main-screen has a minimum size of 24 rows, so we can always
 * display 23 objects + 1 header line.
 */
#define MAX_FLOOR_STACK			23

/* 
 * Timed effects 
 */ 
enum timed_effects
{ 
	TMD_FAST = 0, TMD_SLOW, TMD_BLIND, TMD_PARALYZED, TMD_CONFUSED, 
	TMD_AFRAID, TMD_IMAGE, TMD_POISONED, TMD_CUT, TMD_STUN, TMD_PROTEVIL, 
	TMD_INVULN, TMD_HERO, TMD_SHERO, TMD_SHIELD, TMD_BLESSED, TMD_SINVIS, 
	TMD_SINFRA, TMD_OPP_ACID, TMD_OPP_ELEC, TMD_OPP_FIRE, TMD_OPP_COLD, 
	TMD_OPP_POIS, 

	TMD_MAX 
}; 

/*
 * Skill indexes
 */
enum skills
{
	SKILL_DISARM,			/* Skill: Disarming */
	SKILL_DEVICE,			/* Skill: Magic Devices */
	SKILL_SAVE,				/* Skill: Saving throw */
	SKILL_STEALTH,			/* Skill: Stealth factor */
	SKILL_SEARCH,			/* Skill: Searching ability */
	SKILL_SEARCH_FREQUENCY,	/* Skill: Searching frequency */
	SKILL_TO_HIT_MELEE,		/* Skill: To hit (normal) */
	SKILL_TO_HIT_BOW,		/* Skill: To hit (shooting) */
	SKILL_TO_HIT_THROW,		/* Skill: To hit (throwing) */
	SKILL_DIGGING,			/* Skill: Digging */

	SKILL_MAX,
	SKILL_MAX_NO_RACE_CLASS = SKILL_MAX - 2	/* skills that have racial/class
											 * modifiers, scheduled 
											 * for obviation later */
};

/**
 * Indexes of the various "stats" (hard-coded by savefiles, etc).
 *
 * \sa ::A_MAX
 */
enum stat_index {
	A_STR = 0,
	A_INT,
	A_WIS,
	A_DEX,
	A_CON,
	A_CHR
};

/**
 * total number of stats
 *
 * \sa ::stat_index
 */
#define A_MAX (A_CHR+1)

/** modes for object_desc */
enum object_desc_mode	{	ODESC_BASE = 0, 
							ODESC_COMBAT, 
							ODESC_STORE,
							ODESC_FULL
						};

/*** General index values ***/


/*
 * keymap modes
 */
enum keymap_modes
{	
	KEYMAP_MODE_ORIG = 0,
	KEYMAP_MODE_ROGUE,

	KEYMAP_MODES	/* Total */
};


/*** Feature Indexes (see "lib/edit/feature.txt") ***/

/* Nothing */
#define FEAT_NONE		0x00

/* Various */
#define FEAT_FLOOR		0x01
#define FEAT_INVIS		0x02
#define FEAT_GLYPH		0x03
#define FEAT_OPEN		0x04
#define FEAT_BROKEN		0x05
#define FEAT_LESS		0x06
#define FEAT_MORE		0x07

/* Shops */
#define FEAT_SHOP_HEAD	0x08
#define FEAT_SHOP_TAIL	0x0F

/* Traps */
#define FEAT_TRAP_HEAD	0x10
#define FEAT_TRAP_TAIL	0x1F

#define IS_TRAP(A) (byte)(A)-FEAT_TRAP_HEAD<=FEAT_TRAP_TAIL-FEAT_TRAP_HEAD
inline bool is_trap(byte x) {	return IS_TRAP(x);	}

/* Doors */
#define FEAT_DOOR_HEAD	0x20
#define FEAT_DOOR_TAIL	0x2F

/* Extra */
#define FEAT_SECRET		0x30
#define FEAT_RUBBLE		0x31

/* Seams */
#define FEAT_MAGMA		0x32
#define FEAT_QUARTZ		0x33
#define FEAT_MAGMA_H	0x34
#define FEAT_QUARTZ_H	0x35
#define FEAT_MAGMA_K	0x36
#define FEAT_QUARTZ_K	0x37

/* Walls */
#define FEAT_WALL_EXTRA	0x38
#define FEAT_WALL_INNER	0x39
#define FEAT_WALL_OUTER	0x3A
#define FEAT_WALL_SOLID	0x3B
#define FEAT_PERM_EXTRA	0x3C
#define FEAT_PERM_INNER	0x3D
#define FEAT_PERM_OUTER	0x3E
#define FEAT_PERM_SOLID	0x3F



/*** Important artifact indexes (see "lib/edit/artifact.txt") ***/

#define ART_POWER			13
#define ART_MORGOTH			34
#define ART_GROND			111


/*
 * Hack -- first "normal" artifact in the artifact list.  All of
 * the artifacts with indexes from 1 to 15 are "special" (lights,
 * rings, amulets), and the ones from 16 to 127 are "normal".
 */
#define ART_MIN_NORMAL		16



/*** Monster blow constants ***/


#define MONSTER_BLOW_MAX 4

/*** Function flags ***/

/*
 * Bit flags for the "target_set" function
 *
 *	KILL: Target monsters
 *	LOOK: Describe grid fully
 *	XTRA: Currently unused flag
 *	GRID: Select from all grids
 */
#define TARGET_KILL		0x01
#define TARGET_LOOK		0x02
#define TARGET_XTRA		0x04
#define TARGET_GRID		0x08


/*
 * Bit flags for the "monster_desc" function
 */
#define MDESC_OBJE		0x01	/* Objective (or Reflexive) */
#define MDESC_POSS		0x02	/* Possessive (or Reflexive) */
#define MDESC_IND1		0x04	/* Indefinites for hidden monsters */
#define MDESC_IND2		0x08	/* Indefinites for visible monsters */
#define MDESC_PRO1		0x10	/* Pronominalize hidden monsters */
#define MDESC_PRO2		0x20	/* Pronominalize visible monsters */
#define MDESC_HIDE		0x40	/* Assume the monster is hidden */
#define MDESC_SHOW		0x80	/* Assume the monster is visible */


/*
 * Bit flags for the "get_item" function
 */
#define USE_EQUIP		0x01	/* Allow equip items */
#define USE_INVEN		0x02	/* Allow inven items */
#define USE_FLOOR		0x04	/* Allow floor items */



/*** Player flags ***/


/*
 * Bit flags for the "p_ptr->notice" variable
 */
#define PN_COMBINE		0x00000001L	/* Combine the pack */
#define PN_REORDER		0x00000002L	/* Reorder the pack */
/* xxx (many) */


/*
 * Bit flags for the "p_ptr->update" variable
 */
#define PU_BONUS		0x00000001L	/* Calculate bonuses */
#define PU_TORCH		0x00000002L	/* Calculate torch radius */
/* xxx (many) */
#define PU_HP			0x00000010L	/* Calculate chp and mhp */
#define PU_MANA			0x00000020L	/* Calculate csp and msp */
#define PU_SPELLS		0x00000040L	/* Calculate spells */
/* xxx (many) */
#define PU_FORGET_VIEW	0x00010000L	/* Forget field of view */
#define PU_UPDATE_VIEW	0x00020000L	/* Update field of view */
/* xxx (many) */
#define PU_FORGET_FLOW	0x00100000L	/* Forget flow data */
#define PU_UPDATE_FLOW	0x00200000L	/* Update flow data */
/* xxx (many) */
#define PU_MONSTERS		0x10000000L	/* Update monsters */
#define PU_DISTANCE		0x20000000L	/* Update distances */
/* xxx */
#define PU_PANEL		0x80000000L	/* Update panel */


/*
 * Bit flags for the "p_ptr->redraw" variable
 */
#define PR_MISC			0x00000001L	/* Display Race/Class */
#define PR_TITLE		0x00000002L	/* Display Title */
#define PR_LEV			0x00000004L	/* Display Level */
#define PR_EXP			0x00000008L	/* Display Experience */
#define PR_STATS		0x00000010L	/* Display Stats */
#define PR_ARMOR		0x00000020L	/* Display Armor */
#define PR_HP			0x00000040L	/* Display Hitpoints */
#define PR_MANA			0x00000080L	/* Display Mana */
#define PR_GOLD			0x00000100L	/* Display Gold */
#define PR_DEPTH		0x00000200L	/* Display Depth */
/* XXX deleted XXX */
#define PR_HEALTH		0x00000800L	/* Display Health Bar */
#define PR_CUT			0x00001000L	/* Display Extra (Cut) */
#define PR_STUN			0x00002000L	/* Display Extra (Stun) */
#define PR_HUNGER		0x00004000L	/* Display Extra (Hunger) */
/* xxx */
#define PR_BLIND		0x00010000L	/* Display Extra (Blind) */
#define PR_CONFUSED		0x00020000L	/* Display Extra (Confused) */
#define PR_AFRAID		0x00040000L	/* Display Extra (Afraid) */
#define PR_POISONED		0x00080000L	/* Display Extra (Poisoned) */
#define PR_STATE		0x00100000L	/* Display Extra (State) */
#define PR_SPEED		0x00200000L	/* Display Extra (Speed) */
#define PR_STUDY		0x00400000L	/* Display Extra (Study) */
#define PR_MAP			0x00800000L	/* Display Map */
#define PR_OPPOSE_ELEMENTS	0x01000000L	/* Display temp. resists */
#define PR_TEMP_SPELLS		0x02000000L	/* Display temp. spell effects */
#define PR_INVEN			0x04000000L	/* Display inven/equip */
#define PR_EQUIP			0x08000000L	/* Display equip/inven */
#define PR_MONLIST			0x10000000L /* Display monster list */
#define PR_MESSAGE			0x20000000L /* Display messages */
#define PR_MONSTER			0x40000000L /* Display monster recall */
#define PR_OBJECT			0x80000000L /* Display object recall */

/* Display Basic Info */
#define PR_BASIC \
	(PR_MISC | PR_TITLE | PR_STATS | PR_LEV |\
	 PR_EXP | PR_GOLD | PR_ARMOR | PR_HP |\
	 PR_MANA | PR_DEPTH | PR_HEALTH)

/* Display Extra Info */
#define PR_EXTRA \
	(PR_CUT | PR_STUN | PR_HUNGER | PR_BLIND |\
	 PR_CONFUSED | PR_AFRAID | PR_POISONED | PR_STATE |\
	 PR_SPEED | PR_STUDY | PR_OPPOSE_ELEMENTS)

/* anything status-ish */
#define PR_STATUS \
	(PR_HUNGER | PR_BLIND | PR_CONFUSED | PR_AFRAID |	\
	 PR_POISONED | PR_STATE | PR_SPEED | PR_STUDY |	\
	 PR_DEPTH | PR_CUT | PR_STUN)

/*** Cave flags ***/


/*
 * Special cave grid flags
 */
#define CAVE_MARK		0x01 	/* memorized feature */
#define CAVE_GLOW		0x02 	/* self-illuminating */
#define CAVE_ICKY		0x04 	/* part of a vault */
#define CAVE_ROOM		0x08 	/* part of a room */
#define CAVE_SEEN		0x10 	/* seen flag */
#define CAVE_VIEW		0x20 	/* view flag */
#define CAVE_TEMP		0x40 	/* temp flag */
#define CAVE_WALL		0x80 	/* wall flag */



/*** Object flags ***/


/*
 * Chest trap flags (see "tables.c")
 */
#define CHEST_LOSE_STR	0x01
#define CHEST_LOSE_CON	0x02
#define CHEST_POISON	0x04
#define CHEST_PARALYZE	0x08
#define CHEST_EXPLODE	0x10
#define CHEST_SUMMON	0x20


/*
 * Special object flags
 */
#define IDENT_SENSE     0x01	/* Item has been "sensed" */
#define IDENT_HIDE		0x02	/* Item is in store, but should not be displayed. */
#define IDENT_EMPTY     0x04	/* Item charges are known */
#define IDENT_KNOWN     0x08	/* Item abilities are known */
#define IDENT_STORE     0x10	/* Item is in the inventory of a store */
#define IDENT_MENTAL    0x20	/* Item information is known */
#define IDENT_CURSED    0x40	/* Item is temporarily cursed */
#define IDENT_BROKEN    0x80	/* Item is permanently worthless */


/*
 * The special inscriptions.
 */
#define INSCRIP_NULL            100
#define INSCRIP_TERRIBLE        100+1
#define INSCRIP_WORTHLESS       100+2
#define INSCRIP_CURSED          100+3
#define INSCRIP_BROKEN          100+4
#define INSCRIP_AVERAGE         100+5
#define INSCRIP_GOOD            100+6
#define INSCRIP_EXCELLENT       100+7
#define INSCRIP_SPECIAL         100+8
#define INSCRIP_UNCURSED        100+9
#define INSCRIP_INDESTRUCTIBLE  100+10

/*
 * Number of special inscriptions, plus one.
 */
#define MAX_INSCRIP			11

/*
 * number of 32-bit bitmaps used for object flags
 */
#define OBJECT_FLAG_STRICT_UB 3

/*
 * As of 2.7.8, the "object flags" are valid for all objects, and as
 * of 2.7.9, these flags are not actually stored with the object, but
 * rather in the object_kind, ego_item, and artifact structures.
 *
 * Note that "flags1" contains all flags dependant on "pval" (including
 * stat bonuses, but NOT stat sustainers), plus all "extra attack damage"
 * flags (SLAY_XXX and BRAND_XXX).
 *
 * Note that "flags2" contains all "resistances" (including "sustain" flags,
 * immunity flags, and resistance flags).  Note that "free action" and "hold
 * life" are no longer considered to be "immunities".
 *
 * Note that "flags3" contains everything else (including eight good flags,
 * seven unused flags, four bad flags, four damage ignoring flags, six weird
 * flags, and three cursed flags).
 */

#define TR1_STR             0x00000001L /* STR += "pval" */
#define TR1_INT             0x00000002L /* INT += "pval" */
#define TR1_WIS             0x00000004L /* WIS += "pval" */
#define TR1_DEX             0x00000008L /* DEX += "pval" */
#define TR1_CON             0x00000010L /* CON += "pval" */
#define TR1_CHR             0x00000020L /* CHR += "pval" */
#define TR1_XXX1            0x00000040L /* (reserved) */
#define TR1_XXX2            0x00000080L /* (reserved) */
#define TR1_STEALTH         0x00000100L /* Stealth += "pval" */
#define TR1_SEARCH          0x00000200L /* Search += "pval" */
#define TR1_INFRA           0x00000400L /* Infra += "pval" */
#define TR1_TUNNEL          0x00000800L /* Tunnel += "pval" */
#define TR1_SPEED           0x00001000L /* Speed += "pval" */
#define TR1_BLOWS           0x00002000L /* Blows += "pval" */
#define TR1_SHOTS           0x00004000L /* Shots += "pval" */
#define TR1_MIGHT           0x00008000L /* Might += "pval" */
#define TR1_SLAY_ANIMAL     0x00010000L /* Weapon slays animals */
#define TR1_SLAY_EVIL       0x00020000L /* Weapon slays evil */
#define TR1_SLAY_UNDEAD     0x00040000L /* Weapon slays undead */
#define TR1_SLAY_DEMON      0x00080000L /* Weapon slays demon */
#define TR1_SLAY_ORC        0x00100000L /* Weapon slays orc */
#define TR1_SLAY_TROLL      0x00200000L /* Weapon slays troll */
#define TR1_SLAY_GIANT      0x00400000L /* Weapon slays giant */
#define TR1_SLAY_DRAGON     0x00800000L /* Weapon slays dragon */
#define TR1_KILL_DRAGON     0x01000000L /* Weapon kills dragon */
#define TR1_KILL_DEMON      0x02000000L /* Weapon kills demon */
#define TR1_KILL_UNDEAD     0x04000000L /* Weapon "kills" undead */
#define TR1_BRAND_POIS      0x08000000L /* Weapon has poison brand */
#define TR1_BRAND_ACID      0x10000000L /* Weapon has acid brand */
#define TR1_BRAND_ELEC      0x20000000L /* Weapon has elec brand */
#define TR1_BRAND_FIRE      0x40000000L /* Weapon has fire brand */
#define TR1_BRAND_COLD      0x80000000L /* Weapon has cold brand */

#define TR2_SUST_STR        0x00000001L /* Sustain STR */
#define TR2_SUST_INT        0x00000002L /* Sustain INT */
#define TR2_SUST_WIS        0x00000004L /* Sustain WIS */
#define TR2_SUST_DEX        0x00000008L /* Sustain DEX */
#define TR2_SUST_CON        0x00000010L /* Sustain CON */
#define TR2_SUST_CHR        0x00000020L /* Sustain CHR */
#define TR2_XXX1            0x00000040L /* (reserved) */
#define TR2_XXX2            0x00000080L /* (reserved) */
#define TR2_XXX3            0x00000100L /* (reserved) */
#define TR2_XXX4            0x00000200L /* (reserved) */
#define TR2_XXX5            0x00000400L /* (reserved) */
#define TR2_XXX6            0x00000800L /* (reserved) */
#define TR2_IM_ACID         0x00001000L /* Immunity to acid */
#define TR2_IM_ELEC         0x00002000L /* Immunity to elec */
#define TR2_IM_FIRE         0x00004000L /* Immunity to fire */
#define TR2_IM_COLD         0x00008000L /* Immunity to cold */
#define TR2_RES_ACID        0x00010000L /* Resist acid */
#define TR2_RES_ELEC        0x00020000L /* Resist elec */
#define TR2_RES_FIRE        0x00040000L /* Resist fire */
#define TR2_RES_COLD        0x00080000L /* Resist cold */
#define TR2_RES_POIS        0x00100000L /* Resist poison */
#define TR2_RES_FEAR        0x00200000L /* Resist fear */
#define TR2_RES_LITE        0x00400000L /* Resist lite */
#define TR2_RES_DARK        0x00800000L /* Resist dark */
#define TR2_RES_BLIND       0x01000000L /* Resist blind */
#define TR2_RES_CONFU       0x02000000L /* Resist confusion */
#define TR2_RES_SOUND       0x04000000L /* Resist sound */
#define TR2_RES_SHARD       0x08000000L /* Resist shards */
#define TR2_RES_NEXUS       0x10000000L /* Resist nexus */
#define TR2_RES_NETHR       0x20000000L /* Resist nether */
#define TR2_RES_CHAOS       0x40000000L /* Resist chaos */
#define TR2_RES_DISEN       0x80000000L /* Resist disenchant */

#define TR3_SLOW_DIGEST     0x00000001L /* Slow digest */
#define TR3_FEATHER         0x00000002L /* Feather Falling */
#define TR3_LITE            0x00000004L /* Perma-Lite */
#define TR3_REGEN           0x00000008L /* Regeneration */
#define TR3_TELEPATHY       0x00000010L /* Telepathy */
#define TR3_SEE_INVIS       0x00000020L /* See Invis */
#define TR3_FREE_ACT        0x00000040L /* Free action */
#define TR3_HOLD_LIFE       0x00000080L /* Hold life */
#define TR3_XXX1            0x00000100L
#define TR3_XXX2            0x00000200L
#define TR3_XXX3            0x00000400L
#define TR3_XXX4            0x00000800L
#define TR3_IMPACT          0x00001000L /* Earthquake blows */
#define TR3_TELEPORT        0x00002000L /* Random teleportation */
#define TR3_AGGRAVATE       0x00004000L /* Aggravate monsters */
#define TR3_DRAIN_EXP       0x00008000L /* Experience drain */
#define TR3_IGNORE_ACID     0x00010000L /* Item ignores Acid Damage */
#define TR3_IGNORE_ELEC     0x00020000L /* Item ignores Elec Damage */
#define TR3_IGNORE_FIRE     0x00040000L /* Item ignores Fire Damage */
#define TR3_IGNORE_COLD     0x00080000L /* Item ignores Cold Damage */
#define TR3_XXX5            0x00100000L /* (reserved) */
#define TR3_XXX6            0x00200000L /* (reserved) */
#define TR3_BLESSED         0x00400000L /* Item has been blessed */
#define TR3_XXX7        	0x00800000L /* XXX Item can be activated XXX */
#define TR3_INSTA_ART       0x01000000L /* Item makes an artifact */
#define TR3_EASY_KNOW       0x02000000L /* Item is known if aware */
#define TR3_HIDE_TYPE       0x04000000L /* Item hides description */
#define TR3_SHOW_MODS       0x08000000L /* Item shows Tohit/Todam */
#define TR3_XXX8            0x10000000L /* (reserved) */
#define TR3_LIGHT_CURSE     0x20000000L /* Item has Light Curse */
#define TR3_HEAVY_CURSE     0x40000000L /* Item has Heavy Curse */
#define TR3_PERMA_CURSE     0x80000000L /* Item has Perma Curse */


/*
 * Hack -- flag set 1 -- mask for "pval-dependant" flags.
 * Note that all "pval" dependant flags must be in "flags1".
 */
#define TR1_PVAL_MASK \
	(TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | \
	 TR1_CON | TR1_CHR | TR1_XXX1 | TR1_XXX2 | \
	 TR1_STEALTH | TR1_SEARCH | TR1_INFRA | TR1_TUNNEL | \
	 TR1_SPEED | TR1_BLOWS | TR1_SHOTS | TR1_MIGHT)

/*
 * Flag set 3 -- mask for "ignore element" flags.
 */
#define TR3_IGNORE_MASK \
	(TR3_IGNORE_ACID | TR3_IGNORE_ELEC | TR3_IGNORE_FIRE | \
	 TR3_IGNORE_COLD )


/*
 * Hack -- special "xtra" object flag info (type)
 */
#define OBJECT_XTRA_TYPE_SUSTAIN	1
#define OBJECT_XTRA_TYPE_RESIST		2
#define OBJECT_XTRA_TYPE_POWER		3

/*
 * Hack -- special "xtra" object flag info (what flag set)
 */
#define OBJECT_XTRA_WHAT_SUSTAIN	2
#define OBJECT_XTRA_WHAT_RESIST		2
#define OBJECT_XTRA_WHAT_POWER		3

/*
 * Hack -- special "xtra" object flag info (base flag value)
 */
#define OBJECT_XTRA_BASE_SUSTAIN	TR2_SUST_STR
#define OBJECT_XTRA_BASE_RESIST		TR2_RES_POIS
#define OBJECT_XTRA_BASE_POWER		TR3_SLOW_DIGEST

/*
 * Hack -- special "xtra" object flag info (number of flags)
 */
#define OBJECT_XTRA_SIZE_SUSTAIN	6
#define OBJECT_XTRA_SIZE_RESIST		12
#define OBJECT_XTRA_SIZE_POWER		8


/*** Class flags ***/

#define CF_EXTRA_SHOT		0x00000001L	/* Extra shots */
#define CF_BRAVERY_30		0x00000002L	/* Gains resist fear at plev 30 */
#define CF_BLESS_WEAPON		0x00000004L	/* Requires blessed/hafted weapons */
#define CF_CUMBER_GLOVE		0x00000008L	/* Gloves disturb spellcasting */
#define CF_ZERO_FAIL		0x00000010L /* Fail rates can reach 0% */
#define CF_BEAM				0x00000020L /* Higher chance of spells beaming */
#define CF_CHOOSE_SPELLS	0x00000040L	/* Allow choice of spells */
#define CF_PSEUDO_ID_HEAVY	0x00000080L /* Allow heavy pseudo-id */
#define CF_PSEUDO_ID_IMPROV	0x00000100L /* Pseudo-id improves quicker with player-level */
#define CF_XXX10			0x00000200L
#define CF_XXX11			0x00000400L
#define CF_XXX12			0x00000800L
#define CF_XXX13			0x00001000L
#define CF_XXX14			0x00002000L
#define CF_XXX15			0x00004000L
#define CF_XXX16			0x00008000L
#define CF_XXX17			0x00010000L
#define CF_XXX18			0x00020000L
#define CF_XXX19			0x00040000L
#define CF_XXX20			0x00080000L
#define CF_XXX21			0x00100000L
#define CF_XXX22			0x00200000L
#define CF_XXX23			0x00400000L
#define CF_XXX24			0x00800000L
#define CF_XXX25			0x01000000L
#define CF_XXX26			0x02000000L
#define CF_XXX27			0x04000000L
#define CF_XXX28			0x08000000L
#define CF_XXX29			0x10000000L
#define CF_XXX30			0x20000000L
#define CF_XXX31			0x40000000L
#define CF_XXX32			0x80000000L


/*** Monster flags ***/


/*
 * Special Monster Flags (all temporary)
 */
#define MFLAG_VIEW	0x01	/* Monster is in line of sight */
/* xxx */
#define MFLAG_NICE	0x20	/* Monster is still being nice */
#define MFLAG_SHOW	0x40	/* Monster is recently memorized */
#define MFLAG_MARK	0x80	/* Monster is currently memorized */

/*
 * number of 32-bit bitmaps used for monster race
 * actual RF flags are in raceflag.h
 */
#define RACE_FLAG_STRICT_UB 3
#define RACE_FLAG_SPELL_STRICT_UB 3


/*
 * Redeclaraton of option macro, master is in option.h
 */
#define OPT_MAX						256


/*** Macro Definitions ***/

/*
 * Debugging tracker
 */
#ifdef NDEBUG
#define m_ptr_from_m_idx(i) (mon_list+i)
#else
#define m_ptr_from_m_idx(i) (assert(1<=(s16b)(i)),assert((s16b)(i)<mon_max),mon_list+i)
#endif

/*
 * Hack -- The main "screen"
 */
#define term_screen	(angband_term[0])

/** 
 * Get an item pointer.  Non-negative index is from inventory.
 * Negative index is from floor.
 */
#define get_o_ptr_from_inventory_or_floor(item) \
	((item >= 0) ? &p_ptr->inventory[item] : &o_list[0 - item])

/**
 * Determines if a map location is "meaningful"
 */
#define in_bounds(Y,X) \
	(((unsigned)(Y) < (unsigned)(DUNGEON_HGT)) && \
	 ((unsigned)(X) < (unsigned)(DUNGEON_WID)))

/**
 * Determines if a map location is fully inside the outer walls
 * This is more than twice as expensive as "in_bounds()", but
 * often we need to exclude the outer walls from calculations.
 */
#define in_bounds_fully(Y,X) \
	(((Y) > 0) && ((Y) < DUNGEON_HGT-1) && \
	 ((X) > 0) && ((X) < DUNGEON_WID-1))


/**
 * Determine if a "legal" grid is a "floor" grid
 *
 * Line 1 -- forbid doors, rubble, seams, walls
 *
 * Note the use of the new "CAVE_WALL" flag.
 */
#define cave_floor_bold(Y,X) \
	(!(cave_info[Y][X] & (CAVE_WALL)))

/**
 * Determine if a "legal" grid is a "clean" floor grid
 *
 * Line 1 -- forbid non-floors
 * Line 2 -- forbid normal objects
 */
#define cave_clean_bold(Y,X) \
	((cave_feat[Y][X] == FEAT_FLOOR) && \
	 (cave_o_idx[Y][X] == 0))

/**
 * Determine if a "legal" grid is an "empty" floor grid
 *
 * Line 1 -- forbid doors, rubble, seams, walls
 * Line 2 -- forbid player/monsters
 */
#define cave_empty_bold(Y,X) \
	(cave_floor_bold(Y,X) && \
	 (cave_m_idx[Y][X] == 0))

/**
 * Determine if a "legal" grid is an "naked" floor grid
 *
 * Line 1 -- forbid non-floors
 * Line 2 -- forbid normal objects
 * Line 3 -- forbid player/monsters
 */
#define cave_naked_bold(Y,X) \
	((cave_feat[Y][X] == FEAT_FLOOR) && \
	 (cave_o_idx[Y][X] == 0) && \
	 (cave_m_idx[Y][X] == 0))

#define cave_feat_in_range(Y,X,FEAT_A,FEAT_B)	\
	(((FEAT_A)<=cave_feat[Y][X]) && (cave_feat[Y][X]<=(FEAT_B)))

/**
 * Determine if a "legal" grid is "permanent"
 *
 * Line 1 -- perma-walls
 * Line 2-3 -- stairs
 * Line 4-5 -- shop doors
 */
#define cave_perma_bold(Y,X) \
	((cave_feat[Y][X] >= FEAT_PERM_EXTRA) || \
	 ((cave_feat[Y][X] == FEAT_LESS) || \
	  (cave_feat[Y][X] == FEAT_MORE)) || \
	 (cave_feat_in_range(Y,X,FEAT_SHOP_HEAD,FEAT_SHOP_TAIL)))

/*
 * Maximum number of macro trigger names
 */
#define MAX_MACRO_TRIGGER 200
#define MAX_MACRO_MOD 12


/*** Hack ***/


/*
 * Hack -- attempt to reduce various values
 */
#ifdef ANGBAND_LITE
# undef MACRO_MAX
# define MACRO_MAX	128
#endif


/*
 * Available graphic modes
 */
enum graphics_mode	{
					GRAPHICS_NONE = 0,
					GRAPHICS_ORIGINAL,
					GRAPHICS_ADAM_BOLT,
					GRAPHICS_DAVID_GERVAIS,
					GRAPHICS_PSEUDO
					};

/**
 * List of commands that will be auto-repeated
 *
 * \todo This string should be user-configurable.
 */
#define AUTO_REPEAT_COMMANDS "TBDoc+"


/*
 * Artifact activation index
 */
#define ACT_ILLUMINATION        0
#define ACT_MAGIC_MAP           1
#define ACT_CLAIRVOYANCE        2
#define ACT_PROT_EVIL           3
#define ACT_DISP_EVIL           4
#define ACT_HEAL1               5
#define ACT_HEAL2               6
#define ACT_CURE_WOUNDS         7
#define ACT_HASTE1              8
#define ACT_HASTE2              9
#define ACT_FIRE1               10
#define ACT_FIRE2               11
#define ACT_FIRE3               12
#define ACT_FROST1              13
#define ACT_FROST2              14
#define ACT_FROST3              15
#define ACT_FROST4              16
#define ACT_FROST5              17
#define ACT_ACID1               18
#define ACT_RECHARGE1           19
#define ACT_SLEEP               20
#define ACT_LIGHTNING_BOLT      21
#define ACT_ELEC2               22
#define ACT_BANISHMENT          23
#define ACT_MASS_BANISHMENT     24
#define ACT_IDENTIFY            25
#define ACT_DRAIN_LIFE1         26
#define ACT_DRAIN_LIFE2         27
#define ACT_BIZZARE             28
#define ACT_STAR_BALL           29
#define ACT_RAGE_BLESS_RESIST   30
#define ACT_PHASE               31
#define ACT_TRAP_DOOR_DEST      32
#define ACT_DETECT              33
#define ACT_RESIST              34
#define ACT_TELEPORT            35
#define ACT_RESTORE_LIFE        36
#define ACT_MISSILE             37
#define ACT_ARROW               38
#define ACT_REM_FEAR_POIS       39
#define ACT_STINKING_CLOUD      40
#define ACT_STONE_TO_MUD        41
#define ACT_TELE_AWAY           42
#define ACT_WOR                 43
#define ACT_CONFUSE             44
#define ACT_PROBE               45
#define ACT_FIREBRAND           46
#define ACT_STARLIGHT           47
#define ACT_MANA_BOLT           48
#define ACT_BERSERKER           49

#define ACT_MAX                 50

/* player_type.noscore flags */ 
#define NOSCORE_WIZARD          0x0002 
#define NOSCORE_DEBUG           0x0008 
#define NOSCORE_BORG            0x0010 
