/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2021 EMQ Technologies Co., Ltd All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#include <stdlib.h>

#include "vector.h"

#include "neu_plugin.h"
#include "json/neu_json_error.h"
#include "json/neu_json_fn.h"
#include "json/neu_json_node.h"

#include "handle.h"
#include "http.h"

#include "adapter_handle.h"

void handle_add_adapter(nng_aio *aio)
{
    neu_plugin_t *plugin = neu_rest_get_plugin();

    REST_PROCESS_HTTP_REQUEST_WITH_ERROR(
        aio, neu_json_add_node_req_t, neu_json_decode_add_node_req, {
            if (req->type >= NEU_NODE_TYPE_MAX ||
                req->type <= NEU_NODE_TYPE_UNKNOW) {
                error.error = NEU_ERR_NODE_TYPE_INVALID;
            } else {
                error.error = neu_system_add_node(plugin, req->type, req->name,
                                                  req->plugin_name);
            }

            neu_json_encode_by_fn(&error, neu_json_encode_error_resp,
                                  &result_error);
            if (error.error != 0) {
                http_bad_request(aio, result_error);
            } else {
                http_created(aio, result_error);
            }
        })
}

void handle_del_adapter(nng_aio *aio)
{
    neu_plugin_t *plugin = neu_rest_get_plugin();

    REST_PROCESS_HTTP_REQUEST_WITH_ERROR(
        aio, neu_json_del_node_req_t, neu_json_decode_del_node_req, {
            error.error = neu_system_del_node(plugin, req->id);
            neu_json_encode_by_fn(&error, neu_json_encode_error_resp,
                                  &result_error);
            if (error.error != 0) {
                http_bad_request(aio, result_error);
            } else {
                http_ok(aio, result_error);
            }
        })
}

void handle_update_adapter(nng_aio *aio)
{
    neu_plugin_t *plugin = neu_rest_get_plugin();

    REST_PROCESS_HTTP_REQUEST(
        aio, neu_json_update_node_req_t, neu_json_decode_update_node_req, {
            intptr_t err = neu_system_update_node(plugin, req->type, req->name,
                                                  req->plugin_name);
            if (err != 0) {
                http_bad_request(aio, "{\"error\": 1}");
            } else {
                http_ok(aio, "{\"error\": 0}");
            }
        })
}

void handle_get_adapter(nng_aio *aio)
{
    neu_plugin_t *        plugin      = neu_rest_get_plugin();
    char *                result      = NULL;
    neu_json_error_resp_t error       = { 0 };
    char *                s_node_type = http_get_param(aio, "type");

    if (s_node_type == NULL) {
        error.error = NEU_ERR_PARAM_IS_WRONG;
        neu_json_encode_by_fn(&error, neu_json_encode_error_resp, &result);
        http_bad_request(aio, result);
        return;
    }

    neu_node_type_e           node_type = (neu_node_type_e) atoi(s_node_type);
    neu_json_get_nodes_resp_t nodes_res = { 0 };
    int                       index     = 0;
    vector_t                  nodes = neu_system_get_nodes(plugin, node_type);

    nodes_res.n_node = nodes.size;
    nodes_res.nodes =
        calloc(nodes_res.n_node, sizeof(neu_json_get_nodes_resp_node_t));

    VECTOR_FOR_EACH(&nodes, iter)
    {
        neu_node_info_t *info = (neu_node_info_t *) iterator_get(&iter);

        nodes_res.nodes[index].id        = info->node_id;
        nodes_res.nodes[index].name      = info->node_name;
        nodes_res.nodes[index].plugin_id = info->plugin_id.id_val;
        index += 1;
    }

    neu_json_encode_by_fn(&nodes_res, neu_json_encode_get_nodes_resp, &result);

    http_ok(aio, result);

    free(result);
    free(nodes_res.nodes);

    VECTOR_FOR_EACH(&nodes, iter)
    {
        neu_node_info_t *info = (neu_node_info_t *) iterator_get(&iter);

        free(info->node_name);
        index += 1;
    }
    vector_uninit(&nodes);
}

void handle_set_node_setting(nng_aio *aio)
{
    neu_plugin_t *plugin = neu_rest_get_plugin();

    REST_PROCESS_HTTP_REQUEST_WITH_ERROR(
        aio, neu_json_node_setting_req_t, neu_json_decode_node_setting_req, {
            char *config_buf = calloc(req_data_size + 1, sizeof(char));

            memcpy(config_buf, req_data, req_data_size);
            error.error =
                neu_plugin_set_node_setting(plugin, req->node_id, config_buf);
            free(config_buf);

            neu_json_encode_by_fn(&error, neu_json_encode_error_resp,
                                  &result_error);
            if (error.error != 0) {
                http_bad_request(aio, result_error);
            } else {
                http_ok(aio, result_error);
            }
        })
}

void handle_get_node_setting(nng_aio *aio)
{
    neu_plugin_t *        plugin    = neu_rest_get_plugin();
    char *                s_node_id = http_get_param(aio, "node_id");
    char *                setting   = NULL;
    neu_json_error_resp_t error     = { 0 };
    neu_node_id_t         node_id   = 0;

    if (s_node_id == NULL) {
        error.error = NEU_ERR_PARAM_IS_WRONG;
        neu_json_encode_by_fn(&error, neu_json_encode_error_resp, &setting);
        http_bad_request(aio, setting);
        return;
    }

    node_id = (neu_node_id_t) atoi(s_node_id);

    error.error = neu_plugin_get_node_setting(plugin, node_id, &setting);

    if (error.error != 0) {
        neu_json_encode_by_fn(&error, neu_json_encode_error_resp, &setting);
        http_not_found(aio, setting);
    }

    http_ok(aio, setting);
    free(setting);
}

void handle_node_ctl(nng_aio *aio)
{
    neu_plugin_t *plugin = neu_rest_get_plugin();

    REST_PROCESS_HTTP_REQUEST(
        aio, neu_json_node_ctl_req_t, neu_json_decode_node_ctl_req, {
            intptr_t err = 0;

            err = neu_plugin_node_ctl(plugin, req->id, req->cmd);

            if (err != 0) {
                http_bad_request(aio, "{\"error\": 1}");
            } else {
                http_ok(aio, "{\"error\": 0}");
            }
        })
}

void handle_get_node_state(nng_aio *aio)
{
    neu_plugin_t *                 plugin    = neu_rest_get_plugin();
    char *                         s_node_id = http_get_param(aio, "node_id");
    neu_node_id_t                  node_id   = 0;
    neu_json_get_node_state_resp_t res       = { 0 };
    neu_plugin_state_t             state     = { 0 };
    char *                         result    = NULL;

    if (s_node_id == NULL) {
        http_bad_request(aio, "{\"error\": 1}");
        return;
    }

    node_id = (neu_node_id_t) atoi(s_node_id);

    if (neu_plugin_get_node_state(plugin, node_id, &state) != 0) {
        http_not_found(aio, "{\"error\": 1}");
        return;
    }

    res.running = state.running;
    res.link    = state.link;

    neu_json_encode_by_fn(&res, neu_json_encode_get_node_state_resp, &result);

    http_ok(aio, result);
    free(result);
}