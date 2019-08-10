/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "node-backend.h"

#include "node.h"
#include "enumtypes.h"

/* *INDENT-OFF* */
G_DEFINE_ABSTRACT_TYPE (ChamgeNodeBackend, chamge_node_backend, G_TYPE_OBJECT)
                                  
/* *INDENT-ON* */

static void
chamge_node_backend_dispose (GObject * object)
{
  ChamgeNodeBackend *self = CHAMGE_NODE_BACKEND (object);

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
