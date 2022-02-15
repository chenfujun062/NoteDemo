package com.rockchip.notedemo;

import android.Manifest;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.Toast;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

import static java.lang.Thread.sleep;

public class MainActivity extends Activity {
    private static final String TAG = "MainActivity";

    private static Context mContext;
    private static Handler mHandler;

    public static NoteView mView;
    private Button mTestBtn;
    private Button mSaveBtn;
    private Button mBackgroundBtn;
    private Button mRecoveryBtn;
    private Button mCancelBtn;
    private Button mDumpBtn;
    private Button mUndoBtn;
    private Button mRedoBtn;
    private Button mClearBtn;
    private Spinner mPenColorSp;
    private CheckBox mStrokesCheck;
    private CheckBox mEraserCheck;
    private Button mfullframeBtn;
    private Button mfreshmodeBtn;
    private NoteJNI mNativeJNI;
    private Button mpentypeBtn;

    private static int mScreenH;
    private static int mScreenW;
    private static int mLeft;
    private static int mTop;
    private static int mRight;
    private static int mBottom;
    private static int mFilterLeft;
    private static int mFilterTop;
    private static int mFilterRight;
    private static int mFilterBottom;

    private final static int mPenMode = 3;//画笔模式
    private final static int mEraserMode = 4;//橡皮擦模式

    private boolean buttonLock = false;
    private static final int USB_ATTACHED = 1;
    private BroadcastReceiver mBatInfoReceiver;
    private static final String ACTION_USB_STATE = "android.hardware.usb.action.USB_STATE";
    private boolean mLastConnectStatus = true;
    private boolean mConnectStatus = false;
    private boolean initFlag = false;
    private static int num =1;
    private static int type =0;
    public static final String EPD_NULL ="-1";
    public static final String EPD_AUTO ="0";
    public static final String EPD_OVERLAY ="1";
    public static final String EPD_FULL_GC16 ="2";
    public static final String EPD_FULL_GL16 ="3";
    public static final String EPD_FULL_GLR16 ="4";
    public static final String EPD_FULL_GLD16 ="5";
    public static final String EPD_FULL_GCC16 ="6";
    public static final String EPD_PART_GC16 ="7";
    public static final String EPD_PART_GL16 ="8";
    public static final String EPD_PART_GLR16 ="9";
    public static final String EPD_PART_GLD16 ="10";
    public static final String EPD_PART_GCC16 ="11";
    public static final String EPD_A2 ="12";
    public static final String EPD_A2_DITHER ="13";
    public static final String EPD_DU ="14";
    public static final String EPD_DU4 ="15";
    public static final String EPD_A2_ENTER ="16";
    public static final String EPD_RESET ="17";
    public static final String EPD_AUTO_DU ="22";
    public static final String EPD_AUTO_DU4 ="23";



    private Handler handler=new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case USB_ATTACHED:
                    Toast toast = Toast.makeText(mContext, (String)msg.obj, Toast.LENGTH_LONG);
                    toast.show();
                    break;
            }
        }
    };

    private Runnable mRunnable = new Runnable() {
        public void run() {
            int count = 0;
            while(mView.getHeight() <= 0) {
                try {
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                if (count++ > 40) {
                    Log.d(TAG, "Flash test : ++++++++ removeCallbacks");
                    mHandler.removeCallbacks(mRunnable);
                    System.exit(0);
                }
                Log.d(TAG, "Flash test : ++++++++ mView.getHeight() = " + mView.getHeight() + ", count = " + count);
            }
            //左上角坐标
            mLeft = 0;
            mTop = mScreenH - mView.getHeight()-17;
            //右下角坐标
            mRight = mScreenW;
            mBottom = mScreenH-17;
            /*//左上角坐标
            mLeft = 0;
            mTop = 0;
            //右下角坐标
            mRight = mScreenW;
            mBottom = mView.getHeight();*/
            mFilterLeft = 0;
            mFilterTop = 0;
            mFilterRight = 0;
            mFilterBottom = 0;
            /*mLeft = 0;
            mTop = 0;
            mRight = mView.getHeight();
            mBottom = mScreenH;*/
            mView.initNative(new Rect(mLeft, mTop, mRight, mBottom), false,
                    new Rect(mFilterLeft, mFilterTop, mFilterRight, mFilterBottom));
            initFlag = true;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "Flash test : +++++++ onCreate()");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mContext = MainActivity.this;
        mView = (NoteView) findViewById(R.id.note_view);
        //设置竖屏
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        mNativeJNI = new NoteJNI(mContext);
        mBackgroundBtn = (Button) findViewById(R.id.background);
        mBackgroundBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(){
                    @Override
                    public void run() {
                        if(!buttonLock) {
                            buttonLock = true;
                            //需要在子线程中处理的逻辑
                            mView.changeBackground();
                            buttonLock = false;
                        }
                    }
                }.start();
            }
        });

/*        mTestBtn = (Button) findViewById(R.id.test);
        mTestBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(){
                    @Override
                    public void run() {
                        if(!buttonLock) {
                            buttonLock = true;
                            //需要在子线程中处理的逻辑
                            mNativeJNI.native_clear(1);
                            mView.drawBitmap(0, mLeft, mTop, mRight, mBottom);
                            mView.clear();
                            buttonLock = false;
                        }
                    }
                }.start();
            }
        });*/
        /*mTestBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(){
                    @Override
                    public void run() {
                        if(!buttonLock) {
                            buttonLock = true;
                            //需要在子线程中处理的逻辑
                            mNativeJNI.native_set_is_drawing(1);
                            if(canTestDraw) {
                                Log.d(TAG, "canTestDraw: " + canTestDraw);
                                canTestDraw = false;
                                mNativeJNI.native_test_draw(true);
                            } else {
                                Log.d(TAG, "canTestDraw: " + canTestDraw);
                                canTestDraw = true;
                                mNativeJNI.native_test_draw(false);
                            }
                            mNativeJNI.native_set_is_drawing(0);
                            buttonLock = false;
                        }
                    }
                }.start();
            }
        });*/

        /*mSaveBtn = (Button) findViewById(R.id.save);
        mSaveBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(){
                    @Override
                    public void run() {
                        if(!buttonLock) {
                            buttonLock = true;
                            //需要在子线程中处理的逻辑
                            mView.savePicture("NoteDemo", "picture");
                            mView.savePointInfo("NoteDemo", "picture");
                            buttonLock = false;
                        }
                    }
                }.start();
            }
        });*/

        /*mDumpBtn = (Button) findViewById(R.id.dump);
        mDumpBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(){
                    @Override
                    public void run() {
                        if(!buttonLock) {
                            buttonLock = true;
                            //需要在子线程中处理的逻辑
                            mNativeJNI.native_dump();
                            buttonLock = false;
                        }
                    }
                }.start();
            }
        });*/

        mCancelBtn = (Button) findViewById(R.id.cancel);
        mCancelBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mNativeJNI.native_clear(1);
                MainActivity.this.finish();
            }
        });

        mUndoBtn = (Button) findViewById(R.id.undo);
        mUndoBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(){
                    @Override
                    public void run() {
                        if(!buttonLock) {
                            buttonLock = true;
                            //需要在子线程中处理的逻辑
                            mNativeJNI.native_is_handwriting_enable(false);
                            mNativeJNI.native_clear(0);
                            NoteView.isUndoEnable = true;
                            mView.unDo();
                            mView.postInvalidate();
                            NoteView.isUndoEnable = false;
                            if(!NoteView.isEraserEnable) {
                                mNativeJNI.native_is_handwriting_enable(true);
                            }
                            buttonLock = false;
                        }
                    }
                }.start();
            }
        });

        mRedoBtn = (Button) findViewById(R.id.redo);
        mRedoBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(){
                    @Override
                    public void run() {
                        if(!buttonLock) {
                            buttonLock = true;
                            //需要在子线程中处理的逻辑
                            mNativeJNI.native_is_handwriting_enable(false);
                            mNativeJNI.native_clear(0);
                            NoteView.isRedoEnable = true;
                            mView.reDo();
                            mView.postInvalidate();
                            NoteView.isRedoEnable = false;
                            if(!NoteView.isEraserEnable) {
                                mNativeJNI.native_is_handwriting_enable(true);
                            }
                            buttonLock = false;
                        }
                    }
                }.start();
            }
        });

        mClearBtn = (Button) findViewById(R.id.clear);
        mClearBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if(!buttonLock) {
                    buttonLock = true;
                    mView.clear();
                    mNativeJNI.native_clear(1);
                    buttonLock = false;
                }
            }
        });

        mfullframeBtn = (Button) findViewById(R.id.fullframe);
        mfullframeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.d(TAG,"sendOneFullFrame");
                num = ++num;
                if(num > Integer.MAX_VALUE-100){
                    num =1;
                }
                String numStr = num +"";
                Log.d( TAG,"sendOneFullFrame numStr"+numStr);
                setProperty("sys.eink.one_full_mode_timeline",numStr);
                view.postInvalidate();

            }
        });

        mfreshmodeBtn = (Button) findViewById(R.id.freshmode);
        mfreshmodeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.d(TAG,"freshmode");

                Log.d( TAG,"freshmode"+EPD_A2_DITHER);
                setProperty("sys.eink.mode",EPD_A2_DITHER);
                view.postInvalidate();

            }
        });



        mpentypeBtn = (Button) findViewById(R.id.pentype);
        mpentypeBtn.setOnClickListener(new View.OnClickListener() {
          @Override
        public void onClick(View view) {
        
              type = ++type;
              if(type==3)
                type=0;

         Log.d( TAG,"pen type "+type);
        switch (type) {
            case 0:
                 //NoteView.isStrokesEnable = false;
                 mNativeJNI.native_clear(0);
                 mNativeJNI.native_strokes(false);
                 mNativeJNI.native_set_pen_width(4);
                 mView.setPenColor(PointStruct.PEN_BLACK_COLOR);
                 mNativeJNI.native_set_pen_color(true, PointStruct.PEN_BLACK_COLOR);
                break;
            case 1:
                //NoteView.isStrokesEnable = false;
                mNativeJNI.native_clear(0);
                mNativeJNI.native_strokes(true);

                mView.setPenColor(PointStruct.PEN_BLACK_COLOR);
                mNativeJNI.native_set_pen_color(true, PointStruct.PEN_BLACK_COLOR);
                break;
            case 2:
                //NoteView.isStrokesEnable = true;
                mNativeJNI.native_clear(0);
                mNativeJNI.native_strokes(false);
                mNativeJNI.native_set_pen_width(50);

                mView.setPenColor(PointStruct.PEN_BLUE_COLOR);
                mNativeJNI.native_set_pen_color_any(true, PointStruct.PEN_BLUE_COLOR,0x30,0x30,0x30,0x30);
                break;
            }
           } 
        });







        mStrokesCheck = (CheckBox) findViewById(R.id.strokes);
        mStrokesCheck.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    NoteView.isStrokesEnable = true;
                    mNativeJNI.native_clear(0);
                    mNativeJNI.native_strokes(true);
                } else {
                    NoteView.isStrokesEnable = false;
                    mNativeJNI.native_clear(0);
                    mNativeJNI.native_strokes(false);
                }
            }
        });

        mEraserCheck = (CheckBox) findViewById(R.id.eraser);
        mEraserCheck.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    //mNativeJNI.native_is_handwriting_enable(false);
                    //mNativeJNI.native_clear(0);
                    NoteView.isEraserEnable = true;
                    //mNativeJNI.native_clear(0);
                    mView.setPenColor(PointStruct.PEN_WHITE_COLOR);
                    mNativeJNI.native_set_pen_color(true, PointStruct.PEN_WHITE_COLOR);
                    mNativeJNI.native_eraser(true);
                    mNativeJNI.native_set_pen_width(20);
                    //mView.postInvalidate();
                    /*try {
                        sleep(800);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }*/
                    //mNativeJNI.native_is_handwriting_enable(true);
                } else {
                    mNativeJNI.native_clear(0);
                    NoteView.isEraserEnable = false;
                    mNativeJNI.native_is_handwriting_enable(true);
                    mNativeJNI.native_clear(0);
                    mView.setPenColor(PointStruct.PEN_BLACK_COLOR);
                    mNativeJNI.native_set_pen_color(true, PointStruct.PEN_BLACK_COLOR);
                    mNativeJNI.native_eraser(false);
                    mNativeJNI.native_set_pen_width(4); //
                    //mView.postInvalidate();
                    /*try {
                        sleep(800);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }*/
                    //mNativeJNI.native_is_handwriting_enable(true);
                }
                //mNativeJNI.native_is_overlay_enable(true);
            }
        });

        mPenColorSp = (Spinner) findViewById(R.id.pen_color_spinner);
        mPenColorSp.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
                switch (pos) {
                    case 0:
                        if(initFlag) {
                            mNativeJNI.native_clear(0);
                            mView.setPenColor(PointStruct.PEN_BLACK_COLOR);
                            mNativeJNI.native_set_pen_color(true, PointStruct.PEN_BLACK_COLOR);
                        }
                        break;
                    case 1:
                        mNativeJNI.native_clear(0);
                        mView.setPenColor(PointStruct.PEN_BLUE_COLOR);
                        mNativeJNI.native_set_pen_color(true, PointStruct.PEN_BLUE_COLOR);
                        break;
                    case 2:
                        mNativeJNI.native_clear(0);
                        mView.setPenColor(PointStruct.PEN_GREEN_COLOR);
                        mNativeJNI.native_set_pen_color(true, PointStruct.PEN_GREEN_COLOR);
                        break;
                    case 3:
                        mNativeJNI.native_clear(0);
                        mView.setPenColor(PointStruct.PEN_RED_COLOR);
                        mNativeJNI.native_set_pen_color(true, PointStruct.PEN_RED_COLOR);
                        break;
                    case 4:
                        mNativeJNI.native_clear(0);
                        mView.setPenColor(PointStruct.PEN_WHITE_COLOR);
                        mNativeJNI.native_set_pen_color(true, PointStruct.PEN_WHITE_COLOR);
                        break;

                    case 5://set any A R G B

                        mNativeJNI.native_clear(0);
                        mView.setPenColor(PointStruct.PEN_BLUE_COLOR);
                        mNativeJNI.native_set_pen_color_any(true, PointStruct.PEN_BLUE_COLOR,0x30,0x30,0x30,0x30);
                        break;





                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        DisplayMetrics metrics = new DisplayMetrics();
        metrics = getApplicationContext().getResources().getDisplayMetrics();
        mScreenW = metrics.widthPixels;
        mScreenH = metrics.heightPixels;
        mHandler = new Handler();
        mHandler.postDelayed(mRunnable, 100);
        IntentFilter filter = new IntentFilter();
        // 屏幕灭屏广播
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        // 屏幕亮屏广播
        filter.addAction(Intent.ACTION_SCREEN_ON);
        // 屏幕解锁广播
        filter.addAction(Intent.ACTION_USER_PRESENT);
        //注册home键广播
        filter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        //注册USB插入广播
        filter.addAction(ACTION_USB_STATE);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        mBatInfoReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                //Log.d(TAG, "onReceive");
                String action = intent.getAction();
                //Log.d(TAG, "action: " + action);
                if (Intent.ACTION_SCREEN_ON.equals(action)) {
                    Log.d(TAG, "screen on");
                } else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
                    Log.d(TAG, "screen off");
                } else if (Intent.ACTION_USER_PRESENT.equals(action)) {
                    Log.d(TAG, "screen unlock");
                } else if (Intent.ACTION_CLOSE_SYSTEM_DIALOGS.equals(action)) {
                    Log.d(TAG, "Home,Task");
                    //mNativeJNI.native_clear(1);
                    //mView.postInvalidate();
                } /*else if (ACTION_USB_STATE.equals(action)) {
                    mConnectStatus = intent.getExtras().getBoolean("connected");
                    if(mConnectStatus && !mLastConnectStatus) {
                        Message message = new Message();
                        message.what = USB_ATTACHED;
                        message.obj = (String)"usb is connected";
                        handler.sendMessage(message);
                        //mView.postInvalidate();
                        mNativeJNI.native_change_overlay(0);
                        //mNativeJNI.native_set_is_drawing(1);
                    } else if (!mConnectStatus && mLastConnectStatus) {
                        Message message = new Message();
                        message.what = USB_ATTACHED;
                        message.obj = (String)"usb is disconnected";
                        handler.sendMessage(message);
                        //mNativeJNI.native_set_is_drawing(0);
                    }
                    mLastConnectStatus = mConnectStatus;
                    //mNativeJNI.native_change_overlay(0);
                } else if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                    Message message = new Message();
                    message.what = USB_ATTACHED;
                    message.obj = (String)"usb is attached";
                    handler.sendMessage(message);
                }*/
            }
        };
        registerReceiver(mBatInfoReceiver, filter);
    }

    @Override
    public void onResume() {
        Log.d(TAG, "Flash test : +++++++ onResume()");
        if (initFlag)
            mNativeJNI.native_is_handwriting_enable(true);
        mNativeJNI.native_is_overlay_enable(true);
        if(NoteView.isEraserEnable) {
            try {
                sleep(150);
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            mNativeJNI.native_clear(2);
        }
        super.onResume();
    }

    @Override
    public void onPause() {
        Log.d(TAG, "Flash test : +++++++ onPause()");
        mNativeJNI.native_is_handwriting_enable(false);
        mNativeJNI.native_is_overlay_enable(false);
        super.onPause();
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "Flash test : +++++++ onDestroy()");
        if(mHandler != null) {
            Log.d(TAG, "mHandler != null");
            mHandler.removeCallbacks(mRunnable);
            mHandler = null;
        }
        mView.exitNativeOnly();
        mView.clear();
        unregisterReceiver(mBatInfoReceiver);
        super.onDestroy();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG, "keyCode: " + keyCode);
        if (keyCode == KeyEvent.KEYCODE_VOLUME_UP || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_UNKNOWN) {
            /*NoteView.isChangeOverlay = true;
            mView.postInvalidate();*/
            mNativeJNI.native_is_overlay_enable(false);
            //mNativeJNI.native_change_overlay(0);
        }
        return super.onKeyDown(keyCode, event);
    }

/*    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        int x = (int)event.getRawX();
        int y = (int)event.getRawY();
        Log.d(TAG, "x:" + x + ",y:" + y);
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                Log.d(TAG, "ACTION_DOWN: ");
                break;
            case MotionEvent.ACTION_UP:
                Log.d(TAG, "ACTION_UP: ");
                break;
            case MotionEvent.ACTION_MOVE:
                Log.d(TAG, "ACTION_MOVE: ");
                break;
        }
        return super.dispatchTouchEvent(event);
    }*/

    public static String getProperty(String key, String defaultValue) {
        String value = defaultValue;
        try {
            Class c = Class.forName("android.os.SystemProperties");
            Method get = c.getMethod("get", String.class);
            value = (String)(get.invoke(c, key));
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            return value;
        }
    }

    public static void setProperty(String key, String penDrawMode) {
        try {
            Class c = Class.forName("android.os.SystemProperties");
            Method set = c.getMethod("set", String.class, String.class);
            set.invoke(c, key, penDrawMode);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
