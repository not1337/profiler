/*
 * This file is part of the profiler project
 *
 * (C) 2019 Andreas Steinmetz, ast@domdv.de
 * The contents of this file is licensed under the GPL version 2 or, at
 * your choice, any later version of this license.
 */

#include <pthread.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef SYS_clock_gettime
#define gettime(a,b)	syscall(SYS_clock_gettime,a,b)
#else
#warning "using clock_gettime syscall, this is slower"
#define gettime(a,b)	clock_gettime(a,b)
#endif

static int itermeasure(void)
{
	int i;
	int j=1;
	struct timespec start;
	struct timespec end;
	struct timespec unused;
	unsigned long long val;

	while(1)
	{
		gettime(CLOCK_THREAD_CPUTIME_ID,&start);

		for(i=0;i<j;i++)
		{
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
		}

		gettime(CLOCK_THREAD_CPUTIME_ID,&end);

		end.tv_sec-=start.tv_sec;
		if((end.tv_nsec-=start.tv_nsec)<0)
		{
			end.tv_nsec+=1000000000;
			end.tv_sec--;
		}
		val=((unsigned long long)end.tv_sec)*1000000000ULL+
			((unsigned long long)end.tv_nsec);

		if(val>=1000000000)break;

		j=j*10;
	}

	return j;
}

static void callmeasure(int iter,unsigned long long *c)
{
	int i;
	int j=1;
	struct timespec start;
	struct timespec end;
	struct timespec unused;

	*c=0;

	while(1)
	{
		gettime(CLOCK_THREAD_CPUTIME_ID,&start);

		for(i=0;i<iter;i++)
		{
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
			gettime(CLOCK_THREAD_CPUTIME_ID,&unused);
		}

		gettime(CLOCK_THREAD_CPUTIME_ID,&end);

		end.tv_sec-=start.tv_sec;
		if((end.tv_nsec-=start.tv_nsec)<0)
		{
			end.tv_nsec+=1000000000;
			end.tv_sec--;
		}
		*c+=((unsigned long long)end.tv_sec)*1000000000ULL+
			((unsigned long long)end.tv_nsec);

		if(*c>=30000000000)break;

		j++;
		usleep(500000);
	}

	*c/=(unsigned long long)iter*100ULL*j;
}

int main(int argc,char *argv[])
{
	int i;
	int iter;
	unsigned long long cm[7];
	unsigned long long v;

	printf("This will take about 5 minutes. The system should be mostly "
		"idle.\n");
	usleep(5000000);

	printf("Estimating iterations...\n");
	iter=itermeasure();
	usleep(5000000);

	printf("Measuring clock_gettime correction...\n");
	for(i=0;i<7;i++)
	{
		callmeasure(iter,&cm[i]);
		usleep(5000000);
	}

	for(v=cm[0],i=0;i<7;i++)if(cm[i]<v)v=cm[i];

	printf("The clock_gettime correction in nanoseconds is: %llu\n",v);

	printf("Note that you may have to adjust the above value by a few "
		"nanoseconds\nfor more precise profiling output.\n");

	return 0;
}
