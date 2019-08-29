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

typedef struct _TestFixture
{
  ChamgeArbiter *arbiter;
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
  g_main_loop_unref (fixture->loop);
}

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

static void
test_arbiter_activate (TestFixture * fixture, gconstpointer unused)
{
  ChamgeReturn ret;
  gchar *uid = NULL;
  g_autoptr (ChamgeArbiter) arbiter = NULL;

  arbiter = chamge_arbiter_new_full (DEFAULT_EDGE_UID, DEFAULT_BACKEND);

  g_object_get (arbiter, "uid", &uid, NULL);
  g_assert_cmpstr (uid, ==, DEFAULT_EDGE_UID);

  ret = chamge_node_enroll (CHAMGE_NODE (arbiter), FALSE);
  g_assert (ret == CHAMGE_RETURN_OK);

  ret = chamge_node_activate (CHAMGE_NODE (arbiter));
  g_assert (ret == CHAMGE_RETURN_OK);

  g_main_loop_run (fixture->loop);

  ret = chamge_node_deactivate (CHAMGE_NODE (arbiter));
  g_assert (ret == CHAMGE_RETURN_OK);

  ret = chamge_node_delist (CHAMGE_NODE (arbiter));
  g_assert (ret == CHAMGE_RETURN_OK);

}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/chamge/arbiter-instance", test_arbiter_instance);
  g_test_add ("/chamge/arbiter-activate", TestFixture, NULL,
      fixture_setup, test_arbiter_activate, fixture_teardown);
  return g_test_run ();
}
