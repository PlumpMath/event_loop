#ifndef RPC_OUTPUT_BUF_SIZE
#define RPC_OUTPUT_BUF_SIZE	50000	
#endif
#ifndef VGW_LOG_PATH
#define VGW_LOG_PATH	"../db/"
#endif

#include <sys/timeb.h> 
static long g_tz_minuteswest_ns=-540;
static void set_TZ_variable_ns(void){
	struct timeb tb;
	ftime(&tb);
	g_tz_minuteswest_ns=(long)tb.timezone;
	printf("set_TZ_variable: g_tz_minuteswest_ns=%ld\n",g_tz_minuteswest_ns);
}
#define GET_MINUTESEAST_NS()		(-(long)g_tz_minuteswest_ns)	/*-540 -> +540*/
#define GET_SECEAST_NS()			(GET_MINUTESEAST_NS()*60)		/*32400*/

time_t get_unixtime_from_timestamp(const char *timeStamp)
{
	struct tm Tm;// char weekdayStr[20],TZstr[64];
	sscanf(timeStamp,"%04d-%02d-%02d %02d:%02d:%02d",&Tm.tm_year,&Tm.tm_mon,&Tm.tm_mday,&Tm.tm_hour,&Tm.tm_min,&Tm.tm_sec);
	Tm.tm_year-=1900;Tm.tm_mon-=1;Tm.tm_isdst=0;Tm.tm_wday=0;Tm.tm_yday=0;
	return mktime(&Tm);
}
void proc_LOG_service(struct ns_connection *nc, struct iobuf *pIo,const char* data,int dataInLen,int appendNULL)
{
	struct json_token params[32];int n;
	int strLen=data[dataInLen-1]!=0?  dataInLen: dataInLen-1;
	
	memset(&params[0],0,sizeof(params));
	n = parse_json((const char*)data, strLen, params, sizeof(params) / sizeof(params[0]));
	if(n<=0){
		printf("\nproc_LOG_service: jsonArray err %d(data %.*s)\n",n,strLen,(const char*)data);
		goto EndProc;
	}
	if(0){int i;
	printf("%d bytes parse_json\n",n);
	for(i=0;i<3;i++)printf("[%d] type=%d,num_desc=%d\n",i,params[i].type,params[i].num_desc);
	}
	if(params[0].type==JSON_TYPE_OBJECT&&params[0].num_desc>=6){//command,device,data string members
		char device[128],command[128],stStr[64],etStr[64],userIdStr[32],*logdata=NULL;
		unsigned long st,et,userId;
		int i;int log_data_len=0;int isUTC=GET_SECEAST_NS();
	  	const char* strTableName0="vgw128_kt_log.xml";
		char path[PATH_MAX],path_def[PATH_MAX];
	  	char src[1024];int headLen=0;

		command[0]='\0';
		  for (i = 0; i < params[0].num_desc; i++) {
		    if (params[i + 1].type != JSON_TYPE_STRING) {
		      goto EndProc; //JSON_RPC_INVALID_PARAMS_ERROR;
		    }
		    if(params[i + 1].type == JSON_TYPE_STRING){
		    	if(!memcmp(params[i + 1].ptr,"command",7)){// LOGSAVE,LOGLOAD
				jstrcpy(command,params[i + 2].ptr, params[i + 2].len);
		    	}
			else if(!memcmp(params[i + 1].ptr,"device",6)){
				jstrcpy(device,params[i + 2].ptr, params[i + 2].len);
		    	}
			else if(!memcmp(params[i + 1].ptr,"st",2)){// LOGLOAD
				jstrcpy(stStr,params[i + 2].ptr, params[i + 2].len);st=(unsigned long)get_unixtime_from_timestamp(stStr); 
		    	}
			else if(!memcmp(params[i + 1].ptr,"et",2)){// LOGLOAD
				jstrcpy(etStr,params[i + 2].ptr, params[i + 2].len);et=(unsigned long)get_unixtime_from_timestamp(etStr); 
		    	}
			else if(!memcmp(params[i + 1].ptr,"userId",6)){// LOGLOAD
				jstrcpy(userIdStr,params[i + 2].ptr, params[i + 2].len);sscanf(userIdStr,"%08lx",&userId);
		    	}
			else if(!memcmp(params[i + 1].ptr,"data",4)){// LOGSAVE data or LOGLOAD filter
				int retLen;

				logdata=malloc(params[i + 2].len+2);
				// un-quote filedata is required!!
				retLen=UnquoteJsonString(logdata,params[i + 2].ptr,params[i + 2].len);
				logdata[retLen]=0;
		    	}
			else{
			      goto EndProc; //JSON_RPC_INVALID_PARAMS_ERROR;
			}
			i++;
		    }
		  }

		  // 
		  if(strcmp(command,"LOGLOAD")==0&&logdata){
			sprintf(path,"%s%s_%s",VGW_LOG_PATH,device,strTableName0);// +string(".log")
			sprintf(path_def,"%s%s_%s",VGW_LOG_PATH,"def",strTableName0);// +string(".log")
			strcpy(src,logdata);

			free(logdata);	logdata=malloc(HB_LOGDATA_LEN_MAX);

			sprintf(logdata,"dbmanager %08lx %s ",userId,device);// insert device(TID)

			headLen=strlen(logdata);
			log_data_len=HB_LOGDATA_LEN_MAX-headLen;
			st+=isUTC;et+=isUTC;//conv2LocalTime
		  	log_data_len=LoadLogFromVGWEMSfile(path,path_def,logdata+headLen,log_data_len-headLen,st,et,src,isUTC,",");
		  	if(log_data_len==-1) log_data_len = 0; // hdkang
		  	// printf("sending %d to LOGclient\n",headLen+log_data_len+(appendNULL? 1:0));
		  	ns_send(nc, logdata, headLen+log_data_len+(appendNULL? 1:0));  // include terminating \0
		  }else if(strcmp(command,"LOGSAVE")==0&&logdata){
			sprintf(path,"%s%s_%s",VGW_LOG_PATH,device,strTableName0);// +string(".log")
			sprintf(path_def,"%s%s_%s",VGW_LOG_PATH,"def",strTableName0);// +string(".log")
		  	// printf("recv %d from LOGclient\n",strlen(logdata));
		  	SaveLogToVGWEMSfile(path,path_def,logdata,strlen(logdata),0);
		  }
		  if(logdata)free(logdata);
	}
EndProc:      		
	;
}
void OnTCPLogServDataReceived(struct ns_connection *nc, struct iobuf *pIo,const char* data,int dataInLen)
{
	if(data[dataInLen-1]!=0)printf("NOT Null terminated!!\n");
	if(!memcmp(data,"{",1)||!memcmp(data,"[",1)){
		proc_LOG_service(nc, pIo,data,dataInLen,1);
	}
	else if(!memcmp(data,"PING",4))
      		ns_send(nc, "PONG", 4+1);  // include terminating \0
	else 
      		ns_send(nc, data, dataInLen);  // Echo message back  // include terminating \0
}
void OnUDPLogServDataReceived(struct ns_connection *nc, struct iobuf *pIo,const char* data,int dataInLen)
{
	// if(data[dataInLen-1]!=0)printf("NOT Null terminated!!\n");
	if(!memcmp(data,"{",1)||!memcmp(data,"[",1)){
		proc_LOG_service(nc, pIo,data,dataInLen,0);
	}
	else if(!memcmp(data,"PING",4))
      		ns_send(nc, "PONG", 4+1);  // include terminating \0
	else 
      		ns_send(nc, data, dataInLen);  // Echo message back  // include terminating \0
}
typedef struct _LOGSRVCON_CP{
	int cpType;
	int tcpBufPos;
	char tcpBuf[32000000];
}LOGSRVCON_CP;
extern int countNullCharOnStream(const unsigned char* websockBuf,int len);
extern int isTerminatedRequestStr(const unsigned char* websockBuf,int len);
static void logServ_TCP_ev_handler(struct ns_connection *nc, int ev, void *p) {
  struct iobuf *io = &nc->recv_iobuf;
  union socket_address* p_sa=(union socket_address*)p;
  LOGSRVCON_CP* cp=(LOGSRVCON_CP*)(nc&&nc->user_data? nc->user_data:NULL);
	int numNull=0;
  (void) p;

//	if(ev!=NS_POLL){fprintf(stderr,"logServ_TCP_ev_handler(%p): message %s from %s:%d\n",nc,ns_ev2str(ev),inet_ntoa(nc->sa.sin.sin_addr),ntohs(nc->sa.sin.sin_port));fflush(stderr);}
  switch (ev) {
  case NS_ACCEPT:
  	// {fprintf(stderr,"logServ_TCP_ev_handler(%p): message %s from %s:%d\n",nc,ns_ev2str(ev),inet_ntoa(p_sa->sin.sin_addr),ntohs(p_sa->sin.sin_port));fflush(stderr);}
		nc->user_data=calloc(1,sizeof(LOGSRVCON_CP));
		cp=(LOGSRVCON_CP*)nc->user_data;
		cp->cpType=997;
		cp->tcpBufPos=0;cp->tcpBuf[0]='\0';
		printf("logServ_TCP_ev_handler: accept\n");
		// ns_tcpSetKeepAlive(nc, 1, 10, 2, 5);// 1 (KEEPALIVE ON),최초에 세션 체크 시작하는 시간 (단위 : sec),최초에 세션 체크 패킷 보낸 후, 응답이 없을 경우 다시 보내는 횟수 (단위 : 양수 단수의 갯수),TCP_KEEPIDLE 시간 동안 기다린 후, 패킷을 보냈을 때 응답이 없을 경우 다음 체크 패킷을 보내는 주기 (단위 : sec)
  	break;
    case NS_RECV:
	// null-termination packetization must be HERE!!
	if((numNull=countNullCharOnStream((const unsigned char*)io->buf,io->len))>=1){
		unsigned int ioBufPos=0,idxNull=0;
		do{
			int strLen=strlen(io->buf+ioBufPos)+1;
			if(g_debugLevel>=1){printf("logServ_TCP_ev_handler(%p):buf %d last\n",nc,strLen);fflush(stdout);}
			if(cp->tcpBufPos+strLen>(int)sizeof(cp->tcpBuf)){cp->tcpBufPos=0;goto EndProc;}// no-copy clear Buf
			memcpy(cp->tcpBuf+cp->tcpBufPos,io->buf+ioBufPos,strLen);cp->tcpBufPos+=strLen;ioBufPos+=strLen;idxNull++;
			if(isTerminatedRequestStr((const unsigned char*)cp->tcpBuf,cp->tcpBufPos)){
				OnTCPLogServDataReceived(nc,io,(const char*)cp->tcpBuf,cp->tcpBufPos);
			}
			else printf("logServ_TCP_ev_handler: Bad null terminated dbSock request(%.*s)!!\n",cp->tcpBufPos,cp->tcpBuf);
	      		cp->tcpBufPos=0;// clear Buf
		}while(ioBufPos<io->len&&(int)idxNull<numNull);
		if(ioBufPos<io->len){
			printf("logServ_TCP_ev_handler(%p):buf %d remain\n",nc,io->len-ioBufPos);fflush(stdout);
			if(cp->tcpBufPos+io->len-ioBufPos>(int)sizeof(cp->tcpBuf)){cp->tcpBufPos=0;goto EndProc;}// no-copy clear Buf
			memcpy(cp->tcpBuf+cp->tcpBufPos,io->buf+ioBufPos,io->len-ioBufPos);cp->tcpBufPos+=io->len-ioBufPos;
		}
	}
	else{
		if(g_debugLevel>=2){printf("logServ_TCP_ev_handler(%p):buf %d frag\n",nc,io->len);fflush(stdout);}
		if(cp->tcpBufPos+io->len>(int)sizeof(cp->tcpBuf)){cp->tcpBufPos=0;goto EndProc;}// no-copy clear Buf
		memcpy(cp->tcpBuf+cp->tcpBufPos,io->buf,io->len);cp->tcpBufPos+=io->len;
		// printf("checking self termReq(%d,'%.*s')\n",cp->tcpBufPos,cp->tcpBufPos,cp->tcpBuf);
		if(isTerminatedRequestStr((const unsigned char*)cp->tcpBuf,cp->tcpBufPos)){
			OnTCPLogServDataReceived(nc,io,(const char*)cp->tcpBuf,cp->tcpBufPos);
	      		cp->tcpBufPos=0;// clear Buf
		}
		else printf("logServ_TCP_ev_handler: not null terminated dbSock request. wait more..\n");
	}
EndProc:	
      iobuf_remove(io, io->len);        // Discard message from recv buffer
      break;
	case NS_CLOSE:
		if(cp){
			cp->tcpBufPos=0;
			free(cp);cp=NULL;nc->user_data=NULL;
			printf("logServ_TCP_ev_handler: close\n");
		}
		break;
    default:
      break;
  }
}
static void logServ_UDP_ev_handler(struct ns_connection *nc, int ev, void *p) {// nc is not same as created_nc!!!(NS_RECV)
  struct iobuf *io = &nc->recv_iobuf;
  (void) p;// p is &n

  if(ev!=NS_POLL&&ev!=NS_RECV)printf("logServ_UDP_ev_handler(%p): message %s from %s:%d\n",nc,ns_ev2str(ev),inet_ntoa(nc->sa.sin.sin_addr),ntohs(nc->sa.sin.sin_port));

  switch (ev) {
    case NS_RECV:
#if 1		
	OnUDPLogServDataReceived(nc,io,io->buf,io->len);
#else
	if(io->buf[io->len-1]==0)printf("Null terminated!!\n");
	if(!memcmp(io->buf,"{",1)||!memcmp(io->buf,"[",1)){
		proc_LOG_service(nc, io,(const unsigned char*)io->buf,io->len,0);
	}
	else{
		memcpy(nc->user_data, io->buf, io->len); 	
		
		if(!memcmp(io->buf,"PING",4))
	      		ns_send(nc, "PONG", 4+1);   // include terminating \0
		else ns_send(nc, io->buf, io->len); // echo  
	}
#endif	
      iobuf_remove(io, io->len);        // Discard message from recv buffer
      break;
    default:
      break;
  }
}

#define VGWSOCK_LOGSERVER_PORT 9090
int logServ_TCPUDP_init(struct ns_mgr *mgr1,struct ns_mgr *mgr2)
{
	char* g_log_server_addrUDP="udp://127.0.0.1:9090";// UDP //VGWSOCK_LOGSERVER_PORT
	char* g_log_server_addrTCP="127.0.0.1:9090";// TCP //VGWSOCK_LOGSERVER_PORT
	// char* g_syslog_server_addrUDP="udp://127.0.0.1:9514";// or 514// UDP //VGWSYSLOGSOCK_UDP_PORT
	struct ns_connection *nc_tcp1;
	struct ns_connection *nc_udp1;
	// struct ns_connection *nc_udp2;
	static char buf2[1000] = "";
	(void)nc_tcp1;(void)nc_udp1;
		
	set_TZ_variable_ns();	
	nc_udp1=ns_bind(mgr1, g_log_server_addrUDP, logServ_UDP_ev_handler, buf2);
	// nc_udp2=ns_bind(mgr1, g_syslog_server_addrUDP, syslogServ_UDP_ev_handler, buf2);
	nc_tcp1=ns_bind(mgr1, g_log_server_addrTCP, logServ_TCP_ev_handler, NULL);
	return 0;
}
int logServ_TCPUDP_destroy(void)
{
	return 0;
}
