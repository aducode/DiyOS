#include "type.h"
#include "proc.h"

#ifndef _DIYOS_PROC_QUEUE_H
#define _DIYOS_PROC_QUEUE_H

/**
 * 优先级队列
 */
struct priority_queue_node {
	long long run_tick; 	//key
	int pid;		//pid
};

struct priority_queue {
	int size;
	struct priority_queue_node nodes[MAX_PROCESS_COUNT];
};

extern struct priority_queue sleep_queue;

extern void init_queue(struct priority_queue * queue);
/**
 * @function equeue
 * @brief 入队列
 * @param queue
 * @param node
 * @return TRUE if not full
 */
extern BOOL equeue(struct priority_queue * queue,  struct priority_queue_node *node);

/**
 * @function dqueue
 * @brief 出队列
 * @param queue
 * @param [out]node
 * @return TRUE if not empty
 */
extern BOOL dqueue(struct  priority_queue * queue, struct priority_queue_node * node);


/**
 * 条件函数
 * 当node满足条件返回TRUE，否则返回FALSE
 */
typedef BOOL (when_func *)(struct priority_queue_node * node);

/**
 * @function dqueue_when
 * @brief 当when(head) == TRUE时出对列并返回TRUE， 否则返回FALSE
 * @param [in] queue
 * @param [out] node
 * @param [in] when
 *
 * @return TRUE if not empty and when(head)==TRUE
 */
extern BOOL dqueue_when(struct priority_queue * queue, struct priority_queue_node * node, when_func when);

#endif
