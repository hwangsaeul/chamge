/**
 * tests/test-edge
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
#define DEFAULT_EDGE_UID        "abc-987-123"
#define DEFAULT_BACKEND         CHAMGE_BACKEND_MOCK

typedef struct _TestFixture
{
  ChamgeEdge *edge;
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
test_edge_instance (void)
{
  ChamgeReturn ret;
  gchar *uid = NULL;
  g_autoptr (ChamgeEdge) edge = NULL;

  edge = chamge_edge_new_full (DEFAULT_EDGE_UID, DEFAULT_BACKEND);

  g_object_get (edge, "uid", &uid, NULL);
  g_assert_cmpstr (uid, ==, DEFAULT_EDGE_UID);

  ret = chamge_node_enroll (CHAMGE_NODE (edge), FALSE);
  g_assert (ret == CHAMGE_RETURN_OK);

  ret = chamge_node_delist (CHAMGE_NODE (edge));
  g_assert (ret == CHAMGE_RETURN_OK);
}

static void
state_changed_quit_cb (ChamgeEdge * edge, ChamgeNodeState state,
    TestFixture * fixture)
{
  g_main_loop_quit (fixture->loop);
}

static void
test_edge_instance_lazy (TestFixture * fixture, gconstpointer unused)
{
  ChamgeReturn ret;
  g_autoptr (ChamgeEdge) edge = NULL;
  ChamgeNodeState state;

  edge = chamge_edge_new_full (DEFAULT_EDGE_UID, DEFAULT_BACKEND);

  g_signal_connect (edge, "state-changed", G_CALLBACK (state_changed_quit_cb),
      fixture);

  g_object_get (edge, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_NULL);

  ret = chamge_node_enroll (CHAMGE_NODE (edge), TRUE);
  g_assert (ret == CHAMGE_RETURN_ASYNC);

  g_object_get (edge, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_NULL);

  g_main_loop_run (fixture->loop);

  g_object_get (edge, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_ENROLLED);

  ret = chamge_node_delist (CHAMGE_NODE (edge));
  g_assert (ret == CHAMGE_RETURN_OK);

  g_object_get (edge, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_NULL);
}

static void
test_edge_request_target_uri (TestFixture * fixture, gconstpointer unused)
{
  ChamgeReturn ret;
  g_autoptr (ChamgeEdge) edge = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree gchar *target_uri = NULL;
  ChamgeNodeState state;
  edge = chamge_edge_new_full (DEFAULT_EDGE_UID, DEFAULT_BACKEND);

  target_uri = chamge_edge_request_target_uri (edge, &error);
  g_assert_null (target_uri);

  ret = chamge_node_enroll (CHAMGE_NODE (edge), FALSE);
  g_assert (ret == CHAMGE_RETURN_OK);

  g_object_get (edge, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_ENROLLED);

  ret = chamge_node_activate (CHAMGE_NODE (edge));
  g_assert (ret == CHAMGE_RETURN_OK);

  target_uri = chamge_edge_request_target_uri (edge, &error);
  g_assert_nonnull (target_uri);

  ret = chamge_node_deactivate (CHAMGE_NODE (edge));
  g_assert (ret == CHAMGE_RETURN_OK);

  target_uri = chamge_edge_request_target_uri (edge, &error);
  g_assert_null (target_uri);

  ret = chamge_node_delist (CHAMGE_NODE (edge));
  g_assert (ret == CHAMGE_RETURN_OK);

  target_uri = chamge_edge_request_target_uri (edge, &error);
  g_assert_null (target_uri);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/chamge/edge-instance", test_edge_instance);
  g_test_add ("/chamge/edge-instance-lazy", TestFixture, NULL,
      fixture_setup, test_edge_instance_lazy, fixture_teardown);
  g_test_add ("/chamge/edge-request-target-uri", TestFixture, NULL,
      fixture_setup, test_edge_request_target_uri, fixture_teardown);
  return g_test_run ();
}
