#ifdef _WIN32
#include <windows.h>
#include <winsvc.h>
#include <shlobj.h>
#define PATH_MAX MAX_PATH
#define S_ISDIR(x) ((x) & _S_IFDIR)
#define DIRSEP '\\'
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define sleep(x) Sleep((x) * 1000)
#define WINCDECL __cdecl
#else
#include <sys/wait.h>
#include <unistd.h>
#define DIRSEP '/'
#define WINCDECL
#endif // _WIN32


#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//////////////////////////////

#define USE_DEFAULT_FORK 0	// 1:bDoubleFork 

#undef USE_REDIRECT_SUBPROC_ERRLOG_FILENAME 
#ifdef __CYGWIN__
#define USE_REDIRECT_SUBPROC_LOG_FILENAME "cyg_emsSubOut_and_Err.log"
#else
#define USE_REDIRECT_SUBPROC_LOG_FILENAME "emsSubOut_and_Err.log"
#endif

#define MY_DUP2(oldfd,newfd) (close(newfd),dup2(oldfd,newfd))
int redirect_fd(int fd,const char* filename)
{
	int i,fdout;

	 i=open(filename, O_RDWR|O_APPEND);// prefer append!!
	 if(i<0)i=open(filename, O_RDWR|O_CREAT);// next is create!!
	 if(i<0){
	 	perror("redirect_fd failed");
		fprintf(stderr,"redirect(%s) for fd=%d failed!!\n",filename,fd);
		fflush(stderr);
	 	return -1;
	 }
	 fprintf(stderr,"redirect(%s) for fd=%d take(new fd=%d)!!\n",filename,fd,i);//fd=2 must be last_redirection!!
	 fflush(stdout);fflush(stderr);
	 fdout=MY_DUP2(i,fd);
	 if(fd==1)fprintf(stdout,"redirect(%s) for fd=%d(new fd=%d) started!!\n",filename,fd,i);
	 else if(fd==2)fprintf(stderr,"redirect(%s) for fd=%d(new fd=%d) started!!\n",filename,fd,i);
	 else fprintf(stderr,"redirect(%s) for fd=%d(new fd=%d) started!!\n",filename,fd,i);
	 close(i);
	 return fdout;
}

int safe_fork(const char* pidFileName,int bDoubleFork) // Use double forks. Have your children immediately fork another copy and have the original child process exit.
			     // This is simpler than using signals, in my opinion, and more understandable.
{
  int status=0;
  pid_t pid;
  
  if (!bDoubleFork || !(pid=fork())) {
  	pid_t pidC;

    if (!(pidC=fork())) {
      /* this is the child that keeps going */
      // do_something(); /* or exec */
      return 0;
    } else {
      /* the first child process exits */
      FILE* fp=fopen(pidFileName,"w");
      if(fp){fprintf(fp,"%d\n",pidC);fclose(fp);}
      if(bDoubleFork)exit(0);
      else pid=pidC;
    }
  } else {
    /* this is the original process */  
    /* wait for the first child to exit which it will immediately */
    waitpid(pid,&status,0);// WUNTRACED | WCONTINUED
  }
  return pid;
}
int run_sub_process(char* runStr,const char* pidFile,const char* outputFileName)
{
	pid_t pid;

	printf("run_sub_process:'%s',pidFile='%s'\n",runStr,pidFile);
	pid = safe_fork(pidFile,USE_DEFAULT_FORK);

	if(pid == -1)
	{
		printf("fork() error!\n");
		return -1;
	}
	else if(pid == 0)
	{
		printf("'%s' process start(pid=%d)!\n",runStr,getpid());
		char filepath[PATH_MAX];
		char* argv[256]={NULL,};int i;

		printf("args:");
		for(i=0;i<256;i++){
			char* ptr;
			ptr=strstr(runStr," ");
			if(ptr)*ptr='\0';//cut string
			argv[i]=runStr;
			printf(" %s",argv[i]);
			if(!ptr){
				argv[i+1]=NULL;
				break;
			}
			runStr=ptr+1;
		}
		printf("\n");
		if(argv[0][0]=='/')strcpy(filepath,strrchr(argv[0],'/')+1);//last '/'
		else strcpy(filepath,argv[0]);
		printf("file=%s\n",filepath);
		redirect_fd(0,"/dev/null");  // Redirect stdin
#ifdef USE_REDIRECT_SUBPROC_LOG_FILENAME		 
		if(strcmp(outputFileName,"stdout"))redirect_fd(1,outputFileName? outputFileName:USE_REDIRECT_SUBPROC_LOG_FILENAME);
#ifdef USE_REDIRECT_SUBPROC_ERRLOG_FILENAME
		if(strcmp(USE_REDIRECT_SUBPROC_ERRLOG_FILENAME,"stderr"))redirect_fd(2,USE_REDIRECT_SUBPROC_ERRLOG_FILENAME);
#else
		if(strcmp(outputFileName,"stdout")){
			fprintf(stderr,"redirect fd1 -> fd2 take.\n");fflush(stderr);MY_DUP2(1,2); // 2>&1
			fprintf(stderr,"redirect fd1 -> fd2 started.\n");fflush(stderr);
		}
#endif
#endif
		execvp(filepath, argv);
		// sleep(1);
		unlink(pidFile);
		fprintf(stderr,"CANNOT execute EMS_child(%s)!!!\n",argv[0]);//The exec() functions only return if an error has occurred. The return value is -1, and errno is set to indicate the error.
		exit(-1);
		return -2;
	}
	if(!USE_DEFAULT_FORK){
		int status=-999998;FILE* fp=NULL;int proc1pid=-999997;
		sleep(1);
#ifdef __CYGWIN__
#else
		waitpid(pid,&status,WNOWAIT);
		printf("pid%d: status=0x%x\n",pid,(unsigned int)status);
#endif		
		fp=fopen(pidFile,"r");
		if(fp){
			fscanf(fp,"%d",&proc1pid);
			fclose(fp);
		}
		if(proc1pid==-999997&&status==-999998){
			fprintf(stderr,"CANNOT execute EMS_child('%s')!!!\n",runStr);
			return -3;
		}
	}
	return 0;
}
int kill_sub_process(const char* pidFile)
{
	int proc1pid=-1;
	FILE* fp=NULL;

	fp=fopen(pidFile,"r");
	if(fp){
		fscanf(fp,"%d",&proc1pid);
		fclose(fp);
		unlink(pidFile);
	}
	else errno=0;
	if(proc1pid>=0)kill(proc1pid,SIGKILL);// SIGTERM
	return 0;
}
int wait_sub_process(const char* pidFile)
{
	int status=-1;
	int proc1pid=-1;
	FILE* fp=NULL;

	fp=fopen(pidFile,"r");
	if(fp){
		fscanf(fp,"%d",&proc1pid);
		fclose(fp);
		unlink(pidFile);
	}
	else errno=0;
	if(proc1pid>=0){
		waitpid(proc1pid,&status,0);
		fprintf(stderr,"wait_sub_process(%d) EMS_child:0x%x!!!\n",proc1pid,status);
	}
	return status;
}
int check_sub_process(const char* pidFile)
{
	int status=-1;
	int proc1pid=-1;
	FILE* fp=NULL;

	fp=fopen(pidFile,"r");
	if(fp){
		fscanf(fp,"%d",&proc1pid);
		fclose(fp);
	}
	else errno=0;
	if(proc1pid>=0){
#ifdef __CYGWIN__
#else
		waitpid(proc1pid,&status,WNOWAIT);
		fprintf(stderr,"check_sub_process(%d) EMS_child:0x%x!!!\n",proc1pid,status);
#endif		
	}
	return status;
}

#ifndef _WIN32
int systemGetPipeStr(const char *cmd, char *pRetBuf)
{
	char buf[1024];
	FILE *ptr;

	memset(buf, 0, sizeof(buf));

	ptr = popen(cmd, "r");
	if(ptr != NULL)
	{
		while(fgets(buf, 1024, ptr) != NULL)
		{
			//			if(buf[strlen(buf) - 1] == 0x0a)
			//				buf[strlen(buf) - 1] = 0;
			strcat(pRetBuf, buf);
			memset(buf, 0, sizeof(buf));
		}

		(void)pclose(ptr);
	}
	else
	{
		return -1;
	}

	return 0;
}
#endif

#if 0
/** Redirect stdout and stderr. Returns 0 upon success. **/
static int redirect_std_out_err(const char *log_name) {
  int failure = 1;

  /* Open log FILE stream. */
  FILE *const fp = fopen(log_name, "wb");
  if (fp != NULL) {

    /* Extract log FILE fd, and standard stdout/stderr ones. */
    const int log_fd = fileno(fp);
    const int stdout_fd = fileno(stdout);
    const int stderr_fd = fileno(stderr);
    if (log_fd != -1 && stdout_fd != -1 && stderr_fd != -1) {

      /* Clone log fd to standard stdout/stderr ones. */
      if (dup2(log_fd, stdout_fd) != -1 && dup2(log_fd, stderr_fd) != -1) {
        failure = 0;
      }
    }

    /* Close initial file. We do not need it, as we cloned it. */
    fclose(fp);
  }
  return failure;
}


#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

void run_cmd(char *cmd)
{
    pid_t pid;
    char *argv[] = {"sh", "-c", cmd, NULL};
    int status;
    printf("Run command: %s\n", cmd);
    status = posix_spawn(&pid, "/bin/sh", NULL, NULL, argv, environ);
    if (status == 0) {
        printf("Child pid: %i\n", pid);
        if (waitpid(pid, &status, 0) != -1) {
            printf("Child exited with status %i\n", status);
        } else {
            perror("waitpid");
        }
    } else {
        printf("posix_spawn: %s\n", strerror(status));
    }
}

void test_fork_exec(void) {
  pid_t pid;
  int status;
  puts("Testing fork/exec");
  fflush(NULL);
  pid = fork();
  switch (pid) {
  case -1:
    perror("fork");
    break;
  case 0:
    execl("/bin/ls", "ls", (char *) 0);
    perror("exec");
    break;
  default:
    printf("Child id: %i\n", pid);
    fflush(NULL);
    if (waitpid(pid, &status, 0) != -1) {
      printf("Child exited with status %i\n", status);
    } else {
      perror("waitpid");
    }
    break;
  }
}

void test_posix_spawn(void) {
  pid_t pid;
  char *argv[] = {"ls", (char *) 0};
  int status;
  puts("Testing posix_spawn");
  fflush(NULL);
  status = posix_spawn(&pid, "/bin/ls", NULL, NULL, argv, environ);
  if (status == 0) {
    printf("Child id: %i\n", pid);
    fflush(NULL);
    if (waitpid(pid, &status, 0) != -1) {
      printf("Child exited with status %i\n", status);
    } else {
      perror("waitpid");
    }
  } else {
    printf("posix_spawn: %s\n", strerror(status));
  }
}
#endif
