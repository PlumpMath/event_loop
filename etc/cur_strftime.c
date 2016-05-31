#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/timeb.h> 
#include <string.h>

int
main(int argc, char *argv[])
{
    char outstr[200];
    time_t t,t1;unsigned long lt;
    struct tm *tmp,Tm;
	struct timeval tv;struct timezone tz;
	struct timeb tb;
	int bGMT=0;

#if 1 // for lfs_touch	
	if(argc>=4&&argv[3][0]=='-'&&argv[3][1]=='T'){//convert to time_t
		char *path=argv[1];
		unsigned long ct;
		char tmStr[256];

		sscanf(argv[2],"%lx",&ct);
		 // printf("%lu",ct);fflush(stdout);
		 t=(time_t)ct;
		tmp=localtime_r(&t,&Tm);
		sprintf(tmStr,"touch -t %04d%02d%02d%02d%02d.%02d %s",Tm.tm_year+1900,Tm.tm_mon+1,Tm.tm_mday,Tm.tm_hour,Tm.tm_min,Tm.tm_sec,path);
		printf("%s exec:%s\n",argv[0],tmStr);
		system(tmStr);
		return 0;
	}
#endif
	if(argc>=3&&argv[2][0]=='-'&&argv[2][1]=='t'){//convert to time_t
		int year,mon,mday,hour,min,sec;char weekdayStr[20],TZstr[64];
		// 2015. 04. 24. (±Ý) 15:30:47 KST
		sscanf(argv[1],"%04d. %02d. %02d. %s %02d:%02d:%02d %s",&year,&mon,&mday,weekdayStr,&hour,&min,&sec,TZstr);
		Tm.tm_year=year-1900;Tm.tm_mon=mon-1;Tm.tm_mday=mday;Tm.tm_hour=hour;Tm.tm_min=min;Tm.tm_sec=sec;Tm.tm_isdst=0;Tm.tm_wday=0;Tm.tm_yday=0;
		 t1=mktime(&Tm);
		 printf("%lu",t1);fflush(stdout);
		 if(argv[2][2]=='v'){
			 memset(&Tm,0,sizeof(Tm));
			 tmp=localtime_r(&t1,&Tm);
			fprintf(stderr,"\ntm: converted (%s) tm=%04d/%02d/%02d %02d:%02d:%02d wday=%d,yday=%d(dst=%d)\n",bGMT? "GMT":"LOCAL",tmp->tm_year+1900,tmp->tm_mon+1,tmp->tm_mday,tmp->tm_hour,tmp->tm_min,tmp->tm_sec,tmp->tm_wday,tmp->tm_yday,tmp->tm_isdst);	fflush(stderr);
		 }
		return 0;
	}
	t=time(NULL);// unixTime get local time_t
	ftime(&tb);
	if(!gettimeofday(&tv, &tz)){
		printf("time: %lusec\n",(unsigned long)t);
		tmp=gmtime_r(&t,&Tm);
		strftime(outstr, sizeof(outstr), "%z", &Tm);
		printf("gmtime_r TZ detected=%s\n",outstr);// TZ=+0000
		tmp=localtime_r(&t,&Tm);
		strftime(outstr, sizeof(outstr), "%z", &Tm);
		printf("localtime_r TZ detected=%s\n",outstr);// TZ=+0900
		
		printf("gettimeofday: %lu.%06lusec\n",tv.tv_sec,tv.tv_usec);
		printf("TZ: MINUTESWEST=%d(TZ=%d),DST=%d\n",tz.tz_minuteswest,24*60+(int)tz.tz_minuteswest,tz.tz_dsttime);
		
		printf("ftime: %lu.%03dsec\n",tb.time,tb.millitm);
		printf("tz: minuteswest=%d(TZ=%d),dstflag=%d\n",tb.timezone,24*60+(int)tb.timezone,tb.dstflag);
	}
   if(argc>=4)t=(time_t)strtol(argv[3],NULL,10);
   else{
	system("date");
   	t = time(NULL);// unixTime get local time_t
   }
   printf("elapsed: local time t=%lusec after 1970\n",(unsigned long)t);
   if(argc>=3&&argv[2][0]=='-'&&argv[2][1]=='g')bGMT=1;
   if(bGMT)
    tmp = gmtime(&t);// get UTC tm(modify time)
   else
    tmp = localtime(&t);// get local tm(not modify time)
printf("tm: converted (%s) tm=%04d/%02d/%02d %02d:%02d:%02d wday=%d,yday=%d(dst=%d)\n",bGMT? "GMT":"LOCAL",tmp->tm_year+1900,tmp->tm_mon+1,tmp->tm_mday,tmp->tm_hour,tmp->tm_min,tmp->tm_sec,tmp->tm_wday,tmp->tm_yday,tmp->tm_isdst);	
	Tm=*tmp;
    t1=mktime(&Tm);
printf("converted local time t1=%lusec after 1970\n",(unsigned long)t1);	
    if (tmp == NULL) {
        perror("localtime/gmtime");
        exit(EXIT_FAILURE);
    }
	Tm=*tmp;
	printf("%04d-%02d-%02d %02d:%02d:%02d(%s)\n",Tm.tm_year+1900,Tm.tm_mon+1,Tm.tm_mday,Tm.tm_hour,Tm.tm_min,Tm.tm_sec,argv[2]); // .000
	
   if (strftime(outstr, sizeof(outstr), argv[1], tmp) == 0) {
        fprintf(stderr, "strftime returned 0");
        exit(EXIT_FAILURE);
    }

   printf("Result string is \"%s\"\n", outstr);
   if(!strcmp(argv[1],"%s")){
	   sscanf(outstr,"%lu",&lt);
	   if(lt==t)printf("converted time has LOCAL time_t\n");
	   else printf("converted time has DIFF %dsec(%dhour)\n",(int)(t-lt),(int)(t-lt)/60/60);
   }
    exit(EXIT_SUCCESS);
}
