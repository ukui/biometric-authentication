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

#ifndef BIOMETRICDBUSCOMMON_H
#define BIOMETRICDBUSCOMMON_H

#include <gio/gio.h>

#define GET_CALLER_BUS_NAME	"org.freedesktop.DBus"
#define GET_CALLER_OBJECT_PATH	"/org/freedesktop/DBus"
#define GET_CALLER_INTERFACE_NAME "org.freedesktop.DBus"
#define GET_CALLER_METHOD_UID "GetConnectionUnixUser"
#define GET_CALLER_METHOD_PID "GetConnectionUnixProcessID"

#define POLKIT_ENROLL_OWN_ACTION_ID "org.ukui.biometric.enroll-own-data"
#define POLKIT_ENROLL_ADMIN_ACTION_ID "org.ukui.biometric.enroll-admin-data"

#define POLKIT_CLEAN_OWN_ACTION_ID "org.ukui.biometric.clean-own-data"
#define POLKIT_CLEAN_ADMIN_ACTION_ID "org.ukui.biometric.clean-admin-data"

gboolean get_dbus_caller_uid (GDBusMethodInvocation *invocation, gint *uid);
gboolean get_dbus_caller_pid (GDBusMethodInvocation *invocation, gint *pid);

gboolean authority_check_by_polkit (PolkitAuthority *pAuthority,
									GDBusMethodInvocation *invocation,
									const char * action_id);

#endif // BIOMETRICDBUSCOMMON_H
