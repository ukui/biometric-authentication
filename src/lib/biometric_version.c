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

#include "biometric_version.h"

/**
 * @brief	获取驱动使用的DRV_API版本
 * 获取驱动使用的DRV_API版本
 * @param[in]	int major	主版本号
 * @param[in]	int minor	从版本号
 * @param[in]	int function	功能版本号
 * @return	无
 */
void bio_get_drv_api_version_by_driver_side(bio_dev * dev,
											int * major,
											int * minor,
											int * function)
{
	*major = dev->drv_api_version.major;
	*minor = dev->drv_api_version.minor;
	*function = dev->drv_api_version.function;
}

/**
 * @brief	获取框架使用的DRV_API版本
 * 获取框架使用的DRV_API版本
 * @param[in]	int major	主版本号
 * @param[in]	int minor	从版本号
 * @param[in]	int function	功能版本号
 * @return	无
 */
void bio_get_drv_api_version_by_framework_side(int * major,
											   int * minor,
											   int * function)
{
	*major = BIO_DRV_API_VERSION_MAJOR;
	*minor = BIO_DRV_API_VERSION_MINOR;
	*function = BIO_DRV_API_VERSION_FUNC;
}

/**
 * @brief	获取框架使用的APP_API版本
 * 获取框架使用的APP_API版本
 * @param[in]	int major	主版本号
 * @param[in]	int minor	从版本号
 * @param[in]	int function	功能版本号
 * @return	无
 */
void bio_get_app_api_version_by_framework_side(int * major,
											   int * minor,
											   int * function)
{
	*major = BIO_APP_API_VERSION_MAJOR;
	*minor = BIO_APP_API_VERSION_MINOR;
	*function = BIO_APP_API_VERSION_FUNC;
}

/**
 * @brief	驱动API兼容性检测
 * 对比驱动使用的API版本和当前框架版本
 * @param[in]	int major	主版本号
 * @param[in]	int minor	从版本号
 * @param[in]	int function	功能版本号
 * @return		int	检测结果
 * @retval		=0	驱动使用的API版本兼容框架版本（主、从版本号匹配，功能版本号小于等于框架版本）
 *				>0	驱动使用的API版本不兼容框架版本，驱动过新
 *				<0	驱动使用的API版本不兼容框架版本，驱动过旧
 */
int bio_check_drv_api_version(int major, int minor, int function)
{
	if (major > BIO_DRV_API_VERSION_MAJOR)
		return 3;
	else if (major < BIO_DRV_API_VERSION_MAJOR)
		return -3;

	if (minor > BIO_DRV_API_VERSION_MINOR)
		return 2;
	else if (minor < BIO_DRV_API_VERSION_MINOR)
		return -2;

	if (function > BIO_DRV_API_VERSION_FUNC)
		return 1;
	else if (function < BIO_DRV_API_VERSION_FUNC)
		return 0;

	return 0;
}

/**
 * @brief	应用API兼容性检测
 * 对比应用使用的API版本和当前框架版本
 * @param[in]	int major	主版本号
 * @param[in]	int minor	从版本号
 * @param[in]	int function	功能版本号
 * @return		int	检测结果
 * @retval		=0	应用使用的API版本兼容框架版本（主、从版本号匹配，功能版本号小于等于框架版本）
 *				>0	应用使用的API版本不兼容框架版本，应用过新
 *				<0	应用使用的API版本不兼容框架版本，应用过旧
 */
int bio_check_app_api_version(int major, int minor, int function)
{
	if (major > BIO_APP_API_VERSION_MAJOR)
		return 3;
	else if (major < BIO_APP_API_VERSION_MAJOR)
		return -3;

	if (minor > BIO_APP_API_VERSION_MINOR)
		return 2;
	else if (minor < BIO_APP_API_VERSION_MINOR)
		return -2;

	if (function > BIO_APP_API_VERSION_FUNC)
		return 1;
	else if (function < BIO_APP_API_VERSION_FUNC)
		return 0;

	return 0;
}


