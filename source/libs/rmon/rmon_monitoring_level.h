#ifndef __RMON_MONITORING_LEVEL_H
#define __RMON_MONITORING_LEVEL_H
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

#ifndef WIN32NATIVE
#	include <sys/types.h>
#else /* WIN32NATIVE */
#	include "win32nativetypes.h"
#endif /* WIN32NATIVE */



#ifdef  __cplusplus
extern "C" {
#endif

/* different layers for monitoring */
#define N_LAYER   8

#define TOP_LAYER      0 /* t */
#define CULL_LAYER     1 /* c */
#define BASIS_LAYER    2 /* b */
#define GUI_LAYER      3 /* g */
#define QIDL_LAYER     4 /* q */
#define COMMD_LAYER    5 /* h */
#define GDI_LAYER      6 /* a */
#define PACK_LAYER     7 /* p */

/* different classes of monitoring messages */
/* different classes of monitoring messages */
#ifndef WIN32NATIVE
#	define TRACE            1 /* t */
#else 
#	define TRACEPRINT       1 /* t , since TRACE is used for MFC Debugging */
#endif 

#define INFOPRINT        2 /* i */
#define JOBTRACE         4 /* j */
#define SPECIAL	       8 /* s */
#define TIMING         16 /* m */

#define FREE_CLASS_X	   32 /* X */
#define FREE_CLASS_Y	   64 /* Y */
#define FREE_CLASS_Z   128 /* Z */

#define NO_LEVEL  256  

#define ALL_CLASSES (TRACE|INFOPRINT|JOBTRACE|SPECIAL|TIMING|FREE_CLASS_X|FREE_CLASS_Y|FREE_CLASS_Z)

typedef struct _monitoring_level {
   u_long ml[N_LAYER];
} monitoring_level;

#ifndef WIN32NATIVE

int rmon_mlcmp(monitoring_level *, monitoring_level *);
int rmon_mliszero(monitoring_level *);

void rmon_mlcpy(monitoring_level *, monitoring_level *);
void rmon_mlclr(monitoring_level *);

void rmon_mlor(monitoring_level *, monitoring_level *);
int rmon_mland(monitoring_level *, monitoring_level *);

void rmon_mlset(monitoring_level *, u_long to_set);

u_long rmon_mlgetl(monitoring_level *, int);
void rmon_mlputl(monitoring_level *, int, u_long);

int rmon_mlnlayer(void);
void rmon_mlprint(monitoring_level *);

#endif

#ifdef  __cplusplus
}
#endif

#endif



