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
#include "driver_internal.h"

// 探测设备
int device_discover(bio_dev *dev)
{
	driver_info *priv = (driver_info *)dev->dev_priv;
	int i;
	int deviceNum = 0;
	key_t key;
	shared_number *share;
	// 申请一个key
	key = ftok ("/tmp/biometric_shared_file", 1234);

	priv->shmid = shmget (key, sizeof(shared_number), 0);
	priv->shmaddr = shmat (priv->shmid, NULL, 0);
	share = (shared_number *)(priv->shmaddr);

	// 没有设备返回空
	if (!(share->devices->len)) {
		return 0;
	} else {
		for (i = 0; i < share->devices->len; ++i) {
			// 从设备集中通过索引选择设备
			share->device = g_ptr_array_index (share->devices, i);
			// 获取驱动ID
			const char *drvId = fp_device_get_driver (share->device);
			if (strcmp((char *)drvId, priv->community_driver_id) == 0) {
				deviceNum++;
				share->device = g_ptr_array_index (share->devices, i);
			} else {
				return 0;
			}
		}
	}

	bio_print_debug ("discover device %s (%s) claimed by %s driver\n",
		fp_device_get_device_id (share->device),
		fp_device_get_name (share->device),
		fp_device_get_driver (share->device));

	return deviceNum;
}

int set_fp_common_context(bio_dev *dev)
{
	driver_info *priv = (driver_info *)dev->dev_priv;
	// 打开文件
	priv->fd = open ("/tmp/biometric_shared_file", O_CREAT | O_RDWR, 0664);
	shared_number *share;

	// 申请一个key
	key_t key;
	key = ftok ("/tmp/biometric_shared_file", 1234);

	if (flock (priv->fd , LOCK_EX | LOCK_NB) != -1) {
		// 申请一个共享内存
		priv->shmid = shmget (key, sizeof(shared_number), IPC_CREAT | 0666);
		// 映射共享内存
		priv->shmaddr = shmat (priv->shmid, NULL, 0);
		// 清零空间
		memset(priv->shmaddr, 0, sizeof(shared_number));
		// 设置共享内存
		share = (shared_number *)(priv->shmaddr);

		/*写数据到共享内存*/

		// 获取上下文
		share->ctx = fp_context_new ();
		// 获取设备集
		share->devices = fp_context_get_devices (share->ctx);
		if (!(share->devices)) {
			bio_print_error ("Impossible to get devices");
			return -1;
		}
		// 探测设备
		device_discover (dev);

		share->d_count = 1;

		priv->ctx = share->ctx;
		priv->devices = share->devices;
		priv->device = share->device;

	} else {
		priv->shmid = shmget (key, sizeof(shared_number), 0);
		priv->shmaddr = shmat (priv->shmid, NULL, 0);
		share = (shared_number *)(priv->shmaddr);
		share->d_count++;
		priv->ctx = share->ctx;
		priv->devices = share->devices;
		priv->device = share->device;
	}
	return 0;
}


// 分配空间
void *buf_alloc(unsigned long size)
{
	void *buf = malloc(size);
	memset(buf, 0, size);

	return buf;
}

// 清零空间
void buf_clean(void *buf, unsigned long size)
{
	memset(buf, 0, size);

	return;
}

char *finger_capture(capture_data *user_data)
{
	g_autoptr(GError) error = NULL;
	capture_data *captureData = user_data;
	// 私有结构体
	driver_info *priv = (driver_info *)captureData->dev->dev_priv;
	// 异步控制标志位
	priv->asyn_flag = ASYN_FALG_RUNNING;
	priv->timeused = 0;

	snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "capture start ! Please press your finger.\n");
	bio_set_notify_abs_mid (captureData->dev, MID_EXTENDED_MESSAGE);
	bio_print_debug ("%s\n", bio_get_notify_mid_mesg(captureData->dev));

	// 异步捕获
	fp_device_capture (priv->device, true, priv->cancellable,
		(GAsyncReadyCallback) on_capture_completed,
		captureData);
	while (1) {
		usleep (100);
		// 异步结束
		if (priv->asyn_flag == ASYN_FLAG_DONE) {
			break;
		} else {
			if (priv->timeused > priv->timeoutMS) {
				// 取消libfprint库异步操作
				g_cancellable_cancel (priv->cancellable);
				// 判断取消是否成功
				if (g_cancellable_is_cancelled (priv->cancellable)) {
					while (1) {
						usleep (100);
						if (priv->asyn_flag == ASYN_FLAG_DONE) {
							bio_set_ops_abs_result (captureData->dev, OPS_CAPTURE_TIMEOUT);          // 设置操作结果：录入超时
							bio_set_notify_abs_mid (captureData->dev, NOTIFY_CAPTURE_TIMEOUT);       // 用户提醒消息：录入超时
							bio_set_dev_status (captureData->dev, DEVS_COMM_IDLE);                  // 设备状态：空闲状态
							return NULL;
						}
					}
				}
			}
			priv->timeused += EM1600DEV_FINGER_CHECK_INTERVAL_MS;
			usleep (EM1600DEV_FINGER_CHECK_INTERVAL_MS * 1000);
		}
		// 状态被中断
		if (priv->ctrlFlag == CONTROL_FLAG_STOPING) {
			bio_set_ops_result (captureData->dev, OPS_COMM_STOP_BY_USER);
			bio_set_notify_mid (captureData->dev, NOTIFY_COMM_STOP_BY_USER);
			bio_set_dev_status (captureData->dev, DEVS_COMM_IDLE);
			priv->ctrlFlag = CONTROL_FLAG_STOPPED;
			// 取消libfprint库异步操作
			g_cancellable_cancel (priv->cancellable);
			// 判断取消是否成功
			if (g_cancellable_is_cancelled (priv->cancellable)) {
				while (1) {
					sleep (1);
					if (priv->asyn_flag == ASYN_FLAG_DONE)
						return NULL;
				}
			}
		}
	}

	return captureData->feature_data;
}


int community_para_config(bio_dev * dev, GKeyFile *conf)
{
	driver_info *priv = dev->dev_priv;
	char *key_file = NULL;

	priv->aes_key = NULL;
	GError *err = NULL;

	key_file = g_key_file_get_string (conf, dev->device_name, "AESKey", &err);
	if (err != NULL) {
		bio_print_warning (("Get AES Key File Error[%d]: %s, use default Key.\n"),
				    err->code, err->message);
		g_error_free(err);

		priv->aes_key = malloc(strlen(DEFAULT_AES_KEY) + 1);
		memset (priv->aes_key, 0, strlen(DEFAULT_AES_KEY) + 1);
		sprintf ((char *)priv->aes_key, "%s", DEFAULT_AES_KEY);

		return 0;
	}

	if (access(key_file, F_OK | R_OK) != 0) {
		bio_print_warning (("AES Key File (%s) does not Exist or has no Read "
				    "Permission, use default key.\n"), key_file);

		priv->aes_key = malloc(strlen(DEFAULT_AES_KEY) + 1);
		memset (priv->aes_key, 0, strlen(DEFAULT_AES_KEY) + 1);
		sprintf ((char *)priv->aes_key, "%s", DEFAULT_AES_KEY);

		return 0;
	}

	FILE *fp = NULL;
	int len = 0;
	int read_len = 0;

	fp = fopen (key_file, "r");
	if (fp == NULL) {
		bio_print_warning (("Can not open AES Key File: %s, use default key.\n"), key_file);

		priv->aes_key = malloc(strlen(DEFAULT_AES_KEY) + 1);
		memset (priv->aes_key, 0, strlen(DEFAULT_AES_KEY) + 1);
		sprintf ((char *)priv->aes_key, "%s", DEFAULT_AES_KEY);

		return 0;
	}

	fseek (fp, 0, SEEK_END);
	len = ftell (fp);

	if (len == 0) {
		bio_print_warning (("AES Key File is Enpty, use default Key.\n"));
		fclose (fp);

		priv->aes_key = malloc(strlen(DEFAULT_AES_KEY) + 1);
		memset (priv->aes_key, 0, strlen(DEFAULT_AES_KEY) + 1);
		sprintf ((char *)priv->aes_key, "%s", DEFAULT_AES_KEY);

		return 0;
	}

	if (len > DEFAULT_AES_KEY_MAX_LEN)
		len = DEFAULT_AES_KEY_MAX_LEN;

	priv->aes_key = malloc(len + 1);
	memset (priv->aes_key, 0, len + 1);

	fseek (fp, 0, SEEK_SET);
	read_len = fread (priv->aes_key, 1, len, fp);
	priv->aes_key[read_len * 1] = 0;

	fclose (fp);

	if (priv->aes_key[0] == '\0') {
		bio_print_warning (("AES Key is Enpty, use default Key.\n"));

		free (priv->aes_key);
		priv->aes_key = malloc(strlen(DEFAULT_AES_KEY) + 1);
		memset (priv->aes_key, 0, strlen(DEFAULT_AES_KEY) + 1);
		sprintf ((char *)priv->aes_key, "%s", DEFAULT_AES_KEY);
	}

	return 0;
}

int community_internal_aes_encrypt(unsigned char *in, int len,
			   unsigned char *key, unsigned char *out)
{
	if (!in || !key || !out)
		return -1;

	unsigned char iv[AES_BLOCK_SIZE] = {0};		// 初始偏移向量IV
	int i = 0;

	for (i = 0; i < AES_BLOCK_SIZE; i++)
		iv[i] = i;

	AES_128_CFB_Encrypt(key, iv, in, len, out);

	return 0;
}

int community_internal_aes_decrypt(unsigned char *in, int len,
				    unsigned char *key, unsigned char *out)
{
	if (!in || !key || !out)
		return -1;

	unsigned char iv[AES_BLOCK_SIZE] = {0};		// 初始偏移向量IV
	int i = 0;

	for (i = 0; i < AES_BLOCK_SIZE; i++)
		iv[i] = i;

	AES_128_CFB_Decrypt (key, iv, in, len, out);

	return 0;
}
