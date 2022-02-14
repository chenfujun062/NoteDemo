/*
 * Copyright 2009 Cedric Priscal
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <jni.h>

#include <pthread.h>
#include <linux/input.h>

#include <cutils/properties.h>
#include "paintworker.h"
#include "common.h"
#include <android/bitmap.h>

#include "fields.h"

#include "android/log.h"
#include <list>


using namespace std;


static const char *TAG = "NDK_WeNote";

#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)

#define NUM_STEPS 10 //越大，曲线越密，越逼近

typedef struct {
    float x, y;
} Point;


struct Fields fields;

static JavaVM *g_wenote_jvm = NULL;
static jobject g_wenote_obj = NULL;
//static JNIEnv *m_flash_env;
//static jclass m_class_wenoteview;
static jobject m_obj_class_loader = NULL;
static jmethodID m_method_find_class = NULL;
static jmethodID m_method_receive_writingdata;
static jmethodID m_method_receive_upaciotn;
static jmethodID m_method_forceupdatejava;
jclass class_notejni_global;
static int m_pen_mode = -1;

Point point[4];
static PaintWorker *paintWorker;
static CommitWorker *commitWorker;


int on_down(float ex, float ey);
int on_move(float ex, float ey);
int on_up();
int on_update();
void initPainter(int left, int top, int right, int bottom, bool isFilterFixedArea,
                    int filter_x1, int filter_y1, int filter_x2, int filter_y2);

void deinitField(JNIEnv* env);
int initFields(JNIEnv* env);

void setJavaUIMode(int isJava);

using namespace std;
class CvPoint
{
public:
    float x;
    float y;
    CvPoint() {
        x = 0.0;
        y = 0.0;
    }
    CvPoint(float a,float b) {
        x = a;
        y = b;
    }

};

extern "C" {
    JNIEXPORT jbyteArray JNICALL Java_com_intelgine_handwrite_HandWrite_getPixels
    (JNIEnv *env, jobject obj);
};

char* jstringTostring(JNIEnv* env, jstring jstr)
{
    char* rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr= (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char*)malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: native_init
 * Description: Init the PaintWorker thread.
 * Input: The jrect means the canvas's rect, it contains left, top, right, bottom.
 */
JNIEXPORT jint JNICALL native_init(JNIEnv * env, jobject thiz, jobject jrect,
                                        jboolean isFilterFixedArea, jobject filterRect)
{
    LOGD("Flash test : +++++++++++++ native_init()");
    //initFields(env);
    //m_class_wenoteview = env->GetObjectClass(classview);
    //m_method_receiveemrevent = env->GetStaticMethodID(m_class_wenoteview, "receiveEMREvent", "(III)V");
    //m_flash_env = env;

    jclass clazz;
    jfieldID jfid;
    clazz = env->GetObjectClass(jrect);
    if (0 == clazz) {
        LOGE("Flash test : +++++ native_init() GetObjectClass returned 0\n");
        return JNI_ERR;
    }

    int left = 0, top = 0, right = 0, bottom = 0;

    jfid = (env)->GetFieldID(clazz, "left", "I");
    left = (int)env->GetIntField(jrect, jfid);

    jfid = (env)->GetFieldID(clazz, "top", "I");
    top = (int)env->GetIntField(jrect, jfid);

    jfid = (env)->GetFieldID(clazz, "right", "I");
    right = (int)env->GetIntField(jrect, jfid);

    jfid = (env)->GetFieldID(clazz, "bottom", "I");
    bottom = (int)env->GetIntField(jrect, jfid);    

    int filter_x1 = 0, filter_y1 = 0, filter_x2 = 0, filter_y2 = 0;

    jfid = (env)->GetFieldID(clazz, "left", "I");
    filter_x1 = (int)env->GetIntField(filterRect, jfid);

    jfid = (env)->GetFieldID(clazz, "top", "I");
    filter_y1 = (int)env->GetIntField(filterRect, jfid);

    jfid = (env)->GetFieldID(clazz, "right", "I");
    filter_x2 = (int)env->GetIntField(filterRect, jfid);

    jfid = (env)->GetFieldID(clazz, "bottom", "I");
    filter_y2 = (int)env->GetIntField(filterRect, jfid);

    LOGD("Flash test : +++++++++++++ native_init() read Rect(%d, %d, %d, %d)\n", left, top, right, bottom);

    initPainter(left, top, right, bottom, isFilterFixedArea, filter_x1, filter_y1, filter_x2, filter_y2);

    //env->GetJavaVM(&g_wenote_jvm);
    //不能直接赋值(g_obj = obj)
    g_wenote_obj = env->NewGlobalRef(thiz);
    return JNI_OK;
}

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: native_clear
 * Description: Clear Screen with clearMode
 * Input: The clearMode means use which mode to clear, it is 0 or 1.
 */
JNIEXPORT jint JNICALL native_clear(JNIEnv * env, jobject thiz, jint clearMode)
{
    LOGD("Flash test : +++++++++++++ native_clear()");
    paintWorker->Clear(clearMode);
    return JNI_OK;
}

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: native_clear
 * Description: Exit the PaintWorker thread.
 */
JNIEXPORT jint JNICALL native_exit(JNIEnv * env, jobject thiz)
{
    LOGD("Flash test : +++++++++++++ native_exit()");
    if (paintWorker == NULL) {
        return JNI_OK;
    }

    //property_set("debug.property.enable","0");
    //property_set("debug.mode","0");
    setJavaUIMode(1);
    //usleep(1000*100);

    paintWorker->ExitPaintWorker();
    delete paintWorker;
    paintWorker = NULL;
    return JNI_OK;
}

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: native_undo
 * Description: Undo the last drawing.
 * Input: Use left, top, right, bottom to refresh the screen.
 */
JNIEXPORT jint JNICALL native_undo(JNIEnv * env, jobject thiz, jint left, jint top, jint right, jint bottom)
{
    LOGD("Flash test : +++++++++++++ native_undo()");
    int status = paintWorker->Undo(left, top, right, bottom);
    //LOGD("Flash test : +++++++++++++ status:%d", status);
    return status;
}

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: native_undo
 * Description: Undo the last drawing.
 * Input: Use left, top, right, bottom to refresh the screen.
 */
JNIEXPORT jint JNICALL native_redo(JNIEnv * env, jobject thiz, jint left, jint top, jint right, jint bottom)
{
    LOGD("Flash test : +++++++++++++ native_redo()");
    int status = paintWorker->Redo(left, top, right, bottom);
    //LOGD("Flash test : +++++++++++++ status:%d", status);
    return status;
}

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: native_redraw
 * Description: Playback the process of drawing.
 * Input: Use left, top, right, bottom to refresh the screen.
 */
JNIEXPORT jint JNICALL native_redraw(JNIEnv * env, jobject thiz, jint left, jint top, jint right, jint bottom)
{
    LOGD("Flash test : +++++++++++++ native_redraw()");
    //int status = paintWorker->ReDraw(left, top, right, bottom);
    return JNI_OK;
}

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: native_eraser
 * Description: Switch the pen's mode from pen to eraser, or from eraser to pen, it's up to the mode.
 * Input: The mode decides the pen's mode.
 */
JNIEXPORT jint JNICALL native_eraser(JNIEnv * env, jobject thiz, jboolean isEraserEnable)
{
    LOGD("Flash test : +++++++++++++ native_eraser()");
    paintWorker->SetPenOrEraserMode(isEraserEnable);
    return JNI_OK;
}

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: native_set_is_drawing
 * Description: Set the flag isDrawing, it will prohibit other drawing operations
 * while already existing drawing operations are in progress.
 * Input: The isDrawing is a lock to control drawing, it is 0 or 1.
 */
JNIEXPORT jint JNICALL native_is_handwriting_enable(JNIEnv * env, jobject thiz, jboolean isHangdWritingEnable)
{
    LOGD("Flash test : +++++++++++++ native_is_handwriting_enable()");
    paintWorker->IsHandwritingEnable(isHangdWritingEnable);
    return JNI_OK;
}

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: native_recovery
 * Description: Recovery the drawing information from Screen_Off to Screen_On.
 * Input: Use left, top, right, bottom to refresh the screen.
 */
JNIEXPORT jint JNICALL native_recovery(JNIEnv * env, jobject thiz, jint left, jint top, jint right, jint bottom)
{
    LOGD("Flash test : +++++++++++++ native_recovery()");
    paintWorker->ShowDrawPoint(left, top, right, bottom);
    return JNI_OK;
}

JNIEXPORT jint JNICALL native_is_overlay_enable(JNIEnv * env, jobject thiz, jboolean isOverlayEnable)
{
    LOGD("Flash test : +++++++++++++ native_is_overlay_enable()");
    commitWorker->IsOverlayEnable(isOverlayEnable);
    return JNI_OK;
}

JNIEXPORT jint JNICALL native_strokes(JNIEnv * env, jobject thiz, jboolean isStrokesEnable)
{
    LOGD("Flash test : +++++++++++++ native_strokes()");
    paintWorker->SetStrokesMode(isStrokesEnable);
    return JNI_OK;
}

JNIEXPORT jint JNICALL native_show_draw_point(JNIEnv * env, jobject thiz, jint left, jint top, jint right, jint bottom)
{
    LOGD("Flash test : +++++++++++++ native_show_draw_point()");
    paintWorker->ShowDrawPoint(left, top, right, bottom);
    return JNI_OK;
}

JNIEXPORT jint JNICALL native_dump(JNIEnv * env, jobject thiz)
{
    LOGD("Flash test : +++++++++++++ native_dump()");
    commitWorker->dump();
    return JNI_OK;
}

JNIEXPORT jint JNICALL native_test_draw(JNIEnv * env, jobject thiz, jboolean isTest)
{
    LOGD("Flash test : +++++++++++++ native_test_draw()");
    paintWorker->TestDraw(isTest);
    return JNI_OK;
}

JNIEXPORT jobject JNICALL native_get_notelist(JNIEnv * env, jclass clazz)
{
    //LOGD("Flash test : +++++++++++++ native_get_notelist()");
    jclass list_cls = env->FindClass("java/util/ArrayList");//获得ArrayList类引用
    if(list_cls == NULL) {
        LOGD("Flash test : +++++++++++++ list_cls == NULL");
    }

    jmethodID list_costruct = env->GetMethodID(list_cls, "<init>", "()V"); //获得得构造函数Id
    //LOGD("Flash test : +++++++++++++ get ArrayList costruct is done");
    jobject list_obj = env->NewObject(list_cls, list_costruct); //创建一个Arraylist集合对象
    //LOGD("Flash test : +++++++++++++ ArrayList is done");
    //或得Arraylist类中的 add()方法ID，其方法原型为： boolean add(Object object) ;
    jmethodID list_add  = env->GetMethodID(list_cls,"add","(Ljava/lang/Object;)Z");
    //LOGD("Flash test : +++++++++++++ get ArrayList add is done");
    jclass point_cls = env->FindClass("com/rockchip/notedemo/PointStruct");//获得PointStruct类引用
    if(point_cls == NULL) {
        LOGD("Flash test : +++++++++++++ point_cls == NULL");
    }
    //获得该类型的构造函数  函数名为 <init> 返回类型必须为 void 即 V
    jmethodID point_costruct = env->GetMethodID(point_cls , "<init>", "(IIIIII)V");
    //LOGD("Flash test : +++++++++++++ get PointStruct costruct is done");

    list<PointStruct>::iterator itPoint;
    list<PointStruct> list = paintWorker->GetPathList();
    for (itPoint = list.begin(); itPoint != list.end(); itPoint++) {
        //LOGD("----native_get_notelist--x:%d,y:%d,m_pressed_value:%d,penColor:%d,penWidth:%d,action:%d \n",
        //itPoint->x, itPoint->y, itPoint->pressedValue, itPoint->penColor, itPoint->penWidth, itPoint->action);
        //通过调用该对象的构造函数来new 一个 Student实例
        jobject point_obj = env->NewObject(point_cls, point_costruct, itPoint->x,
                                           itPoint->y, itPoint->pressedValue, itPoint->penColor, itPoint->penWidth, itPoint->action);  //构造一个对象
        env->CallBooleanMethod(list_obj , list_add , point_obj); //执行Arraylist类实例的add方法，添加一个stu对象
    }


    return list_obj ;
}

JNIEXPORT jint JNICALL native_set_pen_width(JNIEnv * env, jobject thiz, jint penWidth)
{
    LOGD("Flash test : +++++++++++++ native_set_pen_width()");
    paintWorker->SetPenWidth(penWidth);
    return JNI_OK;
}

JNIEXPORT jint JNICALL native_set_pen_color(JNIEnv * env, jobject thiz, jboolean isInitColor, jint penColor)
{
    LOGD("Flash test : +++++++++++++ native_set_pen_color()");
    paintWorker->SetPenColor(isInitColor, penColor);
    return JNI_OK;
}

JNIEXPORT jint JNICALL native_set_pen_color_any(JNIEnv * env, jobject thiz, jboolean isInitColor, jint penColor,jint A,jint R,jint G,int B)
{
    LOGD("Flash test : +++++++++++++ native_set_pen_color()");
    paintWorker->SetPenColorAny(isInitColor, penColor,A,R,G,B);
    return JNI_OK;
}


JNIEXPORT jint JNICALL native_draw_bitmap(JNIEnv * env, jobject thiz,
    jobject bitmap, jint deviceId, jint left, jint top, jint right, jint bottom)
{
    LOGD("Flash test : +++++++++++++ native_draw_bitmap(),left:%d,top:%d,right:%d,bottom:%d",
        left, top, right, bottom);
    AndroidBitmapInfo info;
    void* pixels;
    //旋转区域
    int x1 = top;
    int y1 = left;
    int x2 = bottom;
    int y2 = right;

    if(AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGD("AndroidBitmap_getInfo failed");
    }
    int width = (int) info.width;
    int height = (int) info.height;

    AndroidBitmap_lockPixels(env, bitmap, (void**) &pixels);

    if(AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGD("AndroidBitmap_getInfo failed");
    }

    paintWorker->DrawBitmap(pixels, deviceId, width, height, x1, y1, x2, y2);

    AndroidBitmap_unlockPixels(env, bitmap);
    return JNI_OK;
}

JNIEXPORT jbyteArray JNICALL Java_com_intelgine_handwrite_HandWrite_getPixels(JNIEnv * env, jobject thiz)
{
    LOGD("Flash test : +++++++++++++ getPixels");
    jbyte by[1200*825];
    int i=0;

    jbyteArray jarray = env->NewByteArray(1200*825);
    env->SetByteArrayRegion(jarray, 0, 1200*825, by);

    //env->ReleaseIntArrayElements(buf, mJavaBuf, 0);
    return jarray;
}

JNIEXPORT jint JNICALL native_sethandwriterects(JNIEnv * env, jobject thiz, jobject jrect)
{
    LOGD("+++++++++++++ native_sethandwriterects()");

    jclass clazz;
    jfieldID jfid;
    int left = 0, top = 0, right = 0, bottom = 0;

    clazz = env->GetObjectClass(jrect);
    if (0 == clazz) {
        LOGE("+++++ native_sethandwriterects() GetObjectClass returned 0\n");
        return JNI_ERR;
    }

    jfid = (env)->GetFieldID(clazz, "left", "I");
    left = (int)env->GetIntField(jrect, jfid);

    jfid = (env)->GetFieldID(clazz, "top", "I");
    top = (int)env->GetIntField(jrect, jfid);

    jfid = (env)->GetFieldID(clazz, "right", "I");
    right = (int)env->GetIntField(jrect, jfid);

    jfid = (env)->GetFieldID(clazz, "bottom", "I");
    bottom = (int)env->GetIntField(jrect, jfid);

    LOGD("+++++++++++++ native_sethandwriterects() read Rect(%d, %d, %d, %d)\n", left, top, right, bottom);
    paintWorker->sethandwriterects(left, top, right, bottom);

    return JNI_OK;
}
JNIEXPORT jint JNICALL native_setnohandwriterects(JNIEnv * env, jobject thiz, jobject filterRect)
{
    LOGD("+++++++++++++ native_setnohandwriterects()");
    jclass clazz;
    jfieldID jfid;
    int filter_x1 = 0, filter_y1 = 0, filter_x2 = 0, filter_y2 = 0;

    clazz = env->GetObjectClass(filterRect);
    if (0 == clazz) {
        LOGE("+++++ native_setnohandwriterects() GetObjectClass returned 0\n");
        return JNI_ERR;
    }

    jfid = (env)->GetFieldID(clazz, "left", "I");
    filter_x1 = (int)env->GetIntField(filterRect, jfid);

    jfid = (env)->GetFieldID(clazz, "top", "I");
    filter_y1 = (int)env->GetIntField(filterRect, jfid);

    jfid = (env)->GetFieldID(clazz, "right", "I");
    filter_x2 = (int)env->GetIntField(filterRect, jfid);

    jfid = (env)->GetFieldID(clazz, "bottom", "I");
    filter_y2 = (int)env->GetIntField(filterRect, jfid);

    LOGD("+++++++++++++ native_setnohandwriterects() read Rect(%d, %d, %d, %d)\n", filter_x1, filter_y1, filter_x2, filter_y2);
    paintWorker->setnohandwriterects(filter_x1, filter_y1, filter_x2, filter_y2);

    return JNI_OK;
}
static JNINativeMethod gMethods[] = {
    { "native_init",    "(Landroid/graphics/Rect;ZLandroid/graphics/Rect;)I",   (void*) native_init },
    { "native_exit",    "()I",  (void*) native_exit },
    { "native_clear",       "(I)I",  (void*) native_clear },
    { "native_undo",      "(IIII)I",  (void*) native_undo },
    { "native_redo",      "(IIII)I",  (void*) native_redo },
    { "native_redraw",      "(IIII)I",  (void*) native_redraw },
    { "native_eraser",      "(Z)I",  (void*) native_eraser },
    { "native_is_handwriting_enable",      "(Z)I",  (void*) native_is_handwriting_enable },
    { "native_recovery",      "(IIII)I",  (void*) native_recovery },
    { "native_is_overlay_enable",      "(Z)I",  (void*) native_is_overlay_enable },
    { "native_strokes",      "(Z)I",  (void*) native_strokes },
    { "native_show_draw_point",      "(IIII)I",  (void*) native_show_draw_point },
    { "native_dump",      "()I",  (void*) native_dump },
    { "native_test_draw",      "(Z)I",  (void*) native_test_draw },
    { "native_get_notelist",      "()Ljava/util/ArrayList;",  (void*) native_get_notelist },
    { "native_set_pen_width",      "(I)I",  (void*) native_set_pen_width },
    { "native_set_pen_color",      "(ZI)I",  (void*) native_set_pen_color },
    { "native_draw_bitmap",      "(Landroid/graphics/Bitmap;IIIII)I",  (void*) native_draw_bitmap },
    { "native_sethandwriterects",    "(Landroid/graphics/Rect;)I",   (void*) native_sethandwriterects },
    { "native_setnohandwriterects",    "(Landroid/graphics/Rect;)I",   (void*) native_setnohandwriterects },
    { "native_set_pen_color_any",    "(ZIIII)I",   (void*) native_set_pen_color_any },
    
};

/*
 * 为某一个类注册本地方法
 */
static int registerNativeMethods(JNIEnv* env, const char* className,
                                 JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        LOGD("---------clazz is null");
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGD("---------RegisterNatives < 0");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

/*
 * 为所有类注册本地方法
 */
static int registerNatives(JNIEnv* env)
{
    const char* kClassName = "com/rockchip/notedemo/NoteJNI"; //指定要注册的类com.boncare.chair.serialport
    return registerNativeMethods(env, kClassName, gMethods,
                                 sizeof(gMethods) / sizeof(gMethods[0]));
}

void forceUpdateJava()
{
    JNIEnv *env = NULL;
    int isAttacked = 0;
    jclass clazz = NULL;
    jstring class_name = NULL;
    jobject cls_obj = NULL;
    int result;
    int status = g_wenote_jvm->GetEnv((void**) &env, JNI_VERSION_1_4);
    if (status < 0) {
        isAttacked = 1;
        JavaVMAttachArgs args = {JNI_VERSION_1_4, NULL, NULL};
        result = g_wenote_jvm->AttachCurrentThread(&env, (void*) &args);
        if (result != JNI_OK) {
            LOGE("thread attach failed: %#x", result);
            goto error;
        }
    }

    class_name = env->NewStringUTF("com/rockchip/notedemo/NoteJNI");
    cls_obj = env->CallObjectMethod(m_obj_class_loader, m_method_find_class, class_name);
    clazz = env->GetObjectClass(cls_obj);

    if (clazz != NULL) {
        env->CallStaticVoidMethod( clazz, m_method_forceupdatejava);
    }
    env->DeleteLocalRef(clazz);
    env->DeleteLocalRef(class_name);
    env->DeleteLocalRef(cls_obj);

error:
    //Detach主线程
    if(g_wenote_jvm->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
    }
}

void sendWritingDataToJava(int lastX, int lastY, int x, int y, int pressedValue, int penColor,
    int penWidth, int action, bool isEraserEnable, bool isStrokesEnable)   //receiveEMREvent
{
    //LOGD("Flash test : +++++++ sendEMREventToJava action = %d", action);

    JNIEnv *env = NULL;
    int isAttacked = 0;
    jclass clazz = NULL;
    jstring class_name = NULL;
    jobject cls_obj = NULL;
    int result;

    int status = g_wenote_jvm->GetEnv((void**) &env, JNI_VERSION_1_6);
    if (status < 0) {
        isAttacked = 1;
        JavaVMAttachArgs args = {JNI_VERSION_1_6, NULL, NULL};

        result = g_wenote_jvm->AttachCurrentThread(&env, (void*) &args);
        if (result != JNI_OK) {
            LOGE("thread attach failed: %#x", result);
            goto error;
        }
    }


    class_name = env->NewStringUTF("com/rockchip/notedemo/NoteJNI");
    //LOGD("Flash test : ++++++ class_name = %s", jstringTostring(env, class_name));
    cls_obj = env->CallObjectMethod(m_obj_class_loader, m_method_find_class, class_name);

    clazz = env->GetObjectClass(cls_obj);


    if (clazz != NULL) {
        //LOGD("Flash test : ++++++ clazz != NULL");
        //m_method_receiveemrevent = env->GetStaticMethodID(clazz, "receiveEMREvent","(I)V");
        env->CallStaticVoidMethod(class_notejni_global, m_method_receive_writingdata, lastX, lastY, x, y,
            pressedValue, penColor, penWidth, action, isEraserEnable, isStrokesEnable);
    }
    env->DeleteLocalRef(clazz);
    env->DeleteLocalRef(class_name);
    env->DeleteLocalRef(cls_obj);

error:
    //Detach主线程
    if(g_wenote_jvm->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
    }
}

void sendUpActionToJava()
{

    JNIEnv *env = NULL;
    int isAttacked = 0;
    jclass clazz = NULL;
    jstring class_name = NULL;
    jobject cls_obj = NULL;
    int result;

    int status = g_wenote_jvm->GetEnv((void**) &env, JNI_VERSION_1_6);
    if (status < 0) {
        isAttacked = 1;
        JavaVMAttachArgs args = {JNI_VERSION_1_6, NULL, NULL};

        result = g_wenote_jvm->AttachCurrentThread(&env, (void*) &args);
        if (result != JNI_OK) {
            LOGE("thread attach failed: %#x", result);
            goto error;
        }
    }


    class_name = env->NewStringUTF("com/rockchip/notedemo/NoteJNI");
    //LOGD("Flash test : ++++++ class_name = %s", jstringTostring(env, class_name));
    cls_obj = env->CallObjectMethod(m_obj_class_loader, m_method_find_class, class_name);

    clazz = env->GetObjectClass(cls_obj);


    if (clazz != NULL) {
        //LOGD("Flash test : ++++++ clazz != NULL");
        env->CallStaticVoidMethod(clazz, m_method_receive_upaciotn);
    }
    env->DeleteLocalRef(clazz);
    env->DeleteLocalRef(class_name);
    env->DeleteLocalRef(cls_obj);

error:
    //Detach主线程
    if(g_wenote_jvm->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
    }
}


void setJavaUIMode(int isJava)
{
    LOGD("Flash test : +++++++ setJavaUIMode new mode isJava = %d, old mode m_pen_mode = %d", isJava, m_pen_mode);
    if (m_pen_mode == isJava) return;

    if (isJava == 1) {
        //usleep(1000*1500);
        property_set("debug.property.enable","0");
        property_set("debug.mode","0");
        //usleep(1000*200);
        //forceUpdateJava();
    } else {
        property_set("debug.property.enable","1");
        property_set("debug.mode","8");
        //usleep(1000*100);
    }
    m_pen_mode = isJava;
}

void deinitField(JNIEnv* env)
{
    if(fields.IllegalStateException.clazz)
        env->DeleteGlobalRef(fields.IllegalStateException.clazz);
    if(fields.String.clazz)
        env->DeleteGlobalRef(fields.String.clazz);
    if(fields.WeNoteView.clazz)
        env->DeleteGlobalRef(fields.WeNoteView.clazz);
    if(fields.app.clazz)
        env->DeleteGlobalRef(fields.app.clazz);
    if(fields.app.inst)
        env->DeleteGlobalRef(fields.app.inst);
}



void initPainter(int left, int top, int right, int bottom, bool isFilterFixedArea,
                        int filter_x1, int filter_y1, int filter_x2, int filter_y2)
{
    //LOGD("Flash test : +++++++++ initPainter() height = %d\n", height);
    //初始化 PaintWorker 线程类
    paintWorker = new PaintWorker();
    paintWorker->Init(left, top, right, bottom, isFilterFixedArea, filter_x1, filter_y1, filter_x2, filter_y2);
    setJavaUIMode(0);
}

/*
 * System.loadLibrary("lib")时调用
 * 如果成功返回JNI版本, 失败返回-1
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOGD("Flash test : ***** JNI_OnLoad.");
    JNIEnv* env = NULL;
    jint result = -1;
    g_wenote_jvm = vm;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    assert(env != NULL);

    jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
    jclass adapterClass = env->FindClass("com/rockchip/notedemo/NoteJNI");

    if(adapterClass) {
        jmethodID getClassLoader = env->GetStaticMethodID(adapterClass,"getClassLoader","()Ljava/lang/ClassLoader;");
        m_method_receive_writingdata = env->GetStaticMethodID(adapterClass, "receiveWritingDataEvent","(IIIIIIIIZZ)V");

        jobject obj = env->CallStaticObjectMethod(adapterClass,getClassLoader);

        m_obj_class_loader = env->NewGlobalRef( obj);
        m_method_find_class = env->GetMethodID(classLoaderClass, "loadClass","(Ljava/lang/String;)Ljava/lang/Class;");
        class_notejni_global = static_cast<jclass>(env->NewGlobalRef(adapterClass));
        env->DeleteLocalRef(classLoaderClass);
        env->DeleteLocalRef(adapterClass);
        env->DeleteLocalRef(obj);
    }

    if (!registerNatives(env)) { //注册
        return JNI_ERR;
    }

    //initFields(env);
    //成功
    result = JNI_VERSION_1_6;

    return result;
}

void JNI_OnUnload(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK)
        return;

    LOGD("Flash test : ***** JNI_OnUnload.");

    //deinitField(env);
}

