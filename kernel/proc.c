#include "type.h"
#include "syscall.h"
#include "protect.h"
#include "proc.h"
#include "global.h"
#include "string.h"
#include "assert.h"
#include "clock.h"
#include "hd.h"
static int ldt_seg_linear(struct process *p_proc, int idx);
//判断消息发送是否有环
static int deadlock(int src, int dest);

static int msg_send(struct process *p_proc, int dest, struct message *msg);

static int msg_receive(struct process *p_prc, int src, struct message *msg);

static void block(struct process *p_proc);

static void unblock(struct process *p_proc);

/**
 * 进程调度
 */
void schedule()
{
	/*
	struct process *p;
        int greatest_ticks = 0;
        while(!greatest_ticks){
                for(p=proc_table;p<=proc_table+TASKS_COUNT+PROCS_COUNT-1;p++){
                        if(p->p_flags == 0){
                                if(p->ticks > greatest_ticks){
                                        greatest_ticks = p->ticks;
                                        p_proc_ready = p;
                                }
                        }
                }
        }
        if(!greatest_ticks){
                for(p=proc_table;p<=proc_table+TASKS_COUNT+PROCS_COUNT+1;p++){
                        if(p->p_flags == 0){
                                p->ticks = p->priority;
                        }
                }
        }

	*/
	static int i=0;
	struct process *p;
	i++;
	//这里能使用printk是因为我们0号进程设置成task_tty了，printk会用到tty的write,所以当时钟中断发生时write已经可以被调用了
	if(i>=TASKS_COUNT + PROCS_COUNT) i=0;
	p=proc_table + i;
	while(p->p_flags>0){
		//p_flags不为0表示需要阻塞线程，不分配cpu时间片
		i++;
		if(i>=TASKS_COUNT + PROCS_COUNT) i=0;
		p=proc_table + i;
	}
	p_proc_ready = p;
}
/**
 * 计算线程ldt中idx索引的线性地址
 */
int ldt_seg_linear(struct process *p_proc, int idx)
{
	struct descriptor *p_desc = &(p_proc->ldts[idx]);
//	struct descripotr *p_desc = &p_proc->ldts[idx];//不带括号就会出错
	return p_desc->base_high << 24 | p_desc->base_mid << 16 |p_desc->base_low;
}
/**
 * 虚拟地址转线性地址(ring1 - ring3使用)
 */
void* va2la(int pid, void *va)
{
	struct process *p_proc = &proc_table[pid];
	u32 seg_base = ldt_seg_linear(p_proc, INDEX_LDT_RW);
	u32 la = seg_base + (u32)va;
	
	if(pid < TASKS_COUNT + PROCS_COUNT )
	{
		assert(la==(u32)va);
	}
	return (void*)la;
}

/**
 * 重置消息
 */
void reset_msg(struct message *p_msg)
{
	memset(p_msg, 0, sizeof(struct message));
}

/**
 * 阻塞线程
 */
void block(struct process *p_proc)
{
	assert(p_proc->p_flags);
	schedule();
}

/**
 *解除阻塞
 */
void unblock(struct process *p_proc)
{
	assert(p_proc->p_flags == 0);
}

/**
 * 判断消息发送是否存在环
 * @param src 消息来源
 * @param dest  消息目的地
 * @return 0表示没有循环
 */
int deadlock(int src, int dest)
{
	struct process *p_proc = proc_table + dest;
	while(1){
		if(p_proc->p_flags & SENDING){
			//表示目的进程正在发送
			if(p_proc->p_sendto == src){
				//表示目标进程也在给源进程发消息
				p_proc = proc_table + dest;
				printk("=_=%s", p_proc->name);
				do{
					assert(p_proc->p_msg);
					p_proc = proc_table + p_proc->p_sendto;
					printk("->%s", p_proc->name);
				}while(p_proc != proc_table + src);
				printk("=_=");
				return 1;
			}
			p_proc = proc_table + p_proc->p_sendto;
		}else {
			break;
		}
	}
	return 0;	
}

/**
 * 发送消息
 * @param current 当前进程
 * @param dest    目标进程id
 * @param msg     消息
 */
int msg_send(struct process *current, int dest, struct message *m)
{
	struct process *sender = current; //发送者
	struct process *receiver = proc_table + dest;//接收者
	int src = proc2pid(sender);
	assert(src!=dest);	//不能给自己发
	//检查死锁
	if(deadlock(src,dest)){
		panic(">>DEADLOCK<< %s->%s", sender->name, receiver->name);
	}
	//printk("[msg_send]\t[%d] send message to [%d]\n",src, dest);
	if((receiver->p_flags & RECEIVING) && //dest is waiting for the msg
		(receiver->p_recvfrom == src || receiver->p_recvfrom == ANY)){
		//目标进程在等待消息，并且发送的目标接收者被设置成本进程，或接收广播
		assert(receiver->p_msg);
		assert(m);
		memcpy(va2la(dest,receiver->p_msg), va2la(src,m), sizeof(struct message));
		//msg_receive中会把消息地址设置成receiver->p_msg,所以上面的memcpy会把消息体复制到那个地址，复制完后，下面把p_msg归零
		receiver->p_msg = 0; 
		receiver->p_flags &= ~RECEIVING;	//表示已经接收了 这里解除阻塞
		receiver->p_recvfrom = NO_TASK;		//将receiver->p_recvfrom重置成NO_TASK	
		unblock(receiver);
		assert(receiver->p_flags == 0);
		assert(receiver->p_msg == 0);
		assert(receiver->p_recvfrom == NO_TASK);
		assert(receiver->p_sendto == NO_TASK);
		
		assert(sender->p_flags == 0);
		assert(sender->p_msg == 0);
		assert(sender->p_recvfrom == NO_TASK);
		assert(sender->p_sendto == NO_TASK);
	} else {
		//目标没有等待这条消息
		sender->p_flags |= SENDING;	//阻塞
		assert(sender->p_flags == SENDING);
		sender->p_sendto = dest;
		sender->p_msg = m;
		//append to the sending queue
		struct process *p;
		if(receiver->q_sending){
			p=receiver->q_sending;
			while(p->next_sending){
				p=p->next_sending;
			}
			p->next_sending = sender;
		} else {
			receiver->q_sending = sender;
		}
		sender->next_sending = 0;
		block(sender);
		assert(sender->p_flags == SENDING);
		assert(sender->p_msg != 0);
		assert(sender->p_recvfrom == NO_TASK);
		assert(sender->p_sendto == dest);
	}
	return 0;
}



int msg_receive(struct process *current, int src, struct message *m)
{
	struct process *receiver = current;
	struct process *sender = 0;
	struct process *prev = 0;
	int copyok = 0;
	int dest = proc2pid(receiver);
	//printk("[msg_receive]\t[%d] receive message from [%d]\n", dest,src);
	assert(dest != src);
	if((receiver->has_int_msg) && ((src==ANY)||(src==INTERRUPT))){
		//处理中断
		struct message msg;
		reset_msg(&msg);
		msg.source = INTERRUPT;
		msg.type = HARD_INT;
		assert(m);
		memcpy(va2la(dest,m), &msg, sizeof(struct message));
		receiver->has_int_msg=0;
		assert(receiver->p_flags == 0);
		assert(receiver->p_msg == 0);
		assert(receiver->p_sendto == NO_TASK);
		assert(receiver->has_int_msg == 0);
		return 0;
	}
	//一般消息
	if(src==ANY){
		//接收广播消息
		if(receiver->q_sending){
			sender = receiver->q_sending;
			copyok = 1;
			assert(receiver->p_flags == 0);
			assert(receiver->p_msg == 0);
			assert(receiver->p_recvfrom == NO_TASK);
			assert(receiver->p_sendto == NO_TASK);
			assert(receiver->q_sending != 0);
			assert(sender->p_flags == SENDING);
			assert(sender->p_msg != 0);
			assert(sender->p_recvfrom == NO_TASK);
			assert(sender->p_sendto == dest);
		}
	} else {
		//接收点对点消息
		sender = proc_table + src;
		if((sender->p_flags & SENDING) &&
			(sender->p_sendto == dest)){
			//说明sender（消息来源）正好在给该进程发消息
			copyok = 1;
			struct process *p = receiver->q_sending;
			assert(p);
			while(p){
				assert(sender->p_flags & SENDING);
				if(proc2pid(p) == src){
					//找到接收队列中的特定消息
					sender=p;
					break;
				}
				prev = p;
				p=p->next_sending;
			}
			assert(receiver->p_flags==0);
			assert(receiver->p_msg == 0);
			assert(receiver->p_recvfrom == NO_TASK);
			assert(receiver->p_sendto == NO_TASK);
			assert(receiver->q_sending != 0);
			assert(sender->p_flags == SENDING);
			assert(sender->p_msg !=0);
			assert(sender->p_recvfrom == NO_TASK);
			assert(sender->p_sendto == dest);
		}
	}
	if(copyok){
		if(sender == receiver->q_sending){
			//从接收队列中删除
			assert(prev == 0);
			receiver->q_sending = sender->next_sending;
			sender->next_sending = 0;
		} else {
			assert(prev);
			prev->next_sending = sender->next_sending;
			sender->next_sending = 0;
		}
		assert(m);
		assert(sender->p_msg);
		memcpy(va2la(dest, m), va2la(proc2pid(sender), sender->p_msg), sizeof(struct message));
		sender->p_msg = 0;
		sender->p_sendto = NO_TASK;
		sender->p_flags &= ~SENDING;
		unblock(sender);
	} else {
		//nobody's sending any msg
		receiver->p_flags |= RECEIVING;
		receiver->p_msg = m;  //这里将消息地址设置给receiver->p_msg，在msg_send中会使用这个地址
		if(src == ANY){
			receiver->p_recvfrom = ANY;
		} else {
			receiver->p_recvfrom = proc2pid(sender);
		}
		block(receiver);
		assert(receiver->p_flags == RECEIVING);
		assert(receiver->p_msg !=0 );
		assert(receiver->p_recvfrom != NO_TASK);
		assert(receiver->p_sendto == NO_TASK);
		assert(receiver->has_int_msg == 0);
	}
	return 0;
}

//ring0
int sys_sendrec(int function, int dest_src, struct message *msg , struct process *p_proc)
{
	assert(k_reenter == 0); //make sure we are not in ring0
	assert((dest_src >= 0 && dest_src < TASKS_COUNT + PROCS_COUNT) || dest_src == ANY || dest_src == INTERRUPT);	
	int ret;
	int caller = proc2pid(p_proc);
	struct message *mla = (struct message *)va2la(caller, msg);
	mla->source = caller;
	assert(mla->source != dest_src);
	switch(function){
		case SEND:
			ret = msg_send(p_proc, dest_src, msg);
			break;
		case RECEIVE:
			ret = msg_receive(p_proc, dest_src, msg);
			break;
		default:
			panic("{sys_sendrec} invalid function:%d (SEND:%d, RECEIVE:%d).",function, SEND, RECEIVE);
			break;
	}		
	return ret;
}


/**
 * 用于输出消息内容
 */
void dump_msg(const char * title, struct message *msg)
{
	int packed = 0;
	//这里能使用printk，是在tty进程已经被启动起来的情况下，所以dump_msg要在tty之后的进程中使用
	printk("{%s}<0x%x>{%ssrc:%s(%d),%stype:%d,%s(0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)%s}%s\n",
	title,
	(int)msg,
	packed?"":"\n	",
	proc_table[msg->source].name,
	msg->source,
	packed?" ":"\n	",
	msg->type,
	packed?" ":"\n	",
	msg->u.m3.m3i1,
	msg->u.m3.m3i2,
	msg->u.m3.m3i3,
	msg->u.m3.m3i4,
	(int)msg->u.m3.m3p1,
	(int)msg->u.m3.m3p2,
	packed?"":"\n",
	packed?"":"\n"
	);
}



void inform_int(int task_idx)
{
	struct process *p = proc_table + task_idx;
	if((p->p_flags & RECEIVING) && ((p->p_recvfrom == INTERRUPT) || (p->p_recvfrom == ANY))){
		p->p_msg->source = INTERRUPT;
		p->p_msg->type = HARD_INT;
		p->p_msg = 0;
		p->has_int_msg = 0;
		p->p_flags &= ~RECEIVING;
		p->p_recvfrom = NO_TASK;
		assert(p->p_flags == 0);
		unblock(p);

		assert(p->p_flags == 0);
		assert(p->p_msg == 0);
		assert(p->p_recvfrom == NO_TASK);
		assert(p->p_sendto == NO_TASK);
	} else {
		p->has_int_msg = 1;
	}
}


/**
 * Wait for a certain status
 * @param port	reg port
 * @param mask  Status mask
 * @param val   Required status
 * @param timeout      Timeout in milliseconds
 */
int waitfor(int reg_port, int mask, int val, int timeout)
{
        int t = (int)get_ticks();
        while((((int)get_ticks()-t)*1000/HZ)<timeout){
                //
                if((_in_byte(reg_port) & mask) == val){
                        return 1;
                }
        }
        return 0;
}


/**
 * wait until a disk interrupt occurs
 */
void interrupt_wait()
{
        struct message msg;
        send_recv(RECEIVE, INTERRUPT, &msg);
}

