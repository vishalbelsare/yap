/*************************************************************************
 *									 *
 *	 YAP Prolog 							 *
 *									 *
 *	Yap Prolog was developed at NCCUP - Universidade do Porto	 *                 
 *									 *
 * Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
 *									 *
 **************************************************************************
 *									 *
 * File:		sysbits.c *
 * Last rev:	4/03/88							 *
 * mods: *
 * comments:	very much machine dependent routines			 *
 *									 *
 *************************************************************************/

/**
 * @file sysbits.c
 * @brief  Time Operating System Interface
 *
 */

/**
 * @defgroup YAPOSTime Time System Interface
 * @ingroup InputOutput
 * @brief Interface to the Time Operating System functionality.
 * @{
 *
 * These routines provide support for accessing real (watch) time and process execution time (run-time). In order to match routines such as gethrtime() time is stored internally in intervals of nanoseconds.
 *
 *
 */


    
#include "sysbits.h"

#ifdef SIMICS
#ifdef HAVE_GETRUSAGE
#undef HAVE_GETRUSAGE
#endif
#ifdef HAVE_TIMES
#undef HAVE_TIMES
#endif
#endif /* SIMICS */

#ifdef _WIN32
#if HAVE_GETRUSAGE
#undef HAVE_GETRUSAGE
#endif
#endif


#define sec2nanos(T) (((uint64_t)T)*1000000000)
#define usec2nanos(T) (((uint64_t)T)*1000)

/**
   In Unix/Linux/OSX use getrusage to obtain user/sys time.
   in WIN32 use GetProocessTimes */
#if HAVE_GETRUSAGE

#define timevalnanos(T) sec2nanos(T.tv_sec)+usec2nanos(T.tv_usec) 

//#if THREADS
//#define StartOfTimes (*(LOCAL_ThreadHandle.start_of_timesp))
//#define last_time (*(LOCAL_ThreadHandle.last_timep))
//
//#define StartOfTimes_sys (*(LOCAL_ThreadHandle.start_of_times_sysp))
//#define last_time_sys (*(LOCAL_ThreadHandle.last_time_sysp))
//
//#else
/* since the point YAP was started */
uint64_t StartOfTimes;

/* since last call to runtime */
uint64_t last_time;

/* same for system time */
uint64_t last_time_sys;
uint64_t StartOfTimes_sys;

void Yap_InitTime(int wid) {
  struct rusage rusage;

//#if THREADS
//  REMOTE_ThreadHandle(wid).start_of_timesp =
//      (struct timeval *)malloc(sizeof(struct timeval));
//  REMOTE_ThreadHandle(wid).last_timep =
//      (struct timeval *)malloc(sizeof(struct timeval));
//  REMOTE_ThreadHandle(wid).start_of_times_sysp =
//      (struct timeval *)malloc(sizeof(struct timeval));
//  REMOTE_ThreadHandle(wid).last_time_sysp =
//      (struct timeval *)malloc(sizeof(struct timeval));
//  getrusage(RUSAGE_SELF, &rusage);
//  (*REMOTE_ThreadHandle(wid).last_timep).tv_sec =
//      (*REMOTE_ThreadHandle(wid).start_of_timesp).tv_sec =
//          rusage.ru_utime.tv_sec;
//  (*REMOTE_ThreadHandle(wid).last_timep).tv_usec =
//      (*REMOTE_ThreadHandle(wid).start_of_timesp).tv_usec =
//          rusage.ru_utime.tv_usec;
//  (*REMOTE_ThreadHandle(wid).last_time_sysp).tv_sec =
//      (*REMOTE_ThreadHandle(wid).start_of_times_sysp).tv_sec =
//          rusage.ru_stime.tv_sec;
//  (*REMOTE_ThreadHandle(wid).last_time_sysp).tv_usec =
//      (*REMOTE_ThreadHandle(wid).start_of_times_sysp).tv_usec =
//          rusage.ru_stime.tv_usec;
//#else
  getrusage(RUSAGE_SELF, &rusage);
  last_time = StartOfTimes = timevalnanos(rusage.ru_utime);
  last_time_sys = StartOfTimes_sys = timevalnanos(rusage.ru_stime);
//#endif
}

uint64_t Yap_cputime(void) {
  struct rusage rusage;

  getrusage(RUSAGE_SELF, &rusage);
  return timevalnanos(rusage.ru_utime) - (StartOfTimes);
}

void Yap_cputime_interval(uint64_t *now, uint64_t *interval) {
  struct rusage rusage;

  getrusage(RUSAGE_SELF, &rusage);
  uint64_t clock = timevalnanos(rusage.ru_utime);
  *now =clock  - (StartOfTimes);
  *interval = clock - last_time;
  last_time = clock;
}

void Yap_systime_interval(uint64_t *now, uint64_t *interval) {
  struct rusage rusage;

  getrusage(RUSAGE_SELF, &rusage);
  *now = timevalnanos(rusage.ru_stime) - (StartOfTimes_sys);
  *interval = timevalnanos(rusage.ru_stime) -(last_time_sys);
  last_time_sys =timevalnanos(rusage.ru_stime) ;
 }

#elif defined(_WIN32)
static unsi filetime2nanos(T) ( 

#include <time.h>

static FILETIME StartOfTimes, last_time;

static win64_time_t StartOfTimes_sys, last_time_sys;

static clock_t TimesStartOfTimes, Times_last_time;

/* store user time in this variable */
void Yap_InitTime(int wid) {
  HANDLE hProcess = GetCurrentProcess();
  win64_time_t CreationTime, ExitTime, KernelTime, UserTime;
  if (!GetProcessTimes(hProcess, &CreationTime.f, &ExitTime.f, &KernelTime.f,
                       &UserTime.f)) {
    /* WIN98 */
    clock_t t;
    t = clock();
    Times_last_time = TimesStartOfTimes = t;
  } else {
#if THREADS
    REMOTE_ThreadHandle(wid).start_of_timesp =
        (struct _win64_time_t *)malloc(sizeof(win64_time_t));
    REMOTE_ThreadHandle(wid).last_timep =
        (struct _win64_time_t *)malloc(sizeof(win64_time_t));
    REMOTE_ThreadHandle(wid).start_of_times_sysp =
        (struct _win64_time_t *)malloc(sizeof(win64_time_t));
    REMOTE_ThreadHandle(wid).last_time_sysp =
        (struct _win64_time_t *)malloc(sizeof(win64_time_t));
    (*REMOTE_ThreadHandle(wid).last_timep).f.dwLowDateTime =
        UserTime.dwLowDateTime;
    (*REMOTE_ThreadHandle(wid).last_timep).dwHighDateTime =
        UserTime.dwHighDateTime;
    (*REMOTE_ThreadHandle(wid).start_of_timesp).f.dwLowDateTime =
        UserTime.dwLowDateTime;
    (*REMOTE_ThreadHandle(wid).start_of_timesp).f.dwHighDateTime =
        UserTime.dwHighDateTime;
    (*REMOTE_ThreadHandle(wid).last_time_sysp).f.dwLowDateTime =
        KernelTime.dwLowDateTime;
    (*REMOTE_ThreadHandle(wid).last_time_sysp).f.dwHighDateTime =
        KernelTime.dwHighDateTime;
    (*REMOTE_ThreadHandle(wid).start_of_times_sysp).f.dwLowDateTime =
        KernelTime.dwLowDateTime;
    (*REMOTE_ThreadHandle(wid).start_of_times_sysp).f.dwHighDateTime =
        KernelTime.dwHighDateTime;
#else
    last_time.f.dwLowDateTime = UserTime.f.dwLowDateTime;
    last_time.f.dwHighDateTime = UserTime.f.dwHighDateTime;
    StartOfTimes.f.dwLowDateTime = UserTime.f.dwLowDateTime;
    StartOfTimes.f.dwHighDateTime = UserTime.f.dwHighDateTime;
    last_time_sys.f.dwLowDateTime = KernelTime.f.dwLowDateTime;
    last_time_sys.f.dwHighDateTime = KernelTime.f.dwHighDateTime;
    StartOfTimes_sys.f.dwLowDateTime = KernelTime.f.dwLowDateTime;
    StartOfTimes_sys.f.dwHighDateTime = KernelTime.f.dwHighDateTime;
#endif
  }
}

UInt Yap_cputime(void) {
  HANDLE hProcess = GetCurrentProcess();
  win64_time_t CreationTime, ExitTime, KernelTime, UserTime;
  if (!GetProcessTimes(hProcess, &CreationTime.f, & ExitTime.f, &KernelTime.f,
                       &UserTime.f)) {
    clock_t t;
    t = clock();
    return (((t - TimesStartOfTimes) * 1000) / CLOCKS_PER_SEC);
  } else {
    uint64_t t = UserTime.t - StartOfTimes.t;
    return ((Int)(t / 10000));
  }
}

void Yap_cputime_interval(Int *now, Int *interval) {
  HANDLE hProcess = GetCurrentProcess();
  win64_time_t CreationTime, ExitTime, KernelTime, UserTime;
  if (!GetProcessTimes(hProcess, &CreationTime.f, & ExitTime.f, &KernelTime.f,
                       &UserTime.f)) {
    clock_t t;
    t = clock();
    *now = ((t - TimesStartOfTimes) * 1000) / CLOCKS_PER_SEC;
    *interval = (t - Times_last_time) * 1000 / CLOCKS_PER_SEC;
    Times_last_time = t;
  } else {
    __int64 t1 = UserTime.t - StartOfTimes.t;
    __int64 t2 = UserTime.t - last_time.t;
    *now = (Int)(t1 / 10000);
    *interval = (Int)(t2 / 10000);
    last_time.f.dwLowDateTime = UserTime.f.dwLowDateTime;
    last_time.f.dwHighDateTime = UserTime.f.dwHighDateTime;
  }
}

void Yap_systime_interval(Int *now, Int *interval) {
  HANDLE hProcess = GetCurrentProcess();
  win64_time_t CreationTime, ExitTime, KernelTime, UserTime;
  if (!GetProcessTimes(hProcess, &CreationTime.f, &ExitTime.f, &KernelTime.f,
                       &UserTime.f)) {
    *now = *interval = 0; /* not available */
  } else {
    __int64 t1 = KernelTime.t - StartOfTimes_sys.t;
    __int64 t2 = KernelTime.t - last_time_sys.t;
    *now = (Int)(t1 / 10000);
    *interval = (Int)(t2 / 10000);
    last_time_sys.f.dwLowDateTime = KernelTime.f.dwLowDateTime;
    last_time_sys.f.dwHighDateTime = KernelTime.f.dwHighDateTime;
  }
}

#elif HAVE_TIMES

#if defined(_WIN32)

#include <time.h>

#define TicksPerSec CLOCKS_PER_SEC

#else

#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#endif

#if defined(__sun__) && (defined(__svr4__) || defined(__SVR4))

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#define TicksPerSec CLK_TCK
#endif

#if defined(__alpha) || defined(__FreeBSD__) || defined(__linux__) ||          \
    defined(__DragonFly__)

#if HAVE_TIME_H
#include <time.h>
#endif

#define TicksPerSec sysconf(_SC_CLK_TCK)

#endif

#if !TMS_IN_SYS_TIME
#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#endif

static clock_t StartOfTimes, last_time;

static clock_t StartOfTimes_sys, last_time_sys;

/* store user time in this variable */
static void InitTime(void) {
  struct tms t;
  times(&t);
  (*REMOTE_ThreadHandle(wid).last_timep) = StartOfTimes = t.tms_utime;
  last_time_sys = StartOfTimes_sys = t.tms_stime;
}

UInt Yap_cputime(void) {
  struct tms t;
  times(&t);
  return ((t.tms_utime - StartOfTimes) * 1000 / TicksPerSec);
}

void Yap_cputime_interval(Int *now, Int *interval) {
  struct tms t;
  times(&t);
  *now = ((t.tms_utime - StartOfTimes) * 1000) / TicksPerSec;
  *interval = (t.tms_utime - last_time) * 1000 / TicksPerSec;
  last_time = t.tms_utime;
}

void Yap_systime_interval(Int *now, Int *interval) {
  struct tms t;
  times(&t);
  *now = ((t.tms_stime - StartOfTimes_sys) * 1000) / TicksPerSec;
  *interval = (t.tms_stime - last_time_sys) * 1000 / TicksPerSec;
  last_time_sys = t.tms_stime;
}

#else /* HAVE_TIMES */

#ifdef SIMICS

#include <sys/time.h>

/* since the point YAP was started */
static struct timeval StartOfTimes;

/* since last call to runtime */
static struct timeval last_time;

/* store user time in this variable */
static void InitTime(int wid) {
  struct timeval tp;

  gettimeofday(&tp, NULL);
  (*REMOTE_ThreadHandle(wid).last_timep).f.tv_sec =
      (*REMOTE_ThreadHandle.start_of_timesp(wid)).tv_sec = tp.tv_sec;
  (*REMOTE_ThreadHandle(wid).last_timep).f.tv_usec =
      (*REMOTE_ThreadHandle.start_of_timesp(wid)).tv_usec = tp.tv_usec;
}

UInt Yap_cputime(void) {
  struct timeval tp;

  gettimeofday(&tp, NULL);
  if (StartOfTimes.tv_usec > tp.tv_usec)
    return ((tp.tv_sec - StartOfTimes.tv_sec - 1) * 1000 +
            (StartOfTimes.tv_usec - tp.tv_usec) / 1000);
  else
    return ((tp.tv_sec - StartOfTimes.tv_sec)) * 1000 +
           ((tp.tv_usec - StartOfTimes.tv_usec) / 1000);
}

void Yap_cputime_interval(Int *now, Int *interval) {
  struct timeval tp;

  gettimeofday(&tp, NULL);
  *now = (tp.tv_sec - StartOfTimes.tv_sec) * 1000 +
         (tp.tv_usec - StartOfTimes.tv_usec) / 1000;
  *interval = (tp.tv_sec - last_time.tv_sec) * 1000 +
              (tp.tv_usec - last_time.tv_usec) / 1000;
  last_time.tv_usec = tp.tv_usec;
  last_time.tv_sec = tp.tv_sec;
}

void Yap_systime_interval(Int *now, Int *interval) {
  *now = *interval = 0; /* not available */
}

#endif /* SIMICS */

#ifdef COMMENTED_OUT
/* This code is not working properly. I left it here to help future ports */
#ifdef MPW

#include <Events.h>
#include <files.h>

#define TicksPerSec 60.0

static double real_cputime() { return (((double)TickCount()) / TicksPerSec); }

#endif /* MPW */

#ifdef LATTICE

#include "osbind.h"

static long *ptime;

gettime() { *ptime = *(long *)0x462; }

static double real_cputime() {
  long thetime;
  ptime = &thetime;
  xbios(38, gettime);
  return (((double)thetime) / (Getrez() == 2 ? 70 : 60));
}

#endif /* LATTICE */

#ifdef M_WILLIAMS

#include <osbind.h>
#include <xbios.h>

static long *ptime;

static long readtime() { return (*((long *)0x4ba)); }

static double real_cputime() {
  long time;

  time = Supexec(readtime);
  return (time / 200.0);
}

#endif /* M_WILLIAMS */

#ifdef LIGHT

#undef FALSE
#undef TRUE

#include <FileMgr.h>

#define TicksPerSec 60.0

static double real_cputime() { return (((double)TickCount()) / TicksPerSec); }

#endif /* LIGHT */

#endif /* COMMENTED_OUT */

#endif /* HAVE_GETRUSAGE */

uint64_t Yap_StartOfWTimes;

#if HAVE_GETHRTIME

#if HAVE_TIME_H
#include <time.h>
#endif

/* since the point YAP was started */

void Yap_InitWTime(void) { Yap_StartOfWTimes = (uint64_t)gethrtime(); }

/// returns time since Jan 1 1980 in nano-seconds
uint64_t Yap_walltime(uint64_t old) {
  hrtime_t tp = gethrtime();
  /* return time in milliseconds */
  return (uint64_t)tp;
}

#elif HAVE_GETTIMEOFDAY

/* since the point YAP was started */
/* store user time in this variable */
void Yap_InitWTime(void) {
  struct timeval tp;

  gettimeofday(&tp, NULL);
  Yap_StartOfWTimes = timevalnanos(tp);
}

/// returns time in nano-secs since the epoch
uint64_t Yap_walltime(void) {
  struct timeval tp;

  gettimeofday(&tp, NULL);
  return  timevalnanos(tp);
}

#elif defined(_WIN32)

#include <sys/timeb.h>
#include <time.h>

/* since the point YAP was started */
static LARGE_INTEGER Frequency;

/* store user time in this variable */
void Yap_InitWTime(void) {
  LARGE_INTEGER ElapsedNanoseconds;
  QueryPerformanceFrequency(&Frequency);
  QueryPerformanceCounter(&ElapsedNanoseconds);
  ElapsedNanoseconds.QuadPart *= 1000000;
  ElapsedNanoseconds.QuadPart /= Frequency.QuadPart;
  Yap_StartOfWTimes = (uint64_t)ElapsedNanoseconds.QuadPart;
}

uint64_t Yap_walltime(void) {
  LARGE_INTEGER ElapsedNanoseconds;
  QueryPerformanceCounter(&ElapsedNanoseconds);
  //
  // We now have the elapsed number of ticks, along with the
  // number of ticks-per-second. We use these values
  // to convert to the number of elapsed microseconds.
  // To guard against loss-of-precision, we convert
  // to microseconds *before* dividing by ticks-per-second.
  //

  ElapsedNanoseconds.QuadPart *= 1000000;
  ElapsedNanoseconds.QuadPart /= Frequency.QuadPart;
  return ElapsedNanoseconds.QuadPart;
}

#elif HAVE_TIMES

/* store user time in this variable */
void Yap_InitWTime(void) {
  // start thread 0
  REMOTE_LastWTime(0) = Yap_StartOfWTimes =
      ((uint64_t)times(NULL)) * 10000000 / TicksPerSec;
}

uint64_t Yap_walltime(void) {
  clock_t t;
  t = times(NULL);
  return = ((uint64_t)times(NULL)) * 10000000 / TicksPerSec;
}

#endif /* HAVE_TIMES */
void Yap_ReInitWTime(void) { Yap_InitWTime(); }

void Yap_InitTimePreds(void) {
  /* can only do after heap is initialized */
  Yap_InitWTime();
}
