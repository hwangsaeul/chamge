/**
 *  Copyright 2019 SK Telecom Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
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

#ifndef __CHAMGE_MOCK_HUB_BACKEND_H__
#define __CHAMGE_MOCK_HUB_BACKEND_H__

#include <chamge/hub-backend.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_MOCK_HUB_BACKEND       (chamge_mock_hub_backend_get_type ())
G_DECLARE_FINAL_TYPE (ChamgeMockHubBackend, chamge_mock_hub_backend, CHAMGE, MOCK_HUB_BACKEND, ChamgeHubBackend)

G_END_DECLS

#endif // __CHAMGE_MOCK_HUB_BACKEND_H__
