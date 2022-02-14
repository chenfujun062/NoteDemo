

/*
 * Copyright (C) 2018 Fuzhou Rockchip Electronics Co.Ltd.
 *
 * Modification based on code covered by the Apache License, Version 2.0 (the "License").
 * You may not use this software except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS TO YOU ON AN "AS IS" BASIS
 * AND ANY AND ALL WARRANTIES AND REPRESENTATIONS WITH RESPECT TO SUCH SOFTWARE, WHETHER EXPRESS,
 * IMPLIED, STATUTORY OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY IMPLIED WARRANTIES OF TITLE,
 * NON-INFRINGEMENT, MERCHANTABILITY, SATISFACTROY QUALITY, ACCURACY OR FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.
 *
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <paintworker.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cutils/properties.h>
#include <linux/input.h>
#include "getevent.h"
#include "wenote_jni.h"
#include "common.h"
//open header
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//map header
#include <map>
#include <math.h>
#include <time.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include <cstdlib>

using namespace android;
using namespace std;
#include <vector>
#include <list>

static unsigned int  x     = 0;
static unsigned int  y     = 0;
static unsigned int  lastX = 0;
static unsigned int  lastY = 0;
static unsigned int  RelastX = 0;
static unsigned int  RelastY = 0;

//Action Code
static unsigned int ACTION_DOWN = 0;
static unsigned int ACTION_UP = 1;
static unsigned int ACTION_MOVE = 2;
static unsigned int ACTION_OUT = 3;
static unsigned int ACTION_TOOL_UP = -1;

static unsigned int PEN_WIDTH = 4;//画笔宽度
static unsigned int ERASER_WIDTH = 40;//橡皮擦宽度

static SkBitmap bitmap;
static SkCanvas *canvas;
static SkPaint mPaint;
static SkPaint mEraserPaint;
static float XScale = 0.079;//0.999;//0.089;//0.059;//depend on pen device
static float YScale = 0.079;//0.999;//0.089;//0.059;
//static float XScale = 0.11;//0.999;//0.089;//0.059;//depend on pen device
//static float YScale = 0.11;//0.999;//0.089;//0.059;

static float mXScale = 0.999;//depend on device
static float mYScale = 0.998;
static bool mIsTest = true;

//手写区域
static int win_x1, win_y1, win_x2, win_y2;
//手写过滤区域
static int filter_fixed_x1, filter_fixed_x2, filter_fixed_y1, filter_fixed_y2;
static bool mIsFilterFixedArea;

static bool isDown = false;
static bool startDrawPoint = false;//start draw point

static int m_pressed_value = 0;//画笔压力

static list <PointStruct> mPathList;//save the information of one path with points.
static list <list<PointStruct>> mNoteList;//save the information of paths.
static list <list<PointStruct>> mRevokeNoteList;//save the information of removing paths.




PaintWorker::PaintWorker()
    : Worker("PaintWorker", HAL_PRIORITY_URGENT_DISPLAY)
{
    fprintf(stderr, "new PaintWorker\n");

}
PaintWorker::~PaintWorker()
{
    fprintf(stderr, "delete PaintWorker\n");
    if(rgba_buffer != NULL) {
        free(rgba_buffer);
        rgba_buffer = NULL;
    }
    uninit_getevent();
}


int PaintWorker::Init(int left, int top, int right, int bottom, bool isFilterFixedArea, int x1, int y1, int x2, int y2)
{

    ALOGD("---zc---Init left:%d,top:%d,right:%d,bottom:%d", left, top, right, bottom);
    pen_width = 4;
    //画笔
    mPaint.setARGB(255, 0, 0, 0);
    mPaint.setStyle(SkPaint::kStroke_Style);
    //mPaint.setAntiAlias(true);
    //mPaint.setDither(true);
    mPaint.setStrokeWidth(PEN_WIDTH);
    mPaint.setStrokeCap(SkPaint::kRound_Cap);
    mPaint.setStrokeJoin(SkPaint::kRound_Join);
    //橡皮擦
    mEraserPaint.setARGB(255, 255, 255, 255);
    mEraserPaint.setStyle(SkPaint::kStroke_Style);
    //mEraserPaint.setAntiAlias(true);
    //mEraserPaint.setDither(true);
    mEraserPaint.setStrokeWidth(ERASER_WIDTH);
    mEraserPaint.setStrokeCap(SkPaint::kRound_Cap);
    mEraserPaint.setStrokeJoin(SkPaint::kRound_Join);

    if (init_getevent() != 0) {
        ALOGD("error: PaintWorker did not work!\n");
    }


    int fd = open("/dev/ebc", O_RDWR,0);
    if (fd < 0) {
        ALOGE("open /dev/ebc failed\n");
    }

    struct ebc_buf_info_t ebc_buf_info;

    if(ioctl(fd, EBC_GET_BUFFER_INFO,&ebc_buf_info)!=0) {
        ALOGE("GET_EBC_BUFFER failed\n");
    }
    if (ebc_buf_info.panel_color == 2) {
        rgba_buffer = (void *)malloc(ebc_buf_info.width * ebc_buf_info.height * 4);
        memset(rgba_buffer,0xff,ebc_buf_info.width * ebc_buf_info.height * 4);
        bitmap.allocN32Pixels(ebc_buf_info.width, ebc_buf_info.height);
        bitmap.setPixels((void *)rgba_buffer);
    } else {
        rgba_buffer = (void *)malloc(ebc_buf_info.width * ebc_buf_info.height);
        memset(rgba_buffer,0xff,ebc_buf_info.width * ebc_buf_info.height);
        SkImageInfo info;
        info = SkImageInfo::Make(ebc_buf_info.width, ebc_buf_info.height, kGray_8_SkColorType,  kOpaque_SkAlphaType); //
        bitmap.installPixels(info, (void *)rgba_buffer, ebc_buf_info.width);
    }

    close(fd);

    commitWorker.Init();

    commitWorker.set_path_buffer(rgba_buffer);
    commitWorker.setPenWidth(PEN_WIDTH);
    canvas = new SkCanvas(bitmap);
    if(NULL != canvas )
        ALOGD("---canvas exist\n");
    //win_x1 = 0;
    //win_y1 = 0;
    //win_x2 = ebc_buf_info.width;
    //win_y2 = ebc_buf_info.height;
    //w103黑白
    //左上角坐标
    win_x1 = top;
    win_y1 = left;
    //右下角坐标
    win_x2 = bottom;
    win_y2 = right;
    //w103彩屏
    //win_x1 = left;
    //win_y1 = top;
    //win_x2 = left;
    //win_y2 = bottom;
    //ALOGD("---zc--- win_x1:%d,win_y1:%d,win_x2:%d,win_y2:%d\n",
        //win_x1, win_y1, win_x2, win_y2);
    mIsFilterFixedArea = isFilterFixedArea;
    filter_fixed_x1 = y1;
    filter_fixed_y1 = win_y2 - x2;
    filter_fixed_x2 = y2;
    filter_fixed_y2 = win_y2 - x1;
    //ALOGD("---zc--- filter_x1:%d,filter_y1:%d,filter_x2:%d,filter_y2:%d\n",
    //filter_fixed_x1, filter_fixed_y1, filter_fixed_x2, filter_fixed_y2);

    commitWorker.init_draw_win_info(win_x1, win_y1, win_x2, win_y2);

    return InitWorker();
}

void PaintWorker::sethandwriterects(int x1, int y1, int x2, int y2)
{
    win_x1 = x1;
    win_y1 = y1;
    win_x2 = x2;
    win_y2 = y2;
    mIsFilterFixedArea  = 0;
    ALOGD("sethandwriterects() win_x1:%d,win_y1:%d,win_x2:%d,win_y2:%d\n",win_x1, win_y1, win_x2, win_y2);
}

void PaintWorker::setnohandwriterects(int x1, int y1, int x2, int y2)
{
    filter_fixed_x1 = x1;
    filter_fixed_y1 = y1;
    filter_fixed_x2 = x2;
    filter_fixed_y2 = y2;
    mIsFilterFixedArea  = 1;
    ALOGD("setnohandwriterects() win_x1:%d,win_y1:%d,win_x2:%d,win_y2:%d\n",filter_fixed_x1, filter_fixed_y1, filter_fixed_x2, filter_fixed_y2);
}

void PaintWorker::onFirstRef()
{
}
void *PaintWorker::Get_path_buffer()
{
    return rgba_buffer!= NULL ? rgba_buffer : NULL;
}
void PaintWorker::ExitPaintWorker()
{
    commitWorker.Exit();
    Exit();
}

void PaintWorker::ReDrawPoint(int x, int y, int action, int penWidth, int penColor, bool EraserEnable, bool StrokesEnable)
{
    //ALOGD("---start ReDraw -lastX=%d--lastY=%d--x=%d--y=%d--action=%d \n", RelastX, RelastY, x, y, action);
    if(action == ACTION_DOWN) {
    } else if(action == ACTION_MOVE) {
        if(RelastX != 0 || RelastY != 0) {
            if(!EraserEnable) {
                commitWorker.setPenWidth(penWidth);
                mPaint.setStrokeWidth(penWidth);
                SetPenColor(false, penColor);
                canvas->drawLine(RelastX, RelastY, x, y, mPaint);
            }  else {
                commitWorker.setPenWidth(penWidth);
                canvas->drawLine(RelastX, RelastY, x, y, mEraserPaint);
            }
        }
        RelastX = x;
        RelastY = y;
    } else if (action == ACTION_UP) {
        RelastX = 0;
        RelastY = 0;
    } else if (action == ACTION_OUT) {
        RelastX = 0;
        RelastY = 0;
    }
}

void PaintWorker::ShowDrawPoint(int left, int top, int right, int bottom)
{
    commitWorker.init_draw_win_info(left, top, right, bottom);
    commitWorker.Signal();
}

void PaintWorker::ShowReDraw(int x, int y)
{
    commitWorker.set_win_info(x,y);
    commitWorker.Signal();
}

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: Undo
 * Description: 判断noteVector是否大于0，先清除rgba_buffer，再删除noteVector的最后一条数据，遍历noteVector，
 * 往内存里重画一遍，遍历完成送一帧全屏的到驱动，完成撤销，同时重设penWidth，penColor。
 */
int PaintWorker::Undo(int left, int top, int right, int bottom)
{
    if(mNoteList.size() > 0) {
        mRevokeNoteList.push_back(mNoteList.back());
        mNoteList.pop_back();
        Clear(0);
        list<list<PointStruct>>::iterator itPath;
        list<PointStruct>::iterator itPoint;
        list<PointStruct> list_tmp;
        for (itPath = mNoteList.begin(); itPath != mNoteList.end(); itPath++) {
            list_tmp = *itPath;
            for (itPoint = list_tmp.begin(); itPoint != list_tmp.end(); itPoint++) {
                //ALOGD("----Undo--x:%d,y:%d,m_pressed_value:%d,penColor:%d,penWidth:%d,action:%d \n",
                //itPoint->x, itPoint->y, itPoint->m_pressed_value, itPoint->penColor, itPoint->penWidth, itPoint->action);
                ReDrawPoint(itPoint->x, itPoint->y, itPoint->action, itPoint->penWidth, itPoint->penColor,
                            itPoint->eraserEnable, itPoint->strokesEnable);
            }
        }
        ShowDrawPoint(left, top, right, bottom);
        SetPenColor(true, pen_color);
        return 1;
    } else {
        //ALOGD("----noteList.size <= 0");
        return 0;
    }
}

int PaintWorker::Redo(int left, int top, int right, int bottom)
{
    if(mRevokeNoteList.size() > 0) {
        mNoteList.push_back(mRevokeNoteList.back());
        mRevokeNoteList.pop_back();
        Clear(0);
        list<list<PointStruct>>::iterator itPath;
        list<PointStruct>::iterator itPoint;
        list<PointStruct> list_tmp;
        for (itPath = mNoteList.begin(); itPath != mNoteList.end(); itPath++) {
            list_tmp = *itPath;
            for (itPoint = list_tmp.begin(); itPoint != list_tmp.end(); itPoint++) {
                //ALOGD("----Undo--x:%d,y:%d,m_pressed_value:%d,penColor:%d,penWidth:%d,action:%d \n",
                //itPoint->x, itPoint->y, itPoint->m_pressed_value, itPoint->penColor, itPoint->penWidth, itPoint->action);
                ReDrawPoint(itPoint->x, itPoint->y, itPoint->action, itPoint->penWidth, itPoint->penColor,
                            itPoint->eraserEnable, itPoint->strokesEnable);
            }
        }
        ShowDrawPoint(left, top, right, bottom);
        SetPenColor(true, pen_color);
        return 1;
    } else {
        //ALOGD("----noteList.size <= 0");
        return 0;
    }
}

void PaintWorker::Routine()
{


    //ALOGD("chenfujun-----------------------1--------------isHandwritingEnable=%d\n",isHandwritingEnable);
    input_event e;
    if(!isHandwritingEnable) {
        ALOGD("disable handwriting\n");
        return;
    }

    if (get_event(&e, -1) == 0) {
        /*
        *  read axis (x, y)
        */
        if (e.type == EV_ABS && e.code == ABS_PRESSURE) {
           // ALOGD("MyFlash test : ++++++++ press value = %d\n", e.value);
            m_pressed_value = e.value;
        }

        if (e.type == EV_ABS && e.value !=0) {
            if(e.code == ABS_X) { //pen
                x = e.value*XScale;
                //ALOGD("---x---e.value:%d", e.value);
            } else if(e.code == ABS_Y) {
                y = e.value*YScale;
                 //ALOGD("---y---e.value:%d", e.value);
            }
        }
        //ALOGD("---zc---x:%d,y:%d", x ,y);
        /*
        *  the action of point is down
        */




        if (e.type == EV_KEY && e.code == BTN_TOUCH && e.value == 1) {
           // ALOGD("---zc---down x:%d,y:%d,", x, y);
            if(!mIsFilterFixedArea) {
                if((x >= win_x1 && x <= win_x2 && y >= win_y1 && y <= win_y2) || (x == 0 && y == 0)) {
                    Down(x,y);
                }
            } else {
                if(x >= win_x1 && x <= win_x2 && y >= win_y1 && y <= win_y2 &&
                   !(x > filter_fixed_x1 && x < filter_fixed_x2 && y > filter_fixed_y1 && y < filter_fixed_y2)) {
                    Down(x,y);
                }
            }
        }

        /*
        *  the action of point is up
        */
        if (e.type == EV_KEY && e.code == BTN_TOUCH && e.value == 0) {
            //ALOGD("---zc---up x:%d,y:%d,", x, y);
            if(!mIsFilterFixedArea) {
                if(x >= win_x1 && x <= win_x2 && y >= win_y1 && y <= win_y2) {
                    Up(x, y);
                }
            } else {
                if(x >= win_x1 && x <= win_x2 && y >= win_y1 && y <= win_y2 &&
                   !(x > filter_fixed_x1 && x < filter_fixed_x2 && y > filter_fixed_y1 && y < filter_fixed_y2)) {
                    Up(x, y);
                }
            }
        }

        /*
        *  the action of point is move
        */
        if (e.type == EV_SYN) {
            if(isDown) { //touchscreen or pen and touch
                if(!mIsFilterFixedArea) {
                    //ALOGD("---start drawline --x=%d--y=%d \n", x, y);
                    if(x > win_x1 && x < win_x2 && y > win_y1 && y < win_y2) {
                        if(lastX != 0 || lastY != 0) {
                            Move(x, y);
                        }
                        lastX = x;
                        lastY = y;
                    } else {
                        lastX = 0;
                        lastY = 0;
                    }
                } else {
                    if(x > win_x1 && x < win_x2 && y > win_y1 && y < win_y2 &&
                       !(x > filter_fixed_x1 && x < filter_fixed_x2 && y > filter_fixed_y1 && y < filter_fixed_y2)) {
                        //ALOGD("---start drawline -lastX=%d--lastY=%d--x=%d--y=%d \n",lastX,lastY,x,y);
                        if(lastX != 0 || lastY != 0) {
                            Move(x, y);
                        }
                        lastX = x;
                        lastY = y;
                    } else if((!(x > filter_fixed_x1 && x < filter_fixed_x2
                                 && y > filter_fixed_y1 && y < filter_fixed_y2))) {
                        lastX = 0;
                        lastY = 0;
                    } else if ((x > filter_fixed_x1 && x < filter_fixed_x2
                                && y > filter_fixed_y1 && y < filter_fixed_y2)) {
                        lastX = 0;
                        lastY = 0;
                    }
                }
            }
        }
    }
    return;
}


void PaintWorker::SetPenOrEraserMode(bool isEraserEnable)
{
    this->isEraserEnable = isEraserEnable;
}

void PaintWorker::SetStrokesMode(bool isStrokesEnable)
{
    this->isStrokesEnable = isStrokesEnable;
}

void PaintWorker::IsHandwritingEnable(bool isHangdWritingEnable)
{
    this->isHandwritingEnable = isHangdWritingEnable;
}

void PaintWorker::SetPenWidth(int width)
{
    this->pen_width = width;
}

void PaintWorker::SetPenColor(bool isInitColor, int color)
{
    //ALOGD("---isInitColor=%d,color=%d \n", isInitColor, color);
    if(isInitColor)
        this->pen_color = color;
    if (color == PEN_BLACK_COLOR) {
        mPaint.setARGB(255, 0, 0, 0);
        commitWorker.setPenColor(PEN_BLACK_COLOR);
    } else if (color == PEN_BLUE_COLOR) {
        mPaint.setARGB(255, 0, 0, 255);
        commitWorker.setPenColor(PEN_BLUE_COLOR);
    } else if (color == PEN_GREEN_COLOR) {
        mPaint.setARGB(255, 0, 255, 0);
        commitWorker.setPenColor(PEN_GREEN_COLOR);
    } else if (color == PEN_RED_COLOR) {
        mPaint.setARGB(255, 255, 0, 0);
        commitWorker.setPenColor(PEN_RED_COLOR);
    } else if (color == PEN_WHITE_COLOR) {
        mPaint.setARGB(255, 30, 30, 30);
        commitWorker.setPenColor(PEN_WHITE_COLOR);
    }
}


void PaintWorker::SetPenColorAny(bool isInitColor, int color,int A,int R,int G,int B)
{
    //ALOGD("---isInitColor=%d,color=%d \n", isInitColor, color);
    //if(isInitColor)
    //    this->pen_color = color;

        mPaint.setARGB(A, R, G, B);
        commitWorker.setPenColor(color);
    
}




/*bool PaintWorker::GetPenConfig(SkPaint *paint)
{
    if (pen_color != pen_old_color) {
        if (pen_color == PEN_ALPHA_COLOR) {
            paint->setARGB(255, 255, 255, 255);
        }
        if (pen_color == PEN_BLACK_COLOR)
            paint->setARGB(255, 0, 0, 0);
        if (pen_color == PEN_WHITE_COLOR)
            paint->setARGB(255, 30, 30, 30);
        pen_old_color = pen_color;
    }
    if (pen_width != pen_old_width) {
        paint->setStrokeWidth(pen_width);
        pen_old_width = pen_width;
        commitWorker.setPenWidth(pen_width);
    }
    return true;
}*/

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Function: clear
 * Description: There are three clearModes, 1.Use the incomplete clearScreen method;
 * 2.Clear the noteVector and use the complete clearScreen method;
 * 3.Use the complete clearScreen method;
 * Input: The clearMode is the mode to control clearing, it is 0, 1, or 2.
 */
void PaintWorker::Clear(int clearMode)
{
    if(clearMode == 0) {//只清除rga buffer
        commitWorker.clearScreen(false);
    } else if(clearMode == 1) {//清空notelist，清除rga_buffer、gray16_buffer
        mNoteList.clear();
        mRevokeNoteList.clear();
        commitWorker.clearScreen(true);
    } else if (clearMode == 2) {//清除rga_buffer、gray16_buffer,送显
        commitWorker.clearScreen(true);
    }
}

void PaintWorker::Dump(void)
{
    commitWorker.dump();
}

list<PointStruct> PaintWorker::GetPathList()
{
    //ALOGD("--GetPathList--mPathList.size:%d", mPathList.size());
    return mPathList;
}

void PaintWorker::TestDraw(bool isTest)
{
    ALOGD("--TestDraw--isTest:%d", isTest);
    mIsTest = isTest;
    if(mIsTest) {
        RandomDraw();
    }
}

void PaintWorker::RandomDraw()
{
    int testX = 0;
    int textY = 0;
    int testLastX = 0;
    int testLastY = 0;
    while(mIsTest) {
        testX = (rand() % (win_x2 - win_x1 + 1)) + win_x1;
        textY = (rand() % (win_y2 - win_y1 + 1)) + win_y1;
        ALOGD("--TestDraw--testX:%d,textY:%d", testX, textY);
        usleep(50000);
        if(!(testLastX == 0 && testLastY == 0)) {
            canvas->drawLine(testLastX, testLastY, testX, textY, mPaint);
            commitWorker.set_win_info(testX, textY);
            if (testX != testLastX || textY != testLastY)
                commitWorker.Signal();
        }
        testLastX = testX;
        testLastY = textY;
    }

}

void PaintWorker::CheckOutSideLineAndRemove()
{
    if(mNoteList.size() > 0) {
        //ALOGD("---mNoteList.size:%d \n", mNoteList.size());
        list <PointStruct> pathList = mNoteList.back();
        if(pathList.size() <= 2)
            mNoteList.pop_back();
    }
}

void PaintWorker::DrawBitmap(void* pixels, int deviceId, int w, int h, int x1, int y1, int x2, int y2)
{
    SkBitmap bmp = SkBitmap();
    bmp.allocN32Pixels(w, h);
    bmp.setPixels(pixels);
    SkCanvas c(bitmap);
    SkRect srcRect;
    ALOGD("---zc---x1:%d,y1:%d,x2:%d,y2:%d \n", x1, y1, x2, y2);
    if(deviceId == 0) {//具体区域、旋转、平移要根据情况配置,从底层坐标系旋转移动为上层坐标系
        srcRect.setXYWH(0, 0, w, h);
        c.rotate(270.0f);
        c.translate(-y2, x1);
        c.drawBitmapRect(bmp, srcRect, &mPaint);
    }
    commitWorker.setIsSyncBitmap(true);
    ShowDrawPoint(x1, y1, x2, y2);
    usleep(100000);
    commitWorker.setIsSyncBitmap(false);
}

void PaintWorker::Up(int x, int y)
{
    //ALOGD("---up--- x:%d,y:%d", x, y);
    isDown = false;
    lastX = 0;
    lastY = 0;
    PointStruct point;
    point.x = x;
    point.y = y;
    point.pressedValue = m_pressed_value;
    point.penColor = PEN_BLACK_COLOR;
    point.penWidth = PEN_WIDTH;
    point.action = ACTION_UP;
    point.eraserEnable = isEraserEnable;
    point.strokesEnable = isStrokesEnable;
    mPathList.push_back(point);
    mNoteList.push_back(mPathList);
    sendWritingDataToJava(lastX, lastY, x, y, m_pressed_value, PEN_BLACK_COLOR, PEN_WIDTH,
                          ACTION_UP, isEraserEnable, isStrokesEnable);
}

void PaintWorker::Down(int x,int y)
{
    //ALOGD("---down--- x:%d,y:%d", x, y);
    commitWorker.IsOverlayEnable(true);
    isDown = true;
    lastX = 0;
    lastY = 0;
    PointStruct point;
    point.x = x;
    point.y = y;
    point.pressedValue = m_pressed_value;
    point.penColor = PEN_BLACK_COLOR;
    point.penWidth = PEN_WIDTH;
    point.action = ACTION_UP;
    point.eraserEnable = isEraserEnable;
    point.strokesEnable = isStrokesEnable;
    mPathList.push_back(point);
    mNoteList.push_back(mPathList);
    sendWritingDataToJava(lastX, lastY, x, y, m_pressed_value, PEN_BLACK_COLOR, PEN_WIDTH,
                          ACTION_DOWN, isEraserEnable, isStrokesEnable);
}

void PaintWorker::Move(int x, int y)
{
    //ALOGD("---move--- x:%d,y:%d", x, y);
    PointStruct point;
    if(isStrokesEnable) {
        int penWidth = 4;
        if (m_pressed_value <= 2000) {
            penWidth = 1;
        } else if (m_pressed_value > 2000 && m_pressed_value <= 3200) {
            penWidth = ceil((m_pressed_value - 2000) / 300) + 1;
        } else if (m_pressed_value > 3200) {
            penWidth = ceil((m_pressed_value - 3200) / 200) + 5;
        }
        pen_width = penWidth;
    } else {
        //pen_width = pen_width;//PEN_WIDTH;
    }
    commitWorker.setPenWidth(pen_width);
    mPaint.setStrokeWidth(pen_width);
    canvas->drawLine(lastX, lastY, x, y, mPaint);
    commitWorker.set_win_info(x, y);
    if (x != lastX || y != lastY)
        commitWorker.Signal();
    point.x = x;
    point.y = y;
    point.pressedValue = m_pressed_value;
    point.penColor = pen_color;
    point.penWidth = pen_width;
    point.action = ACTION_MOVE;
    point.eraserEnable = isEraserEnable;
    point.strokesEnable = isStrokesEnable;
    mPathList.push_back(point);
    //ALOGD("---move send to java----lastX : %d, lastY : %d, x : %d, y : %d\n",lastX, lastY,x,y);
    sendWritingDataToJava(lastX, lastY, x, y, m_pressed_value, pen_color,
                          pen_width, ACTION_MOVE, isEraserEnable, isStrokesEnable);
}

void PaintWorker::Out(int x, int y)
{
    //ALOGD("---zc--- outside");
    isDown = false;
    PointStruct point;
    point.x = x;
    point.y = y;
    point.pressedValue = m_pressed_value;
    point.penColor = PEN_BLACK_COLOR;
    point.penWidth = PEN_WIDTH;
    point.action = ACTION_OUT;
    point.eraserEnable = isEraserEnable;
    point.strokesEnable = isStrokesEnable;
    mPathList.push_back(point);
    mNoteList.push_back(mPathList);
    sendWritingDataToJava(lastX, lastY, x, y, m_pressed_value, PEN_BLACK_COLOR, PEN_WIDTH,
                          ACTION_OUT, isEraserEnable, isStrokesEnable);
}
