/*
 * This file is part of the profiler project
 *
 * (C) 2019 Andreas Steinmetz, ast@domdv.de
 * The contents of this file is licensed under the GPL version 2 or, at
 * your choice, any later version of this license.
 */

#include <stdlib.h>
#include <time.h>

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

int library_function(void)
{
	int i;
	int sum=0;
	int value;

	srandom(time(NULL));

	value=routine1();
	value&=0x3fffff;

	for(i=0;i<value;i++)sum+=routine2(value);
	sum+=routine3();

	return sum;
}
