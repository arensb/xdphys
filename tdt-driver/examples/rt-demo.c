/*
 * rt-demo.c -- a tiny real-time application test and demonstration tool
 *
 * Markus Kuhn -- 1996-01-04
 *
 * The function test_run() simulates a real-time algorithm. It
 * executes a number of operations which all are likely to cause a
 * page-fault on a system without memory locking. The maximum time
 * that passes during these operations is recorded using
 * gettimeofday(). test_run() is called several times to show the
 * variation in execution delay caused by the virtual memory
 * management. With functioning real-time mechanisms like memory
 * locking and a fixed priority scheduler, the measured maximum
 * execution delay should be almost constant and not above a few
 * hundred microseconds.
 *
 * Start "rt-demo no-rt" in order to execute the test without memory
 * locking and real-timer scheduling (just for comparison) and start
 * "rt-demo" in order to test with real-time extensions.
 *
 * Test this software also e.g. while compiling a new kernel and on
 * a freshly booted machine.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sched.h>
#include <linux/unistd.h>     /* for _syscallX macros/related stuff */

/* ====> Specify here how many bytes RAM you have !!! <==== */
#define TOTAL_RAM  (16*1024*1024)

/* should together be around 50% of the available RAM minus a few megabytes */
#define BYTES_USED ((TOTAL_RAM-3*1024*1024)/4)
#define STACK_USED ((TOTAL_RAM-3*1024*1024)/4)

/* the following lines are only compiled as a quick fix if you still
 * have an old set of libc include files */

/* quick 'n dirty replacement of <sys/mman.h> */
#if !defined(_POSIX_MEMLOCK) && defined(__NR_mlockall)
#define MCL_CURRENT    1               /* lock all current mappings */
#define MCL_FUTURE     2               /* lock all future mappings */
_syscall1(int, mlockall, int, flags);
_syscall0(int, munlockall);
int mlockall(int flags);
int munlockall(void);
#define _POSIX_MEMLOCK
#endif
#if !defined(_POSIX_MEMLOCK_RANGE) && defined(__NR_mlock)
_syscall2(int, mlock, const void *, addr, size_t, len);
_syscall2(int, munlock, const void *, addr, size_t, len);
int mlock(const void *addr, size_t len);
int munlock(const void *addr, size_t len);
#define _POSIX_MEMLOCK_RANGE
#endif
/* quick 'n dirty replacement of <sched.h> */
#if !defined(_POSIX_PRIORITY_SCHEDULING) && defined(__NR_sched_setscheduler)
#if 0
/* belongs into <time.h> */
struct timespec {
  time_t  tv_sec;         /* seconds */
  long    tv_nsec;        /* nanoseconds */
};
#endif
struct sched_param {
  int sched_priority;
};
#define SCHED_OTHER    0
#define SCHED_FIFO     1
#define SCHED_RR       2
#define SCHED_MIN      0
#define SCHED_MAX     31
_syscall2(int, sched_setparam, pid_t, pid, const struct sched_param *, param);
_syscall2(int, sched_getparam, pid_t, pid, struct sched_param *, param);
_syscall3(int, sched_setscheduler, pid_t, pid, int, policy,
          const struct sched_param *, param);
_syscall1(int, sched_getscheduler, pid_t, pid);
_syscall0(int, sched_yield);
_syscall1(int, sched_get_priority_max, int, policy);
_syscall1(int, sched_get_priority_min, int, policy);
_syscall2(int, sched_rr_get_interval, pid_t, pid, struct timespec *, interval);
int sched_setparam(pid_t pid, const struct sched_param *param);
int sched_getparam(pid_t pid, struct sched_param *param);
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
int sched_getscheduler(pid_t pid);
int sched_yield(void);
int sched_get_priority_max(int policy);
int sched_get_priority_min(int policy);
int sched_rr_get_interval(pid_t pid, struct timespec *interval);
#define _POSIX_PRIORITY_SCHEDULING
#endif

struct timeval last_event;
long max_delay;
char *mem;

void start_critical_section(void)
{
  gettimeofday(&last_event, NULL);
  max_delay = 0;
  return;
}

/* simulates steps in the critical phase of a real-time application */
void urgent_stuff(void) {
  struct timeval this_event;
  long delay;

  gettimeofday(&this_event, NULL);
  delay = (this_event.tv_sec - last_event.tv_sec) * 1000000L +
    this_event.tv_usec - last_event.tv_usec;
  if (delay > max_delay)
    max_delay = delay;
  last_event = this_event;

  return;
}

void end_critical_section(void)
{
  urgent_stuff();
  printf("maximum delay between actions was %ld \265s.\n", max_delay);
  return;
}

void test_stack(int loops)
{
  volatile char dummy[1000];

  dummy[0] = dummy[999] = 0x42;
  urgent_stuff();
  if (loops > 0)
    test_stack(loops-1);
  urgent_stuff();
  return;
}

void test_run(void)
{
  int i;
  char buf[500];

  start_critical_section();
  /* do nothing */
  urgent_stuff();
  /* call a few shared library functions */
  i = atoi("0815");
  urgent_stuff();
  sprintf(buf, "%d hello %f\n", 1342, 3.1415926);
  urgent_stuff();
  /* touch my application memory */
  for (i = 0; i < BYTES_USED; i += 4096) {
    buf[0] += mem[i];
    urgent_stuff();
  }
  for (i = 0; i < BYTES_USED; i += 4096) {
    mem[i+1] = i;
    urgent_stuff();
  }
  /* call a few more shared library functions */
  i = rand();
  urgent_stuff();
  i = getegid();
  urgent_stuff();
  /* touch the preallocated stack */
  test_stack(STACK_USED / 1050);

  end_critical_section();
}

void touch_stack(void)
{
  volatile char dummy[STACK_USED];

#if 0
  int i;
  for (i = 0; i < STACK_USED; dummy[i++] = 0);
#endif

  dummy[0] = dummy[STACK_USED-1] = 0;    /* touch boundary pages */
  return;
}

int main(int argc, char *argv)
{
  int i, j, r, minp, maxp;
#ifdef _POSIX_PRIORITY_SCHEDULING
  struct sched_param my_priority;
#endif

  mem = (char *) malloc(BYTES_USED);

  /* First real-time preparation: lock all pages into RAM */
#ifdef _POSIX_MEMLOCK
  if (argc == 1)
    if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
      perror("mlockall(MCL_CURRENT | MCL_FUTURE)");
    }
#else
  printf("Warning: memory locking not available, get a new kernel!\n");
#endif

  /* Second real-time preparation: touch a sufficiently large stack */
  if (argc == 1)
    touch_stack();

  /* Third real-time preparation: we now take the processor exclusively */
#ifdef _POSIX_PRIORITY_SCHEDULING
  minp = sched_get_priority_min(SCHED_FIFO);
  if (minp < 0) {
    perror("sched_get_priority_min(SCHED_FIFO)");
    exit(-1);
  }
  maxp = sched_get_priority_max(SCHED_FIFO);
  if (maxp < 0) {
    perror("sched_get_priority_max(SCHED_FIFO)");
    exit(-1);
  }
  printf("sched_get_priority_min(SCHED_FIFO) = %d\n", minp);
  printf("sched_get_priority_max(SCHED_FIFO) = %d\n", maxp);
  if (maxp > minp)
    /* leave a chance for a higher priority rescue shell */
    my_priority.sched_priority = maxp - 1;
  else
    my_priority.sched_priority = maxp;
  if (my_priority.sched_priority < sched_get_priority_min(SCHED_FIFO))
    my_priority.sched_priority++;
  if (argc == 1)
    if (sched_setscheduler(getpid(), SCHED_FIFO, &my_priority)) {
      perror("sched_setscheduler(..., SCHED_FIFO, ...)");
      exit(-1);
    }
  my_priority.sched_priority = 4711;
  i = sched_getparam(0, &my_priority);
  if (i < 0)
    perror("sched_getparam(0, ...)");
  else
    printf("sched_getparam(0) = %d\n", my_priority.sched_priority);
  i = sched_getscheduler(0);
  if (i < 0)
    perror("sched_getscheduler(0)");
  else
    printf("sched_getscheduler(0) = %d\n", i);
#else
  printf("Warning: realtime scheduling not available, get a new kernel!\n");
#endif

  for (i = 0; i < 10; i++)
    test_run();

  /* back to normal scheduling */
  my_priority.sched_priority = 0;
  if (sched_setscheduler(0, SCHED_OTHER, &my_priority)) {
    perror("sched_setscheduler(..., SCHED_OTHER, ...)");
    exit(-1);
  }

  printf("\nTest of scheduling properties\n");

  printf("FIFO\n\n");
  my_priority.sched_priority = 90;
  if (sched_setscheduler(0, SCHED_FIFO, &my_priority)) {
    perror("sched_setscheduler(..., SCHED_FIFO, ...)");
    exit(-1);
  }
  fflush(stdout);
  if ((r = fork()) < 0) {
    perror("fork()");
    exit(-1);
  }
  if (r == 0) {
    /* child */
    for (i = 0; i < 100; i++) {
      for (j = 0; j < 1000000; j++)
        r += i*j*i*i;
      write(1, "c", 1);
      if (i % 10 == 0) {
        write(1, "CY", 2);
        sched_yield();
      }
    }
    exit(0);
  } else {
    /* parent */
    for (i = 0; i < 100; i++) {
      for (j = 0; j < 1000000; j++)
        r += i*j*i*i;
      write(1, "p", 1);
      if (i % 10 == 0) {
        write(1, "PY", 2);
        sched_yield();
      }
    }
    wait(NULL);
  }

  printf("\n\nRR\n\n");
  my_priority.sched_priority = 90;
  if (sched_setscheduler(0, SCHED_RR, &my_priority)) {
    perror("sched_setscheduler(..., SCHED_RR, ...)");
    exit(-1);
  }
  fflush(stdout);
  if ((r = fork()) < 0) {
    perror("fork()");
    exit(-1);
  }
  if (r == 0) {
    /* child */
    for (i = 0; i < 100; i++) {
      for (j = 0; j < 1000000; j++)
        r += i*j*i;
      write(1, "c", 1);
      if (i % 10 == 0) {
        write(1, "CY", 2);
        sched_yield();
      }
    }
    exit(0);
  } else {
    /* parent */
    for (i = 0; i < 100; i++) {
      for (j = 0; j < 1000000; j++)
        r += i*j*i;
      write(1, "p", 1);
      if (i % 10 == 0) {
        write(1, "PY", 2);
        sched_yield();
      }
    }
    wait(NULL);
  }

  printf("\n");

  return 0;
}
