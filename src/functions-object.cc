#include <obs-module.h>
#include <QMetaObject>
#include <QMetaProperty>

#include "plugin-macros.generated.h"
#include "functions.hh"

void query_object_all(const QObject *obj, obs_data_t *data)
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

bool test_object(const QObject *obj, obs_data_t *data)
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
