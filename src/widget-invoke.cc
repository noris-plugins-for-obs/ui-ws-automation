#include <obs-module.h>
#include <obs.hpp>
#include <QMainWindow>
#include <QMetaObject>
#include <QMetaMethod>

#include "plugin-macros.generated.h"
#include "entrypoints.h"
#include "functions.hh"

static bool find_method(QMetaMethod &method, QObject *obj, const char *method_name)
{
	const QMetaObject *metaObject = obj->metaObject();
	if (!metaObject)
		return false;

	for (int i = 0; i < metaObject->methodCount(); i++) {
		method = metaObject->method(i);
		if (strcmp(method.name().data(), method_name) == 0)
			return true;
	}
	return false;
}

static void qrect_to_obs(obs_data_t *data, const QRect &rect)
{
	obs_data_set_int(data, "x", rect.x());
	obs_data_set_int(data, "y", rect.y());
	obs_data_set_int(data, "width", rect.width());
	obs_data_set_int(data, "height", rect.height());
}

static void frameGeometry(QWidget *widget, obs_data_t *request, obs_data_t *response)
{
	QRect geo = widget->frameGeometry();
	if (obs_data_get_bool(request, "mapToGlobal")) {
		QPoint mapped = widget->mapToGlobal(geo.topLeft());
		geo.moveTopLeft(mapped);
	}
	qrect_to_obs(response, geo);
}

static void resize(QWidget *widget, obs_data_t *request, obs_data_t *)
{
	widget->resize(obs_data_get_int(request, "x"), obs_data_get_int(request, "y"));
}

void widget_invoke(obs_data_t *request, obs_data_t *response, void *priv_data)
{
	auto main_window = static_cast<QMainWindow *>(priv_data);

	QWidget *found = find_widget_by_path(main_window, request);

	if (!found) {
		obs_data_set_string(response, "error", "Error: no object found");
		return;
	}

	static const struct
	{
		const char *name;
		void (*cb)(QWidget *, obs_data_t *, obs_data_t *);
	} custom_methods[] = {
		{"frameGeometry", frameGeometry},
		{"resize", resize},
	};

	const char *method_name = obs_data_get_string(request, "method");
	for (const auto &cm : custom_methods) {
		if (strcmp(method_name, cm.name) != 0)
			continue;

		cm.cb(found, request, response);
		return;
	}

	QMetaMethod method;
	if (!find_method(method, found, method_name)) {
		obs_data_set_string(response, "error", "Error: no method found");

		const QMetaObject *metaObject = found->metaObject();
		if (metaObject) {
			for (int i = 0; i < metaObject->methodCount(); i++) {
				blog(LOG_INFO, "available method[%d]: '%s'", i, metaObject->method(i).name().data());
			}
		}
		return;
	}

	QMetaMethodArgument args[10];
	QString args_str[10];
	int args_int[10];
	bool args_bool[10];

	for (int i = 0; i < method.parameterCount() && i < 10; i++) {
		bool ok = false;
		char arg_name[8];
		snprintf(arg_name, sizeof(arg_name), "arg%d", i + 1);
		obs_data_item_t *item = obs_data_item_byname(request, arg_name);
		if (item && obs_data_item_has_user_value(item)) {
			switch (obs_data_item_gettype(item)) {
			case OBS_DATA_STRING:
				args_str[i] = QString::fromUtf8(obs_data_item_get_string(item));
				args[i] = Q_ARG(QString, args_str[i]);
				ok = true;
				break;
			case OBS_DATA_NUMBER:
				args_int[i] = obs_data_item_get_int(item);
				args[i] = Q_ARG(int, args_int[i]);
				ok = true;
				break;
			case OBS_DATA_BOOLEAN:
				args_bool[i] = obs_data_item_get_bool(item);
				args[i] = Q_ARG(bool, args_bool[i]);
				ok = true;
				break;
			default:
				break;
			}
		}
		obs_data_item_release(&item);
		if (!ok) {
			blog(LOG_ERROR, "Failed to parse '%s' for method '%s'", arg_name, method.name().data());
			obs_data_set_string(response, "error", "Error: invalid arguments");
			return;
		}
	}

	switch (method.parameterCount()) {
	case 0:
		method.invoke(found, Qt::QueuedConnection);
		return;
	case 1:
		method.invoke(found, Qt::QueuedConnection, args[0]);
		return;
	case 2:
		method.invoke(found, Qt::QueuedConnection, args[0], args[1]);
		return;
	case 3:
		method.invoke(found, Qt::QueuedConnection, args[0], args[1], args[2]);
		return;
	case 4:
		method.invoke(found, Qt::QueuedConnection, args[0], args[1], args[2], args[3]);
		return;
	case 5:
		method.invoke(found, Qt::QueuedConnection, args[0], args[1], args[2], args[3], args[4]);
		return;
	case 6:
		method.invoke(found, Qt::QueuedConnection, args[0], args[1], args[2], args[3], args[4], args[5]);
		return;
	case 7:
		method.invoke(found, Qt::QueuedConnection, args[0], args[1], args[2], args[3], args[4], args[5],
			      args[6]);
		return;
	case 8:
		method.invoke(found, Qt::QueuedConnection, args[0], args[1], args[2], args[3], args[4], args[5],
			      args[6], args[7]);
		return;
	case 9:
		method.invoke(found, Qt::QueuedConnection, args[0], args[1], args[2], args[3], args[4], args[5],
			      args[6], args[7], args[8]);
		return;
	case 10:
		method.invoke(found, Qt::QueuedConnection, args[0], args[1], args[2], args[3], args[4], args[5],
			      args[6], args[7], args[8], args[9]);
		return;
	default:
		blog(LOG_ERROR, "Method '%s' has too many parameters (%d)", method.name().data(),
		     method.parameterCount());
		obs_data_set_string(response, "error", "Error: too many parameters");
	}
	return;
}
