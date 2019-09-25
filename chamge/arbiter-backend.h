/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_ARBITER_BACKEND_H__
#define __CHAMGE_ARBITER_BACKEND_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#include <glib-object.h>
#include <chamge/types.h>
#include <chamge/arbiter.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_ARBITER_BACKEND     (chamge_arbiter_backend_get_type ())
G_DECLARE_DERIVABLE_TYPE                (ChamgeArbiterBackend, chamge_arbiter_backend,
                                         CHAMGE, ARBITER_BACKEND, GObject)

struct _ChamgeArbiterBackendClass
{
  GObjectClass  parent_class;

  ChamgeReturn  (* enroll)                      (ChamgeArbiterBackend  *self);
  ChamgeReturn  (* delist)                      (ChamgeArbiterBackend  *self);

  ChamgeReturn  (* activate)                    (ChamgeArbiterBackend  *self);
  ChamgeReturn  (* deactivate)                  (ChamgeArbiterBackend  *self);
  ChamgeReturn  (* user_command)                (ChamgeArbiterBackend  *self,
                                                 const gchar           *cmd,
                                                 gchar                **out,
                                                 GError               **error);

  void          (* approve)                     (ChamgeArbiterBackend  *self,
                                                 const gchar           *edge_id);

  void          (* edge_enrolled)               (ChamgeArbiterBackend  *self,
                                                 const gchar           *edge_id);
  void          (* edge_delisted)               (ChamgeArbiterBackend  *self,
                                                 const gchar           *edge_id);
};

typedef void (*ChamgeArbiterBackendEdgeEnrolled)        (const gchar           *edge_id, 
                                                         ChamgeArbiterBackend  *arbiter_backend);

typedef void (*ChamgeArbiterBackendEdgeDelisted)        (const gchar           *edge_id, 
                                                         ChamgeArbiterBackend  *arbiter_backend);

typedef void (*ChamgeArbiterBackendConnectionRequested) (const gchar           *edge_id, 
                                                         ChamgeArbiterBackend  *arbiter_backend);

ChamgeArbiterBackend   *chamge_arbiter_backend_new      (ChamgeArbiter         *arbiter);

ChamgeReturn    chamge_arbiter_backend_enroll   (ChamgeArbiterBackend  *self);

ChamgeReturn    chamge_arbiter_backend_delist   (ChamgeArbiterBackend  *self);

ChamgeReturn    chamge_arbiter_backend_activate (ChamgeArbiterBackend  *self);

ChamgeReturn    chamge_arbiter_backend_deactivate
                                                (ChamgeArbiterBackend  *self);

ChamgeReturn    chamge_arbiter_backend_user_command
                                                (ChamgeArbiterBackend  *self,
                                                 const gchar           *cmd,
                                                 gchar                **out,
                                                 GError               **error);

void    chamge_arbiter_backend_approve          (ChamgeArbiterBackend  *self,
                                                 const gchar           *edge_id);

void    chamge_arbiter_backend_set_edge_handler (ChamgeArbiterBackend  *self,
                                                 ChamgeArbiterBackendEdgeEnrolled       edge_enrolled,
                                                 ChamgeArbiterBackendEdgeDelisted       edge_delisted,
                                                 ChamgeArbiterBackendConnectionRequested
                                                                                        edge_connection_requested);
                                
G_END_DECLS

#endif // __CHAMGE_ARBITER_BACKEND_H__

