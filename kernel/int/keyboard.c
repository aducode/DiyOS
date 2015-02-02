#include "type.h"
#include "klib.h"
#include "global.h"
#include "string.h"
#include "keyboard.h"
//键盘按键状态
static int code_with_E0 = 0;
static int shift_l;	//左shift键状态
static int shift_r;	//右shift键状态
static int alt_l;	//左alt键状态
static int alt_r;	//右alt键状态
static int ctrl_l;	//左ctrl键状态
static int ctrl_r;	//右ctrl键状态
static int caps_lock;	//大小写
static int num_lock;	//小键盘区数字键
static int scroll_lock;	//滚动锁
static int column;
/**
 * 处理键盘硬件中断
 */
static void keyboard_handler(int irq_no);
//
static struct keyboard_buffer_t keyboard_buffer;

static void init_kb();
static void put_code_into_buffer();
static u8 get_code_from_buffer();

/**
 * 键盘扫描码到字符映射
 */
static u32 keymap[SCAN_CODES_COUNT * MAP_COLS] = {
/* scan-code			!Shift		Shift		E0 XX	*/
/* ==================================================================== */
/* 0x00 - none		*/	0,		0,		0,
/* 0x01 - ESC		*/	ESC,		ESC,		0,
/* 0x02 - '1'		*/	'1',		'!',		0,
/* 0x03 - '2'		*/	'2',		'@',		0,
/* 0x04 - '3'		*/	'3',		'#',		0,
/* 0x05 - '4'		*/	'4',		'$',		0,
/* 0x06 - '5'		*/	'5',		'%',		0,
/* 0x07 - '6'		*/	'6',		'^',		0,
/* 0x08 - '7'		*/	'7',		'&',		0,
/* 0x09 - '8'		*/	'8',		'*',		0,
/* 0x0A - '9'		*/	'9',		'(',		0,
/* 0x0B - '0'		*/	'0',		')',		0,
/* 0x0C - '-'		*/	'-',		'_',		0,
/* 0x0D - '='		*/	'=',		'+',		0,
/* 0x0E - BS		*/	BACKSPACE,	BACKSPACE,	0,
/* 0x0F - TAB		*/	TAB,		TAB,		0,
/* 0x10 - 'q'		*/	'q',		'Q',		0,
/* 0x11 - 'w'		*/	'w',		'W',		0,
/* 0x12 - 'e'		*/	'e',		'E',		0,
/* 0x13 - 'r'		*/	'r',		'R',		0,
/* 0x14 - 't'		*/	't',		'T',		0,
/* 0x15 - 'y'		*/	'y',		'Y',		0,
/* 0x16 - 'u'		*/	'u',		'U',		0,
/* 0x17 - 'i'		*/	'i',		'I',		0,
/* 0x18 - 'o'		*/	'o',		'O',		0,
/* 0x19 - 'p'		*/	'p',		'P',		0,
/* 0x1A - '['		*/	'[',		'{',		0,
/* 0x1B - ']'		*/	']',		'}',		0,
/* 0x1C - CR/LF		*/	ENTER,		ENTER,		PAD_ENTER,
/* 0x1D - l. Ctrl	*/	CTRL_L,		CTRL_L,		CTRL_R,
/* 0x1E - 'a'		*/	'a',		'A',		0,
/* 0x1F - 's'		*/	's',		'S',		0,
/* 0x20 - 'd'		*/	'd',		'D',		0,
/* 0x21 - 'f'		*/	'f',		'F',		0,
/* 0x22 - 'g'		*/	'g',		'G',		0,
/* 0x23 - 'h'		*/	'h',		'H',		0,
/* 0x24 - 'j'		*/	'j',		'J',		0,
/* 0x25 - 'k'		*/	'k',		'K',		0,
/* 0x26 - 'l'		*/	'l',		'L',		0,
/* 0x27 - ';'		*/	';',		':',		0,
/* 0x28 - '\''		*/	'\'',		'"',		0,
/* 0x29 - '`'		*/	'`',		'~',		0,
/* 0x2A - l. SHIFT	*/	SHIFT_L,	SHIFT_L,	0,
/* 0x2B - '\'		*/	'\\',		'|',		0,
/* 0x2C - 'z'		*/	'z',		'Z',		0,
/* 0x2D - 'x'		*/	'x',		'X',		0,
/* 0x2E - 'c'		*/	'c',		'C',		0,
/* 0x2F - 'v'		*/	'v',		'V',		0,
/* 0x30 - 'b'		*/	'b',		'B',		0,
/* 0x31 - 'n'		*/	'n',		'N',		0,
/* 0x32 - 'm'		*/	'm',		'M',		0,
/* 0x33 - ','		*/	',',		'<',		0,
/* 0x34 - '.'		*/	'.',		'>',		0,
/* 0x35 - '/'		*/	'/',		'?',		PAD_SLASH,
/* 0x36 - r. SHIFT	*/	SHIFT_R,	SHIFT_R,	0,
/* 0x37 - '*'		*/	'*',		'*',    	0,
/* 0x38 - ALT		*/	ALT_L,		ALT_L,  	ALT_R,
/* 0x39 - ' '		*/	' ',		' ',		0,
/* 0x3A - CapsLock	*/	CAPS_LOCK,	CAPS_LOCK,	0,
/* 0x3B - F1		*/	F1,		F1,		0,
/* 0x3C - F2		*/	F2,		F2,		0,
/* 0x3D - F3		*/	F3,		F3,		0,
/* 0x3E - F4		*/	F4,		F4,		0,
/* 0x3F - F5		*/	F5,		F5,		0,
/* 0x40 - F6		*/	F6,		F6,		0,
/* 0x41 - F7		*/	F7,		F7,		0,
/* 0x42 - F8		*/	F8,		F8,		0,
/* 0x43 - F9		*/	F9,		F9,		0,
/* 0x44 - F10		*/	F10,		F10,		0,
/* 0x45 - NumLock	*/	NUM_LOCK,	NUM_LOCK,	0,
/* 0x46 - ScrLock	*/	SCROLL_LOCK,	SCROLL_LOCK,	0,
/* 0x47 - Home		*/	PAD_HOME,	'7',		HOME,
/* 0x48 - CurUp		*/	PAD_UP,		'8',		UP,
/* 0x49 - PgUp		*/	PAD_PAGEUP,	'9',		PAGEUP,
/* 0x4A - '-'		*/	PAD_MINUS,	'-',		0,
/* 0x4B - Left		*/	PAD_LEFT,	'4',		LEFT,
/* 0x4C - MID		*/	PAD_MID,	'5',		0,
/* 0x4D - Right		*/	PAD_RIGHT,	'6',		RIGHT,
/* 0x4E - '+'		*/	PAD_PLUS,	'+',		0,
/* 0x4F - End		*/	PAD_END,	'1',		END,
/* 0x50 - Down		*/	PAD_DOWN,	'2',		DOWN,
/* 0x51 - PgDown	*/	PAD_PAGEDOWN,	'3',		PAGEDOWN,
/* 0x52 - Insert	*/	PAD_INS,	'0',		INSERT,
/* 0x53 - Delete	*/	PAD_DOT,	'.',		DELETE,
/* 0x54 - Enter		*/	0,		0,		0,
/* 0x55 - ???		*/	0,		0,		0,
/* 0x56 - ???		*/	0,		0,		0,
/* 0x57 - F11		*/	F11,		F11,		0,	
/* 0x58 - F12		*/	F12,		F12,		0,	
/* 0x59 - ???		*/	0,		0,		0,	
/* 0x5A - ???		*/	0,		0,		0,	
/* 0x5B - ???		*/	0,		0,		GUI_L,	
/* 0x5C - ???		*/	0,		0,		GUI_R,	
/* 0x5D - ???		*/	0,		0,		APPS,	
/* 0x5E - ???		*/	0,		0,		0,	
/* 0x5F - ???		*/	0,		0,		0,
/* 0x60 - ???		*/	0,		0,		0,
/* 0x61 - ???		*/	0,		0,		0,	
/* 0x62 - ???		*/	0,		0,		0,	
/* 0x63 - ???		*/	0,		0,		0,	
/* 0x64 - ???		*/	0,		0,		0,	
/* 0x65 - ???		*/	0,		0,		0,	
/* 0x66 - ???		*/	0,		0,		0,	
/* 0x67 - ???		*/	0,		0,		0,	
/* 0x68 - ???		*/	0,		0,		0,	
/* 0x69 - ???		*/	0,		0,		0,	
/* 0x6A - ???		*/	0,		0,		0,	
/* 0x6B - ???		*/	0,		0,		0,	
/* 0x6C - ???		*/	0,		0,		0,	
/* 0x6D - ???		*/	0,		0,		0,	
/* 0x6E - ???		*/	0,		0,		0,	
/* 0x6F - ???		*/	0,		0,		0,	
/* 0x70 - ???		*/	0,		0,		0,	
/* 0x71 - ???		*/	0,		0,		0,	
/* 0x72 - ???		*/	0,		0,		0,	
/* 0x73 - ???		*/	0,		0,		0,	
/* 0x74 - ???		*/	0,		0,		0,	
/* 0x75 - ???		*/	0,		0,		0,	
/* 0x76 - ???		*/	0,		0,		0,	
/* 0x77 - ???		*/	0,		0,		0,	
/* 0x78 - ???		*/	0,		0,		0,	
/* 0x78 - ???		*/	0,		0,		0,	
/* 0x7A - ???		*/	0,		0,		0,	
/* 0x7B - ???		*/	0,		0,		0,	
/* 0x7C - ???		*/	0,		0,		0,	
/* 0x7D - ???		*/	0,		0,		0,	
/* 0x7E - ???		*/	0,		0,		0,
/* 0x7F - ???		*/	0,		0,		0
};


/**
 *键盘中断响应函数
 */
void keyboard_handler(int irq_no)
{
	//u8 scan_code = _in_byte(IO_8042_PORT);	//清空8042缓冲才能下一次中断
	put_code_into_buffer();
}

/**
 * 初始化键盘
 */
void init_keyboard()
{
	//初始化全局键盘缓冲区，在global.c中
	init_kb();
	//开启键盘硬件中断响应
	_disable_irq(KEYBOARD_IRQ);
	irq_handler_table[KEYBOARD_IRQ] = keyboard_handler;
	_enable_irq(KEYBOARD_IRQ);	
}


//操作键盘缓冲区

/**
 * 初始化
 */
void init_kb()
{
	keyboard_buffer.count = 0;
	keyboard_buffer.p_head = keyboard_buffer.p_tail = keyboard_buffer.buffer;
}

/**
 * 将键盘扫描码放入缓冲
 */
void put_code_into_buffer()
{	u8 scan_code = _in_byte(IO_8042_PORT);
	if(keyboard_buffer.count < KEYBOARD_BUFFER_SIZE){
		*(keyboard_buffer.p_head) = scan_code;
		keyboard_buffer.p_head++;
		if(keyboard_buffer.p_head == keyboard_buffer.buffer+KEYBOARD_BUFFER_SIZE){
			keyboard_buffer.p_head = keyboard_buffer.buffer;
		}
		keyboard_buffer.count++;		
	}
}

/**
 *从缓冲中获取键盘扫描码
 */
u8 get_code_from_buffer()
{
	u8 scan_code;
	while(keyboard_buffer.count<=0){
		//block wait for code
	}
	//这里需要关中断，避免重入中断产生数据不一致
	_disable_int();
	scan_code = *(keyboard_buffer.p_tail);
	keyboard_buffer.p_tail ++;
	if(keyboard_buffer.p_tail == keyboard_buffer.buffer+KEYBOARD_BUFFER_SIZE){
		keyboard_buffer.p_tail = keyboard_buffer.buffer;
	}
	keyboard_buffer.count --;
	_enable_int();	
	return scan_code;
}



//////////////////////////////////
/**
 * 供task/tty.c中使用，用于显示keyboard input字符串
 */
void keyboard_read(){
        static int color = 0x00;
        static char msg[256];

	u8 scan_code;
	char output[2];
	int make; //1:make code 0:break code
	u32 key=0; //key表示定义在keyboard.h中的一个键
	u32* keyrow;	//表示keymap[]中的某一行
	_memset(output,0,2);
	if(keyboard_buffer.count>0){
		_disable_int();
	
	        scan_code = get_code_from_buffer();
		
		_enable_int();
		//
		if(scan_code == 0xE1) {
			//ignore
		} else if (scan_code == 0xE0) {
			code_with_E0 = 1;
		} else {
			//首先判断Make Code还是Break Code
			make = (scan_code & FLAG_BREAK ?0:1);
			keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];
			column = 0;
			if(shift_l || shift_r)
			{
				column = 1;
			}
			if(code_with_E0){	
				column = 2;
				code_with_E0 = 0;
			}
			key = keyrow[column];
			switch(key){
			case SHIFT_L:
				shift_l = make;
				key = 0;
				break;
			case SHIFT_R:
				shift_r = make;
				key = 0;
				break;
			case CTRL_L:
				ctrl_l = make;
				key = 0;
				break;
			case CTRL_R:
				ctrl_r = make;
				key = 0;
				break;
			case ALT_L:
				alt_l = make;
				key = 0;
				break;
			case ALT_R:
				alt_r = make;
				key = 0;
				break;
			default:
				if(!make){ //如果是break code
					key = 0;
				}
				break;
			}
			if(key){
				//make code 则打印
				output[0] = key;
				output[1]='\0';
				_disp_str("Press a key:)",10,0, color++);
				_disp_str(output,11,0, COLOR_WHITE);
				if(color>0xFF) color = 0x00;
				
			}
		}	
        	//_disp_str("Press a key :)",10,0,color++);
	        //itoa(scan_code, msg,16);
	        //_disp_str(msg,11,0,COLOR_WHITE);
	        //if(color>0xFF)color=0x00;
	}
}

