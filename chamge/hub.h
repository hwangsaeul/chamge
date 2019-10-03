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

#ifndef __CHAMGE_HUB_H__
#define __CHAMGE_HUB_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#include <chamge/node.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_HUB       (chamge_hub_get_type ())
CHAMGE_API_EXPORT
G_DECLARE_DERIVABLE_TYPE (ChamgeHub, chamge_hub, CHAMGE, HUB, ChamgeNode)

struct _ChamgeHubClass
{
  ChamgeNodeClass parent_class;
  
  /* methods */
  gchar* (* get_uid)                    (ChamgeHub *self);

  /* signals */
  void (*user_command)                  (ChamgeHub    *self,
                                        const gchar   *cmd,
                                        gchar        **response,
                                        GError       **error);
};

CHAMGE_API_EXPORT
ChamgeHub*    chamge_hub_new                          (const gchar   *uid);

CHAMGE_API_EXPORT
ChamgeHub*    chamge_hub_new_full                     (const gchar   *uid,
                                                       ChamgeBackend  backend);

CHAMGE_API_EXPORT
ChamgeReturn  chamge_hub_get_uid                     (ChamgeHub *self, gchar ** uid);

G_END_DECLS

#endif // __CHAMGE_HUB_H__
