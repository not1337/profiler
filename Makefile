# This file is part of the macsecd project
# 
# (C) 2019 Andreas Steinmetz, ast@domdv.de
# The contents of this file is licensed under the GPL version 2 or, at
# your choice, any later version of this license.
#
CFLAGS=-O3 -Wall -s

all: profiler profadj

profiler: profiler.c
	gcc $(CFLAGS) -o profiler profiler.c

profadj: profadj.c
	gcc $(CFLAGS) -o profadj profadj.c -lpthread

clean:
	rm -f profiler profadj
