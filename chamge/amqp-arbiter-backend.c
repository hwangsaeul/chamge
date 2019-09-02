/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "amqp-arbiter-backend.h"
#include "amqp/amqp-rpc.h"

#include <gio/gio.h>
#include <json-glib/json-glib.h>

#define AMQP_ARBITER_BACKEND_SCHEMA_ID "org.hwangsaeul.Chamge1.Arbiter.AMQP"
#define DEFAULT_CONTENT_TYPE "application/json"

typedef struct _AmqpReturnArguments
{
  gchar *reply_queue;
  guint correlation_id;
} AmqpReturnAuguments;

struct _ChamgeAmqpArbiterBackend
{
  ChamgeArbiterBackend parent;
  GSettings *settings;

  AmqpReturnAuguments amqp_return_arg;
  ChamgeAmqpRpc *amqp_rpc;

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
  GValue value_str = G_VALUE_INIT;
  GValue value_int = G_VALUE_INIT;

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

  /* set amqp login information */
  g_value_init (&value_str, G_TYPE_STRING);
  g_value_init (&value_int, G_TYPE_UINT);

  g_value_set_string (&value_str, amqp_uri);
  g_object_set_property (G_OBJECT (self->amqp_rpc), "url", &value_str);

  g_value_set_uint (&value_int, amqp_channel);
  g_object_set_property (G_OBJECT (self->amqp_rpc), "channel", &value_int);

  g_return_val_if_fail (chamge_amqp_rpc_login (self->amqp_rpc) ==
      CHAMGE_RETURN_OK, CHAMGE_RETURN_FAIL);

  g_return_val_if_fail (chamge_amqp_rpc_subscribe (self->amqp_rpc,
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
_handle_edge_enroll (ChamgeAmqpArbiterBackend * self, JsonNode * root)
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

  chamge_amqp_rpc_respond (self->amqp_rpc, body,
      self->amqp_return_arg.reply_queue, self->amqp_return_arg.correlation_id);
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
  gchar *body = NULL;
  ChamgeReturn ret;

  if (!self->activated)
    return G_SOURCE_REMOVE;

  ret =
      chamge_amqp_rpc_listen (self->amqp_rpc, &body,
      &self->amqp_return_arg.reply_queue,
      &self->amqp_return_arg.correlation_id);

  if (ret == CHAMGE_RETURN_OK) {
    _process_json_message (self, body, strlen (body));
    g_free (body);
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
  g_clear_object (&self->amqp_rpc);

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
  self->amqp_rpc = chamge_amqp_rpc_new ();

  g_assert_nonnull (self->amqp_rpc);
}
