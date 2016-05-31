#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE /* See feature_test_macros(7) */
#include<stdio.h>
#include<string.h>

#include <sys/types.h>
#include <unistd.h> 
#include <time.h> 

#include <sys/stat.h>
#include <fcntl.h>

// off64_t lseek64(int fd, off64_t offset, int whence); 

// gcc -D_FILE_OFFSET_BITS=64 lfs_test.c -o lfs_test
#if !USE_LFS_ACCESS_AS_LIBRARY
void writeSeq(const char* filename,unsigned long t)
{
	FILE* fp;
	char seqFile[256];

	strcpy(seqFile,filename);strcat(seqFile,".seq");
	fp=fopen(seqFile,"w+");
	if(fp){
	fwrite(&t,sizeof(t),1,fp);
	fclose(fp);
	}
}
unsigned long readSeq(const char* filename)
{
	FILE* fp;
	char seqFile[256];
	unsigned long t=time(NULL);

	strcpy(seqFile,filename);strcat(seqFile,".seq");
	fp=fopen(seqFile,"r");
	if(fp){
		fread(&t,sizeof(t),1,fp);
		fclose(fp);
	}
	return t;
}
int main(int argc,char* argv[])
{
	char filename[256];
	int fd=-1;
	unsigned long t=0;
	char sampleData[1024];int sampleLen;
	char data[1024-10];int dataLen;
	int i,numAppend=1;

	if(sizeof(off_t)<8){
		printf("size of off_t=%d\n",sizeof(off_t));
		return -1;
	}
	if(argc<2)return -2;
	strcpy(filename,argv[1]);
	if(argc>=3)numAppend=atoi(argv[2]);
	t=readSeq(filename);
	srand(t^time(NULL));

for(i=0;i<numAppend;i++)
{
	fd=open(filename,O_APPEND|O_CREAT|O_WRONLY|O_LARGEFILE/*O_NONBLOCK*//*O_SYNC*//*O_TRUNC*/,S_IRWXU);
	if(fd<0)return -3;
	lseek(fd,0,SEEK_END);

	// t+=rand()%100+250;// last time + rand// avg 300
	t+=rand()%4+1;// last time + rand// avg 3
	if(argc<3){
		strcpy(data,"agentReady sdklfjksdfkllWfUMLlfklsfklsklsdf");
		dataLen=5+rand()%(strlen(data)-5);
	}
	else{
		strcpy(data,argv[2]);
		dataLen=strlen(data);
	}
	sampleLen=sprintf(sampleData,"%08lx,%.*s\n",t,dataLen,data);
	write(fd,sampleData,sampleLen);
	close(fd);
	writeSeq(filename,t);
}
	return 0;
}
#endif /*USE_LFS_ACCESS_AS_LIBRARY*/
