#ifndef ANDROID_COMMON_H_
#define ANDROID_COMMON_H_
#include <list>
using namespace android;
using namespace std;

/*
 * Copyright: Rockchip
 * Author: ZhuangChao
 * Struct: PointStruct
 * Description: Save the information of point.
 */
typedef struct {
    int x;
    int y;
    int pressedValue;//压力
    int penColor;//颜色
    int penWidth;//宽度
    int action;//类型
    bool eraserEnable;
    bool strokesEnable;
} PointStruct;

#endif
