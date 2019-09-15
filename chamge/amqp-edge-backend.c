/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "amqp-edge-backend.h"

#include <gio/gio.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <json-glib/json-glib.h>

#define AMQP_EDGE_BACKEND_SCHEMA_ID "org.hwangsaeul.Chamge1.Edge.AMQP"
#define DEFAULT_CONTENT_TYPE "application/json"
#define DEFAULT_CONTENT_TYPE_LEN 16

typedef enum
{
  CHAMGE_AMQP_EDGE_BACKEND_NULL = 0,
  CHAMGE_AMQP_EDGE_BACKEND_ENROLLED,
  CHAMGE_AMQP_EDGE_BACKEND_ACTIVATED,
  CHAMGE_AMQP_EDGE_BACKEND_PLAYING,
  CHMAGE_AMQP_EDGE_BACKEND_ERROR
} ChamgeEdgeStatus;

struct _ChamgeAmqpEdgeBackend
{
  ChamgeEdgeBackend parent;
  GSettings *settings;

  amqp_connection_state_t amqp_conn;
  amqp_socket_t *amqp_socket;

  ChamgeEdgeStatus state;
  guint process_id;
  gchar *uid;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeAmqpEdgeBackend, chamge_amqp_edge_backend, CHAMGE_TYPE_EDGE_BACKEND)
/* *INDENT-ON* */

#define RETURN_FAIL_IF_TRUE(cond, format, args...) { \
      if (cond) { \
        g_debug (format \
          ", error(%s:%d) : %s => %d", ##args, __func__, __LINE__, #cond, cond); \
        return CHAMGE_RETURN_FAIL; \
      } \
    }

static ChamgeReturn
_amqp_rpc_login (amqp_connection_state_t amqp_conn, amqp_socket_t * amqp_socket,
    const gchar * url, guint channel)
{
  amqp_rpc_reply_t amqp_r;
  struct amqp_connection_info connection_info;
  g_autofree gchar *str = g_strdup (url);

  amqp_parse_url (str, &connection_info);

  g_return_val_if_fail (connection_info.host != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (connection_info.vhost != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (connection_info.port != 0, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (channel != 0, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (connection_info.user != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (connection_info.password != NULL, CHAMGE_RETURN_FAIL);

  RETURN_FAIL_IF_TRUE (amqp_socket_open (amqp_socket,
          connection_info.host, connection_info.port) < 0,
      "error in socket open");

  /* TODO: The authentication method should be EXTERNAL
   * if we don't want to share user/passwd
   */
  amqp_login (amqp_conn, connection_info.vhost, 0, 131072, 0,
      AMQP_SASL_METHOD_PLAIN, connection_info.user, connection_info.password);
  amqp_channel_open (amqp_conn, channel);
  amqp_r = amqp_get_rpc_reply (amqp_conn);
  RETURN_FAIL_IF_TRUE (amqp_r.reply_type != AMQP_RESPONSE_NORMAL,
      "error in channel open");

  g_debug ("success to login %s:%d/%s id[%s] pwd[%s]",
      connection_info.host, connection_info.port,
      connection_info.vhost, connection_info.user, connection_info.password);
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
_amqp_declare_queue (const amqp_connection_state_t conn,
    const gchar * queue_name, amqp_bytes_t * queue)
{
  amqp_queue_declare_ok_t *amqp_r = NULL;
  g_autofree gchar *response_queue_name = NULL;

  if (queue_name == NULL) {
    amqp_r = amqp_queue_declare (conn, 1, amqp_empty_bytes, 0, 0, 0, 1,
        amqp_empty_table);
  } else {
    amqp_r =
        amqp_queue_declare (conn, 1, amqp_cstring_bytes (queue_name), 0, 0, 0,
        1, amqp_empty_table);
  }

  RETURN_FAIL_IF_TRUE (amqp_r == NULL, "error in amqp_queue_declare");
  amqp_get_rpc_reply (conn);

  g_assert_nonnull (amqp_r->queue.bytes);
  response_queue_name = g_strdup (amqp_r->queue.bytes);
  if (queue != NULL)
    *queue = amqp_cstring_bytes (g_steal_pointer (&response_queue_name));

  g_debug ("create private reply_to queue : name -> %s",
      (gchar *) queue->bytes);
  return CHAMGE_RETURN_OK;
}

static void
_amqp_property_set (amqp_basic_properties_t * const props,
    const guint correlation_id, const gchar * reply_queue_name)
{
  g_autofree gchar *correlation_id_str = NULL;
  g_autofree gchar *content_type_str = g_strdup (DEFAULT_CONTENT_TYPE);

  props->_flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
  props->content_type =
      amqp_cstring_bytes (g_steal_pointer (&content_type_str));
  props->delivery_mode = 2;     /* persistent delivery mode */

  if (reply_queue_name != NULL) {
    g_autofree gchar *reply_to_queue = (gchar *) g_strdup (reply_queue_name);
    g_assert_nonnull (reply_to_queue);
    props->reply_to = amqp_cstring_bytes (g_steal_pointer (&reply_to_queue));
    props->_flags |= AMQP_BASIC_REPLY_TO_FLAG;
  } else {
    props->reply_to.bytes = NULL;
    props->reply_to.bytes = 0;
  }

  if (correlation_id != 0) {
    props->_flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
    correlation_id_str = g_strdup_printf ("%u", correlation_id);
    props->correlation_id =
        amqp_cstring_bytes (g_steal_pointer (&correlation_id_str));
  } else {
    props->correlation_id.bytes = NULL;
    props->correlation_id.len = 0;
  }

  g_debug ("content type : %s , correlation id : %s",
      (gchar *) props->content_type.bytes,
      (gchar *) props->correlation_id.bytes);
}

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
_amqp_get_response (const amqp_connection_state_t conn,
    const amqp_bytes_t reply_to_queue, gchar ** const response_body,
    guint * const size)
{
  amqp_rpc_reply_t amqp_r;
  amqp_envelope_t envelope;

  amqp_basic_consume (conn, 1, reply_to_queue, amqp_empty_bytes, 0, 1, 0,
      amqp_empty_table);
  amqp_get_rpc_reply (conn);

  amqp_maybe_release_buffers (conn);

  g_debug ("Wait for response message from [%s]",
      (gchar *) reply_to_queue.bytes);
  amqp_r = amqp_consume_message (conn, &envelope, NULL, 0);

  if (AMQP_RESPONSE_NORMAL != amqp_r.reply_type) {
    g_error ("response is not normal. >> %s", _amqp_rpc_reply_string (amqp_r));
    return CHAMGE_RETURN_FAIL;
  }

  g_debug ("Delivery %u, exchange %.*s routingkey %.*s",
      (unsigned) envelope.delivery_tag, (int) envelope.exchange.len,
      (char *) envelope.exchange.bytes, (int) envelope.routing_key.len,
      (char *) envelope.routing_key.bytes);

  if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
    g_debug ("Content-type: %.*s",
        (int) envelope.message.properties.content_type.len,
        (char *) envelope.message.properties.content_type.bytes);
  }

  *response_body =
      g_strndup (envelope.message.body.bytes, envelope.message.body.len);
  *size = envelope.message.body.len;

  amqp_destroy_envelope (&envelope);

  return CHAMGE_RETURN_OK;
}


static ChamgeReturn
_amqp_rpc_request (amqp_connection_state_t amqp_conn, guint channel,
    const gchar * request, const gchar * exchange, const gchar * queue_name,
    gchar ** const response_body, guint * const size)
{
  amqp_bytes_t amqp_reply_queue;
  amqp_basic_properties_t amqp_props;
  //gchar *response_body;

  g_return_val_if_fail (queue_name != NULL, CHAMGE_RETURN_FAIL);

  /* create private replay_to_queue and queue name is random that is supplied from amqp server */
  if (_amqp_declare_queue (amqp_conn, NULL,
          &amqp_reply_queue) != CHAMGE_RETURN_OK)
    return CHAMGE_RETURN_FAIL;

  g_debug ("declare a queue for reply : %s", (gchar *) amqp_reply_queue.bytes);

  /* send the message */
  _amqp_property_set (&amqp_props, channel, (gchar *) amqp_reply_queue.bytes);

  /* send requst to queue_name */
  {
    gint r = amqp_basic_publish (amqp_conn, channel,
        exchange ==
        NULL ? amqp_cstring_bytes ("") : amqp_cstring_bytes (exchange),
        amqp_cstring_bytes (queue_name), 0, 0, &amqp_props,
        amqp_cstring_bytes (request));
    if (r < 0) {
      g_error ("publish can not be done >> %s", amqp_error_string2 (r));
      return CHAMGE_RETURN_FAIL;
    }
    g_debug ("published to queue[%s], exchange[%s]", queue_name, exchange);
    g_debug ("    content-type[%.*s], body[%s]",
        (int) amqp_props.content_type.len,
        (gchar *) amqp_props.content_type.bytes, request);
  }

  /* wait an answer */
  return _amqp_get_response (amqp_conn, amqp_reply_queue, response_body, size);
}

static ChamgeReturn
_amqp_rpc_subscribe (amqp_connection_state_t amqp_conn, guint channel,
    const gchar * exchange_name, const gchar * queue_name)
{
  amqp_bytes_t amqp_queue = amqp_cstring_bytes (queue_name);
  amqp_bytes_t amqp_listen_queue;

  g_return_val_if_fail (amqp_conn, CHAMGE_RETURN_FAIL);
  if (_amqp_declare_queue (amqp_conn, queue_name, &amqp_listen_queue) !=
      CHAMGE_RETURN_OK) {
    return CHAMGE_RETURN_FAIL;
  }
  if (g_strcmp0 (queue_name, amqp_listen_queue.bytes) != 0) {
    g_error ("declare required queue name (%s) != declared queue name (%s)",
        queue_name, (char *) amqp_listen_queue.bytes);
    return CHAMGE_RETURN_FAIL;
  }

  amqp_queue_bind (amqp_conn, channel, amqp_queue,
      amqp_cstring_bytes (exchange_name), amqp_queue, amqp_empty_table);

  g_debug ("binding a queue(%s) (exchange: %s, bind_key: %s)",
      queue_name, exchange_name, queue_name);

  amqp_basic_consume (amqp_conn, channel, amqp_queue,
      amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  RETURN_FAIL_IF_TRUE ((amqp_get_rpc_reply (amqp_conn)).reply_type !=
      AMQP_RESPONSE_NORMAL, "error in queue consume");

  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
_amqp_rpc_listen (amqp_connection_state_t amqp_conn, gchar ** const body,
    guint * size, gchar ** const reply_to, guint * const correlation_id,
    guint * const channel)
{
  amqp_rpc_reply_t amqp_r;
  amqp_envelope_t envelope;

  struct timeval timeout = { 1, 0 };

  amqp_maybe_release_buffers (amqp_conn);

  amqp_r = amqp_consume_message (amqp_conn, &envelope, &timeout, 0);

  if (amqp_r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION
      && amqp_r.library_error == AMQP_STATUS_TIMEOUT) {
    return CHAMGE_RETURN_FAIL;
  }

  if (amqp_r.reply_type != AMQP_RESPONSE_NORMAL) {
    g_debug ("abnormal response: %x", amqp_r.reply_type);
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
  g_debug ("    correlation id : %s, content type : %s",
      (gchar *) envelope.message.properties.correlation_id.bytes,
      (gchar *) envelope.message.properties.content_type.bytes);

  /* if the content-type isn't 'application/json', let's drop */
  if (!(envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG)
      || strnlen (DEFAULT_CONTENT_TYPE, DEFAULT_CONTENT_TYPE_LEN) !=
      envelope.message.properties.content_type.len
      || g_ascii_strncasecmp (DEFAULT_CONTENT_TYPE,
          envelope.message.properties.content_type.bytes,
          envelope.message.properties.content_type.len)
      ) {
    g_debug ("invalid content type %s",
        (gchar *) envelope.message.properties.content_type.bytes);

    goto out;
  }

  if (envelope.message.properties.reply_to.len > 0 &&
      (strnlen (envelope.message.properties.reply_to.bytes,
              envelope.message.properties.reply_to.len) >=
          envelope.message.properties.reply_to.len)) {
    *reply_to =
        g_strndup (envelope.message.properties.reply_to.bytes,
        envelope.message.properties.reply_to.len);
    g_debug ("replay to == %s", *reply_to);
  } else {
    g_debug ("there is no reply_to in property");
  }

  if (envelope.message.properties.correlation_id.len > 0 &&
      (strnlen (envelope.message.properties.correlation_id.bytes,
              envelope.message.properties.correlation_id.len) >=
          envelope.message.properties.correlation_id.len)) {
    gchar *id = g_strndup (envelope.message.properties.correlation_id.bytes,
        envelope.message.properties.correlation_id.len);
    *correlation_id = atoi (id);
    g_debug ("correlation id == %d", *correlation_id);
  } else {
    g_debug ("there is no correlation id to in property");
  }

  g_debug ("Content-type: %.*s",
      (int) envelope.message.properties.content_type.len,
      (char *) envelope.message.properties.content_type.bytes);

  if (envelope.message.body.len > 0 && envelope.message.body.bytes != NULL) {
    *body = g_strndup (envelope.message.body.bytes, envelope.message.body.len);
    *size = envelope.message.body.len;
  }

  *channel = envelope.channel;
  amqp_destroy_envelope (&envelope);
  return CHAMGE_RETURN_OK;

out:
  amqp_destroy_envelope (&envelope);
  return CHAMGE_RETURN_FAIL;
}

static ChamgeReturn
_check_result (const gchar * body, guint size, const gchar * shouldbe)
{
  g_autoptr (JsonParser) parser = json_parser_new ();
  g_autoptr (GError) error = NULL;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  JsonNode *root = NULL;
  JsonObject *json_object = NULL;

  if (!json_parser_load_from_data (parser, body, size, &error)) {
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
  guint response_size = 0;
  guint amqp_channel = 1;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  ChamgeAmqpEdgeBackend *self = CHAMGE_AMQP_EDGE_BACKEND (edge_backend);

  g_return_val_if_fail (self->state == CHAMGE_AMQP_EDGE_BACKEND_NULL,
      CHAMGE_RETURN_FAIL);

  /* get configuration from gsetting */
  amqp_uri = g_settings_get_string (self->settings, "amqp-uri");
  g_assert_nonnull (amqp_uri);
  amqp_channel = g_settings_get_int (self->settings, "amqp-channel");
  amqp_enroll_q_name =
      g_settings_get_string (self->settings, "enroll-queue-name");
  amqp_exchange_name =
      g_settings_get_string (self->settings, "enroll-exchange-name");

  RETURN_FAIL_IF_TRUE (_amqp_rpc_login (self->amqp_conn, self->amqp_socket,
          amqp_uri, amqp_channel) != CHAMGE_RETURN_OK, "error in login");

  g_object_get (edge_backend, "uid", &(self->uid), NULL);
  g_return_val_if_fail (self->uid != NULL, CHAMGE_RETURN_FAIL);

  /* TODO
   * request-body need to be filled with real request data
   */
  request_body = g_strdup_printf ("{\"method\":\"enroll\",\"edge-id\":\"%s\"}",
      self->uid);
  ret =
      _amqp_rpc_request (self->amqp_conn, amqp_channel, request_body,
      amqp_exchange_name, amqp_enroll_q_name, &response_body, &response_size);

  RETURN_FAIL_IF_TRUE (ret == CHAMGE_RETURN_FAIL, "failed to get rpc response");

  g_debug ("received response to enroll : %s", response_body);
  if (_check_result (response_body, response_size,
          "enrolled") != CHAMGE_RETURN_OK) {
    g_error ("  received reponse must be [enrolled]");
    return CHAMGE_RETURN_FAIL;
  }

  self->state = CHAMGE_AMQP_EDGE_BACKEND_ENROLLED;
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_amqp_edge_backend_delist (ChamgeEdgeBackend * edge_backend)
{
  return CHAMGE_RETURN_OK;
}

static gchar *
_handle_streaming_start (ChamgeAmqpEdgeBackend * self, JsonNode * root)
{
  g_autofree gchar *response = NULL;
  const gchar *url = NULL;
  JsonObject *json_object = json_node_get_object (root);

  if (json_object_has_member (json_object, "url")) {
    JsonNode *node = json_object_get_member (json_object, "url");
    url = json_node_get_string (node);
  }
  g_debug ("    url : %s", url);

  /* TODO : add to handle streaming start request */
  if (url != NULL) {
    response = g_strdup ("{\"result\":\"playing\"}");
  } else {
    response = g_strdup ("{\"result\":\"url does not exist\"}");
  }
  return g_steal_pointer (&response);
}

static gchar *
_handle_streaming_stop (ChamgeAmqpEdgeBackend * self, JsonNode * root)
{
  g_autofree gchar *response = NULL;
  const gchar *url = NULL;
  JsonObject *json_object = json_node_get_object (root);

  if (json_object_has_member (json_object, "url")) {
    JsonNode *node = json_object_get_member (json_object, "url");
    url = json_node_get_string (node);
  }
  g_debug ("    url : %s", url);

  /* TODO : add to handle streaming stop request */
  if (url != NULL) {
    response = g_strdup ("{\"result\":\"stopped\"}");
  } else {
    response = g_strdup ("{\"result\":\"url does not exist\"}");
  }
  return g_steal_pointer (&response);
}

static gchar *
_handle_change_streaming_option (ChamgeAmqpEdgeBackend * self, JsonNode * root)
{
  JsonObject *json_object = json_node_get_object (root);
  g_autofree gchar *response = NULL;

  /* option
   * resolution = value [sd or hd or fhd or uhd]
   * fps = integer
   * bitrates = integer */
  if (json_object_has_member (json_object, "resolution")) {
    JsonNode *node = json_object_get_member (json_object, "resolution");
    const gchar *resolution = json_node_get_string (node);
    /* TODO : handle change requst */
    g_debug ("get resolution change request : %s", resolution);
    response = g_strdup ("{\"result\":\"ok\"}");
  } else if (json_object_has_member (json_object, "fps")) {
    JsonNode *node = json_object_get_member (json_object, "fps");
    guint fps = json_node_get_int (node);
    /* TODO : handle change requst */
    g_debug ("get fps change request : %u", fps);
    response = g_strdup ("{\"result\":\"ok\"}");
  } else if (json_object_has_member (json_object, "bitrates")) {
    JsonNode *node = json_object_get_member (json_object, "bitrates");
    guint bitrates = json_node_get_int (node);
    /* TODO : handle change requst */
    g_debug ("get bitrates change request : %u", bitrates);
    response = g_strdup ("{\"result\":\"ok\"}");
  } else {
    response = g_strdup ("{\"result\":\"nok - nothing to apply\"}");
  }

  return response;
}

static ChamgeReturn
_check_edge_id (ChamgeAmqpEdgeBackend * self, JsonNode * root)
{
  JsonObject *json_object = json_node_get_object (root);
  JsonNode *node = NULL;
  const gchar *edge_id = NULL;

  RETURN_FAIL_IF_TRUE (json_object_has_member (json_object, "edge_id") == FALSE,
      "edge_id is not exist");
  node = json_object_get_member (json_object, "edge_id");
  edge_id = json_node_get_string (node);

  RETURN_FAIL_IF_TRUE (g_strcmp0 (self->uid, edge_id) != 0,
      "edge id is different. [%s] != [%s]", self->uid, edge_id);
  return CHAMGE_RETURN_OK;
}

static gchar *
_process_json_message (ChamgeAmqpEdgeBackend * self, const gchar * body,
    gssize len)
{
  g_autoptr (JsonParser) parser = json_parser_new ();
  g_autoptr (GError) error = NULL;
  g_autofree gchar *response = NULL;

  JsonNode *root = NULL;
  JsonObject *json_object = NULL;

  do {
    if (!json_parser_load_from_data (parser, body, len, &error)) {
      g_debug ("failed to parse body: %s", error->message);
      break;
    }

    root = json_parser_get_root (parser);
    if (_check_edge_id (self, root) == CHAMGE_RETURN_FAIL) {
      response = g_strdup ("{\"result\":\"edge_id is wrong\"}");
      break;
    }

    if (json_object_has_member (json_object, "method") == FALSE) {
      response = g_strdup ("{\"result\":\"method is not exist\"}");
      break;
    }

    {
      JsonNode *node = json_object_get_member (json_object, "method");
      const gchar *method = json_node_get_string (node);
      g_debug ("method: %s", method);

      if (!g_strcmp0 (method, "streaming-start")) {
        response = _handle_streaming_start (self, root);
      } else if (!g_strcmp0 (method, "streaming-stop")) {
        response = _handle_streaming_stop (self, root);
      } else if (!g_strcmp0 (method, "change")) {
        response = _handle_change_streaming_option (self, root);
      }
    }
  } while (0);

  return g_steal_pointer (&response);
}

static gboolean
_process_amqp_message (ChamgeAmqpEdgeBackend * self)
{
  g_autofree gchar *reply_queue = NULL;
  g_autofree gchar *response = NULL;
  g_autofree gchar *body = NULL;
  guint body_size = 0;
  guint correlation_id = 0;
  guint channel = 0;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  if (self->state < CHAMGE_AMQP_EDGE_BACKEND_ACTIVATED)
    return G_SOURCE_REMOVE;

  ret =
      _amqp_rpc_listen (self->amqp_conn, &body, &body_size, &reply_queue,
      &correlation_id, &channel);

  if (ret != CHAMGE_RETURN_OK) {
    g_debug ("something is wrong in listen");
    goto out;
  }
  response = _process_json_message (self, body, body_size);

  {
    amqp_basic_properties_t amqp_props;
    _amqp_property_set (&amqp_props, correlation_id, NULL);

    /*
     * send response for request mesage
     */
    g_debug ("publishing to [%s] channel [%d]", reply_queue, channel);
    g_debug ("      correlation id [%d] body [%s]", correlation_id, response);
    amqp_basic_publish (self->amqp_conn, channel, amqp_cstring_bytes (""),
        amqp_cstring_bytes (reply_queue), 0, 0, &amqp_props,
        amqp_cstring_bytes (response));
  }

out:
  return G_SOURCE_CONTINUE;
}


static ChamgeReturn
chamge_amqp_edge_backend_activate (ChamgeEdgeBackend * edge_backend)
{
  g_autofree gchar *amqp_enroll_q_name = NULL;
  g_autofree gchar *amqp_exchange_name = NULL;
  guint amqp_channel = 1;

  g_autofree gchar *request_body = NULL;
  g_autofree gchar *response_body = NULL;
  guint response_size = 0;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  ChamgeAmqpEdgeBackend *self = CHAMGE_AMQP_EDGE_BACKEND (edge_backend);

  g_return_val_if_fail (self->state == CHAMGE_AMQP_EDGE_BACKEND_ENROLLED,
      CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (self->uid != NULL, CHAMGE_RETURN_FAIL);

  amqp_channel = g_settings_get_int (self->settings, "amqp-channel");
  amqp_enroll_q_name =
      g_settings_get_string (self->settings, "enroll-queue-name");
  amqp_exchange_name =
      g_settings_get_string (self->settings, "enroll-exchange-name");

  /* send activate */
  request_body =
      g_strdup_printf ("{\"method\":\"activate\",\"edge-id\":\"%s\"}",
      self->uid);
  ret =
      _amqp_rpc_request (self->amqp_conn, amqp_channel, request_body,
      amqp_exchange_name, amqp_enroll_q_name, &response_body, &response_size);

  RETURN_FAIL_IF_TRUE (ret == CHAMGE_RETURN_FAIL, "failed to get rpc response");
  g_debug ("received response to activate: %s", response_body);

  if (_check_result (response_body, response_size,
          "activated") != CHAMGE_RETURN_OK) {
    g_error ("  received reponse must be [enrolled]");
    return CHAMGE_RETURN_FAIL;
  }

  self->state = CHAMGE_AMQP_EDGE_BACKEND_ACTIVATED;

  /* subscribe queue (queue name: edge-id) for streaming start */
  _amqp_rpc_subscribe (self->amqp_conn, amqp_channel, amqp_exchange_name,
      self->uid);

  /* process amqp message that comes from Mujachi 
   * messages to handle
   *  - start/stop streaming
   *  - change resolution/fps/bitrates */
  self->process_id = g_idle_add ((GSourceFunc) _process_amqp_message, self);

  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_amqp_edge_backend_deactivate (ChamgeEdgeBackend * edge_backend)
{
  ChamgeAmqpEdgeBackend *self = CHAMGE_AMQP_EDGE_BACKEND (edge_backend);

  g_return_val_if_fail (self->state == CHAMGE_AMQP_EDGE_BACKEND_ACTIVATED,
      CHAMGE_RETURN_FAIL);

  if (self->process_id != 0) {
    g_source_remove (self->process_id);
    self->process_id = 0;
  }

  self->state = CHAMGE_AMQP_EDGE_BACKEND_ENROLLED;

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

  g_clear_pointer (&self->uid, g_free);
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

  self->state = CHAMGE_AMQP_EDGE_BACKEND_NULL;
  self->uid = NULL;

  g_assert_nonnull (self->amqp_socket);
}
