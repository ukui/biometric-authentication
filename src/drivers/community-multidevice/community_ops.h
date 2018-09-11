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

#ifndef COMMON_OPS_H
#define COMMON_OPS_H

#include <libfprint/fprint.h>
#include <glib.h>
#include "community_define.h"

#define EXTRA_INFO_LENGTH	1024

#define OPS_DETECTION_INTERVAL_MS	100

#define DEFAULT_AES_KEY			"This is UKUI default AES key."
#define DEFAULT_AES_KEY_MAX_LEN	(256 / 8)

typedef enum {
	DOR_STOP_BY_USER = -3,
	DOR_TIMEOUT = -2,
	DOR_NO_MATCH = -1,
	DOR_FAIL = DOR_NO_MATCH,
}DRV_OPS_RESULT;

enum community_notify_mid {
	COMMUNITY_ENROLL_BASE = 1000,

	/**
	 * Sample complete
	 * 录入完成
	 */
	COMMUNITY_ENROLL_COMPLETE = COMMUNITY_ENROLL_BASE + FP_ENROLL_COMPLETE,

	/**
	 * Enrollment failed due to incomprehensible data. (Please use the same
	 * finger at different sampling stages of the same enroll)
	 * 由于采样数据无法理解而导致录入失败（请在同一次录入的不同阶段使用同一手指）
	 */
	COMMUNITY_ENROLL_FAIL = COMMUNITY_ENROLL_BASE + FP_ENROLL_FAIL,

	/**
	 * Once the sampling is completed, the sampling needs to be continued
	 * 一次采样完成，需要继续采样
	 */
	COMMUNITY_ENROLL_PASS = COMMUNITY_ENROLL_BASE + FP_ENROLL_PASS,

	/**
	 * Please place your finger again because of poor quality of the sample
	 * or other scanning problems
	 * 由于采样的图像质量较差或其他扫描问题导致采样指纹失败，请重放手指...
	 */
	COMMUNITY_COMMON_RETRY = COMMUNITY_ENROLL_BASE + FP_ENROLL_RETRY,

	/**
	 * Your swipe was too short, please place your finger again.
	 * 由于手指的滑动距离太短导致采样失败，请重放手指...
	 */
	COMMUNITY_COMMON_RETRY_TOO_SHORT = COMMUNITY_ENROLL_BASE + FP_ENROLL_RETRY_TOO_SHORT,

	/**
	 * Didn't catch that, please center your finger on the sensor and try again.
	 * 由于手指偏离设备导致采样失败，请重放手指...
	 */
	COMMUNITY_COMMON_RETRY_CENTER_FINGER = COMMUNITY_ENROLL_BASE + FP_ENROLL_RETRY_CENTER_FINGER,

	/**
	 * Because of the scanning image quality or finger pressure problem, the
	 * sampling failed, please remove the finger and retry
	 * 由于扫描成像质量或手指压力问题导致采样失败。请移开手指后重试
	 */
	COMMUNITY_COMMON_RETRY_REMOVE_FINGER = COMMUNITY_ENROLL_BASE + FP_ENROLL_RETRY_REMOVE_FINGER,

	/**
	 * Unable to generate feature data, enroll failure
	 * 无法生成特征数据，录入失败
	 */
	COMMUNITY_ENROLL_EMPTY,

	/**
	 * Sample start, please press and lift your finger (Some devices need to
	 * swipe your finger)
	 * 录入开始
	 */
	COMMUNITY_SAMPLE_START,

	/**
	 * Extension information, stored in the extension string buf
	 * 扩展信息，保存在扩展字符串buf中
	 */
	COMMUNITY_ENROLL_EXTRA,
};

typedef struct community_fpdev_t community_fpdev;
struct community_fpdev_t
{
	// Main structure of community device
	struct fp_dev *dev;

	// Community driver's ID in libfprint
	int community_driver_id;

	// The community drives the results of internal operations
	int ops_result;

	// Whether internal operation is completed
	bool ops_done;

	// Whether the internal stop operation is completed
	bool ops_stopped;

	// Internal operation timeout, unit: millisecond
	int ops_timeout_ms;

	// Internal operation timeout point
	struct timeval ops_timeout_tv;

	// Internal operation detection interval
	struct timeval ops_detection_interval_tv;

	// Number of samples required for internal enroll operation
	int enroll_times;

	// The number of samples of the current enroll operation
	int sample_times;

	// Fingerprint feature data after internal input operation
	struct fp_print_data *enrolled_print;

	unsigned char * aes_key;

	// Notify message string
	char extra_info[EXTRA_INFO_LENGTH];
};

// Configure driver parameters
int community_para_config(bio_dev * dev, GKeyFile * conf);

// Driver initialization and resource recovery function
int community_ops_driver_init(bio_dev *dev);
void community_ops_free(bio_dev *dev);
int community_ops_discover(bio_dev *dev);

// Device initialization and resource recovery function
int community_ops_open(bio_dev *dev);
void community_ops_close(bio_dev *dev);

// Functional operation of device
int community_ops_enroll(bio_dev *dev, OpsActions action, int uid, int idx,
				  char * bio_idx_name);
int community_ops_verify(bio_dev *dev, OpsActions action, int uid, int idx);
int community_ops_identify(bio_dev *dev, OpsActions action, int uid,
					int idx_start, int idx_end);
feature_info * community_ops_search(bio_dev *dev, OpsActions action, int uid,
				  int idx_start, int idx_end);
int community_ops_clean(bio_dev *dev, OpsActions action, int uid,
				 int idx_start, int idx_end);
feature_info * community_ops_get_feature_list(bio_dev *dev, OpsActions action,
									   int uid, int idx_start, int idx_end);

// Framework auxiliary function
int community_ops_stop_by_user(bio_dev *dev, int waiting_ms);
const char * community_ops_get_dev_status_mesg(bio_dev *dev);
const char * community_ops_get_ops_result_mesg(bio_dev *dev);
const char * community_ops_get_notify_mid_mesg(bio_dev *dev);
void community_ops_attach(bio_dev *dev);
void community_ops_detach(bio_dev *dev);

#endif // COMMON_OPS_H
