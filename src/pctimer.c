/* Win32 performance counter functions to get a high-resolution timer
 *
 * By Wu Yongwei
 *
 */

#include <stdio.h>
#include <pctimer.h>

#ifdef __MINGW32__
#include <windows.h>
#endif

#if defined(_WIN32) || defined(__CYGWIN__)

#ifndef _WIN32
#define PCTIMER_NO_WIN32
#endif /* _WIN32 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef PCTIMER_NO_WIN32
#undef PCTIMER_NO_WIN32
#undef _WIN32
#endif /* PCTIMER_NO_WIN32 */

pctimer_t pctimer(void)
{
    static LARGE_INTEGER pcount, pcfreq;
    static int initflag;

    if (!initflag)
    {
        QueryPerformanceFrequency(&pcfreq);
        initflag++;
    }

    QueryPerformanceCounter(&pcount);
    return (double)pcount.QuadPart / (double)pcfreq.QuadPart;
}

#else /* Win32/Cygwin */

#include <sys/time.h>

pctimer_t pctimer(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
}

#endif /* Win32/Cygwin */

