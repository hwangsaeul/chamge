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

#ifndef __CHAMGE_EDGE_H__
#define __CHAMGE_EDGE_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#include <chamge/node.h>

/**
 * SECTION: edge
 * @Title: ChamgeEdge
 * @Short_description: An object to handle Edge devices
 *
 * A #ChamgeEdge is an object which implements the specific functionality present in every Edge device.
 */

G_BEGIN_DECLS

#define CHAMGE_TYPE_EDGE       (chamge_edge_get_type ())
CHAMGE_API_EXPORT
G_DECLARE_DERIVABLE_TYPE (ChamgeEdge, chamge_edge, CHAMGE, EDGE, ChamgeNode)

struct _ChamgeEdgeClass
{
  ChamgeNodeClass parent_class;
  
  /* methods */
  gchar* (* request_target_uri)         (ChamgeEdge *self, GError **error);


  /* signals */
	/**
   * ChamgeEdgeClass::user_command:  
   * @self: a #ChamgeEdge object
   * @cmd: command received
   * @response: command response
   * @error: a #GError object
   *
   * Signal to inform that the state of the Chamge device has changed.
   */
  void (*user_command)                  (ChamgeEdge    *self,
                                         const gchar   *cmd,
                                         gchar        **response,
                                         GError       **error);
};

/**
 * chamge_edge_new:
 * @uid: unique identifier for the #ChamgeNode
 *
 * Creates a new #ChamgeEdge object
 *
 * Returns: the newly created object
 */
CHAMGE_API_EXPORT
ChamgeEdge*    chamge_edge_new                          (const gchar   *uid);

/**
 * chamge_edge_new_full:
 * @uid: unique identifier for the #ChamgeNode
 * @backend: the backend support for message broker
 *
 * Creates a new #ChamgeEdge object with specific parameters
 *
 * Returns: the newly created object
 */
CHAMGE_API_EXPORT
ChamgeEdge*    chamge_edge_new_full                     (const gchar   *uid,
                                                         ChamgeBackend  backend);

/**
 * chamge_edge_request_target_uri:
 * @self: a #ChamgeEdge object
 * @error: a #GError object
 *
 * Request the target URI
 *
 * Returns: the requested target URI
 */
CHAMGE_API_EXPORT
gchar*          chamge_edge_request_target_uri          (ChamgeEdge    *self,
                                                         GError       **error);  

G_END_DECLS

#endif // __CHAMGE_EDGE_H__
