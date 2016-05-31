/* Copyright (c) 2005-2006 Russ Cox, MIT; see COPYRIGHT */
#ifdef WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400 //for FiberFuncs!!
#endif
#endif

#include "taskimpl.h"

#if defined(__APPLE__)
#if defined(__i386__)
#define NEEDX86MAKECONTEXT
#define NEEDSWAPCONTEXT
#elif defined(__x86_64__)
#define NEEDAMD64MAKECONTEXT
#define NEEDSWAPCONTEXT
#else
#define NEEDPOWERMAKECONTEXT
#define NEEDSWAPCONTEXT
#endif
#endif

#if defined(__FreeBSD__) && defined(__i386__) && __FreeBSD__ < 5
#define NEEDX86MAKECONTEXT
#define NEEDSWAPCONTEXT
#endif

#if defined(__OpenBSD__) && defined(__i386__)
#define NEEDX86MAKECONTEXT
#define NEEDSWAPCONTEXT
#endif

#if defined(__linux__) && defined(__arm__)
#define NEEDSWAPCONTEXT
#define NEEDARMMAKECONTEXT
#endif

#if defined(__linux__) && defined(__mips__)
#define	NEEDSWAPCONTEXT
#define	NEEDMIPSMAKECONTEXT
#endif

#ifdef NEEDPOWERMAKECONTEXT
void
makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	ulong *sp, *tos;
	va_list arg;

	tos = (ulong*)ucp->uc_stack.ss_sp+ucp->uc_stack.ss_size/sizeof(ulong);
	sp = tos - 16;	
	ucp->mc.pc = (long)func;
	ucp->mc.sp = (long)sp;
	va_start(arg, argc);
	ucp->mc.r3 = va_arg(arg, long);
	va_end(arg);
}
#endif

#ifdef NEEDX86MAKECONTEXT
void
makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	int *sp;

	sp = (int*)ucp->uc_stack.ss_sp+ucp->uc_stack.ss_size/4;
	sp -= argc;
	sp = (void*)((uintptr_t)sp - (uintptr_t)sp%16);	/* 16-align for OS X */
	memmove(sp, &argc+1, argc*sizeof(int));

	*--sp = 0;		/* return address */
	ucp->uc_mcontext.mc_eip = (long)func;
	ucp->uc_mcontext.mc_esp = (int)sp;
}
#endif

#ifdef NEEDAMD64MAKECONTEXT
void
makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	long *sp;
	va_list va;

	memset(&ucp->uc_mcontext, 0, sizeof ucp->uc_mcontext);
	if(argc != 2)
		*(int*)0 = 0;
	va_start(va, argc);
	ucp->uc_mcontext.mc_rdi = va_arg(va, int);
	ucp->uc_mcontext.mc_rsi = va_arg(va, int);
	va_end(va);
	sp = (long*)ucp->uc_stack.ss_sp+ucp->uc_stack.ss_size/sizeof(long);
	sp -= argc;
	sp = (void*)((uintptr_t)sp - (uintptr_t)sp%16);	/* 16-align for OS X */
	*--sp = 0;	/* return address */
	ucp->uc_mcontext.mc_rip = (long)func;
	ucp->uc_mcontext.mc_rsp = (long)sp;
}
#endif

#ifdef NEEDARMMAKECONTEXT
void
makecontext(ucontext_t *uc, void (*fn)(void), int argc, ...)
{
/*
struct sigcontext {	unsigned long trap_no;	unsigned long error_code;	unsigned long oldmask;	unsigned long arm_r0;	unsigned long arm_r1;	unsigned long arm_r2;	unsigned long arm_r3;	unsigned long arm_r4;	unsigned long arm_r5;	unsigned long arm_r6;	unsigned long arm_r7;	unsigned long arm_r8;	unsigned long arm_r9;	unsigned long arm_r10;	unsigned long arm_fp;	unsigned long arm_ip;	unsigned long arm_sp;	unsigned long arm_lr;	unsigned long arm_pc;	unsigned long arm_cpsr;	unsigned long fault_address;};
*/
	unsigned int* p_gregs=(unsigned int*)&uc->uc_mcontext.arm_r0;//or &uc->uc_mcontext.gregs[0] //ywhwang patch for ARM2009Q3
	int i, *sp;
	va_list arg;
	
	sp = (int*)uc->uc_stack.ss_sp+uc->uc_stack.ss_size/4;
	va_start(arg, argc);
	for(i=0; i<4 && i<argc; i++)
		p_gregs[i] = va_arg(arg, uint);
	va_end(arg);
	p_gregs[13] = (uint)sp;
	p_gregs[14] = (uint)fn;
}
#endif

#ifdef NEEDMIPSMAKECONTEXT
void
makecontext(ucontext_t *uc, void (*fn)(void), int argc, ...)
{
	int i, *sp;
	va_list arg;
	
	va_start(arg, argc);
	sp = (int*)uc->uc_stack.ss_sp+uc->uc_stack.ss_size/4;
	for(i=0; i<4 && i<argc; i++)
		uc->uc_mcontext.mc_regs[i+4] = va_arg(arg, int);
	va_end(arg);
	uc->uc_mcontext.mc_regs[29] = (int)sp;
	uc->uc_mcontext.mc_regs[31] = (int)fn;
}
#endif

#ifdef NEEDSWAPCONTEXT
int
swapcontext(ucontext_t *oucp, const ucontext_t *ucp)
{
	if(getcontext(oucp) == 0)
		setcontext(ucp);
	return 0;
}
#endif

///////////////////////////////////////////Fiber library for default_loop///////////////////////////////////////////
#if USE_LIBTASK_LITE
#define DBG_TASK_SCHED 0
#ifdef DBG_TASK_SCHED
#include<stdio.h>
#endif

static PbaseTaskCtx default_ctx=NULL,cur_task_ctx=NULL;
static task_exit_callback_t g_OnExit_task_callback=NULL;
static void ifCompleted_task(PbaseTaskCtx ctx)//on default ctx
{
	int hasExitCallback=0;
	
	if(ctx->callback){
		hasExitCallback=1;
		ctx->callback(ctx,ctx->callback_ctx);//call backfucntion!!
		ctx->callback=NULL;//clear callback
		ctx->callback_ctx=NULL;
	}
	if(ctx->bCompleted){
		;//call on complete callback!!
		if(!hasExitCallback&&g_OnExit_task_callback)g_OnExit_task_callback(ctx);
		destroy_task(ctx);
		if(DBG_TASK_SCHED)printf("ifCompleted_task: task destroyed!!\n");
	}
}
static void* make_default_task(void)//on default ctx
{
	if(!default_ctx){
		if(DBG_TASK_SCHED)printf("+default alloc task\n");
		default_ctx=make_task(NULL,sizeof(baseTaskCtx),0,0);
		if(DBG_TASK_SCHED)printf("-default alloc task\n");
	}
	return default_ctx;
}
static void destroy_default_task(void)//on default ctx
{
	if(default_ctx){
		if(DBG_TASK_SCHED){printf("+default free task\n");fflush(stdout);}
		destroy_task(default_ctx);
		default_ctx=NULL;
		if(DBG_TASK_SCHED){printf("-default free task\n");fflush(stdout);}
	}
}

TASK_API void defaultloop_handler_for_task(void *defaultloop_data/*task ctx*/, void *user_data, int bTimeout)//on default ctx
{
	PbaseTaskCtx ctx=(PbaseTaskCtx)defaultloop_data;
   
	make_default_task();
	if(DBG_TASK_SCHED)printf("defaultloop_handler_for_task: sched_task(ctx)\n");
	ctx->bIsTimeout=(bTimeout? (user_data? (int)(long int)user_data:1):0);// only 32bit is pass to task!!
	sched_task(ctx);

	if(DBG_TASK_SCHED)printf("defaultloop_handler_for_task: exiting\n");
}

TASK_API void* make_task(task_entry_t start_routine,int ctxSize,int stkSize,int arg1)//on default ctx
{
	void* task_ctx;
	PbaseTaskCtx ctx;
#ifdef WIN32
	stkSize=0;//stkSize 0:for auto

	if(ctxSize==0)ctxSize=sizeof(baseTaskCtx);
#else
	unsigned int *func_stack=NULL;

	if(ctxSize==0)ctxSize=sizeof(baseTaskCtx);
	if(start_routine&&stkSize==0)stkSize=DEFAULT_TASK_STACK_SIZE;
#endif
	task_ctx=malloc(ctxSize+stkSize);
	if(DBG_TASK_SCHED)printf("make_task: task_ctx is %lx\n",(unsigned long)task_ctx);	
	memset(task_ctx,0,(ctxSize+stkSize));
	ctx=(PbaseTaskCtx)task_ctx;
	ctx->bCompleted=0;

#ifdef WIN32//kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib 
	if(start_routine==NULL)
		ctx->task_handle=ConvertThreadToFiber(ctx);//start_routine,stkSize,arg1 is not used!!
	else
		ctx->task_handle=CreateFiber(stkSize,(LPFIBER_START_ROUTINE)start_routine,ctx);//stkSize 0:for auto//&ctx->task_uc
	if(DBG_TASK_SCHED)printf("make_task: ctx is made %lx\n",(unsigned long)ctx);	
	//makecontext는 직접 호출되지 않는다.!!
#else
	func_stack=(unsigned int *)((char*)task_ctx+ctxSize);
	if(DBG_TASK_SCHED)printf("make_task: func_stack is %lx\n",(unsigned long)func_stack);	
	if(stkSize>0&&start_routine!=NULL){//only for task, not for default_ctx
	        if (getcontext(&ctx->task_uc) == -1)
	                die("getcontext");
	        ctx->task_uc.uc_stack.ss_sp = func_stack+8;
#if 1			
      		if((unsigned long)ctx->task_uc.uc_stack.ss_sp&7){
      			stkSize-=8-((unsigned long)ctx->task_uc.uc_stack.ss_sp&7);
      			ctx->task_uc.uc_stack.ss_sp=(void*)((unsigned char*)ctx->task_uc.uc_stack.ss_sp+8-((unsigned long)ctx->task_uc.uc_stack.ss_sp&7));/*for float align for STACKBASE*/
      		}
      		if(stkSize&7){
      			stkSize-=8-(stkSize&7);/*for float align for STACKTOP*/
      		}
#endif
	        ctx->task_uc.uc_stack.ss_size = stkSize-64;
			ctx->task_ucp=&ctx->task_uc;
		//COPY args HERE!?
	        makecontext(&ctx->task_uc,(void(*)())start_routine,2,ctx,arg1);
		if(DBG_TASK_SCHED)printf("make_task: ctx is made %lx\n",(unsigned long)ctx);	
	}
#endif
	return ctx;
}
TASK_API void destroy_task(PbaseTaskCtx ctx)//on default ctx
{
#ifdef WIN32
	if(ctx&&default_ctx!=ctx)DeleteFiber(ctx->task_handle);//do not delete initialThread!!
#endif
	free(ctx);
}
TASK_API void switch2task(PbaseTaskCtx curCtx,PbaseTaskCtx newCtx)//on any ctx
{
	if(DBG_TASK_SCHED)printf("+switch2task\n");
	if(cur_task_ctx==curCtx){
		cur_task_ctx=newCtx;
#ifdef WIN32
		SwitchToFiber(newCtx->task_handle);
#else
		//if(swapcontext(curCtx->task_ucp, newCtx->task_ucp)==-1)die("swapcontext");
		if(swapcontext(&curCtx->task_uc, &newCtx->task_uc)==-1)die("swapcontext");
#endif
		if(DBG_TASK_SCHED)printf("switch2task: checking result\n");
		if(cur_task_ctx==default_ctx)ifCompleted_task(newCtx);//for async mode!!(on resume)
		else ;//no task checking!!
	}
	//else printf("switch2task> newCtx:%x,default_ctx%x cur_task_ctx%x is not curCtx%x\n",newCtx,default_ctx,cur_task_ctx,curCtx);
	if(DBG_TASK_SCHED)printf("-switch2task\n");
}
TASK_API void sched_task(PbaseTaskCtx ctx)//on default ctx
{//switch2task(default_ctx,ctx);
	if(DBG_TASK_SCHED)printf("+sched_task\n");
#if 1
	switch2task(default_ctx,ctx);
#else
	if(cur_task_ctx==default_ctx){
	cur_task_ctx=ctx;
	if(swapcontext(&default_ctx->task_uc, &ctx->task_uc)==-1)die("swapcontext");
	if(DBG_TASK_SCHED)printf("sched_task: checking result\n");
	ifCompleted_task(ctx);//for async mode!!(on resume)
	}
	//else printf("sched_task> ctx:%x cur_task_ctx%x is not default_ctx%x\n",ctx,cur_task_ctx,default_ctx);
#endif	
	if(DBG_TASK_SCHED)printf("-sched_task\n");
}
TASK_API void sched_defaultloop(PbaseTaskCtx ctx)//on task ctx
{//switch2task(ctx,default_ctx);
	if(DBG_TASK_SCHED)printf("+sched_defaultloop\n");
#if 1
	switch2task(ctx,default_ctx);
#else
	if(cur_task_ctx==ctx){
	cur_task_ctx=default_ctx;
	if(swapcontext(&ctx->task_uc,&default_ctx->task_uc)==-1)die("swapcontext");
	}
	else printf("sched_defaultloop> default_ctx:%lx cur_task_ctx%lx is not ctx%lx\n",(unsigned long)default_ctx,(unsigned long)cur_task_ctx,(unsigned long)ctx);
#endif
	if(DBG_TASK_SCHED)printf("-sched_defaultloop\n");
}
TASK_API void callback_on_defaultloop(PbaseTaskCtx ctx,task_callback_t callback,void* callback_ctx)//on task ctx
{
#ifndef WIN32
	ucontext_t *oldUcp;ucontext_t runUc;
	oldUcp=ctx->task_ucp;ctx->task_ucp=&runUc;
#endif
	ctx->callback_ctx=callback_ctx? callback_ctx:ctx;
	ctx->callback=callback;//set callback
	sched_defaultloop(ctx);
#ifndef WIN32
	ctx->task_ucp=oldUcp;
#endif
}
TASK_API void exit_task(PbaseTaskCtx ctx,task_callback_t callback,void* callback_ctx)//on task ctx
{
	if(DBG_TASK_SCHED)printf(">exit_task!!\n");
	ctx->bCompleted=1;
	callback_on_defaultloop(ctx,callback,callback_ctx);
	if(DBG_TASK_SCHED)printf(">exit_task invalid resume!!\n");
}
TASK_API void start_task(PbaseTaskCtx ctx)//on default ctx
{
	make_default_task();
#if 0	//only support sync_call
	switch2task(cur_task_ctx,ctx);
#else
	sched_task(ctx);
#endif
}
TASK_API void defaultloop_task_init(task_exit_callback_t OnExit_task_callback)//on default ctx
{
	if(make_default_task()){
		g_OnExit_task_callback=OnExit_task_callback;
		if(!cur_task_ctx)cur_task_ctx=default_ctx;
	}
}
TASK_API void defaultloop_task_final(void)//on default ctx
{
	destroy_default_task();
	g_OnExit_task_callback=NULL;
	cur_task_ctx=NULL;
}
///////////////////////////////////////Eloop depdendet code: must check///////////////
#if USE_LIBTASK_LITE_IMPL0
#define YIELD_SEC 0 // 0
#define YIELD_USEC 1000 // 1
TASK_API int yield_task_to_defaultloop(PbaseTaskCtx ctx,task_callback_t callback,void* callback_ctx)//on task ctx
{
	return sleep_task_on_defaultloop(YIELD_SEC,YIELD_USEC,ctx,callback,callback_ctx);
}
TASK_API int sleep_task_on_defaultloop(int secs,int usecs,PbaseTaskCtx ctx,task_callback_t callback,void* callback_ctx)//on task ctx
{
        int isTimeout=-1;
        
        defaultloop_register_timeout(secs,usecs,defaultloop_timeout_handler_for_task,ctx,(void*)callback_ctx);//Eloop depdendet code
        if(DBG_TASK_SCHED)printf("sleep_task_on_defaultloop: running\n");
        if(DBG_TASK_SCHED)printf("sleep_task_on_defaultloop: callback_on_defaultloop(ctx)\n");
	ctx->bIsTimeout=-1;
        callback_on_defaultloop(ctx,callback,callback_ctx);// sched_defaultloop(ctx);
	isTimeout=ctx->bIsTimeout;
        if(DBG_TASK_SCHED)printf("sleep_task_on_defaultloop: resumed\n");
        if(isTimeout!=1)defaultloop_cancel_timeout(defaultloop_timeout_handler_for_task,ctx,(void*)callback_ctx);//auto unregister timeout!!//Eloop depdendet code
        return isTimeout;
}
#endif /*USE_LIBTASK_LITE_IMPL0*/
#endif /*USE_LIBTASK_LITE*/

///////////////////////////////////////Eloop depdendet code: must implement///////////////
#if USE_LIBTASK_LITE_IMPL
#include"includes.h"
#include"eloop.h"
//You must implement timer
TASK_API void defaultloop_timeout_handler_for_task(void *defaultloop_data, void *user_data)//on default ctx
{
	defaultloop_handler_for_task(defaultloop_data,user_data,1);
}
TASK_API int defaultloop_register_timeout(unsigned int secs, unsigned int usecs,defaultloop_timeout_handler handler, void *defaultloop_data, void *user_data)//on task ctx
{
	if(!handler)handler=defaultloop_timeout_handler_for_task;
	return eloop_register_timeout(secs,usecs,handler,defaultloop_data,user_data);
}
TASK_API int defaultloop_cancel_timeout(defaultloop_timeout_handler handler, void *defaultloop_data, void *user_data)//on task ctx
{
	if(!handler)handler=defaultloop_timeout_handler_for_task;
	return eloop_cancel_timeout(handler,defaultloop_data,user_data);
}

//If you have read IO async
TASK_API void defaultloop_sock_handler_for_task(int sock, void *defaultloop_data, void *user_data)//on default ctx
{
	defaultloop_handler_for_task(defaultloop_data,user_data,0);
}
TASK_API int defaultloop_register_read_sock(int sock, defaultloop_sock_handler handler,void *defaultloop_data, void *user_data)//on task ctx
{
	if(!handler)handler=defaultloop_sock_handler_for_task;
	return eloop_register_read_sock(sock,handler,defaultloop_data,user_data);
}	
TASK_API void defaultloop_unregister_read_sock(int sock)//on task ctx
{
	eloop_unregister_read_sock(sock);
}
#endif /*USE_LIBTASK_LITE_IMPL*/
//>>Rendez
//tasksleep: sleep on Rendez_waitingList & switch to ready any task
//taskwakeup: get a/all task on Rendez list & switch to the task
//>>Qlock
//lock: no owner then owner is Me &return, else sleep on Qlock_waitingList
//unlock: make owner is waiting_head and switch to the task
//rlock: if no writer&write_waiting
// wlock: if no write& no num readers
//fd.c: same as eloop?
//channel.c: IPC??
//net.c : depend on fd.c
//print.c : depend on fd.c
#ifdef CONFIG_NETCONSOLE
extern void nc_puts (const char *s);
extern int nc_start (void);
#endif
#ifdef CONFIG_LOGBUFFER
extern void logbuff_log(char *msg);
#endif
volatile int g_bErrToSyslog=0,g_bErrToNet=0,g_bErrToLog=0,g_bErrToNull=0;
#ifndef CFG_PBSIZE
#define CFG_PBSIZE 256
#endif
void n_printf (/*int level,*/ const char *fmt, ...)
{
	va_list args;
	unsigned int i;
	char printbuffer[CFG_PBSIZE];

	va_start (args, fmt);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
#ifdef CONFIG_NETSYSLOG 
	if(g_bErrToSyslog){
		vsyslog (1, fmt, args);
		return;
	}
	else
#endif
		i = vsprintf (printbuffer, fmt, args);
	va_end (args);

	/* Send to desired file */
#if 1
#ifdef CONFIG_NETCONSOLE
	if(g_bErrToNet)nc_puts (printbuffer);
	else
#endif
#ifdef CONFIG_LOGBUFFER
	if(g_bErrToLog)logbuff_log (printbuffer);
	else
#endif
	if(g_bErrToNull);
	else
#endif
		fputs(printbuffer,stdout);
}
void no_printf(char* fmt, ...)
{
}
