/*
TOUCH_CHEATS.C (test)

The real cheats menu of the Android touch controls, as the game applies it:
touch_game.c's switches and one-time cheats, and touch_input.c's
touch_cheats_update, which reads the menu's asks from the host each frame.
The switches reach the game's own cheat globals, each one-time cheat runs
once, a script's own cheats are left alone, and nothing acts as a client in
another's game or outside a game; the camouflage switch is local player
1's unit's own. test_touch_cheats.py takes the cheat globals' structure,
the unit flags and the report's bits (config.inc), cheats.c's camouflage
(cheats.inc) and the code (under_test.inc).
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

/* local player 1, and its unit: flags and camouflage (players.h, units.h) */
struct player_datum { long unit_index; };
struct unit_datum { struct { unsigned long flags; real active_camouflage; } unit; };
static struct player_datum players[1] = { { 0 } };
static struct unit_datum units[1];
static long local_player = 0;
/* unit_get of no unit: the game's would fail its assertion */
static int bad_unit_reads;

/* reads of the players' globals outside a game: the game's has none yet
(players_globals is NULL before game_initialize) and would crash */
static int bad_player_reads;

static long local_player_get_player_index(short local_player_index)
{
	if (!playing)
		bad_player_reads++;
	return local_player_index == 0 ? local_player : NONE;
}

#define player_get(index) (&players[(index)])
#define unit_get(index) ((index) == NONE ? (bad_unit_reads++, &units[0]) : &units[(index)])
#define SET_FLAG(flags, bit, on) ((on) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))

/* the time freeze (time_freeze.c), and how often it was set */
static boolean frozen;
static int freeze_sets;

static void time_freeze_set(boolean on)
{
	frozen = on;
	freeze_sets++;
}

static boolean time_freeze_wanted(void)
{
	return frozen;
}

/* the one-time cheats (cheats.c), counted */
static int weapons, vehicles, powerups;

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

/* the host (host_touch.c) and the overlay's menu */
static int host_serial, host_wanted, host_actions, host_report, host_super_jump;

static void host_touch_cheats_read(int *request)
{
	request[0] = host_serial;
	request[1] = host_wanted;
	request[2] = host_actions;
	request[3] = host_super_jump;
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

#include "cheats.inc"
#include "under_test.inc"

static boolean camouflaged(void)
{
	return TEST_FLAG(units[0].unit.flags, _unit_active_camouflaged_bit);
}

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
		menu_switches(1 << 11);
		touch_cheats_update();
		CHECK(switches_on() == 0 && !camouflaged() && !frozen, "a switch past the menu's turned a cheat on");
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
		CHECK(weapons == 1 && vehicles == 1 && powerups == 1,
			"ran %d %d %d times, not once each", weapons, vehicles, powerups);
		CHECK(switches_on() == 0 && !camouflaged(), "a one-time cheat turned a switch on");
		return 0;
	}
	/* camouflage, the tenth switch: on, reported, kept while other switches
	change, and off as a powerup ends, super camouflage too */
	CASE("camouflage-switch")
	{
		menu_switches(1 << 9);
		touch_cheats_update();
		CHECK(camouflaged() && units[0].unit.active_camouflage == 1.0f, "camouflage not on");
		CHECK(host_report & (1 << 9), "camouflage not reported, %#x", host_report);
		menu_switches(1 << 9 | 1 << 0);
		touch_cheats_update();
		CHECK(!TEST_FLAG(units[0].unit.flags, _unit_super_camouflaged_bit),
			"another switch's change turned camouflage on again (super camouflage)");
		CHECK(cheat.deathless_player && camouflaged(), "god mode with camouflage not both on");
		/* (a second camouflage powerup's super camouflage, players.c) */
		units[0].unit.flags |= FLAG(_unit_super_camouflaged_bit);
		menu_switches(1 << 0);
		touch_cheats_update();
		CHECK(!camouflaged() && !TEST_FLAG(units[0].unit.flags, _unit_super_camouflaged_bit),
			"camouflage not turned off: flags %#lx", units[0].unit.flags);
		CHECK(cheat.deathless_player, "god mode went off with camouflage");
		CHECK(!(host_report & (1 << 9)), "camouflage still reported, %#x", host_report);
		return 0;
	}
	/* the super jump's strength: 4 (the original's) until the menu sets it,
	then as set, kept within 1 and 4 */
	CASE("super-jump-strength")
	{
		touch_cheats_update();
		CHECK(touch_game_super_jump_scale() == 4.0f, "%g before the menu sets it, not 4",
			touch_game_super_jump_scale());
		host_super_jump = 225;
		touch_cheats_update();
		CHECK(touch_game_super_jump_scale() == 2.25f, "%g, not 2.25", touch_game_super_jump_scale());
		host_super_jump = 20;
		touch_cheats_update();
		CHECK(touch_game_super_jump_scale() == 1.0f, "%g: below a normal jump", touch_game_super_jump_scale());
		host_super_jump = 5000;
		touch_cheats_update();
		CHECK(touch_game_super_jump_scale() == 4.0f, "%g: above the original's", touch_game_super_jump_scale());
		/* (set outside a game too: it is the menu's, not a cheat that acts) */
		playing = FALSE;
		host_super_jump = 150;
		touch_cheats_update();
		CHECK(touch_game_super_jump_scale() == 1.5f, "%g, not 1.5 outside a game", touch_game_super_jump_scale());
		return 0;
	}
	/* the time freeze, the eleventh switch: on, reported, set once while
	other switches change, and off */
	CASE("time-freeze-switch")
	{
		menu_switches(1 << 10);
		touch_cheats_update();
		CHECK(frozen && (host_report & (1 << 10)), "time freeze not on and reported, %#x", host_report);
		CHECK(switches_on() == 0, "the time freeze turned a cheat global on");
		menu_switches(1 << 10 | 1 << 1);
		touch_cheats_update();
		CHECK(freeze_sets == 1, "set %d times: another switch's change set it again", freeze_sets);
		menu_switches(1 << 1);
		touch_cheats_update();
		CHECK(!frozen && !(host_report & (1 << 10)), "time freeze not off");
		return 0;
	}
	/* a camouflage powerup picked up shows as the switch on, and the switch turns it off */
	CASE("camouflage-powerup")
	{
		units[0].unit.flags |= FLAG(_unit_active_camouflaged_bit);
		touch_cheats_update();
		CHECK(host_report & (1 << 9), "a powerup's camouflage not reported");
		menu_switches(0);
		touch_cheats_update();
		CHECK(!camouflaged(), "the switch did not end a powerup's camouflage");
		return 0;
	}
	/* no unit (dead, before spawning): camouflage asked does nothing and reads off */
	CASE("camouflage-no-unit")
	{
		players[0].unit_index = NONE;
		menu_switches(1 << 9);
		touch_cheats_update();
		CHECK(!(host_report & (1 << 9)), "camouflage reported without a unit");
		CHECK(bad_unit_reads == 0, "read a unit that is not there %d times", bad_unit_reads);
		local_player = NONE;
		menu_switches(0);
		touch_cheats_update();
		CHECK(!(host_report & (1 << 9)), "camouflage reported without a player");
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
		CHECK(bad_player_reads == 0, "read the players' globals %d times outside a game (a crash at start)",
			bad_player_reads);
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
