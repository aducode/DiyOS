/*
#include <stdio.h>
void itoa(int value, char * string, int radix);
void strcpy(char *, char*);
int main(int argv, char ** argc)
{
	char str[256];
	char str2[256]="hello world";
	itoa(20, str, 8);
	strcpy(str2+11, str);
	printf("%s\n",str2);
}
*/
//#include "klib.h"
/**
 *不依赖汇编函数
 */
char* strcpy(char * p_dst, char *p_src)
{
	char * r = p_dst;
	while((*(p_dst++)=*(p_src++))!='\0');
	return r;
}
//void strcpy(char * p_dst, char * p_src)
//{
//	int size = strlen(p_src)+1;
//	_strcpy(p_dst, p_src, size);
//}
int strlen(char * p_str)
{
	if(!p_str) return 0;
	int i;
	char * p;
	for(p=p_str,i=0;*p!='\0';p++,i++){}
	return i;
}
void itoa(int value, char * string, int radix)
{
	int len=0,i,t;
	char *s=string, tmp;
	while(value>0)
	{
		t=value%radix;
		if(t<10){
			*s = (char)((int)'0' + t);
		} else {
			//16进制中的A-F
			*s = (char)((int)'A'+ t-10);
		}
		value /= radix;
		len++;
		s++;
	}
	s=string;
	for(i=0;i<len/2;i++)
	{
		tmp = *(s+i);
		*(s+i)=*(s+len-1-i);
		*(s+len-1-i)=tmp;
	}
	if(len<=0){
		*(s+len++)='0';
	}
	switch(radix)
	{
		case 16:
			*(s+len++)='H';
			break;
		case 8:
			*(s+len++)='O';
			break;
		case 2:
			*(s+len++)='B';
			break;
		default:
			break;
			//pass
	}
	*(s+len)='\0';
}




