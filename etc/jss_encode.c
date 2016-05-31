#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int main(int argc,char* argv[])
{
	int ch;
	const char* insertPre=NULL;
	const char* appendPost=NULL;

	if(argc>=2)insertPre=argv[1];
	if(argc>=3)appendPost=argv[2];
	if(insertPre)printf("%s",insertPre);
	while((ch=getchar())!=EOF)
	{
// json_emit_quoted_str
		/*\u1234*/
		if(ch=='\"'){putchar('\\');putchar('\"');}
		else if(ch=='\f'){putchar('\\');putchar('f');}
		else if(ch=='\n'){putchar('\\');putchar('n');}
		else if(ch=='\r'){putchar('\\');putchar('r');}
		else if(ch=='\t'){putchar('\\');putchar('t');}
		else if(ch=='\b'){putchar('\\');putchar('b');}
		else if(ch=='\\'){putchar('\\');putchar('\\');}
		else if(ch=='/'){putchar('\\');putchar('/');}
		else if(ch<0x20||ch>0x10FFFF){printf("\\u%04x",(unsigned char)ch);}
		else 	putchar(ch);
	}
	if(appendPost)printf("%s",appendPost);
}
