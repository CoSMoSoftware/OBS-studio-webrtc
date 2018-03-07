/*
* gettimeofday.c
*    Win32 gettimeofday() replacement
*
* src/port/gettimeofday.c
*
* Copyright (c) 2003 SRA, Inc.
* Copyright (c) 2003 SKC, Inc.
*
* Permission to use, copy, modify, and distribute this software and
* its documentation for any purpose, without fee, and without a
* written agreement is hereby granted, provided that the above
* copyright notice and this paragraph and the following two
* paragraphs appear in all copies.
*
* IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT,
* INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
* LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
* DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*
* THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS
* IS" BASIS, AND THE AUTHOR HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE,
* SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
*/

#include <stddef.h>
#include "gettimeofday.h"

#define NSEC_IN_SEC 1000000000
#define USEC_IN_SEC 1000000
#define MSEC_IN_SEC 1000
#define MSEC_IN_USEC 1000
#define MSEC_IN_NSEC 1000000

#define USEC_TO_SEC(x) ((x) / USEC_IN_SEC)
#define USEC_TO_MSEC(x) ((x) / MSEC_IN_USEC)
#define MSEC_TO_SEC(x) ((x) / MSEC_IN_SEC)
#define MSEC_TO_USEC(x) (((uint64_t)x) * MSEC_IN_USEC)
#define MSEC_TO_NSEC(x) (((uint64_t)x) * MSEC_IN_NSEC)
#define SEC_TO_USEC(x) (((uint64_t)x) * USEC_IN_SEC)
#define SEC_TO_NSEC(x) (((uint64_t)x) * NSEC_IN_SEC)

#ifdef _WIN32
#include <Windows.h>

/* FILETIME of Jan 1 1970 00:00:00. */
static const unsigned __int64 epoch = ((unsigned __int64)116444736000000000ULL);

/*
* timezone information is stored outside the kernel so tzp isn't used anymore.
*
* Note: this function is not for Win32 high precision timing purpose. See
* elapsed_time().
*/
int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
  FILETIME    file_time;
  SYSTEMTIME  system_time;
  ULARGE_INTEGER ularge;

  GetSystemTime(&system_time);
  SystemTimeToFileTime(&system_time, &file_time);
  ularge.LowPart = file_time.dwLowDateTime;
  ularge.HighPart = file_time.dwHighDateTime;

  tp->tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
  tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

  return 0;
}
#else
void timespec_add_ms(struct timespec *ts, int ms) {
  long ns_adjust;
  time_t sec_adjust;

  sec_adjust = MSEC_TO_SEC((time_t)ms);
  ns_adjust = MSEC_TO_NSEC((long)ms);

  ns_adjust -= SEC_TO_NSEC((long)sec_adjust);

  ts->tv_sec += sec_adjust;
  ts->tv_nsec += ns_adjust;

  if(ts->tv_nsec >= NSEC_IN_SEC) {
    ts->tv_nsec -= NSEC_IN_SEC;
    ts->tv_sec++;
  }
}
#endif // _WIN32

//result = end - start
int timeval_subtract(struct timeval *result, const struct timeval *end, const struct timeval *start)
{
  int64_t differenceUs;

  differenceUs = timeval_subtract_to_us(end, start);

  // Get the number of seconds
  result->tv_sec = (long)USEC_TO_SEC(differenceUs);
  
  // Put the remainder NS into the second value.
  result->tv_usec = (long)(differenceUs - SEC_TO_USEC(result->tv_sec));

  /* Return 1 if result is negative. */
  return differenceUs < 0;
}

//result = end - start
int64_t timeval_subtract_to_ms(const struct timeval *end, const struct timeval *start)
{
  return USEC_TO_MSEC(timeval_subtract_to_us(end, start));
}

int64_t timeval_subtract_to_us(const struct timeval *end, const struct timeval *start)
{
    int64_t s, e, d;

    s = (int64_t)SEC_TO_USEC(start->tv_sec) + (int64_t)start->tv_usec;
    e = (int64_t)SEC_TO_USEC(end->tv_sec) + (int64_t)end->tv_usec;

    d = e - s;

    return d;
}

void timeval_add_ms(struct timeval *tv, int ms)
{
  timeval_add_us(tv, MSEC_TO_USEC(ms));
}

void timeval_add_us(struct timeval *tv, uint64_t us)
{
  struct timeval add_timeval;
  us_to_timeval(&add_timeval, us);

  tv->tv_sec += add_timeval.tv_sec;
  tv->tv_usec += add_timeval.tv_usec;

  if (tv->tv_usec >= USEC_IN_SEC) {
    tv->tv_usec -= USEC_IN_SEC;
    tv->tv_sec++;
  }
}

float timeval_to_ms(struct timeval *tv) {
  double sec, usec;

  sec = (double)tv->tv_sec;
  usec = (double)tv->tv_usec;

  return (float)(sec * 1000 + usec / 1000);
}

uint64_t timeval_to_us(struct timeval *tv)
{
  return tv->tv_sec * (uint64_t)1000000 + tv->tv_usec;
}

uint64_t timeval_to_ntp(struct timeval * tv) {
  uint64_t ntpts;

  ntpts = (uint64_t)((((uint64_t)tv->tv_sec + 2208988800u) << 32) + ((uint32_t)tv->tv_usec * 4294.967296));

  return (ntpts);
}

void us_to_timeval(struct timeval *outputTimeVal, const int64_t inputTimeUs)
{
  outputTimeVal->tv_sec = (long)USEC_TO_SEC(inputTimeUs);
  outputTimeVal->tv_usec = (long)(inputTimeUs - SEC_TO_USEC(outputTimeVal->tv_sec));
}

int64_t get_ms_elapsed_since(struct timeval *tv)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return timeval_subtract_to_ms(&now, tv);
}
