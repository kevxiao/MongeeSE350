/**
 * @file:   usr_proc.c
 * @brief:  Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date:   2014/01/17
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
#include "uart_polling.h"
#include "user_proc.h"
#include "str_util.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

extern uint32_t g_timer_count;

/* initialization table item */
PROC_INIT g_user_procs[NUM_USER_PROCS];

typedef struct msglocal
{
	void *mp_next;		/* ptr to next in local queue*/
	int m_kdata[7];		/* extra data so that MSG_BUF can be cast to MSG_LOCAL */
	int mtype;              /* user defined message type */
	char mtext[1];          /* body of the message */
} MSG_LOCAL;

void set_user_procs() {
	int i;
	for( i = 0; i < NUM_USER_PROCS; i++ ) {
		g_user_procs[i].m_priority=LOWEST;
		g_user_procs[i].m_stack_size=0x100;
	}

	g_user_procs[0].m_pid = PID_CLOCK;
	g_user_procs[0].m_priority=MEDIUM;
	g_user_procs[0].mpf_start_pc = &proc_wall_clock;
	g_user_procs[1].m_pid = PID_SET_PRIO;
	g_user_procs[1].m_priority=MEDIUM;
	g_user_procs[1].mpf_start_pc = &proc_set_proc_priority;
	g_user_procs[2].m_pid = PID_A;
	g_user_procs[2].m_priority=MEDIUM;
	g_user_procs[2].mpf_start_pc = &procA;
	g_user_procs[3].m_pid = PID_B;
	g_user_procs[3].m_priority=MEDIUM;
	g_user_procs[3].mpf_start_pc = &procB;
	g_user_procs[4].m_pid = PID_C;
	g_user_procs[4].m_priority=MEDIUM;
	g_user_procs[4].mpf_start_pc = &procC;
}

void proc_wall_clock(void) {
	char ASCII0 = '0';
	int SECONDS_IN_DAY = 86400;
	int SECONDS_IN_MINUTE = 60;
	int MINUTES_IN_HOUR = 60;
	int SECONDS_IN_HOUR = SECONDS_IN_MINUTE*MINUTES_IN_HOUR;
	int CLOCK_DELAY = 1000;
	char com[3] = "%W";
	MSG_BUF* output_msg;
	MSG_BUF* incoming_msg;
	MSG_BUF* self_msg = request_memory_block();
	int sender_id;
	int display_flag = 0;
	char* command_text;
	int numHours;
	int numMinutes;
	int numSeconds;
	U32 clock_time = 0;
	MSG_BUF* kcd_reg_msg = request_memory_block();
	kcd_reg_msg->mtype = KCD_REG;
	str_copy(com, kcd_reg_msg->mtext);
	send_message(PID_KCD, kcd_reg_msg);
	self_msg->mtype = DEFAULT;
	while (1) {
		incoming_msg = (MSG_BUF*) receive_message(&sender_id);
		if (PID_CLOCK == sender_id) {
			if (display_flag) {
				delayed_send(PID_CLOCK, self_msg, CLOCK_DELAY);
			
				clock_time = (clock_time + 1) % SECONDS_IN_DAY;
				
				output_msg = (MSG_BUF*) k_request_memory_block();
				output_msg->mtype = CRT_DISPLAY;
				
				output_msg->mtext[0] = (char)((clock_time/SECONDS_IN_HOUR)/10) + ASCII0;
				output_msg->mtext[1] = (char)((clock_time/SECONDS_IN_HOUR) % 10) + ASCII0;
				output_msg->mtext[2] = ':';
				output_msg->mtext[3] = (char) (((clock_time/SECONDS_IN_MINUTE)%SECONDS_IN_MINUTE)/10) + ASCII0;
				output_msg->mtext[4] = (char) (((clock_time/SECONDS_IN_MINUTE)%SECONDS_IN_MINUTE)%10) + ASCII0;
				output_msg->mtext[5] = ':';
				output_msg->mtext[6] = (char) ((clock_time%SECONDS_IN_MINUTE)/10) + ASCII0;
				output_msg->mtext[7] = (char) ((clock_time%SECONDS_IN_MINUTE)%10) + ASCII0;
				output_msg->mtext[8] = '\n';
				output_msg->mtext[9] = '\r';
				output_msg->mtext[10] = '\0';
				
				send_message(PID_CRT, (void*) output_msg);
			}
		}
		else if (PID_KCD == sender_id) {
			command_text = incoming_msg->mtext;
			if ('W' == command_text[1] && 'R' == command_text[2] && '\0' == command_text[3]) {
				clock_time = 0;
				if (0 == display_flag) {
					delayed_send(PID_CLOCK, self_msg, CLOCK_DELAY);
				}
				display_flag = 1;
			}
			else if ('S' == command_text[2] && ' ' == command_text[3] && '\0' == command_text[12]) {
				if (':' == command_text[6] && ':' == command_text[9]) {
					numHours = str_to_int(command_text+4);
					numMinutes = str_to_int(command_text+7);
					numSeconds = str_to_int(command_text+10);
					
					if ( (int)(command_text[4] - ASCII0) < 0 || (int)(command_text[4] - ASCII0) > 9 || (int)(command_text[5] - ASCII0) < 0 || (int)(command_text[5] - ASCII0) > 9
						|| (int)(command_text[7] - ASCII0) < 0 || (int)(command_text[7] - ASCII0) > 9 || (int)(command_text[8] - ASCII0) < 0 || (int)(command_text[8] - ASCII0) > 9
					  || (int)(command_text[10] - ASCII0) < 0 || (int)(command_text[10] - ASCII0) > 9 || (int)(command_text[11] - ASCII0) < 0 || (int)(command_text[11] - ASCII0) > 9
						|| numHours > 23 || numMinutes > 59 || numSeconds > 59) {
							//Invalid time format error
						}
						else {
							clock_time = numSeconds+SECONDS_IN_MINUTE*(numMinutes+MINUTES_IN_HOUR*numHours);
							if (0 == display_flag) {
								delayed_send(PID_CLOCK, self_msg, CLOCK_DELAY);
							}
							display_flag = 1;
						}
							
				}
				else {
					//argument syntax error
				}
			}
			else if ('T' == command_text[2] && '\0' == command_text[3]) {
				display_flag= 0;
			}
			else {
				//invalid command error
			}
			release_memory_block((void*)incoming_msg);
		}
	}
}

void proc_set_proc_priority(void) {
	char com[3] = "%C";
	MSG_BUF* incoming_msg;
	int sender_id;
	int i;
	int pid;
	int prio;
	MSG_BUF* kcd_reg_msg = request_memory_block();
	kcd_reg_msg->mtype = KCD_REG;
	str_copy(com, kcd_reg_msg->mtext);
	send_message(PID_KCD, kcd_reg_msg);
	while (1) {
		incoming_msg = (MSG_BUF*) receive_message(&sender_id);
		if ('%' == incoming_msg->mtext[0] && 'C' == incoming_msg->mtext[1] && ' ' == incoming_msg->mtext[2]) {
			i = 3;
			while (' ' == incoming_msg->mtext[i]) {
				++i;
			}
			pid = str_to_int(incoming_msg->mtext+i);
			while ('0' <= incoming_msg->mtext[i] && '9' >= incoming_msg->mtext[i]) {
				++i;
			}
			if (' ' == incoming_msg->mtext[i]) {
				while (' ' == incoming_msg->mtext[i]) {
					++i;
				}
				prio = str_to_int(incoming_msg->mtext+i);
				while ('0' <= incoming_msg->mtext[i] && '9' >= incoming_msg->mtext[i]) {
					++i;
				}
				if (' ' == incoming_msg->mtext[i] || '\0' == incoming_msg->mtext[i]) {
					set_process_priority(pid, prio);
				}
			}
		}
		release_memory_block((void*) incoming_msg);
	}
}

void procA(void) {
	char com[3] = "%Z";
	int num = 0;
	MSG_BUF* msg;
	int sender_id;
	MSG_BUF* kcd_reg_msg = (MSG_BUF*) request_memory_block();
	kcd_reg_msg->mtype = KCD_REG;
	str_copy(com, kcd_reg_msg->mtext);
	send_message(PID_KCD, kcd_reg_msg);
	while(1) {
		msg = (MSG_BUF*) receive_message(&sender_id);
		if ('%' == msg->mtext[0] && 'Z' == msg->mtext[1]) {
			release_memory_block((void*) msg);
			break;
		} else {
			release_memory_block((void*) msg);
		}
	}
	num = 0;
	while(1) {
		msg = (MSG_BUF*)request_memory_block();
		msg->mtype = count_report;
		// sketchy as fuck
		msg->mtext[0] = (char)num;
		msg->mtext[1] = (char)(num >> 8);
		msg->mtext[2] = (char)(num >> 16);
		msg->mtext[3] = (char)(num >> 24);
		if((int)(msg->mtext[0]) != num) {
			msg->mtext[0] = (char)(num >> 24);
			msg->mtext[1] = (char)(num >> 16);
			msg->mtext[2] = (char)(num >> 8);
			msg->mtext[3] = (char)num;
		}
		// end sketchy as fuck
		send_message(PID_B, msg);
		++num;
		release_processor();
	}
}

void procB(void) {
	MSG_BUF* msg;
	int sender_id;
	while(1) {
		msg = (MSG_BUF*) receive_message(&sender_id);
		send_message(PID_C, msg);
	}
}

void procC(void) {
	char output[12] = "Process C\n\r";
	MSG_LOCAL* q_local_head = NULL;
	MSG_LOCAL* q_local_tail = NULL;
	MSG_LOCAL* temp;
	MSG_BUF* msg;
	int sender_id;
	MSG_BUF* wakeup_msg;
	int num;
	while (1) {
		if (NULL == q_local_head) {
			msg = receive_message(&sender_id);
		}
		else {
			temp = q_local_head->mp_next;
			msg = (MSG_BUF*) q_local_head;
			if(NULL == temp) {
				q_local_tail = NULL;
			}
			q_local_head = temp;
		}
		
		if (count_report == msg->mtype) {
			num = (int)(msg->mtext[0]);
			if (0 == num % 20) {
				msg->mtype = CRT_DISPLAY;
				str_copy(output, msg->mtext);
				send_message(PID_CRT, msg);
				//hibernate for 10 seconds
				wakeup_msg = (MSG_BUF*) request_memory_block();
				wakeup_msg->mtype = wakeup10;
				delayed_send(PID_C, wakeup_msg, 10000);
				while (1) {
					msg = receive_message(&sender_id);
					if (wakeup10 == msg->mtype) {
						break;
					}
					else {
						temp = (MSG_LOCAL*) msg;
						temp->mp_next = NULL;
						if (NULL != q_local_tail) {
							q_local_tail->mp_next = temp;
						}
						else {
							q_local_head = temp;
						}
						q_local_tail = temp;
					}
				}
			}
		}
		release_memory_block((void*) msg);
		release_processor();
	}
}
