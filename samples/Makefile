# This file is part of the macsecd project
# 
# (C) 2019 Andreas Steinmetz, ast@domdv.de
# The contents of this file is licensed under the GPL version 2 or, at
# your choice, any later version of this license.
#
CFLAGS=-Wall -O3
INSTFILES1=/bits/,alloca.h,complex.h,ctype.h,libintl.h,math.h,string.h,
INSTFILES2=strings.h,tgmath.h,wchar.h
INSTFUNCS=__get_cpuid_max
CFLAGS+=-g -finstrument-functions
CFLAGS+=-finstrument-functions-exclude-file-list=$(INSTFILES1),$(INSTFILES2)
CFLAGS+=-finstrument-functions-exclude-function-list=$(INSTFUNCS)
#
# You can run the samples with the adjustment calue from profadj by calling
# make ADJUST=<value>
#
ifdef ADJUST
ADJ=-g $(ADJUST)
else
ADJ=
endif

all: single-threaded multi-threaded single-constant-calls multi-constant-calls \
	library.so libcaller

single-threaded: single-threaded.c ../profiler.h
	gcc $(CFLAGS) -o single-threaded single-threaded.c

multi-threaded: multi-threaded.c ../profiler.h
	gcc $(CFLAGS) -o multi-threaded multi-threaded.c -lpthread

single-constant-calls: single-constant-calls.c ../profiler.h
	gcc $(CFLAGS) -o single-constant-calls single-constant-calls.c

multi-constant-calls: multi-constant-calls.c ../profiler.h
	gcc $(CFLAGS) -o multi-constant-calls multi-constant-calls.c -lpthread

library.so: library.c ../profiler.h
	gcc $(CFLAGS) -fPIC -shared -o library.so library.c

libcaller: libcaller.c
	gcc -Wall -O3 -Wl,-rpath,`pwd` -o libcaller libcaller.c -L. -lrary

single-threaded-profile: single-threaded
	env PROFILE_LOG_FILE=single-threaded.out ./single-threaded
	../profiler -i single-threaded.out $(ADJ) -scCaAS

multi-threaded-profile: multi-threaded
	env PROFILE_LOG_FILE=multi-threaded.out ./multi-threaded
	../profiler -i multi-threaded.out $(ADJ) -scCaAStTwW

single-constant-calls-profile: single-constant-calls
	env PROFILE_LOG_FILE=single-constant-calls-profile.out \
		./single-constant-calls
	../profiler -i single-constant-calls-profile.out $(ADJ) -scCaAS

multi-constant-calls-profile: multi-constant-calls
	env PROFILE_LOG_FILE=multi-constant-calls-profile.out \
		./multi-constant-calls
	../profiler -i multi-constant-calls-profile.out $(ADJ) -scCaAStTwW

libcaller-profile: libcaller library.so
	env PROFILE_LOG_FILE=libcaller.out ./libcaller
	../profiler -i libcaller.out $(ADJ) -scCaAS

clean:
	rm -f single-threaded single-threaded.out multi-threaded \
		multi-threaded.out single-constant-calls \
		single-constant-calls-profile.out multi-constant-calls \
		multi-constant-calls-profile.out library.so libcaller \
		libcaller.out
