// Copyright (c) 2015-2015 Y.W.Hwang 

#undef UNICODE                    // Use ANSI WinAPI functions
#undef _UNICODE                   // Use multibyte encoding on Windows
#define _MBCS                     // Use multibyte encoding on Windows
#define _WIN32_WINNT 0x500        // Enable MIIM_BITMAP
#define _CRT_SECURE_NO_WARNINGS   // Disable deprecation warning in VS2005
#define _XOPEN_SOURCE 600         // For PATH_MAX on linux
#undef WIN32_LEAN_AND_MEAN        // Let windows.h always include winsock2.h

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "net_skeleton.h"

#include "mongoose.h"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>  // For chdir()
#include <winsvc.h>
#include <shlobj.h>

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#ifndef S_ISDIR
#define S_ISDIR(x) ((x) & _S_IFDIR)
#endif

#define DIRSEP '\\'
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define sleep(x) Sleep((x) * 1000)
#define abs_path(rel, abs, abs_size) _fullpath((abs), (rel), (abs_size))
#define SIGCHLD 0
typedef struct _stat file_stat_t;
#define stat(x, y) _stat((x), (y))
#else
typedef struct stat file_stat_t;
#include <sys/wait.h>
#include <unistd.h>

#ifdef IOS
#include <ifaddrs.h>
#endif

#define DIRSEP '/'
#ifndef __CYGWIN__
#define __cdecl
#endif
#define abs_path(rel, abs, abs_size) realpath((rel), (abs))
#endif // _WIN32

#define MAX_OPTIONS 100
#define MAX_CONF_FILE_LINE_SIZE (8 * 1024)

#ifndef MVER
#define MVER MONGOOSE_VERSION
#endif


//////////////////////////////////eloop.c
#define HYWMON_SUPPORT_WDTMON 0
#define ELOOP_TIMECHANGE_PATCH 0
#define         ELOOP_TIMECHANGE_AUTO 0
#define ELOOP_WATCHDOG_ADDON 0
#define ELOOP_EXCEPTION_ADDON 0
#define ELOOP_MULTIDATA_PATCH 1

#if ELOOP_MULTIDATA_PATCH
struct eloop_data;
#define ELOOP_CTX_ARG 	struct eloop_data* p_eloop_data,
#define ELOOP_CTX_ARG_ONLY struct eloop_data* p_eloop_data
#define ELOOP_CTX_PTR p_eloop_data,
#define ELOOP_CTX_PTR_ONLY p_eloop_data
#define eloop (*p_eloop_data)
#else
#define ELOOP_CTX_ARG 	/*struct eloop_data* p_eloop_data,*/
#define ELOOP_CTX_ARG_ONLY void
#define ELOOP_CTX_PTR /*p_eloop_data,*/
#define ELOOP_CTX_PTR_ONLY /*p_eloop_data*/
#endif

/**
 * ELOOP_ALL_CTX - eloop_cancel_timeout() magic number to match all timeouts
 */
#define ELOOP_ALL_CTX (void *) -1
/**
 * eloop_timeout_handler - eloop timeout event callback type
 * @eloop_ctx: Registered callback context data (eloop_data)
 * @sock_ctx: Registered callback context data (user_data)
 */
typedef void (*eloop_timeout_handler)(void *eloop_data, void *user_ctx);
/**
 * eloop_init() - Initialize global event loop data
 * @user_data: Pointer to global data passed as eloop_ctx to signal handlers
 * Returns: 0 on success, -1 on failure
 *
 * This function must be called before any other eloop_* function. user_data
 * can be used to configure a global (to the process) pointer that will be
 * passed as eloop_ctx parameter to signal handlers.
 */
/**
 * eloop_signal_handler - eloop signal event callback type
 * @sig: Signal number
 * @eloop_ctx: Registered callback context data (global user_data from
 * eloop_init() call)
 * @signal_ctx: Registered callback context data (user_data from
 * eloop_register_signal(), eloop_register_signal_terminate(), or
 * eloop_register_signal_reconfig() call)
 */
typedef void (*eloop_signal_handler)(int sig, void *eloop_ctx,
				     void *signal_ctx);

int eloop_init(ELOOP_CTX_ARG void *user_data);
/**
 * eloop_register_timeout - Register timeout
 * @secs: Number of seconds to the timeout
 * @usecs: Number of microseconds to the timeout
 * @handler: Callback function to be called when timeout occurs
 * @eloop_data: Callback context data (eloop_ctx)
 * @user_data: Callback context data (sock_ctx)
 * Returns: 0 on success, -1 on failure
 *
 * Register a timeout that will cause the handler function to be called after
 * given time.
 */
int eloop_register_timeout(ELOOP_CTX_ARG unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data);

/**
 * eloop_cancel_timeout - Cancel timeouts
 * @handler: Matching callback function
 * @eloop_data: Matching eloop_data or %ELOOP_ALL_CTX to match all
 * @user_data: Matching user_data or %ELOOP_ALL_CTX to match all
 * Returns: Number of cancelled timeouts
 *
 * Cancel matching <handler,eloop_data,user_data> timeouts registered with
 * eloop_register_timeout(). ELOOP_ALL_CTX can be used as a wildcard for
 * cancelling all timeouts regardless of eloop_data/user_data.
 */
int eloop_cancel_timeout(ELOOP_CTX_ARG eloop_timeout_handler handler,
			 void *eloop_data, void *user_data);
/**
 * eloop_terminate - Terminate event loop
 *
 * Terminate event loop even if there are registered events. This can be used
 * to request the program to be terminated cleanly.
 */
void eloop_terminate(ELOOP_CTX_ARG_ONLY);

/**
 * eloop_destroy - Free any resources allocated for the event loop
 *
 * After calling eloop_destroy(), other eloop_* functions must not be called
 * before re-running eloop_init().
 */
void eloop_destroy(ELOOP_CTX_ARG_ONLY);

/**
 * eloop_terminated - Check whether event loop has been terminated
 * Returns: 1 = event loop terminate, 0 = event loop still running
 *
 * This function can be used to check whether eloop_terminate() has been called
 * to request termination of the event loop. This is normally used to abort
 * operations that may still be queued to be run when eloop_terminate() was
 * called.
 */
int eloop_terminated(ELOOP_CTX_ARG_ONLY);
/**
 * eloop_get_user_data - Get global user data
 * Returns: user_data pointer that was registered with eloop_init()
 */
void * eloop_get_user_data(ELOOP_CTX_ARG_ONLY);
void eloop_tmp_terminate(ELOOP_CTX_ARG int tmp_term_val);
/////
typedef long os_time_t;

/**
 * os_sleep - Sleep (sec, usec)
 * @sec: Number of seconds to sleep
 * @usec: Number of microseconds to sleep
 */
void os_sleep(os_time_t sec, os_time_t usec);

struct os_time {
	os_time_t sec;
	os_time_t usec;
};

/**
 * os_get_time - Get current time (sec, usec)
 * @t: Pointer to buffer for the time
 * Returns: 0 on success, -1 on failure
 */
int os_get_time(struct os_time *t);
/* Helper macros for handling struct os_time */

#define os_time_before(a, b) \
	((a)->sec < (b)->sec || \
	 ((a)->sec == (b)->sec && (a)->usec < (b)->usec))

#define os_time_sub(a, b, res) do { \
	(res)->sec = (a)->sec - (b)->sec; \
	(res)->usec = (a)->usec - (b)->usec; \
	if ((res)->usec < 0) { \
		(res)->sec--; \
		(res)->usec += 1000000; \
	} \
} while (0)

////
void os_sleep(os_time_t sec, os_time_t usec)
{
        #if 0   /* original */
	if (sec)
		sleep(sec);
	if (usec)
		usleep(usec);
        #else
        struct timeval t;
        t.tv_sec = sec;
        t.tv_usec = usec;
        select(0, NULL, NULL, NULL, &t);
        #endif
}


int os_get_time(struct os_time *t)
{
	int res;
	struct timeval tv;
	res = gettimeofday(&tv, NULL);
	t->sec = tv.tv_sec;
	t->usec = tv.tv_usec;
	return res;
}

#ifdef OS_NO_C_LIB_DEFINES
void * os_memcpy(void *dest, const void *src, size_t n)
{
	char *d = dest;
	const char *s = src;
	while (n--)
		*d++ = *s++;
	return dest;
}


void * os_memmove(void *dest, const void *src, size_t n)
{
	if (dest < src)
		os_memcpy(dest, src, n);
	else {
		/* overlapping areas */
		char *d = (char *) dest + n;
		const char *s = (const char *) src + n;
		while (n--)
			*--d = *--s;
	}
	return dest;
}


void * os_memset(void *s, int c, size_t n)
{
	char *p = s;
	while (n--)
		*p++ = c;
	return s;
}


int os_memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *p1 = s1, *p2 = s2;

	if (n == 0)
		return 0;

	while (*p1 == *p2) {
		p1++;
		p2++;
		n--;
		if (n == 0)
			return 0;
	}

	return *p1 - *p2;
}

void * os_malloc(size_t size)
{
	return malloc(size);
}


void * os_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void * os_zalloc(size_t size)
{
	void *n = os_malloc(size);
	if (n)
		os_memset(n, 0, size);
	return n;
}


void os_free(void *ptr)
{
	free(ptr);
}
#else
#ifndef os_malloc
#define os_malloc(s) malloc((s))
#endif
#ifndef os_realloc
#define os_realloc(p, s) realloc((p), (s))
#endif
#ifndef os_free
#define os_free(p) free((p))
#endif

#ifndef os_memcpy
#define os_memcpy(d, s, n) memcpy((d), (s), (n))
#endif
#ifndef os_memmove
#define os_memmove(d, s, n) memmove((d), (s), (n))
#endif
#ifndef os_memset
#define os_memset(s, c, n) memset(s, c, n)
#endif
#ifndef os_memcmp
#define os_memcmp(s1, s2, n) memcmp((s1), (s2), (n))
#endif
#endif
////
void call_eloop_timeout_handler(eloop_timeout_handler handler,void *eloop_data, void *user_ctx);
struct eloop_signal {
	int sig;
	void *user_data;
	eloop_signal_handler handler;
	int signaled;
};

struct eloop_timeout {
	struct os_time time;
	void *eloop_data;
	void *user_data;
#if ELOOP_WATCHDOG_ADDON
	int bIsWDTimeout;
#endif /*ELOOP_WATCHDOG_ADDON*/
	eloop_timeout_handler handler;
	struct eloop_timeout *next;
};
struct eloop_data {
	void *user_data;

	struct eloop_timeout *timeout;
#if ELOOP_WATCHDOG_ADDON
	struct eloop_timeout *timeout_currentWDT;
    //int nCurrentWDTime;
#endif /*ELOOP_WATCHDOG_ADDON*/

	int signal_count;
	struct eloop_signal *signals;
	int signaled;

	int pending_terminate;

	int terminate;

	int tmp_terminate;
};

#if ELOOP_MULTIDATA_PATCH
#define NUM_ELOOP_MAX 32
struct eloop_data eloopTbl[NUM_ELOOP_MAX];
struct eloop_data *p_eloopTbl[NUM_ELOOP_MAX]={&eloopTbl[0],&eloopTbl[1],&eloopTbl[2],&eloopTbl[3],&eloopTbl[4],&eloopTbl[5],&eloopTbl[6],&eloopTbl[7]};
#define AUTO_INIT_CTX_TBL() \
		if(!p_eloopTbl[NUM_ELOOP_MAX-1]){ \
			int i; \
			for(i=0;i<NUM_ELOOP_MAX;i++)p_eloopTbl[i]=&eloopTbl[i]; \
		}
#else
static struct eloop_data eloop;
#define AUTO_INIT_CTX_TBL() /**/
#endif

static void eloop_process_pending_signals(ELOOP_CTX_ARG_ONLY);

int eloop_init(ELOOP_CTX_ARG void *user_data)
{
	AUTO_INIT_CTX_TBL();
	os_memset(&eloop, 0, sizeof(eloop));
	eloop.user_data = user_data;
#ifdef WIN32
	//eloop.max_sock=-1;//for no socket case!!
#endif
#if ELOOP_EXCEPTION_ADDON&&!defined(CONFIG_NATIVE_WINDOWS)
	fprintf(stderr,"eloop_init: init_except_handler!!\n");
    init_except_handler();
#endif
	return 0;
}
#if ELOOP_TIMECHANGE_PATCH||ELOOP_EXCEPTION_ADDON
struct os_time GetTimeLeft(struct os_time *pDstTime)
{
        struct os_time curtime,dsttime;
		
        os_get_time(&curtime);
        dsttime=*pDstTime;
		if(dsttime.sec<curtime.sec||(dsttime.usec<curtime.usec&&dsttime.sec<=0)){//<=0 !!
			dsttime.usec=0;dsttime.sec=0;
		}
        else{
			dsttime.sec-=curtime.sec;
			if(dsttime.usec<curtime.usec){
				dsttime.usec=(999999U-curtime.usec+dsttime.usec+1);//prevent negative value
				dsttime.sec--;
				//printf("GetTimeLeft:dsttime.sec=%d,dsttime.usec=%d\n",dsttime.sec,dsttime.usec);
			}
			else dsttime.usec-=curtime.usec;
		}
        return dsttime;
}
int CmpTime(struct os_time *pDstTime,struct os_time *pSrcTime)
{
        if(pDstTime->sec>pSrcTime->sec)return 1;//much bigger
        else if(pDstTime->sec==pSrcTime->sec){
                if(pDstTime->usec>pSrcTime->usec)return 1;//little bigger
                else if(pDstTime->usec==pSrcTime->usec)return 0;//same as
        }
        return -1;//less than
}
#endif
#if ELOOP_WATCHDOG_ADDON
void OnRegisterWDT(struct eloop_timeout *timeout,unsigned int secs, unsigned int usecs);
void OnUnregisterWDT(struct eloop_timeout *timeout);
int CmpTime(struct os_time *pDstTime,struct os_time *pSrcTime);
struct os_time GetTimeLeft(struct os_time *pDstTime);
int eloop_register_timeout0(ELOOP_CTX_ARG unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data, int bIsWDTimeout)
#else /*ELOOP_WATCHDOG_ADDON*/
int eloop_register_timeout(ELOOP_CTX_ARG unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data)
#endif /*!ELOOP_WATCHDOG_ADDON*/
{
	struct eloop_timeout *timeout, *tmp, *prev;

	timeout = os_malloc(sizeof(*timeout));
	if (timeout == NULL)
		return -1;
	os_get_time(&timeout->time);
	timeout->time.sec += secs;
	timeout->time.usec += usecs;
	while (timeout->time.usec >= 1000000) {
		timeout->time.sec++;
		timeout->time.usec -= 1000000;
	}
	timeout->eloop_data = eloop_data;
	timeout->user_data = user_data;
	timeout->handler = handler;
	timeout->next = NULL;

#if ELOOP_WATCHDOG_ADDON
        timeout->bIsWDTimeout=bIsWDTimeout;
	//if(bIsWDTimeout)OnRegisterWDT(timeout,secs,usecs);
#endif /*ELOOP_WATCHDOG_ADDON*/

	if (eloop.timeout == NULL) {
		eloop.timeout = timeout;
#if ELOOP_WATCHDOG_ADDON
	        if(bIsWDTimeout)goto regWDT;
#endif /*ELOOP_WATCHDOG_ADDON*/

		return 0;
	}

	prev = NULL;
	tmp = eloop.timeout;
	while (tmp != NULL) {
		if (os_time_before(&timeout->time, &tmp->time))
			break;
		prev = tmp;
		tmp = tmp->next;
	}

	if (prev == NULL) {
		timeout->next = eloop.timeout;
		eloop.timeout = timeout;
	} else {
		timeout->next = prev->next;
		prev->next = timeout;
	}
#if ELOOP_WATCHDOG_ADDON
regWDT:	
#if HYWMON_SUPPORT_WDTMON
        if(bIsWDTimeout)OnRegisterWDT(ELOOP_CTX_PTR timeout,secs,usecs);
#endif
#endif /*ELOOP_WATCHDOG_ADDON*/
	return 0;
}


#if ELOOP_WATCHDOG_ADDON
int eloop_cancel_timeout0(ELOOP_CTX_ARG eloop_timeout_handler handler,
			 void *eloop_data, void *user_data, int bIsWDTimeout)
#else /*ELOOP_WATCHDOG_ADDON*/
int eloop_cancel_timeout(ELOOP_CTX_ARG eloop_timeout_handler handler,
			 void *eloop_data, void *user_data)
#endif /*!ELOOP_WATCHDOG_ADDON*/
{
	struct eloop_timeout *timeout, *prev, *next;
	int removed = 0;

	prev = NULL;
	timeout = eloop.timeout;
	while (timeout != NULL) {
		next = timeout->next;

		if (timeout->handler == handler &&
		    (timeout->eloop_data == eloop_data ||
		     eloop_data == ELOOP_ALL_CTX) &&
		    (timeout->user_data == user_data ||
		     user_data == ELOOP_ALL_CTX)) {
#if ELOOP_WATCHDOG_ADDON
#if HYWMON_SUPPORT_WDTMON
			if(timeout->bIsWDTimeout||bIsWDTimeout)OnUnregisterWDT(ELOOP_CTX_PTR timeout);
#endif
#endif /*ELOOP_WATCHDOG_ADDON*/
			if (prev == NULL)
				eloop.timeout = next;
			else
				prev->next = next;
			os_free(timeout);
			removed++;
		} else
			prev = timeout;

		timeout = next;
	}

	return removed;
}
#if ELOOP_TIMECHANGE_PATCH//NTP notify�߰�
#ifndef WIN32	//for ELOOP_TIMECHANGE_AUTO
#include <sys/times.h> 
#endif
struct os_time GetTimeElapsed(struct os_time *pDstTime)
{
        struct os_time curtime,dsttime;
		
        os_get_time(&curtime);
        dsttime=*pDstTime;
		if(dsttime.sec>curtime.sec||(dsttime.usec>curtime.usec&&curtime.sec<=0)){//<=0 !!
			curtime.usec=0;curtime.sec=0;
		}
        else{
			curtime.sec-=dsttime.sec;
			if(curtime.usec<dsttime.usec){
				curtime.usec=(0xFFFFFFFF-dsttime.usec+curtime.usec+1);//prevent negative value
				curtime.usec--;
			}
			else curtime.usec-=dsttime.usec;
		}
        return curtime;
}
struct os_time AddTime(struct os_time *pTimeA,struct os_time *pTimeDiff)
{
        struct os_time newtime;
        
        newtime.sec=pTimeA->sec+pTimeDiff->sec;
        newtime.usec=pTimeA->usec+pTimeDiff->usec;
        if(newtime.usec>=1000000){newtime.usec-=1000000;newtime.sec++;}
        return newtime;
}
struct os_time SubTime(struct os_time *pTimeA,struct os_time *pTimeDiff)
{
        struct os_time newtime;
        long usec;
        
        newtime.sec=pTimeA->sec-pTimeDiff->sec;
        usec=(long)pTimeA->usec-(long)pTimeDiff->usec;
        if(usec<0){usec+=1000000;newtime.sec--;}
        newtime.usec=usec;
        return newtime;
}
unsigned long debug_clockdiff=0;
void eloop_timeUpdate(ELOOP_CTX_ARG int sign,struct os_time *pDifftime)
{
	struct eloop_timeout *timeout, *prev, *next;
#if 1//20110516 ywhwangpatch for UpdateTime BUG
	int bHasPrehistoric=0,bIsPrehistoric=0;
#endif
	
//if(pDifftime->sec>2)
//fprintf(stderr,"TU2:%c%d.%06d\r\n",(sign>0? '+':'-'),pDifftime->sec,pDifftime->usec);
        if(sign==0||pDifftime->sec<=0&&pDifftime->usec<=900000)return;//less than 1900ms
//fprintf(stderr,"TUv:%c%d.%06d(%d)\r\n",(sign>0? '+':'-'),pDifftime->sec,pDifftime->usec,debug_clockdiff);
	prev = NULL;
	timeout = eloop.timeout;
	while (timeout != NULL) {
		next = timeout->next;

#if 0//dbg
{FILE* fpCons=fopen("/dev/console","w");fprintf(fpCons,"time:%ld.%06ld",timeout->time.sec,timeout->time.usec);
#endif
#if 1//20110516 ywhwangpatch for UpdateTime BUG
	if(timeout->time.sec<=1000000000)bIsPrehistoric=1;//946684990(2011.01.01/00:02:55//1305594060(2011.05.17/01:01:00)
	else bIsPrehistoric=0;
	if(bIsPrehistoric)bHasPrehistoric=1;
	if(bIsPrehistoric||!bIsPrehistoric&&!bHasPrehistoric)
                timeout->time=(sign>0? AddTime(&timeout->time,pDifftime):SubTime(&timeout->time,pDifftime));
#else
                timeout->time=(sign>0? AddTime(&timeout->time,pDifftime):SubTime(&timeout->time,pDifftime));
#endif
#if 0//dbg
fprintf(fpCons,"->time:%ld.%06ld\n",timeout->time.sec,timeout->time.usec);fclose(fpCons);}
#endif
                
		prev = timeout;
		timeout = next;
	}
}
void eloop_timeChange0(ELOOP_CTX_ARG struct os_time *pNewTime,struct os_time *pCurtime)
{
        struct os_time difftime;
        int sign;

        /*calc diff time*/
        sign=CmpTime(pNewTime,pCurtime);
        if(sign==0)return;
//fprintf(stderr,"TU1:%c,%d.%06d,%d.%06d\r\n",(sign>0? '+':(sign==0? ' ':'-')),pNewTime->sec,pNewTime->usec,pCurtime->sec,pCurtime->usec);
        if(sign>0)difftime=SubTime(pNewTime,pCurtime);
        else difftime=SubTime(pCurtime,pNewTime);
        eloop_timeUpdate(ELOOP_CTX_PTR sign,&difftime);
}
void eloop_timeChange(ELOOP_CTX_ARG unsigned long new_sec,unsigned long new_usec)
{
#if !ELOOP_TIMECHANGE_AUTO
	struct os_time newtime,curtime;

        /*get new time*/
        newtime.sec=new_sec;newtime.usec=new_usec;
        /*get cur time*/
        os_get_time(&curtime);
        /*update*/
        eloop_timeChange0(ELOOP_CTX_PTR &newtime,&curtime);
#else
        fprintf(stderr,"TUntp:%ld.%06ld\n",new_sec,new_usec);
#endif
}
#define MY_CLOCKS_PER_SEC 100 /*sysconf(_SC_CLK_TCK)*/
void eloop_timeChangeAuto(ELOOP_CTX_ARG clock_t t1,clock_t t0,struct os_time *pNewTime,struct os_time *pPrevTime)
{
#if ELOOP_TIMECHANGE_AUTO
	struct os_time newtime,curtime;
        long usec;
        unsigned long clockdiff;
        
        if(t1>t0)clockdiff=t1-t0;
        else clockdiff=0xFFFFFFFF-t0+t1+1;
        //if(clockdiff<0)return;//every 72 minutes??
        debug_clockdiff=clockdiff;
        /*get new time*/
        ;
        /*calc cur time*/
        curtime=*pPrevTime;
        curtime.sec+=clockdiff/MY_CLOCKS_PER_SEC;
        usec=(long)curtime.usec+(clockdiff%MY_CLOCKS_PER_SEC*1000000/MY_CLOCKS_PER_SEC);
        if(usec>1000000){usec-=1000000;curtime.sec++;}
        else if(usec<0){usec+=1000000;curtime.sec--;}
        curtime.usec=usec;
        /*update*/
        eloop_timeChange0(ELOOP_CTX_PTR pNewTime,&curtime);
#else
        //fprintf(stderr,"TUauto:%d.%06d\n",new_sec,new_usec);
#endif
}
#endif
void eloop_tmp_terminate(ELOOP_CTX_ARG int tmp_term_val)
{
	eloop.tmp_terminate=tmp_term_val;
}
void eloop_terminate(ELOOP_CTX_ARG_ONLY)
{
	eloop.terminate = 1;
}


void eloop_destroy(ELOOP_CTX_ARG_ONLY)
{
	struct eloop_timeout *timeout, *prev;

	timeout = eloop.timeout;
	while (timeout != NULL) {
		prev = timeout;
		timeout = timeout->next;
		os_free(prev);
	}
}


int eloop_terminated(ELOOP_CTX_ARG_ONLY)
{
	return eloop.terminate;
}
void * eloop_get_user_data(ELOOP_CTX_ARG_ONLY)
{
	return eloop.user_data;
}
#if ELOOP_WATCHDOG_ADDON
#if HYWMON_SUPPORT_WDTMON
int eloop_register_watchdog_timeout(ELOOP_CTX_ARG unsigned int secs, unsigned int usecs,
			   /*eloop_timeout_handler handler,*/
			   void *eloop_data, void *user_data)
{
        return eloop_register_timeout0(ELOOP_CTX_PTR secs,usecs,eloop_timeout_handler_for_WDT,eloop_data,user_data,1);
}
int eloop_cancel_watchdog_timeout(ELOOP_CTX_ARG /*eloop_timeout_handler handler,*/
			 void *eloop_data, void *user_data)
{
        return eloop_cancel_timeout0(ELOOP_CTX_PTR eloop_timeout_handler_for_WDT,eloop_data,user_data,1);
}
#endif
int eloop_register_timeout(ELOOP_CTX_ARG unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data)
{
        return eloop_register_timeout0(ELOOP_CTX_PTR secs,usecs,handler,eloop_data,user_data,0);
}
int eloop_cancel_timeout(ELOOP_CTX_ARG eloop_timeout_handler handler,
			 void *eloop_data, void *user_data)
{
        return eloop_cancel_timeout0(ELOOP_CTX_PTR handler,eloop_data,user_data,0);
}
#if HYWMON_SUPPORT_WDTMON
int dbgWdt_eloop=0;
void OnRegisterWDT(ELOOP_CTX_ARG struct eloop_timeout *timeout,unsigned int secs, unsigned int usecs)//after inserted!!
{
if(dbgWdt_eloop)printf("OnRegisterWDT:%x(Id=%d)\n",(unsigned int)timeout,(unsigned int)timeout->user_data);
	timeout->bIsWDTimeout=1;
	if(eloop.timeout_currentWDT==NULL){
if(dbgWdt_eloop)printf("OnRegisterWDTfirst:%x(Id=%d)\n",(unsigned int)timeout,(unsigned int)timeout->user_data);
		//eloop.nCurrentWDTime=secs;
		eloop_startWDT(eloop.user_data,secs,usecs);//start
		eloop.timeout_currentWDT=timeout;
	}
	else{
#if 1
		//find next WDT
		struct eloop_timeout *timeoutWDT;struct os_time timeleft;
		
		timeoutWDT=eloop.timeout;
		while(timeoutWDT != NULL){
			if(timeoutWDT->bIsWDTimeout){
			        if(timeoutWDT==timeout){
if(dbgWdt_eloop)printf("OnRegisterWDTupdate2ME:%x(Id=%d)\n",(unsigned int)timeoutWDT,(unsigned int)timeoutWDT->user_data);
        				timeleft=GetTimeLeft(&timeoutWDT->time);
        				//eloop.nCurrentWDTime=timeleft.sec;
        				eloop_restartWDT(eloop.user_data,timeleft.sec,timeleft.usec);//stop&start
				        eloop.timeout_currentWDT=timeoutWDT;
			        }
			        else{
if(dbgWdt_eloop)printf("OnRegisterWDTupdate2other:%x(Id=%d)\n",(unsigned int)timeoutWDT,(unsigned int)timeoutWDT->user_data);
				        ;//no action
				}
				break;
			}
			timeoutWDT=timeoutWDT->next;
		}
#else
		if(CmpTime(&eloop.timeout_currentWDT->time,&timeout->time)){
			//eloop.nCurrentWDTime=secs;
			eloop_restartWDT(eloop.user_data,secs,usecs);//stop&start
			eloop.timeout_currentWDT=timeout;
		}
		else{
			;//no action
		}
#endif
	}
}
void OnUnregisterWDT(ELOOP_CTX_ARG struct eloop_timeout *timeout)//before ejected!!
{
if(dbgWdt_eloop)printf("OnUnregisterWDT:%x(Id=%d)\n",(unsigned int)timeout,(unsigned int)timeout->user_data);
	//if(timeout->bIsWDTimeout==bIsWDTimeout);
	if(eloop.timeout_currentWDT==NULL){
printf("OnUnregisterWDTnone??:%x(Id=%d)\n",(unsigned int)timeout,(unsigned int)timeout->user_data);
		;//??
	}
	else{
		if(eloop.timeout_currentWDT==timeout){
			//find next WDT
			struct eloop_timeout *timeoutWDT;struct os_time timeleft;

			//eloop.nCurrentWDTime=0;
			eloop.timeout_currentWDT=NULL;
			timeoutWDT=timeout->next;//20110520 ywhwang patch!!!
			while(timeoutWDT != NULL){
				if(timeoutWDT->bIsWDTimeout){
					timeleft=GetTimeLeft(&timeoutWDT->time);
					//eloop.nCurrentWDTime=timeleft.sec;
					eloop.timeout_currentWDT=timeoutWDT;
if(dbgWdt_eloop)printf("OnUnregisterWDTupdate2ME:%x(Id=%d)\n",(unsigned int)timeoutWDT,(unsigned int)timeoutWDT->user_data);
					break;
				}
			        timeoutWDT=timeoutWDT->next;
			}
			timeout->bIsWDTimeout=0;//dbg
			if(!eloop.timeout_currentWDT)
			        eloop_stopWDT(ELOOP_CTX_PTR eloop.user_data);
			else
				eloop_restartWDT(ELOOP_CTX_PTR eloop.user_data,timeleft.sec,timeleft.usec);//stop&start
		}
		else{
			timeout->bIsWDTimeout=0;//dbg
			;//no action
		}
	}
}
#endif
#endif /*ELOOP_WATCHDOG_ADDON*/
#if ELOOP_EXCEPTION_ADDON
extern char debug_str[4096+1];
extern char* debug_ptr;
void call_eloop_timeout_handler(ELOOP_CTX_ARG eloop_timeout_handler handler,void *eloop_data, void *user_ctx)
{FIND_THREAD_EXCEPT_CONTEXT(e,"call_eloop_timeout_handler");
	Try{
		//printf("monitoring exception!!\n");fflush(stdout);
		SetPendingMonitor(2);
		handler(eloop_data,user_ctx);
		ClrPendingMonitor(2);
	}
	Catch(e){
		printf("call_eloop_timeout_handler%lx(%lu,%lu): catch exception(%d)!!\n",(unsigned long)handler,(unsigned long)eloop_data,(unsigned long)user_ctx,e.flavor);fflush(stdout);
		eloop_terminate(ELOOP_CTX_PTR );exit(-99);//can not terminate!!
	}
	CHECK_THREAD_EXCEPT_CONTEXT(e,"call_eloop_timeout_handler");	
}
#endif /*ELOOP_EXCEPTION_ADDON*/
#ifndef WIN32
# define TIMEVAL_TO_TIMESPEC(tv, ts) {                                   \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
}
# define TIMESPEC_TO_TIMEVAL(tv, ts) {                                   \
        (tv)->tv_sec = (ts)->tv_sec;                                    \
        (tv)->tv_usec = (ts)->tv_nsec / 1000;                           \
}
#endif

struct timeval get_eloop_timeval(ELOOP_CTX_ARG_ONLY)
{
	struct timeval _tv;
	struct os_time tv, now;
	
	if (eloop.timeout) {
		os_get_time(&now);
		if (os_time_before(&now, &eloop.timeout->time))
			os_time_sub(&eloop.timeout->time, &now, &tv);
		else
			tv.sec = tv.usec = 0;
		_tv.tv_sec = tv.sec;
		_tv.tv_usec = tv.usec;
	}
	else{
		_tv.tv_sec = 0x7FFFFFF;
		_tv.tv_usec = 0;
	}
	return _tv;
}
// struct timespec _ts;TIMEVAL_TO_TIMESPEC(&_tv,&_ts);return _ts// for pselect!!
unsigned int find_nearest_timeout_ms(ELOOP_CTX_ARG_ONLY)
{
	struct timeval _tv=get_eloop_timeval(ELOOP_CTX_PTR_ONLY );

	if(_tv.tv_sec>=0x7FFFFFF)return 0xFFFFFFFF;
	return _tv.tv_sec*1000+_tv.tv_usec/1000;
}
int expire_timers(ELOOP_CTX_ARG_ONLY)
{
	struct os_time now;

	eloop_process_pending_signals(ELOOP_CTX_PTR_ONLY);// only for mainThread!!
	
	/* check if some registered timeouts have occurred */
	if (eloop.timeout) {
		struct eloop_timeout *tmp;

		os_get_time(&now);
		if (!os_time_before(&now, &eloop.timeout->time)) {
			tmp = eloop.timeout;
			eloop.timeout = eloop.timeout->next;
#if ELOOP_EXCEPTION_ADDON
			call_eloop_timeout_handler(
				tmp->handler,
					tmp->eloop_data,
					tmp->user_data);
#else
			tmp->handler(tmp->eloop_data,
				     tmp->user_data);
#endif
			os_free(tmp);
			return 1;
		}

	}
	return 0;
}
void init_timers(ELOOP_CTX_ARG void* user_data)
{
	eloop_init(ELOOP_CTX_PTR user_data);
}
void destroy_timers(ELOOP_CTX_ARG_ONLY)
{
	eloop_destroy(ELOOP_CTX_PTR_ONLY );
}

#ifdef _WIN32
#define getcurTimeMs() GetTickCount()
#else
int os_get_time(struct os_time *t);
/*
   struct os_time{
   unsigned int sec,usec;
   };
   int os_get_time(struct os_time *t)
   {
   int res;
   struct timeval tv;
   res = gettimeofday(&tv, NULL);
   t->sec = tv.tv_sec;
   t->usec = tv.tv_usec;
   return res;
   }
   */
unsigned int getcurTimeMs()
{
	struct os_time t;
	os_get_time(&t);
	return t.sec*1000+t.usec/1000;
}
#endif
#if 0
#ifdef _WIN32
#define getcurTimeMs() GetTickCount()
#else
extern "C" int os_get_time(struct os_time *t);
/*
   struct os_time{
   unsigned int sec,usec;
   };
   int os_get_time(struct os_time *t)
   {
   int res;
   struct timeval tv;
   res = gettimeofday(&tv, NULL);
   t->sec = tv.tv_sec;
   t->usec = tv.tv_usec;
   return res;
   }
   */
unsigned int getcurTimeMs()
{
	struct os_time t;
	os_get_time(&t);
	return t.sec*1000+t.usec/1000;
}
#endif
#endif


//////////////////////////////////////////dulktape/c_eventloop.c
#if 0
#define  MAX_TIMERS             4096     /* this is quite excessive for embedded use, but good for testing */
#define  MIN_DELAY              1.0
#define  MIN_WAIT               1.0
#define  MAX_WAIT               60000.0
#define  MAX_EXPIRYS            10

#define  MAX_FDS                256

typedef struct {
	int64_t id;       /* numeric ID (returned from e.g. setTimeout); zero if unused */
	double target;    /* next target time */
	double delay;     /* delay/interval */
	int oneshot;      /* oneshot=1 (setTimeout), repeated=0 (setInterval) */
	int removed;      /* timer has been requested for removal */

	/* The callback associated with the timer is held in the "global stash",
	 * in <stash>.eventTimers[String(id)].  The references must be deleted
	 * when a timer struct is deleted.
	 */
} ev_timer;

/* Active timers.  Dense list, terminates to end of list or first unused timer.
 * The list is sorted by 'target', with lowest 'target' (earliest expiry) last
 * in the list.  When a timer's callback is being called, the timer is moved
 * to 'timer_expiring' as it needs special handling should the user callback
 * delete that particular timer.
 */
static ev_timer timer_list[MAX_TIMERS];
static ev_timer timer_expiring;
static int timer_count;  /* last timer at timer_count - 1 */
static int64_t timer_next_id = 1;
#endif


// ¼ÒÄÏ ÇÔ¼ö ¿À·ù Ãâ·Â ÈÄ Á¾·á
void err_quit(const char *msg)
{
#ifdef _WIN32
	LPVOID lpMsgBuf;
	FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER|
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
#else
	printf("err_quit:%s(errno=?)\n",msg);
#endif
	exit(-1);
}
void err_quit0(const char *msg)
{
#ifdef _WIN32
	LPVOID lpMsgBuf;
	FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER|
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
#else
	printf("err_quit:%s(errno=?)\n",msg);
#endif
	//	exit(-1);
}

// ¼ÒÄÏ ÇÔ¼ö ¿À·ù Ãâ·Â
void err_display(const char *msg)
{
#ifdef _WIN32
	LPVOID lpMsgBuf;
	FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER|
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
#else
	printf("err_quit:%s(errno=?)\n",msg);
#endif
}


//////////////////////////////////////
///////////////////////////////////////////////////
#ifdef WIN32
struct tm *localtime_r(const time_t* timep, struct tm *result) // emulate re-entrant CALL
{
	struct tm* staticResult=localtime(timep);
	if(result){
		if(staticResult)*result=*staticResult;
		else{memset(result,0,sizeof(*result));result=NULL;}
	}
	return result;
}
#endif
char* systemGetTimeStr(char* returnBuf)//systemGetPipeStr("date +'%m%d%H%M%Y.%S'", returnBuf);
{
#ifdef _WIN32
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	sprintf(returnBuf,"%02d%02d%02d%02d%04d.%02d", 
			sysTime.wMonth,
			sysTime.wDay,
			sysTime.wHour,
			sysTime.wMinute,
			sysTime.wYear,
			sysTime.wSecond);
#else // linux
	struct tm local_tm;
	time_t t;

	time(&t);
	localtime_r(&t,&local_tm);//gmtime_r(&t,&local_tm);
	sprintf(returnBuf,"%02d%02d%02d%02d%04d.%02d\n",local_tm.tm_mon+1,local_tm.tm_mday,local_tm.tm_hour,local_tm.tm_min,local_tm.tm_year+1900,local_tm.tm_sec);
#endif	
	return returnBuf;
}

////////////////////////////////////
#ifndef CONFIG_NATIVE_WINDOWS
static void eloop_handle_alarm(int sig)
{
	fprintf(stderr, "eloop: could not process SIGINT or SIGTERM in two "
		"seconds. Looks like there\n"
		"is a bug that ends up in a busy loop that "
		"prevents clean shutdown.\n"
		"Killing program forcefully.\n");
	//eloop_terminate();
	exit(1);
}
#endif /* CONFIG_NATIVE_WINDOWS */


static void eloop_handle_signal(int sig)
{
	int i;
	ELOOP_CTX_ARG_ONLY; 

#if ELOOP_MULTIDATA_PATCH
	p_eloop_data=p_eloopTbl[0];// default signal handler Loop
#endif
#ifndef CONFIG_NATIVE_WINDOWS
	if ((sig == SIGINT || sig == SIGTERM) && !eloop.pending_terminate) {
		/* Use SIGALRM to break out from potential busy loops that
		 * would not allow the program to be killed. */
		eloop.pending_terminate = 1;
		signal(SIGALRM, eloop_handle_alarm);
		alarm(2);
	}
#endif /* CONFIG_NATIVE_WINDOWS */

	eloop.signaled++;
	for (i = 0; i < eloop.signal_count; i++) {
		if (eloop.signals[i].sig == sig) {
			eloop.signals[i].signaled++;
			break;
		}
	}
}


static void eloop_process_pending_signals(ELOOP_CTX_ARG_ONLY)
{
	int i;

	if(errno){perror("eloop_process_pending_signals");/*fprintf(stderr,"errno detected(errno%d)!!\n",errno);*/fflush(stderr);errno=0;/*auto clear error_no*/}
	if (eloop.signaled == 0)
		return;
	eloop.signaled = 0;

	if (eloop.pending_terminate) {
#ifndef CONFIG_NATIVE_WINDOWS
		alarm(0);
#endif /* CONFIG_NATIVE_WINDOWS */
		eloop.pending_terminate = 0;
	}

	for (i = 0; i < eloop.signal_count; i++) {
		if (eloop.signals[i].signaled) {
			eloop.signals[i].signaled = 0;

#if 1
		  // Reinstantiate signal handler
			signal(eloop.signals[i].sig, eloop_handle_signal);
#endif
#if ELOOP_EXCEPTION_ADDON
			call_eloop_signal_handler(
				eloop.signals[i].handler,
						eloop.signals[i].sig,
						eloop.user_data,
						eloop.signals[i].user_data);
#else
			eloop.signals[i].handler(eloop.signals[i].sig,
						 eloop.user_data,
						 eloop.signals[i].user_data);
#endif
		}
	}
}

int eloop_register_signal(ELOOP_CTX_ARG int sig, eloop_signal_handler handler,
			  void *user_data)
{
	struct eloop_signal *tmp;

	tmp = (struct eloop_signal *)
		os_realloc(eloop.signals,
			   (eloop.signal_count + 1) *
			   sizeof(struct eloop_signal));
	if (tmp == NULL)
		return -1;

	tmp[eloop.signal_count].sig = sig;
	tmp[eloop.signal_count].user_data = user_data;
	tmp[eloop.signal_count].handler = handler;
	tmp[eloop.signal_count].signaled = 0;
	eloop.signal_count++;
	eloop.signals = tmp;
	signal(sig, eloop_handle_signal);

	return 0;
}

int eloop_register_signal_terminate(ELOOP_CTX_ARG eloop_signal_handler handler,
				    void *user_data)
{
	int ret = eloop_register_signal(ELOOP_CTX_PTR SIGINT, handler, user_data);
	if (ret == 0)
		ret = eloop_register_signal(ELOOP_CTX_PTR SIGTERM, handler, user_data);
	return ret;
}


int eloop_register_signal_reconfig(ELOOP_CTX_ARG eloop_signal_handler handler,
				   void *user_data)
{
#ifdef CONFIG_NATIVE_WINDOWS
	return 0;
#else /* CONFIG_NATIVE_WINDOWS */
	return eloop_register_signal(ELOOP_CTX_PTR SIGHUP, handler, user_data);
#endif /* CONFIG_NATIVE_WINDOWS */
}

