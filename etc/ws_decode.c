#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<time.h>
#define print_function() //printf("%s\n",__FUNCTION__)
#define print_err	printf
#define print_warn	no_printf
#define print_dbg	printf
int no_printf(const char* fmt,...)
{
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

int main(int argc,char* argv[])
{
	unsigned char websockMsg[65536];
	int bHex=0,bSilent=0,bFastExit=0,edelay=0,i;
	int netLen=0;
	int ch;

	for(i=1;i<argc;i++){
		if(argv[i][0]=='-'&&argv[i][1]=='H')bHex=1;
		if(argv[i][0]=='-'&&argv[i][1]=='S')bSilent=1;
		if(argv[i][0]=='-'&&argv[i][1]=='f'){
			bFastExit=1;
			if(i+1<argc&&argv[i+1][0]!='-'){edelay=atoi(argv[i+1]);i++;}
		}
	}
	while((ch=getchar())!=EOF){
		char str[65536];
		int nLenTx;int i;
		int bFinal,nCtrl;
		time_t t;
		char* ct;

		websockMsg[netLen]=(unsigned char)ch;
		netLen++;
		// return payload_len
		nLenTx=decodeWebSockMsg(str,websockMsg,netLen,&bFinal,&nCtrl);
		if(nLenTx<0)continue;
		t=time(NULL);ct=ctime(&t);
		netLen=0;
		str[nLenTx]=0;
if(!bSilent){fprintf(stderr,"	ws_decode(%d) recv(%d):'%s'@%s",netLen,nLenTx,str,ct);	fflush(stderr);}
		if(!bHex)
			for(i=0;i<nLenTx;i++)putchar(str[i]);
		else{
			printf("msg(%d):",nLenTx);
			for(i=0;i<nLenTx;i++)printf("%02x",str[i]);
			printf("\n");
			if(!strcmp(str,"EXITEXIT"))break;// for output scripting
		}
		fflush(stdout);
		if(bFastExit){
			if(edelay)usleep(edelay);
			exit(0);
		}
		nLenTx=0;
	}
	return 0;
}
