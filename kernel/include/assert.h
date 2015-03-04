#ifndef _DIYOS_ASSERT_H
#define _DIYOS_ASSERT_H
#define MAG_CH_PANIC	'\002'
#define MAG_CH_ASSERT	'\003'
#define ASSERT
#ifdef ASSERT
extern int printk(const char* fmt, ...);
extern void spin(const char *);
extern void assertion_failure(char *exp, char *file, char *base_file, int line);
#define assert(exp) if(exp);\
	else assertion_failure(#exp,__FILE__,__BASE_FILE__, __LINE__);
#else
#define assert(exp)
#endif
#endif
