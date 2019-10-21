/**
 *  Copyright 2019 SK Telecom Co., Ltd.
 *    Author: Jakub Adam <jakub.adam@collabora.com>
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

#ifndef __CHAMGE_AMQP_SOURCE_H__
#define __CHAMGE_AMQP_SOURCE_H__

#include <amqp.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef gboolean        (*ChamgeAmqpFunc)               (amqp_connection_state_t state,
                                                         amqp_rpc_reply_t      *reply,
                                                         amqp_envelope_t       *envelope,
                                                         gpointer               user_data);

guint                   chamge_amqp_add_watch           (amqp_connection_state_t state,
                                                         ChamgeAmqpFunc         callback,
                                                         gpointer               data);

G_END_DECLS

#endif // __CHAMGE_AMQP_SOURCE_H__

