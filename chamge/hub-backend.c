/**
 *  Copyright 2019 SK Telecom Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *            Heekyoung Seo <hkseo@sk.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include "config.h"

#include "hub-backend.h"

#include "hub.h"
#include "enumtypes.h"

/* FIXME: Choosing a backend in compile time? */
#include "mock-hub-backend.h"
#include "amqp-hub-backend.h"

typedef struct
{
  ChamgeHub *hub;

  GClosure *user_command;
} ChamgeHubBackendPrivate;

typedef enum
{
  PROP_HUB = 1,

  /*< private > */
  PROP_LAST = PROP_HUB
} _ChamgeHubBackendProperty;

static GParamSpec *properties[PROP_LAST + 1];

/* *INDENT-OFF* */
G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ChamgeHubBackend, chamge_hub_backend, G_TYPE_OBJECT)
/* *INDENT-ON* */

static void
chamge_hub_backend_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  ChamgeHubBackend *self = CHAMGE_HUB_BACKEND (object);
  ChamgeHubBackendPrivate *priv =
      chamge_hub_backend_get_instance_private (self);

  switch ((_ChamgeHubBackendProperty) prop_id) {
    case PROP_HUB:
      g_value_set_object (value, priv->hub);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_hub_backend_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ChamgeHubBackend *self = CHAMGE_HUB_BACKEND (object);
  ChamgeHubBackendPrivate *priv =
      chamge_hub_backend_get_instance_private (self);

  switch ((_ChamgeHubBackendProperty) prop_id) {
    case PROP_HUB:
      priv->hub = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static ChamgeReturn
chamge_hub_backend_user_command (ChamgeHubBackend * self,
    const gchar * cmd, gchar ** response, GError ** error)
{
  ChamgeHubBackendPrivate *priv =
      chamge_hub_backend_get_instance_private (self);
  ChamgeReturn ret = CHAMGE_RETURN_OK;

  if (priv->user_command != NULL) {
    GValue in_values[3] = { G_VALUE_INIT };
    GValue out_value = G_VALUE_INIT;
    g_value_init (&in_values[0], G_TYPE_STRING);
    g_value_set_string (&in_values[0], cmd);
    g_value_init (&in_values[1], G_TYPE_POINTER);
    g_value_set_pointer (&in_values[1], response);
    g_value_init (&in_values[2], G_TYPE_POINTER);
    g_value_set_pointer (&in_values[2], error);

    g_value_init (&out_value, G_TYPE_INT);
    g_closure_invoke (priv->user_command, &out_value, 3, in_values, NULL);
    ret = g_value_get_int (&out_value);
  }

  return ret;
}

static void
chamge_hub_backend_dispose (GObject * object)
{
  ChamgeHubBackend *self = CHAMGE_HUB_BACKEND (object);
  ChamgeHubBackendPrivate *priv =
      chamge_hub_backend_get_instance_private (self);

  g_clear_object (&priv->hub);

  g_clear_pointer (&priv->user_command, g_closure_unref);

  G_OBJECT_CLASS (chamge_hub_backend_parent_class)->dispose (object);
}

static void
chamge_hub_backend_class_init (ChamgeHubBackendClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = chamge_hub_backend_get_property;
  object_class->set_property = chamge_hub_backend_set_property;
  object_class->dispose = chamge_hub_backend_dispose;

  properties[PROP_HUB] = g_param_spec_object ("hub", "hub", "hub",
      CHAMGE_TYPE_HUB,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties),
      properties);

  klass->user_command = chamge_hub_backend_user_command;
}

static void
chamge_hub_backend_init (ChamgeHubBackend * self)
{
}

ChamgeHubBackend *
chamge_hub_backend_new (ChamgeHub * hub)
{
  ChamgeBackend backend;
  GType backend_type;
  g_autoptr (ChamgeHubBackend) hub_backend = NULL;

  g_return_val_if_fail (CHAMGE_IS_HUB (hub), NULL);

  g_object_get (hub, "backend", &backend, NULL);

  if (backend == CHAMGE_BACKEND_AMQP) {
    backend_type = CHAMGE_TYPE_AMQP_HUB_BACKEND;
  } else {
    backend_type = CHAMGE_TYPE_MOCK_HUB_BACKEND;
  }

  hub_backend = g_object_new (backend_type, "hub", hub, NULL);
  return g_steal_pointer (&hub_backend);
}

ChamgeReturn
chamge_hub_backend_enroll (ChamgeHubBackend * self)
{
  ChamgeHubBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_HUB_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_HUB_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->enroll != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->enroll (self);

  return ret;
}

ChamgeReturn
chamge_hub_backend_delist (ChamgeHubBackend * self)
{
  ChamgeHubBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_HUB_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_HUB_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->delist != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->delist (self);

  return ret;
}

ChamgeReturn
chamge_hub_backend_activate (ChamgeHubBackend * self)
{
  ChamgeHubBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_HUB_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_HUB_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->activate != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->activate (self);

  return ret;
}

ChamgeReturn
chamge_hub_backend_deactivate (ChamgeHubBackend * self)
{
  ChamgeHubBackendClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_return_val_if_fail (CHAMGE_IS_HUB_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_HUB_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->deactivate != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->deactivate (self);

  return ret;
}

ChamgeReturn
chamge_hub_backend_user_command_send (ChamgeHubBackend * self,
    const gchar * cmd, gchar ** out, GError ** error)
{
  ChamgeHubBackendClass *klass;
  g_return_val_if_fail (CHAMGE_IS_HUB_BACKEND (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_HUB_BACKEND_GET_CLASS (self);
  g_return_val_if_fail (klass->user_command != NULL, CHAMGE_RETURN_FAIL);

  return klass->user_command_send (self, cmd, out, error);
}


void
chamge_hub_backend_set_user_command_handler (ChamgeHubBackend * self,
    ChamgeHubBackendUserCommand user_command)
{
  ChamgeHubBackendPrivate *priv =
      chamge_hub_backend_get_instance_private (self);

  g_return_if_fail (CHAMGE_IS_HUB_BACKEND (self));

  if (priv->user_command != NULL) {
    g_debug ("streaming handlers are already set, it will be reset");
  }
  g_clear_pointer (&priv->user_command, g_closure_unref);

  if (user_command != NULL) {
    priv->user_command = g_cclosure_new (G_CALLBACK (user_command), self, NULL);
    g_closure_set_marshal (priv->user_command, g_cclosure_marshal_generic);
  }
}
