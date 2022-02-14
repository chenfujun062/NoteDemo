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

#include <commitworker.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cutils/properties.h>
//open header
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//map header
#include <map>
#include <time.h>
#include <ui/Region.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include <paintworker.h>

using namespace android;

static pthread_mutex_t win_info_lock;
static int ebc_fd = -1;
static bool mIsSendBuffer = false;
static bool mIsSyncBitmap = false;
static void *rgba_buffer;
static int *gray16_buffer;

#define RGB888_AVG_RGB(r1, r2, r, g, b)    do { \
	uint32_t s1 = (r1[0]) & 0x00F0F0F0; \
	uint32_t s2 = (r1[1]) & 0x00F0F0F0; \
	uint32_t s3 = (r2[0]) & 0x00F0F0F0; \
	uint32_t s4 = (r2[1]) & 0x00F0F0F0; \
	uint32_t s = (s1 + s2 + s3 + s4) >> 2; \
	r = (s >> 4) & 0xF; \
	g = (s >> 12) & 0xF; \
	b = (s >> 20) & 0xF; \
	r1 += 2; \
	r2 += 2; \
} while(0)


void Rgb888_to_color_eink2(char *dst, int *src, int fb_height, int fb_width, int vir_width, Rect *rect)
{
	int i, j;
	char *dst_r1, *dst_r2;
	int *src_r1, *src_r2;
	int h_div2 = fb_height / 2;
	int w_div6 = vir_width / 6;
	uint8_t r, g, b;

	int win_x1;
	int win_x2;
	int win_y1;
	int win_y2;

	win_x1 = rect->left / 6;
	win_x2 = rect->right / 6 + 1;
	win_y1 = rect->top / 2;
	win_y2 = rect->bottom / 2 +1;

	if (win_x2 >= w_div6)
		win_x2 = w_div6;
	if (win_y2 >= h_div2)
		win_y2 = h_div2;

	dst += (win_y1 * vir_width) + (win_x1 * 3);
	src += (win_y1 * vir_width * 2) + (win_x1 * 6);

	for (i = win_y1; i < win_y2; i++) {
		dst_r1 = dst;
		dst_r2 = dst_r1 + (vir_width >> 1);
		src_r1 = src;
		src_r2 = src_r1 + vir_width;
		int row_mod3 = i % 3;
		if (row_mod3 == 0) {
			for (j = win_x1; j < win_x2; j++) {
				RGB888_AVG_RGB(src_r1, src_r2, r, g, b);
				*dst_r1++ = r | (b<<4);
				*dst_r2++ = g | (r<<4);
				RGB888_AVG_RGB(src_r1, src_r2, r, g, b);
				*dst_r1++ = g | (r<<4);
				*dst_r2++ = b | (g<<4);
				RGB888_AVG_RGB(src_r1, src_r2, r, g, b);
				*dst_r1++ = b | (g<<4);
				*dst_r2++ = r | (b<<4);
			}
		} else if (row_mod3 == 1) {
			for (j = win_x1; j < win_x2; j++) {
				RGB888_AVG_RGB(src_r1, src_r2, r, g, b);
				*dst_r1++ = b | (g<<4);
				*dst_r2++ = r | (b<<4);
				RGB888_AVG_RGB(src_r1, src_r2, r, g, b);
				*dst_r1++ = r | (b<<4);
				*dst_r2++ = g | (r<<4);
				RGB888_AVG_RGB(src_r1, src_r2, r, g, b);
				*dst_r1++ = g | (r<<4);
				*dst_r2++ = b | (g<<4);
			}
		} else if (row_mod3 == 2) {
			for (j = win_x1; j < win_x2; j++) {
				RGB888_AVG_RGB(src_r1, src_r2, r, g, b);
				*dst_r1++ = g | (r<<4);
				*dst_r2++ = b | (g<<4);
				RGB888_AVG_RGB(src_r1, src_r2, r, g, b);
				*dst_r1++ = b | (g<<4);
				*dst_r2++ = r | (b<<4);
				RGB888_AVG_RGB(src_r1, src_r2, r, g, b);
				*dst_r1++ = r | (b<<4);
				*dst_r2++ = g | (r<<4);
			}
		}
		dst += vir_width;
		src += (vir_width<<1);
	}
}

void Luma8bit_to_4bit_bw(short int  *src,  char *dst, int w)
{
    int i;
    int g0, g1;
    int src_data;

    for (i = 0; i < w; i += 2) {
        src_data =  *src++;
        g0 = src_data & 0x0f;

        g1 = ((src_data & 0x0f00) >> 4);

        *dst++ = g1 | g0;
    }
}


void Luma8bit_to_4bit_gray(short int  *src,  char *dst, int w, int parity)
{
    int i;
    int g0, g1;
    int src_data;

    for (i = 0; i < w; i += 2) {
        src_data =  *src++;
        g0 = src_data & 0x0f;
        if (g0 != 0x0f) {
            if (parity)
                g0 = 0x0f;
            else
                g0 = 0x00;
        }
        g1 = ((src_data & 0x0f00) >> 4);
        if (g1 != 0xf0) {
            if (parity)
                g1 = 0x00;
            else
                g1 = 0xf0;
        }
        *dst++ = g1 | g0;
    }
}

void gray256_to_gray2_fix(uint8_t *dst, uint8_t *src, int panel_h, int panel_w,
                          int vir_width, Region region, int pen_color)
{
    size_t count = 0;
    const Rect* rects = region.getArray(&count);
    for (int i = 0; i < (int)count; i++) {

        int w = rects[i].right - rects[i].left;
        int offset = rects[i].top * panel_w + rects[i].left;
        int offset_dst = rects[i].top * vir_width + rects[i].left;
        if (offset_dst % 2) {
            offset_dst += (2 - offset_dst % 2);
        }
        if (offset % 2) {
            offset += (2 - offset % 2);
        }
        if ((offset_dst + w) % 2) {
            w -=  (offset_dst + w) % 2;
        }
        for (int h = rects[i].top; h <= rects[i].bottom && h < panel_h; h++) {
            if (pen_color > 2 && !mIsSyncBitmap)
                Luma8bit_to_4bit_gray((short int*)(src + offset), (char*)(dst + (offset_dst >> 1)), w, h&1);
            else
                Luma8bit_to_4bit_bw((short int*)(src + offset), (char*)(dst + (offset_dst >> 1)), w);
            offset += panel_w;
            offset_dst += vir_width;
        }
    }
}

CommitWorker::CommitWorker()
    : Worker("CommitWorker", HAL_PRIORITY_URGENT_DISPLAY)
{
    fprintf(stderr, "new CommitWorker\n");

}

CommitWorker::~CommitWorker()
{
    ALOGE("~CommitWorker");
    fprintf(stderr, "~CommitWorker() \n");
    //apk完全退出时，可能需要销毁
    if(ioctl(ebc_fd, EBC_DISABLE_OVERLAY, NULL) != 0) {
        ALOGE("DISABLE_EBC_OVERLAY failed\n");
    }
    if(ebc_fd > 0) {
        close(ebc_fd);
    }
    if(ebc_buffer_base != NULL) {
        munmap(ebc_buffer_base, EINK_FB_SIZE * 4);
        ebc_buffer_base = NULL;
    }
    if(rgba_buffer != NULL) {
        rgba_buffer = NULL;
    }
}

int CommitWorker::Init()
{
    struct ebc_buf_info_t buf_info;

    ebc_fd = open("/dev/ebc", O_RDWR,0);
    if (ebc_fd < 0) {
        ALOGE("open /dev/ebc failed\n");
    }

    if(ioctl(ebc_fd, EBC_GET_BUFFER_INFO,&ebc_buf_info)!=0) {
        ALOGE("GET_EBC_BUFFER failed\n");
    }

    ebc_buffer_base = mmap(0, EINK_FB_SIZE * 4, PROT_READ|PROT_WRITE, MAP_SHARED, ebc_fd, 0);
    if (ebc_buffer_base == MAP_FAILED) {
        ALOGE("Error mapping the ebc buffer (%s)\n", strerror(errno));
    }

    unsigned long vaddr_real = intptr_t(ebc_buffer_base);
    if(ioctl(ebc_fd, EBC_GET_OSD_BUFFER, &buf_info)!=0) {
        ALOGE("EBC_GET_OSD_BUFFER failed\n");
    }
    gray16_buffer = (int *)(vaddr_real + buf_info.offset);
    memset(gray16_buffer, 0xff, ebc_buf_info.width * ebc_buf_info.height >> 1);
    ALOGD("width:%d,height:%d", ebc_buf_info.width, ebc_buf_info.height);
    if(ioctl(ebc_fd, EBC_ENABLE_OVERLAY, NULL) != 0) {
        ALOGE("ENABLE_EBC_OVERLAY failed\n");
    }

    pthread_mutex_init(&win_info_lock, NULL);

    update_enable = false;
    return InitWorker();
}

void CommitWorker::set_path_buffer(void *path_buffer)
{
    rgba_buffer = path_buffer;
}

void CommitWorker::setPenWidth(int width)
{
    pen_width = width + 10;
}

void CommitWorker::setPenColor(int color)
{
    pen_color = color;
    ALOGE("lyx:----------pen_color = %d\n", pen_color);
}

void CommitWorker::set_point_info(int x, int y)
{
    pthread_mutex_lock(&win_info_lock);
    last_x = x;
    last_y = y;
    update_enable = true;
    win_info.left = x;
    win_info.right = x;
    win_info.top = y;
    win_info.bottom = y;
    pthread_mutex_unlock(&win_info_lock);
}

void CommitWorker::set_win_info(int x, int y)
{
    pthread_mutex_lock(&win_info_lock);
    last_x = x;
    last_y = y;
    update_enable = true;
    if(win_info.left > x)
        win_info.left = x;
    if(win_info.right < x)
        win_info.right = x;
    if(win_info.top > y)
        win_info.top = y;
    if(win_info.bottom < y)
        win_info.bottom = y;
    pthread_mutex_unlock(&win_info_lock);
}

void CommitWorker::set_first_win_info()
{
    win_info.left = last_x;
    win_info.right = last_x;
    win_info.top = last_y;
    win_info.bottom = last_y;
    update_enable = false;
}

void CommitWorker::init_draw_win_info(int x1, int y1, int x2, int y2)
{
    ALOGE("Flash test : +++++++++++++ native_init() read init_draw_win_info(%d, %d, %d, %d)\n", x1, y1, x2, y2);
    pthread_mutex_lock(&win_info_lock);
    win_info.left = x1;
    win_info.right = x2;
    win_info.top = y1;
    win_info.bottom = y2;
    pthread_mutex_unlock(&win_info_lock);
}

void CommitWorker::Routine()
{
    int ret = Lock();
    if (ret) {
        ALOGE("Failed to lock worker, %d", ret);
        return;
    }

    int wait_ret = 0;
    if (rgba_buffer != NULL && ebc_buffer_base != NULL) {
        if(update_enable == false)
            wait_ret = WaitForSignalOrExitLocked();

        ret = Unlock();
        if (ret) {
            ALOGE("Failed to lock worker, %d", ret);
            return;
        }

        struct ebc_buf_info_t buf_info;

        pthread_mutex_lock(&win_info_lock);
        buf_info.win_x1 = win_info.left - pen_width >= 0 ? win_info.left - pen_width : 0;
        buf_info.win_y1 = win_info.top - pen_width >= 0 ? win_info.top - pen_width : 0;
        buf_info.win_x2 = win_info.right + pen_width <= ebc_buf_info.width ? win_info.right + pen_width : ebc_buf_info.width;
        buf_info.win_y2 = win_info.bottom + pen_width <= ebc_buf_info.height ? win_info.bottom + pen_width : ebc_buf_info.height;
        set_first_win_info();
        pthread_mutex_unlock(&win_info_lock);
        unsigned long vaddr_real = intptr_t(ebc_buffer_base);

        Rect rect = Rect(buf_info.win_x1, buf_info.win_y1, buf_info.win_x2, buf_info.win_y2);
        Region region = Region(rect);
        buf_info.epd_mode = EPD_OVERLAY;

        if (ebc_buf_info.panel_color == 2)
            Rgb888_to_color_eink2((char *)gray16_buffer, (int *)rgba_buffer, ebc_buf_info.height, ebc_buf_info.width, ebc_buf_info.width, &rect);
        else
            gray256_to_gray2_fix((uint8_t *)(gray16_buffer), (uint8_t *)rgba_buffer, ebc_buf_info.height, ebc_buf_info.width,ebc_buf_info.width,region,pen_color);

        if(ioctl(ebc_fd, EBC_SEND_OSD_BUFFER,&buf_info)!=0) {
            fprintf(stderr,"SET_EBC_SEND_BUFFER failed\n");
            return;
        }
        return;
    } else {
        int ret = Unlock();
        if (ret) {
            ALOGE("Failed to lock worker, %d", ret);
            return;
        }
        return;
    }
}

void CommitWorker::clearScreen(bool isSendBuffer)
{
    ALOGD("---zc---clearScreen");

    if (ebc_buf_info.panel_color == 2)
        memset(rgba_buffer, 0xff, ebc_buf_info.height * ebc_buf_info.width * 4);
    else
        memset(rgba_buffer, 0xff, ebc_buf_info.height * ebc_buf_info.width);


    memset(gray16_buffer, 0xff, ebc_buf_info.height * ebc_buf_info.width >> 1);

    if(isSendBuffer) {
        struct ebc_buf_info_t buf_info;

        unsigned long vaddr_real = intptr_t(ebc_buffer_base);
        buf_info.win_x1 = 0;
        buf_info.win_y1 = 0;
        buf_info.win_y2 = ebc_buf_info.height;
        buf_info.win_x2 = ebc_buf_info.width;
        buf_info.epd_mode = EPD_OVERLAY;

        if(ioctl(ebc_fd, EBC_SEND_OSD_BUFFER,&buf_info) != 0) {
            fprintf(stderr,"SET_EBC_SEND_BUFFER failed\n");
            return;
        }
    }

}

void CommitWorker::dump(void)
{
    ALOGD("dump");
    char value[PROPERTY_VALUE_MAX];
    char data_name[] = "/data/dump/1.bin";
    char data_name_1[] = "/data/dump/2.bin";
    //sprintf(data_name,"/data/dump/gray8bit_%d_%d.bin", ebc_buf_info.width, ebc_buf_info.height);
    FILE *file = fopen(data_name, "wb");
    if(NULL == rgba_buffer) 
        ALOGD("NULL == rgba_buffer");
    if (file) {
        if (ebc_buf_info.panel_color == 2)
            fwrite(rgba_buffer, ebc_buf_info.height * ebc_buf_info.width * 4, 1, file);
        else
            fwrite(rgba_buffer, ebc_buf_info.height * ebc_buf_info.width, 1, file);
        fclose(file);
    }
    ////sprintf(data_name,"/data/dump/gray4bit_%d_%d.bin", ebc_buf_info.width, ebc_buf_info.height);
    file = fopen(data_name_1, "wb+");
    if (file) {
        fwrite(gray16_buffer, ebc_buf_info.height * ebc_buf_info.width / 2 , 1, file);
        fclose(file);
    }
}

void CommitWorker::IsOverlayEnable(bool isOverlayEnable)
{
    if(isOverlayEnable) {
        //ALOGD("true \n");
        if(ioctl(ebc_fd, EBC_ENABLE_OVERLAY, NULL) != 0) {
            ALOGE("ENABLE_EBC_OVERLAY failed\n");
        }
    } else {
        //ALOGD("false \n");
        if(ioctl(ebc_fd, EBC_DISABLE_OVERLAY, NULL) != 0) {
            ALOGE("DISABLE_EBC_OVERLAY failed\n");
        }
    }
}

void CommitWorker::setIsSendBuffer(bool isSendBuffer)
{
    mIsSendBuffer = isSendBuffer;
}

void CommitWorker::setIsSyncBitmap(bool isSyncBitmap)
{
    mIsSyncBitmap = isSyncBitmap;
}

