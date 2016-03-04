#ifndef SYS_PROC_H_
#define SYS_PROC_H

#define NUM_SYS_PROCS 3
#define NUM_IRQ 2

void set_sys_procs(void);
void set_irq_procs(void);
void nullproc(void);
void crtproc(void);
void kcdproc(void);

typedef struct reg_com_id
{
	void *mp_next;		/* ptr to next message received*/
	int mpid;              /* user defined message type */
	char mtext_id[1];          /* body of the message */
} REG_COM_ID;

#endif /* SYS_PROC_H_ */
