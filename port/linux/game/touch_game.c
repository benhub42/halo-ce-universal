/*
TOUCH_GAME.C

What the touch controls (port/linux/src/touch_input.c) need to know of the
game. The platform layer cannot see the game's types, so it asks here
(this file is compiled as the game's own sources are).
*/

#include "cseries.h"
#include "game/game.h"
#include "cutscene/cinematics.h"
#include "game/cheats.h"
#include "input/input.h"
#include "input/input_abstraction.h"
#include "interface/ui_widget.h"

/* asks whether a cinematic is playing that A would skip
(player_control.c, player_control_action_test_check_reset_input_blob); tests
cinematic_globals first because the controller is read before the game exists
during input_initialize and shell_initialize, and game_initialize makes it
after the game time globals that game_in_progress reads (game.c) */
int touch_game_cinematic_skippable(void)
{
	return cinematic_globals && game_in_progress() && cinematic_in_progress() &&
		cinematic_can_be_skipped();
}

/* asks whether a cinematic is playing, skippable or not: the player has no
controls, so the on-screen touch controls hide (touch_input.c) */
int touch_game_cinematic_playing(void)
{
	return cinematic_globals && game_in_progress() && cinematic_in_progress();
}

/* asks whether a game is being played: not while the game starts, nor
between maps, nor at the main menu (whose ui.map is a game too), where the
on-screen touch controls do not show (touch_input.c); cinematic_globals
first, as touch_game_cinematic_skippable has it */
int touch_game_playing(void)
{
	return cinematic_globals && game_in_progress() && !main_menu_is_active();
}

/* (touch_input.c's TOUCH_BUTTONS and the overlay count 16 buttons) */
typedef char verify_touch_buttons[NUMBER_OF_GAMEPAD_BUTTONS == 16 ? 1 : -1];

/* the game control on each of player 1's 16 controller buttons, as the
gamepad's buttons are numbered (input.h), -1 for none: the touch controls
name their buttons by them, so a profile's own mapping shows */
void touch_game_button_controls(int *controls)
{
	short buttons[NUMBER_OF_GAMEPAD_BUTTONS];
	short index;

	input_abstraction_port_button_controls(0, buttons);
	for (index = 0; index < NUMBER_OF_GAMEPAD_BUTTONS; index++)
	{
		controls[index] = buttons[index];
	}
}

/* ---------- the touch controls' cheats menu (TouchControls.java) */

/* network_game_globals.c's */
boolean network_game_distributed_client(void);

/* the cheats menu's switches, as bits: TouchControls.java's CHEAT_SWITCHES
has them in this order */
static boolean *touch_game_cheat_switch(int index)
{
	switch (index)
	{
	case 0: return &cheat.deathless_player;
	case 1: return &cheat.infinite_ammo;
	case 2: return &cheat.bottomless_clip;
	case 3: return &cheat.jetpack;
	case 4: return &cheat.super_jump;
	case 5: return &cheat.medusa;
	case 6: return &cheat.omnipotent;
	case 7: return &cheat.bump_possession;
	case 8: return &cheat.reflexive_damage_effects;
	default: return NULL;
	}
}

/* asks whether the cheats menu may act: in a game being played, and not as
a client in another's game, where the host's rules hold
(cheats_network_client_enforce puts a client's cheats back each frame) */
int touch_game_cheats_allowed(void)
{
	return touch_game_playing() && !network_game_distributed_client();
}

/* the switches that are on, as bits; scripts set some too (b30 makes the
player deathless around its cinematics), so the menu shows these */
int touch_game_cheats_state(void)
{
	boolean *value;
	int bits = 0;
	int index;

	for (index = 0; (value = touch_game_cheat_switch(index)) != NULL; index++)
	{
		if (*value)
			bits |= 1 << index;
	}
	return bits;
}

/* sets every switch from the menu's bits */
void touch_game_cheats_set(int bits)
{
	boolean *value;
	int index;

	for (index = 0; (value = touch_game_cheat_switch(index)) != NULL; index++)
	{
		*value = (bits & (1 << index)) ? TRUE : FALSE;
	}
}

/* the menu's one-time cheats, as bits (TouchControls.java's CHEAT_ACTIONS):
each places its objects around the first player with a unit, as the
console's cheat_all_weapons and the others do */
void touch_game_cheats_act(int actions)
{
	if (actions & (1 << 0))
		cheat_all_weapons();
	if (actions & (1 << 1))
		cheat_all_vehicles();
	if (actions & (1 << 2))
		cheat_all_powerups();
	if (actions & (1 << 3))
		cheat_active_camouflage();
}
