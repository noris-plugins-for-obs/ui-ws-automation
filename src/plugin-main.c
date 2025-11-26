/*
UI WS Automation Plugin for OBS Studio
Copyright (C) 2025 Norihiro Kamae

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <stdlib.h>

#include <obs-websocket-api.h>
#include "plugin-macros.generated.h"
#include "entrypoints.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

const char *obs_module_name(void)
{
	return obs_module_text("Module.Name");
}

bool obs_module_load(void)
{
	int version_major = atoi(obs_get_version_string());
	if (version_major && version_major < LIBOBS_API_MAJOR_VER) {
		blog(LOG_ERROR, "Cancel loading plugin since OBS version '%s' is older than plugin API version %d",
		     obs_get_version_string(), LIBOBS_API_MAJOR_VER);
		return false;
	}

	/* TODO: Register your source-types, output-types, etc. here. */

	blog(LOG_INFO, "plugin loaded (plugin version %s, API version %d.%d.%d)", PLUGIN_VERSION, LIBOBS_API_MAJOR_VER,
	     LIBOBS_API_MINOR_VER, LIBOBS_API_PATCH_VER);
	return true;
}

struct ws_interface_s
{
	obs_data_t *request;
	obs_data_t *response;
	void *priv_data;
};

#define FUNC_MTSAFE(func)                                                                     \
	static void func##_mtsafe_int(void *data)                                             \
	{                                                                                     \
		struct ws_interface_s *wsif = data;                                           \
		func(wsif->request, wsif->response, wsif->priv_data);                         \
	}                                                                                     \
	static void func##_mtsafe(obs_data_t *request, obs_data_t *response, void *priv_data) \
	{                                                                                     \
		if (obs_in_task_thread(OBS_TASK_UI))                                          \
			func(request, response, priv_data);                                   \
		else {                                                                        \
			struct ws_interface_s wsif = {request, response, priv_data};          \
			obs_queue_task(OBS_TASK_UI, func##_mtsafe_int, &wsif, true);          \
		}                                                                             \
	}

FUNC_MTSAFE(menu_list);
FUNC_MTSAFE(menu_trigger);
FUNC_MTSAFE(widget_list);
FUNC_MTSAFE(widget_invoke);

void obs_module_post_load()
{
	void *main_window = obs_frontend_get_main_window();
	if (!main_window) {
		blog(LOG_ERROR, "Cannot get the main window pointer");
		return;
	}

	obs_websocket_vendor ws_vendor = obs_websocket_register_vendor(PLUGIN_NAME);
	if (!ws_vendor) {
		blog(LOG_ERROR, "Cannot register websocket vendor '%s'", PLUGIN_NAME);
		return;
	}

	obs_websocket_vendor_register_request(ws_vendor, "menu-list", menu_list_mtsafe, main_window);
	obs_websocket_vendor_register_request(ws_vendor, "menu-trigger", menu_trigger_mtsafe, main_window);
	obs_websocket_vendor_register_request(ws_vendor, "widget-list", widget_list_mtsafe, main_window);
	obs_websocket_vendor_register_request(ws_vendor, "widget-invoke", widget_invoke_mtsafe, main_window);
}
