#ifndef _DIYOS_INTERRUPT_H
#define _DIYOS_INTERRUPT_H
//中断处理函数类型
typedef void (*int_handler)();			//中断向量的表项
typedef void (*irq_handler)(int irq_no);	//定义硬件中断处理函数
#define MAX_IRQ_HANDLER_COUNT	16

#define MAX_SYSCALL_COUNT	1		//系统调用数
/* Hardware interrupts */
#define CLOCK_IRQ       0
#define KEYBOARD_IRQ    1
#define CASCADE_IRQ     2       /* cascade enable for 2nd AT controller */
#define ETHER_IRQ       3       /* default ethernet interrupt vector */
#define SECONDARY_IRQ   3       /* RS232 interrupt vector for port 2 */
#define RS232_IRQ       4       /* RS232 interrupt vector for port 1 */
#define XT_WINI_IRQ     5       /* xt winchester */
#define FLOPPY_IRQ      6       /* floppy disk */
#define PRINTER_IRQ     7
#define AT_WINI_IRQ     14      /* at winchester */
//中断处理函数
/* 中断处理函数 */
extern void	_divide_error();
extern void	_single_step_exception();
extern void	_nmi();
extern void	_breakpoint_exception();
extern void	_overflow();
extern void	_bounds_check();
extern void	_inval_opcode();
extern void	_copr_not_available();
extern void	_double_fault();
extern void	_copr_seg_overrun();
extern void	_inval_tss();
extern void	_segment_not_present();
extern void	_stack_exception();
extern void	_general_protection();
extern void	_page_fault();
extern void	_copr_error();

//硬件中断
extern void    _hwint00();
extern void    _hwint01();
extern void    _hwint02();
extern void    _hwint03();
extern void    _hwint04();
extern void    _hwint05();
extern void    _hwint06();
extern void    _hwint07();
extern void    _hwint08();
extern void    _hwint09();
extern void    _hwint10();
extern void    _hwint11();
extern void    _hwint12();
extern void    _hwint13();
extern void    _hwint14();
extern void    _hwint15();


extern void default_irq_handler(int irq_no);
#endif
