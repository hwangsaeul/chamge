/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_MOCK_ARBITER_BACKEND_H__
#define __CHAMGE_MOCK_ARBITER_BACKEND_H__

#include <chamge/arbiter-backend.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_MOCK_ARBITER_BACKEND        (chamge_mock_arbiter_backend_get_type ())
G_DECLARE_FINAL_TYPE                            (ChamgeMockArbiterBackend, chamge_mock_arbiter_backend,
                                                 CHAMGE, MOCK_ARBITER_BACKEND, ChamgeArbiterBackend)

G_END_DECLS

#endif // __CHAMGE_MOCK_ARBITER_BACKEND_H__
