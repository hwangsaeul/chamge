/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "arbiter-backend.h"

#include "arbiter.h"
#include "enumtypes.h"

#include "mock-arbiter-backend.h"
#include "amqp-arbiter-backend.h"

typedef struct
{
  ChamgeArbiter *arbiter;

  GClosure *edge_enrolled;
  GClosure *edge_delisted;
  GClosure *edge_connection_requested;
} ChamgeArbiterBackendPrivate;

typedef enum
{
  PROP_ARBITER = 1,

  /*< private > */
  PROP_LAST = PROP_ARBITER
} _ChamgeArbiterBackendProperty;

static GParamSpec *properties[PROP_LAST + 1];

/* *INDENT-OFF* */
G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ChamgeArbiterBackend, chamge_arbiter_backend, G_TYPE_OBJECT)
/* *INDENT-ON* */

static void
chamge_arbiter_backend_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  ChamgeArbiterBackend *self = CHAMGE_ARBITER_BACKEND (object);
  ChamgeArbiterBackendPrivate *priv =
      chamge_arbiter_backend_get_instance_private (self);

  switch ((_ChamgeArbiterBackendProperty) prop_id) {
    case PROP_ARBITER:
      g_value_set_object (value, priv->arbiter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_arbiter_backend_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ChamgeArbiterBackend *self = CHAMGE_ARBITER_BACKEND (object);
  ChamgeArbiterBackendPrivate *priv =
      chamge_arbiter_backend_get_instance_private (self);

  switch ((_ChamgeArbiterBackendProperty) prop_id) {
    case PROP_ARBITER:
      priv->arbiter = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_arbiter_backend_edge_enrolled (ChamgeArbiterBackend * self,
    const gchar * edge_id)
{
  ChamgeArbiterBackendClass *klass = CHAMGE_ARBITER_BACKEND_GET_CLASS (self);

  g_return_if_fail (klass->approve != NULL);

  /* TODO: fire event to application */
  klass->approve (self, edge_id);
}

static void
chamge_arbiter_backend_dispose (GObject * object)
{
  ChamgeArbiterBackend *self = CHAMGE_ARBITER_BACKEND (object);
  ChamgeArbiterBackendPrivate *priv =
      chamge_arbiter_backend_get_instance_private (self);

  g_clear_object (&priv->arbiter);

  g_clear_pointer (&priv->edge_enrolled, g_closure_unref);
  g_clear_pointer (&priv->edge_delisted, g_closure_unref);
  g_clear_pointer (&priv->edge_connection_requested, g_closure_unref);

  G_OBJECT_CLASS (chamge_arbiter_backend_parent_class)->dispose (object);
}

static void
chamge_arbiter_backend_class_init (ChamgeArbiterBackendClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = chamge_arbiter_backend_get_property;
  object_class->set_property = chamge_arbiter_backend_set_property;
  object_class->dispose = chamge_arbiter_backend_dispose;

  properties[PROP_ARBITER] =
      g_param_spec_object ("arbiter", "arbiter", "arbiter", CHAMGE_TYPE_ARBITER,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties),
      properties);

  klass->edge_enrolled = chamge_arbiter_backend_edge_enrolled;
}

static void
chamge_arbiter_backend_init (ChamgeArbiterBackend * self)
{
}

ChamgeArbiterBackend *
chamge_arbiter_backend_new (ChamgeArbiter * arbiter)
{
  ChamgeBackend backend;
  GType backend_type;
  g_autoptr (ChamgeArbiterBackend) arbiter_backend = NULL;

  g_return_val_if_fail (CHAMGE_IS_ARBITER (arbiter), NULL);

  g_object_get (arbiter, "backend", &backend, NULL);

  if (backend == CHAMGE_BACKEND_AMQP) {
    backend_type = CHAMGE_TYPE_AMQP_ARBITER_BACKEND;
  } else {
    backend_type = CHAMGE_TYPE_MOCK_ARBITER_BACKEND;
  }

  arbiter_backend = g_object_new (backend_type, "arbiter", arbiter, NULL);
  return g_steal_pointer (&arbiter_backend);
}

ChamgeReturn
chamge_arbiter_backend_enroll (ChamgeArbiterBackend * self)
{
  ChamgeArbiterBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_ARBITER_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_ARBITER_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->enroll != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->enroll (self);

  return ret;
}

ChamgeReturn
chamge_arbiter_backend_delist (ChamgeArbiterBackend * self)
{
  ChamgeArbiterBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_ARBITER_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_ARBITER_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->delist != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->delist (self);

  return ret;
}

ChamgeReturn
chamge_arbiter_backend_activate (ChamgeArbiterBackend * self)
{
  ChamgeArbiterBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_ARBITER_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_ARBITER_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->activate != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->activate (self);

  return ret;
}

ChamgeReturn
chamge_arbiter_backend_deactivate (ChamgeArbiterBackend * self)
{
  ChamgeArbiterBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_ARBITER_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_ARBITER_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->deactivate != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->deactivate (self);

  return ret;
}

void
chamge_arbiter_backend_approve (ChamgeArbiterBackend * self,
    const gchar * edge_id)
{
  ChamgeArbiterBackendClass *klass;
  g_return_if_fail (CHAMGE_IS_ARBITER_BACKEND (self));

  klass = CHAMGE_ARBITER_BACKEND_GET_CLASS (self);
  g_return_if_fail (klass->approve != NULL);

  klass->approve (self, edge_id);
}

void
chamge_arbiter_backend_set_edge_handler (ChamgeArbiterBackend * self,
    ChamgeArbiterBackendEdgeEnrolled edge_enrolled,
    ChamgeArbiterBackendEdgeDelisted edge_delisted,
    ChamgeArbiterBackendConnectionRequested edge_connection_requested)
{
  ChamgeArbiterBackendPrivate *priv =
      chamge_arbiter_backend_get_instance_private (self);

  g_return_if_fail (CHAMGE_IS_ARBITER_BACKEND (self));

  if (edge_enrolled != NULL) {
    priv->edge_enrolled =
        g_cclosure_new (G_CALLBACK (edge_enrolled), self, NULL);
    g_closure_set_marshal (priv->edge_enrolled, g_cclosure_marshal_generic);
  }

  if (edge_delisted != NULL) {
    priv->edge_delisted =
        g_cclosure_new (G_CALLBACK (edge_delisted), self, NULL);
    g_closure_set_marshal (priv->edge_delisted, g_cclosure_marshal_generic);
  }

  if (edge_connection_requested != NULL) {
    priv->edge_connection_requested =
        g_cclosure_new (G_CALLBACK (edge_connection_requested), self, NULL);
    g_closure_set_marshal (priv->edge_connection_requested,
        g_cclosure_marshal_generic);
  }
}
