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

void widget_invoke(obs_data_t *request, obs_data_t *response, void *priv_data)
{
	auto main_window = static_cast<QMainWindow *>(priv_data);

	QWidget *found = find_widget_by_path(main_window, request);

	if (!found) {
		obs_data_set_string(response, "error", "Error: no object found");
		return;
	}

	QMetaMethod method;
	if (!find_method(method, found, obs_data_get_string(request, "method"))) {
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
		char arg_name[8] = "arg1";
		obs_data_item_t *item = obs_data_item_byname(request, arg_name);
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
