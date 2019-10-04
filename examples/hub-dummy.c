/**
 * examples/hub-dummy 
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

#include <stdio.h>

#include <glib.h>

#define DEFAULT_HUB_UID        "hub-987-123"
#define DEFAULT_BACKEND         CHAMGE_BACKEND_AMQP

typedef struct _HubContext
{
  ChamgeHub *hub;
  GMainLoop *loop;
} HubContext;

static void
context_setup (HubContext * context)
{
  context->loop = g_main_loop_new (NULL, FALSE);
}

static void
context_teardown (HubContext * context)
{
  g_main_loop_unref (context->loop);
}

static void
state_changed_quit_cb (ChamgeHub * hub, ChamgeNodeState state,
    HubContext * context)
{
  g_main_loop_quit (context->loop);
}

void
user_command_cb (ChamgeHub * hub, const gchar * user_command,
    gchar ** response, GError ** error)
{
  printf ("[DUMMY] ==> user command callback >> %s\n", user_command);
  *response = g_strdup ("{\"result\":\"ok\"}");
}

int
main (int argc, char *argv[])
{
  HubContext context = { 0 };
  ChamgeNodeState state;
  ChamgeReturn ret;

  g_autofree gchar *uid =
      g_compute_checksum_for_string (G_CHECKSUM_SHA256, DEFAULT_HUB_UID,
      strlen (DEFAULT_HUB_UID));
  g_autofree gchar *check_uid = NULL;

  context.hub = chamge_hub_new_full (uid, DEFAULT_BACKEND);

  g_object_get (context.hub, "uid", &check_uid, NULL);
  g_assert_cmpstr (uid, ==, check_uid);

  context_setup (&context);
  g_signal_connect (context.hub, "state-changed",
      G_CALLBACK (state_changed_quit_cb), &context);
  g_signal_connect (context.hub, "user-command", G_CALLBACK (user_command_cb),
      &context);

  g_object_get (context.hub, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_NULL);

  ret = chamge_node_enroll (CHAMGE_NODE (context.hub), FALSE);
  g_assert (ret == CHAMGE_RETURN_OK);

  g_object_get (context.hub, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_ENROLLED);

  ret = chamge_node_activate (CHAMGE_NODE (context.hub));
  g_assert (ret == CHAMGE_RETURN_OK);

  g_main_loop_run (context.loop);

  ret = chamge_node_delist (CHAMGE_NODE (context.hub));
  g_assert (ret == CHAMGE_RETURN_OK);

  g_object_get (context.hub, "state", &state, NULL);
  g_assert (state == CHAMGE_NODE_STATE_NULL);

  context_teardown (&context);

  return 1;
}
