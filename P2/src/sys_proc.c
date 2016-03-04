#include "uart_polling.h"
#include "uart.h"
#include "sys_proc.h"
#include "rtx.h"


#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

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
		release_processor();
	}
}

void kcdproc(void) {
	MSG_BUF* command_msg;
	//MSG_BUF* string_msg;
	int sender_id;
	char* command_text;
	while (1) {
		command_msg = (MSG_BUF*) receive_message(&sender_id);
		//execute command
		command_text = command_msg->mtext;
		//check if valid command
		if ('%' == command_text[0]) {
			if ('W' == command_text[1]) {
				send_message(PID_CLOCK, command_msg);
			}
			else {
				//invalid command error
			}
		}
		release_memory_block(command_msg);
	}
}
