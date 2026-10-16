/*
JNI.H

A stand-in for the JNI header, enough for host_touch_test.c to compile
port/android/host/host_touch.c on the build machine: the types it names
and the two JNIEnv functions it calls (GetArrayLength, SetIntArrayRegion).
*/

#ifndef HOST_TEST_JNI_H
#define HOST_TEST_JNI_H

#include <stdint.h>

#define JNIEXPORT
#define JNICALL

typedef int32_t jint;
typedef int32_t jsize;
typedef float jfloat;
typedef void *jclass;
typedef void *jintArray;

struct JNINativeInterface_
{
	jsize (*GetArrayLength)(const struct JNINativeInterface_ **env, jintArray array);
	void (*SetIntArrayRegion)(const struct JNINativeInterface_ **env, jintArray array, jsize start,
		jsize length, const jint *values);
};
typedef const struct JNINativeInterface_ *JNIEnv;

#endif
