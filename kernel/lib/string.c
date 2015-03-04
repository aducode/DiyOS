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
/**
 * memcpy
 */
void* memcpy(void* p_dst,const void* p_src, int size)
{
	void *ret = p_dst;
	int i;
	for(i=0;i<size;i++)
	{
		//void* 类型不能直接用*取值，需要转换类型
		*(char*)p_dst = *(char*)p_src;
		p_dst = (char*)p_dst + 1;
		p_src = (char*)p_src + 1;
	}
	return ret;
}

/**
 * @function memset
 * @brief 设置一段数据的值
 *
 * @param p_dst 目标
 * @param ch 要填充的内容
 * @param size 填充的长度
 *
 * @return oid
 */
void memset(void* p_dst, char ch, int size)
{
	int i;
	for(i=0;i<size;i++)
	{
		*(char*)p_dst = ch;
		p_dst = (char*)p_dst + 1;
	}
}

/**
 * @function memcmp
 * @brief 比较段数据大小
 *
 * @param s1 the 1st area.
 * @param s2 the 2nd area.
 * @param n the first n bytes will be compared.
 *
 * @return 0  eqaul
 *        >0 1st larger
 *        <0 2nd larger
 */
int memcmp(const void* s1, const void* s2, int n)
{
	if((s1==0)||(s2==0)){
		return (s1-s2);
	}
	const char*p1=(const char*)s1;
	const char*p2=(const char*)s2;
	int i;
	for(i=0;i<n;i++, p1++, p2++){
		if(*p1!=*p2){
			return (*p1-*p2);
		}
	}
	return 0;
}
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
int strlen(const char * p_str)
{
	if(!p_str) return 0;
	int i;
	const char * p;
	for(p=p_str,i=0;*p!='\0';p++,i++){}
	return i;
}


/**
 * @function strcmp
 * @brief  比较字符串
 *
 * @param s1 字符串1
 * @param s2 字符串2
 *
 * @return 0相等  >0 s1大  <0 s2大
 */
int strcmp(const char * s1, const char *s2)
{
	if((s1==0)||(s2==0)){
		return (s1-s2);
	}
	const char* p1=s1;
	const char* p2=s2;
	for(;*p1 && *p2;p1++,p2++){
		if(*p1 != *p2){
			break;
		}
	}
	return (*p1 - *p2);
}

/**
 * @function itoa
 * @brief 整形转字符串
 *
 * @param value 整数
 * @param string 字符串位置
 * @param radix  进制 2 8 16 10
 *
 * @return void
 */
void itoa(int value, char * string, int radix)
{
	int flag = 0; //是否小于0
        if(value<0){
		value = 0-value;
                flag = 1;
        }

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
	if(flag){
               *s++='-';
               len++;
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
	/*
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
	*/
	*(s+len)='\0';
}




