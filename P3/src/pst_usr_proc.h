/**
 * @file:   pst_usr_proc.h
 * @brief:  Two user processes header file
 * @author: Mongeese
 * @date:   2014/01/17
 */
 
#ifndef PST_USR_PROC_H_
#define PST_USR_PROC_H_

void set_user_procs(void);

void proc_wall_clock(void);
void proc_set_proc_priority(void);
void procA(void);
void procB(void);
void procC(void);

#endif /* PST_USR_PROC_H_ */
