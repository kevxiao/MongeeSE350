/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_

#include "common.h"
#include "sys_proc.h"

/*----- Definitions -----*/

#define NUM_PRIORITIES 5

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#endif /* DEBUG_0 */

/*----- Types -----*/
typedef unsigned char U8;
typedef unsigned int U32;

/* process states, note we only assume three states in this example */
typedef enum {NEW = 0, RDY, RUN, BLOCKED, BLOCKED_ON_RECEIVE} PROC_STATE_E;  

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project 
*/
typedef struct pcb 
{ 
	struct pcb *mp_next;  /* next pcb */  
	U32 *mp_sp;		/* stack pointer of the process */
	U32 m_pid;		/* process id */
	U32 m_priority; /* process priority */
	PROC_STATE_E m_state;   /* state of the process */     \
	MSG_BUF* mp_first_msg;
	MSG_BUF* mp_last_msg;
} PCB;

#endif // ! K_RTX_H_
