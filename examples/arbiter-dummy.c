/**
 *  tests/test-arbiter
 *
 *  Copyright 2019 SK Telecom Co., Ltd.
 *    Author: Heekyoung Seo <hkseo@sk.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0.
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
#include <termios.h>
#include <unistd.h>
#include <execinfo.h>

#include <glib.h>

#define DEFAULT_ARBITER_UID     "arbiter-987-123"
#define DEFAULT_BACKEND         CHAMGE_BACKEND_AMQP

typedef struct _ArbiterContext
{
  ChamgeArbiter *arbiter;
  GMainLoop *loop;
  GThread *user_input_thread;

  GList *edges;
  GList *hubs;

  struct termios term_properties;
} ArbiterContext;

static struct termios orig_term_properties;
typedef void (*sig_handler_t) (int);
static sig_handler_t signal_register[32];

static void
user_input_exit ()
{
  printf ("[DUMMY] restore tty setting at exit\n");
  tcsetattr (STDIN_FILENO, TCSANOW, &orig_term_properties);
}

static void
signal_handler (int sig)
{
  void *array[10];
  size_t size;

  printf ("signal %d. org %p. restore tty\n", sig, signal_register[sig]);
  user_input_exit ();

  // get void*'s for all entries on the stack
  size = backtrace (array, 10);

  // print out all the frames to stderr
  fprintf (stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd (array, size, STDERR_FILENO);

  exit (128 + sig);
}

static int
user_input_init ()
{
  struct termios term_properties;

  if (!isatty (STDIN_FILENO))
    return 0;

  tcgetattr (STDIN_FILENO, &term_properties);
  orig_term_properties = term_properties;

  atexit (user_input_exit);
  signal_register[SIGHUP] = signal (SIGHUP, signal_handler);
  signal_register[SIGINT] = signal (SIGINT, signal_handler);
  signal_register[SIGABRT] = signal (SIGABRT, signal_handler);
  signal_register[SIGFPE] = signal (SIGFPE, signal_handler);
  signal_register[SIGKILL] = signal (SIGKILL, signal_handler);
  signal_register[SIGSEGV] = signal (SIGSEGV, signal_handler);

  term_properties.c_lflag &= ~(ICANON | ECHO | ECHOE);
  term_properties.c_cc[VMIN] = 1;
  tcsetattr (STDIN_FILENO, TCSANOW, &term_properties);

  return 0;
}

static int
user_input_get ()
{
  int ret;
  char buf[1];

  if (!isatty (STDIN_FILENO)) {
    sleep (5);
    return 0;
  }

  ret = read (STDIN_FILENO, buf, 1);
  if (ret != 1)
    return 0;
  return buf[0];
}

static gpointer
do_user_command (gpointer data)
{
  int user_input = 0;
  gboolean quit = FALSE;
  ChamgeReturn ret = CHAMGE_RETURN_OK;
  ArbiterContext *context = (ArbiterContext *) data;

  user_input_init (context);

  while (!quit) {
    printf
        ("[DUMMY] ==> wait for key input to send pre defined user command\n");
    user_input = user_input_get ();
    switch (user_input) {
      case 's':                /* send start streaming */
      {
        g_autofree gchar *cmd = NULL;
        g_autofree gchar *response = NULL;
        GError *error = NULL;
        gchar *edge_id = NULL;
        ChamgeReturn ret = CHAMGE_RETURN_OK;

        edge_id = (gchar *) g_list_nth_data (context->edges, 0);
        if (edge_id == NULL) {
          printf ("[DUMMY] no edge is enrolled\n");
          break;
        }

        cmd =
            g_strdup_printf
            ("{\"to\":\"%s\",\"method\":\"streamingStart\"}", edge_id);
        printf ("[DUMMY] ==> user command : %s\n", cmd);
        ret =
            chamge_node_user_command (CHAMGE_NODE (context->arbiter), cmd,
            &response, &error);
        if (ret != CHAMGE_RETURN_OK) {
          printf ("[DUMMY] failed to send user command >> %s\n",
              error->message);
        }
      }
        break;
      case 't':
      {
        g_autofree gchar *cmd = NULL;
        g_autofree gchar *response = NULL;
        GError *error = NULL;
        gchar *edge_id = NULL;
        ChamgeReturn ret = CHAMGE_RETURN_OK;

        edge_id = (gchar *) g_list_nth_data (context->edges, 0);
        if (edge_id == NULL) {
          printf ("[DUMMY] no edge is enrolled\n");
          break;
        }

        cmd =
            g_strdup_printf
            ("{\"to\":\"%s\",\"method\":\"streamingStop\"}", edge_id);
        printf ("[DUMMY] ==> user command : %s\n", cmd);
        ret =
            chamge_node_user_command (CHAMGE_NODE (context->arbiter), cmd,
            &response, &error);
        if (ret != CHAMGE_RETURN_OK) {
          printf ("[DUMMY] failed to send user command >> %s\n",
              error->message);
        }
      }
        break;
      case 'r':                /* send start streaming */
      {
        g_autofree gchar *cmd = NULL;
        g_autofree gchar *response = NULL;
        GError *error = NULL;
        gchar *hub_id = NULL;
        ChamgeReturn ret = CHAMGE_RETURN_OK;

        hub_id = (gchar *) g_list_nth_data (context->hubs, 0);
        if (hub_id == NULL) {
          printf ("[DUMMY] no hub is enrolled\n");
          break;
        }
        cmd =
            g_strdup_printf
            ("{\"to\":\"%s\",\"method\":\"recordingStart\"}", hub_id);
        printf ("[DUMMY] ==> user command : %s\n", cmd);
        ret =
            chamge_node_user_command (CHAMGE_NODE (context->arbiter), cmd,
            &response, &error);
        if (ret != CHAMGE_RETURN_OK) {
          printf ("[DUMMY] failed to send user command >> %s\n",
              error->message);
        }
      }
        break;
      case 'x':
      {
        g_autofree gchar *cmd = NULL;
        g_autofree gchar *response = NULL;
        GError *error = NULL;
        gchar *hub_id = NULL;
        ChamgeReturn ret = CHAMGE_RETURN_OK;

        hub_id = (gchar *) g_list_nth_data (context->hubs, 0);
        if (hub_id == NULL) {
          printf ("[DUMMY] no hub is enrolled\n");
          break;
        }

        cmd =
            g_strdup_printf
            ("{\"to\":\"%s\",\"method\":\"recordingStop\"}", hub_id);
        printf ("[DUMMY] ==> user command : %s\n", cmd);
        ret =
            chamge_node_user_command (CHAMGE_NODE (context->arbiter), cmd,
            &response, &error);
        if (ret != CHAMGE_RETURN_OK) {
          printf ("[DUMMY] failed to send user command >> %s\n",
              error->message);
        }
      }
        break;
      case 'h':
        printf ("[DUMMY] s : send streaming start to first edge in the list\n");
        printf ("[DUMMY] t : send streaming stop to first edge in the list\n");
        printf ("[DUMMY] r : send recording start to first hub in the list\n");
        printf ("[DUMMY] x : send recording stop to first hub in the list\n");
        printf ("[DUMMY] q : exit\n");
        break;
      case 'q':
        ret = chamge_node_deactivate (CHAMGE_NODE (context->arbiter));
        g_assert (ret == CHAMGE_RETURN_OK);
        quit = TRUE;
        break;
    }
  }
  g_main_loop_quit (context->loop);
  return NULL;
}

static void
context_setup (ArbiterContext * context)
{
  context->loop = g_main_loop_new (NULL, FALSE);
}

static void
context_teardown (ArbiterContext * context)
{
  g_thread_join (context->user_input_thread);
  g_main_loop_unref (context->loop);
}

void
edge_enrolled_cb (ChamgeArbiter * arbiter, const gchar * edge_id,
    ArbiterContext * context)
{
  printf ("[DUMMY] edge enroll callback >> edge_id: %s\n", edge_id);
  context->edges = g_list_insert (context->edges, g_strdup (edge_id), 0);
}

void
edge_delisted_cb (ChamgeArbiter * arbiter, const gchar * edge_id)
{
  printf ("[DUMMY] edge delisted callback >>  edge_id: %s\n", edge_id);
}

void
hub_enrolled_cb (ChamgeArbiter * arbiter, const gchar * hub_id,
    ArbiterContext * context)
{
  printf ("[DUMMY] hub enroll callback >> hub_id: %s\n", hub_id);
  context->hubs = g_list_insert (context->hubs, g_strdup (hub_id), 0);
}

void
hub_delisted_cb (ChamgeArbiter * arbiter, const gchar * hub_id)
{
  printf ("[DUMMY] hub delisted callback  >> hub_id: %s\n", hub_id);
}

int
main (int argc, char *argv[])
{
  ArbiterContext context = { 0 };
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;
  g_autofree gchar *uid =
      g_compute_checksum_for_string (G_CHECKSUM_SHA256, DEFAULT_ARBITER_UID,
      strlen (DEFAULT_ARBITER_UID));
  g_autofree gchar *check_uid = NULL;

  context.arbiter = chamge_arbiter_new_full (uid, DEFAULT_BACKEND);

  g_object_get (context.arbiter, "uid", &check_uid, NULL);
  g_assert_cmpstr (uid, ==, check_uid);

  context_setup (&context);
  g_signal_connect (context.arbiter, "edge-enrolled",
      G_CALLBACK (edge_enrolled_cb), &context);
  g_signal_connect (context.arbiter, "edge-delisted",
      G_CALLBACK (edge_delisted_cb), &context);
  g_signal_connect (context.arbiter, "hub-enrolled",
      G_CALLBACK (hub_enrolled_cb), &context);
  g_signal_connect (context.arbiter, "hub-delisted",
      G_CALLBACK (hub_delisted_cb), &context);

  ret = chamge_node_enroll (CHAMGE_NODE (context.arbiter), FALSE);
  g_assert (ret == CHAMGE_RETURN_OK);

  ret = chamge_node_activate (CHAMGE_NODE (context.arbiter));
  g_assert (ret == CHAMGE_RETURN_OK);

  context.user_input_thread =
      g_thread_new ("user-input", do_user_command, &context);
  g_main_loop_run (context.loop);

  ret = chamge_node_delist (CHAMGE_NODE (context.arbiter));
  g_assert (ret == CHAMGE_RETURN_OK);

  context_teardown (&context);
  return 1;

}
