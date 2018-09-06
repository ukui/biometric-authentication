/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * Author: Droiing <jianglinxuan@kylinos.cn>
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

#ifndef AUTH_DAEMON_H
#define AUTH_DAEMON_H

#include <pthread.h>
#include <glib/gi18n.h>

#include "lib/biometric_common.h"
#include "biometric-generated.h"
#include "dbus_comm.h"

#define PID_DIR_NUM	2
#define PID_DIR_0	"/run/user/0/"
#define PID_DIR_1	"/tmp/"

#define AUTH_SERVER_NAME	"biometric-authentication"
#define MAX_PID_STRING_LEN	64

#define SIGNAL_DEV_CHANGED		STATUS_TYPE_DEVICE
#define SIGNAL_OPS_CHANGED		STATUS_TYPE_OPERATION
#define SIGNAL_NOTIFY_CHANGED	STATUS_TYPE_NOTIFY

#define MAX_DEVICES	BIO_DRVID_DYNAMIC_MAX

extern GList * bio_drv_list;
extern GList * bio_dev_list;

typedef enum {
	/**
	 * Successful execution.
	 * For verify and identify, it also means successful verification.
	 * 执行成功。
	 * 对于verify和identify还表示验证成功。
	 */
	RetOpsSuccess = 0,

	/**
	 * 执行成功。
	 * 对于verify和identify还表示验证不成功
	 */
	RetOpsNoMatch,

	/**
	 * An error was encountered during execution
	 * 执行过程中遇到错误
	 */
	RetOpsError,

	/**
	 * Device busy
	 * 设备忙
	 */
	RetDeviceBusy,

	/**
	 * No such device
	 * 无该设备
	 */
	RetNoSuchDevice,

	/**
	 * Permission denied
	 * 没有权限
	 */
	RetPermissionDenied,
}RetCode;

char* mesg_nodev;
char* mesg_devbusy;

pthread_mutex_t dev_dbus_mutex[MAX_DEVICES];

#ifndef DOMAIN_NAME
	#define DOMAIN_NAME "biometric-authentication"
#endif

#ifndef LOCALEDIR
	#define LOCALEDIR "/usr/local/share/locale/"
#endif

#endif // AUTH_DAEMON_H

