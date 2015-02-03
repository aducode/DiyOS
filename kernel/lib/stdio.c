#include "type.h"
#include "string.h"
#include "stdlib.h"
static int vsprintf(char *buf, const char * fmt, va_list args);
int printf(const char *fmt, ...)
{
	int i;
	char buf[256];
	va_list arg = (va_list)((char*)(&fmt+4));
	i = vsprintf(buf,fmt, arg);
	write(buf, i);
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
			case 's':
				break;
			default:
				break;
		}
		return (p-buf);
	}
}
