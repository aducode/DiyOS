#ifndef _DIYOS_WAIT_H
#define _DIYOS_WAIT_H
/**
 * @function wait
 * @brief Wait for the child process to terminiate.
 * 
 * @param status The value returned from the child.
 *
 * @return PID of the terminated child.
 */
extern int wait(int * status);
#endif
