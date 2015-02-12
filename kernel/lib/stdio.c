#include "type.h"
#include "string.h"
#include "syscall.h"
/**
 *printf
 */
int printf(const char *fmt, ...)
{
	int i;
	char buf[256];
	//开始写成下面的，导致不定参赛出错
	//va_list arg = (va_list)((char*)(&fmt+4));
	//正确写法如下，&fmt取地址（地址类型是4Byte 32bit的，我机器是32位），将4Byte的地址转换成char* 变为1Byte 这样后面+4 就是真正的加4Byte，变成下一个参数首地址
	va_list arg = (va_list)((char*)(&fmt)+4);
	//或者下面这种写法
	//va_list arg = (va_list)((char*)(&fmt+1));
	i = vsprintf(buf,fmt, arg);
	write(buf, i);
	return i;
}
/**
 *格式化输出到字符串
 */
int sprintf(char *buffer, const char *fmt, ...)
{
	int i;
	va_list arg = (va_list)((char*)(&fmt)+4);
	i = vsprintf(buffer, fmt, arg);
	return i;
}
int vsprintf(char *buf, const char * fmt, va_list args)
{
	char *p;
	char tmp[256];
	va_list p_next_arg = args;
	for(p=buf;*fmt;fmt++){
		if(*fmt!='%'){
			*p++ = *fmt;
			continue;	
		}
		fmt++;
		switch(*fmt){
			case 'x':
				itoa(*((int*)p_next_arg),tmp, 16);
				strcpy(p,tmp);	
				p_next_arg += 4;
				p+=strlen(tmp);
				break;
			case 'd':
				itoa(*((int*)p_next_arg),tmp,10);
				strcpy(p, tmp);
				p_next_arg += 4;
				p+=strlen(tmp);
				break;
			case 'o':
				itoa(*((int*)p_next_arg),tmp, 8);
				strcpy(p, tmp);
				p_next_arg += 4;
				p+=strlen(tmp);
				break;
			case 'c':
				*p++ = *((char*)p_next_arg);
				p_next_arg += 4;
				break;
			case 's':
				//p_next_arg 是char *类型的地址，所以这里要把类型转成char **  代表是char *的地址，然后在*运算符取地址，变成char*类型
				strcpy(p,(*((char**)p_next_arg)));
				p += strlen(*((char**)p_next_arg));
				p_next_arg += 4;
				break;
			default:
				break;
		}
	}
	return (p-buf);
}
