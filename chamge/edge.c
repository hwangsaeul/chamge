/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "edge.h"
#include "edge-backend.h"

typedef struct
{
  ChamgeEdgeBackend *backend;

  gint n_stream;
} ChamgeEdgePrivate;

/* *INDENT-OFF* */
G_DEFINE_TYPE_WITH_CODE (ChamgeEdge, chamge_edge, CHAMGE_TYPE_NODE,
                         G_ADD_PRIVATE (ChamgeEdge))
/* *INDENT-ON* */

static gchar *
chamge_edge_request_target_uri_default (ChamgeEdge * self, GError ** error)
{
  return NULL;
}

static void
chamge_edge_dispose (GObject * object)
{
  ChamgeEdge *self = CHAMGE_EDGE (object);
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);

  g_clear_object (&priv->backend);
  G_OBJECT_CLASS (chamge_edge_parent_class)->dispose (object);
}

static void
chamge_edge_class_init (ChamgeEdgeClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = chamge_edge_dispose;

  klass->request_target_uri = chamge_edge_request_target_uri_default;
}

static void
chamge_edge_init (ChamgeEdge * self)
{
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);
  priv->backend = chamge_edge_backend_new ();

}

ChamgeEdge *
chamge_edge_new (const gchar * uid)
{
  return g_object_new (CHAMGE_TYPE_EDGE, "uid", uid, NULL);
}
