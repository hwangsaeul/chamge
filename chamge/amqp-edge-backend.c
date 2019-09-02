/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "amqp-edge-backend.h"
#include "amqp/amqp-rpc.h"

#include <gio/gio.h>

#define AMQP_EDGE_BACKEND_SCHEMA_ID "org.hwangsaeul.Chamge1.Arbiter.AMQP"
#define AMQP_ARBITER_BACKEND_SCHEMA_ID "org.hwangsaeul.Chamge1.Arbiter.AMQP"
#define DEFAULT_CONTENT_TYPE "application/json"

struct _ChamgeAmqpEdgeBackend
{
  ChamgeEdgeBackend parent;
  GSettings *settings;

  gboolean activated;

  ChamgeAmqpRpc *amqp_rpc;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeAmqpEdgeBackend, chamge_amqp_edge_backend, CHAMGE_TYPE_EDGE_BACKEND)
/* *INDENT-ON* */


static ChamgeReturn
chamge_amqp_edge_backend_enroll (ChamgeEdgeBackend * edge_backend)
{
  g_autofree gchar *amqp_uri = NULL;
  g_autofree gchar *amqp_enroll_q_name = NULL;
  g_autofree gchar *amqp_exchange_name = NULL;
  g_autofree gchar *response_body = NULL;
  gint amqp_channel = 1;
  GValue value_str = G_VALUE_INIT;
  GValue value_int = G_VALUE_INIT;

  ChamgeAmqpEdgeBackend *self = CHAMGE_AMQP_EDGE_BACKEND (edge_backend);

  /* get configuration from gsetting */
  amqp_uri = g_settings_get_string (self->settings, "amqp-uri");
  g_assert_nonnull (amqp_uri);
  amqp_channel = g_settings_get_int (self->settings, "amqp-channel");
  amqp_enroll_q_name =
      g_settings_get_string (self->settings, "enroll-queue-name");
  amqp_exchange_name =
      g_settings_get_string (self->settings, "enroll-exchange-name");

  /*setting amqp login information */
  g_value_init (&value_str, G_TYPE_STRING);
  g_value_init (&value_int, G_TYPE_UINT);

  g_value_set_string (&value_str, amqp_uri);
  g_object_set_property (G_OBJECT (self->amqp_rpc), "url", &value_str);

  g_value_set_uint (&value_int, amqp_channel);
  g_object_set_property (G_OBJECT (self->amqp_rpc), "channel", &value_int);

  g_return_val_if_fail (chamge_amqp_rpc_login (self->amqp_rpc) ==
      CHAMGE_RETURN_OK, CHAMGE_RETURN_FAIL);

  /* TODO
   * request-body need to be filled with real request data
   */
  response_body =
      chamge_amqp_rpc_request (self->amqp_rpc,
      "{\"method\":\"enroll\",\"edge-id\":\"abcd1234\"}", amqp_exchange_name,
      amqp_enroll_q_name);
  g_debug ("received response to enroll : %s", response_body);

  return CHAMGE_RETURN_OK;
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

  g_clear_object (&self->amqp_rpc);

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
  self->settings = g_settings_new (AMQP_ARBITER_BACKEND_SCHEMA_ID);
  g_assert_nonnull (self->settings);

  self->amqp_rpc = chamge_amqp_rpc_new ();
  g_assert_nonnull (self->amqp_rpc);
}
