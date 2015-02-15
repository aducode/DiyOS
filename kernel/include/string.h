#ifndef _DIYOS_INCLUDE_STRING_H
#define _DIYOS_INCLUDE_STRING_H
extern void memset(void* p_dst, char ch , int size);
extern void* memcpy(void* p_dst, void* p_src, int size);
extern int memcmp(const void* s1, const void* s2, int n);
extern int strlen(const char * p_str);
extern char* strcpy(char *p_dst, char *p_src);
extern int strcmp(const char* s1, const char *s2);
extern void itoa(int value, char * string, int radix);
#endif
