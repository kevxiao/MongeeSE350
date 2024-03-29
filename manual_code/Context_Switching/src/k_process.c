/**
 * @file:   k_process.c  
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
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

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs;                  /* array of pointers to pcbs */
PCB *gp_current_process = NULL; /* always point to the current RUN process */

/* process initialization table */
PROC_INIT g_proc_table[NUM_TEST_PROCS + 1];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS + 1];

PCB* gp_priority_end[NUM_PRIORITIES] = {NULL, NULL, NULL, NULL, NULL};
PCB* gp_priority_begin[NUM_PRIORITIES] = {NULL, NULL, NULL, NULL, NULL};

/**
 * @brief: the null process
 */
void nullproc(void)
{
	while (1) {
		k_release_processor();
	}
}

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
	g_proc_table[NUM_TEST_PROCS].m_pid = 0;
	g_proc_table[NUM_TEST_PROCS].m_stack_size = 0x100;
	g_proc_table[NUM_TEST_PROCS].mpf_start_pc = &nullproc;
	g_proc_table[NUM_TEST_PROCS].m_priority = 4;
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[i].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
		g_proc_table[i].m_priority = g_test_procs[i].m_priority;
	}
  
	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( i = 0; i < NUM_TEST_PROCS + 1; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
		(gp_pcbs[i])->m_state = NEW;
		
		sp = alloc_stack((g_proc_table[i]).m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		(gp_pcbs[i])->mp_sp = sp;
		
		// set priority and put into correct list
		(gp_pcbs[i])->mp_next = NULL;
		if (gp_priority_end[(gp_pcbs[i])->m_priority] != NULL) {
			gp_priority_end[(gp_pcbs[i])->m_priority]->mp_next = (gp_pcbs[i]);
		} else {
			gp_priority_begin[(gp_pcbs[i])->m_priority] = (gp_pcbs[i]);
		}
		gp_priority_end[(gp_pcbs[i])->m_priority] = (gp_pcbs[i]);
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
	for(i=0;i<NUM_PRIORITIES;++i){
		cur_pcb = gp_priority_begin[i];
		prev_pcb = NULL;
		// find first unblocked process for priority i
		while(cur_pcb != NULL){
			if(cur_pcb->m_state != BLOCKED){
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
			if (p_pcb_old->m_state != BLOCKED){
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
			if (p_pcb_old->m_state != BLOCKED){
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
	if((process_id == 0 && priority != 4)|| (process_id != 0 && priority == 4)) {
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
				
				if ((priority < gp_current_process->m_priority || gp_current_process->m_pid == cur_pcb->m_pid) && cur_pcb->m_state != BLOCKED) {
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
