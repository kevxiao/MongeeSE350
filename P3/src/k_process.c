/**
 * @file:   k_process.c  
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/02/28
 * NOTE: The example code shows one way of implementing context switching.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes only two simple user processes and NO HARDWARE INTERRUPTS. 
 *       The purpose is to show how context switch could be done under stated assumptions. 
 *       These assumptions are not true in the required RTX Project!!!
 *       If you decide to use this piece of code, you need to understand the assumptions and
 *       the limitations. 
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"
#include "uart.h"


#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs;                  /* array of pcbs */
PCB *gp_current_process = NULL; /* always point to the current RUN process */

MSG_BUF * gp_delayed_msgs = NULL;

U32 g_switch_flag = 0;         /* whether to continue to run the process before the UART receive interrupt */
                                /* 1 means to switch to another process, 0 means to continue the current process */
				/* this value will be set by UART handler */

/* process initialization table */
PROC_INIT g_proc_table[NUM_TEST_PROCS + NUM_SYS_PROCS + NUM_IRQ_PROCS + NUM_USER_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];
extern PROC_INIT g_user_procs[NUM_USER_PROCS];
extern PROC_INIT g_sys_procs[NUM_SYS_PROCS];
extern PROC_INIT g_irq_procs[NUM_IRQ_PROCS];

//DELAYED_MSGS g_delayed_msgs;

PCB* gp_priority_end[NUM_PRIORITIES] = {NULL, NULL, NULL, NULL, NULL};
PCB* gp_priority_begin[NUM_PRIORITIES] = {NULL, NULL, NULL, NULL, NULL};
PCB* gp_sys_procs_begin = NULL;
PCB* gp_sys_procs_end = NULL;

extern uint32_t g_timer_count;

/**
 * @brief: initialize all processes in the system
 * NOTE: We assume there are only two user processes in the system in this example.
 */
void process_init() 
{
	int i;
	U32 *sp;
  
        /* fill out the initialization table */
	set_test_procs();
	set_user_procs();
	set_sys_procs();
	set_irq_procs();
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[i].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
		g_proc_table[i].m_priority = g_test_procs[i].m_priority;
	}
	for (i = 0; i < NUM_USER_PROCS; i++ ) {
		g_proc_table[NUM_TEST_PROCS + i].m_pid = g_user_procs[i].m_pid;
		g_proc_table[NUM_TEST_PROCS + i].m_stack_size = g_user_procs[i].m_stack_size;
		g_proc_table[NUM_TEST_PROCS + i].mpf_start_pc = g_user_procs[i].mpf_start_pc;
		g_proc_table[NUM_TEST_PROCS + i].m_priority = g_user_procs[i].m_priority;
	}
	for (i = 0; i < NUM_SYS_PROCS; i++ ) {
		g_proc_table[NUM_TEST_PROCS + NUM_USER_PROCS + i].m_pid = g_sys_procs[i].m_pid;
		g_proc_table[NUM_TEST_PROCS + NUM_USER_PROCS + i].m_stack_size = g_sys_procs[i].m_stack_size;
		g_proc_table[NUM_TEST_PROCS + NUM_USER_PROCS + i].mpf_start_pc = g_sys_procs[i].mpf_start_pc;
		g_proc_table[NUM_TEST_PROCS + NUM_USER_PROCS + i].m_priority = g_sys_procs[i].m_priority;
	}
	for (i = 0; i < NUM_IRQ_PROCS; i++ ) {
		g_proc_table[NUM_TEST_PROCS + NUM_USER_PROCS + NUM_SYS_PROCS + i].m_pid = g_irq_procs[i].m_pid;
		g_proc_table[NUM_TEST_PROCS + NUM_USER_PROCS + NUM_SYS_PROCS + i].m_stack_size = g_irq_procs[i].m_stack_size;
		g_proc_table[NUM_TEST_PROCS + NUM_USER_PROCS + NUM_SYS_PROCS + i].mpf_start_pc = g_irq_procs[i].mpf_start_pc;
		g_proc_table[NUM_TEST_PROCS + NUM_USER_PROCS + NUM_SYS_PROCS + i].m_priority = g_irq_procs[i].m_priority;
	}
	
  
	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( i = 0; i < NUM_TEST_PROCS + NUM_SYS_PROCS + NUM_IRQ_PROCS + NUM_USER_PROCS; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
		(gp_pcbs[i])->m_state = NEW;
		(gp_pcbs[i])->mp_first_msg = NULL;
		(gp_pcbs[i])->mp_last_msg = NULL;
		
		sp = alloc_stack((g_proc_table[i]).m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		(gp_pcbs[i])->mp_sp = sp;
		
		// set priority and put into correct list
		(gp_pcbs[i])->mp_next = NULL;
		if ((gp_pcbs[i])->m_priority == 5) {
			if (gp_sys_procs_end != NULL) {
				gp_sys_procs_end->mp_next = (gp_pcbs[i]);
			} else {
				gp_sys_procs_begin = (gp_pcbs[i]);
			}
			gp_sys_procs_end = (gp_pcbs[i]);
		} else if((gp_pcbs[i])->m_priority <= 4){
			if (gp_priority_end[(gp_pcbs[i])->m_priority] != NULL) {
				gp_priority_end[(gp_pcbs[i])->m_priority]->mp_next = (gp_pcbs[i]);
			} else {
				gp_priority_begin[(gp_pcbs[i])->m_priority] = (gp_pcbs[i]);
			}
			gp_priority_end[(gp_pcbs[i])->m_priority] = (gp_pcbs[i]);
		}
	}
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */

PCB *scheduler(void)
{
	PCB* cur_pcb = NULL;
	PCB* prev_pcb = NULL;
	int i;
	// go through priorities in order and find the first unblocked process
	
	cur_pcb = gp_sys_procs_begin;
	while(cur_pcb != NULL){
		if(cur_pcb->m_state != BLOCKED && cur_pcb->m_state != BLOCKED_ON_RECEIVE){
			if(prev_pcb != NULL){
				prev_pcb->mp_next = cur_pcb->mp_next;
				if (prev_pcb->mp_next == NULL) {
					gp_sys_procs_end = prev_pcb;
				}
			}else{
				gp_sys_procs_begin = cur_pcb->mp_next;
				if(gp_sys_procs_begin == NULL){
					gp_sys_procs_end = NULL;
				}
			}
			cur_pcb->mp_next = NULL;
			
			if(gp_sys_procs_end != NULL){
				gp_sys_procs_end->mp_next = cur_pcb;
			}else{
				gp_sys_procs_begin = cur_pcb;
			}
			gp_sys_procs_end = cur_pcb;

			return cur_pcb;
		}
		
		prev_pcb = cur_pcb;
		cur_pcb = cur_pcb->mp_next;
	}
	
	for(i=0;i<NUM_PRIORITIES;++i){
		cur_pcb = gp_priority_begin[i];
		prev_pcb = NULL;
		// find first unblocked process for priority i
		while(cur_pcb != NULL){
			if(cur_pcb->m_state != BLOCKED && cur_pcb->m_state != BLOCKED_ON_RECEIVE){
				if(prev_pcb != NULL){
					prev_pcb->mp_next = cur_pcb->mp_next;
					if (prev_pcb->mp_next == NULL) {
						gp_priority_end[i] = prev_pcb;
					}
				}else{
					gp_priority_begin[i] = cur_pcb->mp_next;
					if(gp_priority_begin[i] == NULL){
						gp_priority_end[i] = NULL;
					}
				}
				cur_pcb->mp_next = NULL;
				
				if(gp_priority_end[i] != NULL){
					gp_priority_end[i]->mp_next = cur_pcb;
				}else{
					gp_priority_begin[i] = cur_pcb;
				}
				gp_priority_end[i] = cur_pcb;

				return cur_pcb;
			}
			
			prev_pcb = cur_pcb;
			cur_pcb = cur_pcb->mp_next;
		}
	}
	return NULL;	
}

/*@brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 *@param: p_pcb_old, the old pcb that was in RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
int process_switch(PCB *p_pcb_old) 
{
	PROC_STATE_E state;
	
	state = gp_current_process->m_state;

	if (state == NEW) {
		if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
			if (p_pcb_old->m_state != BLOCKED && p_pcb_old->m_state != BLOCKED_ON_RECEIVE){
				p_pcb_old->m_state = RDY;
			}
			p_pcb_old->mp_sp = (U32 *) __get_MSP();
		}
		gp_current_process->m_state = RUN;
		__set_MSP((U32) gp_current_process->mp_sp);
		__rte();  // pop exception stack frame from the stack for a new processes
	} 
	
	/* The following will only execute if the if block above is FALSE */

	if (gp_current_process != p_pcb_old) {
		if (state == RDY){ 		
			if (p_pcb_old->m_state != BLOCKED && p_pcb_old->m_state != BLOCKED_ON_RECEIVE){
				p_pcb_old->m_state = RDY;
			}
			p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
			gp_current_process->m_state = RUN;
			__set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack
		} else {
			gp_current_process = p_pcb_old; // revert back to the old proc on error
			return RTX_ERR;
		}
	}
	return RTX_OK;
}
/**
 * @brief release_processor(). 
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void)
{
	PCB *p_pcb_old = NULL;
	
	p_pcb_old = gp_current_process;
	gp_current_process = scheduler();
	
	if ( gp_current_process == NULL  ) {
		gp_current_process = p_pcb_old; // revert back to the old process
		return RTX_ERR;
	}
  if ( p_pcb_old == NULL ) {
		p_pcb_old = gp_current_process;
	}
	
	process_switch(p_pcb_old);
	return RTX_OK;
}

/**
 * @brief set_process_priority(). 
 * @param process_id, the pid for the process to set the priority
 * 				priority, the priority to set for the process
 * @return RTX_ERR on error and zero on success
 * POST: process with pricess_id gets set to the specified priority and moved to the correct queue
 */
int k_set_process_priority (int process_id, int priority)
{
	int i = 0;
	PCB* cur_pcb = NULL;
	PCB* prev_pcb = NULL;
	if((process_id == PID_NULL && priority != 4)
			|| (process_id != PID_NULL && priority == 4) 
			|| PID_KCD == process_id
			|| PID_CRT == process_id
			|| PID_TIMER_IPROC == process_id
			|| PID_UART_IPROC == process_id) {
		return RTX_ERR;
	}
	for ( i = 0; i < NUM_PRIORITIES; ++i) {
		cur_pcb = gp_priority_begin[i];
		prev_pcb = NULL;
		while (cur_pcb != NULL) {
			if (cur_pcb->m_pid == process_id) {
				if(cur_pcb->m_priority == priority) {
					return RTX_OK;
				}
				cur_pcb->m_priority = priority;
				if(prev_pcb != NULL){
					prev_pcb->mp_next = cur_pcb->mp_next;
					if (gp_priority_end[i] == cur_pcb) {
						gp_priority_end[i] = prev_pcb;
					}
				}else{
					gp_priority_begin[i] = cur_pcb->mp_next;
					if(gp_priority_begin[i] == NULL){
						gp_priority_end[i] = NULL;
					}
				}
				cur_pcb->mp_next = NULL;
				
				if(gp_priority_end[priority] != NULL){
					gp_priority_end[priority]->mp_next = cur_pcb;
				}else{
					gp_priority_begin[priority] = cur_pcb;
				}
				gp_priority_end[priority] = cur_pcb;
				
				if ((priority < gp_current_process->m_priority || gp_current_process->m_pid == cur_pcb->m_pid) && cur_pcb->m_state != BLOCKED && cur_pcb->m_state != BLOCKED_ON_RECEIVE) {
					k_release_processor();
				}
				return RTX_OK;
			}
			prev_pcb = cur_pcb;
			cur_pcb = cur_pcb->mp_next;
		}
	}
	return RTX_ERR;
}

/**
 * @brief get_process_priority(). 
 * @param process_id, the pid for the process to get the priority
 * @return RTX_ERR on error and the priority of the process on success
 */
int k_get_process_priority (int process_id)
{
	int i = 0;
	PCB* cur_pcb = NULL;
	for ( i = 0; i < NUM_PRIORITIES; ++i) {
		cur_pcb = gp_priority_begin[i];
		while (cur_pcb != NULL) {
			if (cur_pcb->m_pid == process_id) {
				return cur_pcb->m_priority;
			}
			cur_pcb = cur_pcb->mp_next;
		}
	}
	return RTX_ERR;
}

/*int k_get_current_process_id(){
	if(gp_current_process == NULL){
		return -1;
	}
	return gp_current_process->m_pid;
}*/

PCB* get_PCB (int process_id)
{
	int i = 0;
	PCB* cur_pcb = NULL;
	for ( i = 0; i < NUM_PRIORITIES; ++i) {
		cur_pcb = gp_priority_begin[i];
		while (cur_pcb != NULL) {
			if (cur_pcb->m_pid == process_id) {
				return cur_pcb;
			}
			cur_pcb = cur_pcb->mp_next;
		}
	}
	return NULL;
}

int k_send_message(int process_id, void *message_envelope) {
	//atomic(on) ???
	PCB *receiving = NULL;
	MSG_BUF * new_node = (MSG_BUF*) message_envelope;
	int i;
	new_node->m_recv_pid = process_id;
	new_node->m_send_pid = gp_current_process->m_pid;
	for ( i = 0; i < NUM_TEST_PROCS + NUM_SYS_PROCS + NUM_IRQ_PROCS + NUM_USER_PROCS; i++ ) {
		if ((gp_pcbs[i])->m_pid == process_id) {
			receiving = gp_pcbs[i];
		}
	}
	if (receiving != NULL) {
		new_node->mp_next=NULL;
		if (NULL == receiving->mp_first_msg) {
			receiving->mp_first_msg = new_node;
			receiving->mp_last_msg = new_node;
		}else{
			receiving->mp_last_msg->mp_next = new_node;
			receiving->mp_last_msg = new_node;
		}
		if (BLOCKED_ON_RECEIVE == receiving->m_state) {
			receiving->m_state = RDY;
			
// 			if ((receiving->m_pid != PID_KCD && receiving->m_pid != PID_CRT && gp_current_process->m_priority >= receiving->m_priority) 
// 						|| receiving->m_pid == PID_KCD || receiving->m_pid == PID_CRT){
// 				k_release_processor();
// 			}
			//PUT INTO receiving process queue???
		}
		return RTX_OK;
	}
	//atomic(off) ???
	return RTX_ERR;
}

void* k_receive_message(int *sender_id) {
	void* temp;
	//atomic(on) ???
	while (gp_current_process->mp_first_msg == NULL) {
		gp_current_process->m_state = BLOCKED_ON_RECEIVE;
		k_release_processor();
	}
	temp = (void*) gp_current_process->mp_first_msg;
	*sender_id = gp_current_process->mp_first_msg->m_send_pid;
	gp_current_process->mp_first_msg = gp_current_process->mp_first_msg->mp_next;
	if (NULL == gp_current_process->mp_first_msg) {
		gp_current_process->mp_last_msg = NULL;
	}
	//atomic(off) ???
	return temp;
}

int k_delayed_send(int process_id, void *message_envelope, int delay) {
	int sendTime = delay + g_timer_count;
	
	MSG_BUF* cur = gp_delayed_msgs;
	MSG_BUF* prev = NULL;
	MSG_BUF* new_msg = (MSG_BUF*) message_envelope;
	
	new_msg->m_send_time = sendTime;
	
	new_msg->m_recv_pid = process_id;
	new_msg->m_send_pid = gp_current_process->m_pid;
	
	while(cur != NULL && cur->m_send_time < sendTime ){
		prev = cur;
		cur = cur->mp_next;
	}
	
	if(prev == NULL){
		new_msg->mp_next = gp_delayed_msgs;
		gp_delayed_msgs = new_msg;
	}
	else{
		prev->mp_next = new_msg;
		new_msg->mp_next = cur;
	}
	
	return RTX_OK;
}

void queue_debug_statement(PROC_STATE_E state) {
	int i = 0;
	PCB* cur_pcb = NULL;
	printf("State ");
	if(state == RDY) {
		printf("READY");
	} else if(state == BLOCKED) {
		printf("BLOCKED");
	} else if(state == BLOCKED_ON_RECEIVE) {
		printf("BLOCKED OF RECEIVE");
	} else {
		printf("UNKNOWN");
	}
	printf(":\n\r");
	for ( i = 0; i < NUM_PRIORITIES; ++i) {
		printf("    Priority %d: ", i);
		cur_pcb = gp_priority_begin[i];
		while (cur_pcb != NULL) {
			if (cur_pcb->m_state == state /*|| (state == RDY && cur_pcb->m_state == NEW)*/) {
				#ifdef DEBUG_0
				printf(" %u", cur_pcb->m_pid );
				#endif
			}
			cur_pcb = cur_pcb->mp_next;
		}
		printf("\n\r");
	}
}
