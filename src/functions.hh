#pragma once

#include <obs.hpp>

#define QT_TO_UTF8(str) ((str).toUtf8().constData())

class QObject;
class QWidget;
class QMainWindow;

void query_object_all(const QObject *obj, obs_data_t *data);
bool test_object(const QObject *obj, obs_data_t *data);
QWidget *find_widget_by_path(QMainWindow *main_window, obs_data_t *request);

static inline void obs_data_item_release_1(obs_data_item_t *item)
{
	obs_data_item_release(&item);
}

using OBSDataItemAutoRelease = OBSRefAutoRelease<obs_data_item_t *, obs_data_item_release_1>;
