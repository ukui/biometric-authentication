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

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <libgen.h>
#include <libfprint/fprint.h>

#include <community_define.h>
#include "community_multidevice_discover_tool.h"

device_info device_info_list[COMMUNITY_MULTIDEVICE_MAX_ID];

bool has_prefix(char *str, char *prefix)
{
	char *p1 = NULL;
	char *p2 = NULL;

	if (str == NULL || prefix == NULL)
		return false;

	p1 = str;
	p2 = prefix;

	while ((*p1 != '\0') && (*p2 != '\0') && (*p1 == *p2))
	{
		p1++;
		p2++;
	}

	if (*p2 == '\0')
		return true;
	else
		return false;
}

bool is_device_exist(char * device_name)
{
	int i = 0;

	for (i  = 0; i < COMMUNITY_MULTIDEVICE_MAX_ID; i++)
	{
		if (strcmp(device_name, device_info_list[i].drv_name) == 0)
			return device_info_list[i].exist;
	}

	return false;
}

void device_info_list_init()
{
	int i;
	for (i = 0; i < COMMUNITY_MULTIDEVICE_MAX_ID; i++)
	{
		device_info_list[i].drv_id = i;
		device_info_list[i].exist = false;
	}

	device_info_list[0].drv_name = "";
	device_info_list[UPEKTS_ID].drv_name = UPEKTS_NAME;
	device_info_list[URU4000_ID].drv_name = URU4000_NAME;
	device_info_list[AES4000_ID].drv_name = AES4000_NAME;
	device_info_list[AES2501_ID].drv_name = AES2501_NAME;
	device_info_list[UPEKTC_ID].drv_name = UPEKTC_NAME;
	device_info_list[AES1610_ID].drv_name = AES1610_NAME;
	device_info_list[FDU2000_ID].drv_name = FDU2000_NAME;
	device_info_list[VCOM5S_ID].drv_name = VCOM5S_NAME;
	device_info_list[UPEKSONLY_ID].drv_name = UPEKSONLY_NAME;
	device_info_list[VFS101_ID].drv_name = VFS101_NAME;
	device_info_list[VFS301_ID].drv_name = VFS301_NAME;
	device_info_list[AES2550_ID].drv_name = AES2550_NAME;
	device_info_list[UPEKE2_ID].drv_name = UPEKE2_NAME;
	device_info_list[AES1660_ID].drv_name = AES1660_NAME;
	device_info_list[AES2660_ID].drv_name = AES2660_NAME;
	device_info_list[AES3500_ID].drv_name = AES3500_NAME;
	device_info_list[UPEKTC_IMG_ID].drv_name = UPEKTC_IMG_NAME;
	device_info_list[ETES603_ID].drv_name = ETES603_NAME;
	device_info_list[VFS5011_ID].drv_name = VFS5011_NAME;
	device_info_list[VFS0050_ID].drv_name = VFS0050_NAME;
	device_info_list[ELAN_ID].drv_name = ELAN_NAME;
}

int main(int argc, char **argv)
{
	struct fp_dscv_dev **discovered_devs = NULL;
	struct fp_driver *drv = NULL;

	char * base_name = NULL;
	char * drv_name = NULL;
	int r = 1;
	int i = 0;
	int drv_id = 0;

	base_name = basename(argv[0]);

	device_info_list_init();

	r = fp_init();
	if (r < 0) {
		fprintf(stderr, _("Failed to initialize libfprint\n"));
		return 0;
	}

	discovered_devs = fp_discover_devs();
	for (i = 0; discovered_devs[i] != NULL; i++)
	{
		drv = fp_dscv_dev_get_driver(discovered_devs[i]);
		drv_id = fp_driver_get_driver_id(drv);
		device_info_list[drv_id].exist = true;
	}

	r = 0;
	if (strcmp(base_name, PROGAM_NAME) != 0)
	{
		if (has_prefix(base_name, "usb-"))
			drv_name = base_name + strlen("usb-");
		else
			drv_name = base_name;

		if (is_device_exist(drv_name))
		{
			printf("%s\n", drv_name);
			r++;
		}

		goto FP_EXIT;
	}
	else
	{
		if (argc == 1)
		{
			for (i  = 0; i < COMMUNITY_MULTIDEVICE_MAX_ID; i++)
				if (device_info_list[i].exist)
				{
					printf("%s\n", device_info_list[i].drv_name);
					r++;
				}

			goto FP_EXIT;
		}

		for (i = 1; i < argc; i++)
			if (is_device_exist(argv[i]))
			{
				printf("%s\n", argv[i]);
				r++;
			}
	}

FP_EXIT:
	fp_exit();

	return r;
}
