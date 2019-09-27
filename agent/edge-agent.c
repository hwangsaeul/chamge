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

#include "edge-agent.h"

#include <glib.h>

#include <chamge/enumtypes.h>
#include <chamge/edge.h>
#include <chamge/dbus/edge-manager-generated.h>

struct _ChamgeEdgeAgent
{
  GApplication parent;
  ChamgeDBusEdgeManager *edge_manager;
  ChamgeBackend backend;
};

typedef enum
{
  PROP_HOST = 1,
  PROP_BACKEND,

  /*< private > */
  PROP_LAST = PROP_BACKEND
} _ChamgeEdgeAgentProperty;

static GParamSpec *properties[PROP_LAST + 1];

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeEdgeAgent, chamge_edge_agent, G_TYPE_APPLICATION)
/* *INDENT-ON* */

static void
chamge_edge_agent_activate (GApplication * app)
{
  g_debug ("activate");
}

static gboolean
chamge_edge_agent_dbus_register (GApplication * app,
    GDBusConnection * connection, const gchar * object_path, GError ** error)
{
  gboolean ret;
  ChamgeEdgeAgent *self = CHAMGE_EDGE_AGENT (app);

  g_application_hold (app);

  /* chain up */
  ret =
      G_APPLICATION_CLASS (chamge_edge_agent_parent_class)->dbus_register (app,
      connection, object_path, error);

  if (ret &&
      !g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON
          (self->edge_manager), connection,
          "/org/hwangsaeul/Chamge1/Edge/Manager", error)) {
    g_warning ("Failed to export Chamge1 D-Bus interface (reason: %s)",
        (*error)->message);
  }

  return ret;
}

static void
chamge_edge_agent_dbus_unregister (GApplication * app,
    GDBusConnection * connection, const gchar * object_path)
{
  ChamgeEdgeAgent *self = CHAMGE_EDGE_AGENT (app);

  if (self->edge_manager)
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON
        (self->edge_manager));

  g_application_release (app);

  /* chain up */
  G_APPLICATION_CLASS (chamge_edge_agent_parent_class)->dbus_unregister (app,
      connection, object_path);
}

static void
chamge_edge_agent_dispose (GObject * object)
{
  ChamgeEdgeAgent *self = CHAMGE_EDGE_AGENT (object);

  g_clear_object (&self->edge_manager);

  G_OBJECT_CLASS (chamge_edge_agent_parent_class)->dispose (object);
}

static void
chamge_edge_agent_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  ChamgeEdgeAgent *self = CHAMGE_EDGE_AGENT (object);

  switch ((_ChamgeEdgeAgentProperty) prop_id) {
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
chamge_edge_agent_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ChamgeEdgeAgent *self = CHAMGE_EDGE_AGENT (object);

  switch ((_ChamgeEdgeAgentProperty) prop_id) {
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
chamge_edge_agent_class_init (ChamgeEdgeAgentClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->get_property = chamge_edge_agent_get_property;
  object_class->set_property = chamge_edge_agent_set_property;
  object_class->dispose = chamge_edge_agent_dispose;

  properties[PROP_HOST] = g_param_spec_string ("host", "host", "host",
      NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  properties[PROP_BACKEND] = g_param_spec_enum ("backend", "backend", "backend",
      CHAMGE_TYPE_BACKEND, CHAMGE_BACKEND_UNKNOWN,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties),
      properties);

  app_class->activate = chamge_edge_agent_activate;
  app_class->dbus_register = chamge_edge_agent_dbus_register;
  app_class->dbus_unregister = chamge_edge_agent_dbus_unregister;
}

static gboolean
chamge_edge_agent_handle_enroll (ChamgeDBusEdgeManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeEdgeAgent *self = CHAMGE_EDGE_AGENT (user_data);

  chamge_dbus_edge_manager_complete_enroll (manager, invocation);

  return TRUE;
}

static gboolean
chamge_edge_agent_handle_delist (ChamgeDBusEdgeManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeEdgeAgent *self = CHAMGE_EDGE_AGENT (user_data);

  chamge_dbus_edge_manager_complete_delist (manager, invocation);

  return TRUE;
}

static gboolean
chamge_edge_agent_handle_activate (ChamgeDBusEdgeManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeEdgeAgent *self = CHAMGE_EDGE_AGENT (user_data);

  chamge_dbus_edge_manager_complete_activate (manager, invocation);

  return TRUE;
}

static gboolean
chamge_edge_agent_handle_deactivate (ChamgeDBusEdgeManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeEdgeAgent *self = CHAMGE_EDGE_AGENT (user_data);

  chamge_dbus_edge_manager_complete_deactivate (manager, invocation);

  return TRUE;
}

static gboolean
chamge_edge_agent_handle_request_srtconnection_uri (ChamgeDBusEdgeManager *
    manager, GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeEdgeAgent *self = CHAMGE_EDGE_AGENT (user_data);

  chamge_dbus_edge_manager_complete_request_srtconnection_uri (manager,
      invocation, NULL);

  return TRUE;
}

static void
chamge_edge_agent_init (ChamgeEdgeAgent * self)
{
  self->edge_manager = chamge_dbus_edge_manager_skeleton_new ();

  g_signal_connect (self->edge_manager, "handle-enroll",
      G_CALLBACK (chamge_edge_agent_handle_enroll), self);

  g_signal_connect (self->edge_manager, "handle-delist",
      G_CALLBACK (chamge_edge_agent_handle_delist), self);

  g_signal_connect (self->edge_manager, "handle-activate",
      G_CALLBACK (chamge_edge_agent_handle_activate), self);

  g_signal_connect (self->edge_manager, "handle-deactivate",
      G_CALLBACK (chamge_edge_agent_handle_deactivate), self);

  g_signal_connect (self->edge_manager, "handle-request-srtconnection-uri",
      G_CALLBACK (chamge_edge_agent_handle_request_srtconnection_uri), self);
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

  app = G_APPLICATION (g_object_new (CHAMGE_TYPE_EDGE_AGENT,
          "application-id", "org.hwangsaeul.Chamge1",
          "flags",
          G_APPLICATION_IS_SERVICE | G_APPLICATION_HANDLES_COMMAND_LINE, NULL));

  g_application_add_main_option_entries (app, entries);

  g_signal_connect (app, "handle-local-options",
      G_CALLBACK (handle_local_options), NULL);

  g_application_set_inactivity_timeout (app, 10000);

  return g_application_run (app, argc, argv);
}
