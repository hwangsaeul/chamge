/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_AMQP_EDGE_BACKEND_H__
#define __CHAMGE_AMQP_EDGE_BACKEND_H__

#include <chamge/edge-backend.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_AMQP_EDGE_BACKEND   (chamge_amqp_edge_backend_get_type ())
G_DECLARE_FINAL_TYPE (ChamgeAmqpEdgeBackend, chamge_amqp_edge_backend, CHAMGE, AMQP_EDGE_BACKEND, ChamgeEdgeBackend)

G_END_DECLS

#endif // __CHAMGE_AMQP_EDGE_BACKEND_H__
