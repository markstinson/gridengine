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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

#include "sgermon.h"
#include "sge_pids.h"
#include "sge_peopen.h"

/*-----------------------------------------------------------------------
 * Name:    get_pids
 * Descr:   return all "pids" of a running processes with given "name"
 *          only first 8 characters of "name" are significant
 *          check only basename of command after "/" 
 *          
 *
 * Return:  -1 if yes
 *          0 no program with given name found
 *          >0 number of processes with "name"
 *          "pids" array holds pid of found processes
 *-----------------------------------------------------------------------*/
int get_pids(
pid_t *pids,
int max_pids,
char *name,
char *pscommand 
) {
   FILE *fp_in, *fp_out, *fp_err;
   char buf[10000], *ptr;
   int num_of_pids = 0, last, len;
   pid_t pid, command_pid;

   DENTER(TOP_LAYER, "get_pids");
   
   command_pid = peopen("/bin/sh", 0, pscommand, NULL, NULL, &fp_in, &fp_out, &fp_err);

   if (command_pid == -1) {
      DEXIT;
      return -1;
   }

   while (!feof(fp_out)) {
      if ((fgets(buf, sizeof(buf), fp_out))) {
         if ((len = strlen(buf))) {

            /* handles first line of ps command */
            if ((pid = (pid_t) atoi(buf)) <= 0)
               continue;

            /* strip off trailing white spaces */
            last = len - 1;
            while (last >= 0 && isspace((int) buf[last])) {
               buf[last] = '\0';
               last--;
            }
            
            /* set pointer to first character of process name */
            while (last >= 0 && !isspace((int) buf[last]))
               last--;
            last++;

            /* DPRINTF(("pid: %d - progname: >%s<\n", pid, &buf[last])); */
            
            /* get basename of program */
            ptr = strrchr(&buf[last], '/');
            if (ptr)
               ptr++;
            else
               ptr = &buf[last];                  
   
            /* check if process has given name */
            if (!strncmp(ptr, name, 8))
               pids[num_of_pids++] = pid;
         }
      }
   }            

   peclose(command_pid, fp_in, fp_out, fp_err, NULL);
   return num_of_pids;
}


/*--------------------------------------------------------------------
 * Name: contains_pid
 *       whether pid array contains pid
 *
 * Return: 1 if pid is found in array of pids
 *         0 if pid is not found
 *
 *--------------------------------------------------------------------*/
int contains_pid(
pid_t pid,
pid_t *pids,
int npids 
) {
   int i;

   for (i = 0; i < npids; i++)
      if (pids[i] == pid)
         return 1;

   return 0;
}


/*-----------------------------------------------------------------------
 * Name:    checkprog
 * Descr:   check if "pid" of a running process has given "name"
 *          only first 8 characters of "name" are significant
 *          check only basename of command after "/"
 *
 * Return:  0 if yes
 *          1 if not (no such pid or pid has other name)
 *          -1 error occurred (mostly peopen() failed)
 *-----------------------------------------------------------------------*/
int checkprog(
pid_t pid,
char *name,
char *pscommand 
) {
   FILE *fp_in, *fp_out, *fp_err;
   char buf[1000], *ptr;
   pid_t command_pid, pidfound;
   int len, last, notfound;
#if defined(QIDL) && defined(SOLARIS64)
   sigset_t sigset, osigset;
#endif

   DENTER(TOP_LAYER, "checkprog");

#if defined(QIDL) && defined(SOLARIS64)
   {
      sigemptyset(&sigset);
      sigaddset(&sigset, SIGCLD);
      sigprocmask(SIG_BLOCK, &sigset, &osigset);
   }
#endif

   command_pid = peopen("/bin/sh", 0, pscommand, NULL, NULL, &fp_in, &fp_out, &fp_err);

   if (command_pid == -1) {
      DEXIT;
#if defined(QIDL) && defined(SOLARIS64) 
      sigprocmask(SIG_SETMASK, &osigset, NULL);
#endif  
      return -1;
   }

   notfound = 1;
   while (!feof(fp_out)) {
      if ((fgets(buf, sizeof(buf), fp_out))) {
         if ((len = strlen(buf))) {
            pidfound = (pid_t) atoi(buf);

            if (pidfound == pid) {
               last = len - 1;
               DPRINTF(("last pos in line: %d\n", last));
               while (last >= 0 && isspace((int) buf[last])) {
                  buf[last] = '\0';
                  last--;
               }

               /* DPRINTF(("last pos in line now: %d\n", last)); */
               
               while (last >= 0 && !isspace((int) buf[last]))
                  last--;
               last++;

               /* DPRINTF(("pid: %d - progname: >%s<\n", pid, &buf[last])); */ 

               /* get basename of program */
               ptr = strrchr(&buf[last], '/');
	       if (ptr)
	          ptr++;
	       else
	          ptr = &buf[last];

               if (!strncmp(ptr, name, 8)) {
                  notfound = 0;
                  break;
               }
               else
                  break;
            }
         }
      }
   }

   peclose(command_pid, fp_in, fp_out, fp_err, NULL);

#if defined(QIDL) && defined(SOLARIS64) 
   sigprocmask(SIG_SETMASK, &osigset, NULL);
#endif  
   
   DEXIT;
   return notfound;
}
