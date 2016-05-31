/*
 * Event loop based on select() loop
 * Copyright (c) 2002-2005, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include "includes.h"

#include "common.h"
#include "eloop.h"


#if ELOOP_EXCEPTION_ADDON
#ifdef _WIN32
typedef DWORD pthread_t;
//static CRITICAL_SECTION global_log_file_lock;
static pthread_t pthread_self(void) {
  return GetCurrentThreadId();
}
static int pthread_equal(pthread_t tid1,pthread_t tid2){return tid1==tid2;}
#warning "ELOOP_EXCEPTION_ADDON&_WIN32 enabled!!! "
#endif // _WIN32

#if 1	//DEBUG TID
#define __get_tid()       pthread_self()
#define __equal_tid(t1,t2)     pthread_equal(t1,t2)
pthread_t eloop_tid;
#else
#define __get_tid()       	(pthread_t)0xaaaa /*pthread_self()*/
#define __equal_tid(t1,t2)     1
pthread_t eloop_tid;
#warning "ELOOP_EXCEPTION_ADDON enabled!!! "
#endif
void call_eloop_sock_handler(eloop_sock_handler handler,int sock, void *eloop_ctx, void *sock_ctx);
void call_eloop_signal_handler(eloop_signal_handler handler,int sig, void *eloop_ctx, void *signal_ctx);
void call_eloop_timeout_handler(eloop_timeout_handler handler,void *eloop_data, void *user_ctx);
void call_eloop_event_handler(eloop_event_handler handler,void *eloop_data, void *user_ctx);
#else
#warning "ELOOP_EXCEPTION_ADDON disabled!!! "
#define __get_tid()       	(int)0xaaaa /*pthread_self()*/
int eloop_tid=(int)0xaaaa;
#define __equal_tid(t1,t2)     1
#endif /*ELOOP_EXCEPTION_ADDON*/
struct eloop_sock {
	int sock;
	void *eloop_data;
	void *user_data;
	eloop_sock_handler handler;
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

struct eloop_signal {
	int sig;
	void *user_data;
	eloop_signal_handler handler;
	int signaled;
};

struct eloop_sock_table {
	int count;
	struct eloop_sock *table;
	int changed;
};

struct eloop_data {
	void *user_data;

	int max_sock;

	struct eloop_sock_table readers;
	struct eloop_sock_table writers;
	struct eloop_sock_table exceptions;

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
	int reader_table_changed;

	int tmp_terminate;
};

static struct eloop_data eloop;


int eloop_init(void *user_data)
{
#ifdef WIN32
#else
	eloop_tid=(pthread_t)(long int)__get_tid();
#endif
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


static int eloop_sock_table_add_sock(struct eloop_sock_table *table,
                                     int sock, eloop_sock_handler handler,
                                     void *eloop_data, void *user_data)
{
	struct eloop_sock *tmp;

	if(!__equal_tid(eloop_tid,__get_tid()))printf("eloop_sock_table_add_sock diffTid!!!\n");

	if (table == NULL)
		return -1;

	tmp = (struct eloop_sock *)
		os_realloc(table->table,
			   (table->count + 1) * sizeof(struct eloop_sock));
	if (tmp == NULL)
		return -1;

	tmp[table->count].sock = sock;
	tmp[table->count].eloop_data = eloop_data;
	tmp[table->count].user_data = user_data;
	tmp[table->count].handler = handler;
	table->count++;
	table->table = tmp;
	if (sock > eloop.max_sock){
		eloop.max_sock = sock;
		// printf("eloop_sock_table_add_sock: eloop.max_sock changed to %d\n",eloop.max_sock);
	}
	table->changed = 1;

	return 0;
}


static void eloop_recalc_max_sock()
{
#if 1//2014.7.8 ywhwang patch for max_sock
	int i;
	struct eloop_sock_table *table=NULL;
	int tmp_max_sock=-1;

	table=&eloop.readers;
	for (i = 0; i < table->count; i++) {
		if (table->table[i].sock > tmp_max_sock)tmp_max_sock=table->table[i].sock;
	}
	table=&eloop.writers;
	for (i = 0; i < table->count; i++) {
		if (table->table[i].sock > tmp_max_sock)tmp_max_sock=table->table[i].sock;
	}
	table=&eloop.exceptions;
	for (i = 0; i < table->count; i++) {
		if (table->table[i].sock > tmp_max_sock)tmp_max_sock=table->table[i].sock;
	}
	// if(eloop.max_sock != tmp_max_sock)printf("eloop_unregister_sock: eloop.max_sock changed to %d\n",tmp_max_sock);
	eloop.max_sock = tmp_max_sock;
#endif
}
static void eloop_sock_table_remove_sock(struct eloop_sock_table *table,
                                         int sock)
{
	int i;
#if 0//2013.9.2 ywhwang patch for max_sock
	int tmp_max_sock=-1;
#endif

	if(!__equal_tid(eloop_tid,__get_tid()))printf("eloop_sock_table_remove_sock diffTid!!!\n");

	if (table == NULL || table->table == NULL || table->count == 0)
		return;

	for (i = 0; i < table->count; i++) {
		if (table->table[i].sock == sock)
			break;
	}
	if (i == table->count)
		return;
	if (i != table->count - 1) {
		os_memmove(&table->table[i], &table->table[i + 1],
			   (table->count - i - 1) *
			   sizeof(struct eloop_sock));
	}
	table->count--;
#if 0//2013.9.2 ywhwang patch for max_sock
	for (i = 0; i < table->count; i++) {
		if (table->table[i].sock > tmp_max_sock)tmp_max_sock=table->table[i].sock;
	}
#if 1//2014.7.8 ywhwang patch for max_sock
	if(sock==eloop.max_sock && tmp_max_sock<eloop.max_sock)eloop.max_sock = tmp_max_sock;
#else
	if(tmp_max_sock<eloop.max_sock)eloop.max_sock = tmp_max_sock;
#endif
#endif
	table->changed = 1;
}


static void eloop_sock_table_set_fds(struct eloop_sock_table *table,
				     fd_set *fds)
{
	int i;

	FD_ZERO(fds);

	if (table->table == NULL)
		return;

	for (i = 0; i < table->count; i++)
		FD_SET(table->table[i].sock, fds);
}


static void eloop_sock_table_dispatch(struct eloop_sock_table *table,
				      fd_set *fds)
{
	int i;

	if (table == NULL || table->table == NULL)
		return;

	table->changed = 0;
	for (i = 0; i < table->count; i++) {
		if (FD_ISSET(table->table[i].sock, fds)) {
#if ELOOP_EXCEPTION_ADDON
			call_eloop_sock_handler(
				table->table[i].handler,
					table->table[i].sock,
					table->table[i].eloop_data,
					table->table[i].user_data);
#else
			table->table[i].handler(table->table[i].sock,
						table->table[i].eloop_data,
						table->table[i].user_data);
#endif
			if (table->changed)
				break;
		}
	}
}


static void eloop_sock_table_destroy(struct eloop_sock_table *table)
{
	if (table)
		os_free(table->table);
}


int eloop_register_read_sock(int sock, eloop_sock_handler handler,
			     void *eloop_data, void *user_data)
{
	return eloop_register_sock(sock, EVENT_TYPE_READ, handler,
				   eloop_data, user_data);
}


void eloop_unregister_read_sock(int sock)
{
	eloop_unregister_sock(sock, EVENT_TYPE_READ);
}


static struct eloop_sock_table *eloop_get_sock_table(eloop_event_type type)
{
	switch (type) {
	case EVENT_TYPE_READ:
		return &eloop.readers;
	case EVENT_TYPE_WRITE:
		return &eloop.writers;
	case EVENT_TYPE_EXCEPTION:
		return &eloop.exceptions;
	}

	return NULL;
}


int eloop_register_sock(int sock, eloop_event_type type,
			eloop_sock_handler handler,
			void *eloop_data, void *user_data)
{
	struct eloop_sock_table *table;

#ifdef WIN32
// printf("eloop_register_sock(%d,type=%d)\n",sock,type);
#endif
	table = eloop_get_sock_table(type);
	return eloop_sock_table_add_sock(table, sock, handler,
					 eloop_data, user_data);
}


void eloop_unregister_sock(int sock, eloop_event_type type)
{
	struct eloop_sock_table *table;

#ifdef WIN32
//printf("eloop_unregister_sock(%d,type=%d)\n",sock,type);
#endif
	table = eloop_get_sock_table(type);
	eloop_sock_table_remove_sock(table, sock);
#if 1//2014.7.8 ywhwang patch for max_sock
	eloop_recalc_max_sock();
#endif
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
int eloop_register_timeout0(unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data, int bIsWDTimeout)
#else /*ELOOP_WATCHDOG_ADDON*/
int eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data)
#endif /*!ELOOP_WATCHDOG_ADDON*/
{
	struct eloop_timeout *timeout, *tmp, *prev;

	if(!__equal_tid(eloop_tid,__get_tid()))printf("eloop_register_timeout0 diffTid!!!\n");

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
        if(bIsWDTimeout)OnRegisterWDT(timeout,secs,usecs);
#endif
#endif /*ELOOP_WATCHDOG_ADDON*/
	return 0;
}


#if ELOOP_WATCHDOG_ADDON
int eloop_cancel_timeout0(eloop_timeout_handler handler,
			 void *eloop_data, void *user_data, int bIsWDTimeout)
#else /*ELOOP_WATCHDOG_ADDON*/
int eloop_cancel_timeout(eloop_timeout_handler handler,
			 void *eloop_data, void *user_data)
#endif /*!ELOOP_WATCHDOG_ADDON*/
{
	struct eloop_timeout *timeout, *prev, *next;
	int removed = 0;

	if(!__equal_tid(eloop_tid,__get_tid()))printf("eloop_cancel_timeout0 diffTid!!!\n");

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
			if(timeout->bIsWDTimeout||bIsWDTimeout)OnUnregisterWDT(timeout);
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
#if ELOOP_TIMECHANGE_PATCH//NTP notify√ﬂ∞°
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
void eloop_timeUpdate(int sign,struct os_time *pDifftime)
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
void eloop_timeChange0(struct os_time *pNewTime,struct os_time *pCurtime)
{
        struct os_time difftime;
        int sign;

        /*calc diff time*/
        sign=CmpTime(pNewTime,pCurtime);
        if(sign==0)return;
//fprintf(stderr,"TU1:%c,%d.%06d,%d.%06d\r\n",(sign>0? '+':(sign==0? ' ':'-')),pNewTime->sec,pNewTime->usec,pCurtime->sec,pCurtime->usec);
        if(sign>0)difftime=SubTime(pNewTime,pCurtime);
        else difftime=SubTime(pCurtime,pNewTime);
        eloop_timeUpdate(sign,&difftime);
}
void eloop_timeChange(unsigned long new_sec,unsigned long new_usec)
{
#if !ELOOP_TIMECHANGE_AUTO
	struct os_time newtime,curtime;

        /*get new time*/
        newtime.sec=new_sec;newtime.usec=new_usec;
        /*get cur time*/
        os_get_time(&curtime);
        /*update*/
        eloop_timeChange0(&newtime,&curtime);
#else
        fprintf(stderr,"TUntp:%ld.%06ld\n",new_sec,new_usec);
#endif
}
#define MY_CLOCKS_PER_SEC 100 /*sysconf(_SC_CLK_TCK)*/
void eloop_timeChangeAuto(clock_t t1,clock_t t0,struct os_time *pNewTime,struct os_time *pPrevTime)
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
        eloop_timeChange0(pNewTime,&curtime);
#else
        //fprintf(stderr,"TUauto:%d.%06d\n",new_sec,new_usec);
#endif
}
#endif

#if ELOOP_CALLBACKLONGDEBUG_ADDON&&defined(CONFIG_NATIVE_WINDOWS)//CATCH PENDING callback
void SetPendingMonitor(int PendingMonitorType){
}
void ClrPendingMonitor(int PendingMonitorType){
}
#endif
#if ELOOP_CALLBACKLONGDEBUG_ADDON&&!defined(CONFIG_NATIVE_WINDOWS)//CATCH PENDING callback
#define MAX_CALLBACK_PROC_TIME 4
volatile int g_PendingMonitorType=-1;
static void eloop_handle_callback_alarm(int sig)
{
	fprintf(stderr, "eloop: could not process CallBack(%d) in %d "
		"seconds. Looks like there\n"
		"is a bug that ends up in a busy loop that "
		"prevents short callback processing.\n"
		"Killing program forcefully.\n",g_PendingMonitorType,MAX_CALLBACK_PROC_TIME);fflush(stderr);
	//eloop_terminate();
	exit(2);
}
void SetPendingMonitor(int PendingMonitorType){
	if(g_PendingMonitorType!=-1||eloop.pending_terminate){
		fprintf(stderr, "cannot set g_PendingMonitorType\n");fflush(stderr);
		return;
	}
	g_PendingMonitorType=PendingMonitorType;
	//signal(SIGALRM, eloop_handle_callback_alarm);
	//alarm(MAX_CALLBACK_PROC_TIME);
}
void ClrPendingMonitor(int PendingMonitorType){
	if(g_PendingMonitorType==-1||eloop.pending_terminate){
		fprintf(stderr, "cannot clr g_PendingMonitorType\n");fflush(stderr);
		return ;
	}
	//alarm(0);
	g_PendingMonitorType=-1;
}
#endif

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


static void eloop_process_pending_signals(void)
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


int eloop_register_signal(int sig, eloop_signal_handler handler,
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


int eloop_register_signal_terminate(eloop_signal_handler handler,
				    void *user_data)
{
	int ret = eloop_register_signal(SIGINT, handler, user_data);
	if (ret == 0)
		ret = eloop_register_signal(SIGTERM, handler, user_data);
	return ret;
}


int eloop_register_signal_reconfig(eloop_signal_handler handler,
				   void *user_data)
{
#ifdef CONFIG_NATIVE_WINDOWS
	return 0;
#else /* CONFIG_NATIVE_WINDOWS */
	return eloop_register_signal(SIGHUP, handler, user_data);
#endif /* CONFIG_NATIVE_WINDOWS */
}

void eloop_tmp_terminate(int tmp_term_val)
{
	eloop.tmp_terminate=tmp_term_val;
}
void eloop_run(void)
{
	fd_set *rfds, *wfds, *efds;
	int res;
	struct timeval _tv;
	struct os_time tv, now;
#if ELOOP_TIMECHANGE_AUTO
        clock_t t0,t1;struct tms tms0,tms1;
        struct os_time time0,time1;
#endif
#if ELOOP_EXCEPTION_ADDON
	NEW_THREAD_EXCEPT_CONTEXT(e);//must be end of variable!!
#endif

#if ELOOP_TIMECHANGE_AUTO
        t0=times(&tms0);os_get_time(&time0);
#endif

	if(!__equal_tid(eloop_tid,__get_tid()))printf("eloop_run diffTid!!!\n");

	rfds = os_malloc(sizeof(*rfds));
	wfds = os_malloc(sizeof(*wfds));
	efds = os_malloc(sizeof(*efds));
	if (rfds == NULL || wfds == NULL || efds == NULL) {
		printf("eloop_run - malloc failed\n");
		goto out;
	}

	while (!eloop.tmp_terminate && 
		!eloop.terminate &&
	       (eloop.timeout || eloop.readers.count > 0 ||
		eloop.writers.count > 0 || eloop.exceptions.count > 0)) {
#if ELOOP_TIMECHANGE_AUTO
                t1=times(&tms1);os_get_time(&time1);
                eloop_timeChangeAuto(t1,t0,&time1,&time0);
                t0=t1;time0=time1;
#endif
		if (eloop.timeout) {
			os_get_time(&now);
			if (os_time_before(&now, &eloop.timeout->time))
				os_time_sub(&eloop.timeout->time, &now, &tv);
			else
				tv.sec = tv.usec = 0;
#if 0
			printf("next timeout in %lu.%06lu sec\n",
			       tv.sec, tv.usec);
#endif
			_tv.tv_sec = tv.sec;
			_tv.tv_usec = tv.usec;
		}
		memset(rfds,0,sizeof(*rfds));memset(wfds,0,sizeof(*wfds));memset(efds,0,sizeof(*efds));

		eloop_sock_table_set_fds(&eloop.readers, rfds);
		eloop_sock_table_set_fds(&eloop.writers, wfds);
		eloop_sock_table_set_fds(&eloop.exceptions, efds);
#ifndef WIN32
#ifndef TIMEVAL_TO_TIMESPEC
# define TIMEVAL_TO_TIMESPEC(tv, ts) {                                   \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
}
#endif
#ifndef TIMESPEC_TO_TIMEVAL
# define TIMESPEC_TO_TIMEVAL(tv, ts) {                                   \
        (tv)->tv_sec = (ts)->tv_sec;                                    \
        (tv)->tv_usec = (ts)->tv_nsec / 1000;                           \
}
#endif
		{
			struct timespec _ts;TIMEVAL_TO_TIMESPEC(&_tv,&_ts);//sigset_t sigmask=0;
			printf("%s(): before select!\n", __func__); fflush(stdout);
			res = pselect(eloop.max_sock + 1, rfds, wfds, efds, eloop.timeout ? &_ts : NULL,NULL);
		}
#else
		//printf("maxSock=%d,timeout=0x%x,tv(%d.%06d)\n",eloop.max_sock,eloop.timeout,_tv.tv_sec,_tv.tv_usec);
		SetLastError(ERROR_SUCCESS);
		res = select(eloop.max_sock + 1, rfds, wfds, efds,
			     eloop.timeout ? &_tv : NULL);
#endif
#if ELOOP_TIMECHANGE_AUTO
                t1=times(&tms1);os_get_time(&time1);
                eloop_timeChangeAuto(t1,t0,&time1,&time0);
                t0=t1;time0=time1;
#endif
#ifdef WIN32
		//printf("res(%d/%d/errno=%d)\n",res,GetLastError(),errno);//WSAENOTSOCK=10038
		if(GetLastError())errno=GetLastError();
#endif
		if (res < 0 && errno != EINTR && errno != 0) {
			perror("select");
			goto out;
		}
		eloop_process_pending_signals();
#ifdef WIN32
		printf("eloop_run(): after select!\n"); fflush(stdout);
#else
		printf("%s(): after select!\n", __func__); fflush(stdout);
#endif

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
#ifdef WIN32
				printf("eloop_run(): handler call OK!\n"); fflush(stdout);
#else
				printf("%s(): handler call OK!\n", __func__); fflush(stdout);
#endif
#endif
				os_free(tmp);
			}

		}

		if (res <= 0)
			continue;

		eloop_sock_table_dispatch(&eloop.readers, rfds);
		eloop_sock_table_dispatch(&eloop.writers, wfds);
		eloop_sock_table_dispatch(&eloop.exceptions, efds);
	}

out:
	os_free(rfds);
	os_free(wfds);
	os_free(efds);
#if ELOOP_EXCEPTION_ADDON
	DEL_THREAD_EXCEPT_CONTEXT();
#endif
}


void eloop_terminate(void)
{
	eloop.terminate = 1;
}


void eloop_destroy(void)
{
	struct eloop_timeout *timeout, *prev;

	timeout = eloop.timeout;
	while (timeout != NULL) {
		prev = timeout;
		timeout = timeout->next;
		os_free(prev);
	}
	eloop_sock_table_destroy(&eloop.readers);
	eloop_sock_table_destroy(&eloop.writers);
	eloop_sock_table_destroy(&eloop.exceptions);
	os_free(eloop.signals);
}


int eloop_terminated(void)
{
	return eloop.terminate;
}


void eloop_wait_for_read_sock(int sock)
{
	fd_set rfds;

	if (sock < 0)
		return;

	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);
	select(sock + 1, &rfds, NULL, NULL, NULL);
}


void * eloop_get_user_data(void)
{
	return eloop.user_data;
}

#if ELOOP_WATCHDOG_ADDON
#if HYWMON_SUPPORT_WDTMON
int eloop_register_watchdog_timeout(unsigned int secs, unsigned int usecs,
			   /*eloop_timeout_handler handler,*/
			   void *eloop_data, void *user_data)
{
        return eloop_register_timeout0(secs,usecs,eloop_timeout_handler_for_WDT,eloop_data,user_data,1);
}
int eloop_cancel_watchdog_timeout(/*eloop_timeout_handler handler,*/
			 void *eloop_data, void *user_data)
{
        return eloop_cancel_timeout0(eloop_timeout_handler_for_WDT,eloop_data,user_data,1);
}
#endif
int eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data)
{
        return eloop_register_timeout0(secs,usecs,handler,eloop_data,user_data,0);
}
int eloop_cancel_timeout(eloop_timeout_handler handler,
			 void *eloop_data, void *user_data)
{
        return eloop_cancel_timeout0(handler,eloop_data,user_data,0);
}
#if HYWMON_SUPPORT_WDTMON
int dbgWdt_eloop=0;
void OnRegisterWDT(struct eloop_timeout *timeout,unsigned int secs, unsigned int usecs)//after inserted!!
{
if(dbgWdt_eloop)printf("OnRegisterWDT:%lx(Id=%ld)\n",(unsigned long)timeout,(unsigned long)timeout->user_data);
	timeout->bIsWDTimeout=1;
	if(eloop.timeout_currentWDT==NULL){
if(dbgWdt_eloop)printf("OnRegisterWDTfirst:%lx(Id=%ld)\n",(unsigned long)timeout,(unsigned long)timeout->user_data);
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
if(dbgWdt_eloop)printf("OnRegisterWDTupdate2ME:%lx(Id=%ld)\n",(unsigned long)timeoutWDT,(unsigned long)timeoutWDT->user_data);
        				timeleft=GetTimeLeft(&timeoutWDT->time);
        				//eloop.nCurrentWDTime=timeleft.sec;
        				eloop_restartWDT(eloop.user_data,timeleft.sec,timeleft.usec);//stop&start
				        eloop.timeout_currentWDT=timeoutWDT;
			        }
			        else{
if(dbgWdt_eloop)printf("OnRegisterWDTupdate2other:%lx(Id=%ld)\n",(unsigned long)timeoutWDT,(unsigned long)timeoutWDT->user_data);
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
void OnUnregisterWDT(struct eloop_timeout *timeout)//before ejected!!
{
if(dbgWdt_eloop)printf("OnUnregisterWDT:%lx(Id=%ld)\n",(unsigned long)timeout,(unsigned long)timeout->user_data);
	//if(timeout->bIsWDTimeout==bIsWDTimeout);
	if(eloop.timeout_currentWDT==NULL){
printf("OnUnregisterWDTnone??:%lx(Id=%ld)\n",(unsigned long)timeout,(unsigned long)timeout->user_data);
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
if(dbgWdt_eloop)printf("OnUnregisterWDTupdate2ME:%lx(Id=%ld)\n",(unsigned long)timeoutWDT,(unsigned long)timeoutWDT->user_data);
					break;
				}
			        timeoutWDT=timeoutWDT->next;
			}
			timeout->bIsWDTimeout=0;//dbg
			if(!eloop.timeout_currentWDT)
			        eloop_stopWDT(eloop.user_data);
			else
				eloop_restartWDT(eloop.user_data,timeleft.sec,timeleft.usec);//stop&start
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
void call_eloop_sock_handler(eloop_sock_handler handler,int sock, void *eloop_ctx, void *sock_ctx)
{FIND_THREAD_EXCEPT_CONTEXT(e,"call_eloop_sock_handler");
        Try{
                //printf("call_eloop_sock_handler(): monitoring exception!!\n");fflush(stdout);
                SetPendingMonitor(1);
                handler(sock,eloop_ctx,sock_ctx);
		  ClrPendingMonitor(1);
        }
        Catch(e){
                printf("call_eloop_sock_handler: catch exception(%d)!!\n",e.flavor);fflush(stdout);
		eloop_terminate();exit(-99);//can not terminate!!
        }
	CHECK_THREAD_EXCEPT_CONTEXT(e,"call_eloop_sock_handler");	
}
void call_eloop_signal_handler(eloop_signal_handler handler,int sig, void *eloop_ctx, void *signal_ctx)
{FIND_THREAD_EXCEPT_CONTEXT(e,"call_eloop_signal_handler");
        Try{
                //printf("call_eloop_signal_handler(): monitoring exception!!\n");fflush(stdout);
                if(sig!=SIGINT&&sig!=SIGTERM)SetPendingMonitor(4);
                handler(sig,eloop_ctx,signal_ctx);
		  if(sig!=SIGINT&&sig!=SIGTERM)ClrPendingMonitor(4);
        }
        Catch(e){
                printf("call_eloop_signal_handler: catch exception(%d)!!\n",e.flavor);fflush(stdout);
		eloop_terminate();exit(-99);//can not terminate!!
        }
	CHECK_THREAD_EXCEPT_CONTEXT(e,"call_eloop_signal_handler");	
}
void call_eloop_timeout_handler(eloop_timeout_handler handler,void *eloop_data, void *user_ctx)
{FIND_THREAD_EXCEPT_CONTEXT(e,"call_eloop_timeout_handler");
        Try{
                //printf("call_eloop_timeout_handler(): monitoring exception!!\n");fflush(stdout);
                SetPendingMonitor(2);
                handler(eloop_data,user_ctx);
		  ClrPendingMonitor(2);
        }
        Catch(e){
                printf("call_eloop_timeout_handler%lx(%lu,%lu): catch exception(%d)!!\n",(unsigned long)handler,(unsigned long)eloop_data,(unsigned long)user_ctx,e.flavor);fflush(stdout);
		eloop_terminate();exit(-99);//can not terminate!!
        }
	CHECK_THREAD_EXCEPT_CONTEXT(e,"call_eloop_timeout_handler");	
}
void call_eloop_event_handler(eloop_event_handler handler,void *eloop_data, void *user_ctx)
{FIND_THREAD_EXCEPT_CONTEXT(e,"call_eloop_event_handler");
        Try{
                //printf("call_eloop_event_handler(): monitoring exception!!\n");fflush(stdout);
                SetPendingMonitor(3);
                handler(eloop_data,user_ctx);
 		  ClrPendingMonitor(3);
        }
        Catch(e){
                printf("call_eloop_event_handler: catch exception(%d)!!\n",e.flavor);fflush(stdout);
		eloop_terminate();exit(-99);//can not terminate!!
        }
	CHECK_THREAD_EXCEPT_CONTEXT(e,"call_eloop_event_handler");	
}
#endif /*ELOOP_EXCEPTION_ADDON*/
#if 0//YWHWANGPATCH_MULTIBSS_ACN
static void eloop_timeout_handler_mySock(void *eloop_data, void *user_ctx)
{
        volatile int *p_myTerminate=(volatile int *)eloop_data;
        
        *p_myTerminate=ELOOP_MY_SOCK_RET_TIMEOUT;
}
static void eloop_sock_handler_mySock(int sock, void *eloop_ctx, void *sock_ctx)
{
        volatile int *p_myTerminate=(volatile int *)eloop_ctx;

        *p_myTerminate=ELOOP_MY_SOCK_RET_RECEIVED;
}
int eloop_run_with_mySocket(int sock,unsigned int secs, unsigned int usecs)
{
	fd_set *rfds, *wfds, *efds;
	int res;
	struct timeval _tv;
	struct os_time tv, now;
#if ELOOP_TIMECHANGE_AUTO
        clock_t t0,t1;struct tms tms0,tms1;
        struct os_time time0,time1;
        
        t0=times(&tms0);os_get_time(&time0);
#endif
        volatile int myTerminate=ELOOP_MY_SOCK_RET_TERMINATED;

        eloop_register_read_sock(sock,eloop_sock_handler_mySock,(void*)&myTerminate, NULL);
        eloop_register_timeout(secs, usecs,eloop_timeout_handler_mySock,(void*)&myTerminate, NULL);
        
	rfds = os_malloc(sizeof(*rfds));
	wfds = os_malloc(sizeof(*wfds));
	efds = os_malloc(sizeof(*efds));
	if (rfds == NULL || wfds == NULL || efds == NULL) {
		printf("eloop_run - malloc failed\n");
		goto out;
	}

	while (myTerminate!=ELOOP_MY_SOCK_RET_TERMINATED && !eloop.terminate &&
	       (eloop.timeout || eloop.readers.count > 0 ||
		eloop.writers.count > 0 || eloop.exceptions.count > 0)) {
#if ELOOP_TIMECHANGE_AUTO
                t1=times(&tms1);os_get_time(&time1);
                eloop_timeChangeAuto(t1,t0,&time1,&time0);
                t0=t1;time0=time1;
#endif
		if (eloop.timeout) {
			os_get_time(&now);
			if (os_time_before(&now, &eloop.timeout->time))
				os_time_sub(&eloop.timeout->time, &now, &tv);
			else
				tv.sec = tv.usec = 0;
#if 0
			printf("next timeout in %lu.%06lu sec\n",
			       tv.sec, tv.usec);
#endif
			_tv.tv_sec = tv.sec;
			_tv.tv_usec = tv.usec;
		}

		eloop_sock_table_set_fds(&eloop.readers, rfds);
		eloop_sock_table_set_fds(&eloop.writers, wfds);
		eloop_sock_table_set_fds(&eloop.exceptions, efds);
		res = select(eloop.max_sock + 1, rfds, wfds, efds,
			     eloop.timeout ? &_tv : NULL);
#if ELOOP_TIMECHANGE_AUTO
                t1=times(&tms1);os_get_time(&time1);
                eloop_timeChangeAuto(t1,t0,&time1,&time0);
                t0=t1;time0=time1;
#endif
		if (res < 0 && errno != EINTR && errno != 0) {
			perror("select");
			goto out;
		}
		eloop_process_pending_signals();

		/* check if some registered timeouts have occurred */
		if (eloop.timeout) {
			struct eloop_timeout *tmp;

			os_get_time(&now);
			if (!os_time_before(&now, &eloop.timeout->time)) {
				tmp = eloop.timeout;
				eloop.timeout = eloop.timeout->next;
				tmp->handler(tmp->eloop_data,
					     tmp->user_data);
				os_free(tmp);
			}

		}

		if (res <= 0)
			continue;

		eloop_sock_table_dispatch(&eloop.readers, rfds);
		eloop_sock_table_dispatch(&eloop.writers, wfds);
		eloop_sock_table_dispatch(&eloop.exceptions, efds);
	}
	
	if(myTerminate==ELOOP_MY_SOCK_RET_TIMEOUT)//sock timeout
                eloop_unregister_read_sock(sock);
	else if(myTerminate==ELOOP_MY_SOCK_RET_RECEIVED){//sock received
                eloop_cancel_timeout(eloop_timeout_handler_mySock,(void*)&myTerminate, NULL);
                eloop_unregister_read_sock(sock);
        }
	else ;//eloop.terminate//ELOOP_MY_SOCK_RET_TERMINATED

out:
	os_free(rfds);
	os_free(wfds);
	os_free(efds);
	
	return myTerminate;
}
#endif
