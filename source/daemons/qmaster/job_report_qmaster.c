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
#include <string.h>

#include "sgermon.h"
#include "sge_log.h"
#include "sge.h"
#include "def.h"
#include "sge_peL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_hostL.h"
#include "sge_usageL.h"
#include "sge_eventL.h"
#include "sge_reportL.h"
#include "sge_job_reportL.h"
#include "sge_sched.h"
#include "sge_prognames.h"
#include "execution_states.h"
#include "sge_feature.h"
#include "job_report.h"
#include "job_report_qmaster.h"
#include "job_exit.h"
#include "sge_signal.h"
#include "sge_m_event.h"
#include "sge_job.h"
#include "sge_host.h"
#include "sge_give_jobs.h"
#include "sge_pe_qmaster.h"
#include "read_write_job.h"
#include "sge_me.h"
#include "sge_time.h"
#include "job.h"
#include "time_event.h"
#include "reschedule.h"
#include "msg_daemons_common.h"
#include "msg_qmaster.h"
#include "sge_string.h"

extern lList *Master_Job_List;
extern lList *Master_Exechost_List;

static void pack_job_exit(sge_pack_buffer *pb, u_long32 jobid, u_long32 jataskid, char *task_str);

#define is_running(state) (state==JWRITTEN || state==JRUNNING|| state==JWAITING4OSJID)


static void pack_job_exit(
sge_pack_buffer *pb,
u_long32 jobid,
u_long32 jataskid,
char *task_str 
) {
   packint(pb, ACK_JOB_EXIT);
   packint(pb, jobid);
   packint(pb, jataskid);
   packstr(pb, task_str);
}

void pack_job_kill(
sge_pack_buffer *pb,
u_long32 jobid,
u_long32 jataskid 
) {
   packint(pb, ACK_SIGNAL_JOB);
   packint(pb, jobid);
   packint(pb, jataskid);
   packint(pb, SGE_SIGKILL);
}

static char *status2str(u_long32 status);
static char *status2str(
u_long32 status 
) {
   char *s;

   switch (status) {
   case JTRANSITING:
      s = "JTRANSITING";
      break;
   case JRUNNING:
      s = "JRUNNING";
      break;
   case JFINISHED:
      s = "JFINISHED";
      break;
   case JIDLE:
      s = "JIDLE";
      break;
   default:
      s = "<unknown>";
      break;
   }

   return s;
}
/* ----------------------------------------

NAME 
   process_job_report

DESCR
   Process 'report' containing a job report list from 
   'commproc' at 'rhost'.

   The 'pb' may get used to collect requests that will be 
   generated in this process. The caller should reply it
   to the sender of this job report list if 'pb' remains
   not empty.

RETURN
   void  because all necessary state changings are done 
         in the apropriate objects

   ---------------------------------------- */
void process_job_report(
lListElem *report, 
lListElem *hep,
char *rhost,
char *commproc,
sge_pack_buffer *pb  
) {
   lList* jrl = NULL; /* JR_Type */
   lListElem *jep, *jr, *ep, *jatep = NULL; 
   u_long32 jobid, rstate = 0, jataskid = 0;
   char *s;

   DENTER(TOP_LAYER, "process_job_report");

   lXchgList(report, REP_list, &jrl);

   DPRINTF(("received job report with %d elements:\n", lGetNumberOfElem(jrl)));

   /* 
   ** first process job reports of sub tasks to ensure this we put all these 
   ** job reports to the top of the 'jrl' list this is necessary to ensure 
   ** slave tasks get accounted on a shm machine 
   */
   {
      static lSortOrder *jr_sort_order = NULL;
      if (!jr_sort_order) {
         DPRINTF(("parsing job report sort order\n"));
         jr_sort_order = lParseSortOrderVarArg(JR_Type, "%I-", 
            JR_pe_task_id_str);
      }
      lSortList(jrl, jr_sort_order);
   }

   /* RU: */
   /* tag all reschedule_unknown list entries we hope to 
      hear about in that job report */
   update_reschedule_unknown_list(hep);

   /*
   ** now check all job reports job reports found in step 1 are 
   ** removed from job report list
   */
   for_each(jr, jrl) {
      char *queue_name, *pe_task_id_str;
      u_long32 status = 0;
      lListElem *task = NULL;
      lListElem *task_task = NULL;
      int fret;

      jobid = lGetUlong(jr, JR_job_number);
      jataskid = lGetUlong(jr, JR_ja_task_number);
      rstate = lGetUlong(jr, JR_state);

      /* handle protocol to execd for all jobs which are
         already finished and maybe rescheduled */
      /* RU: */
      fret = skip_restarted_job(hep, jr, jobid, jataskid);
      if (fret > 0) {
         if (fret == 2) {
            pack_job_kill(pb, jobid, jataskid);
         } else if (fret == 3) {
            pack_job_exit(pb, jobid, jataskid, 
               lGetString(jr, JR_pe_task_id_str)?
               lGetString(jr, JR_pe_task_id_str):"");
         }
         continue;
      }

      /* seach job/jatask */
      for_each (jep, Master_Job_List) {
         if (lGetUlong(jep, JB_job_number) == jobid) {
            int Break = 0;

            for_each (jatep, lGetList(jep, JB_ja_tasks)) {
               if (lGetUlong(jatep, JAT_task_number) == jataskid) {
                  Break = 1;
                  break;
               }
            }
            if (Break)  
               break;
         }
      }
      if (jep && jatep)
         status = lGetUlong(jatep, JAT_status);

      queue_name = (s=lGetString(jr, JR_queue_name))?s:(char*)MSG_OBJ_UNKNOWNQ;
      if ((pe_task_id_str = lGetString(jr, JR_pe_task_id_str)) && jep && jatep)
         task = lGetSubStr(jatep, JB_pe_task_id_str, pe_task_id_str, 
                           JAT_task_list); 
      switch(rstate) {
      case JWRITTEN:
      case JRUNNING:   
      case JWAITING4OSJID:
         if (jep && jatep) {
            switch (status) {
            case JTRANSITING:
            case JRUNNING:   
               if (!pe_task_id_str) {
                  /* store unscaled usage directly in job */
                  lXchgList(jr, JR_usage, lGetListRef(jatep, JAT_usage_list));

                  /* update jobs scaled usage list */

                  lSetList(jatep, JAT_scaled_usage_list, 
                      lCopyList("scaled", lGetList(jatep, JAT_usage_list)));
                  scale_usage(jep, jatep, lGetList(hep, EH_usage_scaling_list), 
                        lGetList(jatep, JAT_previous_usage_list));
                 
                  if (status==JTRANSITING) { /* got async ack for this job */ 
                     DPRINTF(("--- transisting job "u32" is running\n", jobid));
                     sge_commit_job(jep, jatep, 1, 1); /* implicitly sending usage to schedd in sge_mode */
                     cancel_job_resend(jobid, jataskid);
                  } else if (feature_is_enabled(FEATURE_SGEEE)) /* need to generate a job event for new usage */
                        sge_add_list_event(sgeE_JOB_USAGE, jobid, jataskid, NULL, lGetList(jatep, JAT_scaled_usage_list));
               } else {
                  /* register running task qmaster will log accounting for all registered tasks */
                  lListElem *pe;
                  int new_task = 0;
                  if (lGetString(jatep, JAT_granted_pe)
                        && (pe=sge_locate_pe(lGetString(jatep, JAT_granted_pe)))
                        && lGetUlong(pe, PE_control_slaves)
                        && lGetElemHost(lGetList(jatep, JAT_granted_destin_identifier_list), JG_qhostname, rhost)) {

                    if (!task) {
                        lList* task_tasks;
                        lListElem *task_task;

                        /* here qmaster hears the first time about this task
                           and thus adds it to the task list of the appropriate job */
                        new_task = 1;
                        DPRINTF(("--- task (#%d) "u32"/%s -> running\n", 
                           lGetNumberOfElem(lGetList(jatep, JAT_task_list)), jobid, pe_task_id_str));
                        task = lAddSubStr(jatep, JB_pe_task_id_str, pe_task_id_str, JAT_task_list, JB_Type);
                        task_tasks = lCreateList("", JAT_Type);
                        task_task = lCreateElem(JAT_Type);
                        lAppendElem(task_tasks, task_task);
                        lSetList(task, JB_ja_tasks, task_tasks);      
                        lSetUlong(task_task, JAT_status, JRUNNING);
                        lSetList(task_task, JAT_granted_destin_identifier_list, NULL);
                        if ((ep=lAddSubStr(task_task, JG_qhostname, rhost, JAT_granted_destin_identifier_list, JG_Type)))
                           lSetString(ep, JG_qname, queue_name);
                        cull_write_job_to_disk(jep);
                    }

                    /* store unscaled usage directly in sub-task */
                    lXchgList(jr, JR_usage, lGetListRef(lFirst(lGetList(task, JB_ja_tasks)), JAT_usage_list));

                    /* update task's scaled usage list */
                    lSetList(lFirst(lGetList(task, JB_ja_tasks)), JAT_scaled_usage_list,
                             lCopyList("scaled", lGetList(lFirst(lGetList(task, JB_ja_tasks)), JAT_usage_list)));
                    scale_usage(task, lFirst(lGetList(task, JB_ja_tasks)),
                           lGetList(hep, EH_usage_scaling_list), 
                           lGetList(lFirst(lGetList(task, JB_ja_tasks)), JAT_previous_usage_list));

                              /* notify scheduler of task usage event */
                    if (feature_is_enabled(FEATURE_SGEEE)) {
                         if (new_task)
                             sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);
                         else
                             sge_add_list_event(sgeE_JOB_USAGE, jobid, jataskid, pe_task_id_str,
                                                lGetList(lFirst(lGetList(task, JB_ja_tasks)), JAT_scaled_usage_list));
                    }

                  } else {
                     lListElem *jg;
                     const char *shouldbe_queue_name;
                     const char *shouldbe_host_name;
                     
                     if (lGetUlong(jatep, JAT_status) != JFINISHED) {
                        if (!(jg = lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)))) {
                           shouldbe_queue_name = "<not running>";
                           shouldbe_host_name = "<not running>";
                        }
                        else {
                           shouldbe_queue_name = (s=lGetString(jg, JG_qname))?s: MSG_OBJ_UNKNOWN;
                           shouldbe_host_name = (s=lGetString(jg, JG_qhostname))?s: MSG_OBJ_UNKNOWN;
                        }
                        /* should never happen */
                        ERROR((SGE_EVENT, MSG_JOB_REPORTEXITQ_SUUSSSSS, 
                               rhost, u32c(jobid), u32c(jataskid), 
                               pe_task_id_str?pe_task_id_str:MSG_MASTER, 
                               queue_name, shouldbe_queue_name, 
                               shouldbe_host_name, 
                               status2str(lGetUlong(jatep, JAT_status))));
                        }
                  }
               }
               break;
            default:
               ERROR((SGE_EVENT, MSG_JOB_REPORTRUNQ_SUUSSU, 
                     rhost, u32c(jobid), u32c(jataskid), 
                     pe_task_id_str?pe_task_id_str:"master", 
                     queue_name, u32c(status)));
               break;
            } 
         } else {
            /* execd reports a running job that is unknown */
            /* signal this job to kill it at execd 
               this can be caused by a qdel -f while 
               execd was unreachable or by deletion of 
               the job in qmasters spool dir + qmaster 
               restart  
               retry is triggered if execd reports
               this job again as running
            */
            ERROR((SGE_EVENT, MSG_JOB_REPORTRUNFALSE_SUUSS, rhost, 
                   u32c(jobid), u32c(jataskid), 
                   pe_task_id_str?pe_task_id_str:MSG_MASTER, queue_name));
            pack_job_kill(pb, jobid, jataskid);
         }
         break;
         
      case JSLAVE:
         if (!jep) {
            DPRINTF(("send cleanup request for slave job "u32"."u32"\n", 
               jobid, jataskid));
            pack_job_exit(pb, jobid, jataskid, pe_task_id_str);
         } else {
            /* must be ack for slave job */
            lListElem *first_at_host, *gdil_ep;

            first_at_host = lGetElemHost(lGetList(jatep, 
               JAT_granted_destin_identifier_list), JG_qhostname, rhost);
            if (first_at_host) {
               if (lGetUlong(first_at_host, JG_tag_slave_job)!=0) {
                  int all_slaves_arrived = 1;
                  DPRINTF(("slave job "u32" arrived at %s\n", jobid, rhost));
                  lSetUlong(first_at_host, JG_tag_slave_job, 0);

                  /* should trigger a fast delivery of the job to master execd 
                     script but only when all other slaves have also arrived */ 
                  for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) 
                     if (lGetUlong(gdil_ep, JG_tag_slave_job)!=0)
                        all_slaves_arrived = 0;

                  if (all_slaves_arrived) {
                     /* triggers direct job delivery to master execd */
                     DPRINTF(("trigger retry of job delivery to master execd\n"));
                     lSetUlong(jatep, JAT_start_time, 0);
                     cancel_job_resend(jobid, jataskid);
                     trigger_job_resend(sge_get_gmt(), NULL, jobid, jataskid);
                  }
               }
            } else {
               {
                  /* clear state with regards to slave controlled container */
                  lListElem *host;

                  host = lGetElemHost(Master_Exechost_List, EH_name, rhost);
                  update_reschedule_unknown_list_for_job(host, jobid, jataskid);

                  DPRINTF(("RU: CLEANUP FOR SLAVE JOB "u32"."u32" on host "SFN"\n", 
                     jobid, jataskid, rhost));
               }

               /* clean up */
               pack_job_exit(pb, jobid, jataskid, pe_task_id_str);
            }
         }
         break;
      case JEXITING:   
      {
         int skip_job_exit = 0;

         if (!jep || !jatep || (jep && status==JFINISHED)) {
            /* must be retry of execds job exit */ 
            /* or job was deleted using "qdel -f" */
            /* while execd was down or .. */
            if (!jatep)
               DPRINTF(("exiting job "u32" does not exist\n", jobid));
            else
               DPRINTF(("exiting job "u32"."u32" does not exist\n", jobid, jataskid));
         } else {
            /* job exited */  
            if (!pe_task_id_str) {

               /* store unscaled usage directly in job */
               lXchgList(jr, JR_usage, lGetListRef(jatep, JAT_usage_list));

               /* update jobs scaled usage list */
               lSetList(jatep, JAT_scaled_usage_list, 
                  lCopyList("scaled", lGetList(jatep, JAT_usage_list)));
               scale_usage(jep, jatep, lGetList(hep, EH_usage_scaling_list),
                        lGetList(jatep, JAT_previous_usage_list));
               /* skip sge_job_exit() and pack_job_exit() in case there 
                  are still running tasks, since execd resends job exit */
               for_each (task, lGetList(jatep, JAT_task_list)) {
                  if (lGetUlong(lFirst(lGetList(task, JB_ja_tasks)), JAT_status)==JRUNNING) {
                     DPRINTF(("job exit for job "u32": still waiting for task %s\n", 
                        jobid, lGetString(task, JB_pe_task_id_str)));
                     skip_job_exit = 1;
                  }
               }

               switch (status) {
               case JRUNNING:
               case JTRANSITING:
                  if (!skip_job_exit) {
                     DPRINTF(("--- running job "u32"."u32" is exiting\n", 
                        jobid, jataskid, (status==JTRANSITING)?"transisting":"running"));

                     sge_job_exit(jr, jep, jatep);
                  }
                  break;
               case JFINISHED:
                  /* must be retry */
                  skip_job_exit = 1;
                  break;
               default:
                  ERROR((SGE_EVENT, MSG_JOB_REPORTEXITJ_UUU,
                        u32c(jobid), u32c(jataskid), u32c(status)));
                  break;
               }
            } else {
               lListElem *pe;
               if ( lGetString(jatep, JAT_granted_pe)
                  && (pe=sge_locate_pe(lGetString(jatep, JAT_granted_pe)))
                  && lGetUlong(pe, PE_control_slaves)
                  && lGetElemHost(lGetList(jatep, JAT_granted_destin_identifier_list), 
                           JG_qhostname, rhost)) {
                  /* here we get usage of tasks that ran on slave/master execd's 
                     we store a job element for each job in the task list of the job
                     this is needed to prevent multiple debitation of one task 
                     -- need a state in qmaster for each task */

                  if (!task) {
   
                     task = lAddSubStr(jatep, JB_pe_task_id_str, 
                           pe_task_id_str, JAT_task_list, JB_Type);
                     task_task = lAddSubUlong(task, JAT_status, 
                           JRUNNING, JB_ja_tasks, JAT_Type); 
                  }

                  /* store unscaled usage directly in sub-task */
                  /* lXchgList(jr, JR_usage, lGetListRef(task, JB_usage_list)); */
                  /* copy list because we need to keep usage in jr for sge_log_dusage() */
         		  lSetList(lFirst(lGetList(task, JB_ja_tasks)), JAT_usage_list, 
                     lCopyList(NULL, lGetList(jr, JR_usage)));

                  /* update task's scaled usage list */
                  lSetList(lFirst(lGetList(task, JB_ja_tasks)), JAT_scaled_usage_list,
                           lCopyList("scaled", lGetList(lFirst(lGetList(task, JB_ja_tasks)), 
                           JAT_usage_list)));
                  scale_usage(lFirst(lGetList(task, JB_ja_tasks)), task_task, 
                           lGetList(hep, EH_usage_scaling_list), 
                           lGetList(lFirst(lGetList(task, JB_ja_tasks)), JAT_previous_usage_list));


                  if (lGetUlong(lFirst(lGetList(task, JB_ja_tasks)), JAT_status)==JRUNNING ||
                     lGetUlong(lFirst(lGetList(task, JB_ja_tasks)), JAT_status)==JTRANSITING) {
                     u_long32 failed;
                     char *err_str, failed_msg[256];

                     failed = lGetUlong(jr, JR_failed);

                     DPRINTF(("--- task (#%d) "u32"/%s -> final usage\n", 
                        lGetElemIndex(lFirst(lGetList(task, JB_ja_tasks)), 
                        lGetList(jatep, JAT_task_list)), jobid, pe_task_id_str));
                     lSetUlong(lFirst(lGetList(task, JB_ja_tasks)), JAT_status, JFINISHED);
                     err_str = lGetString(jr, JR_err_str);
                     sprintf(failed_msg, u32" %s %s", failed, err_str?":":"", err_str?err_str:"");
                     lSetString(task, JB_sge_o_mail, failed_msg);
                     sge_log_dusage(jr, jep, jatep);
                     cull_write_job_to_disk(jep);


                     /* get rid of this job in case a task died from XCPU/XFSZ or 
                        exited with a core dump */
                     if (failed==SSTATE_FAILURE_AFTER_JOB
                           && (ep=lGetElemStr(lGetList(jr, JR_usage), UA_name, "signal"))) {
                        u_long32 sge_signo;
                        sge_signo = (int)lGetDouble(ep, UA_value);
                        switch (sge_signo) {
                        case SGE_SIGXFSZ:
                           INFO((SGE_EVENT, MSG_JOB_FILESIZEEXCEED_SSUU, 
                                pe_task_id_str, rhost, u32c(jobid), u32c(jataskid)));
                           break;
                        case SGE_SIGXCPU:
                           INFO((SGE_EVENT, MSG_JOB_CPULIMEXCEED_SSUU, 
                                 pe_task_id_str, rhost, u32c(jobid), u32c(jataskid)));
                           break;
                        default: 
                           INFO((SGE_EVENT, MSG_JOB_DIEDTHROUGHSIG_SSUUS, 
                                pe_task_id_str, rhost, u32c(jobid), u32c(jataskid), sge_sig2str(sge_signo)));
                           break;
                        }   
                     } else {
                        if (failed==0) {
                           INFO((SGE_EVENT, MSG_JOB_TASKFINISHED_SSUU, 
                              pe_task_id_str, rhost, u32c(jobid), u32c(jataskid)));
                        } else {
                           INFO((SGE_EVENT, MSG_JOB_TASKFAILED_SSUUU,
                              pe_task_id_str, rhost, u32c(jobid), u32c(jataskid), u32c(failed)));
                        }
                     }
                     if (failed == SSTATE_FAILURE_AFTER_JOB && 
                           !lGetString(jep, JB_checkpoint_object)) {
                        get_rid_of_job(NULL, jep, jatep, 0, pb, rhost, me.user_name, 
                              me.qualified_hostname, lGetString(jr, JR_err_str), commproc);
                        pack_job_kill(pb, jobid, jataskid);
                        ERROR((SGE_EVENT, MSG_JOB_JOBTASKFAILED_SU, pe_task_id_str, u32c(jobid)));
                     }
                  }

                  /* notify scheduler of task state change */
                  if (feature_is_enabled(FEATURE_SGEEE))
                     sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);

               } else {
                  lListElem *jg;
                  const char *shouldbe_queue_name;
                  const char *shouldbe_host_name;
                  
                  if (lGetUlong(jatep, JAT_status) != JFINISHED) {
                     if (!(jg = lFirst(lGetList(jatep, 
                         JAT_granted_destin_identifier_list)))) {
                        shouldbe_queue_name = MSG_OBJ_NOTRUNNING;
                        shouldbe_host_name = MSG_OBJ_NOTRUNNING;
                     }
                     else {
                        shouldbe_queue_name = (s=lGetString(jg, JG_qname))?s: 
                          MSG_OBJ_UNKNOWN;
                        shouldbe_host_name = (s=lGetString(jg, JG_qhostname))?s:
                          MSG_OBJ_UNKNOWN;
                     }
                     /* should never happen */
                     ERROR((SGE_EVENT, MSG_JOB_REPORTEXITQ_SUUSSSSS, 
                           rhost, u32c(jobid), u32c(jataskid), 
                           pe_task_id_str?pe_task_id_str:MSG_MASTER, queue_name, 
                           shouldbe_queue_name, shouldbe_host_name, 
                           status2str(lGetUlong(jatep, JAT_status))));
                     }
               }
            }
         }
         /* pack ack to enable execd cleaning up */
         if (!skip_job_exit) {
            pack_job_exit(pb, jobid, jataskid, pe_task_id_str);
         }
      }
         break;
      default:
         ERROR((SGE_EVENT, MSG_EXECD_UNKNOWNJ_SUUSUS, 
                rhost, 
                u32c(jobid), 
                u32c(jataskid), 
                pe_task_id_str?pe_task_id_str:MSG_MASTER, 
                u32c(rstate), 
                queue_name));


         pack_job_exit(pb, jobid, jataskid, pe_task_id_str);
         break;
      }
   }
   
   /* RU: */
   /* delete reschedule unknown list entries we heard about */
   delete_from_reschedule_unknown_list(hep);

   lXchgList(report, REP_list, &jrl);

   /*
   ** trigger resend of master registered jobs on execd/qstd startup after 
   ** first job report
   */
   if (!strcmp(commproc, prognames[QSTD])) {
      lListElem *psep;
      for_each(psep, Master_Exechost_List) {
         if (!hostcmp(rhost, lGetString(psep, EH_real_name)) && 
             lGetUlong(psep, EH_startup)) {
            lSetUlong(psep, EH_startup, 0);
            DPRINTF(("->>Resend Trigger for host: %s\n", 
                     lGetString(psep, EH_name)));
         }
      }
   }
   else {   /* execd case */
      if (lGetUlong(hep, EH_startup)) {
         lSetUlong(hep, EH_startup, 0);
      }
   } 
   

   DEXIT;
   return;
}

