/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#ifndef __CHAMGE_ARBITER_H__
#define __CHAMGE_ARBITER_H__

#if !defined(__CHAMGE_INSIDE__) && !defined(CHAMGE_COMPILATION)
#error "Only <chamge/chamge.h> can be included directly."
#endif

#include <chamge/node.h>

G_BEGIN_DECLS

#define CHAMGE_TYPE_ARBITER     (chamge_arbiter_get_type ())
CHAMGE_API_EXPORT
G_DECLARE_DERIVABLE_TYPE        (ChamgeArbiter, chamge_arbiter, CHAMGE, ARBITER, ChamgeNode)

struct _ChamgeArbiterClass
{
  ChamgeNodeClass parent_class;
  
  /* methods */
  void  (* approve)                     (ChamgeArbiter *self,
                                         const gchar   *uid);

  /* signals */
  void  (* enrolled)                    (ChamgeArbiter *self,
                                         const gchar   *uid);
  void  (* delisted)                    (ChamgeArbiter *self,
                                         const gchar   *uid);
};

CHAMGE_API_EXPORT
ChamgeArbiter*  chamge_arbiter_new                      (const gchar   *uid);  

CHAMGE_API_EXPORT
ChamgeArbiter*  chamge_arbiter_new_full                 (const gchar   *uid,
                                                         ChamgeBackend  bakend);

G_END_DECLS

#endif //__CHAMGE_ARBITER_H__

