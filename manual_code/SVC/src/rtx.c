/** 
 * @brief:  rtx.c kernel API implementations, this is only a skeleton.
 * @author: Yiqing Huang
 * @date:   2014/01/12
 */

#include "uart_polling.h"
#include "printf.h"
/****************************************************************************
 ************************READ BEFORE YOU PROCEED FURTHER*********************
 ****************************************************************************
  To better strcture your code, you may want to split these functions
  into different files. For example, memory related kernel APIs in one file
  and process related API in another file.
*/

/* 
  This symbol is defined in the scatter file, 
  refer to RVCT Linker User Guide
*/  
extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;

extern unsigned int MEM_BLK_SIZE = 128;

typedef struct mem_blk
{
	uint32_t* next_blk;
} mem_blk;

extern mem_blk* p_mem_head = 0;

int k_mem_init(void* p_mem_blk)
{
	mem_blk * temp = (mem_blk *) p_mem_blk;
	printf("k_mem_init: init starts at 0x%x\n", p_mem_blk);
	p_mem_head = temp;
	printf("k_mem_init: ram limit at 0x%x\n", &Image$$RW_IRAM1$$ZI$$Limit);
	while((unsigned int) p_mem_blk + MEM_BLK_SIZE < (unsigned int) &Image$$RW_IRAM1$$ZI$$Limit){
		temp = (mem_blk *) p_mem_blk;
		temp->next_blk = (uint32_t *)((unsigned int)p_mem_blk + MEM_BLK_SIZE); //doesn't seem to work
		printf("k_mem_init: next block at 0x%x\n", (void *)((*temp).next_blk));
		p_mem_blk = (*temp).next_blk;
	}
	(*temp).next_blk = NULL;
	
	return 0;
}

int k_release_processor(void)
{
  uart0_put_string("k_release_processor: entering\n\r");
	return 0;
}

void* k_request_memory_block(void) {
  unsigned int end_addr = (unsigned int) &Image$$RW_IRAM1$$ZI$$Limit;
	printf("k_request_memory_block: image ends at 0x%x\n", end_addr);
	
	return (void *)0;
}

int k_release_memory_block(void* p_mem_blk) {
	printf("k_release_memory_block: releasing block @ 0x%x\n", p_mem_blk);
  return 0;
}
