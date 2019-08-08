/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "node-backend.h"

#include "node.h"
#include "enumtypes.h"

typedef struct
{
  ChamgeNode *node;
} ChamgeNodeBackendPrivate;

/* *INDENT-OFF* */
G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ChamgeNodeBackend, chamge_node_backend, G_TYPE_OBJECT,
                                  G_ADD_PRIVATE (ChamgeNodeBackend))
/* *INDENT-ON* */

static void
chamge_node_backend_dispose (GObject * object)
{
  ChamgeNodeBackend *self = CHAMGE_NODE_BACKEND (object);
  ChamgeNodeBackendPrivate *priv =
      chamge_node_backend_get_instance_private (self);

  g_clear_object (&priv->node);

  G_OBJECT_CLASS (chamge_node_backend_parent_class)->dispose (object);
}

static void
chamge_node_backend_class_init (ChamgeNodeBackendClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = chamge_node_backend_dispose;
}

static void
chamge_node_backend_init (ChamgeNodeBackend * self)
{
}
