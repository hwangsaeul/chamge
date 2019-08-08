/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_NODE_BACKEND_H__
#define __CHAMGE_NODE_BACKEND_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#include <glib-object.h>
#include <chamge/types.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_NODE_BACKEND       (chamge_node_backend_get_type ())
CHAMGE_API_EXPORT
G_DECLARE_DERIVABLE_TYPE (ChamgeNodeBackend, chamge_node_backend, CHAMGE, NODE_BACKEND, GObject)

struct _ChamgeNodeBackendClass
{
  GObjectClass parent_class;

    /* methods */
  ChamgeReturn (* enroll)               (ChamgeNodeBackend *self, gboolean lazy);
  ChamgeReturn (* delist)               (ChamgeNodeBackend *self);

  ChamgeReturn (* activate)             (ChamgeNodeBackend *self);
  ChamgeReturn (* deactivate)           (ChamgeNodeBackend *self);
};

G_END_DECLS

#endif // __CHAMGE_NODE_BACKEND_H__
