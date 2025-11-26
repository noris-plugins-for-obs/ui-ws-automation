#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void menu_list(obs_data_t *request, obs_data_t *response, void *priv_data);
void menu_trigger(obs_data_t *request, obs_data_t *response, void *priv_data);
void widget_list(obs_data_t *request, obs_data_t *response, void *priv_data);
void widget_invoke(obs_data_t *request, obs_data_t *response, void *priv_data);

#ifdef __cplusplus
} // extern "C"
#endif
