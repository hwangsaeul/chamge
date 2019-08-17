/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_TYPES_H__
#define __CHAMGE_TYPES_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#ifndef _CHAMGE_EXTERN
#define _CHAMGE_EXTERN         extern
#endif

#define CHAMGE_API_EXPORT      _CHAMGE_EXTERN

typedef enum {
  CHAMGE_RETURN_FAIL = -1,
  CHAMGE_RETURN_OK,
  CHAMGE_RETURN_ASYNC,
} ChamgeReturn;

typedef enum {
  CHAMGE_BACKEND_UNKNOWN = 0,
  CHAMGE_BACKEND_MOCK,
  CHAMGE_BACKEND_AMQP,
} ChamgeBackend;


#endif // __CHAMGE_TYPES_H__
