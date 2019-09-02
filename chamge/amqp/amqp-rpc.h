/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Heekyoung Seo <hkseo@sk.com>
 *
 */

#ifndef __CHAMGE_AMQP_RPC_H__
#define __CHAMGE_AMQP_RPC_H__

#include <gio/gio.h>
#include <chamge/types.h>

G_BEGIN_DECLS
#define CHAMGE_TYPE_AMQP_RPC       (chamge_amqp_rpc_get_type ())
    CHAMGE_API_EXPORT
G_DECLARE_DERIVABLE_TYPE (ChamgeAmqpRpc, chamge_amqp_rpc, CHAMGE, AMQP_RPC,
    GObject)
     struct _ChamgeAmqpRpcClass
     {
       GObjectClass parent_class;

     };




     CHAMGE_API_EXPORT ChamgeAmqpRpc *chamge_amqp_rpc_new ();

CHAMGE_API_EXPORT
    ChamgeReturn chamge_amqp_rpc_set_url (ChamgeAmqpRpc * self,
    const gchar * url);

     CHAMGE_API_EXPORT ChamgeReturn
     chamge_amqp_rpc_login (ChamgeAmqpRpc * self);

/* for client */
CHAMGE_API_EXPORT
    gchar * chamge_amqp_rpc_request (ChamgeAmqpRpc * self,
    const gchar * request, const gchar * exchange, const gchar * queue_name);

/* for server */
CHAMGE_API_EXPORT
    ChamgeReturn chamge_amqp_rpc_subscribe (ChamgeAmqpRpc * self,
    const gchar * exchange_name, const gchar * queue_name);

/* for server */
CHAMGE_API_EXPORT
    ChamgeReturn chamge_amqp_rpc_listen (ChamgeAmqpRpc * self,
    gchar ** const body, gchar ** const reply_to, guint * const correlation_id);

/* for server */
CHAMGE_API_EXPORT
    ChamgeReturn chamge_amqp_rpc_respond (ChamgeAmqpRpc * self,
    const gchar * response, const gchar * reply_to, guint correlation_id);


G_END_DECLS
#endif // __CHAMGE_AMQP_H__
