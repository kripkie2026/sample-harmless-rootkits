#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <time.h>
#include <sys/time.h>

/* Helper: get the original function pointer once */
static void *get_real(const char *name) {
    void *f = dlsym(RTLD_NEXT, name);
    if (!f) {
        fprintf(stderr, "Error: %s\n", dlerror());
        _exit(1);
    }
    return f;
}

/* Compute today’s 13:37:00 local time as a time_t epoch */
static time_t epoch_1337() {
    typedef time_t (*real_time_t)(time_t*);
    static real_time_t real_time = NULL;
    if (!real_time) real_time = (real_time_t) get_real("time");

    // get current real time
    time_t now = real_time(NULL);

    // break down into local date/time
    typedef struct tm* (*real_localtime_r_t)(const time_t*, struct tm*);
    static real_localtime_r_t real_localtime_r = NULL;
    if (!real_localtime_r) real_localtime_r = (real_localtime_r_t) get_real("localtime_r");

    struct tm tm;
    real_localtime_r(&now, &tm);

    // force time to 13:37:00
    tm.tm_hour = 13;
    tm.tm_min  = 37;
    tm.tm_sec  = 0;
    tm.tm_isdst = -1;   // let mktime figure out DST

    typedef time_t (*real_mktime_t)(struct tm*);
    static real_mktime_t real_mktime = NULL;
    if (!real_mktime) real_mktime = (real_mktime_t) get_real("mktime");

    return real_mktime(&tm);
}

/* ----- overridden functions ----- */

time_t time(time_t *t) {
    time_t e = epoch_1337();
    if (t) *t = e;
    return e;
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
    typedef int (*real_gettimeofday_t)(struct timeval*, struct timezone*);
    static real_gettimeofday_t real_gettimeofday = NULL;
    if (!real_gettimeofday) real_gettimeofday = (real_gettimeofday_t) get_real("gettimeofday");

    if (tv) {
        tv->tv_sec  = epoch_1337();
        tv->tv_usec = 0;
    }
    if (tz) {
        // pass through the real timezone (we don't touch it)
        return real_gettimeofday(NULL, tz);
    }
    return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    typedef int (*real_clock_gettime_t)(clockid_t, struct timespec*);
    static real_clock_gettime_t real_clock_gettime = NULL;
    if (!real_clock_gettime) real_clock_gettime = (real_clock_gettime_t) get_real("clock_gettime");

    if (clk_id == CLOCK_REALTIME && tp) {
        tp->tv_sec  = epoch_1337();
        tp->tv_nsec = 0;
        return 0;
    }
    // other clocks are passed through unchanged
    return real_clock_gettime(clk_id, tp);
}
