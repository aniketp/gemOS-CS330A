#include<context.h>
#include<memory.h>
#include<schedule.h>
#include<apic.h>
#include<lib.h>
#include<idt.h>

static u64 numticks;

static void save_current_context(struct exec_context *current,
	struct exec_context *next, u64 *rrsp, u64 *rrbp)
{
	// Save the current context GPRs from stack
	current->regs.rdi = *(rrsp + 0);
	current->regs.rsi = *(rrsp + 1);
	current->regs.rdx = *(rrsp + 2);
	current->regs.rcx = *(rrsp + 3);
	current->regs.rbx = *(rrsp + 4);
	current->regs.rax = *(rrsp + 5);
	current->regs.r15 = *(rrsp + 6);
	current->regs.r14 = *(rrsp + 7);
	current->regs.r13 = *(rrsp + 8);
	current->regs.r12 = *(rrsp + 9);
	current->regs.r11 = *(rrsp + 10);
	current->regs.r10 = *(rrsp + 11);
	current->regs.r9 = *(rrsp + 12);
	current->regs.r8 = *(rrsp + 13);

	current->regs.rbp = *(rrbp + 0);
	current->regs.entry_rip = *(rrbp + 1);
	current->regs.entry_cs = *(rrbp + 2);
	current->regs.entry_rflags = *(rrbp + 3);
	current->regs.entry_rsp = *(rrbp + 4);
	current->regs.entry_ss = *(rrbp + 5);

	// Set the GPRs of next context to stack
	*(rrsp + 0) = next->regs.rdi;
	*(rrsp + 1) = next->regs.rsi;
	*(rrsp + 2) = next->regs.rdx;
	*(rrsp + 3) = next->regs.rcx;
	*(rrsp + 4) = next->regs.rbx;
	*(rrsp + 5) = next->regs.rax;
	*(rrsp + 6) = next->regs.r15;
	*(rrsp + 7) = next->regs.r14;
	*(rrsp + 8) = next->regs.r13;
	*(rrsp + 9) = next->regs.r12;
	*(rrsp + 10) = next->regs.r11;
	*(rrsp + 11) = next->regs.r10;
	*(rrsp + 12) = next->regs.r9;
	*(rrsp + 13) = next->regs.r8;

	*(rrbp + 0) = next->regs.rbp;
	*(rrbp + 1) = next->regs.entry_rip;
	*(rrbp + 2) = next->regs.entry_cs;
	*(rrbp + 3) = next->regs.entry_rflags;
	*(rrbp + 4) = next->regs.entry_rsp;
	*(rrbp + 5) = next->regs.entry_ss;
}

static void schedule_context(struct exec_context *next)
{
	 /*Your code goes in here. get_current_ctx() still returns the old context*/
	 struct exec_context *current = get_current_ctx();
	 printf("scheduling: old pid = %d  new pid  = %d\n",
	 current->pid, next->pid); /*XXX: Don't remove*/
	/*These two lines must be executed*/
	 set_tss_stack_ptr(next);
	 set_current_ctx(next);
	/*Your code for scheduling context*/
	 return;
}

static struct exec_context *pick_next_context(struct exec_context *list)
{
	struct exec_context *current = get_current_ctx();
	u32 cpid = current->pid;

	for (int i = cpid + 1; i <= cpid + MAX_PROCESSES; i++)
		if (((list + (i % MAX_PROCESSES))->state == READY) && (i != MAX_PROCESSES))
			return (list + (i % MAX_PROCESSES));

	// Return the swapper
	return list;
}

// Scheduling routine
static void schedule()
{
	printf("reached schedule() function\n");
	struct exec_context *next;
	struct exec_context *current = get_current_ctx();
	struct exec_context *list = get_ctx_list();
	next = pick_next_context(list);
	schedule_context(next);
}

static void do_sleep_and_alarm_account()
{
 /*All processes in sleep() must decrement their sleep count*/
	struct exec_context *list = get_ctx_list();

	for (int i = 0; i < MAX_PROCESSES; i++)
		if (list[i].state == WAITING && list[i].ticks_to_sleep != 0)
			list[i].ticks_to_sleep--;

	for (int i = 0; i < MAX_PROCESSES; i++)
		if (list[i].state == RUNNING && list[i].ticks_to_alarm != 0)
			list[i].ticks_to_alarm--;

	return;
}

/*The five functions above are just a template. You may change the signatures as you wish*/
void handle_timer_tick()
{
	asm volatile(
		"push %%r8;"
		"push %%r9;"
		"push %%r10;"
		"push %%r11;"
		"push %%r12;"
		"push %%r13;"
		"push %%r14;"
		"push %%r15;"
		"push %%rax;"
		"push %%rbx;"
		"push %%rcx;"
		"push %%rdx;"
		"push %%rsi;"
		"push %%rdi;"
		:::"memory"
	);
 /*
   This is the timer interrupt handler.
   You should account timer ticks for alarm and sleep
   and invoke schedule
 */

	u64 *saved_stk;
	asm volatile(
		"mov %%rsp, %0"
		:"=r" (saved_stk)
		::"memory"
	);

	u64 *baseptr;
	asm volatile(
		"mov %%rbp, %0"
		:"=r" (baseptr)
		::"memory"
	);

	struct exec_context *current = get_current_ctx();
	struct exec_context *list = get_ctx_list();
	struct exec_context *swapper = get_ctx_by_pid(0);

	for(int i = 1; i < MAX_PROCESSES; i++) {
		if(((list + i)->state == WAITING) && ((list + i)->ticks_to_sleep > 0)) {
			(list + i)->ticks_to_sleep--;
			if ((list + i)->ticks_to_sleep == 0)
				(list + i)->state = READY;
		}
	}

	if (current->ticks_to_alarm) {
		current->ticks_to_alarm--;
		if (!(current->ticks_to_alarm) && (current->sighandlers[SIGALRM] != NULL)) {
			current->ticks_to_alarm = current->alarm_config_time;
			invoke_sync_signal(SIGALRM, baseptr+4, baseptr+1);
		}
	}

	asm volatile("cli;"
	        :::"memory");
	printf("Got a tick. #ticks = %u\n", numticks++);   /*XXX Do not modify this line*/
	ack_irq();  /*acknowledge the interrupt, before calling iretq */
	asm volatile (  "mov %0, %%rsp;"
		"pop %%rdi;"
		"pop %%rsi;"
		"pop %%rdx;"
		"pop %%rcx;"
		"pop %%rbx;"
		"pop %%rax;"
		"pop %%r15;"
		"pop %%r14;"
		"pop %%r13;"
		"pop %%r12;"
		"pop %%r11;"
		"pop %%r10;"
		"pop %%r9;"
		"pop %%r8;"
		"mov %%rbp, %%rsp;"
		"pop %%rbp;"
		"iretq;"
		:: "r" (saved_stk)
		: "memory"
	);
}

void do_exit()
{
  /*You may need to invoke the scheduler from here if there are
    other processes except swapper in the system. Make sure you make
    the status of the current process to UNUSED before scheduling
    the next process. If the only process alive in system is swapper,
    invoke do_cleanup() to shutdown gem5 (by crashing it, huh!)
    */

	struct exec_context *current = get_current_ctx();
	struct exec_context *list = get_ctx_list();
	os_pfn_free(OS_PT_REG, current->os_stack_pfn);
	current->state = UNUSED;

	for (int i = 1; i < MAX_PROCESSES; i++)
		if ((list + i)->state == READY)
			schedule();

	do_cleanup();  /*Call this conditionally, see comments above*/
}

/*system call handler for sleep*/
long do_sleep(u32 ticks)
{
	struct exec_context *current = get_current_ctx();
	struct exec_context *swapper = get_ctx_by_pid(0);	// Confirm whether swapper

	current->ticks_to_sleep = ticks;
	current->state = WAITING;

	// Modify the states for swapper
	swapper->state = RUNNING;

	// Do we need to do this?
	set_tss_stack_ptr(swapper);
	set_current_ctx(swapper);



	// save_current_context();
	schedule();
}

/*
  system call handler for clone, create thread like
  execution contexts
*/
long do_clone(void *th_func, void *user_stack)
{
	struct exec_context *current = get_current_ctx();
	struct exec_context *child = get_new_ctx();

	/* Either copy from the parent or initialize according to the question */
	printf("New PID: %d\n", child->pid);
	int len = strlen(current->name);
	// char cpid[1];

	child->os_stack_pfn = os_pfn_alloc(OS_PT_REG);
	memcpy(child->name, current->name, strlen(current->name));
	// snprintf(cpid, "%d", current->pid, 1);
	// memcpy(child->name + strlen(current->name), cpid, strlen(cpid));
	// TODO: Fix this
	printf("Child name: %s\n", child->name);

	// Copy from parent
	child->pgd = current->pgd;
	child->state = READY;			// Ready to be scheduled
	child->os_rsp = current->os_rsp;
	child->used_mem = current->used_mem;
	child->ticks_to_sleep = current->ticks_to_sleep;
	child->ticks_to_alarm = current->ticks_to_alarm;
	child->alarm_config_time = current->alarm_config_time;

	// Signal handlers are also copied
	for (int i = 0; i < MAX_SIGNALS; i++)
		child->sighandlers[i] = current->sighandlers[i];
	// But not the pending signals
	child->pending_signal_bitmap = 0x0;

	// Now, copy the user_regs registers
	child->regs.r15 = current->regs.r15;
	child->regs.r14 = current->regs.r14;
	child->regs.r13 = current->regs.r13;
	child->regs.r12 = current->regs.r12;
	child->regs.r11 = current->regs.r11;
	child->regs.r10 = current->regs.r10;
	child->regs.r9 = current->regs.r9;
	child->regs.r8 = current->regs.r8;

	child->regs.rax = current->regs.rax;
	child->regs.rbx = current->regs.rbx;
	child->regs.rcx = current->regs.rcx;
	child->regs.rdx = current->regs.rdx;
	child->regs.rdi = current->regs.rdi;
	child->regs.rsi = current->regs.rsi;

	child->mms[MM_SEG_CODE].start = current->mms[MM_SEG_CODE].start;
	child->mms[MM_SEG_CODE].end = current->mms[MM_SEG_CODE].end;
	child->mms[MM_SEG_CODE].next_free = current->mms[MM_SEG_CODE].next_free;
	child->mms[MM_SEG_CODE].access_flags = current->mms[MM_SEG_CODE].access_flags;

	child->mms[MM_SEG_DATA].start = current->mms[MM_SEG_DATA].start;
	child->mms[MM_SEG_DATA].end = current->mms[MM_SEG_DATA].end;
	child->mms[MM_SEG_DATA].next_free = current->mms[MM_SEG_DATA].next_free;
	child->mms[MM_SEG_DATA].access_flags = current->mms[MM_SEG_DATA].access_flags;

	child->mms[MM_SEG_RODATA].start = current->mms[MM_SEG_RODATA].start;
	child->mms[MM_SEG_RODATA].end = current->mms[MM_SEG_RODATA].end;
	child->mms[MM_SEG_RODATA].next_free = current->mms[MM_SEG_RODATA].next_free;
	child->mms[MM_SEG_RODATA].access_flags = current->mms[MM_SEG_RODATA].access_flags;

	child->mms[MM_SEG_STACK].start = current->mms[MM_SEG_STACK].start;
	child->mms[MM_SEG_STACK].end = current->mms[MM_SEG_STACK].end;
	child->mms[MM_SEG_STACK].next_free = current->mms[MM_SEG_STACK].next_free;
	child->mms[MM_SEG_STACK].access_flags = current->mms[MM_SEG_STACK].access_flags;

	child->regs.entry_cs = 0x2b;
	child->regs.entry_ss = 0x23;
	child->regs.entry_rflags = current->regs.entry_rflags;
	child->regs.entry_rip = (u64)th_func;
	child->regs.entry_rsp = (u64)user_stack;
	child->regs.rbp = (u64)user_stack;	// TODO: Check the validity

	return 0;
}

long invoke_sync_signal(int signo, u64 *ustackp, u64 *urip)
{

/*If signal handler is registered, manipulate user stack and RIP to execute signal handler*/
/*ustackp and urip are pointers to user RSP and user RIP in the exception/interrupt stack*/
/*Default behavior is exit( ) if sighandler is not registered for SIGFPE or SIGSEGV.
Ignore for SIGALRM*/

    	struct exec_context *current = get_current_ctx();
	printf("Called signal with ustackp=%x urip=%x\n", *ustackp, *urip);

	if((signo != SIGALRM) && (current->sighandlers[signo] == NULL))
		do_exit();

	// Main routine
	if (current->sighandlers[signo] != NULL) {
		*ustackp = *ustackp - 1;
		*(u64 *)(*ustackp) = *urip;
		*urip = (u64)current->sighandlers[signo];
	}

	return 0;
}

/*system call handler for signal, to register a handler*/
long do_signal(int signo, unsigned long handler)
{
	struct exec_context *current = get_current_ctx();
	current->sighandlers[signo] = (void *)handler;
	return 0; // TODO: Check the exact return value
}

/* system call handler for alarm */
long do_alarm(u32 ticks)
{
	struct exec_context *current = get_current_ctx();
	long retval = current->ticks_to_alarm;
	current->alarm_config_time = ticks;
	current->ticks_to_alarm = ticks;
	return retval;
}
