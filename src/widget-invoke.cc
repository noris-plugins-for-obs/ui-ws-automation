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

	if (method.parameterCount() == 0) {
		method.invoke(found, Qt::QueuedConnection);
		return;
	}

	if (method.parameterCount() == 1) {
		bool ok = false;
		obs_data_item_t *item = obs_data_item_byname(request, "arg1");
		if (item && obs_data_item_has_user_value(item)) {
			switch (obs_data_item_gettype(item)) {
			case OBS_DATA_STRING: {
				QString str = QString::fromUtf8(obs_data_item_get_string(item));
				method.invoke(found, Qt::QueuedConnection, Q_ARG(QString, str));
				ok = true;
				break;
			}
			case OBS_DATA_NUMBER:
				method.invoke(found, Qt::QueuedConnection, Q_ARG(int, obs_data_item_get_int(item)));
				ok = true;
				break;
			case OBS_DATA_BOOLEAN:
				method.invoke(found, Qt::QueuedConnection, Q_ARG(bool, obs_data_item_get_bool(item)));
				ok = true;
				break;
			default:
				break;
			}
		}
		obs_data_item_release(&item);
		if (ok)
			return;
	}

	blog(LOG_ERROR, "Failed to invoke method '%s'", method.name().data());
	obs_data_set_string(response, "error", "Error: invalid arguments");
}
