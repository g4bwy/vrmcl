#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

static void get_time(struct timespec *ts)
{
	memset(ts, 0, sizeof(*ts));
	clock_gettime(CLOCK_MONOTONIC, ts);
}

static double time_diff(struct timespec *start, struct timespec *end)
{
	struct timespec res;
	double ret;

	res.tv_sec = end->tv_sec - start->tv_sec;
	res.tv_nsec = end->tv_nsec - start->tv_nsec;
	if (res.tv_nsec < 0) {
		res.tv_sec--;
		res.tv_nsec += 1000000000;
	}

	ret = (double)res.tv_sec + (double)(res.tv_nsec / 1000000000.0f);

	return ret;
}

#ifdef TIMING_TEST

int main()
{
	struct timespec start, end;

	get_time(&start);
	sleep(1);
	get_time(&end);

	printf("duration=%f\n", time_diff(&start, &end));
}

#endif

