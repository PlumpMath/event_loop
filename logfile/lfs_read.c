#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE /* See feature_test_macros(7) */
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include <sys/types.h>
#include <unistd.h> 
#include <time.h> 
#include <errno.h> 

#include <sys/stat.h>
#include <fcntl.h>
#ifdef __CYGWIN__
#define O_LARGEFILE 0
#define llseek lseek
#else
	// off64_t lseek64(int fd, off64_t offset, int whence); 
loff_t llseek(int fd, loff_t offset, int whence);
#endif

#define min(a,b) ((a)<(b)? (a):(b))

// gcc -D_FILE_OFFSET_BITS=64 lfs_test.c -o lfs_test
void writeSeq(const char* filename,unsigned long t)
{
	FILE* fp;

	fp=fopen("seqfile.txt","w+");
	if(fp){
	fwrite(&t,sizeof(t),1,fp);
	fclose(fp);
	}
}
unsigned long readSeq(const char* filename)
{
	FILE* fp;
	unsigned long t=time(NULL);

	fp=fopen("seqfile.txt","r");
	if(fp){
		fread(&t,sizeof(t),1,fp);
		fclose(fp);
	}
	return t;
}
// #define BULK_BUF_SIZE 0x500000
// #define BULK_BUF_SIZE 0x50000
#define BULK_BUF_SIZE 0x100000
#define DATA_SIZE (1024)
#if !USE_LFS_ACCESS_AS_LIBRARY
int g_debugLevel=-1;
#else
extern volatile int g_debugLevel;
// #define g_debugLevel 5
#endif
unsigned long getNearSeq(int fd,off_t off,off_t* p_moff)
{
	unsigned long mt;
	char data[DATA_SIZE+1];int i,dataLen;

	if(off==0){;}//???
	if(g_debugLevel>=3)printf("checkinf off=%Lx..\n",(unsigned long long)off);
	lseek(fd,off-1,SEEK_SET);//for exact matchCase
	if(errno){if(errno!=0){perror("1) after lseek()");printf(">>>>>>>>>>>>>>> errno = %d after lseek(%ld)\n", errno,(unsigned long)off-1);}errno=0;}
	// get cur seek HERE
	memset(data,0,sizeof(data));//for nullTermination
	dataLen=read(fd,data,sizeof(data)-1);
	// printf("cur: read %d bytes.%.*s<EOT>\n",dataLen,dataLen,data);
	for(i=0;i<dataLen;i++)
		if(data[i]=='\n')break;

	//if(i==0)printf("exact match@%Lx..\n",(unsigned long long)off);
	if(i>=dataLen){
		printf("FATAL newLine not found!!\n");return 0;
	}
	// modify off
	off+=i;
	sscanf(data+i+1,"%lx,",&mt);
	if(p_moff)*p_moff=off;
	return mt;
}
off_t _getSeqPosition(//ft<t<lt
	int fd,unsigned long t,
	unsigned long ft,unsigned long lt,off_t foff,off_t loff,
	unsigned long *p_mt,int* pDepth,int* pCount)
{
	unsigned long mt;
	unsigned long nt;off_t noff;
	off_t moff;

	if(t<ft)return -1;
	if(t>lt)return 0xFFFFFFFEULL;
	if(loff-foff<DATA_SIZE-1||lt-ft<=3){//for smallSize or smallTime
		// read & detect is more efficient
		while(1){
			if(pCount)(*pCount)++;
			nt=getNearSeq(fd,foff+1,&noff);//adjust t
			if(nt==lt||nt>=t)break;
			ft=nt;foff=noff;
		}
		goto RetNT;
	}

	if(g_debugLevel>=2)printf("finding for t=%lx(%lx~%lx:off %Lx~%Lx)\n",t,ft,lt,(unsigned long long)foff,(unsigned long long)loff);
	if(foff==loff)goto RetFT;
	if(t<=ft)goto RetFT;//nearest match is FT
	if(t>=lt)goto RetLT;//nearest match is LT

	// t must be between ft and lt
	// find Middle Pos
	if(pCount)(*pCount)++;
	if(lt-ft<=2)
		mt=getNearSeq(fd,foff+1,&moff);
	else
		mt=getNearSeq(fd,(foff+loff)/2,&moff);
	if(g_debugLevel>=2)printf("mt:%lx,moff=%Lx detect near%Lx\n",mt,(unsigned long long)moff,(unsigned long long)(foff+loff)/2);

	if(mt==t)goto RetMT; // Exact match
	else if(t<mt){ // Lower Search
		if(g_debugLevel>=2)printf("Lower Search\n");
		if(pDepth)(*pDepth)++;
		return _getSeqPosition(fd,t,ft,mt,foff,moff,p_mt,pDepth,pCount);
	}
	else{ // Higher Search
		if(g_debugLevel>=2)printf("Higher Search\n");
		if(pDepth)(*pDepth)++;
		return _getSeqPosition(fd,t,mt,lt,moff,loff,p_mt,pDepth,pCount);
	}
RetLT:	if(p_mt)*p_mt=lt;
	return loff;
RetMT:	if(p_mt)*p_mt=t;
	return moff;
RetFT:	if(p_mt)*p_mt=ft;
	return foff;
RetNT:	if(p_mt)*p_mt=nt;
	return noff;
}
off_t getSeqPosition(//ft<=t<=lt
	int fd,unsigned long t,
	unsigned long ft,unsigned long lt,off_t foff,off_t loff,
	unsigned long *p_mt,int* pDepth,int* pCount)
{
	if(t<ft)return -1;
	if(t>lt)return 0xFFFFFFFEULL;
	if(t==ft)goto RetFT;
	if(t==lt)goto RetLT;
	return _getSeqPosition(fd,t,ft,lt,foff,loff,p_mt,pDepth,pCount);
RetLT:	if(p_mt)*p_mt=lt;
	return loff;
RetFT:	if(p_mt)*p_mt=ft;
	return foff;
}
typedef struct _lfs_ctx{
	char filename[256];//lfs input
	char buf[BULK_BUF_SIZE+1];
	off_t foff,loff;
	unsigned long ft,lt;
	int fd;
	off_t FileSize;

	unsigned long st,et;//filter1 input
	off_t soff,eoff;//filter1 output
	char filter2[256];//filter2 input
}lfs_ctx;
lfs_ctx* init_lfs(const char* filename)
{
	int fd;
	off_t foff,loff;// off,
	unsigned long ft,lt;
	char data[DATA_SIZE+1];int i,dataLen;
	char sampleData[DATA_SIZE+1];
	lfs_ctx* plfs;

	fd=open(filename,O_RDONLY|O_LARGEFILE/*O_NONBLOCK*//*O_SYNC*//*O_TRUNC*/,S_IRWXU);
	if(fd<0)return NULL;
	plfs=(lfs_ctx*)malloc(sizeof(lfs_ctx));
	plfs->fd=fd;
	// off=-(sizeof(data));
	// lseek(fd,-DATA_SIZE,SEEK_END);
	loff=lseek(fd,0,SEEK_END);loff=lseek(fd,0,SEEK_CUR);
	if(errno){if(errno!=0){perror("2) after lseek()");printf(">>>>>>>>>>>>>>> errno = %d after lseek(%ld)\n", errno,0L);}errno=0;}
	if(g_debugLevel>=2)printf("init_lfs: file size is %lx(%ld)\n",(unsigned long)loff,(unsigned long)loff);
	if(loff>=DATA_SIZE){
		llseek(fd,-DATA_SIZE,SEEK_END);
		if(errno){if(errno!=0){perror("3) after lseek()");printf(">>>>>>>>>>>>>>> errno = %d after lseek(%ld)\n", errno,(long)-DATA_SIZE);}errno=0;}
	}
	else lseek(fd,0,SEEK_SET);
	// loff=lseek(fd,0,SEEK_CUR);//get position
	plfs->FileSize=loff;

	// get last seek HERE
	memset(data,0,sizeof(data));//for nullTermination
	dataLen=read(fd,data,sizeof(data)-1);
	loff=lseek(fd,0,SEEK_CUR);//get position
	// printf("last: read %d(%d) bytes.%.*s<EOT>\n",dataLen,sizeof(data)-1,dataLen,data);
	for(i=1;i<dataLen;i++)
		if(data[dataLen-1-i]=='\n')break;
	if(g_debugLevel>=2)printf("NEWLINE @ %d offset\n",dataLen-1-i);
	if(g_debugLevel>=2)printf("last entry is:%.*s<EOT>\n",i,&data[dataLen-1-i+1]);
	memcpy(sampleData,&data[dataLen-1-i+1],i);sampleData[i]=0;
	sscanf(sampleData,"%lx,",&lt);
	if(g_debugLevel>=2)printf("lastSeq:%lx\n",lt);
	if(g_debugLevel>=2)printf("last: read %x(%x)%lx ",dataLen,i,(unsigned long)loff);
	loff-=i;//adjust lastSeq position
	plfs->lt=lt;
	plfs->loff=loff;
	if(g_debugLevel>=2)printf("lastOff:%Lx\n",(unsigned long long)loff);

	lseek(fd,0,SEEK_SET);
	foff=0;//this is firstSeq position
	plfs->foff=foff;
	memset(data,0,sizeof(data));//for nullTermination
	dataLen=read(fd,data,sizeof(data)-1);
//	printf("first: read %d bytes\n",dataLen);
	for(i=1;i<dataLen;i++)
		if(data[i]=='\n')break;
	if(g_debugLevel>=2)printf("first entry is:%.*s<EOT>\n",i+1,data);
	memcpy(sampleData,data,i+1);sampleData[i+1]=0;
	sscanf(sampleData,"%lx,",&ft);
	plfs->ft=ft;
	if(g_debugLevel>=2)printf("firstSeq:%lx\n",ft);
	if(g_debugLevel>=2)printf("firstOff:%Lx\n",(unsigned long long)foff);
	if(errno){if(errno!=0){perror("**) after open()");printf(">>>>>>>>>>>>>>> errno = %d after open(%ld)\n", errno,(long)0);}errno=0;}
	return plfs;
}
void destroy_lfs(lfs_ctx* plfs)
{
	close(plfs->fd);
	if(errno){if(errno!=0){perror("3) after close()");printf(">>>>>>>>>>>>>>> errno = %d after close(%ld)\n", errno,(long)0);}errno=0;}
	free(plfs);
}
int filter1(lfs_ctx* plfs,unsigned long st,unsigned long et)
{
	int fd=plfs->fd;
	off_t foff=plfs->foff,loff=plfs->loff,eoff,soff;
	unsigned long ft=plfs->ft,lt=plfs->lt;
	unsigned long mst=0,met=0;
	int depth=0,count=0;

	plfs->st=st;plfs->et=et;
	if(g_debugLevel>=2)printf("finding at st=%lx,et=%lx(ft=%lx,lt=%lx)\n",st,et,ft,lt);fflush(stdout);
	if(st<ft){soff=foff;mst=ft;}
	else
		soff=getSeqPosition(fd,st,ft,lt,foff,loff,&mst,&depth,&count);
	plfs->soff=soff;
	if(g_debugLevel>=2)printf("found at soff=%Lx,mst=%lx,depth=%d,count=%d\n",(unsigned long long)soff,mst,depth,count);fflush(stdout);
	depth=0;count=0;
#if 1 // for lfs_touch	
	if(et>lt){eoff=plfs->FileSize;met=et;}
	else 
#endif	
	if(et>=lt){eoff=loff;met=lt;}
	else
		eoff=getSeqPosition(fd,et,ft,lt,foff,loff,&met,&depth,&count);
	plfs->eoff=eoff;
	if(g_debugLevel>=2)printf("found at eoff=%Lx,mst=%lx,met=%lx,depth=%d,count=%d(lt=%lx)\n",(unsigned long long)eoff,mst,met,depth,count,lt);fflush(stdout);
	if(met==lt){
		plfs->eoff=plfs->FileSize;
		if(g_debugLevel>=2)printf("found at eoff=%Lx(FileSize),mst=%lx,met=%lx,depth=%d,count=%d(lt=%lx)\n",(unsigned long long)eoff,mst,met,depth,count,lt);fflush(stdout);
	}
	else if(met==et){
		depth=0;count=0;
		eoff=getSeqPosition(fd,et+1,ft,lt,foff,loff,&met,&depth,&count);
		plfs->eoff=eoff;
		if(g_debugLevel>=1)printf("next found at eoff=%Lx,met=%lx,depth=%d,count=%d\n",(unsigned long long)eoff,met,depth,count);fflush(stdout);
	}
	if(errno){if(errno!=0){perror("**) after filter1()");printf(">>>>>>>>>>>>>>> errno = %d after filter1(%ld)\n", errno,(long)0);}errno=0;}
	return 0;
}
#if 0
int filter2_0(lfs_ctx* plfs,const char* filter)
{
	int fd=plfs->fd;
	off_t soff=plfs->soff,eoff=plfs->eoff;
	char buf[BULK_BUF_SIZE];
	int len;

	lseek(fd,soff,SEEK_SET);
	len=read(fd,buf,min(BULK_BUF_SIZE-1,eoff-soff));
	buf[len]=0;
	if(len<10024)
		printf("%s",buf);
	else
		printf("%.*s..%.*s",512,buf,512,&buf[len-512]);
}
#endif
int outfilter(const char* msg,const char* filter,char* outbuf,int* p_outPos,int t_offset,const char* sep)
{
	unsigned long t;
	
	if(!filter||strstr(msg+9,filter))
	{
		sscanf(msg,"%lx,",&t);
		if(outbuf&&p_outPos){
			*p_outPos+=sprintf(outbuf+*p_outPos,"%lu%s%s",t+t_offset,sep,msg+9);//$lu new-format
			if(g_debugLevel>=2)printf("*p_outPos=%d\n",*p_outPos);
		}
		else{
			if((t&0xf)==0)
				if(g_debugLevel>=0)printf("%lu%s%s",t+t_offset,sep,msg+9);//$lu new-format
		}
	}
	if(errno){if(errno!=0){perror("**) after outfilter()");printf(">>>>>>>>>>>>>>> errno = %d after outfilter(%ld)\n", errno,(long)0);}errno=0;}
	return 0;
}
int filter2(lfs_ctx* plfs,const char* filter,char* outbuf,int* p_outPos,int t_offset,const char* sep)
{
	int fd=plfs->fd;
	off_t soff=plfs->soff,eoff=plfs->eoff,off;
	int unbufBytes=0;
	// char buf[BULK_BUF_SIZE+1];
	char* buf=plfs->buf;

	lseek(fd,soff,SEEK_SET);
	if(errno){if(errno!=0){perror("4) after lseek()");printf(">>>>>>>>>>>>>>> errno = %d after lseek(%ld)\n", errno,(unsigned long)soff);}errno=0;}
	off=soff;
	while(off<eoff){
		int len;
		int pos=0;
		int toRead=min(BULK_BUF_SIZE-unbufBytes-1,eoff-off);

		if(off%512)toRead-=(512-off%512);
		if(g_debugLevel>=2)printf("reading @ %Lx\n",(unsigned long long)off);
		if(g_debugLevel>=2)printf("off%ld,eoff%ld,toRead%d,unbufBytes%d\n",(unsigned long)off,(unsigned long)eoff,toRead,unbufBytes);
		if(toRead<=0)break;
		len=read(fd,buf+unbufBytes,toRead);
		if(g_debugLevel>=2)printf("read: %dBytes\n",len);
	if(errno){if(errno!=0){perror("**) after read()");printf(">>>>>>>>>>>>>>> errno = %d after read(%ld)\n", errno,(long)toRead);}errno=0;}
		if(len<=0)break;
		len+=unbufBytes;unbufBytes=0;
		buf[len]=0;
		do{
			char msg[DATA_SIZE+1];
			int j;

		// printf("filter@ %Lx\n",(unsigned long long)off+pos);
			for(j=0;buf[pos+j]!='\n'&&pos+j<len;j++);
			if(buf[pos+j]!='\n'){
				if(g_debugLevel>=2)printf("read: last is not new_line(pos=%d,len=%d,j=%d,buf='%.*s')\n",pos,len,j,len,buf);
#if 1
				memcpy(buf,&buf[pos],j);unbufBytes=j;
#else
				printf("%s(%d) not processed\n",&buf[pos],len-pos);
#endif
				break;
			}
			memcpy(msg,buf+pos,j+1);msg[j+1]=0;
			outfilter(msg,filter,outbuf,p_outPos,t_offset,sep);
			pos+=j+1;
		}while(pos<len);
#if 1
		if(g_debugLevel>=2)printf("len%d,pos%d\n",len,pos);
		off+=len;
#else
		off+=pos;
		if(pos<len)lseek(fd,pos-len,SEEK_CUR);
#endif
	}
	if(errno){if(errno!=0){perror("**) after filter2()");printf(">>>>>>>>>>>>>>> errno = %d after filter2(%ld)\n", errno,(long)0);}errno=0;}
	return 0;
}

extern int jss_encode(char *s, int s_len, const char *str, int len, int bAddQuote);
extern int jss_decode(char* dest,const char* src,int srcLen,int* p_bHaveEsc);
#define MAX_LOGLINE_SIZE 	32768 //single LogLine size!!
#define MAX_LOGLINES_SIZE 	(32*1024*1024) //filtered MAX size!!
#define JSS_ENCODE_LOGLINE 1
typedef long lfd_t;
lfd_t lfs_read_open(const char* filename)
{
	lfd_t lfd=-1L;
	
	lfd=(lfd_t)init_lfs(filename);
	// if(!lfd)lfd=-1;
	return lfd;
}
int lfs_read_in(lfd_t lfd,char* buf,int bufLen,unsigned long st,unsigned long et,const char* filter,int t_offset,const char* sep,int opt)
{
#if JSS_ENCODE_LOGLINE
	char *sampleData=NULL;int sampleLen;
	int orgBufLen=bufLen;
#endif	

	if(lfd==0)return -1; // lfd<0||
#if 1 // for lfs_touch	
	if(opt==2){st=((lfs_ctx*)lfd)->lt;et=((lfs_ctx*)lfd)->lt+1;t_offset=0;filter=NULL;}
	else if(opt==1){st=((lfs_ctx*)lfd)->ft;et=((lfs_ctx*)lfd)->ft+1;t_offset=0;filter=NULL;}
	else {;}
	if(opt)printf("lfs_read_in: st=%lx,et=%lx\n",st,et);
#endif	
	bufLen=0;
	filter1((lfs_ctx*)lfd,st-t_offset,et-t_offset);// limit range(subtract t_offset HERE:localtime -> UTC)
#if JSS_ENCODE_LOGLINE
	sampleData=(char*)malloc(MAX_LOGLINES_SIZE);
	if(!sampleData)return 0;
	sampleLen=0;
	filter2((lfs_ctx*)lfd,filter,sampleData,&sampleLen,t_offset,sep);// search!!(t_offset=32400)(add t_offset @ OUTPUT:UTC -> localtime)
	if(orgBufLen<sampleLen){free(sampleData);printf("lfs_read_in: FATAL return big Sample%d/%d\n",sampleLen,orgBufLen);return 0;}
	bufLen=jss_decode(buf,sampleData,sampleLen,NULL);if(bufLen>0&&bufLen<orgBufLen)buf[bufLen]='\0';
	if(g_debugLevel>=2)printf("lfs_read_in: sampleLen=%d,bufLen=%d\n",sampleLen,bufLen);
	free(sampleData);
#else
	filter2((lfs_ctx*)lfd,filter,buf,&bufLen,t_offset,sep);// search!!(t_offset=32400)(add t_offset @ OUTPUT:UTC -> localtime)
#endif	
	return bufLen;
}
int lfs_read_close(lfd_t lfd)
{
	int ret=0;
	
	if(lfd==0)return -1;// lfd<0||
	destroy_lfs((void*)lfd);
	return ret;
}

int lfs_write_open(const char* filename)
{
	int fd=-1;
	
	fd=open(filename,O_APPEND|O_CREAT|O_WRONLY|O_LARGEFILE/*O_NONBLOCK*//*O_SYNC*//*O_TRUNC*/,S_IRWXU);
	if(fd<0)return -3;
	lseek(fd,0,SEEK_END);
	return fd;
}
int lfs_write_out(int fd,unsigned long ct,const char* data,int dataLen)
{
	int ret;
	char sampleData[MAX_LOGLINE_SIZE+4096];int sampleLen;
	
#if JSS_ENCODE_LOGLINE
	char sampleEncData[MAX_LOGLINE_SIZE+2048];int sampleEncLen;

	if(dataLen>=MAX_LOGLINE_SIZE)return 0;
	sampleEncLen=jss_encode(sampleEncData,MAX_LOGLINE_SIZE+2048,data,dataLen,0);// include nul term!!
	sampleLen=sprintf(sampleData,"%08lx,%.*s\n",ct,sampleEncLen,sampleEncData);
#else
	sampleLen=sprintf(sampleData,"%08lx,%.*s\n",ct,dataLen,data);
#endif
	ret=write(fd,sampleData,sampleLen);
	return ret;
}
int lfs_write_close(int fd)
{
	int ret;
	
	ret=close(fd);
	return ret;
}

#if !USE_LFS_ACCESS_AS_LIBRARY
int main(int argc,char* argv[])
{
	lfs_ctx* plfs;

	if(argc>=6)g_debugLevel=atoi(argv[5]);
	if(argc<5)return -2;
	plfs=init_lfs(argv[1]);
	if(!plfs)return -1;

	filter1(plfs,strtoul(argv[2],NULL,16),strtoul(argv[3],NULL,16));
	filter2(plfs,argv[4]);
	destroy_lfs(plfs);
	return 0;
}
int test_main(int argc,char* argv[])
{
	char filename[256];
	int fd=-1;
	unsigned long t=0,ft,lt;
	off_t off,foff,loff;
	char data[DATA_SIZE+1];int i,dataLen;
	char sampleData[DATA_SIZE+1];

	printf("sizeof loff_t=%d\n",sizeof(loff_t));
	if(sizeof(off_t)<8){
		printf("size of off_t=%d\n",sizeof(off_t));
		return -1;
	}
	if(argc<2)return -2;
	strcpy(filename,argv[1]);
	fd=open(filename,O_RDONLY|O_LARGEFILE/*O_NONBLOCK*//*O_SYNC*//*O_TRUNC*/,S_IRWXU);
	if(fd<0)return -3;
	off=-(sizeof(data));
	// lseek(fd,-DATA_SIZE,SEEK_END);
	llseek(fd,-DATA_SIZE,SEEK_END);
	loff=lseek(fd,0,SEEK_CUR);//get position

	// get last seek HERE
	memset(data,0,sizeof(data));//for nullTermination
	dataLen=read(fd,data,sizeof(data)-1);
	// printf("last: read %d bytes.%.*s<EOT>\n",dataLen,dataLen,data);
	for(i=1;i<dataLen;i++)
		if(data[dataLen-1-i]=='\n')break;
	printf("NEWLINE @ %d offset\n",dataLen-1-i);
	printf("last entry is:%.*s<EOT>\n",i,&data[dataLen-1-i+1]);
	memcpy(sampleData,&data[dataLen-1-i+1],i);sampleData[i]=0;
	sscanf(sampleData,"%lx,",&lt);
	printf("lastSeq:%lx\n",lt);
	loff-=i;//adjust lastSeq position
	printf("lastOff:%Lx\n",(unsigned long long)loff);

	lseek(fd,0,SEEK_SET);
	foff=0;//this is firstSeq position
	memset(data,0,sizeof(data));//for nullTermination
	dataLen=read(fd,data,sizeof(data)-1);
//	printf("first: read %d bytes\n",dataLen);
	for(i=1;i<dataLen;i++)
		if(data[i]=='\n')break;
	printf("first entry is:%.*s<EOT>\n",i+1,data);
	memcpy(sampleData,data,i+1);sampleData[i+1]=0;
	sscanf(sampleData,"%lx,",&ft);
	printf("firstSeq:%lx\n",ft);
	printf("firstOff:%Lx\n",(unsigned long long)foff);
if(argc>=3){
	unsigned long st,mst;
	off_t soff;
	int depth=0,count=0;
	sscanf(argv[2],"%lx",&st);
	soff=getSeqPosition(fd,st,ft,lt,foff,loff,&mst,&depth,&count);
	printf("found at soff=%Lx,mst=%lx,depth=%d,count=%d\n",(unsigned long long)soff,mst,depth,count);fflush(stdout);
	if(argc>=4){
	unsigned long et,met;
	off_t eoff;

	sscanf(argv[3],"%lx",&et);
	depth=0;count=0;
	eoff=getSeqPosition(fd,et,ft,lt,foff,loff,&met,&depth,&count);
	printf("found at eoff=%Lx,met=%lx,depth=%d,count=%d\n",(unsigned long long)eoff,met,depth,count);fflush(stdout);
	if(met==et){
	eoff=getSeqPosition(fd,et+1,ft,lt,foff,loff,&met,&depth,&count);
	printf("next found at eoff=%Lx,met=%lx,depth=%d,count=%d\n",(unsigned long long)eoff,met,depth,count);fflush(stdout);
	}
	{
	#if 0
		filter2(fd,soff,eoff,argv[4]);
	#else
		char buf[BULK_BUF_SIZE];int len;

		lseek(fd,soff,SEEK_SET);
		len=read(fd,buf,min(sizeof(buf)-1,eoff-soff));
		buf[len]=0;
		if(len<10024)
			printf("%s",buf);
		else
			printf("%.*s..%.*s",512,buf,512,&buf[len-512]);
	#endif
	}
	}
	}
	close(fd);
	return 0;
}
#endif
