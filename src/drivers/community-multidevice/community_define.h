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
#ifndef COMMUNITY_DEFINE_H
#define COMMUNITY_DEFINE_H

#include <libintl.h>

enum {
	UPEKTS_ID	= 1,
	URU4000_ID	= 2,
	AES4000_ID	= 3,
	AES2501_ID	= 4,
	UPEKTC_ID	= 5,
	AES1610_ID	= 6,
	FDU2000_ID	= 7,
	VCOM5S_ID	= 8,
	UPEKSONLY_ID= 9,
	VFS101_ID	= 10,
	VFS301_ID	= 11,
	AES2550_ID	= 12,
	AES1660_ID	= 13,
	AES2660_ID	= 14,
	AES3500_ID	= 15,
	UPEKTC_IMG_ID= 16,
	ETES603_ID	= 17,
	VFS5011_ID	= 18,
	VFS0050_ID	= 19,
	ELAN_ID	= 20,
	COMMUNITY_MULTIDEVICE_MAX_ID,
};

#define UPEKTS_NAME	"upekts"
#define URU4000_NAME	"uru4000"
#define AES4000_NAME	"aes4000"
#define AES2501_NAME	"aes2501"
#define UPEKTC_NAME	"upektc"
#define AES1610_NAME	"aes1610"
#define FDU2000_NAME	"fdu2000"
#define VCOM5S_NAME	"vcom5s"
#define UPEKSONLY_NAME	"upeksonly"
#define VFS101_NAME	"vfs101"
#define VFS301_NAME	"vfs301"
#define AES2550_NAME	"aes2550"
#define AES1660_NAME	"aes1660"
#define AES2660_NAME	"aes2660"
#define AES3500_NAME	"aes3500"
#define UPEKTC_IMG_NAME	"upektc_img"
#define ETES603_NAME	"etes603"
#define VFS5011_NAME	"vfs5011"
#define VFS0050_NAME	"vfs0050"
#define ELAN_NAME	"elan"

#ifndef BIOMETRIC_DRIVER_COMMUNITY_MULTIDEVICE_DOMAIN_NAME
	#define BIOMETRIC_DRIVER_COMMUNITY_MULTIDEVICE_DOMAIN_NAME \
		"biometric-driver-community-multidevice"
#endif

#ifndef LOCALEDIR
	#define LOCALEDIR "/usr/local/share/locale/"
#endif

#define _(String) dgettext (BIOMETRIC_DRIVER_COMMUNITY_MULTIDEVICE_DOMAIN_NAME, String)
#define N_(String) (String)

#endif // COMMUNITY_DEFINE_H
