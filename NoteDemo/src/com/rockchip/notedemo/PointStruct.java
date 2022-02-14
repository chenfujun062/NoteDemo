package com.rockchip.notedemo;

import java.io.Serializable;
import java.security.PublicKey;

public class PointStruct implements Serializable {
    //Eraser Mode
    public static final int PEN_ERASER_DISABLE = 0;
    public static final int PEN_ERASER_ENABLE = 1;
    //Strokes Mode
    public static final int PEN_STROKES_DISABLE = 0;
    public static final int PEN_STROKES_ENABLE = 1;
    //Action Code
    public static final int ACTION_DOWN = 0;
    public static final int ACTION_UP = 1;
    public static final int ACTION_MOVE = 2;
    public static final int ACTION_OUT = 3;
    public static final int ACTION_TOOL_UP = -1;
    //Pen Color
    public static final int PEN_ALPHA_COLOR = 0;
    public static final int PEN_BLACK_COLOR = 1;
    public static final int PEN_WHITE_COLOR = 2;
    public static final int PEN_BLUE_COLOR = 3;
    public static final int PEN_GREEN_COLOR = 4;
    public static final int PEN_RED_COLOR = 5;

    public int x;
    public int y;
    public int lastX;
    public int lastY;
    public int pressedValue;//压力
    public int penColor;//颜色
    public int penWidth;//宽度
    public int action;//类型
    public boolean eraserEnable;//是否使用橡皮擦
    public boolean strokesEnable;//是否使用笔锋

    public PointStruct(int lastX, int lastY, int x, int y, int pressedValue, int penColor,
                        int penWidth, int action, boolean eraserEnable, boolean strokesEnable) {
        this.lastX = lastX;
        this.lastY = lastY;
        this.x = x;
        this.y = y;
        this.pressedValue = pressedValue;
        this.penColor = penColor;
        this.penWidth = penWidth;
        this.action = action;
        this.eraserEnable = eraserEnable;
        this.strokesEnable = strokesEnable;
    }
}
