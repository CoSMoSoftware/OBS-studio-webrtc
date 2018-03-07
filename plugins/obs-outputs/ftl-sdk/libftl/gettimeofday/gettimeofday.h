#ifndef __GETTIMEOFDAY_H
#define __GETTIMEOFDAY_H

#include <stdint.h>

#ifndef _WIN32
#include <sys/time.h>
#endif

#ifdef _WIN32
int gettimeofday(struct timeval * tp, struct timezone * tzp);
#endif
int timeval_subtract(struct timeval *result, const struct timeval *end, const struct timeval *start);
int64_t timeval_subtract_to_ms(const struct timeval *end, const struct timeval *start);
int64_t timeval_subtract_to_us(const struct timeval *end, const struct timeval *start);
void timeval_add_ms(struct timeval *tv, int ms);
void timespec_add_ms(struct timespec *tv, int ms);
void timeval_add_us(struct timeval *tv, uint64_t us);
float timeval_to_ms(struct timeval *tv);
uint64_t timeval_to_us(struct timeval *tv);
uint64_t timeval_to_ntp(struct timeval *tv);
void us_to_timeval(struct timeval *outputTimeVal, const int64_t inputTimeUs);
int64_t get_ms_elapsed_since(struct timeval *tv);

#endif // __GETTIMEOFDAY_H
