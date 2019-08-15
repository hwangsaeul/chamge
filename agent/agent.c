/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "agent.h"

struct _ChamgeAgent
{
  GApplication parent;
};

/* *INDENT-OFF* */
G_DEFINE_TYPE (ChamgeAgent, chamge_agent, G_TYPE_APPLICATION)
/* *INDENT-ON* */

static void
chamge_agent_class_init (ChamgeAgentClass * klass)
{
}

static void
chamge_agent_init (ChamgeAgent * self)
{
}

int
main (int argc, char *argv[])
{
  g_autoptr (GApplication) app = NULL;

  app = G_APPLICATION (g_object_new (CHAMGE_TYPE_AGENT,
          "application-id", "org.chamgeul.Chamge1",
          "flags", G_APPLICATION_IS_SERVICE, NULL));

  return g_application_run (app, argc, argv);
}
