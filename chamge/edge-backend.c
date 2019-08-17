/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "edge-backend.h"

#include "edge.h"
#include "enumtypes.h"

/* FIXME: Choosing a backend in compile time? */
#include "mock-edge-backend.h"

typedef struct
{
  ChamgeEdge *edge;
} ChamgeEdgeBackendPrivate;

typedef enum
{
  PROP_EDGE = 1,

  /*< private > */
  PROP_LAST = PROP_EDGE
} _ChamgeEdgeBackendProperty;

static GParamSpec *properties[PROP_LAST + 1];

/* *INDENT-OFF* */
G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ChamgeEdgeBackend, chamge_edge_backend, G_TYPE_OBJECT)
/* *INDENT-ON* */

static void
chamge_edge_backend_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  ChamgeEdgeBackend *self = CHAMGE_EDGE_BACKEND (object);
  ChamgeEdgeBackendPrivate *priv =
      chamge_edge_backend_get_instance_private (self);

  switch ((_ChamgeEdgeBackendProperty) prop_id) {
    case PROP_EDGE:
      g_value_set_object (value, priv->edge);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_edge_backend_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ChamgeEdgeBackend *self = CHAMGE_EDGE_BACKEND (object);
  ChamgeEdgeBackendPrivate *priv =
      chamge_edge_backend_get_instance_private (self);

  switch ((_ChamgeEdgeBackendProperty) prop_id) {
    case PROP_EDGE:
      priv->edge = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_edge_backend_dispose (GObject * object)
{
  ChamgeEdgeBackend *self = CHAMGE_EDGE_BACKEND (object);
  ChamgeEdgeBackendPrivate *priv =
      chamge_edge_backend_get_instance_private (self);

  g_clear_object (&priv->edge);

  G_OBJECT_CLASS (chamge_edge_backend_parent_class)->dispose (object);
}

static void
chamge_edge_backend_class_init (ChamgeEdgeBackendClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = chamge_edge_backend_get_property;
  object_class->set_property = chamge_edge_backend_set_property;
  object_class->dispose = chamge_edge_backend_dispose;

  properties[PROP_EDGE] = g_param_spec_object ("edge", "edge", "edge",
      CHAMGE_TYPE_EDGE,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties),
      properties);
}

static void
chamge_edge_backend_init (ChamgeEdgeBackend * self)
{
}

ChamgeEdgeBackend *
chamge_edge_backend_new (ChamgeEdge * edge)
{
  return g_object_new (CHAMGE_TYPE_MOCK_EDGE_BACKEND, "edge", edge, NULL);
}

ChamgeReturn
chamge_edge_backend_enroll (ChamgeEdgeBackend * self)
{
  ChamgeEdgeBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_EDGE_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_EDGE_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->enroll != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->enroll (self);

  return ret;
}

ChamgeReturn
chamge_edge_backend_delist (ChamgeEdgeBackend * self)
{
  ChamgeEdgeBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_EDGE_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_EDGE_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->delist != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->delist (self);

  return ret;
}

ChamgeReturn
chamge_edge_backend_activate (ChamgeEdgeBackend * self)
{
  ChamgeEdgeBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_EDGE_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_EDGE_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->activate != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->activate (self);

  return ret;
}

ChamgeReturn
chamge_edge_backend_deactivate (ChamgeEdgeBackend * self)
{
  ChamgeEdgeBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_EDGE_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_EDGE_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->deactivate != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->deactivate (self);

  return ret;
}

gchar *chamge_edge_backend_request_target_uri
    (ChamgeEdgeBackend * self, GError ** error)
{
  g_autofree gchar *target_uri = NULL;
  ChamgeEdgeBackendClass *klass;
  g_return_val_if_fail (CHAMGE_IS_EDGE_BACKEND (self), NULL);

  klass = CHAMGE_EDGE_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->request_target_uri != NULL, NULL);

  target_uri = klass->request_target_uri (self, error);

  return g_steal_pointer (&target_uri);
}
