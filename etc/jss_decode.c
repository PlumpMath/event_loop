#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int main(int argc,char* argv[])
{
	int ch,ch1,ch2;
	int skipN=34,tailCut=2;

	if(argc>=2)skipN=atoi(argv[1]);
	if(argc>=3)tailCut=atoi(argv[2]);
	while((ch=getchar())!=EOF)
	{
		/*\u1234*/
		if(ch=='\\'){
			ch1=getchar();
			if(ch1=='\"')putchar('\"');
			else if(ch1=='n')putchar('\n');
			else if(ch1=='r')putchar('\r');
			else if(ch1=='t')putchar('\t');
			else if(ch1=='b')putchar('\b');
			else if(ch1=='f')putchar('\f');
			else if(ch1=='\\')putchar('\\');
			else if(ch1=='/')putchar('/');
			else if(ch1=='u'){scanf("%04x",&ch2);putchar(ch2);}
		}
		else{
			if(skipN>0)skipN--;
			else if(tailCut&&ch=='\"'){
				tailCut--;	
				if(tailCut>0){ch1=getchar();tailCut--;}
			}
			else putchar(ch);
		}
	}
}
