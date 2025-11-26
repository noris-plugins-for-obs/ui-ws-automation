#include <obs-module.h>
#include <obs.hpp>
#include <QMainWindow>
#include <QMenuBar>
#include <QMetaProperty>

#include "plugin-macros.generated.h"
#include "entrypoints.h"

#define QT_TO_UTF8(str) str.toUtf8().constData()

static void query_object(const QObject *obj, obs_data_t *data)
{
	obs_data_set_string(data, "objectName", QT_TO_UTF8(obj->objectName()));
	obs_data_set_string(data, "className", obj->metaObject()->className());
}

static void query_object_all(const QObject *obj, obs_data_t *data)
{
	obs_data_set_string(data, "className", obj->metaObject()->className());

	const QMetaObject *metaObject = obj->metaObject();
	for (int i = 0; i < metaObject->propertyCount(); i++) {
		const char *prop = metaObject->property(i).name();

		const QVariant var = obj->property(prop);
		if (!var.isValid())
			continue;
		switch (var.typeId()) {
		case QMetaType::QString:
			obs_data_set_string(data, prop, QT_TO_UTF8(var.toString()));
			break;
		case QMetaType::Bool:
			obs_data_set_bool(data, prop, var.toBool());
			break;
		case QMetaType::Int:
			obs_data_set_int(data, prop, var.toInt());
			break;
		case QMetaType::UInt:
		case QMetaType::Long:
		case QMetaType::LongLong:
		case QMetaType::Short:
		case QMetaType::ULong:
		case QMetaType::ULongLong:
		case QMetaType::UShort: {
			bool ok = false;
			long long value = var.toLongLong(&ok);
			if (ok)
				obs_data_set_int(data, prop, value);
			break;
		}
		case QMetaType::Double:
			obs_data_set_double(data, prop, var.toDouble());
			break;
		default:
			break;
		}
	}
}

static bool test_object(const QObject *obj, obs_data_t *data)
{
	obs_data_item_t *item = obs_data_first(data);
	bool ret = true;
	for (; item; obs_data_item_next(&item)) {
		if (!obs_data_item_has_user_value(item))
			continue;

		const char *prop = obs_data_item_get_name(item);

		if (strcmp(prop, "className") == 0) {
			if (strcmp(obj->metaObject()->className(), obs_data_item_get_string(item)) != 0) {
				ret = false;
				break;
			}
			continue;
		}

		const QVariant var = obj->property(prop);
		if (!var.isValid()) {
			ret = false;
			break;
		}
		bool ok = false;
		switch (obs_data_item_gettype(item)) {
		case OBS_DATA_STRING:
			if (strcmp(QT_TO_UTF8(var.toString()), obs_data_item_get_string(item)) != 0)
				ret = false;
			break;
		case OBS_DATA_NUMBER:
			if (var.toLongLong(&ok) != obs_data_item_get_int(item) || !ok)
				ret = false;
			break;
		case OBS_DATA_BOOLEAN:
			if (var.toBool() != obs_data_item_get_bool(item))
				ret = false;
			break;
		default:
			ret = false;
		}

		if (!ret)
			break;
	}

	if (item)
		obs_data_item_release(&item);

	return ret;
}

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

void menu_list(obs_data_t *request, obs_data_t *response, void *priv_data)
{
	auto main_window = static_cast<QMainWindow *>(priv_data);

	QMenuBar *menuBar = qobject_cast<QMenuBar *>(main_window->menuWidget());
	if (!menuBar) {
		obs_data_set_string(response, "result", "Error: no menu bar");
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
		obs_data_set_string(response, "result", "Error: no menu bar");
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
		obs_data_set_string(response, "result", "Error: not found");
	}
}

void widget_list(obs_data_t *request, obs_data_t *response, void *priv_data)
{
	auto main_window = static_cast<QMainWindow *>(priv_data);

	query_widget(main_window, response, 0);
}

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

	OBSDataArrayAutoRelease array = obs_data_get_array(request, "path");
	QWidget *found = nullptr;
	for (QObject *obj : main_window->children()) {
		QWidget *w = qobject_cast<QWidget *>(obj);
		if (!w)
			continue;
		if (QWidget *ret = find_widget(w, array, 0)) {
			found = ret;
			break;
		}
	}

	if (!found) {
		obs_data_set_string(response, "result", "Error: no object found");
		return;
	}

	QMetaMethod method;
	if (!find_method(method, found, obs_data_get_string(request, "method"))) {
		obs_data_set_string(response, "result", "Error: no method found");

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
		char arg_name[8] = "arg1";
		obs_data_item_t *item = obs_data_item_byname(request, arg_name);
		if (!item)
			return;
		if (obs_data_item_has_user_value(item)) {
			switch (obs_data_item_gettype(item)) {
			case OBS_DATA_STRING: {
				QString str = QString::fromUtf8(obs_data_item_get_string(item));
				method.invoke(found, Qt::QueuedConnection, Q_ARG(QString, str));
				break;
			}
			case OBS_DATA_NUMBER:
				// TODO: Implement
				break;
			case OBS_DATA_BOOLEAN:
				// TODO: Implement
				break;
			default:
				break;
			}
		}
		obs_data_item_release(&item);
	}
}
