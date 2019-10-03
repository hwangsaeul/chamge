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

#include "config.h"

#include "mock-hub-backend.h"

struct _ChamgeMockHubBackend
{
  ChamgeHubBackend parent;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeMockHubBackend, chamge_mock_hub_backend, CHAMGE_TYPE_HUB_BACKEND)
/* *INDENT-ON* */

static ChamgeReturn
chamge_mock_hub_backend_enroll (ChamgeHubBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_hub_backend_delist (ChamgeHubBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_hub_backend_activate (ChamgeHubBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_hub_backend_deactivate (ChamgeHubBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static void
chamge_mock_hub_backend_class_init (ChamgeMockHubBackendClass * klass)
{
  ChamgeHubBackendClass *backend_class = CHAMGE_HUB_BACKEND_CLASS (klass);

  backend_class->enroll = chamge_mock_hub_backend_enroll;
  backend_class->delist = chamge_mock_hub_backend_delist;
  backend_class->activate = chamge_mock_hub_backend_activate;
  backend_class->deactivate = chamge_mock_hub_backend_deactivate;

}

static void
chamge_mock_hub_backend_init (ChamgeMockHubBackend * self)
{
}
