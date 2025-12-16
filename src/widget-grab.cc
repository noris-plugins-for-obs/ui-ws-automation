#include <obs-module.h>
#include <obs.hpp>
#include <QMainWindow>
#include <QBuffer>
#include <QWindow>
#include <QScreen>

#include "plugin-macros.generated.h"
#include "entrypoints.h"
#include "functions.hh"

struct widget_grab_s
{
	obs_data_t *request;
	obs_data_t *response;
	QMainWindow *main_window;
	QImage image;
};

static void widget_grab_internal(void *priv_data)
{
	auto *param = static_cast<widget_grab_s *>(priv_data);

	QWidget *found = find_widget_by_path(param->main_window, param->request);

	if (!found) {
		obs_data_set_string(param->response, "error", "Error: no object found");
		return;
	}

	const char *type = obs_data_get_string(param->request, "type");
	if (type && strcmp(type, "window") == 0) {
		QWidget *nw = found;
		QWindow *wh = nullptr;
		QScreen *screen = nullptr;
		while (nw) {
			wh = nw->windowHandle();
			if (wh) {
				screen = wh->screen();
				if (screen)
					break;
			}

			nw = nw->nativeParentWidget();
		}
		if (!wh) {
			obs_data_set_string(param->response, "error", "Error: no window handle");
			return;
		}

		if (!screen) {
			obs_data_set_string(param->response, "error", "Error: no screen");
			return;
		}

		int x = 0, y = 0;
		int w = -1, h = -1;
		if (found != nw) {
			QPoint p = found->mapTo(nw, QPoint(0, 0));
			x = p.x();
			y = p.y();
			w = found->rect().width();
			h = found->rect().height();
		}
		param->image = screen->grabWindow(nw->winId(), x, y, w, h).toImage();
	} else {
		param->image = found->grab().toImage();
	}
}

void widget_grab_mtsafe(obs_data_t *request, obs_data_t *response, void *priv_data)
{
	widget_grab_s param = {
		request,
		response,
		static_cast<QMainWindow *>(priv_data),
		QImage(),
	};

	obs_queue_task(OBS_TASK_UI, widget_grab_internal, &param, true);

	if (param.image.isNull()) {
		if (!obs_data_has_user_value(response, "error"))
			obs_data_set_string(response, "error", "Error: failed to convert to image");
		return;
	}

	QByteArray png_data;
	QBuffer buffer(&png_data);
	buffer.open(QIODevice::WriteOnly);
	param.image.save(&buffer, "PNG");
	QByteArray png_b64 = png_data.toBase64();
	obs_data_set_string(response, "image", png_b64.toStdString().c_str());
}
