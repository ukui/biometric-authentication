/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * Author: Droiing <jianglinxuan@kylinos.cn>
 *         chenziyi <chenziyi@kylinos.cn>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef CLOSE_H
#define CLOSE_H

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "driver_type.h"
#include "driver_internal.h"
#include "aes_128_cfb.h"
#include "community_define.h"

#include <biometric_common.h>
#include <biometric_storage.h>
#include <biometric_version.h>

#include <libfprint-2/fprint.h>

// 创建指纹模板
FpPrint *print_create_template(FpDevice *device, FpFinger finger, bio_dev *dev);

// 设备打开后的回调函数
void on_device_opened(FpDevice *device, GAsyncResult *res, void *user_data);

// 设备录入后的回调函数
void on_enroll_completed(FpDevice *device, GAsyncResult *res, void *user_data);

// 设备录入过程回调函数
void on_enroll_progress(FpDevice *device, gint completed_stages, FpPrint *print, gpointer user_data, GError *error);

// 关闭设备完成回调函数
void on_device_closed(FpDevice *device, GAsyncResult *res, void *user_data);

// 捕获函数完成的回调函数
void on_capture_completed(FpDevice *device, GAsyncResult *res, void *user_data);

// 验证完成回调函数
void on_verify_completed(FpDevice *dev, GAsyncResult *res, void *user_data);

// 识别完成回调函数
void on_device_identify(FpDevice *dev, GAsyncResult *res, void *user_data);

// 创建指纹列表
GPtrArray *create_prints(bio_dev *dev, int uid, int idx_start, int idx_end);

// 验证函数结果函数
void on_match_cb_verify(FpDevice *device, FpPrint *match, FpPrint *print, gpointer user_data, GError *error);

// 识别函数结果函数(搜索)
void on_match_cb_search(FpDevice *device, FpPrint *match, FpPrint *print, gpointer user_data, GError *error);

// 识别函数结果函数(识别)
void on_match_cb_identify(FpDevice *device, FpPrint *match, FpPrint *print, gpointer user_data, GError *error);

#endif // CLOSE_H
