/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "edge.h"
#include "edge-backend.h"

typedef struct
{
  ChamgeEdgeBackend *backend;

  gint n_stream;
} ChamgeEdgePrivate;

/* *INDENT-OFF* */
G_DEFINE_TYPE_WITH_CODE (ChamgeEdge, chamge_edge, CHAMGE_TYPE_NODE,
                         G_ADD_PRIVATE (ChamgeEdge))
/* *INDENT-ON* */

static gchar *
chamge_edge_request_target_uri_default (ChamgeEdge * self, GError ** error)
{
  return NULL;
}

static ChamgeReturn
lazy_enroll (ChamgeEdge * self)
{
  ChamgeReturn ret = CHAMGE_RETURN_OK;

  return ret;
}

static gboolean
enroll_by_uid_group_func (gpointer user_data)
{
  ChamgeEdge *self = CHAMGE_EDGE (user_data);
  ChamgeReturn ret = CHAMGE_RETURN_OK;

  ret = lazy_enroll (self);

  if (ret == CHAMGE_RETURN_OK) {
    g_signal_emit_by_name (self, "state-changed", 0,
        CHAMGE_NODE_STATE_ENROLLED);
  }

  return G_SOURCE_REMOVE;
}

static ChamgeReturn
chamge_edge_enroll (ChamgeNode * node, gboolean lazy)
{
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  ChamgeEdge *self = CHAMGE_EDGE (node);
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);

  if (lazy) {
    const gchar *md5_digest;
    guint group_time_ms, trigger_ms;
    gint64 rtime_ms;
    g_autoptr (GChecksum) md5 = NULL;
    g_autofree gchar *uid = NULL;

    g_object_get (self, "uid", &uid, NULL);

    md5 = g_checksum_new (G_CHECKSUM_MD5);
    g_checksum_update (md5, (const guchar *) uid, strlen (uid));
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
  }

  return ret;
}


static void
chamge_edge_dispose (GObject * object)
{
  ChamgeEdge *self = CHAMGE_EDGE (object);
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);

  g_clear_object (&priv->backend);
  G_OBJECT_CLASS (chamge_edge_parent_class)->dispose (object);
}

static void
chamge_edge_class_init (ChamgeEdgeClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ChamgeNodeClass *node_class = CHAMGE_NODE_CLASS (klass);

  object_class->dispose = chamge_edge_dispose;

  node_class->enroll = chamge_edge_enroll;

  klass->request_target_uri = chamge_edge_request_target_uri_default;
}

static void
chamge_edge_init (ChamgeEdge * self)
{
  ChamgeEdgePrivate *priv = chamge_edge_get_instance_private (self);
  priv->backend = chamge_edge_backend_new ();

}

ChamgeEdge *
chamge_edge_new (const gchar * uid)
{
  return g_object_new (CHAMGE_TYPE_EDGE, "uid", uid, NULL);
}
