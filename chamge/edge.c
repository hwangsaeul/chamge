/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "edge.h"
#include "enumtypes.h"
#include "edge-backend.h"

typedef struct
{
  ChamgeBackend backend;

  ChamgeEdgeBackend *edge_backend;

  gint n_stream;
} ChamgeEdgePrivate;

typedef enum
{
  PROP_BACKEND = 1,

  /*< private > */
  PROP_LAST = PROP_BACKEND
} _ChamgeEdgeProperty;

static GParamSpec *properties[PROP_LAST + 1];

/* *INDENT-OFF* */
G_DEFINE_TYPE_WITH_CODE (ChamgeEdge, chamge_edge, CHAMGE_TYPE_NODE,
                         G_ADD_PRIVATE (ChamgeEdge))
/* *INDENT-ON* */

static gchar *
chamge_edge_request_target_uri_default (ChamgeEdge * self, GError ** error)
{
  g_autofree gchar *target_uri = NULL;
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);

  target_uri =
      chamge_edge_backend_request_target_uri (priv->edge_backend, error);

  return g_steal_pointer (&target_uri);
}

static ChamgeReturn
chamge_edge_enroll (ChamgeNode * node)
{
  ChamgeEdge *self = CHAMGE_EDGE (node);
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);
  ChamgeReturn ret;

  if (priv->edge_backend == NULL)
    priv->edge_backend = chamge_edge_backend_new (self);

  ret = chamge_edge_backend_enroll (priv->edge_backend);

  return ret;
}

static ChamgeReturn
chamge_edge_delist (ChamgeNode * node)
{
  ChamgeEdge *self = CHAMGE_EDGE (node);
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);
  ChamgeReturn ret;

  ret = chamge_edge_backend_delist (priv->edge_backend);

  g_clear_object (&priv->edge_backend);

  return ret;
}

static ChamgeReturn
chamge_edge_activate (ChamgeNode * node)
{
  ChamgeEdge *self = CHAMGE_EDGE (node);
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);
  ChamgeReturn ret;

  ret = chamge_edge_backend_activate (priv->edge_backend);

  return ret;
}

static ChamgeReturn
chamge_edge_deactivate (ChamgeNode * node)
{
  ChamgeEdge *self = CHAMGE_EDGE (node);
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);
  ChamgeReturn ret;

  ret = chamge_edge_backend_deactivate (priv->edge_backend);

  return ret;
}

static void
chamge_edge_dispose (GObject * object)
{
  ChamgeEdge *self = CHAMGE_EDGE (object);
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);

  g_clear_object (&priv->edge_backend);
  G_OBJECT_CLASS (chamge_edge_parent_class)->dispose (object);
}

static void
chamge_edge_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  ChamgeEdge *self = CHAMGE_EDGE (object);
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);

  switch ((_ChamgeEdgeProperty) prop_id) {
    case PROP_BACKEND:
      g_value_set_enum (value, priv->backend);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_edge_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ChamgeEdge *self = CHAMGE_EDGE (object);
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);

  switch ((_ChamgeEdgeProperty) prop_id) {
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
chamge_edge_class_init (ChamgeEdgeClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ChamgeNodeClass *node_class = CHAMGE_NODE_CLASS (klass);

  object_class->get_property = chamge_edge_get_property;
  object_class->set_property = chamge_edge_set_property;
  object_class->dispose = chamge_edge_dispose;

  properties[PROP_BACKEND] = g_param_spec_enum ("backend", "backend", "backend",
      CHAMGE_TYPE_BACKEND, CHAMGE_BACKEND_UNKNOWN,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties),
      properties);

  node_class->enroll = chamge_edge_enroll;
  node_class->delist = chamge_edge_delist;
  node_class->activate = chamge_edge_activate;
  node_class->deactivate = chamge_edge_deactivate;

  klass->request_target_uri = chamge_edge_request_target_uri_default;
}

static void
chamge_edge_init (ChamgeEdge * self)
{
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);
}

ChamgeEdge *
chamge_edge_new (const gchar * uid, ChamgeBackend backend)
{
  g_assert_nonnull (uid);
  return g_object_new (CHAMGE_TYPE_EDGE, "uid", uid, "backend", backend, NULL);
}

gchar *
chamge_edge_request_target_uri (ChamgeEdge * self, GError ** error)
{
  ChamgeEdgeClass *klass;
  ChamgeNodeState state;
  g_autofree gchar *target_uri = NULL;
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);

  g_return_val_if_fail (CHAMGE_IS_EDGE (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  klass = CHAMGE_EDGE_GET_CLASS (self);
  g_return_val_if_fail (klass->request_target_uri != NULL, NULL);

  g_object_get (self, "state", &state, NULL);

  if (state == CHAMGE_NODE_STATE_ACTIVATED) {
    target_uri = klass->request_target_uri (self, error);
  }

  return g_steal_pointer (&target_uri);
}
