/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_DBUS_TYPES_H__
#define __CHAMGE_DBUS_TYPES_H__

#include <chamge/types.h>

#define CHAMGE_DBUS_ERROR      (tview_dbus_error_quark ())

typedef enum {
  CHAMGE_DBUS_ERROR_RESOURCE_NOT_FOUND = -1,
  CHAMGE_DBUS_ERROR_WRONG_PARAMETER = -2,
  CHAMGE_DBUS_ERROR_NOT_IMPLEMENTED = -999,
} ChamgaeDBusError;

GQuark chamge_dbus_error_quark (void);

#endif // __CHAMGE_DBUS_TYPES_H__
