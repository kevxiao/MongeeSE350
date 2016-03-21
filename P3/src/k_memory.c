/**
 * @file:   k_memory.c
 * @brief:  kernel memory managment routines
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_memory.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

/* ----- Global Variables ----- */
U32 *gp_stack; /* The last allocated stack low address. 8 bytes aligned */
               /* The first stack starts at the RAM high address */
	       /* stack grows down. Fully decremental stack */

/**
 * @brief: Initialize RAM as follows:

0x10008000+---------------------------+ High Address
          |    Proc 1 STACK           |
          |---------------------------|
          |    Proc 2 STACK           |
          |---------------------------|<--- gp_stack
          |                           |
          |        HEAP               |
          |                           |
          |---------------------------|
          |        PCB 2              |
          |---------------------------|
          |        PCB 1              |
          |---------------------------|
          |        PCB pointers       |
          |---------------------------|<--- gp_pcbs
          |        Padding            |
          |---------------------------|  
          |Image$$RW_IRAM1$$ZI$$Limit |
          |...........................|          
          |       RTX  Image          |
          |                           |
0x10000000+---------------------------+ Low Address

*/

unsigned int MEM_BLK_SIZE = 128;

extern PCB* gp_current_process;
U8 * p_end;

typedef struct mem_blk
{
	U32* next_blk;
} mem_blk;

extern mem_blk* p_heap_head = NULL;
int numBlocks = 0;

void memory_init(void)
{
	int i;
	p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
  
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += (NUM_TEST_PROCS + NUM_SYS_PROCS + NUM_IRQ_PROCS + NUM_USER_PROCS) * sizeof(PCB *);
  
	for ( i = 0; i < NUM_TEST_PROCS + NUM_SYS_PROCS + NUM_IRQ_PROCS + NUM_USER_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		p_end += sizeof(PCB); 
	}
#ifdef DEBUG_0  
	printf("gp_pcbs[0] = 0x%x \n", gp_pcbs[0]);
	printf("gp_pcbs[1] = 0x%x \n", gp_pcbs[1]);
#endif
	
	/* prepare for alloc_stack() to allocate memory for stacks */
	
	gp_stack = (U32 *)RAM_END_ADDR;
	if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
		--gp_stack; 
	}
  
	/* allocate memory for heap, not implemented yet*/
  
}

void heap_init(void) {
	mem_blk* temp;
  #ifdef DEBUG_0
		printf("memory_init: heap init starts at 0x%x\n\r", p_end);
	#endif
	p_heap_head = (mem_blk*)p_end;
	temp = (mem_blk*)p_end;
	#ifdef DEBUG_0
		printf("k_mem_init: ram limit at 0x%x\n\r", gp_stack);
	#endif
	while( (unsigned int)temp + MEM_BLK_SIZE < /*(unsigned int)p_end + 128 * 10*/(unsigned int)gp_stack - MEM_BLK_SIZE){
		temp->next_blk = (U32*)((unsigned int)temp + MEM_BLK_SIZE);
		#ifdef DEBUG_0
			printf("k_mem_init: block at 0x%x\n\r", (void*)temp);
		#endif
		temp = (mem_blk*)((*temp).next_blk);
		numBlocks++;
	}
	(*temp).next_blk = NULL;
}

/**
 * @brief: allocate stack for a process, align to 8 bytes boundary
 * @param: size, stack size in bytes
 * @return: The top of the stack (i.e. high address)
 * POST:  gp_stack is updated.
 */

U32 *alloc_stack(U32 size_b) 
{
	U32 *sp;
	sp = gp_stack; /* gp_stack is always 8 bytes aligned */
	
	/* update gp_stack */
	gp_stack = (U32 *)((U8 *)sp - size_b);
	
	/* 8 bytes alignement adjustment to exception stack frame */
	if ((U32)gp_stack & 0x04) {
		--gp_stack; 
	}
	return sp;
}

void *k_request_memory_block(void) {
	//int i = 0;
	mem_blk* temp = p_heap_head;
#ifdef DEBUG_0 
	printf("k_request_memory_block: Proc %d requesting block @ 0x%x\n\r", gp_current_process->m_pid, p_heap_head);
#endif /* ! DEBUG_0 */
	while (NULL == p_heap_head) {
		#ifdef DEBUG_0
		printf("k_request_memory_block: no moar memory\n\r");
		#endif /* ! DEBUG_0 */
		if (gp_current_process->m_pid == PID_UART_IPROC || gp_current_process->m_pid == PID_TIMER_IPROC /*|| gp_current_process->m_pid == PID_KCD*/){
			return NULL;
		}
		k_change_process_state(gp_current_process, BLOCKED);
	}
// 	while(blocks_in_use[i] != NULL && i < numBlocks){
// 		if (i >= numBlocks){
// 			printf("MAJOR ERROR\n\r");
// 		}
// 		i++;
// 	}
// 	blocks_in_use[i] = temp;
	temp = p_heap_head;
	p_heap_head = (mem_blk*) p_heap_head->next_blk;
	return (void*) temp ;
}

int k_release_memory_block(void *p_mem_blk) {
	mem_blk* temp = (mem_blk*) p_mem_blk;
	int i;
	//int found = 0;
	PCB* cur_pcb = NULL;
// 	if (p_mem_blk == NULL){
// 		return RTX_ERR;
// 	}
// 	for (i=0; i < numBlocks; i++){
// 		if (blocks_in_use[i] == temp){
// 			blocks_in_use[i] = NULL;
// 			found++;
// 			break;
// 		}
// 	}
// 	if(found == 0){
// 		return RTX_ERR;
// 	}
#ifdef DEBUG_0 
	printf("k_release_memory_block: releasing block @ 0x%x\n\r", p_mem_blk);
#endif /* ! DEBUG_0 */
	temp->next_blk = (U32*) p_heap_head;
	p_heap_head = temp;
	for (i=0; i < NUM_PRIORITIES; i++){
		cur_pcb = gp_blocked_priority_begin[i];
		if(cur_pcb != NULL){
			k_change_process_state(cur_pcb, RDY);
			break;
		}
	}
	return RTX_OK;
}
