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
#include <stdio.h>

#include "biometric_common.h"
#include "biometric_intl.h"
#include "biometric_config.h"

/* 全局配置文件 */
GKeyFile * bioconf = NULL;

int bio_conf_init()
{
	bioconf = g_key_file_new();
	GError *err = NULL;

	g_key_file_load_from_file(bioconf, DRIVERS_CONFIG_FILE, G_KEY_FILE_KEEP_COMMENTS, &err);
	if (err != NULL) {
		bio_print_error(_("Error[%d]: %s\n"), err->code, err->message);
		g_error_free(err);
		err = NULL;
		return -1;
	}

	return 0;
}

void bio_conf_free()
{
	g_key_file_free(bioconf);
}

GKeyFile * get_bio_conf()
{
	return bioconf;
}

