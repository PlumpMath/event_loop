#if TICB_WS2TCP
//websockify + ricsConnector!!

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include"ns_websockify.h"
#if INCLUDE_UART2TCP_SUPPORT
void /*vgwCardMan::*/UART_close_dev(int sd);
int /*vgwCardMan::*/UART_open_dev(int uartNo,const char* devNameInAndParam);
#endif

#if USE_LFS_ACCESS_AS_LIBRARY
int lfs_write_open(const char* filename);
int lfs_write_out(int fd,unsigned long ct,const char* data,int dataLen);
int lfs_write_close(int fd);
static char* convDeviceId2Path(const char* device_id,char* dst_path)
{
	int i,j,len=strlen(device_id);
	char* dst_path0=dst_path;

	for(i=j=0;i<len;i++){
		char c=device_id[i];

		if(c==':'){strcpy(dst_path,"-");dst_path++;j++;}
		else if(c=='/'){strcpy(dst_path,"-");dst_path++;j++;}
		else if(c=='\\'){strcpy(dst_path,"-");dst_path++;j++;}
		else {*dst_path++=c;*dst_path='\0';j++;}
	}
	return dst_path0;
}
#endif
// logEn var support
static void logEn_for_SOCKdevice(ws2tcp_conn_param* cp,const char* logEn)
{
	if(logEn&&logEn[0]){
		int bLogEn=(logEn[0]-'0');
		if(cp){
			if(bLogEn&&cp->logEnTime==0){cp->logEnTime=time(NULL);if(!cp->logEnTime)cp->logEnTime=1;}
			else if(!bLogEn&&cp->logEnTime!=0){;}
		}
	}
}
static void log_SOCKSend(const char* device_id,char portAB,const char* msg,int msgLen,unsigned long logEnTime)// only for echooff case??
{
#if USE_LFS_ACCESS_AS_LIBRARY
	int fd=-1,ret=0;
	char path[PATH_MAX],pathBase[PATH_MAX];

	if(logEnTime==0||!(portAB=='A'||portAB=='B'))return;
	convDeviceId2Path(device_id,pathBase);
	sprintf(path,"log/%s_%c_%08lx.log",pathBase,portAB,logEnTime);
	fd=lfs_write_open(path);
	if(fd>=0){
		// unsigned long ct=0;
		// if(ct==0)ct=time(NULL)-GMT2KST_OFFSET;// save with GMT
		// ret=lfs_write_out(fd,ct,msg,msgLen);
			ret=write(fd,msg,msgLen);
		lfs_write_close(fd);
	}
	(void)ret;
#else
	(void)device_id;(void)portAB;(void)msg;(void)msgLen;(void)logEnTime;
#endif	
}
static void log_SOCKRecv(const char* device_id,char portAB,const char* msg,int msgLen,unsigned long logEnTime)
{
#if USE_LFS_ACCESS_AS_LIBRARY
		int fd=-1,ret=0;
		char path[PATH_MAX],pathBase[PATH_MAX];
	
		if(logEnTime==0||!(portAB=='A'||portAB=='B'))return;
		convDeviceId2Path(device_id,pathBase);
		sprintf(path,"log/%s_%c_%08lx.log",pathBase,portAB,logEnTime);
		fd=lfs_write_open(path);
		if(fd>=0){
			// unsigned long ct=0;
			// if(ct==0)ct=time(NULL)-GMT2KST_OFFSET;// save with GMT
			// ret=lfs_write_out(fd,ct,msg,msgLen);
			ret=write(fd,msg,msgLen);
			lfs_write_close(fd);
		}
		(void)ret;
#else
		(void)device_id;(void)portAB;(void)msg;(void)msgLen;(void)logEnTime;
#endif	
}
void logEn_for_RICSdevice(const char* device,const char* logEn)
{
	if(logEn&&logEn[0]){
		ricsConn *rc=(ricsConn*)find_ricsconn(device);
		int bLogEn=(logEn[0]-'0');
		if(rc){
			if(bLogEn&&rc->logEnTime==0){rc->logEnTime=time(NULL);if(!rc->logEnTime)rc->logEnTime=1;}
			else if(!bLogEn&&rc->logEnTime!=0){;}
		}
	}
}
static void log_RICS_serialSend(const char* device_id,char portAB,const char* msg,int msgLen,unsigned long logEnTime)// only for echooff case??
{
#if USE_LFS_ACCESS_AS_LIBRARY
	int fd=-1,ret=0;
	char path[PATH_MAX];

	if(logEnTime==0||!(portAB=='A'||portAB=='B'))return;
	sprintf(path,"log/%s_%c_%08lx.log",device_id,portAB,logEnTime);
	fd=lfs_write_open(path);
	if(fd>=0){
		// unsigned long ct=0;
		// if(ct==0)ct=time(NULL)-GMT2KST_OFFSET;// save with GMT
		// ret=lfs_write_out(fd,ct,msg,msgLen);
			ret=write(fd,msg,msgLen);
		lfs_write_close(fd);
	}
	(void)ret;
#else
	(void)device_id;(void)portAB;(void)msg;(void)msgLen;(void)logEnTime;
#endif	
}
static void log_RICS_serialRecv(const char* device_id,char portAB,const char* recvMsg,int recvMsgLen)
{
	ricsConn* rc=find_ricsconn(device_id);
	if(rc&&rc->logEnTime!=0)
		log_RICS_serialSend(device_id,portAB,recvMsg,recvMsgLen,rc->logEnTime);
}

#if RICS_CONTABLE_MUTEX_LOCK	
static volatile pthread_mutex_t g_lockR=PTHREAD_MUTEX_INITIALIZER;
static int r_Lock(void)
{
	int ret=0;
	ret=pthread_mutex_lock((pthread_mutex_t*)&g_lockR);
	return ret;
}
static int r_Unlock(void)
{
	int ret=0;
	ret=pthread_mutex_unlock((pthread_mutex_t*)&g_lockR);
	return ret;
}
#define aj_Lock() r_Lock()
#define aj_Unlock() r_Unlock()
// #define rq_Lock(cp)
// #define rq_Unlock(cp)
// #define ws_Lock(cp)
// #define ws_Unlock(cp)
static int rq_Lock(ricsConn* cp)
{
	int ret=0;
	ret=pthread_mutex_lock((pthread_mutex_t*)&cp->lock_rq);
	return ret;
}
static int rq_Unlock(ricsConn* cp)
{
	int ret=0;
	ret=pthread_mutex_unlock((pthread_mutex_t*)&cp->lock_rq);
	return ret;
}
static int ws_Lock(ricsConn* cp)
{
	int ret=0;
	ret=pthread_mutex_lock((pthread_mutex_t*)&cp->lock_ws);
	return ret;
}
static int ws_Unlock(ricsConn* cp)
{
	int ret=0;
	ret=pthread_mutex_unlock((pthread_mutex_t*)&cp->lock_ws);
	return ret;
}
#else
#define r_Lock()
#define r_Unlock()
#define aj_Unlock()
#define aj_Lock()
#define rq_Lock(cp)
#define rq_Unlock(cp)
#define ws_Lock(cp)
#define ws_Unlock(cp)
#endif


#ifdef NS_FOSSA_VERSION
// only for ws2soc service
#if RICS_CONTABLE_MUTEX_LOCK	
#define WS_Lock()
#define WS_Unlock()
#else
#define WS_Lock()
#define WS_Unlock()
#endif
volatile PLIST g_websock_List=NULL;
int websockify_websock_find(void* nc)
{
	int ret=0;

	if(!nc)return 0;// not found// cannot be called!!
	WS_Lock();
	if(!g_websock_List)goto EndFind;
	ret=find_ptr_list(g_websock_List,nc);
EndFind:	
	WS_Unlock();
	return ret;
}
void websockify_websock_add(void* nc)
{
	if(!nc)return;// cannot be called!!
	WS_Lock();
	if(!g_websock_List)g_websock_List=alloc_ptr_list();
	addTo_ptr_list(g_websock_List,nc);
	WS_Unlock();
}
void websockify_websock_del(void* nc)
{
	if(!nc)return;// cannot be called!!
	WS_Lock();
	if(!g_websock_List)goto EndDel;
	removeFrom_ptr_list(g_websock_List,nc);
	// auto free_ptr_list on EMPTY??
EndDel:	
	WS_Unlock();
	return;
}
#endif

#ifdef USE_CHECKBYTES_BEFORE_RECV
#include <sys/types.h>
#warning "USE_CHECKBYTES_BEFORE_RECV defined"
#if 1 // ndef FIONREAD    // cygwin defines FIONREAD in  socket.h  instead of  ioctl.h
#warning "FIONREAD NOT defined"
typedef	unsigned long	u_long;// for IOR & FIONREAD
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif
#endif

#ifdef USE_CHECKBYTES_BEFORE_RECV
volatile int g_uartFd[30];
int isUARTfd(int fd)
{
	return g_uartFd[3]==fd;
}
int get_recv_size_for_fd_1st(struct ns_connection *nc,int bufSize)
{
	int ret,bytes=0,fd;// sizeof(buf)
	ws2tcp_conn_param* cp= (ws2tcp_conn_param*) nc->user_data;

	// fprintf(stderr,"+fd(%d) checking 1st(uartFd[3]=%d,cpType=%d)\n",nc->sock,g_uartFd[3],(cp? cp->cpType:-1));
	if(!cp||cp->cpType!=RMCP_WEBSOCKIFY_WEBSOCK||!isUARTfd(nc->sock))return bufSize;
	fd=nc->sock;

	// fprintf(stderr,"-fd(%d) checking 1st\n",fd);
	if(errno)errno=0;//auto clear error_no
	ret=ioctl(fd, FIONREAD /*SIOCINQ*/, &bytes);
	if(ret<0||errno){
		int myerrno=errno;
		
		fprintf(stderr,"fd(%d): error FIONREAD to fd ret=%d errno%d,len=%d(1st)\n",fd,ret,myerrno,bytes);
		errno=0;//auto clear error_no
		bytes=bufSize;
		if(myerrno==ENOTSUP){errno=0;return bufSize;}// ENOTSUP : 134
	}
	else if(bytes>(int)bufSize)bytes=bufSize;
	else if(bytes==0)bytes=1;// for zero-byte packet!!
	// if(bytes<=0){n=-1;errno=0;return;}
	return bytes;
}
int get_recv_size_for_fd_2nd(struct ns_connection *nc,int bufSize,int n)
{
	int ret,bytes=0,fd;// sizeof(buf)
	ws2tcp_conn_param* cp= (ws2tcp_conn_param*) nc->user_data;
	
	if(!cp||cp->cpType!=RMCP_WEBSOCKIFY_WEBSOCK||!isUARTfd(nc->sock))return bufSize;
	fd=nc->sock;

	// fprintf(stderr,"fd(%d) checking 2nd\n",fd);
	if(errno){
	  int myerrno=errno;
	  
	  fprintf(stderr,"fd(%d): error%d after NS_RECV after n=%d\n",fd,myerrno,n);
	  // errno=0;//auto clear error_no
	  return -1;
	}
	bytes=0;
	ret=ioctl(fd, FIONREAD /*SIOCINQ*/, &bytes);
	if(ret<0||errno){
		int myerrno=errno;
		
		fprintf(stderr,"fd(%d): error FIONREAD to fd ret=%d errno%d,len=%d(next),after n=%d\n",fd,ret,myerrno,bytes,n);// error 2
		errno=0;//auto clear error_no
		if(myerrno==ENOTSUP){errno=0;return bufSize;}// ENOTSUP : 134
		bytes=0;
	}
	else if(bytes>(int)bufSize)bytes=bufSize;
	// else if(bytes==0)bytes=1;// for zero-byte packet!!
	if(bytes<=0){return 0;}
	return bytes;
}
#endif
static void x2soc_handler(struct ns_connection *nc, int ev, void *p) {//TCP/UDP socket Handler(target)
  // struct mg_connection *conn = (struct mg_connection *) nc->user_data;
  ws2tcp_conn_param* cp= (ws2tcp_conn_param*) nc->user_data;
#ifdef NS_FOSSA_VERSION
  struct ns_connection *conn = (struct ns_connection *)(cp&&cp->parentType==0? cp->parent.conn:NULL);
  struct ns_connection *parent_nc = (struct ns_connection *)(cp? cp->parent.nc:NULL);
#else
  struct mg_connection *conn = (struct mg_connection *)(cp&&cp->parentType==0? cp->parent.conn:NULL);
  struct ns_connection *parent_nc = (struct ns_connection *)(cp&&cp->parentType==1? cp->parent.nc:NULL);
#endif
  struct iobuf *io = &nc->recv_iobuf;
  // union socket_address* p_sa=(union socket_address*)p;// for ACCEPT
  int* connect_err=(int*)p;// for CONNECT
  int* p_n=(int*)p;//
  (void) p;
  
  switch (ev) {
    case NS_CONNECT:
	if(!*connect_err){
		printf("target(%p) connected OK=%d,client(%p) detected.\n",nc,!*connect_err,(conn? (void*)conn:(void*)parent_nc));	
		if(!strcmp(cp->target,"unix")&&!strcmp(cp->target,"domain")&&!strcmp(cp->target,"udp"))ns_tcpSetKeepAlive(nc, 1, 10, 2, 5);// 1 (KEEPALIVE ON),최초에 세션 체크 시작하는 시간 (단위 : sec),최초에 세션 체크 패킷 보낸 후, 응답이 없을 경우 다시 보내는 횟수 (단위 : 양수 단수의 갯수),TCP_KEEPIDLE 시간 동안 기다린 후, 패킷을 보냈을 때 응답이 없을 경우 다음 체크 패킷을 보내는 주기 (단위 : sec)
	}
	else{
		printf("target(%p for %s) (flags=0x%lx) connection FAILED=%d\n",nc,(cp? cp->tmp:""),(unsigned long)nc->flags,*connect_err);// 111	
	}
	break;

    case NS_CLOSE://network broken!!
       // If either connection closes, unlink them and schedule closing
       g_num_ws2socSub++;
	printf("target(%p for %s) closed(flags=0x%lx,DEVtx=%d/%d,DEVrx=%d/%d/%d)(add=%d,sub=%d).\n",nc,(cp? cp->tmp:""),(unsigned long)nc->flags,(cp? cp->numTx:-1),(cp? cp->numTxComp:-1),(cp? cp->numRx0:-1),(cp? cp->numRx:-1),(cp? cp->numRxComp:-1),g_num_ws2socAdd,g_num_ws2socSub);// NSF_CLOSE_IMMEDIATELY on connectErr!!	
	if(cp){
 		  free(cp);
	      if (conn != NULL) {
#ifdef NS_FOSSA_VERSION
			printf("client(%p) close requested.\n",parent_nc);	
			ns_send_websocket_frame(parent_nc, WEBSOCKET_OP_CLOSE,NULL,0);
			websockify_websock_del(parent_nc);
#else
			struct ns_connection *parent_nc=mg_get_nc_conn(conn);

			printf("client(%p) close requested(parent_nc=%p).\n",conn,parent_nc);	
			conn->connection_param=NULL;mg_websocket_write(conn,WEBSOCKET_OPCODE_CONNECTION_CLOSE,NULL,0);
			// websockify_websock_del(conn);
#endif		 
	        parent_nc->user_data = NULL;
	      }
	      else if(parent_nc!=NULL){
			printf("client close requested(parent_nc=%p).\n",parent_nc);	
#ifdef NS_FOSSA_VERSION
	        parent_nc->flags |= NSF_SEND_AND_CLOSE;
#else
	        parent_nc->flags |= NSF_FINISHED_SENDING_DATA;
#endif
	        parent_nc->user_data = NULL;
	      }
	}
      nc->user_data = NULL;
      break;

    case NS_RECV:
      // Forward arrived data to the other connection, and discard from buffer
        // ns_send(pc, io->buf, io->len);
	// printf("target(%p) recv %d bytes\n",nc,io->len);	
	// printf("pR%d",*p_n);// OK(num recv!!)
	if(io->len>0&&io->buf){
	      if (conn != NULL) {
		  	if(cp&&io->len>0)cp->numTx++;
			log_SOCKRecv(cp->target,'A',io->buf, io->len,cp->logEnTime);
#ifdef NS_FOSSA_VERSION
    		ns_send_websocket_frame(parent_nc, WEBSOCKET_OP_BINARY, io->buf, io->len);//forward to Websock(client)
#else
	        mg_websocket_write(conn, WEBSOCKET_OPCODE_BINARY, io->buf, io->len);//forward to Websock(client)
#endif	        
	      }
	      else if(parent_nc!=NULL){
	        ns_send(parent_nc, io->buf, io->len);
	      }
	}
        iobuf_remove(io, io->len);
      break;
    case NS_SEND:
	if(cp&&*p_n>0)cp->numRxComp++;// -1 on connectErr!!
      //printf("pS%d.",*p_n);// OK(num sent!!)
      break;
    case NS_POLL:
	// printf("pP.");// printf("target(%p) polling\n",nc);	
   	break;
    default:
      break;
  }
}
#ifdef NS_FOSSA_VERSION
void handle_ws2soc_websock_connect(struct ns_connection *nc, struct http_message *hm)
{
	char device[PATH_MAX], dst[PATH_MAX], src[PATH_MAX];char target_buf[PATH_MAX];char logEn[64];
	const char* target_addr=hm->uri.p+7;int target_len=hm->uri.len-7;
	struct ns_mgr* mgr=nc->mgr;
	ws2tcp_conn_param* cp=NULL;
	struct ns_str* protocol=hm? ns_get_http_header(hm,"Sec-WebSocket-Protocol"):NULL;

	g_num_ws2socAdd++;
	((char* )target_addr)[target_len]='\0';
	memcpy(target_buf,target_addr,target_len);((char* )target_buf)[target_len]='\0';target_addr=target_buf;
	memset(logEn,0,sizeof(logEn));
	printf("client(query)='%.*s'\n",(int)hm->query_string.len,hm->query_string.p);
	if(!target_addr[0])target_addr="10.0.68.71:23";
	device[0]='\0';dst[0]='\0';src[0]='\0';
	// device=aaa.bbb.ccc.ddd or udp://aaa.bbb.ccc.ddd, dst=pppp
	ns_get_http_var(&hm->query_string, "device", device, sizeof(device));// for query not body
	ns_get_http_var(&hm->query_string, "dst", dst, sizeof(dst));
	ns_get_http_var(&hm->query_string, "src", src, sizeof(src));
	ns_get_http_var(&hm->query_string, "logEn", logEn, sizeof(logEn));
	if(logEn[0]&&logEn[0]=='1'){;}
	if(device[0]){
		target_addr=device;
		if(dst[0]){strcat(device,":");strcat(device,dst);}
	}

	printf("client(%p) connecting to '%.*s'\n",nc,target_len,target_addr);
	websockify_websock_add(nc);
	nc->user_data=(cp=(ws2tcp_conn_param*)calloc(1,sizeof(ws2tcp_conn_param)));
	cp->cpType=RMCP_WEBSOCKIFY_WEBSOCK;
	cp->parentType=0;
	cp->parent.conn=nc;cp->parent.nc=nc;
#if INCLUDE_UART2TCP_SUPPORT
	if(memcmp(target_addr,"uart:",5)==0){
		int uartNo=-1,fd=-1;
		
		sscanf(target_addr+5,"%d",&uartNo);// baud,parity,char,stop,...
		fd=UART_open_dev(uartNo,target_addr+5);
#ifdef USE_CHECKBYTES_BEFORE_RECV
		g_uartFd[uartNo]=fd;
#endif		
		printf("UART%d open g_uartFd[%d]=%d\n",uartNo,uartNo,fd);
		// write(fd,"hello UART\r\n",12);
		cp->pc=ns_add_sock(mgr,(sock_t)fd,x2soc_handler, cp/*conn*/);
		strcpy(cp->tmp,target_addr);
	}else
	if(memcmp(target_addr,"fd:",3)==0){
		int fd=-1;
		
		sscanf(target_addr+3,"%d",&fd);
		cp->pc=ns_add_sock(mgr,(sock_t)fd,x2soc_handler, cp/*conn*/);
		strcpy(cp->tmp,target_addr);
	}else
#endif	
	if(memcmp(target_addr,"unix:",5)==0||memcmp(target_addr,"domain:",7)==0){
		char tmp[PATH_MAX];
		struct ns_bind_opts opts;

		if(src[0])strcpy(tmp,src);
		else strcpy(tmp,"unix:/tmp/1234");
		if(strstr(tmp,"/"))unlink(strstr(tmp,"/"));
		printf("target(%s) connecting to '%s'\n",tmp,target_addr);
		opts.user_data=cp;//opts.error_string;opts.flags;
		cp->pc=ns_bind_opt(mgr, tmp, x2soc_handler, opts);// on client Connect -> start target connect
		ns_setPeer(cp->pc,target_addr);
		strcpy(cp->tmp,tmp);
	}else{
		struct ns_connect_opts opts;
		
		opts.user_data=cp;//opts.error_string;opts.flags;
		cp->pc=ns_connect_opt(mgr, target_addr, x2soc_handler, opts);// on client Connect -> start target connect
		strcpy(cp->tmp,"");
	}
	strcpy(cp->target,target_addr);
	if(protocol&&strstr(protocol->p,"binary")!=NULL)cp->binaryCon=1;
	logEn_for_SOCKdevice(cp,logEn);
	printf("target(%p) created.\n",cp->pc);
}
#else
void handle_ws2soc_websock_connect(struct mg_connection *conn)
{
	char device[PATH_MAX], dst[PATH_MAX], src[PATH_MAX];char logEn[64];
	const char* target_addr=conn->uri+7;
	struct ns_mgr* mgr=mg_get_nc_conn(conn)->mgr;
	ws2tcp_conn_param* cp;
	const char *protocol=mg_get_header(conn, "Sec-WebSocket-Protocol");
	
	if(!target_addr[0])target_addr="10.0.68.71:23";
	device[0]='\0';dst[0]='\0';src[0]='\0';memset(logEn,0,sizeof(logEn));
	// device=aaa.bbb.ccc.ddd or udp://aaa.bbb.ccc.ddd, dst=pppp
	mg_get_var(conn, "device", device, sizeof(device));// for query or content!!
	mg_get_var(conn, "dst", dst, sizeof(dst));
	mg_get_var(conn, "src", src, sizeof(src));
	mg_get_var(conn, "logEn", logEn, sizeof(logEn));
	if(logEn[0]&&logEn[0]=='1'){;}
	if(device[0]){
		target_addr=device;
		if(dst[0]){strcat(device,":");strcat(device,dst);}
	}

	printf("client(%p) connecting to %s\n",conn,target_addr);
	conn->connection_param=(cp=(ws2tcp_conn_param*)calloc(1,sizeof(ws2tcp_conn_param)));
	cp->cpType=RMCP_WEBSOCKIFY_WEBSOCK;
	cp->parentType=0;
	cp->parent.conn=conn;
#if INCLUDE_UART2TCP_SUPPORT
	if(memcmp(target_addr,"uart:",5)==0){
		int uartNo=-1,fd=-1;
		
		sscanf(target_addr+5,"%d",&uartNo);// baud,parity,char,stop,...
		fd=UART_open_dev(uartNo,target_addr+5);
#ifdef USE_CHECKBYTES_BEFORE_RECV
		g_uartFd[uartNo]=fd;
#endif
		printf("UART%d open g_uartFd[%d]=%d\n",uartNo,uartNo,fd);
		// write(fd,"hello UART\r\n",12);
		cp->pc=ns_add_sock(mgr,(sock_t)fd,x2soc_handler, cp/*conn*/);
		strcpy(cp->tmp,target_addr);
	}else
	if(memcmp(target_addr,"fd:",3)==0){
		int fd=-1;
		
		sscanf(target_addr+3,"%d",&fd);
		cp->pc=ns_add_sock(mgr,(sock_t)fd,x2soc_handler, cp/*conn*/);
		strcpy(cp->tmp,target_addr);
	}else
#endif	
	if(memcmp(target_addr,"unix:",5)==0||memcmp(target_addr,"domain:",7)==0){
		char tmp[PATH_MAX];

		if(src[0])strcpy(tmp,src);
		else strcpy(tmp,"unix:/tmp/1234");
		if(strstr(tmp,"/"))unlink(strstr(tmp,"/"));
		printf("target(%s) connecting to %s\n",tmp,target_addr);
		cp->pc=ns_bind(mgr, tmp, x2soc_handler, cp/*conn*/);// on client Connect -> start target connect
		ns_connectTo(cp->pc,target_addr);
		strcpy(cp->tmp,tmp);
	}else{
		cp->pc=ns_connect(mgr, target_addr, x2soc_handler, cp/*conn*/);// on client Connect -> start target connect
		strcpy(cp->tmp,"");
	}
	strcpy(cp->target,target_addr);
	if(protocol&&strstr(protocol,"binary")!=NULL)cp->binaryCon=1;
	logEn_for_SOCKdevice(cp,logEn);
	printf("target(%p) created.\n",cp->pc);
}
#endif

// serial 15minutes timeout!!
// 
void* g_ricsConnByIPmap=NULL;
ricsMan_master_struct* g_ricsMan_cp=NULL; //RICS_MAN global!!// RMCP_RICS_RECEIVER
void* find_ricsconn(const char* conn)
{
	void* ret;

	if(!g_ricsConnByIPmap)return NULL;
	r_Lock();
	ret=_find_ptr_map(g_ricsConnByIPmap,conn);
	if(!ret)printf("find_ricsconn(%s):%p ERROR\n",conn,ret);
	r_Unlock();
	return ret;
}
int countAll_ricsconn(void)
{
	int ret;

	if(!g_ricsConnByIPmap)return -1;
	r_Lock();
	ret=_countAll_ptr_map(g_ricsConnByIPmap);
	r_Unlock();
	return ret;
}
int insert_ricsconn(const char* conn,void* conn_data)
{
	int ret;
	
	if(!g_ricsConnByIPmap)return -1;
	r_Lock();
	printf("insert_ricsconn(%s):%p\n",conn,conn_data);
	_insert_ptr_map(g_ricsConnByIPmap,conn,conn_data);
	ret=_countAll_ptr_map(g_ricsConnByIPmap);
	r_Unlock();
	return ret;
}
int delete_ricsconn(const char* conn)
{
	int ret;
	void* conn_data;

	if(!g_ricsConnByIPmap)return -1;
	r_Lock();
	if((conn_data=_find_ptr_map(g_ricsConnByIPmap,conn))){
		printf("delete_ricsconn(%s):%p\n",conn,conn_data);
		_delete_ptr_map(g_ricsConnByIPmap,conn);
	}else{
		printf("delete_ricsconn(%s):%p ERROR\n",conn,conn_data);
	}
	ret=_countAll_ptr_map(g_ricsConnByIPmap);
	r_Unlock();

	return ret;
}

int iterateAll_ricsconn(int (*conn_func)(void* map,const char* dev_id,void* conn_data,void* Ctx,void* Ctx2,void* Ctx3),void* Ctx,void* Ctx2,void* Ctx3)
{
	int count=0;

	r_Lock();
	if(_countAll_ptr_map(g_ricsConnByIPmap)>0){
		count=_iterateAll_ptr_map(g_ricsConnByIPmap,conn_func,Ctx,Ctx2,Ctx3);
	}
	r_Unlock();
	return count;
}
// only for RICS ajax
volatile PLIST g_ajaxsock_list=NULL;
int rics_ajaxsock_find(void* nc)
{
	int ret=0;
	
	if(!nc)return 0;// not found// can not be called!!
	aj_Lock();
	if(!g_ajaxsock_list)goto EndFind;
	ret=find_ptr_list(g_ajaxsock_list,nc);
EndFind:	
	aj_Unlock();
	return ret;
}
void rics_ajaxsock_add(void* nc)
{
	if(!nc)return;// can not be called!!
	aj_Lock();
	if(!g_ajaxsock_list)g_ajaxsock_list=alloc_ptr_list();
	addTo_ptr_list(g_ajaxsock_list,nc);
	aj_Unlock();
	return;
}
void rics_ajaxsock_del(void* nc)
{
	int found=0;
	if(!nc)return;// can not be called!!
	aj_Lock();
	if(!g_ajaxsock_list)goto EndDel;
	found=removeFrom_ptr_list(g_ajaxsock_list,nc);
	// auto free_ptr_list on EMPTY??
EndDel:	
	aj_Unlock();
	if(!found){printf("rics_ajaxsock_del(%p) failed!!\n",nc);fflush(stdout);}
	return;
}

void _rics_requester_del(ricsConn *cp,void* nc)
{
	nc_cb_t* nc_cb=NULL,*nc_cb_removed=NULL;

	if(!cp)return;// can not be called!!
	
	rq_Lock(cp);
	if(cp->request_queue)nc_cb=(nc_cb_t*)getFront_ptr_queue(cp->request_queue);
	if(!nc_cb)goto EndDel;//empty
	if(nc_cb->nc!=nc)printf("_rics_requester_del: diff first nc(%p!=%p) on cp%p\n",nc,nc_cb->nc,cp);
	nc_cb->nc=NULL;nc_cb->cb[0]=0;
	free(nc_cb);
	
	nc_cb_removed=removeFront_ptr_queue(cp->request_queue);
	// printf("_rics_requester_del:nc%p on cp%p removed\n",nc,cp);fflush(stdout);
	if(nc_cb!=nc_cb_removed)printf("_rics_requester_del: nc_cb(%p!=%p) removed? on cp%p\n",nc_cb_removed,nc_cb,cp);
	
	if(getFront_ptr_queue(cp->request_queue)){
		printf("_rics_requester_del:request_queue have item yet on cp%p!!\n",cp);
	}
EndDel:
	rq_Unlock(cp);
	return;
}
void _rics_requester_add(ricsConn *cp,void* nc,const char* cb)
{
	nc_cb_t* nc_cb=NULL;
	
	if(!cp)return;// can not be called!!
	
	rq_Lock(cp);
	if(!cp->request_queue)goto EndAdd;// not READY!!
	nc_cb=(nc_cb_t*)calloc(1,sizeof(nc_cb_t));
	nc_cb->nc=nc;strcpy(nc_cb->cb,cb);
	appendBack_ptr_queue(cp->request_queue,nc_cb);
	// printf("_rics_requester_add:nc%p on on cp%p add\n",nc,cp);fflush(stdout);
EndAdd:
	rq_Unlock(cp);
	return;
}
void* _rics_requester_getFirst(ricsConn *cp,const char* *p_jsonp_cb)
{
	nc_cb_t* nc_cb=NULL;void* nc=NULL;

	if(!cp)return NULL;// can not be called!!
	
	rq_Lock(cp);
	if(!cp->request_queue)goto EndGet;
	nc_cb=(nc_cb_t*)getFront_ptr_queue(cp->request_queue);
// fprintf(stderr,"/f4_%p",nc_cb);fflush(stderr);
#ifdef _DEBUG
	if((unsigned int)nc_cb==0x1D1)goto EndGet;// ???
#endif  
	if(!nc_cb)goto EndGet;//empty
	if(p_jsonp_cb)*p_jsonp_cb=nc_cb->cb;
	nc=nc_cb->nc;
	
	rq_Unlock(cp);
	return nc;
EndGet:
	rq_Unlock(cp);
	return NULL;
}

#ifdef NS_FOSSA_VERSION
void OnDeviceDeparted(long remote_ip,long remote_port,char* deviceSn)
{
	printf("RICS(%s): disconnected(0x%lx:%ld)\n",deviceSn,remote_ip,remote_port);
}
void OnDeviceArrived(long remote_ip,long remote_port,char* deviceSn)
{
	printf("RICS(%s):    connected(0x%lx:%ld)\n",deviceSn,remote_ip,remote_port);
}
int broadcastLogMsgToUser(struct ns_context *ctxFrom,char* ipcLogMsg)
{
	printf("MSG(%p):    '%s'\n",ctxFrom,ipcLogMsg);
	return 0;
}
#endif
#if 1	// ricsLib.c
ssize_t NS_write(struct ns_connection *nc, const void *buf, size_t len)
{
	// printf("NS_write(%p): '%.*s'\n",nc,(int)len,(const char*)buf);
	return (ssize_t)ns_send(nc,buf,len);
}
ssize_t NS_sendto(struct ns_connection *nc, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{(void)flags;(void)addrlen;
	// printf("NS_sendto(%p): '%.*s'\n",nc,(int)len,(const char*)buf);
	return (ssize_t)ns_sendto(nc,(union socket_address*)dest_addr,buf,len);
}
int make_ip_peerAddr(const char *serv_ip, int port, struct sockaddr_in *pAddr)
{
	memset(pAddr, 0, sizeof(*pAddr));
	pAddr->sin_family = AF_INET;
	pAddr->sin_addr.s_addr = inet_addr(serv_ip);
	pAddr->sin_port = htons(port);

	return 0;
}
int is_IPADDR(const char* device_id)
{
	int cntDot=0;
	char* ptr;

	while(*device_id){
		ptr=strstr(device_id,".");
		if(ptr){cntDot++;device_id=ptr+1;}
		else device_id+=strlen(device_id);
	}
	return cntDot>=3;
}
#endif
/* URL_encode
The unreserved characters can be encoded, but should not be encoded. The unreserved characters are:

A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
a b c d e f g h i j k l m n o p q r s t u v w x y z
0 1 2 3 4 5 6 7 8 9 - _ . ~

The reserved characters have to be encoded only under certain circumstances. The reserved characters are:

! * ' ( ) ; : @ & = + $ , / ? % # [ ]
*/
/*
string = quotation-mark *char quotation-mark

         char = unescaped /
                escape (
                    %x22 /          ; "    quotation mark  U+0022
                    %x5C /          ; \    reverse solidus U+005C
                    %x2F /          ; /    solidus         U+002F
                    %x62 /          ; b    backspace       U+0008
                    %x66 /          ; f    form feed       U+000C
                    %x6E /          ; n    line feed       U+000A
                    %x72 /          ; r    carriage return U+000D
                    %x74 /          ; t    tab             U+0009
                    %x75 4HEXDIG )  ; uXXXX                U+XXXX

         escape = %x5C              ; \

         quotation-mark = %x22      ; "

         unescaped = %x20-21 / %x23-5B / %x5D-10FFFF

      var my_JSON_object = !(/[^,:{}\[\]0-9.\-+Eaeflnr-u \n\r\t]/.test(
             text.replace(/"(\\.|[^"\\])*"/g, ''))) &&
         eval('(' + text + ')');

static inline bool 
isControlCharacter(char ch)
{
   return ch > 0 && ch <= 0x1F;
}
         
*/
/*Table 43 (Informative) ? UTF-8 Encodings
Code Unit Value	Representation	1st Octet	2nd Octet	3rd Octet	4th Octet
0x0000 - 0x007F	00000000 0zzzzzzz	0zzzzzzz			
0x0080 - 0x07FF	00000yyy yyzzzzzz	110yyyyy	10zzzzzz		
0x0800 - 0xD7FF	xxxxyyyy yyzzzzzz	1110xxxx	10yyyyyy	10zzzzzz	
0xD800 - 0xDBFF
followed by
0xDC00 ? 0xDFFF	110110vv vvwwwwxx
followed by
110111yy yyzzzzzz	11110uuu	10uuwwww	10xxyyyy	10zzzzzz
0xD800 - 0xDBFF
not followed by
0xDC00 ? 0xDFFF	causes URIError				
0xDC00 ? 0xDFFF	causes URIError				
0xE000 - 0xFFFF	xxxxyyyy yyzzzzzz	1110xxxx	10yyyyyy	10zzzzzz	
*/
int jss_encode(char *s, int s_len, const char *str, int len, int bAddQuote)
{
  const char *begin = s, *end = s + s_len, *str_end = str + len;
  unsigned int ch;

#define EMIT(x) do { if (s < end) *s = x; s++; } while (0)

  if(bAddQuote)EMIT('"');
  while (str < str_end) {
    ch = (unsigned char)*str++;
    switch (ch) {
      case '"':  EMIT('\\'); EMIT('"'); break;
      case '/': EMIT('\\'); EMIT('/'); break;
      case '\\': EMIT('\\'); EMIT('\\'); break;
      case '\b': EMIT('\\'); EMIT('b'); break;
      case '\f': EMIT('\\'); EMIT('f'); break;
      case '\n': EMIT('\\'); EMIT('n'); break;
      case '\r': EMIT('\\'); EMIT('r'); break;
      case '\t': EMIT('\\'); EMIT('t'); break;
      default: 
	  	// isControlCharacter??
	  	if(ch<0x20||ch>0x10FFFF){if (s+5 < end) {int ret=sprintf(s,"\\u%04x",ch);/*printf("JE[%s]\n",s);*/s+=ret;}} else
	  	EMIT(ch);
    }
  }
  if(bAddQuote)EMIT('"');
  if (s < end) {
    *s = '\0';
  }

  return s - begin;
}

int jss_decode(char* dest,const char* src,int srcLen,int* p_bHaveEsc)// RFC 4627// same as UnquoteJsonString
{
	int i,outLen=0;

	for(i=0;i<srcLen;i++,src++){
		if(src[0]=='\\'&&src[1]=='n'){src++;i++;// line feed
			memcpy(dest,"\n",1);dest++;outLen++;if(p_bHaveEsc)*p_bHaveEsc=1;	
		}else if(src[0]=='\\'&&src[1]=='r'){src++;i++;// carriage return
			memcpy(dest,"\r",1);dest++;outLen++;if(p_bHaveEsc)*p_bHaveEsc=1;
		}else if(src[0]=='\\'&&src[1]=='b'){src++;i++;// backspace
			memcpy(dest,"\b",1);dest++;outLen++;if(p_bHaveEsc)*p_bHaveEsc=1;
		}else if(src[0]=='\\'&&src[1]=='t'){src++;i++;// tab
			memcpy(dest,"\t",1);dest++;outLen++;if(p_bHaveEsc)*p_bHaveEsc=1;
		}else if(src[0]=='\\'&&src[1]=='f'){src++;i++;// form feed
			memcpy(dest,"\f",1);dest++;outLen++;if(p_bHaveEsc)*p_bHaveEsc=1;
		}else if(src[0]=='\\'&&src[1]=='\\'){src++;i++;// reverse solidus
			memcpy(dest,"\\",1);dest++;outLen++;if(p_bHaveEsc)*p_bHaveEsc=1;
		}else if(src[0]=='\\'&&src[1]=='/'){src++;i++;// solidus 
			memcpy(dest,"/",1);dest++;outLen++;if(p_bHaveEsc)*p_bHaveEsc=1;
		}else if(src[0]=='\\'&&src[1]=='\"'){src++;i++;// quotation mark
			memcpy(dest,"\"",1);dest++;outLen++;if(p_bHaveEsc)*p_bHaveEsc=1;
		}else if(src[0]=='\\'&&src[1]=='u'){// uXXXX
			unsigned int ch=0;
			sscanf(src+2,"%04x",&ch);if(ch<0x100){memcpy(dest,&ch,1);dest++;outLen++;}else{memcpy(dest,&ch,2);dest+=2;outLen+=2;}if(p_bHaveEsc)*p_bHaveEsc=1;
			src+=5;i+=5;
		}
#if 0			
		else if(src[0]=='\\'&&src[1]=='x'){
			int ch=0;
			sscanf(src+2,"%02x",&ch);memcpy(dest,&ch,1);dest++;outLen++;if(p_bHaveEsc)*p_bHaveEsc=1;
			src+=3;i+=3;
		}else if(src[0]=='\x1b'){
			memcpy(dest,src,1);dest++;outLen++;if(p_bHaveEsc)*p_bHaveEsc=1;
		}
#endif			
		else{// unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
			memcpy(dest,src,1);dest++;outLen++;
		}
	}
	// *dest='\0'; outLen++;//null terminate!!
	return outLen;
}




#define EMS_SUB_RICS_TEST_OUTPUT_FILENAME "stdout" // "/tmp/rics_test.log"
#define EMS_SUB_RICS_TEST_PID_FILENAME "/tmp/rics_test_%s.pid" // "/tmp/rics_test.pid"
#if INCLUDE_SERIAL_CON
#define EMS_SUB_RICS_TEST_DBG_ARGS "--no-serial --print-err" //"--print-all" // --print-inf --print-dbg // "--udp-only"
#else
#define EMS_SUB_RICS_TEST_DBG_ARGS "--print-err" //"--print-all" // --print-inf --print-dbg // "--udp-only"
#endif
#ifdef NS_FOSSA_VERSION
int rics_port=7788;const char* rics_test="./rics_test";
#else
int rics_port=7788;const char* rics_test="../rics_test";
#endif
void rics_module_connect_serial(const char* device_id_name)// ??
{
	char device_id[PATH_MAX],*side;

	strcpy(device_id,device_id_name);
	side=strrchr(device_id,'_');
	if(!side)return;
	*side++='\0';
	// do not create send-only socket!!
	printf("rics_module_connect_serial(%s,%s)\n",device_id,side);
}
static int run_rics_test(struct ns_mgr* mgr,const char* device_id)
{
#if INCLUDE_RICS_UDP_CON||INCLUDE_RICS_TCP_CON
#if INCLUDE_SERIAL_CON
	g_rics_ctrl_debug_flag=DBG_FLAG_PRINT_ERR/*|DBG_FLAG_PRINT_INF|DBG_FLAG_PRINT_DBG*/;
#endif
#if INCLUDE_RICS_UDP_CON
	start_ricsUdp_connector(mgr,device_id);
#endif
#if INCLUDE_RICS_TCP_CON
	start_ricsTcp_connector(mgr,device_id);
#endif
	return 0;
#else
	char runStr[1024];
	char pidFile[PATH_MAX];
	char logFile[PATH_MAX];const char* logFmt=EMS_SUB_RICS_TEST_OUTPUT_FILENAME;
	(void)mgr;

#if INCLUDE_SERIAL_CON
	g_rics_ctrl_debug_flag=DBG_FLAG_PRINT_ERR;
#endif
	sprintf(pidFile,EMS_SUB_RICS_TEST_PID_FILENAME,device_id);// append device_id
	if(strcmp(logFmt,"stdout"))
		sprintf(logFile,logFmt,device_id);// append device_id
	else strcpy(logFile,logFmt);
	sprintf(runStr,"%s --rics-ip %s --rics-port %d " EMS_SUB_RICS_TEST_DBG_ARGS,rics_test,device_id,rics_port);
	return run_sub_process(runStr,pidFile,logFile);
#endif	
}
static int check_rics_test(const char* device_id)
{
#if INCLUDE_RICS_UDP_CON||INCLUDE_RICS_TCP_CON
	(void)device_id;
	return 0;
#else
	char pidFile[PATH_MAX];

	sprintf(pidFile,EMS_SUB_RICS_TEST_PID_FILENAME,device_id);
	return check_sub_process(pidFile);
#endif	
}
void rics_receiver_create(struct ns_mgr *mgr)
{
	ricsMan_master_struct* cp=g_ricsMan_cp;

	if(g_ricsConnByIPmap||g_ricsMan_cp)return;// already alloc-ed!!
	
	g_ricsConnByIPmap=_alloc_ptr_map();
	cp=(ricsMan_master_struct*)calloc(1,sizeof(ricsMan_master_struct));// RMCP_RICS_RECEIVER
	cp->cpType_X=RMCP_RICS_RECEIVER;
	g_ricsMan_cp=cp;
#if INCLUDE_RICS_UDP_CON||INCLUDE_RICS_TCP_CON
#else
	strcpy(cp->tmp,"unix:" EMS_RICS_DOMAIN_NAME);
	unlink(EMS_RICS_DOMAIN_NAME);
#ifdef NS_FOSSA_VERSION
	{
	struct ns_bind_opts opts;
	opts.user_data=cp;
	g_ricsMan_cp->ricsUds_nc=ns_bind_opt(mgr, "unix:" EMS_RICS_DOMAIN_NAME, ricsmanIpc_handler, opts);
	}
#else
	g_ricsMan_cp->ricsUds_nc=ns_bind(mgr, "unix:" EMS_RICS_DOMAIN_NAME, ricsmanIpc_handler, cp);	
#endif
#endif
#if INCLUDE_RICS_UDP_CON&&INCLUDE_RICS_UDP_MASTER
#ifdef NS_FOSSA_VERSION
	{
	struct ns_bind_opts opts;
	opts.user_data=cp;
	g_ricsMan_cp->ricsMasterUdp_nc=ns_bind_opt(mgr, "udp://7788", ricsmanRicsUdp_handler, opts);
	}
#else
	g_ricsMan_cp->ricsMasterUdp_nc=ns_bind(mgr, "udp://7788", ricsmanRicsUdp_handler, cp);	
#endif
	// connect to HERE
	if(!g_ricsMan_cp->ricsMasterUdp_nc)free(g_ricsMan_cp);
	else{
		int connect_err=0;
		
		printf("rics_receiver_create:%p(cp=%p) to %s\n",g_ricsMan_cp->ricsMasterUdp_nc,cp,"ALL_RICS");
		ricsmanRicsUdp_handler(g_ricsMan_cp->ricsMasterUdp_nc,NS_CONNECT,&connect_err);// simulate TCP connect
	}
#else
	(void)mgr;
#endif
}
void rics_receiver_destroy(struct ns_mgr *mgr)
{(void)mgr;
#if INCLUDE_RICS_UDP_CON&&INCLUDE_RICS_UDP_MASTER
	ricsMan_master_struct* cp=g_ricsMan_cp;

	printf("rics_receiver_destroy: start 1\n");fflush(stdout);
	if(!cp||!g_ricsConnByIPmap)return;// already free-ed!!
	printf("rics_receiver_destroy: start 2\n");fflush(stdout);
	if(cp->ricsMasterUdp_nc){cp->ricsMasterUdp_nc->user_data=NULL;NS_close_conn(cp->ricsMasterUdp_nc);}
	printf("rics_receiver_destroy: start 3\n");fflush(stdout);
	cp->ricsMasterUdp_nc=NULL;
#endif
#if INCLUDE_RICS_UDP_CON||INCLUDE_RICS_TCP_CON
#else
	if(g_ricsMan_cp->ricsUds_nc){g_ricsMan_cp->ricsUds_nc->user_data=NULL;NS_close_conn(g_ricsMan_cp->ricsUds_nc);}
	unlink(EMS_RICS_DOMAIN_NAME);
	g_ricsMan_cp->ricsUds_nc=NULL;
#endif	
	printf("rics_receiver_destroy: end 4\n");fflush(stdout);
	free(g_ricsMan_cp);
	printf("rics_receiver_destroy: end 5\n");fflush(stdout);
	g_ricsMan_cp=NULL;
	printf("rics_receiver_destroy: end 6\n");fflush(stdout);
	_free_ptr_map(g_ricsConnByIPmap);
	printf("rics_receiver_destroy: end\n");fflush(stdout);
	g_ricsConnByIPmap=NULL;
}

int delayed_send_serial_data_rpt(struct ns_connection *ctxFrom,ricsConn *rc, int rics_node_id, char port_AB, const char *message, int message_len);
int bRICS_ResponseFileDataURLencodeWS=1;// URIencode is more safe for textWebsocket!!
int bRICS_ResponseFileDataURLencodeAJAX=1;// URIencode is more safe for textWebsocket!!
void websocket2serialData(struct ns_connection *ctxFrom,const char* data,int dataLen,int bAppendLF,void* cpOrg)//websocket->RICS
{
	// ['192.168.1.123_A', 'ls']
	char urlDecMsg[32768];
	char device_id_name[PATH_MAX];
	int j;
	char sideAB;char device_id[PATH_MAX],*device;
	ricsWebsock_conn_param* cp=(ricsWebsock_conn_param*)cpOrg;
	(void)ctxFrom;

	// printf("websocket2serialData:data='%.*s',bin=%d\n",dataLen,data,bAppendLF);
	if(dataLen<=0)return;
	if(!(data[0]=='%'||data[0]=='['||data[0]=='{')){// not a url_encoded JSON data// binary data have data that data[0] is 'A','B','@'
		sideAB=data[0];

		printf("bin%d(%c):'%.*s'\n",bAppendLF,sideAB,dataLen-1,data+1);
		data++;dataLen--;device=cp->device;// only for binary Data
transfer2RICS_serialDataLab:		
		if(sideAB=='@'||sideAB=='C'){
			if(memcmp(data,"logEn=",6)==0){
				const char* logEn=data+6;
				logEn_for_RICSdevice(cp->device,logEn);
			}
			return;
		}
		// log_RICS_serialSend(device,sideAB,data,dataLen,rc->logEnTime);
#if USE_ANOTHER_RICS_SERVER
		transfer2RICS_serialData_safe(ctxFrom,device,sideAB,data,dataLen);// 3. websocket -> RICSserial
#else
		transfer2RICS_serialData(ctxFrom,device,sideAB,data,dataLen);
#endif
		return;
	}
	// decode('utf-8') first??
	urlDecMsg[0]=0;
	if(data[0]=='%'){// [ -> %xx
		// printf("NS_url_decode:'%.*s' start\n",wm->size,(char *) wm->data);
		j=NS_url_decode((char *) data, dataLen,urlDecMsg,sizeof(urlDecMsg),1/*for '+'->' '*/);
	}
	else{// JSON.stringify only CASE!!
		j=dataLen;memcpy(urlDecMsg,data, dataLen);urlDecMsg[dataLen]=0;
	}
	if(j<=0)return;
	// printf("\nwebsocket2serialData: jsonArray:'%s'(%d bytes)\n",urlDecMsg,strlen(urlDecMsg));
	// reverse of JSON.stringify!!
	{  struct json_token tokens0[32];int n;struct json_token *device_id_name_tok,*message_tok;
		// sscanf(urlDecMsg,"[\"%s\",\"%s\"]",device_id_name,message);
		memset(&tokens0[0],0,sizeof(tokens0));
		n = parse_json(urlDecMsg, strlen(urlDecMsg), tokens0, sizeof(tokens0) / sizeof(tokens0[0]));
		if(n<=0){
			printf("\nwebsocket2serialData: jsonArray err %d\n",n);
			return;
		}
		if(0){int i;
		printf("%d bytes parse_json\n",n);
		for(i=0;i<3;i++)printf("[%d] type=%d,num_desc=%d\n",i,tokens0[i].type,tokens0[i].num_desc);
		}
		//tokens0[0].type==JSON_TYPE_ARRAY
		if(tokens0[0].type==JSON_TYPE_ARRAY&&tokens0[0].num_desc>=2){
			char message[32000];
			int bHaveEsc=0,outLen=0;
			device_id_name_tok=&tokens0[1];// device_id_name_tok=find_json_token(tokens0, "device");
			memcpy(device_id_name,device_id_name_tok->ptr,device_id_name_tok->len);device_id_name[device_id_name_tok->len]=0;
			// don't care second name!!
			message_tok=&tokens0[2];// message_tok=find_json_token(tokens0, "message");
			// printf("message_tok='%.*s'\n",message_tok->len,message_tok->ptr);
			outLen=jss_decode(message,message_tok->ptr,message_tok->len,&bHaveEsc);message[outLen]='\0';//null terminate!!
			// printf("device_id_name:'%s',message:'%s'(%d bytes)\n",device_id_name,message,strlen(message));
			{
				char *side;

				strcpy(device_id,device_id_name);
				side=strrchr(device_id,'_');
				if(!side)return;
				*side++='\0';
				if(strlen(message)<=1)bAppendLF=0;// for non-binary single Char
				if(bAppendLF&&!bHaveEsc)strcat(message,"\n");
				data=message;dataLen=strlen(message);sideAB=*side;device=device_id;goto transfer2RICS_serialDataLab;				
				// transfer2RICS_serialData(ctxFrom,device_id,*side,message,strlen(message));
			}
		}
	}
}
int encodeSerial2WebsockMsg(unsigned char* encodedData,int encodeDataSize,const char* serialSendFmt,const unsigned char *sideANDdata,int sideANDdataLen)
{
	if(!strcmp(serialSendFmt,"URIencode")){//bRICS_ResponseFileDataURLencodeWS
		int nPtr=0;
		
		// sideStr[0]=sideANDdata[0];sideStr[1]='\0';json_emit((char*)encodedData,encodeDataSize,"{s:s,s:s}","side",sideStr,"data",urlEncData);
		nPtr=sprintf((char*)encodedData,"{\"side\":\"%c\",\"data\":\"",sideANDdata[0]);// simple JSON.stringify??
		NS_url_encode((const char*)&sideANDdata[1], sideANDdataLen-1,(char*)encodedData+nPtr,encodeDataSize-nPtr);nPtr+=strlen((const char*)encodedData+nPtr);// can include NUL char
		sprintf((char*)encodedData+nPtr,"\"}");nPtr+=2;
		// sprintf((char*)encodedData,"{\\\"side\\\": \\\"%c\\\", \\\"data\\\": \\\"%s\\\"}",sideANDdata[0],urlEncData);// simple JSON.stringify??
		return nPtr;
	}
	else if(!strcmp(serialSendFmt,"json")){//!bRICS_ResponseFileDataURLencodeWS
		int nPtr=0;

		nPtr+=sprintf((char*)encodedData+nPtr,"{\"side\":\"%c\",\"jsondata\":\"",sideANDdata[0]);
		jss_encode((char*)encodedData+nPtr,encodeDataSize-nPtr,(const char*)&sideANDdata[1], sideANDdataLen-1,0);nPtr+=strlen((const char*)encodedData+nPtr);// can include NUL char//json_emit_quoted_str
		sprintf((char*)encodedData+nPtr,"\"}");nPtr+=2;// simple JSON.stringify
		return nPtr;
	}
	else{
		memcpy(encodedData,sideANDdata,sideANDdataLen);
		return sideANDdataLen;
	}
	return 0;
}
void serialData2Websocket(struct ns_mgr* mgr,const char* device_id,char side,const char* rxBuf,int rxLen)// RICS->websocket
{
	char msg[32768];
#if 1	// if all-binary connected!!, or encode on send-routine
	msg[0]=side;
	memcpy(&msg[1],rxBuf,rxLen);
	rics_websock_forward_msg(mgr,device_id,msg,rxLen+1,wsfo_broadcast);
#elif 1
	int nPtr=0;

	nPtr+=sprintf(msg+nPtr,"{\"side\":\"%c\",\"jsondata\":\"",side);
	jss_encode(msg+nPtr,sizeof(msg)-nPtr,rxBuf, rxLen,0);nPtr+=strlen(msg+nPtr);// can include NUL char//json_emit_quoted_str
	sprintf(msg+nPtr,"\"}");nPtr+=2;// simple JSON.stringify
	rics_websock_forward_msg(mgr,device_id,msg,nPtr,wsfo_broadcast);
#else
	char urlEncData[32000];

	NS_url_encode(rxBuf, rxLen,urlEncData,sizeof(urlEncData));// can include NUL char
	// sideStr[0]=side;sideStr[1]='\0';json_emit(msg,sizeof(msg),"{s:s,s:s}","side",sideStr,"data",urlEncData);
	sprintf(msg,"{\"side\":\"%c\",\"data\":\"%s\"}",side,urlEncData);// simple JSON.stringify??
	// sprintf(msg,"{\\\"side\\\": \\\"%c\\\", \\\"data\\\": \\\"%s\\\"}",side,urlEncData);// simple JSON.stringify??
	rics_websock_forward_msg(mgr,device_id,msg,strlen(msg),wsfo_broadcast);
#endif
}
void indData2Websocket(struct ns_mgr* mgr,const char* device_id,char side,const char* rxBuf,int rxLen)// RICS->websocket// from on_rics_reply_msg
{
	char msg[32768];
#if 1	// if all-binary connected!!, or encode on send-routine(use serialSendFmt)
	msg[0]=side;
	memcpy(&msg[1],rxBuf,rxLen);
	rics_websock_forward_msg(mgr,device_id,msg,rxLen+1,wsfo_broadcast);
#elif 1
	int nPtr=0;

	nPtr+=sprintf(msg+nPtr,"{\"side\":\"%c\",\"jsondata\":\"",side);
	jss_encode(msg+nPtr,sizeof(msg)-nPtr,rxBuf, rxLen,0);nPtr+=strlen(msg+nPtr);// can include NUL char//json_emit_quoted_str
	sprintf(msg+nPtr,"\"}");nPtr+=2;// simple JSON.stringify
	rics_websock_forward_msg(mgr,device_id,msg,nPtr,wsfo_broadcast);
#else
	char urlEncData[32000];

	NS_url_encode(rxBuf, rxLen,urlEncData,sizeof(urlEncData));// can include NUL char
	// sideStr[0]=side;sideStr[1]='\0';json_emit(msg,sizeof(msg),"{s:s,s:s}","side",sideStr,"data",urlEncData);
	sprintf(msg,"{\"side\":\"%c\",\"data\":\"%s\"}",side,urlEncData);// simple JSON.stringify??
	// sprintf(msg,"{\\\"side\\\": \\\"%c\\\", \\\"data\\\": \\\"%s\\\"}",side,urlEncData);// simple JSON.stringify??
	rics_websock_forward_msg(mgr,device_id,msg,strlen(msg),wsfo_broadcast);
#endif
}


#if INCLUDE_RICS_UDP_CON||INCLUDE_RICS_TCP_CON
int proc_recv_rics_msg(struct __rics_msg_rx_arg *arg, const char* recvData, int recvDataLen,unsigned int* pRecvProcessed)// good for fragmented stream, but bad for concatenated stream??
{
	int ret=-2;
	struct __rics_msg_rx_arg *pArg = (struct __rics_msg_rx_arg *)arg;
	int rc;

	tcp_msg_t *pTcp_msg;
	unsigned char *pTcp_cur_rx_buf;

	pTcp_cur_rx_buf = pArg->cur_rx_buf;

	// printf("+proc_recv_rics_msg:pArg=%p\n",pArg);fflush(stdout);
	if(!pArg){ret=-3;goto EndProc;}// return -3;
	// printf(">proc_recv_rics_msg:rxBuf=%p,pArg->rxState=%d\n",pTcp_cur_rx_buf,pArg->rxState);fflush(stdout);
	rprint_dbg("%s() entering(%d).. %d(recvData=%p,recvDataLen=%d,*pRecvProcessed=%d)\n", __FUNCTION__, pArg->rxState, pArg->cur_rx_len,recvData,recvDataLen,(pRecvProcessed? (int)*pRecvProcessed:-1));

	while(1)
	{
		switch(pArg->rxState)
		{
			case eRECV_HOME:
			{
				pArg->rxState = eRECV_HEAD;
				pArg->cur_rx_len = 0;
				memset(pArg->cur_rx_buf, 0, TCP_MSG_BUF_MAX);
//				break;
			}
			case eRECV_HEAD:
			{
				// rc = recv(pArg->rics_msg_sock, &pTcp_cur_rx_buf[pArg->cur_rx_len], 4 - pArg->cur_rx_len, MSG_DONTWAIT);
				rc = recvDataLen; if(recvDataLen==0){rc=-1;errno = EAGAIN;}
				if(rc>0){
					rc=MIN(recvDataLen,4 - pArg->cur_rx_len);
					memcpy(&pTcp_cur_rx_buf[pArg->cur_rx_len],recvData,rc); recvData+=rc;recvDataLen-=rc;
					if(pRecvProcessed)(*pRecvProcessed)+=rc;
				}
				if(rc < 0)
				{
					if(errno == EAGAIN)
					{
						rprint_dbg("rics : EAGAIN(head)\n");

						ret=0;goto EndProc;// return 0;
//						break;
					}

					rprint_err("rics : tcp rx head error(%d:%s)\n", rc, strerror(errno));
					errno = 0;

					pArg->rxState = eRECV_ERROR;
					break;
				}
				else if(rc == 0)
				{
					rprint_err("rics : tcp EOF(head)\n");

					pArg->rxState = eRECV_ERROR;
					break;
				}
				else
				{
					pArg->cur_rx_len += rc;
					if(pArg->cur_rx_len >= 4)
					{
						pTcp_msg = (tcp_msg_t *)pTcp_cur_rx_buf;
						if((pTcp_msg->msg_len + 4) == pArg->cur_rx_len)
						{
							pArg->rxState = eRECV_DONE;
							break;
						}
						else
						{
							pArg->rxState = eRECV_DATA;
							// no break
						}
					}
					else
					{
						pArg->rxState = eRECV_HEAD;
						break;
					}
				}
			}
			case eRECV_DATA:
			{
				pTcp_msg = (tcp_msg_t *)pTcp_cur_rx_buf;
				// rc = recv(pArg->rics_msg_sock, &pTcp_cur_rx_buf[pArg->cur_rx_len], (pTcp_msg->msg_len + 4) - pArg->cur_rx_len, MSG_DONTWAIT);
				rc = recvDataLen;  if(recvDataLen==0){rc=-1;errno = EAGAIN;}
				if(rc>0){
					rc=MIN(recvDataLen,(pTcp_msg->msg_len + 4) - pArg->cur_rx_len);
					memcpy(&pTcp_cur_rx_buf[pArg->cur_rx_len],recvData,rc); recvData+=rc;recvDataLen-=rc;
					if(pRecvProcessed)(*pRecvProcessed)+=rc;
				}
				if(rc < 0)
				{
					if(errno == EAGAIN)
					{
						rprint_dbg("rics : EAGAIN(data)\n");

						ret=0;goto EndProc;// return 0;
//						break;
					}

					rprint_err("rics : tcp rx data error(%d:%s)\n", rc, strerror(errno));
					errno = 0;

					pArg->rxState = eRECV_ERROR;
					break;
				}
				else if(rc == 0)
				{
					rprint_err("rics : tcp EOF(data)\n");

					pArg->rxState = eRECV_ERROR;
					break;
				}
				else
				{
					pArg->cur_rx_len += rc;
					if((pTcp_msg->msg_len + 4) == pArg->cur_rx_len)
					{
						pArg->rxState = eRECV_DONE;
						// no break
					}
					else
					{
						pArg->rxState = eRECV_DATA;
						break;
					}
				}
			}
			case eRECV_DONE:
			{
				pArg->rxState = eRECV_HOME;
				ret=pArg->cur_rx_len;goto EndProc;// return pArg->cur_rx_len; // return immediately
//				break;
			}
			case eRECV_ERROR:
			{
				pArg->rxState = eRECV_HOME;
				ret=-1;goto EndProc;// return -1;
//				break;
			}
			default:
			{
				rprint_fatal("recv_rics_msg SM(%d) error!!\n", pArg->rxState);

				pArg->rxState = eRECV_HOME;
				ret=-1;goto EndProc;// return -1;
//				break;
			}
		}
	} // while(loop_flag)

	rprint_dbg("%s() exit(%d). %d\n", __FUNCTION__, pArg->rxState, pArg->cur_rx_len);
EndProc:
	// printf("-proc_recv_rics_msg:\n");fflush(stdout);
	return ret;
}
#else
int rics_module_sendTo_viaRicsMan(void *ctxFrom,const char* device_id,const char* data,int dataLen)
{
	char target_addr[PATH_MAX];
	(void)ctxFrom;

	if(!g_ricsMan_cp->ricsUds_nc)return 0;
	
	sprintf(target_addr,"unix:/tmp/rics/%s",device_id);
// printf("sending to rics%s('%.*s')\n",device_id,dataLen,data);
	ns_setPeer(g_ricsMan_cp->ricsUds_nc,target_addr);
	return ns_send(g_ricsMan_cp->ricsUds_nc,data,dataLen);
}
static void ricsmanIpc_handler(struct ns_connection *nc, int ev, void *p) 
{//TCP/UDP socket Handler(target)
  struct sockaddr_un* p_peeraddr=(struct sockaddr_un*)&nc->sa.sun;
  struct iobuf *io = &nc->recv_iobuf;
  (void) p;
  
  switch (ev) {
    case NS_RECV:
	if(io->len>0&&io->buf){
		  	const char* sock_name=NULL;
		  	// if(cp&&io->len>0)cp->numTx++;

// printf("ipc from %s('%.*s')\n",p_peeraddr->sun_path,io->len,io->buf);
			if(memcmp(p_peeraddr->sun_path,"/tmp/rics/",10)){return;}
			sock_name=strrchr(p_peeraddr->sun_path,'/')+1;
// printf("rics_ipc from %s\n",sock_name);
			if(strstr(sock_name,"_")){
				char device_id[PATH_MAX];char side=sock_name[strlen(sock_name)-1];
					
				if(!memcmp(io->buf,"RICS[node",9)){
					;//rics_module_connect_serial(sock_name);
					printf("rics_module_connect_serial(%s,%c):'%s'\n",device_id,side,io->len,(char*)io->buf);
				}
				else{
					memcpy(device_id,sock_name,strlen(sock_name)-2);device_id[strlen(sock_name)-2]=0;
					serialData2Websocket(nc->mgr,device_id,side,(char*)io->buf,io->len);
				}
			}
			else{
// printf("rics_ipc from %s('%.*s')\n",sock_name,io->len,io->buf);
				rics_request_reply_msg(nc->mgr,sock_name,io->buf, io->len);
			}
 	}
        iobuf_remove(io, io->len);
      break;
    default:
      break;
  }
}
#endif

#if INCLUDE_SERIAL_CON
static void proc_delayed_serialSend(ricsConn *rc, struct ns_connection *serial_nc)
{(void)serial_nc;
	if(rc->serial_nc&&rc->rics_state.serial_connected==1&&rc->sendMessageLen>0){
		int ret=0;

		log_RICS_serialSend(rc->device_id,rc->sendPortAB,rc->sendMessage,rc->sendMessageLen,rc->logEnTime);
		ret=__send_serial_data_rpt((ncSock_t)rc->serial_nc, rc->sendRICS_node_id, rc->sendPortAB, rc->sendMessage,rc->sendMessageLen);// to RICS(using NS_write)
		printf("proc_delayed_serialSend: for '%s' %d bytes,ret=%d\n",rc->device_id,rc->sendMessageLen,ret);fflush(stdout);
		rc->sendMessageLen=0;rc->sendMessage[0]=0;
	}
}
int delayed_send_serial_data_rpt(struct ns_connection *ctxFrom,ricsConn *rc, int rics_node_id, char port_AB, const char *message, int message_len)
{
	int ret=0;

	if(!rc)return 0;
	if(rc->serial_nc&&rc->rics_state.serial_connected==1){
		log_RICS_serialSend(rc->device_id,rc->sendPortAB,rc->sendMessage,rc->sendMessageLen,rc->logEnTime);
		ret=__send_serial_data_rpt((ncSock_t)rc->serial_nc, rics_node_id, port_AB, (char*)message,message_len);// to RICS(using NS_write)
	}
	else{
		memcpy(rc->sendMessage,message,message_len);rc->sendMessageLen=message_len;rc->sendPortAB=port_AB;rc->sendRICS_node_id=rics_node_id;
		start_serialTcp_connector(ctxFrom->mgr,rc->device_id);
		return -99;// to RICS(using NS_write)
	}
	return ret;
}

static void ricsmanSerial_handler(struct ns_connection *nc, int ev, void *p) 
{//TCP socket Handler(target)
  ricsSerial_conn_param* cp= (ricsSerial_conn_param*) nc->user_data;// SERIAL for a RICS
  struct iobuf *io = &nc->recv_iobuf;
  struct __rics_state *pRics_state=NULL;
  int *connect_err=(int*)p;
  (void) p;
  
  switch (ev) {
    case NS_CONNECT:
	if(!*connect_err){
		printf("rics_serial(%p) connected OK=%d\n",nc,!*connect_err);	
		if(!cp||!cp->ricsConn)return;// to protect proc_delayed_serialSend
		pRics_state=&cp->ricsConn->rics_state;
		if(pRics_state){pRics_state->serial_connected=1;pRics_state->serial_num_user=1;}
		ns_tcpSetKeepAlive(nc, 1, 200, 5, 100);// 1 (KEEPALIVE ON),최초에 세션 체크 시작하는 시간 (단위 : sec),최초에 세션 체크 패킷 보낸 후, 응답이 없을 경우 다시 보내는 횟수 (단위 : 양수 단수의 갯수),TCP_KEEPIDLE 시간 동안 기다린 후, 패킷을 보냈을 때 응답이 없을 경우 다음 체크 패킷을 보내는 주기 (단위 : sec)
		proc_delayed_serialSend(cp->ricsConn,nc);
	}
	else{
		printf("rics_serial(%p) connection FAILED=%d\n",nc,*connect_err);	
	}
	break;
    case NS_CLOSE://network broken!!
       // If either connection closes, unlink them and schedule closing
	printf("rics_serial(%p) closed.\n",nc);
	if(!cp||!cp->ricsConn)return;
	else{
		cp->ricsConn->serial_nc=NULL;cp->ricsConn->logEnTime=0;
		pRics_state=&cp->ricsConn->rics_state;
		if(pRics_state){pRics_state->serial_connected=0;pRics_state->serial_num_user=0;}
		free(cp);
		rprint_err("(%p)serial remote disconnect!\n",nc);
	}
      nc->user_data = NULL;
      break;

    case NS_RECV:
	if(io->len>0&&io->buf){
		int rc=io->len;unsigned int recvPos=0;

		if(!cp||!cp->ricsConn)return;// to protect proc_recv_rics_msg
		do{
		rc=proc_recv_rics_msg(&cp->rics_serial_rx_arg, io->buf+recvPos, io->len-recvPos,&recvPos);
		if(rc > 0)
		{
			tcp_msg_t *pTcp_msg;
			MA_SE_Serial_Data_RPT_t *ma_se_serial;

			pTcp_msg = (tcp_msg_t *)cp->rics_serial_rx_arg.cur_rx_buf;
			ma_se_serial = (MA_SE_Serial_Data_RPT_t *)pTcp_msg->msg;

			if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)
			{
				int i,msglen=ma_se_serial->header.msg_len;
				
				if(msglen!=250)rprint_dbg("serial->ems(%d):", msglen);
				for(i=0;i<msglen;i++)
				{
					if(ma_se_serial->data.serial_data[i]!='\r'&&ma_se_serial->data.serial_data[i]!='\n'&&ma_se_serial->data.serial_data[i]!='\b'&&(ma_se_serial->data.serial_data[i]<0x20||ma_se_serial->data.serial_data[i]>0x7e))
						rprint_rel_c("\\x%02x", ma_se_serial->data.serial_data[i]);
					else
						rprint_rel_c("%c", ma_se_serial->data.serial_data[i]);
				}
				if(msglen!=250)rprint_dbg("\n");
			}

			// serial --> ems
			if(ma_se_serial->header.msg_type == SERIAL_A_DATA_MA_SE)
			{
				if(!memcmp((char*)ma_se_serial->data.serial_data,"RICS[node",9)){// RICS[node 0, A_comm_num 0, baud 115200]\n
					//rics_module_connect_serial(sock_name);// cp->device,'A'
					printf("rics_module_connect_serial(%s,%c):'%.*s'\n",cp->device,'A',ma_se_serial->header.msg_len,(char*)ma_se_serial->data.serial_data);
				}
				else{
					log_RICS_serialRecv(cp->device,'A',(char*)ma_se_serial->data.serial_data, ma_se_serial->header.msg_len);
					// my_send_to_uds(rics_serial_A_ipc_sock, ma_se_serial->data.serial_data, ma_se_serial->header.msg_len, pEms_serial_A_ipc_path);
					serialData2Websocket(nc->mgr,cp->device,'A',(char*)ma_se_serial->data.serial_data, ma_se_serial->header.msg_len);
				}
			}
			else if(ma_se_serial->header.msg_type == SERIAL_B_DATA_MA_SE)
			{
				if(!memcmp((char*)ma_se_serial->data.serial_data,"RICS[node",9)){// RICS[node 0, B_comm_num 1, baud 115200]\n
					//rics_module_connect_serial(sock_name);// cp->device,'B'
					printf("rics_module_connect_serial(%s,%c):'%.*s'\n",cp->device,'B',ma_se_serial->header.msg_len,(char*)ma_se_serial->data.serial_data);
				}
				else{
					log_RICS_serialRecv(cp->device,'B',(char*)ma_se_serial->data.serial_data, ma_se_serial->header.msg_len);
					// my_send_to_uds(rics_serial_B_ipc_sock, ma_se_serial->data.serial_data, ma_se_serial->header.msg_len, pEms_serial_B_ipc_path);
					serialData2Websocket(nc->mgr,cp->device,'B',(char*)ma_se_serial->data.serial_data, ma_se_serial->header.msg_len);
				}
			}
			else
			{
				// msg_type error
				rprint_err("serial msg_type error! %02x\n", ma_se_serial->header.msg_type);
			}
		}
		else if(rc < 0)
		{
			rprint_err("serial remote disconnect(detect recv err.)!\n");
			goto out;
		}
		}while(recvPos<io->len);
 	}
        iobuf_remove(io, io->len);
      break;
    case NS_SEND:
	// if(cp&&*p_n>0)cp->numRxComp++;// -1 on connectErr!!
      //printf("pS%d.",*p_n);// OK(num sent!!)
      break;
    case NS_POLL:
	// printf("pP.");// printf("target(%p) polling\n",nc);	
   	break;
    default:
      break;
  }
out: ;
}
int get_serialTcp_connector_state(struct ns_mgr* mgr,const char* device_id,int* p_serial_num_user)//return serial_connected
{(void)mgr;
#if INCLUDE_SERIAL_CON
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);

	if(rc){
		struct __rics_state *p_rics_state=&rc->rics_state;
		
		if(rc->serial_nc){
			if(p_serial_num_user)*p_serial_num_user=p_rics_state->serial_num_user;
			return p_rics_state->serial_connected;
		}
	}
#endif	
	if(p_serial_num_user)*p_serial_num_user=0;
	return 0;
}
int refmod_serialTcp_connector_state(struct ns_mgr* mgr,const char* device_id,int serial_num_user_inc)//if serial_connected//return serial_connected
{(void)mgr;(void)device_id;(void)serial_num_user_inc;
#if INCLUDE_SERIAL_CON
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);

	if(rc){
		struct __rics_state *p_rics_state=&rc->rics_state;
		
		if(rc->serial_nc&&p_rics_state->serial_connected){
			if((serial_num_user_inc<0&&p_rics_state->serial_num_user>0)||(serial_num_user_inc>0&&p_rics_state->serial_num_user<20)){
				p_rics_state->serial_num_user+=serial_num_user_inc;
				printf("refmod_serialTcp_connector_state(%s): user=%d\n",device_id,p_rics_state->serial_num_user);
			}
			return p_rics_state->serial_connected;
		}
	}
#endif	
	return 0;
}

void start_serialTcp_connector(struct ns_mgr* mgr,const char* device_id)
{
#if INCLUDE_SERIAL_CON
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);
	char serialAddr[PATH_MAX];
	ricsSerial_conn_param* cp=NULL;

	if(!rc)return;
	if(rc->serial_nc)return;
	cp=(ricsSerial_conn_param*)calloc(1,sizeof(ricsSerial_conn_param));// RMCP_RICS_SERIAL
	if(!cp)return;
	cp->cpType=RMCP_RICS_SERIAL;
	strcpy(cp->device,device_id);
	cp->ricsConn=rc;
	sprintf(serialAddr,"%s:%d",device_id,rics_port+1);// rics_serial_port!!
	printf("opening serial to %s:%d\n",device_id,rics_port+1);fflush(stdout);// rics_serial_port!!
#ifdef NS_FOSSA_VERSION
	{
	struct ns_connect_opts opts;
	
	opts.user_data=cp;//opts.error_string;opts.flags;
	rc->serial_nc=ns_connect_opt(mgr, serialAddr, ricsmanSerial_handler, opts);// connect to RICS(SerialTCP) 
	}
#else
	rc->serial_nc=ns_connect(mgr,serialAddr,ricsmanSerial_handler,cp);// connect to RICS(SerialTCP)
#endif					
	if(!rc->serial_nc)free(cp);
#else
	(void)mgr;(void)device_id;
#endif
}
void stop_serialTcp_connector(struct ns_mgr* mgr,const char* device_id)
{
#if INCLUDE_SERIAL_CON
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);
	(void)mgr;

	if(!rc)return;
	rc->rics_state.serial_connected=0;
	if(!rc->serial_nc)return;
	printf("closing serial from %s:%d\n",device_id,rics_port+1);fflush(stdout);// rics_serial_port!!
	{
		ricsSerial_conn_param* cp= (ricsSerial_conn_param*) rc->serial_nc->user_data;
		
		rc->serial_nc->user_data=NULL;
		free(cp);
		NS_close_conn(rc->serial_nc);// to RICS
	}
	rc->serial_nc=NULL;rc->logEnTime=0;
#else
	(void)mgr;(void)device_id;
#endif
}
#endif // INCLUDE_SERIAL_CON
#if INCLUDE_RICS_TCP_CON
int rics_module_sendTo_TCP(void *ctxFrom,const char* device_id,const char* data,int dataLen);
int tcp__connect_level_rpt(ricsConn* ricsConn, int user_level)
{int ret=0;
	// send control user level
	ret=__connect_level_rpt((ncSock_t)ricsConn->ricsTcp_nc, user_level);
	return ret;
}

// int recv_rics_msg(struct __rics_msg_rx_arg *arg) not considered!!
static void ricsconnRicsTcp_handler(struct ns_connection *nc, int ev, void *p) 
{//TCP/UDP socket Handler(target)
  ricsCtrl_conn_param* cp= (ricsCtrl_conn_param*)nc->user_data;// TCP for a RICS
  struct __rics_state *pRics_state=NULL;
  struct iobuf *io = &nc->recv_iobuf;
  int *connect_err=(int*)p;
  (void) p;
  
  switch (ev) {
    case NS_CONNECT:
	if(!*connect_err){
		printf("RicsTcp(%p/cp%p) connected OK=%d\n",nc,cp,!*connect_err);	
		if(!cp||!cp->ricsConn)return;
		pRics_state=&cp->ricsConn->rics_state;
		ns_tcpSetKeepAlive(nc, 1, 100, 5, 50);// 1 (KEEPALIVE ON),최초에 세션 체크 시작하는 시간 (단위 : sec),최초에 세션 체크 패킷 보낸 후, 응답이 없을 경우 다시 보내는 횟수 (단위 : 양수 단수의 갯수),TCP_KEEPIDLE 시간 동안 기다린 후, 패킷을 보냈을 때 응답이 없을 경우 다음 체크 패킷을 보내는 주기 (단위 : sec)
		// send control user level
		tcp__connect_level_rpt(cp->ricsConn, CONNECT_STATE_TCP_AGW);
	}
	else{
		printf("RicsTcp(%p/cp%p) connection FAILED=%d\n",nc,cp,*connect_err);	
		if(!cp||!cp->ricsConn)return;
		cp->ricsConn->ricsTcp_nc=NULL;
		if(cp)free(cp);
		nc->user_data=NULL;
	}
	break;
    case NS_CLOSE://network broken!!
       // If either connection closes, unlink them and schedule closing
	printf("RicsTcp(%p/cp%p) closed.\n",nc,cp);
	if(!cp||!cp->ricsConn){;}//return;
	else {
		pRics_state=&cp->ricsConn->rics_state;
		pRics_state->start_up=0;
		cp->ricsConn->ricsTcp_nc=NULL;
		rprint_err("(%p)rics ctrl remote disconnect!\n", nc);
	}
	if(cp)free(cp);
	nc->user_data = NULL;
	break;

    case NS_RECV:
	if(io->len>0&&io->buf){
		int rc=io->len;unsigned int recvPos=0;

		if(!cp||!cp->ricsConn)return;// to protect proc_recv_rics_msg
		do{
		rc=proc_recv_rics_msg(&cp->rics_ctrl_rx_arg, io->buf+recvPos, io->len-recvPos,&recvPos);
		if(rc > 0)
		{
			int res_msg_type=0;
			tcp_msg_t *pTcp_msg=NULL;
			char ipc_buf[RICS_IPC_BUF_MAX];int ipc_buf_len=0;
			struct __rics_state *pRics_state=NULL;
			(void)res_msg_type;

			if(io->len<io->size)io->buf[io->len]=0;//NULL termination
			
			rprint_dbg("RX_TCP: [%d] nc%p,cp%p\n", rc, nc, cp);
			if(!cp||!cp->ricsConn)return;
			pRics_state=&cp->ricsConn->rics_state;

			pTcp_msg = (tcp_msg_t *)cp->rics_ctrl_rx_arg.cur_rx_buf;

			res_msg_type = do_msg_parsing(pTcp_msg, ipc_buf, RICS_IPC_BUF_MAX, pRics_state);
#if 1		
			if(res_msg_type==START_UP_RPT)printf("ricsconnRicsTcp_handler%p: START_UP_RPT received from '%s'\n",cp,cp->device);
			if(res_msg_type==START_UP_RPT&&cp->ricsConn->sendDataLen>0&&pRics_state->start_up){// res_msg_type&&START_UP_RPT
				printf("start delayed TCP send(%d) to %s(on startUp%d)\n",cp->ricsConn->sendDataLen,cp->device,pRics_state->start_up);
				rics_module_sendTo_TCP(nc,cp->device,cp->ricsConn->sendData,cp->ricsConn->sendDataLen);
				cp->ricsConn->sendData[0]=0;cp->ricsConn->sendDataLen=0;
			}
#endif				
			if(ipc_buf[0] != 0)
			{
				ipc_buf_len = strlen(ipc_buf);
				rprint_dbg("TX_ipc(%d): %s\n", ipc_buf_len, ipc_buf);

				// my_send_to_uds(rics_ctrl_ipc_sock, pArg->ipc_buf, pArg->ipc_buf_len, pArg->pMain_thread_arg->rics_ipc_path.main);
				rics_request_reply_msg(nc->mgr,cp->device,ipc_buf, ipc_buf_len);
			}
		}
		else if(rc < 0)
		{
			rprint_err("RicsTcp: remote disconnect!\n");
			if(!cp||!cp->ricsConn){;}//return;
			{
				pRics_state=&cp->ricsConn->rics_state;
				pRics_state->start_up=0;
			}
			goto out;
		}
		}while(recvPos<io->len);		
 	}
        iobuf_remove(io, io->len);
      break;
    case NS_SEND:
	// if(cp&&*p_n>0)cp->numRxComp++;// -1 on connectErr!!
      //printf("pS%d.",*p_n);// OK(num sent!!)
      break;
    case NS_POLL:
	// printf("pP.");// printf("target(%p) polling\n",nc);	
   	break;
    default:
      break;
  }
out: ;
}
void start_ricsTcp_connector(struct ns_mgr* mgr,const char* device_id)
{
#if INCLUDE_RICS_TCP_CON
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);
	char destAddr[PATH_MAX];
	ricsCtrl_conn_param* cp=NULL;
		
	if(!rc)return;
	if(rc->ricsTcp_nc)return;
	
	cp=(ricsCtrl_conn_param*)calloc(1,sizeof(ricsCtrl_conn_param));// RMCP_RICS_TCP
	if(!cp)return;
	
	cp->cpType=RMCP_RICS_TCP;
	strcpy(cp->device,device_id);
	cp->ricsConn=rc;
	sprintf(destAddr,"%s:%d",device_id,rics_port);
#ifdef NS_FOSSA_VERSION
	{
	struct ns_connect_opts opts;
	
	opts.user_data=cp;//opts.error_string;opts.flags;
	rc->ricsTcp_nc=ns_connect_opt(mgr, destAddr, ricsconnRicsTcp_handler, opts);// connect to RICS(TCP) 
	}
#else
	rc->ricsTcp_nc=ns_connect(mgr, destAddr, ricsconnRicsTcp_handler, cp);// connect to RICS(TCP)
#endif					
	if(!rc->ricsTcp_nc)free(cp);
	printf("start_ricsTcp_connector:%p to %s\n",rc->ricsTcp_nc,destAddr);
#else
	(void)mgr;(void)device_id;
#endif
}
void stop_ricsTcp_connector(struct ns_mgr* mgr,const char* device_id)
{
#if INCLUDE_RICS_TCP_CON
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);
	(void)mgr;
	
	if(!rc)return;
	if(!rc->ricsTcp_nc)return;
	
	NS_close_conn(rc->ricsTcp_nc);// to RICS
	rc->ricsTcp_nc=NULL;
#else
	(void)mgr;(void)device_id;
#endif
}

int rics_module_sendTo_TCP(void *ctxFrom,const char* device_id,const char* data,int dataLen)
{(void)ctxFrom;
	ricsConn *ricsconn=(ricsConn*)find_ricsconn(device_id);
	if(!ricsconn||!ricsconn->ricsTcp_nc){
		if(!ricsconn)printf("%s error\n",!ricsconn? "no_ricsConn":"no_ricsTCP");
		return 0;
	}
	printf("rics_module_sendTo_TCP(%s):%d bytes using %p\n",device_id,dataLen,ricsconn->ricsTcp_nc);
	return ns_send(ricsconn->ricsTcp_nc,data,dataLen);
}
int delayed_rics_module_sendTo_TCP(struct ns_connection *ctxFrom,const char* device_id,const char* data,int dataLen)
{
	ricsConn *ricsconn=(ricsConn*)find_ricsconn(device_id);

	if(!ctxFrom)return -98;
	if(!ricsconn||!ricsconn->ricsTcp_nc){
		if(!ricsconn)printf("%s error\n",!ricsconn? "no_ricsConn":"no_ricsTCP");
		printf("delayed_rics_module_sendTo_TCP(%s): connection closed!! try-reconnection!!\n",device_id);
		ricsconn->sendDataLen=dataLen;memcpy(ricsconn->sendData,data,dataLen);
		start_ricsTcp_connector(((struct ns_connection *)ctxFrom)->mgr,device_id);
		printf("delayed_rics_module_sendTo_TCP(%s): %p created.\n",device_id,ricsconn->ricsTcp_nc);
		return -99;// delayed send
	}
	printf("delayed_rics_module_sendTo_TCP(%s): %d bytes using %p\n",device_id,dataLen,ricsconn->ricsTcp_nc);
	return ns_send(ricsconn->ricsTcp_nc,data,dataLen);
}
int rics_module_sendTo_TCP_ipcstr(void *ctxFrom,const char* device_id,const char* pIpc_recv_data)
{
	int rc=0;
	int tcp_msg_len=0;
	tcp_msg_t tcp_msg;
	const char *pRics_ip=device_id;
	struct __rics_state *pRics_state;
	
	ricsConn *ricsconn=(ricsConn*)find_ricsconn(device_id);
	if(!ricsconn){
		printf("rics_module_sendTo_TCP_ipcstr(%s): RICS CONN not found\n",device_id);
		return 0;
	}
	
	pRics_state=&ricsconn->rics_state;
	
	// convert ems_msg to rics_msg
	tcp_msg_len = convert_ems_ipc_to_rics_ctrl_msg((unsigned char*)pIpc_recv_data, (void *)&tcp_msg);
	if(tcp_msg_len > 0)
	{
		// TCP
		if(pRics_state->start_up == 0)
		{
			return delayed_rics_module_sendTo_TCP(ctxFrom,device_id,(const char*)&tcp_msg, tcp_msg_len);
		}

		// send msg to rics
		// rc = send(rics_tcp_sock, (void *)&tcp_msg, tcp_msg_len, 0);
		rc = rics_module_sendTo_TCP(ctxFrom,device_id,(const char*)&tcp_msg, tcp_msg_len);
		if(rc != tcp_msg_len)
		{
			if(!ricsconn||!ricsconn->ricsTcp_nc){
				if(!ricsconn)printf("%s error\n",!ricsconn? "no_ricsConn":"no_ricsTCP");
				// return 0;
			}
			else printf("%p: %s error\n",(!ricsconn||!ricsconn->ricsTcp_nc? NULL:ricsconn->ricsTcp_nc),!ricsconn? "no_ricsConn":"send_ricsTCP");
			rprint_err("(%s)tcp send error! %d/%d\n", pRics_ip, rc, tcp_msg_len);
		}
	}
	else{
		printf("rics_module_sendTo_TCP_ipcstr(%s):%d bytes?? FAILED using %p(ipc_data='%s')\n",device_id,tcp_msg_len,ricsconn->ricsTcp_nc,pIpc_recv_data);
	}
	return rc;
}
#endif /*INCLUDE_RICS_TCP_CON*/

#if INCLUDE_RICS_UDP_CON
int udp_connection_check_timeout(ricsConn* ricsConn, time_t cur_time)
{int ret=0;
	if(ricsConn->heartbeat_timerActive&&(cur_time - ricsConn->heartbits_send_time) >= 5)
	{
#if INCLUDE_RICS_UDP_MASTER	
		void *nc=g_ricsMan_cp->ricsMasterUdp_nc;
#else
		void *nc=ricsConn->ricsUdp_nc;
#endif
		(void)nc;

		if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("udp_connection_check_timeout:%s\n",ricsConn->device_id);
		ricsConn->heartbeat_timerActive=0;
		// check heartbeat timeout
		if(!ricsConn->heartbeat_received){// OnTimeout!!
			int bOldState=ricsConn->heartbeat_alive;

			if(++ricsConn->heatbeat_lostCnt>=3)ricsConn->heartbeat_alive=0;
			if(ricsConn->heartbeat_alive!=bOldState){
				OnDeviceDeparted(ricsConn->rics_udp_sockaddr.sin_addr.s_addr,ntohs(ricsConn->rics_udp_sockaddr.sin_port),ricsConn->device_id);// RICS_disconnected
				ret=-1;
			}
		}
	}
	return ret;
}
int udp_connection_req(ricsConn* ricsConn, struct sockaddr_in *pTo_addr,time_t cur_time)
{int ret=0;
	if((cur_time - ricsConn->heartbits_send_time) >= RICS_UDP_PING_EVERY_SEC)
	{
#if INCLUDE_RICS_UDP_MASTER	
		void *nc=g_ricsMan_cp->ricsMasterUdp_nc;
#else
		void *nc=ricsConn->ricsUdp_nc;
#endif
		if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("__udp_connection_req:%s\n",ricsConn->device_id);
		// check heartbeat
		ricsConn->heartbeat_received=0;ricsConn->heartbeat_timerActive=1;// on Send!!
		ret=__udp_connection_req((ncSock_t)nc, pTo_addr);
		ricsConn->heartbits_send_time = cur_time;
	}
	return ret;
}
int udp_serial_timeout_refresh_req(ricsConn* ricsConn, struct sockaddr_in *pTo_addr,time_t cur_time)
{int ret=0;
	if((cur_time - ricsConn->serial_timeout_refresh_time) >= RICS_SERIAL_REFRESH_EVERY_SEC)
	{
#if INCLUDE_RICS_UDP_MASTER	
		void *nc=g_ricsMan_cp->ricsMasterUdp_nc;
#else
		void *nc=ricsConn->ricsUdp_nc;
#endif
		if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("__udp_serial_timeout_refresh_req:%s\n",ricsConn->device_id);
		// serial session idle timeout refresh
		ret=__udp_serial_timeout_refresh_req((ncSock_t)nc, pTo_addr);
		ricsConn->serial_timeout_refresh_time = cur_time;
	}
	return ret;
}
int find_conn_func_PING(void* map,const char* device_id,void* ricsconn,void* Ctx,void* Ctx2,void* Ctx3)
{(void)map;(void)Ctx3;
	time_t cur_time=(time_t)Ctx;int* pCount=(int*)Ctx2;

	if(ricsconn){
		int ret=0;
		ricsConn *rc=(ricsConn*)ricsconn;

		ret=udp_connection_check_timeout(rc,cur_time);
		// printf("find_conn_func_PING:%s nTh timeRet=%d\n",device_id,ret);fflush(stdout);
		ret=udp_connection_req(rc, &rc->rics_udp_sockaddr,cur_time);
		// printf("find_conn_func_PING:%s nTh sendRet=%d\n",device_id,ret);fflush(stdout);
		if(ret&&pCount)(*pCount)++;

		if(rc->rics_state.serial_connected == 1)
		{
			udp_serial_timeout_refresh_req(rc, &rc->rics_udp_sockaddr,cur_time);
		}
	}
	else{
		printf("find_conn_func_PING:%s already removed\n",device_id);fflush(stdout);//impossible
	}
	return 0;//do-not stop Looping
}
int find_conn_func_PING0(void* map,const char* device_id,void* ricsconn,void* Ctx,void* Ctx2,void* Ctx3)
{(void)map;(void)Ctx3;
	time_t cur_time=(time_t)Ctx;int* pCount=(int*)Ctx2;

	if(ricsconn){
		int ret=0;
		ricsConn *rc=(ricsConn*)ricsconn;
		
		rc->heartbits_recv_time=cur_time;
		ret=udp_connection_req(rc, &rc->rics_udp_sockaddr,rc->heartbits_recv_time);
		printf("find_conn_func_PING0: %s RicsUdp(@%x) 1st sendRet=%d\n",device_id,(unsigned int)cur_time,ret);	

		if(ret&&pCount)(*pCount)++;
	}
	else{
		printf("find_conn_func_PING0: %s already removed\n",device_id);fflush(stdout);//impossible
	}
	return 0;//do-not stop Looping
}

#if INCLUDE_RICS_UDP_MASTER
static void ricsmanRicsUdp_handler(struct ns_connection *nc, int ev, void *p) 
{//TCP/UDP socket Handler(target)
  ricsMan_master_struct* cp= (ricsMan_master_struct*)nc->user_data;// UDP for mananger// same as g_ricsMan_cp
  struct iobuf *io = &nc->recv_iobuf;
  int *connect_err=(int*)p;
  
  switch (ev) {
    case NS_CONNECT:
	if(!*connect_err){
		printf("RicsUdpALL(%p/cp%p) connected OK=%d\n",nc,cp,!*connect_err);	
		if(cp)
		{time_t curtime;int count=0,ret=0;
		
			time(&curtime);
			ret=iterateAll_ricsconn(find_conn_func_PING0,(void*)curtime,&count,0);
			if(count>0)printf("RicsUdpALL(%p): %d target UDP ping0 of %d\n",nc,count,ret);
			g_ricsMan_cp->last_check_time=curtime;
		}
	}
	else{
		printf("RicsUdpALL(%p/cp%p) connection FAILED=%d\n",nc,cp,*connect_err);	
	}
	break;
    case NS_CLOSE://network broken!!
       // If either connection closes, unlink them and schedule closing
	printf("+RicsUdpALL(%p/cp%p) closed\n",nc,cp);fflush(stdout);
	if(cp)free(cp);
	printf("-RicsUdpALL(%p/cp%p) closed.\n",nc,cp);fflush(stdout);
       nc->user_data = NULL;
      break;

    case NS_RECV:
	if(io->len>0&&io->buf){
		int rc=io->len;char* cur_rx_buf=io->buf;
		if(rc > 0)
		{
			int res_msg_type=0;
			udp_msg_t *pUdp_msg=NULL;
			unsigned char msgBuf[sizeof(udp_msg_t)+1024];
			char ipc_buf[RICS_IPC_BUF_MAX];int ipc_buf_len=0;
			(void)res_msg_type;(void)cur_rx_buf;
			char device_id[PATH_MAX];
			ricsConn* ricsconn=NULL;
			struct __rics_state *pRics_state=NULL;

			if(!cp)return;
			if(io->len<io->size)io->buf[io->len]=0;//NULL termination
			memcpy(msgBuf,io->buf,io->len);memset(msgBuf+io->len,0,sizeof(unsigned long));

			strcpy(device_id,inet_ntoa(nc->sa.sin.sin_addr));
			rprint_dbg("RX_UDP_ALL: [%d] nc%p,cp%p from %s:%d\n", rc, nc, cp,device_id,ntohs(nc->sa.sin.sin_port));
			ricsconn=(ricsConn*)find_ricsconn(device_id);
			if(!cp||!ricsconn)return;
			pRics_state=&ricsconn->rics_state;
			
			pUdp_msg=(udp_msg_t*)msgBuf;
			
			res_msg_type = do_msg_parsing((tcp_msg_t *)pUdp_msg, ipc_buf, RICS_IPC_BUF_MAX, pRics_state);
			if(res_msg_type==CONNECTION_RPT){
				int bOldState=ricsconn->heartbeat_alive;
				ricsconn->heatbeat_lostCnt=0;ricsconn->heartbeat_alive=1;ricsconn->heartbeat_received=1;ricsconn->heartbeat_timerActive=0;// OnReceived
				if(ricsconn->heartbeat_alive!=bOldState){
					OnDeviceArrived(nc->sa.sin.sin_addr.s_addr,ntohs(nc->sa.sin.sin_port),device_id);// -> RICS_connected
				}
			}
			if(ipc_buf[0] != 0)
			{
				ipc_buf_len = strlen(ipc_buf);
				rprint_dbg("TX_ipc_all(%d): %s\n", ipc_buf_len, ipc_buf);
				
				// my_send_to_uds(rics_ctrl_ipc_sock, pArg->ipc_buf, pArg->ipc_buf_len, pArg->pMain_thread_arg->rics_ipc_path.main);
				rics_request_reply_msg(nc->mgr,inet_ntoa(nc->sa.sin.sin_addr),ipc_buf, ipc_buf_len);
			}
		}
		else if(rc < 0)
		{
			rprint_err("RicsUdpALL: remote disconnect!\n");
			goto out;
		}
 	}
        iobuf_remove(io, io->len);
      break;
    case NS_SEND:
	// if(cp&&*p_n>0)cp->numRxComp++;// -1 on connectErr!!
      //printf("pS%d.",*p_n);// OK(num sent!!)
      break;
    case NS_POLL:
	// printf("pP@%u.",*(time_t*)p);// printf("target(%p) polling\n",nc);	
	if(cp)
	{	
		time_t curtime=*(time_t*)p;

		if(!g_ricsMan_cp)return;
		if(curtime-g_ricsMan_cp->last_check_time>=5){//check every 5 sec for 5 sec timeout!!
			int count=0,ret;

			// printf("all find_conn_func_PING@%u.",(unsigned int)*(time_t*)p);
			ret=iterateAll_ricsconn(find_conn_func_PING,(void*)curtime,&count,0);
			if(count>0)if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("%d target UDP poll of %d\n",count,ret);
			g_ricsMan_cp->last_check_time=curtime;
		}
	}
   	break;
    default:
      break;
  }
out: ;
}
#else
static void ricsconnRicsUdp_handler(struct ns_connection *nc, int ev, void *p) 
{// UDP socket Handler(target)
  ricsHBT_conn_param* cp= (ricsHBT_conn_param*) nc->user_data;// UDP for a RICS
  struct iobuf *io = &nc->recv_iobuf;
  int *connect_err=(int*)p;// NS_CONNECT
  (void) p;
  
  switch (ev) {
    case NS_CONNECT:
	if(!*connect_err){
		int count=0,ret=0;time_t curtime;// int sendRet=0;
		
		printf("RicsUdp(%p/cp%p) connected OK=%d\n",nc,cp,!*connect_err);	
		if(!cp||!cp->ricsConn)return;
		
		time(&curtime);
		ret=find_conn_func_PING0(g_ricsConnByIPmap,cp->device,cp->ricsConn,(void*)curtime,&count);
		printf("RicsUdp(%p) 1st sendRet=%d\n",nc,ret);	
	}
	else{
		printf("RicsUdp(%p/cp%p) connection FAILED=%d\n",nc,cp,*connect_err);	
	}
	break;
    case NS_CLOSE://network broken!!
       // If either connection closes, unlink them and schedule closing
	printf("RicsUdp(%p/cp%p) closed.\n",nc,cp);
	if(cp)free(cp);
       nc->user_data = NULL;
      break;

    case NS_RECV:
	if(io->len>0&&io->buf){
		int rc=io->len;char* cur_rx_buf=io->buf;
		if(rc > 0)
		{
			int res_msg_type=0;
			udp_msg_t *pUdp_msg=NULL;
			unsigned char msgBuf[sizeof(udp_msg_t)+1024];
			char ipc_buf[RICS_IPC_BUF_MAX];int ipc_buf_len=0;
			ricsConn* ricsconn=NULL;
			struct __rics_state *pRics_state=NULL;
			(void)res_msg_type;(void)cur_rx_buf;

			if(io->len<io->size)io->buf[io->len]=0;//NULL termination
			memcpy(msgBuf,io->buf,io->len);memset(msgBuf+io->len,0,sizeof(unsigned long));
			
			rprint_dbg("RX_UDP: [%d] nc%p,cp%p\n", rc, nc, cp);
			ricsconn=cp->ricsConn;
			if(!cp||!cp->ricsConn)return;
			pRics_state=&cp->ricsConn->rics_state;
			
			pUdp_msg=(udp_msg_t*)msgBuf;
			
			res_msg_type = do_msg_parsing((tcp_msg_t *)pUdp_msg, ipc_buf, RICS_IPC_BUF_MAX, pRics_state);
			if(res_msg_type==CONNECTION_RPT){
				int bOldState=ricsconn->heartbeat_alive;
				ricsconn->heatbeat_lostCnt=0;ricsconn->heartbeat_alive=1;ricsconn->heartbeat_received=1;ricsconn->heartbeat_timerActive=0;// OnReceived
				if(ricsconn->heartbeat_alive!=bOldState){
					OnDeviceArrived(nc->sa.sin.sin_addr.s_addr,ntohs(nc->sa.sin.sin_port),cp->device);// -> RICS_connected
				}
			}
			if(ipc_buf[0] != 0)
			{
				ipc_buf_len = strlen(ipc_buf);
				rprint_dbg("TX_ipc(%d): %s\n", ipc_buf_len, ipc_buf);
				
				// my_send_to_uds(rics_ctrl_ipc_sock, pArg->ipc_buf, pArg->ipc_buf_len, pArg->pMain_thread_arg->rics_ipc_path.main);
				rics_request_reply_msg(nc->mgr,cp->device,ipc_buf, ipc_buf_len);
			}
		}
		else if(rc < 0)
		{
			rprint_err("RicsUdp: remote disconnect!\n");
			goto out;
		}
 	}
        iobuf_remove(io, io->len);
      break;
    case NS_SEND:
	// if(cp&&*p_n>0)cp->numRxComp++;// -1 on connectErr!!
      //printf("pS%d.",*p_n);// OK(num sent!!)
      break;
    case NS_POLL:
	// printf("pP.");// printf("target(%p) polling\n",nc);	
	{	time_t* pNow=(time_t*)p;int count=0,ret=0;(void)ret;

		if(!cp||!cp->ricsConn)return;
//		rprint_dbg("rics : ctrl select timeout!\n");
		ret=find_conn_func_PING(g_ricsConnByIPmap,cp->device,cp->ricsConn,(void*)*pNow,&count);
	}
   	break;
    default:
      break;
  }
out: ;
}
#endif
void start_ricsUdp_connector(struct ns_mgr* mgr,const char* device_id)
{
#if INCLUDE_RICS_UDP_CON
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);
	char destAddr[PATH_MAX];

	if(!rc)return;
	if(rc->ricsUdp_nc)return;
	
	sprintf(destAddr,"udp://%s:%d",device_id,rics_port);
	make_ip_peerAddr(device_id,rics_port, &rc->rics_udp_sockaddr);
	rc->heatbeat_lostCnt=0;rc->heartbeat_timerActive=rc->heartbeat_received=0;// init!!
	rc->heartbeat_alive=-1;// unknown
#if INCLUDE_RICS_UDP_MASTER
	rc->ricsUdp_nc=g_ricsMan_cp->ricsMasterUdp_nc;
	{int ret=0,count=0;time_t curtime;(void)mgr;
		time(&curtime);
		ret=find_conn_func_PING0(g_ricsConnByIPmap,device_id,rc,(void*)curtime,&count,mgr);
		printf("start_ricsUdp_connector(%p) 1st sendRet=%d\n",rc,ret);	
	}
#else
	{
		ricsHBT_conn_param* cp=NULL;
		cp=(ricsHBT_conn_param*)calloc(1,sizeof(ricsHBT_conn_param));// RMCP_RICS_UDP
		if(!cp)return;
		
		cp->cpType=RMCP_RICS_UDP;
		strcpy(cp->device,device_id);
		cp->ricsConn=rc;
#ifdef NS_FOSSA_VERSION
		{
		struct ns_connect_opts opts;
		
		opts.user_data=cp;//opts.error_string;opts.flags;
		rc->ricsUdp_nc=ns_connect_opt(mgr, destAddr, ricsconnRicsUdp_handler, opts);// connect to RICS(UDP) 
		}
#else
		rc->ricsUdp_nc=(void*)ns_connect(mgr, destAddr, ricsconnRicsUdp_handler, cp);// connect to RICS(UDP)
#endif					
		if(!rc->ricsUdp_nc)free(cp);
		else{
			int connect_err=0;
			
			printf("start_ricsUdp_connector:%p(cp=%p) to %s\n",rc->ricsUdp_nc,cp,destAddr);
			ricsconnRicsUdp_handler(rc->ricsUdp_nc,NS_CONNECT,&connect_err);// simulate TCP connect
		}
	}
#endif	
#else
	(void)mgr;(void)device_id;
#endif
}
void stop_ricsUdp_connector(struct ns_mgr* mgr,const char* device_id)
{
#if INCLUDE_RICS_UDP_CON
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);
	(void)mgr;

	if(!rc)return;
	if(!rc->ricsUdp_nc)return;
	
#if INCLUDE_RICS_UDP_MASTER
	;
#else
	NS_close_conn(rc->ricsUdp_nc);// to RICS
#endif
	rc->ricsUdp_nc=NULL;
#else
	(void)mgr;(void)device_id;
#endif
}

int rics_module_sendTo_UDP(void *ctxFrom,const char* device_id,const char* data,int dataLen)
{(void)ctxFrom;
	ricsConn *ricsconn=(ricsConn*)find_ricsconn(device_id);
	
#if INCLUDE_RICS_UDP_MASTER	
	if(!ricsconn||!g_ricsMan_cp->ricsMasterUdp_nc){
		if(!ricsconn)printf("%s error\n",!ricsconn? "no_ricsConn":"no_ricUDP");
		return 0;
	}
	return ns_sendto(g_ricsMan_cp->ricsMasterUdp_nc,(union socket_address*)&ricsconn->rics_udp_sockaddr,data,dataLen);
#else
	if(!ricsconn||!ricsconn->ricsUdp_nc){
		if(!ricsconn)printf("%s error\n",!ricsconn? "no_ricsConn":"no_ricUDP");
		return 0;
	}
	return ns_send(ricsconn->ricsUdp_nc,data,dataLen);
#endif
}
int rics_module_sendTo_UDP_ipcstr(void *ctxFrom,const char* device_id,const char* pIpc_recv_data)
{
	int rc=0;
	int tcp_msg_len=0;
	tcp_msg_t tcp_msg;
	
	// convert ems_msg to rics_msg
	tcp_msg_len = convert_ems_ipc_to_rics_ctrl_msg((unsigned char*)pIpc_recv_data, (void *)&tcp_msg);
	if(tcp_msg_len > 0)
	{
		// UDP
		// rc = sendto(rics_udp_sock, &tcp_msg, tcp_msg_len, 0, (struct sockaddr *)&rics_udp_sockaddr, sizeof(rics_udp_sockaddr));
		// rprint_inf("send UDP(%d) control msg(%d).\n", rics_udp_sock, rc);
		rc = rics_module_sendTo_UDP(ctxFrom,device_id,(const char*)&tcp_msg, tcp_msg_len);
		 rprint_inf("send UDP(%s) control msg(%d).\n", device_id, rc);
		if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)
		{
			int i;
			for(i=0;i<tcp_msg_len;i++)
				rprint_rel_c(" %02x", *((unsigned char *)(&tcp_msg) + i));
			rprint_rel_c("\n");
		}
	}
	else{
		printf("rics_module_sendTo_UDP_ipcstr(%s):%d bytes?? FAILED (ipc_data='%s')\n",device_id,tcp_msg_len,pIpc_recv_data);
	}
	return rc;
}
void on_mgr_poll(struct ns_mgr* mgr)
{(void)mgr;
	time_t curtime;
	time(&curtime);
#if INCLUDE_RICS_UDP_MASTER
	ricsmanRicsUdp_handler(g_ricsMan_cp->ricsMasterUdp_nc,NS_POLL,&curtime);
#else
	if(curtime-g_ricsMan_cp->last_check_time>=10){//check every 10 sec
		int ret=0,count=0;
		
		ret=iterateAll_ricsconn(find_conn_func_PING,(void*)curtime,&count,mgr);
		if(count>0)printf("%d target UDP poll of %d\n",count,ret);
		g_ricsMan_cp->last_check_time=curtime;
	}
#endif
}
#endif /*INCLUDE_RICS_UDP_CON*/


void rics_request_delayedReply_msg(struct ns_mgr* mgr,const char* device_id,const char* msg,int msgLen)
{
	return rics_request_reply_msg(mgr,device_id, msg, msgLen);
}
int rics_module_sendTo(struct ns_connection *ctxFrom,const char* device_id,const char* data,int dataLen,char* response,int *pResponseLen)
{
#if INCLUDE_RICS_UDP_CON||INCLUDE_RICS_TCP_CON
	static int rics_ctrl_udp_only=0;// INCLUDE_RICS_UDP_CON&&!INCLUDE_RICS_TCP_CON;// rics_ctrl_udp_only is not working for IPC
	const char *pRics_ip=device_id;// alias
	int rc=0;
	char ipc_recv_buf[RICS_IPC_BUF_MAX];int ipc_recv_buf_len=0;
	char ipc_send_buf[RICS_IPC_BUF_MAX];int ipc_send_buf_len=0;
	// struct __rics_state *pRics_state;
	// int tcp_msg_len;
	// tcp_msg_t tcp_msg;

	// MSG: ipc_name args ...
	char ipc_name[64];
	char *pIpc_name_ptr;
	int ResponseBufLen=0;
	ricsConn *ricsconn=NULL;
	char* pIpc_recv_data=ipc_recv_buf;
	(void)pIpc_name_ptr;(void)ResponseBufLen;

	if(pResponseLen){
		ResponseBufLen=*pResponseLen;
		*pResponseLen=0;
	}
	ricsconn=(ricsConn*)find_ricsconn(device_id);
	if(!ricsconn)return 0;
	// pRics_state=&ricsconn->rics_state;

	// rprint_dbg("(%s)IPC recv>>\n", pRics_ip);
	memcpy(ipc_recv_buf,data,dataLen);ipc_recv_buf[dataLen]=0;ipc_recv_buf_len=dataLen;

	if(ipc_recv_buf_len > 0)
	{
		
		ipc_recv_buf[ipc_recv_buf_len] = 0;

		// get ipc command
		pIpc_name_ptr = (char *)my_strlgetword(ipc_recv_buf, 1, ipc_name, sizeof(ipc_name));

		// msg parsing
		if(!strcmp(ipc_name, "QUIT"))
		{
			rprint_err("Quit by EMS\n");
			sprintf(ipc_send_buf, "%s_RES OK", ipc_name);
			ipc_send_buf_len = strlen(ipc_send_buf);
			if(pResponseLen&&response){*pResponseLen=ipc_send_buf_len;strcpy(response,ipc_send_buf);}
			else rics_request_delayedReply_msg(ctxFrom->mgr,pRics_ip,ipc_send_buf, ipc_send_buf_len);
			// my_send_to_uds(pArg->rics_ctrl_ipc_sock, pArg->ipc_send_buf, pArg->ipc_send_buf_len, pArg->ems_ipc_path.main);
			goto out;
		}
		else if(!strcmp(ipc_name, "SERIAL_CON"))
		{
			sprintf(ipc_send_buf, "%s_RES OK", ipc_name);
			ipc_send_buf_len = strlen(ipc_send_buf);
			if(pResponseLen&&response){*pResponseLen=ipc_send_buf_len;strcpy(response,ipc_send_buf);}
			else rics_request_delayedReply_msg(ctxFrom->mgr,pRics_ip,ipc_send_buf, ipc_send_buf_len);
		}
		else if(!strcmp(ipc_name, "SERIAL_DISCON"))
		{
			sprintf(ipc_send_buf, "%s_RES OK", ipc_name);
			ipc_send_buf_len = strlen(ipc_send_buf);
			if(pResponseLen&&response){*pResponseLen=ipc_send_buf_len;strcpy(response,ipc_send_buf);}
			else rics_request_delayedReply_msg(ctxFrom->mgr,pRics_ip,ipc_send_buf, ipc_send_buf_len);
		}
		else if(!strcmp(ipc_name, "UDP"))
		{
			char *pRealCommand = 0;

			pRealCommand = strstr(ipc_recv_buf, "UDP");
			pRealCommand += strlen("UDP") + 1; // real Command

			if(strlen(pRealCommand) > 0){pIpc_recv_data=pRealCommand;goto UDP_sendto;}
		}
		else if(!strcmp(ipc_name, "TCP"))
		{
			char *pRealCommand = 0;

			pRealCommand = strstr(ipc_recv_buf, "TCP");
			pRealCommand += strlen("TCP") + 1; // real Command

			if(strlen(pRealCommand) > 0){pIpc_recv_data=pRealCommand;goto TCP_sendto;}
		}
		else
		{
			if(rics_ctrl_udp_only)
			{
UDP_sendto:	
#if USE_ANOTHER_RICS_SERVER
				return rics_module_sendTo_UDP_ipcstr_safe(ctxFrom,device_id,pIpc_recv_data);// 1. ajax -> RICS
#else
				return rics_module_sendTo_UDP_ipcstr(ctxFrom,device_id,pIpc_recv_data);
#endif
			}
			else
			{
TCP_sendto:				
#if USE_ANOTHER_RICS_SERVER
				return rics_module_sendTo_TCP_ipcstr_safe(ctxFrom,device_id,pIpc_recv_data);// 1. ajax -> RICS
#else
				return rics_module_sendTo_TCP_ipcstr(ctxFrom,device_id,pIpc_recv_data);
#endif
			}
		}
	}
out:	
	return rc;
#else
	(void)response;(void)pResponseLen;
	return rics_module_sendTo_viaRicsMan(ctxFrom,device_id,data,dataLen);
#endif
}
static pthread_mutex_t lock_init_date=PTHREAD_MUTEX_INITIALIZER;
static ricsConn *alloc_rics_module_conn(const char* device_id)
{
	ricsConn *rc=(ricsConn*)calloc(1,sizeof(ricsConn));

	rc->cpType=RMCP_RICS_CONNECTOR;
	strcpy(rc->device_id,device_id);
	rc->websock_list=NULL;rc->request_queue=NULL;

#if RICS_CONTABLE_MUTEX_LOCK	
	memcpy((void*)&rc->lock_ws,&lock_init_date,sizeof(pthread_mutex_t));
	memcpy((void*)&rc->lock_rq,&lock_init_date,sizeof(pthread_mutex_t));
	if(pthread_mutex_init((pthread_mutex_t*)&rc->lock_ws,NULL)!=0)printf("lock_ws init error!!\n");
	else printf("lock_ws(%s) created!\n",device_id);
	if(pthread_mutex_init((pthread_mutex_t*)&rc->lock_rq,NULL)!=0)printf("lock_rq init error!!\n");
	else printf("lock_rq(%s) created!\n",device_id);
#endif
	ws_Lock(rc);
	rc->websock_list=alloc_ptr_list();
	ws_Unlock(rc);

	rq_Lock(rc);
	rc->request_queue=alloc_ptr_queue();
	rq_Unlock(rc);
	return rc;
}
static void free_rics_module_conn(ricsConn *rc)
{
	rq_Lock(rc);
	free_ptr_queue(rc->request_queue);rc->request_queue=NULL;
	rq_Unlock(rc);
	ws_Lock(rc);
	free_ptr_list(rc->websock_list);rc->websock_list=NULL;
	ws_Unlock(rc);
#if RICS_CONTABLE_MUTEX_LOCK	
	if(pthread_mutex_destroy((pthread_mutex_t*)&rc->lock_rq)!=0)printf("lock_rq destroy error!!\n");
	else printf("lock_rq(%s) destroyed!\n",rc->device_id);
#endif	
#if RICS_CONTABLE_MUTEX_LOCK	
	if(pthread_mutex_destroy((pthread_mutex_t*)&rc->lock_ws)!=0)printf("lock_ws destroy error!!\n");
	else printf("lock_ws(%s) destroyed!\n",rc->device_id);
#endif	
	free(rc);
}
int rics_module_add(struct ns_mgr* mgr,void *ctxFrom,const char* device_id)
{
	int ret=0;
	void* ricsconn=find_ricsconn(device_id);
	(void)ctxFrom;

	if(ricsconn){
		ricsConn *rc=(ricsConn*)ricsconn;
		(void)rc;
		// printf("rics_module_add(%p):%s/%s exist\n",rc,device_id,rc->device_id);fflush(stdout);
		;
	}
	else{
		ricsConn *rc=alloc_rics_module_conn(device_id);
		printf("rics_module_add(%p):%s add\n",rc,device_id);fflush(stdout);
#if INCLUDE_RICS_UDP_CON||INCLUDE_RICS_TCP_CON
		ricsconn=rc;
		insert_ricsconn(device_id,ricsconn);
		ret=run_rics_test(mgr,device_id);
		if(ret<0){free(rc);delete_ricsconn(device_id);return ret;}
#else
		ret=run_rics_test(mgr,device_id);
		if(ret<0){free(rc);return ret;}
		ricsconn=rc;
		insert_ricsconn(device_id,ricsconn);
#endif
	}
	return ret;
}
void rics_module_remove(struct ns_mgr* mgr,void *ctxFrom,const char* device_id,int bForceRemove)
{
	void* ricsconn=find_ricsconn(device_id);
	(void)ctxFrom;(void)mgr;

	if(ricsconn){
		ricsConn *rc=(ricsConn*)ricsconn;
		
		// printf("rics_module_remove:%s/%s remove. bForceRemove=%d\n",device_id,rc->device_id,bForceRemove);fflush(stdout);
#ifdef NS_FOSSA_VERSION
		bForceRemove=1;
#endif
		// printf("rics_module_remove:%s removing A\n",device_id);fflush(stdout);
		ws_Lock(rc);
		if(getFront_ptr_list(rc->websock_list)){
			printf("rics_module_remove: websock_list(%s) is not empty!!\n",device_id);
			if(!bForceRemove){
				ws_Unlock(rc);
				return;
			}
		}
		ws_Unlock(rc);

		// printf("rics_module_remove:%s removing B\n",device_id);fflush(stdout);
		// rics_module_sendTo(ctxFrom,device_id,"QUIT",4,NULL,0);
#if INCLUDE_SERIAL_CON
		stop_serialTcp_connector(mgr,device_id);
#endif
		// printf("rics_module_remove:%s removing C\n",device_id);fflush(stdout);
#if INCLUDE_RICS_TCP_CON
		stop_ricsTcp_connector(mgr,device_id);
#endif
		// printf("rics_module_remove:%s removing D\n",device_id);fflush(stdout);
#if INCLUDE_RICS_UDP_CON
		stop_ricsUdp_connector(mgr,device_id);
#endif

		if(bForceRemove){
			// printf("rics_module_remove:%s removing 0\n",device_id);fflush(stdout);
#if INCLUDE_RICS_UDP_CON||INCLUDE_RICS_TCP_CON
			;
#else
			wait_sub_process("/tmp/rics_test.pid");
#endif
			// printf("rics_module_remove:%s removing 1\n",device_id);fflush(stdout);
			delete_ricsconn(device_id);
			// printf("rics_module_remove:%s removing 2\n",device_id);fflush(stdout);
			free_rics_module_conn(rc);
			// printf("rics_module_remove:%s removed\n",device_id);fflush(stdout);
		}
		else{
			printf("rics_module_remove:%s remain connected\n",device_id);fflush(stdout);
		}
	}
	else{
		printf("rics_module_remove:%s already removed\n",device_id);fflush(stdout);
	}
}
int rics_module_can_QUIT(void *ctxFrom,const char* device_id)
{
	void* ricsconn=find_ricsconn(device_id);
	(void)ctxFrom;

	if(ricsconn){
		ricsConn *rc=(ricsConn*)ricsconn;

		ws_Lock(rc);
		if(getFront_ptr_list(rc->websock_list)){
			ws_Unlock(rc);
			printf("rics_module_can_QUIT: websock_list(%s) is not empty!!\n",device_id);
			return 0;
		}
		ws_Unlock(rc);
		return 1;
	}
	return 1;
}


void* rics_requester_getFirst(const char* device_id,const char* *p_jsonp_cb)
{
	void* ricsconn=find_ricsconn(device_id);

	if(ricsconn){
		return _rics_requester_getFirst((ricsConn*)ricsconn,p_jsonp_cb);
	}
	else{
		printf("rics_requester_getFirst:all nc on %s already removed\n",device_id);fflush(stdout);
	}
	return NULL;
}
void rics_requester_add(const char* device_id,void* nc,const char* cb)
{
	void* ricsconn=find_ricsconn(device_id);

	if(ricsconn){
		_rics_requester_add((ricsConn*)ricsconn,nc,cb);
		printf("rics_requester_add:nc%p on %s add\n",nc,device_id);fflush(stdout);
	}
	else{
		printf("rics_requester_add:nc%p on %s already removed\n",nc,device_id);fflush(stdout);
	}
}
void rics_requester_del(const char* device_id,void* nc)
{
	void* ricsconn=find_ricsconn(device_id);

	if(ricsconn){
		_rics_requester_del((ricsConn*)ricsconn,nc);
		printf("rics_requester_del:nc%p on %s removed\n",nc,device_id);fflush(stdout);
	}
	else{
		printf("rics_requester_del:nc%p on %s already removed\n",nc,device_id);fflush(stdout);
	}
}
void rics_websock_add(const char* device_id,void* nc)
{
	void* ricsconn=find_ricsconn(device_id);

	if(!ricsconn){// for websocket first!!
		ricsConn *rc=alloc_rics_module_conn(device_id);

		ricsconn=rc;
		// printf("rics_module_add(PRE):%s add\n",device_id);fflush(stdout);
		// run_rics_test(mgr,device_id);
		insert_ricsconn(device_id,ricsconn);
	}
	
	if(ricsconn){
		ricsConn *cp=(ricsConn*)ricsconn;

		ws_Lock(cp);
		addTo_ptr_list(cp->websock_list,nc);
		ws_Unlock(cp);
		// printf("rics_websock_add:%s add %p\n",device_id,nc);fflush(stdout);
		// if mgr is diff with rc->mgr then, transfer to the mgr!! and return positive;// RICS_MULTITHREAD_SERVER
	}
	else{
		printf("rics_websock_add:%s already removed\n",device_id);fflush(stdout);
	}
}
void rics_websock_del(const char* device_id,void* nc)
{
	void* ricsconn=find_ricsconn(device_id);

	if(ricsconn){
		int found=0;void* ncRemainFirst=NULL;
		ricsConn *cp=(ricsConn*)ricsconn;

		ws_Lock(cp);
		found=removeFrom_ptr_list(cp->websock_list,nc);
		if((ncRemainFirst=getFront_ptr_list(cp->websock_list))!=NULL){
			ws_Unlock(cp);
			printf("rics_websock_del:websock_list(%s) is not empty!!\n",device_id);
		}
		else{
			ws_Unlock(cp);
			// printf("rics_websock_del:websock_list(%s) is empty!!\n",device_id);
#if INCLUDE_SERIAL_CON
#ifdef NS_FOSSA_VERSION
			stop_serialTcp_connector(((struct ns_connection*)nc)->mgr,device_id);
#else
			stop_serialTcp_connector(mg_get_nc_conn((struct mg_connection*)nc)->mgr,device_id);
#endif
#endif
		}
		printf("rics_websock_del:%s removed(%d) %p/ncRemainFirst=%p\n",device_id,found,nc,ncRemainFirst);fflush(stdout);
	}
	else{
		printf("rics_websock_del:%s already removed %p\n",device_id,nc);fflush(stdout);
	}
}
static int find_conn_func_cnt(void* map,const char* device_id,void* conn_data,void* Ctx,void* Ctx2,void* Ctx3)
{(void)Ctx3;
	void* ricsconn=conn_data;
	void* nc=(void*)Ctx;int* pCount=(int*)Ctx2;
	int ret=0;
	(void)map;

	if(ricsconn){
		ricsConn *cp=(ricsConn*)ricsconn;

		ws_Lock(cp);
		ret=find_ptr_list(cp->websock_list,nc);
		ws_Unlock(cp);
		if(ret&&pCount)(*pCount)++;
		
		printf("find_conn_func_cnt:%s %s\n",device_id,(ret? "found":"not found"));fflush(stdout);
		if(ret)return 1;//break;
	}
	else{
		printf("find_conn_func_cnt:%s already removed\n",device_id);fflush(stdout);
	}
	return 0;
}
static int find_conn_func_dev(void* map,const char* device_id,void* conn_data,void* Ctx,void* Ctx2,void* Ctx3)
{(void)Ctx3;
	void* ricsconn=conn_data;
	void* nc=(void*)Ctx;char* device_id_out=(char*)Ctx2;
	int ret=0;
	(void)map;

	if(ricsconn){
		ricsConn *cp=(ricsConn*)ricsconn;

		ws_Lock(cp);
		ret=find_ptr_list(cp->websock_list,nc);
		ws_Unlock(cp);
		if(ret&&device_id)strcpy(device_id_out,device_id);
		
		printf("find_conn_func_dev:%s %s\n",device_id,(ret? "found":"not found"));fflush(stdout);
		if(ret)return 1;//break;
	}
	else{
		printf("find_conn_func_dev:%s already removed\n",device_id);fflush(stdout);
	}
	return 0;
}
int rics_websock_find_cnt(void* nc)
{
	int ret=0;
	int count=0;

	ret=iterateAll_ricsconn(find_conn_func_cnt,nc,&count,0);
	if(count){
		printf("rics_websock_find:%p(%d) %s\n",nc,count,(count? "found":"not found"));fflush(stdout);
	}
	else{
		printf("rics_websock_find:%p already removed\n",nc);fflush(stdout);
	}
	ret=count>0;
	return ret;
}
int rics_websock_find_dev(void* nc,char *device_id)
{
	int ret=0;

	if(device_id)device_id[0]=0;
	ret=iterateAll_ricsconn(find_conn_func_dev,nc,device_id,0);
	if(device_id&&device_id[0]){
		printf("rics_websock_find_dev:%p(%s) %s\n",nc,device_id,(device_id&&device_id[0]? "found":"not found"));fflush(stdout);
	}
	else{
		;// printf("rics_websock_find_dev:%p already removed\n",nc);fflush(stdout);
	}
	if(device_id&&device_id[0])ret=1;
	else ret=0;
	return ret;
}
void transfer2RICS_serialData(struct ns_connection *ctxFrom,const char* device_id,char side,const char* data,int dataLen)// must be called on same ns_mgr!!
{
#if INCLUDE_SERIAL_CON
	void* ricsconn=find_ricsconn(device_id);

	if(ricsconn){
		ricsConn *rc=(ricsConn*)ricsconn;
		int rics_node_id = 0;

		delayed_send_serial_data_rpt(ctxFrom, rc, rics_node_id, side, data,dataLen);// to RICS(using NS_write)
	}
	else printf("serialDrop: no ricsConn%s\n",device_id);
#else
	char device_id_name[PATH_MAX];

	sprintf(device_id_name,"%s_%c",device_id,side);
	rics_module_sendTo(ctxFrom,device_id_name,data,dataLen,NULL,0);// to side!!
#endif
}
int encodeSerial2WebsockMsg(unsigned char* encodedData,int encodeDataSize,const char* serialSendFmt,const unsigned char *sideANDdata,int sideANDdataLen);
static int websock_send_func(void* websock_list,void* conn_data,void* Ctx,void* Ctx2,void* Ctx3)// must be called on same ns_mgr!!
{
	void* websock_nc=conn_data;
	const char* msg=(const char*)Ctx;int msgLen=(int)(long int)Ctx2;
	(void)websock_list;(void)Ctx3;

#ifdef NS_FOSSA_VERSION
#define RICS_WEBSOCKET_OP_DATA_COND(cp)	(cp->binaryCon? WEBSOCKET_OP_BINARY:WEBSOCKET_OP_TEXT) // WEBSOCKET_OP_BINARY // 
	 if(websock_nc){
		ricsWebsock_conn_param* cp=((struct ns_connection*)websock_nc)->user_data;// cp->binaryCon
		unsigned char encmsg[32768];int encmsgSize=0;

		// log_RICS_serialRecv(cp->device,msg[0],msg+1, msgLen-1,rc->logEnTime);
		encmsgSize=encodeSerial2WebsockMsg(encmsg,sizeof(encmsg),cp->serialSendFmt,(const unsigned char*)msg, msgLen);
	 	ns_send_websocket_frame((struct ns_connection*)websock_nc, RICS_WEBSOCKET_OP_DATA_COND(cp), (const void*)encmsg, encmsgSize);//forward to Websock(client)
	 }
#else
#define RICS_WEBSOCKET_OP_DATA_COND(cp)	(cp->binaryCon? WEBSOCKET_OPCODE_BINARY:WEBSOCKET_OPCODE_TEXT) // WEBSOCKET_OPCODE_BINARY // 
	if(websock_nc){
		ricsWebsock_conn_param* cp=((struct mg_connection*)websock_nc)->connection_param;// cp->binaryCon
		unsigned char encmsg[32768];int encmsgSize=0;

		// log_RICS_serialRecv(cp->device,msg[0],msg+1, msgLen-1,rc->logEnTime);
		encmsgSize=encodeSerial2WebsockMsg(encmsg,sizeof(encmsg),cp->serialSendFmt,(const unsigned char*)msg, msgLen);
		mg_websocket_write((struct mg_connection*)websock_nc, RICS_WEBSOCKET_OP_DATA_COND(cp), (const char*)encmsg, encmsgSize);//forward to Websock(client)
	}
#endif
	return 0;
}

#if USE_ANOTHER_RICS_SERVER
#if INCLUDE_SERIAL_CON
void transfer2RICS_serialData_on_dest_mgr(struct ns_connection *nc_input, int ev, void *param)// must be called on same ns_mgr!!
{
  
  if(ev==NS_POLL){
	const char *msg = (const char *) param;
	int n,msgLen=0;
	char side='A';
	char device_id[PATH_MAX];
	struct ns_connection *ctxFrom,*serial_nc;

	  if (sscanf(msg, "RUN %p %p %s %c %d|%n", &serial_nc, &ctxFrom,device_id,&side,&msgLen,&n)) { // for RUN-patch
if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("transfer2RICS_serialData_forward%p:%p/%s_%c/%d\n",serial_nc,ctxFrom,device_id,side,msgLen);
		transfer2RICS_serialData(ctxFrom,device_id,side,(void*)(msg+n),msgLen);
	  }
  }
  else{// NS_CLOSE: if not found
	  printf("transfer2RICS_serialData_on_dest_mgr:%p unknown ev=%d\n",nc_input,ev);
    }
}
#else
void rics_module_sendTo_on_dest_mgr(struct ns_connection *nc_input, int ev, void *param)// must be called on same ns_mgr!!
{
  if(ev==NS_POLL){
	const char *msg = (const char *) param;
	int n,msgLen=0;
	char side='A';
	char device_id[PATH_MAX];
	struct ns_connection *ctxFrom,*ricsUds_nc;

	  if (sscanf(msg, "RUN %p %p %s %c %d|%n", &ricsUds_nc, &ctxFrom,device_id,&side,&msgLen,&n)) { // for RUN-patch
		char device_id_name[PATH_MAX];

		sprintf(device_id_name,"%s_%c",device_id,side);
if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("rics_module_sendTo_forward%p:%p/%s_%c/%d\n",ricsUds_nc,ctxFrom,device_id,side,msgLen);
		rics_module_sendTo(ctxFrom,device_id_name,void*)(msg+n),msgLen,NULL,0);// to side!!
	  }
  }
  else{// NS_CLOSE: if not found
	  printf("rics_module_sendTo_on_dest_mgr:%p unknown ev=%d\n",nc_input,ev);
    }
}
#endif
void transfer2RICS_serialData_safe(struct ns_connection *ctxFrom,const char* device_id,char side,const char* data,int dataLen)// may be called on other ns_mgr!!
{
#if INCLUDE_SERIAL_CON
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);

	if(rc){
		if(!rc->serial_nc||ctxFrom->mgr==rc->serial_nc->mgr){
			int rics_node_id = 0;
			
			delayed_send_serial_data_rpt(ctxFrom, rc, rics_node_id, side, data,dataLen);// to RICS(using NS_write)
		}
		else{
			char buf[NS_CTL_MSG_MESSAGE_SIZE]; int len;

			len = snprintf(buf, sizeof(buf), "RUN %p %p %s %c %d|%.*s", rc->serial_nc, ctxFrom,device_id,side,dataLen,dataLen,data); // for RUN-patch
			// "len + 1" is to include terminating \0 in the message
			ns_broadcast_new(rc->serial_nc->mgr, transfer2RICS_serialData_on_dest_mgr, buf, len + 1);
		}
	}
	else printf("serialDrop: no ricsConn%s\n",device_id);
#else
	if(!g_ricsMan_cp->ricsUds_nc||ctxFrom->mgr==g_ricsMan_cp->ricsUds_nc->mgr){
		char device_id_name[PATH_MAX];

		sprintf(device_id_name,"%s_%c",device_id,side);
		rics_module_sendTo(ctxFrom,device_id_name,data,dataLen,NULL,0);// to side!!
	}
	else{
		char buf[NS_CTL_MSG_MESSAGE_SIZE]; int len;

		len = snprintf(buf, sizeof(buf), "RUN %p %p %s %c %d|%.*s", g_ricsMan_cp->ricsUds_nc, ctxFrom,device_id,side,dataLen,dataLen,data); // for RUN-patch
		// "len + 1" is to include terminating \0 in the message
		ns_broadcast_new(g_ricsMan_cp->ricsUds_nc->mgr, rics_module_sendTo_on_dest_mgr, buf, len + 1);
	}
#endif
}
static void websock_send_func_on_dest_mgr(struct ns_connection *nc_input, int ev, void *param)// must be called on same ns_mgr!!
{
  if(ev==NS_POLL){
	const char *msg = (const char *) param;
	int n,msgLen=0;
	void* conn;struct ns_connection* websock_nc;

	  if (sscanf(msg, "RUN %p %p %d|%n", &websock_nc,&conn,&msgLen,&n)) { // for RUN-patch
if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("websock_send_func_forward%p:%p/%d\n",websock_nc,conn,msgLen);
		websock_send_func(websock_nc/*dummy*/,conn,(void*)(msg+n),(void*)(long int)msgLen,0);
	  }
  }
  else{// NS_CLOSE: if not found
	  printf("websock_send_func_on_dest_mgr:%p unknown ev=%d\n",nc_input,ev);
    }
}
static int websock_send_func_safe(void* websock_list,void* conn_data,void* Ctx,void* Ctx2,void* Ctx3)// may be called on other ns_mgr!!
{
	const char* msg=(const char*)Ctx;int msgLen=(int)(long int)Ctx2;
	struct ns_mgr* mgr=(struct ns_mgr*)Ctx3;
#ifdef NS_FOSSA_VERSION
	struct ns_connection* websock_nc=conn_data;
#else
	struct mg_connection* websock_conn=(struct mg_connection*)conn_data;
	struct ns_connection* websock_nc=mg_get_nc_conn(websock_conn);
#endif	

	if(websock_nc->mgr==mgr)return websock_send_func(websock_list,conn_data,Ctx,Ctx2,0);
	else{
		char buf[NS_CTL_MSG_MESSAGE_SIZE]; int len;

		len = snprintf(buf, sizeof(buf), "RUN %p %p %d|%.*s", websock_nc,conn_data,msgLen,msgLen,msg); // for RUN-patch
		// "len + 1" is to include terminating \0 in the message
		ns_broadcast_new(websock_nc->mgr, websock_send_func_on_dest_mgr, buf, len + 1);
	}
	return  0;
}
void rics_module_sendTo_UDP_ipcstr_on_dest_mgr(struct ns_connection *nc_input, int ev, void *param)// must be called on same ns_mgr!!
{
  if(ev==NS_POLL){
	const char *msg = (const char *) param;
	int n;int ipcLen=0;
	void* ctxFrom;
	char device_id[PATH_MAX];
	struct ns_connection *nc;

	  if (sscanf(msg, "RUN %p %p %s %d|%n", &nc,&ctxFrom,device_id,&ipcLen,&n)) { // for RUN-patch
if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("rics_module_sendTo_UDP_ipcstr_forward%p:%p/%s\n",nc,ctxFrom,device_id);
		rics_module_sendTo_UDP_ipcstr(ctxFrom,device_id,(const char*)(msg+n));
	  }
  }
  else{// NS_CLOSE: if not found
	  printf("rics_module_sendTo_UDP_ipcstr_on_dest_mgr:%p unknown ev=%d\n",nc_input,ev);
  }
}

int rics_module_sendTo_UDP_ipcstr_safe(struct ns_connection *ctxFrom,const char* device_id,const char* pIpc_recv_data)
{
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);

	if(rc){
#if INCLUDE_RICS_UDP_MASTER	
		struct ns_connection *ricsUdp_nc=g_ricsMan_cp->ricsMasterUdp_nc;
#else
		struct ns_connection *ricsUdp_nc=ricsConn->ricsUdp_nc;
#endif
		int bOnSameMgr=(!ricsUdp_nc||ctxFrom->mgr==ricsUdp_nc->mgr);

		if(bOnSameMgr){
			return rics_module_sendTo_UDP_ipcstr(ctxFrom,device_id,pIpc_recv_data);
		}
		else{
			char buf[NS_CTL_MSG_MESSAGE_SIZE]; int len;

			len = snprintf(buf, sizeof(buf), "RUN %p %p %s %d|%s", ricsUdp_nc, ctxFrom,device_id,(int)strlen(pIpc_recv_data),pIpc_recv_data); // for RUN-patch
			// "len + 1" is to include terminating \0 in the message
			ns_broadcast_new(ricsUdp_nc->mgr, rics_module_sendTo_UDP_ipcstr_on_dest_mgr, buf, len + 1);
			return len;// in-correct
		}
	}
	else return 0;
}
void rics_module_sendTo_TCP_ipcstr_on_dest_mgr(struct ns_connection *nc_input, int ev, void *param)// must be called on same ns_mgr!!
{
  if(ev==NS_POLL){
	const char *msg = (const char *) param;
	int n;int ipcLen=0;
	void* ctxFrom;
	char device_id[PATH_MAX];
	struct ns_connection *nc;

	  if (sscanf(msg, "RUN %p %p %s %d|%n", &nc, &ctxFrom,device_id,&ipcLen,&n)) { // for RUN-patch
if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("rics_module_sendTo_TCP_ipcstr_forward%p:%p/'%s'\n",nc,ctxFrom,device_id);
		rics_module_sendTo_TCP_ipcstr(ctxFrom,device_id,(const char*)(msg+n));
	  }
  }
  else{// NS_CLOSE: if not found
	  printf("rics_module_sendTo_TCP_ipcstr_on_dest_mgr:%p unknown ev=%d\n",nc_input,ev);
  }
}
int rics_module_sendTo_TCP_ipcstr_safe(struct ns_connection *ctxFrom,const char* device_id,const char* pIpc_recv_data)
{
	ricsConn *rc=(ricsConn*)find_ricsconn(device_id);

	if(rc){
		int bOnSameMgr=(!rc->ricsTcp_nc||ctxFrom->mgr==rc->ricsTcp_nc->mgr);

		if(bOnSameMgr){
			return rics_module_sendTo_TCP_ipcstr(ctxFrom,device_id,pIpc_recv_data);
		}
		else{
			char buf[NS_CTL_MSG_MESSAGE_SIZE]; int len;

			len = snprintf(buf, sizeof(buf), "RUN %p %p %s %d|%s", rc->ricsTcp_nc, ctxFrom,device_id,(int)strlen(pIpc_recv_data),pIpc_recv_data); // for RUN-patch
			// "len + 1" is to include terminating \0 in the message
			ns_broadcast_new(rc->ricsTcp_nc->mgr, rics_module_sendTo_TCP_ipcstr_on_dest_mgr, buf, len + 1);
			return len;// in-correct
		}
	}
	else return 0;
}
#ifdef NS_FOSSA_VERSION
void handle_rics_reply_on_dest_mgr(struct ns_connection *nc_input, int ev, void *param)// must be called on same ns_mgr!!
{
  if(ev==NS_POLL){
	const char *msg = (const char *) param;
	int n,outputLen;
	char device_id[PATH_MAX],jsonp_cb[128];
	struct ns_connection* nc,*ncDummy;

	  if (sscanf(msg, "RUN %p %p %s %s %d|%n", &nc,&ncDummy,device_id,jsonp_cb,&outputLen,&n)) { // for RUN-patch
if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("handle_rics_reply_forward%p:%s/%s/%d=%.*s\n",nc,jsonp_cb,device_id,outputLen,outputLen,(const char*)(msg+n));
		handle_rics_reply(nc,(const char*)(msg+n),outputLen,jsonp_cb,device_id);
	  }
  }
  else{// NS_CLOSE: if not found
	  printf("handle_rics_reply_on_dest_mgr:%p unknown ev=%d\n",nc_input,ev);
  }
}
void handle_rics_reply_safe(struct ns_mgr* mgr,struct ns_connection *nc, const char* output,int outputLen,const char* jsonp_cb,const char * device)
{
	int bOnSameMgr=(!nc||nc->mgr==mgr);

	if(bOnSameMgr){
		handle_rics_reply(nc, output,outputLen,jsonp_cb,device);
	}
	else{
		char buf[NS_CTL_MSG_MESSAGE_SIZE]; int len;

		len = snprintf(buf, sizeof(buf), "RUN %p %p %s %s %d|%.*s", nc,nc,device,jsonp_cb,outputLen,outputLen,output); // for RUN-patch
		// "len + 1" is to include terminating \0 in the message
		ns_broadcast_new(nc->mgr, handle_rics_reply_on_dest_mgr, buf, len + 1);
		return;
	}
}
#else
void handle_rics_reply_on_dest_mgr(struct ns_connection *nc_input, int ev, void *param)// must be called on same ns_mgr!!
{
  if(ev==NS_POLL){
	const char *msg = (const char *) param;
	int n,outputLen;
	void* ctxFrom;
	char device_id[PATH_MAX],jsonp_cb[128];
	struct ns_connection* nc;
	struct mg_connection* conn;

	  if (sscanf(msg, "RUN %p %p %s %s %d|%n", &nc,&conn,device_id,jsonp_cb,&outputLen,&n)) { // for RUN-patch
if(g_rics_ctrl_debug_flag & DBG_FLAG_PRINT_DBG)printf("handle_rics_reply_forward%p:%s/%s/%d=%.*s\n",conn,jsonp_cb,device_id,outputLen,outputLen,(const char*)(msg+n));
		handle_rics_reply(conn,(const char*)(msg+n),outputLen,jsonp_cb,device_id);
	  }
  }
  else{// NS_CLOSE: if not found
	  printf("handle_rics_reply_on_dest_mgr:%p unknown ev=%d\n",nc_input,ev);
  }
}
void handle_rics_reply_safe(struct ns_mgr* mgr,struct mg_connection *conn, const char* output,int outputLen,const char* jsonp_cb,const char * device)
{
	struct ns_connection *nc=conn? mg_get_nc_conn(conn):NULL;
	int bOnSameMgr=(!conn||nc->mgr==mgr);
	
// printf("handle_rics_reply_safe%p:%s/%s/%d=%.*s\n",conn,jsonp_cb,device,outputLen,outputLen,output);
	if(bOnSameMgr){
		return handle_rics_reply(conn, output,outputLen,jsonp_cb,device);
	}
	else{
		char buf[NS_CTL_MSG_MESSAGE_SIZE]; int len;

		len = snprintf(buf, sizeof(buf), "RUN %p %p %s %s %d|%.*s", nc,conn,device,jsonp_cb,outputLen,outputLen,output); // for RUN-patch
		// "len + 1" is to include terminating \0 in the message
		ns_broadcast_new(nc->mgr, handle_rics_reply_on_dest_mgr, buf, len + 1);
		return;
	}
}
#endif
#endif /*#if USE_ANOTHER_RICS_SERVER*/


void rics_websock_forward_msg(struct ns_mgr* mgr,const char* device_id,const char* msg,int msgLen,int opt)
{(void)mgr;(void)opt;
	void* ricsconn=find_ricsconn(device_id);

	if(ricsconn){
		ricsConn *cp=(ricsConn*)ricsconn;

		// 4. RICSserial -> websock	
#if USE_ANOTHER_RICS_SERVER
		iterate_ptr_list(cp->websock_list,websock_send_func_safe,(void*)msg,(void*)(long int)msgLen,mgr);// cannot Lock!!
#else
		iterate_ptr_list(cp->websock_list,websock_send_func,(void*)msg,(void*)msgLen,mgr);// may Lock!!
#endif
	}
	else{
		printf("rics_websock_forward_msg:%s already removed\n",device_id);fflush(stdout);
	}
}
void on_rics_reply_msg(struct ns_mgr* mgr,const char* device_id,const char* msg,int msgLen)
{
	printf("on_rics_reply_msg: IPC(%s) -> msg='%.*s'\n",device_id,msgLen,msg);fflush(stdout);
	if(!memcmp(msg,"LINE_STATE_RES ",strlen("LINE_STATE_RES "))||!memcmp(msg,"POWER_RESET_RES ",strlen("POWER_RESET_RES "))
		||!memcmp(msg,"POWER_ON_RES ",strlen("POWER_ON_RES "))||!memcmp(msg,"POWER_OFF_RES ",strlen("POWER_OFF_RES "))
		||!memcmp(msg,"SERIAL_CON_RES ",strlen("SERIAL_CON_RES "))||!memcmp(msg,"SERIAL_DISCON_RES ",strlen("SERIAL_DISCON_RES "))
		){
		indData2Websocket(mgr,device_id,'@',msg,msgLen);// for new-HTML
#if 1	// must working code!!	
		if(!memcmp(msg,"SERIAL_CON_RES OK",strlen("SERIAL_CON_RES OK"))){
#if INCLUDE_SERIAL_CON
			int serial_num_user=0;

			// serial_num_user==0//serial_connected==0// if not connected yet!!
			if(!get_serialTcp_connector_state(mgr,device_id,&serial_num_user))
				start_serialTcp_connector(mgr,device_id);
			else serialData2Websocket(mgr,device_id,'A',"[already connected]",17+2);//refmod_serialTcp_connector_state(mgr,device_id,+1);
#endif
		}
		else if(!memcmp(msg,"SERIAL_DISCON_RES OK",strlen("SERIAL_DISCON_RES OK"))){
#if INCLUDE_SERIAL_CON
			int serial_num_user=0;

			// serial_num_user==1//serial_connected<=1// if not connected to any other!!
			if(get_serialTcp_connector_state(mgr,device_id,&serial_num_user)&&serial_num_user<=1)
				 serialData2Websocket(mgr,device_id,'A',"[stay connected]",14+2);//stop_serialTcp_connector(mgr,device_id);
			else serialData2Websocket(mgr,device_id,'A',"[stay connected]",14+2);//refmod_serialTcp_connector_state(mgr,device_id,-1);
#endif
		}
#endif	
	}
#if 0	// not working code!!	
	else if(!memcmp(msg,"SERIAL_CON_RES OK",strlen("SERIAL_CON_RES OK"))){
#if INCLUDE_SERIAL_CON
		start_serialTcp_connector(mgr,device_id);
#endif
	}
	else if(!memcmp(msg,"SERIAL_DISCON_RES OK",strlen("SERIAL_DISCON_RES OK"))){
#if INCLUDE_SERIAL_CON
		stop_serialTcp_connector(mgr,device_id);
#endif
	}
#endif	
	else if(!memcmp(msg,"QUIT_RES OK",strlen("QUIT_RES OK"))){
		;// printf("on_rics_reply_msg:%s QUIT_RES OK\n",device_id);fflush(stdout);
	}
	// else printf("on_rics_reply_msg: not a serial con/discon msg\n");
	  if(!memcmp(msg,"RICS_EXIT Ctrl Thread Fail(201)",strlen("RICS_EXIT Ctrl Thread Fail(201)"))){
	  	check_rics_test(device_id);
	  }
}
void rics_request_reply_msg(struct ns_mgr* mgr,const char* device_id,const char* msg,int msgLen)
{(void)mgr;
	int cnt=0;
	void* ricsconn=find_ricsconn(device_id);

	if(ricsconn){
		const char* jsonp_cb=NULL;
#ifdef NS_FOSSA_VERSION
		struct ns_connection *request_nc=NULL;
#else
		struct mg_connection *request_nc=NULL;
#endif
tryAgain:
#ifdef NS_FOSSA_VERSION
		request_nc=rics_requester_getFirst(device_id,&jsonp_cb);// _rics_requester_getFirst((ricsConn*)ricsconn,&jsonp_cb);// 
#else
		request_nc=rics_requester_getFirst(device_id,&jsonp_cb);// _rics_requester_getFirst((ricsConn*)ricsconn,&jsonp_cb);// 
#endif
		// printf("rics_request_reply_msg: IPC(%s) -> request_nc=%p\n",device_id,request_nc);fflush(stdout);
		if(request_nc){
			if(rics_ajaxsock_find(request_nc)){// ?? // must find on current device ricsconn!! -> not enough!!
#if USE_ANOTHER_RICS_SERVER
				handle_rics_reply_safe(mgr,request_nc,msg, msgLen,jsonp_cb,device_id);// 2. RICS -> ajax
				rics_requester_del(device_id,request_nc);//_rics_requester_del((ricsConn*)ricsconn,request_nc);//
#else
				handle_rics_reply(request_nc,msg, msgLen,jsonp_cb,device_id);
				rics_requester_del(device_id,request_nc);//_rics_requester_del((ricsConn*)ricsconn,request_nc);//
#endif
			}
			else{
				rics_requester_del(device_id,request_nc);
				printf("rics_request_reply_msg%d: already removed AJAXsock found%p\n",cnt,request_nc);cnt++;goto tryAgain;
			}
		}
		else{
			printf("rics_request_reply_msg: ipc drop or forward to websock\n");
			on_rics_reply_msg(mgr,device_id,msg,msgLen);
		}
	}
	else{
		printf("rics_request_reply_msg:%s already removed\n",device_id);fflush(stdout);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef NS_FOSSA_VERSION
void on_rics_ajax_call(const char* user,struct ns_mgr* mgr,struct ns_connection *org_nc,const char* command,const char* device,const char* cb,const char* logEn)
#else
int on_rics_ajax_call(const char* user,struct ns_mgr* mgr,struct mg_connection *org_nc/*conn*/,const char* command,const char* device,const char* cb,const char* logEn)
#endif
{
	char output[256];char response[2048];int responseLen=sizeof(response);
	int bRemove=0,bReqAdd=0;
	ricsConn *cp=NULL;
	struct ns_connection *real_nc=NULL;
	
#ifdef NS_FOSSA_VERSION
	real_nc=org_nc;
#else
	real_nc=mg_get_nc_conn(org_nc);
#endif
	response[0]=0;output[0]=0;

	if(strcmp(command,"INIT")==0){
		int addRet=0;
		strcpy(output,"INIT_RES OK");

		printf("on_rics_ajax_call: user='%s', request_nc=%p, device='%s', command='%s'\n",user,org_nc,device,command);
		addRet=rics_module_add(mgr,org_nc,device);
		if(addRet<0)strcpy(output,"INIT_RES FAILED");
		if(addRet>0){
			// printf("> on_rics_ajax_call(%p): user='%s', request_nc=%p, device='%s', command='%s'\n",find_ricsconn(device),user,org_nc,device,command);
#ifdef NS_FOSSA_VERSION
			return;// server.NOT_DONE_YET
#else
			return MG_MORE;// server.NOT_DONE_YET
#endif
		}
		cp=(ricsConn*)find_ricsconn(device);
		// printf("< on_rics_ajax_call(%p): user='%s', request_nc=%p, device='%s', command='%s'\n",find_ricsconn(device),user,org_nc,device,command);
		// FAILED or OK!!
	}
	else if(strcmp(command,"QUIT")==0){
		cp=(ricsConn*)find_ricsconn(device);
		strcpy(output,"QUIT_RES OK");
		if(cp){
			bRemove=rics_module_can_QUIT(org_nc,device);
			 // send "QUIT" & disconnect
			printf("on_rics_ajax_call(%p): user='%s', request_nc=%p, device='%s', command='%s'(remove=%d)\n",cp,user,org_nc,device,command,bRemove);
			if(bRemove)rics_module_sendTo(real_nc,device,command,strlen(command),response,&responseLen);
			else responseLen=0;
			if(responseLen>0&&response[0])goto ImmResp;
		}
		else strcpy(output,"rics_device NOT ready");
	}
	else{
		cp=(ricsConn*)find_ricsconn(device);
		if(cp){
			printf("on_rics_ajax_call(%p): user='%s', request_nc=%p, device='%s', command='%s'\n",cp,user,org_nc,device,command);
			bReqAdd=1;_rics_requester_add(cp,org_nc,cb);
			rics_module_sendTo(real_nc,device,command,strlen(command),response,&responseLen);
			if(responseLen>0&&response[0])goto ImmResp;
#ifdef NS_FOSSA_VERSION
			return;// server.NOT_DONE_YET
#else
			return MG_MORE;// server.NOT_DONE_YET
#endif
		}
		else strcpy(output,"rics_device NOT ready");
	}
ImmResp:	
	if(responseLen>0&&response[0])strcpy(output,response);// this is callback response
	else printf("on_rics_ajax_call(%p): request_nc=%p, send imm response to user='%s'\n",cp,org_nc,user);
	handle_rics_reply(org_nc,output,strlen(output),cb,device);
	if(bReqAdd&&cp)_rics_requester_del(cp,org_nc);
	if(bRemove)rics_module_remove(mgr,org_nc,device,0); 
	logEn_for_RICSdevice(device,logEn);
#ifdef NS_FOSSA_VERSION
	return;
#else
	return MG_TRUE;
#endif
}
#ifdef NS_FOSSA_VERSION
void on_rics_websock_connect(const char* user,struct ns_mgr* mgr,struct ns_connection *org_nc,const char* device,const char* protocol,const char* serialSendFmt,const char* logEn)
#else
void on_rics_websock_connect(const char* user,struct ns_mgr* mgr,struct mg_connection *org_nc/*conn*/,const char* device,const char* protocol,const char* serialSendFmt,const char* logEn)
#endif
{
	ricsWebsock_conn_param* cp=NULL;(void)mgr;

	rics_websock_add(device,org_nc);
#ifdef NS_FOSSA_VERSION
	org_nc->user_data=(cp=(ricsWebsock_conn_param*)calloc(1,sizeof(ricsWebsock_conn_param)));//register my data// RMCP_RICS_WEBSOCK
#else
	org_nc->connection_param=(cp=(ricsWebsock_conn_param*)calloc(1,sizeof(ricsWebsock_conn_param)));//register my data// RMCP_RICS_WEBSOCK
#endif
	cp->cpType=RMCP_RICS_WEBSOCK;
	strcpy(cp->device,device);
	cp->ricsUds_nc_XY=(g_ricsMan_cp->ricsUds_nc);//ricsWebsock_conn_param*
	printf("client(%p) created(cp=%p(%d),user='%s',device='%s').\n",org_nc,cp,cp->cpType,user,cp->device);
	if(protocol&&strstr(protocol,"binary")!=NULL)cp->binaryCon=1;
	if(serialSendFmt[0])strcpy(cp->serialSendFmt,serialSendFmt);
	else strcpy(cp->serialSendFmt,"URIencode");
	logEn_for_RICSdevice(device,logEn);
}
#ifdef NS_FOSSA_VERSION
static void handle_rics_reply(struct ns_connection *nc, const char* output,int outputLen,const char* jsonp_cb,const char * device)
{

	on_rics_reply_msg(nc->mgr,device,output,outputLen);
	/* Send headers */
	ns_printf(nc, "%s", "HTTP/1.1 200 OK\r\nContent-type: application/json\r\nTransfer-Encoding: chunked\r\n\r\n");
	/* send it back*/
// printf("sending to client%p from %s('%.*s')\n",nc,device,outputLen,output);fflush(stdout);
	if(jsonp_cb){// this is DEFAULT
	  	char quoted[32768];int nPtr=0;

		if(bRICS_ResponseFileDataURLencodeAJAX){
		  	// json_emit_quoted_str(quoted,sizeof(quoted),output,outputLen);
			NS_url_encode(output,outputLen,quoted,sizeof(quoted));
			//sprintf(LengthStr,"%u",strlen(jsonp_cb)+2 +4 +10 +strlen(device)+2 +2+8 +strlen(quoted)+2);
			ns_printf_http_chunk(nc, "%s([{\"device\": \"%s\", \"data\": \"%.*s\"}])",jsonp_cb,device,strlen(quoted),quoted);
		}
		else{
			nPtr+=sprintf(quoted+nPtr,"%s([{\"device\":\"%s\",\"jsondata\":\"",jsonp_cb,device);
			jss_encode(quoted+nPtr,sizeof(quoted)-nPtr,output,outputLen,0);nPtr+=strlen(quoted+nPtr);
			nPtr+=sprintf(quoted+nPtr,"\"}])");
			ns_send_http_chunk(nc,quoted,nPtr);
		}
	}
	else{
		ns_send_http_chunk(nc, output, outputLen);
	}
	ns_send_http_chunk(nc, "", 0);  /* Send empty chunk, the end of response */
	nc->flags |= NSF_SEND_AND_CLOSE;
}

static void handle_rics_call(struct ns_connection *nc, struct http_message *hm) 
{
	char user[20+1]; // MAX_USER_LEN
	struct ns_str *hdr;
	struct ns_str* pVars;
	char cb[64],_var[128],t[64]; //int isJSONpc=0;//cb:_jsonpcallback
	char src[64],dst[64],device[128], command[128];char logEn[64];

	user[0]=0;
	hdr = ns_get_http_header(hm, "Cookie");
	if(hdr)ns_http_parse_header(hdr, "user", user, sizeof(user));
	
	memset(device,0,sizeof(device));memset(logEn,0,sizeof(logEn));
	pVars=hm->body.len<=0? &hm->query_string:&hm->body;
	/* Get form variables */
	ns_get_http_var(pVars, "callback", cb, sizeof(cb));
	ns_get_http_var(pVars, "_", _var, sizeof(_var));
	ns_get_http_var(pVars, "t", t, sizeof(t));
	ns_get_http_var(pVars, "src", src, sizeof(src));// ems
	ns_get_http_var(pVars, "dst", dst, sizeof(dst));// rics
	ns_get_http_var(pVars, "command", command, sizeof(command));
	ns_get_http_var(pVars, "device", device, sizeof(device));
	ns_get_http_var(pVars, "logEn", logEn, sizeof(logEn));
	if(logEn[0]&&logEn[0]=='1'){;}

	on_rics_ajax_call(user,nc->mgr,nc,command,device,cb,logEn);
}
void handle_rics_websock_connect(struct ns_connection *nc,struct http_message *hm)
{
	char user[20+1]; // MAX_USER_LEN
	struct ns_str *hdr;
	char device[PATH_MAX];char serialSendFmt[64];char logEn[64];
	struct ns_str* protocol=hm? ns_get_http_header(hm,"Sec-WebSocket-Protocol"):NULL;

	user[0]=0;
	hdr = ns_get_http_header(hm, "Cookie");
	if(hdr)ns_http_parse_header(hdr, "user", user, sizeof(user));
	
	memset(device,0,sizeof(device));memset(serialSendFmt,0,sizeof(serialSendFmt));memset(logEn,0,sizeof(logEn));
	ns_get_http_var(&hm->query_string, "device", device, sizeof(device));// for query not body
	ns_get_http_var(&hm->query_string, "serialSendFmt", serialSendFmt, sizeof(serialSendFmt));// for query not body
	ns_get_http_var(&hm->query_string, "logEn", logEn, sizeof(logEn));// for query not body
	if(logEn[0]&&logEn[0]=='1'){;}
#if 1
	on_rics_websock_connect(user,nc->mgr,nc,device,(protocol? protocol->p:NULL),serialSendFmt,logEn);
#else
	{
	ricsWebsock_conn_param* cp=NULL;
	rics_websock_add(device,nc);
	nc->user_data=(cp=(ricsWebsock_conn_param*)calloc(1,sizeof(ricsWebsock_conn_param)));//register my data// RMCP_RICS_WEBSOCK
	cp->cpType=RMCP_RICS_WEBSOCK;
	strcpy(cp->device,device);
	cp->ricsUds_nc_XY=(g_ricsMan_cp->ricsUds_nc);//ricsWebsock_conn_param*
	printf("client(%p) created(cp=%p(%d),user='%s',device='%s').\n",nc,cp,cp->cpType,user,cp->device);
	if(protocol&&strstr(protocol->p,"binary")!=NULL)cp->binaryCon=1;
	if(serialSendFmt[0])strcpy(cp->serialSendFmt,serialSendFmt);
	else strcpy(cp->serialSendFmt,"URIencode");
	}
#endif	
}
#else
void mg_printf_data_last(struct mg_connection *c);
static void handle_rics_reply(struct mg_connection *conn, const char* output,int outputLen,const char* jsonp_cb,const char * device)
{

	char LengthStr[30];

	on_rics_reply_msg(mg_get_nc_conn(conn)->mgr,device,output,outputLen);
	  /* Send headers */
	mg_send_status(conn, 200);
	mg_send_header(conn, "Content-Type", "application/json");
	  /* send it back*/
// printf("sending to client%p from %s('%.*s')\n",conn,device,outputLen,output);fflush(stdout);
	if(jsonp_cb){// this is DEFAULT
		char quoted[32768];int nPtr=0;

		if(bRICS_ResponseFileDataURLencodeAJAX){
			// json_emit_quoted_str(quoted,sizeof(quoted),output,outputLen);
			NS_url_encode(output,outputLen,quoted,sizeof(quoted));

			sprintf(LengthStr,"%u",(unsigned int)(strlen(jsonp_cb)+2 +4 +10 +strlen(device)+2 +2+8 +strlen(quoted)+2));
			mg_send_header(conn, "Content-Length", LengthStr);
			mg_printf_data(conn, "%s([{\"device\": \"%s\", \"data\": \"%.*s\"}])",jsonp_cb,device,strlen(quoted),quoted);
		}
		else{
			nPtr+=sprintf(quoted+nPtr,"%s([{\"device\":\"%s\",\"jsondata\":\"",jsonp_cb,device);
			jss_encode(quoted+nPtr,sizeof(quoted)-nPtr,output,outputLen,0);nPtr+=strlen(quoted+nPtr);
			nPtr+=sprintf(quoted+nPtr,"\"}])");
			sprintf(LengthStr,"%u",nPtr);
			mg_send_header(conn, "Content-Length", LengthStr);
			mg_send_data(conn,quoted,nPtr);
		}
	}
	else{
		sprintf(LengthStr,"%u",outputLen);
		mg_send_header(conn, "Content-Length", LengthStr);
		mg_send_data(conn,output,  outputLen);
	}
	mg_printf_data_last(conn);// Zero-Length CHUNK!!
	mg_get_nc_conn(conn)->flags |= NSF_FINISHED_SENDING_DATA;
}
static int handle_rics_call(struct mg_connection *conn) 
{
	char user[20+1]; // MAX_USER_LEN
	char cb[64],_var[128],t[64]; int isJSONpc=0;//cb:_jsonpcallback
	char device[128], dst[64], src[64],command[128];char logEn[64];

	user[0]=0;
	mg_get_cookie(conn, "user", user, sizeof(user));
	memset(device,0,sizeof(device));memset(logEn,0,sizeof(logEn));
	cb[0]=0;_var[0]=0;t[0]=0;
	// Get form variables
	isJSONpc=(mg_get_var(conn, "callback", cb, sizeof(cb))>0);// is_jsonp //jQueryXXXXX_time1
	if(isJSONpc)mg_get_var(conn, "_", _var, sizeof(_var));// time2
	mg_get_var(conn, "t", t, sizeof(t));// time0 // if POST not required
	mg_get_var(conn, "src", src, sizeof(src));// ems
	mg_get_var(conn, "dst", dst, sizeof(dst));// rics
	mg_get_var(conn, "command", command, sizeof(command));
	mg_get_var(conn, "device", device, sizeof(device));
	mg_get_var(conn, "logEn", logEn, sizeof(logEn));// for query or content!!
	if(logEn[0]&&logEn[0]=='1'){;}

	// printf("handle_rics_call: user='%s', request_nc=%p, device='%s', command='%s'\n",user,mg_get_nc_conn(conn),device,command);
	return on_rics_ajax_call(user,mg_get_nc_conn(conn)->mgr,conn,command,device,cb,logEn);
}
void handle_rics_websock_connect(struct mg_connection *conn)
{
	char user[20+1]; // MAX_USER_LEN
	char device[PATH_MAX];char serialSendFmt[64];char logEn[64];
	const char *protocol=mg_get_header(conn, "Sec-WebSocket-Protocol");

	user[0]=0;
	mg_get_cookie(conn, "user", user, sizeof(user));
	memset(device,0,sizeof(device));memset(serialSendFmt,0,sizeof(serialSendFmt));memset(logEn,0,sizeof(logEn));
	mg_get_var(conn, "device", device, sizeof(device));// for query
	mg_get_var(conn, "serialSendFmt", serialSendFmt, sizeof(serialSendFmt));// for query
	mg_get_var(conn, "logEn", logEn, sizeof(logEn));// for query
	if(logEn[0]&&logEn[0]=='1'){;}

#if 1
	on_rics_websock_connect(user,mg_get_nc_conn(conn)->mgr,conn,device,protocol,serialSendFmt,logEn);
#else
	{
	ricsWebsock_conn_param* cp=NULL;
	rics_websock_add(device,conn);
	conn->connection_param=(cp=(ricsWebsock_conn_param*)calloc(1,sizeof(ricsWebsock_conn_param)));//register my data// RMCP_RICS_WEBSOCK
	cp->cpType=RMCP_RICS_WEBSOCK;
	strcpy(cp->device,device);
	cp->ricsUds_nc_XY=(g_ricsMan_cp->ricsUds_nc);//ricsWebsock_conn_param*
	printf("client(%p) created(cp=%p(%d),user='%s',device='%s').\n",conn,cp,cp->cpType,user,cp->device);
	if(protocol&&strstr(protocol,"binary")!=NULL)cp->binaryCon=1;
	if(serialSendFmt[0])strcpy(cp->serialSendFmt,serialSendFmt);
	else strcpy(cp->serialSendFmt,"URIencode");
	}
#endif	
}
#endif	

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef NS_FOSSA_VERSION
static struct ns_serve_http_opts s_http_server_opts;
static int is_websocket(const struct ns_connection *nc) {
  return nc->flags & NSF_IS_WEBSOCKET;
}
static void broadcast(struct ns_connection *nc, const char *msg, size_t len) {
  struct ns_connection *c;
  char buf[500];

  snprintf(buf, sizeof(buf), "%p %.*s", nc, (int) len, msg);
  for (c = ns_next(nc->mgr, NULL); c != NULL; c = ns_next(nc->mgr, c)) {
    ns_send_websocket_frame(c, WEBSOCKET_OP_TEXT, buf, strlen(buf));
  }
}
static void handle_sum_call(struct ns_connection *nc, struct http_message *hm) {
  char n1[100], n2[100];
  double result;

  /* Get form variables */
  ns_get_http_var(&hm->body, "n1", n1, sizeof(n1));// or on query_string
  ns_get_http_var(&hm->body, "n2", n2, sizeof(n2));// or on query_string

  /* Send headers */
  ns_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

  /* Compute the result and send it back as a JSON object */
  result = strtod(n1, NULL) + strtod(n2, NULL);
  ns_printf_http_chunk(nc, "{ \"result\": %lf }", result);
  ns_send_http_chunk(nc, "", 0);  /* Send empty chunk, the end of response */
}

//port:13080,url=rics,wsUrl=websocket
void ws_handler(struct ns_connection *nc, int ev, void *ev_data) {//Websock Handler(client)
  struct http_message *hm = (struct http_message *) ev_data;
  struct websocket_message *wm = (struct websocket_message *) ev_data;
  char device_id[PATH_MAX];

  switch (ev) {
    case NS_HTTP_REQUEST:// MG_REQUEST
	  if (ns_vcmp(&hm->uri, "/rics") == 0) {
	  	rics_ajaxsock_add(nc);// ??
	  	handle_rics_call(nc, hm);return;
	  } else if(memcmp(hm->uri.p,"/ws2con",7)==0){
  		ns_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
		  ns_printf_http_chunk(nc, "websocket connection expected");
		  ns_send_http_chunk(nc, "", 0);  /* Send empty chunk, the end of response */
	  } else if (ns_vcmp(&hm->uri, "/printcontent") == 0) {
	        const char* buf;int len;// char buf[100] = {0};
	        len=hm->body.len<=0? hm->query_string.len:hm->body.len;buf=hm->body.len<=0? hm->query_string.p:hm->body.p;
	        printf("%.*s\n", len,buf);
		  /* Send headers */
		  ns_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
		  ns_printf_http_chunk(nc, "%.*s\n", len,buf);
		  ns_send_http_chunk(nc, "", 0);  /* Send empty chunk, the end of response */
	  } else if (ns_vcmp(&hm->uri, "/bcastcontent") == 0) {
	        const char* buf;int len;// char buf[100] = {0};
	        len=hm->body.len<=0? hm->query_string.len:hm->body.len;buf=hm->body.len<=0? hm->query_string.p:hm->body.p;
	        printf("%.*s\n", len,buf);
		  /* Send headers */
		  ns_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
		  ns_printf_http_chunk(nc, "%.*s\n", len,buf);
		  ns_send_http_chunk(nc, "", 0);  /* Send empty chunk, the end of response */
		 broadcast(nc,buf,len);
	  } else if(ns_vcmp(&hm->uri, "/api/v1/sum") == 0){
	        handle_sum_call(nc, hm);                    /* Handle RESTful call */
         } else{
	      /* Usual HTTP request - serve static files */
	      ns_serve_http(nc, hm, s_http_server_opts);/* Serve static content */
	  }
      nc->flags |= NSF_SEND_AND_CLOSE;
      break;
    case NS_WEBSOCKET_HANDSHAKE_REQUEST: // MG_WS_HANDSHAKE(for server)
    	{
		struct ns_str* ver=hm? ns_get_http_header(hm,"Sec-WebSocket-Version"):NULL;
		struct ns_str* protocol=hm? ns_get_http_header(hm,"Sec-WebSocket-Protocol"):NULL;
		struct ns_str* key=hm? ns_get_http_header(hm,"Sec-WebSocket-Key"):NULL;

		if(!ver||!key)return;
		printf("(ws2soc/rics_websocket)client MG_WS_HANDSHAKE: ver='%.*s',protocol='%.*s',key='%.*s'\n",(int)ver->len,ver->p,(int)(protocol? protocol->len:0),(protocol? protocol->p:NULL),(int)key->len,key->p);fflush(stdout);
		nc->flags &= ~NSF_CLOSE_IMMEDIATELY;
		printf("client(%p) MG_WS_CONNECT:uri=%.*s ?%.*s\n",nc,(int)hm->uri.len,hm->uri.p,(int)hm->query_string.len,hm->query_string.p);
		if (nc&&is_websocket(nc)&&(!memcmp(hm->uri.p,"/websocket",10))) {
			handle_rics_websock_connect(nc,hm);
		}
		else if (nc&&is_websocket(nc)&&(!memcmp(hm->uri.p,"/ws2tcp",7)||!memcmp(hm->uri.p,"/ws2udp",7)||!memcmp(hm->uri.p,"/ws2soc",7))) {
			handle_ws2soc_websock_connect(nc,hm);
		}
    	}
	break;
    case NS_WEBSOCKET_FRAME:// MG_REQUEST
	{
		  ws2tcp_conn_param* cp0=(ws2tcp_conn_param*)nc->user_data;
	      /* New websocket message. Tell everybody. */
	      // broadcast(nc, (char *) wm->data, wm->size);
	      // printf("wRq%x_%d",wm->flags,wm->size);fflush(stdout);//must be WEBSOCKET_OPCODE_BINARY|0x80 
	      // printf("websock receive(nc=%p,cp=%p(%d),data='%.*s'\n",nc,cp0,cp0->cpType,wm->size,(char *) wm->data);
	      if(wm->size>0&&((wm->flags&0x7f)==WEBSOCKET_OP_TEXT||(wm->flags&0x7f)==WEBSOCKET_OP_BINARY)){
			// printf("wRq%x_%d(%02x)",wm->flags,wm->size,(unsigned char)wm->data[0]);fflush(stdout);
			  if (wm->size >=2 && !memcmp(wm->data, "\x03\xe9", 2)) {
			  	printf("ws2soc: websock discon by Data!!\n");
				ns_send_websocket_frame(nc, WEBSOCKET_OP_CLOSE, "",0);
				return;  // Close websocket
			  }
		       if(cp0->cpType==RMCP_WEBSOCKIFY_WEBSOCK){
				  struct ns_connection *pc = (struct ns_connection *)(cp0? cp0->pc:NULL);
				  
				if(cp0)cp0->numRx++;
				if(pc&&wm->size>0&&wm->data){
					log_SOCKSend(cp0->target,'A',(char *) wm->data, wm->size,cp0->logEnTime);
					ns_send(pc, (char *) wm->data, wm->size);//forward to TCP(target)
				}
		       }
			else if(cp0->cpType==RMCP_RICS_WEBSOCK)websocket2serialData(nc,(char *) wm->data, wm->size,(wm->flags&0x7f)==WEBSOCKET_OP_TEXT,cp0);
	      }
		 else if((wm->flags&0xf)==WEBSOCKET_OP_CLOSE){
		 	printf("ws2soc: websock discon by Remote!!\n");
			return;  // Close websocket
		 }
		 else if(wm->size==0){//Keep-alive
		 	;
		 }
    	}
      break;
    case NS_CLOSE:// MG_CLOSE
      /* Disconnect. Tell everybody. */
	// printf("client(%p) closing ...\n",nc);
    	// printf("ws2soc websocket(%p) closing..",nc);fflush(stdout);
      if (is_websocket(nc)||rics_websock_find_dev(nc,device_id)||websockify_websock_find(nc)) // rics_websock_find_dev is for websock_flag cleared CASE!!
      {
		  ws2tcp_conn_param* cp0=(ws2tcp_conn_param*)nc->user_data;
		  
	        ;//broadcast(nc, "left", 4);
	        if(!cp0){
			printf("cp0 of nc%p already cleard by child!\n",nc);
			if(websockify_websock_find(nc))websockify_websock_del(nc);
	        }
		 else if(cp0->cpType==RMCP_WEBSOCKIFY_WEBSOCK){
		  	struct ns_connection *pc = (struct ns_connection *)(cp0? cp0->pc:NULL);
		  
			printf("client(%p) closed(hm=%p,DEVtx=%d/%d,DEVrx=%d/%d/%d)(add=%d,sub=%d).\n",nc,hm,(cp0? cp0->numTx:-1),(cp0? cp0->numTxComp:-1),(cp0? cp0->numRx0:-1),(cp0? cp0->numRx:-1),(cp0? cp0->numRxComp:-1),g_num_ws2socAdd,g_num_ws2socSub);
			if (nc/*&&(!memcmp(hm->uri.p,"/ws2tcp",7)||!memcmp(hm->uri.p,"/ws2udp",7)||!memcmp(hm->uri.p,"/ws2soc",7))*/) {// on client disconnect -> start target disconnect
				if(pc){
					printf("target(%p) close requested(tmp='%s').\n",pc,cp0->tmp);	
					pc->user_data=NULL;NS_close_conn(pc);// pc->flags|=NSF_CLOSE_IMMEDIATELY;//
					if(cp0->tmp[0]&&strstr(cp0->tmp,"/"))unlink(strstr(cp0->tmp,"/"));
#if INCLUDE_UART2TCP_SUPPORT
					else if(cp0->tmp[0]&&strstr(cp0->tmp,"uart:")){
						printf("UART? close fd=%d\n",pc->sock);
						// write(pc->sock,"bye UART\r\n",10);
						UART_close_dev(pc->sock);
					}
					else if(cp0->tmp[0]&&strstr(cp0->tmp,"fd:")){
						;
					}
#endif					
					free(nc->user_data);nc->user_data=NULL;
				}
				websockify_websock_del(nc);
			}
	        }
		else if(cp0->cpType==RMCP_RICS_WEBSOCK){
		  	ricsWebsock_conn_param* cp1=(ricsWebsock_conn_param*)nc->user_data;
			printf("client(%p) closed(ricsUds_nc=%p,ws=%d)\n",nc,cp1->ricsUds_nc_XY,is_websocket(nc));
			rics_websock_del(cp1->device,nc);
			
			free(nc->user_data);nc->user_data=NULL;
		}
      }
      else{
	  	rics_ajaxsock_del(nc);// ??// must use rics_requester_find & rics_requester_del
	  	printf("non-ws client(%p) closed\n",nc);// listen socket!!
      }
      break;
    case NS_POLL:// MG_POLL
	// ;	
   	break;
    case NS_WEBSOCKET_HANDSHAKE_DONE:// MG_WS_CONNECT(for server&client)
    	printf("(ws2soc/rics_websocket)client MG_WS_HANDSHAKE done(%p):hm=%p!!\n",nc,hm);fflush(stdout);
      /* New websocket connection. Tell everybody. */
      // broadcast(nc, "joined", 6);
      break;
    case NS_WEBSOCKET_CONTROL_FRAME:
		break;
    default:
      break;
  }
}
int* pQuitVar=NULL;
static void *serve_thread_func(void *param) {
  struct ns_mgr *mgr = (struct ns_mgr *) param;
  
#if TICB_PYRICS_EMUL
#if RICS_CONTABLE_MUTEX_LOCK	
  if(pthread_mutex_init((pthread_mutex_t*)&g_lockR,NULL)!=0)printf("g_lockR init error!!\n");
  else printf("g_lockR created!\n");
#endif
  rics_receiver_create(mgr);
#endif
  printf("ws2soc: starting...\n");
  while (*pQuitVar == 0) {
    ns_mgr_poll(mgr, 200);
#if INCLUDE_RICS_UDP_CON	
    on_mgr_poll(mgr);
#endif
  }
  printf("ws2soc: + ending..\n");
#if TICB_PYRICS_EMUL
  rics_receiver_destroy(mgr);
#if RICS_CONTABLE_MUTEX_LOCK	
  if(pthread_mutex_destroy((pthread_mutex_t*)&g_lockR)!=0)printf("g_lockR destroy error!!\n");
  else printf("g_lockR destroyed!\n");
#endif	
#endif
  printf("ws2soc: > end.\n");
  ns_mgr_free(mgr);
  printf("ws2soc: - end.\n");
  return NULL;
}

void ws_create_server(const char *address,int* quit)
{//13080:address
	static struct ns_mgr mgr;
	struct ns_connection *nc;
	
  	ns_mgr_init(&mgr, NULL);

	nc = ns_bind(&mgr, address, ws_handler);
	s_http_server_opts.document_root = ".";
	ns_set_protocol_http_websocket(nc);

	printf("NC> ws2soc: Started on port %s\n", address);
	pQuitVar=quit;
#if 1
	ns_start_thread(serve_thread_func, &mgr);
#else
	while (*quit== 0) {
		ns_mgr_poll(&mgr, 200);
	}
	printf("NC> ws2soc: ending...\n");
	ns_mgr_free(&mgr);
#endif	
}
#else
#ifdef MONGOOSE_SEND_NS_EVENTS
void* get_send_ns_event_callback_param(struct mg_connection *conn);// conn->callback_param
#endif
static int is_websocket(const struct mg_connection *conn) {
  return conn->is_websocket;
}
static int _ws_handler(struct mg_connection *conn, enum mg_event ev) {//Websock Handler(client)
  ws2tcp_conn_param* cp0=(ws2tcp_conn_param*)(conn? conn->connection_param:NULL);
  struct ns_connection *pc = (struct ns_connection *)(cp0? cp0->pc:NULL);

	// printf("ws_handler: cp0=%p,conn=%p,pc=%p\n",cp0,conn,pc);fflush(stdout);
#ifdef MONGOOSE_SEND_NS_EVENTS
  typedef struct {struct ns_connection *nc;int *p;}callback_param_V;//0: nc, 1:p
  // typedef void* callback_param_V[2];
  typedef callback_param_V* callback_param_t;
    if((int)ev==NS_RECV){
		// void *Conn=(void *)conn;
		callback_param_t callback_param=(callback_param_t)get_send_ns_event_callback_param(conn);
		struct ns_connection *nc=(struct ns_connection *)(callback_param? callback_param->nc:NULL);int* p_n=(int*)(callback_param? callback_param->p:NULL);
		// struct ns_connection *nc=(struct ns_connection *)(callback_param? callback_param[0]:NULL);int* p_n=(int*)(callback_param? callback_param[1]:NULL);
		
	// printf("wR%d",*p_n);//(num recv!!)
	if(cp0&&cp0->cpType==RMCP_WEBSOCKIFY_WEBSOCK&&*p_n>0)cp0->numRx0++;
      return MG_FALSE;
    }
    else if((int)ev==NS_SEND){
		// void *Conn=(void *)conn;
		callback_param_t callback_param=(callback_param_t)get_send_ns_event_callback_param(conn);
		struct ns_connection *nc=(struct ns_connection *)(callback_param? callback_param->nc:NULL);int* p_n=(int*)(callback_param? callback_param->p:NULL);
		// struct ns_connection *nc=(struct ns_connection *)(callback_param? callback_param[0]:NULL);int* p_n=(int*)(callback_param? callback_param[1]:NULL);
		
	// printf("wS%d.",*p_n);//(num sent!!)
	if(cp0&&cp0->cpType==RMCP_WEBSOCKIFY_WEBSOCK&&*p_n>0)cp0->numTxComp++;
      return MG_FALSE;
    }
    else if((int)ev==NS_POLL){
	//printf("wP.");
      return MG_FALSE;
    }
#endif	
	
  switch (ev) {
    case MG_AUTH:
      return MG_TRUE;
    case MG_REQUEST:
      // printf("client(%p) recv %d bytes\n",conn,conn->content_len);	
      // printf("wRq%x_%d",conn->wsbits,conn->content_len);fflush(stdout);//must be WEBSOCKET_OPCODE_BINARY|0x80 
      if (is_websocket(conn)) {// uri is "websocket" for RICS
        // // Simple websocket echo server
        // mg_websocket_write(conn, WEBSOCKET_OPCODE_BINARY, conn->content, conn->content_len);
        if(conn->content_len>0&&((conn->wsbits&0x7f)==WEBSOCKET_OPCODE_TEXT||(conn->wsbits&0x7f)==WEBSOCKET_OPCODE_BINARY)){
		// printf("wRq%02x(%.*s)",conn->wsbits,conn->content_len,conn->content);fflush(stdout);//must be WEBSOCKET_OPCODE_BINARY|0x80 
		  if (conn->content_len >=2 && !memcmp(conn->content, "\x03\xe9", 2)) {
		  	printf("ws2soc: websock discon by Data!!\n");
			mg_websocket_printf(conn, WEBSOCKET_OPCODE_CONNECTION_CLOSE, "");
			return MG_FALSE;  // Close websocket
		  }

			if(cp0->cpType==RMCP_WEBSOCKIFY_WEBSOCK){
				if(cp0)cp0->numRx++;
				if(pc&&conn->content_len>0&&conn->content){
					log_SOCKSend(cp0->target,'A',(char *) conn->content, conn->content_len,cp0->logEnTime);
					ns_send(pc, conn->content, conn->content_len);//forward to TCP(target)
				}
			}
			else if(cp0->cpType==RMCP_RICS_WEBSOCK)websocket2serialData(mg_get_nc_conn(conn),(char *) conn->content, conn->content_len,(conn->wsbits&0x7f)==WEBSOCKET_OPCODE_TEXT,cp0);
			return MG_TRUE;
         }
		 else if((conn->wsbits&0xf)==WEBSOCKET_OPCODE_CONNECTION_CLOSE){
		 	printf("ws2soc: websock discon by Remote!!\n");
			return MG_FALSE;  // Close websocket
		 }
		 else if(conn->content_len==0){//websocket keep-alive
			;
		 }
		 return MG_TRUE;	
      } 
      else if (memcmp(conn->uri, "/rics",5) == 0) {
		int ret;
			
		// printf("ws2soc: /rics +conn=%p\n",conn);
		rics_ajaxsock_add(conn);// ??
	  	ret=handle_rics_call(conn);
		if(ret!=MG_MORE){
#if 0			
			printf("ws2soc: /rics, +rics_ajaxsock_del(ret=%d)\n",ret);fflush(stdout);
			rics_ajaxsock_del(conn);// ??
			printf("ws2soc: /rics, -rics_ajaxsock_del\n");fflush(stdout);
#endif			
		}
		// printf("ws2soc: /rics -conn=%p\n",conn);
		return ret;
      } 
      else if (memcmp(conn->uri, "/test",5) == 0) {
	  printf("ws2soc: expected announce service for %s\n",conn->uri);
        mg_printf_data(conn, "%s", "websocket connection expected");
        return MG_TRUE;
      }
      else if (memcmp(conn->uri, "/index.html",10) == 0) {
	 printf("ws2soc: %s forward to 80 port server\n",conn->uri);
        mg_forward(conn, mg_is_ssl(conn)? "ssl://127.0.0.1:443":"127.0.0.1:80");
        return MG_MORE;
      }
      else if (memcmp(conn->uri, "/lib",4) == 0) {
	 printf("ws2soc: %s forward to 80 port server\n",conn->uri);
        mg_forward(conn, mg_is_ssl(conn)? "ssl://127.0.0.1:443":"127.0.0.1:80");
        return MG_MORE;
      }
	printf("ws2soc%p: static file service for %s\n",conn,conn->uri);
      return MG_FALSE;// for static-file-server!!
    case MG_POLL:
	// printf("client(%p) polling\n",conn);	
   	return MG_FALSE;
    case MG_WS_CONNECT:
		g_num_ws2socAdd++;
	printf("client(%p/nc=%p) MG_WS_CONNECT:uri=%s ?%s\n",conn,mg_get_nc_conn(conn),conn->uri,conn->query_string);
	if (conn&&is_websocket(conn)&&conn->uri&&(!memcmp(conn->uri,"/foo",4))) {
		printf("TICB websocket%p forward\n",conn);
		mg_forward(conn, mg_is_ssl(conn)? "ssl://127.0.0.1:443":"127.0.0.1:80");
	       return MG_FALSE;
	} else 
	if (conn&&is_websocket(conn)&&conn->uri&&(!memcmp(conn->uri,"/websocket",10))) {
		handle_rics_websock_connect(conn);
		return MG_FALSE;
	} else 
	if (conn&&conn->is_websocket&&conn->uri&&(!memcmp(conn->uri,"/ws2tcp",7)||!memcmp(conn->uri,"/ws2udp",7)||!memcmp(conn->uri,"/ws2soc",7))) {
		handle_ws2soc_websock_connect(conn);
	    return MG_FALSE;
	}
	return MG_FALSE;// return value is not required!!
    case MG_WS_HANDSHAKE:// false is required for Handshake
    	{
		const char *ver = mg_get_header(conn, "Sec-WebSocket-Version"),
				*key = mg_get_header(conn, "Sec-WebSocket-Key"),
				*protocol=mg_get_header(conn, "Sec-WebSocket-Protocol");
		printf("(ws2soc/rics_websocket) client%p MG_WS_HANDSHAKE: ver='%s',protocol='%s',key='%s'\n",conn,ver,protocol,key);
    	}
#if 1	//TICS_RICS_FORWARD
	if(!strcmp((conn->uri? conn->uri:""),"/foo"))return MG_TRUE;// not to send_websocket_handshake
#endif
	return MG_FALSE;
    case MG_CLOSE:  //User Close!!  
    	// printf("ws2soc websocket(%p) closing..",conn);fflush(stdout);
	if (conn&&conn->is_websocket&&cp0&&cp0->cpType==RMCP_WEBSOCKIFY_WEBSOCK&&conn->uri&&(!memcmp(conn->uri,"/ws2tcp",7)||!memcmp(conn->uri,"/ws2udp",7)||!memcmp(conn->uri,"/ws2soc",7))) {// on client disconnect -> start target disconnect
		printf("ws2soc websocket(%p) closed(DEVtx=%d/%d,DEVrx=%d/%d/%d)(add=%d,sub=%d).\n",conn,(cp0? cp0->numTx:-1),(cp0? cp0->numTxComp:-1),(cp0? cp0->numRx0:-1),(cp0? cp0->numRx:-1),(cp0? cp0->numRxComp:-1),g_num_ws2socAdd,g_num_ws2socSub);
		if(pc){
			printf("target(%p) close requested(tmp=%s).\n",pc,cp0->tmp);	
			pc->user_data=NULL;NS_close_conn(pc);
			if(cp0->tmp[0]&&strstr(cp0->tmp,"/"))unlink(strstr(cp0->tmp,"/"));
#if INCLUDE_UART2TCP_SUPPORT
			else if(cp0->tmp[0]&&strstr(cp0->tmp,"uart:")){
				printf("UART? close fd=%d\n",pc->sock);
				// write(pc->sock,"bye UART\r\n",10);
				UART_close_dev(pc->sock);
			}
			else if(cp0->tmp[0]&&strstr(cp0->tmp,"fd:")){
				;
			}
#endif					
			free(conn->connection_param);conn->connection_param=NULL;
		}
		return MG_TRUE;
	}
	else if(conn&&is_websocket(conn)&&cp0&&cp0->cpType==RMCP_RICS_WEBSOCK){
		  	ricsWebsock_conn_param* cp1=(ricsWebsock_conn_param*)conn->connection_param;
			printf("RICS websocket(%p) closed(cp=%p(%d),device=%s,ricsUds_nc=%p)\n",conn,cp1,cp1->cpType,cp1->device,cp1->ricsUds_nc_XY);
			rics_websock_del(cp1->device,conn);
			
			free(conn->connection_param);conn->connection_param=NULL;
	}
#if 1	//TICS_RICS_FORWARD
	else if(conn&&is_websocket(conn)&&conn->uri&&!memcmp(conn->uri,"/foo",4)){
		printf("TICB-websocket(%p) closed\n",conn);
		return MG_TRUE;
	}
#endif	
	else if(conn&&!is_websocket(conn)){
		if(rics_ajaxsock_find(conn)>0){
			rics_ajaxsock_del(conn);// ??// must use rics_requester_find & rics_requester_del
			printf("RICS ajaxsock(%p) closed\n",conn);
		}
		else printf("nonRICS httpsocket(%p) closed\n",conn);
	}
	return MG_FALSE;
    default:
      return MG_FALSE;
  }
}
static int ws_handler(struct mg_connection *conn, enum mg_event ev) {//Websock Handler(client)
	int ret;
	// printf("+ ws_handler: conn=%p,ev=%d\n",conn,ev);fflush(stdout);
	ret=_ws_handler(conn,ev);
	// printf("- ws_handler: conn=%p,ev=%d\n",conn,ev);fflush(stdout);
	return ret;
}
#if TICB_PYRICS_EMUL
void rics_modules_add_auto(struct ns_mgr *mgr)
{char listdata[HB_POSTDATA_LEN_MAX],*devlist=listdata;int ret_data_len=0;
	char conffile[PATH_MAX],folder[PATH_MAX];

	strcpy(folder,"database");
	strcpy(conffile,"vgw128_kt.xml");//ricsTDX_kt.xml
	ret_data_len=ListFromVGWEMSdb(folder,conffile,listdata,sizeof(listdata));
	printf("rics_modules_add_auto: device list(%dbytes)='%s'\n",ret_data_len,listdata);
	do{char* ptr,*device_id;
		device_id=devlist;
		if((ptr=strstr(devlist,","))!=NULL){*ptr='\0';devlist=ptr+1;}//first or middle
		else devlist+=strlen(devlist);//last

		if(is_IPADDR(device_id)){
			printf("RICS: devid=%s\n",device_id);
			rics_module_add(mgr,NULL,device_id);
		}
	}while(*devlist);
}
int find_conn_func_remove(void* map,const char* device_id,void* ricsconn,void* Ctx,void* Ctx2,void* Ctx3)// this is called locked state
{(void)map;(void)Ctx3;
	struct ns_mgr *mgr=(struct ns_mgr*)Ctx;int* pCount=(int*)Ctx2;

	if(ricsconn){
		int ret=0;
		ricsConn *rc=(ricsConn*)ricsconn;
		
		printf("+find_conn_func_remove: %s removing\n",device_id);fflush(stdout);
		r_Unlock();
		rics_module_remove(mgr,NULL,device_id,1);
		r_Lock();
		printf("-find_conn_func_remove: %s removed\n",device_id);fflush(stdout);

		if(ret&&pCount)(*pCount)++;
	}
	else{
		printf("find_conn_func_remove: %s already removed\n",device_id);fflush(stdout);//impossible
	}
	return 0;//do-not stop Looping
}
void rics_modules_remove_auto(struct ns_mgr *mgr)
{	
#if 1
	int Count=0,ret=0;

	printf("+%ld rics_modules_remove_auto%p: Count=%d, ret=%d\n",time(NULL),mgr,Count,ret);fflush(stdout);
	ret=iterateAll_ricsconn(find_conn_func_remove,mgr,&Count,NULL);
	printf("-%ld rics_modules_remove_auto%p: Count=%d, ret=%d\n",time(NULL),mgr,Count,ret);fflush(stdout);
#else
	char listdata[HB_POSTDATA_LEN_MAX],*devlist=listdata;int ret_data_len=0;
	char conffile[PATH_MAX],folder[PATH_MAX];

	strcpy(folder,"database");
	strcpy(conffile,"vgw128_kt.xml");//ricsTDX_kt.xml
	ret_data_len=ListFromVGWEMSdb(folder,conffile,listdata,sizeof(listdata));
	printf("rics_modules_remove_auto: device list(%dbytes)='%s'\n",ret_data_len,listdata);
	do{char* ptr,*device_id;
		device_id=devlist;
		if((ptr=strstr(devlist,","))!=NULL){*ptr='\0';devlist=ptr+1;}//first or middle
		else devlist+=strlen(devlist);//last

		if(is_IPADDR(device_id)){
			printf("RICS: devid=%s\n",device_id);
			rics_module_remove(mgr,NULL,device_id,1);
		}
	}while(*devlist);
#endif
}

struct mg_server *def_RICS_server=NULL;
// const char *g_wsWrap_listen="13080",*g_wssWrap_listen="ssl://13081:" SSL_DEF_CERT_PEM; // S2_PEM ":" CA2_PEM;
const char *g_wsWrap_listen_ports="13080,ssl://13081:" SSL_DEF_CERT_PEM; // S2_PEM ":" CA2_PEM;
#else
// const char *g_wsWrap_listen="9001",*g_wssWrap_listen="ssl://9002:" SSL_DEF_CERT_PEM; // S2_PEM ":" CA2_PEM;
const char *g_wsWrap_listen_ports="9001,ssl://13081:" SSL_DEF_CERT_PEM; // S2_PEM ":" CA2_PEM;
#endif
#if 0
#define SERVER_POLL_WAIT_TIME 500	// for fast serial-send!!
#else
#define SERVER_POLL_WAIT_TIME 5000	// 200
#endif
static void *serve_thread_func(void *param) {
  struct mg_server *server = (struct mg_server *) param;
  
#if TICB_PYRICS_EMUL
	if(def_RICS_server==server){
#if RICS_CONTABLE_MUTEX_LOCK	
		if(pthread_mutex_init((pthread_mutex_t*)&g_lockR,NULL)!=0)printf("g_lockR init error!!\n");
		else printf("g_lockR created!\n");
#endif
		rics_receiver_create((struct ns_mgr*)server);
		sleep(2);
		rics_modules_add_auto((struct ns_mgr*)server);
	}
#endif
  printf("ws2soc: Listening on port %s\n", mg_get_option(server, "listening_port"));
  while (exit_flag == 0) {
   // printf("+poll thread %p@%u\n",server,(unsigned int)time(NULL));fflush(stdout);
    mg_poll_server(server, SERVER_POLL_WAIT_TIME);
   // printf("-poll thread %p@%u\n",server,(unsigned int)time(NULL));fflush(stdout);
#if INCLUDE_RICS_UDP_CON	
    if(def_RICS_server==server)on_mgr_poll((struct ns_mgr*)server);
#endif
  }
  printf("ws2soc: + ending...\n");
#if TICB_PYRICS_EMUL
  if(def_RICS_server==server){
  	sleep(1);
  	rics_modules_remove_auto((struct ns_mgr*)server);
	rics_receiver_destroy((struct ns_mgr*)server); // this is double free??
#if RICS_CONTABLE_MUTEX_LOCK	
	if(pthread_mutex_destroy((pthread_mutex_t*)&g_lockR)!=0)printf("g_lockR destroy error!!\n");
	else printf("g_lockR destroyed!\n");
#endif	
  }
#endif  
  printf("ws2soc: > end.\n");
  mg_destroy_server(&server);
  printf("ws2soc: - end.\n");
  return NULL;
}
void ws_create_server(const char* work_dir)
{
  struct mg_server *ws1_server = mg_create_server(NULL, ws_handler);// user-connections
#if USE_ANOTHER_RICS_SERVER  
  struct mg_server *ws2_server = mg_create_server(NULL, ws_handler);// worker
#else
  struct mg_server *ws2_server = NULL;
#endif

  printf("ws1_server%p,ws2_server%p created\n",ws1_server,ws2_server);
  def_RICS_server=ws2_server? ws2_server:ws1_server;
	
  mg_set_option(ws1_server, "listening_port", g_wsWrap_listen_ports);// user-connections
  mg_set_option(ws1_server, "document_root", ".");
  if(ws2_server){
  	mg_set_option(ws2_server, "document_root", ".");
	mg_copy_listeners(ws1_server, ws2_server);
  }

  if(work_dir)chdir(work_dir);
  
  mg_start_thread(serve_thread_func, ws1_server);
  if(ws2_server)mg_start_thread(serve_thread_func, ws2_server);
}
void ws_end_server(void)
{
  printf("%ld ws_servers ending(dummy)\n",(unsigned long)time(NULL));
}
#endif
#endif
// for rics_test(EMS)
// domain socket '/tmp/ems_rics' listen
// for webUI
// 'ws://localhost:13080/websocket'
	// '{"side": "%s", "data": "%s"}' ,data is encode urllib.quote(xx)
	// 'http://localhost:13080/rics'
// rics: AJAX
	// postData: src,dst,command,device,callbackt,_
	// INIT,QUIT command --> forward to rics_test
	// response: INIT_RES OK
	// response: QUIT_RES OK
	// etc cmd -> LongPoll
	// response: [{'device': _device, 'data': data}]
// for rics_test(c_prog)
// domain socket '/tmp/rics/'+device_id+_+side

/////////////////////////to add serial port support//////////////////////////////////////
// to add a file descriptor to net_skeleton
//  struct ns_connection *ns_add_sock(struct ns_mgr *s, sock_t sock, ns_callback_t callback, void *user_data);
//		static void ns_add_conn(struct ns_mgr *mgr, struct ns_connection *c);
//	static void ns_close_conn(struct ns_connection *conn);
//  	static void ns_remove_conn(struct ns_connection *conn);
// extern int /*vgwCardMan::*/UART_open_dev(int uartNo);
// extern void /*vgwCardMan::*/UART_close_dev(int sd);
// extern int /*vgwCardMan::*/OnSerialReceive(int sock,void *sock_ctx);
// int /*vgwCardMan::*/OnSerialMessage(int sock,char* strSerialMsg);
