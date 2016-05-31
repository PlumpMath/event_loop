#ifndef __TASKIMPL_H__
#define __TASKIMPL_H__
/* Copyright (c) 2005-2006 Russ Cox, MIT; see COPYRIGHT */

#if defined(__sun__)
#	define __EXTENSIONS__ 1 /* SunOS */
#	if defined(__SunOS5_6__) || defined(__SunOS5_7__) || defined(__SunOS5_8__)
		/* NOT USING #define __MAKECONTEXT_V2_SOURCE 1 / * SunOS */
#	else
#		define __MAKECONTEXT_V2_SOURCE 1
#	endif
#endif

#ifdef WIN32
#define USE_UCONTEXT 0
#define USE_WIN32FIBER 1
#else
#define USE_UCONTEXT 1
#endif
#define USE_LIBTASK 0
#define USE_LIBTASK_LITE 1
#define USE_LIBTASK_LITE_IMPL0 1
#define USE_LIBTASK_LITE_IMPL 1

#if defined(__OpenBSD__) || defined(__mips__)
#undef USE_UCONTEXT
#define USE_UCONTEXT 0
#endif

#if defined(__APPLE__)
#include <AvailabilityMacros.h>
#if defined(MAC_OS_X_VERSION_10_5)
#undef USE_UCONTEXT
#define USE_UCONTEXT 0
#endif
#endif

#include <errno.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#include <string.h>
#include <assert.h>
#include <time.h>
typedef struct
{
   DWORD dwParameter;          // DWORD parameter to fiber (unused)
   DWORD dwFiberResultCode;    // GetLastError() result code
   HANDLE hFile;               // handle to operate on
   DWORD dwBytesProcessed;     // number of bytes processed
} FIBERDATASTRUCT, *PFIBERDATASTRUCT, *LPFIBERDATASTRUCT;
//typedef FIBERDATASTRUCT ucontext_t;
#else
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#include <signal.h>
#if USE_UCONTEXT
#include <ucontext.h>
#endif
#include <sys/utsname.h>
#include <inttypes.h>
#endif

#if USE_LIBTASK
#include "task.h"
#else

#if USE_LIBTASK_LITE
#include <stdarg.h>	/*for makecontext*/
#define TASK_API 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _baseTaskCtx *PbaseTaskCtx;
typedef void* (*task_callback_t)(void* ctx,void* user_data);
#ifdef WIN32
typedef void* (*task_entry_t)(void* ctx,int arg1);
#else
typedef void* (*task_entry_t)(void* ctx,int arg1);
#endif
typedef void (*task_exit_callback_t)(PbaseTaskCtx ctx);

//////////////////////API for task
TASK_API void sched_defaultloop(PbaseTaskCtx ctx);//on task ctx
TASK_API void callback_on_defaultloop(PbaseTaskCtx ctx,task_callback_t callback,void* callback_ctx);//on task ctx
TASK_API void exit_task(PbaseTaskCtx ctx,task_callback_t callback,void* callback_ctx);//on task ctx
TASK_API void switch2task(PbaseTaskCtx curCtx,PbaseTaskCtx newCtx);//on any ctx
//////////////////////API for defaultloop
TASK_API void* make_task(task_entry_t start_routine,int ctxSize,int stkSize,int arg1);//on default ctx
TASK_API void start_task(PbaseTaskCtx ctx);//on default ctx
TASK_API void sched_task(PbaseTaskCtx ctx);//on default ctx
TASK_API void destroy_task(PbaseTaskCtx ctx);//on default ctx
TASK_API void defaultloop_task_init(task_exit_callback_t OnExit_task_callback);//on default ctx
TASK_API void defaultloop_task_final(void);//on default ctx
TASK_API void defaultloop_handler_for_task(void *defaultloop_data, void *user_data, int bTimeout);//on default ctx

///////////////////////////////////////Eloop depdendet code/////////////////////////////////////////////////
/////////////////////must DEFINE///////////////
#define DEFAULT_TASK_STACK_SIZE 16384
#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)
//////////////////////API for task: must check///////////////
TASK_API int sleep_task_on_defaultloop(int secs,int usecs,PbaseTaskCtx ctx,task_callback_t callback,void* callback_ctx);//on task ctx
TASK_API int yield_task_to_defaultloop(PbaseTaskCtx ctx,task_callback_t callback,void* callback_ctx);//on task ctx
//////////////////////API for defaultloop: must implement///////////////
TASK_API void defaultloop_timeout_handler_for_task(void *defaultloop_data, void *user_data);//on default ctx
TASK_API void defaultloop_sock_handler_for_task(int sock, void *defaultloop_data, void *user_data);//on default ctx
//////////////////////API for task: must implement///////////////
typedef void (*defaultloop_timeout_handler)(void *eloop_data, void *user_ctx);//on default ctx
typedef void (*defaultloop_sock_handler)(int sock, void *eloop_ctx, void *sock_ctx);//on default ctx
TASK_API int defaultloop_register_read_sock(int sock, defaultloop_sock_handler handler,void *defaultloop_data, void *user_data);//on task ctx
TASK_API void defaultloop_unregister_read_sock(int sock);//on task ctx
TASK_API int defaultloop_register_timeout(unsigned int secs, unsigned int usecs,defaultloop_timeout_handler handler, void *defaultloop_data, void *user_data);//on task ctx
TASK_API int defaultloop_cancel_timeout(defaultloop_timeout_handler handler, void *defaultloop_data, void *user_data);//on task ctx
//TASK_API void OnExit_task(PbaseTaskCtx ctx);//on default ctx

#ifdef __cplusplus
}
#endif
#endif  /*USE_LIBTASK_LITE*/

#endif

#if USE_LIBTASK
#define nil ((void*)0)
#define nelem(x) (sizeof(x)/sizeof((x)[0]))

#define ulong task_ulong
#define uint task_uint
#define uchar task_uchar
#define ushort task_ushort
#define uvlong task_uvlong
#define vlong task_vlong

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long long uvlong;
typedef long long vlong;

#define print task_print
#define fprint task_fprint
#define snprint task_snprint
#define seprint task_seprint
#define vprint task_vprint
#define vfprint task_vfprint
#define vsnprint task_vsnprint
#define vseprint task_vseprint
#define strecpy task_strecpy

int print(char*, ...);
int fprint(int, char*, ...);
char *snprint(char*, uint, char*, ...);
char *seprint(char*, char*, char*, ...);
int vprint(char*, va_list);
int vfprint(int, char*, va_list);
char *vsnprint(char*, uint, char*, va_list);
char *vseprint(char*, char*, char*, va_list);
char *strecpy(char*, char*, char*);
#endif

#if defined(__FreeBSD__) && __FreeBSD__ < 5
extern	int		getmcontext(mcontext_t*);
extern	void		setmcontext(const mcontext_t*);
#define	setcontext(u)	setmcontext(&(u)->uc_mcontext)
#define	getcontext(u)	getmcontext(&(u)->uc_mcontext)
extern	int		swapcontext(ucontext_t*, const ucontext_t*);
extern	void		makecontext(ucontext_t*, void(*)(), int, ...);
#endif

#if defined(__APPLE__)
#	define mcontext libthread_mcontext
#	define mcontext_t libthread_mcontext_t
#	define ucontext libthread_ucontext
#	define ucontext_t libthread_ucontext_t
#	if defined(__i386__)
#		include "386-ucontext.h"
#	elif defined(__x86_64__)
#		include "amd64-ucontext.h"
#	else
#		include "power-ucontext.h"
#	endif	
#endif

#if defined(__OpenBSD__)
#	define mcontext libthread_mcontext
#	define mcontext_t libthread_mcontext_t
#	define ucontext libthread_ucontext
#	define ucontext_t libthread_ucontext_t
#	if defined __i386__
#		include "386-ucontext.h"
#	else
#		include "power-ucontext.h"
#	endif
extern pid_t rfork_thread(int, void*, int(*)(void*), void*);
#endif

#if 0 &&  defined(__sun__)
#	define mcontext libthread_mcontext
#	define mcontext_t libthread_mcontext_t
#	define ucontext libthread_ucontext
#	define ucontext_t libthread_ucontext_t
#	include "sparc-ucontext.h"
#endif

#if defined(__arm__)
int getmcontext(mcontext_t*);
void setmcontext(const mcontext_t*);
#if 0//USE_UCONTEXT
#define	setcontext(u)	setmcontext((const struct mcontext_t *)&(u)->uc_mcontext.arm_r0)
#define	getcontext(u)	getmcontext((struct mcontext_t *)&(u)->uc_mcontext.arm_r0)
#else
#define	setcontext(u)	setmcontext(&(u)->uc_mcontext)
#define	getcontext(u)	getmcontext(&(u)->uc_mcontext)
#endif
#endif

#if defined(__mips__)
#include "mips-ucontext.h"
int getmcontext(mcontext_t*);
void setmcontext(const mcontext_t*);
#define	setcontext(u)	setmcontext(&(u)->uc_mcontext)
#define	getcontext(u)	getmcontext(&(u)->uc_mcontext)
#endif


#if USE_LIBTASK_LITE
#ifdef __cplusplus
extern "C" {
#endif

typedef struct TaskCexception {
  //enum exception_flavor flavor;
  int flavor;
#define NO_INFO_EXCEPT  0
#if !NO_INFO_EXCEPT
  const char *msg;
  union {
    int oops;
    long screwup;
    long segvexcept;
    char barf[8];
  } info;
#endif
}TaskCexception_t;

struct _baseTaskCtx{
        /*for ucontext*/
#ifdef WIN32
        LPVOID task_handle;
#else
        ucontext_t /*main_uc, */task_uc;//for caller & callee
        ucontext_t *task_ucp;
#endif
        /*exception_context part*/
        int caught;
        volatile struct _TaskETYPE_ { TaskCexception_t etmp; } v; 

        /*for return value*/
        int bCompleted,ret;int bIsTimeout;
        /*for callback*/
	task_callback_t callback;void* callback_ctx;
	/*for initial args*/
	int args[16];
	/*for stack*/
	int stack[1];
};
typedef  struct _baseTaskCtx baseTaskCtx;

/////////////////////Exception macro for task
#define TaskTry	\
			{	\
				ucontext_t *task_ucp_prev,task_uc_env; \
				task_ucp_prev=cur_task_ctx->task_ucp; \
				cur_task_ctx->task_ucp=&task_uc_env; \
				if(getcontext(cur_task_ctx->task_ucp)==0){ \
					do
#define TaskException__catch(action) \
					while(cur_task_ctx->caught=0, cur_task_ctx->caught); \
				}	\
				else{ cur_task_ctx->caught=1; } \
				cur_task_ctx->task_ucp=task_ucp_prev; \
			} \
			if (!cur_task_ctx->caught || action) { } \
			else
#define TaskCatch(e) TaskException__catch(((e) =*(Cexception_t*)&(cur_task_ctx->v.etmp), 0))
#define TaskCatch_anonymous TaskException__catch(0)
#define TaskThrow \
  for (;; setcontext(cur_task_ctx->task_ucp)) \
    cur_task_ctx->v.etmp =


#ifdef __cplusplus
}
#endif
#endif


#if USE_LIBTASK
typedef struct Context Context;

enum
{
	STACK = 8192
};

struct Context
{
	ucontext_t	uc;
};

struct Task
{
	char	name[256];	// offset known to acid
	char	state[256];
	Task	*next;
	Task	*prev;
	Task	*allnext;
	Task	*allprev;
	Context	context;
	uvlong	alarmtime;
	uint	id;
	uchar	*stk;
	uint	stksize;
	int	exiting;
	int	alltaskslot;
	int	system;
	int	ready;
	void	(*startfn)(void*);
	void	*startarg;
	void	*udata;
};

void	taskready(Task*);
void	taskswitch(void);

void	addtask(Tasklist*, Task*);
void	deltask(Tasklist*, Task*);

extern Task	*taskrunning;
extern int	 taskcount;
#endif

#endif /*__TASKIMPL_H__*/