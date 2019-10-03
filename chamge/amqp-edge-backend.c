/**
 *  Copyright 2019 SK Telecom Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *            Heekyoung Seo <hkseo@sk.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include "config.h"

#include "amqp-edge-backend.h"
#include "glib-compat.h"

#include <gio/gio.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <json-glib/json-glib.h>
#include <stdlib.h>
#include <string.h>

#define AMQP_EDGE_BACKEND_SCHEMA_ID "org.hwangsaeul.Chamge1.Edge.AMQP"
#define DEFAULT_CONTENT_TYPE "application/json"

struct _ChamgeAmqpEdgeBackend
{
  ChamgeEdgeBackend parent;
  GSettings *settings;

  amqp_connection_state_t amqp_conn;
  amqp_socket_t *amqp_socket;

  gboolean activated;
  guint process_id;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeAmqpEdgeBackend, chamge_amqp_edge_backend, CHAMGE_TYPE_EDGE_BACKEND)
/* *INDENT-ON* */

/*
  reference from tools/common.c of rabbitmq-c
*/
inline static const char *
_amqp_server_exception_string (amqp_rpc_reply_t r)
{
  int res;
  static char s[512];

  switch (r.reply.id) {
    case AMQP_CONNECTION_CLOSE_METHOD:{
      amqp_connection_close_t *m = (amqp_connection_close_t *) r.reply.decoded;
      res =
          g_snprintf (s, sizeof (s),
          "server connection error %d, message: %.*s", m->reply_code,
          (int) m->reply_text.len, (char *) m->reply_text.bytes);
      break;
    }

    case AMQP_CHANNEL_CLOSE_METHOD:{
      amqp_channel_close_t *m = (amqp_channel_close_t *) r.reply.decoded;
      res = g_snprintf (s, sizeof (s), "server channel error %d, message: %.*s",
          m->reply_code, (int) m->reply_text.len, (char *) m->reply_text.bytes);
      break;
    }

    default:
      res = g_snprintf (s, sizeof (s), "unknown server error, method id 0x%08X",
          r.reply.id);
      break;
  }

  return res >= 0 ? s : NULL;
}


/*
  reference from tools/common.c of rabbitmq-c
*/
inline static const gchar *
_amqp_rpc_reply_string (amqp_rpc_reply_t r)
{
  switch (r.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      return "normal response";

    case AMQP_RESPONSE_NONE:
      return "missing RPC reply type";

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      return amqp_error_string2 (r.library_error);

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      return _amqp_server_exception_string (r);

    default:
      abort ();
  }
}

static ChamgeReturn
_amqp_rpc_login (amqp_connection_state_t amqp_conn, amqp_socket_t * amqp_socket,
    const gchar * url, guint channel, GError ** error)
{
  amqp_rpc_reply_t amqp_r;
  struct amqp_connection_info connection_info = { 0 };
  g_autofree gchar *str = g_strdup (url);
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  g_return_val_if_fail (amqp_conn != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (amqp_socket != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (url != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (channel != 0, CHAMGE_RETURN_FAIL);

  if (amqp_parse_url (str, &connection_info) != AMQP_STATUS_OK) {
    g_set_error_literal (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_INVALID_PARAMETER, "url pasring failure");
    goto out;
  }

  if (amqp_socket_open (amqp_socket,
          connection_info.host, connection_info.port)) {
    g_set_error_literal (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "socket open failure");
    goto out;
  }

  /* TODO: The authentication method should be EXTERNAL
   * if we don't want to share user/passwd
   */
  amqp_r = amqp_login (amqp_conn, connection_info.vhost, 0, 131072, 0,
      AMQP_SASL_METHOD_PLAIN, connection_info.user, connection_info.password);
  if (amqp_r.reply_type != AMQP_RESPONSE_NORMAL) {
    g_set_error (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "login failure >> %s",
        _amqp_rpc_reply_string (amqp_r));
    goto out;
  }
  if (!amqp_channel_open (amqp_conn, channel)) {
    g_set_error (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "channel open failure >> %s",
        _amqp_rpc_reply_string (amqp_get_rpc_reply (amqp_conn)));
    goto out;
  }

  g_debug ("success to login %s:%d/%s id[%s] pwd[%s]",
      connection_info.host, connection_info.port,
      connection_info.vhost, connection_info.user, connection_info.password);
  ret = CHAMGE_RETURN_OK;

out:
  return ret;
}

static ChamgeReturn
_amqp_declare_queue (const amqp_connection_state_t conn,
    const gchar * queue_name, amqp_bytes_t * queue, GError ** error)
{
  amqp_queue_declare_ok_t *amqp_declar_r = NULL;
  gchar *response_queue_name = NULL;

  g_return_val_if_fail (conn != NULL, CHAMGE_RETURN_FAIL);

  if (queue_name == NULL) {
    amqp_declar_r = amqp_queue_declare (conn, 1, amqp_empty_bytes, 0, 0, 0, 1,
        amqp_empty_table);
  } else {
    amqp_declar_r =
        amqp_queue_declare (conn, 1, amqp_cstring_bytes (queue_name), 0, 0, 0,
        1, amqp_empty_table);
  }
  if (amqp_declar_r == NULL) {
    g_set_error (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "declare queue failure >> %s",
        _amqp_rpc_reply_string (amqp_get_rpc_reply (conn)));
    return CHAMGE_RETURN_FAIL;
  }

  g_assert_nonnull (amqp_declar_r->queue.bytes);
  response_queue_name = g_strdup (amqp_declar_r->queue.bytes);
  if (queue->bytes)
    amqp_bytes_free (*queue);
  *queue = amqp_cstring_bytes (response_queue_name);

  g_debug ("create private reply_to queue : name -> %s",
      (gchar *) queue->bytes);
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
_amqp_rpc_request (amqp_connection_state_t amqp_conn, guint channel,
    const gchar * request, const gchar * exchange, const gchar * queue_name,
    gchar ** response_body, GError ** error)
{
  amqp_bytes_t amqp_reply_queue = { 0 };
  amqp_basic_properties_t amqp_props = { 0 };
  g_autofree gchar *correlation_id = NULL;
  guint retry_count = 0;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  g_return_val_if_fail (amqp_conn != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (channel != 0, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (queue_name != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (request != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (response_body != NULL, CHAMGE_RETURN_FAIL);

  /* create private replay_to_queue and queue name is random that is supplied from amqp server */
  if (_amqp_declare_queue (amqp_conn, NULL,
          &amqp_reply_queue, error) != CHAMGE_RETURN_OK)
    return ret;

  g_debug ("declare a queue for reply : %s", (gchar *) amqp_reply_queue.bytes);

  /* property setting to send rpc request */
  amqp_props._flags =
      AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;

  amqp_props.content_type = amqp_cstring_bytes (DEFAULT_CONTENT_TYPE);
  amqp_props.delivery_mode = 2; /* persistent delivery mode */

  amqp_props._flags |= AMQP_BASIC_REPLY_TO_FLAG;
  amqp_props.reply_to = amqp_reply_queue;

  amqp_props._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
  correlation_id = g_uuid_string_random ();
  correlation_id =
      g_compute_checksum_for_string (G_CHECKSUM_SHA256, correlation_id,
      strlen (correlation_id));
  amqp_props.correlation_id = amqp_cstring_bytes (correlation_id);

  /* send requst to queue_name */
  {
    gint r = amqp_basic_publish (amqp_conn, channel,
        exchange ==
        NULL ? amqp_cstring_bytes ("") : amqp_cstring_bytes (exchange),
        amqp_cstring_bytes (queue_name), 0, 0, &amqp_props,
        amqp_cstring_bytes (request));
    if (r < 0) {
      g_set_error (error, CHAMGE_BACKEND_ERROR,
          CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "publish failure >> %s",
          amqp_error_string2 (r));
      goto out;
    }
    g_debug ("published to queue[%s], exchange[%s], request[%s]", queue_name,
        exchange, request);
  }

  if (amqp_basic_consume (amqp_conn, 1, amqp_reply_queue, amqp_empty_bytes, 0,
          1, 0, amqp_empty_table) == NULL) {
    g_set_error (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "basic consume failure >> %s",
        _amqp_rpc_reply_string (amqp_get_rpc_reply (amqp_conn)));
    goto out;
  }


  /* wait an answer */
  do {
    amqp_rpc_reply_t amqp_r;
    amqp_envelope_t envelope;
    struct timeval timeout = { 10, 0 };

    if (retry_count > 10) {
      g_set_error_literal (error, CHAMGE_BACKEND_ERROR,
          CHAMGE_BACKEND_ERROR_OPERATION_FAILURE,
          "no response to the request in 10 messages");
      break;
    }
    amqp_maybe_release_buffers (amqp_conn);

    g_debug ("Wait for response message from [%s]",
        (gchar *) amqp_reply_queue.bytes);
    amqp_r = amqp_consume_message (amqp_conn, &envelope, &timeout, 0);
    retry_count++;

    if (amqp_r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION
        && amqp_r.library_error == AMQP_STATUS_TIMEOUT) {
      g_set_error_literal (error, CHAMGE_BACKEND_ERROR,
          CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "consume msg timeout");
      break;
    }

    if (amqp_r.reply_type != AMQP_RESPONSE_NORMAL) {
      g_set_error (error, CHAMGE_BACKEND_ERROR,
          CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "consume msg failure >> %s",
          _amqp_rpc_reply_string (amqp_r));
      break;
    }

    g_debug ("Delivery %u, exchange %.*s routingkey %.*s",
        (unsigned) envelope.delivery_tag, (int) envelope.exchange.len,
        (char *) envelope.exchange.bytes, (int) envelope.routing_key.len,
        (char *) envelope.routing_key.bytes);

    if ((envelope.message.properties._flags & AMQP_BASIC_CORRELATION_ID_FLAG) ==
        0) {
      g_debug ("discard >> correaltion id is not exist");
      continue;
    }

    if (strncmp (correlation_id,
            envelope.message.properties.correlation_id.bytes,
            envelope.message.properties.correlation_id.len) != 0) {
      g_debug ("discard >> correaltion id is not matched [%s] [%.*s]",
          correlation_id, (gint) envelope.message.properties.correlation_id.len,
          (gchar *) envelope.message.properties.correlation_id.bytes);
      continue;
    }

    g_debug ("Correlation ID : %.*s",
        (gint) envelope.message.properties.correlation_id.len,
        (gchar *) envelope.message.properties.correlation_id.bytes);
    if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
      g_debug ("Content-type: %.*s",
          (int) envelope.message.properties.content_type.len,
          (char *) envelope.message.properties.content_type.bytes);
    }

    if (*response_body != NULL) {
      g_free (*response_body);
      *response_body = NULL;
    }
    *response_body =
        g_strndup (envelope.message.body.bytes, envelope.message.body.len);

    amqp_destroy_envelope (&envelope);
    break;
  } while (1);
  ret = CHAMGE_RETURN_OK;

out:
  amqp_bytes_free (amqp_reply_queue);
  return ret;
}

static ChamgeReturn
_amqp_rpc_subscribe (amqp_connection_state_t amqp_conn, guint channel,
    const gchar * exchange_name, const gchar * queue_name, GError ** error)
{
  amqp_bytes_t amqp_queue = amqp_cstring_bytes (queue_name);
  amqp_bytes_t amqp_listen_queue = { 0 };
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  g_return_val_if_fail (amqp_conn != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (channel != 0, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (queue_name != 0, CHAMGE_RETURN_FAIL);

  if (_amqp_declare_queue (amqp_conn, queue_name, &amqp_listen_queue, error) !=
      CHAMGE_RETURN_OK) {
    return CHAMGE_RETURN_FAIL;
  }
  if (g_strcmp0 (queue_name, amqp_listen_queue.bytes) != 0) {
    g_set_error (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE,
        "declare required queue name (%s) != declared queue name (%s)",
        queue_name, (char *) amqp_listen_queue.bytes);
    goto out;
  }

  if (!amqp_queue_bind (amqp_conn, channel, amqp_queue,
          amqp_cstring_bytes (exchange_name), amqp_queue, amqp_empty_table)) {
    g_set_error (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "queue bind failure >> %s",
        _amqp_rpc_reply_string (amqp_get_rpc_reply (amqp_conn)));
    goto out;
  }

  g_debug ("binding a queue(%s) (exchange: %s, bind_key: %s)",
      queue_name, exchange_name, queue_name);

  if (amqp_basic_consume (amqp_conn, channel, amqp_queue,
          amqp_empty_bytes, 0, 0, 0, amqp_empty_table) == NULL) {
    g_set_error (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "basic consume failure >> %s",
        _amqp_rpc_reply_string (amqp_get_rpc_reply (amqp_conn)));
    goto out;
  }

  ret = CHAMGE_RETURN_OK;
out:
  amqp_bytes_free (amqp_listen_queue);
  return ret;
}

static ChamgeReturn
_validate_response (const gchar * response, const gchar * shouldbe)
{
  g_autoptr (JsonParser) parser = json_parser_new ();
  g_autoptr (GError) error = NULL;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  JsonNode *root = NULL;
  JsonObject *json_object = NULL;

  if (!json_parser_load_from_data (parser, response, strlen (response), &error)) {
    g_debug ("failed to parse body: %s", error->message);
    return ret;
  }

  root = json_parser_get_root (parser);
  json_object = json_node_get_object (root);
  if (json_object_has_member (json_object, "result")) {
    JsonNode *node = json_object_get_member (json_object, "result");
    const gchar *result = json_node_get_string (node);
    if (!g_strcmp0 (result, shouldbe)) {
      ret = CHAMGE_RETURN_OK;
    }
  }

  return ret;
}

static ChamgeReturn
chamge_amqp_edge_backend_enroll (ChamgeEdgeBackend * edge_backend)
{
  g_autofree gchar *amqp_uri = NULL;
  g_autofree gchar *amqp_enroll_q_name = NULL;
  g_autofree gchar *amqp_exchange_name = NULL;
  g_autofree gchar *response_body = NULL;
  g_autofree gchar *request_body = NULL;
  g_autoptr (GError) error = NULL;
  gint amqp_channel = 1;

  g_autofree gchar *edge_id = NULL;
  ChamgeEdge *edge = NULL;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  ChamgeAmqpEdgeBackend *self = CHAMGE_AMQP_EDGE_BACKEND (edge_backend);

  g_object_get (self, "edge", &edge, NULL);

  if (edge == NULL) {
    g_debug ("failed to get edge");
    goto out;
  }

  if (chamge_node_get_uid (CHAMGE_NODE (edge), &edge_id) != CHAMGE_RETURN_OK) {
    g_debug ("failed to get edge_id from node(parent)");
    goto out;
  }

  /* get configuration from gsetting */
  amqp_uri = g_settings_get_string (self->settings, "amqp-uri");
  g_assert_nonnull (amqp_uri);
  amqp_channel = g_settings_get_int (self->settings, "amqp-channel");
  amqp_enroll_q_name =
      g_settings_get_string (self->settings, "enroll-queue-name");
  amqp_exchange_name =
      g_settings_get_string (self->settings, "enroll-exchange-name");

  if (_amqp_rpc_login (self->amqp_conn, self->amqp_socket,
          amqp_uri, amqp_channel, &error) != CHAMGE_RETURN_OK) {
    if (error != NULL)
      g_debug ("amqp_login ERROR : %s", error->message);
    goto out;
  }

  /* TODO
   * request-body need to be filled with real request data
   */
  request_body =
      g_strdup_printf
      ("{\"method\":\"enroll\",\"deviceType\":\"edge\",\"edgeId\":\"%s\"}",
      edge_id);
  if (_amqp_rpc_request (self->amqp_conn, amqp_channel, request_body,
          amqp_exchange_name, amqp_enroll_q_name, &response_body,
          &error) != CHAMGE_RETURN_OK) {
    if (error != NULL)
      g_debug ("rpc request ERROR : %s", error->message);
    goto out;
  }
  g_debug ("received response to enroll : %s", response_body);
  if (_validate_response (response_body, "enrolled") != CHAMGE_RETURN_OK) {
    g_debug ("  received reponse must be [enrolled] but [%s]", response_body);
    goto out;
  }
  ret = CHAMGE_RETURN_OK;

out:
  return ret;
}

static ChamgeReturn
chamge_amqp_edge_backend_delist (ChamgeEdgeBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static gchar *
_process_json_message (ChamgeAmqpEdgeBackend * self, const gchar * body,
    gssize len)
{
  g_autoptr (JsonParser) parser = json_parser_new ();
  g_autoptr (GError) error = NULL;
  gchar *response = NULL;

  if (!json_parser_load_from_data (parser, body, len, &error)) {
    g_debug ("failed to parse body: %s", error->message);
    response = g_strdup ("{\"result\":\"failed to parse body\"}");
    goto out;
  }

  {
    ChamgeEdgeBackendClass *backend_class =
        CHAMGE_EDGE_BACKEND_GET_CLASS (&self->parent);
    backend_class->user_command (&self->parent, body, &response, &error);
    if (response == NULL)
      response = g_strdup ("{\"result\":\"not ok\"}");
  }

out:
  return response;
}

static gboolean
_process_amqp_message (ChamgeAmqpEdgeBackend * self)
{
  amqp_rpc_reply_t amqp_rpc_res;
  amqp_envelope_t envelope;
  g_autofree gchar *reply_queue = NULL;
  g_autofree gchar *correlation_id = NULL;
  struct timeval timeout = { 1, 0 };
  g_autofree gchar *response = NULL;

  if (!self->activated)
    return G_SOURCE_REMOVE;

  amqp_maybe_release_buffers (self->amqp_conn);

  amqp_rpc_res = amqp_consume_message (self->amqp_conn, &envelope, &timeout, 0);

  if (amqp_rpc_res.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION
      && amqp_rpc_res.library_error == AMQP_STATUS_TIMEOUT) {
    return G_SOURCE_CONTINUE;
  }

  if (amqp_rpc_res.reply_type != AMQP_RESPONSE_NORMAL) {
    g_debug ("abnormal response: %x", amqp_rpc_res.reply_type);
    goto out;
  }

  if (envelope.message.body.bytes == NULL) {
    g_debug ("no reply queue in request message");
    goto out;
  }

  g_debug ("Delivery %u, exchange %.*s routingkey %.*s",
      (unsigned) envelope.delivery_tag, (int) envelope.exchange.len,
      (char *) envelope.exchange.bytes, (int) envelope.routing_key.len,
      (char *) envelope.routing_key.bytes);

  /* if the content-type isn't 'application/json', let's drop */
  if (!(envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG)
      || strlen (DEFAULT_CONTENT_TYPE) !=
      envelope.message.properties.content_type.len
      || g_ascii_strncasecmp (DEFAULT_CONTENT_TYPE,
          envelope.message.properties.content_type.bytes,
          envelope.message.properties.content_type.len)
      ) {
    g_debug ("invalid content type %s",
        (gchar *) envelope.message.properties.content_type.bytes);

    goto out;
  }

  if ((envelope.message.properties._flags & AMQP_BASIC_REPLY_TO_FLAG) &&
      envelope.message.properties.reply_to.len > 0 &&
      (strlen (envelope.message.properties.reply_to.bytes) >=
          envelope.message.properties.reply_to.len)) {
    reply_queue = g_strndup (envelope.message.properties.reply_to.bytes,
        envelope.message.properties.reply_to.len);
  } else {
    g_debug ("not exist replay_to in request message's property");
    goto out;
  }

  if ((envelope.message.properties._flags & AMQP_BASIC_CORRELATION_ID_FLAG) &&
      envelope.message.properties.correlation_id.len > 0 &&
      (strlen (envelope.message.properties.correlation_id.bytes) >=
          envelope.message.properties.correlation_id.len)) {
    correlation_id =
        g_strndup (envelope.message.properties.correlation_id.bytes,
        envelope.message.properties.correlation_id.len);
  }

  g_debug ("Content-type: %.*s, replay_to : %s, correlation_id : %s",
      (int) envelope.message.properties.content_type.len,
      (char *) envelope.message.properties.content_type.bytes,
      reply_queue, correlation_id);

  response = _process_json_message (self, envelope.message.body.bytes,
      envelope.message.body.len);

  if (response == NULL) {
    g_error ("response is NULL. response should be non null");
    goto out;
  }
  {
    amqp_basic_properties_t amqp_props;

    memset (&amqp_props, 0, sizeof (amqp_basic_properties_t));
    amqp_props._flags =
        AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    amqp_props.content_type = amqp_cstring_bytes (DEFAULT_CONTENT_TYPE);
    amqp_props.delivery_mode = 2;       /* persistent delivery mode */

    if (correlation_id != NULL) {
      amqp_props._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
      amqp_props.correlation_id = amqp_cstring_bytes (correlation_id);
    }

    /*
     * publish
     */
    g_debug ("publishing to [%s] channel [%d]", reply_queue, envelope.channel);
    g_debug ("      correlation id [%s] body [%s]", correlation_id, response);
    amqp_basic_publish (self->amqp_conn, envelope.channel,
        amqp_cstring_bytes (""), amqp_cstring_bytes (reply_queue), 0, 0,
        &amqp_props, amqp_cstring_bytes (response));
  }

out:
  amqp_destroy_envelope (&envelope);

  return G_SOURCE_CONTINUE;
}

static ChamgeReturn
chamge_amqp_edge_backend_activate (ChamgeEdgeBackend * edge_backend)
{
  g_autofree gchar *amqp_enroll_q_name = NULL;
  g_autofree gchar *amqp_exchange_name = NULL;
  g_autofree gchar *request_body = NULL;
  g_autofree gchar *response_body = NULL;
  guint amqp_channel = 1;

  g_autofree gchar *edge_id = NULL;
  ChamgeEdge *edge = NULL;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;
  g_autoptr (GError) error = NULL;

  ChamgeAmqpEdgeBackend *self = CHAMGE_AMQP_EDGE_BACKEND (edge_backend);

  g_object_get (self, "edge", &edge, NULL);

  if (edge == NULL) {
    g_error ("failed to get edge");
    goto out;
  }

  if (chamge_node_get_uid (CHAMGE_NODE (edge), &edge_id) != CHAMGE_RETURN_OK) {
    g_error ("failed to get edge_id from node(parent)");
    goto out;
  }

  amqp_channel = g_settings_get_int (self->settings, "amqp-channel");
  amqp_enroll_q_name =
      g_settings_get_string (self->settings, "enroll-queue-name");
  amqp_exchange_name =
      g_settings_get_string (self->settings, "enroll-exchange-name");

  /* send activate */
  request_body =
      g_strdup_printf
      ("{\"method\":\"activate\",\"deviceType\":\"edge\",\"edgeId\":\"%s\"}",
      edge_id);
  if (_amqp_rpc_request (self->amqp_conn, amqp_channel, request_body,
          amqp_exchange_name, amqp_enroll_q_name, &response_body,
          &error) != CHAMGE_RETURN_OK) {
    if (error != NULL)
      g_debug ("rpc_request ERROR : %s", error->message);
    goto out;
  }

  g_debug ("received response to activate: %s", response_body);

  if (_validate_response (response_body, "activated") != CHAMGE_RETURN_OK) {
    g_debug ("  received reponse must be [activated], but [%s]", response_body);
    goto out;
  }

  self->activated = TRUE;

  /* subscribe queue (queue name: edgeId) for streaming start */
  if (_amqp_rpc_subscribe (self->amqp_conn, amqp_channel, amqp_exchange_name,
          edge_id, &error) == CHAMGE_RETURN_FAIL) {
    g_debug ("rpc_subscribe ERROR [ch:%d][exchange:%s][edge_id:%s]",
        amqp_channel, amqp_exchange_name, edge_id);
    if (error != NULL)
      g_debug ("    %s", error->message);
    goto out;
  }
  /* process amqp message that comes from Mujachi */
  self->process_id = g_idle_add ((GSourceFunc) _process_amqp_message, self);
  ret = CHAMGE_RETURN_OK;

out:
  return ret;
}

static ChamgeReturn
chamge_amqp_edge_backend_deactivate (ChamgeEdgeBackend * edge_backend)
{
  ChamgeAmqpEdgeBackend *self = CHAMGE_AMQP_EDGE_BACKEND (edge_backend);

  if (self->process_id != 0) {
    g_source_remove (self->process_id);
    self->process_id = 0;
  }

  return CHAMGE_RETURN_OK;
}


static gchar *
chamge_amqp_edge_backend_request_target_uri (ChamgeEdgeBackend * self,
    GError ** error)
{
  /* TODO
   * Need to return real data that is gotten from server
   */
  g_autofree gchar *target_uri = g_strdup ("srt://localhost:8888");
  return g_steal_pointer (&target_uri);
}

static void
chamge_amqp_edge_backend_dispose (GObject * object)
{
  ChamgeAmqpEdgeBackend *self = CHAMGE_AMQP_EDGE_BACKEND (object);

  if (self->process_id != 0) {
    g_source_remove (self->process_id);
    self->process_id = 0;
  }

  if (self->amqp_conn != NULL) {
    amqp_destroy_connection (self->amqp_conn);
    self->amqp_conn = NULL;
  }

  G_OBJECT_CLASS (chamge_amqp_edge_backend_parent_class)->dispose (object);
}


static void
chamge_amqp_edge_backend_class_init (ChamgeAmqpEdgeBackendClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ChamgeEdgeBackendClass *backend_class = CHAMGE_EDGE_BACKEND_CLASS (klass);

  object_class->dispose = chamge_amqp_edge_backend_dispose;

  backend_class->enroll = chamge_amqp_edge_backend_enroll;
  backend_class->delist = chamge_amqp_edge_backend_delist;
  backend_class->activate = chamge_amqp_edge_backend_activate;
  backend_class->deactivate = chamge_amqp_edge_backend_deactivate;
  backend_class->request_target_uri =
      chamge_amqp_edge_backend_request_target_uri;

}

static void
chamge_amqp_edge_backend_init (ChamgeAmqpEdgeBackend * self)
{
  self->settings = g_settings_new (AMQP_EDGE_BACKEND_SCHEMA_ID);
  g_assert_nonnull (self->settings);

  self->amqp_conn = amqp_new_connection ();
  self->amqp_socket = amqp_tcp_socket_new (self->amqp_conn);

  g_assert_nonnull (self->amqp_socket);
}
