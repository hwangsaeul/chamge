/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "edge-agent.h"
#include <chamge/edge.h>
#include <chamge/dbus/edge-manager-generated.h>

struct _ChamgeEdgeAgent
{
  GApplication parent;
  ChamgeDBusEdgeManager *edge_manager;
};

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
chamge_edge_agent_class_init (ChamgeEdgeAgentClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->dispose = chamge_edge_agent_dispose;

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

  if (g_variant_dict_lookup (options, "version", "b", &b)) {
    g_print ("version %s\n", VERSION);
    return EXIT_SUCCESS;
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
