/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "amqp-arbiter-backend.h"

struct _ChamgeAmqpArbiterBackend
{
  ChamgeArbiterBackend parent;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeAmqpArbiterBackend, chamge_amqp_arbiter_backend, CHAMGE_TYPE_ARBITER_BACKEND)
/* *INDENT-ON* */

static ChamgeReturn
chamge_amqp_arbiter_backend_enroll (ChamgeArbiterBackend * arbiter_backend)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_amqp_arbiter_backend_delist (ChamgeArbiterBackend * arbiter_backend)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_amqp_arbiter_backend_activate (ChamgeArbiterBackend * arbiter_backend)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_amqp_arbiter_backend_deactivate (ChamgeArbiterBackend * arbiter_backend)
{
  return CHAMGE_RETURN_OK;
}

static void
chamge_amqp_arbiter_backend_class_init (ChamgeAmqpArbiterBackendClass * klass)
{
  ChamgeArbiterBackendClass *backend_class =
      CHAMGE_ARBITER_BACKEND_CLASS (klass);

  backend_class->enroll = chamge_amqp_arbiter_backend_enroll;
  backend_class->delist = chamge_amqp_arbiter_backend_delist;
  backend_class->activate = chamge_amqp_arbiter_backend_activate;
  backend_class->deactivate = chamge_amqp_arbiter_backend_deactivate;
}

static void
chamge_amqp_arbiter_backend_init (ChamgeAmqpArbiterBackend * self)
{
}
