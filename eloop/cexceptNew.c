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
 

����
 
�������� �Ʒ� ������ �ñ׳��� �����Ѵ�. ��� ���� �ñ׳� ��ȣ�� ��Ű���� �������̴�. ���� POSIX.1���� �����ϴ� �ñ׳��̴�.
  




�ñ׳� 


��ȣ 


�ൿ 


���� 




SIGINT 


 2 


A 


Ű����κ����� ���ͷ�Ʈ(interrupt) �ñ׳� 




SIGQUIT 


 3 


C 


Ű����κ����� ����(quit) �ñ׳� 




SIGILL 


 4 


C 


�߸��� ��ɾ�(Illegal Instruction) 




SIGABRT 


 6 


C 


abort(3)�κ����� �ߴ�(abort) �ñ׳� 




SIGFPE 


 8 


C 


�ε� �Ҽ��� ����(exception) 




SIGKILL 


 9 


AEF 


kill �ñ׳� 




SIGSEGV 


11 


C 


�߸��� �޸� ���� 




SIGPIPE 


13 


A 


���� ������: �����ڰ� ���� �������� ���� 




SIGALRM 


14 


A 


alarm(2)���κ����� Ÿ�̸� �ñ׳� 




SIGTERM 


15 


A 


����(termination) �ñ׳� 




SIGUSR1 


30,10,16 


A 


����� ���� �ñ׳� 1 




SIGUSR2 


31,12,17 


A 


����� ���� �ñ׳� 2 




SIGCHLD 


20,17,18 


B 


�ڽ� ���μ����� �ߴ� �Ǵ� ���� 




SIGCONT 


19,18,25 


  


�ߴܵǾ��ٸ� �簳(continue) 




SIGSTOP 


17,19,23 


DEF 


���μ��� �ߴ� 




SIGTSTP 


18,20,24 


D 


�͹̳ο����� �ߴ� �ñ׳� 




SIGTTIN 


21,21,26 


D 


��׶��� ���μ����� ���� �͹̳� �Է� 




SIGTTOU 


22,22,27 


D 


��׶��� ���μ����� ���� �͹̳� ��� 

 
 
  


������ POSIX.1�� �ñ׳��� �ƴ����� SUSv2���� �����ϰ� �ִ� ���̴�. 




�ñ׳� 


��ȣ 


�ൿ 


���� 




SIGPOLL 


  


A 


����(poll) �̺�Ʈ (Sys V). SIGIO�� ����. 




SIGPROF 


27,27,29 


A 


�������ϸ�(profiling) Ÿ�̸� �ñ׳� 




SIGSYS 


12,-,12 


C 


��ƾ�� �߸��� ���� (SVID) 




SIGTRAP 


5 


C 


trace/breakpoint Ʈ�� 




SIGURG 


16,23,21 


B 


���Ͽ� ���� ���(urgent) ��Ȳ (4.2 BSD) 




SIGVTALRM 


26,26,28 


  


���� �˶� Ŭ�� (4.2 BSD) 




SIGXCPU 


24,24,30 


C 


CPU �ð� ���� �ʰ� (4.2 BSD) 




SIGXFSZ 


25,25,31 


C 


���� ũ�� ���� �ʰ� (4.2 BSD) 


(SIGSYS, SIGXCPU, SIGXFSZ�� ��� ��Ű���Ŀ����� SIGBUS�� �⺻ �ൿ�� SUSv2���� C(����� �ھ� ����)�� ���������� ���� ������(2.3.27)���� ������ A(����)�̴�) 

 
 
������ ���� ���� �ٸ� �ñ׳�. 




�ñ׳� 


��ȣ 


�ൿ 


���� 




SIGEMT 


7,-,7 


  


  




SIGSTKFLT 


-,16,- 


  


�������μ����� ���� ���� 




SIGIO 


23,29,22 


A 


���� I/O�� ���� (4.2 BSD) 




SIGCLD 


-,-,18 


  


SIGCHLD�� ����. 




SIGPWR 


29,30,19 


A 


���� ���� (System V) 




SIGINFO 


29,-,- 


  


SIGPWR�� ����. 




SIGLOST 


-,-,- 


A 


���� ��(lock) �ս� 




SIGWINCH 


28,28,20 


B 


������ ũ�� ���� �ñ׳� (4.3 BSD, Sun) 




SIGUNUSED 


-,31,- 


A 


������ �ʴ� �ñ׳� (SIGSYS���� ���̴�) 


(���⼭ -�� �ñ׳��� ������ ��Ÿ����; �� ���� ���� �ִ�. ù��° ���� �밳 alpha�� sparc����, �߰��� ���� i386, ppc�� sh����, ������ ���� mips���� ��ȿ�� ���̴�. 29�� �ñ׳��� alpha������ SIGINFO / SIGPWR������ sparc������ SIGLOST�̴�.) 

"�ൿ" �÷��� ���ڴ� ������ ���� �ǹ��̴�: 

A           �⺻ �ൿ�� ���μ����� �����ϴ� ���̴�. 

B           �⺻ �ൿ�� �ñ׳��� �����ϴ� ���̴�. 

C           �⺻ �ൿ�� ���μ����� �����ϰ� �ھ �����Ѵ�. 

D           �⺻ �ൿ�� ���μ����� ���ߴ� ���̴�. 

E           �ڵ鷯�� �� �� ���� �ñ׳��̴�. 

F           ������ �� ���� �ñ׳��̴�. 




ȣȯ 

POSIX.1   



���� : SIGIO�� SIGLOST�� ���� ���� ���´�. ���ڴ� Ŀ�� �ҽ����� �ּ� ó���Ǿ�����, ��� ����Ʈ������ ���μ����� ������ 29�� �ñ׳��� SIGLOST�� �����Ѵ�.



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
///exception handling�� task_sched ����.
//wait�ÿ� ����� context�� ���� ��Ͽ����� ���߿� �ٽ� self throw�ÿ� ���ư� �� �ִ�.�̶� user_defined exception�� �߻��Ѵ�.
//... wait API�� ���� API�� argument�� invalid�� ��쿣 API exception�� �߻��� �� �ִ�.
//���� timeout�ÿ��� default_loop�κ����� self throw�� ���� ���ư��鼭 timeout exception�� �߻���ų�� �ִ�.
//		(�׷��� context�� taskCtx�� �ϳ��� �����ϹǷ� nested structured exception handling�� �� �� ����.)
//		(�̸� �����Ϸ��� taskCtx�� linkedList���·� context������ ����Ǿ���Ѵ�.wait�ÿ� malloc�� �޸𸮿� �����ϰ�,wait���� �������Ë� free�ϰ� �����ϸ�ȴ�.)
//�������� message/event�߻��ÿ� return value�� ó���ȴ�.
