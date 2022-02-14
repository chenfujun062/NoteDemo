/*****************************************************************************
 * utils.h
 *****************************************************************************
 * Copyright © 2012 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef LIBVLCJNI_UTILS_H
#define LIBVLCJNI_UTILS_H

#include <jni.h>
#include <string>

struct Fields {
    JNIEnv *env;
    jint SDK_INT;
    struct {
        jclass clazz;
    } IllegalStateException;
    struct {
        jclass clazz;
    } String;
    struct {
        jclass clazz;
        jobject inst;
    } WeNoteActivity;
    struct {
        jclass clazz;
        jmethodID receiveEMREvent;
    } WeNoteView;

    struct {
        jclass clazz;
        jobject inst;
    } app;
};

extern struct Fields fields;
#endif
