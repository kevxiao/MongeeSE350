#include "uart_polling.h"
#include "uart.h"
#include "sys_proc.h"
#include "rtx.h"


#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

PROC_INIT g_sys_procs[NUM_SYS_PROCS];
PROC_INIT g_irq_procs[NUM_IRQ_PROCS];
REG_COM_ID* gp_reg_com_head = NULL;

void set_sys_procs() {
	int i;
	for( i = 0; i < NUM_SYS_PROCS; i++ ) {
		g_sys_procs[i].m_priority=5;
		g_sys_procs[i].m_stack_size=0x100;
	}
	g_sys_procs[0].m_priority = 4;
	g_sys_procs[0].m_pid = PID_NULL;
	g_sys_procs[0].mpf_start_pc = &nullproc;
	g_sys_procs[1].m_pid = PID_KCD;
	g_sys_procs[1].mpf_start_pc = &kcdproc;
	g_sys_procs[2].m_pid = PID_CRT;
	g_sys_procs[2].mpf_start_pc = &crtproc;
}

void set_irq_procs() {
	int i;
	for( i = 0; i < NUM_IRQ_PROCS; i++ ) {
		g_irq_procs[i].m_priority=6;
		g_irq_procs[i].m_stack_size=0x100;
	}
	g_irq_procs[0].m_pid = PID_TIMER_IPROC;
	g_irq_procs[0].mpf_start_pc = NULL;
	g_irq_procs[1].m_pid = PID_UART_IPROC;
	g_irq_procs[1].mpf_start_pc = NULL;
}

void nullproc(void)
{
	while (1) {
		release_processor();
	}
}

void crtproc(void){
	MSG_BUF *message;
	int sender_pid;
	uart_irq_init(0);
	
	while(1){
		message = receive_message(&sender_pid);
		if(CRT_DISPLAY == message->mtype) {
			write_to_CRT(message->mtext);
		}
		release_memory_block(message);
		//release_processor();
	}
}

void set_command_id(int pid, char* identifier)  {
	REG_COM_ID* prev;
	REG_COM_ID* cur;
	int i = 0;
	
	prev = NULL;
	
	cur = gp_reg_com_head;
	
	while (NULL != cur) {
		if (cur->mpid == pid) {
			break;
		}
		prev = cur;
		cur = (REG_COM_ID*) cur->mp_next;
	}
	if (NULL == cur) {
		cur = (REG_COM_ID*) k_request_memory_block();
	}
	
	cur->mpid = pid;
	cur->mp_next = NULL;
	while ('\0' != identifier[i]) {
		cur->mtext_id[i] = identifier[i];
		i++;
	}
	cur->mtext_id[i] = '\0';
	
	if (NULL == prev) {
		gp_reg_com_head = cur;
	}
}

int get_pid_from_com_id(char* command) {
	REG_COM_ID* cur = gp_reg_com_head;
	int i;
	//int pid;
	int found;
	while (NULL != cur) {
		found = 1;
		i = 0;
		while ('\0' != cur->mtext_id[i]) {
			if (cur->mtext_id[i] != command[i]) {
				found = 0;
				break;
			}
			i++;
		}
		if (found) {
			return cur->mpid;
		}
		cur = cur->mp_next;
	}
	return RTX_ERR;
}

void kcdproc(void) {
	MSG_BUF* command_msg;
	//MSG_BUF* string_msg;
	int sender_id;
	int pid;
	char* command_text;
	while (1) {
		command_msg = (MSG_BUF*) receive_message(&sender_id);
		//execute command
		command_text = command_msg->mtext;
		if (KCD_REG == command_msg->mtype) {
			set_command_id(sender_id, command_text);
			release_memory_block(command_msg);
		}
		else if (CRT_DISPLAY == command_msg->mtype) {
			send_message(PID_CRT, (void*) command_msg);
		}
		//check if valid command
		else if ('%' == command_text[0]) {
			pid = get_pid_from_com_id(command_text+1);
			if (RTX_ERR != pid) {
				send_message(pid, (void*)command_msg);
			}
			else {
				//unrecognized command
			}
		}
		else {
			release_memory_block(command_msg);
		}
	}
}
