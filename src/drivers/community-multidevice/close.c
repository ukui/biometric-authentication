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
#include "close.h"

// 创建指纹模板
FpPrint *print_create_template(FpDevice *device, FpFinger finger, bio_dev *dev)
{
	bio_print_debug ("print_create_template start\n");

	driver_info *priv = dev->dev_priv;
	priv->device = device;
	// 智能指针
	g_autoptr(GDateTime) datetime = NULL;
	g_autoptr(GDate) date = NULL;
	FpPrint *template = NULL;
	gint year;
	gint month;
	gint day;

	// 创建一个新的FpPrint指纹
	template = fp_print_new (priv->device);
	// 设置指纹的手指
	fp_print_set_finger (template, finger);
	// 设置指纹的用户名
	fp_print_set_username (template, g_get_user_name ());
	// 获取当前时间
	datetime = g_date_time_new_now_local ();
	g_date_time_get_ymd (datetime, &year, &month, &day);
	date = g_date_new_dmy (day, month, year);
	// 设置录入指纹时间
	fp_print_set_enroll_date (template, date);

	bio_print_debug ("print_create_template end\n");

	return template;
}

// 设备打开后的回调函数
void on_device_opened(FpDevice *device, GAsyncResult *res, void *user_data)
{
	bio_dev *dev = user_data;
	driver_info *priv = dev->dev_priv;
	priv->device = device;
	// 智能指针
	g_autoptr(GError) error = NULL;

	if (!fp_device_open_finish (priv->device, res, &error)) {
		bio_print_error ("Failed to open device: %s", error->message);
		return;
	}

	bio_print_debug ("Opened device. It's now time to operate.\n\n");
	priv->asyn_flag = ASYN_FLAG_DONE;
}

// 设备关闭后的回调函数
void on_device_closed(FpDevice *device, GAsyncResult *res, void *user_data)
{
	bio_dev *dev = user_data;
	driver_info *priv = dev->dev_priv;
	priv->device = device;
	g_autoptr(GError) error = NULL;

	fp_device_close_finish (priv->device, res, &error);
	if (error)
		bio_print_error ("Failed closing device %s\n", error->message);

	priv->asyn_flag = ASYN_FLAG_DONE;
}

// 设备录入过程回调函数，参数不可更改
void on_enroll_progress(FpDevice *device, gint completed_stages, FpPrint *print, gpointer user_data, GError *error)
{
	bio_print_debug ("on_enroll_progress start\n");

	enroll_data *enrollData = user_data;
	driver_info *priv = enrollData->dev->dev_priv;
	priv->device = device;

	if (error) {
		bio_print_error ("Enroll stage %d of %d failed with error %s",
				 completed_stages,
				 fp_device_get_nr_enroll_stages (priv->device),
				 error->message);
	}

	snprintf (priv->extra_info, EXTRA_INFO_LENGTH, ("Enroll stage[ %d / %d ] passed. Yay! Please press your finger again.\n"),
		completed_stages, fp_device_get_nr_enroll_stages (device));
	bio_set_notify_abs_mid (enrollData->dev, MID_EXTENDED_MESSAGE);
	bio_print_debug ("%s\n", bio_get_notify_mid_mesg (enrollData->dev));

	priv->timeused = 0;

	bio_print_debug ("on_enroll_progress end\n");
}

// 录入函数完成后的回调函数
void on_enroll_completed(FpDevice *device, GAsyncResult *res, void *user_data)
{
	bio_print_debug ("on_enroll_completed start\n");

	enroll_data *enrollData = user_data;
	driver_info *priv = (driver_info *)enrollData->dev->dev_priv;
	g_autoptr(FpPrint) print = NULL;
	g_autoptr(GError) error = NULL;
	g_autofree guchar *data = NULL;
	gsize size;
	guchar *feature_data = NULL;
	char *feature_encode = NULL;

	// 录入是否成功
	print = fp_device_enroll_finish (device, res, &error);

	// 成功
	if (!error) {
		// 序列化永久存储的指纹定义。
		fp_print_serialize (print, &data, &size, &error);

		if (error) {
			g_warning ("Error serializing data: %s", error->message);
			return;
		}
		feature_data = buf_alloc(size);
		feature_encode = buf_alloc(size * 2 + 1);

		// 特征值加密处理
		community_internal_aes_encrypt (data, size, priv->aes_key, feature_data);
		// 64编码
		bio_base64_encode (feature_data, feature_encode, size);
		// 存储模板
		feature_info *info;
		// 创建特征结构体
		info = bio_sto_new_feature_info (enrollData->uid, enrollData->dev->bioinfo.biotype, enrollData->dev->device_name, enrollData->idx, enrollData->bio_idx_name);
		// 创建新的采样结构体
		info->sample = bio_sto_new_feature_sample (-1, NULL);
		info->sample->no = size;
		info->sample->data = bio_sto_new_str (feature_encode);
		// 打印特征结构体
		print_feature_info (info);
		// 连接数据库
		sqlite3 *db = bio_sto_connect_db ();
		// 保存特征信息链表
		bio_sto_set_feature_info (db, info);
		// 断开数据库连接
		bio_sto_disconnect_db (db);
		 // 回收特征数据项
		bio_sto_free_feature_info_list (info);
	} else {
		bio_print_error ("Enroll failed with error %s\n", error->message);
		priv->asyn_flag = ASYN_FLAG_DONE;
		return;
	}

	bio_set_ops_abs_result (enrollData->dev, OPS_ENROLL_SUCCESS);		// 设置操作结果：录入信息成功
	bio_set_notify_abs_mid (enrollData->dev, NOTIFY_ENROLL_SUCCESS);	// 用户提醒消息：录入信息成功
	bio_set_dev_status (enrollData->dev, DEVS_COMM_IDLE);			// 设备状态：空闲状态

	bio_print_debug ("on_enroll_completed end\n");

	priv->asyn_flag = ASYN_FLAG_DONE;

	return;
}

// 捕获函数完成回调函数
void on_capture_completed(FpDevice *device, GAsyncResult *res, void *user_data)
{
	capture_data *captureData = user_data;
	driver_info *priv = (driver_info *)captureData->dev->dev_priv;
	priv->device = device;
	g_autoptr(FpImage) img = NULL;
	g_autoptr(GError) error = NULL;
	gsize size;

	img = fp_device_capture_finish (priv->device, res, &error);

	if (!error){
		// 获取图像的数据
		const guchar *data = fp_image_get_binarized (img, &size);
		captureData->feature_data = buf_alloc (size);
		captureData->feature_encode = buf_alloc (size * 2 +1);
		captureData->feature_data = (char *)data;
		bio_print_debug ("Captrue successful!!\n");
		priv->asyn_flag = ASYN_FLAG_DONE;
		return;
	} else {
		bio_print_debug ("Capture failed with error %s\n", error->message);
		priv->asyn_flag = ASYN_FLAG_DONE;
		return;
	}
}

// 验证完成回调函数
void on_verify_completed(FpDevice *device, GAsyncResult *res, void *user_data)
{
	bio_dev *dev = user_data;
	driver_info *priv = (driver_info *)dev->dev_priv;
	priv->device = device;

	g_autoptr(FpPrint) print = NULL;
	g_autoptr(GError) error = NULL;
	gboolean match;

	// 异步验证失败
	if (!fp_device_verify_finish (priv->device, res, &match, &print, &error)) {
		bio_print_error ("Failed to verify print: %s\n", error->message);
		priv->asyn_flag = ASYN_FLAG_DONE;
		return;
	} else {
		priv->asyn_flag = ASYN_FLAG_DONE;
		return;
	}
}

// 创建指纹列表
GPtrArray *create_prints(bio_dev *dev, int uid, int idx_start, int idx_end)
{
	bio_print_debug ("create_prints start\n");

	driver_info *priv = (driver_info *)dev->dev_priv;
	GPtrArray *prints = NULL;
	feature_info *info_list = NULL;
	const guchar *stored_data = NULL;
	gsize stored_len;
	g_autoptr(GError) error = NULL;
	guchar *template_data = NULL;

	// 连接数据库
	sqlite3 *db = bio_sto_connect_db ();
	// 获取特征列表
	info_list = bio_sto_get_feature_info (db, uid, dev->bioinfo.biotype, dev->device_name, idx_start, idx_end);
	// 打印特征列表
	print_feature_info (info_list);
	// 断开数据库
	bio_sto_disconnect_db (db);
	// 创建一个GPtrArray
	prints = g_ptr_array_new ();

	// 为匹配的特征构建新的特征链表，每个特征值对应一个采样特征值
	while (info_list != NULL) {
		feature_sample *sample = info_list->sample;
		while (sample != NULL) {
			FpPrint *print = NULL;
			char *temp = (char *)stored_data;
			template_data = buf_alloc (sample->no);
			stored_data = buf_alloc (sample->no);
			stored_len = sample->no;
			// 特征解码
			bio_base64_decode (sample->data, template_data);
			// 解密
			community_internal_aes_decrypt (template_data, sample->no, priv->aes_key, (guchar *)stored_data);
			// 将数据库中的值反序列化
			print = fp_print_deserialize (stored_data, stored_len, &error);
			// 插入数据
			g_ptr_array_add (prints, print);

			sample = sample->next;
			free (temp);
			temp = NULL;
			stored_data = NULL;
			free (template_data);
		}
		info_list = info_list->next;
	}
	// 回收特征数据项
	bio_sto_free_feature_info_list (info_list);

	bio_print_debug ("create_prints end\n");

	return prints;
}

// 识别完成回调函数
void on_device_identify(FpDevice *device, GAsyncResult *res, void *user_data)
{
	bio_dev *dev = user_data;
	driver_info *priv = (driver_info *)dev->dev_priv;
	priv->device = device;
	g_autoptr(GError) error = NULL;
	g_autoptr(FpPrint) match = NULL;
	g_autoptr(FpPrint) print = NULL;

	fp_device_identify_finish (priv->device, res, &match, &print, &error);
	if (error) {
		bio_print_error ("Failed identify device %s\n", error->message);
		priv->asyn_flag = ASYN_FLAG_DONE;
	}

	priv->asyn_flag = ASYN_FLAG_DONE;
}

// 验证函数结果函数
void on_match_cb_verify(FpDevice *device, FpPrint *match, FpPrint *print, gpointer user_data, GError *error)
{
	bio_print_debug ("on_math_cb start\n");

	bio_dev *dev = user_data;
	driver_info *priv = (driver_info *)dev->dev_priv;

	if (error) {
		bio_print_error ("Match report: Finger not matched, retry error reported: %s", error->message);
		return;
	}

	if (match) {
		char date_str[128];

		// 以特定于区域设置的方式生成日期的打印表示形式。
		g_date_strftime (date_str, G_N_ELEMENTS (date_str), "%Y-%m-%d\0", fp_print_get_enroll_date (match));
		bio_print_debug ("Match report: device %s matched finger successifully "
				 "with print %s, enrolled on date %s by user %s",
				 fp_device_get_name (device),
				 fp_print_get_description (match),
				 date_str,
				 fp_print_get_username (match));
		bio_print_debug ("MATCH!\n");

		snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "_Verify fingerprint template successful");
		bio_set_ops_abs_result (dev, OPS_VERIFY_MATCH);
		bio_set_notify_abs_mid (dev, NOTIFY_VERIFY_MATCH);
		bio_set_notify_abs_mid (dev, MID_EXTENDED_MESSAGE);

	} else {
		bio_print_debug ("Match report: Finger not matched");
		bio_print_debug ("NO MATCH!\n");

		snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "_Verify fingerprint template fail");
		bio_set_ops_abs_result (dev, OPS_VERIFY_NO_MATCH);
		bio_set_notify_abs_mid (dev, NOTIFY_VERIFY_NO_MATCH);
		bio_set_notify_abs_mid (dev, MID_EXTENDED_MESSAGE);
	}
}

// 识别函数结果函数(搜索)
void on_match_cb_search(FpDevice *device, FpPrint *match, FpPrint *print, gpointer user_data, GError *error)
{
	bio_print_debug ("on_math_cb start\n");

	search_data *searchData = user_data;
	driver_info *priv = (driver_info *)searchData->dev->dev_priv;
	priv->device = device;
	feature_info *info = NULL;
	feature_info *info_list = NULL;
	const guchar *stored_data = NULL;
	gsize stored_len;
	guchar *template_data = NULL;

	if (error)  {
		bio_print_error ("Match report: Finger not matched, retry error reported: %s", error->message);
		return;
	}

	if (match) {
		// 连接数据库
		sqlite3 *db = bio_sto_connect_db();
		// 获取特征列表
		info_list = bio_sto_get_feature_info(db, searchData->uid, searchData->dev->bioinfo.biotype, searchData->dev->device_name, searchData->index, searchData->idx_end);
		// 打印特征列表
		print_feature_info(info_list);
		// 断开数据库
		bio_sto_disconnect_db(db);

		info = info_list;            // info指向info_list

		while (info != NULL) {
			feature_sample *sample = info->sample;
			while (sample != NULL) {
				FpPrint *compare_print = NULL;
				char *temp = (char *)stored_data;
				template_data = buf_alloc (sample->no);
				stored_data = buf_alloc (sample->no);
				stored_len = sample->no;
				// 特征解码
				bio_base64_decode (sample->data, template_data);
				// 解密
				community_internal_aes_decrypt (template_data, sample->no, priv->aes_key, (guchar *)stored_data);
				// 将数据库中的值反序列化
				compare_print = fp_print_deserialize (stored_data, stored_len, &error);
				// 指纹比较是否相等
				if (fp_print_equal (match, compare_print)) {
					searchData->found->next = bio_sto_new_feature_info (info->uid, info->biotype, info->driver, info->index, info->index_name);
					searchData->found->next->sample = bio_sto_new_feature_sample (sample->no, sample->data);
					searchData->found = searchData->found->next;
					searchData->index = info->index;
				} else {
					if(sample->next != NULL) {
						free(temp);
						temp = NULL;
						stored_data = NULL;
						free(template_data);
						sample = sample->next;
					} else {
						free(temp);
						temp = NULL;
						stored_data = NULL;
						free(template_data);
						break;
					}
				}
				sample = sample->next;
				free(temp);
				temp = NULL;
				stored_data = NULL;
				free(template_data);
			}
			info = info->next;
		}
	snprintf (priv->extra_info, EXTRA_INFO_LENGTH, ("search successful [index = %d] ! Please press your finger again to search.\n"), searchData->index);
	bio_set_notify_abs_mid (searchData->dev, MID_EXTENDED_MESSAGE);
	bio_print_debug ("%s\n", bio_get_notify_mid_mesg(searchData->dev));
	bio_sto_free_feature_info_list (info_list);	// 释放原始的特征列表

	} else {
		searchData->found = searchData->found_head.next;
		priv->ctrlFlag = CONTROL_FLAG_DONE;         // 控制标志设置为：4
	}

	return;
}

// 识别函数结果函数(识别)
void on_match_cb_identify (FpDevice *device, FpPrint *match, FpPrint *print, gpointer user_data, GError *error)
{
	bio_print_debug ("on_math_cb_identify start\n");

	identify_data *identifyData = user_data;
	driver_info *priv = (driver_info *)identifyData->dev->dev_priv;
	priv->device = device;
	feature_info *info_list = NULL;
	const guchar *stored_data = NULL;
	gsize stored_len;
	guchar *template_data = NULL;

	if (error)  {
		bio_print_error ("Match report: Finger not matched, retry error reported: %s\n", error->message);
		return;
	}

	if (match) {
		// 连接数据库
		sqlite3 *db = bio_sto_connect_db ();
		// 获取特征列表
		info_list = bio_sto_get_feature_info (db, identifyData->uid, identifyData->dev->bioinfo.biotype, identifyData->dev->device_name, identifyData->idx_start, identifyData->idx_end);
		// 打印特征列表
		print_feature_info (info_list);
		// 断开数据库
		bio_sto_disconnect_db (db);
		// 为匹配的特征构建新的特征链表，每个特征值对应一个采样特征值
		while (info_list != NULL) {
			feature_sample *sample = info_list->sample;
			while (sample != NULL) {
				FpPrint *compare_print = NULL;
				char *temp = (char *)stored_data;
				template_data = buf_alloc(sample->no);
				stored_data = buf_alloc(sample->no);
				stored_len = sample->no;
				// 特征解码
				bio_base64_decode (sample->data, template_data);
				// 解密
				community_internal_aes_decrypt (template_data, sample->no, priv->aes_key, (guchar *)stored_data);
				// 将数据库中的值反序列化
				compare_print = fp_print_deserialize (stored_data, stored_len, &error);
				// 指纹比较是否相等
				if (fp_print_equal (match, compare_print)) {
					free (temp);
					temp = NULL;
					stored_data = NULL;
					identifyData->uid = info_list->uid;
				}
				sample = sample->next;
				free (temp);
				temp = NULL;
				stored_data = NULL;
				free (template_data);
			}
			info_list = info_list->next;
		}
		// 回收特征数据项
		bio_sto_free_feature_info_list (info_list);

		snprintf (priv->extra_info, EXTRA_INFO_LENGTH, ("_identify fingerprint template successful, its uid is %d"), identifyData->uid);
		bio_set_ops_abs_result (identifyData->dev, OPS_IDENTIFY_MATCH);
		bio_set_notify_abs_mid (identifyData->dev, NOTIFY_IDENTIFY_MATCH);
		bio_set_notify_abs_mid (identifyData->dev, MID_EXTENDED_MESSAGE);

		bio_print_info ("%s\n", bio_get_notify_mid_mesg (identifyData->dev));
	} else {
		snprintf (priv->extra_info, EXTRA_INFO_LENGTH, "_identify fingerprint template fail");

		bio_set_ops_abs_result (identifyData->dev, OPS_IDENTIFY_NO_MATCH);
		bio_set_notify_abs_mid (identifyData->dev, NOTIFY_IDENTIFY_NO_MATCH);
		bio_set_notify_abs_mid (identifyData->dev, MID_EXTENDED_MESSAGE);

		bio_print_info ("%s\n", bio_get_notify_mid_mesg (identifyData->dev));
	}
	return;
}
