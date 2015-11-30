#ifndef _DIYOS_CLOCK_H
#define _DIYOS_CLOCK_H
extern void init_clock();
//#define MAX_TICKS	0x7FFFABCD
#define MAX_TICKS	0xFFFFFFFFFFFL

/**
 * 关于8253/54 PIT,可以参考:[https://en.wikipedia.org/wiki/Intel_8253](https://en.wikipedia.org/wiki/Intel_8253)
 */
//8253/8254 PIT（Programmable Interval Timer)
//默认的中断频率是18.2HZ
//通过如下参数设置后会10ms发生一次时钟中断
#define TIMER0		0x40	//I/O port for timer channel 0
#define TIMER_MODE	0x43    //I/O port for timer mode control

/*********************************************************/
/** SC1   SC0 |  RW2 RW1 	| M2 M1 M0  | 数制选择	**/
/** 计数器选择| 读/写指示	| 模式选择  | 	        **/
/** 00:计数器0| 00:锁存	 	| 000:模式0 | 0:二进制	**/
/** 01:计数器1| 01:置读/写低8位 | 001:模式1 | 1:BCD	**/
/** 10:计数器2| 10:只读/写高8位 | 010:模式2 |		**/
/** 11：非法  | 11:先读/写高8位 | 011:模式3 |		**/
/** 	      |    再读/写低8位 | 100:模式4 |		**/
/**	      |			| 101:模式5 |		**/
/*********************************************************/
#define RATE_GENERATOR	0x34    //00-11-010-0: 计数器0-先读写高8位再读写低8位-模式2-二进制
				//Counter0 - LSB then MSB - rate generator - binary


//1秒中有1193180次周期
//每一个周期会对timer0的初始值做一次递减，当减为0后产生一次时钟中断，然后恢复初始值
#define TIMER_FREQ	1193180L	//clock frequency for timer in PC and AT
#define HZ		100	    	// clock freq (software settable on IBM-PC)

//我们设置的timer0初始值 每10ms产生一次时钟中断
#define LATCH		((TIMER_FREQ + HZ / 2 ) / HZ)

//每个时钟中断的时间
#define TICK_SIZE	(1000 / HZ)
#endif
