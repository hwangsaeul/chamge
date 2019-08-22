/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_EDGE_H__
#define __CHAMGE_EDGE_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#include <chamge/node.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_EDGE       (chamge_edge_get_type ())
CHAMGE_API_EXPORT
G_DECLARE_DERIVABLE_TYPE (ChamgeEdge, chamge_edge, CHAMGE, EDGE, ChamgeNode)

struct _ChamgeEdgeClass
{
  ChamgeNodeClass parent_class;
  
  /* methods */
  gchar* (* request_target_uri)         (ChamgeEdge *self, GError **error);
};

CHAMGE_API_EXPORT
ChamgeEdge*    chamge_edge_new                          (const gchar   *uid);

CHAMGE_API_EXPORT
ChamgeEdge*    chamge_edge_new_full                     (const gchar   *uid,
                                                         ChamgeBackend  backend);

CHAMGE_API_EXPORT
gchar*          chamge_edge_request_target_uri          (ChamgeEdge    *self,
                                                         GError       **error);  

G_END_DECLS

#endif // __CHAMGE_EDGE_H__
