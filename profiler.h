/*
 * This file is part of the profiler project
 *
 * (C) 2019 Andreas Steinmetz, ast@domdv.de
 * The contents of this file is licensed under the GPL version 2 or, at
 * your choice, any later version of this license.
 *
 * Exemption: If this file is referenced in a source file of a product
 * but during regular product build inclusion of this file is disabled
 * by preprocessor directives and if this file is not shipped in any
 * form (source or binary) with the product then this file is not part
 * of the shipped product and thus the GPL license does not apply to the
 * shipped product. Any kind of circumvention, e.g. not shipping this file
 * but requiring the product recipient to download and insert this file,
 * does void this exemption.
 */

#ifndef _PROFILE_INSTRUMENTATION_H
#define _PROFILE_INSTRUMENTATION_H

/*
 * To enable profiling include this file once (if using threads include
 * after including pthread.h) and compile everything with:
 *
 * -g -finstrument-functions -finstrument-functions-exclude-file-list=/bits/,alloca.h,complex.h,ctype.h,libintl.h,math.h,string.h,strings.h,tgmath.h,wchar.h -finstrument-functions-exclude-function-list=__get_cpuid_max
 *
 * The above excludes are inlined compiler builtins (x86_64) that otherwise
 * would get un-inlined and instrumented. You may need to adapt or enhance
 * the exclusion lists to your requirements.
 *
 * Options passed via environment:
 *
 * PROFILE_LOG_FILE	instrumentation file, default "instrumentation.out"
 * PROFILE_STACK_SIZE	maximum instrumentation stack, default 100
 * PROFILE_FUNC_POOL	elements in function pool, default 1000
 * PROFILE_CALLER_POOL	elements in function caller pool, default 5000
 * PROFILE_DAEMON	write instrumentation only for child if set,
 *                      otherwise write instrumentation only for parent
 * PROFILE_DISABLE      disable profiling completely except for compiled in
 *                      stub calls.
 *
 * The instrumentation file gets the profiling data written to when the
 * executable terminates.
 * The instrumentation stack is required for time keeping and each element
 * represents one call depth level. There is one instrumentation stack per
 * thread.
 * The function pool is the maximum amount of different functions that
 * can be instrumented.
 * The caller pool is the maximum amount of different function callers
 * prt function that can be instrumented.
 * If any of the above limits would be exceeded the whole profiling will
 * fail. This failure may cause profiler memory leaks.
 *
 * Important: If longjmp or siglongjmp are called this code will utterly
 * fail. You have been warned. Do not profile beyond setjmp/sigsetjmp.
 * If you call _exit or _Exit profiling will fail, too.
 * If you call exit while threads are active you will lose some or all
 * cpu usage information for the call stack of every thread.
 * The same is true for terminatimg signals.
 * Using pthread_exit will add some CPU usage imprecision as there is no
 * usable profiling hook and one has to rely on standard pthread stuff.
 *
 * If you don't want to profile some function you may add it to the
 * -finstrument-functions-exclude-function-list option of gcc or you
 * can use __attribute__((no_instrument_function)) in the function
 * declaration.
 *
 * Below are some options you can define before including this header:
 *
 * Define, if your platform doesn't support thread local
 * storage (only for pthreads, causes minimal slower code):
 *
 * #define PROFILE_NO_TLS
 *
 * Define, if your platform doesn't support the required
 * atomic operations (only for pthreads, causes slower code):
 *
 * #define PROFILE_NO_ATOMICS
 *
 * Define, if you want strict error checks for calls that
 * usually do not fail (causes minimal slower code):
 *
 * #define PROFILE_STRICT
 */

#include <sys/types.h>
#include <sys/resource.h>
#include <syscall.h>
#include <sched.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define PROFILE_THREAD_TABLE_SIZE	64
#define PROFILE_FUNC_TABLE_SIZE		64

#if __SIZEOF_LONG__ > 4
#define PROFILE_CALLER_TABLE_SIZE	8
#else
#define PROFILE_CALLER_TABLE_SIZE	16
#endif

#define profile_nsecs(a) (((unsigned long long)(a).tv_sec)*1000000000ULL+\
	((unsigned long long)(a).tv_nsec))

#define profile_usecs(a) (((unsigned long long)(a).tv_sec)*1000000000ULL+\
	((unsigned long long)(a).tv_usec)*1000ULL)

#ifdef SYS_clock_gettime
#define profile_gettime(a,b)	syscall(SYS_clock_gettime,a,b)
#else
#warning "profiling not using clock_gettime syscall, this is slower"
#define profile_gettime(a,b)	clock_gettime(a,b)
#endif

#ifdef SYS_sched_yield
#define profile_yield()		syscall(SYS_sched_yield)
#else
#warning "profiling not using sched_yield syscall, this is slower"
#define profile_yield()		sched_yield()
#endif

#define profile_deltatime(a,b)					\
do								\
{								\
	(a).tv_sec-=(b).tv_sec;					\
	if(((a).tv_nsec-=(b).tv_nsec)<0)			\
	{							\
		(a).tv_nsec+=1000000000;			\
		(a).tv_sec--;					\
	}							\
}								\
while(0)

#ifndef PROFILE_NO_ATOMICS

#define lock(a)							\
do								\
{								\
__label__ done;							\
	int z=0;						\
								\
	if(__builtin_expect(!__atomic_compare_exchange_n((&a),	\
		&z,1,1,__ATOMIC_SEQ_CST,__ATOMIC_RELAXED),0))	\
	{							\
		int i=1024;					\
								\
		while(1)					\
		{						\
			if(__atomic_compare_exchange_n((&a),	\
				&z,1,1,__ATOMIC_SEQ_CST,	\
				__ATOMIC_RELAXED))goto done;	\
			if(--i<0)				\
			{					\
				profile_yield();		\
				i=1024;				\
			}					\
		}						\
	}							\
done:;								\
}								\
while(0)

#define unlock(a)						\
do								\
{								\
	__atomic_store_n(&(a),0,__ATOMIC_SEQ_CST);		\
}								\
while(0)

#else

#define lock(a)		pthread_mutex_lock(&(a))
#define unlock(a)	pthread_mutex_unlock(&(a))

#endif

typedef struct profile_caller
{
	union
	{
		struct
		{
			struct profile_caller *left;
			struct profile_caller *right;
			void *caller;
			unsigned long long calls;
			unsigned long long nsecs;
			unsigned long long secs;
			unsigned long long calling;
			unsigned int unwind;
		};
		unsigned char align[64];
	};
} PROFILE_CALLER;

typedef struct profile_func
{
	union
	{
		struct
		{
			struct profile_func *left;
			struct profile_func *right;
			void *func;
			PROFILE_CALLER *caller[PROFILE_CALLER_TABLE_SIZE];
			unsigned long long calls;
			unsigned long long funcs;
			unsigned long long nsecs;
			unsigned long long secs;
			unsigned int unwind;
			unsigned int depth;
		};
		unsigned char align[128];
	};
} PROFILE_FUNC;

typedef struct
{
	union
	{
		struct
		{
			PROFILE_FUNC *e;
			PROFILE_CALLER *c;
			struct timespec used;
		};
		unsigned char align[32];
	};
} PROFILE_STACK;

typedef struct profile_thread
{
	union
	{
		struct
		{
#ifdef _PTHREAD_H
			struct profile_thread *next;
			int table_index;
#endif
			int stack_index;
			unsigned int unwind;
			unsigned int depth;
			unsigned long long funcs;
			unsigned long long nsecs;
			unsigned long long secs;
			struct timespec start_time;
		};
		unsigned char align[64];
	};
	PROFILE_STACK stack[0];
} PROFILE_THREAD;

#if defined(_PTHREAD_H) && !defined(PROFILE_NO_TLS)
static __thread PROFILE_THREAD *profile_thread;
#elif !defined(_PTHREAD_H)
static PROFILE_THREAD *profile_thread;
#endif
static PROFILE_FUNC *profile_root[PROFILE_FUNC_TABLE_SIZE];
static PROFILE_FUNC *profile_func_alloc;
static PROFILE_CALLER *profile_caller_alloc;
static int profile_numthreads;
static int profile_fpool_limit;
static int profile_fpool_used;
static int profile_cpool_limit;
static int profile_cpool_used;
static int profile_stack_limit;
static int profile_error;
static int profile_thread_size;
static int profile_func_exhausted;
static int profile_caller_exhausted;
static int profile_stack_exhausted;
static int profile_time_error;
static int profile_disabled;
static int profile_daemon;
static int profile_pid;
static char *profile_log_file;
static struct timespec profile_process_time;

#ifdef _PTHREAD_H

static PROFILE_THREAD *profile_thread_table[PROFILE_THREAD_TABLE_SIZE];
static pthread_key_t profile_key;
static int profile_table_next;
static int profile_maxthreads;
#ifndef PROFILE_NO_ATOMICS
static int profile_mutex;
static int profile_mutex2;
#else
static pthread_mutex_t profile_mutex=PTHREAD_MUTEX_INITIALIZER;
#endif

#endif

static void __attribute__((no_instrument_function))
	__attribute__((no_sanitize_address))
	__attribute__((no_sanitize_thread))
	__attribute__((no_sanitize_undefined))
	__attribute__((no_profile_instrument_function))
	__attribute__((no_stack_limit))
	__attribute__((optimize("no-stack-protector")))
	__attribute__((optimize("omit-frame-pointer")))
	__attribute__((optimize("Os")))
	profile_stack_unwind(PROFILE_THREAD *tt,int mode)
{
	PROFILE_STACK *p=&tt->stack[tt->stack_index];

	for(;tt->stack_index;tt->stack_index--,p--)
	{
#if defined(_PTHREAD_H) && !defined(PROFILE_NO_ATOMICS)
		__atomic_add_fetch(&p->c->secs,p->used.tv_sec,__ATOMIC_RELAXED);
		__atomic_add_fetch(&p->c->nsecs,p->used.tv_nsec,
			__ATOMIC_RELAXED);
		if(!mode)__atomic_add_fetch(&p->c->unwind,1,__ATOMIC_RELAXED);

		tt->secs+=p->used.tv_sec;
		tt->nsecs+=p->used.tv_nsec;
		if(!mode)tt->unwind++;

		if(tt->stack_index==1)
		{
#if defined(_PTHREAD_H) && !defined(PROFILE_NO_ATOMICS)
			unsigned int depth;
#endif

			__atomic_add_fetch(&p->e->calls,1,__ATOMIC_RELAXED);
			__atomic_add_fetch(&p->e->secs,tt->secs,
				__ATOMIC_RELAXED);
			__atomic_add_fetch(&p->e->nsecs,tt->nsecs,
				__ATOMIC_RELAXED);
			__atomic_add_fetch(&p->e->funcs,tt->funcs,
				__ATOMIC_RELAXED);
			__atomic_add_fetch(&p->e->unwind,tt->unwind,
				__ATOMIC_RELAXED);
repeat:			depth= __atomic_load_n(&p->e->depth,__ATOMIC_SEQ_CST);
			if(tt->depth>depth)if(__builtin_expect(
				!__atomic_compare_exchange_n(&p->e->depth,
					&depth,tt->depth,1,__ATOMIC_SEQ_CST,
					__ATOMIC_RELAXED),0))goto repeat;
		}
#else
		p->c->secs+=p->used.tv_sec;
		p->c->nsecs+=p->used.tv_nsec;
		if(!mode)p->c->unwind++;

		tt->secs+=p->used.tv_sec;
		tt->nsecs+=p->used.tv_nsec;
		if(!mode)tt->unwind++;

		if(tt->stack_index==1)
		{
			p->e->calls++;
			p->e->secs+=tt->secs;
			p->e->nsecs+=tt->nsecs;
			p->e->funcs+=tt->funcs;
			p->e->unwind+=tt->unwind;
			if(tt->depth>p->e->depth)p->e->depth=tt->depth;
		}
#endif
	}
}

#ifdef _PTHREAD_H

static void __attribute__((no_instrument_function))
	__attribute__((no_sanitize_address))
	__attribute__((no_sanitize_thread))
	__attribute__((no_sanitize_undefined))
	__attribute__((no_profile_instrument_function))
	__attribute__((no_stack_limit))
	__attribute__((optimize("no-stack-protector")))
	__attribute__((optimize("omit-frame-pointer")))
	__attribute__((optimize("Os")))
	profile_thread_cleaner(void *ptr)
{
	int mode=0;
	PROFILE_THREAD **t;
	PROFILE_THREAD *tt=ptr;
	struct timespec stamp;

	if(__builtin_expect(!profile_error,1)&&tt->stack_index)
	{
		if(tt->stack_index==1)
		{
#ifdef PROFILE_STRICT
			if(__builtin_expect(
				profile_gettime(CLOCK_THREAD_CPUTIME_ID,
					&stamp),0))
			{
				profile_time_error=1;
				profile_error=1;
				return;
			}
#else
			profile_gettime(CLOCK_THREAD_CPUTIME_ID,&stamp);
#endif
			profile_deltatime(stamp,tt->start_time);

			tt->stack[tt->stack_index].used.tv_nsec+=stamp.tv_nsec;
			tt->stack[tt->stack_index].used.tv_sec+=stamp.tv_sec;
			mode=1;
		}

#ifdef PROFILE_NO_ATOMICS
		lock(profile_mutex);
#endif
		profile_stack_unwind(tt,mode);
#ifdef PROFILE_NO_ATOMICS
		unlock(profile_mutex);
#endif

		for(t=&profile_thread_table[tt->table_index];*t;t=&(*t)->next)
			if(*t==tt)
		{
			*t=tt->next;
			break;
		}

		free(tt);
#ifndef PROFILE_NO_TLS
		profile_thread=NULL;
#endif
	}
#ifndef PROFILE_NO_ATOMICS
	__atomic_sub_fetch(&profile_numthreads,1,__ATOMIC_SEQ_CST);
#else
	lock(profile_mutex);
	profile_numthreads--;
	unlock(profile_mutex);
#endif
}

#endif

void __attribute__ ((constructor)) __attribute__((no_instrument_function))
	__attribute__((no_sanitize_address))
	__attribute__((no_sanitize_thread))
	__attribute__((no_sanitize_undefined))
	__attribute__((no_profile_instrument_function))
	__attribute__((no_stack_limit))
	__attribute__((optimize("no-stack-protector")))
	__attribute__((optimize("omit-frame-pointer")))
	__attribute__((optimize("Os")))
	__attribute__((cold)) profile_init(void)
{
	char *p;

	if(getenv("PROFILE_DISABLE"))
	{
		profile_disabled=1;
		goto err1;
	}

	if(getenv("PROFILE_DAEMON"))profile_daemon=1;

	if(!(p=getenv("PROFILE_FUNC_POOL")))profile_fpool_limit=1000;
	else if((profile_fpool_limit=atoi(p))<=0)profile_fpool_limit=1000;

	if(!(p=getenv("PROFILE_CALLER_POOL")))profile_cpool_limit=5000;
	else if((profile_cpool_limit=atoi(p))<=0)profile_cpool_limit=5000;

	if(!(p=getenv("PROFILE_STACK_SIZE")))profile_stack_limit=100;
	else if((profile_stack_limit=atoi(p))<=0)profile_stack_limit=100;
	profile_thread_size=++profile_stack_limit*sizeof(PROFILE_STACK)+
		sizeof(PROFILE_THREAD);

	if(!(profile_log_file=getenv("PROFILE_LOG_FILE")))
		profile_log_file="instrumentation.out";

	if(__builtin_expect(!(profile_func_alloc=
		malloc(profile_fpool_limit*sizeof(PROFILE_FUNC))),0))
	{
		profile_func_exhausted=1;
		goto err1;
	}

	memset(profile_func_alloc,0,profile_fpool_limit*sizeof(PROFILE_FUNC));

	if(__builtin_expect(!(profile_caller_alloc=
		malloc(profile_cpool_limit*sizeof(PROFILE_CALLER))),0))
	{
		profile_caller_exhausted=1;
		goto err2;
	}

	memset(profile_caller_alloc,0,
		profile_cpool_limit*sizeof(PROFILE_CALLER));

#ifdef _PTHREAD_H
	if(__builtin_expect(pthread_key_create(&profile_key,
		profile_thread_cleaner),0))goto err3;
#endif

	if(!profile_pid)profile_pid=getpid();

	if(__builtin_expect(clock_gettime(CLOCK_MONOTONIC,
		&profile_process_time),0))
	{
		profile_time_error=1;
#ifdef _PTHREAD_H
err3:
#endif
		free(profile_caller_alloc);
		profile_caller_alloc=NULL;
err2:		free(profile_func_alloc);
		profile_func_alloc=NULL;
err1:		profile_error=1;
	}
}

static void __attribute__((no_instrument_function)) __attribute__((cold))
	__attribute__((no_sanitize_address))
	__attribute__((no_sanitize_thread))
	__attribute__((no_sanitize_undefined))
	__attribute__((no_profile_instrument_function))
	__attribute__((no_stack_limit))
	__attribute__((optimize("no-stack-protector")))
	__attribute__((optimize("omit-frame-pointer")))
	__attribute__((optimize("Os")))
	profile_caller_walk(PROFILE_CALLER *f,void *func,FILE *fp)
{
	if(f->left)profile_caller_walk(f->left,func,fp);
	if(f->right)profile_caller_walk(f->right,func,fp);
	fprintf(fp,"TRACE: %p %p %llu %llu %llu %u\n",func,f->caller,f->calls,
		f->secs*1000000000ULL+f->nsecs,f->calling,f->unwind);
}

static void __attribute__((no_instrument_function)) __attribute__((cold))
	__attribute__((no_sanitize_address))
	__attribute__((no_sanitize_thread))
	__attribute__((no_sanitize_undefined))
	__attribute__((no_profile_instrument_function))
	__attribute__((no_stack_limit))
	__attribute__((optimize("no-stack-protector")))
	__attribute__((optimize("omit-frame-pointer")))
	__attribute__((optimize("Os")))
	profile_func_walk(PROFILE_FUNC *f,FILE *fp)
{
	int i;

	if(f->left)profile_func_walk(f->left,fp);
	if(f->right)profile_func_walk(f->right,fp);
	for(i=0;i<PROFILE_CALLER_TABLE_SIZE;i++)if(f->caller[i])
		profile_caller_walk(f->caller[i],f->func,fp);
	if(f->calls)fprintf(fp,"THREAD: %p %llu %llu %llu %u %u\n",f->func,
		f->calls,f->secs*1000000000ULL+f->nsecs,f->funcs,
		f->unwind,f->depth);
}

static void __attribute__((no_instrument_function)) __attribute__((cold))
	__attribute__((no_sanitize_address))
	__attribute__((no_sanitize_thread))
	__attribute__((no_sanitize_undefined))
	__attribute__((no_profile_instrument_function))
	__attribute__((no_stack_limit))
	__attribute__((optimize("no-stack-protector")))
	__attribute__((optimize("omit-frame-pointer")))
	__attribute__((optimize("Os")))
	profile_dump_maps(char *bfr,FILE *out)
{
	FILE *fp;
	char *range;
	char *start;
	char *end;
	char *mode;
	char *unused1;
	char *unused2;
	char *unused3;
	char *target;
	char *mem;

	snprintf(bfr,PATH_MAX,"/proc/%d/maps",getpid());
	if(__builtin_expect(!(fp=fopen(bfr,"re")),0))return;
	while(fgets(bfr,PATH_MAX,fp))
	{
		range=strtok_r(bfr," \t\r\n",&mem);
		mode=strtok_r(NULL," \t\r\n,",&mem);
		unused1=strtok_r(NULL," \t\r\n,",&mem);
		unused2=strtok_r(NULL," \t\r\n,",&mem);
		unused3=strtok_r(NULL," \t\r\n,",&mem);
		target=strtok_r(NULL," \t\r\n,",&mem);
		if(__builtin_expect(!range,0)||__builtin_expect(!mode,0)||
			__builtin_expect(!unused1,0)||
			__builtin_expect(!unused2,0)||
			__builtin_expect(!unused3,0)||
			__builtin_expect(!target,0))continue;
		if(strcmp(mode,"r-xp")||*target!='/')continue;
		start=strtok_r(range,"-",&mem);
		end=strtok_r(NULL,"\n",&mem);
		if(__builtin_expect(!start,0)||__builtin_expect(!end,0)||
			__builtin_expect(!*start,0)||
			__builtin_expect(!*end,0))continue;
		fprintf(out,"MAP: 0x%s 0x%s %s\n",start,end,target);
	}
	fclose(fp);
}

static void __attribute__((no_instrument_function)) __attribute__((cold))
	__attribute__((no_sanitize_address))
	__attribute__((no_sanitize_thread))
	__attribute__((no_sanitize_undefined))
	__attribute__((no_profile_instrument_function))
	__attribute__((no_stack_limit))
	__attribute__((optimize("no-stack-protector")))
	__attribute__((optimize("omit-frame-pointer")))
	__attribute__((optimize("Os")))
	profile_dump_cmd(char *bfr,FILE *out)
{
	FILE *fp;

	snprintf(bfr,PATH_MAX,"/proc/%d/cmdline",getpid());
	if(__builtin_expect(!(fp=fopen(bfr,"re")),0))return;
	if(__builtin_expect(!fgets(bfr,PATH_MAX,fp),0))
	{
		fclose(fp);
		return;
	}
	fclose(fp);
	if(__builtin_expect(realpath(bfr,bfr+PATH_MAX)!=NULL,1))
		fprintf(out,"CMD: %s\n",bfr+PATH_MAX);
}

void __attribute__ ((destructor)) __attribute__((no_instrument_function))
	__attribute__((no_sanitize_address))
	__attribute__((no_sanitize_thread))
	__attribute__((no_sanitize_undefined))
	__attribute__((no_profile_instrument_function))
	__attribute__((no_stack_limit))
	__attribute__((optimize("no-stack-protector")))
	__attribute__((optimize("omit-frame-pointer")))
	__attribute__((optimize("Os")))
	__attribute__((cold)) profile_fini(void)
{
	int i;
#if defined(_PTHREAD_H) && defined(PROFILE_NO_TLS)
	PROFILE_THREAD *tt=pthread_getspecific(profile_key);
#else
	PROFILE_THREAD *tt=profile_thread;
#endif
	FILE *fp;
	char *data=NULL;
	struct timespec stamp;
	struct timespec cpu;
	struct rusage r;

	if(__builtin_expect(profile_disabled,0))return;

	if(__builtin_expect(profile_gettime(CLOCK_PROCESS_CPUTIME_ID,&cpu),0))
		goto timeerr;

	if(__builtin_expect(clock_gettime(CLOCK_MONOTONIC,&stamp),0))
		goto timeerr;

	if(__builtin_expect(getrusage(RUSAGE_SELF,&r),0))
	{
timeerr:	profile_time_error=1;
		goto fail;
	}

#ifdef _PTHREAD_H
	if(__builtin_expect(!profile_error,1))
		for(i=0;i<PROFILE_THREAD_TABLE_SIZE;i++)
			while(profile_thread_table[i])
	{
		tt=profile_thread_table[i];
		profile_thread_table[i]=tt->next;

		profile_stack_unwind(tt,0);
		free(tt);
	}
#else
	if(__builtin_expect(!profile_error,1)&&tt)
	{
		profile_stack_unwind(tt,0);
		free(tt);
	}
#endif

	if(__builtin_expect(!profile_error,1))
	{
#ifdef _PTHREAD_H
		pthread_setspecific(profile_key,NULL);
		pthread_key_delete(profile_key);
#endif

		profile_deltatime(stamp,profile_process_time);

		if(!(data=malloc(2*PATH_MAX)))
		{
fail:			profile_error=1;
		}
	}

	if(profile_pid==getpid())
	{
		if(profile_daemon)goto out;
	}
	else if(!profile_daemon)goto out;

	if(__builtin_expect((fp=fopen(profile_log_file,"we"))!=NULL,1))
	{
		if(__builtin_expect(!profile_error,1))
		{
			profile_dump_cmd(data,fp);
			fprintf(fp,"INFO: runtime %llu\n",profile_nsecs(stamp));
			fprintf(fp,"INFO: cpu-usage %llu\n",profile_nsecs(cpu));
			fprintf(fp,"INFO: maxrss %lu\n",r.ru_maxrss);
			fprintf(fp,"INFO: f-pool-use %d\n",profile_fpool_used);
			fprintf(fp,"INFO: f-pool-size %d\n",
				profile_fpool_limit);
			fprintf(fp,"INFO: f-pool-mem %zd\n",
				sizeof(PROFILE_FUNC)*profile_fpool_limit);
			fprintf(fp,"INFO: c-pool-use %d\n",profile_cpool_used);
			fprintf(fp,"INFO: c-pool-size %d\n",
				profile_cpool_limit);
			fprintf(fp,"INFO: c-pool-mem %zd\n",
				sizeof(PROFILE_CALLER)*profile_cpool_limit);
			fprintf(fp,"INFO: stack-size %d\n",
				profile_stack_limit-1);
			fprintf(fp,"INFO: thread-mem %d\n",profile_thread_size);
#ifdef _PTHREAD_H
			fprintf(fp,"INFO: max-threads %d\n",profile_maxthreads);
#else
			fprintf(fp,"INFO: max-threads %d\n",1);
#endif
			profile_dump_maps(data,fp);
			for(i=0;i<PROFILE_FUNC_TABLE_SIZE;i++)
				if(profile_root[i])
					profile_func_walk(profile_root[i],fp);
		}
		else if(!profile_func_exhausted&&!profile_caller_exhausted&&
		    !profile_stack_exhausted&&!profile_time_error)
			fprintf(fp,"ERROR: internal or resource problem\n");

		if(__builtin_expect(profile_func_exhausted,0))
		    fprintf(fp,"ERROR: func pool exhausted\n");
		if(__builtin_expect(profile_caller_exhausted,0))
		    fprintf(fp,"ERROR: caller pool exhausted\n");
		if(__builtin_expect(profile_stack_exhausted,0))
		    fprintf(fp,"ERROR: time stack exhausted\n");
		if(__builtin_expect(profile_time_error,0))
		    fprintf(fp,"ERROR: time access failure\n");

		fclose(fp);
	}

out:	if(__builtin_expect(data!=NULL,1))free(data);
	if(__builtin_expect(profile_func_alloc!=NULL,1))
		free(profile_func_alloc);
	if(__builtin_expect(profile_caller_alloc!=NULL,1))
		free(profile_caller_alloc);
}

void __attribute__((no_instrument_function)) __attribute__((hot))
	__attribute__((no_sanitize_address))
	__attribute__((no_sanitize_thread))
	__attribute__((no_sanitize_undefined))
	__attribute__((no_profile_instrument_function))
	__attribute__((no_stack_limit))
	__attribute__((optimize("no-stack-protector")))
	__attribute__((optimize("omit-frame-pointer")))
	__attribute__((optimize("Os")))
	__cyg_profile_func_enter(void *func,void *caller)
{
	PROFILE_FUNC **e;
	PROFILE_CALLER **c;
	PROFILE_STACK *p;
#if defined(_PTHREAD_H) && defined(PROFILE_NO_TLS)
	PROFILE_THREAD *tt=pthread_getspecific(profile_key);
#else
	PROFILE_THREAD *tt=profile_thread;
#endif
#if defined(_PTHREAD_H) && !defined(PROFILE_NO_ATOMICS)
	register union
	{
		PROFILE_FUNC *e;
		PROFILE_CALLER *c;
	} m;
#endif
	struct timespec stamp;

	if(__builtin_expect(profile_error,0))return;

#ifdef PROFILE_STRICT
	if(__builtin_expect(profile_gettime(CLOCK_THREAD_CPUTIME_ID,&stamp),0))
		goto timeerr;
#else
	profile_gettime(CLOCK_THREAD_CPUTIME_ID,&stamp);
#endif

	if(__builtin_expect(!tt,0))
	{
#ifdef PROFILE_STRICT
		if(__builtin_expect(!(tt=malloc(profile_thread_size)),0))
		{
			profile_stack_exhausted=1;
			goto fail;
		}
#else
		tt=malloc(profile_thread_size);
#endif
		p=tt->stack;
		tt->stack_index=0;
		tt->unwind=0;
		tt->depth=0;
		tt->funcs=0;
		tt->nsecs=0;
		tt->secs=0;
#ifdef _PTHREAD_H
		tt->table_index=profile_table_next;
		tt->next=profile_thread_table[tt->table_index];
		profile_thread_table[tt->table_index]=tt;
		if((profile_table_next+=1)==PROFILE_THREAD_TABLE_SIZE)
		    profile_table_next=0;
		pthread_setspecific(profile_key,tt);
#ifndef PROFILE_NO_ATOMICS
		{
			unsigned int n;
			unsigned int t;

			n=__atomic_add_fetch(&profile_numthreads,1,
				__ATOMIC_SEQ_CST);
repeat:			t=__atomic_load_n(&profile_maxthreads,__ATOMIC_SEQ_CST);
			if(n>t)if(__builtin_expect(
				!__atomic_compare_exchange_n(
					&profile_maxthreads,&t,n,1,
					__ATOMIC_SEQ_CST,__ATOMIC_RELAXED),0))
						goto repeat;
		};
#else
		lock(profile_mutex);
		if(++profile_numthreads>profile_maxthreads)
			profile_maxthreads=profile_numthreads;
		unlock(profile_mutex);
#endif
#else
		if(++profile_numthreads>1)goto fail;
#endif
#if !defined(_PTHREAD_H) || !defined(PROFILE_NO_TLS)
		profile_thread=tt;
#endif
	}
	else
	{
		profile_deltatime(stamp,tt->start_time);

		p=&tt->stack[tt->stack_index];
		p->used.tv_nsec+=stamp.tv_nsec;
		p->used.tv_sec+=stamp.tv_sec;

#if defined(_PTHREAD_H) && !defined(PROFILE_NO_ATOMICS)
		__atomic_add_fetch(&p->c->calling,1,__ATOMIC_RELAXED);
#else
#ifdef _PTHREAD_H
		lock(profile_mutex);
#endif
		p->c->calling++;
#ifdef _PTHREAD_H
		unlock(profile_mutex);
#endif
#endif
	}

	if(__builtin_expect(++(tt->stack_index)==profile_stack_limit,0))
	{
		profile_stack_exhausted=1;
		goto err;
	}
	else if(tt->stack_index>tt->depth)tt->depth=tt->stack_index;
	tt->funcs++;
	p++;

#if defined(_PTHREAD_H) && !defined(PROFILE_NO_ATOMICS)

	e=&profile_root[(((unsigned long)func)>>4)&(PROFILE_FUNC_TABLE_SIZE-1)];

eagain:	if((m.e=__atomic_load_n(e,__ATOMIC_SEQ_CST)))
	{
		if(m.e->func<func)
		{
			e=&m.e->left;
			goto eagain;
		}
		if(m.e->func>func)
		{
			e=&m.e->right;
			goto eagain;
		}
	}
	else
	{
		lock(profile_mutex);
		if(__builtin_expect(__atomic_load_n(e,__ATOMIC_SEQ_CST)
			!=NULL,0))
		{
			unlock(profile_mutex);
			goto eagain;
		}
		if(__builtin_expect(profile_fpool_used==profile_fpool_limit,0))
		{
			profile_func_exhausted=1;
			goto err;
		}
		m.e=&profile_func_alloc[profile_fpool_used++];
		m.e->func=func;
		__atomic_store_n(e,m.e,__ATOMIC_SEQ_CST);
		unlock(profile_mutex);
	}

	c=&(*e)->caller[(((unsigned long)caller)>>4)&
		(PROFILE_CALLER_TABLE_SIZE-1)];

cagain:	if((m.c=__atomic_load_n(c,__ATOMIC_SEQ_CST)))
	{
		if(m.c->caller<caller)
		{
			c=&m.c->left;
			goto cagain;
		}
		if(m.c->caller>caller)
		{
			c=&m.c->right;
			goto cagain;
		}
	}
	else
	{
		lock(profile_mutex2);
		if(__builtin_expect(__atomic_load_n(c,__ATOMIC_SEQ_CST)
			!=NULL,0))
		{
			unlock(profile_mutex2);
			goto cagain;
		}
		if(__builtin_expect(profile_cpool_used==profile_cpool_limit,0))
		{
			profile_caller_exhausted=1;
			goto err2;
		}
		m.c=&profile_caller_alloc[profile_cpool_used++];
		m.c->caller=caller;
		__atomic_store_n(c,m.c,__ATOMIC_SEQ_CST);
		unlock(profile_mutex2);
	}

	__atomic_add_fetch(&m.c->calls,1,__ATOMIC_RELAXED);

#else

#ifdef _PTHREAD_H
	lock(profile_mutex);
#endif

	e=&profile_root[(((unsigned long)func)>>4)&(PROFILE_FUNC_TABLE_SIZE-1)];

	while(*e)
	{
		if((*e)->func<func)e=&(*e)->left;
		else  if((*e)->func>func)e=&(*e)->right;
		else break;
	}

	if(!*e)
	{
		if(__builtin_expect(profile_fpool_used==profile_fpool_limit,0))
		{
			profile_func_exhausted=1;
			goto err;
		}
		*e=&profile_func_alloc[profile_fpool_used++];
		(*e)->func=func;
	}

	c=&(*e)->caller[(((unsigned long)caller)>>4)&
		(PROFILE_CALLER_TABLE_SIZE-1)];

	while(*c)
	{
		if((*c)->caller<caller)c=&(*c)->left;
		else  if((*c)->caller>caller)c=&(*c)->right;
		else break;
	}

	if(!*c)
	{
		if(__builtin_expect(profile_cpool_used==profile_cpool_limit,0))
		{
			profile_caller_exhausted=1;
			goto err;
		}
		*c=&profile_caller_alloc[profile_cpool_used++];
		(*c)->caller=caller;
	}

	(*c)->calls++;

#ifdef _PTHREAD_H
	unlock(profile_mutex);
#endif

#endif

	p->e=*e;
	p->c=*c;
	p->used.tv_nsec=0;
	p->used.tv_sec=0;

#ifdef PROFILE_STRICT
	if(__builtin_expect(profile_gettime(CLOCK_THREAD_CPUTIME_ID,
		&tt->start_time),0))
	{
timeerr:	profile_time_error=1;
#ifdef _PTHREAD_H
#ifndef PROFILE_NO_ATOMICS
		if(0)
		{
err2:			unlock(profile_mutex2);
		}
#endif
		if(0)
		{
err:			unlock(profile_mutex);
		}
#else
err:
#endif
fail:		profile_error=1;
		return;
	}
#else
	profile_gettime(CLOCK_THREAD_CPUTIME_ID,&tt->start_time);
	return;

#if defined(_PTHREAD_H) && !defined(PROFILE_NO_ATOMICS)
err2:	unlock(profile_mutex2);
	profile_error=1;
	return;
#endif
#ifdef _PTHREAD_H
err:	unlock(profile_mutex);
	profile_error=1;
	return;
#else
fail:
err:	profile_error=1;
	return;
#endif
#endif
}

void __attribute__((no_instrument_function)) __attribute__((hot))
	__attribute__((no_sanitize_address))
	__attribute__((no_sanitize_thread))
	__attribute__((no_sanitize_undefined))
	__attribute__((no_profile_instrument_function))
	__attribute__((no_stack_limit))
	__attribute__((optimize("no-stack-protector")))
	__attribute__((optimize("omit-frame-pointer")))
	__attribute__((optimize("Os")))
	__cyg_profile_func_exit(void *func,void *caller)
{
	PROFILE_STACK *p;
#ifdef _PTHREAD_H
	PROFILE_THREAD **t;
#endif
#if defined(_PTHREAD_H) && defined(PROFILE_NO_TLS)
	PROFILE_THREAD *tt=pthread_getspecific(profile_key);
#else
	PROFILE_THREAD *tt=profile_thread;
#endif
	struct timespec stamp;

	if(__builtin_expect(profile_error,0))return;

#ifdef PROFILE_STRICT
	if(__builtin_expect(profile_gettime(CLOCK_THREAD_CPUTIME_ID,&stamp),0))
		goto timeerr;
#else
	profile_gettime(CLOCK_THREAD_CPUTIME_ID,&stamp);
#endif

	p=&tt->stack[tt->stack_index];

#ifdef PROFILE_STRICT
	if(__builtin_expect(p->e->func!=func,0)||
		__builtin_expect(p->c->caller!=caller,0))goto err;
#endif

	profile_deltatime(stamp,tt->start_time);

#if defined(_PTHREAD_H) && !defined(PROFILE_NO_ATOMICS)

	__atomic_add_fetch(&p->c->nsecs,p->used.tv_nsec+stamp.tv_nsec,
		__ATOMIC_RELAXED);
	__atomic_add_fetch(&p->c->secs,p->used.tv_sec+stamp.tv_sec,
		__ATOMIC_RELAXED);

#else

#ifdef _PTHREAD_H
	lock(profile_mutex);
#endif

	p->c->nsecs+=p->used.tv_nsec+stamp.tv_nsec;
	p->c->secs+=p->used.tv_sec+stamp.tv_sec;

#ifdef _PTHREAD_H
	unlock(profile_mutex);
#endif

#endif

	tt->nsecs+=p->used.tv_nsec+stamp.tv_nsec;
	tt->secs+=p->used.tv_sec+stamp.tv_sec;

	if(__builtin_expect(!(--(tt->stack_index)),0))
	{
#if defined(_PTHREAD_H) && !defined(PROFILE_NO_ATOMICS)
		__atomic_add_fetch(&p->e->funcs,tt->funcs,__ATOMIC_RELAXED);
		__atomic_add_fetch(&p->e->calls,1,__ATOMIC_RELAXED);
		__atomic_add_fetch(&p->e->secs,tt->secs,__ATOMIC_RELAXED);
		__atomic_add_fetch(&p->e->nsecs,tt->nsecs,__ATOMIC_RELAXED);
		{
			unsigned long long depth;

repeat:			depth= __atomic_load_n(&p->e->depth,__ATOMIC_SEQ_CST);
			if(tt->depth>depth)if(__builtin_expect(
				!__atomic_compare_exchange_n(&p->e->depth,
				&depth,tt->depth,1,__ATOMIC_SEQ_CST,
				__ATOMIC_RELAXED),0))goto repeat;
		};
		__atomic_sub_fetch(&profile_numthreads,1,__ATOMIC_SEQ_CST);
#else
#ifdef _PTHREAD_H
		lock(profile_mutex);
#endif
		p->e->funcs+=tt->funcs;
		p->e->calls++;
		p->e->secs+=tt->secs;
		p->e->nsecs+=tt->nsecs;
		if(tt->depth>p->e->depth)p->e->depth=tt->depth;
		profile_numthreads--;
#ifdef _PTHREAD_H
		unlock(profile_mutex);
#endif
#endif
#ifdef _PTHREAD_H
		for(t=&profile_thread_table[tt->table_index];*t;t=&(*t)->next)
			if(*t==tt)
		{
			*t=tt->next;
			break;
		}
#endif
		free(tt);
#ifdef _PTHREAD_H
		pthread_setspecific(profile_key,NULL);
#endif
#if !defined(_PTHREAD_H) || !defined(PROFILE_NO_TLS)
		profile_thread=NULL;
#endif
	}
#ifdef PROFILE_STRICT
	else if(__builtin_expect(profile_gettime(CLOCK_THREAD_CPUTIME_ID,
		&tt->start_time),0))
	{
timeerr:	profile_time_error=1;
err:		profile_error=1;
		return;
	}
#else
	else profile_gettime(CLOCK_THREAD_CPUTIME_ID,&tt->start_time);
#endif
}

#endif
