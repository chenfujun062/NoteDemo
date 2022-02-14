#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <linux/fb.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IMemory.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>
#include <binder/Binder.h>
#include <utils/Log.h>

#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <utils/threads.h>
#include <utils/Atomic.h>
#include <utils/String16.h>
#include <android_runtime/AndroidRuntime.h>

#include <android/native_window.h>
#include <gui/Surface.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayInfo.h>

#include <SkTypeface.h>
//#include <SkTemplates.h>

#include <SkRegion.h>
#include <SkDevice.h>
#include <SkRect.h>
#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkStream.h>
#include "../../../external/skia/include/private/SkTemplates.h"

#include <linux/input.h>
#include "getevent.h"


//open header
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//map header
#include <map>

#include <time.h>

#include <paintworker.h>
#include <commitworker.h>


#include <iostream>


#undef NDEBUG

//#define LOG_TAG "DrawPath"

using namespace android;

int main()
{
    fprintf(stderr,"Drawpath start!---------------------------- \n");

    //初始化 PaintWorker 线程类
    PaintWorker paintWorker;
    paintWorker.Init(0, 0, 0, 0, false, 0, 0, 0, 0);

    char command[100];
    /*while(1) {
        if (!strcmp(command,"exit"))
            break;
        else if (!strcmp(command,"alpha"))
            paintWorker.SetPenColor(0);
        else if (!strcmp(command,"black"))
            paintWorker.SetPenColor(1);
        else if (!strcmp(command,"white"))
            paintWorker.SetPenColor(2);
        else if (!strcmp(command,"dump"))
            paintWorker.Dump();
        else if (!strcmp(command,"thick"))
            paintWorker.SetPenWidth(15);
        else if (!strcmp(command,"middle"))
            paintWorker.SetPenWidth(10);
        else if (!strcmp(command,"fine"))
            paintWorker.SetPenWidth(4);
        else if (!strcmp(command,"clr"))
            paintWorker.Clear(2);


        std::cout<< "Cammand you may use:" << std::endl;
        std::cout<< "exit:     to stop demo" << std::endl;
        std::cout<< "alpha:    to set pen color alpha" << std::endl;
        std::cout<< "black:    to set pen color black" << std::endl;
        std::cout<< "white:    to set pen color white" << std::endl;
        std::cout<< "thick:    to draw thick line" << std::endl;
        std::cout<< "middle:   to draw middle line" << std::endl;
        std::cout<< "fine:     to draw fine line" << std::endl;
        std::cout<< "clr:      to clear screen" << std::endl;
        std::cout<< "dump:     to dump buf store to /data/dump/" << std::endl;
        std::cin >> command;
    }*/

    paintWorker.ExitPaintWorker();
    fprintf(stderr, "Drawpath end!---------------------------- \n");

    return 0;
}

