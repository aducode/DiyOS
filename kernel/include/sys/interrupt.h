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

//硬件中断
void    _hwint00();
void    _hwint01();
void    _hwint02();
void    _hwint03();
void    _hwint04();
void    _hwint05();
void    _hwint06();
void    _hwint07();
void    _hwint08();
void    _hwint09();
void    _hwint10();
void    _hwint11();
void    _hwint12();
void    _hwint13();
void    _hwint14();
void    _hwint15();

#endif
