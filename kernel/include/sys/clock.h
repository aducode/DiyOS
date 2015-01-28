#ifndef _DIYOS_CLOCK_H
#define _DIYOS_CLOCK_H
extern void init_clock();
//#define MAX_TICKS	0x7FFFABCD
#define MAX_TICKS	0xFFFFFFFFFFFL

//8253/8254 PIT（Programmable Interval Timer)
#define TIMER0		0x40	//I/O port for timer channel 0
#define TIMER_MODE	0x43    //I/O port for timer mode control
#define RATE_GENERATOR	0x34    //00-11-010-0:
				//Counter0 - LSB then MSB - rate generator - binary

#define TIMER_FREQ	1193182L//clock frequency for timer in PC and AT
#define HZ		100     // clock freq (software settable on IBM-PC)
#endif
