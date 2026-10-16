/*
HOST_TOUCH_TEST.C

Tests of the cheats menu's way to the game through the Android host
(port/android/host/host_touch.c): the overlay's switches and one-time cheats
reach the guest's read, each one-time cheat once, and the game's report
reaches the overlay. Run by tools/test_host_touch.py with the build
machine's C compiler and a stand-in JNI header (tests/jni_stub).
*/

#include <jni.h>
#include <stdio.h>

void Java_com_halo_decomp_TouchControls_nativeCheats(JNIEnv *env, jclass cls, jint switches);
void Java_com_halo_decomp_TouchControls_nativeCheatAction(JNIEnv *env, jclass cls, jint actions);
jint Java_com_halo_decomp_TouchControls_nativeCheatsState(JNIEnv *env, jclass cls);
void host_touch_cheats_read(int32_t *request);
void host_touch_cheats_report(int32_t state);

static int failures;

static void expect(int condition, const char *what)
{
	if (!condition)
	{
		printf("FAILED: %s\n", what);
		failures++;
	}
}

int main(void)
{
	int32_t request[3];
	int32_t serial;

	host_touch_cheats_read(request);
	expect(request[0] == 0 && request[1] == 0 && request[2] == 0, "nothing asked before the menu acts");
	expect(Java_com_halo_decomp_TouchControls_nativeCheatsState(NULL, NULL) == 0,
		"the overlay reads 0 before the game reports");

	Java_com_halo_decomp_TouchControls_nativeCheats(NULL, NULL, 1 << 0 | 1 << 3);
	host_touch_cheats_read(request);
	expect(request[1] == (1 << 0 | 1 << 3), "the switches reach the game");
	expect(request[0] != 0, "a change of the switches is counted");
	serial = request[0];

	host_touch_cheats_read(request);
	expect(request[0] == serial, "no change counted without the menu");
	expect(request[1] == (1 << 0 | 1 << 3), "the switches stay as the menu left them");

	Java_com_halo_decomp_TouchControls_nativeCheats(NULL, NULL, 0);
	host_touch_cheats_read(request);
	expect(request[0] != serial && request[1] == 0, "switching all off is a change too");

	Java_com_halo_decomp_TouchControls_nativeCheatAction(NULL, NULL, 1 << 0);
	Java_com_halo_decomp_TouchControls_nativeCheatAction(NULL, NULL, 1 << 2);
	host_touch_cheats_read(request);
	expect(request[2] == (1 << 0 | 1 << 2), "one-time cheats asked between reads all reach the game");
	host_touch_cheats_read(request);
	expect(request[2] == 0, "each one-time cheat reaches the game once");

	host_touch_cheats_report(1 << 17 | 1 << 16 | 1 << 1);
	expect(Java_com_halo_decomp_TouchControls_nativeCheatsState(NULL, NULL) == (1 << 17 | 1 << 16 | 1 << 1),
		"the game's report reaches the overlay");

	if (failures)
		return 1;
	printf("host_touch_test: all passed\n");
	return 0;
}
