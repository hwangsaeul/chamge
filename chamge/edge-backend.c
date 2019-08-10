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
G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ChamgeEdgeBackend, chamge_edge_backend, CHAMGE_TYPE_NODE_BACKEND)
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
  return CHAMGE_RETURN_OK;
}

ChamgeReturn
chamge_edge_backend_delist (ChamgeEdgeBackend * self)
{
  return CHAMGE_RETURN_OK;
}

ChamgeReturn
chamge_edge_backend_activate (ChamgeEdgeBackend * self)
{
  return CHAMGE_RETURN_OK;
}

ChamgeReturn
chamge_edge_backend_deactivate (ChamgeEdgeBackend * self)
{
  return CHAMGE_RETURN_OK;
}

ChamgeReturn chagme_edge_backend_request_target_uri
    (ChamgeEdgeBackend * self, GError ** error)
{
  return CHAMGE_RETURN_OK;
}
