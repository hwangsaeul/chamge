/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "mock-arbiter-backend.h"

struct _ChamgeMockArbiterBackend
{
  ChamgeArbiterBackend parent;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeMockArbiterBackend, chamge_mock_arbiter_backend, CHAMGE_TYPE_ARBITER_BACKEND)
/* *INDENT-ON* */

static ChamgeReturn
chamge_mock_arbiter_backend_enroll (ChamgeArbiterBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_arbiter_backend_delist (ChamgeArbiterBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_arbiter_backend_activate (ChamgeArbiterBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_arbiter_backend_deactivate (ChamgeArbiterBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static void
chamge_mock_arbiter_backend_class_init (ChamgeMockArbiterBackendClass * klass)
{
  ChamgeArbiterBackendClass *backend_class =
      CHAMGE_ARBITER_BACKEND_CLASS (klass);

  backend_class->enroll = chamge_mock_arbiter_backend_enroll;
  backend_class->delist = chamge_mock_arbiter_backend_delist;
  backend_class->activate = chamge_mock_arbiter_backend_activate;
  backend_class->deactivate = chamge_mock_arbiter_backend_deactivate;
}

static void
chamge_mock_arbiter_backend_init (ChamgeMockArbiterBackend * self)
{
}
