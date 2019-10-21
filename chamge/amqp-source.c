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

#include "amqp-source.h"

typedef struct
{
  GSource source;
  amqp_connection_state_t state;
  GPollFD pollfd;
} ChamgeAmpqSource;

static gboolean
chamge_amqp_source_prepare (GSource * source, gint * timeout)
{
  ChamgeAmpqSource *amqp_source = (ChamgeAmpqSource *) source;

  if (amqp_frames_enqueued (amqp_source->state)) {
    return TRUE;
  }

  *timeout = -1;

  return FALSE;
}

static gboolean
chamge_amqp_source_check (GSource * source)
{
  ChamgeAmpqSource *amqp_source = (ChamgeAmpqSource *) source;

  return amqp_source->pollfd.revents & (G_IO_IN | G_IO_HUP | G_IO_ERR);
}

static gboolean
chamge_amqp_source_dispatch (GSource * source, GSourceFunc callback,
    gpointer user_data)
{
  ChamgeAmpqSource *amqp_source = (ChamgeAmpqSource *) source;
  ChamgeAmqpFunc handler = (ChamgeAmqpFunc) callback;
  static struct timeval timeout = { 0, 0 };
  amqp_rpc_reply_t rpc_reply;
  amqp_envelope_t envelope;
  gboolean keep;

  if (!callback) {
    return G_SOURCE_REMOVE;
  }

  amqp_maybe_release_buffers (amqp_source->state);

  rpc_reply = amqp_consume_message (amqp_source->state, &envelope, &timeout, 0);

  if (rpc_reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION
      && rpc_reply.library_error == AMQP_STATUS_TIMEOUT) {
    return G_SOURCE_CONTINUE;
  }

  keep = handler (amqp_source->state, &rpc_reply, &envelope, user_data);

  amqp_destroy_envelope (&envelope);

  return keep;
}

static GSource *
chamge_amqp_source_new (amqp_connection_state_t state)
{
  static GSourceFuncs source_funcs = {
    chamge_amqp_source_prepare,
    chamge_amqp_source_check,
    chamge_amqp_source_dispatch,
    NULL
  };

  GSource *source = g_source_new (&source_funcs, sizeof (ChamgeAmpqSource));
  ChamgeAmpqSource *amqp_source = (ChamgeAmpqSource *) source;

  amqp_source->state = state;
  amqp_source->pollfd.fd = amqp_get_sockfd (state);
  amqp_source->pollfd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
  amqp_source->pollfd.revents = 0;

  g_source_add_poll (source, &amqp_source->pollfd);

  return source;
}

guint
chamge_amqp_add_watch (amqp_connection_state_t state, ChamgeAmqpFunc callback,
    gpointer data)
{
  g_autoptr (GSource) source = NULL;

  g_return_val_if_fail (callback != NULL, 0);

  source = chamge_amqp_source_new (state);

  g_source_set_callback (source, (GSourceFunc) callback, data, NULL);

  return g_source_attach (source, NULL);
}
