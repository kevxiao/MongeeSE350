#include "uart_polling.h"
#include "uart.h"
#include "sys_proc.h"
#include "rtx.h"


#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

REG_COM_ID* gp_reg_com_head = NULL;

/**
 * @brief: the null process
 */
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
		if (found && '\0' == command[i]) {
			return cur->mpid;
		}
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
		//check if valid command
		else if ('%' == command_text[0]) {
			pid = get_pid_from_com_id(&(command_text[1]));
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
