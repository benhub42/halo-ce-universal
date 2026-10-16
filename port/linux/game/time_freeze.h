/*
TIME_FREEZE.H

The cheats menu's time freeze: the world stops but the players
(time_freeze.c).
*/

#ifndef TIME_FREEZE_H
#define TIME_FREEZE_H

/* turns the time freeze on or off (the cheats menu, touch_game.c); refused
in a network game, where the other machines would go on without it */
void time_freeze_set(boolean on);
/* whether it is on, as the cheats menu shows it */
boolean time_freeze_wanted(void);

/* in a tick: whether the world is frozen (on, a local game, no cinematic);
off again once the game clock goes back, at a new map or a reverted game */
boolean time_freeze_active(void);
/* in a tick, while frozen: whether an object updates (a root object that is
a player's unit or carries one, its vehicle) */
boolean time_freeze_object_updates(long object_index);

#endif
