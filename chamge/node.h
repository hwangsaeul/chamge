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

  /* callbacks */
  void (* state_changed)                (ChamgeNode *self, ChamgeNodeState state);

  /* virtual public methods */
  ChamgeReturn (* enroll)               (ChamgeNode *self);
  ChamgeReturn (* delist)               (ChamgeNode *self);

  ChamgeReturn (* activate)             (ChamgeNode *self);
  ChamgeReturn (* deactivate)           (ChamgeNode *self);

  gchar *      (* get_uid)              (ChamgeNode *self);
  ChamgeReturn (* user_command)         (ChamgeNode *self, const gchar* cmd, gchar **out, GError ** error);
};

CHAMGE_API_EXPORT
ChamgeReturn chamge_node_enroll         (ChamgeNode *self, gboolean lazy);

CHAMGE_API_EXPORT
ChamgeReturn chamge_node_delist         (ChamgeNode *self);

CHAMGE_API_EXPORT
ChamgeReturn chamge_node_activate       (ChamgeNode *self);

CHAMGE_API_EXPORT
ChamgeReturn chamge_node_deactivate     (ChamgeNode *self);

CHAMGE_API_EXPORT
ChamgeReturn chamge_node_get_uid        (ChamgeNode *self, gchar ** uid);

CHAMGE_API_EXPORT
ChamgeReturn chamge_node_user_command   (ChamgeNode *self, const gchar *cmd, gchar **out, GError ** error);

G_END_DECLS

#endif // __CHAMGE_NODE_H__
