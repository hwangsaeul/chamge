/**
 * tests/test-edge
 *
 * Copyright 2019 SK Telecom, Co., Ltd.
 *   Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 *
 */

#include <chamge/chamge.h>

#include <glib.h>

/*
 * md5 ("abc-987-123") == defaa0a0d935c7b52c459159788a8b7c
 * time group: c
 */
#define DEFAULT_EDGE_UID "abc-987-123"

typedef struct _TestFixture
{
  ChamgeEdge *edge;
  GMainLoop *loop;
} TestFixture;

static void
fixture_setup (TestFixture * fixture, gconstpointer unused)
{
  fixture->loop = g_main_loop_new (NULL, FALSE);
}

static void
fixture_teardown (TestFixture * fixture, gconstpointer unused)
{
  g_main_loop_quit (fixture->loop);
}

static void
test_edge_instance (void)
{
  ChamgeReturn ret;
  gchar *uid = NULL;
  g_autoptr (ChamgeEdge) edge = NULL;

  edge = chamge_edge_new (DEFAULT_EDGE_UID);

  g_object_get (edge, "uid", &uid, NULL);
  g_assert_cmpstr (uid, ==, DEFAULT_EDGE_UID);

  ret = chamge_node_enroll (CHAMGE_NODE (edge), FALSE);
  g_assert (ret == CHAMGE_RETURN_OK);

  ret = chamge_node_delist (CHAMGE_NODE (edge));
  g_assert (ret == CHAMGE_RETURN_OK);

}

static void
state_changed_cb (ChamgeEdge * edge, ChamgeNodeState state,
    TestFixture * fixture)
{
  g_main_loop_quit (fixture->loop);
}

static void
test_edge_instance_lazy (TestFixture * fixture, gconstpointer unused)
{
  ChamgeReturn ret;
  g_autoptr (ChamgeEdge) edge = NULL;

  edge = chamge_edge_new (DEFAULT_EDGE_UID);

  g_signal_connect (edge, "state-changed", G_CALLBACK (state_changed_cb),
      fixture);

  ret = chamge_node_enroll (CHAMGE_NODE (edge), TRUE);
  g_assert (ret == CHAMGE_RETURN_ASYNC);

  g_main_loop_run (fixture->loop);

  ret = chamge_node_delist (CHAMGE_NODE (edge));
  g_assert (ret == CHAMGE_RETURN_OK);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/chamge/edge-instance", test_edge_instance);
  g_test_add ("/chamge/edge-instance-lazy", TestFixture, NULL,
      fixture_setup, test_edge_instance_lazy, fixture_teardown);

  return g_test_run ();
}
