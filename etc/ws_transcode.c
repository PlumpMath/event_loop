#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define print_function() //printf("%s\n",__FUNCTION__)
#define print_err	printf
#define print_warn	no_printf
#define print_dbg	printf
int no_printf(const char* fmt,...)
{
	return 0;
}

#include"websock_code.h"


int encodeWebSockMsg(unsigned char webSockMsg[],const char *msg,int msgLen,int bFinal,const unsigned char encKey[])
{
	//printf("+ %s(): ================\n", __func__);
	int head_len = 0;

#if USE_WEBSOCK_LONG_DATA
	if(msgLen > 65535)
	{
		head_len = 10;
		webSockMsg[1] = 127;
#ifdef WIN32
		webSockMsg[2] = 0;
		webSockMsg[3] = 0;
		webSockMsg[4] = 0;
		webSockMsg[5] = 0;
#else
		webSockMsg[2] = (msgLen & 0x7f00000000000000ULL) >> 56;
		webSockMsg[3] = (msgLen & 0xff000000000000ULL) >> 48;
		webSockMsg[4] = (msgLen & 0xff0000000000ULL) >> 40;
		webSockMsg[5] = (msgLen & 0xff00000000ULL) >> 32;
#endif
		webSockMsg[6] = (msgLen & 0xff000000) >> 24;
		webSockMsg[7] = (msgLen & 0xff0000) >> 16;
		webSockMsg[8] = (msgLen & 0xff00) >> 8;
		webSockMsg[9] = (msgLen & 0xff) >> 0;
	}
	else if(msgLen > 125)
	{
		head_len = 4;
		webSockMsg[1] = 126;
		webSockMsg[2] = (msgLen & 0xff00) >> 8;
		webSockMsg[3] = msgLen & 0xff;
	}
	else
	{
		head_len = 2;
		webSockMsg[1] = msgLen & 0x7f;
	}

	//webSockMsg[0] = bFinal ? 0x88:0x81;  // text, FIN set
	webSockMsg[0] = (bFinal==0? 0x81:(0x80|(bFinal&0xf)));  // text or control, FIN set
	if(encKey)
	{int i;
		webSockMsg[1] |= 0x80;
		memcpy(webSockMsg + head_len, encKey, WEBSOCK_MSKLEN);
		head_len += WEBSOCK_MSKLEN;
		for(i = 0;i < msgLen;i++)
		{
			unsigned char xorValue = encKey[i % WEBSOCK_MSKLEN];
			webSockMsg[head_len + i] = msg[i] ^ xorValue;
		}
	}
	else
	{
		memcpy(webSockMsg + head_len, msg, msgLen);
	}

	//printf("- %s(): ================\n", __func__);
	return msgLen + head_len;
#else
	if(msgLen > WEBSOCK_MAXLEN) return 0;

	webSockMsg[0] = bFinal ? 0x88:0x81;  // text, FIN set
	if(encKey)
	{int i;
		webSockMsg[1] = msgLen & 0x7f | 0x80;
		memcpy(webSockMsg + 2, encKey, WEBSOCK_MSKLEN);
		for(i = 0;i < msgLen;i++)
		{
			unsigned char xorValue = encKey[i % WEBSOCK_MSKLEN];
			webSockMsg[2 + WEBSOCK_MSKLEN + i] = msg[i] ^ xorValue;
		}
		//printf("- %s(): ================\n", __func__);
		return msgLen + 2 + WEBSOCK_MSKLEN;
	}
	else
	{
		webSockMsg[1] = msgLen;
		memcpy(webSockMsg + 2, msg, msgLen);
		//printf("- %s(): ================\n", __func__);
		return msgLen + 2;
	}
#endif
	//printf("- %s(): ================\n", __func__);
}
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
	unsigned char websockMsg[65536];int nLenTx;
	char sendmsg[256],respmsg[256],outfile[256],infile[256];
	int bHex=0,bSilent=0;
	int netLen=0;
	int ch;
	int i;int bCloseWait=0,bSleep=0;
	FILE *outfp=NULL,*infp=NULL;

	sendmsg[0]=0;respmsg[0]=0;outfile[0]=0;
	for(i=0;i<argc;i++){
		if(argv[i][0]=='-'&&argv[i][1]=='H')bHex=1;
		else if(argv[i][0]=='-'&&argv[i][1]=='S')bSilent=1;
		else if(argc>i+1&&argv[i][0]=='-'&&argv[i][1]=='q'){bCloseWait=atoi(argv[i+1]);i++;}
		else if(argc>i+1&&argv[i][0]=='-'&&argv[i][1]=='s'){bSleep=atoi(argv[i+1]);i++;}
		else if(argc>i+1&&argv[i][0]=='-'&&argv[i][1]=='c'){strcpy(sendmsg,argv[i+1]);i++;}
		else if(argc>i+1&&argv[i][0]=='-'&&argv[i][1]=='r'){strcpy(respmsg,argv[i+1]);i++;}
		else if(argc>i+1&&argv[i][0]=='-'&&argv[i][1]=='o'){strcpy(outfile,argv[i+1]);i++;}
		else if(argc>i+1&&argv[i][0]=='-'&&argv[i][1]=='i'){strcpy(infile,argv[i+1]);i++;}
	}
	if(outfile[0])outfp=fopen(outfile,"w");
	if(infile[0])infp=fopen(infile,"w");
	if(sendmsg[0]){// recv from input FILE!!
if(!bSilent){fprintf(stderr,"ws_encode send:%s\n",sendmsg);fflush(stderr);}
		nLenTx=encodeWebSockMsg(websockMsg,sendmsg,strlen(sendmsg),1,WEBSOCK_TESTKEY);sendmsg[0]=0;
		if(!bHex){
			for(i=0;i<nLenTx;i++)putchar(websockMsg[i]);
			fflush(stdout);
		}
		else{
			fprintf(stderr,"msg(%d):",nLenTx);
			for(i=0;i<nLenTx;i++)fprintf(stderr,"%02x",websockMsg[i]);
			fprintf(stderr,"\n");fflush(stderr);
		}
	}
	while((ch=getc(infp))!=EOF){
		char str[65536];
		int nLenTx;int i;
		int bFinal,nCtrl;

		websockMsg[netLen]=(unsigned char)ch;
		netLen++;
		// return payload_len
		nLenTx=decodeWebSockMsg(str,websockMsg,netLen,&bFinal,&nCtrl);
		if(nLenTx<0)continue;
		netLen=0;
		str[nLenTx]=0;
if(!bSilent){fprintf(stderr,"	ws_decode(%d) recv(%d):%s\n",netLen,nLenTx,str);	fflush(stderr);}
		if(!bHex){
			for(i=0;i<nLenTx;i++)fputc(str[i],stdout);//send to output FILE!!
			fflush(stdout);
		}
		else{
			fprintf(stderr,"msg(%d):",nLenTx);
			for(i=0;i<nLenTx;i++)fprintf(stderr,"%02x",str[i]);
			fprintf(stderr,"\n");fflush(stderr);
			
			if(!strcmp(str,"EXITEXIT"))break;// for output scripting
		}
		nLenTx=0;

		if(bSleep){sleep(bSleep);bSleep=0;}

		if(respmsg[0]){// recv from input FILE!!
	if(!bSilent){fprintf(stderr,"ws_encode resp:%s\n",respmsg);fflush(stderr);}
			nLenTx=encodeWebSockMsg(websockMsg,respmsg,strlen(respmsg),1,WEBSOCK_TESTKEY);respmsg[0]=0;
			if(!bHex){
				for(i=0;i<nLenTx;i++)fputc(websockMsg[i],outfp);
				fflush(outfp);
			}
			else{
				fprintf(stderr,"msg(%d):",nLenTx);
				for(i=0;i<nLenTx;i++)fprintf(stderr,"%02x",websockMsg[i]);
				fprintf(stderr,"\n");fflush(stderr);
			}
		}
	}
	if(bCloseWait){sleep(bCloseWait);bCloseWait=0;}
	if(outfp)fclose(outfp);
	if(infp)fclose(infp);
	return 0;
}
