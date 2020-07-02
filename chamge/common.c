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

#define HWANGSAEUL_CONF "hwangsaeul.conf"

GSettings *
chamge_common_gsettings_new (const gchar * schema_id)
{
  const gchar *config_path = "/etc/" HWANGSAEUL_CONF;

  g_autofree gchar *user_config_path = NULL;
  g_autoptr (GFile) config_file = NULL;
  g_autoptr (GSettingsBackend) backend = NULL;

  user_config_path =
      g_build_filename (g_get_user_config_dir (), HWANGSAEUL_CONF, NULL);
  config_file = g_file_new_for_path (user_config_path);
  if (g_file_query_exists (config_file, NULL)) {
    config_path = user_config_path;
  }

  backend = g_keyfile_settings_backend_new (config_path, "/", NULL);

  return g_settings_new_with_backend (schema_id, backend);
}
