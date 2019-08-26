/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_AMQP_ARBITER_BACKEND_H__
#define __CHAMGE_AMQP_ARBITER_BACKEND_H__

#include <chamge/arbiter-backend.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_AMQP_ARBITER_BACKEND        (chamge_amqp_arbiter_backend_get_type ())
G_DECLARE_FINAL_TYPE                            (ChamgeAmqpArbiterBackend, chamge_amqp_arbiter_backend,
                                                 CHAMGE, AMQP_ARBITER_BACKEND,
                                                 ChamgeArbiterBackend)

G_END_DECLS

#endif // __CHAMGE_AMQP_ARBITER_BACKEND_H__
