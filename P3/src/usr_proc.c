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
#include "str_util.h"
#include "timer.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

extern uint32_t g_timer_count;
extern uint32_t g_timer1_count;

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void* used_mem_blocks[1000];
int curr_index = 0;
int memoryAllocated = 0;
int proc1_count = 0;
int shouldNotRun = 0;
int proc1Prio = -1;
int difference5000 = 0;
char text[20] = "Hello";
char textReceived[20] = "";
int isDelayed = 1;

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
	g_test_procs[5].m_priority = MEDIUM;
}

//process that runs the tests
void proc6(void){
	int passed = 0;
	int failed = 0;
	int total = 0;
	MSG_BUF* message;
	char* startTestText = "G008_test: START\n\rG008_test: total 3 tests\n\r";
	char* passTestText = "G008_test: test x OK\n\r";	//replace 16 for test
	char* failTestText = "G008_test: test x FAIL\n\r";	//replace 16 for test
	char* endTestText = "G008_test: p/t tests OK\n\rG008_test: f/t tests FAIL\n\rG008_test: END\n\r"; //p at 11, t at 13, f at 36, t at 38 
	
	message = request_memory_block();
	str_copy(startTestText, message->mtext);
	message->mtype = CRT_DISPLAY;
	send_message(PID_CRT , message);
	
	set_process_priority(PID_P1, LOWEST);
	set_process_priority(PID_P2, LOWEST);
	set_process_priority(PID_P3, LOWEST);
	set_process_priority(PID_P4, LOWEST);
	set_process_priority(PID_P5, LOWEST);
	
	//send-receive test
	set_process_priority(PID_P2, MEDIUM);
	set_process_priority(PID_P3, MEDIUM);
	set_process_priority(PID_P6, LOW);
	total++;

	//check passed/failed
	if (str_compare(text, textReceived)) {
		passed++;
		passTestText[16] = '1';
		message = request_memory_block();
		str_copy(passTestText, message->mtext);
		message->mtype = CRT_DISPLAY;
		send_message(PID_CRT , message);
	} else {
		failed++;
		failTestText[16] = '1';
		message = request_memory_block();
		str_copy(failTestText, message->mtext);
		message->mtype = CRT_DISPLAY;
		send_message(PID_CRT , message);
	}
	
	set_process_priority(PID_P2, LOWEST);
	set_process_priority(PID_P3, LOWEST);
	
	//delayed test
	set_process_priority(PID_P1, MEDIUM);
	set_process_priority(PID_P6, LOW);
	total++;
	
	while(isDelayed){
		release_processor();
	}
	
	//check passed/failed
	if(difference5000 == 5000){
		passed++;
		passTestText[16] = '2';
		message = request_memory_block();
		str_copy(passTestText, message->mtext);
		message->mtype = CRT_DISPLAY;
		send_message(PID_CRT , message);
	} else {
		failed++;
		failTestText[16] = '2';
		message = request_memory_block();
		str_copy(failTestText, message->mtext);
		message->mtype = CRT_DISPLAY;
		send_message(PID_CRT , message);
	}
	
	set_process_priority(PID_P1, LOWEST);	
	
	set_process_priority(PID_P4, MEDIUM);
	set_process_priority(PID_P6, LOW);
	total++;
	
	//check passed/failed
	if(shouldNotRun != 1){
		passed++;
		passTestText[16] = '3';
		message = request_memory_block();
		str_copy(passTestText, message->mtext);
		message->mtype = CRT_DISPLAY;
		send_message(PID_CRT , message);
	} else {
		failed++;
		failTestText[16] = '3';
		message = request_memory_block();
		str_copy(failTestText, message->mtext);
		message->mtype = CRT_DISPLAY;
		send_message(PID_CRT , message);
	}

	set_process_priority(PID_P4, LOWEST);
	
	//Output result
	
	endTestText[11] = '0'+passed;
	endTestText[13] = '0'+total;
	endTestText[36] = '0'+failed;
	endTestText[38] = '0'+total;
	message = request_memory_block();
	str_copy(endTestText, message->mtext);
	message->mtype = CRT_DISPLAY;
	send_message(PID_CRT , message);
	
	set_process_priority(PID_CLOCK, MEDIUM);
	set_process_priority(PID_P5, HIGH);
	set_process_priority(PID_A, LOW);
	set_process_priority(PID_B, LOW);
	set_process_priority(PID_C, MEDIUM);
	set_process_priority(PID_P6, LOW);
	
	while (1) {
		release_processor();
	}
}

//delayed send to self in 5 seconds
void proc1(void) {
	MSG_BUF* message;
	int i = 0;
	char text[20] = "Delayed Message\n\r";
	int sender_id;
	int send_time;
	int receive_time;
	MSG_BUF* received_message;
	while ( 1) {
		message = request_memory_block();
		i=0;
		while(text[i]!='\0'){
			message->mtext[i] = text[i];
			i++;
		}
		message->mtext[i] = text[i];
		message->mtype = DEFAULT;
		//send message to self in 5 seconds
		delayed_send(PID_P1, message, 5000);
		send_time = g_timer_count;
		i=0;
		received_message = receive_message(&sender_id);
		receive_time = g_timer_count;
		difference5000 = receive_time - send_time;
		isDelayed = 0;
		
		#ifdef DEBUG_0
		printf("Received the following message from process %d with delay %dms:\n\r", sender_id, receive_time - send_time);
		#endif
		while(message->mtext[i]!='\0'){
			#ifdef DEBUG_0
			printf("%c", message->mtext[i]);
			#endif
			i++;
		}
		release_memory_block(received_message);
		set_process_priority(PID_P6, MEDIUM);
		release_processor();
	}
}

//sends a message to proc 3
void proc2(void) {
	MSG_BUF* message;
	while ( 1) {
		message = request_memory_block();
		str_copy(text, message->mtext);
		message->mtype = DEFAULT;
		send_message(PID_P3 ,message);
		set_process_priority(PID_P2, LOWEST);
		release_processor();
	}
}

//receives a message from proc 2
void proc3(void) {
	int i=0;
	int sender_id;
	while ( 1) {
		MSG_BUF* message = receive_message(&sender_id);
		#ifdef DEBUG_0
			printf("Received the following message from process %d:\n\r", sender_id);
			while(message->mtext[i]!='\0'){
				printf("%c", message->mtext[i]);
				i++;
			}
			printf("\n\r");
		#endif
		str_copy(message->mtext, textReceived);
		release_memory_block(message);
		set_process_priority(PID_P6, MEDIUM);
	}
}

//should be blocked on receive always
void proc4(void) {
	int sender_id;
	while ( 1) {
		receive_message(&sender_id);
		shouldNotRun = 1;
	}
}

//Used to analyze timings
void proc5(void){
	
	MSG_BUF* message;
	char test_message[20] = "Hello";
	int sender_id;
	MSG_BUF* received_message;
	int start_time, finish_time;
	while ( 1) {
		
 		start_time = g_timer1_count;
		message = request_memory_block();
		finish_time = g_timer1_count;
		#ifdef DEBUG_0
			printf("Request memory block time: %d\n\r", (finish_time - start_time));
		#endif
		
		str_copy(test_message, message->mtext);
		message->mtype = DEFAULT;
		
		start_time = g_timer1_count;
		send_message(PID_P5 ,message);
		finish_time = g_timer1_count;
		#ifdef DEBUG_0
			printf("Send message time: %d\n\r", finish_time - start_time);
		#endif
		start_time = g_timer1_count;
		received_message = receive_message(&sender_id);
		finish_time = g_timer1_count;
		#ifdef DEBUG_0
			printf("Receive message time: %d\n\r", finish_time - start_time);
		#endif
		set_process_priority(PID_P5, LOW);
		release_processor();
	}
	
}