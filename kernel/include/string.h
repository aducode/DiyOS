#ifndef _DIYOS_INCLUDE_STRING_H
#define _DIYOS_INCLUDE_STRING_H
/**
 * @function memset
 * @brief 初始化内存
 * @param p_dst  内存起始位置
 * @param ch  初始化的值
 * @param size limit
 * @return 0 success
 */
extern void memset(void* p_dst, char ch , int size);
/**
 * @function memcpy
 * @brief 复制内存数据
 * @param p_dst
 * @param p_src
 * @param size
 * @return
 */
extern void* memcpy(void* p_dst, void* p_src, int size);
/**
 * @function memcmp
 * @brief 比较内存数据
 * @param s1
 * @param s2
 * @param n
 * @return
 */
extern int memcmp(const void* s1, const void* s2, int n);
/**
 * @function strlen
 * @brief 计算字符串长度
 * @param p_str
 * @return 字符串长度
 */
extern int strlen(const char * p_str);
/**
 * @function strcpy
 * @brief 复制字符串
 * @param p_dst
 * @param p_src
 * @return 
 */
extern char* strcpy(char *p_dst, char *p_src);
/**
 * @function strcmp
 * @brief 字符串比较
 * @param s1
 * @param s2
 * @return
 */
extern int strcmp(const char* s1, const char *s2);
/**
 * @function itoa
 * @brief 整型转字符串
 * @param value 整型值
 * @param string 返回的字符串
 * @param int radix 制式
 */
extern void itoa(int value, char * string, int radix);
#endif
