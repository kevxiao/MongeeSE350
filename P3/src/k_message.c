#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

struct msgbuf {
	int mtype; /* user defined message type */
	char mtext[1]; /* body of the message */
};

int k_send_message(int process_id, void *message_envelope) {
	
}
