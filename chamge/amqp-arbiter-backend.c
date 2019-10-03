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

#include "amqp-arbiter-backend.h"
#include "glib-compat.h"

#include <gio/gio.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <json-glib/json-glib.h>
#include <string.h>

#define AMQP_ARBITER_BACKEND_SCHEMA_ID "org.hwangsaeul.Chamge1.Arbiter.AMQP"
#define DEFAULT_CONTENT_TYPE "application/json"

struct _ChamgeAmqpArbiterBackend
{
  ChamgeArbiterBackend parent;
  GSettings *settings;

  amqp_connection_state_t amqp_conn;
  amqp_socket_t *amqp_socket;

  gboolean activated;
  guint process_id;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeAmqpArbiterBackend, chamge_amqp_arbiter_backend, CHAMGE_TYPE_ARBITER_BACKEND)
/* *INDENT-ON* */

inline static const gchar *
_amqp_get_rpc_reply_string (amqp_rpc_reply_t r)
{
  if (r.reply_type == AMQP_RESPONSE_NORMAL) {
    return "response normal";
  } else if (r.reply_type == AMQP_RESPONSE_NONE) {
    return "reply type is missing";
  } else if (r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
    return amqp_error_string2 (r.library_error);
  } else if (r.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
    int ret = 0;
    static char str[512] = { 0 };

    if (r.reply.id == AMQP_CONNECTION_CLOSE_METHOD) {
      amqp_connection_close_t *msg =
          (amqp_connection_close_t *) r.reply.decoded;
      ret =
          g_snprintf (str, sizeof (str),
          "server connection error %d, message: %.*s", msg->reply_code,
          (int) msg->reply_text.len, (char *) msg->reply_text.bytes);
    } else if (r.reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
      amqp_channel_close_t *msg = (amqp_channel_close_t *) r.reply.decoded;
      ret =
          g_snprintf (str, sizeof (str),
          "server channel error %d, message: %.*s", msg->reply_code,
          (int) msg->reply_text.len, (char *) msg->reply_text.bytes);
    } else {
      ret =
          g_snprintf (str, sizeof (str),
          "unknown server error, method id 0x%08X", r.reply.id);
    }
    return ret >= 0 ? str : NULL;
  }
  g_error ("rpc reply type is abnormal. 0x%x", r.reply_type);
  return "rpc replay type is abnormal";
}

static ChamgeReturn
chamge_amqp_arbiter_backend_enroll (ChamgeArbiterBackend * arbiter_backend)
{
  g_autofree gchar *amqp_uri = NULL;
  g_autofree gchar *amqp_enroll_q_name = NULL;
  g_autofree gchar *amqp_exchange_name = NULL;
  gint amqp_channel;

  struct amqp_connection_info amqp_c_info;
  amqp_bytes_t queue;
  amqp_queue_declare_ok_t *amqp_r = NULL;

  ChamgeAmqpArbiterBackend *self =
      CHAMGE_AMQP_ARBITER_BACKEND (arbiter_backend);

  amqp_uri = g_settings_get_string (self->settings, "amqp-uri");
  g_assert_nonnull (amqp_uri);

  amqp_channel = g_settings_get_int (self->settings, "amqp-channel");

  amqp_parse_url (amqp_uri, &amqp_c_info);

  g_debug ("parsed amqp uri (host: %s, vhost: %s)", amqp_c_info.host,
      amqp_c_info.vhost);

  amqp_socket_open (self->amqp_socket, amqp_c_info.host, amqp_c_info.port);

  /*
   * TODO: The authentication method should be EXTERNAL
   * if we don't want to share user/passwd
   */
  amqp_login (self->amqp_conn, amqp_c_info.vhost, 0, 131072, 0,
      AMQP_SASL_METHOD_PLAIN, amqp_c_info.user, amqp_c_info.password);
  amqp_channel_open (self->amqp_conn, amqp_channel);
  amqp_get_rpc_reply (self->amqp_conn);

  amqp_enroll_q_name =
      g_settings_get_string (self->settings, "enroll-queue-name");
  queue = amqp_cstring_bytes (amqp_enroll_q_name);
  amqp_r =
      amqp_queue_declare (self->amqp_conn, amqp_channel, queue, 0, 0, 0, 1,
      amqp_empty_table);
  amqp_get_rpc_reply (self->amqp_conn);

  g_debug ("declaring a queue : %s", (gchar *) amqp_r->queue.bytes);

  amqp_exchange_name =
      g_settings_get_string (self->settings, "enroll-exchange-name");

  amqp_queue_bind (self->amqp_conn, amqp_channel, amqp_r->queue,
      amqp_cstring_bytes (amqp_exchange_name), queue, amqp_empty_table);

  g_debug ("binding a queue (exchange: %s, bind_key: %s)", amqp_exchange_name,
      amqp_enroll_q_name);

  /* FIXME: A empty consuming MUST be called after binding, but it blocks until
     a message is coming. This consuming call should be revised to accept timeout */
  amqp_basic_consume (self->amqp_conn, amqp_channel, amqp_r->queue,
      amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  amqp_get_rpc_reply (self->amqp_conn);

  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_amqp_arbiter_backend_delist (ChamgeArbiterBackend * arbiter_backend)
{
  return CHAMGE_RETURN_OK;
}

static void
_handle_edge_enroll (ChamgeAmqpArbiterBackend * self, const gchar * edge_id)
{
  ChamgeArbiterBackendClass *klass = CHAMGE_ARBITER_BACKEND_GET_CLASS (self);

  klass->edge_enrolled (CHAMGE_ARBITER_BACKEND (self), edge_id);
}

static void
_handle_edge_activate (ChamgeAmqpArbiterBackend * self, const gchar * edge_id)
{
  /* TODO : check whether manager need to handle activate request of edge */
}

static void
_handle_edge_deactivate (ChamgeAmqpArbiterBackend * self, const gchar * edge_id)
{
  ChamgeArbiterBackendClass *klass = CHAMGE_ARBITER_BACKEND_GET_CLASS (self);

  klass->edge_delisted (CHAMGE_ARBITER_BACKEND (self), edge_id);
}


static gchar *
_process_json_message (ChamgeAmqpArbiterBackend * self, const gchar * body,
    gssize len)
{
  g_autoptr (JsonParser) parser = json_parser_new ();
  g_autoptr (GError) error = NULL;

  JsonNode *root = NULL;
  JsonObject *json_object = NULL;

  if (!json_parser_load_from_data (parser, body, len, &error)) {
    g_debug ("failed to parse body: %s", error->message);
    return g_strdup_printf ("{\"result\":\"failed to parse body\"}");
  }

  root = json_parser_get_root (parser);
  json_object = json_node_get_object (root);
  if (json_object_has_member (json_object, "method")) {
    JsonNode *node = json_object_get_member (json_object, "method");
    const gchar *method = json_node_get_string (node);
    const gchar *edge_id = NULL;
    gchar *response = NULL;

    if (json_object_has_member (json_object, "edgeId")) {
      node = json_object_get_member (json_object, "edgeId");
      edge_id = json_node_get_string (node);
      if (edge_id == NULL)
        return g_strdup ("{\"result\":\"edge_id does not exist\"}");
    }
    g_debug ("method: %s", method);

    if (!g_strcmp0 (method, "enroll")) {
      _handle_edge_enroll (self, edge_id);
      response = g_strdup ("{\"result\":\"enrolled\"}");
    } else if (!g_strcmp0 (method, "activate")) {
      _handle_edge_activate (self, edge_id);
      response = g_strdup ("{\"result\":\"activated\"}");
    } else if (!g_strcmp0 (method, "deactdivate")) {
      _handle_edge_deactivate (self, edge_id);
      response = g_strdup ("{\"result\":\"deactivated\"}");
    } else {
      response =
          g_strdup_printf ("{\"result\":\"method(%s) is not supported\"}",
          method);
    }
    return response;
  }

  return g_strdup ("{\"result\":\"method does not exist\"}");
}

static gboolean
_process_amqp_message (ChamgeAmqpArbiterBackend * self)
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
chamge_amqp_arbiter_backend_activate (ChamgeArbiterBackend * arbiter_backend)
{
  ChamgeAmqpArbiterBackend *self =
      CHAMGE_AMQP_ARBITER_BACKEND (arbiter_backend);

  g_debug ("waiting for message");

  self->process_id = g_idle_add ((GSourceFunc) _process_amqp_message, self);
  self->activated = TRUE;

  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_amqp_arbiter_backend_deactivate (ChamgeArbiterBackend * arbiter_backend)
{
  ChamgeAmqpArbiterBackend *self =
      CHAMGE_AMQP_ARBITER_BACKEND (arbiter_backend);

  self->activated = FALSE;

  if (self->process_id != 0) {
    g_source_remove (self->process_id);
    self->process_id = 0;
  }

  return CHAMGE_RETURN_OK;
}

gchar *
_get_queue_name (const gchar * request)
{
  g_autoptr (JsonParser) parser = json_parser_new ();
  g_autoptr (GError) error = NULL;
  const gchar *name = NULL;

  JsonNode *root = NULL;
  JsonObject *json_object = NULL;

  if (!json_parser_load_from_data (parser, request, strlen (request), &error)) {
    g_debug ("failed to parse body: %s", error->message);
    return NULL;
  }

  root = json_parser_get_root (parser);
  json_object = json_node_get_object (root);
  if (json_object_has_member (json_object, "to")) {
    JsonNode *node = json_object_get_member (json_object, "to");
    name = json_node_get_string (node);
    return g_strdup (name);
  }
  return NULL;
}

static ChamgeReturn
_handle_rpc_user_command (amqp_connection_state_t amqp_conn, gint channel,
    const gchar * request, const gchar * exchange, gchar ** response,
    GError ** error)
{
  amqp_bytes_t amqp_reply_queue = { 0 };
  amqp_basic_properties_t amqp_props = { 0 };
  amqp_queue_declare_ok_t *amqp_declar_r = NULL;
  g_autofree gchar *correlation_id = NULL;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;
  g_autofree gchar *queue_name = NULL;

  g_return_val_if_fail (amqp_conn != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (channel != 0, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (request != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (response != NULL, CHAMGE_RETURN_FAIL);

  queue_name = _get_queue_name (request);
  if (queue_name == NULL) {
    g_set_error_literal (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_MISSING_PARAMETER,
        "json parsing failure to get \"to\"");
    return CHAMGE_RETURN_FAIL;
  }
  amqp_cstring_bytes (queue_name);

  g_debug ("queue name : %s ", queue_name);
  amqp_declar_r =
      amqp_queue_declare (amqp_conn, 1, amqp_empty_bytes, 0, 0, 0,
      1, amqp_empty_table);
  if (amqp_declar_r == NULL) {
    g_set_error (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "declare queue failure >> %s",
        _amqp_get_rpc_reply_string (amqp_get_rpc_reply (amqp_conn)));
    return CHAMGE_RETURN_FAIL;
  }
  amqp_reply_queue.bytes = amqp_declar_r->queue.bytes;
  amqp_reply_queue.len = amqp_declar_r->queue.len;

  g_debug ("queue for reply : %s", (gchar *) amqp_reply_queue.bytes);

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

  /* wait an answer */
  if (amqp_basic_consume (amqp_conn, 1, amqp_reply_queue, amqp_empty_bytes, 0,
          1, 0, amqp_empty_table) == NULL) {
    g_set_error (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "basic consume failure >> %s",
        _amqp_get_rpc_reply_string (amqp_get_rpc_reply (amqp_conn)));
    goto out;
  }


  /* wait an answer */
  do {
    amqp_rpc_reply_t amqp_r;
    amqp_envelope_t envelope;
    struct timeval timeout = { 10, 0 };

    amqp_maybe_release_buffers (amqp_conn);

    g_debug ("Wait for response message from [%s]",
        (gchar *) amqp_reply_queue.bytes);
    amqp_r = amqp_consume_message (amqp_conn, &envelope, &timeout, 0);

    if (amqp_r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION
        && amqp_r.library_error == AMQP_STATUS_TIMEOUT) {
      g_set_error_literal (error, CHAMGE_BACKEND_ERROR,
          CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "consume msg timeout");
      break;
    }

    if (amqp_r.reply_type != AMQP_RESPONSE_NORMAL) {
      g_set_error (error, CHAMGE_BACKEND_ERROR,
          CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "consume msg failure >> %s",
          _amqp_get_rpc_reply_string (amqp_r));
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
        (int) envelope.message.properties.correlation_id.len,
        (char *) envelope.message.properties.correlation_id.bytes);
    if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
      g_debug ("Content-type: %.*s",
          (int) envelope.message.properties.content_type.len,
          (char *) envelope.message.properties.content_type.bytes);
    }

    if (*response != NULL) {
      g_free (*response);
      *response = NULL;
    }
    *response =
        g_strndup (envelope.message.body.bytes, envelope.message.body.len);

    amqp_destroy_envelope (&envelope);
    break;
  } while (1);

  ret = CHAMGE_RETURN_OK;
out:
  return ret;
}

static ChamgeReturn
chamge_amqp_arbiter_backend_user_command (ChamgeArbiterBackend *
    arbiter_backend, const gchar * cmd, gchar ** out, GError ** error)
{
  ChamgeAmqpArbiterBackend *self =
      CHAMGE_AMQP_ARBITER_BACKEND (arbiter_backend);
  g_autofree gchar *amqp_uri = NULL;
  struct amqp_connection_info connection_info;
  amqp_rpc_reply_t amqp_r;
  amqp_connection_state_t amqp_conn;
  amqp_socket_t *amqp_socket = NULL;

  g_autofree gchar *amqp_exchange_name = NULL;
  gint amqp_channel = 0;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  amqp_uri = g_settings_get_string (self->settings, "amqp-uri");
  g_assert_nonnull (amqp_uri);

  if (amqp_parse_url (amqp_uri, &connection_info) != AMQP_STATUS_OK) {
    g_set_error_literal (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_INVALID_PARAMETER, "url pasring failure");
    goto out;
  }

  amqp_channel = g_settings_get_int (self->settings, "amqp-channel");
  amqp_exchange_name =
      g_settings_get_string (self->settings, "enroll-exchange-name");

  amqp_conn = amqp_new_connection ();
  amqp_socket = amqp_tcp_socket_new (amqp_conn);
  g_assert_nonnull (amqp_socket);

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
        _amqp_get_rpc_reply_string (amqp_r));
    goto out;
  }
  if (!amqp_channel_open (amqp_conn, amqp_channel)) {
    g_set_error (error, CHAMGE_BACKEND_ERROR,
        CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "channel open failure >> %s",
        _amqp_get_rpc_reply_string (amqp_get_rpc_reply (amqp_conn)));
    goto out;
  }

  g_debug ("success to login %s:%d/%s id[%s] pwd[%s]",
      connection_info.host, connection_info.port,
      connection_info.vhost, connection_info.user, connection_info.password);

  ret =
      _handle_rpc_user_command (amqp_conn, amqp_channel, cmd,
      amqp_exchange_name, out, error);
  if (ret != CHAMGE_RETURN_OK && *error != NULL) {
    g_debug ("rpc request failure >> %s", (*error)->message);
    goto out;
  }

  if (amqp_conn != NULL) {
    gint destroy_ret = 0;
    amqp_r = amqp_channel_close (amqp_conn, amqp_channel, AMQP_REPLY_SUCCESS);
    if (amqp_r.reply_type != AMQP_RESPONSE_NORMAL) {
      g_set_error (error, CHAMGE_BACKEND_ERROR,
          CHAMGE_BACKEND_ERROR_OPERATION_FAILURE, "channel close failure >> %s",
          _amqp_get_rpc_reply_string (amqp_r));
      ret = CHAMGE_RETURN_FAIL;
      goto out;
    }
    amqp_r = amqp_connection_close (amqp_conn, AMQP_REPLY_SUCCESS);
    if (amqp_r.reply_type != AMQP_RESPONSE_NORMAL) {
      g_set_error (error, CHAMGE_BACKEND_ERROR,
          CHAMGE_BACKEND_ERROR_OPERATION_FAILURE,
          "connection close failure >> %s",
          _amqp_get_rpc_reply_string (amqp_r));
      ret = CHAMGE_RETURN_FAIL;
      goto out;
    }
    destroy_ret = amqp_destroy_connection (amqp_conn);
    if (destroy_ret < 0) {
      g_set_error (error, CHAMGE_BACKEND_ERROR,
          CHAMGE_BACKEND_ERROR_OPERATION_FAILURE,
          "connection destroy failure >> %s", amqp_error_string2 (destroy_ret));
      ret = CHAMGE_RETURN_FAIL;
      goto out;
    }
  }

out:
  return ret;
}


static void
chamge_amqp_arbiter_backend_approve (ChamgeArbiterBackend * arbiter_backend,
    const gchar * edge_id)
{
  ChamgeAmqpArbiterBackend *self =
      CHAMGE_AMQP_ARBITER_BACKEND (arbiter_backend);

  /* TODO: publish approval message */
  g_debug ("approved (id: %s)", edge_id);
}

static void
chamge_amqp_arbiter_backend_dispose (GObject * object)
{
  ChamgeAmqpArbiterBackend *self = CHAMGE_AMQP_ARBITER_BACKEND (object);

  if (self->process_id != 0) {
    g_source_remove (self->process_id);
    self->process_id = 0;
  }

  g_clear_object (&self->settings);

  if (self->amqp_conn != NULL) {
    amqp_destroy_connection (self->amqp_conn);
    self->amqp_conn = NULL;
  }

  G_OBJECT_CLASS (chamge_amqp_arbiter_backend_parent_class)->dispose (object);
}

static void
chamge_amqp_arbiter_backend_class_init (ChamgeAmqpArbiterBackendClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ChamgeArbiterBackendClass *backend_class =
      CHAMGE_ARBITER_BACKEND_CLASS (klass);

  object_class->dispose = chamge_amqp_arbiter_backend_dispose;

  backend_class->enroll = chamge_amqp_arbiter_backend_enroll;
  backend_class->delist = chamge_amqp_arbiter_backend_delist;
  backend_class->activate = chamge_amqp_arbiter_backend_activate;
  backend_class->deactivate = chamge_amqp_arbiter_backend_deactivate;
  backend_class->user_command = chamge_amqp_arbiter_backend_user_command;
  backend_class->approve = chamge_amqp_arbiter_backend_approve;
}

static void
chamge_amqp_arbiter_backend_init (ChamgeAmqpArbiterBackend * self)
{
  /* TODO: load settings from schema source */
  self->settings = g_settings_new (AMQP_ARBITER_BACKEND_SCHEMA_ID);

  self->amqp_conn = amqp_new_connection ();
  self->amqp_socket = amqp_tcp_socket_new (self->amqp_conn);

  g_assert_nonnull (self->amqp_socket);
}
