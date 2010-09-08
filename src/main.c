/*
 * File: main.c
 * Purpose: Core game initialisation for UNIX (and other) machines
 *
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


/*
 * Some machines have a "main()" function in their "main-xxx.c" file,
 * all the others use this file for their "main()" function.
 */


#if defined(WIN32_CONSOLE_MODE) \
    || (!defined(WINDOWS) && !defined(RISCOS)) \
    || defined(USE_SDL)

#include "main.h"
#include "game-cmd.h"

/*
 * List of the available modules in the order they are tried.
 */
static const struct module modules[] =
{
#ifdef USE_GTK
	{ "gtk", help_gtk, init_gtk },
#endif /* USE_GTK */

#ifdef USE_X11
	{ "x11", help_x11, init_x11 },
#endif /* USE_X11 */

#ifdef USE_SDL
	{ "sdl", help_sdl, init_sdl },
#endif /* USE_SDL */

#ifdef USE_GCU
	{ "gcu", help_gcu, init_gcu },
#endif /* USE_GCU */
};


#ifdef USE_SOUND

/*
 * List of sound modules in the order they should be tried.
 */
static const struct module sound_modules[] =
{
#ifdef SOUND_SDL
	{ "sdl", "SDL_mixer sound module", init_sound_sdl },
#endif /* SOUND_SDL */

	{ "dummy", "Dummy module", NULL },
};

#endif


/*
 * A hook for "quit()".
 *
 * Close down, then fall back into "quit()".
 */
static void quit_hook(cptr s)
{
	int j;

	/* Unused parameter */
	(void)s;

	/* Scan windows */
	for (j = ANGBAND_TERM_MAX - 1; j >= 0; j--)
	{
		/* Unused */
		if (!angband_term[j]) continue;

		/* Nuke it */
		term_nuke(angband_term[j]);
	}
}



/*
 * SDL needs a look-in
 */
#ifdef USE_SDL
# include "SDL.h"
#endif


/*
 * Initialize and verify the file paths, and the score file.
 *
 * Use the ANGBAND_PATH environment var if possible, else use
 * DEFAULT_PATH, and in either case, branch off appropriately.
 *
 * First, we'll look for the ANGBAND_PATH environment variable,
 * and then look for the files in there.  If that doesn't work,
 * we'll try the DEFAULT_PATH constant.  So be sure that one of
 * these two things works...
 *
 * We must ensure that the path ends with "PATH_SEP" if needed,
 * since the "init_file_paths()" function will simply append the
 * relevant "sub-directory names" to the given path.
 *
 * Make sure that the path doesn't overflow the buffer.  We have
 * to leave enough space for the path separator, directory, and
 * filenames.
 */
static void init_stuff(void)
{
	char path[1024];

	cptr tail = NULL;


	/* Use the angband_path, or a default */
	my_strcpy(path, tail ? tail : DEFAULT_PATH, sizeof(path));

	/* Make sure it's terminated */
	path[511] = '\0';

	/* Hack -- Add a path separator (only if needed) */
	if (!suffix(path, PATH_SEP)) my_strcat(path, PATH_SEP, sizeof(path));

	/* Initialize */
	init_file_paths(path);
}



/*
 * Handle a "-d<what>=<path>" option
 *
 * The "<what>" can be any string starting with the same letter as the
 * name of a subdirectory of the "lib" folder (i.e. "i" or "info").
 *
 * The "<path>" can be any legal path for the given system, and should
 * not end in any special path separator (i.e. "/tmp" or "~/.ang-info").
 */
static void change_path(cptr info)
{
	if (!info || !info[0])
		quit_fmt("Try '-d<path>'.", info);

	string_free(ANGBAND_DIR_USER);
	ANGBAND_DIR_USER = string_make(info);
}




#ifdef SET_UID

/*
 * Find a default user name from the system.
 */
static void user_name(char *buf, size_t len, int id)
{
	struct passwd *pw = getpwuid(id);

	/* Default to PLAYER */
	if (!pw)
	{
		my_strcpy(buf, "PLAYER", len);
		return;
	}

	/* Capitalise and copy */
	strnfmt(buf, len, "%^s", pw->pw_name);
}

#endif /* SET_UID */

static bool new_game;

/*
 * Pass the appropriate "Initialisation screen" command to the game,
 * getting user input if needed.
 */ 
static game_command get_init_cmd()
{
	game_command cmd;

	/* Wait for response */
	pause_line(Term->hgt - 1);

	if (new_game)
		cmd.command = CMD_NEWGAME;
	else
		/* This might be modified to supply the filename in future. */
		cmd.command = CMD_LOADFILE;

	return cmd;
}


/*
 * Simple "main" function for multiple platforms.
 *
 * Note the special "--" option which terminates the processing of
 * standard options.  All non-standard options (if any) are passed
 * directly to the "init_xxx()" function.
 */
int main(int argc, char *argv[])
{
	int i;

	bool done = FALSE;

	const char *mstr = NULL;

	bool args = TRUE;


	/* Save the "program name" XXX XXX XXX */
	argv0 = argv[0];


#ifdef SET_UID

	/* Default permissions on files */
	(void)umask(022);

#endif /* SET_UID */


	/* Get the file paths */
	init_stuff();


#ifdef SET_UID

	/* Get the user id */
	player_uid = getuid();

	/* Save the effective GID for later recall */
	player_egid = getegid();

#endif /* SET_UID */


	/* Drop permissions */
	safe_setuid_drop();


#ifdef SET_UID

	/* Get the "user name" as a default player name */
	user_name(op_ptr->full_name, sizeof(op_ptr->full_name), player_uid);

#ifdef PRIVATE_USER_PATH

	/* Create directories for the users files */
	create_user_dirs();

#endif /* PRIVATE_USER_PATH */

#endif /* SET_UID */


	/* Process the command line arguments */
	for (i = 1; args && (i < argc); i++)
	{
		cptr arg = argv[i];

		/* Require proper options */
		if (*arg++ != '-') goto usage;

		/* Analyze option */
		switch (*arg++)
		{
			case 'N':
			case 'n':
			{
				new_game = TRUE;
				break;
			}

			case 'L':
			{
				new_save = TRUE;
				break;
			}

			case 'F':
			case 'f':
			{
				arg_fiddle = TRUE;
				break;
			}

			case 'W':
			case 'w':
			{
				arg_wizard = TRUE;
				break;
			}

			case 'V':
			case 'v':
			{
				arg_sound = TRUE;
				break;
			}

			case 'G':
			case 'g':
			{
				/* Default graphics tile */
				arg_graphics = GRAPHICS_ADAM_BOLT;
				break;
			}

			case 'u':
			case 'U':
			{
				if (!*arg) goto usage;

				/* Get the savefile name */
				my_strcpy(op_ptr->full_name, arg, sizeof(op_ptr->full_name));
				continue;
			}

			case 'm':
			case 'M':
			{
				if (!*arg) goto usage;
				mstr = arg;
				continue;
			}

			case 'd':
			case 'D':
			{
				change_path(arg);
				continue;
			}

			case '-':
			{
				argv[i] = argv[0];
				argc = argc - i;
				argv = argv + i;
				args = FALSE;
				break;
			}

			default:
			usage:
			{
				/* Dump usage information */
				puts("Usage: angband [options] [-- subopts]");
				puts("  -n             Start a new character");
				puts("  -L             Load a new-format save file");
				puts("  -w             Resurrect dead character (marks savefile)");
				puts("  -f             Request fiddle (verbose) mode");
				puts("  -v             Request sound mode");
				puts("  -g             Request graphics mode");
				puts("  -u<who>        Use your <who> savefile");
				puts("  -d<path>       Store pref files and screendumps in <path>");
				puts("  -m<sys>        Use module <sys>, where <sys> can be:");

				/* Print the name and help for each available module */
				for (i = 0; i < (int)N_ELEMENTS(modules); i++)
				{
					printf("     %s   %s\n",
					       modules[i].name, modules[i].help);
				}

				/* Actually abort the process */
				quit(NULL);
			}
		}
		if (*arg) goto usage;
	}

	/* Hack -- Forget standard args */
	if (args)
	{
		argc = 1;
		argv[1] = NULL;
	}


	/* Try the modules in the order specified by modules[] */
	for (i = 0; i < (int)N_ELEMENTS(modules); i++)
	{
		/* User requested a specific module? */
		if (!mstr || (streq(mstr, modules[i].name)))
		{
			if (0 == modules[i].init(argc, argv))
			{
				ANGBAND_SYS = modules[i].name;
				done = TRUE;
				break;
			}
		}
	}

	/* Make sure we have a display! */
	if (!done) quit("Unable to prepare any 'display module'!");


	/* Process the player name */
	process_player_name(TRUE);

	/* Install "quit" hook */
	quit_aux = quit_hook;

#ifdef USE_SOUND

	/* Try the modules in the order specified by sound_modules[] */
	for (i = 0; i < (int)N_ELEMENTS(sound_modules) - 1; i++)
	{
		if (0 == sound_modules[i].init(argc, argv))
			break;
	}

#endif


	/* Catch nasty signals */
	signals_init();

	/* Set up the command hooks */
	if (get_game_command == NULL)
		get_game_command = get_init_cmd;

	/* Set up the display handlers and things. */
	init_display();

	/* Play the game */
	play_game();

	/* Free resources */
	cleanup_angband();

	/* Quit */
	quit(NULL);

	/* Exit */
	return (0);
}

#endif /* !defined(MACINTOSH) && !defined(WINDOWS) && !defined(RISCOS) */
