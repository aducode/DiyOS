#ifndef _DIYOS_INTERRUPT_H
#define _DIYOS_INTERRUPT_H
//中断处理函数类型
typedef void (*int_handler)();

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
#endif
