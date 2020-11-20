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

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dlfcn.h>
#include <libusb.h>
#include <syslog.h>

#include "biometric_intl.h"
#include "biometric_common.h"
#include "biometric_config.h"
#include "biometric_version.h"

/* 定义全局变量 */
static int bio_ops_timeout_ms = BIO_OPS_DEFAULT_TIMEOUT * 1000;	/** 操作的超时时间，单位：毫秒 **/
GList * bio_drv_list = NULL;
GList * bio_dev_list = NULL;
BIO_STATUS_CHANGED_CALLBACK bio_status_changed_callback = NULL;
BIO_USB_DEVICE_HOT_PLUG_CALLBACK bio_usb_device_hot_plug_callback = NULL;

/********** 声明内部函数 **********/
bio_dev * bio_dev_new();
void bio_dev_free(bio_dev * dev);
void* get_plugin_ops(void *handle, char *func_name);
static int bio_usb_hotplug_callback_attach(libusb_context *ctx,
										   libusb_device *usbdev,
										   libusb_hotplug_event event,
										   void *user_data);
static int bio_usb_hotplug_callback_detach(libusb_context *ctx,
										   libusb_device *usbdev,
										   libusb_hotplug_event event,
										   void *user_data);
feature_info * bio_common_get_feature_list(bio_dev *dev, OpsActions action,
										   int uid, int idx_start, int idx_end);
int bio_comm_feature_rename_by_db(bio_dev *dev, int uid, int idx, char * new_name);
void bio_status_changed_common_callback(int drvid, int type);
void bio_usb_device_hot_plug_common_callbak(int drvid, int action, int dev_num_now);
void bio_dev_glist_data_free(bio_dev * dev, gpointer p2);

/********** 外部函数实现 **********/
char * bio_new_string(char * old_str){
	char * str = NULL;

	if (old_str == NULL) {
		str = malloc(1);
		if (str != NULL)
			*str = '\0';
	} else {
		int l = strlen(old_str) + 1;
		str = malloc(l);
		if (str != NULL)
			strncpy(str, old_str, l);
	}

	return str;
}

int bio_base64_encode(unsigned char * bin_data, char * base64, int bin_len )
{
	int i, j;
	unsigned char current;
	const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	for ( i = 0, j = 0 ; i < bin_len ; i += 3 )
	{
		current = (bin_data[i] >> 2) ;
		current &= (unsigned char)0x3F;
		base64[j++] = base64char[(int)current];

		current = ( (unsigned char)(bin_data[i] << 4 ) ) & ( (unsigned char)0x30 ) ;
		if ( i + 1 >= bin_len )
		{
			base64[j++] = base64char[(int)current];
			base64[j++] = '=';
			base64[j++] = '=';
			break;
		}
		current |= ( (unsigned char)(bin_data[i+1] >> 4) ) & ( (unsigned char) 0x0F );
		base64[j++] = base64char[(int)current];

		current = ( (unsigned char)(bin_data[i+1] << 2) ) & ( (unsigned char)0x3C ) ;
		if ( i + 2 >= bin_len )
		{
			base64[j++] = base64char[(int)current];
			base64[j++] = '=';
			break;
		}
		current |= ( (unsigned char)(bin_data[i+2] >> 6) ) & ( (unsigned char) 0x03 );
		base64[j++] = base64char[(int)current];

		current = ( (unsigned char)bin_data[i+2] ) & ( (unsigned char)0x3F ) ;
		base64[j++] = base64char[(int)current];
	}
	base64[j] = '\0';
	return strlen(base64);
}

int bio_base64_decode(char * base64, unsigned char * bin_data)
{
	int i, j;
	unsigned char k;
	unsigned char temp[4];
	const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	for ( i = 0, j = 0; base64[i] != '\0' ; i += 4 )
	{
		memset( temp, 0xFF, sizeof(temp) );
		for ( k = 0 ; k < 64 ; k ++ )
		{
			if ( base64char[k] == base64[i] )
				temp[0]= k;
		}
		for ( k = 0 ; k < 64 ; k ++ )
		{
			if ( base64char[k] == base64[i+1] )
				temp[1]= k;
		}
		for ( k = 0 ; k < 64 ; k ++ )
		{
			if ( base64char[k] == base64[i+2] )
				temp[2]= k;
		}
		for ( k = 0 ; k < 64 ; k ++ )
		{
			if ( base64char[k] == base64[i+3] )
				temp[3]= k;
		}

		bin_data[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
				((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
		if ( base64[i+2] == '=' )
			break;

		bin_data[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
				((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
		if ( base64[i+3] == '=' )
			break;

		bin_data[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
				((unsigned char)(temp[3]&0x3F));
	}
	return j;
}

void bio_print_drv_list(int print_level)
{
	GList *l = bio_drv_list;
	GString *print_list = g_string_new(_("Current driver list:"));

	while (l != NULL) {
		bio_dev * dev = l->data;
		if (dev->enable)
			g_string_sprintfa(print_list, "%s[T] -> " , dev->device_name);
		else
			g_string_sprintfa(print_list, "%s[F] -> " , dev->device_name);
		l = l->next;
	}
	g_string_append(print_list, "NULL");

	switch (print_level)
	{
	case BIO_PRINT_LEVEL_ERROR:
		bio_print_error("%s\n", print_list->str);
		break;
	case BIO_PRINT_LEVEL_WARN:
		bio_print_warning("%s\n", print_list->str);
		break;
	case BIO_PRINT_LEVEL_NOTICE:
		bio_print_notice("%s\n", print_list->str);
		break;
	case BIO_PRINT_LEVEL_INFO:
		bio_print_info("%s\n", print_list->str);
		break;
	case BIO_PRINT_LEVEL_DEBUG:
	default:
		bio_print_debug("%s\n", print_list->str);
		break;
	}

	g_string_free(print_list, TRUE);
}

void bio_print_dev_list(int print_level)
{
	GList *l = bio_dev_list;
	GString *print_list = g_string_new(_("Current device list:"));

	while (l != NULL) {
		bio_dev * dev = l->data;
		if (dev->enable)
			g_string_sprintfa(print_list, "%s[%d] -> " ,
							  dev->device_name, dev->dev_num);
		l = l->next;
	}
	g_string_append(print_list, "NULL");

	switch (print_level)
	{
	case BIO_PRINT_LEVEL_ERROR:
		bio_print_error("%s\n", print_list->str);
		break;
	case BIO_PRINT_LEVEL_WARN:
		bio_print_warning("%s\n", print_list->str);
		break;
	case BIO_PRINT_LEVEL_NOTICE:
		bio_print_notice("%s\n", print_list->str);
		break;
	case BIO_PRINT_LEVEL_INFO:
		bio_print_info("%s\n", print_list->str);
		break;
	case BIO_PRINT_LEVEL_DEBUG:
	default:
		bio_print_debug("%s\n", print_list->str);
		break;
	}

	g_string_free(print_list, TRUE);
}

GList * bio_get_drv_list() {
	return bio_drv_list;
}

GList * bio_get_dev_list() {
	return bio_dev_list;
}

void bio_free_drv_list() {
	bio_free_driver(bio_drv_list);
}

void bio_free_dev_list() {
	bio_free_device(bio_dev_list);
}

int bio_dev_is_enable(bio_dev * dev, GKeyFile * conf)
{
	GError * err = NULL;
	if (dev->device_name == NULL)
		return FALSE;

	gboolean enable = g_key_file_get_boolean(conf, dev->device_name,
											   "Enable", &err);
	if (err != NULL) {
		bio_print_error(_("Error[%d]: %s\n"), err->code, err->message);
		g_error_free(err);
		err = NULL;
	} else bio_print_debug("IsEnable = %s\n", (enable)?"true":"false");
	return enable;
}

int bio_dev_set_serial_path(bio_dev * dev, GKeyFile * conf)
{
	char * path = NULL;
	GError * err = NULL;

	memset(dev->serial_info.path, 0, MAX_PATH_LEN);
	if (dev->device_name == NULL)
		return -1;

	path = g_key_file_get_string(conf, dev->device_name, "Path", &err);
	if (err != NULL) {
		bio_print_error(_("Error[%d]: %s\n"), err->code, err->message);
		g_error_free(err);
		free(path);
		return -1;
	} else bio_print_debug("Serial Path = %s\n", path);

	if (path[0] == '\0') {
		bio_print_notice("Serial Path is Enpty!\n");
		free(path);
		return -1;
	}

	sprintf(dev->serial_info.path, "%s", path);
	free(path);

	return 0;
}

GList * bio_driver_list_init(void) {
	GKeyFile * conf = NULL;
	gchar **  drv_list_conf = NULL;
	gsize driver_count = 0;
	bio_dev * p = NULL;

	conf = get_bio_conf();
	if (conf == NULL) {
		bio_print_error(_("can't find \"bioconf\" struct, "
			   "maybe someone forget use \"bio_conf_init()\" before\n"));
		return NULL;
	}
	 drv_list_conf = g_key_file_get_groups(conf, &driver_count);

	int i = 0;
	GError *err = NULL;
	for (i = 0; i < driver_count; i++) {
		if ( drv_list_conf[i][0] != '\0') {
			p = bio_dev_new();
			if (p == NULL) {
				bio_print_error(_("Unable to allocate memory\n"));
				return NULL;
			}

			/* 加载配置文件中指定的驱动文件 */
			gchar *path = g_key_file_get_string(conf,  drv_list_conf[i],
												"Driver", &err);
			if (err != NULL) {
				bio_print_error(_("Error[%d]: %s\n"), err->code, err->message);
				g_error_free(err);
				err = NULL;
				bio_free_driver(bio_drv_list);
				return NULL;
			}
			if (path[0] == '\0') {
				bio_print_error(_("No define driver in [%s]\n"),  drv_list_conf[i]);
				bio_free_driver(bio_drv_list);
				return NULL;
			}

			dlerror();
			p->plugin_handle = dlopen(path, RTLD_NOW);
			if (p->plugin_handle == NULL) {
				bio_print_error("%s\n", dlerror());
				dlerror();
				continue;
			}

			bio_print_debug(_("Loaded Driver: %s\n"), path);

			/* 填充用于描述驱动与设备的bio_dev结构体 */
			p->ops_configure = get_plugin_ops(p->plugin_handle, "ops_configure");

			/* 执行驱动的参数处理函数 */
			int r = p->ops_configure(p, conf);
			if (r != 0)
			{
				bio_print_error(_("%s driver configure failed\n"),
								p->device_name);
				dlclose(p->plugin_handle);
				free(p);
				continue;
			}

			/* 检测驱动版本兼容性 */
			bio_print_debug(_("%s driver DRV_API version: %d.%d.%d\n"),
							p->device_name,
							p->drv_api_version.major,
							p->drv_api_version.minor,
							p->drv_api_version.function);
			int version_check = 0;
			version_check = bio_check_drv_api_version(p->drv_api_version.major,
													  p->drv_api_version.minor,
													  p->drv_api_version.function);
			if (version_check != 0)
			{
				if (version_check > 0)
					bio_print_error(_("Detected Compatibility issues: %s driver "
									  "version is higher than biometric framework\n"),
									p->device_name);
				else
					bio_print_error(_("Detected Compatibility issues: %s driver "
									  "version is lower than biometric framework\n"),
									p->device_name);
				dlclose(p->plugin_handle);
				free(p);
				continue;
			}

			/* 执行驱动初始化函数，失败则释放资源，进行下一个循环 */
			if (p->ops_driver_init(p) < 0)
			{
				bio_print_warning(_("Driver %s initialization failed\n"),
								  p->device_name);
				p->ops_free(p);
				if (p->plugin_handle != NULL) {
					dlclose(p->plugin_handle);
					p->plugin_handle = NULL;
				}
				free(p);
				continue;
			}

			bio_set_dev_status(p, DEVS_COMM_IDLE);
			bio_set_ops_result(p, OPS_COMM_SUCCESS);
			bio_set_notify_mid(p, NOTIFY_COMM_IDLE);
			p->enable = bio_dev_is_enable(p, conf);

			bio_drv_list = g_list_append(bio_drv_list, p);

			/* 注册USB热插拔时的回掉函数 */
			if (p->bioinfo.bustype == BusT_USB) {
				int rc = 0;
				int i;
				const struct usb_id * id_table = p->usb_info.id_table;
				for (i = 0; id_table[i].idVendor != 0; i++)
				{
					rc = libusb_hotplug_register_callback(NULL,
														  LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
														  0,
														  id_table[i].idVendor,
														  id_table[i].idProduct,
														  LIBUSB_HOTPLUG_MATCH_ANY,
														  bio_usb_hotplug_callback_attach,
														  NULL,
														  &(p->usb_info.callback_handle)[0]);
					if (LIBUSB_SUCCESS != rc) {
						bio_print_error (_("Error: Can not register attach callback error\n"));
					}

					rc = libusb_hotplug_register_callback (NULL,
														   LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
														   0,
														   id_table[i].idVendor,
														   id_table[i].idProduct,
														   LIBUSB_HOTPLUG_MATCH_ANY,
														   bio_usb_hotplug_callback_detach,
														   NULL,
														   &(p->usb_info.callback_handle)[1]);
					if (LIBUSB_SUCCESS != rc) {
						bio_print_error (_("Error: Can not register detach callback error\n"));
					}
				}
			}
		}
	}

	return bio_drv_list;
}

/**
 * @brief	发现设备，构建设备链表
 * 搜索系统中支持生物识别的设备
 * @return	GList*	GList链表，data指针指向bio_dev结构体
 * @note	设备链表的data指针与驱动链表的data指针指向同一个bio_dev结构体，设备链表只
 *			记录当前有驱动并且已连接的设备
 * @par	修改日志
 *			JianglinXuan于2017-05-12创建
 */
GList *bio_device_list_init(GList * drv_list)
{
	GList * dev_list = NULL;
	GList * l = drv_list;

	/* 构建设备链表，不重新建立bio_dev结构体，直接引用驱动链表中的值 */
	while (l != NULL)
	{
		GList * next = l->next;
		bio_dev * dev = l->data;
		if (dev->enable)
		{
			dev->dev_num = dev->ops_discover(dev);
			if (dev->dev_num > 0)
			{
				dev_list = g_list_append(dev_list, dev);
			}
		}
		l = next;
	}

	bio_dev_list = dev_list;

	return bio_dev_list;
}

int bio_init(void) {
	int ret = 0;

	setlocale (LC_ALL, "");
	bindtextdomain(LIBBIOMETRIC_DOMAIN_NAME, LOCALEDIR);

	bio_print_debug(_("Biometric framework API version:\n"));
	bio_print_debug(_("         Driver API(DRV_API): %d.%d.%d\n"),
					BIO_DRV_API_VERSION_MAJOR,
					BIO_DRV_API_VERSION_MINOR,
					BIO_DRV_API_VERSION_FUNC);
	bio_print_debug(_("    Application API(APP_API): %d.%d.%d\n"),
					BIO_APP_API_VERSION_MAJOR,
					BIO_APP_API_VERSION_MINOR,
					BIO_APP_API_VERSION_FUNC);

	/* 初始化配置 */
	if (bio_conf_init() != 0) {
		bio_print_error(_("bio_conf_init failed\n"));
		return -1;
	}

	/* 检测并升级数据库 */
	sqlite3 *db = bio_sto_connect_db();
	ret = bio_sto_check_and_upgrade_db_format(db);
	if (ret != 0 )
	{
		bio_sto_disconnect_db(db);
		return -1;
	}

	int rc;
	rc = libusb_init (NULL);
	if (rc < 0)	{
		bio_print_error(_("failed to initialise libusb: %s\n"), libusb_error_name(rc));
		return -1;
	}

	return 0;
}

int bio_close(void) {
	libusb_exit (NULL);

	bio_conf_free();

	return 0;
}

void bio_free_driver(GList * drv_list) {
	g_list_foreach(drv_list, (GFunc)bio_dev_glist_data_free, NULL);
	g_list_free(drv_list);
	bio_drv_list = NULL;
}

void bio_free_device(GList * dev_list) {
	g_list_free(dev_list);
	bio_dev_list = NULL;
}

/*
 * 特征捕获
 *   捕获特征信息并保存在给定的buf中
 *
 * @param[in]	dev		设备结构体
 * @param[out]	buff	特征捕获后的存放的buf
 * @return		操作结果
 */
char * bio_ops_capture(bio_dev *dev)
{
	int ret;
	char * caps = NULL;

	if (dev->enable == FALSE)
	{
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		bio_set_ops_result(dev, OPS_COMM_ERROR);
		bio_set_notify_mid(dev, NOTIFY_COMM_DISABLE);
		return NULL;
	}

	ret = dev->ops_open(dev);
	bio_print_debug(_("Open Result: %d\n"), ret);

	if (ret == 0)
	{
		if (dev->ops_capture != NULL)
			caps = bio_new_string(dev->ops_capture(dev, ACTION_START));
		else
		{
			caps = NULL;
			bio_set_ops_result(dev, OPS_COMM_ERROR);
			bio_set_notify_abs_mid(dev, NOTIFY_COMM_UNSUPPORTED_OPS);
			bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
		}
	}

	dev->ops_close(dev);
	bio_print_debug(_("Close Device: %s\n"), dev->device_name);
	bio_print_debug(_("Device Status: [%d]%s\n"), bio_get_ops_result(dev), bio_get_ops_result_mesg(dev));

	if (caps == NULL)
		caps = bio_new_string("");

	return caps;
}

/*
 * 特征录入
 *   录入生物特征
 *
 * @param[in]	dev		设备结构体
 * @param[in]	index	录入索引，小于0则自动插入到最小的空白值中
 * @return		操作结果
 */
int bio_ops_enroll(bio_dev *dev, int uid, int idx, char * idx_name)
{
	int ret = -1;

	if (dev->enable == FALSE)
	{
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		bio_set_ops_result(dev, OPS_COMM_ERROR);
		bio_set_notify_mid(dev, NOTIFY_COMM_DISABLE);
		return -1;
	}

	ret = dev->ops_open(dev);
	bio_print_debug(_("Open Result: %d\n"), ret);

	if (ret == 0)
	{
		if (dev->ops_enroll != NULL)
			ret = dev->ops_enroll(dev, ACTION_START, uid, idx, (char *)idx_name);
		else
		{
			ret = -1;
			bio_set_ops_result(dev, OPS_COMM_ERROR);
			bio_set_notify_abs_mid(dev, NOTIFY_COMM_UNSUPPORTED_OPS);
			bio_print_debug(_("%s\n"), bio_get_notify_mid_mesg(dev));
		}
	}

	dev->ops_close(dev);
	bio_print_debug(_("Close Device: %s\n"), dev->device_name);
	bio_print_debug(_("Device Status: [%d]%s\n"), bio_get_ops_result(dev), bio_get_ops_result_mesg(dev));
	bio_print_debug("---------\n");

	return ret;
}

/*
 * 特征验证
 *   与给定的索引值的特征对比验证
 *
 * @param[in]	dev		设备结构体
 * @param[in]	index	给定需要对比的索引值
 * @return		操作结果
 */
int bio_ops_verify(bio_dev *dev, int uid, int idx)
{
	int ret = -1;

	if (dev->enable == FALSE)
	{
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		bio_set_ops_result(dev, OPS_COMM_ERROR);
		bio_set_notify_mid(dev, NOTIFY_COMM_DISABLE);
		return -1;
	}

	ret = dev->ops_open(dev);
	bio_print_debug(_("Open Result: %d\n"), ret);

	if (ret == 0)
	{
		if (dev->ops_verify != NULL)
			ret = dev->ops_verify(dev, ACTION_START, uid, idx);
		else
		{
			ret = -1;
			bio_set_ops_result(dev, OPS_COMM_ERROR);
			bio_set_notify_abs_mid(dev, NOTIFY_COMM_UNSUPPORTED_OPS);
			bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
		}
	}

	dev->ops_close(dev);
	bio_print_debug(_("Close Device: %s\n"), dev->device_name);
	bio_print_debug(_("Device Status: [%d]%s\n"), bio_get_ops_result(dev), bio_get_ops_result_mesg(dev));
	bio_print_debug("---------\n");

	return ret;
}

/*
 * 特征识别
 *   在给定范围内识别特征信息（全闭区间[start, end]）
 *
 * @param[in]	dev		设备结构体
 * @param[in]	start	指定开始的索引
 * @param[in]	end		指定结束的索引，-1代表识别到最后
 * @return		操作结果
 */
int bio_ops_identify(bio_dev *dev, int uid, int idx_start, int idx_end)
{
	int ret = -1;
	int uid_ret = -1;

	if (dev->enable == FALSE)
	{
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		bio_set_ops_result(dev, OPS_COMM_ERROR);
		bio_set_notify_mid(dev, NOTIFY_COMM_DISABLE);
		return -1;
	}

	ret = dev->ops_open(dev);
	bio_print_debug(_("Open Result: %d\n"), ret);

	if (ret == 0)
	{
		if (dev->ops_identify != NULL)
			uid_ret = dev->ops_identify(dev, ACTION_START, uid, idx_start, idx_end);
		else
		{
			ret = -1;
			bio_set_ops_result(dev, OPS_COMM_ERROR);
			bio_set_notify_abs_mid(dev, NOTIFY_COMM_UNSUPPORTED_OPS);
			bio_print_debug(_("%s\n"), bio_get_notify_mid_mesg(dev));
		}
	}

	dev->ops_close(dev);
	bio_print_debug(_("Close Device: %s\n"), dev->device_name);
	bio_print_debug(_("Device Status: [%d]%s\n"), bio_get_ops_result(dev), bio_get_ops_result_mesg(dev));
	bio_print_debug("---------\n");

	return uid_ret;
}

/*
 * 特征搜索
 *   在给定范围内搜索特征信息（全闭区间[start, end]）
 *
 * @param[in]	dev		设备结构体
 * @param[in]	start	指定开始的索引
 * @param[in]	end		指定结束的索引，-1代表搜索到最后
 * @param[out]	index	记录搜索成功后的索引，-1代表未搜索到
 * @return		搜索到的特征列表
 * @note	函数执行过程中出错返回NULL，由ops_result来判断是“未搜索到”还是“搜索出错”
 */
feature_info * bio_ops_search(bio_dev *dev, int uid, int idx_start, int idx_end)
{
	int ret;
	feature_info * found = NULL;

	if (dev->enable == FALSE)
	{
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		bio_set_ops_result(dev, OPS_COMM_ERROR);
		bio_set_notify_mid(dev, NOTIFY_COMM_DISABLE);
		return NULL;
	}

	ret = dev->ops_open(dev);
	bio_print_debug(_("Open Result: %d\n"), ret);

	if (ret == 0)
	{
		if (dev->ops_search != NULL)
			found = dev->ops_search(dev, ACTION_START, uid, idx_start, idx_end);
		else
		{
			ret = -1;
			bio_set_ops_result(dev, OPS_COMM_ERROR);
			bio_set_notify_abs_mid(dev, NOTIFY_COMM_UNSUPPORTED_OPS);
			bio_print_debug(_("%s\n"), bio_get_notify_mid_mesg(dev));
		}
	}

	dev->ops_close(dev);
	bio_print_debug(_("Close Device: %s\n"), dev->device_name);
	bio_print_debug(_("Device Status: [%d]%s\n"), bio_get_ops_result(dev), bio_get_ops_result_mesg(dev));

	return found;
}

/*
 * 特征清理
 *   清理给定范围的特征信息（全闭区间[start, end]）
 *
 * @param[in]	dev		设备结构体
 * @param[in]	start	指定开始的索引
 * @param[in]	end		指定结束的索引，-1代表清理到最后
 * @return	操作结果
 */
int bio_ops_clean(bio_dev *dev, int uid, int idx_start, int idx_end)
{
	int ret = -1;

	if (dev->enable == FALSE)
	{
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		bio_set_ops_result(dev, OPS_COMM_ERROR);
		bio_set_notify_mid(dev, NOTIFY_COMM_DISABLE);
		return -1;
	}

	ret = dev->ops_open(dev);
	bio_print_debug(_("Open Result: %d\n"), ret);

	if (ret == 0)
	{
		if (dev->ops_clean != NULL)
			ret = dev->ops_clean(dev, ACTION_START, uid, idx_start, idx_end);
		else
		{
			ret = -1;
			bio_set_ops_result(dev, OPS_COMM_ERROR);
			bio_set_notify_abs_mid(dev, NOTIFY_COMM_UNSUPPORTED_OPS);
			bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
		}
	}

	dev->ops_close(dev);
	bio_print_debug(_("Close Device: %s\n"), dev->device_name);
	bio_print_debug(_("Device Status: [%d]%s\n"), bio_get_ops_result(dev), bio_get_ops_result_mesg(dev));
	bio_print_debug("---------\n");

	return ret;
}

/*
 * 获取特征列表
 *   获取用户指定范围的特征列表
 *
 * @param[in]	dev			设备结构体
 * @param[in]	uid			指定用户的ID
 * @param[in]	idx_start	指定索引的起始位置
 * @param[in]	idx_end		指定索引的结束位置
 * @return		搜索到的特征列表
 */
feature_info * bio_ops_get_feature_list(bio_dev *dev, int uid,
										int idx_start, int idx_end)
{
	int ret = -1;
	feature_info * flist = NULL;

	if (dev->enable == FALSE)
	{
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		bio_set_ops_result(dev, OPS_COMM_ERROR);
		bio_set_notify_mid(dev, NOTIFY_COMM_DISABLE);
		return NULL;
	}

	if (dev->ops_get_feature_list != NULL)
	{
		ret = dev->ops_open(dev);
		bio_print_debug(_("Open Result: %d\n"), ret);

		if (ret == 0)
			flist = dev->ops_get_feature_list(dev, ACTION_START, uid, idx_start, idx_end);

		dev->ops_close(dev);
	} else
		flist = bio_common_get_feature_list(dev, ACTION_START, uid, idx_start, idx_end);

	bio_print_debug(_("Close Device: %s\n"), dev->device_name);
	bio_print_debug(_("Device Status: [%d]%s\n"), bio_get_ops_result(dev), bio_get_ops_result_mesg(dev));
	bio_print_debug("---------\n");

	return flist;
}

/*
 * 重命名特征名称
 *
 * @param[in]	bio_dev *dev	设备结构体
 * @param[in]	int uid			需更新特征的UID
 * @param[in]	int idx			需更新特征的索引
 * @param[int]	char *new_name	特征的新名称
 * @return		操作结果
 * @retval		0	重命名成功
 *				>0	重命名失败的数量
 */
int bio_ops_feature_rename(bio_dev *dev, int uid, int idx, char * new_name)
{
	int ret = -1;

	if (dev->ops_feature_rename != NULL)
	{
		if (dev->enable == FALSE)
		{
			bio_set_dev_status(dev, DEVS_COMM_DISABLE);
			bio_set_ops_result(dev, OPS_COMM_ERROR);
			bio_set_notify_mid(dev, NOTIFY_COMM_DISABLE);
			return -1;
		}

		ret = dev->ops_open(dev);
		bio_print_debug(_("Open Result: %d\n"), ret);

		if (ret == 0)
			dev->ops_feature_rename(dev, uid, idx, new_name);

		dev->ops_close(dev);
		return ret;
	}
	else
	{
		int ret = 0;
		bio_set_dev_abs_status(dev, DEVS_RENAME_DOING);

		ret = bio_comm_feature_rename_by_db(dev, uid, idx, new_name);

		if (ret == 0)
			bio_set_all_abs_status(dev, DEVS_COMM_IDLE, OPS_RENAME_SUCCESS,
								   NOTIFY_RENAME_SUCCESS);
		else
			bio_set_all_abs_status(dev, DEVS_COMM_IDLE, OPS_RENAME_FAIL,
								   NOTIFY_RENAME_INCOMPLETE);
		return ret;
	}
}

int bio_ops_stop_ops(bio_dev *dev, int waiting_sec){
	return dev->ops_stop_by_user(dev, waiting_sec * 1000);
}

void bio_set_ops_timeout_ms(int t) {
	bio_ops_timeout_ms = t;
}

int bio_get_ops_timeout_ms() {
	return bio_ops_timeout_ms;
}

bool bio_is_stop_by_user(bio_dev * dev) {
	if (dev->dev_status % 100 == DEVS_COMM_STOP_BY_USER)
		return true;
	else
		return false;
}

int bio_get_dev_status(bio_dev * dev) {
	return dev->dev_status;
}

void bio_set_dev_status(bio_dev * dev, int status) {
	dev->dev_status = status;

	if ((dev->ops_status_changed_callback != NULL) &&
			(dev->ops_status_changed_callback != bio_status_changed_callback))
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_DEVICE);
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_DEVICE);
}

void bio_set_dev_abs_status(bio_dev * dev, int status) {
	dev->dev_status = status;

	if ((dev->ops_status_changed_callback != NULL) &&
			(dev->ops_status_changed_callback != bio_status_changed_callback))
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_DEVICE);
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_DEVICE);
}

int bio_get_ops_result(bio_dev * dev) {
	return dev->ops_result;
}

void bio_set_ops_result(bio_dev * dev, int result) {
	int ops = 0;
	ops = bio_get_dev_status(dev) / 100;
	result  = result % 100;
	dev->ops_result = ops * 100 + result;

	if ((dev->ops_status_changed_callback != NULL) &&
			(dev->ops_status_changed_callback != bio_status_changed_callback))
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_OPERATION);
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_OPERATION);
}

void bio_set_ops_abs_result(bio_dev * dev, int result) {
	dev->ops_result = result;

	if ((dev->ops_status_changed_callback != NULL) &&
			(dev->ops_status_changed_callback != bio_status_changed_callback))
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_OPERATION);
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_OPERATION);
}

int bio_get_notify_mid(bio_dev * dev) {
	return dev->notify_mid;
}

void bio_set_notify_mid(bio_dev * dev, int mid) {
	int ops = 0;
	ops = bio_get_dev_status(dev) / 100;
	mid  = mid % 100;
	dev->notify_mid = ops * 100 + mid;

	if ((dev->ops_status_changed_callback != NULL) &&
			(dev->ops_status_changed_callback != bio_status_changed_callback))
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_NOTIFY);
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_NOTIFY);
}

void bio_set_notify_abs_mid(bio_dev * dev, int mid) {
	dev->notify_mid = mid;

	if ((dev->ops_status_changed_callback != NULL) &&
			(dev->ops_status_changed_callback != bio_status_changed_callback))
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_NOTIFY);
    bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_NOTIFY);
}

void bio_set_all_status(bio_dev * dev,
							int dev_status,
							int ops_result,
							int notify_mid)
{
	int type = bio_get_dev_status(dev) / 100;
	dev->dev_status = type * 100 + (dev_status % 100);
	dev->ops_result = type * 100 + (ops_result % 100);
	dev->notify_mid = type * 100 + (notify_mid % 100);

	if ((dev->ops_status_changed_callback != NULL) &&
			(dev->ops_status_changed_callback != bio_status_changed_callback))
	{
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_DEVICE);
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_OPERATION);
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_NOTIFY);
	}
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_DEVICE);
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_OPERATION);
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_NOTIFY);
}

void bio_set_all_abs_status(bio_dev * dev,
							int dev_status,
							int ops_result,
							int notify_mid)
{
	dev->dev_status = dev_status;
	dev->ops_result = ops_result;
	dev->notify_mid = notify_mid;

	if ((dev->ops_status_changed_callback != NULL) &&
			(dev->ops_status_changed_callback != bio_status_changed_callback))
	{
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_DEVICE);
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_OPERATION);
		dev->ops_status_changed_callback(dev->driver_id, STATUS_TYPE_NOTIFY);
	}
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_DEVICE);
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_OPERATION);
	bio_status_changed_common_callback(dev->driver_id, STATUS_TYPE_NOTIFY);
}

const char * bio_get_dev_status_mesg(bio_dev *dev){
	const char * mesg = NULL;
	mesg = dev->ops_get_dev_status_mesg(dev);
	if (mesg != NULL)
		return mesg;
	switch (bio_get_dev_status(dev)) {
	case DEVS_COMM_IDLE:
		return _("Device idle");
	case DEVS_COMM_DOING:
		return _("Device is in process");
	case DEVS_COMM_STOP_BY_USER:
		return _("Terminating current operation");
	case DEVS_COMM_DISABLE:
		return _("Device is disabled");

	case DEVS_OPEN_DOING:
		return _("Opening device");
	case DEVS_OPEN_STOP_BY_USER:
		return _("Terminating open operation");

	case DEVS_ENROLL_DOING:
		return _("Enrolling");
	case DEVS_ENROLL_STOP_BY_USER:
		return _("Terminating enroll operation");

	case DEVS_VERIFY_DOING:
		return _("Verifying");
	case DEVS_VERIFY_STOP_BY_USER:
		return _("Terminating Verify operation");

	case DEVS_IDENTIFY_DOING:
		return _("Identifying");
	case DEVS_IDENTIFY_STOP_BY_USER:
		return _("Terminating identify operation");

	case DEVS_CAPTURE_DOING:
		return _("Capturing");
	case DEVS_CAPTURE_STOP_BY_USER:
		return _("Terminating capture operation");

	case DEVS_SEARCH_DOING:
		return _("Searching");
	case DEVS_SEARCH_STOP_BY_USER:
		return _("Terminating search operation");

	case DEVS_CLEAN_DOING:
		return _("Cleaning");
	case DEVS_CLEAN_STOP_BY_USER:
		return _("Terminating clean operation");

	case DEVS_GET_FLIST_DOING:
		return _("Getting feature list");
	case DEVS_GET_FLIST_STOP_BY_USER:
		return _("Terminating get feature list operation");

	case DEVS_RENAME_DOING:
		return _("Renaming feature");
	case DEVS_RENAME_STOP_BY_USER:
		return _("Terminating feature rename operation");

	case DEVS_CLOSE_DOING:
		return _("Closing");
	case DEVS_CLOSE_STOP_BY_USER:
		return _("Terminating close operation");

	default:
		bio_print_warning(_("Device unknown status code: %d\n"), bio_get_dev_status(dev));
		return _("Device is in an unknown status...");
	}
}

const char * bio_get_ops_result_mesg(bio_dev *dev){
	const char * mesg = NULL;
	mesg = dev->ops_get_ops_result_mesg(dev);
	if (mesg != NULL)
		return mesg;

	switch (bio_get_ops_result(dev)) {
	case OPS_COMM_SUCCESS:
		return _("Operation successful");
	case OPS_COMM_FAIL:
		return _("Operation failed");
	case OPS_COMM_ERROR:
		return _("An error was encountered during the operation");
	case OPS_COMM_STOP_BY_USER:
		return _("The operation was interrupted by the user");
	case OPS_COMM_TIMEOUT:
		return _("Operation timeout");
	case OPS_COMM_OUT_OF_MEM:
		return _("Out of memory");

	case OPS_OPEN_SUCCESS:
		return _("Open device completion");
	case OPS_OPEN_FAIL:
		return _("Open device failed");
	case OPS_OPEN_ERROR:
		return _("An error was encountered during the opening");

	case OPS_ENROLL_SUCCESS:
		return _("Enroll feature successful");
	case OPS_ENROLL_FAIL:
		return _("Enroll feature failed");
	case OPS_ENROLL_ERROR:
		return _("An error was encountered during enrolling");
	case OPS_ENROLL_STOP_BY_USER:
		return _("Enroll operation was interrupted by the user");
	case OPS_ENROLL_TIMEOUT:
		return _("Enroll operation timeout");

	case OPS_VERIFY_MATCH:
		return _("Verify feature match");
	case OPS_VERIFY_NO_MATCH:
		return _("Verify feature no match");
	case OPS_VERIFY_ERROR:
		return _("An error was encountered during verifying");
	case OPS_VERIFY_STOP_BY_USER:
		return _("Verify operation was interrupted by the user");
	case OPS_VERIFY_TIMEOUT:
		return _("Verify operation timeout");

	case OPS_IDENTIFY_MATCH:
		return _("Identify feature match");
	case OPS_IDENTIFY_NO_MATCH:
		return _("Identify feature no match");
	case OPS_IDENTIFY_ERROR:
		return _("An error was encountered during identifying");
	case OPS_IDENTIFY_STOP_BY_USER:
		return _("Identify operation was interrupted by the user");
	case OPS_IDENTIFY_TIMEOUT:
		return _("Identify operation timeout");

	case OPS_CAPTURE_SUCCESS:
		return _("Capture feature successful");
	case OPS_CAPTURE_FAIL:
		return _("Capture feature failed");
	case OPS_CAPTURE_ERROR:
		return _("An error was encountered during capturing");
	case OPS_CAPTURE_STOP_BY_USER:
		return _("Capture operation was interrupted by the user");
	case OPS_CAPTURE_TIMEOUT:
		return _("Capture operation timeout");

	case OPS_SEARCH_MATCH:
		return _("Find the specified features");
	case OPS_SEARCH_NO_MATCH:
		return _("No specified features were found");
	case OPS_SEARCH_ERROR:
		return _("An error was encountered during searching");
	case OPS_SEARCH_STOP_BY_USER:
		return _("Search operation was interrupted by the user");
	case OPS_SEARCH_TIMEOUT:
		return _("Search operation timeout");

	case OPS_CLEAN_SUCCESS:
		return _("Clean feature successful");
	case OPS_CLEAN_FAIL:
		return _("Clean feature failed");
	case OPS_CLEAN_ERROR:
		return _("An error was encountered during cleaning");
	case OPS_CLEAN_STOP_BY_USER:
		return _("Clean operation was interrupted by the user");
	case OPS_CLEAN_TIMEOUT:
		return _("Clean operation timeout");

	case OPS_GET_FLIST_SUCCESS:
		return _("Get feature list completion");
	case OPS_GET_FLIST_FAIL:
		return _("Get feature list failed");
	case OPS_GET_FLIST_ERROR:
		return _("An error was encountered during getting feature list");

	case OPS_RENAME_SUCCESS:
		return _("Rename feature successful");
	case OPS_RENAME_FAIL:
		return _("Rename feature failed");
	case OPS_RENAME_ERROR:
		return _("An error was encountered during renaming");

	case OPS_CLOSE_SUCCESS:
		return _("Close device completion");
	case OPS_CLOSE_FAIL:
		return _("Close device failed");
	case OPS_CLOSE_ERROR:
		return _("An error was encountered during closing");

	default:
		return _("Operation is in an unknown status......");
	}
}

const char * bio_get_notify_mid_mesg(bio_dev *dev){
	const char * mesg = NULL;
	mesg = dev->ops_get_notify_mid_mesg(dev);
	if (mesg != NULL)
		return mesg;

	switch (bio_get_notify_mid(dev)) {
	case NOTIFY_COMM_IDLE:
		return _("Device idle");
	case NOTIFY_COMM_FAIL:
		return _("Operation failed");
	case NOTIFY_COMM_ERROR:
		return _("An error was encountered during the operation");
	case NOTIFY_COMM_STOP_BY_USER:
		return _("The operation was interrupted by the user");
	case NOTIFY_COMM_TIMEOUT:
		return _("Operation timeout");
	case NOTIFY_COMM_DISABLE:
		return _("Device is disable");
	case NOTIFY_COMM_OUT_OF_MEM:
		return _("Out of memory");
	case NOTIFY_COMM_UNSUPPORTED_OPS:
		return _("The device or driver does not support this operation");

	case NOTIFY_OPEN_SUCCESS:
		return _("Open device completion");
	case NOTIFY_OPEN_FAIL:
		return _("Open device failed");
	case NOTIFY_OPEN_ERROR:
		return _("An error was encountered during the opening");

	case NOTIFY_ENROLL_SUCCESS:
		return _("Enroll feature successful");
	case NOTIFY_ENROLL_FAIL:
		return _("Enroll feature failed");
	case NOTIFY_ENROLL_ERROR:
		return _("An error was encountered during enrolling");
	case NOTIFY_ENROLL_STOP_BY_USER:
		return _("Enroll operation was interrupted by the user");
	case NOTIFY_ENROLL_TIMEOUT:
		return _("Enroll operation timeout");

	case NOTIFY_VERIFY_MATCH:
		return _("Verify feature match");
	case NOTIFY_VERIFY_NO_MATCH:
		return _("Verify feature no match");
	case NOTIFY_VERIFY_ERROR:
		return _("An error was encountered during verifying");
	case NOTIFY_VERIFY_STOP_BY_USER:
		return _("Verify operation was interrupted by the user");
	case NOTIFY_VERIFY_TIMEOUT:
		return _("Verify operation timeout");

	case NOTIFY_IDENTIFY_MATCH:
		return _("Identify feature match");
	case NOTIFY_IDENTIFY_NO_MATCH:
		return _("Identify feature no match");
	case NOTIFY_IDENTIFY_ERROR:
		return _("An error was encountered during identifying");
	case NOTIFY_IDENTIFY_STOP_BY_USER:
		return _("Identify operation was interrupted by the user");
	case NOTIFY_IDENTIFY_TIMEOUT:
		return _("Identify operation timeout");

	case NOTIFY_CAPTURE_SUCCESS:
		return _("Capture feature successful");
	case NOTIFY_CAPTURE_FAIL:
		return _("Capture feature failed");
	case NOTIFY_CAPTURE_ERROR:
		return _("An error was encountered during capturing");
	case NOTIFY_CAPTURE_STOP_BY_USER:
		return _("Capture operation was interrupted by the user");
	case NOTIFY_CAPTURE_TIMEOUT:
		return _("Capture operation timeout");

	case NOTIFY_SEARCH_MATCH:
		return _("Find the specified features");
	case NOTIFY_SEARCH_NO_MATCH:
		return _("No specified features were found");
	case NOTIFY_SEARCH_ERROR:
		return _("An error was encountered during searching");
	case NOTIFY_SEARCH_STOP_BY_USER:
		return _("Search operation was interrupted by the user");
	case NOTIFY_SEARCH_TIMEOUT:
		return _("Search operation timeout");

	case NOTIFY_CLEAN_SUCCESS:
		return _("Clean feature successful");
	case NOTIFY_CLEAN_FAIL:
		return _("Clean feature failed");
	case NOTIFY_CLEAN_ERROR:
		return _("An error was encountered during cleaning");
	case NOTIFY_CLEAN_STOP_BY_USER:
		return _("Clean operation was interrupted by the user");
	case NOTIFY_CLEAN_TIMEOUT:
		return _("Clean operation timeout");

	case NOTIFY_GET_FLIST_SUCCESS:
		return _("Get feature list completion");
	case NOTIFY_GET_FLIST_FAIL:
		return _("Get feature list failed");
	case NOTIFY_GET_FLIST_ERROR:
		return _("An error was encountered during getting feature list");

	case NOTIFY_RENAME_SUCCESS:
		return _("Rename feature successful");
	case NOTIFY_RENAME_FAIL:
		return _("Rename feature failed");
	case NOTIFY_RENAME_ERROR:
		return _("An error was encountered during renaming");
	case NOTIFY_RENAME_INCOMPLETE:
		return _("Feature renaming is not complete, please try again");

	case NOTIFY_CLOSE_SUCCESS:
		return _("Close device completion");
	case NOTIFY_CLOSE_FAIL:
		return _("Close device failed");
	case NOTIFY_CLOSE_ERROR:
		return _("An error was encountered during closing");

	default:
		return _("Device internal error");
	}
}

int bio_common_get_empty_index(bio_dev *dev, int uid, int start, int end)
{
	sqlite3 *db = bio_sto_connect_db();
	feature_info * info, * info_list;
	int cmp = 0;
	int empty_index = 0;
	int current_index = -1;

	if ((end != -1) && (end < start))
		end = start;

	info_list = bio_sto_get_feature_info(db, uid, dev->bioinfo.biotype,
												 dev->device_name, start,
												 end);
	bio_sto_disconnect_db(db);

	info = info_list;
	empty_index = start;

	while (info != NULL)
	{
		current_index = info->index;
		cmp = current_index - empty_index;

		if (cmp == 0)
			empty_index++;

		if (cmp > 0)
			break;

		info = info->next;
	}
	bio_sto_free_feature_info_list(info_list);

	/* 找到空闲值，返回 */
	if (cmp > 0)
		return empty_index;

	/* 未找到空闲值，且不限大小的情况下，返回值为：列表中最大索引值+1 */
	if (end == -1)
		return current_index + 1;

	/* 其他情况代表这未找到，返回-1 */
	return -1;
}

int bio_common_get_empty_sample_no(bio_dev *dev, int start, int end)
{
	sqlite3 *db = bio_sto_connect_db();
	feature_info * info, * info_list;
	unsigned char * sample_flag = NULL;
	int i = 0;
	int empty_sample_no = -1;

	if (end < start)
		end = start;

	sample_flag = malloc(sizeof(unsigned char) * (end + 1));

	for (i = 0; i <= end; i++) {
		if (i >= start)
			sample_flag[i] = 0;
		else
			sample_flag[i] = 1;
	}

	info_list = bio_sto_get_feature_info(db, -1, dev->bioinfo.biotype,
												 dev->device_name, 0,
												 -1);
	bio_sto_disconnect_db(db);

	info = info_list;
	while (info != NULL) {
		feature_sample * sample = info->sample;
		while (sample != NULL) {
			sample_flag[sample->no] = 1;
			sample = sample->next;
		}
		info = info->next;
	}
	bio_sto_free_feature_info_list(info_list);

	i = start;
	while ((i <= end) && (empty_sample_no == -1))
	{
		if (sample_flag[i] == 0)
			empty_sample_no = i;
		i++;
	}

	free(sample_flag);
	return empty_sample_no;
}

/*
 * 检测usb设备，返回检测到的数量
 */
int bio_common_detect_usb_device(bio_dev *dev)
{
	libusb_device ** usb_devs_list = NULL;			// 指向设备指针的指针，用于检索设备列表
	struct libusb_device_descriptor usb_dev_desc= {0};
	libusb_context *ctx = NULL;			// libusb session
	ssize_t cnt;						// 列表中的设备数量
	int ret = 0;
	int idx = 0;
	int num = 0;

	ret = libusb_init(&ctx); //	初始化刚刚声明的会话库
	if(ret < 0) {
		bio_print_error(_("Init libusb Error\n"));
		return -1;
	}
	libusb_set_debug(ctx, LIBUSB_LOG_LEVEL_WARNING);

	cnt = libusb_get_device_list(ctx, &usb_devs_list);	// 获取设备列表
	if(cnt < 0) {
		bio_print_error(_("Get USB Device List Error\n"));
		return -1;
	}

	for (idx = 0; idx < cnt; idx++)
	{
		memset(&usb_dev_desc, 0, sizeof(struct libusb_device_descriptor));
		if(libusb_get_device_descriptor(usb_devs_list[idx], &usb_dev_desc) != 0)
		{
			bio_print_error(_("Can not get usb information\n"));
			return num;
		}

		int i;
		const struct usb_id * id_table = dev->usb_info.id_table;
		for (i = 0; id_table[i].idVendor != 0; i++)
		{
			if (id_table[i].idVendor == usb_dev_desc.idVendor &&
					id_table[i].idProduct == usb_dev_desc.idProduct)
			{
				num++;
				break;
			}
		}
	}

	bio_print_debug(_("libbiometric detected usb device(%s): %d\n"),
					dev->device_name, num);
	libusb_exit(ctx);

	return num;
}

int bio_get_empty_driver_id()
{
	int i = 0;
	uint8_t drv_id_list[BIO_DRVID_DYNAMIC_MAX - BIO_DRVID_DYNAMIC_START + 1] = {0};
	GList * l = bio_get_drv_list();

	while (l != NULL) {
		bio_dev * dev = l->data;
		if (dev->driver_id >= BIO_DRVID_DYNAMIC_START)
			drv_id_list[dev->driver_id - BIO_DRVID_DYNAMIC_START] = 1;

		l = l->next;
	}

	for (i = 0; i < BIO_DRVID_DYNAMIC_MAX - BIO_DRVID_DYNAMIC_START + 1; i++)
	{
		if (drv_id_list[i] != 1)
			break;
	}

	if (drv_id_list[i] != 1)
		return i + BIO_DRVID_DYNAMIC_START;
	else
		return -1;
}

int bio_vprint_by_level(int print_level, FILE * __restrict stream,
						const char * __restrict format, va_list args)
{
	int ret = 0;
	int level_int = 0;
	char * level_char = getenv(BIO_PRINT_LEVEL_ENV);

	if (level_char == NULL)
	{
		level_int = BIO_PRINT_LEVEL_DEFAULT;
	} else {
		level_int = atoi(level_char);
	}

	if (print_level <= level_int)
	{
		char * color_char = getenv(BIO_PRINT_LEVEL_COLOR);
		int color_int = 0;

		if (color_char != NULL)
			color_int = atoi(color_char);

		if (color_int == 1)
		{
			switch (print_level)
			{
			case BIO_PRINT_LEVEL_EMERG:
			case BIO_PRINT_LEVEL_ALERT:
			case BIO_PRINT_LEVEL_CRIT:
			case BIO_PRINT_LEVEL_ERROR:
				fprintf(stderr, "%s", BIO_PRINT_COLOR_RED);
				break;
			case BIO_PRINT_LEVEL_WARN:
				fprintf(stdout, "%s", BIO_PRINT_COLOR_PURPLE);
				break;
			case BIO_PRINT_LEVEL_NOTICE:
				fprintf(stdout, "%s", BIO_PRINT_COLOR_BLUE);
				break;
			case BIO_PRINT_LEVEL_INFO:
				fprintf(stdout, "%s", BIO_PRINT_COLOR_CYAN);
				break;
			case BIO_PRINT_LEVEL_DEBUG:
				fprintf(stdout, "%s", BIO_PRINT_COLOR_YELLOW);
				break;
			default:
				fprintf(stdout, "%s", BIO_PRINT_COLOR_WHITE);
			}
		}

		switch (print_level)
		{
		case BIO_PRINT_LEVEL_EMERG:
			fprintf(stderr, "[EMERG] ");
			break;
		case BIO_PRINT_LEVEL_ALERT:
			fprintf(stderr, "[ALERT] ");
			break;
		case BIO_PRINT_LEVEL_CRIT:
			fprintf(stderr, "[ CRIT] ");
			break;
		case BIO_PRINT_LEVEL_ERROR:
			fprintf(stderr, "[ERROR] ");
			break;
		case BIO_PRINT_LEVEL_WARN:
			fprintf(stdout, "[ WARN] ");
			break;
		case BIO_PRINT_LEVEL_NOTICE:
			fprintf(stdout, "[ NOTE] ");
			break;
		case BIO_PRINT_LEVEL_INFO:
			fprintf(stdout, "[ INFO] ");
			break;
		case BIO_PRINT_LEVEL_DEBUG:
			fprintf(stdout, "[DEBUG] ");
			break;
		default:
			fprintf(stdout, "[UKNOW] ");
		}

		ret = vfprintf(stream, format, args);

		if (color_int == 1)
		{
			if (print_level <= BIO_PRINT_LEVEL_ERROR && print_level >= 0)
				fprintf(stderr, "%s", BIO_PRINT_COLOR_END);
			else
				fprintf(stdout, "%s", BIO_PRINT_COLOR_END);
		}
	}

	fflush(stream);

	return ret;
}

int bio_print_by_level(int print_level, FILE * stream, char * format, ...)
{
	int ret = 0;
	va_list args;

	va_start(args, format);
	ret = bio_vprint_by_level(print_level, stream, format, args);
	va_end(args);

	return ret;
}

int bio_print_debug(const char * format, ...)
{
	int ret = 0;
	va_list args;

	va_start(args, format);
	ret = bio_vprint_by_level(BIO_PRINT_LEVEL_DEBUG, stdout, format, args);
	va_end(args);

	return ret;
}

int bio_print_info(const char * format, ...)
{
	int ret = 0;
	va_list args;

	va_start(args, format);
	ret = bio_vprint_by_level(BIO_PRINT_LEVEL_INFO, stdout, format, args);
	va_end(args);

	return ret;
}

int bio_print_notice(const char *format, ...)
{
	int ret = 0;
	va_list args;

	va_start(args, format);
	ret = bio_vprint_by_level(BIO_PRINT_LEVEL_NOTICE, stdout, format, args);
	va_end(args);

	return ret;
}

int bio_print_warning(const char * format, ...)
{
	int ret = 0;
	va_list args;

	va_start(args, format);
	ret = bio_vprint_by_level(BIO_PRINT_LEVEL_WARN, stdout, format, args);
	va_end(args);

	return ret;
}

int bio_print_error(const char * format, ...)
{
	int ret = 0;
	va_list args;

	va_start(args, format);
	ret = bio_vprint_by_level(BIO_PRINT_LEVEL_ERROR, stderr, format, args);
	va_end(args);

	return ret;
}

/********** 内部函数实现 **********/
/* 分配一个bio_dev资源 */
bio_dev * bio_dev_new()
{
	bio_dev * dev;
	dev = malloc(sizeof(bio_dev));
	if (dev != NULL)
		memset(dev, 0, sizeof(bio_dev));
	return dev;
}

/* 释放bio_dev资源 */
void bio_dev_free(bio_dev * dev)
{
	dev->ops_free(dev);
	if (dev->plugin_handle != NULL) {
		dlclose(dev->plugin_handle);
		dev->plugin_handle = NULL;
	}

	free(dev);
}

/*
 * 获取插件中的函数句柄
 *
 * @param[in]	handle		插件句柄
 * @param[in]	func_name	需获取的函数名
 * @return		函数句柄地址
 * @retval		非NULL	获取成功
 *				NULL	获取失败
 */
void* get_plugin_ops(void *handle, char *func_name)
{
	char* err = NULL;
	void* func = NULL;

	func = dlsym(handle, func_name);
	err = dlerror();
	if(err != NULL)
	{
		bio_print_error(_("Get function %s handle error: %s\n"), func_name, err);
		return NULL;
	}
	return func;
}

/* USB设备插入时的回调函数 */
static int bio_usb_hotplug_callback_attach(libusb_context *ctx,
										   libusb_device *usbdev,
										   libusb_hotplug_event event,
										   void *user_data)
{
	struct libusb_device_descriptor desc;
	int rc;

	rc = libusb_get_device_descriptor(usbdev, &desc);
	if (LIBUSB_SUCCESS != rc) {
		bio_print_error(_("Error: Can not get device descriptor\n"));
	}

	bio_print_info(_("Device attached: %04x:%04x\n"), desc.idVendor, desc.idProduct);

	GList * l = bio_drv_list;
	bio_dev * biodev = NULL;

	while (l != NULL)
	{
		bio_dev * dev = l->data;

		if (dev->enable == 0 || dev->bioinfo.bustype != BusT_USB ||
				dev->usb_info.id_table == NULL)
		{
			l = l->next;
			continue;
		}

		int i;
		const struct usb_id * id_table = dev->usb_info.id_table;

		for (i = 0; id_table[i].idVendor != 0; i++)
		{
			if (id_table[i].idVendor == desc.idVendor &&
					id_table[i].idProduct == desc.idProduct)
			{
				biodev = dev;
				biodev->dev_num = biodev->ops_discover(biodev);
				bio_usb_device_hot_plug_common_callbak(dev->driver_id,
													   USB_HOT_PLUG_ATTACHED,
													   biodev->dev_num);
				break;
			}
		}

		if (biodev != NULL)
			break;

		l = l->next;
	}

	if ((biodev != NULL) && (biodev->dev_num > 0)) {
		bio_dev_list = g_list_append(bio_dev_list, biodev);
		biodev->ops_attach(biodev);
	}

	bio_print_drv_list(BIO_PRINT_LEVEL_DEBUG);
	bio_print_dev_list(BIO_PRINT_LEVEL_DEBUG);

	return 0;
}

/* USB设备拔出时的回调函数 */
static int bio_usb_hotplug_callback_detach(libusb_context *ctx,
										   libusb_device *usbdev,
										   libusb_hotplug_event event,
										   void *user_data)
{
	struct libusb_device_descriptor desc;
	int rc;

	rc = libusb_get_device_descriptor(usbdev, &desc);
	if (LIBUSB_SUCCESS != rc) {
		bio_print_error(_("Error getting device descriptor"));
	}

	bio_print_info(_("Device detached: %04x:%04x\n"), desc.idVendor, desc.idProduct);

	GList * l = bio_drv_list;
	bio_dev * biodev = NULL;

	while (l != NULL)
	{
		bio_dev * dev = l->data;

		if (dev->enable == 0 || dev->bioinfo.bustype != BusT_USB ||
				dev->usb_info.id_table == NULL)
		{
			l = l->next;
			continue;
		}

		int i;
		const struct usb_id * id_table = dev->usb_info.id_table;

		for (i = 0; id_table[i].idVendor != 0; i++)
		{
			if (id_table[i].idVendor == desc.idVendor &&
					id_table[i].idProduct == desc.idProduct)
			{
				biodev = dev;
				biodev->dev_num = biodev->ops_discover(biodev);
				bio_usb_device_hot_plug_common_callbak(dev->driver_id,
													   USB_HOT_PLUG_DETACHED,
													   biodev->dev_num);
				break;
			}
		}

		if (biodev != NULL)
			break;

		l = l->next;
	}

	if (biodev != NULL) {
		bio_print_debug("Dev No. = %d\n", g_list_index(bio_dev_list, biodev));
		if (biodev->dev_num <= 0)
			bio_dev_list = g_list_remove(bio_dev_list, biodev);
		biodev->ops_detach(biodev);
	}

	bio_print_drv_list(BIO_PRINT_LEVEL_DEBUG);
	bio_print_dev_list(BIO_PRINT_LEVEL_DEBUG);

	return 0;
}

/* 框架缺省的获取特征列表函数 */
feature_info * bio_common_get_feature_list(bio_dev *dev, OpsActions action,
										   int uid, int idx_start, int idx_end)
{
	if (dev->enable == FALSE) {
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return NULL;
	}

	if (action == ACTION_START)
	{
		feature_info * finfo_list = NULL;
		sqlite3 *db = bio_sto_connect_db();

		bio_set_dev_status(dev, DEVS_GET_FLIST_DOING);

		finfo_list = bio_sto_get_feature_info(db, uid, dev->bioinfo.biotype,
											  dev->device_name, idx_start,
											  idx_end);
		print_feature_info(finfo_list);
		bio_sto_disconnect_db(db);

		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		bio_set_ops_abs_result(dev, OPS_GET_FLIST_SUCCESS);
		bio_set_notify_abs_mid(dev, NOTIFY_GET_FLIST_SUCCESS);
		return finfo_list;
	} else {
		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		bio_set_ops_abs_result(dev, OPS_GET_FLIST_STOP_BY_USER);
		bio_set_notify_abs_mid(dev, NOTIFY_GET_FLIST_STOP_BY_USER);
		return NULL;
	}
}

/* 框架缺省的特征重命名函数 */
int bio_comm_feature_rename_by_db(bio_dev *dev, int uid, int idx, char * new_name)
{
	feature_info * info_list = NULL;
	feature_info * info = NULL;
	sqlite3 *db = bio_sto_connect_db();
	int count = 0;
	int update_num = 0;

	info_list = bio_sto_get_feature_info(db, uid, dev->bioinfo.biotype,
										 dev->device_name, idx, idx);

	info = info_list;
	while (info != NULL) {
		if (info->uid == uid && info->index == idx)
		{
			feature_sample *sample = info->sample;
			while (sample != NULL)
			{
				int ret = 0;
				ret = bio_sto_update_feature_info_by_dbid(db,
														  sample->dbid,
														  uid,
														  dev->bioinfo.biotype,
														  dev->device_name,
														  idx,
														  new_name,
														  sample->no);
				if (ret == 0)
					update_num++;
				count++;
				sample = sample->next;
			}
		}
		info = info->next;
	}

	bio_sto_disconnect_db(db);

	if (count == 0)
		bio_print_warning(_("Unable to find feature that require renaming\n"));

	if (update_num != count)
		bio_print_warning(_("There are %d feature samples to renaming failed, "
							"please try again\n"), count - update_num);

	return (count - update_num);
}

/* 状态变更的回调函数 */
void bio_status_changed_common_callback(int drvid, int type)
{
	if (bio_status_changed_callback != NULL)
		bio_status_changed_callback(drvid, type);
}

/* 给上层服务提供的USB热插拔事件的回调函数接口 */
void bio_usb_device_hot_plug_common_callbak(int drvid, int action, int dev_num_now)
{
	if (bio_usb_device_hot_plug_callback != NULL)
		bio_usb_device_hot_plug_callback(drvid, action, dev_num_now);
}

/* 释放单个bio_dev结构体 */
void bio_dev_glist_data_free(bio_dev * dev, gpointer p2)
{
	bio_dev_free(dev);
}



