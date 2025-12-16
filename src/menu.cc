#include <obs-module.h>
#include <obs.hpp>
#include <QMainWindow>
#include <QMenuBar>

#include "plugin-macros.generated.h"
#include "entrypoints.h"
#include "functions.hh"

static bool query_menu_action(const QAction *action, obs_data_t *data)
{
	if (action->isSeparator())
		return false;

	query_object_all(action, data);
	obs_data_set_string(data, "text", QT_TO_UTF8(action->text()));

	if (const QMenu *menu = action->menu()) {
		OBSDataArrayAutoRelease array = obs_data_array_create();
		for (QAction *action : menu->actions()) {
			OBSDataAutoRelease submenu_data = obs_data_create();
			if (query_menu_action(action, submenu_data))
				obs_data_array_push_back(array, submenu_data);
		}
		obs_data_set_array(data, "menu", array);
	}

	return true;
}

static bool test_menu_action(const QAction *action, obs_data_t *data)
{
	if (action->isSeparator())
		return false;

	if (!test_object(action, data))
		return false;

	return true;
}

static QAction *find_menu_action(QAction *action, obs_data_array_t *array, int idx)
{
	OBSDataAutoRelease data = obs_data_array_item(array, idx);
	if (!data)
		return nullptr;
	if (!test_menu_action(action, data))
		return nullptr;

	if ((size_t)idx + 1 == obs_data_array_count(array))
		return action;

	if (const QMenu *menu = action->menu()) {
		for (QAction *sub_action : menu->actions()) {
			if (QAction *ret = find_menu_action(sub_action, array, idx + 1))
				return ret;
		}
	}

	return nullptr;
}

void menu_list(obs_data_t *request, obs_data_t *response, void *priv_data)
{
	UNUSED_PARAMETER(request);
	auto main_window = static_cast<QMainWindow *>(priv_data);

	QMenuBar *menuBar = qobject_cast<QMenuBar *>(main_window->menuWidget());
	if (!menuBar) {
		obs_data_set_string(response, "error", "Error: no menu bar");
	}

	OBSDataArrayAutoRelease array = obs_data_array_create();
	for (QAction *action : menuBar->actions()) {
		OBSDataAutoRelease data = obs_data_create();
		if (query_menu_action(action, data))
			obs_data_array_push_back(array, data);
	}

	obs_data_set_array(response, "menu", array);
}

void menu_trigger(obs_data_t *request, obs_data_t *response, void *priv_data)
{
	auto main_window = static_cast<QMainWindow *>(priv_data);

	QMenuBar *menuBar = qobject_cast<QMenuBar *>(main_window->menuWidget());
	if (!menuBar) {
		obs_data_set_string(response, "error", "Error: no menu bar");
	}

	OBSDataArrayAutoRelease array = obs_data_get_array(request, "path");
	QAction *found = nullptr;
	for (QAction *action : menuBar->actions()) {
		found = find_menu_action(action, array, 0);
		if (found)
			break;
	}
	if (found) {
		found->trigger();
	} else {
		obs_data_set_string(response, "error", "Error: not found");
	}
}
