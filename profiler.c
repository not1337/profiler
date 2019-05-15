/*
 * This file is part of the profiler project
 *
 * (C) 2019 Andreas Steinmetz, ast@domdv.de
 * The contents of this file is licensed under the GPL version 2 or, at
 * your choice, any later version of this license.
 */

#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

typedef struct map
{
	struct map *next;
	unsigned long start;
	unsigned long end;
	char *brief;
	char file[0];
} MAP;

typedef struct addr
{
	struct addr *next;
	char *func;
	char *file;
	unsigned long addr;
	int line;
} ADDR;

typedef struct trace
{
	struct trace *next;
	ADDR *funcdata;
	ADDR *callerdata;
	MAP *funcmap;
	MAP *callermap;
	unsigned long func;
	unsigned long caller;
	unsigned long long calls;
	unsigned long long nsecs;
	unsigned long long calling;
	unsigned long long unwind;
	int funcid;
	int callerid;
} TRACE;

typedef struct thread
{
	struct thread *next;
	ADDR *funcdata;
	MAP *funcmap;
	unsigned long func;
	unsigned long long calls;
	unsigned long long nsecs;
	unsigned long long funcs;
	unsigned long long unwind;
	unsigned long long depth;
	unsigned long long avg;
} THREAD;

typedef struct
{
	ADDR *funcdata;
	MAP *funcmap;
	unsigned long func;
	unsigned long long calls;
	unsigned long long nsecs;
	unsigned long long avg;
} FUNC;

static char *cmd;
static ADDR *list;
static ADDR **sortedlist;
static TRACE *data;
static TRACE **sorted;
static TRACE **sortedcaller;
static THREAD *jobs;
static THREAD **sortedjobs;
static MAP *maps;
static MAP **sortedmaps;
static int tracetotal;
static int addrtotal;
static int maptotal;
static int jobstotal;
static int base;
static int fpool;
static int cpool;
static int fsize;
static int csize;
static int fmem;
static int cmem;
static int ssize;
static int tmem;
static int maxthreads;
static unsigned long long runtime;
static unsigned long long cpuuse;
static unsigned long long maxrss;

static int funcsort(const void *p1, const void *p2)
{
	const TRACE **a1=(const TRACE **)p1;
	const TRACE **a2=(const TRACE **)p2;

	if((*a1)->func<(*a2)->func)return -1;
	if((*a1)->func>(*a2)->func)return 1;
	if((*a1)->caller<(*a2)->caller)return -1;
	if((*a1)->caller>(*a2)->caller)return 1;
	return 0;
}

static int callersort(const void *p1, const void *p2)
{
	const TRACE **a1=(const TRACE **)p1;
	const TRACE **a2=(const TRACE **)p2;

	if((*a1)->caller<(*a2)->caller)return -1;
	if((*a1)->caller>(*a2)->caller)return 1;
	if((*a1)->func<(*a2)->func)return -1;
	if((*a1)->func>(*a2)->func)return 1;
	return 0;
}

static int addrsort(const void *p1, const void *p2)
{
	const ADDR **a1=(const ADDR **)p1;
	const ADDR **a2=(const ADDR **)p2;

	if((*a1)->addr<(*a2)->addr)return -1;
	if((*a1)->addr>(*a2)->addr)return 1;
	return 0;
}

static int numsort(const void *p1, const void *p2)
{
	const unsigned long *n1=p1;
	const unsigned long *n2=p2;

	if(*n1<*n2)return -1;
	if(*n1>*n2)return 1;
	return 0;
}

static int calleridsort(const void *p1, const void *p2)
{
	const TRACE **a1=(const TRACE **)p1;
	const TRACE **a2=(const TRACE **)p2;

	if((*a1)->callerid<(*a2)->callerid)return -1;
	if((*a1)->callerid>(*a2)->callerid)return 1;
	if((*a1)->caller<(*a2)->caller)return -1;
	if((*a1)->caller>(*a2)->caller)return 1;
	return 0;
}

static int jobssort(const void *p1, const void *p2)
{
	const THREAD **j1=(const THREAD **)p1;
	const THREAD **j2=(const THREAD **)p2;

	if((*j1)->func<(*j2)->func)return -1;
	if((*j1)->func>(*j2)->func)return 1;
	return 0;
}

static int mapsort(const void *p1, const void *p2)
{
	const MAP **m1=(const MAP **)p1;
	const MAP **m2=(const MAP **)p2;

	if((*m1)->start<(*m2)->start)return -1;
	if((*m1)->start>(*m2)->start)return 1;
	return 0;
}

static int readtrace(char *fn,int mode,char *pfx)
{
	int i;
	int j;
	int line;
	int err=0;
	int in[2];
	int out[2];
	unsigned long addr;
	char *func;
	char *caller;
	char *calls;
	char *nsecs;
	char *calling;
	char *unwind;
	char *funcs;
	char *depth;
	char *file;
	char *ptr;
	char *start;
	char *end;
	unsigned long *addrs;
	TRACE *t;
	ADDR *a;
	THREAD *job;
	MAP *m;
	FILE *fp;
	FILE *fp2;
	char bfr[1024];

	if(!(fp=fopen(fn,"re")))
	{
		perror("fopen");
		return -1;
	}
	while(fgets(bfr,sizeof(bfr),fp))
	{
		if(!strncmp(bfr,"TRACE: ",7))
		{
			func=strtok(bfr+7," ");
			caller=strtok(NULL," ");
			calls=strtok(NULL," ");
			nsecs=strtok(NULL," ");
			calling=strtok(NULL," ");
			unwind=strtok(NULL,"\n");
			if(!func||!caller||!calls||!nsecs||!calling||!unwind)
				continue;
			if(!(t=malloc(sizeof(TRACE))))
			{
				perror("malloc");
				return -1;
			}
			t->funcdata=NULL;
			t->callerdata=NULL;
			t->funcmap=NULL;
			t->callermap=NULL;
			t->func=strtol(func,NULL,16);
			t->caller=strtol(caller,NULL,16);
			t->calls=strtoll(calls,NULL,10);
			t->nsecs=strtoll(nsecs,NULL,10);
			t->calling=strtoll(calling,NULL,10);
			t->unwind=strtoll(unwind,NULL,10);
			t->next=data;
			data=t;
			tracetotal++;
		}
		else if(!strncmp(bfr,"THREAD: ",8))
		{
			func=strtok(bfr+8," ");
			calls=strtok(NULL," ");
			nsecs=strtok(NULL," ");
			funcs=strtok(NULL," ");
			unwind=strtok(NULL," ");
			depth=strtok(NULL,"\n");
			if(!func||!calls||!nsecs||!funcs||!unwind||!depth)
				continue;
			if(!(job=malloc(sizeof(THREAD))))
			{
				perror("malloc");
				return -1;
			}
			job->funcdata=NULL;
			job->funcmap=NULL;
			job->func=strtol(func,NULL,16);
			job->calls=strtoll(calls,NULL,10);
			job->nsecs=strtoll(nsecs,NULL,10);
			job->funcs=strtoll(funcs,NULL,10);
			job->unwind=strtoll(unwind,NULL,10);
			job->depth=strtoll(depth,NULL,10);
			job->next=jobs;
			jobs=job;
			jobstotal++;
		}
		else if(!strncmp(bfr,"MAP: ",5))
		{
			start=strtok(bfr+5," ");
			end=strtok(NULL," ");
			file=strtok(NULL,"\n");
			if(!start||!end||!file)continue;
			if(pfx)
			{
				if(!(m=malloc(sizeof(MAP)+strlen(file)+
					strlen(pfx)+2)))
				{
					perror("malloc");
					return -1;
				}
			}
			else if(!(m=malloc(sizeof(MAP)+strlen(file)+1)))
			{
				perror("malloc");
				return -1;
			}
			m->start=strtol(start,NULL,16);
			m->end=strtol(end,NULL,16);
			if(pfx)
			{
				strcpy(m->file,pfx);
				strcat(m->file,"/");
				strcat(m->file,file);
			}
			else strcpy(m->file,file);
			if((ptr=strrchr(m->file,'/')))m->brief=ptr+1;
			else m->brief=m->file;
			m->next=maps;
			maps=m;
			maptotal++;
		}
		else if(!strncmp(bfr,"INFO: ",6))
		{
			if(!strncmp(bfr+6,"runtime ",8))
				runtime=strtoll(bfr+14,NULL,10);
			else if(!strncmp(bfr+6,"cpu-usage ",10))
				cpuuse=strtoll(bfr+16,NULL,10);
			else if(!strncmp(bfr+6,"maxrss ",7))
				maxrss=strtoll(bfr+13,NULL,10);
			else if(!strncmp(bfr+6,"f-pool-use ",11))
				fpool=atoi(bfr+17);
			else if(!strncmp(bfr+6,"f-pool-size ",12))
				fsize=atoi(bfr+18);
			else if(!strncmp(bfr+6,"f-pool-mem ",11))
				fmem=atoi(bfr+17);
			else if(!strncmp(bfr+6,"c-pool-use ",11))
				cpool=atoi(bfr+17);
			else if(!strncmp(bfr+6,"c-pool-size ",12))
				csize=atoi(bfr+18);
			else if(!strncmp(bfr+6,"c-pool-mem ",11))
				cmem=atoi(bfr+17);
			else if(!strncmp(bfr+6,"stack-size ",11))
				ssize=atoi(bfr+17);
			else if(!strncmp(bfr+6,"thread-mem ",11))
				tmem=atoi(bfr+17);
			else if(!strncmp(bfr+6,"max-threads ",12))
				maxthreads=atoi(bfr+18);
		}
		else if(!strncmp(bfr,"CMD: ",5))
		{
			if(pfx)
			{
				if((ptr=strtok(bfr+5," \n")))
				{
					if(!(cmd=malloc(strlen(pfx)+strlen(ptr)
						+2)))
					{
						perror("malloc");
						return -1;
					}
					strcpy(cmd,pfx);
					strcat(cmd,"/");
					strcat(cmd,ptr);
				}
			}
			else if((ptr=strtok(bfr+5," \n")))if(!(cmd=strdup(ptr)))
			{
				perror("strdup");
				return -1;
			}
		}
		else if(!strncmp(bfr,"ERROR: ",7))
		{
			printf("%s",bfr);
			err=1;
		}
	}
	fclose(fp);

	if(err)return -1;

	if(!tracetotal)
	{
		fprintf(stderr,"incomplete input\n");
		return -1;
	}

	if(!(sortedmaps=malloc(maptotal*sizeof(MAP *))))
	{
		perror("malloc");
		return -1;
	}

	for(i=0,m=maps;i<maptotal;i++,m=m->next)sortedmaps[i]=m;

	qsort(sortedmaps,maptotal,sizeof(MAP *),mapsort);

	if(!(addrs=malloc(2*tracetotal*sizeof(unsigned long))))
	{
		perror("malloc");
		return -1;
	}

	for(i=0,t=data;i<2*tracetotal;t=t->next)
	{
		addrs[i++]=t->func;
		addrs[i++]=t->caller;
	}

	qsort(addrs,2*tracetotal,sizeof(unsigned long),numsort);

	for(i=0,j=0,fp=NULL,fp2=NULL;i<2*tracetotal&&j<maptotal;)
	{
		if(i&&addrs[i-1]==addrs[i])
		{
			i++;
			continue;
		}

		if(addrs[i]<sortedmaps[j]->start)
		{
			i++;
			continue;
		}
	
		if(addrs[i]>=sortedmaps[j]->end)
		{
			if(fp)
			{
				fclose(fp);
				fclose(fp2);
				fp=NULL;
				wait(NULL);
			}
			j++;
			continue;
		}

		if(!fp)
		{
			if(pipe(in)||pipe(out))
			{
				perror("pipe");
				return -1;
			}

			switch(fork())
			{
			case 0:	if(dup2(in[0],0)==-1||dup2(out[1],1)==-1)
				{
					perror("dup2");
					exit(1);
				}
				close(in[1]);
				close(out[0]);
				execlp("addr2line","addr2line",
					mode?"-asfpCe":"-afpCe",
					sortedmaps[j]->file,NULL);
				perror("execlp");
				exit(1);

			case -1:perror("fork");
				return -1;

			default:close(in[0]);
				close(out[1]);
			}

			if(!(fp=fdopen(out[0],"re"))||!(fp2=fdopen(in[1],"we")))
			{
				perror("fdopen");
				return 1;
			}

		}

		fprintf(fp2,"0x%lx\n",addrs[i]-sortedmaps[j]->start);
		if(fflush(fp2))
		{
			perror("fflush");
			return -1;
		}
		if(!fgets(bfr,sizeof(bfr),fp))
		{
			perror("fgets");
			return -1;
		}
		ptr=strtok(bfr,":");
		if(!ptr)
		{
			i++;
			continue;
		}
		addr=strtol(ptr,NULL,16);
		func=strtok(NULL," ");
		if(!addr||!func||!strcmp(func,"??"))
		{
			i++;
			continue;
		}
		if(!strtok(NULL," "))
		{
			i++;
			continue;
		}
		if(!(ptr=strtok(NULL," \n")))
		{
			i++;
			continue;
		}
		if(!strcmp(ptr,"??:?"))
		{
			file=(mode?sortedmaps[j]->brief:sortedmaps[j]->file);
			line=0;
		}
		else
		{
			file=strtok(ptr,":");
			ptr=strtok(NULL,"\n");
			if(!file||!ptr)
			{
				i++;
				continue;
			}
			line=atoi(ptr);
		}
		if(!(a=malloc(sizeof(ADDR))))
		{
			perror("malloc");
			return -1;
		}
		if(!(a->func=strdup(func)))
		{
			perror("strdup");
			return -1;
		}
		if(!(a->file=strdup(file)))
		{
			perror("strdup");
			return -1;
		}
		a->addr=addr+sortedmaps[j]->start;
		a->line=line;
		a->next=list;
		list=a;
		addrtotal++;

		i++;
	}

	if(fp)
	{
		fclose(fp);
		fclose(fp2);
		wait(NULL);
	}

	free(addrs);

	if(!(sortedlist=malloc(addrtotal*sizeof(ADDR *))))
	{
		perror("malloc");
		return -1;
	}

	for(i=0,a=list;i<addrtotal;i++,a=a->next)sortedlist[i]=a;

	qsort(sortedlist,addrtotal,sizeof(ADDR *),addrsort);

	if(!(sortedjobs=malloc(jobstotal*sizeof(THREAD *))))
	{
		perror("malloc");
		return -1;
	}

	for(i=0,job=jobs;i<jobstotal;i++,job=job->next)sortedjobs[i]=job;

	qsort(sortedjobs,jobstotal,sizeof(THREAD *),jobssort);

	for(i=0,j=0;i<jobstotal&&j<addrtotal;)
	{
		if(sortedjobs[i]->func<sortedlist[j]->addr)i++;
		else if(sortedlist[j]->addr<sortedjobs[i]->func)j++;
		else sortedjobs[i++]->funcdata=sortedlist[j];
	}

	for(i=0,j=0;i<jobstotal&&j<maptotal;)
	{
		if(sortedjobs[i]->func<sortedmaps[j]->start)i++;
		else if(sortedjobs[i]->func>=sortedmaps[j]->end)j++;
		else sortedjobs[i++]->funcmap=sortedmaps[j];
	}

	if(!(sortedcaller=malloc(tracetotal*sizeof(TRACE *))))
	{
		perror("malloc");
		return -1;
	}

	for(i=0,t=data;i<tracetotal;i++,t=t->next)sortedcaller[i]=t;

	qsort(sortedcaller,tracetotal,sizeof(TRACE *),callersort);

	for(i=0,j=0;i<tracetotal&&j<addrtotal;)
	{
		if(sortedcaller[i]->caller<sortedlist[j]->addr)i++;
		else if(sortedlist[j]->addr<sortedcaller[i]->caller)j++;
		else sortedcaller[i++]->callerdata=sortedlist[j];
	}

	for(i=0,j=0;i<tracetotal&&j<maptotal;)
	{
		if(sortedcaller[i]->caller<sortedmaps[j]->start)i++;
		else if(sortedcaller[i]->caller>=sortedmaps[j]->end)j++;
		else sortedcaller[i++]->callermap=sortedmaps[j];
	}

	if(!(sorted=malloc(tracetotal*sizeof(TRACE *))))
	{
		perror("malloc");
		return -1;
	}

	for(i=0,t=data;i<tracetotal;i++,t=t->next)sorted[i]=t;

	qsort(sorted,tracetotal,sizeof(TRACE *),funcsort);

	for(i=0,j=0;i<tracetotal&&j<addrtotal;)
	{
		if(sorted[i]->func<sortedlist[j]->addr)i++;
		else if(sortedlist[j]->addr<sorted[i]->func)j++;
		else sorted[i++]->funcdata=sortedlist[j];
	}

	for(i=0,j=0;i<tracetotal&&j<maptotal;)
	{
		if(sorted[i]->func<sortedmaps[j]->start)i++;
		else if(sorted[i]->func>=sortedmaps[j]->end)j++;
		else sorted[i++]->funcmap=sortedmaps[j];
	}

	for(i=0,j=0;i<tracetotal;i++)
	{
		if(i&&sorted[i-1]->func==sorted[i]->func)
		{
			sorted[i]->funcid=sorted[i-1]->funcid;
			continue;
		}
		if(sorted[i]->funcdata)sorted[i]->funcid=j++;
		else sorted[i]->funcid=-1;
	}

	for(i=0;i<tracetotal;i++)
	{
		if(i&&sortedcaller[i-1]->caller==sortedcaller[i]->caller)
		{
			sortedcaller[i]->callerid=sortedcaller[i-1]->callerid;
			continue;
		}
		if(!sortedcaller[i]->callerdata)
		{
			sortedcaller[i]->callerid=-1;
			continue;
		}
		sortedcaller[i]->callerid=-1;
		for(j=0;j<tracetotal;j++)if(sorted[j]->funcid!=-1)
		    if(!strcmp(sortedcaller[i]->callerdata->func,
			sorted[j]->funcdata->func))
			    if(!strcmp(sortedcaller[i]->callerdata->file,
				sorted[j]->funcdata->file))
		{
			sortedcaller[i]->callerid=sorted[j]->funcid;
			break;
		}
	}

	qsort(sortedcaller,tracetotal,sizeof(TRACE *),calleridsort);

	for(i=1,j=tracetotal;j;j>>=1,i<<=1);
	if(!(tracetotal&~(i>>1)))i>>=1;
	base=i>>1;

	return 0;
}

static int adjust(int adjust)
{
	int i;
	unsigned long long adj;

	for(i=0;i<tracetotal;i++)
	{
		adj=adjust;
		adj*=sorted[i]->calls+sorted[i]->calling-sorted[i]->unwind;

		if(adj>sorted[i]->nsecs)sorted[i]->nsecs=0;
		else sorted[i]->nsecs-=adj;
	}

	for(i=0;i<jobstotal;i++)
	{
		adj=adjust;
		adj*=(sortedjobs[i]->funcs<<1)-sortedjobs[i]->calls-
			sortedjobs[i]->unwind;

		if(adj>sortedjobs[i]->nsecs)sortedjobs[i]->nsecs=0;
		else sortedjobs[i]->nsecs-=adj;

		sortedjobs[i]->avg=sortedjobs[i]->nsecs/sortedjobs[i]->calls;
	}

	return 0;
}

static int callssort(const void *p1, const void *p2)
{
	const FUNC *f1=p1;
	const FUNC *f2=p2;

	if(f1->calls<f2->calls)return 1;
	if(f1->calls>f2->calls)return -1;
	if(f1->func<f2->func)return -1;
	if(f1->func>f2->func)return 1;
	return 0;
}

static int cputimesort(const void *p1, const void *p2)
{
	const FUNC *f1=p1;
	const FUNC *f2=p2;

	if(f1->nsecs<f2->nsecs)return 1;
	if(f1->nsecs>f2->nsecs)return -1;
	if(f1->func<f2->func)return -1;
	if(f1->func>f2->func)return 1;
	return 0;
}

static int avgcpusort(const void *p1, const void *p2)
{
	const FUNC *f1=p1;
	const FUNC *f2=p2;

	if(f1->avg<f2->avg)return 1;
	if(f1->avg>f2->avg)return -1;
	if(f1->func<f2->func)return -1;
	if(f1->func>f2->func)return 1;
	return 0;
}

static int tops(int mode,int brief)
{
	int i;
	int l;
	int total;
	FUNC *list;

	if(!(list=malloc(tracetotal*sizeof(FUNC))))
	{
		perror("malloc");
		return -1;
	}

	for(i=0,total=0;i<tracetotal;i++)
	{
		if(i&&sorted[i-1]->func==sorted[i]->func)
		{
			list[total-1].calls+=sorted[i]->calls;
			list[total-1].nsecs+=sorted[i]->nsecs;
			continue;
		}
		list[total].func=sorted[i]->func;
		list[total].funcdata=sorted[i]->funcdata;
		list[total].funcmap=sorted[i]->funcmap;
		list[total].calls=sorted[i]->calls;
		list[total].nsecs=sorted[i]->nsecs;
		list[total].avg=list[total].nsecs/list[total].calls;
		total++;
	}

	switch(mode)
	{
	case 0:	printf("\nFunctions sorted by amount of calls:\n\n");
		qsort(list,total,sizeof(FUNC),callssort);
		break;

	case 1: printf("\nFunctions sorted by CPU usage:\n\n");
		qsort(list,total,sizeof(FUNC),cputimesort);
		break;

	case 2:	printf("\nFunctions sorted by amount of calls (avg. CPU usage):"
			"\n\n");
		qsort(list,total,sizeof(FUNC),callssort);
		break;

	case 3: printf("\nFunctions sorted by average CPU usage:\n\n");
		qsort(list,total,sizeof(FUNC),avgcpusort);
		break;
	}

	printf("Function                                               "
		"Calls        CPU Usage\n");
	printf("======================================================="
		"=========================\n");
	for(i=0;i<total;i++)
	{
		if(list[i].funcdata)
		{
			if(!list[i].funcdata->line)l=printf("%s (%s) ",
				list[i].funcdata->func,
				list[i].funcdata->file);
			else l=printf("%s (%s:%d) ",list[i].funcdata->func,
				list[i].funcdata->file,
				list[i].funcdata->line);
		}
		else if(list[i].funcmap)
		{
			l=printf("%s+%p ",brief?list[i].funcmap->brief:
				list[i].funcmap->file,(void *)(list[i].func-
				list[i].funcmap->start));
		}
		else l=printf("%p ",(void *)list[i].func);

		while(l<43)l+=printf("          ");
		while(l<53)l+=printf(" ");

		if(mode<2)printf(" %7llu %7llu.%09llu\n",list[i].calls,
			list[i].nsecs/1000000000,list[i].nsecs%1000000000);
		else printf(" %7llu %7llu.%09llu\n",list[i].calls,
			list[i].avg/1000000000,list[i].avg%1000000000);
	}

	free(list);
	return 0;
}

static int jobcallsort(const void *p1, const void *p2)
{
	const THREAD **j1=(const THREAD **)p1;
	const THREAD **j2=(const THREAD **)p2;

	if((*j1)->calls<(*j2)->calls)return 1;
	if((*j1)->calls>(*j2)->calls)return -1;
	if((*j1)->func<(*j2)->func)return -1;
	if((*j1)->func>(*j2)->func)return 1;
	return 0;
}

static int jobcpusort(const void *p1, const void *p2)
{
	const THREAD **j1=(const THREAD **)p1;
	const THREAD **j2=(const THREAD **)p2;

	if((*j1)->nsecs<(*j2)->nsecs)return 1;
	if((*j1)->nsecs>(*j2)->nsecs)return -1;
	if((*j1)->func<(*j2)->func)return -1;
	if((*j1)->func>(*j2)->func)return 1;
	return 0;
}

static int jobavgcpusort(const void *p1, const void *p2)
{
	const THREAD **j1=(const THREAD **)p1;
	const THREAD **j2=(const THREAD **)p2;

	if((*j1)->avg<(*j2)->avg)return 1;
	if((*j1)->avg>(*j2)->avg)return -1;
	if((*j1)->func<(*j2)->func)return -1;
	if((*j1)->func>(*j2)->func)return 1;
	return 0;
}

static int jobsproc(int mode,int brief)
{
	int i;
	int l;

	switch(mode)
	{
	case 0:	printf("\nThreads sorted by amount of calls:\n\n");
		qsort(sortedjobs,jobstotal,sizeof(THREAD *),jobcallsort);
		break;

	case 1: printf("\nThreads sorted by CPU usage:\n\n");
		qsort(sortedjobs,jobstotal,sizeof(THREAD *),jobcpusort);
		break;

	case 2:	printf("\nThreads sorted by amount of calls (avg. CPU usage):"
			"\n\n");
		qsort(sortedjobs,jobstotal,sizeof(THREAD *),jobcallsort);
		break;

	case 3: printf("\nThreads sorted by average CPU usage:\n\n");
		qsort(sortedjobs,jobstotal,sizeof(THREAD *),jobavgcpusort);
		break;
	}

	printf("Thread                                           Invoca"
		"tions        CPU Usage\n");
	printf("======================================================="
		"=========================\n");
	for(i=0;i<jobstotal;i++)
	{
		if(sortedjobs[i]->funcdata)
		{
			if(!sortedjobs[i]->funcdata->line)l=printf("%s (%s) ",
				sortedjobs[i]->funcdata->func,
				sortedjobs[i]->funcdata->file);
			else l=printf("%s (%s:%d) ",
				sortedjobs[i]->funcdata->func,
				sortedjobs[i]->funcdata->file,
				sortedjobs[i]->funcdata->line);
		}
		else if(sortedjobs[i]->funcmap)
		{
			l=printf("%s+%p ",brief?sortedjobs[i]->funcmap->brief:
				sortedjobs[i]->funcmap->file,
				(void *)(sortedjobs[i]->func-
				sortedjobs[i]->funcmap->start));
		}
		else l=printf("%p ",(void *)(sortedjobs[i]->func));

		while(l<43)l+=printf("          ");
		while(l<53)l+=printf(" ");

		if(mode<2)printf(" %7llu %7llu.%09llu\n",sortedjobs[i]->calls,
			sortedjobs[i]->nsecs/1000000000,
			sortedjobs[i]->nsecs%1000000000);
		else printf(" %7llu %7llu.%09llu\n",sortedjobs[i]->calls,
			sortedjobs[i]->avg/1000000000,
			sortedjobs[i]->avg%1000000000);
	}

	return 0;
}

static int search_caller(int funcid)
{
	int i=base;
	int x=base;

	while(1)
	{
		if(i>=tracetotal)
		{
			if(!x)return -1;
			i-=x;
		}
		else if(sortedcaller[i]->callerid<funcid)
		{
			if(!x)return -1;
			i+=x;
		}
		else if(sortedcaller[i]->callerid>funcid)
		{
			if(!i||!x)return -1;
			i-=x;
		}
		else
		{
			while(i&&sortedcaller[i-1]->callerid==funcid)i--;
			return i;
		}

		x>>=1;
	}
}

static int search_func(int funcid)
{
	int i=base;
	int x=base;

	while(1)
	{
		if(i>=tracetotal)
		{
			if(!x)return -1;
			i-=x;
		}
		else if(sorted[i]->funcid<funcid)
		{
			if(!x)return -1;
			i+=x;
		}
		else if(sorted[i]->funcid>funcid)
		{
			if(!i||!x)return -1;
			i-=x;
		}
		else
		{
			while(i&&sorted[i-1]->funcid==funcid)i--;
			return i;
		}

		x>>=1;
	}
}

static void fwalk(int idx,int level,int brief)
{
	int i;
	int funcid=sorted[idx]->funcid;
	int callerid=-1;
	int cidx;

	for(i=0;i<level;i++)printf(" ");
	if(sorted[idx]->funcdata)
	{
		if(!sorted[idx]->funcdata->line)printf("%s  (%s)\n",
			sorted[idx]->funcdata->func,
			sorted[idx]->funcdata->file);
		else printf("%s  (%s:%d)\n",
			sorted[idx]->funcdata->func,
			sorted[idx]->funcdata->file,
			sorted[idx]->funcdata->line);
	}
	else if(sorted[idx]->funcmap)
	{
		printf("%s+%p\n",brief?sorted[idx]->funcmap->brief:
				sorted[idx]->funcmap->file,
				(void *)(sorted[idx]->func-
				sorted[idx]->funcmap->start));
	}
	else printf("%p\n",(void *)sorted[idx]->func);

	while(idx<tracetotal&&sorted[idx]->funcid==funcid)
	{
		if(callerid!=-1&&callerid==sorted[idx]->callerid)
		{
			idx++;
			continue;
		}
		else callerid=sorted[idx]->callerid;

		if(sorted[idx]->callerid!=-1)
			if((cidx=search_func(sorted[idx]->callerid))!=-1)
				fwalk(cidx,level+2,brief);
		idx++;
	}
}

static int tree(char *func,int brief)
{
	int i;

	if(!func)printf("\nComplete function call tree:\n\n");
	else printf("\nFunction call tree for %s:\n\n",func);

	if(!func)for(i=0;i<tracetotal;i++)
	{
		if(i&&sorted[i-1]->func==sorted[i]->func)continue;
		if(sorted[i]->funcid==-1)continue;
		if(search_caller(sorted[i]->funcid)!=-1)continue;
		fwalk(i,0,brief);
	}
	else
	{
		for(i=0;i<tracetotal;i++)if(sorted[i]->funcdata)
			if(!strcmp(func,sorted[i]->funcdata->func))
		{
			fwalk(i,0,brief);
			return 0;
		}

		return -1;
	}

	return 0;
}

static int summary(int brief)
{
	int i;
	unsigned long long d=0;
	unsigned long long n=0;
	unsigned long long c=0;
	char *ptr;

	for(i=0;i<tracetotal;i++)
	{
		n+=sorted[i]->nsecs;
		c+=sorted[i]->calls;
	}

	for(i=0;i<jobstotal;i++)
		if(sortedjobs[i]->depth>d)d=sortedjobs[i]->depth;

	printf("\nSummary:\n\n");

	if(cmd)
	{
		if(!brief)ptr=cmd;
		else
		{
			if((ptr=strrchr(cmd,'/')))ptr++;
			else ptr=cmd;
		}
		printf("Command: %s\n",ptr);
	}
	printf("Total run time: %llu.%09llu seconds\n",runtime/1000000000,
		runtime%1000000000);
	printf("Total CPU time: %llu.%09llu seconds\n",cpuuse/1000000000,
		cpuuse%1000000000);
	printf("Profiled CPU time: %llu.%09llu seconds\n",n/1000000000,
		n%1000000000);
	printf("Total function calls profiled: %llu\n",c);
	printf("Maximum parallelism: %d\n",maxthreads);
	printf("Maximum resident set size: %llu kbytes\n",maxrss);
	printf("Maximum profiling memory: %u kbytes\n",
		(fmem+cmem+maxthreads*tmem+1023)>>10);
	printf("Function pool usage: %u/%u\n",fpool,fsize);
	printf("Caller pool usage: %u/%u\n",cpool,csize);
	printf("Stack usage: %llu/%u\n",d,ssize);
	return 0;
}

static void usage(void)
{
	fprintf(stderr,
"Usage: profiler [-s] [-i instrumentation] [OPTIONS]\n"
"\n"
"Options:\n"
"-s                 print only file name, not full path to file\n"
"-i instrumentation profiling output, default is 'instrumentation.out'\n"
"-p <prefix>        process pathnames with chroot <prefix>\n"
"-g <adjust>        clock_gettime correction in nanoseconds\n"
"-S                 show summary\n"
"-c                 list functions sorted by amount of calls\n"
"-C                 list functions sorted by total cpu time used\n"
"-a                 list functions sorted by calls, show avg. cpu time per"
	" call\n"
"-A                 list functions sorted by average cpu time per call\n"
"-t                 list threads sorted by amount of invocations\n"
"-T                 list threads sorted by total cpu time used\n"
"-w                 list threads sorted by invocations, avg. cpu time per"
	" call\n"
"-W                 list threads sorted by average cpu time per call\n"
"-f                 show complete function call tree(s)\n"
"-F function        show function call tree for <function>\n"
"\n"
"Note that call trees are based on actually executed calls.\n");
	exit(1);
}

int main(int argc,char *argv[])
{
	int c;
	char *inst="instrumentation.out";
	int adj=0;
	int op=0;
	int brief=0;
	char *func=NULL;
	char *pfx=NULL;

	while((c=getopt(argc,argv,"aAcCfF:g:i:p:sStTwW"))!=-1)switch(c)
	{
	case 's':
		brief=1;
		break;

	case 'p':
		pfx=optarg;
		break;

	case 'g':
		adj=atoi(optarg);
		break;

	case 'c':
		op|=1;
		break;

	case 'C':
		op|=2;
		break;

	case 'a':
		op|=4;
		break;

	case 'A':
		op|=8;
		break;

	case 't':
		op|=16;
		break;

	case 'T':
		op|=32;
		break;

	case 'w':
		op|=64;
		break;

	case 'W':
		op|=128;
		break;

	case 'f':
		op|=256;
		break;

	case 'F':
		func=optarg;
		op|=512;
		break;

	case 'S':
		op|=1024;
		break;

	case 'i':
		inst=optarg;
		break;

	default:usage();
	}

	if(optind!=argc||!op||adj<0||adj>100000)usage();

	if(readtrace(inst,brief,pfx))return 1;
	if(adjust(adj))return 1;

	if(op&1)if(tops(0,brief))return 1;
	if(op&2)if(tops(1,brief))return 1;
	if(op&4)if(tops(2,brief))return 1;
	if(op&8)if(tops(3,brief))return 1;
	if(op&16)if(jobsproc(0,brief))return 1;
	if(op&32)if(jobsproc(1,brief))return 1;
	if(op&64)if(jobsproc(2,brief))return 1;
	if(op&128)if(jobsproc(3,brief))return 1;
	if(op&256)if(tree(NULL,brief))return 1;
	if(op&512)if(tree(func,brief))return 1;
	if(op&1024)if(summary(brief))return 1;
	return 0;
}
