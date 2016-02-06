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
int curr_index = -1;	

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
}

void nullproc(void)
{
	while (1) {
		k_release_processor();
	}
}

void process1(void){
	
}

void proc1(void) {
	while(1) {
		#ifdef DEBUG_0
			printf("Requesting block of memory - this is proc1\n");
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
	int i = 0;
 	int ret_val;
 	void* p_mem_blk1 ;
 	void* p_mem_blk2 ;
 	void* p_mem_blk3 ;
 	while ( 1) {
 		p_mem_blk1 = k_request_memory_block();
 		uart0_put_string("Successfully obtained first memory at address: "+ p_mem_blk1 + "\n\r");
 		p_mem_blk2 = k_request_memory_block();
 		uart0_put_string("Successfully obtained second memory at address: "+ p_mem_blk2 + "\n\r");
 		p_mem_blk3 = k_request_memory_block();
 		uart0_put_string("Successfully obtained third memory at address: "+ p_mem_blk3 + "\n\r");
 		k_release_memory_block(p_mem_blk1);
 		k_release_memory_block(p_mem_blk2);
 		k_release_memory_block(p_mem_blk3);
 		ret_val = release_processor();
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
	set_process_priority(PID_P4, HIGH);
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
	int i = 0;
	int cur_priority = 0;
	while ( 1) {
		cur_priority = k_get_process_priority(PID_P5);
		if(cur_priority != 0){
			uart0_put_string("Changing this processes priority from");
			uart0_put_char('0'+cur_priority);
			uart0_put_string(" to 0\n\r");
			set_process_priority(PID_P5, HIGH);
			release_processor();
		}else{
			uart0_put_string("Changing this processes priority from 0 to 3\n\r");
			set_process_priority(PID_P5, LOWEST);
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
		curr_index++;
		used_mem_blocks[curr_index] = request_memory_block();
		uart0_put_string("Obtained memory at address: "+ used_mem_blocks[curr_index] + "\n\r");
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
			release_processor();
		}
		if(curr_index <0){
			curr_index = -1;
			release_processor();
		}
		k_release_memory_block(used_mem_blocks[curr_index]);
		uart0_put_string("Released memory at address: "+ used_mem_blocks[curr_index] + "\n\r");
		curr_index--;
		i++;
	}
}
	

/**
 * @brief: a process that prints five uppercase letters
 *         and then yields the cpu.
 */
// void proc1(void)
// {
// 	int i = 0;
// 	int ret_val;
// 	void* temp ;
// 	while ( 1) {
// 		temp = k_request_memory_block();
// 		if (NULL == temp) {
// 			printf("im proc 1 and i got some null going on here\n\r");
// 		}
// 		else {
// 			printf("im proc 1 and i got mem at 0x%x\n\r", temp);
// 		}
// 		if ( i != 0 && i%5 == 0 ) {
// 			uart0_put_string("\n\r");
// 			ret_val = release_processor();
// #ifdef DEBUG_0
// 			printf("proc1: ret_val=%d\n", ret_val);
// #endif /* DEBUG_0 */
// 		}
// 		uart0_put_char('A' + i%26);
// 		i++;
// 	}
// }

// /**
//  * @brief: a process that prints five numbers
//  *         and then yields the cpu.
//  */
// void proc2(void)
// {
// 	int i = 0;
// 	int ret_val = 20;
// 	while ( 1) {
// 		if ( i != 0 && i%5 == 0 ) {
// 			uart0_put_string("\n\r");
// 			ret_val = release_processor();
// #ifdef DEBUG_0
// 			printf("proc2: ret_val=%d\n", ret_val);
// #endif /* DEBUG_0 */
// 		}
// 		uart0_put_char('0' + i%10);
// 		i++;
// 	}
// }
