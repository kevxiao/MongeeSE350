/**
 * @brief timer.c - Timer example code. Tiemr IRQ is invoked every 1ms
 * @author T. Reidemeister
 * @author Y. Huang
 * @author NXP Semiconductors
 * @date 2012/02/12
 */

#include <LPC17xx.h>
#include "timer.h"
#include "rtx.h"
#include "k_process.h"

#define BIT(X) (1<<X)

volatile uint32_t g_timer_count = 0; // increment every 1 ms
volatile uint32_t g_timer1_count = 0; // increment every 1.2 us

extern MSG_BUF* gp_delayed_msgs;

PCB* gp_timer_pcb;
extern PCB* gp_current_process;
extern PCB** gp_pcbs;	

/**
 * @brief: initialize timer. Only timer 0 and timer 1 is supported
 */
uint32_t timer_init(uint8_t n_timer) 
{
	LPC_TIM_TypeDef *pTimer;
	int i;
	
	for ( i = 0; i < NUM_TEST_PROCS + NUM_SYS_PROCS + NUM_USER_PROCS + NUM_IRQ_PROCS; i++ ) {
		if (PID_UART_IPROC == gp_pcbs[i]->m_pid) {
			gp_timer_pcb = gp_pcbs[i];
			break;
		}
	}
	
	if (n_timer == 0) {
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
		//LPC_SC->PCONP |= BIT(1);
	
		-----------------------------------------------------
		Step2: Select the clock source, 
		       default PCLK=CCLK/4 , where CCLK = 100MHZ.
		       See tables 40 & 42 on pg56-57 in LPC17xx_UM.
		-----------------------------------------------------
		Check the PLL0 configuration to see how XTAL=12.0MHZ 
		gets to CCLK=100MHZ in system_LPC17xx.c file.
		PCLK = CCLK/4, default setting in system_LPC17xx.c.
		Enclose the code for your reference
		//LPC_SC->PCLKSEL0 &= ~(BIT(3)|BIT(2));	

		-----------------------------------------------------
		Step 3: Pin Ctrl Block configuration. 
		        Optional, not used in this example
		        See Table 82 on pg110 in LPC17xx_UM 
		-----------------------------------------------------
		*/
		pTimer = (LPC_TIM_TypeDef *) LPC_TIM0;

	} else if (n_timer == 1) {
		pTimer = (LPC_TIM_TypeDef *) LPC_TIM1;
	}		else { /* other timer not supported yet */
		return 1;
	}

	/*
	-----------------------------------------------------
	Step 4: Interrupts configuration
	-----------------------------------------------------
	*/

	/* Step 4.1: Prescale Register PR setting 
	   CCLK = 100 MHZ, PCLK = CCLK/4 = 25 MHZ
	   2*(12499 + 1)*(1/25) * 10^(-6) s = 10^(-3) s = 1 ms
	   TC (Timer Counter) toggles b/w 0 and 1 every 12500 PCLKs
	   see MR setting below 
	*/
	if(n_timer == 0) {
		pTimer->PR = 12499;
	} else if(n_timer == 1) {
		pTimer->PR = 15;
	}

	/* Step 4.2: MR setting, see section 21.6.7 on pg496 of LPC17xx_UM. */
	pTimer->MR0 = 1;

	/* Step 4.3: MCR setting, see table 429 on pg496 of LPC17xx_UM.
	   Interrupt on MR0: when MR0 mathches the value in the TC, 
	                     generate an interrupt.
	   Reset on MR0: Reset TC if MR0 mathches it.
	*/
	pTimer->MCR = BIT(0) | BIT(1);
	
	if(n_timer == 0) {
		g_timer_count = 0;

		/* Step 4.4: CSMSIS enable timer0 IRQ */
		NVIC_EnableIRQ(TIMER0_IRQn);
	} else if(n_timer == 1) {
		g_timer1_count = 0;

		/* Step 4.4: CSMSIS enable timer1 IRQ */
		NVIC_EnableIRQ(TIMER1_IRQn);
	}

	/* Step 4.5: Enable the TCR. See table 427 on pg494 of LPC17xx_UM. */
	pTimer->TCR = 1;
	
	return 0;
}

/**
 * @brief: use CMSIS ISR for TIMER0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_TIMER0_IRQHandler does the rest of irq handling
 */
__asm void TIMER0_IRQHandler(void)
{
	PRESERVE8
	IMPORT c_TIMER0_IRQHandler
	PUSH{r4-r11, lr}
	BL c_TIMER0_IRQHandler
	POP{r4-r11, pc}
} 
/**
 * @brief: c TIMER0 IRQ Handler
 */
void c_TIMER0_IRQHandler(void)
{
	int orig_sender;
	PCB* orig_proc = gp_current_process; //save process
	MSG_BUF* temp;
	
	gp_current_process = gp_timer_pcb;
	
	/* ack inttrupt, see section  21.6.1 on pg 493 of LPC17XX_UM */
	LPC_TIM0->IR = BIT(0);  
	
	g_timer_count++ ;
	
	//send all msgs past send time
	while (gp_delayed_msgs != NULL && gp_delayed_msgs->m_send_time <= g_timer_count) {
		temp = gp_delayed_msgs;
		gp_delayed_msgs = gp_delayed_msgs->mp_next;
		orig_sender = temp->m_send_pid;
		k_send_message(temp->m_recv_pid, (void*) temp);
		temp->m_send_pid = orig_sender;
	}
	//restore process
	gp_current_process = orig_proc;
}

/**
 * @brief: use CMSIS ISR for TIMER1 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_TIMER1_IRQHandler does the rest of irq handling
 */
__asm void TIMER1_IRQHandler(void)
{
	PRESERVE8
	IMPORT c_TIMER1_IRQHandler
	PUSH{r4-r11, lr}
	BL c_TIMER1_IRQHandler
	POP{r4-r11, pc}
} 
/**
 * @brief: c TIMER1 IRQ Handler
 */
void c_TIMER1_IRQHandler(void)
{
	g_timer1_count++ ;
		
	/* ack inttrupt, see section  21.6.1 on pg 493 of LPC17XX_UM */
	LPC_TIM1->IR = BIT(0);  
}
