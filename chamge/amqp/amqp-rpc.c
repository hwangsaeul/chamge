/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Heekyoung Seo <hkseo@sk.com>
 *
 */

#include "amqp-rpc.h"

#include <amqp.h>
#include <amqp_tcp_socket.h>

#define DEFAULT_PORT 5672
#define DEFAULT_CHANNEL 1
#define DEFAULT_CONTENT_TYPE "application/json"

typedef enum
{
  CHAMGE_AMQP_STATUS_NULL = 0,  /* initial state */
  CHAMGE_AMQP_STATUS_READY,     /* socket created */
  CHAMGE_AMQP_STATUS_CONNECTED, /* login finshed */
  CHAMGE_AMQP_STATUS_SUBSCRIBING,       /* ready to read message */
  CHAMGE_AMQP_STATUS_RECEIVED,  /* message received */
} ChamgeAmqpRpcStatus;

typedef struct amqp_connection_info rpc_connection_info;

typedef struct _ChamgeAmqpRpcPrivate ChamgeAmqpRpcPrivate;
struct _ChamgeAmqpRpcPrivate
{
  g_autofree gchar *amqp_url;

  rpc_connection_info connection_info;
  guint channel;

  ChamgeAmqpRpcStatus state;

  amqp_connection_state_t amqp_conn;
  amqp_socket_t *amqp_socket;

  GList *bindingkey_list;

} ChamgeAmqpRcpPrivate;


typedef enum
{
  PROP_HOSTNAME = 1,
  PROP_PORT,
  PROP_VHOST,
  PROP_ID,
  PROP_PWD,
  PROP_CHANNEL,
  PROP_URL,
  PROP_LAST = PROP_URL
} _ChamgeAmqpRpcProperty;

static GParamSpec *properties[PROP_LAST + 1];

/* *INDENT-OFF* */
G_DEFINE_TYPE_WITH_CODE (ChamgeAmqpRpc, chamge_amqp_rpc, G_TYPE_OBJECT,
    G_ADD_PRIVATE (ChamgeAmqpRpc))
/* *INDENT-ON* */

static ChamgeReturn
_amqp_declare_queue (const amqp_connection_state_t conn,
    const gchar * queue_name, amqp_bytes_t * queue)
{
  amqp_queue_declare_ok_t *amqp_r = NULL;
  gchar *response_queue_name = NULL;

  if (queue_name == NULL) {
    amqp_r = amqp_queue_declare (conn, 1, amqp_empty_bytes, 0, 0, 0, 1,
        amqp_empty_table);
  } else {
    amqp_r =
        amqp_queue_declare (conn, 1, amqp_cstring_bytes (queue_name), 0, 0, 0,
        1, amqp_empty_table);
  }
  if (amqp_r == NULL) {
    g_error ("Can not declare a new queue or queue does not exist...");
    return CHAMGE_RETURN_FAIL;
  }
  amqp_get_rpc_reply (conn);

  g_assert_nonnull (amqp_r->queue.bytes);
  response_queue_name = g_strdup (amqp_r->queue.bytes);
  *queue = amqp_cstring_bytes (response_queue_name);

  g_debug ("create private reply_to queue : name -> %s",
      (gchar *) queue->bytes);
  return CHAMGE_RETURN_OK;
}

static ChamgeReturn
_amqp_property_set (amqp_basic_properties_t * const props,
    const guint correlation_id, const gchar * reply_queue_name)
{
  gchar *correlation_id_str = NULL;
  gchar *content_type_str = g_strdup (DEFAULT_CONTENT_TYPE);

  props->_flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
  props->content_type = amqp_cstring_bytes (content_type_str);
  props->delivery_mode = 2;     /* persistent delivery mode */

  if (reply_queue_name != NULL) {
    gchar *reply_to_queue = (gchar *) g_strdup (reply_queue_name);
    g_assert_nonnull (reply_to_queue);
    props->reply_to = amqp_cstring_bytes (reply_to_queue);
    props->_flags |= AMQP_BASIC_REPLY_TO_FLAG;
  } else {
    props->reply_to.bytes = NULL;
    props->reply_to.bytes = 0;
  }

  if (correlation_id != 0) {
    guint correlation_id_str_len = correlation_id / 10 + 2;
    props->_flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
    correlation_id_str =
        (char *) calloc (correlation_id_str_len, sizeof (char));
    g_return_val_if_fail (correlation_id_str != NULL, CHAMGE_RETURN_FAIL);
    g_snprintf (correlation_id_str, correlation_id_str_len, "%u",
        correlation_id);
    props->correlation_id = amqp_cstring_bytes (correlation_id_str);
  } else {
    props->correlation_id.bytes = NULL;
    props->correlation_id.len = 0;
  }

  g_debug ("content type : %s , correlation id : %s",
      (gchar *) props->content_type.bytes,
      (gchar *) props->correlation_id.bytes);
  return CHAMGE_RETURN_OK;
}

static void
_amqp_property_free (amqp_basic_properties_t * const props)
{
  if (props->content_type.bytes)
    amqp_bytes_free (props->content_type);
  if (props->reply_to.bytes)
    amqp_bytes_free (props->reply_to);
  if (props->correlation_id.bytes)
    amqp_bytes_free (props->correlation_id);
}

/* 
  reference from tools/common.c of rabbitmq-c
*/
inline static const char *
_amqp_server_exception_string (amqp_rpc_reply_t r)
{
  int res;
  static char s[512];

  switch (r.reply.id) {
    case AMQP_CONNECTION_CLOSE_METHOD:{
      amqp_connection_close_t *m = (amqp_connection_close_t *) r.reply.decoded;
      res =
          g_snprintf (s, sizeof (s),
          "server connection error %d, message: %.*s", m->reply_code,
          (int) m->reply_text.len, (char *) m->reply_text.bytes);
      break;
    }

    case AMQP_CHANNEL_CLOSE_METHOD:{
      amqp_channel_close_t *m = (amqp_channel_close_t *) r.reply.decoded;
      res = g_snprintf (s, sizeof (s), "server channel error %d, message: %.*s",
          m->reply_code, (int) m->reply_text.len, (char *) m->reply_text.bytes);
      break;
    }

    default:
      res = g_snprintf (s, sizeof (s), "unknown server error, method id 0x%08X",
          r.reply.id);
      break;
  }

  return res >= 0 ? s : NULL;
}


/* 
  reference from tools/common.c of rabbitmq-c
*/
inline static const gchar *
_amqp_rpc_reply_string (amqp_rpc_reply_t r)
{
  switch (r.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      return "normal response";

    case AMQP_RESPONSE_NONE:
      return "missing RPC reply type";

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      return amqp_error_string2 (r.library_error);

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      return _amqp_server_exception_string (r);

    default:
      abort ();
  }
}

static gchar *
_amqp_get_response (const amqp_connection_state_t conn,
    const amqp_bytes_t reply_to_queue)
{
  gchar *response_body = NULL;
  amqp_rpc_reply_t amqp_r;
  amqp_envelope_t envelope;

  amqp_basic_consume (conn, 1, reply_to_queue, amqp_empty_bytes, 0, 1, 0,
      amqp_empty_table);
  amqp_get_rpc_reply (conn);

  amqp_maybe_release_buffers (conn);

  g_debug ("Wait for response message from [%s]",
      (gchar *) reply_to_queue.bytes);
  amqp_r = amqp_consume_message (conn, &envelope, NULL, 0);

  if (AMQP_RESPONSE_NORMAL != amqp_r.reply_type) {
    g_error ("response is not normal. >> %s", _amqp_rpc_reply_string (amqp_r));
    return NULL;
  }

  g_debug ("Delivery %u, exchange %.*s routingkey %.*s",
      (unsigned) envelope.delivery_tag, (int) envelope.exchange.len,
      (char *) envelope.exchange.bytes, (int) envelope.routing_key.len,
      (char *) envelope.routing_key.bytes);

  if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
    g_debug ("Content-type: %.*s",
        (int) envelope.message.properties.content_type.len,
        (char *) envelope.message.properties.content_type.bytes);
  }

  response_body =
      g_strndup (envelope.message.body.bytes, envelope.message.body.len);
  amqp_destroy_envelope (&envelope);

  return response_body;
}

ChamgeAmqpRpc *
chamge_amqp_rpc_new ()
{
  return CHAMGE_AMQP_RPC (g_object_new (CHAMGE_TYPE_AMQP_RPC, NULL));
}

static void
chamge_amqp_rpc_dispose (GObject * object)
{
  ChamgeAmqpRpc *self = CHAMGE_AMQP_RPC (object);
  ChamgeAmqpRpcPrivate *priv = chamge_amqp_rpc_get_instance_private (self);

  if (priv->amqp_conn) {
    /* close connection & destroy */
    amqp_connection_close (priv->amqp_conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection (priv->amqp_conn);
    priv->amqp_conn = NULL;
  }

  G_OBJECT_CLASS (chamge_amqp_rpc_parent_class)->dispose (object);
}

static void
chamge_amqp_rpc_finalize (GObject * object)
{
  ChamgeAmqpRpc *self = CHAMGE_AMQP_RPC (object);
  ChamgeAmqpRpcPrivate *priv = chamge_amqp_rpc_get_instance_private (self);

  G_OBJECT_CLASS (chamge_amqp_rpc_parent_class)->finalize (object);
}

static void
chamge_amqp_rpc_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  ChamgeAmqpRpc *self = CHAMGE_AMQP_RPC (object);
  ChamgeAmqpRpcPrivate *priv = chamge_amqp_rpc_get_instance_private (self);

  switch ((_ChamgeAmqpRpcProperty) prop_id) {
    case PROP_HOSTNAME:
      g_value_set_string (value, priv->connection_info.host);
      break;
    case PROP_PORT:
      g_value_set_uint (value, priv->connection_info.port);
      break;
    case PROP_CHANNEL:
      g_value_set_uint (value, priv->channel);
      break;
    case PROP_VHOST:
      g_value_set_string (value, priv->connection_info.vhost);
      break;
    case PROP_ID:
      g_value_set_string (value, priv->connection_info.user);
      break;
    case PROP_PWD:
      g_value_set_string (value, priv->connection_info.password);
      break;
    case PROP_URL:
      g_value_set_string (value, priv->amqp_url);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
_amqp_parse_url (const gchar * url, rpc_connection_info * const conn_info)
{
  g_autofree gchar *s = g_strdup (url);
  amqp_parse_url (url, conn_info);
  g_debug ("url parsed : host[%s] vhost[%s]", conn_info->host,
      conn_info->vhost);
  g_debug ("    user[%s] pwd[%s] port[%d] ssl[%d]", (conn_info)->user,
      (conn_info)->password, (conn_info)->port);
}

static void
chamge_amqp_rpc_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ChamgeAmqpRpc *self = CHAMGE_AMQP_RPC (object);
  ChamgeAmqpRpcPrivate *priv = chamge_amqp_rpc_get_instance_private (self);

  switch ((_ChamgeAmqpRpcProperty) prop_id) {
    case PROP_HOSTNAME:
      priv->connection_info.host = g_value_dup_string (value);
      g_debug ("  set hostname : %s", priv->connection_info.host);
      break;
    case PROP_PORT:
      priv->connection_info.port = g_value_get_uint (value);
      g_debug ("  set port : %d", priv->connection_info.port);
      break;
    case PROP_CHANNEL:
      priv->channel = g_value_get_uint (value);
      g_debug ("  set channel : %d", priv->channel);
      break;
    case PROP_VHOST:
      priv->connection_info.vhost = g_value_dup_string (value);
      g_debug ("  set vhost : %s", priv->connection_info.vhost);
      break;
    case PROP_ID:
      priv->connection_info.user = g_value_dup_string (value);
      g_debug ("  set id : %s", priv->connection_info.user);
      break;
    case PROP_PWD:
      priv->connection_info.password = g_value_dup_string (value);
      g_debug ("  set pwd : %s", priv->connection_info.password);
      break;
    case PROP_URL:
      priv->amqp_url = g_value_dup_string (value);
      _amqp_parse_url (priv->amqp_url, &priv->connection_info);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
chamge_amqp_rpc_class_init (ChamgeAmqpRpcClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = chamge_amqp_rpc_get_property;
  object_class->set_property = chamge_amqp_rpc_set_property;
  object_class->dispose = chamge_amqp_rpc_dispose;
  object_class->finalize = chamge_amqp_rpc_finalize;

  properties[PROP_HOSTNAME] =
      g_param_spec_string ("hostname", "hostname", "hostname", NULL,
      G_PARAM_READWRITE);

  properties[PROP_PORT] = g_param_spec_uint ("port", "port", "port number",
      1025, 65535, 5672, G_PARAM_READWRITE);

  properties[PROP_CHANNEL] =
      g_param_spec_uint ("channel", "channel", "channel number", 1, 2047, 1,
      G_PARAM_READWRITE);

  properties[PROP_VHOST] = g_param_spec_string ("vhost", "vhost", "vhost name",
      NULL, G_PARAM_READWRITE);

  properties[PROP_ID] =
      g_param_spec_string ("id", "id", "use id for login amqp broker", NULL,
      G_PARAM_READWRITE);

  properties[PROP_PWD] =
      g_param_spec_string ("pwd", "pwd", "password for login amqp broker", NULL,
      G_PARAM_READWRITE);

  properties[PROP_URL] =
      g_param_spec_string ("url", "url", "url of amqp broker", NULL,
      G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties),
      properties);

}

static void
chamge_amqp_rpc_init (ChamgeAmqpRpc * self)
{
  ChamgeAmqpRpcPrivate *priv = chamge_amqp_rpc_get_instance_private (self);

  priv->connection_info.host = NULL;
  priv->connection_info.port = DEFAULT_PORT;
  priv->connection_info.vhost = NULL;
  priv->connection_info.user = NULL;
  priv->connection_info.password = NULL;

  priv->amqp_url = NULL;
  priv->channel = DEFAULT_CHANNEL;

  priv->state = CHAMGE_AMQP_STATUS_NULL;

  priv->amqp_conn = amqp_new_connection ();
  priv->amqp_socket = amqp_tcp_socket_new (priv->amqp_conn);
  g_assert_nonnull (priv->amqp_socket);

  priv->bindingkey_list = NULL;

  priv->state = CHAMGE_AMQP_STATUS_READY;

}


ChamgeReturn
chamge_amqp_rpc_login (ChamgeAmqpRpc * self)
{
  ChamgeAmqpRpcClass *klass = CHAMGE_AMQP_RPC_GET_CLASS (self);
  ChamgeAmqpRpcPrivate *priv = chamge_amqp_rpc_get_instance_private (self);
  amqp_rpc_reply_t amqp_r;

  g_return_val_if_fail (priv->state == CHAMGE_AMQP_STATUS_READY,
      CHAMGE_RETURN_FAIL);

  g_return_val_if_fail (priv->connection_info.host != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->connection_info.vhost != NULL,
      CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->connection_info.port != 0, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->channel != 0, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->connection_info.user != NULL, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->connection_info.password != NULL,
      CHAMGE_RETURN_FAIL);

  g_return_val_if_fail (amqp_socket_open (priv->amqp_socket,
          priv->connection_info.host, priv->connection_info.port) >= 0,
      CHAMGE_RETURN_FAIL);

  amqp_login (priv->amqp_conn, priv->connection_info.vhost, 0, 131072, 0,
      AMQP_SASL_METHOD_PLAIN, priv->connection_info.user,
      priv->connection_info.password);
  amqp_channel_open (priv->amqp_conn, priv->channel);
  amqp_r = amqp_get_rpc_reply (priv->amqp_conn);
  g_return_val_if_fail (amqp_r.reply_type == AMQP_RESPONSE_NORMAL,
      CHAMGE_RETURN_FAIL);

  priv->state = CHAMGE_AMQP_STATUS_CONNECTED;
  g_debug ("success to login %s:%d/%s id[%s] pwd[%s]",
      priv->connection_info.host, priv->connection_info.port,
      priv->connection_info.vhost, priv->connection_info.user,
      priv->connection_info.password);
  return CHAMGE_RETURN_OK;
}

gchar *
chamge_amqp_rpc_request (ChamgeAmqpRpc * self, const gchar * request,
    const gchar * exchange, const gchar * queue_name)
{
  ChamgeAmqpRpcClass *klass = CHAMGE_AMQP_RPC_GET_CLASS (self);
  ChamgeAmqpRpcPrivate *priv = chamge_amqp_rpc_get_instance_private (self);
  amqp_bytes_t amqp_reply_queue;
  amqp_basic_properties_t amqp_props;
  gchar *response_body;

  g_return_val_if_fail (priv->state == CHAMGE_AMQP_STATUS_CONNECTED, NULL);
  g_return_val_if_fail (queue_name != NULL, NULL);

  /* create private replay_to_queue and queue name is random that is supplied from amqp server */
  if (_amqp_declare_queue (priv->amqp_conn, NULL,
          &amqp_reply_queue) != CHAMGE_RETURN_OK)
    return NULL;

  g_debug ("declare a queue for reply : %s", (gchar *) amqp_reply_queue.bytes);

  /* send the message */
  if (_amqp_property_set (&amqp_props, priv->channel,
          (gchar *) amqp_reply_queue.bytes) != CHAMGE_RETURN_OK) {
    g_error ("create property failed.");
    return NULL;
  }

  /* send requst to queue_name */
  {
    gint r = amqp_basic_publish (priv->amqp_conn, priv->channel,
        exchange ==
        NULL ? amqp_cstring_bytes ("") : amqp_cstring_bytes (exchange),
        amqp_cstring_bytes (queue_name), 0, 0, &amqp_props,
        amqp_cstring_bytes (request));
    if (r < 0) {
      g_error ("publish can not be done >> %s", amqp_error_string2 (r));
      return NULL;
    }
    g_debug ("published to queue[%s], exchange[%s], request[%s]", queue_name,
        exchange, request);
  }

  /* wait an answer */
  response_body = _amqp_get_response (priv->amqp_conn, amqp_reply_queue);

  _amqp_property_free (&amqp_props);
  amqp_bytes_free (amqp_reply_queue);
  return response_body;
}


ChamgeReturn
chamge_amqp_rpc_subscribe (ChamgeAmqpRpc * self, const gchar * exchange_name,
    const gchar * queue_name)
{
  ChamgeAmqpRpcClass *klass = CHAMGE_AMQP_RPC_GET_CLASS (self);
  ChamgeAmqpRpcPrivate *priv = chamge_amqp_rpc_get_instance_private (self);

  amqp_bytes_t amqp_queue = amqp_cstring_bytes (queue_name);
  amqp_bytes_t amqp_listen_queue;

  g_return_val_if_fail (priv->state == CHAMGE_AMQP_STATUS_CONNECTED,
      CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->amqp_conn, CHAMGE_RETURN_FAIL);
  g_return_val_if_fail (priv->amqp_socket, CHAMGE_RETURN_FAIL);
  if (_amqp_declare_queue (priv->amqp_conn, queue_name, &amqp_listen_queue) !=
      CHAMGE_RETURN_OK) {
    return CHAMGE_RETURN_FAIL;
  }
  if (g_strcmp0 (queue_name, amqp_listen_queue.bytes) != 0) {
    g_error ("declare required queue name (%s) != declared queue name (%s)",
        queue_name, (char *) amqp_listen_queue.bytes);
    return CHAMGE_RETURN_FAIL;
  }
  amqp_bytes_free (amqp_listen_queue);

  if (g_list_first (priv->bindingkey_list) == NULL) {
    amqp_queue_bind (priv->amqp_conn, priv->channel, amqp_queue,
        amqp_cstring_bytes (exchange_name), amqp_queue, amqp_empty_table);

    g_debug ("binding a queue(%s) (exchange: %s, bind_key: %s)",
        queue_name, exchange_name, queue_name);
  } else {
    GList *bindingkey = g_list_first (priv->bindingkey_list);
    do {
      amqp_queue_bind (priv->amqp_conn, priv->channel, amqp_queue,
          amqp_cstring_bytes (exchange_name),
          amqp_cstring_bytes ((gchar *) bindingkey->data), amqp_empty_table);
      bindingkey = bindingkey->next;
      g_debug ("binding a queue(%s) (exchange: %s, bind_key: %s)", queue_name,
          exchange_name, (gchar *) bindingkey->data);
    } while (bindingkey);
  }

  amqp_basic_consume (priv->amqp_conn, priv->channel, amqp_queue,
      amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  g_return_val_if_fail ((amqp_get_rpc_reply (priv->amqp_conn)).reply_type ==
      AMQP_RESPONSE_NORMAL, CHAMGE_RETURN_FAIL);

  priv->state = CHAMGE_AMQP_STATUS_SUBSCRIBING;
  return CHAMGE_RETURN_OK;
}

ChamgeReturn
chamge_amqp_rpc_listen (ChamgeAmqpRpc * self, gchar ** const body,
    gchar ** const reply_to, guint * const correlation_id)
{
  ChamgeAmqpRpcClass *klass = CHAMGE_AMQP_RPC_GET_CLASS (self);
  ChamgeAmqpRpcPrivate *priv = chamge_amqp_rpc_get_instance_private (self);

  amqp_rpc_reply_t amqp_r;
  amqp_envelope_t envelope;

  struct timeval timeout = { 1, 0 };

  g_return_val_if_fail (priv->state == CHAMGE_AMQP_STATUS_SUBSCRIBING,
      CHAMGE_RETURN_FAIL);

  amqp_maybe_release_buffers (priv->amqp_conn);

  amqp_r = amqp_consume_message (priv->amqp_conn, &envelope, &timeout, 0);

  if (amqp_r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION
      && amqp_r.library_error == AMQP_STATUS_TIMEOUT) {
    return CHAMGE_RETURN_FAIL;
  }

  if (amqp_r.reply_type != AMQP_RESPONSE_NORMAL) {
    g_debug ("abnormal response: %x", amqp_r.reply_type);
    goto out;
  }

  if (envelope.message.body.bytes == NULL) {
    g_debug ("no reply queue in request message");
    goto out;
  }

  g_debug ("Delivery %u, exchange %.*s routingkey %.*s",
      (unsigned) envelope.delivery_tag, (int) envelope.exchange.len,
      (char *) envelope.exchange.bytes, (int) envelope.routing_key.len,
      (char *) envelope.routing_key.bytes);
  g_debug ("    correlation id : %s, content type : %s",
      (gchar *) envelope.message.properties.correlation_id.bytes,
      (gchar *) envelope.message.properties.content_type.bytes);

  /* if the content-type isn't 'application/json', let's drop */
  if (!(envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG)
      || strlen ("application/json") !=
      envelope.message.properties.content_type.len
      || g_ascii_strncasecmp ("application/json",
          envelope.message.properties.content_type.bytes,
          envelope.message.properties.content_type.len)
      ) {
    g_debug ("invalid content type %s",
        (gchar *) envelope.message.properties.content_type.bytes);

    goto out;
  }

  if (envelope.message.properties.reply_to.len > 0 &&
      (strlen (envelope.message.properties.reply_to.bytes) >=
          envelope.message.properties.reply_to.len)) {
    g_free (*reply_to);
    *reply_to = g_strndup (envelope.message.properties.reply_to.bytes,
        envelope.message.properties.reply_to.len);
    g_debug ("replay to == %s", *reply_to);
  } else {
    g_debug ("there is no replay to in property");
  }

  if (envelope.message.properties.correlation_id.len > 0 &&
      (strlen (envelope.message.properties.correlation_id.bytes) >=
          envelope.message.properties.correlation_id.len)) {
    gchar *id = g_strndup (envelope.message.properties.correlation_id.bytes,
        envelope.message.properties.correlation_id.len);
    *correlation_id = atoi (id);
    g_debug ("correlation id == %d", *correlation_id);
  } else {
    g_debug ("there is no correlation id to in property");
  }

  g_debug ("Content-type: %.*s",
      (int) envelope.message.properties.content_type.len,
      (char *) envelope.message.properties.content_type.bytes);

  if (envelope.message.body.len > 0 && envelope.message.body.bytes != NULL) {
    g_free (*body);
    *body = g_strndup (envelope.message.body.bytes, envelope.message.body.len);
  }

  priv->state = CHAMGE_AMQP_STATUS_RECEIVED;
  amqp_destroy_envelope (&envelope);
  return CHAMGE_RETURN_OK;

out:
  amqp_destroy_envelope (&envelope);
  return CHAMGE_RETURN_FAIL;
}

ChamgeReturn
chamge_amqp_rpc_respond (ChamgeAmqpRpc * self, const gchar * response,
    const gchar * reply_to, guint correlation_id)
{
  ChamgeAmqpRpcClass *klass = CHAMGE_AMQP_RPC_GET_CLASS (self);
  ChamgeAmqpRpcPrivate *priv = chamge_amqp_rpc_get_instance_private (self);

  amqp_basic_properties_t amqp_props;

  g_return_val_if_fail (priv->state == CHAMGE_AMQP_STATUS_RECEIVED,
      CHAMGE_RETURN_FAIL);

  if (_amqp_property_set (&amqp_props, correlation_id,
          NULL) != CHAMGE_RETURN_OK) {
    return CHAMGE_RETURN_FAIL;
  }
  /*
   * publish
   */
  g_debug ("publishing to [%s]", reply_to);
  g_debug ("      correlation id : %d body : \"%s\"", correlation_id, response);
  amqp_basic_publish (priv->amqp_conn, 1, amqp_cstring_bytes (""),
      amqp_cstring_bytes (reply_to), 0, 0, &amqp_props,
      amqp_cstring_bytes (response));

  _amqp_property_free (&amqp_props);
  priv->state = CHAMGE_AMQP_STATUS_SUBSCRIBING;
  return CHAMGE_RETURN_OK;
}
