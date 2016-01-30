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

extern unsigned int MEM_BLK_SIZE = 128;

typedef struct mem_blk
{
	U32* next_blk;
} mem_blk;

extern mem_blk* p_heap_head = 0;

void memory_init(void)
{
	mem_blk* temp;
	U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	int i;
  
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += NUM_TEST_PROCS * sizeof(PCB *);
  
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
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
  
	printf("memory_init: heap init starts at 0x%x\n", p_end
	);
	p_heap_head = (mem_blk*)p_end;
	temp = (mem_blk*)p_end;
	printf("k_mem_init: ram limit at 0x%x\n", gp_stack);
	while( temp + MEM_BLK_SIZE < (mem_blk*)gp_stack){
		temp->next_blk = (U32*)((unsigned int)temp + MEM_BLK_SIZE); //doesn't seem to work
		printf("k_mem_init: next block at 0x%x\n", (void *)((*temp).next_blk));
		temp = (mem_blk*)((*temp).next_blk);
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
#ifdef DEBUG_0 
	printf("k_request_memory_block: entering...\n");
#endif /* ! DEBUG_0 */
	return (void *) NULL;
}

int k_release_memory_block(void *p_mem_blk) {
#ifdef DEBUG_0 
	printf("k_release_memory_block: releasing block @ 0x%x\n", p_mem_blk);
#endif /* ! DEBUG_0 */
	return RTX_OK;
}
