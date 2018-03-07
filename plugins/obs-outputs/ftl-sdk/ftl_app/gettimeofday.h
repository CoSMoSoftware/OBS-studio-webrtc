#include <stdint.h>

#ifdef _WIN32
int gettimeofday(struct timeval * tp, struct timezone * tzp);
#else
#include <sys/time.h>
#endif
int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y);
float timeval_to_ms(struct timeval *tv);
uint64_t timeval_to_us(struct timeval *tv);
