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

static void
state_changed_cb (ChamgeEdge *edge, ChamgeNodeState state)
{
  
}

static void
test_edge_instance (void)
{
  ChamgeReturn ret; 
  gchar *uid = NULL;
  ChamgeEdge *edge = chamge_edge_new (DEFAULT_EDGE_UID);

  g_object_get (edge, "uid", &uid , NULL);
  g_assert_cmpstr (uid, ==, DEFAULT_EDGE_UID);

  g_signal_connect (edge, "state-changed", G_CALLBACK (state_changed_cb), NULL);

  ret = chamge_node_enroll (CHAMGE_NODE (edge), FALSE);
  g_assert (ret == CHAMGE_RETURN_OK);

  ret = chamge_node_delist (CHAMGE_NODE (edge));
  g_assert (ret == CHAMGE_RETURN_OK);

  g_object_unref (edge);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/chamge/edge-instance", test_edge_instance);

  return g_test_run();
}
