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
#ifndef DRIVE_TYPE_H
#define DRIVE_TYPE_H

#include <stdio.h>
#include <glib.h>

#include <biometric_common.h>
#include <biometric_storage.h>
#include <biometric_version.h>

#include <libfprint-2/fprint.h>

// 状态控制标志定义
#define CONTROL_FLAG_IDLE 0
#define CONTROL_FLAG_RUNNING 1
#define CONTROL_FLAG_STOPING 2
#define CONTROL_FLAG_STOPPED 3
#define CONTROL_FLAG_DONE 4

// 异步控制标志
#define ASYN_FLAG_DONE 0
#define ASYN_FALG_RUNNING 1

// 扩展提示信息的长度
#define EXTRA_INFO_LENGTH 1024

// 检测时间间隔
#define EM1600DEV_FINGER_CHECK_INTERVAL_MS 100

#define DEFAULT_AES_KEY     "This is UKUI default AES key."
#define DEFAULT_AES_KEY_MAX_LEN	(256 / 8)

typedef struct _enroll_data
{
	bio_dev *dev;
	int uid;
	int idx;
	char *bio_idx_name;
}enroll_data;

typedef struct _capture_data
{
	bio_dev *dev;
	char *feature_data;
	char *feature_encode;
}capture_data;

typedef struct _identify_data
{
	bio_dev *dev;
	int uid;
	int idx_start;
	int idx_end;
}identify_data;

typedef struct _search_data
{
	bio_dev *dev;
	int uid;
	int idx_start;
	int idx_end;
	int index;
	feature_info *found;
	feature_info found_head;
}search_data;

// 定义操作提示
enum demo_notify_mid {
	MID_EXTENDED_MESSAGE = NOTIFY_COMM_MAX + 1, // 定义扩展信息
};

// 驱动私有结构体
// 设备驱动私有结构体，指向设备驱动还需要的额外结构体
typedef struct driver_info_t driver_info;
struct driver_info_t {
	int timeoutMS;
	int timeused;
	int ctrlFlag;
	char extra_info[EXTRA_INFO_LENGTH];
	GPtrArray *devices;
	FpDevice *device;
	FpContext *ctx;
	int asyn_flag;
	GCancellable *cancellable;
	unsigned char *aes_key;
	char *community_driver_id;
	int shmid;
	void *shmaddr;
	int fd;
};

typedef struct shared_number_t shared_number;
struct shared_number_t {
	GPtrArray *devices;
	FpDevice *device;
	FpContext *ctx;
	int d_count;
};


#endif // DRIVE_TYPE_H
