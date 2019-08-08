/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "edge-backend.h"

#include "edge.h"
#include "enumtypes.h"

typedef struct
{
} ChamgeEdgeBackendPrivate;

/* *INDENT-OFF* */
G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ChamgeEdgeBackend, chamge_edge_backend, CHAMGE_TYPE_NODE_BACKEND,
                                  G_ADD_PRIVATE (ChamgeEdgeBackend))
/* *INDENT-ON* */

static void
chamge_edge_backend_class_init (ChamgeEdgeBackendClass * klass)
{
}

static void
chamge_edge_backend_init (ChamgeEdgeBackend * self)
{
}

ChamgeEdgeBackend *
chamge_edge_backend_new (void)
{
  return NULL;
}
