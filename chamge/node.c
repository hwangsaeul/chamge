/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "node.h"

#include "enumtypes.h"

#include <glib.h>

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

  signals[SIG_STATE_CHANGED] =
      g_signal_new ("state-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (ChamgeNodeClass, state_changed), NULL,
      NULL, g_cclosure_marshal_VOID__ENUM, G_TYPE_NONE, 1,
      CHAMGE_TYPE_NODE_STATE);

  klass->enroll = chamge_node_enroll_default;
  klass->delist = chamge_node_delist_default;
  klass->activate = chamge_node_activate_default;
  klass->deactivate = chamge_node_deactivate_default;
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

  klass = CHAMGE_NODE_GET_CLASS (self);
  g_return_val_if_fail (klass->activate != NULL, CHAMGE_RETURN_FAIL);

  /* Check return value by local variable to make debugger trace stack easy */
  ret = klass->activate (self);

  if (ret == CHAMGE_RETURN_OK) {
    locker = g_mutex_locker_new (&priv->mutex);
    priv->state = CHAMGE_NODE_STATE_ACTIVATED;
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

  klass = CHAMGE_NODE_GET_CLASS (self);
  g_return_val_if_fail (klass->deactivate != NULL, CHAMGE_RETURN_FAIL);

  /* Check return value by local variable to make debugger trace stack easy */
  ret = klass->deactivate (self);

  if (ret == CHAMGE_RETURN_OK) {
    locker = g_mutex_locker_new (&priv->mutex);
    priv->state = CHAMGE_NODE_STATE_ENROLLED;
  }

  return ret;
}
