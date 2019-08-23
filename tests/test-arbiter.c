/**
 * tests/test-arbiter
 *
 * Copyright 2019 SK Telecom, Co., Ltd.
 *   Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 *
 */

#include <chamge/chamge.h>

#include <glib.h>

#define DEFAULT_EDGE_UID        "abc-987-123"
#define DEFAULT_BACKEND         CHAMGE_BACKEND_MOCK

static void
test_arbiter_instance (void)
{
  ChamgeReturn ret;
  gchar *uid = NULL;
  g_autoptr (ChamgeArbiter) arbiter = NULL;

  arbiter = chamge_arbiter_new_full (DEFAULT_EDGE_UID, DEFAULT_BACKEND);

  g_object_get (arbiter, "uid", &uid, NULL);
  g_assert_cmpstr (uid, ==, DEFAULT_EDGE_UID);

  ret = chamge_node_enroll (CHAMGE_NODE (arbiter), FALSE);
  g_assert (ret == CHAMGE_RETURN_OK);

  ret = chamge_node_delist (CHAMGE_NODE (arbiter));
  g_assert (ret == CHAMGE_RETURN_OK);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/chamge/arbiter-instance", test_arbiter_instance);
  return g_test_run ();
}
