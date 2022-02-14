LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
uses-permission android:name="android.permission.ACCESS_SURFACE_FLINGER"
LOCAL_SRC_FILES:= \
	wenote_jni.cpp \
	getevent.cpp \
	commitworker.cpp \
	paintworker.cpp \
	main.cpp \
	worker.cpp

#rgb888_to_gray_256_neon.s \

LOCAL_SHARED_LIBRARIES := \
	libjpeg \
	libpng \
	libcutils \
	libandroidfw \
	libutils \
	libbinder \
	libui \
	libskia \
	libgui \
	liblog

LOCAL_C_INCLUDES := \
	system/core/include/utils \
	frameworks/base/core/jni/include \
	external/skia/src/core/ \
	external/libjpeg-turbo \
	external/libpng

LOCAL_CFLAGS := -O2 -g -W -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-constant-conversion -Wno-sign-compare
LOCAL_LDLIBS := -ljnigraphics

LOCAL_MODULE:= libpaintworker

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
