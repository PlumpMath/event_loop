#include<stdio.h>
#ifdef __CYGWIN__
#warning "CYGWIN defined"
#endif
#ifdef __CYGWIN64__
#warning "CYGWIN64 defined"
#endif
#ifdef __x86_x64__
#warning "x64 defined"
#endif
int main(int argc,char* argv[])
{
	printf("sizeof char=%d\n",sizeof(char));//1,1
	printf("sizeof short=%d\n",sizeof(short));//2,2
	printf("sizeof int=%d\n",sizeof(int));//4,4
	printf("sizeof long long=%d\n",sizeof(long long));//8,8
printf("\n");
	printf("sizeof long int=%d\n",sizeof(long int));//4,8
	printf("sizeof void*=%d\n",sizeof(void*));//4,8
printf("\n");
	if(sizeof(long int)==8)printf("running on 64bits(gcc)\n");
	if(sizeof(long int)==4)printf("running on 32bits(gcc)\n");
	return 0;
}
