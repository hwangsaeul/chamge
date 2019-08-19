/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_EDGE_AGENT_H__
#define __CHAMGE_EDGE_AGENT_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_EDGE_AGENT  (chamge_edge_agent_get_type ())
G_DECLARE_FINAL_TYPE            (ChamgeEdgeAgent, chamge_edge_agent, CHAMGE, EDGE_AGENT, GApplication)

G_END_DECLS

#endif // __CHAMGE_EDGE_AGENT_H__
