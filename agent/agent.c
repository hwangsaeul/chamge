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

static void
chamge_agent_init (ChamgeAgent * self)
{
  self->dbus_manager = chamge_dbus_manager_skeleton_new ();
}

int
main (int argc, char *argv[])
{
  g_autoptr (GApplication) app = NULL;

  app = G_APPLICATION (g_object_new (CHAMGE_TYPE_AGENT,
          "application-id", "org.hwangsaeul.Chamge1",
          "flags", G_APPLICATION_IS_SERVICE, NULL));

  return g_application_run (app, argc, argv);
}
