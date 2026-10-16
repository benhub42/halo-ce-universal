"""The real time freeze of the cheats menu (port/linux/game/time_freeze.c): only the players' root objects update, it
goes off when the game clock goes back but not when turned on after it, it is refused in a network game, and it holds
nothing still during a cinematic."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run  # noqa: E402

CASES = ["player-only", "vehicle", "players", "clock-back", "network", "cinematic"]
FUNCTIONS = ("time_freeze_set", "time_freeze_wanted", "time_freeze_active", "time_freeze_find_roots",
             "time_freeze_object_updates")

# a fault, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "unit-not-root": (("\t\tfor (depth = 0;", "\t\tfor (depth = 8;"), "time_freeze_find_roots", "vehicle"),
    "roots-kept": (("if (time_freeze_roots_tick != game_time_get())", "if (time_freeze_roots_tick == NONE)"),
                   "time_freeze_object_updates", "vehicle"),
    "clock-not-taken-at-set": (("\ttime_freeze_clock = game_time_get();\n", ""), "time_freeze_set", "clock-back"),
    "never-off-at-new-map": (("clock < time_freeze_clock || ", ""), "time_freeze_active", "clock-back"),
    "network-allowed": (("on && !network_game_is_active()", "on"), "time_freeze_set", "network"),
    "frozen-cinematic": (("return !(cinematic_globals && cinematic_in_progress());", "return TRUE;"),
                         "time_freeze_active", "cinematic"),
    "dead-player-root": (("\t\tif (object_index == NONE)\n\t\t\tcontinue;\n", ""), "time_freeze_find_roots",
                         "players"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/game/time_freeze.c")
    text = f"#define TIME_FREEZE_MAXIMUM_ROOTS {constant(source, 'TIME_FREEZE_MAXIMUM_ROOTS')}\n"
    text += "static boolean time_freeze_on;\nstatic long time_freeze_clock;\n"
    text += "static long time_freeze_roots[TIME_FREEZE_MAXIMUM_ROOTS];\nstatic short time_freeze_root_count;\n"
    text += "static long time_freeze_roots_tick = NONE;\n"
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("under_test.inc", text),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("time_freeze", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("time_freeze", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"the time freeze with a fault ({control}) passed '{case}': the test cannot see it"
