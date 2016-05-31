#include"taskimpl.h"
#include<stdio.h>
////////////////////////////////////////////////////////////////////////////////////////

/////////////////header¿¡¼­

#define TEST_TASK_USE 0
 #define TEST_DNSTASK_USE 0
 #define SWITCH_TASK_USE 0
 #define USE_TASK_LIB (SWITCH_TASK_USE||TEST_TASK_USE||TEST_DNSTASK_USE)

 

#if SWITCH_TASK_USE
   void* swRead_task_routine(void* ctxOrg,int arg1);  
   void switchRead_task_init(int arg1);
   void switchRead_task_final(void);
   //////
   void SwitchReadRegsCallbackDynamic(void* Ctx,int devAddr,int regAddr,int regVal);
   void switchReadTimeout(int timerId);
 #endif  

 

 #if USE_TASK_LIB
// #include"taskimpl.h"

///////////////////////////////////////Eloop depdendet code: must implement///////////////
 #if !USE_LIBTASK_LITE_IMPL
 //You must implement timer
 TASK_API void defaultloop_timeout_handler_for_task(void *defaultloop_data, void *user_data)//on default ctx
 {
         return defaultloop_handler_for_task(defaultloop_data,user_data,1);
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
         return defaultloop_handler_for_task(defaultloop_data,user_data,0);
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
 #endif /*!USE_LIBTASK_LITE_IMPL*/

#endif


#if SWITCH_TASK_USE
 typedef struct _swTaskCtx{
         /*for ucontext*/
         baseTaskCtx baseCtx;

        /*for return value*/
  int ret;
         /*for callback*/
  void (*callback)(void* ctx);int param;void* This;
  int devAddr,regAddr,regVal;//SwitchReadRegsCallback
  int cur_devAddr,cur_regAddr,cur_regVal;//swWriteJob_callback
  /*for initial args*/
  int arg1,arg2,arg3,arg4,arg5;
  /*for stack*/
  int stack[1];
 }swTaskCtx,*PswTaskCtx;
 PswTaskCtx sw_ctx=NULL;
 #endif

#if TEST_TASK_USE
 typedef struct _testTaskCtx{
         /*for ucontext*/
         baseTaskCtx baseCtx;

        /*for return value*/
  int ret;
         /*for callback*/
  void (*callback)(void* ctx);int param;
  /*for initial args*/
  int arg1,arg2;
  /*for stack*/
  int stack[1];
 }testTaskCtx,*PtestTaskCtx;
 PtestTaskCtx test_ctx=NULL;
 #endif

#if USE_TASK_LIB
//extern "C"{
  TASK_API void OnExit_task_defaultloop(PbaseTaskCtx ctx)//on default ctx
  {
 #if TEST_DNSTASK_USE
 #endif
 #if TEST_TASK_USE
   if(ctx==(void*)test_ctx){
    printf("test task routine exit detected with(%d,%d,ret=%d)!!\n",test_ctx->arg1,test_ctx->arg2,test_ctx->ret);
    test_ctx=NULL;
   }
   else
 #endif
 #if SWITCH_TASK_USE
   if(ctx==(void*)sw_ctx){
    printf("swRead task routine exit detected with(%d,%d,ret=%d)!!\n",sw_ctx->arg1,sw_ctx->arg2,sw_ctx->ret);
    sw_ctx=NULL;
   }
   else
 #endif
   {
    printf("unknown task(%x) exit detected!!\n",ctx);
   }
  }
//}
#endif 

#if TEST_TASK_USE
 //extern "C"{
  void* test_callback(void* ctx,void* user_data)
  {
   PtestTaskCtx test_ctx=(PtestTaskCtx)ctx;
   printf("test_task_routine resume3rd on defaultloop(param=%d)!!\n",test_ctx->param);
   return NULL;
  }
  void* test_task_routine(void* ctxOrg,int arg1)//on task ctx
  {
   PbaseTaskCtx ctx=(PbaseTaskCtx)ctxOrg;
   PtestTaskCtx test_ctx=(PtestTaskCtx)ctx;
   int i;
   
   printf("test_task_routine start(%d,%d,ARG1=%d)!!\n",test_ctx->arg1,test_ctx->arg2,arg1);

  //do not call blocking call,but can call registerSockOrTimer to defaultloop and sched_defaultloop!!
   for(i=0;i<10;i++){
    printf("test_task_routine loop%d!!\n",i);
    sleep_task_on_defaultloop(5,0,ctx,NULL,NULL/*or timerId*/);
    printf("test_task_routine resume1St!!\n");
    
    sleep_task_on_defaultloop(3,0,ctx,NULL,NULL/*or timerId*/);
    printf("test_task_routine resume2nd!!\n");
    test_ctx->param=i;
    yield_task_to_defaultloop(ctx,test_callback,NULL/*or timerId*/);
   }
   test_ctx->ret=3;
   exit_task(ctx,NULL,NULL);
   printf("test_task_routine invalid resume!!!\n");
   return NULL;
  }
 //}

void test_task_init(void)//on task ctx
 {
  PbaseTaskCtx task_ctx=NULL;
  
   printf("task starting!!\n");
  task_ctx=(PbaseTaskCtx)make_task(test_task_routine,sizeof(testTaskCtx),2048,0);
  test_ctx=(PtestTaskCtx)task_ctx;
  test_ctx->arg1=1;test_ctx->arg2=2;
  start_task(task_ctx);
 }
 void test_task_final(void)//on task ctx
 {
  if(test_ctx){
   printf("task ending!!\n");
   destroy_task((PbaseTaskCtx)test_ctx);
   test_ctx=NULL;
  }
 }
 #endif


 
/////////////////////////
void taskTestInit(void)
{
#if USE_TASK_LIB
  defaultloop_task_init(OnExit_task_defaultloop);
#endif
#if TEST_TASK_USE
     test_task_init();
#endif
}
void taskTestFinal(void)
{
#if TEST_TASK_USE
     test_task_final();
#endif
#if USE_TASK_LIB
  defaultloop_task_final();
#endif
}