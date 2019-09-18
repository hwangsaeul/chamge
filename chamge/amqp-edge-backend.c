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

#define AMQP_EDGE_BACKEND_SCHEMA_ID "org.hwangsaeul.Chamge1.Edge.AMQP"
#define DEFAULT_CONTENT_TYPE "application/json"

struct _ChamgeAmqpEdgeBackend
{
  ChamgeEdgeBackend parent;
  GSettings *settings;

  amqp_connection_state_t amqp_conn;
  amqp_socket_t *amqp_socket;

  gboolean activated;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeAmqpEdgeBackend, chamge_amqp_edge_backend, CHAMGE_TYPE_EDGE_BACKEND)
/* *INDENT-ON* */

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

  g_return_val_if_fail (amqp_socket_open (amqp_socket,
          connection_info.host, connection_info.port) >= 0, CHAMGE_RETURN_FAIL);

  /* TODO: The authentication method should be EXTERNAL
   * if we don't want to share user/passwd
   */
  amqp_login (amqp_conn, connection_info.vhost, 0, 131072, 0,
      AMQP_SASL_METHOD_PLAIN, connection_info.user, connection_info.password);
  amqp_channel_open (amqp_conn, channel);
  amqp_r = amqp_get_rpc_reply (amqp_conn);
  g_return_val_if_fail (amqp_r.reply_type == AMQP_RESPONSE_NORMAL,
      CHAMGE_RETURN_FAIL);

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
  gchar *response_queue_name = NULL;

  if (queue_name == NULL) {
    amqp_r = amqp_queue_declare (conn, 1, amqp_empty_bytes, 0, 0, 0, 1,
        amqp_empty_table);
  } else {
    amqp_r =
        amqp_queue_declare (conn, 1, amqp_cstring_bytes (queue_name), 0, 0, 0,
        1, amqp_empty_table);
  }
  if (amqp_r == NULL) {
    g_error ("Can not declare a new queue or queue does not exist...");
    return CHAMGE_RETURN_FAIL;
  }
  amqp_get_rpc_reply (conn);

  g_assert_nonnull (amqp_r->queue.bytes);
  response_queue_name = g_strdup (amqp_r->queue.bytes);
  *queue = amqp_cstring_bytes (response_queue_name);

  g_debug ("create private reply_to queue : name -> %s",
      (gchar *) queue->bytes);
  return CHAMGE_RETURN_OK;
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


static gchar *
_amqp_get_response (const amqp_connection_state_t conn,
    const amqp_bytes_t reply_to_queue)
{
  gchar *response_body = NULL;
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
    return NULL;
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

  response_body =
      g_strndup (envelope.message.body.bytes, envelope.message.body.len);
  amqp_destroy_envelope (&envelope);

  return response_body;
}


static gchar *
_amqp_rpc_request (amqp_connection_state_t amqp_conn, guint channel,
    const gchar * request, const gchar * exchange, const gchar * queue_name)
{
  amqp_bytes_t amqp_reply_queue;
  amqp_basic_properties_t amqp_props = { 0 };
  gchar *response_body = NULL;

  g_return_val_if_fail (queue_name != NULL, NULL);

  /* create private replay_to_queue and queue name is random that is supplied from amqp server */
  if (_amqp_declare_queue (amqp_conn, NULL,
          &amqp_reply_queue) != CHAMGE_RETURN_OK)
    return NULL;

  g_debug ("declare a queue for reply : %s", (gchar *) amqp_reply_queue.bytes);

  /* property setting to send rpc request */
  amqp_props._flags =
      AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;

  amqp_props.content_type = amqp_cstring_bytes (DEFAULT_CONTENT_TYPE);
  amqp_props.delivery_mode = 2; /* persistent delivery mode */

  amqp_props._flags |= AMQP_BASIC_REPLY_TO_FLAG;
  amqp_props.reply_to = amqp_reply_queue;

  amqp_props._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
  amqp_props.correlation_id = amqp_cstring_bytes ("1");

  /* send requst to queue_name */
  {
    gint r = amqp_basic_publish (amqp_conn, channel,
        exchange ==
        NULL ? amqp_cstring_bytes ("") : amqp_cstring_bytes (exchange),
        amqp_cstring_bytes (queue_name), 0, 0, &amqp_props,
        amqp_cstring_bytes (request));
    if (r < 0) {
      g_error ("publish can not be done >> %s", amqp_error_string2 (r));
      goto out;
    }
    g_debug ("published to queue[%s], exchange[%s], request[%s]", queue_name,
        exchange, request);
  }

  /* wait an answer */
  response_body = _amqp_get_response (amqp_conn, amqp_reply_queue);

out:
  amqp_bytes_free (amqp_reply_queue);
  return response_body;
}


static ChamgeReturn
chamge_amqp_edge_backend_enroll (ChamgeEdgeBackend * edge_backend)
{
  g_autofree gchar *amqp_uri = NULL;
  g_autofree gchar *amqp_enroll_q_name = NULL;
  g_autofree gchar *amqp_exchange_name = NULL;
  g_autofree gchar *response_body = NULL;
  g_autofree gchar *request_body = NULL;
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
          amqp_uri, amqp_channel) != CHAMGE_RETURN_OK) {
    g_debug ("failed to amqp login");
    goto out;
  }

  /* TODO
   * request-body need to be filled with real request data
   */
  request_body =
      g_strdup_printf ("{\"method\":\"enroll\",\"edge-id\":\"%s\"}", edge_id);
  response_body =
      _amqp_rpc_request (self->amqp_conn, amqp_channel, request_body,
      amqp_exchange_name, amqp_enroll_q_name);
  g_debug ("received response to enroll : %s", response_body);
  ret = CHAMGE_RETURN_OK;

out:
  return ret;
}

static ChamgeReturn
chamge_amqp_edge_backend_delist (ChamgeEdgeBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_amqp_edge_backend_activate (ChamgeEdgeBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_amqp_edge_backend_deactivate (ChamgeEdgeBackend * self)
{
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
