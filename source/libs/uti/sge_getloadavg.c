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
#include <errno.h>
#include <fcntl.h>  
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "sge_getloadavg.h"

#include "sge.h"
#include "sge_log.h"
#include "sgermon.h"

#if defined(SOLARIS) 
#  include <kvm.h>
#  include <nlist.h> 
#  include <sys/cpuvar.h>
#  if defined(SOLARIS64)
#     include <sys/loadavg.h> 
#  endif
#elif defined(LINUX)
#  include <ctype.h>
#elif defined(ALPHA4) || defined(ALPHA5)
#  include <nlist.h>
#  include <sys/table.h>
#elif defined(IRIX6)
#  include <sys/sysmp.h> 
#  include <sys/sysinfo.h> 
#elif defined(HP10) 
#  include <nlist.h> 
#  include <sys/time.h>
#  include <sys/dk.h>
#  include <sys/file.h>
#  include <errno.h>
#  include <unistd.h> 
#elif defined(HP11)
#  include <sys/param.h>
#  include <sys/pstat.h>
#elif defined(CRAY)
#  include <nlist.h>
#  include <sys/listio.h>
#  include <sys/param.h>
#  include <sys/sysmacros.h>
#  include <sys/sysent.h>
#  include <sys/var.h>
#  include <sys/buf.h>
#  include <sys/map.h>
#  include <sys/iobuf.h>
#  include <sys/stat.h>
#  include <sys/jtab.h>
#  include <sys/session.h>
#  include <sys/dir.h>
#  include <sys/ssd.h>
#  include <sys/schedv.h>
#  include <sys/signal.h>
#  include <sys/aoutdata.h>

#  define KERNEL
#  include <sys/sysinfo.h>
#  undef KERNEL

#  include <sys/iosw.h>
#  include <sys/mbuf.h>
#  include <sys/cnt.h>
#  include <sys/ddcntl.h>
#  include <sys/hpm.h>
#  include <sys/ios.h>
#  include <sys/machcons.h>
#  include <sys/pdd.h>
#  include <sys/pws.h>
#  include <sys/sema.h>
#  include <sys/semmacros.h>
#  include <sys/swap.h>
#  include <sys/epack.h>
#  include <sys/epackt.h>
#  include <sys/er90_cmdpkt.h>

#  include <sys/acct.h>
#  include <acct/dacct.h>
#  include <sys/cred.h>
#  include <sys/proc.h>
#  include <sys/user.h>         
#elif defined(NECSX4) || defined(NECSX5)
#  include <sys/rsg.h>
#  include <sys/types.h>
#  include <fcntl.h>    
#endif

#define KERNEL_TO_USER_AVG(x) ((double)x/SGE_FSCALE)

#if defined(SOLARIS) 
#  define CPUSTATES 5
#  define CPUSTATE_IOWAIT 3
#  define CPUSTATE_SWAP 4    
#  if SOLARIS64
#     define KERNEL_AVG_TYPE long
#     define KERNEL_AVG_NAME "avenrun"
#  else
#     define SGE_FSCALE FSCALE 
#     define KERNEL_AVG_TYPE long
#     define KERNEL_AVG_NAME "avenrun"
#  endif
#elif defined(LINUX)
#  define LINUX_LOAD_SOURCE "/proc/loadavg"
#  define CPUSTATES 4
#  define PROCFS "/proc" 
#elif defined(ALPHA4) || defined(ALPHA5)
#  define MP_KERNADDR 8
#  define MPKA_AVENRUN 19
#  define KERNEL_NAME_FILE "/vmunix"
#  define KERNEL_AVG_NAME "_avenrun"
#  define SGE_FSCALE 1000.0
#  define KERNEL_AVG_TYPE long
#  define CPUSTATES 4
#elif defined(IRIX6)
#  define SGE_FSCALE 1024.0
#  define KERNEL_AVG_TYPE long
#  define KERNEL_AVG_NAME "avenrun"
#  define CPUSTATES 6
#elif defined(HP10) 
#  define KERNEL_NAME_FILE "/stand/vmunix"
#  define KERNEL_AVG_NAME "avenrun"
#  define MP_KERNADDR 8
#  define MPKA_AVENRUN 19
#  define SGE_FSCALE 1.0
#  define KERNEL_AVG_TYPE double 
#  define VMUNIX "/stand/vmunix"
#  define X_CP_TIME 0 
#endif

#if defined(SOLARIS) || defined(SOLARIS64)
typedef kvm_t* kernel_fd_type;
#else 
typedef int kernel_fd_type;
#endif

static long percentages(int cnt, double *out, long *new, long *old, long *diffs);   

static double get_cpu_load(void); 

#if defined(ALPHA4) || defined(ALPHA5) || defined(HP10) || defined(HP11) || defined(SOLARIS) || defined(SOLARIS64) || defined(IRIX6) || defined(LINUX)

static int get_load_avg(double loadv[], int nelem);    

static double get_cpu_load(void);    

#endif

#if defined(LINUX)
static char* skip_token(char *p); 
#endif

#if defined(ALPHA4) || defined(ALPHA5) || defined(SOLARIS) || defined(SOLARIS64)|| defined(IRIX6) || defined(HP10)

static int sge_get_kernel_fd(kernel_fd_type *kernel_fd);

static int sge_get_kernel_address(char *name, long *address);
 
static int getkval(unsigned long offset, int *ptr, int size, char *refstr);  

#endif 

static int kernel_initialized = 0;

#if !defined(LINUX)
static kernel_fd_type kernel_fd;
#endif

#if defined(ALPHA4) || defined(ALPHA5) || defined(SOLARIS) || defined(SOLARIS64) || defined(IRIX6) || defined(HP10) 

static int sge_get_kernel_address(
char *name,
long *address 
) {
   int ret = 0;

   DENTER(TOP_LAYER, "sge_get_kernel_address");

#if defined(IRIX6)
   if (!strcmp(KERNEL_AVG_NAME, name)) {
      *address = sysmp(MP_KERNADDR, MPKA_AVENRUN); 
      ret = 1;
   } else {
      *address = 0;
      ret = 0; 
   }
#else
   {
      struct nlist kernel_nlist[2];

      kernel_nlist[0].n_name = name;
      kernel_nlist[1].n_name = NULL;
#  if defined(ALPHA4) || defined(ALPHA5) || defined(HP10) || defined(HP11)
      if (nlist(KERNEL_NAME_FILE, kernel_nlist) >= 0)
#  else
      if (kernel_initialized && (kvm_nlist(kernel_fd, kernel_nlist) >= 0)) 
#  endif
      {
         *address = kernel_nlist[0].n_value;
         ret = 1;
      } else {
         DPRINTF(("nlist(%s) failed: %s\n", name, strerror(errno)));
         *address = 0;
         ret = 0;
      }
   }
#endif
   DEXIT;
   return ret;
}    


static int sge_get_kernel_fd(
kernel_fd_type *fd 
) {
   char prefix[256] = "my_error:";

   DENTER(TOP_LAYER, "sge_get_kernel_fd");

   if (!kernel_initialized) {

#if defined(IRIX6) || defined(HP10) || defined(ALPHA4) || defined(ALPHA5) 
      kernel_fd = open("/dev/kmem", 0);
      if (kernel_fd != -1) 
#else 
      kernel_fd = kvm_open (NULL, NULL, NULL, O_RDONLY, prefix);
      if (kernel_fd != NULL) 
#endif
      {
         kernel_initialized = 1;
      } else {
         DPRINTF(("kvm_open() failed: %s\n", strerror(errno)));
         kernel_initialized = 0;
      }
   } 
   *fd = kernel_fd; 
   DEXIT;
   return kernel_initialized;
}

static int getkval(
unsigned long offset, 
int *ptr, 
int size, 
char *refstr 
) {
   kernel_fd_type kernel_fd;

#if defined(SOLARIS) || defined(SOLARIS64)
   if (sge_get_kernel_fd(&kernel_fd)
       && kvm_read (kernel_fd, offset, (char *) ptr, size) != size) {
      if (*refstr == '!') {
         return 0;
      } else {
         return -1;
      }
   }
#else
   if (sge_get_kernel_fd(&kernel_fd)) {
      if (lseek(kernel_fd, (long)offset, 0) == -1) {
         if (*refstr == '!') {
            refstr++;
         }
         return -1;
      }
      if (read(kernel_fd, (char *) ptr, size) == -1) {
         if (*refstr == '!') {
            return 0;
         } else {
            return -1;
         }
      }
   }
#endif
   return 0;
}


#if defined(SOLARIS)
int get_freemem(
long *freememp 
) {
   kvm_t *kd;

   long address_freemem;
   if (sge_get_kernel_fd(&kd) && sge_get_kernel_address("freemem", &address_freemem)) {
      getkval(address_freemem, (int *)freememp, sizeof(long), "freemem");
      return 0;
   } else
      return -1;
}
#endif


#elif defined(CRAY)

struct nlist nmlist[] = {
   { "cpuw" },
   { "sysinfoa" },
   { 0 }
};
static struct listreq iolist[1];
static struct iosw iosw[1];
struct listreq Krdlist[] = {
   LO_READ, 0, LF_LSEEK, 0, 0, (char *)0, 0, &iosw[0], 0, 1, 0, 0
};
struct sysinfo_t *i_sysinfoa;
int Ncpus;

#endif 


#if defined(SOLARIS) || defined(SOLARIS64)

double get_cpu_load() {
   static unsigned long *cpu_offset = NULL;
   kernel_fd_type kernel_fd;
   static long address_cpu = 0;
   static long address_ncpus = 0;
   int cpus_found, i, j;
   static int number_of_cpus;
   int cpu_load = -1.0;
   static long cpu_time[CPUSTATES];
   static long cpu_old[CPUSTATES];
   static long cpu_diff[CPUSTATES]; 
   double cpu_states[CPUSTATES];

   if (sge_get_kernel_fd(&kernel_fd)
       && (address_cpu || sge_get_kernel_address("cpu", &address_cpu))
       && (address_ncpus || sge_get_kernel_address("ncpus", &address_ncpus))) {

      /* how many cpu's ? */
      if (getkval(address_ncpus, &number_of_cpus, sizeof(number_of_cpus), 
         "ncpus")) {
         return -1.0;
      }

      /* alloc adress array */
      if (!cpu_offset) {
         cpu_offset = (unsigned long*)malloc(
                                     number_of_cpus * sizeof (unsigned long));
      }
      if (cpu_offset) {
         for(i = cpus_found = 0; cpus_found < number_of_cpus; i++) {
            if (getkval(address_cpu + i*sizeof(unsigned long), 
               (int*)&cpu_offset[cpus_found], sizeof(unsigned long), "cpu")) {
               return -1.0;
            }
            if (cpu_offset[cpus_found] != 0) {
               cpus_found++;
            }
         }  
         for (i=0; i<CPUSTATES; i++) {
            cpu_time[i] = 0;
         }
         for (i=0; i<number_of_cpus; i++) {
            if (cpu_offset[i] != 0) {
               struct cpu cpu;

               if (getkval(cpu_offset[i], (int*)&cpu, sizeof(struct cpu), "cpu")) {
                  return -1.0;
               }
               for (j=0; j < CPU_WAIT; j++) {
                  cpu_time[j] += cpu.cpu_stat.cpu_sysinfo.cpu[j];
               }
               cpu_time[CPUSTATE_IOWAIT] += cpu.cpu_stat.cpu_sysinfo.wait[W_IO] 
                  + cpu.cpu_stat.cpu_sysinfo.wait[W_PIO];
               cpu_time[CPUSTATE_SWAP] += cpu.cpu_stat.cpu_sysinfo.wait[W_SWAP]; 
            }
         }
         percentages (CPUSTATES, cpu_states, cpu_time, cpu_old, cpu_diff); 
         cpu_load = cpu_states[1] + cpu_states[2] + cpu_states[3] + cpu_states[4];
      }
   } else {
      return -1.0;
   }
   return cpu_load;
}
#elif defined(LINUX)

static char* skip_token(
char *p 
) {
   while (isspace(*p)) {
      p++;
   }
   while (*p && !isspace(*p)) {
      p++;
   }
   return p;
}

static double get_cpu_load()
{
   int fd = -1;
   int len, i;
   char buffer[4096];
   char filename[4096];
   char *p;
   double cpu_load;
   static long cpu_new[CPUSTATES];
   static long cpu_old[CPUSTATES];
   static long cpu_diff[CPUSTATES];
   static double cpu_states[CPUSTATES];

   sprintf(filename, "%s/stat", PROCFS);
   fd = open(filename, O_RDONLY);
   if (fd == -1) {
      return -1;
   }
   len = read(fd, buffer, sizeof(buffer)-1);
   close(fd);
   buffer[len] = '\0';
   p=skip_token(buffer);
   for (i=0; i<CPUSTATES; i++) {
      cpu_new[i] = strtoul(p, &p, 10);
   }
   percentages(CPUSTATES, cpu_states, cpu_new, cpu_old, cpu_diff);

   cpu_load = cpu_states[0] + cpu_states[1] + cpu_states[2];

   if (cpu_load < 0.0) {
      cpu_load = -1.0;
   }
   return cpu_load;
}                  

#elif defined(ALPHA4) || defined(ALPHA5)

double get_cpu_load() {
   static long cpu_old_ticks[CPUSTATES];
   long cpu_new_ticks[CPUSTATES];
   long cpu_diff_ticks[CPUSTATES];
   double cpu_states[CPUSTATES];
   long delta_ticks;
   double cpu_load;
   struct tbl_sysinfo sys_info;
   int i;

   if (table(TBL_SYSINFO,0, &sys_info, 1, sizeof(struct tbl_sysinfo)) < 0) {
      return -1.0;
   }  
   cpu_new_ticks[0] = sys_info.si_user;
   cpu_new_ticks[1] = sys_info.si_nice;
   cpu_new_ticks[2] = sys_info.si_sys;
   cpu_new_ticks[3] = sys_info.si_idle;
   delta_ticks = 0;
   for (i=0; i<CPUSTATES; i++) {
      cpu_diff_ticks[i] = cpu_new_ticks[i] - cpu_old_ticks[i];
      delta_ticks += cpu_diff_ticks[i];
      cpu_old_ticks[i] = cpu_new_ticks[i]; 
   }
   cpu_load = 0.0;
   if (delta_ticks) {
      for(i=0; i<CPUSTATES; i++) {
         cpu_states[i] = ((double)cpu_diff_ticks[i] / delta_ticks) * 100.0;
      }
   }
   cpu_load += cpu_states[0] + cpu_states[1] + cpu_states[2];
   if (cpu_load < 0.0) {
      cpu_load = -1.0;
   }
   return cpu_load;
}

#elif defined(IRIX6)

double get_cpu_load() 
{
   static long cpu_new[CPUSTATES];
   static long cpu_old[CPUSTATES];
   static long cpu_diff[CPUSTATES]; 
   double cpu_states[CPUSTATES];
   double cpu_load;
   struct sysinfo sys_info;
   int i;

   if (sysmp(MP_SAGET, MPSA_SINFO, &sys_info, sizeof(struct sysinfo)) == -1) {
      return -1.0;
   }

   for (i = 0; i < CPUSTATES; i++) {
      cpu_new[i] = sys_info.cpu[i];
   }

   percentages (CPUSTATES, cpu_states, cpu_new, cpu_old, cpu_diff);

   cpu_load = cpu_states[1] + cpu_states[2] + cpu_states[3] 
      + cpu_states[4] + cpu_states[5];
   if (cpu_load < 0.0) {
      cpu_load = -1.0;
   }
   return cpu_load;
}

#elif defined(HP10)

static double get_cpu_load()
{
   kernel_fd_type kernel_fd;
   long address = 0;
   static long cpu_time[CPUSTATES];
   static long cpu_old[CPUSTATES];
   static long cpu_diff[CPUSTATES];
   double cpu_states[CPUSTATES];
   double cpu_load;

   if (sge_get_kernel_fd(&kernel_fd)
       && sge_get_kernel_address("cp_time", &address)) {
      getkval(address, (int*)&cpu_time, sizeof(cpu_time), "cp_time"); 
      percentages(CPUSTATES, cpu_states, cpu_time, cpu_old, cpu_diff);
      cpu_load = cpu_states[0] + cpu_states[1] + cpu_states[2];
      if (cpu_load < 0.0) {
         cpu_load = -1.0;
      }
   } else {
      cpu_load = -1.0;
   }
   return cpu_load;
}    

#elif defined(HP11)

static double get_cpu_load()
{  
   struct pst_processor cpu_buffer;
   struct pst_dynamic dynamic_buffer;
   int ret, i, cpus;
   static long cpu_time[PST_MAX_CPUSTATES];
   static long cpu_old[PST_MAX_CPUSTATES];
   static long cpu_diff[PST_MAX_CPUSTATES];
   double cpu_states[PST_MAX_CPUSTATES];
   double cpu_load;

   ret = pstat_getdynamic(&dynamic_buffer, sizeof(dynamic_buffer), 1, 0);
   if (ret != -1) {
      cpus = dynamic_buffer.psd_max_proc_cnt;
      for (i = 0; i < cpus; i++) {
         ret = pstat_getprocessor(&cpu_buffer, sizeof(cpu_buffer), 1, i);
         if (ret != -1) {
            percentages(PST_MAX_CPUSTATES, cpu_states, 
               (long *)cpu_buffer.psp_cpu_time, cpu_old, cpu_diff);
            cpu_load = cpu_states[0] + cpu_states[1] + cpu_states[2];
         }
      } 
      return cpu_load;
   } else {
      return -1.0;
   }
}

#endif

#if defined(ALPHA4) || defined(ALPHA5) || defined(IRIX6) || defined(HP10) || (defined(SOLARIS) && !defined(SOLARIS64))

static int get_load_avg(
double loadavg[],
int nelem 
) {
   kernel_fd_type kernel_fd;
   long address;
   KERNEL_AVG_TYPE avg[3];
   int elements = 0;

   if (sge_get_kernel_fd(&kernel_fd)
       && sge_get_kernel_address(KERNEL_AVG_NAME, &address)) {
      getkval(address, (int*)&avg, sizeof(avg), KERNEL_AVG_NAME);

      while (elements < nelem) {
         loadavg[elements] = KERNEL_TO_USER_AVG(avg[elements]);

         elements++;
      }
   } else {
      elements = -1;
   }
   return elements;
}

#elif defined(HP11)

static int get_load_avg(
double loadavg[],
int nelem 
) {
   struct pst_processor cpu_buffer;
   struct pst_dynamic dynamic_buffer;
   int ret, i, cpus;

   ret = pstat_getdynamic(&dynamic_buffer, sizeof(dynamic_buffer), 1, 0);
   if (ret != -1) {
      cpus = dynamic_buffer.psd_max_proc_cnt;
      loadavg[0] = 0.0;
      loadavg[1] = 0.0;
      loadavg[2] = 0.0;
      for (i = 0; i < cpus; i++) {
         ret = pstat_getprocessor(&cpu_buffer, sizeof(cpu_buffer), 1, i);
         if (ret != -1) {
            loadavg[0] += cpu_buffer.psp_avg_1_min;
            loadavg[1] += cpu_buffer.psp_avg_5_min;
            loadavg[2] += cpu_buffer.psp_avg_15_min;
         }
      } 
      loadavg[0] /= cpus;
      loadavg[1] /= cpus;
      loadavg[2] /= cpus;
      return 3;
   } else {
      return -1;
   }
}

#elif defined(LINUX)

static int get_load_avg(
double loadv[],
int nelem 
) {
   char buffer[41];
   int fd, count;

   fd = open(LINUX_LOAD_SOURCE, O_RDONLY);
   if (fd == -1) {
      return -1;
   }
   count = read(fd, buffer, 40);
   buffer[count] = '\0';
   close(fd);
   if (count <= 0) {
      return -1;
   }
   count = sscanf(buffer, "%lf %lf %lf", &(loadv[0]), &loadv[1], &loadv[2]);
   if (count < 1) {
      return -1;
   }
   return 0;
}

#elif defined(CRAY)

void KmemRead(iolist, size, k_fd)
struct listreq *iolistr; 
int size; 
int k_fd;
{
   struct listreq *lp;
   struct iosw *sp;
   int n;

   lp = iolist;
   for (n = 0; n < size; n++) {
      sp = iolist[n].li_status;
      *(word *)sp = 0;
      iolist[n].li_fildes = k_fd;
   }
   if (listio(LC_WAIT, iolist, size) < 0) {
          perror("listio:");
          exit(1);
   }
   lp = iolist;
   for (n = 0; n < size; n++) {
          sp = iolist[n].li_status;
          sp = lp->li_status;
          lp++;
   }
}

static int getloadavg(
double loadv[],
int nelem 
) {
   struct listreq *lp;
   struct iosw *sp;
   struct pw cpuw;
   int i;
   double avenrun1=0, avenrun2=0, avenrun3=0;
   int highest;

   if (nlist("/unicos", nmlist) == -1) {
      return -1;
      perror("nlist()");
      exit(1);
   }

   if (kernel_fd == -1) {
      if ((kernel_fd = open("/dev/kmem", 0)) < 0) {
         return -1;
         perror("open(/dev/kmem)");
         exit(1);
      }
   }

   lp = &iolist[0];
   sp = &iosw[0];

   lp->li_offset = nmlist[0].n_value;
   lp->li_buf = (char *)&cpuw;
   lp->li_nbyte = sizeof(struct pw);
   lp->li_fildes = kernel_fd;
   lp->li_opcode = LO_READ;
   lp->li_flags = LF_LSEEK;
   lp->li_nstride = 1;
   lp->li_filstride = lp->li_memstride = lp->li_nbyte;
   lp->li_status = sp;

   if (listio(LC_WAIT, iolist, 1) < 0) {
      return -1;
      perror("listio()");
      exit(1);
   }

   /* cpuw now shold contain some data */
   Ncpus = cpuw.pw_ccpu;
   if (Ncpus < 1)
      return -1;

#if O
   printf("Ncpus=%d\n", Ncpus);
   printf("started cpus = %d\n", cpuw.pw_scpu);
   printf("name = %x\n", cpuw.pw_name);
#endif

   Krdlist[0].li_offset = nmlist[1].n_value;
   if ((i_sysinfoa = (struct sysinfo_t *)malloc(sizeof(*i_sysinfoa) * Ncpus))
            == NULL) {
      return -1;
      perror("malloc");
      exit(1);
   }
   Krdlist[0].li_nbyte = sizeof(*i_sysinfoa) * Ncpus;
   Krdlist[0].li_buf = (char *)i_sysinfoa;

   KmemRead(Krdlist, 1, kernel_fd);
   highest = 0;
   for (i = 0; i < Ncpus; i++) {
      if (i_sysinfoa[i].avenrun[0] ||
          i_sysinfoa[i].avenrun[1] ||
          i_sysinfoa[i].avenrun[2])
         highest++;	/* Ncpus may not be correct */
      avenrun1 += i_sysinfoa[i].avenrun[0];
      avenrun2 += i_sysinfoa[i].avenrun[1];
      avenrun3 += i_sysinfoa[i].avenrun[2];
   }
   if (!highest)
      highest = 1;
   if (nelem > 0)
      loadv[0] = ((double)avenrun1)/highest;
   if (nelem > 1)
      loadv[1] = ((double)avenrun2)/highest;
   if (nelem > 2)
      loadv[2] = ((double)avenrun3)/highest;
   return 0;
}

#elif defined(NECSX4) || defined(NECSX5)

int getloadavg_necsx_rsg(
int rsg_id,
double loadv[] 
) {
   int fd;
   rsgavg_t avg;
   char fsg_dev_string[256];
   int avg_count;
   int avg_id;

   sprintf(fsg_dev_string, "/dev/rsg/%d", rsg_id);
   fd = open(fsg_dev_string, O_RDONLY);
   if (fd > 0) {
      if (ioctl(fd, RSG_AVG, &avg) == -1) {
         close(fd);
         return -1;
      }
      close(fd);

      if (avg.avgrun.fscale > 0) {
         for(avg_id=0; avg_id<3; avg_id++) {
            loadv[avg_id] += (double)avg.avgrun.average[avg_id]/
             avg.avgrun.fscale;
         }
      } else
         return -1;
   }
   return 0;
}       

static int getloadavg(
double loadv[],
int nelem 
) {
   int fd;
   int fsg_id;
   rsgavg_t avg;
   char fsg_dev_string[256];
   int avg_count;
   int avg_id;

   avg_count = 0;
   for(avg_id=0; avg_id<3; avg_id++)
      loadv[avg_id] = 0.0;

   for (fsg_id=0; fsg_id<32; fsg_id++) {
      sprintf(fsg_dev_string, "/dev/rsg/%d", fsg_id);
      fd = open(fsg_dev_string, O_RDONLY);
      if (fd > 0) {
         if (ioctl(fd, RSG_AVG, &avg) == -1) {
            close(fd);
            continue;
         }
         close(fd);

         if (avg.avgrun.fscale > 0) {
            for(avg_id=0; avg_id<3; avg_id++) {
               loadv[avg_id] += (double)avg.avgrun.average[avg_id]/
                avg.avgrun.fscale;
            }
            avg_count++;
         }
      }
   }
   if (avg_count > 0) {
      for(avg_id=0; avg_id<3; avg_id++) {
         loadv[avg_id] /= avg_count;
      }
   } else {
      return -1;
   }
   return 0;
}
#endif 


int get_channel_fd()
{
   if (kernel_initialized) {
#if defined(SOLARIS) || defined(SOLARIS64) || defined(LINUX) || defined(HP11)
      return -1;
#else
      return kernel_fd;
#endif
   } else {
      return -1;
   }
}    

int sge_getloadavg(
double loadavg[],
int nelem 
) {
   int elem = 0;   

#if defined(SOLARIS64)
   elem = getloadavg(loadavg, nelem); /* <== library function */
#elif (defined(SOLARIS) && !defined(SOLARIS64)) || defined(ALPHA4) || defined(ALPHA5) || defined(IRIX6) || defined(HP10) || defined(HP11) || defined(CRAY) || defined(NECSX4) || defined(NECSX5) || defined(LINUX)
   elem = get_load_avg(loadavg, nelem); 
#else
   elem = -1;    
#endif
   if (elem != -1) {
      elem = nelem;  
   }
   return elem;
}

#ifdef SGE_LOADCPU

int sge_getcpuload(
double *cpu_load 
) {
   double load;
   int ret;

   load = get_cpu_load();
   if (load < 0.0) {
      ret = -1;
   } else {
      *cpu_load = load;
      ret = 0;
   }
   return ret;
}

static long percentages(int cnt, double *out, long *new, long *old, long *diffs)
{
   int i;
   register long change;
   register long total_change;
   register long *dp;
   long half_total;

   /* initialization */
   total_change = 0;
   dp = diffs;
   /* calculate changes for each state and the overall change */
   for (i = 0; i < cnt; i++) {
      if ((change = *new - *old) < 0) {
       /* this only happens when the counter wraps */
         change = (int)
         ((unsigned long)*new-(unsigned long)*old);
      }
      total_change += (*dp++ = change);
      *old++ = *new++;
   }
   /* avoid divide by zero potential */
   if (total_change == 0) {
      total_change = 1;
   }
   /* calculate percentages based on overall change, rounding up */
   half_total = total_change / 2l;
   for (i = 0; i < cnt; i++)
      *out++ = ((double)((*diffs++ * 1000 + half_total) / total_change))/10;
   return total_change;
}       

#endif

#ifdef TEST
int main(
int argc,
char *argv[],
char *envp[] 
) {
   int naptime = -1;

   if (argc > 1)
      naptime = atoi(argv[1]);

   while (1) {
      double avg[3];
      int loads;
      double cpu_load;

      errno = 0;             
      loads = sge_getloadavg(avg, 3);
      if (loads == -1) {
         perror("Error getting load average");
         exit(1);
      }
      if (loads > 0)
         printf("load average: %.2f", avg[0]);
      if (loads > 1)
         printf(", %.2f", avg[1]);
      if (loads > 2)
         printf(", %.2f", avg[2]);
      if (loads > 0)
         putchar('\n');

      cpu_load = get_cpu_load();
      if (cpu_load >= 0.0) {
         printf("cpu load: %.2f\n", cpu_load);
      }

      if (naptime == -1) {
         break;
      }
      sleep(naptime);
   }

   exit(0);
}
#endif /* TEST */


