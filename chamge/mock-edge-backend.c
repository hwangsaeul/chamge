/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "mock-edge-backend.h"

struct _ChamgeMockEdgeBackend
{
  ChamgeEdgeBackend parent;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeMockEdgeBackend, chamge_mock_edge_backend, CHAMGE_TYPE_EDGE_BACKEND)
/* *INDENT-ON* */

static ChamgeReturn
chamge_mock_edge_backend_enroll (ChamgeEdgeBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_edge_backend_delist (ChamgeEdgeBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_edge_backend_activate (ChamgeEdgeBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_edge_backend_deactivate (ChamgeEdgeBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static gchar *
chamge_mock_edge_backend_request_target_uri (ChamgeEdgeBackend * self,
    GError ** error)
{
  g_autofree gchar *target_uri = g_strdup ("srt://localhost:8888");
  return g_steal_pointer (&target_uri);
}


static void
chamge_mock_edge_backend_class_init (ChamgeMockEdgeBackendClass * klass)
{
  ChamgeEdgeBackendClass *backend_class = CHAMGE_EDGE_BACKEND_CLASS (klass);

  backend_class->enroll = chamge_mock_edge_backend_enroll;
  backend_class->delist = chamge_mock_edge_backend_delist;
  backend_class->activate = chamge_mock_edge_backend_activate;
  backend_class->deactivate = chamge_mock_edge_backend_deactivate;
  backend_class->request_target_uri =
      chamge_mock_edge_backend_request_target_uri;

}

static void
chamge_mock_edge_backend_init (ChamgeMockEdgeBackend * self)
{
}
