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

#ifndef BIOMETRICCONFIG_H
#define BIOMETRICCONFIG_H

#include <glib.h>
#include <glib/gi18n.h>
//#include "biometric_intl.h"

//GKeyFile * bio_config_file;

#ifndef DOMAIN_NAME
	#define DOMAIN_NAME "biometric-authentication"
#endif

#ifndef LOCALEDIR
	#define LOCALEDIR "/usr/local/share/locale/"
#endif

#endif // BIOMETRICCONFIG_H
