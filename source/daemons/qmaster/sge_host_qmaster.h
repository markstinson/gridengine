#ifndef __SGE_HOST_QMASTER_H
#define __SGE_HOST_QMASTER_H
/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/



#include "sge_gdi_intern.h"
#include "sge_c_gdi.h"
#include "sge_feature.h"

/* funtions called via gdi and inside the qmaster */
int sge_del_host(lListElem *, lList **, char *, char *, u_long32);

int host_spool(lList **alpp, lListElem *ep, gdi_object_t *object);

int host_mod(lList **alpp, lListElem *new_host, lListElem *ep, int add, char *ruser, char *rhost, gdi_object_t *object, int sub_command);

int host_success(lListElem *ep, lListElem *old_ep, gdi_object_t *object);

void sge_mark_unheard(lListElem *hep, char *target);

int sge_add_host_of_type(char *, u_long32);

void sge_gdi_kill_exechost(char *host, sge_gdi_request *request, sge_gdi_request *answer);

void sge_update_load_values(char *rhost, lList *lp);

void sge_load_value_garbage_collector(u_long32 now);

int sge_count_uniq_hosts(lList *ahl, lList *shl);

int sge_execd_startedup(lListElem *hep, lList **alpp, char *ruser, char *rhost, u_long32 target, int qstd_mode);

u_long32 load_report_interval(lListElem *hep); 

void sge_change_queue_version_exechost(char *exechost_name);

lListElem *get_local_conf_val(char *host, char *name);  

int notify_new_features(lListElem *host, enum featureset_id_t featureset, char *target);

void master_notify_execds(void);

#endif /* __SGE_HOST_QMASTER_H */

