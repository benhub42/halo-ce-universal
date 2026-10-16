"""The real cheats menu of the Android touch controls (port/linux/game/touch_game.c and port/linux/src/touch_input.c's
touch_cheats_update): its switches reach the game's cheat globals, its one-time cheats run once, a script's own cheats
stay, and nothing acts as a client in another's game or outside a game."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["switches-apply", "every-switch", "scripts-kept", "actions-once", "camouflage-switch", "time-freeze-switch",
         "super-jump-strength", "camouflage-powerup", "camouflage-no-unit", "client-refused", "not-playing"]
GAME_FUNCTIONS = ("touch_game_super_jump_scale", "touch_game_super_jump_set", "touch_game_cheat_switch",
                  "touch_game_player_unit", "touch_game_camouflaged", "touch_game_camouflage_set",
                  "touch_game_cheats_allowed", "touch_game_cheats_state", "touch_game_cheats_set",
                  "touch_game_cheats_act")
INPUT_FUNCTIONS = ("touch_cheats_update",)

# a fault, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "client-cheats": ((" && !network_game_distributed_client()", ""), "touch_game_cheats_allowed",
                      "client-refused"),
    "applied-every-read": (("if (request[0] != serial_read)", "if (request[0] != serial_read || 1)"),
                           "touch_cheats_update", "scripts-kept"),
    "asks-kept-for-later": (("\tserial_read = request[0];", "\tif (allowed)\n\t\tserial_read = request[0];"),
                            "touch_cheats_update", "client-refused"),
    "switches-swapped": (("case 5: return &cheat.medusa;", "case 5: return &cheat.omnipotent;"),
                         "touch_game_cheat_switch", "every-switch"),
    "camouflage-reapplied": (("\tif (camouflage != touch_game_camouflaged())\n"
                               "\t\ttouch_game_camouflage_set(camouflage);",
                               "\ttouch_game_camouflage_set(camouflage);"), "touch_game_cheats_set",
                             "camouflage-switch"),
    "camouflage-off-keeps-super": (("~(FLAG(_unit_active_camouflaged_bit) | FLAG(_unit_super_camouflaged_bit))",
                                    "~FLAG(_unit_active_camouflaged_bit)"), "touch_game_camouflage_set",
                                   "camouflage-switch"),
    "dead-unit-camouflaged": (("\tif (player->unit_index == NONE)\n\t\treturn NULL;\n", ""),
                              "touch_game_player_unit", "camouflage-no-unit"),
    "players-read-at-start": (("\tif (!touch_game_playing())\n\t\treturn NULL;\n", ""), "touch_game_player_unit",
                              "not-playing"),
    "freeze-set-every-change": (("\tif (time_freeze != time_freeze_wanted())\n\t\ttime_freeze_set(time_freeze);",
                                 "\ttime_freeze_set(time_freeze);"), "touch_game_cheats_set", "time-freeze-switch"),
    "jump-unclamped": (("\tif (percent > TOUCH_GAME_SUPER_JUMP_MAXIMUM_PERCENT)\n"
                        "\t\tpercent = TOUCH_GAME_SUPER_JUMP_MAXIMUM_PERCENT;\n", ""), "touch_game_super_jump_set",
                       "super-jump-strength"),
    "jump-unset-zeroes": (("\tif (percent <= 0)\n\t\treturn;\n", ""), "touch_game_super_jump_set",
                          "super-jump-strength"),
    "action-twice": (("\tif (actions & (1 << 1))\n\t\tcheat_all_vehicles();",
                      "\tif (actions & (1 << 1))\n\t\tcheat_all_vehicles();\n\tif (actions)\n\t\tcheat_all_weapons();"),
                     "touch_game_cheats_act", "actions-once"),
}


def generated(fault=None, faulty_function=None):
    game = read("port/linux/game/touch_game.c")
    platform = read("port/linux/src/touch_input.c")
    config = structure(read("source/game/cheats.h"), "cheat_globals") + "\n"
    config += enum_with(platform, "_touch_cheats_allowed") + "\n"
    config += enum_with(read("source/units/units.h"), "_unit_active_camouflaged_bit") + "\n"
    config += f"#define MAXIMUM_LOCAL_PLAYERS {constant(read('source/game/players.h'), 'MAXIMUM_LOCAL_PLAYERS')}\n"
    config += f"#define TOUCH_GAME_CHEAT_CAMOUFLAGE {constant(game, 'TOUCH_GAME_CHEAT_CAMOUFLAGE')}\n"
    config += f"#define TOUCH_GAME_CHEAT_TIME_FREEZE {constant(game, 'TOUCH_GAME_CHEAT_TIME_FREEZE')}\n"
    for name in ("TOUCH_GAME_SUPER_JUMP_MINIMUM_PERCENT", "TOUCH_GAME_SUPER_JUMP_MAXIMUM_PERCENT"):
        config += f"#define {name} {constant(game, name)}\n"
    config += "static float touch_game_super_jump = 4.0f;\n"
    cheats = function(read("source/game/cheats.c"), "cheat_active_camouflage_local_player") + "\n"
    text = ""
    for source, names in ((game, GAME_FUNCTIONS), (platform, INPUT_FUNCTIONS)):
        for name in names:
            code = function(source, name)
            if fault and name == faulty_function:
                code = mutated(code, *fault)
            text += code + "\n"
    return (("config.inc", config), ("cheats.inc", cheats), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("touch_cheats", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("touch_cheats", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the cheats menu with a fault ({control}) passed '{case}': the test cannot see it"
