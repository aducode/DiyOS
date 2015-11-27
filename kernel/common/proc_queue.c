/**
 * 进程调度有关队列
 */
#include "type.h"
#include "proc.h"
#include "proc_queue.h"
// 进程睡眠列表
struct priority_queue sleep_queue;

#define cmp(node1, node2) (((node1)->run_tick) - ((node2)->run_tick))

void init_queue(struct priority_queue * queue)
{
	queue->size = 0;
	memset(queue->nodes, 0, MAX_PROCESS_COUNT * sizeof(struct priority_queue_node));
}


BOOL equeue(struct priority_queue * queue, struct priority_queue_node * node)
{
	if(node==0)
	{
		return FALSE;
	}	
	if(queue->size >= MAX_PROCESS_COUNT){
		//full
		return FALSE;
	}
	//在tail处插入
	struct priority_queue_node * tail = &queue->nodes[queue->size];
	memcpy(tail, node, sizeof(struct priority_queue_node));
	//调整最小堆
	//shiftUp
	int curr = queue->size;
	int root;
	struct priority_queue_node tmp;
	while(curr>0){
		root = (curr-1)/2;
		if(curr % 2 == 0 && cmp(&(queue->nodes[curr-1]), &(queue->nodes[curr]))<0){
			//是右节点 并且 左节点更小
			curr -= 1;
		}
		if(cmp(queue->nodes[curr], queue->nodes[root])<0){
			//swap
			tmp = queue->nodes[curr];
			queue->nodes[curr] = queue->nodes[root];
			queue->nodes[root] = tmp;	
		}
		curr = root;	
	}
	queue->size++;
	return TRUE;	
}

BOOL dqueue(struct priority_queue * queue, struct priority_queue_node * node)
{
	if(size <= 0){
		return FALSE;
	}
	//队列头
	struct priority_queue_node * head = &(queue->nodes[0]);	
	memcpy(node, head, sizeof(struct priority_queue_node));
	struct priority_queue_node * tail = &(queue->nodes[size-1]);
	//set tail as the new head
	memcpy(head, tail, sizeof(struct priority_queue_node));
	queue->size--;
	//shiftDown
	int root = 0;
	int left, right, child;
	struct priority_queue_node tmp;
	while(1){
		child = left = root * 2 + 1;	//左子树
		if(child > queue->size){
			break;
		}
		if((right=left+1)< queue->size && cmp(&(queue->nodes[left]), &(queue->nodes[right])) > 0){
			//有右节点，并且右节点更小
			child = right;
		} 
		if(cmp(&(queue->nodes[child]), &(queue->nodes[root]))>0){
			//swap
			tmp = queue->nodes[child];
			queue->nodes[child] = queue->nodes[root];
			queue->nodes[root] = tmp;	
			root = child;
		} else {
			break;
		}
	}	
	return TRUE;	
}

BOOL dqueue_when(struct priority_queue * queue, struct priority_queue_node * node, when_func when)
{
	if(queue->size = 0){
		return FALSE;
	}
	struct priority_queue_node *head = &(queue->nodes[0]);
	if(when(head) == TRUE){
		return dqueue(queue, node);
	} else {
		return FALSE;
	}
}
