"""The real cheats menu of the Android touch controls (port/linux/game/touch_game.c and port/linux/src/touch_input.c's
touch_cheats_update): its switches reach the game's cheat globals, its one-time cheats run once, a script's own cheats
stay, and nothing acts as a client in another's game or outside a game."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["switches-apply", "every-switch", "scripts-kept", "actions-once", "client-refused", "not-playing"]
GAME_FUNCTIONS = ("touch_game_cheat_switch", "touch_game_cheats_allowed", "touch_game_cheats_state",
                  "touch_game_cheats_set", "touch_game_cheats_act")
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
    "action-twice": (("\tif (actions & (1 << 1))\n\t\tcheat_all_vehicles();",
                      "\tif (actions & (1 << 1))\n\t\tcheat_all_vehicles();\n\tif (actions)\n\t\tcheat_all_weapons();"),
                     "touch_game_cheats_act", "actions-once"),
}


def generated(fault=None, faulty_function=None):
    game = read("port/linux/game/touch_game.c")
    platform = read("port/linux/src/touch_input.c")
    config = structure(read("source/game/cheats.h"), "cheat_globals") + "\n"
    config += enum_with(platform, "_touch_cheats_allowed") + "\n"
    text = ""
    for source, names in ((game, GAME_FUNCTIONS), (platform, INPUT_FUNCTIONS)):
        for name in names:
            code = function(source, name)
            if fault and name == faulty_function:
                code = mutated(code, *fault)
            text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("touch_cheats", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("touch_cheats", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the cheats menu with a fault ({control}) passed '{case}': the test cannot see it"
