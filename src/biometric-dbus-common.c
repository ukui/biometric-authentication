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

#include <stdio.h>
#include <glib.h>
#include <polkit/polkit.h>
#include <glib/gi18n.h>

#include "lib/biometric_common.h"
#include "biometric-dbus-common.h"

gboolean
get_dbus_caller_uid (GDBusMethodInvocation *invocation, gint *uid)
{
	GVariant *reply;
	GError *error;

	error = NULL;
	reply = g_dbus_connection_call_sync (g_dbus_method_invocation_get_connection (invocation),
										 GET_CALLER_BUS_NAME,
										 GET_CALLER_OBJECT_PATH,
										 GET_CALLER_INTERFACE_NAME,
										 GET_CALLER_METHOD_UID,
										 g_variant_new ("(s)",
														g_dbus_method_invocation_get_sender (invocation)),
										 G_VARIANT_TYPE ("(u)"),
										 G_DBUS_CALL_FLAGS_NONE,
										 -1,
										 NULL,
										 &error);

	if (reply == NULL) {
			bio_print_warning (_("The UID information of the requester (%s) "
								 "could not be obtained. Error: %s\n"),
								g_dbus_method_invocation_get_sender (invocation),
								error->message);
			g_error_free (error);

			return FALSE;
	}

	g_variant_get (reply, "(u)", uid);
	g_variant_unref (reply);

	return TRUE;
}

gboolean
get_dbus_caller_pid (GDBusMethodInvocation *invocation, gint *pid)
{
	GVariant *reply;
	GError *error;

	error = NULL;
	reply = g_dbus_connection_call_sync (g_dbus_method_invocation_get_connection (invocation),
										 GET_CALLER_BUS_NAME,
										 GET_CALLER_OBJECT_PATH,
										 GET_CALLER_INTERFACE_NAME,
										 GET_CALLER_METHOD_PID,
										 g_variant_new ("(s)",
														g_dbus_method_invocation_get_sender (invocation)),
										 G_VARIANT_TYPE ("(u)"),
										 G_DBUS_CALL_FLAGS_NONE,
										 -1,
										 NULL,
										 &error);

	if (reply == NULL) {
			bio_print_warning (_("The PID information of the requester (%s) "
								 "could not be obtained. Error: %s\n"),
							   g_dbus_method_invocation_get_sender (invocation),
							   error->message);
			g_error_free (error);

			return FALSE;
	}

	g_variant_get (reply, "(u)", pid);
	g_variant_unref (reply);

	return TRUE;
}

gboolean
authority_check_by_polkit (PolkitAuthority *pAuthority,
						   GDBusMethodInvocation *invocation,
						   const char *action_id)
{
	PolkitSubject *subject;
	PolkitCheckAuthorizationFlags flags;
	PolkitAuthorizationResult * polkit_ret = NULL;
	gboolean auth_ret = FALSE;

	subject = polkit_system_bus_name_new (g_dbus_method_invocation_get_sender (invocation));

	flags = POLKIT_CHECK_AUTHORIZATION_FLAGS_NONE;
	flags |= POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION;

	polkit_ret = polkit_authority_check_authorization_sync(pAuthority,
														   subject,
														   action_id,
														   NULL,
														   flags,
														   NULL,
														   NULL);

	if (polkit_authorization_result_get_is_authorized (polkit_ret))
	{
		auth_ret = TRUE;
	};

	g_object_unref (polkit_ret);
	g_object_unref (subject);

	return auth_ret;
}
