/*
 * This file is part of the profiler project
 *
 * (C) 2019 Andreas Steinmetz, ast@domdv.de
 * The contents of this file is licensed under the GPL version 2 or, at
 * your choice, any later version of this license.
 */

#include <stdio.h>

extern int library_function(void);

int main(int argc,char *argv[])
{
	int sum;

	sum=library_function();

	printf("sum=%d\n",sum);

	return 0;
}
