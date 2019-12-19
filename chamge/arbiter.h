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

#ifndef __CHAMGE_ARBITER_H__
#define __CHAMGE_ARBITER_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#include <chamge/node.h>

/**
 * SECTION: arbiter
 * @Title: ChamgeArbiter
 * @Short_description: An object to handle Arbiter devices
 *
 * A #ChamgeArbiter is an object which implements the specific functionality present in every Arbiter device.
 */

G_BEGIN_DECLS

#define CHAMGE_TYPE_ARBITER     (chamge_arbiter_get_type ())
CHAMGE_API_EXPORT
G_DECLARE_DERIVABLE_TYPE        (ChamgeArbiter, chamge_arbiter, CHAMGE, ARBITER, ChamgeNode)

struct _ChamgeArbiterClass
{
  ChamgeNodeClass parent_class;
  
  /* methods */
  void  (* approve)                     (ChamgeArbiter *self,
                                         const gchar   *uid);

 /* signals */
  void  (* edge_enrolled)               (ChamgeArbiter *self,
                                         const gchar   *uid);
  void  (* edge_delisted)               (ChamgeArbiter *self,
                                         const gchar   *uid);
  void  (* hub_enrolled)                (ChamgeArbiter *self,
                                         const gchar   *uid);
  void  (* hub_delisted)                (ChamgeArbiter *self,
                                         const gchar   *uid);
};

/**
 * chamge_arbiter_new:
 * @uid: unique identifier for the #ChamgeNode
 *
 * Creates a new #ChamgeArbiter object
 *
 * Returns: the newly created object
 */
 CHAMGE_API_EXPORT
ChamgeArbiter*  chamge_arbiter_new                      (const gchar   *uid);  

/**
 * chamge_arbiter_new_full:
 * @uid: unique identifier for the #ChamgeNode
 * @backend: the backend support for message broker
 *
 * Creates a new #ChamgeArbiter object with specific parameters
 *
 * Returns: the newly created object
 */
CHAMGE_API_EXPORT
ChamgeArbiter*  chamge_arbiter_new_full                 (const gchar   *uid,
                                                         ChamgeBackend  bakend);

G_END_DECLS

#endif //__CHAMGE_ARBITER_H__

