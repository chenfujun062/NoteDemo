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

#ifndef ANDROID_PAINT_WORKER_H_
#define ANDROID_PAINT_WORKER_H_

#include "worker.h"
#include <SkTypeface.h>
#include <SkRegion.h>
#include <SkDevice.h>
#include <SkRect.h>
#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkRegion.h>
#include <SkPath.h>
#include "../../../../external/skia/include/private/SkTemplates.h"
#include <system/graphics.h>
#include <commitworker.h>
#include <list>
#include <common.h>


using namespace android;
using namespace std;


class PaintWorker : public Worker
{
public:
    PaintWorker();
    ~PaintWorker() override;
    int Init(int left, int top, int right, int bottom, bool isFilterFixedArea, int x1, int y1, int x2, int y2);
    bool GetPenConfig(SkPaint  *paint);
    void *Get_path_buffer();
    void ExitPaintWorker();
    void ReDrawPoint(int x, int y, int action, int penWidth, int penColor, bool isEraserEnable, bool isStrokesEnable);
    void ShowDrawPoint(int left, int top, int right, int bottom);
    void SetPenColor(bool isInitColor, int color);
    void SetPenWidth(int width);
    void Clear(int clearMode);
    void IsHandwritingEnable(bool isHangdWritingEnable);
    void Dump(void);
    int Undo(int left, int top, int right, int bottom);
    int Redo(int left, int top, int right, int bottom);
    int ReDraw(int left, int top, int right, int bottom);
    void ShowReDraw(int x, int y);
    void SetPenOrEraserMode(bool isEraserEnable);
    void SetStrokesMode(bool isStrokesEnable);
    list<PointStruct> GetPathList();
    void TestDraw(bool isTest);
    void RandomDraw();
    void CheckOutSideLineAndRemove();
    void DrawBitmap(void* pixels, int deviceId, int w, int h, int x1, int y1, int x2, int y2);
    void Up(int x, int y);
    void Down(int x,int y);
    void Move(int x, int y);
    void Out(int x, int y);
    void sethandwriterects(int x1, int y1, int x2, int y2);
    void setnohandwriterects(int x1, int y1, int x2, int y2);
    void SetPenColorAny(bool isInitColor, int color,int A,int R,int G,int B);
protected:
    void Routine() override;

private:

    void onFirstRef();
    void *rgba_buffer;
    //CommitWorker çº¿ç¨‹ç±»
    CommitWorker commitWorker;
    int pen_color;
    //int pen_old_color;
    int pen_width;
    //int pen_old_width;
    bool isHandwritingEnable = true;
    bool isEraserEnable = false;
    bool isStrokesEnable = false;
};

#define PEN_ALPHA_COLOR 0
#define PEN_BLACK_COLOR 1
#define PEN_WHITE_COLOR 2
#define PEN_BLUE_COLOR 3
#define PEN_GREEN_COLOR 4
#define PEN_RED_COLOR 5

#endif

