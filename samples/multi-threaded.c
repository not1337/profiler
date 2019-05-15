/*
 * This file is part of the profiler project
 *
 * (C) 2019 Andreas Steinmetz, ast@domdv.de
 * The contents of this file is licensed under the GPL version 2 or, at
 * your choice, any later version of this license.
 */

#include <pthread.h>
#include <stdio.h>

#include "../profiler.h"

static int routine1(void)
{
	return (int)random();
}

static int routine2(int value)
{
	int i;
	int j=0;

	value&=0x3fffff;
	for(i=0;i<value;i++)j=i%value;
	return j;
}

static int routine3(void)
{
	int value;

	value=routine1();
	return routine2(value);
}

static void *thread2(void *arg)
{
	int i;
	int sum=0;
	int value;
 
	value=routine1();
	value&=0x3fffff;

	for(i=0;i<value;i++)sum+=routine2(value);
	sum+=routine3();

	printf("sum2=%d\n",sum);

	return NULL;
}

static void *thread1(void *arg)
{
	int i;
	int sum=0;
	int value;
 
	value=routine1();
	value&=0x3fffff;

	for(i=0;i<value;i++)sum+=routine2(value);
	sum+=routine3();

	printf("sum1=%d\n",sum);

	pthread_exit(NULL);
}

int main(int argc,char *argv[])
{
	pthread_t t1;
	pthread_t t2;

	srandom(time(NULL));

	if(!(pthread_create(&t1,NULL,thread1,NULL)))
	{
		if(!(pthread_create(&t2,NULL,thread2,NULL)))
		{
			pthread_join(t2,NULL);
		}

		if(!(pthread_create(&t2,NULL,thread2,NULL)))
		{
			pthread_join(t2,NULL);
		}

		pthread_join(t1,NULL);
	}

	return 0;
}
