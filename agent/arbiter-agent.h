/**
 *  Copyright 2019 SK Telecom Co., Ltd.
 *    Author: Heekyoung Seo <hkseo@sk.com>
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

#ifndef __CHAMGE_ARBITER_AGENT_H__
#define __CHAMGE_ARBITER_AGENT_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_ARBITER_AGENT  (chamge_arbiter_agent_get_type ())
G_DECLARE_FINAL_TYPE            (ChamgeArbiterAgent, chamge_arbiter_agent, CHAMGE, ARBITER_AGENT, GApplication)

G_END_DECLS

#endif // __CHAMGE_ARBITER_AGENT_H__
