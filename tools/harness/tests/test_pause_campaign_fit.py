"""The real fit of the campaign pause menu's rows after SETTINGS is added (port/linux/game/menu_tags.c): the rows stay
within the span of the map's own, so SAVE AND QUIT is not drawn over the box's "B = CANCEL  A = SELECT"."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["room-past-last", "room-within", "grew"]
FUNCTIONS = ("pause_list_fit", "pause_campaign_list_fit")

# a fault, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "fit-only-when-grown": (("(!grew && children[count - 1].vertical_offset <= last)", "(!grew)"),
                            "pause_campaign_list_fit", "room-past-last"),
    "always-fit": (("(!grew && children[count - 1].vertical_offset <= last)", "(0)"), "pause_campaign_list_fit",
                   "room-within"),
    "from-the-centred-place": (("\tchildren[0].vertical_offset = first;\n", ""), "pause_campaign_list_fit",
                               "room-past-last"),
    "bounds-kept-grown": (("\tlist->bounds.y1 = bottom;\n", ""), "pause_campaign_list_fit", "grew"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/game/menu_tags.c")
    text = ""
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("under_test.inc", text),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("pause_campaign_fit", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("pause_campaign_fit", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, \
        f"the pause menu's fit with a fault ({control}) passed '{case}': the test cannot see it"
