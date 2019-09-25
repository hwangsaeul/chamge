/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_EDGE_BACKEND_H__
#define __CHAMGE_EDGE_BACKEND_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#include <glib-object.h>
#include <chamge/types.h>
#include <chamge/edge.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_EDGE_BACKEND       (chamge_edge_backend_get_type ())
G_DECLARE_DERIVABLE_TYPE (ChamgeEdgeBackend, chamge_edge_backend, CHAMGE, EDGE_BACKEND, GObject)

struct _ChamgeEdgeBackendClass
{
  GObjectClass  parent_class;

  ChamgeReturn  (* enroll)                      (ChamgeEdgeBackend     *self);
  ChamgeReturn  (* delist)                      (ChamgeEdgeBackend     *self);

  ChamgeReturn  (* activate)                    (ChamgeEdgeBackend     *self);
  ChamgeReturn  (* deactivate)                  (ChamgeEdgeBackend     *self);

  gchar*        (* request_target_uri)          (ChamgeEdgeBackend     *self,
                                                 GError               **error);

  ChamgeReturn  (* user_command)               (ChamgeEdgeBackend     *self,
                                                 const gchar           *cmd,
                                                 gchar                **response,
                                                 GError               **error);
};
typedef ChamgeReturn (*ChamgeEdgeBackendUserCommand)    (const gchar          *cmd,
                                                         gchar               **response,
                                                         GError              **error,
                                                         ChamgeEdgeBackend     *edge_backend);

ChamgeEdgeBackend      *chamge_edge_backend_new         (ChamgeEdge            *edge);

ChamgeReturn            chamge_edge_backend_enroll      (ChamgeEdgeBackend     *self);

ChamgeReturn            chamge_edge_backend_delist      (ChamgeEdgeBackend     *self);

ChamgeReturn            chamge_edge_backend_activate    (ChamgeEdgeBackend     *self);

ChamgeReturn            chamge_edge_backend_deactivate  (ChamgeEdgeBackend     *self);

gchar                  *chamge_edge_backend_request_target_uri
                                                        (ChamgeEdgeBackend     *self,
                                                         GError               **error);

void  chamge_edge_backend_set_user_command_handler      (ChamgeEdgeBackend     *self,
                                                         ChamgeEdgeBackendUserCommand
                                                                                user_command);

G_END_DECLS

#endif // __CHAMGE_EDGE_BACKEND_H__

