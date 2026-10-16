/*
TIME_FREEZE.C

The cheats menu's time freeze (port/android/.../TouchControls.java,
touch_game.c): the world stops but the players. While it is on, each tick
updates only the objects that are a player's unit or carry one (its
vehicle, and with it its passengers), as objects_update already updates
only the players' units on every other tick of the double speed powerup;
the actors do not think and the scripts and recorded animations do not run
(game.c). Effects, the HUD, the players' own updates and the game clock go
on. What the players fire stops where it is until the world goes on.

Only in a local game, and never during a cinematic (scripts that do not run
would never end it). Off again when the game clock goes back: a new map, or
a reverted game, so that a level's opening scripts run.
*/

#include "cseries.h"
#include "cutscene/cinematics.h"
#include "game/game.h"
#include "game/players.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "time_freeze.h"

/* network_game_globals.c's */
boolean network_game_is_active(void);

/* the root objects that update while frozen: one for each player */
#define TIME_FREEZE_MAXIMUM_ROOTS 16

static boolean time_freeze_on;
/* the game clock when last seen: it going back turns the freeze off */
static long time_freeze_clock;
static long time_freeze_roots[TIME_FREEZE_MAXIMUM_ROOTS];
static short time_freeze_root_count;
/* the tick the roots are of (NONE: none yet) */
static long time_freeze_roots_tick = NONE;

void time_freeze_set(boolean on)
{
	time_freeze_on = on && !network_game_is_active();
	time_freeze_clock = game_time_get();
	time_freeze_roots_tick = NONE;
}

boolean time_freeze_wanted(void)
{
	return time_freeze_on;
}

boolean time_freeze_active(void)
{
	long clock;

	if (!time_freeze_on)
		return FALSE;
	clock = game_time_get();
	if (clock < time_freeze_clock || network_game_is_active())
	{
		time_freeze_on = FALSE;
		return FALSE;
	}
	time_freeze_clock = clock;
	return !(cinematic_globals && cinematic_in_progress());
}

/* each player's unit's root object, once a tick */
static void time_freeze_find_roots(void)
{
	struct data_iterator iterator;
	struct player_datum *player;

	time_freeze_root_count = 0;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL &&
		time_freeze_root_count < TIME_FREEZE_MAXIMUM_ROOTS)
	{
		long object_index = player->unit_index;
		short depth;

		if (object_index == NONE)
			continue;
		/* (a parent's chain is short: a unit in a seat of a vehicle) */
		for (depth = 0; depth < 8 && object_get(object_index)->object.parent_object_index != NONE; depth++)
			object_index = object_get(object_index)->object.parent_object_index;
		time_freeze_roots[time_freeze_root_count++] = object_index;
	}
	time_freeze_roots_tick = game_time_get();
}

boolean time_freeze_object_updates(long object_index)
{
	short root;

	if (time_freeze_roots_tick != game_time_get())
		time_freeze_find_roots();
	for (root = 0; root < time_freeze_root_count; root++)
	{
		if (time_freeze_roots[root] == object_index)
			return TRUE;
	}
	return FALSE;
}
