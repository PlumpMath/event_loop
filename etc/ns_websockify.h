#ifndef __NS_WEBSOCKIFY_H__
#define __NS_WEBSOCKIFY_H__

#ifndef NS_CTL_MSG_MESSAGE_SIZE
#define NS_CTL_MSG_MESSAGE_SIZE 32768
#endif
#ifdef NS_FOSSA_VERSION
void ns_broadcast_new(struct ns_mgr *mgr, ns_event_handler_t cb,void *data, size_t len);
#define USE_ANOTHER_RICS_SERVER 1
#else
void ns_broadcast_new(struct ns_mgr *mgr, ns_callback_t cb,void *data, size_t len);
#define USE_ANOTHER_RICS_SERVER 1
#endif
#define RICS_CONTABLE_MUTEX_LOCK 	USE_ANOTHER_RICS_SERVER


typedef void* PVOID;
typedef void* PQUEUE;
typedef void* PLIST;
typedef void* PMAP;
typedef struct ns_connection* PNC;
#ifndef MAX
# define MAX(x,y) ((x) < (y) ? (y) : (x))
#endif
#ifndef MIN
# define MIN(x,y) ((x) > (y) ? (y) : (x))
#endif
enum ricsManCP{
	RMCP_WEBSOCKIFY_WEBSOCK=0,
	RMCP_RICS_WEBSOCK=1,// a User(serial and notify)
	RMCP_RICS_SERIAL=2,// a RICS serial(A&B)
	RMCP_RICS_UDP=3,// a RICS poll
	RMCP_RICS_TCP=4,// a RICS control
	RMCP_RICS_CONNECTOR=98,// for a RICS
	RMCP_RICS_RECEIVER=99,// for IPC or MasterUDP poll
	RMCP_RICS_AJAXSOCK=100,// unused// a User(instant control)
};

/* data structure STL API */
void* alloc_ptr_queue(void);
void free_ptr_queue(void* queue);
void appendBack_ptr_queue(void* queue,void* data);
void* removeFront_ptr_queue(void* queue);
void* getFront_ptr_queue(void* queue);
void removeAny_ptr_queue(void* queue,void* data);

void* alloc_ptr_list(void);
void free_ptr_list(void* list);
void addTo_ptr_list(void* list,void* data);
int removeFrom_ptr_list(void* list,void* data);
void* getFront_ptr_list(void* list);
void* getBack_ptr_list(void* list);
int find_ptr_list(void* list,void* data);
void iterate_ptr_list(void* list,int (*conn_func)(void* list,void* conn_data,void* Ctx,void* Ctx2,void* Ctx3),void* Ctx,void* Ctx2,void* Ctx3);
void* _alloc_ptr_map(void);
void _free_ptr_map(void* map);
int _countAll_ptr_map(void* map);
void _insert_ptr_map(void* map,const char* dev_id,void* conn_data);
void _delete_ptr_map(void* map,const char* dev_id);
void* _find_ptr_map(void* map,const char* dev_id);
int _iterateAll_ptr_map(void* map,int (*conn_func)(void* map,const char* dev_id,void* conn_data,void* Ctx,void* Ctx2,void* Ctx3),void* Ctx,void* Ctx2,void* Ctx3);	

/* net-skeleton API */
volatile int g_num_ws2socAdd=0,g_num_ws2socSub=0;
int ns_setPeer(struct ns_connection *nc, const char *address) ;
int ns_connectTo(struct ns_connection *nc, const char *address) ;
int ns_sendto(struct ns_connection *conn, union socket_address *p_sa, const void *buf, size_t len);
void NS_close_conn(struct ns_connection *nc);
int ns_tcpSetKeepAlive(struct ns_connection *nc, int nKeepAlive_, int nKeepAliveIdle_, int nKeepAliveCnt_, int nKeepAliveInterval_);

size_t NS_url_encode(const char *src, size_t s_len, char *dst, size_t dst_len);
int NS_url_decode(const char *src, int src_len, char *dst, int dst_len, int is_form_url_encoded);
int parse_json(const char *s, int s_len, struct json_token *arr, int arr_len);

typedef struct _ws2tcp_conn_param{
	int cpType;
	struct ns_connection *pc;
	int numTx,numTxComp;
	int numRx,numRxComp,numRx0;
	int parentType;//0: for mg_conn, 1: TCP/UDP
	union{
		struct ns_connection *nc;
#ifdef NS_FOSSA_VERSION
		struct ns_connection *conn;
#else
		struct mg_connection *conn;
#endif
	}parent;
	char tmp[PATH_MAX];
	char target[PATH_MAX];
	int binaryCon;unsigned long logEnTime;
}ws2tcp_conn_param;

/* TDXAGW broadcast log API */
#define mg_context mg_connection
#define ns_context ns_connection
#define mg_request_info mg_connection 
typedef long ncSock_t;// same as struct ns_connection*
#define HB_POSTDATA_LEN_MAX	132036 //65536 //16384
extern int ListFromVGWEMSdb(const char* strDBname,const char* strTableName0,char* strBuf,int nBufLen);
extern void OnDeviceDeparted(long remote_ip,long remote_port,char* deviceSn);
extern void OnDeviceArrived(long remote_ip,long remote_port,char* deviceSn);
extern int broadcastLogMsgToUser(struct ns_context *ctxFrom,char* ipcLogMsg);

#if 1	// ricsLib.c
#define INCLUDE_SERIAL_CON 		1	// for serial port
#define INCLUDE_RICS_TCP_CON 		1	// for control
#define INCLUDE_RICS_UDP_CON 		1	// for PING&connection_alive
#define 	INCLUDE_RICS_UDP_MASTER 		1

#define RICS_SERIAL_REFRESH_EVERY_SEC		(15*60/3)	// 15minutes
#define RICS_UDP_PING_EVERY_SEC			60
#ifdef NS_FOSSA_VERSION
#define RICS_WEBSOCKET_OP_DATA	WEBSOCKET_OP_TEXT // WEBSOCKET_OP_BINARY // 
#else
#define RICS_WEBSOCKET_OP_DATA	WEBSOCKET_OPCODE_TEXT // WEBSOCKET_OPCODE_BINARY // 
#endif


#if INCLUDE_RICS_UDP_CON||INCLUDE_RICS_TCP_CON
#else
#define EMS_RICS_DOMAIN_NAME "/tmp/ems_rics"
extern int run_sub_process(char* runStr,const char* pidFile,const char* outputFileName);
extern int wait_sub_process(const char* pidFile);
extern int check_sub_process(const char* pidFile);
#endif
extern int insert_ricsconn(const char* conn,void* conn_data);
extern int delete_ricsconn(const char* conn);
extern void* find_ricsconn(const char* conn);
extern int iterateAll_ricsconn(int (*conn_func)(void* map,const char* dev_id,void* conn_data,void* Ctx,void* Ctx2,void* Ctx3),void* Ctx,void* Ctx2,void* Ctx3);



#include <sys/timeb.h>

#define CONNECT_STATE_TCP_SUPER_TDX		0x00007e7e/*0x7e7e7e7e*/
#define CONNECT_STATE_TCP_NORMAL_TDX	0x00008181/*0x81818181*/
#define CONNECT_STATE_TCP_AGW			0x0000e7e7

#define RICS_TCP_CONNECT_TIMETOUT_SEC	35
#define RICS_IPC_BUF_MAX	(16 * 1024)
#define TCP_MSG_HEADER_SIZE	4
#define TCP_MSG_MAX	1024
#define TCP_MSG_BUF_MAX	(TCP_MSG_MAX + TCP_MSG_HEADER_SIZE)

/* msg type */
#define CONNECT_LEVEL_RPT		0x66

#define START_UP_RPT			0x90
#define ERROR_RPT				0x91
#define CONNECTION_RPT			0x92
#define CONNECTION_ACK			0x12
#define CONNECTION_REQ			0x12

#define LINE_STATE_REQ			0x41
#define LINE_STATE_RPT			0xC1

#define POWER_OFF_REQ			0x20
#define POWER_ON_REQ			0x21
#define POWER_RESET_REQ			0x22
#define POWER_RESET2_REQ		0x23 /*"TAM16/32K RESET"*/

#define POWER_OFF_RPT			0xA0
#define POWER_ON_RPT			0xA1
#define POWER_RESET_RPT			0xA2
#define POWER_RESET2_RPT		0xA3

#define SERIAL_AB_TIMEOUT_REFRESH_REQ	0x3C
#define SERIAL_AB_TIMEOUT_REFRESH_RPT	0xBC

#define SERIAL_AB_SELECT_CONFIG_REQ	0x3D
#define SERIAL_AB_SELECT_CONFIG_RPT	0xBD
#define SERIAL_A_DATA_SE_MA		0x32
#define SERIAL_B_DATA_SE_MA		0x33
#define SERIAL_A_DATA_MA_SE		0xB2
#define SERIAL_B_DATA_MA_SE		0xB3
typedef struct _tcp_msg {
	unsigned short type;
	unsigned short msg_len;
	unsigned char msg[TCP_MSG_MAX];
} __attribute__ ((packed)) tcp_msg_t, udp_msg_t;
struct __Control_MSG_Header {
	unsigned short node_id;
	unsigned char msg_type;
	unsigned short msg_len;
} __attribute__ ((packed)) ;

struct __rics_state {
	int start_up;
	unsigned long long connection;

#if 1	// ywhwang add
	int serial_num_user;
#endif
	// serial session
	int serial_connected;

	// serial config
	unsigned int A_comm_num;
	unsigned int B_comm_num;
	unsigned int A_baudrate;	
	unsigned int B_baudrate;	

	// line_state
	unsigned int port_size;
	unsigned int ctrl_reg;
	unsigned int port_err;
	unsigned int swm_det;
	unsigned int swm_pon;

	// power control
	unsigned int pwr_req_flag;
};
enum _RxState {
	eRECV_HOME = 0,
	eRECV_HEAD,
	eRECV_DATA,
	eRECV_DONE,
	eRECV_ERROR,
};

struct __rics_msg_rx_arg {
	enum _RxState rxState;

	int rics_msg_sock;

	unsigned char cur_rx_buf[TCP_MSG_BUF_MAX];
	int cur_rx_len;

};
#define SERIAL_DATA_MAX	256
#define SERIAL_A_DATA_MA_SE		0xB2
#define SERIAL_B_DATA_MA_SE		0xB3
typedef struct {
	struct __Control_MSG_Header header;
	struct {
		unsigned char serial_data[SERIAL_DATA_MAX];
	} __attribute__ ((packed)) data;
} SE_MA_Serial_Data_RPT_t, MA_SE_Serial_Data_RPT_t;


#define DBG_FLAG_PRINT_DBG	0x00000001
#define DBG_FLAG_PRINT_INF	0x00000002
#define DBG_FLAG_PRINT_ERR	0x00000004

#define DBG_FLAG_DEFAULT	(DBG_FLAG_PRINT_ERR)

extern int g_rics_ctrl_debug_flag;
#if 1//def DEBUG
#define rprint_dbg(fmt, arg...) do{if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG){ printf("rics:dbg> "fmt, ##arg);fflush(stdout);}}while(0)
#else
#define rprint_dbg(fmt, arg...) do{}while(0)
#endif

#define rprint_rel_c(fmt, arg...) do{printf(fmt, ##arg);fflush(stdout);}while(0)
#define rprint_rel(fmt, arg...) do{struct timeb tb;ftime(&tb);printf("[%10u.%03u]"fmt, (unsigned int)tb.time, (unsigned int)tb.millitm, ##arg);fflush(stdout);}while(0)
#define rprint_inf(fmt, arg...) do{if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_INF){struct timeb tb;ftime(&tb);printf("[%10u.%03u]rics:inf>"fmt, (unsigned int)tb.time, (unsigned int)tb.millitm, ##arg);fflush(stdout);}}while(0)
#define rprint_err(fmt, arg...) do{if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_ERR){struct timeb tb;ftime(&tb);printf("[%10u.%03u]rics:err>"fmt, (unsigned int)tb.time, (unsigned int)tb.millitm, ##arg);fflush(stdout);}}while(0)
#define rprint_fatal(fmt, arg...) do{struct timeb tb;ftime(&tb);printf("[%10u.%03u]rics:FATAL ERROR(%s:%d):"fmt, (unsigned int)tb.time, (unsigned int)tb.millitm, __FILE__, __LINE__, ##arg);fflush(stdout);}while(0)

extern char *my_strlgetword(char *str, const int find_order, char *ret_buf, int ret_buf_size);
extern int buf_args(char *buf, char **argv, const char *delim);
extern int arg_debug_flag(int argc, char *argv[]);
extern int convert_ems_ipc_to_rics_ctrl_msg(unsigned char *pIpc_buf, void *pRics_msg);
extern int ipc_name_to_rics_ctrl(unsigned char *pIpc_name);
extern unsigned char baudrate_to_rics_baud_type(int baudrate);
extern int __udp_serial_timeout_refresh_req(ncSock_t udp_sock, struct sockaddr_in *pTo_addr);
extern int __udp_connection_req(ncSock_t udp_sock, struct sockaddr_in *pTo_addr);
extern int __connect_level_rpt(ncSock_t tcp_sock, int user_level);
extern int __serial_tcp_connect(ncSock_t sock, struct sockaddr_in *pAddr);// use connect!! -> do-not USE!!
extern int __common_cntl_req(ncSock_t tcp_sock, int node_id, unsigned char ctrl_type);//unused
extern int __serial_config_AB(ncSock_t tcp_sock, int node_id, int A_comm_num, int B_comm_num, int A_baudrate, int B_baudrate);//unused
extern int __pwr_cntl_req(ncSock_t tcp_sock, int node_id, unsigned char ctrl_type, int port_no);//unused
extern int __send_serial_data_rpt(ncSock_t clnt_sd, int node_id, char port_AB, char *data, int data_len);
	extern int rics_get_act_end_result(unsigned char result, char *result_str);
	extern int rics_get_rate(unsigned char type);
extern int do_msg_parsing(tcp_msg_t *pTcp_msg, char *pIpc_res_buf, int ipc_res_buf_size, struct __rics_state *pRics_state);/**/
	extern unsigned char baudrate_to_rics_baud_type(int baudrate);
	extern int ipc_name_to_rics_ctrl(unsigned char *pIpc_name);
extern int convert_ems_ipc_to_rics_ctrl_msg(unsigned char *pIpc_buf, void *pRics_msg);/**/

typedef struct _ricsConn{// RMCP_RICS_CONNECTOR
	int cpType;
	
	char device_id[PATH_MAX];
	struct __rics_state rics_state;
#if INCLUDE_RICS_UDP_CON
	struct ns_connection* ricsUdp_nc;
	struct sockaddr_in rics_udp_sockaddr;
	time_t heartbits_send_time;
	time_t heartbits_recv_time;
	time_t serial_timeout_refresh_time;
	int heartbeat_received,heartbeat_timerActive,heatbeat_lostCnt;
	int heartbeat_alive;
#endif
#if INCLUDE_RICS_TCP_CON
	// struct __rics_msg_rx_arg rics_ctrl_rx_arg;// for recv_msg
	struct ns_connection* ricsTcp_nc;
	char sendData[TCP_MSG_BUF_MAX+1];int sendDataLen;// for not connected case!!
#endif
#if INCLUDE_SERIAL_CON	
	// struct __rics_msg_rx_arg rics_serial_rx_arg;// for recv_msg
	struct ns_connection* serial_nc;unsigned long logEnTime;
	char sendMessage[SERIAL_DATA_MAX+1];int sendMessageLen;char sendPortAB;int sendRICS_node_id;// for not connected case!!
#endif

	// request_nc queue
	// websock_nc list
	volatile PLIST websock_list;
	volatile PQUEUE request_queue;
#if RICS_CONTABLE_MUTEX_LOCK	
	volatile pthread_mutex_t lock_ws;
	volatile pthread_mutex_t lock_rq;
#endif
}ricsConn;


typedef struct _ricsMan_master_struct{// RMCP_RICS_RECEIVER
	int cpType_X;

	char device_XXX[PATH_MAX];
	void* ricsConn_XXX;
	
	struct ns_connection* ricsUds_nc;
	struct ns_connection* ricsMasterUdp_nc;
  	time_t last_check_time;

	char tmp_XXX[PATH_MAX];
}ricsMan_master_struct,ricsHBTmaster_conn_param;


typedef struct _ricsWebsock_conn_param{// RMCP_RICS_WEBSOCK
	int cpType;

	char device[PATH_MAX];

	void* ricsConn_XXX;
	void* ricsUds_nc_XY;
	void* ricsMasterUdp_nc_XXX;
  	time_t last_check_time_XXX;// for INCLUDE_RICS_UDP_MASTER
	char tmp_XXX[PATH_MAX];
	int binaryCon;char serialSendFmt[64];
}ricsWebsock_conn_param;

typedef struct _ricsSerial_conn_param{// RMCP_RICS_SERIAL
	int cpType;

	char device[PATH_MAX];
	ricsConn* ricsConn;

	void* ricsUds_nc_XXX;
	void* ricsMasterUdp_nc_XXX;
  	time_t last_check_time_XXX;// for INCLUDE_RICS_UDP_MASTER
	char tmp_XXX[PATH_MAX];

	struct __rics_msg_rx_arg rics_serial_rx_arg;// for recv_msg
}ricsSerial_conn_param;

typedef struct _ricsCtrl_conn_param{// RMCP_RICS_TCP
	int cpType;

	char device[PATH_MAX];
	ricsConn* ricsConn;

	void* ricsUds_nc_XXX;
	void* ricsMasterUdp_nc_XXX;
  	time_t last_check_time_XXX;// for INCLUDE_RICS_UDP_MASTER
	char tmp_XXX[PATH_MAX];

	struct __rics_msg_rx_arg rics_ctrl_rx_arg;// for recv_msg
}ricsCtrl_conn_param;

typedef struct _ricsHBT_conn_param{// RMCP_RICS_UDP
	int cpType;

	char device[PATH_MAX];
	ricsConn* ricsConn;
	
	void* ricsUds_nc_XXX;
	void* ricsMasterUdp_nc_XXX;
  	time_t last_check_time_XXX;// for INCLUDE_RICS_UDP_MASTER
	char tmp_XXX[PATH_MAX];
}ricsHBT_conn_param;

typedef struct _nc_cb_t {// for AJAX
	void* nc;
	char cb[128];
}nc_cb_t;

enum WS_FORWARD_OPT{
	wsfo_broadcast=0,
	wsfo_last=1,
	wsfo_first=2,
	wsfo_firstAndLast=3,
};
#ifdef NS_FOSSA_VERSION
int websockify_websock_find(void* nc);
void websockify_websock_add(void* nc);
void websockify_websock_del(void* nc);
#endif
void rics_websock_forward_msg(struct ns_mgr* mgr,const char* device_id,const char* msg,int msgLen,int opt);/*WS_FORWARD_OPT*/
void rics_request_reply_msg(struct ns_mgr* mgr,const char* device_id,const char* msg,int msgLen);
void rics_request_delayedReply_msg(struct ns_mgr* mgr,const char* device_id,const char* msg,int msgLen);
void serialData2Websocket(struct ns_mgr* mgr,const char* device_id,char side,const char* rxBuf,int rxLen);//RICS->websocket
void indData2Websocket(struct ns_mgr* mgr,const char* device_id,char side,const char* rxBuf,int rxLen);//RICS->websocket
void websocket2serialData(struct ns_connection *ctxFrom,const char* data,int dataLen,int bAppendLF,void* cpOrg);//websocket->RICS
int rics_module_sendTo(struct ns_connection *ctxFrom,const char* device_id,const char* data,int dataLen,char* response,int *pResponseLen);
#ifdef NS_FOSSA_VERSION
static void handle_rics_reply(struct ns_connection *nc, const char* output,int outputLen,const char* jsonp_cb,const char * device);
static void handle_rics_call(struct ns_connection *nc, struct http_message *hm) ;
#else
static void handle_rics_reply(struct mg_connection *conn, const char* output,int outputLen,const char* jsonp_cb,const char * device);
static int handle_rics_call(struct mg_connection *conn) ;
#endif
int rics_module_add(struct ns_mgr* mgr,void *ctxFrom,const char* device_id);
void rics_module_remove(struct ns_mgr* mgr,void *ctxFrom,const char* device_id,int bForceRemove);

#if INCLUDE_SERIAL_CON
void start_serialTcp_connector(struct ns_mgr* mgr,const char* device_id);
void stop_serialTcp_connector(struct ns_mgr* mgr,const char* device_id);
#endif
#if INCLUDE_RICS_UDP_CON||INCLUDE_RICS_TCP_CON
void start_ricsUdp_connector(struct ns_mgr* mgr,const char* device_id);
void stop_ricsUdp_connector(struct ns_mgr* mgr,const char* device_id);
void start_ricsTcp_connector(struct ns_mgr* mgr,const char* device_id);
void stop_ricsTcp_connector(struct ns_mgr* mgr,const char* device_id);
#if INCLUDE_RICS_UDP_MASTER
static void ricsmanRicsUdp_handler(struct ns_connection *nc, int ev, void *p);
#else
static void ricsconnRicsUdp_handler(struct ns_connection *nc, int ev, void *p);
#endif
#else
static void ricsmanIpc_handler(struct ns_connection *nc, int ev, void *p) ;
#endif

void transfer2RICS_serialData(struct ns_connection *ctxFrom,const char* device_id,char side,const char* data,int dataLen);
#if USE_ANOTHER_RICS_SERVER
void transfer2RICS_serialData_safe(struct ns_connection *ctxFrom,const char* device_id,char side,const char* data,int dataLen);
int rics_module_sendTo_UDP_ipcstr_safe(struct ns_connection *ctxFrom,const char* device_id,const char* pIpc_recv_data);
int rics_module_sendTo_TCP_ipcstr_safe(struct ns_connection *ctxFrom,const char* device_id,const char* pIpc_recv_data);
#endif

#endif /*ricsLib.c*/

#endif /*__NS_WEBSOCKIFY_H__*/
