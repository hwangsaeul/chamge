/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "edge.h"

typedef struct
{
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
chamge_edge_class_init (ChamgeEdgeClass * klass)
{
  klass->request_target_uri = chamge_edge_request_target_uri_default;
}

static void
chamge_edge_init (ChamgeEdge * self)
{
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);
}

ChamgeEdge *
chamge_edge_new (const gchar * uid)
{
  return g_object_new (CHAMGE_TYPE_EDGE, "uid", uid, NULL);
}
