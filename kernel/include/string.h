#ifndef _DIYOS_INCLUDE_STRING_H
#define _DIYOS_INCLUDE_STRING_H
extern void memset(void* p_dst, char ch , int size);
extern void* memcpy(void* p_dst, void* p_src, int size);
extern int strlen(char * p_str);
extern char* strcpy(char *p_dst, char *p_src);
extern void itoa(int value, char * string, int radix);
#endif
