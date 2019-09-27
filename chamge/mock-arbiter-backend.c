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

#include "mock-arbiter-backend.h"

struct _ChamgeMockArbiterBackend
{
  ChamgeArbiterBackend parent;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeMockArbiterBackend, chamge_mock_arbiter_backend, CHAMGE_TYPE_ARBITER_BACKEND)
/* *INDENT-ON* */

static ChamgeReturn
chamge_mock_arbiter_backend_enroll (ChamgeArbiterBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_arbiter_backend_delist (ChamgeArbiterBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_arbiter_backend_activate (ChamgeArbiterBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
chamge_mock_arbiter_backend_deactivate (ChamgeArbiterBackend * self)
{
  return CHAMGE_RETURN_OK;
}

static void
chamge_mock_arbiter_backend_class_init (ChamgeMockArbiterBackendClass * klass)
{
  ChamgeArbiterBackendClass *backend_class =
      CHAMGE_ARBITER_BACKEND_CLASS (klass);

  backend_class->enroll = chamge_mock_arbiter_backend_enroll;
  backend_class->delist = chamge_mock_arbiter_backend_delist;
  backend_class->activate = chamge_mock_arbiter_backend_activate;
  backend_class->deactivate = chamge_mock_arbiter_backend_deactivate;
}

static void
chamge_mock_arbiter_backend_init (ChamgeMockArbiterBackend * self)
{
}
