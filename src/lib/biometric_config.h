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

#ifndef CONFIG_H
#define CONFIG_H

#include <glib.h>
#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define _STR(s)	#s
#define DEF2STR(s)	_STR(s)

#ifdef BIO_CONFIG_DIR
	#define CONFIG_DIR DEF2STR(BIO_CONFIG_DIR)
#else
	#define CONFIG_DIR "/etc/biometric-auth"
#endif

#define DRIVERS_CONFIG_FILE	CONFIG_DIR "/biometric-drivers.conf"

/*
 * 初始化配置（仅由框架内部调用）
 *   加载配置文件，并初始化全局配置结构体
 *
 * @return	返回初始化结果
 * @retval	0	成功
 *			其他	失败
 */
int bio_conf_init();

/*
 * 释放配置相关资源（仅由框架内部调用）
 *
 * @return	无
 */
void bio_conf_free();

/*
 * 获取配置文件
 *
 * @return	返回配置文件结构体
 */
GKeyFile * get_bio_conf();

/* 全局配置文件声明 */
extern GKeyFile * bioconf;

#ifdef  __cplusplus
}
#endif

#endif // CONFIG_H
