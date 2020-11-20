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


#include "community_multidevice_discover_tool.h"

device_info device_info_list[COMMUNITY_MULTIDEVICE_MAX_ID];

bool has_prefix (char *str, char *prefix)
{
	char *p1 = NULL;
	char *p2 = NULL;

	if (str == NULL || prefix == NULL)
		return false;

	p1 = str;
	p2 = prefix;

	while ((*p1 != '\0') && (*p2 != '\0') && (*p1 == *p2)) {
		p1++;
		p2++;
	}

	if (*p2 == '\0')
		return true;
	else
		return false;
}

bool is_device_exist (char *device_name)
{
	int i = 0;

	for (i  = 0; i < COMMUNITY_MULTIDEVICE_MAX_ID; i++) {
		if (strcmp(device_name, device_info_list[i].drv_id) == 0)
			return device_info_list[i].exist;
	}

	return false;
}

void device_info_list_init()
{
	int i;
	for (i = 0; i < COMMUNITY_MULTIDEVICE_MAX_ID; i++) {
		device_info_list[i].exist = false;
	}

	device_info_list[0].drv_id = "";
	device_info_list[UPEKTS_ID].drv_id = UPEKTS_NAME;
	device_info_list[URU4000_ID].drv_id = URU4000_NAME;
	device_info_list[AES4000_ID].drv_id = AES4000_NAME;
	device_info_list[AES2501_ID].drv_id = AES2501_NAME;
	device_info_list[UPEKTC_ID].drv_id = UPEKTC_NAME;
	device_info_list[AES1610_ID].drv_id = AES1610_NAME;
	device_info_list[FDU2000_ID].drv_id = FDU2000_NAME;
	device_info_list[VCOM5S_ID].drv_id = VCOM5S_NAME;
	device_info_list[UPEKSONLY_ID].drv_id = UPEKSONLY_NAME;
	device_info_list[VFS101_ID].drv_id = VFS101_NAME;
	device_info_list[VFS301_ID].drv_id = VFS301_NAME;
	device_info_list[AES2550_ID].drv_id = AES2550_NAME;
	device_info_list[AES1660_ID].drv_id = AES1660_NAME;
	device_info_list[AES2660_ID].drv_id = AES2660_NAME;
	device_info_list[AES3500_ID].drv_id = AES3500_NAME;
	device_info_list[UPEKTC_IMG_ID].drv_id = UPEKTC_IMG_NAME;
	device_info_list[ETES603_ID].drv_id = ETES603_NAME;
	device_info_list[VFS5011_ID].drv_id = VFS5011_NAME;
	device_info_list[VFS0050_ID].drv_id = VFS0050_NAME;
	device_info_list[ELAN_ID].drv_id = ELAN_NAME;
}

int main(int argc, char **argv)
{
	driver_info *drv_info = malloc(sizeof(driver_info));
	drv_info->ctx = NULL;
	drv_info->devices = NULL;
	drv_info->device = NULL;

	char *base_name = NULL;
	char *drv_name = NULL;
	int ret = -1;
	int i = 0;
	int j = 0;

	base_name = basename(argv[0]);

	// 初始化
	device_info_list_init();

	// 创建新的FpContext
	drv_info->ctx = fp_context_new ();
	// 获取设备集
	drv_info->devices = fp_context_get_devices (drv_info->ctx);
	if (!(drv_info->devices)) {
		printf ("Impossible to get devices");
		return ret;
	}
	// 检测到的设备，将exist设置为true
	for (i = 0; i < drv_info->devices->len; i++) {
		// 从设备集中通过索引选择设备
		drv_info->device = g_ptr_array_index (drv_info->devices, i);
		// 获取驱动ID
		const char *drv_id = fp_device_get_driver (drv_info->device);
		for(j = 0; j < COMMUNITY_MULTIDEVICE_MAX_ID; j++) {
			if(strcmp(drv_id, device_info_list[j].drv_id) == 0)
				device_info_list[j].exist = true;
		}
	}

	// 输入参数不是：community-multidevice-discover-tool
	if (strcmp(base_name, PROGAM_NAME) != 0) {
		// 参数是：usb-
		if (has_prefix(base_name, "usb-"))
			drv_name = base_name + strlen("usb-");
		else
			drv_name = base_name;

		// 判断设备是否存在
		if (is_device_exist(drv_name)) {
			printf("%s\n", drv_name);
			ret++;
		}

		return ret;
	}else {	// 输入参数是：community-multidevice-discover-tool
		// 只输入一个参数
		if (argc == 1) {
			for (i  = 0; i < COMMUNITY_MULTIDEVICE_MAX_ID; i++)
				if (device_info_list[i].exist) {
					printf("%s\n", device_info_list[i].drv_id);
					ret++;
				}

			return ret;
		}
		// 输入多个参数
		for (i = 1; i < argc; i++)
			if (is_device_exist(argv[i])) {
				printf("%s\n", argv[i]);
				ret++;
			}
	}

	return ret;
}
