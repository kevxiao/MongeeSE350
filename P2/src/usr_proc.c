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

void* used_mem_blocks[1000];
int curr_index = 0;
int memoryAllocated = 0;
int proc1_count = 0;
int shouldNotRun = 0;
int proc1Prio = -1;

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[3].mpf_start_pc = &proc4;
	g_test_procs[4].mpf_start_pc = &proc5;
	g_test_procs[5].mpf_start_pc = &proc6;
	g_test_procs[5].m_priority = HIGH;
}

void proc6(void){
	int passed = 0;
	int failed = 0;
	int total = 0;
	int locMemoryAllocated;
	int locProc1Run;
	int locShouldNotRun;
	int locProc1Prio;
	
	uart0_put_string("G008_test: START\n\r");
	uart0_put_string("G008_test: total x tests\n\r");
	
	set_process_priority(1, LOW);
	set_process_priority(2, LOW);
	set_process_priority(3, LOW);
	set_process_priority(4, LOW);
	set_process_priority(5, LOW);
	
	//TEST 1
	set_process_priority(4, MEDIUM);
	set_process_priority(5, MEDIUM);
	set_process_priority(6, LOWEST);
	release_processor();
	
	locMemoryAllocated = memoryAllocated;
	
	set_process_priority(4, LOW);
	set_process_priority(5, LOW);
	
	//TEST 2
	set_process_priority(1, MEDIUM);
	set_process_priority(2, MEDIUM);
	set_process_priority(6, LOWEST);
	release_processor();
	
	locProc1Run = proc1_count;
	
	set_process_priority(1, LOW);
	set_process_priority(2, LOW);
	
	//TEST 3
	set_process_priority(3, MEDIUM);
	set_process_priority(1, MEDIUM);
	set_process_priority(6, LOWEST);
	release_processor();
	
	locShouldNotRun = shouldNotRun;
	
	set_process_priority(1, LOW);
	set_process_priority(3, LOW);
	
	//TEST 4
	proc1Prio = -1;
	set_process_priority(4, HIGH);
	set_process_priority(1, MEDIUM);
	set_process_priority(6, LOWEST);
	release_processor();
	
	locProc1Prio = proc1Prio;
	
	set_process_priority(1, LOW);
	set_process_priority(4, LOW);
	
	if (locMemoryAllocated == 15) {
		passed++;
		uart0_put_string("G008_test: test 1 OK\n\r");
	}	
	else {
		failed++;
		uart0_put_string("G008_test: test 1 FAIL\n\r");
	}
	total++;
	
	if (locProc1Run == 1) {
		passed++;
		uart0_put_string("G008_test: test 2 OK\n\r");
	}	
	else {
		failed++;
		uart0_put_string("G008_test: test 2 FAIL\n\r");
	}
	total++;
	
	if (locShouldNotRun == 0) {
		passed++;
		uart0_put_string("G008_test: test 3 OK\n\r");
	}	
	else {
		failed++;
		uart0_put_string("G008_test: test 3 FAIL\n\r");
	}
	total++;
	if (locProc1Prio == MEDIUM) {
		passed++;
		uart0_put_string("G008_test: test 4 OK\n\r");
	}	
	else {
		failed++;
		uart0_put_string("G008_test: test 4 FAIL\n\r");
	}
	total++;
	
	uart0_put_string("G008_test: ");
	uart0_put_char('0'+passed);
	uart0_put_char('/');
	uart0_put_char('0'+total);
	uart0_put_string(" tests OK\n\r");
	
	uart0_put_string("G008_test: ");
	uart0_put_char('0'+failed);
	uart0_put_char('/');
	uart0_put_char('0'+total);
	uart0_put_string(" tests FAIL\n\r");
	
	uart0_put_string("G008_test: END\n\r");
	while (1) {
		release_processor();
	}
}

void proc1(void) {
	while(1) {
		proc1Prio = get_process_priority(1);
		#ifdef DEBUG_0
			printf("This is process 1\n\r");
		#endif
		proc1_count++;
		if (proc1_count > 30) {
			proc1_count = 0;
			set_process_priority(6, HIGH);
		}
		release_processor();
	}
}

/**
 * @brief: a process that sets its own priority to HIGH and prints all 26 letters before releasing processor.
 * Process will keep repeating if there are no other HIGH priority processes.
 * Used to test the function of the priority queue.
 */
void proc2(void)
{
	int i = 0;
	set_process_priority(2, HIGH);
	while ( 1) {
		if ( i != 0 && i%26 == 0 ) {
			set_process_priority(6, HIGH);
			release_processor();
		}
		i++;
		release_processor();
	}
}


/**
 * @brief: a process that alternates its priority between 0 and 3.
 */
void proc3(void)
{
	while ( 1) {
		set_process_priority(1, HIGH);
		shouldNotRun = 1;
	}
}

/**
 * @brief: a process that repeatedly requests memory without releasing the processor.
 *         This process will be blocked when there is no memory remaining
 */
void proc4(void)
{
	while (1) {
		used_mem_blocks[curr_index] = request_memory_block();
		#ifdef DEBUG_0
			printf("Im proc4 and i got : 0x%x\r\n",  used_mem_blocks[curr_index]);
		#endif
		curr_index++;
		memoryAllocated++;
	}
}

/**
 * @brief: a process that releases the 5 most recent blocks of memory saved by process 6.
 * This should unblock process 6.
 */
void proc5(void)
{
	int i=0;
	while(1){
		if (i == 0) {
			memoryAllocated = 0;		
		}
		if ( i != 0 && i%5 == 0 ) {
			release_processor();
		}
		if(curr_index <=0){
			curr_index = 0;
			release_processor();
		}
		curr_index--;
		release_memory_block(used_mem_blocks[curr_index]);
		i++;
		if (i == 15) {
			set_process_priority(4, LOW);
			while (curr_index > 0) {
				release_memory_block(used_mem_blocks[curr_index - 1]);
				curr_index--;
			}
			set_process_priority(6, HIGH);
		}
	}
}

/**
 * @brief: a process that sends a message to proc8.
 */
void proc7(void)
{
	MSG_BUF* message = request_memory_block();
	char text[15] = "This is a test";
	int i = 0;
	while ( 1) {
		while(text[i]!='\0'){
			message->mtext[i] = text[i];
			i++;
		}
		message->mtype = DEFAULT;
		send_message(PID_P4 ,message);
	}
}

/**
 * @brief: a process that receives a message from proc7.
 */
void proc8(void)
{
	int i=0;
	int* sender_id;
	MSG_BUF* message = receive_message(sender_id);
	while ( 1) {
		#ifdef DEBUG_0
			printf("Received the following message from process %d:\n\r", *sender_id);
			while(message->mtext[i]!='\0'){
				printf("%c", message->mtext[i]);
				i++;
			}
		#endif
		
		release_memory_block(message);
	}
}
