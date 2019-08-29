/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "amqp-arbiter-backend.h"

#include <gio/gio.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <json-glib/json-glib.h>

#define AMQP_ARBITER_BACKEND_SCHEMA_ID "org.hwangsaeul.Chamge1.Arbiter.AMQP"

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

  g_debug ("declaring a queue : %s", amqp_r->queue.bytes);

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
_handle_edge_enroll (ChamgeAmqpArbiterBackend * self, JsonNode * root)
{
  JsonObject *json_object = NULL;
  const gchar *edge_id = NULL;
  ChamgeArbiterBackendClass *klass = CHAMGE_ARBITER_BACKEND_GET_CLASS (self);

  /* TODO: validate json value */

  json_object = json_node_get_object (root);
  if (json_object_has_member (json_object, "edge-id")) {
    JsonNode *node = json_object_get_member (json_object, "edge-id");
    edge_id = json_node_get_string (node);
  }

  if (edge_id != NULL) {
    klass->edge_enrolled (CHAMGE_ARBITER_BACKEND (self), edge_id);
  }
}

static void
_process_json_message (ChamgeAmqpArbiterBackend * self, const gchar * body,
    gssize len)
{
  g_autoptr (JsonParser) parser = json_parser_new ();
  g_autoptr (GError) error = NULL;

  JsonNode *root = NULL;
  JsonObject *json_object = NULL;

  if (!json_parser_load_from_data (parser, body, len, &error)) {
    g_debug ("failed to parse body: %s", error->message);
    return;
  }

  root = json_parser_get_root (parser);
  json_object = json_node_get_object (root);
  if (json_object_has_member (json_object, "method")) {
    JsonNode *node = json_object_get_member (json_object, "method");
    const gchar *method = json_node_get_string (node);
    g_debug ("method: %s", method);

    if (!g_strcmp0 (method, "enroll")) {
      _handle_edge_enroll (self, root);
    }
  }
}

static gboolean
_process_amqp_message (ChamgeAmqpArbiterBackend * self)
{
  amqp_rpc_reply_t amqp_rpc_res;
  amqp_envelope_t envelope;

  struct timeval timeout = { 1, 0 };

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
      || strlen ("application/json") !=
      envelope.message.properties.content_type.len
      || g_ascii_strcasecmp ("application/json",
          envelope.message.properties.content_type.bytes)
      ) {
    g_debug ("invalid content type");

    goto out;
  }

  g_debug ("Content-type: %.*s",
      (int) envelope.message.properties.content_type.len,
      (char *) envelope.message.properties.content_type.bytes);

  _process_json_message (self, envelope.message.body.bytes,
      envelope.message.body.len);

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
