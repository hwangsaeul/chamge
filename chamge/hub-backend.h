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

#ifndef __CHAMGE_HUB_BACKEND_H__
#define __CHAMGE_HUB_BACKEND_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#include <glib-object.h>
#include <chamge/types.h>
#include <chamge/hub.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_HUB_BACKEND       (chamge_hub_backend_get_type ())
G_DECLARE_DERIVABLE_TYPE (ChamgeHubBackend, chamge_hub_backend, CHAMGE, HUB_BACKEND, GObject)

struct _ChamgeHubBackendClass
{
  GObjectClass  parent_class;

  ChamgeReturn  (* enroll)                      (ChamgeHubBackend     *self);
  ChamgeReturn  (* delist)                      (ChamgeHubBackend     *self);

  ChamgeReturn  (* activate)                    (ChamgeHubBackend     *self);
  ChamgeReturn  (* deactivate)                  (ChamgeHubBackend     *self);

  ChamgeReturn  (* user_command)               (ChamgeHubBackend      *self,
                                                 const gchar          *cmd,
                                                 gchar               **response,
                                                 GError              **error);

  ChamgeReturn  (* user_command_send)          (ChamgeHubBackend      *self,
                                                 const gchar          *cmd,
                                                 gchar               **response,
                                                 GError              **error);
};
typedef ChamgeReturn (*ChamgeHubBackendUserCommand)    (const gchar          *cmd,
                                                        gchar               **response,
                                                        GError              **error,
                                                        ChamgeHubBackend     *hub_backend);

ChamgeHubBackend      *chamge_hub_backend_new          (ChamgeHub            *hub);

ChamgeReturn            chamge_hub_backend_enroll      (ChamgeHubBackend     *self);

ChamgeReturn            chamge_hub_backend_delist      (ChamgeHubBackend     *self);

ChamgeReturn            chamge_hub_backend_activate    (ChamgeHubBackend     *self);

ChamgeReturn            chamge_hub_backend_deactivate  (ChamgeHubBackend     *self);

ChamgeReturn            chamge_hub_backend_user_command_send (ChamgeHubBackend    *self,
                                                         const gchar         *cmd,
                                                         gchar              **out,
                                                         GError             **error);

void  chamge_hub_backend_set_user_command_handler      (ChamgeHubBackend     *self,
                                                        ChamgeHubBackendUserCommand
                                                                              user_command);

G_END_DECLS

#endif // __CHAMGE_HUB_BACKEND_H__

