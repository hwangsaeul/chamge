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

#include "node.h"

#include "enumtypes.h"

#include <glib.h>
#include <string.h>

typedef struct
{
  GMutex mutex;

  gchar *uid;
  ChamgeNodeState state;

} ChamgeNodePrivate;

typedef enum
{
  PROP_UID = 1,
  PROP_STATE,

  /*< private > */
  PROP_LAST = PROP_STATE
} _ChamgeNodeProperty;

static GParamSpec *properties[PROP_LAST + 1];

enum
{
  SIG_STATE_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* *INDENT-OFF* */
G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ChamgeNode, chamge_node, G_TYPE_OBJECT,
                                  G_ADD_PRIVATE (ChamgeNode))
/* *INDENT-ON* */

static ChamgeReturn
chamge_node_enroll_default (ChamgeNode * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_node_delist_default (ChamgeNode * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_node_activate_default (ChamgeNode * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_node_deactivate_default (ChamgeNode * self)
{
  return CHAMGE_RETURN_OK;
}

static gchar *
chamge_node_get_uid_default (ChamgeNode * self)
{
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);
  return g_strdup (priv->uid);
}

static ChamgeReturn
chamge_node_user_command_default (ChamgeNode * self, const gchar * user_data,
    gchar ** out, GError ** error)
{
  return CHAMGE_RETURN_OK;
}

static void
chamge_node_dispose (GObject * object)
{
  ChamgeNode *self = CHAMGE_NODE (object);
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);

  g_clear_pointer (&priv->uid, g_free);

  G_OBJECT_CLASS (chamge_node_parent_class)->dispose (object);
}

static void
chamge_node_finalize (GObject * object)
{
  ChamgeNode *self = CHAMGE_NODE (object);
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);

  g_mutex_clear (&priv->mutex);

  G_OBJECT_CLASS (chamge_node_parent_class)->finalize (object);
}

static void
chamge_node_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  ChamgeNode *self = CHAMGE_NODE (object);
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);
  g_autoptr (GMutexLocker) locker = NULL;

  switch ((_ChamgeNodeProperty) prop_id) {
    case PROP_UID:
      g_value_set_string (value, priv->uid);
      break;
    case PROP_STATE:
      locker = g_mutex_locker_new (&priv->mutex);
      g_value_set_enum (value, priv->state);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_node_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ChamgeNode *self = CHAMGE_NODE (object);
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);

  switch ((_ChamgeNodeProperty) prop_id) {
    case PROP_UID:
      g_assert (priv->uid == NULL);     /* construct only */
      priv->uid = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_node_class_init (ChamgeNodeClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = chamge_node_get_property;
  object_class->set_property = chamge_node_set_property;
  object_class->dispose = chamge_node_dispose;
  object_class->finalize = chamge_node_finalize;

  properties[PROP_UID] = g_param_spec_string ("uid", "uid", "uid", NULL,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  properties[PROP_STATE] = g_param_spec_enum ("state", "state", "state",
      CHAMGE_TYPE_NODE_STATE, CHAMGE_NODE_STATE_NULL,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties),
      properties);

  /**
   * ChamgeNode::state_changed:
   * @self: a #ChamgeNode object
   *
   * Signal to inform that the state of the Chamge device has changed.
   */
  signals[SIG_STATE_CHANGED] =
      g_signal_new ("state-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (ChamgeNodeClass, state_changed), NULL,
      NULL, g_cclosure_marshal_VOID__ENUM, G_TYPE_NONE, 1,
      CHAMGE_TYPE_NODE_STATE);

  klass->enroll = chamge_node_enroll_default;
  klass->delist = chamge_node_delist_default;
  klass->activate = chamge_node_activate_default;
  klass->deactivate = chamge_node_deactivate_default;
  klass->get_uid = chamge_node_get_uid_default;
  klass->user_command = chamge_node_user_command_default;
}

static void
chamge_node_init (ChamgeNode * self)
{
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);

  g_mutex_init (&priv->mutex);

  priv->state = CHAMGE_NODE_STATE_NULL;
}

static gboolean
enroll_by_uid_group_func (gpointer user_data)
{
  ChamgeNode *self = CHAMGE_NODE (user_data);
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);
  ChamgeNodeClass *klass = CHAMGE_NODE_GET_CLASS (self);
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  g_autoptr (GMutexLocker) locker = NULL;

  g_return_val_if_fail (klass->enroll != NULL, G_SOURCE_REMOVE);
  g_return_val_if_fail (priv->state == CHAMGE_NODE_STATE_NULL,
      CHAMGE_RETURN_FAIL);
  ret = klass->enroll (self);

  if (ret == CHAMGE_RETURN_OK) {
    locker = g_mutex_locker_new (&priv->mutex);
    priv->state = CHAMGE_NODE_STATE_ENROLLED;
    g_clear_pointer (&locker, g_mutex_locker_free);

    g_signal_emit (self, signals[SIG_STATE_CHANGED], 0, priv->state);
  }

  return G_SOURCE_REMOVE;
}

ChamgeReturn
chamge_node_enroll (ChamgeNode * self, gboolean lazy)
{
  ChamgeNodeClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);
  g_autoptr (GMutexLocker) locker = NULL;

  g_return_val_if_fail (CHAMGE_IS_NODE (self), CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->state == CHAMGE_NODE_STATE_NULL,
      CHAMGE_RETURN_FAIL);

  klass = CHAMGE_NODE_GET_CLASS (self);

  g_return_val_if_fail (klass->enroll != NULL, CHAMGE_RETURN_FAIL);

  if (lazy) {
    const gchar *md5_digest;
    guint group_time_ms, trigger_ms;
    gint64 rtime_ms;
    g_autoptr (GChecksum) md5 = NULL;

    md5 = g_checksum_new (G_CHECKSUM_MD5);
    g_checksum_update (md5, (const guchar *) priv->uid, strlen (priv->uid));
    md5_digest = g_checksum_get_string (md5);

    /* 16 groups in 100 seconds */
    group_time_ms =
        g_ascii_strtoull (md5_digest + strlen (md5_digest) - 1, NULL,
        16) * 6250;

    rtime_ms = g_get_real_time () / 1000;
    trigger_ms =
        rtime_ms % 100000 >
        group_time_ms ? 100000 + group_time_ms -
        rtime_ms % 100000 : group_time_ms - rtime_ms % 100000;

    g_timeout_add (trigger_ms, enroll_by_uid_group_func, self);

    ret = CHAMGE_RETURN_ASYNC;

    goto out;
  }

  ret = klass->enroll (self);

  if (ret == CHAMGE_RETURN_OK) {
    locker = g_mutex_locker_new (&priv->mutex);
    priv->state = CHAMGE_NODE_STATE_ENROLLED;
    g_signal_emit (self, signals[SIG_STATE_CHANGED], 0, priv->state);
  }

out:
  return ret;
}

ChamgeReturn
chamge_node_delist (ChamgeNode * self)
{
  ChamgeNodeClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);
  g_autoptr (GMutexLocker) locker = NULL;

  g_return_val_if_fail (CHAMGE_IS_NODE (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_NODE_GET_CLASS (self);
  g_return_val_if_fail (klass->delist != NULL, CHAMGE_RETURN_FAIL);

  /* Check return value by local variable to make debugger trace stack easy */
  ret = klass->delist (self);

  if (ret == CHAMGE_RETURN_OK) {
    locker = g_mutex_locker_new (&priv->mutex);
    priv->state = CHAMGE_NODE_STATE_NULL;
    g_signal_emit (self, signals[SIG_STATE_CHANGED], 0, priv->state);
  }

  return ret;
}

ChamgeReturn
chamge_node_activate (ChamgeNode * self)
{
  ChamgeNodeClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);
  g_autoptr (GMutexLocker) locker = NULL;

  g_return_val_if_fail (CHAMGE_IS_NODE (self), CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->state == CHAMGE_NODE_STATE_ENROLLED,
      CHAMGE_RETURN_FAIL);

  klass = CHAMGE_NODE_GET_CLASS (self);
  g_return_val_if_fail (klass->activate != NULL, CHAMGE_RETURN_FAIL);

  /* Check return value by local variable to make debugger trace stack easy */
  ret = klass->activate (self);

  if (ret == CHAMGE_RETURN_OK) {
    locker = g_mutex_locker_new (&priv->mutex);
    priv->state = CHAMGE_NODE_STATE_ACTIVATED;
    g_signal_emit (self, signals[SIG_STATE_CHANGED], 0, priv->state);
  }

  return ret;
}

ChamgeReturn
chamge_node_deactivate (ChamgeNode * self)
{
  ChamgeNodeClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);
  g_autoptr (GMutexLocker) locker = NULL;

  g_return_val_if_fail (CHAMGE_IS_NODE (self), CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->state == CHAMGE_NODE_STATE_ACTIVATED,
      CHAMGE_RETURN_FAIL);

  klass = CHAMGE_NODE_GET_CLASS (self);
  g_return_val_if_fail (klass->deactivate != NULL, CHAMGE_RETURN_FAIL);

  /* Check return value by local variable to make debugger trace stack easy */
  ret = klass->deactivate (self);

  if (ret == CHAMGE_RETURN_OK) {
    locker = g_mutex_locker_new (&priv->mutex);
    priv->state = CHAMGE_NODE_STATE_ENROLLED;
    g_signal_emit (self, signals[SIG_STATE_CHANGED], 0, priv->state);
  }

  return ret;
}

ChamgeReturn
chamge_node_get_uid (ChamgeNode * self, gchar ** uid)
{
  ChamgeNodeClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;

  g_return_val_if_fail (CHAMGE_IS_NODE (self), CHAMGE_RETURN_FAIL);

  klass = CHAMGE_NODE_GET_CLASS (self);
  g_return_val_if_fail (klass->get_uid != NULL, CHAMGE_RETURN_FAIL);

  *uid = klass->get_uid (self);
  if (*uid == NULL)
    ret = CHAMGE_RETURN_FAIL;

  return ret;
}

ChamgeReturn
chamge_node_user_command (ChamgeNode * self, const gchar * cmd, gchar ** out,
    GError ** error)
{
  ChamgeNodeClass *klass;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  ChamgeNodePrivate *priv = chamge_node_get_instance_private (self);

  g_return_val_if_fail (CHAMGE_IS_NODE (self), CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->state == CHAMGE_NODE_STATE_ACTIVATED,
      CHAMGE_RETURN_FAIL);

  klass = CHAMGE_NODE_GET_CLASS (self);
  g_return_val_if_fail (klass->user_command != NULL, CHAMGE_RETURN_FAIL);

  ret = klass->user_command (self, cmd, out, error);
  if (ret == CHAMGE_RETURN_FAIL)
    g_debug ("error in user command: %s", *error ? (*error)->message : "NULL");

  return ret;
}
