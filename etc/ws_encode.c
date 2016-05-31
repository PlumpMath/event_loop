#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<time.h>

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

int main(int argc,char* argv[])
{
	char str[65536];
	int bHex=0,bSilent=0,bFastExit=0,edelay=0,i;

	for(i=1;i<argc;i++){
		if(argv[i][0]=='-'&&argv[i][1]=='H')bHex=1;
		if(argv[i][0]=='-'&&argv[i][1]=='S')bSilent=1;
		if(argv[i][0]=='-'&&argv[i][1]=='f'){
			bFastExit=1;
			if(i+1<argc&&argv[i+1][0]!='-'){edelay=atoi(argv[i+1]);i++;}
		}
	}
	while(fgets(str,sizeof(str),stdin)){
		unsigned char websockMsg[65536];
		int nLenTx;int i;
		time_t t=time(NULL);
		char* ct=ctime(&t);

		if(str[strlen(str)-1]=='\n')str[strlen(str)-1]='\0';// emulate gets!!
		
		if(memcmp(str,"sleep ",6)==0){// for input scripting
if(!bSilent){fprintf(stderr,"ws_encode process:SLEEP %d@%s",atoi(str+6),ct);fflush(stderr);}
			sleep(atoi(str+6));
			continue;
		}
if(!bSilent){fprintf(stderr,"ws_encode send:'%s'@%s",str,ct);fflush(stderr);}
		nLenTx=encodeWebSockMsg(websockMsg,str,strlen(str),1,WEBSOCK_TESTKEY);
		if(!bHex)
			for(i=0;i<nLenTx;i++)putchar(websockMsg[i]);
		else{
			printf("msg(%d):",nLenTx);
			for(i=0;i<nLenTx;i++)printf("%02x",websockMsg[i]);
			printf("\n");
		}
		fflush(stdout);
		if(bFastExit){
			if(edelay)usleep(edelay);
			exit(0);
		}
	}
	return 0;
}
