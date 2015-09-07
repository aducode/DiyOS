#include "type.h"
#include "fs.h"
#include "interrupt.h"
#include "protect.h"
#include "proc.h"
#include "keyboard.h"
#ifndef _DIYOS_GLOBAL_H
#define _DIYOS_GLOBAL_H
#ifndef _DIYOS_GLOBAL_C_HERE
//extern struct descriptor_table gdt_ptr;
extern struct descriptor gdt[];
extern u8 gdt_ptr[];
extern struct gate idt[];
extern u8  idt_ptr[];

extern irq_handler irq_handler_table[];

extern struct process* p_proc_ready;
extern struct process  proc_table[];

extern struct tss g_tss;

//extern struct bios_data_area bda;
extern struct boot_params boot_params;

extern int k_reenter;

extern struct task task_table[];
extern struct task user_proc_table[];
extern char task_stack[];

//extern int ticks;
extern long long ticks;

//sys_call
extern system_call sys_call_table[];

//
extern struct tty tty_table[];
extern struct console console_table[];
//当前console
extern int current_console;
//
extern struct dev_drv_map dd_map[];

//FS Buffer
extern u8 * fsbuf;
extern const int FSBUF_SIZE;

extern struct inode *root_inode;
extern struct file_desc f_desc_table[];
extern struct inode inode_table[];

extern int key_pressed;

//粗粒度的不可重入锁，全局内核锁
//只提供给内核级进程(TASK)使用
//进程内调用lock_kernel之后独占CPU时间片
extern int kernel_lock;
#endif
#endif
