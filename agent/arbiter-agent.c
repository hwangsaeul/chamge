/**
 *  Copyright 2019 SK Telecom Co., Ltd.
 *    Author: Heekyoung Seo <hkseo@sk.com>
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

#include "arbiter-agent.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <json-glib/json-glib.h>

#include <glib.h>

#include <chamge/enumtypes.h>
#include <chamge/arbiter.h>
#include <chamge/dbus/arbiter-manager-generated.h>

struct _ChamgeArbiterAgent
{
  GApplication parent;
  ChamgeDBusArbiterManager *arbiter_manager;
  ChamgeBackend backend;
  ChamgeArbiter *arbiter;

  /* TODO: belows are using temporaryily.
   * mySQL would be use for management data.
   */
  GList *edges;
  GList *hubs;
};

typedef enum
{
  PROP_HOST = 1,
  PROP_BACKEND,

  /*< private > */
  PROP_LAST = PROP_BACKEND
} _ChamgeArbiterAgentProperty;

static GParamSpec *properties[PROP_LAST + 1];
typedef void (*sig_handler_t) (int);
static sig_handler_t signal_register[32];

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeArbiterAgent, chamge_arbiter_agent, G_TYPE_APPLICATION)
/* *INDENT-ON* */

static void
signal_handler (int sig)
{
  void *array[10];
  size_t size;

  printf ("signal %d. org %p. restore tty\n", sig, signal_register[sig]);

  // get void*'s for all entries on the stack
  size = backtrace (array, 10);

  // print out all the frames to stderr
  fprintf (stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd (array, size, STDERR_FILENO);

  exit (128 + sig);
}

static gboolean
chamge_arbiter_agent_dbus_register (GApplication * app,
    GDBusConnection * connection, const gchar * object_path, GError ** error)
{
  gboolean ret;
  ChamgeArbiterAgent *self = CHAMGE_ARBITER_AGENT (app);

  g_application_hold (app);

  /* chain up */
  ret =
      G_APPLICATION_CLASS (chamge_arbiter_agent_parent_class)->dbus_register
      (app, connection, object_path, error);

  if (ret &&
      !g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON
          (self->arbiter_manager), connection,
          "/org/hwangsaeul/Chamge1/Arbiter/Manager", error)) {
    g_warning ("Failed to export Chamge1 D-Bus interface (reason: %s)",
        (*error)->message);
  }

  return ret;
}

static void
chamge_arbiter_agent_dbus_unregister (GApplication * app,
    GDBusConnection * connection, const gchar * object_path)
{
  ChamgeArbiterAgent *self = CHAMGE_ARBITER_AGENT (app);

  if (self->arbiter_manager)
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON
        (self->arbiter_manager));

  g_application_release (app);

  /* chain up */
  G_APPLICATION_CLASS (chamge_arbiter_agent_parent_class)->dbus_unregister (app,
      connection, object_path);
}

static void
chamge_arbiter_agent_dispose (GObject * object)
{
  ChamgeArbiterAgent *self = CHAMGE_ARBITER_AGENT (object);

  g_list_foreach (self->edges, (GFunc) g_free, NULL);
  g_list_free (self->edges);

  g_list_foreach (self->hubs, (GFunc) g_free, NULL);
  g_list_free (self->hubs);

  g_clear_object (&self->arbiter_manager);

  G_OBJECT_CLASS (chamge_arbiter_agent_parent_class)->dispose (object);
}

static void
chamge_arbiter_agent_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  ChamgeArbiterAgent *self = CHAMGE_ARBITER_AGENT (object);

  switch ((_ChamgeArbiterAgentProperty) prop_id) {
    case PROP_HOST:
    case PROP_BACKEND:
      g_value_set_enum (value, self->backend);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_arbiter_agent_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ChamgeArbiterAgent *self = CHAMGE_ARBITER_AGENT (object);

  switch ((_ChamgeArbiterAgentProperty) prop_id) {
    case PROP_HOST:
      break;
    case PROP_BACKEND:
      self->backend = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_arbiter_agent_class_init (ChamgeArbiterAgentClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->get_property = chamge_arbiter_agent_get_property;
  object_class->set_property = chamge_arbiter_agent_set_property;
  object_class->dispose = chamge_arbiter_agent_dispose;

  properties[PROP_HOST] = g_param_spec_string ("host", "host", "host",
      NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  properties[PROP_BACKEND] = g_param_spec_enum ("backend", "backend", "backend",
      CHAMGE_TYPE_BACKEND, CHAMGE_BACKEND_UNKNOWN,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties),
      properties);

  app_class->dbus_register = chamge_arbiter_agent_dbus_register;
  app_class->dbus_unregister = chamge_arbiter_agent_dbus_unregister;
}

static gboolean
chamge_arbiter_agent_handle_enroll (ChamgeDBusArbiterManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeArbiterAgent *self = (ChamgeArbiterAgent *) user_data;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  if (self->arbiter == NULL)
    g_error ("arbiter is NULL");
  ret = chamge_node_enroll (CHAMGE_NODE (self->arbiter), FALSE);
  if (ret != CHAMGE_RETURN_OK) {
    g_error ("arbiter enroll failure");
  }

  chamge_dbus_arbiter_manager_complete_enroll (manager, invocation);

  return TRUE;
}

static gboolean
chamge_arbiter_agent_handle_delist (ChamgeDBusArbiterManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeArbiterAgent *self = (ChamgeArbiterAgent *) user_data;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  ret = chamge_node_delist (CHAMGE_NODE (self->arbiter));
  if (ret != CHAMGE_RETURN_OK) {
    g_error ("arbiter delist failure");
  }

  chamge_dbus_arbiter_manager_complete_delist (manager, invocation);

  return TRUE;
}

static gboolean
chamge_arbiter_agent_handle_activate (ChamgeDBusArbiterManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeArbiterAgent *self = (ChamgeArbiterAgent *) user_data;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  ret = chamge_node_activate (CHAMGE_NODE (self->arbiter));
  if (ret != CHAMGE_RETURN_OK) {
    g_error ("arbiter activate failure");
  }

  chamge_dbus_arbiter_manager_complete_activate (manager, invocation);

  return TRUE;
}

static gboolean
chamge_arbiter_agent_handle_deactivate (ChamgeDBusArbiterManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeArbiterAgent *self = (ChamgeArbiterAgent *) user_data;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;

  ret = chamge_node_deactivate (CHAMGE_NODE (self->arbiter));
  if (ret != CHAMGE_RETURN_OK) {
    g_error ("arbiter deactivate failure");
  }

  chamge_dbus_arbiter_manager_complete_deactivate (manager, invocation);

  return TRUE;
}

static gboolean
chamge_arbiter_agent_handle_user_command (ChamgeDBusArbiterManager *
    manager, GDBusMethodInvocation * invocation, gchar * user_cmd,
    gpointer user_data)
{
  ChamgeArbiterAgent *self = (ChamgeArbiterAgent *) user_data;
  ChamgeReturn ret = CHAMGE_RETURN_FAIL;
  g_autofree gchar *response = NULL;
  GError *error = NULL;

  JsonNode *root = NULL;
  JsonObject *json_object = NULL;
  g_autoptr (JsonParser) parser = json_parser_new ();
  g_autofree gchar *final_cmd = g_strdup (user_cmd);

  g_debug ("user command >> %s", (gchar *) user_cmd);

  /* TODO
   * check user command and raplace "to" parameter to server
   * for vod service
   * below codes are tempory code because hub(hwangsae) is not
   * implemented yet
   */
  if (!json_parser_load_from_data (parser, user_cmd, strlen (user_cmd), &error)) {
    g_debug ("failed to parse body: %s", error->message);
    response = g_strdup ("{\"result\":\"failed to parse body\"}");
    goto out;
  }

  root = json_parser_get_root (parser);
  json_object = json_node_get_object (root);
  if (json_object_has_member (json_object, "method")) {
    JsonNode *node = json_object_get_member (json_object, "method");
    const gchar *method = json_node_get_string (node);
    /* send to edge */
    if (!g_strcmp0 (method, "streamingStart")
        || !g_strcmp0 (method, "streamingStop")
        || !g_strcmp0 (method, "getUrl")) {
      g_debug ("command for edge");
    } else {
      g_debug ("command is not for edge");
      if (json_object_has_member (json_object, "to")) {
        const gchar *hub_id = NULL;

        hub_id = (gchar *) g_list_nth_data (self->hubs, 0);
        if (hub_id == NULL) {
          printf ("no hub is enrolled\n");
        } else {
          json_object_set_string_member (json_object, "to", hub_id);
          final_cmd = json_to_string (root, FALSE);
          g_debug ("cmd changed from %s", user_cmd);
          g_debug ("              to %s", final_cmd);
        }
      }
    }
  }

  ret =
      chamge_node_user_command (CHAMGE_NODE (self->arbiter),
      (gchar *) final_cmd, &response, &error);
  if (ret != CHAMGE_RETURN_OK) {
    g_warning ("arbiter user command failure >> %s",
        error ? error->message : "");
    if (response == NULL) {
      response =
          g_strdup_printf ("{\"result\":\"%s\"}",
          error ? error->message : "nok");
    }
  }
out:
  g_debug ("response >> %s (%d)", response, ret);

  chamge_dbus_arbiter_manager_complete_user_command (manager,
      invocation, ret, response);

  return TRUE;
}

static void
chamge_arbiter_agent_init (ChamgeArbiterAgent * self)
{
  self->arbiter_manager = chamge_dbus_arbiter_manager_skeleton_new ();

  g_signal_connect (self->arbiter_manager, "handle-enroll",
      G_CALLBACK (chamge_arbiter_agent_handle_enroll), self);

  g_signal_connect (self->arbiter_manager, "handle-delist",
      G_CALLBACK (chamge_arbiter_agent_handle_delist), self);

  g_signal_connect (self->arbiter_manager, "handle-activate",
      G_CALLBACK (chamge_arbiter_agent_handle_activate), self);

  g_signal_connect (self->arbiter_manager, "handle-deactivate",
      G_CALLBACK (chamge_arbiter_agent_handle_deactivate), self);

  g_signal_connect (self->arbiter_manager, "handle-user-command",
      G_CALLBACK (chamge_arbiter_agent_handle_user_command), self);
}

static gint
handle_local_options (GApplication * app, GVariantDict * options,
    gpointer user_data)
{
  gboolean b;
  const gchar *host;
  const gchar *backend;

  if (g_variant_dict_lookup (options, "version", "b", &b)) {
    g_print ("version %s\n", VERSION);
    return EXIT_SUCCESS;
  }

  if (g_variant_dict_lookup (options, "host", "s", &host)) {
    g_object_set (G_OBJECT (app), "host", host, NULL);
  }

  if (g_variant_dict_lookup (options, "backend", "s", &backend)) {
    GEnumClass *c = g_type_class_ref (CHAMGE_TYPE_BACKEND);
    GEnumValue *v = g_enum_get_value_by_nick (c, backend);

    if (v == NULL) {
      /* TODO: print usage? */
      g_type_class_unref (c);
      return EXIT_SUCCESS;
    }

    g_object_set (G_OBJECT (app), "backend", v->value, NULL);
    g_type_class_unref (c);
  }

  return -1;                    /* continue to prcess */
}

void
edge_enrolled_cb (ChamgeArbiter * arbiter, const gchar * edge_id,
    ChamgeArbiterAgent * agent)
{
  /* TODO : need to implement */
  printf ("[DUMMY] edge enroll callback >> edge_id: %s\n", edge_id);
  agent->edges = g_list_insert (agent->edges, g_strdup (edge_id), 0);
}

void
edge_delisted_cb (ChamgeArbiter * arbiter, const gchar * edge_id,
    ChamgeArbiterAgent * agent)
{
  /* TODO : need to implement */
  printf ("[DUMMY] edge delisted callback >>  edge_id: %s\n", edge_id);
}

void
hub_enrolled_cb (ChamgeArbiter * arbiter, const gchar * hub_id,
    ChamgeArbiterAgent * agent)
{
  /* TODO : need to implement */
  printf ("[DUMMY] hub enroll callback >> hub_id: %s\n", hub_id);
  agent->hubs = g_list_insert (agent->hubs, g_strdup (hub_id), 0);
}

void
hub_delisted_cb (ChamgeArbiter * arbiter, const gchar * hub_id,
    ChamgeArbiterAgent * agent)
{
  /* TODO : need to implement */
  printf ("[DUMMY] hub delisted callback  >> hub_id: %s\n", hub_id);

}

static void
chamge_arbiter_agent_startup (GApplication * app)
{
  ChamgeArbiterAgent *self = CHAMGE_ARBITER_AGENT (app);
  g_autofree gchar *uid = NULL;

  uid = g_uuid_string_random ();
  uid = g_compute_checksum_for_string (G_CHECKSUM_SHA256, uid, strlen (uid));

  self->arbiter = chamge_arbiter_new_full (uid, self->backend);
  if (self->arbiter == NULL) {
    g_error ("failed to create arbiter");
  }
  g_debug ("arbiter is created :%p ->  %p", self, self->arbiter);

  g_signal_connect (self->arbiter, "edge-enrolled",
      G_CALLBACK (edge_enrolled_cb), self);
  g_signal_connect (self->arbiter, "edge-delisted",
      G_CALLBACK (edge_delisted_cb), self);
  g_signal_connect (self->arbiter, "hub-enrolled",
      G_CALLBACK (hub_enrolled_cb), self);
  g_signal_connect (self->arbiter, "hub-delisted",
      G_CALLBACK (hub_delisted_cb), self);
}

int
main (int argc, char *argv[])
{
  g_autoptr (GApplication) app = NULL;
  GOptionEntry entries[] = {
    {"version", 'v', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, NULL,
        "Show the application version", NULL},
    {"host", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, NULL,
        "Set the arbiter host", NULL},
    {"backend", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, NULL,
        "Set the backend type {amqp,mock}", NULL},
    {NULL}
  };

  signal_register[SIGHUP] = signal (SIGHUP, signal_handler);
  signal_register[SIGINT] = signal (SIGINT, signal_handler);
  signal_register[SIGABRT] = signal (SIGABRT, signal_handler);
  signal_register[SIGFPE] = signal (SIGFPE, signal_handler);
  signal_register[SIGKILL] = signal (SIGKILL, signal_handler);
  signal_register[SIGSEGV] = signal (SIGSEGV, signal_handler);

  app = G_APPLICATION (g_object_new (CHAMGE_TYPE_ARBITER_AGENT,
          "application-id", "org.hwangsaeul.Chamge1",
          "flags",
          G_APPLICATION_IS_SERVICE | G_APPLICATION_HANDLES_COMMAND_LINE, NULL));

  g_application_add_main_option_entries (app, entries);

  g_signal_connect (app, "handle-local-options",
      G_CALLBACK (handle_local_options), app);
  g_signal_connect (app, "startup",
      G_CALLBACK (chamge_arbiter_agent_startup), app);

  g_application_set_inactivity_timeout (app, 10000);

  return g_application_run (app, argc, argv);
}
