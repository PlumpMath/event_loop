#if TICB_ENABLE_TIMER
#if ELOOP_MULTIDATA_PATCH
struct eloop_data;
#define ELOOP_CTX_ARG 	struct eloop_data* p_eloop_data,
#define ELOOP_CTX_ARG_ONLY struct eloop_data* p_eloop_data
#define ELOOP_CTX_PTR p_eloop_data,
#define ELOOP_CTX_PTR_ONLY p_eloop_data
#define eloop (*p_eloop_data)
#define NUM_ELOOP_MAX 32
extern struct eloop_data *p_eloopTbl[NUM_ELOOP_MAX];// 2+1+numSrv
#define ELOOP_SRV_CTX_PTR(i) p_eloopTbl[i],
#define ELOOP_SRV_CTX_PTR_ONLY(i) p_eloopTbl[i]
#else
#define ELOOP_CTX_ARG 	/*struct eloop_data* p_eloop_data,*/
#define ELOOP_CTX_ARG_ONLY void
#define ELOOP_CTX_PTR /*p_eloop_data,*/
#define ELOOP_CTX_PTR_ONLY /*p_eloop_data*/
#define ELOOP_SRV_CTX_PTR(i) /*p_eloopTbl[i],*/
#define ELOOP_SRV_CTX_PTR_ONLY(i) /*p_eloopTbl[i]*/
#endif

#define ms2eloopsecs(ms)		(ms)/1000,((ms)%1000)*1000
void init_timers(ELOOP_CTX_ARG void* user_data);
void destroy_timers(ELOOP_CTX_ARG_ONLY);
int expire_timers(ELOOP_CTX_ARG_ONLY);
unsigned int find_nearest_timeout_ms(ELOOP_CTX_ARG_ONLY);
typedef void (*eloop_signal_handler)(int sig, void *eloop_ctx,void *signal_ctx);
typedef void (*eloop_timeout_handler)(void *eloop_data, void *user_ctx);
int eloop_register_timeout(ELOOP_CTX_ARG unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data);
int eloop_cancel_timeout(ELOOP_CTX_ARG eloop_timeout_handler handler,
			 void *eloop_data, void *user_data);
int eloop_terminated(ELOOP_CTX_ARG_ONLY);
void eloop_terminate(ELOOP_CTX_ARG_ONLY);
int eloop_init(ELOOP_CTX_ARG void *user_data);
void eloop_destroy(ELOOP_CTX_ARG_ONLY);
typedef long os_time_t;
struct os_time {
	os_time_t sec;
	os_time_t usec;
};
int os_get_time(struct os_time *t);
unsigned int getcurTimeMs();
char* systemGetTimeStr(char* returnBuf);
#endif
int eloop_register_signal(ELOOP_CTX_ARG int sig, eloop_signal_handler handler,
			  void *user_data);
int eloop_register_signal_terminate(ELOOP_CTX_ARG eloop_signal_handler handler,
				    void *user_data);
int eloop_register_signal_reconfig(ELOOP_CTX_ARG eloop_signal_handler handler,
				   void *user_data);