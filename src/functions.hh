#pragma once

#define QT_TO_UTF8(str) str.toUtf8().constData()

class QObject;
class QWidget;
class QMainWindow;

void query_object_all(const QObject *obj, obs_data_t *data);
bool test_object(const QObject *obj, obs_data_t *data);
QWidget *find_widget_by_path(QMainWindow *main_window, obs_data_t *request);
