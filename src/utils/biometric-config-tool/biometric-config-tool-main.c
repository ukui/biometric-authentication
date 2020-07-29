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
#include <string.h>
#include <gio/gio.h>
#include <locale.h>
#include <glib/gi18n.h>

#include "biometric_config.h"
#include "biometric-config-tool-main.h"
#include "biometric-config-tool-add-driver.h"
#include "biometric-config-tool-remove-driver.h"
#include "biometric-config-tool-enable-driver.h"
#include "biometric-config-tool-disable-driver.h"
#include "biometric-config-tool-set-key.h"
#include "biometric-config-tool-remove-key.h"

GKeyFile * bio_config_file;
gboolean force_override;
gboolean driver_disable;
gboolean driver_ignore;
gboolean ignore_exist;
gboolean key_is_exist;

typedef struct
{
	const char *name;
	const char *description;
	gboolean (*fn)(int argc, char **argv, GError **error);
} BiometricConfigCommand;

static BiometricConfigCommand commands[] = {
	{ "add-driver", N_("Add driver"), biometric_config_add_driver },
	{ "remove-driver", N_("Remove drive"), biometric_config_remove_driver },
	{ "enable-driver", N_("Enable driver"), biometric_config_enable_driver },
	{ "disable-driver", N_("Disable driver"), biometric_config_disable_driver },
	{ "set-key", N_("Set the key value of the specified driver"), biometric_config_set_key },
	{ "remove-key", N_("remove the key value of the specified driver"), biometric_config_remove_key },
	{ NULL }
};

GOptionEntry global_entries[] = {
	{ NULL }
};

int config_init()
{
	bio_config_file = g_key_file_new();
	GError *err = NULL;

	g_key_file_load_from_file(bio_config_file, DRIVERS_CONFIG_FILE, G_KEY_FILE_KEEP_COMMENTS, &err);
	if (err != NULL) {
		// 若配置文件不存在，继续执行
		if (err->code == 4)
		{
			g_error_free(err);
			return 0;
		}

		g_printerr ("Loading configuration file error[%d]: %s\n", err->code, err->message);
		g_error_free(err);
		err = NULL;
		return -1;
	}

	return 0;
}

void config_free()
{
	g_key_file_free(bio_config_file);
}

GKeyFile * get_config()
{
	return bio_config_file;
}

static GOptionContext *
biometric_config_option_context_new_with_commands (BiometricConfigCommand *commands)
{
	GOptionContext *context;
	GString *description;

	context = g_option_context_new (_("BuiltinCommands"));
//	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);

	description = g_string_new (_("Builtin commands: "));

	while (commands->name != NULL)
	{
		if (commands->fn != NULL)
		{
			g_string_append_printf (description, "\n  %s", commands->name);
			if (commands->description)
				g_string_append_printf (description, "%*s%s",
										(int) (20 - strlen (commands->name)),
										"",
										_(commands->description));
		}
		else
		{
			g_string_append_printf (description, "\n%s", commands->name);
		}
		commands++;
	}

	g_string_append_printf (description, "\n");
	g_option_context_set_description (context, description->str);

	g_string_free (description, TRUE);

	return context;
}

static int
biometric_config_usage(BiometricConfigCommand *commands, gboolean is_error)
{
	GOptionContext *context;
	g_autofree char *help = NULL;

	context = biometric_config_option_context_new_with_commands (commands);

	g_option_context_add_main_entries (context, global_entries, NULL);

	help = g_option_context_get_help (context, FALSE, NULL);

	if (is_error)
		g_printerr ("%s", help);
	else
		g_print ("%s", help);

	g_option_context_free (context);

	return is_error ? 1 : 0;
}

static BiometricConfigCommand *
extract_command (int     *argc,
				 char   **argv,
				 const char **command_name_out)
{
	BiometricConfigCommand *command;
	const char *command_name = NULL;
	int in, out;

	for (in = 1, out = 1; in < *argc; in++, out++)
	{
		if (argv[in][0] != '-')
		{
			if (command_name == NULL)
			{
				command_name = argv[in];
				out--;
				continue;
			}
		}

		argv[out] = argv[in];
	}

	*argc = out;
	argv[out] = NULL;

	command = commands;
	while (command->name)
	{
		if (command->fn != NULL &&
				g_strcmp0 (command_name, command->name) == 0)
			break;
		command++;
	}

	*command_name_out = command_name;

	return command;
}

static int
biometric_config_run (int      argc,
					  char   **argv,
					  GError **res_error)
{
	BiometricConfigCommand *command;
	GError *error = NULL;
	g_autofree char *prgname = NULL;
	gboolean success = FALSE;
	const char *command_name = NULL;

	command = extract_command (&argc, argv, &command_name);

	if (!command->fn)
	{
		GOptionContext *context;
		g_autofree char *help = NULL;

		context = biometric_config_option_context_new_with_commands (commands);

		if (command_name != NULL)
		{
			g_set_error (&error, G_IO_ERROR, G_IO_ERROR_FAILED,
						 _("Unknown command %s"), command_name);
		}
		else
		{
			g_option_context_add_main_entries (context, global_entries, NULL);

			if (g_option_context_parse (context, &argc, &argv, &error))
			{
				g_set_error_literal (&error, G_IO_ERROR, G_IO_ERROR_FAILED,
									 _("Missing builtin command"));
			}
		}

		help = g_option_context_get_help (context, FALSE, NULL);
		g_printerr ("%s", help);

		g_option_context_free (context);

		goto out;
	}

	prgname = g_strdup_printf ("%s %s", g_get_prgname (), command_name);
	g_set_prgname (prgname);

	if (!command->fn (argc, argv, &error))
		goto out;

	success = TRUE;
out:
	g_assert (success || error);

	if (error)
	{
		g_propagate_error (res_error, error);
		return 1;
	}
	return 0;
}

int main (int argc, char **argv)
{
	GError *error = NULL;
	int ret;

	setlocale (LC_ALL, "");
	bindtextdomain (DOMAIN_NAME, LOCALEDIR);
	textdomain (DOMAIN_NAME);

	g_set_prgname (argv[0]);

	config_init();

	ret = biometric_config_run (argc, argv, &error);
	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
		biometric_config_usage(commands, TRUE);

	if (error != NULL)
	{
		g_printerr (_("Error parsing commandline options: %s\n"), error->message);
		g_error_free (error);
	}

	config_free();

	return ret;
}
