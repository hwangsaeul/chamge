/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "agent.h"
#include "chamge/dbus/manager-generated.h"

struct _ChamgeAgent
{
  GApplication parent;
  ChamgeDBusManager *dbus_manager;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeAgent, chamge_agent, G_TYPE_APPLICATION)
/* *INDENT-ON* */

static void
chamge_agent_activate (GApplication * app)
{
  g_debug ("activate");
}

static gboolean
chamge_agent_dbus_register (GApplication * app,
    GDBusConnection * connection, const gchar * object_path, GError ** error)
{
  gboolean ret;
  ChamgeAgent *self = CHAMGE_AGENT (app);

  g_application_hold (app);

  /* chain up */
  ret = G_APPLICATION_CLASS (chamge_agent_parent_class)->dbus_register (app,
      connection, object_path, error);

  if (ret &&
      !g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON
          (self->dbus_manager), connection, "/org/hwangsaeul/Chamge1/Manager",
          error)) {
    g_warning ("Failed to export Chamge1 D-Bus interface (reason: %s)",
        (*error)->message);
  }

  return ret;
}

static void
chamge_agent_dbus_unregister (GApplication * app,
    GDBusConnection * connection, const gchar * object_path)
{
  ChamgeAgent *self = CHAMGE_AGENT (app);

  if (self->dbus_manager)
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON
        (self->dbus_manager));

  g_application_release (app);

  /* chain up */
  G_APPLICATION_CLASS (chamge_agent_parent_class)->dbus_unregister (app,
      connection, object_path);
}

static void
chamge_agent_dispose (GObject * object)
{
  ChamgeAgent *self = CHAMGE_AGENT (object);

  g_clear_object (&self->dbus_manager);

  G_OBJECT_CLASS (chamge_agent_parent_class)->dispose (object);
}

static void
chamge_agent_class_init (ChamgeAgentClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->dispose = chamge_agent_dispose;

  app_class->activate = chamge_agent_activate;
  app_class->dbus_register = chamge_agent_dbus_register;
  app_class->dbus_unregister = chamge_agent_dbus_unregister;
}

static gboolean
chamge_agent_handle_enroll (ChamgeDBusManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeAgent *self = CHAMGE_AGENT (user_data);

  chamge_dbus_manager_complete_enroll (manager, invocation);

  return TRUE;
}

static gboolean
chamge_agent_handle_delist (ChamgeDBusManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeAgent *self = CHAMGE_AGENT (user_data);

  chamge_dbus_manager_complete_delist (manager, invocation);

  return TRUE;
}

static gboolean
chamge_agent_handle_activate (ChamgeDBusManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeAgent *self = CHAMGE_AGENT (user_data);

  chamge_dbus_manager_complete_activate (manager, invocation);

  return TRUE;
}

static gboolean
chamge_agent_handle_deactivate (ChamgeDBusManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeAgent *self = CHAMGE_AGENT (user_data);

  chamge_dbus_manager_complete_deactivate (manager, invocation);

  return TRUE;
}

static gboolean
chamge_agent_handle_request_srtconnection_uri (ChamgeDBusManager * manager,
    GDBusMethodInvocation * invocation, gpointer user_data)
{
  ChamgeAgent *self = CHAMGE_AGENT (user_data);

  chamge_dbus_manager_complete_request_srtconnection_uri (manager, invocation,
      NULL);

  return TRUE;
}

static void
chamge_agent_init (ChamgeAgent * self)
{
  self->dbus_manager = chamge_dbus_manager_skeleton_new ();

  g_signal_connect (self->dbus_manager, "handle-enroll",
      G_CALLBACK (chamge_agent_handle_enroll), self);

  g_signal_connect (self->dbus_manager, "handle-delist",
      G_CALLBACK (chamge_agent_handle_delist), self);

  g_signal_connect (self->dbus_manager, "handle-activate",
      G_CALLBACK (chamge_agent_handle_activate), self);

  g_signal_connect (self->dbus_manager, "handle-deactivate",
      G_CALLBACK (chamge_agent_handle_deactivate), self);

  g_signal_connect (self->dbus_manager, "handle-request-srtconnection-uri",
      G_CALLBACK (chamge_agent_handle_request_srtconnection_uri), self);
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

  app = G_APPLICATION (g_object_new (CHAMGE_TYPE_AGENT,
          "application-id", "org.hwangsaeul.Chamge1",
          "flags",
          G_APPLICATION_IS_SERVICE | G_APPLICATION_HANDLES_COMMAND_LINE, NULL));

  g_application_add_main_option_entries (app, entries);

  g_signal_connect (app, "handle-local-options",
      G_CALLBACK (handle_local_options), NULL);

  g_application_set_inactivity_timeout (app, 10000);

  return g_application_run (app, argc, argv);
}
