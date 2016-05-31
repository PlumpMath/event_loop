// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved
//
// This software is dual-licensed: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. For the terms of this
// license, see <http://www.gnu.org/licenses/>.
//
// You are free to use this software under the terms of the GNU General
// Public License, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// Alternatively, you can license this software under a commercial
// license, as set out in <http://cesanta.com/>.
//
// $Date: 2014-09-28 05:04:41 UTC $

// This file implements "netcat" utility with SSL and traffic hexdump.

#include "fossa.h"
#define iobuf mbuf
#define send_iobuf send_mbuf
#define recv_iobuf recv_mbuf
#define iobuf_remove mbuf_remove


#define TICB_WS2TCP 1
#if TICB_WS2TCP
#include <limits.h>
static const char *s_http_port = "9001";//"ssl://9002:" S2_PEM ":" CA2_PEM
#include "ns_websockify.c"
#endif

static sig_atomic_t s_received_signal = 0;

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);
  s_received_signal = sig_num;
}

static void show_usage_and_exit(const char *prog_name) {
  fprintf(stderr, "%s\n", "Copyright (c) 2014 CESANTA SOFTWARE");
  fprintf(stderr, "%s\n", "Usage:");
  fprintf(stderr, "  %s [-d debug_file] [-l] [tcp|ssl]://[ip:]port[:cert][:ca_cert]\n",
          prog_name);
  fprintf(stderr, "%s\n", "Examples:");
  fprintf(stderr, "  %s -d hexdump.txt ssl://google.com:443\n", prog_name);
  fprintf(stderr, "  %s -l ssl://443:ssl_cert.pem\n", prog_name);
  fprintf(stderr, "  %s -l tcp://8080\n", prog_name);
  exit(EXIT_FAILURE);
}


#define print_function() //printf("%s\n",__FUNCTION__)
#define print_err	printf
#define print_warn	no_printf
#define print_dbg	printf
int no_printf(const char* fmt,...)
{(void)fmt;
	return 0;
}

#include"websock_code.h"


int decodeWebSockMsg(char* msg, const unsigned char webSockMsg[], int netLen, int *p_bFinal,int* p_nCtrl)
{
	unsigned char mask_flag = 0;
	int head_len = 0;
	int payload_len = 0;
	unsigned char *pWebsock_payload = (unsigned char *)NULL;
	int ret=0;

	print_function();

	if(p_nCtrl)*p_nCtrl=(webSockMsg[0]&0xf);
	if(netLen<2){ret=-(2-netLen);goto decode_error;}//payload0with_mask4?,header2
	if((webSockMsg[0] == 0x81 || webSockMsg[0] == 0x88 || webSockMsg[0] == 0x8a || webSockMsg[0] == 0x89)) // is websock data?
	{
		/* Payload length:  7 bits, 16 bits, or 64 bits */
		payload_len = webSockMsg[1] & 0x7f;
		mask_flag = (webSockMsg[1] & 0x80) ? 1 : 0;
		head_len = 2;

#if USE_WEBSOCK_LONG_DATA
		if(payload_len == 126)
		{
			if(netLen<(mask_flag? 8:4)){ret=-((mask_flag? 8:4)-netLen);goto decode_error;}//payload0with_mask4,header2+2
			payload_len = (webSockMsg[2] << 8) | webSockMsg[3];
			head_len += 2;
		}
		else if(payload_len == 127)
		{
#if 0		
			print_err("Not support over 65535 length websock data\n");
			goto decode_error;
#else			
			if(netLen<(mask_flag? 14:10)){ret=-((mask_flag? 14:10)-netLen);goto decode_error;}//payload0with_mask4,header2+8
			payload_len = ((unsigned int)webSockMsg[6] << 24) | ((unsigned int)webSockMsg[7] << 16) | ((unsigned int)webSockMsg[8] << 8) | (unsigned int)webSockMsg[9];//low 32bit only!!
			head_len += 8;
#endif
		}

		if(mask_flag)
		{
			head_len += WEBSOCK_MSKLEN;
			pWebsock_payload += WEBSOCK_MSKLEN;
		}

		pWebsock_payload = (unsigned char *)(webSockMsg + head_len);

		//if(netLen<payload_len+head_len){ret=-(payload_len+head_len-netLen);goto decode_error;}//payload=payload_len_with_mask4,header2+8
		if((head_len + payload_len) != netLen)
		{
			// length error
			print_warn("data length error. expect %d but %d", (head_len + payload_len), netLen);
			ret=netLen-(head_len + payload_len);
			goto decode_error;
		}

		if(mask_flag == 0) // plain data
		{
			memcpy(msg, pWebsock_payload, payload_len);
		}
		else // masked data
		{int i;
			const unsigned char *encKey = pWebsock_payload - WEBSOCK_MSKLEN;

			for(i=0;i<payload_len;i++)
			{
				unsigned char xorValue = encKey[i % WEBSOCK_MSKLEN];

				msg[i] = webSockMsg[head_len + i] ^ xorValue;
			}
		}
#else
		if(payload_len > WEBSOCK_MAXLEN)
		{
			print_err("payload length is 0x%02x.\n", payload_len);
			return 0;
		}

		if((webSockMsg[1] & 0x80) == 0) // is plain data
		{
			if(payload_len + 2 == netLen) // length OK?
			{
				memcpy(msg, pWebsock_payload, payload_len);
			}
			else // length error
			{
				print_err("data length error. expect %d but %d", (webSockMsg[1] + 2), netLen);
				goto decode_error;
			}
		}
		else if((webSockMsg[1] & 0x80) != 0) // is masked data
		{
			if(payload_len + 2 + WEBSOCK_MSKLEN == netLen) // is length OK?
			{int i;
				const unsigned char *encKey = webSockMsg + 2;

				for(i=0;i<payload_len;i++)
				{
					unsigned char xorValue = encKey[i % WEBSOCK_MSKLEN];

					msg[i] = webSockMsg[2 + WEBSOCK_MSKLEN + i] ^ xorValue;
				}
			}
			else // length error
			{
				print_err("data length error. expect %d but %d", ((webSockMsg[1] & 0x7f) + 2 + WEBSOCK_MSKLEN), netLen);
				goto decode_error;
			}
		}
#endif
		if(p_bFinal) *p_bFinal = (webSockMsg[0] == 0x88);

		return payload_len;
	}
	else
	{
		print_dbg("websock data error: first byte is 0x%02x.\n", webSockMsg[0]);
		ret= -9999;
	}

decode_error:

	memcpy(msg, webSockMsg, netLen);

	return ret;

}
int isWSpacket(unsigned char websockMsg[65536],int netLen){
	char str[65536];
	int bFinal,nCtrl;int nLenTx;
	nLenTx=decodeWebSockMsg(str,websockMsg,netLen,&bFinal,&nCtrl);
	return nLenTx>=0;
}
int g_bufferedSend=1;
int b_useRx=1;
int b_useTx=1;
int b_wsMode=0;
int b_srvMode=0;
volatile int g_afterRecv=0;
int g_closeDelay=0,g_recvTimeout;
struct ns_connection* g_nc=NULL;
extern int ns_setPeer(struct ns_connection *nc, const char *address);
const char* ns_ev2str(int ev)
{
	switch(ev){
		case NS_CONNECT:return "NS_CONNECT";
		case NS_POLL:return "NS_POLL";
		case NS_CLOSE:return "NS_CLOSE";
		case NS_RECV:return "NS_RECV";
		case NS_SEND:return "NS_SEND";
		case NS_ACCEPT:return "NS_ACCEPT";
	}
	return "NS_UNK";
}

static volatile int BufPos=0;
static char Buf[65536];
static void on_stdin_read(struct ns_connection *nc, int ev, void *p) {
  int ch = * (int *) p;

  (void) ev;

// fprintf(stderr,"?issued(%d) %d flush\n",BufPos,ch);fflush(stderr);	
  if (ch < 0) {
	if(g_bufferedSend&&BufPos>0){//Buf Flush	//for no-newLine Ending request
		int SendLen=BufPos;
// fprintf(stderr,"-issued(%d) flush\n",SendLen);fflush(stderr);	
	    BufPos=0;ns_send(nc, Buf, SendLen);
	}
#if 0 	//for no-newLine Ending request	
    // EOF is received from stdin. Schedule the connection to close
    nc->flags |= NSF_SEND_AND_CLOSE;
    if (nc->send_iobuf.len <= 0) {
      nc->flags |= NSF_CLOSE_IMMEDIATELY;
    }
#endif	
  } else {
    // A character is received from stdin. Send it to the connection.
	if(g_bufferedSend){
		Buf[BufPos++]=(char)ch;
		if((b_wsMode&&isWSpacket((unsigned char*)Buf,BufPos))||(!b_wsMode&&(char)ch=='\n')){//Buf Flush
			int SendLen=BufPos;
//fprintf(stderr,">issued(%d) flush:%s\n",SendLen,Buf);fflush(stderr);
			BufPos=0;ns_send(nc, Buf, SendLen);
		}
	}
	else{
	    unsigned char c = (unsigned char) ch;
	    ns_send(nc, &c, 1);
	}
  }
}

static void *stdio_thread_func(void *param) {
  struct ns_mgr *mgr = (struct ns_mgr *) param;
  int ch;

  // Read stdin until EOF character by character, sending them to the mgr
  while ((ch = getchar()) != EOF) {
    ns_broadcast(mgr, on_stdin_read, &ch, sizeof(ch));
  }
#if 1 
  if(g_bufferedSend&&BufPos>0){//Buf Flush
  	ch=-1;
// fprintf(stderr,"+issue(%d) %d flush\n",BufPos,ch);fflush(stderr);	//for no-newLine Ending request
  	ns_broadcast(mgr, on_stdin_read, &ch, sizeof(ch));
	sleep(0);
  }
#endif  

 while(b_useRx&&!g_afterRecv)sleep(0);
 if(g_closeDelay>0)sleep(g_closeDelay);

  s_received_signal = 1;

  return NULL;
}

static void ev_handler(struct ns_connection *nc, int ev, void *p) {
  (void) p;

// if(ev!=NS_POLL)fprintf(stderr,"ev_handler(%p): message %s from %s:%d\n",nc,ns_ev2str(ev),inet_ntoa(nc->sa.sin.sin_addr),ntohs(nc->sa.sin.sin_port));

  switch (ev) {
    case NS_ACCEPT:
    case NS_CONNECT:
	// ns_tcpSetKeepAlive(nc, 1, 10, 2, 5);// 1 (KEEPALIVE ON),최초에 세션 체크 시작하는 시간 (단위 : sec),최초에 세션 체크 패킷 보낸 후, 응답이 없을 경우 다시 보내는 횟수 (단위 : 양수 단수의 갯수),TCP_KEEPIDLE 시간 동안 기다린 후, 패킷을 보냈을 때 응답이 없을 경우 다음 체크 패킷을 보내는 주기 (단위 : sec)
      ns_start_thread(stdio_thread_func, nc->mgr);
      break;

    case NS_CLOSE:
      s_received_signal = 1;
      break;

    case NS_RECV:
#if 0		
	if(nc->recv_iobuf.len>=36){
		fwrite("<sot>",1,5,stderr);fwrite(nc->recv_iobuf.buf, 1, 19, stderr);fwrite("..",1,2,stderr);fwrite(nc->recv_iobuf.buf+nc->recv_iobuf.len-17, 1, 17, stderr);fwrite("<eot>\r\n",1,7,stderr);fflush(stderr);		
	}else{fwrite(nc->recv_iobuf.buf, 1, nc->recv_iobuf.len, stderr);fflush(stderr);}	
#endif	
      fwrite(nc->recv_iobuf.buf, 1, nc->recv_iobuf.len, stdout);
      iobuf_remove(&nc->recv_iobuf, nc->recv_iobuf.len);
	g_afterRecv=1;
	if(g_nc){
		g_nc->sa=nc->sa;
	}
      break;

    default:
      break;
  }
}

// ./nc -t udp://127.0.0.1:1111 udp://1112 <<==>> ./nc -t udp://127.0.0.1:1112 udp://1111
// ./nc -T udp://1112                                 <<==>   ./nc -t udp://127.0.0.1:1112 udp://1111
// ./nc -t udp://127.0.0.1:1111 udp://1112 <==>>   ./nc -T udp://1111
// ./nc -l udp://1112                                  <<==>   ./nc  udp://127.0.0.1:1112
// ./nc udp://127.0.0.1:1111                       <==>>   ./nc  -l udp://1111
const char *g_destAddr=NULL;
int main(int argc, char *argv[]) {
  struct ns_mgr mgr;
  int i, is_listening = 0;
  const char *address = NULL;

  ns_mgr_init(&mgr, NULL);

  // Parse command line options
  
  // default is is_listening = 0; b_useRx = 1;b_useTx = 1;
  for (i = 1; i < argc && argv[i][0] == '-'; i++) {
    if (strcmp(argv[i], "-l") == 0) {
      is_listening = 1; b_useRx = 1;b_useTx = 0;
    } else if (strcmp(argv[i], "-A") == 0 && i+1 <argc) {// with LocalPort, TX&RX active
      is_listening = 1;b_useRx = 1;b_useTx = 1;
	g_destAddr=argv[++i];
    } else if (strcmp(argv[i], "-t") == 0 && i+1 <argc) {// with LocalPort, TX only
      is_listening = 1;b_useRx = 0;b_useTx = 1;
	  g_destAddr=argv[++i];
    } else if (strcmp(argv[i], "-L") == 0) {// with localPort, RX&TX passive
      is_listening = 1;b_useRx = 1;b_useTx = 1;
    } else if (strcmp(argv[i], "-r") == 0) {// with LocalPort, RX only
      is_listening = 1;b_useRx = 1;b_useTx = 0;
    } else if (strcmp(argv[i], "-T") == 0) {// without LocalPort, TX only
      is_listening = 0;b_useRx = 0;b_useTx = 1;
    } else if (strcmp(argv[i], "-c") == 0) {// without LocalPort, TX&RX active
      is_listening = 0;b_useRx = 0;b_useTx = 1;
    } else if (strcmp(argv[i], "-s") == 0) {
      b_srvMode = 1;
    } else if (strcmp(argv[i], "-W") == 0) {
      b_wsMode = 1;
    } else if (strcmp(argv[i], "-C") == 0) {
      g_bufferedSend = 0;
    } else if (strcmp(argv[i], "-w") == 0 && i+1 <argc) {
      g_recvTimeout = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-q") == 0 && i+1 <argc) {
      g_closeDelay = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
      mgr.hexdump_file = argv[++i];
    } else {
      show_usage_and_exit(argv[0]);
    }
  }

  if(b_srvMode){ 
	  if (i + 1 == argc) {
	    address = argv[i];
	  } else {
	    address = s_http_port;
	  }
  
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

#if 1
	ws_create_server(address,&s_received_signal);// s_received_signal is shared!!
	while(s_received_signal==0)getchar();
	sleep(1);//wait for thread quit!!
#else
	{
	struct ns_connection *nc;
	nc = ns_bind(&mgr, address, ws_handler);
	s_http_server_opts.document_root = ".";
	ns_set_protocol_http_websocket(nc);
	
	printf("NC> ws2soc: Started on port %s\n", address);
	while (s_received_signal == 0) {
		ns_mgr_poll(&mgr, 200);
	}
  	printf("NC> ws2soc: ending...\n");
	ns_mgr_free(&mgr);
	}
  #endif
	return EXIT_SUCCESS;
  }
  if (i + 1 == argc) {
    address = argv[i];
  } else {
    show_usage_and_exit(argv[0]);
  }

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGPIPE, SIG_IGN);

  if (is_listening) {
	struct ns_connection* nc;

	if(address&&memcmp(address,"unix:/",6)==0)unlink(address+5);
    if ((nc=ns_bind(&mgr, address, ev_handler)) == NULL) {
      fprintf(stderr, "ns_bind(%s) failed\n", address);
      exit(EXIT_FAILURE);
    }
	if(g_destAddr)ns_setPeer(nc,g_destAddr);
	if(b_useTx)ns_start_thread(stdio_thread_func, nc->mgr);
	g_nc=nc;
  } else if (ns_connect(&mgr, address, ev_handler) == NULL) {// only for UDP/TCP
    fprintf(stderr, "ns_connect(%s) failed\n", address);
    exit(EXIT_FAILURE);
  }

  while (s_received_signal == 0) {
    ns_mgr_poll(&mgr, 1000);
	if(!b_useTx&&g_afterRecv)break;
  }
  ns_mgr_free(&mgr);

	if(is_listening)if(address&&memcmp(address,"unix:/",6)==0)unlink(address+5);
  return EXIT_SUCCESS;
}
