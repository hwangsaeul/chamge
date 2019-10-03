/**
 * tests/test-hub
 *
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

#include <chamge/chamge.h>

#include <glib.h>

/*
 * md5 ("abc-987-123") == defaa0a0d935c7b52c459159788a8b7c
 * time group: c
 */
#define DEFAULT_HUB_UID        "abc-987-123"
#define DEFAULT_BACKEND         CHAMGE_BACKEND_MOCK

typedef struct _TestFixture
{
  ChamgeHub *hub;
  ChamgeNodeState prev_state;
  GMainLoop *loop;
} TestFixture;

static void
fixture_setup (TestFixture * fixture, gconstpointer unused)
{
  fixture->prev_state = CHAMGE_NODE_STATE_NULL;
  fixture->loop = g_main_loop_new (NULL, FALSE);
}

static void
fixture_teardown (TestFixture * fixture, gconstpointer unused)
{
  g_main_loop_unref (fixture->loop);
}

static void
test_hub_instance (void)
{
  ChamgeReturn ret;
  gchar *uid = NULL;
  g_autoptr (ChamgeHub) hub = NULL;

  hub = chamge_hub_new_full (DEFAULT_HUB_UID, DEFAULT_BACKEND);

  g_object_get (hub, "uid", &uid, NULL);
  g_assert_cmpstr (uid, ==, DEFAULT_HUB_UID);

  ret = chamge_node_enroll (CHAMGE_NODE (hub), FALSE);
  g_assert (ret == CHAMGE_RETURN_OK);

  ret = chamge_node_delist (CHAMGE_NODE (hub));
  g_assert (ret == CHAMGE_RETURN_OK);
}

static void
state_changed_quit_cb (ChamgeHub * hub, ChamgeNodeState state,
    TestFixture * fixture)
{
  g_main_loop_quit (fixture->loop);
}

static void
test_hub_instance_lazy (TestFixture * fixture, gconstpointer unused)
{
  ChamgeReturn ret;
  g_autoptr (ChamgeHub) hub = NULL;
  ChamgeNodeState state;

  hub = chamge_hub_new_full (DEFAULT_HUB_UID, DEFAULT_BACKEND);

  g_signal_connect (hub, "state-changed", G_CALLBACK (state_changed_quit_cb),
      fixture);

  g_object_get (hub, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_NULL);

  ret = chamge_node_enroll (CHAMGE_NODE (hub), TRUE);
  g_assert (ret == CHAMGE_RETURN_ASYNC);

  g_object_get (hub, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_NULL);

  g_main_loop_run (fixture->loop);

  g_object_get (hub, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_ENROLLED);

  ret = chamge_node_delist (CHAMGE_NODE (hub));
  g_assert (ret == CHAMGE_RETURN_OK);

  g_object_get (hub, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_NULL);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/chamge/hub-instance", test_hub_instance);
  g_test_add ("/chamge/hub-instance-lazy", TestFixture, NULL,
      fixture_setup, test_hub_instance_lazy, fixture_teardown);
  return g_test_run ();
}
