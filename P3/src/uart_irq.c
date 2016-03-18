/**
 * @brief: uart_irq.c 
 * @author: NXP Semiconductors
 * @author: Y. Huang
 * @date: 2014/02/28
 */

#include <LPC17xx.h>
#include "uart.h"
#include "uart_polling.h"
#include "rtx.h"
#include "str_util.h"
#ifdef DEBUG_0
#include "printf.h"
#endif
#include "k_process.h"


uint8_t g_buffer[]  = "placeholder text";
uint8_t *gp_buffer = g_buffer;
uint8_t g_send_char = 0;
uint8_t g_char_in;
uint8_t g_char_out;
volatile int still_writing = 1;

volatile MSG_BUF* gp_cur_msg_buf = NULL;
MSG_BUF* gp_cur_char = NULL;
int g_entering_kc = 0;

PCB* gp_uart_pcb;
extern PCB* gp_current_process;
extern PCB** gp_pcbs;	

extern uint32_t g_switch_flag;

/**
 * @brief: initialize the n_uart
 * NOTES: It only supports UART0. It can be easily extended to support UART1 IRQ.
 * The step number in the comments matches the item number in Section 14.1 on pg 298
 * of LPC17xx_UM
 */
int uart_irq_init(int n_uart) {
	
	int i;
	
	LPC_UART_TypeDef *pUart;
	
	for ( i = 0; i < NUM_TEST_PROCS + NUM_SYS_PROCS + NUM_USER_PROCS + NUM_IRQ_PROCS; i++ ) {
		if (PID_UART_IPROC == gp_pcbs[i]->m_pid) {
			gp_uart_pcb = gp_pcbs[i];
			break;
		}
	}
	
	if ( n_uart ==0 ) {
		/*
		Steps 1 & 2: system control configuration.
		Under CMSIS, system_LPC17xx.c does these two steps
		 
		-----------------------------------------------------
		Step 1: Power control configuration. 
		        See table 46 pg63 in LPC17xx_UM
		-----------------------------------------------------
		Enable UART0 power, this is the default setting
		done in system_LPC17xx.c under CMSIS.
		Enclose the code for your refrence
		//LPC_SC->PCONP |= BIT(3);
	
		-----------------------------------------------------
		Step2: Select the clock source. 
		       Default PCLK=CCLK/4 , where CCLK = 100MHZ.
		       See tables 40 & 42 on pg56-57 in LPC17xx_UM.
		-----------------------------------------------------
		Check the PLL0 configuration to see how XTAL=12.0MHZ 
		gets to CCLK=100MHZin system_LPC17xx.c file.
		PCLK = CCLK/4, default setting after reset.
		Enclose the code for your reference
		//LPC_SC->PCLKSEL0 &= ~(BIT(7)|BIT(6));	
			
		-----------------------------------------------------
		Step 5: Pin Ctrl Block configuration for TXD and RXD
		        See Table 79 on pg108 in LPC17xx_UM.
		-----------------------------------------------------
		Note this is done before Steps3-4 for coding purpose.
		*/
		
		/* Pin P0.2 used as TXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 4);  
		
		/* Pin P0.3 used as RXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 6);  

		pUart = (LPC_UART_TypeDef *) LPC_UART0;	 
		
	} else if ( n_uart == 1) {
	    
		/* see Table 79 on pg108 in LPC17xx_UM */ 
		/* Pin P2.0 used as TXD1 (Com1) */
		LPC_PINCON->PINSEL4 |= (2 << 0);

		/* Pin P2.1 used as RXD1 (Com1) */
		LPC_PINCON->PINSEL4 |= (2 << 2);	      

		pUart = (LPC_UART_TypeDef *) LPC_UART1;
		
	} else {
		return 1; /* not supported yet */
	} 
	
	/*
	-----------------------------------------------------
	Step 3: Transmission Configuration.
	        See section 14.4.12.1 pg313-315 in LPC17xx_UM 
	        for baud rate calculation.
	-----------------------------------------------------
        */
	
	/* Step 3a: DLAB=1, 8N1 */
	pUart->LCR = UART_8N1; /* see uart.h file */ 

	/* Step 3b: 115200 baud rate @ 25.0 MHZ PCLK */
	pUart->DLM = 0; /* see table 274, pg302 in LPC17xx_UM */
	pUart->DLL = 9;	/* see table 273, pg302 in LPC17xx_UM */
	
	/* FR = 1.507 ~ 1/2, DivAddVal = 1, MulVal = 2
	   FR = 1.507 = 25MHZ/(16*9*115200)
	   see table 285 on pg312 in LPC_17xxUM
	*/
	pUart->FDR = 0x21;       
	
 

	/*
	----------------------------------------------------- 
	Step 4: FIFO setup.
	       see table 278 on pg305 in LPC17xx_UM
	-----------------------------------------------------
        enable Rx and Tx FIFOs, clear Rx and Tx FIFOs
	Trigger level 0 (1 char per interrupt)
	*/
	
	pUart->FCR = 0x07;

	/* Step 5 was done between step 2 and step 4 a few lines above */

	/*
	----------------------------------------------------- 
	Step 6 Interrupt setting and enabling
	-----------------------------------------------------
	*/
	/* Step 6a: 
	   Enable interrupt bit(s) wihtin the specific peripheral register.
           Interrupt Sources Setting: RBR, THRE or RX Line Stats
	   See Table 50 on pg73 in LPC17xx_UM for all possible UART0 interrupt sources
	   See Table 275 on pg 302 in LPC17xx_UM for IER setting 
	*/
	/* disable the Divisior Latch Access Bit DLAB=0 */
	pUart->LCR &= ~(BIT(7)); 
	
	//pUart->IER = IER_RBR | IER_THRE | IER_RLS; 
	pUart->IER = IER_RBR | IER_RLS;

	/* Step 6b: enable the UART interrupt from the system level */
	
	if ( n_uart == 0 ) {
		NVIC_EnableIRQ(UART0_IRQn); /* CMSIS function */
	} else if ( n_uart == 1 ) {
		NVIC_EnableIRQ(UART1_IRQn); /* CMSIS function */
	} else {
		return 1; /* not supported yet */
	}
	pUart->THR = '\0';
	return 0;
}


/**
 * @brief: use CMSIS ISR for UART0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_UART0_IRQHandler does the rest of irq handling
 */
__asm void UART0_IRQHandler(void)
{
	PRESERVE8
	IMPORT c_UART0_IRQHandler
	IMPORT k_release_processor
	PUSH{r4-r11, lr}
	BL c_UART0_IRQHandler
	LDR R4, =__cpp(&g_switch_flag)
	LDR R4, [R4]
	MOV R5, #0     
	CMP R4, R5
	BEQ  RESTORE    ; if g_switch_flag == 0, then restore the process that was interrupted
	BL k_release_processor  ; otherwise (i.e g_switch_flag == 1, then switch to the other process)
RESTORE
	POP{r4-r11, pc}
} 
/**
 * @brief: c UART0 IRQ Handler
 */
void c_UART0_IRQHandler(void)
{
	MSG_BUF* cur_char_msg;
	uint8_t IIR_IntId;	    // Interrupt ID from IIR 		 
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
	PCB* orig_proc = gp_current_process;
	char endline[3] = "\n\r";
	
	gp_current_process = gp_uart_pcb;
	
#ifdef DEBUG_0
	uart1_put_string("Entering c_UART0_IRQHandler\n\r");
#endif // DEBUG_0

	/* Reading IIR automatically acknowledges the interrupt */
	IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR 
	if (IIR_IntId & IIR_RDA) { // Receive Data Avaialbe
		/* read UART. Read RBR will clear the interrupt */
		g_char_in = pUart->RBR;
#ifdef DEBUG_0
		uart1_put_string("Reading a char = ");
		uart1_put_char(g_char_in);
		uart1_put_string("\n\r");
#endif // DEBUG_0
		g_buffer[12] = g_char_in; // nasty hack
		g_send_char = 1;
		
		#ifdef _DEBUG_HOTKEYS
		if ('1' == g_char_in) {
			queue_debug_statement(RDY);
		}
		else if ('2' == g_char_in) {
			queue_debug_statement(BLOCKED);
		}
		else if ('3' == g_char_in) {
			queue_debug_statement(BLOCKED_ON_RECEIVE);
		}
		#endif 
		
		//forward single character to kcd
		cur_char_msg = (MSG_BUF*) k_request_memory_block();
		if (cur_char_msg == NULL){
			#ifdef DEBUG_0
				uart1_put_string("No moar memory to output character\n\r");
			#endif // DEBUG_0
			gp_current_process = orig_proc;
			return;
		}
		cur_char_msg->mtype = CRT_DISPLAY;
		cur_char_msg->mtext[0] = g_char_in;
		if ('\n'  == g_char_in || '\r' == g_char_in) {
			str_copy(endline, cur_char_msg->mtext);
// 			cur_char_msg->mtext[0] = '\n';
// 			cur_char_msg->mtext[1] = '\r';
// 			cur_char_msg->mtext[2] = '\0';
		}
		else {
			cur_char_msg->mtext[1] = '\0';
		}
		k_send_message(PID_KCD, (void*) cur_char_msg);
		
		if (g_entering_kc) {
			//add to msg_buf
			//make sure you don't overflow the message buffer, could be like 100
			gp_cur_msg_buf->mtext[g_entering_kc] = g_char_in;
			if ('\n' == g_char_in || '\r' == g_char_in) {
				gp_cur_msg_buf->mtext[g_entering_kc] = '\0';
				k_send_message(PID_KCD, (void*) gp_cur_msg_buf);
				g_entering_kc = -1;
			}
			g_entering_kc++;
		}
		else if (g_char_in == '%') {
			//make msg_buf
			gp_cur_msg_buf = (MSG_BUF*) k_request_memory_block();
			if (gp_cur_msg_buf == NULL){
				#ifdef DEBUG_0
					uart1_put_string("No moar memory for command\n\r");
				#endif // DEBUG_0
				gp_current_process = orig_proc;
				return;
			}
			gp_cur_msg_buf->mtype = DEFAULT;
			gp_cur_msg_buf->mtext[g_entering_kc] = g_char_in;
			g_entering_kc++;
		}
		
		/* setting the g_switch_flag */
// 		if ( g_char_in == 'S' ) {
// 			g_switch_flag = 1; 
// 		} else {
// 			g_switch_flag = 0;
// 		}
	} else if (IIR_IntId & IIR_THRE) {
	/* THRE Interrupt, transmit holding register becomes empty */

		if (*gp_buffer != '\0' ) {
			g_char_out = *gp_buffer;
#ifdef DEBUG_0
			//uart1_put_string("Writing a char = ");
			//uart1_put_char(g_char_out);
			//uart1_put_string("\n\r");
			
			// you could use the printf instead
			printf("Writing a char = %c \n\r", g_char_out);
#endif // DEBUG_0			
			pUart->THR = g_char_out;
			gp_buffer++;
		} else {
#ifdef DEBUG_0
			uart1_put_string("Finish writing. Turning off IER_THRE\n\r");
#endif // DEBUG_0
			pUart->IER ^= IER_THRE; // toggle the IER_THRE bit 
			pUart->THR = '\0';
			g_send_char = 0;
			gp_buffer = g_buffer;		
			still_writing = 0;//has finshed writing string to output
		}
	      
	} else {  /* not implemented yet */
#ifdef DEBUG_0
			uart1_put_string("Should not get here!\n\r");
#endif // DEBUG_0
		gp_current_process = orig_proc;
		return;
	}	
	gp_current_process = orig_proc;
}

void write_to_CRT(char* text){
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
	
	still_writing = 1;
	
	gp_buffer = (uint8_t *)text;	
	pUart->IER = IER_THRE | IER_RLS | IER_RBR; // set these bits to 1
	
	while(still_writing){
		//continue writing until the text is finished
	}
	
}

