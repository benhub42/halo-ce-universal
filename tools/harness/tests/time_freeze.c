/*
TIME_FREEZE.C (test)

The real time freeze of the cheats menu (port/linux/game/time_freeze.c):
while it is on only the players' root objects update (a unit on foot, or
the vehicle it rides), it goes off when the game clock goes back (a new
map, a reverted game) but not when turned on after a later map's clock, it
is refused in a network game, and it holds no world still during a
cinematic. test_time_freeze.py takes the code (under_test.inc).
*/

#include "harness.h"

/* the game */
static long clock_now;
static boolean network;
static void *cinematic_globals = &clock_now;
static boolean cinematic;

static long game_time_get(void)
{
	return clock_now;
}

static boolean network_game_is_active(void)
{
	return network;
}

static boolean cinematic_in_progress(void)
{
	return cinematic;
}

/* players and their units' parents (objects.h, players.h) */
struct player_datum { long unit_index; };
struct object_datum { struct { long parent_object_index; } object; };
static struct player_datum players[4];
static short player_count;
static struct object_datum objects[32];
struct data_iterator { long next; };
static int player_data_array;
static int *player_data = &player_data_array;

static void data_iterator_new(struct data_iterator *iterator, int *data)
{
	(void)data;
	iterator->next = 0;
}

static void *data_iterator_next(struct data_iterator *iterator)
{
	return iterator->next < player_count ? &players[iterator->next++] : NULL;
}

/* object_get of no object: the game's would fail its assertion */
static int bad_object_reads;
#define object_get(index) ((index) < 0 ? (bad_object_reads++, &objects[0]) : &objects[(index)])

#include "under_test.inc"

static void world(void)
{
	long index;

	for (index = 0; index < 32; index++)
		objects[index].object.parent_object_index = NONE;
	player_count = 1;
	players[0].unit_index = 3;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	world();
	/* on: only the player's unit updates; off: nothing is frozen */
	CASE("player-only")
	{
		clock_now = 100;
		time_freeze_set(TRUE);
		clock_now++;
		CHECK(time_freeze_active(), "not frozen");
		CHECK(time_freeze_object_updates(3), "the player's unit frozen");
		CHECK(!time_freeze_object_updates(4) && !time_freeze_object_updates(0), "another object updates");
		time_freeze_set(FALSE);
		CHECK(!time_freeze_active() && !time_freeze_wanted(), "still frozen after off");
		return 0;
	}
	/* in a vehicle the vehicle (the root, which carries the player) updates */
	CASE("vehicle")
	{
		objects[3].object.parent_object_index = 7;
		clock_now = 10;
		time_freeze_set(TRUE);
		CHECK(time_freeze_object_updates(7), "the player's vehicle frozen");
		CHECK(!time_freeze_object_updates(3), "a child counted as a root");
		/* (out of the vehicle the next tick) */
		objects[3].object.parent_object_index = NONE;
		clock_now++;
		CHECK(time_freeze_object_updates(3) && !time_freeze_object_updates(7), "roots not found again a tick on");
		return 0;
	}
	/* a dead player (no unit) freezes everything; every player's unit updates */
	CASE("players")
	{
		player_count = 3;
		players[0].unit_index = NONE;
		players[1].unit_index = 5;
		players[2].unit_index = 6;
		clock_now = 1;
		time_freeze_set(TRUE);
		CHECK(time_freeze_object_updates(5) && time_freeze_object_updates(6), "a player's unit frozen");
		CHECK(!time_freeze_object_updates(0), "object 0 updates for the player without a unit");
		CHECK(bad_object_reads == 0, "read the object of a player without a unit %d times", bad_object_reads);
		return 0;
	}
	/* the clock going back (a new map, a revert) turns it off; turned on
	at a new map's early clock it stays on */
	CASE("clock-back")
	{
		clock_now = 5000;
		time_freeze_set(TRUE);
		clock_now = 5001;
		CHECK(time_freeze_active(), "not frozen");
		clock_now = 20;
		CHECK(!time_freeze_active() && !time_freeze_wanted(), "still frozen at a new map");
		time_freeze_set(TRUE);
		clock_now = 21;
		CHECK(time_freeze_active(), "turned on at a new map's clock, it went off");
		return 0;
	}
	/* a network game: refused, and off if one begins */
	CASE("network")
	{
		network = TRUE;
		clock_now = 3;
		time_freeze_set(TRUE);
		CHECK(!time_freeze_wanted() && !time_freeze_active(), "frozen in a network game");
		network = FALSE;
		time_freeze_set(TRUE);
		network = TRUE;
		CHECK(!time_freeze_active() && !time_freeze_wanted(), "still frozen once a network game began");
		return 0;
	}
	/* a cinematic: the world goes on (its scripts end it), and the freeze stays on for after */
	CASE("cinematic")
	{
		clock_now = 3;
		time_freeze_set(TRUE);
		cinematic = TRUE;
		CHECK(!time_freeze_active(), "frozen during a cinematic");
		CHECK(time_freeze_wanted(), "the freeze went off for a cinematic");
		cinematic = FALSE;
		CHECK(time_freeze_active(), "not frozen after the cinematic");
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
