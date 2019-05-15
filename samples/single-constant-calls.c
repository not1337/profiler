/*
 * This file is part of the profiler project
 *
 * (C) 2019 Andreas Steinmetz, ast@domdv.de
 * The contents of this file is licensed under the GPL version 2 or, at
 * your choice, any later version of this license.
 */

#include <stdio.h>

#include "../profiler.h"

static int routine3(int value)
{
	return (value+483)%33;
}

static int routine2(int value)
{
	int i;

	for(i=0;i<0xfff;i++)value+=routine3(value);
	return value;
}

static int routine1(int value)
{
	int i;

	for(i=0;i<0xff;i++)value+=routine2(value);
	return value;
}

int main(int argc,char *argv[])
{
	int i;
	int sum=0;

	for(i=0;i<0xf;i++)sum+=routine1(i+99);

	printf("sum=%d\n",sum);

	return 0;
}
