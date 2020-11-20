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
#ifndef DRIVE_INTERNAL_H
#define DRIVE_INTERNAL_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <unistd.h>
#include <glib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "driver_type.h"
#include "close.h"
#include "aes_128_cfb.h"
#include "community_define.h"
#include "community_ops.h"

#include <biometric_common.h>
#include <biometric_storage.h>
#include <biometric_version.h>
#include <libfprint-2/fprint.h>


// 探测设备
int device_discover(bio_dev *dev);

int set_fp_common_context(bio_dev *dev);

void *buf_alloc(unsigned long size);
void buf_clean(void *buf, unsigned long size);

char *finger_capture(capture_data *user_data);

int community_para_config(bio_dev *dev, GKeyFile *conf);

int community_internal_aes_encrypt(unsigned char *in, int len, unsigned char *key, unsigned char *out);

int community_internal_aes_decrypt(unsigned char *in, int len, unsigned char *key, unsigned char *out);

#endif // DRIVE_INTERNAL_H
