/**
 * @file:   usr_proc.c
 * @brief:  Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date:   2014/01/17
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

int PID_P4 = 4;
int PID_P5 = 5;
void* used_mem_blocks[1000];
int curr_index = 0;	

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}
  
	g_test_procs[0].mpf_start_pc = &proc6;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc7;
}

void nullproc(void)
{
	while (1) {
		k_release_processor();
	}
}

void testFunction(void){
	uart0_put_string("G008_test: START\n\r");
	uart0_put_string("G008_test: total x tests\n\r");
	
	uart0_put_string("G008_test: ");
	uart0_put_char("x");
	uart0_put_char("/");
	uart0_put_char("N");
	uart0_put_char(" tests OK\n\r");
	
	uart0_put_string("G008_test: ");
	uart0_put_char("y");
	uart0_put_char("/");
	uart0_put_char("N");
	uart0_put_char(" tests FAIL\n\r");
	
	uart0_put_string("G008_test: END\n\r");
}

void proc1(void) {
	while(1) {
		#ifdef DEBUG_0
			printf("Requesting block of memory - this is proc1\n\r");
		#endif
		request_memory_block();
		release_processor();
	}
}

void proc2(void) {
	while(1) {
		
		#ifdef DEBUG_0
			printf("This is process 2\n");
		#endif
		release_processor();
	}
}

/**
 * @brief: a process that requests 3 memory blocks and prints their addresses 
 *         and then releases the 3 processes and releases the processor
 * Used to test the function of the k_request_memory_block and k_release_memory_block.
 */
void proc3(void) {
 	void* p_mem_blk1 ;
 	void* p_mem_blk2 ;
 	void* p_mem_blk3 ;
 	while ( 1) {
 		p_mem_blk1 = request_memory_block();
 		uart0_put_string("Successfully obtained first memory block at address: ");
		uart0_put_char('0' + (unsigned int) p_mem_blk1);
		uart0_put_string("\n\r");
 		p_mem_blk2 = request_memory_block();
 		uart0_put_string("Successfully obtained second memory block at address: ");
		uart0_put_char('0' + (unsigned int) p_mem_blk2);
		uart0_put_string("\n\r");
 		p_mem_blk3 = request_memory_block();
 		uart0_put_string("Successfully obtained third memory block at address: ");
		uart0_put_char('0' + (unsigned int) p_mem_blk3);
		uart0_put_string("\n\r");
 		release_memory_block(p_mem_blk1);
 		release_memory_block(p_mem_blk2);
 		release_memory_block(p_mem_blk3);
 		release_processor();
 	}
}


/**
 * @brief: a process that sets its own priority to HIGH and prints all 26 letters before releasing processor.
 * Process will keep repeating if there are no other HIGH priority processes.
 * Used to test the function of the priority queue.
 */
void proc4(void)
{
	int i = 0;
	k_set_process_priority(k_get_current_process_id(), HIGH);
	while ( 1) {
		if ( i != 0 && i%26 == 0 ) {
			uart0_put_string("\n\r");
			release_processor();
		}
		uart0_put_char('A' + i%26);
		i++;
	}
}


/**
 * @brief: a process that alternates its priority between 0 and 3.
 */
void proc5(void)
{
	int cur_priority = 0;
	while ( 1) {
		cur_priority = k_get_process_priority(k_get_current_process_id());
		if(cur_priority != HIGH){
			uart0_put_string("Changing this processes priority from ");
			uart0_put_char('0'+cur_priority);
			uart0_put_string(" to 0\n\r");
			k_set_process_priority(k_get_current_process_id(), HIGH);
			release_processor();
		}else{
			uart0_put_string("Changing this processes priority from 0 to 3\n\r");
			k_set_process_priority(k_get_current_process_id(), LOWEST);
			release_processor();
		}
	}
}

/**
 * @brief: a process that repeatedly requests memory without releasing the processor.
 *         This process will be blocked when there is no memory remaining
 */
void proc6(void)
{
	while (1) {
		used_mem_blocks[curr_index] = request_memory_block();
		printf("Im proc6 and i got : 0x%x\r\n",  used_mem_blocks[curr_index]);
		curr_index++;
		//uart0_put_string("Obtained memory at address: ")+ used_mem_blocks[curr_index] + "\n\r");
	}
}

/**
 * @brief: a process that releases the 5 most recent blocks of memory saved by process 6.
 * This should unblock process 6.
 */
void proc7(void)
{
	int i=0;
	while(1){
		if ( i != 0 && i%5 == 0 ) {
			i = 0;
			release_processor();
		}
		if(curr_index <=0){
			curr_index = 0;
			release_processor();
		}
		k_release_memory_block(used_mem_blocks[curr_index - 1]);
//		uart0_put_string("Released memory at address: "+ used_mem_blocks[curr_index] + "\n\r");
		curr_index--;
		i++;
	}
}
