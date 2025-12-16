#include <obs-module.h>
#include <obs.hpp>
#include <QMainWindow>
#include <QMetaObject>
#include <QMetaMethod>

#include "plugin-macros.generated.h"
#include "entrypoints.h"
#include "functions.hh"

static bool query_widget(const QWidget *widget, obs_data_t *data, int depth)
{
	query_object_all(widget, data);

	if (depth == 0 || depth > 1) {
		OBSDataArrayAutoRelease array = obs_data_array_create();
		for (const QObject *obj : widget->children()) {
			const QWidget *w = qobject_cast<const QWidget *>(obj);
			if (!w)
				continue;

			OBSDataAutoRelease child_data = obs_data_create();
			if (query_widget(w, child_data, depth ? depth - 1 : 0))
				obs_data_array_push_back(array, child_data);
		}
		obs_data_set_array(data, "children", array);
	}

	return true;
}

static QWidget *find_widget(QWidget *widget, obs_data_array_t *array, int idx)
{
	OBSDataAutoRelease data = obs_data_array_item(array, idx);
	if (!data)
		return nullptr;
	if (!test_object(widget, data))
		return nullptr;

	if ((size_t)idx + 1 == obs_data_array_count(array))
		return widget;

	for (QObject *obj : widget->children()) {
		QWidget *w = qobject_cast<QWidget *>(obj);
		if (!w)
			continue;
		if (QWidget *ret = find_widget(w, array, idx + 1))
			return ret;
	}

	return nullptr;
}

void widget_list(obs_data_t *request, obs_data_t *response, void *priv_data)
{
	UNUSED_PARAMETER(request);
	auto main_window = static_cast<QMainWindow *>(priv_data);

	query_widget(main_window, response, 0);
}

QWidget *find_widget_by_path(QMainWindow *main_window, obs_data_t *request)
{
	OBSDataArrayAutoRelease array = obs_data_get_array(request, "path");

	if (obs_data_array_count(array) == 0)
		return main_window;

	for (QObject *obj : main_window->children()) {
		QWidget *w = qobject_cast<QWidget *>(obj);
		if (!w)
			continue;
		if (QWidget *ret = find_widget(w, array, 0)) {
			return ret;
		}
	}
	return nullptr;
}
