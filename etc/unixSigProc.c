#if TICB_SIGSEGV_SUPPORT		  
#ifdef __CYGWIN__
int backtrace(void **buffer, int size);
char **backtrace_symbols(void *const *buffer, int size);
void backtrace_symbols_fd(void *const *buffer, int size, int fd);
#else
#include <execinfo.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if 1
// cc -rdynamic prog.c -o prog
void myfunc3(void)
{
#if USE_RDYNAMIC
    int j, nptrs;
#define SIZE 100
    void *buffer[100];
    char **strings;

   nptrs = backtrace(buffer, SIZE);
   if(nptrs<=0)return;
   
    fprintf(stderr,"backtrace() returned %d addresses\n", nptrs);fflush(stderr);

   /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
       would produce similar output to the following: */

	if(nptrs>30)exit(-75);
   strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        return;//exit(EXIT_FAILURE);
    }

   for (j = 0; j < nptrs; j++){
        if(strings[j]){fprintf(stderr,"%s\n", strings[j]);fflush(stderr);}
   }

   free(strings);
#endif   
}
#if 0
static void   /* "static" means don't export the symbol... */
myfunc2(void)
{
    myfunc3();
}

void
myfunc(int ncalls)
{
    if (ncalls > 1)
        myfunc(ncalls - 1);
    else
        myfunc2();
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "%s num-calls\n", argv[0]);
        exit(EXIT_FAILURE);
    }

   myfunc(atoi(argv[1]));
    exit(EXIT_SUCCESS);
}
#endif
#endif
static void __cdecl signal_SEGV_handler(int sig_num) {
  // Reinstantiate signal handler
  signal(sig_num, signal_SEGV_handler);
  fprintf(stderr,"segv(%d) occurred!!\n",sig_num);fflush(stderr);
  myfunc3();
  exit(-77);
}
#endif

#if TICB_SIGKILL_SUPPORT		  
static void __cdecl signal_handler(int sig_num) {
  // Reinstantiate signal handler
  signal(sig_num, signal_handler);

#if 1
	s_signal_received = sig_num;
#endif
#ifndef _WIN32
  // Do not do the trick with ignoring SIGCHLD, cause not all OSes (e.g. QNX)
  // reap zombies if SIGCHLD is ignored. On QNX, for example, waitpid()
  // fails if SIGCHLD is ignored, making system() non-functional.
  if (sig_num == SIGCHLD) {
    do {printf("SIGCHLD waiting...\n");} while (waitpid(-1, &sig_num, WNOHANG) > 0);// on aging test, sometimes, this signal occurr!!!!
  } else
#endif
  { exit_flag = sig_num; }
}
#endif

