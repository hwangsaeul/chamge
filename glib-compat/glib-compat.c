/**
 *  Copyright 2019 SK Telecom Co., Ltd.
 *    Author: Jakub Adam <jakub.adam@collabora.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include "glib-compat.h"

#if ! GLIB_CHECK_VERSION (2, 52, 0)

gchar *
g_uuid_string_random (void)
{
  int i;
  union
  {
    guint8 bytes[16];
    guint32 ints[4];
  } uuid;

  for (i = 0; i != G_N_ELEMENTS (uuid.ints); ++i) {
    uuid.ints[i] = g_random_int ();
  }

  return g_strdup_printf
      ("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      uuid.bytes[0], uuid.bytes[1], uuid.bytes[2], uuid.bytes[3],
      uuid.bytes[4], uuid.bytes[5], uuid.bytes[6], uuid.bytes[7],
      uuid.bytes[8], uuid.bytes[9], uuid.bytes[10], uuid.bytes[11],
      uuid.bytes[12], uuid.bytes[13], uuid.bytes[14], uuid.bytes[15]);
}

#endif
