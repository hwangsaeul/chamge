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

#ifndef __CHAMGE_NODE_H__
#define __CHAMGE_NODE_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#include <glib-object.h>
#include <chamge/types.h>

/**
 * SECTION: node
 * @Title: ChamgeNode
 * @Short_description: Base object to buid different types of Chamge brokers
 *
 * A #ChamgeNode is a base object which implements the basic functionality present in every Chamge broker.
 */

G_BEGIN_DECLS

#define CHAMGE_TYPE_NODE       (chamge_node_get_type ())
CHAMGE_API_EXPORT
G_DECLARE_DERIVABLE_TYPE (ChamgeNode, chamge_node, CHAMGE, NODE, GObject)

typedef enum {
  CHAMGE_NODE_STATE_NULL,
  CHAMGE_NODE_STATE_ENROLLED,
  CHAMGE_NODE_STATE_ACTIVATED,
} ChamgeNodeState;

struct _ChamgeNodeClass
{
  GObjectClass parent_class;

  /* virtual public methods */
  ChamgeReturn (* enroll)               (ChamgeNode *self);
  ChamgeReturn (* delist)               (ChamgeNode *self);

  ChamgeReturn (* activate)             (ChamgeNode *self);
  ChamgeReturn (* deactivate)           (ChamgeNode *self);

  gchar *      (* get_uid)              (ChamgeNode *self);
  ChamgeReturn (* user_command)         (ChamgeNode *self, const gchar* cmd, gchar **out, GError ** error);

  /* signals */
  /**
   * ChamgeNodeClass::state_changed:
   * @self: a #ChamgeNode object
   *
   * Signal to inform that the state of the Chamge device has changed.
   */
  void (* state_changed)                (ChamgeNode *self, ChamgeNodeState state);

};

/**
 * chamge_node_enroll:
 * @self: a #ChamgeNode object
 * @lazy:
 *
 * Enrolls the node in the message broker
 *
 * Returns: a #ChamgeReturn object
 */
CHAMGE_API_EXPORT
ChamgeReturn chamge_node_enroll         (ChamgeNode *self, gboolean lazy);

/**
 * chamge_node_delist:
 * @self: a #ChamgeNode object
 *
 * Delists the node from the message broker
 *
 * Returns: a #ChamgeReturn object
 */
CHAMGE_API_EXPORT
ChamgeReturn chamge_node_delist         (ChamgeNode *self);

/**
 * chamge_node_activate:
 * @self:  a #ChamgeNode object
 *
 * Activates the node
 *
 * Returns: a #ChamgeReturn object
 */
CHAMGE_API_EXPORT
ChamgeReturn chamge_node_activate       (ChamgeNode *self);

/**
 * chamge_node_deactivate:
 * @self: a #ChamgeNode object
 *
 * Deactivates the node
 *
 * Returns: a #ChamgeReturn object
 */
CHAMGE_API_EXPORT
ChamgeReturn chamge_node_deactivate     (ChamgeNode *self);

/**
 * chamge_node_get_uid:
 * @self: a #ChamgeNode object
 * @uid: unique #ChamgeNode identifier
 *
 * Gets the unique identifier for the #ChamgeNode.
 *
 * Returns: a #ChamgeReturn object
 */
CHAMGE_API_EXPORT
ChamgeReturn chamge_node_get_uid        (ChamgeNode *self, gchar ** uid);

/**
 * chamge_node_user_command:
 * @self: a #ChamgeNode object
 * @cmd: user command to send
 * @out: response to user command
 * @error: a #GError
 *
 * Sends a command using the message broker.
 *
 * Returns: a #ChamgeReturn object
 */
CHAMGE_API_EXPORT
ChamgeReturn chamge_node_user_command   (ChamgeNode *self, const gchar *cmd, gchar **out, GError ** error);

G_END_DECLS

#endif // __CHAMGE_NODE_H__
