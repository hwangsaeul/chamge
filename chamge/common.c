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

#include "common.h"

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

GSettings *
chamge_common_gsettings_new (const gchar * schema_id)
{
  if (glib_check_version (2, 60, 5) && getppid () == 1) {
    /* Workaround GLib bug where keyfile GSettings backend wasn't registered. */
    g_autoptr (GSettingsBackend) backend = NULL;
    g_autofree char *filename = NULL;

    filename = g_build_filename (g_get_user_config_dir (), "glib-2.0",
        "settings", "keyfile", NULL);

    backend = g_keyfile_settings_backend_new (filename, "/", NULL);

    return g_settings_new_with_backend (schema_id, backend);
  }

  return g_settings_new (schema_id);
}
