#include<init.h>
#include<lib.h>
#include<context.h>
#include<memory.h>

#define osmap(a)        (unsigned long *) osmap(a)

#define PToffset       0x1FF             /* (1 << 9) - 1 : PT offset size */
#define L4offset       0x027             /* L4 Offset : 39-48 bits */
#define L3offset       0x01E             /* L3 Offset : 30-39 bits */
#define L2offset       0x015             /* L2 Offset : 21-30 bits */
#define L1offset       0x00C             /* L1 Offset : 12-21 bits */
#define PTEoffset      0x00C             /* PFN is at an offset of 12 in PTE */

/*System Call handler*/
int do_syscall(int syscall, u64 param1, u64 param2, u64 param3, u64 param4)
{
    struct exec_context *current = get_current_ctx();
    printf("[GemOS] System call invoked. syscall no  = %d\n", syscall);
    switch(syscall)
    {
          case SYSCALL_EXIT:
                              printf("[GemOS] exit code = %d\n", (int) param1);
                              do_exit();
                              break;
          case SYSCALL_GETPID:
                              printf("[GemOS] getpid called for process %s, "
                              " with pid = %d\n", current->name, current->id);
                              return current->id;
          case SYSCALL_WRITE:
	  {
		unsigned long code_start = current->mms[MM_SEG_CODE].start;
		unsigned long code_end = current->mms[MM_SEG_CODE].end;

		// Check for proper VA to PA mapping
		unsigned long address = (unsigned long) param1;
		unsigned long *l4addr = osmap(current->pgd);
		unsigned long *l3addr, *l2addr, *l1addr;

		u32 offsetL4, offsetL3, offsetL2, offsetL1;
		u32 pfnL1, pfnL2, pfnL3;

		/* Extract out all 4 level page offsets */
		offsetL4 = PToffset & (address >> 39);
		offsetL3 = PToffset & (address >> 30);
		offsetL2 = PToffset & (address >> 21);
		offsetL1 = PToffset & (address >> 12);


		/* Check Proper PT Mapping and exit if Page Fault occurs */
		if ((*(l4addr + offsetL4) & 1) == 0)
			return -1;
		else {
			pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
			l3addr = osmap(pfnL3);
		}

		if ((*(l3addr + offsetL3) & 1) == 0)
			return -1;
		else {
			pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
			l2addr = osmap(pfnL2);
		}

		if ((*(l2addr + offsetL2) & 1) == 0)
			return -1;
		else {
			pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
			l1addr = osmap(pfnL1);
		}

		if ((*(l1addr + offsetL1) & 1) == 0)
			return -1;


		// Check whether buff data is not invalid address mapping
		if (param1 > code_end && param1 < code_start)
			return -1;

		// Same sanity check for buff + len - 1
		if (param1 + param2 - 1 > code_end &&
			param1 + param2 - 1 < code_start)
			return -1;

		char *buff = (char *) param1;
		int len = (int) param2;

		/* Sanity for value of len */
		if (len <= 0 || len > 1024 || buff == 0)
			return -1;

		// Write only the len bytes
		len = (len > strlen(buff)) ? strlen(buff) : len;
		for (int i = 0; i < len; i++)
		 	printf("%c", buff[i]);
		printf("\n");
		return len;

          }
	  case SYSCALL_EXPAND:
	  {
		u32 size = (u32) param1;
		int flags = (int) param2;

		if (size < 0 || size > 512)
			return 0;

		unsigned long data_next = current->mms[MM_SEG_DATA].next_free;
		unsigned long data_end = current->mms[MM_SEG_DATA].end;

		unsigned long rodata_next = current->mms[MM_SEG_RODATA].next_free;
		unsigned long rodata_end = current->mms[MM_SEG_RODATA].end;

		if (flags == MAP_WR) {
			if (data_next + (size * 4096) > data_end)
				return 0;
			current->mms[MM_SEG_DATA].next_free += size * 4096;
			return data_next;
		}
		else if (flags == MAP_RD) {
			if (rodata_next + (size * 4096) > rodata_end)
				return 0;
			current->mms[MM_SEG_RODATA].next_free += size * 4096;
			return rodata_next;
		}

		// If the flags arg was none of the above two, then return 0
		return 0;

	  }

          case SYSCALL_SHRINK:
	  {
		  u32 size = (u32) param1;
		  int flags = (int) param2;

		  if (size < 0)
			  return 0;

		  unsigned long data_next = current->mms[MM_SEG_DATA].next_free;
		  unsigned long data_start = current->mms[MM_SEG_DATA].start;

		  unsigned long rodata_next = current->mms[MM_SEG_RODATA].next_free;
		  unsigned long rodata_start = current->mms[MM_SEG_RODATA].start;

		  if (flags == MAP_WR) {
			  if (data_next - (size * 4096) < data_start)
				  return 0;
			  current->mms[MM_SEG_DATA].next_free -= size * 4096;
			  return current->mms[MM_SEG_DATA].next_free;
		  }
		  else if (flags == MAP_RD) {
			  if (rodata_next - (size * 4096) < rodata_start)
				  return 0;
			  current->mms[MM_SEG_RODATA].next_free -= size * 4096;
			  return current->mms[MM_SEG_RODATA].next_free;
		  }

		  // If the flags arg was none of the above two, then return 0
		  return 0;
	  }

          default:
                              return -1;

    }
    return 0;   /*GCC shut up!*/
}

extern int handle_div_by_zero(void)
{
	unsigned long stackptr;
	asm volatile(
	     "mov 8(%%rsp), %0"
	     :"=r" (stackptr)
	     :
	     :"memory"
	);
	printf("Div-by-zero detected at %x\n", stackptr);
	do_exit();
}

extern int handle_page_fault(void)
{
    /*Your code goes in here*/
    printf("page fault handler: unimplemented!\n");
    return 0;
}
