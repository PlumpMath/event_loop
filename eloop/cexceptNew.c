#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "cexcept.h"
#if 1//ndef WIN32
#ifdef _WIN32
typedef DWORD pthread_t;
//static CRITICAL_SECTION global_log_file_lock;
static pthread_t pthread_self(void) {
  return GetCurrentThreadId();
}
static int pthread_equal(pthread_t tid1,pthread_t tid2){return tid1==tid2;}
#endif // _WIN32

struct thread_state g_state[MAX_THREAD];
struct thread_state* allocstate(long int pid)
{
  int i;

  for(i=0;i<MAX_THREAD;i++)
    if(g_state[i].pid ==  0){
      g_state[i].pid=pid;
//      strcpy(g_state[i].name, t_name);
      //fprintf(stderr,"state%d alloc to pid0x%x\n",i,(unsigned int)pid);
      return (struct thread_state*)&g_state[i];
    }
  fprintf(stderr,"alloc fail...!!!! pid%d not allocation!!!!!!!!!!!!!!!!!!!\n", (unsigned int)pid);
  return NULL;
}
struct thread_state* findstate(long int pid,const char* dbgStr)
{
  int i;

  for(i=0;i<MAX_THREAD;i++)
    if(equal_tid((pthread_t)g_state[i].pid,(pthread_t)pid)){
//	fprintf(stderr,"state%d found for pid%d\n",i,pid);
      return (struct thread_state*)&g_state[i];
    }
  fprintf(stderr,"pid%d is not valid on exception state(%s) !!\n",(unsigned int)pid,dbgStr);
  return NULL;
}
void freestate(long int pid)
{
  int i;

  for(i=0;i<MAX_THREAD;i++)
    if(equal_tid((pthread_t)g_state[i].pid,(pthread_t)pid)){
      //fprintf(stderr,"state%d free from pid0x%x\n",i,(unsigned int)pid);
      g_state[i].pid=0;
      break;
    }

  if(i == MAX_THREAD) fprintf(stderr,"free fail...!!!!! pid 0x%x\n", (unsigned int)pid);
}
#endif

#ifndef WIN32
#if HYW_PATCH_1
void wdt_TimerStop(void);
#endif
/*
SIGNAL
 

설명
 
리눅스는 아래 나열된 시그널을 지원한다. 몇몇 개의 시그널 번호는 아키텍쳐 의존적이다. 먼저 POSIX.1에서 설명하는 시그널이다.
  




시그널 


번호 


행동 


설명 




SIGINT 


 2 


A 


키보드로부터의 인터럽트(interrupt) 시그널 




SIGQUIT 


 3 


C 


키보드로부터의 종료(quit) 시그널 




SIGILL 


 4 


C 


잘못된 명령어(Illegal Instruction) 




SIGABRT 


 6 


C 


abort(3)로부터의 중단(abort) 시그널 




SIGFPE 


 8 


C 


부동 소수점 예외(exception) 




SIGKILL 


 9 


AEF 


kill 시그널 




SIGSEGV 


11 


C 


잘못된 메모리 참조 




SIGPIPE 


13 


A 


깨진 파이프: 수신자가 없는 파이프에 쓰기 




SIGALRM 


14 


A 


alarm(2)으로부터의 타이머 시그널 




SIGTERM 


15 


A 


종료(termination) 시그널 




SIGUSR1 


30,10,16 


A 


사용자 정의 시그널 1 




SIGUSR2 


31,12,17 


A 


사용자 정의 시그널 2 




SIGCHLD 


20,17,18 


B 


자식 프로세스가 중단 또는 종료 




SIGCONT 


19,18,25 


  


중단되었다면 재개(continue) 




SIGSTOP 


17,19,23 


DEF 


프로세스 중단 




SIGTSTP 


18,20,24 


D 


터미널에서의 중단 시그널 




SIGTTIN 


21,21,26 


D 


백그라운드 프로세스에 대한 터미널 입력 




SIGTTOU 


22,22,27 


D 


백그라운드 프로세스에 대한 터미널 출력 

 
 
  


다음은 POSIX.1의 시그널은 아니지만 SUSv2에서 설명하고 있는 것이다. 




시그널 


번호 


행동 


설명 




SIGPOLL 


  


A 


폴링(poll) 이벤트 (Sys V). SIGIO와 같다. 




SIGPROF 


27,27,29 


A 


프로파일링(profiling) 타이머 시그널 




SIGSYS 


12,-,12 


C 


루틴에 잘못된 인자 (SVID) 




SIGTRAP 


5 


C 


trace/breakpoint 트랩 




SIGURG 


16,23,21 


B 


소켓에 대한 긴급(urgent) 상황 (4.2 BSD) 




SIGVTALRM 


26,26,28 


  


가상 알람 클럭 (4.2 BSD) 




SIGXCPU 


24,24,30 


C 


CPU 시간 제한 초과 (4.2 BSD) 




SIGXFSZ 


25,25,31 


C 


파일 크기 제한 초과 (4.2 BSD) 


(SIGSYS, SIGXCPU, SIGXFSZ와 몇몇 아키텍쳐에서는 SIGBUS의 기본 행동은 SUSv2에서 C(종료와 코어 덤프)로 나와있지만 현재 리눅스(2.3.27)에서 까지는 A(종료)이다) 

 
 
다음은 여러 가지 다른 시그널. 




시그널 


번호 


행동 


설명 




SIGEMT 


7,-,7 


  


  




SIGSTKFLT 


-,16,- 


  


보조프로세서의 스택 오류 




SIGIO 


23,29,22 


A 


현재 I/O가 가능 (4.2 BSD) 




SIGCLD 


-,-,18 


  


SIGCHLD와 같다. 




SIGPWR 


29,30,19 


A 


전원 문제 (System V) 




SIGINFO 


29,-,- 


  


SIGPWR와 같다. 




SIGLOST 


-,-,- 


A 


파일 락(lock) 손실 




SIGWINCH 


28,28,20 


B 


윈도우 크기 변경 시그널 (4.3 BSD, Sun) 




SIGUNUSED 


-,31,- 


A 


사용되지 않는 시그널 (SIGSYS가될 것이다) 


(여기서 -는 시그널이 없음을 나타낸다; 세 가지 값이 있다. 첫번째 것은 대개 alpha와 sparc에서, 중간의 것은 i386, ppc와 sh에서, 마지막 것은 mips에서 유효한 값이다. 29번 시그널은 alpha에서는 SIGINFO / SIGPWR이지만 sparc에서는 SIGLOST이다.) 

"행동" 컬럼의 문자는 다음과 같은 의미이다: 

A           기본 행동이 프로세스를 종료하는 것이다. 

B           기본 행동이 시그널을 무시하는 것이다. 

C           기본 행동이 프로세스를 종료하고 코어를 덤프한다. 

D           기본 행동이 프로세스를 멈추는 것이다. 

E           핸들러를 둘 수 없는 시그널이다. 

F           무시할 수 없는 시그널이다. 




호환 

POSIX.1   



버그 : SIGIO와 SIGLOST는 같은 값을 갖는다. 후자는 커널 소스에서 주석 처리되었지만, 몇몇 소프트웨어의 프로세스는 여전히 29번 시그널을 SIGLOST로 생각한다.



*/
const char* signum2str(int signum)
{
	switch(signum){
		case SIGUSR1:return "SIGUSR1";
		case SIGUSR2:return "SIGUSR2";
		case SIGILL: return "SIGILL";
		case SIGSEGV:return "SIGSEGV";
		case SIGINT:return "SIGINT";
		case SIGQUIT: return "SIGQUIT";
		case SIGABRT: return "SIGABRT";
		case SIGPIPE: return "SIGPIPE";
		case SIGPWR: return "SIGPWR";
#ifndef __CYGWIN__
		case SIGSTKFLT: return "SIGSTKFLT";
#endif
		case SIGFPE: return "SIGFPE";
		default: return "UnKnown";
	}
}
/*********SAMPLE-HANDLER********************/
void sighandler(int signum)//ywhwang patch
{
  FIND_THREAD_EXCEPT_CONTEXT(e,"SIGHANDLER");

	if(!state){fprintf(stderr,"pid%d has no exception state!!! returing to OS\n",(int)___tid);return;}
	if(!the_exception_context){fprintf(stderr,"pid%d has no the_exception_context!!! returing to OS\n",(int)___tid);return;}
#if HYW_PATCH_1
  fprintf(stderr,"HYWDEBUG: signum is %d!!!!!\n",signum);
  if(signum==SIGHUP)//ywhwang add for Developer
  {
	  printf("killed by DEV-USER\n");
	  wdt_TimerStop();
	  exit(0);
	  return;
  }
#endif
//  if(signum==SIGSEGV){
    fprintf(stderr,"pid(%d):sending exception to thread on sighandler(%ld) signum(%d=%s)!!\n",(unsigned int)___tid,(long int)state->arg, signum,signum2str(signum));
    //if(signum==SIGSTOP){SIG_DFL;return;}
    {
/*
      FILE* fp=fopen("/mnt/contents/exceptions.log","a");
      if(fp){
      fprintf(fp,"pid(%d):sending exception to thread on sighandler(%ld) signum(%d)!!\n",tid,(int)state->arg, signum);
      fclose(fp);
      }
*/
    /*********convert to highlevel exception********************/
    e.flavor = signum;
#if !NO_INFO_EXCEPT
    e.msg = "demo segvexcept message";
    e.info.segvexcept = 12345678+(long int)state->arg*100;// this is just test-data!!
#endif
    Throw e;
/*********convert to highlevel exception********************/

    fprintf(stderr,"after throw\n");
    }
//  }
}

void SetTID(char *name, long int tid)
{
	/*
      FILE* fp=fopen("/mnt/contents/exceptions.log","a");
      
	printf("%s =>  pid0(%x)----\n",name, tid);
      if(fp){
      	fprintf(fp,"%s =>  pid0(%x)----\n",name, tid);
      	fclose(fp);
      }
      */
}

void init_except_handler()
{
sigset_t newmask, oldmask;

	if(errno){perror("cexceptNew:+init_except_handler");fprintf(stderr,"errno detected(errno%d)!!\n",errno);fflush(stderr);errno=0;/*auto clear error_no*/}
  signal(SIGILL,sighandler);//ywhwang patch
  signal(SIGTRAP,sighandler);//ywhwang patch
  signal(SIGABRT,sighandler);//ywhwang patch
  signal(SIGFPE,sighandler);//ywhwang patch
  signal(SIGPIPE,sighandler);//ywhwang patch
	if(errno){perror("cexceptNew:>init_except_handler");fprintf(stderr,"errno detected(errno%d)!!\n",errno);fflush(stderr);errno=0;/*auto clear error_no*/}
//signal(SIGCHLD,sighandler);//ywhwang patch
  //signal(SIGCONT,sighandler);//ywhwang patch/* Continue (POSIX).  */
  //signal(SIGSTOP,sighandler);//ywhwang patch// cannot be caught!!
  //signal(SIGTSTP,sighandler);//ywhwang patch/* Keyboard stop (POSIX).  */
	if(errno){perror("cexceptNew:+init_except_handler");fprintf(stderr,"errno detected(errno%d)!!\n",errno);fflush(stderr);errno=0;/*auto clear error_no*/}
  signal(SIGTTIN,sighandler);//ywhwang patch
  signal(SIGTTOU,sighandler);//ywhwang patch
  signal(SIGSEGV,sighandler);//ywhwang patch

  
	if(errno){perror("cexceptNew:=init_except_handler");fprintf(stderr,"errno detected(errno%d)!!\n",errno);fflush(stderr);errno=0;/*auto clear error_no*/}
#if 0   //for eloop!!
  signal(SIGALARM,sighandler);//ywhwang patch
  signal(SIGTERM,sighandler);//ywhwang patch
  signal(SIGKILL,sighandler);//ywhwang patch
  signal(SIGHUP,sighandler);//ywhwang patch
  signal(SIGUSR1,sighandler);//ywhwang patch
  signal(SIGUSR2,sighandler);//ywhwang patch
#endif

   sigemptyset(&newmask);
    sigaddset(&newmask, SIGILL);
    sigaddset(&newmask, SIGTRAP);
    sigaddset(&newmask, SIGABRT);
    sigaddset(&newmask, SIGFPE);
    sigaddset(&newmask, SIGPIPE);
    //sigaddset(&newmask, SIGCONT);/* Continue (POSIX).  */
    sigaddset(&newmask, SIGSTOP);// cannot be caught!!
    //sigaddset(&newmask, SIGTSTP);/* Keyboard stop (POSIX).  */
    sigaddset(&newmask, SIGTTIN);
    sigaddset(&newmask, SIGTTOU);
    sigaddset(&newmask, SIGSEGV);
    //sigaddset(&newmask, SIGCHLD);
#if 1   //for eloop!!
    //sigaddset(&newmask, SIGALARM);
    sigaddset(&newmask, SIGTERM);
    sigaddset(&newmask, SIGKILL);
    sigaddset(&newmask, SIGHUP);
    sigaddset(&newmask, SIGUSR1);
    sigaddset(&newmask, SIGUSR2);
#endif	
    if (sigprocmask(SIG_UNBLOCK, &newmask, &oldmask) < 0)
    {
        perror("sigmask error : ");
        exit(0);
    }
  memset(g_state,0,sizeof(g_state));
	if(errno){perror("cexceptNew:-init_except_handler");fprintf(stderr,"errno detected(errno%d)!!\n",errno);fflush(stderr);errno=0;/*auto clear error_no*/}
}

#if HYW_PATCH_2
size_t os_strlcpy(char *dest, const char *src, size_t siz)
{
	const char *s = src;
	size_t left = siz;

	if (left) {
		/* Copy string up to the maximum size of the dest buffer */
		while (--left != 0) {
			if ((*dest++ = *s++) == '\0')
				break;
		}
	}

	if (left == 0) {
		/* Not enough room for the string; force NUL-termination */
		if (siz != 0)
			*dest = '\0';
		while (*s++)
			; /* determine total src string length */
	}

	return s - src - 1;
}
#endif

#endif

#if 1	//test exception!!
void test_exception(void)
{
  FIND_THREAD_EXCEPT_CONTEXT(e,"test_exception");

  fprintf(stderr,"test_exception start\n");
  printf("test_exception start\n");
	Try{
	    e.flavor = 1234567;
#if !NO_INFO_EXCEPT
		e.msg = "demo barf message";
		strcpy(e.info.barf,"1234567");
#endif
		Throw e;
	}
	Catch(e){
		fprintf(stderr,"flavor=%d,msg=%s\n",e.flavor,e.msg);
		printf("flavor=%d,msg=%s\n",e.flavor,e.msg);
	}
  fprintf(stderr,"test_exception end\n");
  printf("test_exception end\n");
}
#endif
///exception handling과 task_sched 통합.
//wait시에 저장된 context로 내부 블록에서는 나중에 다시 self throw시에 돌아갈 수 있다.이때 user_defined exception이 발생한다.
//... wait API에 사용된 API의 argument에 invalid한 경우엔 API exception일 발생할 수 있다.
//또한 timeout시에도 default_loop로부터의 self throw를 통해 돌아가면서 timeout exception을 발생시킬수 있다.
//		(그러나 context가 taskCtx에 하나만 존재하므로 nested structured exception handling은 할 수 없다.)
//		(이를 보완하려면 taskCtx에 linkedList형태로 context정보가 저장되어야한다.wait시에 malloc한 메모리에 저장하고,wait에서 빠져나올떄 free하고 복구하면된다.)
//정상적인 message/event발생시에 return value로 처리된다.
