/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "arbiter.h"
#include "enumtypes.h"
#include "arbiter-backend.h"

typedef struct
{
  ChamgeBackend backend;
  ChamgeArbiterBackend *arbiter_backend;
} ChamgeArbiterPrivate;

typedef enum
{
  PROP_BACKEND = 1,

  /*< private > */
  PROP_LAST = PROP_BACKEND
} _ChamgeArbiterProperty;

typedef enum
{
  SIG_EDGE_ENROLLED,
  SIG_EDGE_DELISTED,

  LAST_SIGNAL
};

static GParamSpec *properties[PROP_LAST + 1];
static guint signals[LAST_SIGNAL] = { 0 };

/* *INDENT-OFF* */
G_DEFINE_TYPE_WITH_PRIVATE (ChamgeArbiter, chamge_arbiter, CHAMGE_TYPE_NODE)
/* *INDENT-ON* */

static void
chamge_arbiter_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  ChamgeArbiter *self = CHAMGE_ARBITER (object);
  ChamgeArbiterPrivate *priv = chamge_arbiter_get_instance_private (self);

  switch ((_ChamgeArbiterProperty) prop_id) {
    case PROP_BACKEND:
      g_value_set_enum (value, priv->backend);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_arbiter_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ChamgeArbiter *self = CHAMGE_ARBITER (object);
  ChamgeArbiterPrivate *priv = chamge_arbiter_get_instance_private (self);

  switch ((_ChamgeArbiterProperty) prop_id) {
    case PROP_BACKEND:
      g_assert (priv->backend == CHAMGE_BACKEND_UNKNOWN);       /* construct only */
      priv->backend = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
chamge_arbiter_enrolled_cb (const gchar * uid, ChamgeArbiterBackend * self)
{
  ChamgeArbiter *arbiter = NULL;

  g_object_get (self, "arbiter", &arbiter, NULL);
  g_assert (arbiter != NULL);

  g_signal_emit (arbiter, signals[SIG_EDGE_ENROLLED], 0, uid);
}

static void
chamge_arbiter_delisted_cb (const gchar * uid, ChamgeArbiterBackend * self)
{
  ChamgeArbiter *arbiter = NULL;

  g_object_get (self, "arbiter", &arbiter, NULL);
  g_assert (arbiter != NULL);

  g_signal_emit (arbiter, signals[SIG_EDGE_DELISTED], 0, uid);
}

static ChamgeReturn
chamge_arbiter_enroll (ChamgeNode * node)
{
  ChamgeArbiter *self = CHAMGE_ARBITER (node);
  ChamgeArbiterPrivate *priv = chamge_arbiter_get_instance_private (self);
  ChamgeReturn ret;

  if (priv->arbiter_backend == NULL)
    priv->arbiter_backend = chamge_arbiter_backend_new (self);

  chamge_arbiter_backend_set_edge_handler (priv->arbiter_backend,
      chamge_arbiter_enrolled_cb, chamge_arbiter_delisted_cb, NULL);
  ret = chamge_arbiter_backend_enroll (priv->arbiter_backend);

  return ret;
}

static ChamgeReturn
chamge_arbiter_delist (ChamgeNode * node)
{
  ChamgeArbiter *self = CHAMGE_ARBITER (node);
  ChamgeArbiterPrivate *priv = chamge_arbiter_get_instance_private (self);
  ChamgeReturn ret;

  ret = chamge_arbiter_backend_delist (priv->arbiter_backend);

  g_clear_object (&priv->arbiter_backend);

  return ret;
}

static ChamgeReturn
chamge_arbiter_activate (ChamgeNode * node)
{
  ChamgeArbiter *self = CHAMGE_ARBITER (node);
  ChamgeArbiterPrivate *priv = chamge_arbiter_get_instance_private (self);
  ChamgeReturn ret;

  ret = chamge_arbiter_backend_activate (priv->arbiter_backend);

  return ret;
}

static ChamgeReturn
chamge_arbiter_deactivate (ChamgeNode * node)
{
  ChamgeArbiter *self = CHAMGE_ARBITER (node);
  ChamgeArbiterPrivate *priv = chamge_arbiter_get_instance_private (self);
  ChamgeReturn ret;

  ret = chamge_arbiter_backend_deactivate (priv->arbiter_backend);

  return ret;
}

static ChamgeReturn
chamge_arbiter_user_command (ChamgeNode * node, const gchar * cmd, gchar ** out,
    GError ** error)
{
  ChamgeArbiter *self = CHAMGE_ARBITER (node);
  ChamgeArbiterPrivate *priv = chamge_arbiter_get_instance_private (self);

  return chamge_arbiter_backend_user_command (priv->arbiter_backend, cmd, out,
      error);
}

static void
chamge_arbiter_dispose (GObject * object)
{
  ChamgeArbiter *self = CHAMGE_ARBITER (object);
  ChamgeArbiterPrivate *priv = chamge_arbiter_get_instance_private (self);

  g_clear_object (&priv->arbiter_backend);
  G_OBJECT_CLASS (chamge_arbiter_parent_class)->dispose (object);
}

static void
chamge_arbiter_class_init (ChamgeArbiterClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ChamgeNodeClass *node_class = CHAMGE_NODE_CLASS (klass);

  object_class->get_property = chamge_arbiter_get_property;
  object_class->set_property = chamge_arbiter_set_property;
  object_class->dispose = chamge_arbiter_dispose;

  properties[PROP_BACKEND] = g_param_spec_enum ("backend", "backend", "backend",
      CHAMGE_TYPE_BACKEND, CHAMGE_BACKEND_UNKNOWN,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties),
      properties);

  signals[SIG_EDGE_ENROLLED] =
      g_signal_new ("enrolled", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (ChamgeArbiterClass, enrolled), NULL,
      NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 1, G_TYPE_STRING);

  signals[SIG_EDGE_DELISTED] =
      g_signal_new ("delisted", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (ChamgeArbiterClass, delisted), NULL,
      NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 1, G_TYPE_STRING);

  node_class->enroll = chamge_arbiter_enroll;
  node_class->delist = chamge_arbiter_delist;
  node_class->activate = chamge_arbiter_activate;
  node_class->deactivate = chamge_arbiter_deactivate;
  node_class->user_command = chamge_arbiter_user_command;

}

static void
chamge_arbiter_init (ChamgeArbiter * self)
{
}

ChamgeArbiter *
chamge_arbiter_new (const gchar * uid)
{
  return chamge_arbiter_new_full (uid, CHAMGE_BACKEND_AMQP);
}

ChamgeArbiter *
chamge_arbiter_new_full (const gchar * uid, ChamgeBackend backend)
{
  g_assert_nonnull (uid);
  return g_object_new (CHAMGE_TYPE_ARBITER, "uid", uid, "backend", backend,
      NULL);
}
