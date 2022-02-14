#ifndef WENOTE_JNI_H
#define WENOTE_JNI_H

#include <jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    extern JavaVM *g_wenote_jvm;
    extern jobject g_wenote_obj;

#ifdef __cplusplus
}
#endif

void sendWritingDataToJava(int lastX, int lastY, int x, int y, int pressedValue, int penColor,
    int penWidth, int action, bool isEraserEnable, bool isStrokesEnable);
void sendUpActionToJava();
void forceUpdateJava();

extern void setJavaUIMode(int isJava);

#endif /* WENOTE_JNI_H */
