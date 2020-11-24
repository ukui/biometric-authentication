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
#include "community_ops.h"

/*
 * 驱动初始化函数
*/
int community_ops_driver_init(bio_dev *dev)
{
	bio_print_debug ("bio_drv_demo_ops_driver_init start\n");

	// 私有结构体
	driver_info *priv = (driver_info *)dev->dev_priv;

	// 私有结构体超时时间成员变量设置初值为：框架通用的操作超时时间
	priv->timeoutMS = bio_get_ops_timeout_ms ();

	// 私有结构体控制标志成员变量设置初值为：CONTROL_FLAG_IDLE
	priv->ctrlFlag = CONTROL_FLAG_IDLE;

	priv->cancellable = NULL;

	// 获取通用的context
	set_fp_common_context (dev);

	bio_print_debug ("bio_drv_demo_ops_driver_init end\n");

	return 0;
}


/*
 * 设备检测函数
*/
int community_ops_discover(bio_dev *dev)
{
	bio_print_debug ("bio_drv_demo_ops_discover start\n");

	int ret = 0;

	if ((strcmp(getenv("BIO_PRINT_LEVEL"), "7") == 0) && (strcmp(getenv("BIO_PRINT_COLOR"), "1") == 0))
		setenv ("G_MESSAGES_DEBUG", "all", 0);

	// 探测设备
	ret = device_discover (dev);
	if ( ret < 0 ) {
		bio_print_info (_("No %s fingerprint device detected\n"), dev->device_name);
		return -1;
	}
	if ( ret == 0 ) {
		bio_print_info (_("No %s fingerprint device detected\n"), dev->device_name);
		return 0;
	}

	bio_print_debug ("bio_drv_demo_ops_discover end\n");

	return ret;
}

/*
 * 驱动释放函数
*/
void community_ops_free(bio_dev *dev)
{
	bio_print_debug ("bio_drv_demo_ops_free start\n");

	driver_info *priv = (driver_info *)dev->dev_priv;

	key_t key;
	shared_number *share;
	// 申请一个key
	key = ftok ("/tmp/biometric_shared_file", 1234);

	priv->shmid = shmget (key, sizeof(shared_number), 0);
	priv->shmaddr = shmat (priv->shmid, NULL, 0);
	share = (shared_number *)(priv->shmaddr);

	share->d_count--;

	if (share->d_count == 0) {
		// 资源释放
		g_object_unref (priv->ctx);
		priv->devices = NULL;
		priv->device = NULL;
		// 解映射
		shmdt (priv->shmaddr);
		// 释放共享内存
		shmctl (priv->shmid, IPC_RMID, NULL);
	}
	// 关闭文件描述符
	close (priv->fd);

	bio_print_debug ("bio_drv_demo_ops_free end\n");
}

/*
 * 设备打开函数
*/
int community_ops_open(bio_dev *dev)
{
	bio_print_debug ("bio_drv_demo_ops_open start\n");

	// 私有结构体
	driver_info *priv = (driver_info *)dev->dev_priv;
	priv->asyn_flag = ASYN_FALG_RUNNING;
	// 私有结构体控制变量成员设置为:CONTROL_FLAG_RUNNING
	priv->ctrlFlag = CONTROL_FLAG_RUNNING;

	if (dev->enable == false) {
		bio_set_dev_status (dev, DEVS_COMM_DISABLE);            //设备状态设置为：设备被禁用
		bio_set_ops_result (dev, OPS_COMM_ERROR);               //操作结果设置为：通用操作错误
		bio_set_notify_abs_mid (dev, NOTIFY_COMM_DISABLE);      //提示消息设置为：设备不可用
		return -1;
	}

	bio_set_dev_status (dev, DEVS_OPEN_DOING);                        //设备状态设置为：正在打开设备

	// 创建一个GCancellable
	priv->cancellable = g_cancellable_new ();

	// 启动异步操作打开设备
	fp_device_open (priv->device, NULL, (GAsyncReadyCallback)on_device_opened, dev);
	while (1) {
		usleep (100);
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
							if(priv->asyn_flag == ASYN_FLAG_DONE) {
								bio_set_ops_abs_result (dev, OPS_OPEN_FAIL);          // 设置操作结果：打开设备失败
								bio_set_notify_abs_mid (dev, NOTIFY_OPEN_FAIL);       // 用户提醒消息：打开设备失败
								bio_set_dev_status (dev, DEVS_COMM_IDLE);             // 设备状态：空闲状态
								return -1;
							}
						}
				}
			}
			priv->timeused += EM1600DEV_FINGER_CHECK_INTERVAL_MS;
			usleep (EM1600DEV_FINGER_CHECK_INTERVAL_MS * 1000);
		}
	}

	// 设置状态
	bio_set_dev_status (dev, DEVS_COMM_IDLE);                    //当前状态设置为：空闲状态
	bio_set_ops_abs_result (dev, OPS_OPEN_SUCCESS);              //操作结果设置为：打开设备成功
	bio_set_notify_abs_mid (dev, NOTIFY_OPEN_SUCCESS);           //提示消息设置为：打开设备成功

	bio_print_debug ("bio_drv_demo_ops_open end\n");

	return 0;
}

/*
 * 设备关闭函数
*/
void community_ops_close(bio_dev *dev)
{
	bio_print_debug ("bio_drv_demo_ops_close start\n");

	// 私有结构体
	driver_info *priv = (driver_info *)dev->dev_priv;

	priv->asyn_flag = ASYN_FALG_RUNNING;

	if (dev->enable == false) {
		bio_set_dev_status (dev, DEVS_COMM_DISABLE);                   //设备状态设置为：设备被禁用
		bio_set_ops_result (dev, OPS_COMM_ERROR);                      //操作结果设置为：通用操作错误
		bio_set_notify_abs_mid (dev, NOTIFY_COMM_DISABLE);             //提示消息设置为：设备不可用
	}

	// 异步关闭
	fp_device_close (priv->device, NULL, (GAsyncReadyCallback)on_device_closed, dev);
	while (1) {
		usleep (100);
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
						if(priv->asyn_flag == ASYN_FLAG_DONE) {
							bio_set_ops_abs_result (dev, OPS_CLOSE_FAIL);          // 设置操作结果：关闭设备失败
							bio_set_notify_abs_mid (dev, NOTIFY_CLOSE_FAIL);       // 用户提醒消息：关闭设备失败
							bio_set_dev_status (dev, DEVS_COMM_IDLE);              // 设备状态：空闲状态
							return;
						}
					}
				}
			}
			priv->timeused += EM1600DEV_FINGER_CHECK_INTERVAL_MS;
			usleep (EM1600DEV_FINGER_CHECK_INTERVAL_MS * 1000);
		}
	}

	priv->ctrlFlag = CONTROL_FLAG_IDLE;

	bio_print_debug ("bio_drv_demo_ops_close end\n");

	return;
}

/*
 * 特征捕获函数，用来捕获生物特征信息
*/
char *community_ops_capture(bio_dev *dev, OpsActions action)
{
	bio_print_debug ("bio_drv_demo_ops_capture start\n");

	capture_data *captureData = malloc(sizeof(capture_data));
	captureData->dev = dev;
	captureData->feature_data = NULL;
	captureData->feature_encode = NULL;

	//设备驱动不启用
	if (dev->enable == false) {
		bio_set_dev_status (dev, DEVS_COMM_DISABLE);              //设备状态设置为：设备被禁用
		bio_set_ops_result (dev, OPS_COMM_ERROR);                 //操作结果设置为：通用操作错误
		bio_set_notify_abs_mid (dev, NOTIFY_COMM_DISABLE);        //提示消息设置为：设备不可用
		return NULL;
	}

	bio_set_dev_status (dev, DEVS_CAPTURE_DOING);                 //设备状态设置为：正在捕获信息

	if (dev->bioinfo.eigtype == EigT_Data) {        // 原始数据

		captureData->feature_data = finger_capture (captureData);
		if (captureData->feature_data == NULL)
			return NULL;

		bio_set_dev_status (dev, DEVS_COMM_IDLE);                    //当前状态设置为：空闲状态
		bio_set_ops_result (dev, OPS_CAPTURE_SUCCESS);               //操作结果设置为：捕获成功
		bio_set_notify_abs_mid (dev, NOTIFY_CAPTURE_SUCCESS);        //提示消息设置为：捕获成功

		bio_print_debug ("bio_drv_demo_ops_capture end 1\n");
			return captureData->feature_data;

	} else if (dev->bioinfo.eigtype == EigT_Eigenvalue || dev->bioinfo.eigtype == EigT_Eigenvector) {   //特征值或者特征向量非纯字符串

		captureData->feature_data = finger_capture(captureData);
		if (captureData->feature_data == NULL)
			return NULL;

		// 64编码
		bio_base64_encode ((guchar *)captureData->feature_data, captureData->feature_encode, (sizeof(captureData->feature_encode)));

		bio_set_dev_status (dev, DEVS_COMM_IDLE);                      //当前状态设置为：空闲状态
		bio_set_ops_result (dev, OPS_CAPTURE_SUCCESS);                 //操作结果设置为：捕获成功
		bio_set_notify_abs_mid (dev, NOTIFY_CAPTURE_SUCCESS);          //提示消息设置为：捕获成功

		bio_print_debug ("bio_drv_demo_ops_capture end 2\n");
			return captureData->feature_encode;

	} else {
		bio_set_dev_status (dev, DEVS_COMM_IDLE);                      //当前状态设置为：空闲状态
		bio_set_ops_result (dev, OPS_CAPTURE_FAIL);                    //操作结果设置为：捕获失败
		bio_set_notify_abs_mid (dev, NOTIFY_CAPTURE_FAIL);             //提示消息设置为：捕获失败

		bio_print_debug ("bio_drv_demo_ops_capture end 3\n");
		return NULL;
	}
}

/*
 * 特征录入函数,用来录入用户的生物特征信息
*/
int community_ops_enroll(bio_dev *dev, OpsActions action, int uid, int idx, char *bio_idx_name)
{
	bio_print_debug ("bio_drv_demo_ops_enroll start\n");
	// 用于获取可用索引
	if (idx == -1)
		idx = bio_common_get_empty_index (dev, uid, 0, -1);

	//设备驱动不启用
	if (dev->enable == false) {
		bio_set_dev_status (dev, DEVS_COMM_DISABLE);                           //设备状态设置为：设备被禁用
		bio_set_ops_result (dev, OPS_COMM_ERROR);                              //操作结果设置为：通用操作错误
		bio_set_notify_abs_mid (dev, NOTIFY_COMM_DISABLE);                     //提示消息设置为：设备不可用
		return -1;
	}

	bio_set_dev_status (dev, DEVS_ENROLL_DOING);                           //设备状态设置为：正在录入

	driver_info *priv = (driver_info *)dev->dev_priv;
	FpPrint *print_template = NULL;
	g_autoptr(GError) error = NULL;

	enroll_data *enrollData ;
	enrollData = (enroll_data *)malloc(sizeof(enroll_data));
	enrollData->dev = dev;
	enrollData->uid = uid;
	enrollData->idx = idx;
	enrollData->bio_idx_name = bio_idx_name;

	priv->asyn_flag = ASYN_FALG_RUNNING;
	priv->timeused = 0;

	// 生成指纹模板
	print_template = print_create_template (priv->device, 1, dev);

	snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "enroll start ! Please press your finger.\n");
	bio_set_notify_abs_mid (dev, MID_EXTENDED_MESSAGE);
	bio_print_debug ("%s\n", bio_get_notify_mid_mesg(dev));

	// 启动异步操作以进行打印
	fp_device_enroll (priv->device, print_template, priv->cancellable, on_enroll_progress, enrollData,
			NULL, (GAsyncReadyCallback)on_enroll_completed,
			enrollData);
	while (1) {
		usleep (100);
		// 异步结束
		if (priv->asyn_flag == ASYN_FLAG_DONE){
			break;
		} else {
			if (priv->timeused > priv->timeoutMS) {
				// 取消libfprint库异步操作
				g_cancellable_cancel (priv->cancellable);
				// 判断取消是否成功
				if (g_cancellable_is_cancelled (priv->cancellable)) {
					while (1) {
						usleep (100);
						if(priv->asyn_flag == ASYN_FLAG_DONE) {
							bio_set_ops_abs_result (enrollData->dev, OPS_ENROLL_TIMEOUT);          // 设置操作结果：录入超时
							bio_set_notify_abs_mid (enrollData->dev, NOTIFY_ENROLL_TIMEOUT);       // 用户提醒消息：录入超时
							bio_set_dev_status (enrollData->dev, DEVS_COMM_IDLE);                  // 设备状态：空闲状态
							return -1;
						}
					}
				}
			}
			priv->timeused += EM1600DEV_FINGER_CHECK_INTERVAL_MS;
			usleep (EM1600DEV_FINGER_CHECK_INTERVAL_MS * 1000);
		}
		// 状态被中断
		if (priv->ctrlFlag == CONTROL_FLAG_STOPING) {
			bio_set_ops_result (enrollData->dev, OPS_COMM_STOP_BY_USER);
			bio_set_notify_mid (enrollData->dev, NOTIFY_COMM_STOP_BY_USER);
			bio_set_dev_status (enrollData->dev, DEVS_COMM_IDLE);
			priv->ctrlFlag = CONTROL_FLAG_STOPPED;
			// 取消libfprint库异步操作
			g_cancellable_cancel (priv->cancellable);
			// 判断取消是否成功
			if (g_cancellable_is_cancelled (priv->cancellable)) {
				while (1) {
					usleep (100);
					if (priv->asyn_flag == ASYN_FLAG_DONE)
						return -1;
				}
			}
		}
	}
	bio_print_debug ("bio_drv_demo_ops_enroll end\n");
	return 0;
}


/*
 * 特征验证函数,使用当前生物特征与指定特征对比,判断是否是同一个特征
*/
int community_ops_verify(bio_dev *dev, OpsActions action, int uid, int idx)
{
	bio_print_debug ("bio_drv_demo_ops_verify start\n");

	if (dev->enable == FALSE) {       // 设备不可用
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return -1;
	}

	bio_set_dev_status (dev, DEVS_VERIFY_DOING);         // 设备状态：正在验证

	driver_info *priv = (driver_info *)dev->dev_priv;
	feature_info *info_list = NULL;
	FpPrint *verify_print = NULL;
	const guchar *stored_data = NULL;
	guchar *template_data = NULL;
	guchar *feature_data = NULL;
	gsize stored_len;
	g_autoptr(GError) error = NULL;

	priv->asyn_flag = ASYN_FALG_RUNNING;
	priv->timeused = 0;

	// 连接数据库
	sqlite3 *db = bio_sto_connect_db ();
	// 从数据库中获取特征列表
	info_list = bio_sto_get_feature_info (db, uid, dev->bioinfo.biotype, dev->device_name, idx, idx);
	// 打印特征列表
	print_feature_info (info_list);
	// 断开数据库
	bio_sto_disconnect_db (db);

	feature_sample *sample = info_list->sample;
	template_data = buf_alloc (sample->no);
	feature_data = buf_alloc (sample->no);
	stored_data = buf_alloc (sample->no);
	stored_len = sample->no;

	// base64解码
	bio_base64_decode (sample->data, feature_data);
	// 解密
	community_internal_aes_decrypt (feature_data, sample->no, priv->aes_key, template_data);

	stored_data = (const guchar *)template_data;

	// 将数据库中的值反序列化
	verify_print = fp_print_deserialize (stored_data, stored_len, &error);
	if (error) {
		g_warning ("Error deserializing data: %s", error->message);
		return -1;
	}
	// 回收资源
	free (template_data);
	// 回收特征数据项
	bio_sto_free_feature_info_list (info_list);

	snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "verify start ! Please press your finger.\n");
	bio_set_notify_abs_mid (dev, MID_EXTENDED_MESSAGE);
	bio_print_debug ("%s\n", bio_get_notify_mid_mesg(dev));

	// 异步验证
	fp_device_verify (priv->device, verify_print, priv->cancellable,
			on_match_cb_verify, dev, NULL,
			(GAsyncReadyCallback) on_verify_completed,
			dev);
	while (1) {
		usleep (100);
		// 异步结束
		if (priv->asyn_flag == ASYN_FLAG_DONE) {
			break;
		} else {
			if(priv->timeused > priv->timeoutMS) {
				// 取消libfprint库异步操作
				g_cancellable_cancel (priv->cancellable);
				// 判断取消是否成功
				if (g_cancellable_is_cancelled (priv->cancellable)) {
					while (1) {
						usleep (100);
						if(priv->asyn_flag == ASYN_FLAG_DONE) {
							bio_set_ops_abs_result (dev, OPS_VERIFY_TIMEOUT);          // 设置操作结果：验证超时
							bio_set_notify_abs_mid (dev, NOTIFY_VERIFY_TIMEOUT);       // 用户提醒消息：验证超时
							bio_set_dev_status (dev, DEVS_COMM_IDLE);                  // 设备状态：空闲状态
							return -1;
						}
					}
				}
			}
			priv->timeused += EM1600DEV_FINGER_CHECK_INTERVAL_MS;
			usleep (EM1600DEV_FINGER_CHECK_INTERVAL_MS * 1000);
		}
		// 状态被中断
		if (priv->ctrlFlag == CONTROL_FLAG_STOPING) {
			bio_set_ops_result (dev, OPS_COMM_STOP_BY_USER);
			bio_set_notify_mid (dev, NOTIFY_COMM_STOP_BY_USER);
			bio_set_dev_status (dev, DEVS_COMM_IDLE);
			priv->ctrlFlag = CONTROL_FLAG_STOPPED;
			// 取消libfprint库异步操作
			g_cancellable_cancel (priv->cancellable);
			// 判断取消是否成功
			if (g_cancellable_is_cancelled (priv->cancellable)) {
				while (1) {
					usleep (100);
					if (priv->asyn_flag == ASYN_FLAG_DONE)
						return -1;
				}
			}
		}
	}

	bio_set_dev_status (dev, DEVS_COMM_IDLE);

	bio_print_debug ("bio_drv_demo_ops_verify end\n");
	return 0;
}

/*
 * 特征识别函数,使用当前生物特征与指定范围的特征比对,识别出当前特征与指定范围内的哪个特征匹配
*/
int community_ops_identify(bio_dev *dev, OpsActions action, int uid, int idx_start, int idx_end)
{
	bio_print_debug ("bio_drv_demo_ops_identify start\n");

	GPtrArray *prints = NULL;
	identify_data *identifyData = malloc(sizeof(identify_data));
	identifyData->dev = dev;
	identifyData->uid = uid;
	identifyData->idx_start = idx_start;
	identifyData->idx_end = idx_end;

	// 设备不可用
	if (dev->enable == FALSE) {
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return -1;
	}

	bio_set_dev_status (dev, OPS_TYPE_IDENTIFY);                    // 设备状态设置为：正在识别指定特征

	// 验证特征操作放在此处
	driver_info *priv = (driver_info *)dev->dev_priv;
	// 异步控制标志
	priv->asyn_flag = ASYN_FALG_RUNNING;
	priv->timeused = 0;

	// 创建指纹集
	prints = create_prints(dev, uid, idx_start, idx_end);

	snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "identify start ! Please press your finger.\n");
	bio_set_notify_abs_mid (dev, MID_EXTENDED_MESSAGE);
	bio_print_debug ("%s\n", bio_get_notify_mid_mesg(dev));

	// 异步识别
	fp_device_identify (priv->device, prints, priv->cancellable, on_match_cb_identify, identifyData, NULL, (GAsyncReadyCallback)on_device_identify, dev);
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
						if(priv->asyn_flag == ASYN_FLAG_DONE) {
							bio_set_ops_abs_result (identifyData->dev, OPS_IDENTIFY_TIMEOUT);          // 设置操作结果：识别超时
							bio_set_notify_abs_mid (identifyData->dev, NOTIFY_IDENTIFY_TIMEOUT);       // 用户提醒消息：识别超时
							bio_set_dev_status (identifyData->dev, DEVS_COMM_IDLE);                    // 设备状态：空闲状态
							return -1;
						}
					}
				}
			}
			priv->timeused += EM1600DEV_FINGER_CHECK_INTERVAL_MS;
			usleep (EM1600DEV_FINGER_CHECK_INTERVAL_MS * 1000);
		}
		// 状态被中断
		if (priv->ctrlFlag == CONTROL_FLAG_STOPING) {
			bio_set_ops_result (identifyData->dev, OPS_COMM_STOP_BY_USER);
			bio_set_notify_mid (identifyData->dev, NOTIFY_COMM_STOP_BY_USER);
			bio_set_dev_status (identifyData->dev, DEVS_COMM_IDLE);
			priv->ctrlFlag = CONTROL_FLAG_STOPPED;
			// 取消libfprint库异步操作
			g_cancellable_cancel (priv->cancellable);
			// 判断取消是否成功
			if (g_cancellable_is_cancelled (priv->cancellable)) {
				while (1) {
					usleep (100);
					if (priv->asyn_flag == ASYN_FLAG_DONE)
						return -1;
				}
			}
		}
	}

	bio_set_dev_status (dev, DEVS_COMM_IDLE);

	bio_print_debug ("bio_drv_demo_ops_identify end\n");

	return identifyData->uid;
}

/*
 * 特征搜索函数,使用当前生物特征与指定范围的特征比对,搜索出指定范围内所有匹配的特征
*/
feature_info *community_ops_search(bio_dev *dev, OpsActions action, int uid, int idx_start, int idx_end)
{
	bio_print_debug ("bio_drv_demo_ops_search start\n");

	GPtrArray *prints = NULL;
	search_data *searchData = malloc(sizeof(search_data));
	searchData->dev = dev;
	searchData->uid = uid;
	searchData->idx_start = idx_start;
	searchData->idx_end = idx_end;
	searchData->index = 0;
	searchData->found = NULL;
	searchData->found_head.next = NULL;      // found_head的下一个特征信息指向空
	searchData->found = &(searchData->found_head);         // found指针指向found_head

	// 设备不可用
	if (dev->enable == FALSE) {
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return NULL;
	}

	bio_set_dev_status (dev, OPS_TYPE_SEARCH);            // 设备状态设置为：正在搜索指定特征

	// 验证特征操作放在此处
	driver_info *priv = (driver_info *)dev->dev_priv;

	snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "search start ! Please press your finger.\n");
	bio_set_notify_abs_mid (dev, MID_EXTENDED_MESSAGE);
	bio_print_debug ("%s\n", bio_get_notify_mid_mesg(dev));

	while (priv->ctrlFlag != CONTROL_FLAG_DONE) {
		// 异步控制标志位
		priv->asyn_flag = ASYN_FALG_RUNNING;
		priv->timeused = 0;
		// 创建指纹集
		prints = create_prints(dev, uid, searchData->index, idx_end);
		// 异步识别
		fp_device_identify (priv->device, prints, priv->cancellable, on_match_cb_search, searchData, NULL, (GAsyncReadyCallback)on_device_identify, dev);
		while (1) {
			usleep (100);
			// 异步结束
			if (priv->asyn_flag == ASYN_FLAG_DONE){
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
							bio_set_ops_abs_result (dev, OPS_SEARCH_TIMEOUT);          // 设置操作结果：搜索超时
							bio_set_notify_abs_mid (dev, NOTIFY_SEARCH_TIMEOUT);       // 用户提醒消息：搜索超时
							bio_set_dev_status (dev, DEVS_COMM_IDLE);                  // 设备状态：空闲状态
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
				bio_set_ops_result(dev, OPS_COMM_STOP_BY_USER);
				bio_set_notify_mid(dev, NOTIFY_COMM_STOP_BY_USER);
				bio_set_dev_status(dev, DEVS_COMM_IDLE);
				priv->ctrlFlag = CONTROL_FLAG_STOPPED;
				// 取消libfprint库异步操作
				g_cancellable_cancel (priv->cancellable);
				// 判断取消是否成功
				if (g_cancellable_is_cancelled (priv->cancellable)) {
					while (1) {
						usleep (100);
						if (priv->asyn_flag == ASYN_FLAG_DONE)
							return NULL;
					}
				}
			}
		}
		searchData->index = searchData->index + 1;
	}

	if (searchData->found) {
		snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "_search fingerprint template successful");

		bio_set_ops_abs_result (searchData->dev, OPS_SEARCH_MATCH);
		bio_set_notify_abs_mid (searchData->dev, NOTIFY_SEARCH_MATCH);
		bio_set_notify_abs_mid (searchData->dev, MID_EXTENDED_MESSAGE);

		bio_print_info("%s\n", bio_get_notify_mid_mesg(searchData->dev));
	} else {
		snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "_search fingerprint template fail");

		bio_set_ops_abs_result (searchData->dev, OPS_SEARCH_NO_MATCH);
		bio_set_notify_abs_mid (searchData->dev, NOTIFY_SEARCH_NO_MATCH);
		bio_set_notify_abs_mid (searchData->dev, MID_EXTENDED_MESSAGE);

		bio_print_info ("%s\n", bio_get_notify_mid_mesg(searchData->dev));
	}

	bio_set_dev_status (dev, DEVS_COMM_IDLE);

	bio_print_debug ("bio_drv_demo_ops_search end\n");

	return searchData->found;
}


/*
 * 特征清理(删除)函数,删除指定范围内的所有特征
*/
int community_ops_clean(bio_dev *dev, OpsActions action, int uid, int idx_start, int idx_end)
{
	bio_print_debug ("bio_drv_demo_ops_clean start\n");

	if (dev->enable == FALSE) {       // 设备不可用
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return 0;
	}

	bio_set_dev_status (dev, DEVS_CLEAN_DOING);       // 设备状态设置为：正在清理特征数据

	sqlite3 *db;
	int ret = 0;
	// 连接数据库
	db = bio_sto_connect_db ();
	// 从数据库中删除特征
	ret = bio_sto_clean_feature_info (db, uid, dev->bioinfo.biotype,
					dev->device_name, idx_start,
					idx_end);
	// 断开数据库
	bio_sto_disconnect_db (db);

	if (ret == 0) {
		bio_set_ops_abs_result (dev, OPS_CLEAN_SUCCESS);              // 操作结果设置为：清理特征成功
		bio_set_notify_abs_mid (dev, NOTIFY_CLEAN_SUCCESS);           // 用户提示设置为：清理特征成功
	} else {
		bio_set_ops_result (dev, OPS_CLEAN_FAIL);                     // 操作结果设置为：清理特征失败
		bio_set_notify_abs_mid (dev, NOTIFY_CLEAN_FAIL);              // 用户提示设置为：清理特征失败
	}

	bio_set_dev_status (dev, DEVS_COMM_IDLE);

	return ret;
}

/*
 * 获取指定设备的特征列表
*/
feature_info *community_ops_get_feature_list(bio_dev *dev, OpsActions action, int uid, int idx_start, int idx_end)
{
	bio_print_debug ("bio_drv_demo_ops_get_feature_list start\n");

	feature_info *found = NULL;
	driver_info *priv = (driver_info *)dev->dev_priv;

	if (dev->enable == FALSE) {           // 设备不可用
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return NULL;
	}

	bio_set_dev_status (dev, OPS_TYPE_GET_FLIST);                        // 设备状态设置为：正在获取特征列表
	// 连接数据库
	sqlite3 *db = bio_sto_connect_db ();
	// 从数据库中获取特征列表
	found = bio_sto_get_feature_info (db, uid, dev->bioinfo.biotype,
					dev->device_name, idx_start,
					idx_end);
	// 打印特征列表
	print_feature_info (found);
	// 断开数据库
	bio_sto_disconnect_db (db);
	// 设置状态
	snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "_get_feature_list fingerprint template seccessful");
	bio_set_dev_status (dev, DEVS_COMM_IDLE);
	bio_set_ops_abs_result (dev, OPS_GET_FLIST_SUCCESS);
	bio_set_notify_abs_mid (dev, MID_EXTENDED_MESSAGE);

	bio_print_info ("%s\n", bio_get_notify_mid_mesg(dev));

	bio_set_dev_status (dev, DEVS_COMM_IDLE);

	return found;
}

/*
 * 中止设备的当前操作
*/
int community_ops_stop_by_user(bio_dev *dev, int waiting_ms)
{
	bio_print_debug ("bio_drv_demo_ops_stop_by_user start\n");

	bio_print_info (("_Device %s[%d] received interrupt request\n"), dev->device_name, dev->driver_id);

	if (bio_get_dev_status(dev) == DEVS_COMM_IDLE)    // 如果设备为空闲状态
		return 0;

	driver_info *priv = (driver_info *)dev->dev_priv;
	int timeout = bio_get_ops_timeout_ms();          // 获取通用超时时间
	int timeused = 0;

	priv->asyn_flag = ASYN_FALG_RUNNING;

	if (waiting_ms < timeout)
		timeout = waiting_ms;

	// 设置状态位，用于通知用户中断
	priv->ctrlFlag = CONTROL_FLAG_STOPING;
	snprintf (priv->extra_info, EXTRA_INFO_LENGTH, ("_Device %s[%d] received interrupt request\n"), dev->device_name, dev->driver_id);
	bio_set_notify_abs_mid (dev, MID_EXTENDED_MESSAGE);

	while ((priv->ctrlFlag != CONTROL_FLAG_STOPPED) &&
		(priv->ctrlFlag != CONTROL_FLAG_DONE) &&
		(priv->ctrlFlag != CONTROL_FLAG_IDLE) &&
		(timeused < timeout))
	{
		timeused += EM1600DEV_FINGER_CHECK_INTERVAL_MS;
		usleep(EM1600DEV_FINGER_CHECK_INTERVAL_MS * 1000);
	}

	if ((priv->ctrlFlag == CONTROL_FLAG_STOPPED)
	    || (priv->ctrlFlag == CONTROL_FLAG_DONE)
	    || (priv->ctrlFlag == CONTROL_FLAG_IDLE)) {
		return 0;
	}

	return -1;
}


/*
 * 获取设备状态的文本消息
*/
const char *community_ops_get_dev_status_mesg(bio_dev *dev)
{
	bio_print_debug ("bio_drv_demo_ops_get_dev_status_mesg end\n");
	return NULL;
}

/*
 * 获取操作结果的文本消息
*/
const char *community_ops_get_ops_result_mesg(bio_dev *dev)
{
	bio_print_debug ("bio_drv_demo_ops_get_ops_result_mesg end\n");
	return NULL;
}

/*
 * 获取面向用户的提示消息
*/
const char *community_ops_get_notify_mid_mesg(bio_dev *dev)
{
	bio_print_debug ("bio_drv_demo_ops_get_notify_mid_mesg start\n");

	driver_info *priv = (driver_info *)dev->dev_priv;

	switch (bio_get_notify_mid(dev))
	{
	case MID_EXTENDED_MESSAGE:
		return priv->extra_info;

	default:
		return NULL;
	}
}

void *community_ops_attach(bio_dev *dev)
{

}

void *community_ops_detach(bio_dev *dev)
{

}
