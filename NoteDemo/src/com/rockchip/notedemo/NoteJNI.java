package com.rockchip.notedemo;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.Vector;

public class NoteJNI {
    private static final String TAG = "NoteJNI";

    private static Context mContext;
    private static NoteView mNoteView;
    private static Long mStartTime;
    private static Long mEndTime;
    private static Handler mPointHandler = null;
    public static final int MSG_FLAG_DRAW = 0;
    private static int UI_Height;
    private static int mScreenW;
    private static int mScreenH;
    // JNI
    public native int native_init(Rect rect, boolean isFilterFixedArea, Rect filterRect);
    public native int native_exit();
    public native int native_clear(int clearMode);
    public native int native_undo(int left, int top, int right, int bottom);
    public native int native_redo(int left, int top, int right, int bottom);
    public native int native_redraw(int left, int top, int right, int bottom);
    public native int native_eraser(boolean isEraserEnable);
    public native int native_is_handwriting_enable(boolean isHangdWritingEnable);
    public native int native_recovery(int left, int top, int right, int bottom);
    public native int native_is_overlay_enable(boolean isOverlayEnable);
    public native int native_strokes(boolean isStrokesEnable);
    public native int native_dump();
    public native int native_test_draw(boolean isTest);
    public native int native_show_draw_point(int left, int top, int right, int bottom);
    public static native ArrayList<PointStruct> native_get_notelist();
    public native int native_set_pen_width(int penWidth);
    public native int native_set_pen_color(boolean isInitColor, int penColor);
    public native int native_draw_bitmap(Bitmap bitmap, int deviceId, int left, int top, int right, int bottom);
    public native int native_sethandwriterects(Rect filterRect);
    public native int native_setnohandwriterects(Rect rect);

    public NoteJNI(Context context) {
        Log.d(TAG, "NoteJNI");
        mContext = context;
        getClassLoader();
        System.loadLibrary("paintworker");
    }

    public NoteJNI() {
        Log.d(TAG, "NoteJNI");
        getClassLoader();
        System.loadLibrary("paintworker");
    }

    public int init(Rect rect, boolean isFilterFixedArea, Rect filterRect) {
        Log.d(TAG, "Flash test : +++++++++ init() rect = " + rect + ",filterRect = "  + filterRect);
        UI_Height = rect.top;
        mScreenW = rect.right;
        mScreenH = rect.bottom;
        int initStatus = native_init(rect, isFilterFixedArea, filterRect);
        return initStatus;
    }

    public static ClassLoader getClassLoader() {
        Log.d(TAG,"zj add getClassLoader");
        return NoteJNI.class.getClassLoader();
    }

    public static void receiveWritingDataEvent(int lastX, int lastY, int x, int y, int pressedValue, int penColor, int penWidth,
                                               int action, boolean isEraserEnable, boolean isStrokesEnable) {
        //Log.d(TAG, "action: " + action);
        //Convert handwriting data
        /*int Android_X = mScreenW - y;
        int Android_Y = x - UI_Height;
        int Android_LastX = mScreenW - lastY;
        int Android_LastY = lastX - UI_Height;
         */
        int Android_X = y;
        int Android_Y = mScreenH - x;
        int Android_LastX = lastY;
        int Android_LastY = mScreenH - lastX;
        PointStruct pointStruct = new PointStruct(Android_LastX, Android_LastY, Android_X, Android_Y, pressedValue, penColor,
                                                    penWidth, action, isEraserEnable, isStrokesEnable);
        Message msg = mPointHandler.obtainMessage();
        msg.what = MSG_FLAG_DRAW;
        msg.obj = (Object) pointStruct;
        msg.sendToTarget();
    }

    public void setPointHandler(Handler handler) {
        mPointHandler = handler;
    }
}
