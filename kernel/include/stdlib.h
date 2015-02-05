#ifndef _DIYOS_STDLIB_H
#define _DIYOS_STDLIB_H
//系统调用
extern void sendrec(int function, int src_dest, int p_msg);
extern void write(char * buffer, int size);
#endif
