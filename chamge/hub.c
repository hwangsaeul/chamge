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

#include "hub.h"
#include "enumtypes.h"
#include "hub-backend.h"

typedef struct
{
  ChamgeBackend backend;

  ChamgeHubBackend *hub_backend;

  gint n_stream;
} ChamgeHubPrivate;

typedef enum
{
  PROP_BACKEND = 1,

  /*< private > */
  PROP_LAST = PROP_BACKEND
} _ChamgeHubProperty;

enum
{
  SIG_USER_COMMAND,

  LAST_SIGNAL
};

static GParamSpec *properties[PROP_LAST + 1];
static guint signals[LAST_SIGNAL] = { 0 };

/* *INDENT-OFF* */
G_DEFINE_TYPE_WITH_CODE (ChamgeHub, chamge_hub, CHAMGE_TYPE_NODE,
                         G_ADD_PRIVATE (ChamgeHub))
/* *INDENT-ON* */

static gchar *
chamge_hub_get_uid_default (ChamgeHub * self)
{
  ChamgeNode *node = CHAMGE_NODE (self);
  gchar *uid = NULL;

  g_object_get (node, "uid", &uid, NULL);

  return uid;
}

static ChamgeReturn
chamge_hub_user_command_cb (const gchar * cmd, gchar ** response,
    GError ** error, ChamgeHubBackend * self)
{
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;
  ChamgeHub *hub = NULL;

  g_object_get (self, "hub", &hub, NULL);
  g_assert (hub != NULL);

  g_signal_emit (hub, signals[SIG_USER_COMMAND], 0, cmd, response, error, &ret);

  return ret;
}

static ChamgeReturn
chamge_hub_enroll (ChamgeNode * node)
{
  ChamgeHub *self = CHAMGE_HUB (node);
  ChamgeHubPrivate *priv = chamge_hub_get_instance_private (self);
  ChamgeReturn ret;

  if (priv->hub_backend == NULL)
    priv->hub_backend = chamge_hub_backend_new (self);

  chamge_hub_backend_set_user_command_handler (priv->hub_backend,
      chamge_hub_user_command_cb);
  ret = chamge_hub_backend_enroll (priv->hub_backend);

  return ret;
}

static ChamgeReturn
chamge_hub_delist (ChamgeNode * node)
{
  ChamgeHub *self = CHAMGE_HUB (node);
  ChamgeHubPrivate *priv = chamge_hub_get_instance_private (self);
  ChamgeReturn ret;

  ret = chamge_hub_backend_delist (priv->hub_backend);

  g_clear_object (&priv->hub_backend);

  return ret;
}

static ChamgeReturn
chamge_hub_activate (ChamgeNode * node)
{
  ChamgeHub *self = CHAMGE_HUB (node);
  ChamgeHubPrivate *priv = chamge_hub_get_instance_private (self);
  ChamgeReturn ret;

  ret = chamge_hub_backend_activate (priv->hub_backend);

  return ret;
}

static ChamgeReturn
chamge_hub_deactivate (ChamgeNode * node)
{
  ChamgeHub *self = CHAMGE_HUB (node);
  ChamgeHubPrivate *priv = chamge_hub_get_instance_private (self);
  ChamgeReturn ret;

  ret = chamge_hub_backend_deactivate (priv->hub_backend);

  return ret;
}

static ChamgeReturn
chamge_hub_user_command (ChamgeNode * node, const gchar * cmd, gchar ** out,
    GError ** error)
{
  ChamgeHub *self = CHAMGE_HUB (node);
  ChamgeHubPrivate *priv = chamge_hub_get_instance_private (self);

  return chamge_hub_backend_user_command_send (priv->hub_backend, cmd, out,
      error);
}


static void
chamge_hub_dispose (GObject * object)
{
  ChamgeHub *self = CHAMGE_HUB (object);
  ChamgeHubPrivate *priv = chamge_hub_get_instance_private (self);

  g_clear_object (&priv->hub_backend);
  G_OBJECT_CLASS (chamge_hub_parent_class)->dispose (object);
}

static void
chamge_hub_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  ChamgeHub *self = CHAMGE_HUB (object);
  ChamgeHubPrivate *priv = chamge_hub_get_instance_private (self);

  switch ((_ChamgeHubProperty) prop_id) {
    case PROP_BACKEND:
      g_value_set_enum (value, priv->backend);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_hub_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ChamgeHub *self = CHAMGE_HUB (object);
  ChamgeHubPrivate *priv = chamge_hub_get_instance_private (self);

  switch ((_ChamgeHubProperty) prop_id) {
    case PROP_BACKEND:
      g_assert (priv->backend == CHAMGE_BACKEND_UNKNOWN);       /* construct only */
      priv->backend = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_hub_class_init (ChamgeHubClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ChamgeNodeClass *node_class = CHAMGE_NODE_CLASS (klass);

  object_class->get_property = chamge_hub_get_property;
  object_class->set_property = chamge_hub_set_property;
  object_class->dispose = chamge_hub_dispose;

  properties[PROP_BACKEND] = g_param_spec_enum ("backend", "backend", "backend",
      CHAMGE_TYPE_BACKEND, CHAMGE_BACKEND_UNKNOWN,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties),
      properties);

  signals[SIG_USER_COMMAND] =
      g_signal_new ("user-command", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (ChamgeHubClass, user_command), NULL,
      NULL, g_cclosure_marshal_generic, G_TYPE_INT, 3,
      G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER);

  node_class->enroll = chamge_hub_enroll;
  node_class->delist = chamge_hub_delist;
  node_class->activate = chamge_hub_activate;
  node_class->deactivate = chamge_hub_deactivate;
  node_class->user_command = chamge_hub_user_command;

  klass->get_uid = chamge_hub_get_uid_default;
}

static void
chamge_hub_init (ChamgeHub * self)
{
}

ChamgeHub *
chamge_hub_new (const gchar * uid)
{
  return chamge_hub_new_full (uid, CHAMGE_BACKEND_AMQP);
}

ChamgeHub *
chamge_hub_new_full (const gchar * uid, ChamgeBackend backend)
{
  g_assert_nonnull (uid);
  return g_object_new (CHAMGE_TYPE_HUB, "uid", uid, "backend", backend, NULL);
}

ChamgeReturn
chamge_hub_get_uid (ChamgeHub * self, gchar ** uid)
{
  ChamgeHubClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;

  g_return_val_if_fail (CHAMGE_IS_HUB (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_HUB_GET_CLASS (self);
  g_return_val_if_fail (klass->get_uid != NULL, CHAMGE_RETURN_FAIL);

  *uid = klass->get_uid (self);
  if (*uid == NULL)
    ret = CHAMGE_RETURN_FAIL;

  return ret;
}
