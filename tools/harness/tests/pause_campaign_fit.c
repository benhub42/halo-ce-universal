/*
PAUSE_CAMPAIGN_FIT.C (test)

The real fit of the campaign pause menu's rows after the port adds
SETTINGS (port/linux/game/menu_tags.c's pause_campaign_list_fit and
pause_list_fit): the rows stay within the span the map's own rows had,
so the last (SAVE AND QUIT) is not drawn over the box's button key, and a
list whose rows already fit is left as it is. test_pause_campaign_fit.py
takes the code (under_test.inc).
*/

#include "harness.h"

struct ui_widget_child_reference { short vertical_offset; };
struct tag_block { long count; void *address; };
struct rectangle2d { short y0, x0, y1, x1; };
struct ui_widget_definition { struct rectangle2d bounds; struct tag_block child_widgets; };

#include "under_test.inc"

static struct ui_widget_child_reference rows[8];
static struct ui_widget_definition list;

/* a list of count rows from first, spacing apart, bounds 0..bottom */
static void make(long count, short first, short spacing, short bottom)
{
	long row;

	for (row = 0; row < count; row++)
		rows[row].vertical_offset = (short)(first + row * spacing);
	list.child_widgets.count = count;
	list.child_widgets.address = rows;
	list.bounds.y0 = 0;
	list.bounds.y1 = bottom;
}

/* the rows in order, none above first nor below last */
static void check_within(short first, short last)
{
	long row;

	CHECK(rows[0].vertical_offset == first, "the first row at %d, not %d", rows[0].vertical_offset, first);
	for (row = 1; row < list.child_widgets.count; row++)
	{
		CHECK(rows[row].vertical_offset > rows[row - 1].vertical_offset, "row %ld not below row %ld", row, row - 1);
		CHECK(rows[row].vertical_offset <= last, "row %ld at %d, below the last row's %d", row,
			rows[row].vertical_offset, last);
	}
	CHECK(rows[list.child_widgets.count - 1].vertical_offset == last, "the last row at %d, not %d",
		rows[list.child_widgets.count - 1].vertical_offset, last);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* the Xbox's: four rows (0..90), bounds with room for a fifth, which
	pause_list_patch spaced at the full 30 from its centring (6) */
	CASE("room-past-last")
	{
		make(5, 6, 30, 140);
		pause_campaign_list_fit(&list, 0, 90, 140, FALSE);
		check_within(0, 90);
		CHECK(rows[1].vertical_offset - rows[0].vertical_offset >= 20, "rows %d apart: too close",
			rows[1].vertical_offset - rows[0].vertical_offset);
		CHECK(list.bounds.y1 == 140, "the bounds changed to %d", list.bounds.y1);
		return 0;
	}
	/* a list with room whose centred rows stay above its last: as it was */
	CASE("room-within")
	{
		make(5, 20, 30, 220);
		pause_campaign_list_fit(&list, 0, 200, 220, FALSE);
		CHECK(rows[0].vertical_offset == 20 && rows[4].vertical_offset == 140, "rows moved: %d..%d",
			rows[0].vertical_offset, rows[4].vertical_offset);
		return 0;
	}
	/* a list that grew for SETTINGS: its rows closer, its bounds back */
	CASE("grew")
	{
		make(5, 0, 30, 170);
		pause_campaign_list_fit(&list, 0, 90, 140, TRUE);
		check_within(0, 90);
		CHECK(list.bounds.y1 == 140, "the bounds %d, not back to 140", list.bounds.y1);
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
