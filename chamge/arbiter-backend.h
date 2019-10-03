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
  void          (* hub_enrolled)                (ChamgeArbiterBackend  *self,
                                                 const gchar           *edge_id);
  void          (* hub_delisted)                (ChamgeArbiterBackend  *self,
                                                 const gchar           *edge_id);
};

typedef void (*ChamgeArbiterBackendEnrollCallback)      (const gchar           *edge_id,
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
                                                 ChamgeArbiterBackendEnrollCallback     edge_enrolled,
                                                 ChamgeArbiterBackendEnrollCallback     edge_delisted,
                                                 ChamgeArbiterBackendConnectionRequested
                                                                                        edge_connection_requested);
 void    chamge_arbiter_backend_set_hub_handler (ChamgeArbiterBackend  *self,
                                                 ChamgeArbiterBackendEnrollCallback     hub_enrolled,
                                                 ChamgeArbiterBackendEnrollCallback     hub_delisted);
                                
G_END_DECLS

#endif // __CHAMGE_ARBITER_BACKEND_H__

