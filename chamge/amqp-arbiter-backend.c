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
  amqp_bytes_free (amqp_listen_queue);

  amqp_queue_bind (amqp_conn, channel, amqp_queue,
      amqp_cstring_bytes (exchange_name), amqp_queue, amqp_empty_table);

  g_debug ("binding a queue(%s) (exchange: %s, bind_key: %s)",
      queue_name, exchange_name, queue_name);

  amqp_basic_consume (amqp_conn, channel, amqp_queue,
      amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  g_return_val_if_fail ((amqp_get_rpc_reply (amqp_conn)).reply_type ==
      AMQP_RESPONSE_NORMAL, CHAMGE_RETURN_FAIL);

  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
_amqp_property_set (amqp_basic_properties_t * const props,
    const guint correlation_id, const gchar * reply_queue_name)
{
  gchar *correlation_id_str = NULL;
  gchar *content_type_str = g_strdup (DEFAULT_CONTENT_TYPE);

  props->_flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
  props->content_type = amqp_cstring_bytes (content_type_str);
  props->delivery_mode = 2;     /* persistent delivery mode */

  if (reply_queue_name != NULL) {
    gchar *reply_to_queue = g_strdup (reply_queue_name);
    g_assert_nonnull (reply_to_queue);
    props->reply_to = amqp_cstring_bytes (reply_to_queue);
    props->_flags |= AMQP_BASIC_REPLY_TO_FLAG;
  } else {
    props->reply_to.bytes = NULL;
    props->reply_to.bytes = 0;
  }

  if (correlation_id != 0) {
    guint correlation_id_str_len = correlation_id / 10 + 2;
    props->_flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
    correlation_id_str =
        (char *) calloc (correlation_id_str_len, sizeof (char));
    g_return_val_if_fail (correlation_id_str != NULL, CHAMGE_RETURN_FAIL);
    g_snprintf (correlation_id_str, correlation_id_str_len, "%u",
        correlation_id);
    props->correlation_id = amqp_cstring_bytes (correlation_id_str);
  } else {
    props->correlation_id.bytes = NULL;
    props->correlation_id.len = 0;
  }

  g_debug ("content type : %s , correlation id : %s",
      (gchar *) props->content_type.bytes,
      (gchar *) props->correlation_id.bytes);
  return CHAMGE_RETURN_OK;
}

static void
_amqp_property_free (amqp_basic_properties_t * const props)
{
  if (props->content_type.bytes)
    amqp_bytes_free (props->content_type);
  if (props->reply_to.bytes)
    amqp_bytes_free (props->reply_to);
  if (props->correlation_id.bytes)
    amqp_bytes_free (props->correlation_id);
}

static ChamgeReturn
_amqp_rpc_listen (amqp_connection_state_t amqp_conn, gchar ** const body,
    gchar ** const reply_to, guint * const correlation_id)
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
      || strlen ("application/json") !=
      envelope.message.properties.content_type.len
      || g_ascii_strncasecmp ("application/json",
          envelope.message.properties.content_type.bytes,
          envelope.message.properties.content_type.len)
      ) {
    g_debug ("invalid content type %s",
        (gchar *) envelope.message.properties.content_type.bytes);

    goto out;
  }

  if (envelope.message.properties.reply_to.len > 0 &&
      (strlen (envelope.message.properties.reply_to.bytes) >=
          envelope.message.properties.reply_to.len)) {
    *reply_to = g_strndup (envelope.message.properties.reply_to.bytes,
        envelope.message.properties.reply_to.len);
    g_debug ("replay to == %s", *reply_to);
  } else {
    g_debug ("there is no replay to in property");
  }

  if (envelope.message.properties.correlation_id.len > 0 &&
      (strlen (envelope.message.properties.correlation_id.bytes) >=
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
  }

  amqp_destroy_envelope (&envelope);
  return CHAMGE_RETURN_OK;

out:
  amqp_destroy_envelope (&envelope);
  return CHAMGE_RETURN_FAIL;
}

static ChamgeReturn
_amqp_rpc_respond (amqp_connection_state_t amqp_conn, const gchar * response,
    const gchar * reply_to, guint correlation_id)
{
  amqp_basic_properties_t amqp_props;

  if (_amqp_property_set (&amqp_props, correlation_id,
          NULL) != CHAMGE_RETURN_OK) {
    return CHAMGE_RETURN_FAIL;
  }
  /*
   * publish
   */
  g_debug ("publishing to [%s]", reply_to);
  g_debug ("      correlation id : %d body : \"%s\"", correlation_id, response);
  amqp_basic_publish (amqp_conn, 1, amqp_cstring_bytes (""),
      amqp_cstring_bytes (reply_to), 0, 0, &amqp_props,
      amqp_cstring_bytes (response));

  _amqp_property_free (&amqp_props);
  return CHAMGE_RETURN_OK;
}


static ChamgeReturn
chamge_amqp_arbiter_backend_enroll (ChamgeArbiterBackend * arbiter_backend)
{
  g_autofree gchar *amqp_uri = NULL;
  g_autofree gchar *amqp_enroll_q_name = NULL;
  g_autofree gchar *amqp_exchange_name = NULL;
  gint amqp_channel;

  ChamgeAmqpArbiterBackend *self =
      CHAMGE_AMQP_ARBITER_BACKEND (arbiter_backend);

  /* get configuration from gsetting */
  amqp_uri = g_settings_get_string (self->settings, "amqp-uri");
  g_assert_nonnull (amqp_uri);
  amqp_channel = g_settings_get_int (self->settings, "amqp-channel");
  amqp_enroll_q_name =
      g_settings_get_string (self->settings, "enroll-queue-name");
  amqp_exchange_name =
      g_settings_get_string (self->settings, "enroll-exchange-name");

  g_return_val_if_fail (_amqp_rpc_login (self->amqp_conn, self->amqp_socket,
          amqp_uri, amqp_channel) == CHAMGE_RETURN_OK, CHAMGE_RETURN_FAIL);

  g_return_val_if_fail (_amqp_rpc_subscribe (self->amqp_conn, amqp_channel,
          amqp_exchange_name, amqp_enroll_q_name) != CHAMGE_RETURN_FAIL,
      CHAMGE_RETURN_FAIL);

  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_amqp_arbiter_backend_delist (ChamgeArbiterBackend * arbiter_backend)
{
  return CHAMGE_RETURN_OK;
}

static void
_handle_edge_enroll (ChamgeAmqpArbiterBackend * self, JsonNode * root,
    const gchar * reply_queue, guint correlation_id)
{
  JsonObject *json_object = NULL;
  const gchar *edge_id = NULL;
  ChamgeArbiterBackendClass *klass = CHAMGE_ARBITER_BACKEND_GET_CLASS (self);
  gchar *body = NULL;;

  /* TODO: validate json value */
  json_object = json_node_get_object (root);
  if (json_object_has_member (json_object, "edge-id")) {
    JsonNode *node = json_object_get_member (json_object, "edge-id");
    edge_id = json_node_get_string (node);
  }
  /* TODO 
   * body need to be filled with real data
   */
  if (edge_id != NULL) {
    klass->edge_enrolled (CHAMGE_ARBITER_BACKEND (self), edge_id);
    body = g_strdup ("{\"result\":\"enrolled\"}");
  } else {
    body = g_strdup ("{\"result\":\"edge_id does not exist\"}");
  }

  _amqp_rpc_respond (self->amqp_conn, body, reply_queue, correlation_id);
}

static void
_process_json_message (ChamgeAmqpArbiterBackend * self, const gchar * body,
    gssize len, const gchar * reply_queue, guint correlation_id)
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
      _handle_edge_enroll (self, root, reply_queue, correlation_id);
    }
  }
}

static gboolean
_process_amqp_message (ChamgeAmqpArbiterBackend * self)
{
  g_autofree gchar *body = NULL;
  g_autofree gchar *reply_queue = NULL;
  guint correlation_id = 0;
  ChamgeReturn ret;


  if (!self->activated)
    return G_SOURCE_REMOVE;

  ret =
      _amqp_rpc_listen (self->amqp_conn, &body, &reply_queue, &correlation_id);

  if (ret == CHAMGE_RETURN_OK) {
    _process_json_message (self, body, strlen (body), reply_queue,
        correlation_id);
  }

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
  self->settings = g_settings_new (AMQP_ARBITER_BACKEND_SCHEMA_ID);

  self->amqp_conn = amqp_new_connection ();
  self->amqp_socket = amqp_tcp_socket_new (self->amqp_conn);

  g_assert_nonnull (self->amqp_socket);
}
