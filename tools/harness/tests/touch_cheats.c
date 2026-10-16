/*
TOUCH_CHEATS.C (test)

The real cheats menu of the Android touch controls, as the game applies it:
touch_game.c's switches and one-time cheats, and touch_input.c's
touch_cheats_update, which reads the menu's asks from the host each frame.
The switches reach the game's own cheat globals, each one-time cheat runs
once, a script's own cheats are left alone, and nothing acts as a client in
another's game or outside a game. test_touch_cheats.py takes the cheat
globals' structure and the report's bits (config.inc) and the code
(under_test.inc).
*/

#include "harness.h"

#include "config.inc"

struct cheat_globals cheat;

/* the game */
static int playing = TRUE;
static boolean client;

static int touch_game_playing(void)
{
	return playing;
}

static boolean network_game_distributed_client(void)
{
	return client;
}

/* the one-time cheats (cheats.c), counted */
static int weapons, vehicles, powerups, camouflages;

static void cheat_all_weapons(void)
{
	weapons++;
}

static void cheat_all_vehicles(void)
{
	vehicles++;
}

static void cheat_all_powerups(void)
{
	powerups++;
}

static void cheat_active_camouflage(void)
{
	camouflages++;
}

/* the host (host_touch.c) and the overlay's menu */
static int host_serial, host_wanted, host_actions, host_report;

static void host_touch_cheats_read(int *request)
{
	request[0] = host_serial;
	request[1] = host_wanted;
	request[2] = host_actions;
	host_actions = 0;
}

static void host_touch_cheats_report(int state)
{
	host_report = state;
}

static void menu_switches(int bits)
{
	host_wanted = bits;
	host_serial++;
}

static void menu_action(int actions)
{
	host_actions |= actions;
}

#include "under_test.inc"

static int switches_on(void)
{
	return (cheat.deathless_player != 0) + (cheat.infinite_ammo != 0) + (cheat.bottomless_clip != 0) +
		(cheat.jetpack != 0) + (cheat.super_jump != 0) + (cheat.medusa != 0) + (cheat.omnipotent != 0) +
		(cheat.bump_possession != 0) + (cheat.reflexive_damage_effects != 0) + (cheat.controller_enabled != 0);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* the menu's switches reach the game's globals, and the game says so */
	CASE("switches-apply")
	{
		menu_switches(1 << 0 | 1 << 1);
		touch_cheats_update();
		CHECK(cheat.deathless_player && cheat.infinite_ammo, "god mode and infinite ammo not on");
		CHECK(switches_on() == 2, "%d switches on, not 2", switches_on());
		CHECK((host_report & 0xffff) == (1 << 0 | 1 << 1), "reported %#x", host_report);
		CHECK((host_report & _touch_cheats_allowed) && (host_report & _touch_cheats_known), "reported %#x", host_report);
		menu_switches(1 << 0);
		touch_cheats_update();
		CHECK(cheat.deathless_player && !cheat.infinite_ammo, "infinite ammo not turned off");
		return 0;
	}
	/* each switch is its own cheat, in the menu's order */
	CASE("every-switch")
	{
		boolean *expected[] = { &cheat.deathless_player, &cheat.infinite_ammo, &cheat.bottomless_clip,
			&cheat.jetpack, &cheat.super_jump, &cheat.medusa, &cheat.omnipotent, &cheat.bump_possession,
			&cheat.reflexive_damage_effects };
		int index;

		for (index = 0; index < (int)(sizeof(expected) / sizeof(expected[0])); index++)
		{
			menu_switches(1 << index);
			touch_cheats_update();
			CHECK(*expected[index] && switches_on() == 1, "switch %d is not its own cheat", index);
			CHECK((host_report & 0xffff) == 1 << index, "switch %d reported as %#x", index, host_report);
		}
		menu_switches(1 << 9);
		touch_cheats_update();
		CHECK(switches_on() == 0, "a switch past the menu's turned a cheat on");
		return 0;
	}
	/* a script's cheat (b30's deathless player) stays while the menu does nothing */
	CASE("scripts-kept")
	{
		touch_cheats_update();
		cheat.deathless_player = TRUE;
		touch_cheats_update();
		touch_cheats_update();
		CHECK(cheat.deathless_player, "the menu put back a script's cheat without a change of its own");
		CHECK(host_report & (1 << 0), "a script's cheat not reported to the menu");
		return 0;
	}
	/* each one-time cheat runs once */
	CASE("actions-once")
	{
		menu_action(1 << 0);
		menu_action(1 << 1 | 1 << 2 | 1 << 3);
		touch_cheats_update();
		touch_cheats_update();
		CHECK(weapons == 1 && vehicles == 1 && powerups == 1 && camouflages == 1,
			"ran %d %d %d %d times, not once each", weapons, vehicles, powerups, camouflages);
		CHECK(switches_on() == 0, "a one-time cheat turned a switch on");
		return 0;
	}
	/* a client in another's game: the host's rules; its asks are dropped, not kept for later */
	CASE("client-refused")
	{
		client = TRUE;
		menu_switches(1 << 3);
		menu_action(1 << 0);
		touch_cheats_update();
		CHECK(!cheat.jetpack && weapons == 0, "cheats acted in another's game");
		CHECK(!(host_report & _touch_cheats_allowed) && (host_report & _touch_cheats_known),
			"reported %#x in another's game", host_report);
		client = FALSE;
		touch_cheats_update();
		CHECK(!cheat.jetpack && weapons == 0, "asks made in another's game acted after it");
		CHECK(host_report & _touch_cheats_allowed, "not allowed back in an own game");
		return 0;
	}
	/* outside a game (starting, between maps, the main menu) nothing acts */
	CASE("not-playing")
	{
		playing = FALSE;
		menu_switches(1 << 0);
		menu_action(1 << 1);
		touch_cheats_update();
		CHECK(switches_on() == 0 && vehicles == 0, "cheats acted outside a game");
		CHECK(!(host_report & _touch_cheats_allowed), "reported allowed outside a game");
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
