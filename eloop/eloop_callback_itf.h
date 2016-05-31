#ifndef __ELOOP_CALLBACK_IF__
#define __ELOOP_CALLBACK_IF__
//#define USE_ELOOP 1
#if USE_ELOOP
#ifndef WIN32
#include <unistd.h>
#if USE_DOMAIN_SOCKET
#include<sys/un.h>
#endif
#ifndef CONFIG_NATIVE_WINDOWS
#include <syslog.h>
#endif /* CONFIG_NATIVE_WINDOWS */
typedef unsigned long long ulonglong;
typedef long long longlong;
//typedef unsigned long ulong;
#else
typedef unsigned __int64 ulonglong;
typedef __int64 longlong;
typedef unsigned long ulong;
#endif

#include "includes.h"

#undef HYW_SW_WDT
#define HYW_SW_WDT 1

#ifdef __cplusplus
extern "C" {
#endif
#include"eloop.h"
#include "common.h"
#include "cexcept.h"
#ifndef WIN32
typedef int SOCKET;
#else
//typedef UINT_PTR        SOCKET;
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#if HYWMON_SUPPORT_RELOAD
extern "C" int reloading;
extern "C" int g_exitCode;
#endif
extern "C" int g_background;
//extern int countPairs[10];
/* EloopSignalCallbackInterface */
class EloopSignalCallbackInterface ;
typedef void (EloopSignalCallbackInterface::*pOnSignal_t)(int sig,void* eloop_ctx);
/* EloopSockCallbackInterface */
class EloopSockCallbackInterface;
typedef bool (EloopSockCallbackInterface::*pOnSockReceive_t)(int sock,void *sock_ctx);
typedef bool (EloopSockCallbackInterface::*pOnSockWrite_t)(int sock,void *sock_ctx);
typedef bool (EloopSockCallbackInterface::*pOnSockSend_t)(int sock,void *sock_ctx);
/* EloopTimeoutCallbackInterface */
class EloopTimeoutCallbackInterface;
typedef void (EloopTimeoutCallbackInterface::*pOnTimeout_t)(void *timeout_ctx);
/* TaskCallbackInterface */
class TaskCallbackInterface;
typedef void* (TaskCallbackInterface::*pRun_t)(int arg1);
/* addtional context for function callback */
typedef struct _EloopIntFuncPair{
	union{
		EloopSignalCallbackInterface* pSigObj;
		EloopSockCallbackInterface* pSockObj;
		EloopTimeoutCallbackInterface* pTimeoutObj;
		TaskCallbackInterface* pTaskObj;
	};
	union{
		pOnSignal_t pOnSignal;
		pOnSockReceive_t pOnSockReceive;
		pOnSockWrite_t pOnSockWrite;
		pOnTimeout_t pOnTimeout;
		pRun_t pRun;
	};
	unsigned int uiHandled;
}EloopIntFuncPair;
/* EloopSignalCallbackInterface */
class EloopSignalCallbackInterface {
public:
	virtual void OnSignal(int sig, void *eloop_ctx){eloop_terminate();};//must implement
	virtual const int GetSignalNo() const {return SIGINT;};//must implement
#if USE_ELOOP
	static void OnSignalFuncDynamic(int sig, void *eloop_ctx, void *signal_ctx)
	{//called with registered state
		EloopIntFuncPair* ctx=(EloopIntFuncPair*)signal_ctx;
		if(!eloop_ctx||!ctx){printf("xxOnSignalFuncDynamic\n");return;}
		EloopSignalCallbackInterface* pThis=ctx->pSigObj;
		pOnSignal_t pOnSignal=ctx->pOnSignal;

		ctx->uiHandled++;
		(pThis->*pOnSignal)(sig,eloop_ctx);
	}
	static EloopIntFuncPair* SignalRegister(EloopSignalCallbackInterface* pThis,pOnSignal_t pOnSignal,int sig=-1,EloopIntFuncPair* ctx=NULL)
	{
		int ret;
		if(!ctx)ctx=new EloopIntFuncPair;//countPairs[0]++;

		if(!ctx)return NULL;
		ctx->pSigObj=pThis;ctx->pOnSignal=pOnSignal;
		ctx->uiHandled=0;
		ret=eloop_register_signal((sig==-1? pThis->GetSignalNo():sig),OnSignalFuncDynamic,ctx);
		if(ret){printf("SignalRegister: eloop_register_signal failed(OnSignalFuncDynamic)!!\n");delete ctx;/*countPairs[0]--;*/}
		return ret? NULL:ctx;
	}
	static void SignalUnregister(EloopSignalCallbackInterface* pThis,EloopIntFuncPair* ctx,int sig=-1,bool bCtxFree=true){if(ctx&&bCtxFree)delete ctx;/*if(ctx)countPairs[0]--;*/}
	virtual EloopIntFuncPair* SignalRegisterSelfCb(int sig=-1,pOnSignal_t pOnSignal=&EloopSignalCallbackInterface::OnSignal,EloopIntFuncPair* ctx=NULL){return SignalRegister(this,pOnSignal,sig,ctx);}
	virtual void SignalUnregisterSelfCb(int sig,EloopIntFuncPair* ctx,bool bCtxFree=true){SignalUnregister(this,ctx,sig,bCtxFree);}
	
	static void OnSignalFunc(int sig, void *eloop_ctx, void *signal_ctx)
	{//called with registered state
		EloopSignalCallbackInterface* pThis=(EloopSignalCallbackInterface*)signal_ctx;

		if(!eloop_ctx){printf("xxOnSignalFunc\n");return;}
		pThis->OnSignal(sig,eloop_ctx);
	}
	static int SignalRegister(EloopSignalCallbackInterface* pThis,int sig=-1)
	{
		int ret;
		ret=eloop_register_signal((sig==-1? pThis->GetSignalNo():sig),OnSignalFunc,pThis);
		if(ret)printf("SignalRegister: eloop_register_signal failed!!\n");
		return ret;
	}
	static void SignalUnregister(EloopSignalCallbackInterface* pThis,int sig=-1){}
	virtual int SignalRegisterSelfSig(int sig=-1){return SignalRegister(this,sig);}
	virtual void SignalUnregisterSelfSig(int sig=-1){SignalUnregister(this,sig);}
#endif

	virtual ~EloopSignalCallbackInterface(){}

protected:
	EloopSignalCallbackInterface(){}
};

/* EloopSockCallbackInterface */
class EloopSockCallbackInterface {
public:
	virtual const SOCKET GetSocketFd() const = 0;//must implement
	virtual const void* GetSocketCtx() const = 0;//may be same as GetSocketFd()
	virtual bool OnSockReceiveNew(int sock,void *sock_ctx){return false;}//may implement
	virtual void OnSockReceive(int sock,void *sock_ctx) = 0;//must implement
	virtual bool OnSockWriteNew(int sock,void *sock_ctx){return false;}//may implement
	virtual void OnSockWrite(int sock,void *sock_ctx){return;}//must implement
#if USE_ELOOP
	static void OnSocketReceiveFuncDynamic(int sock, void *eloop_ctx, void *sock_ctx)
	{//called with registered state
		EloopIntFuncPair* ctx=(EloopIntFuncPair*)eloop_ctx;
		if(!eloop_ctx||!ctx){printf("xxOnSocketReceiveFuncDynamic\n");return;}
		EloopSockCallbackInterface* pThis=ctx->pSockObj;
		pOnSockReceive_t pOnSockReceive=ctx->pOnSockReceive;

		ctx->uiHandled++;
		(pThis->*pOnSockReceive)(sock,sock_ctx);
	}
	static void OnSocketWriteFuncDynamic(int sock, void *eloop_ctx, void *sock_ctx)
	{//called with registered state
		EloopIntFuncPair* ctx=(EloopIntFuncPair*)eloop_ctx;
		if(!eloop_ctx||!ctx){printf("xxOnSocketWriteFuncDynamic\n");return;}
		EloopSockCallbackInterface* pThis=ctx->pSockObj;
		pOnSockWrite_t pOnSockWrite=ctx->pOnSockWrite;

		ctx->uiHandled++;
		(pThis->*pOnSockWrite)(sock,sock_ctx);
	}
	static EloopIntFuncPair* ReadRegister(EloopSockCallbackInterface* pThis,pOnSockReceive_t pOnSockReceive,int sock=-1,void *sock_ctx=NULL,EloopIntFuncPair* ctx=NULL)
	{
		int ret;
		if(!ctx)ctx=new EloopIntFuncPair;//countPairs[1]++;

		if(!ctx)return NULL;
		ctx->pSockObj=pThis;ctx->pOnSockReceive=pOnSockReceive;
		ctx->uiHandled=0;
		ret=eloop_register_read_sock((sock==-1? pThis->GetSocketFd():sock),OnSocketReceiveFuncDynamic,ctx,(void*)(sock_ctx==NULL? pThis->GetSocketCtx():sock_ctx));
		if(ret){printf("ReadRegister: eloop_register_read_sock failed(OnSocketReceiveFuncDynamic)!!\n");delete ctx;/*countPairs[1]--;*/}
		return ret? NULL:ctx;
	} 
	static EloopIntFuncPair* WriteRegister(EloopSockCallbackInterface* pThis,pOnSockWrite_t pOnSockWrite,int sock=-1,void *sock_ctx=NULL,EloopIntFuncPair* ctx=NULL)
	{
		int ret;
		if(!ctx)ctx=new EloopIntFuncPair;//countPairs[1]++;

		if(!ctx)return NULL;
		ctx->pSockObj=pThis;ctx->pOnSockWrite=pOnSockWrite;
		ctx->uiHandled=0;
		ret=eloop_register_sock((sock==-1? pThis->GetSocketFd():sock),EVENT_TYPE_WRITE,OnSocketWriteFuncDynamic,ctx,(void*)(sock_ctx==NULL? pThis->GetSocketCtx():sock_ctx));
		if(ret){printf("WriteRegister: eloop_register_write_sock failed(OnSocketWriteFuncDynamic)!!\n");delete ctx;/*countPairs[1]--;*/}
		return ret? NULL:ctx;
	} 
	static void ReadUnregister(EloopSockCallbackInterface* pThis,EloopIntFuncPair* ctx,int sock=-1,bool bCtxFree=true)
	{
		eloop_unregister_read_sock((sock==-1? pThis->GetSocketFd():sock));
		if(ctx&&bCtxFree)delete ctx;//if(ctx)countPairs[1]--;
	}
	static void WriteUnregister(EloopSockCallbackInterface* pThis,EloopIntFuncPair* ctx,int sock=-1,bool bCtxFree=true)
	{
		eloop_unregister_sock((sock==-1? pThis->GetSocketFd():sock),EVENT_TYPE_WRITE);
		if(ctx&&bCtxFree)delete ctx;//if(ctx)countPairs[1]--;
	}
	virtual EloopIntFuncPair* ReadRegisterSelfCb(int sock=-1,void *sock_ctx=NULL,pOnSockReceive_t pOnSockReceive=&EloopSockCallbackInterface::OnSockReceiveNew,EloopIntFuncPair* ctx=NULL){return ReadRegister(this,pOnSockReceive,sock,sock_ctx,ctx);}
	virtual void ReadUnregisterSelfCb(int sock,EloopIntFuncPair* ctx,bool bCtxFree=true){ReadUnregister(this,ctx,sock,bCtxFree);}
	virtual EloopIntFuncPair* WriteRegisterSelfCb(int sock=-1,void *sock_ctx=NULL,pOnSockWrite_t pOnSockWrite=&EloopSockCallbackInterface::OnSockWriteNew,EloopIntFuncPair* ctx=NULL){return WriteRegister(this,pOnSockWrite,sock,sock_ctx,ctx);}
	virtual void WriteUnregisterSelfCb(int sock,EloopIntFuncPair* ctx,bool bCtxFree=true){WriteUnregister(this,ctx,sock,bCtxFree);}

	static void OnSocketReceiveFunc(int sock, void *eloop_ctx, void *sock_ctx)
	{//called with registered state
		EloopSockCallbackInterface* pThis=(EloopSockCallbackInterface*)eloop_ctx;

		if(!eloop_ctx){printf("xxSockRecv\n");return;}
		pThis->OnSockReceive(sock,sock_ctx);
	}
	static void OnSocketWriteFunc(int sock, void *eloop_ctx, void *sock_ctx)
	{//called with registered state
		EloopSockCallbackInterface* pThis=(EloopSockCallbackInterface*)eloop_ctx;

		if(!eloop_ctx){printf("xxSockWrite\n");return;}
		pThis->OnSockWrite(sock,sock_ctx);
	}
	static int ReadRegister(EloopSockCallbackInterface* pThis,int sock=-1,void *sock_ctx=NULL)
	{
		int ret;
		ret=eloop_register_read_sock((sock==-1? pThis->GetSocketFd():sock),OnSocketReceiveFunc,pThis,(void*)(sock_ctx==NULL? pThis->GetSocketCtx():sock_ctx));
		if(ret)printf("eloop_register_read_sock failed!!\n");
		return ret;
	}
	static int WriteRegister(EloopSockCallbackInterface* pThis,int sock=-1,void *sock_ctx=NULL)
	{
		int ret;
		ret=eloop_register_sock((sock==-1? pThis->GetSocketFd():sock),EVENT_TYPE_WRITE,OnSocketWriteFunc,pThis,(void*)(sock_ctx==NULL? pThis->GetSocketCtx():sock_ctx));
		if(ret)printf("eloop_register_write_sock failed!!\n");
		return ret;
	}
	static void ReadUnregister(EloopSockCallbackInterface* pThis,int sock=-1)
	{
		eloop_unregister_read_sock((sock==-1? pThis->GetSocketFd():sock));
	}
	static void WriteUnregister(EloopSockCallbackInterface* pThis,int sock=-1)
	{
		eloop_unregister_sock((sock==-1? pThis->GetSocketFd():sock),EVENT_TYPE_WRITE);
	}
	virtual int ReadRegisterSelfId(int sock=-1,void *sock_ctx=NULL){return ReadRegister(this,sock,sock_ctx);}
	virtual void ReadUnregisterSelfId(int sock=-1){ReadUnregister(this,sock);}
	virtual int WriteRegisterSelfId(int sock=-1,void *sock_ctx=NULL){return WriteRegister(this,sock,sock_ctx);}
	virtual void WriteUnregisterSelfId(int sock=-1){WriteUnregister(this,sock);}
#endif

	virtual ~EloopSockCallbackInterface(){}

protected:
	EloopSockCallbackInterface(){}
};
/* EloopTimeoutCallbackInterface */
class EloopTimeoutCallbackInterface {
#define ms2eloopsecs(ms)		(ms)/1000,((ms)%1000)*1000
public:
	virtual void OnTimeout(void *timeout_ctx) = 0;//must implement
	virtual const void* GetTimeoutCtx() const = 0;//may be same as this
	virtual const void* GetWatchdogTimeoutCtx() const {return this;}
#if USE_ELOOP
	static void OnTimeoutCallbackFuncDynamic(void *eloop_ctx, void *timeout_ctx)
	{//called with registered state
		EloopIntFuncPair* ctx=(EloopIntFuncPair*)eloop_ctx;
		if(!eloop_ctx||!ctx){printf("xxOnTimeoutCallbackFuncDynamic\n");return;}
		EloopTimeoutCallbackInterface* pThis=ctx->pTimeoutObj;
		pOnTimeout_t pOnTimeout=ctx->pOnTimeout;

		//printf("++OnTimeoutCallbackFuncDynamic(eloop_ctx0x%x,timeout_ctx%d)(0x%x->*0x%x)\n",eloop_ctx,timeout_ctx,(unsigned int)pThis,pOnTimeout);fflush(stdout);
		//if((unsigned int)pThis<1000){printf("fatal failure!!!!!!!!!!!!!(eloop_ctx0x%x,timeout_ctx%d)\n",eloop_ctx,timeout_ctx);return;}//if not cancelled, this special value can occurred!!
		ctx->uiHandled++;
		(pThis->*pOnTimeout)(timeout_ctx);
		//printf("--OnTimeoutCallbackFuncDynamic\n");fflush(stdout);
	}
	static EloopIntFuncPair* TimeoutRegisterCb(EloopTimeoutCallbackInterface* pThis,pOnTimeout_t pOnTimeout,unsigned int secs,unsigned int usecs,void *timeout_ctx=NULL,EloopIntFuncPair* ctx=NULL)//ctx provided when reuse ctx
	{
		int ret;
		if(!ctx)ctx=new EloopIntFuncPair;//countPairs[2]++;

		if(!ctx)return NULL;
		ctx->uiHandled=0;
		ctx->pTimeoutObj=pThis;ctx->pOnTimeout=pOnTimeout;
		ret=eloop_register_timeout(secs, usecs, OnTimeoutCallbackFuncDynamic, (void *)ctx, (timeout_ctx==NULL? (void*)pThis->GetTimeoutCtx():timeout_ctx));// register
		if(ret){printf("TimeoutRegister2: eloop_register_timeout failed(OnTimeoutCallbackFuncDynamic)!!\n");delete ctx;/*countPairs[2]--;*/}
		return ret? NULL:ctx;
	}
	static int TimeoutUnregisterCb(EloopTimeoutCallbackInterface* pThis,EloopIntFuncPair* ctx,void *timeout_ctx,bool bCtxFree=true)//bCtxFree false when reuse ctx
	{
		int ret;

		if(!ctx->uiHandled)
			ret=eloop_cancel_timeout(OnTimeoutCallbackFuncDynamic, (void *)ctx, (timeout_ctx==NULL? (void*)pThis->GetTimeoutCtx():timeout_ctx));// un-register
		if(ctx->uiHandled)ret=1;
		//if(!ret)printf("TimeoutUnregister2: eloop_cancel_timeout failed for timeout_ctx=0x%x(%d)!!\n",(unsigned int)timeout_ctx,(unsigned int)timeout_ctx);
		if(ctx&&bCtxFree)delete ctx;//if(ctx)countPairs[2]--;
		return ret;
	}
	virtual EloopIntFuncPair* TimeoutRegisterSelfCb(unsigned int secs,unsigned int usecs,void *timeout_ctx=NULL,pOnTimeout_t pOnTimeout=&EloopTimeoutCallbackInterface::OnTimeout,EloopIntFuncPair* ctx=NULL){return TimeoutRegisterCb(this,pOnTimeout,secs,usecs,timeout_ctx,ctx);}
	virtual int TimeoutUnregisterSelfCb(void *timeout_ctx,EloopIntFuncPair* ctx,bool bCtxFree=true){return TimeoutUnregisterCb(this,ctx,timeout_ctx,bCtxFree);}
	//friendly alias!!
	virtual EloopIntFuncPair* SetTimeout(unsigned int secs,unsigned int usecs,unsigned int uiTimerId,pOnTimeout_t pOnTimeout,EloopIntFuncPair* ctx=NULL){return TimeoutRegisterSelfCb(secs,usecs,(void*)uiTimerId,pOnTimeout,ctx);}
	virtual int KillTimeout(unsigned int uiTimerId,EloopIntFuncPair* ctx,bool bCtxFree=true){return TimeoutUnregisterSelfCb((void*)uiTimerId,ctx,bCtxFree);}

	static void OnTimeoutCallbackFunc(void *eloop_ctx, void *timeout_ctx)
	{//called after already removed from Eloop_list
		EloopTimeoutCallbackInterface* pThis=(EloopTimeoutCallbackInterface*)eloop_ctx;

		if(!eloop_ctx){printf("xxTimeout\n");return;}
		//printf("++OnTimeoutCallbackFunc\n");fflush(stdout);
		pThis->OnTimeout(timeout_ctx);
		//printf("--OnTimeoutCallbackFunc\n");fflush(stdout);
	}
	static int TimeoutRegisterId(EloopTimeoutCallbackInterface* pThis,unsigned int secs,unsigned int usecs,void *timeout_ctx=NULL)
	{
		int ret;
		ret=eloop_register_timeout(secs, usecs, OnTimeoutCallbackFunc, (void *)pThis, (timeout_ctx==NULL? (void*)pThis->GetTimeoutCtx():timeout_ctx));// register
		if(ret)printf("TimeoutRegister: eloop_register_timeout failed for timeout_ctx=0x%x(%d)!!\n",(unsigned int)timeout_ctx,(unsigned int)timeout_ctx);
		return ret;
	}
	static int TimeoutUnregisterId(EloopTimeoutCallbackInterface* pThis,void *timeout_ctx=NULL)
	{
		int ret;
		ret=eloop_cancel_timeout(OnTimeoutCallbackFunc, (void *)pThis, (timeout_ctx==NULL? (void*)pThis->GetTimeoutCtx():timeout_ctx));// un-register
		//if(!ret)printf("TimeoutUnregister: eloop_cancel_timeout failed for timeout_ctx=0x%x(%d)!!\n",(unsigned int)timeout_ctx,(unsigned int)timeout_ctx);
		return ret;
	}
	virtual int TimeoutRegisterSelfId(unsigned int secs,unsigned int usecs,void *timeout_ctx=NULL){return TimeoutRegisterId(this,secs,usecs,timeout_ctx);}
	virtual int TimeoutUnregisterSelfId(void *timeout_ctx=NULL){return TimeoutUnregisterId(this,timeout_ctx);}
	//friendly alias!!
	virtual int SetTimer(unsigned int secs,unsigned int usecs,unsigned int uiTimerId){return TimeoutRegisterSelfId(secs,usecs,(void*)uiTimerId);}
	virtual int KillTimer(unsigned int uiTimerId){return  TimeoutUnregisterSelfId((void*)uiTimerId);}
#if ELOOP_WATCHDOG_ADDON
#if HYWMON_SUPPORT_WDTMON
	static int WatchdogTimeoutRegister(EloopTimeoutCallbackInterface* pThis,unsigned int secs,unsigned int usecs,void *timeout_ctx=NULL)
	{
		int ret;
		ret=eloop_register_watchdog_timeout(secs, usecs, (void *)pThis, (timeout_ctx==NULL? (void*)pThis->GetWatchdogTimeoutCtx():timeout_ctx));// register
		if(ret)printf("eloop_register_watchdog_timeout failed!!\n");
		return ret;
	}
	static int WatchdogTimeoutUnregister(EloopTimeoutCallbackInterface* pThis,void *timeout_ctx=NULL)
	{
		return eloop_cancel_watchdog_timeout((void *)pThis, (timeout_ctx==NULL? (void*)pThis->GetWatchdogTimeoutCtx():timeout_ctx));// un-register
	}
	virtual int WatchdogTimeoutRegisterSelf(unsigned int secs,unsigned int usecs,void *timeout_ctx=NULL){return WatchdogTimeoutRegister(this,secs,usecs,timeout_ctx);}
	virtual int WatchdogTimeoutUnregisterSelf(void *timeout_ctx=NULL){return WatchdogTimeoutUnregister(this,timeout_ctx);}
#endif
#endif
	virtual void Terminate(int reload=0,int exitCode=0){reloading=reload;g_exitCode=exitCode;eloop_terminate();}
#endif
	virtual ~EloopTimeoutCallbackInterface(){}

protected:
	EloopTimeoutCallbackInterface(){}
};
#if 1
#include"taskimpl.h"

typedef struct _cppTaskCtx{
        /*for ucontext*/
        baseTaskCtx baseCtx;

	/*for object pointer*/
	void* pThis;
        /*for return value*/
	int ret;
        /*for callback*/
	void (*callback)(void* ctx);int param;
	/*for initial args*/
	int arg1,arg2;
	/*for stack*/
	int stack[1];
}cppTaskCtx,*PcppTaskCtx;

class TaskCallbackInterface {
	PcppTaskCtx pTaskCtx;
public:
	PcppTaskCtx GetTaskCtx(void){return pTaskCtx;}
	virtual void* Run(int arg1) {//must implement
		PbaseTaskCtx ctx=(PbaseTaskCtx)&pTaskCtx->baseCtx;
		PcppTaskCtx test_ctx=(PcppTaskCtx)pTaskCtx;
		int i;

		printf("Run_routine start(%d,%d,ARG1=%d)!!\n",test_ctx->arg1,test_ctx->arg2,arg1);
		printf("ctx=0x%x\n",(unsigned int)ctx);
		//do not call blocking call,but can call registerSockOrTimer to defaultloop and sleep_task_on_defaultloop!!
		for(i=0;i<5;i++){
			int tmo;
			
			printf("Run_routine loop%d(ctx->arg1=%d,ctx->arg2=%d)!!\n",i,test_ctx->arg1,test_ctx->arg2);
			//tmo=sleep_task_on_defaultloop(4,0,ctx,NULL,NULL/*or timerId*/);
			tmo=sleep_wait(4000);
			printf("Run_routine resume1St(tmo=%d)!!\n",tmo);
			
			//tmo=sleep_task_on_defaultloop(2,0,ctx,NULL,NULL/*or timerId*/);
			tmo=sleep_wait(2000);
			printf("Run_routine resume2nd(tmo=%d)!!\n",tmo);
			test_ctx->param=i;
			//tmo=yield_task_to_defaultloop(ctx,test_callback,NULL/*or timerId*/);
			tmo=yield_wait();
			printf("Run_routine resume3rd(tmo=%d)!!\n",tmo);
		}
		//test_ctx->arg2=30;
		test_ctx->ret=3;
		exit_task(ctx,NULL,NULL);
		printf("Run_routine invalid resume!!!\n");
		return NULL;
	}
	static TASK_API void OnExit_task_defaultloop(PbaseTaskCtx ctx)//on default ctx
	{
		{
			printf("unknown task(%x) exit detected!!\n",(unsigned int)ctx);
		}
	}
	static void* RunFuncDynamic(void* ctxOrg,int arg1){
		EloopIntFuncPair* ctx=(EloopIntFuncPair*)ctxOrg;
		if(!ctxOrg||!ctx){printf("xxRunFuncDynamic\n");return NULL;}
		TaskCallbackInterface* pThis=(TaskCallbackInterface*)ctx->pTaskObj;
		pRun_t pRun=ctx->pRun;

		//printf("++RunFuncDynamic(ctxOrg=0x%x,arg1=%d)(0x%x->*0x%x)\n",ctxOrg,arg1,(unsigned int)pThis,pRun);fflush(stdout);
		ctx->uiHandled++;
		(pThis->*pRun)(arg1);
		//printf("--RunFuncDynamic\n");fflush(stdout);
		return NULL;
	}
	static void* RunFunc(void* ctx,int arg1){
		TaskCallbackInterface* pThis=(TaskCallbackInterface*)ctx;

		if(!ctx){printf("xxRunFunc\n");return NULL;}
		//printf("++RunFunc\n");fflush(stdout);
		pThis->Run(arg1);
		//printf("--RunFunc\n");fflush(stdout);
		return NULL;
	}

	EloopIntFuncPair* TaskStartCb(TaskCallbackInterface* pThis,pRun_t pRun,int stkSize=DEFAULT_TASK_STACK_SIZE,int arg1=0,int arg2=0,EloopIntFuncPair* ctx=NULL)//ctx provided when reuse ctx
	{
		int ret=0;
		if(!ctx)ctx=new EloopIntFuncPair;//countPairs[2]++;

		if(!ctx)return NULL;
		ctx->uiHandled=0;
		ctx->pTaskObj=pThis;ctx->pRun=pRun;
		{
				static PcppTaskCtx test_ctx=NULL;
				PbaseTaskCtx task_ctx=NULL;
				
				task_ctx=(PbaseTaskCtx)make_task(RunFuncDynamic,sizeof(cppTaskCtx),stkSize,arg1);
				test_ctx=(PcppTaskCtx)task_ctx;
				this->pTaskCtx=test_ctx;
				test_ctx->ret=-1;
				test_ctx->arg1=arg1;test_ctx->arg2=arg2;
				//set exit callback here!!
				start_task(task_ctx);
		}
		
		if(ret){printf("TaskStartCb: start start failed(RunFuncDynamic)!!\n");delete ctx;/*countPairs[2]--;*/}
		return ret? NULL:ctx;
	}
	int TaskStopCb(TaskCallbackInterface* pThis,EloopIntFuncPair* ctx,bool bCtxFree=true)//bCtxFree false when reuse ctx
	{
		int ret=0;

		//if(!ctx->uiHandled)
		{
			if(pTaskCtx){
				printf("cpp task ending!!\n");
				destroy_task((PbaseTaskCtx)pTaskCtx);
				pTaskCtx=NULL;
			}
		}
		if(ctx->uiHandled)ret=1;
		//if(!ret)printf("TaskStopCb: taskStop failed for pTaskCtx=0x%x(%d)!!\n",(unsigned int)pTaskCtx,(unsigned int)pTaskCtx);
		if(ctx&&bCtxFree)delete ctx;//if(ctx)countPairs[2]--;
		return ret;
	}
	virtual EloopIntFuncPair* TaskStartSelfCb(int stkSize=DEFAULT_TASK_STACK_SIZE,int arg1=0,int arg2=0,pRun_t pRun=&TaskCallbackInterface::Run,EloopIntFuncPair* ctx=NULL){return TaskStartCb(this,pRun,stkSize,arg1,arg2,ctx);}
	virtual int TaskStopSelfCb(EloopIntFuncPair* ctx,bool bCtxFree=true){return TaskStopCb(this,ctx,bCtxFree);}
	//friendly alias!!
	//....
	
	int TaskStartSelf(int stkSize=DEFAULT_TASK_STACK_SIZE,int arg1=0,int arg2=0){
		static PcppTaskCtx test_ctx=NULL;
		PbaseTaskCtx task_ctx=NULL;
		
		task_ctx=(PbaseTaskCtx)make_task(RunFunc,sizeof(cppTaskCtx),stkSize,arg1);
		if(!task_ctx)return 0;
		test_ctx=(PcppTaskCtx)task_ctx;
		this->pTaskCtx=test_ctx;
		test_ctx->ret=-1;
		test_ctx->arg1=arg1;test_ctx->arg2=arg2;
		//set exit callback here!!
		start_task(task_ctx);
		return 1;
	}
	void TaskStopSelf(void){
		if(pTaskCtx){
			printf("cpp task ending!!\n");
			destroy_task((PbaseTaskCtx)pTaskCtx);
			pTaskCtx=NULL;
		}
	}
	//friendly alias!!
	//....

	///
	int register_read_sock(int sock,defaultloop_sock_handler handler=NULL,void *user_data=NULL)
	{
		return defaultloop_register_read_sock(sock,handler,&pTaskCtx->baseCtx,user_data);
	}
	void unregister_read_sock(int sock)
	{
		defaultloop_unregister_read_sock(sock);
	}

	///
	int sleep_wait(unsigned int ms,task_callback_t callback=NULL,void* callback_ctx=NULL){
		if(callback&&!callback_ctx)callback_ctx=this;
		return sleep_task_on_defaultloop(ms/1000,ms%1000*1000,&pTaskCtx->baseCtx,callback,callback_ctx);
	}
	int yield_wait(task_callback_t callback=NULL,void* callback_ctx=NULL){
		if(callback&&!callback_ctx)callback_ctx=this;
		return yield_task_to_defaultloop(&pTaskCtx->baseCtx,callback,callback_ctx);
	}

	////
	virtual ~TaskCallbackInterface(){TaskStopSelf();}
protected:
	TaskCallbackInterface(int stkSize=DEFAULT_TASK_STACK_SIZE,int arg1=0,int arg2=0){TaskStartSelf(stkSize,arg1,arg2);}
};
#endif
#endif /* __cplusplus */

#endif	//USE_ELOOP
#endif /* __ELOOP_CALLBACK_IF__ */
