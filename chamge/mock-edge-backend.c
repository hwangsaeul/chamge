/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "mock-edge-backend.h"

struct _ChamgeMockEdgeBackend
{

};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeMockEdgeBackend, chamge_mock_edge_backend, CHAMGE_TYPE_EDGE_BACKEND)
/* *INDENT-ON* */

static void
chamge_mock_edge_backend_class_init (ChamgeMockEdgeBackendClass * klass)
{
}

static void
chamge_mock_edge_backend_init (ChamgeMockEdgeBackend * self)
{
}
