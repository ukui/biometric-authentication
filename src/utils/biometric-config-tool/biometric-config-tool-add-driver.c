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
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <locale.h>

#include "biometric_config.h"
#include "biometric-config-tool-main.h"
#include "biometric-config-tool-add-driver.h"

extern GKeyFile * bio_config_file;
extern gboolean force_override;
extern gboolean driver_disable;

GOptionEntry add_driver_entries[] = {
	{ "force", 'f', 0, G_OPTION_ARG_NONE, &force_override,
	  N_("When there is a driver of the same name, force to override it"), NULL},
	{ "disable", 'd', 0, G_OPTION_ARG_NONE, &driver_disable,
	  N_("Only add driver and Not enable driver"), NULL},
	{ NULL }
};

gboolean
biometric_config_add_driver (int argc, char **argv, GError **error)
{
	char * driver_name = NULL;
	char * driver_path = NULL;

	GOptionContext *context;
	context = g_option_context_new (_("DriverName DriverFilePath"));
	g_option_context_add_main_entries (context, add_driver_entries, NULL);
	if (!g_option_context_parse (context, &argc, &argv, error))
	{
		char * help;

		help = g_option_context_get_help (context, FALSE, NULL);
		g_printerr ("%s", help);

		g_printerr (_("Error parsing commandline options: %s\n"), (*error)->message);
		exit (1);
	}

	if (argc != 3)
	{
		char * help;

		help = g_option_context_get_help (context, FALSE, NULL);
		g_printerr ("%s", help);

		GError *error = NULL;
		g_set_error_literal (&error, G_IO_ERROR, G_IO_ERROR_FAILED,
							 _("The number of parameters is incorrect"));
		g_printerr (_("Error parsing commandline options: %s\n"), error->message);
		g_error_free (error);

		exit (1);
	}

	driver_name = argv[1];
	driver_path = argv[2];

	if (g_key_file_has_group (bio_config_file, driver_name) && (!force_override))
	{
		g_printerr (_("This driver already exists, using the -f parameter if you "
					  "need to override the configuration\n"));
		exit (1);
	} else {
		g_key_file_set_boolean (bio_config_file,
								driver_name,
								"Enable",
								!driver_disable);

		g_key_file_set_value (bio_config_file,
							   driver_name,
							   "Driver",
							   driver_path);

		if (!g_key_file_save_to_file (bio_config_file,
									  DRIVERS_CONFIG_FILE,
									  error))
		{
			g_printerr (_("Write configuration failure: %s\n"), (*error)->message);
			exit (1);
		}
	}

	return TRUE;
}
