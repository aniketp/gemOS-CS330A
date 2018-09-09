/*
 * AUTHOR : Aniket Pandey <aniketp@iitk.ac.in>	(160113)
 */

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

#define pr             0x001             /* Present Bit */
#define rw             0x002             /* Write Bit: Not for Code Segment */
#define us             0x004             /* User Mode */

/*System Call handler*/
unsigned long do_syscall(int syscall, u64 param1, u64 param2, u64 param3, u64 param4)
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
		unsigned long stack_start = current->mms[MM_SEG_STACK].start;
		unsigned long stack_end = current->mms[MM_SEG_STACK].end;

		// Check for proper VA to PA mapping
		unsigned long address = (unsigned long) param1;
		unsigned long *l4addr = osmap(current->pgd);
		unsigned long *l3addr, *l2addr, *l1addr;

		u32 offsetL4, offsetL3, offsetL2, offsetL1;
		u32 pfnL1, pfnL2, pfnL3;

		/* Extract out all 4 level page offsets */
		offsetL4 = PToffset & (address >> L4offset);
		offsetL3 = PToffset & (address >> L3offset);
		offsetL2 = PToffset & (address >> L2offset);
		offsetL1 = PToffset & (address >> L1offset);

#if 0
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
#endif
		// printf("%x\n", param1);
		// printf("%x\n", stack_start);
		// printf("%x\n", stack_end);

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

		  unsigned long address = (unsigned long) param1;
		  unsigned long *l4addr = osmap(current->pgd);
		  unsigned long *l3addr, *l2addr, *l1addr;

		  u32 offsetL4, offsetL3, offsetL2, offsetL1;
		  u32 pfnL1, pfnL2, pfnL3, dataPFN;

		  unsigned long data_next = current->mms[MM_SEG_DATA].next_free;
		  unsigned long data_start = current->mms[MM_SEG_DATA].start;

		  unsigned long rodata_next = current->mms[MM_SEG_RODATA].next_free;
		  unsigned long rodata_start = current->mms[MM_SEG_RODATA].start;

		  if (flags == MAP_WR) {
			  if (data_next - (size * 4096) < data_start)
				  return 0;

			for (int i = 0; i < size; i++) {
				current->mms[MM_SEG_DATA].next_free -= 4096;
				address = current->mms[MM_SEG_DATA].next_free;

				/* Extract out all 4 level page offsets */
				offsetL4 = PToffset & (address >> L4offset);
				offsetL3 = PToffset & (address >> L3offset);
				offsetL2 = PToffset & (address >> L2offset);
				offsetL1 = PToffset & (address >> L1offset);

				/* Cleanup Page Table entries */
				if (*(l4addr + offsetL4) & 1) {
					pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
					l3addr = osmap(pfnL3);
				}

				if (*(l3addr + offsetL3) & 1) {
					pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
					l2addr = osmap(pfnL2);
				}

				if (*(l2addr + offsetL2) & 1) {
					pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
					l1addr = osmap(pfnL1);
				}

				if (*(l1addr + offsetL1) & 1) {
					dataPFN = *(l1addr + offsetL1) >> PTEoffset;
					// Set the present bit as 0
					*(l1addr + offsetL1) &= ~1;
				}

				// Free the data page
				os_pfn_free(USER_REG, dataPFN);


			}

			return current->mms[MM_SEG_DATA].next_free;
		  }
		  else if (flags == MAP_RD) {
			  if (rodata_next - (size * 4096) < rodata_start)
				  return 0;

			  for (int i = 0; i < size; i++) {
				  current->mms[MM_SEG_RODATA].next_free -= 4096;
				  address = current->mms[MM_SEG_RODATA].next_free;

				  /* Extract out all 4 level page offsets */
				  offsetL4 = PToffset & (address >> L4offset);
				  offsetL3 = PToffset & (address >> L3offset);
				  offsetL2 = PToffset & (address >> L2offset);
				  offsetL1 = PToffset & (address >> L1offset);

				  /* Cleanup Page Table entries */
				  if (*(l4addr + offsetL4) & 1) {
					  pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
					  l3addr = osmap(pfnL3);
				  }

				  if (*(l3addr + offsetL3) & 1) {
					  pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
					  l2addr = osmap(pfnL2);
				  }

				  if (*(l2addr + offsetL2) & 1) {
					  pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
					  l1addr = osmap(pfnL1);
				  }

				  if (*(l1addr + offsetL1) & 1) {
					  dataPFN = *(l1addr + offsetL1) >> PTEoffset;
					  // Set the present bit as 0
					  *(l1addr + offsetL1) &= ~1;
				  }

				  // Free the data page
				  os_pfn_free(USER_REG, dataPFN);

			  }
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
	unsigned long *baseptr;
	asm volatile(
	     "mov %%rbp, %0"
	     :"=r" (baseptr)
	     :
	     :"memory"
	);
	printf("Div-by-zero detected at %x\n", *(baseptr + 1));
	do_exit();
}

extern int handle_page_fault(void)
{
	unsigned long fault, *baseptr;
	asm volatile(
    	     "mov %%cr2, %0"
    	     :"=r" (fault)
    	     :
    	     :"memory"
    	);

	asm volatile(
    	     "mov %%rbp, %0"
    	     :"=r" (baseptr)
    	     :
    	     :"memory"
    	);

	// TODO: Extract out the WRITE bit
	unsigned long *error = baseptr + 1;
	unsigned long *insptr = baseptr + 2;
	u32 write_bit = (*error >> 1) & 1;

	struct exec_context *current = get_current_ctx();

	// Store address limits of all segments
	unsigned long data_end = current->mms[MM_SEG_DATA].end;
	unsigned long data_next = current->mms[MM_SEG_DATA].next_free;
	unsigned long data_start = current->mms[MM_SEG_DATA].start;

	unsigned long rodata_end = current->mms[MM_SEG_RODATA].end;
	unsigned long rodata_next = current->mms[MM_SEG_RODATA].next_free;
	unsigned long rodata_start = current->mms[MM_SEG_RODATA].start;

	unsigned long stack_end = current->mms[MM_SEG_STACK].end;
	unsigned long stack_start = current->mms[MM_SEG_STACK].start;

	// Reuse this
	unsigned long address = fault;
	unsigned long *l4addr = osmap(current->pgd);
	unsigned long *l3addr, *l2addr, *l1addr;

	u32 offsetL4, offsetL3, offsetL2, offsetL1;
	u32 pfnL1, pfnL2, pfnL3, dataPFN;

	/* Extract out all 4 level page offsets */
	offsetL4 = PToffset & (address >> L4offset);
	offsetL3 = PToffset & (address >> L3offset);
	offsetL2 = PToffset & (address >> L2offset);
	offsetL1 = PToffset & (address >> L1offset);


	/**************** Data Segment *******************/

	if (fault <= data_end && fault >= data_start) {
		if (fault > data_next) {
			printf("Virtual address not b/w Data start and next_free\n");
			printf("Accessed Address: %x\nRIP: %x\nError Code: &x\n",
				fault, *insptr, *error);
			do_exit();
		}
		else {

			/* Fill up Page Table Pages entries */
		        if (*(l4addr + offsetL4) & 1)        // If PT already present
		                pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
		        else {
		                pfnL3 = os_pfn_alloc(OS_PT_REG);
		                *(l4addr + offsetL4) = (pfnL3 << PTEoffset) | (pr | rw | us);
		        }

		        l3addr = osmap(pfnL3);
		        if (*(l3addr + offsetL3) & 1)        // If PT already present
		                pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
		        else {
		                pfnL2 = os_pfn_alloc(OS_PT_REG);
		                *(l3addr + offsetL3) = (pfnL2 << PTEoffset) | (pr | rw | us);
		        }

		        l2addr = osmap(pfnL2);
		        if (*(l2addr + offsetL2) & 1)        // If PT already present
		                pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
		        else {
		                pfnL1 = os_pfn_alloc(OS_PT_REG);
		                *(l2addr + offsetL2) = (pfnL1 << PTEoffset) | (pr | rw | us);
		        }

		        /* Final mapping to Data Physical Page */
		        l1addr = osmap(pfnL1);
		        if (*(l1addr + offsetL1) & 1)        // If PT already present
		                dataPFN = *(l1addr + offsetL1) >> PTEoffset;
		        else {
		                dataPFN = os_pfn_alloc(USER_REG);
		                *(l1addr + offsetL1) = (dataPFN << PTEoffset) | (pr | rw | us);
		        }

		}
	}


	/**************** Code Segment *******************/

	else if (fault <= rodata_end && fault >= rodata_start) {
		if (fault > rodata_next) {
			printf("Virtual address not b/w RODATA start and next_free\n");
			printf("Accessed Address: %x\nRIP: %x\nError Code: &x\n",
				fault, *insptr, *error);
			do_exit();
		}
		else if (write_bit == 1) {
			printf("Write Access not allowed for CODE segment\n");
			printf("Accessed Address: %x\nRIP: %x\nError Code: &x\n",
				fault, *insptr, *error);
			do_exit();
		}
		else {

			/* Fill up Page Table Pages entries */
		        if (*(l4addr + offsetL4) & 1)        // If PT already present
		                pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
		        else {
		                pfnL3 = os_pfn_alloc(OS_PT_REG);
		                *(l4addr + offsetL4) = (pfnL3 << PTEoffset) | (pr | us);
		        }

		        l3addr = osmap(pfnL3);
		        if (*(l3addr + offsetL3) & 1)        // If PT already present
		                pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
		        else {
		                pfnL2 = os_pfn_alloc(OS_PT_REG);
		                *(l3addr + offsetL3) = (pfnL2 << PTEoffset) | (pr | us);
		        }

		        l2addr = osmap(pfnL2);
		        if (*(l2addr + offsetL2) & 1)        // If PT already present
		                pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
		        else {
		                pfnL1 = os_pfn_alloc(OS_PT_REG);
		                *(l2addr + offsetL2) = (pfnL1 << PTEoffset) | (pr | us);
		        }

		        /* Final mapping to Data Physical Page */
		        l1addr = osmap(pfnL1);
		        if (*(l1addr + offsetL1) & 1)        // If PT already present
		                dataPFN = *(l1addr + offsetL1) >> PTEoffset;
		        else {
		                dataPFN = os_pfn_alloc(USER_REG);
		                *(l1addr + offsetL1) = (dataPFN << PTEoffset) | (pr | us);
		        }
		}
	}


	/**************** Stack Segment *******************/

	else if (fault <= stack_end && fault >= stack_start) {

		/* Fill up Page Table Pages entries */
	        if (*(l4addr + offsetL4) & 1)        // If PT already present
	                pfnL3 = *(l4addr + offsetL4) >> PTEoffset;
	        else {
	                pfnL3 = os_pfn_alloc(OS_PT_REG);
	                *(l4addr + offsetL4) = (pfnL3 << PTEoffset) | (pr | rw | us);
	        }

	        l3addr = osmap(pfnL3);
	        if (*(l3addr + offsetL3) & 1)        // If PT already present
	                pfnL2 = *(l3addr + offsetL3) >> PTEoffset;
	        else {
	                pfnL2 = os_pfn_alloc(OS_PT_REG);
	                *(l3addr + offsetL3) = (pfnL2 << PTEoffset) | (pr | rw | us);
	        }

	        l2addr = osmap(pfnL2);
	        if (*(l2addr + offsetL2) & 1)        // If PT already present
	                pfnL1 = *(l2addr + offsetL2) >> PTEoffset;
	        else {
	                pfnL1 = os_pfn_alloc(OS_PT_REG);
	                *(l2addr + offsetL2) = (pfnL1 << PTEoffset) | (pr | rw | us);
	        }

	        /* Final mapping to Data Physical Page */
	        l1addr = osmap(pfnL1);
	        if (*(l1addr + offsetL1) & 1)        // If PT already present
	                dataPFN = *(l1addr + offsetL1) >> PTEoffset;
	        else {
	                dataPFN = os_pfn_alloc(USER_REG);
	                *(l1addr + offsetL1) = (dataPFN << PTEoffset) | (pr | rw | us);
	        }

	}

	// Else: The virtual address is nowhere to be found :P
	else {
		printf("Virtual address not in range of any segment\n");
		printf("Accessed Address: %x\nRIP: %x\nError Code: &x\n",
			fault, *insptr, *error);
		do_exit();
	}

	printf("PF Handler invoked %x\n", fault);

	asm volatile(
    	     "mov %0, %%rsp;"
	     "iretq;"
    	     :"=r" (insptr)
    	     :
	     :"memory"
    	);

	do_exit();
}
