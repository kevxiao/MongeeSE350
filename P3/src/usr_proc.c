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

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

extern uint32_t g_timer_count;

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
	
	//something test
	set_process_priority(PID_P4, MEDIUM);
	set_process_priority(PID_P6, LOW);
	total++;
	
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
	
	endTestText[11] = '0'+passed;
	endTestText[13] = '0'+total;
	endTestText[36] = '0'+failed;
	endTestText[38] = '0'+total;
	message = request_memory_block();
	str_copy(endTestText, message->mtext);
	message->mtype = CRT_DISPLAY;
	send_message(PID_CRT , message);
	
	set_process_priority(PID_CLOCK, MEDIUM);
	set_process_priority(PID_P5, MEDIUM);
	set_process_priority(PID_A, LOW);
	set_process_priority(PID_B, LOW);
	set_process_priority(PID_C, MEDIUM);
	set_process_priority(PID_P6, LOW);
	
	while (1) {
		release_processor();
	}
}

void proc1(void) {
	MSG_BUF* message;// = request_memory_block();
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
		//send_message(PID_CRT ,message);
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

void proc2(void) {
	MSG_BUF* message;
// 	int i = 0;
	while ( 1) {
		message = request_memory_block();
		//i=0;
		str_copy(text, message->mtext);
// 		while(text[i]!='\0'){
// 			message->mtext[i] = text[i];
// 			i++;
// 		}
// 		message->mtext[i] = text[i];
		message->mtype = DEFAULT;
		send_message(PID_P3 ,message);
		set_process_priority(PID_P2, LOWEST);
		release_processor();
	}
}

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
// 		while(message->mtext[i]!='\0'){
// 			textReceived[i] = message->mtext[i];
// 			i++;
// 		}
// 		textReceived[i] = message->mtext[i];
		release_memory_block(message);
		set_process_priority(PID_P6, MEDIUM);
	}
}

void proc4(void) {
	int sender_id;
	while ( 1) {
		receive_message(&sender_id);
		shouldNotRun = 1;
	}
}

//could use more str_copy
void proc5(void) {
	int sender_id;
	MSG_BUF* incoming_msg;
	MSG_BUF* action_msg;
	char com[3] = "%R";
	MSG_BUF* kcd_reg_msg = request_memory_block();
	int i;
	int j;
	char dtext[40] = "Proc 5 says this is a command.\n\r";
	kcd_reg_msg->mtype = KCD_REG;
	str_copy(com, kcd_reg_msg->mtext);
// 	kcd_reg_msg->mtext[0] = '%';
// 	kcd_reg_msg->mtext[1] = 'R';
// 	kcd_reg_msg->mtext[2] = '\0';
	send_message(PID_KCD, kcd_reg_msg);
	while (1) {
		incoming_msg = (MSG_BUF*) receive_message(&sender_id);
		if ('R' == incoming_msg->mtext[1]) {
			action_msg = request_memory_block();
			i = 0;
			j = 0;
			action_msg->mtext[j] = incoming_msg->mtext[i];
			i++;
			j++;
			while(incoming_msg->mtext[i] == 'R'){i++;}
			while(incoming_msg->mtext[i] != '\0') {
				action_msg->mtext[j] = incoming_msg->mtext[i];
				i++;
				j++;
			}
			
			action_msg->mtext[j] = incoming_msg->mtext[i];
			action_msg->mtype = KCD_REG;
			send_message(PID_KCD, action_msg);
		}
		else {
			action_msg = request_memory_block();
			i = 0;
			while(dtext[i] !='\0'){
				action_msg->mtext[i] = dtext[i];
				i++;
			}
			action_msg->mtext[i] = dtext[i];
			action_msg->mtype = CRT_DISPLAY;
			send_message(PID_CRT , action_msg);
			
		}
		release_memory_block(incoming_msg);
	}
}
