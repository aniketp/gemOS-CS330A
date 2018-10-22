#include<context.h>
#include<init.h>
#include<memory.h>
#include<schedule.h>
#include<apic.h>
#include<lib.h>
#include<idt.h>

static u64 numticks;

static void switch_context_timer(struct exec_context *current,
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

	return;
}


static void switch_context_sleep(struct exec_context *current,
	struct exec_context *next, u64 *rrbp)
{
	// Save the current context GPRs from stack  (redundant comment, LOL)
	current->regs.rbp = *(rrbp + 10);
	current->regs.rdi = *(rrbp + 11);
	current->regs.rsi = *(rrbp + 12);
	current->regs.rdx = *(rrbp + 13);
	current->regs.rcx = *(rrbp + 14);
	current->regs.rbx = *(rrbp + 15);

	current->regs.r15 = *(rrbp + 2);
	current->regs.r14 = *(rrbp + 3);
	current->regs.r13 = *(rrbp + 4);
	current->regs.r12 = *(rrbp + 5);
	current->regs.r11 = *(rrbp + 6);
	current->regs.r10 = *(rrbp + 7);
	current->regs.r9 = *(rrbp + 8);
	current->regs.r8 = *(rrbp + 9);

	current->regs.entry_rip = *(rrbp + 16);
	current->regs.entry_cs = *(rrbp + 17);
	current->regs.entry_rflags = *(rrbp + 18);
	current->regs.entry_rsp = *(rrbp + 19);
	current->regs.entry_ss = *(rrbp + 20);

	// Set the GPRs of next context to stack
	*(rrbp + 10) = next->regs.rbp;
	*(rrbp + 11) = next->regs.rdi;
	*(rrbp + 12) = next->regs.rsi;
	*(rrbp + 13) = next->regs.rdx;
	*(rrbp + 14) = next->regs.rcx;
	*(rrbp + 15) = next->regs.rbx;

	*(rrbp + 2) = next->regs.r15;
	*(rrbp + 3) = next->regs.r14;
	*(rrbp + 4) = next->regs.r13;
	*(rrbp + 5) = next->regs.r12;
	*(rrbp + 6) = next->regs.r11;
	*(rrbp + 7) = next->regs.r10;
	*(rrbp + 8) = next->regs.r9;
	*(rrbp + 9) = next->regs.r8;

	*(rrbp + 16) = next->regs.entry_rip;
	*(rrbp + 17) = next->regs.entry_cs;
	*(rrbp + 18) = next->regs.entry_rflags;
	*(rrbp + 19) = next->regs.entry_rsp;
	*(rrbp + 20) = next->regs.entry_ss;

	return;
}

static void switch_context_exit(struct exec_context *next, u64 *rrbp) {

	*(rrbp) = next->regs.entry_ss;
	*(rrbp - 1) = next->regs.entry_rsp;
	*(rrbp - 2) = next->regs.entry_rflags;
	*(rrbp - 3) = next->regs.entry_cs;
	*(rrbp - 4) = next->regs.entry_rip;

	*(rrbp - 5) = next->regs.rbx;
	*(rrbp - 6) = next->regs.rcx;
	*(rrbp - 7) = next->regs.rdx;
	*(rrbp - 8) = next->regs.rsi;
	*(rrbp - 9) = next->regs.rdi;
	*(rrbp - 10) = next->regs.rbp;

	*(rrbp - 11) = next->regs.r8;
	*(rrbp - 12) = next->regs.r9;
	*(rrbp - 13) = next->regs.r10;
	*(rrbp - 14) = next->regs.r11;
	*(rrbp - 15) = next->regs.r12;
	*(rrbp - 16) = next->regs.r13;
	*(rrbp - 17) = next->regs.r14;
	*(rrbp - 18) = next->regs.r15;
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
		"mov %%rsp, %0;"
		:"=r" (saved_stk)
		::"memory"
	);

	u64 *baseptr;
	asm volatile(
		"mov %%rbp, %0;"
		:"=r" (baseptr)
		::"memory"
	);

	struct exec_context *current = get_current_ctx();
	struct exec_context *list = get_ctx_list();
	struct exec_context *swapper = get_ctx_by_pid(0);

	int process_found = 0;
	for(int i = 1; i < MAX_PROCESSES; i++) {
		if(((list + i)->state == WAITING) && ((list + i)->ticks_to_sleep > 0)) {
			(list + i)->ticks_to_sleep--;
			if ((list + i)->ticks_to_sleep == 0)
				(list + i)->state = READY;
		}

		if (i && (list + i)->state == READY && i != current->pid)
			process_found = 1;
	}

	if (current->ticks_to_alarm && current->alarm_config_time) {
		current->ticks_to_alarm--;
		if (!(current->ticks_to_alarm) && (current->sighandlers[SIGALRM] != NULL)) {
			current->ticks_to_alarm = current->alarm_config_time;
			invoke_sync_signal(SIGALRM, baseptr + 4, baseptr + 1);
		}
	}

	asm volatile("cli;"
	        :::"memory");
	printf("Got a tick. #ticks = %u\n", numticks++);   /*XXX Do not modify this line*/

	// We found a process, schedule it while the timer interrupt arrives
	if (process_found) {
		struct exec_context *next = pick_next_context(list);
		printf("scheduling: old pid = %d  new pid  = %d\n",
		current->pid, next->pid); /*XXX: Don't remove*/
		/*These two lines must be executed*/
		/*Your code for scheduling context*/

		switch_context_timer(current, next, saved_stk, baseptr);
		set_tss_stack_ptr(next);
		set_current_ctx(next);

		next->state = RUNNING;
		current->state = READY;
	}

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

static void exit_helper(){
    u64* baseptr;
    asm volatile (  "mov %%rbp, %0;"
                    :"=r" (baseptr)
                    :
                    :"memory"
    );

    os_pfn_free(OS_PT_REG, *(baseptr + 2));

    asm volatile (  "mov %%rbp, %%rsp;"
                    "pop %%r15;"
                    "pop %%r15;"
                    "pop %%r15;"
                    "pop %%r15;"
                    "pop %%r14;"
                    "pop %%r13;"
                    "pop %%r12;"
                    "pop %%r11;"
                    "pop %%r10;"
                    "pop %%r9;"
                    "pop %%r8;"
                    "pop %%rbp;"
                    "pop %%rdi;"
                    "pop %%rsi;"
                    "pop %%rdx;"
                    "pop %%rcx;"
                    "pop %%rbx;"
                    "iretq;"
                    :::"memory"
    );
    return;
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
	struct exec_context *next = pick_next_context(next);

	current->state = UNUSED;

	// No other process is available, schedule the swapper
	if (next == list)
		do_cleanup();

	u64 *next_rbp = (u64 *)osmap(next->os_stack_pfn);
	u64 *next_stk = next_rbp - 19;

	// Else, schedule the other process
	printf("scheduling: old pid = %d  new pid  = %d\n",
		current->pid, next->pid);

	set_tss_stack_ptr(next);
	set_current_ctx(next);
	next->state = RUNNING;

	*(next_rbp) = next->regs.entry_ss;
	*(next_rbp - 1) = next->regs.entry_rsp;
	*(next_rbp - 2) = next->regs.entry_rflags;
	*(next_rbp - 3) = next->regs.entry_cs;
	*(next_rbp - 4) = next->regs.entry_rip;

	*(next_rbp - 5) = next->regs.rbx;
	*(next_rbp - 6) = next->regs.rcx;
	*(next_rbp - 7) = next->regs.rdx;
	*(next_rbp - 8) = next->regs.rsi;
	*(next_rbp - 9) = next->regs.rdi;
	*(next_rbp - 10) = next->regs.rbp;

	*(next_rbp - 11) = next->regs.r8;
	*(next_rbp - 12) = next->regs.r9;
	*(next_rbp - 13) = next->regs.r10;
	*(next_rbp - 14) = next->regs.r11;
	*(next_rbp - 15) = next->regs.r12;
	*(next_rbp - 16) = next->regs.r13;
	*(next_rbp - 17) = next->regs.r14;
	*(next_rbp - 18) = next->regs.r15;

	*next_stk = current->os_stack_pfn;

	os_pfn_free(OS_PT_REG, current->os_stack_pfn);
	// Set stack pointer to the top of next stack frame
	asm volatile(
		"mov %0, %%rsp;"
		::"r" (next_stk)
		:"memory"
	);

	exit_helper();
	return;
}

/*system call handler for sleep*/
long do_sleep(u32 ticks)
{
	struct exec_context *current = get_current_ctx();
	struct exec_context *list = get_ctx_list();
	struct exec_context *next = pick_next_context(list);

	current->ticks_to_sleep = ticks;
	current->state = WAITING;

	u64 *sysrbp;
	asm volatile(
		"mov %%rbp, %0;"
		:"=r" (sysrbp)
		::"memory"
	);

	switch_context_sleep(current, next, (u64 *)*sysrbp);
	printf("scheduling: old pid = %d  new pid  = %d\n", current->pid, next->pid);
	set_tss_stack_ptr(next);
	set_current_ctx(next);
	next->state = RUNNING;

	return ticks;
}

/*
  system call handler for clone, create thread like
  execution contexts
*/
long do_clone(void *th_func, void *user_stack)
{
	struct exec_context *current = get_current_ctx();
	struct exec_context *child = get_new_ctx();

	int len = strlen(current->name);
	child->os_stack_pfn = os_pfn_alloc(OS_PT_REG);
	memcpy(child->name, current->name, strlen(current->name));
	child->name[len] = '0' + child->pid;
	child->name[len+1] = 0;
	printf("Child name: %s\n", child->name);

	// Copy from parent
	child->pgd = current->pgd;
	child->state = READY;			// Ready to be scheduled
	child->os_rsp = current->os_rsp;
	child->used_mem = current->used_mem;

	// Config times initialized to 0
	child->ticks_to_sleep = 0x0;
	child->ticks_to_alarm = 0x0;
	child->alarm_config_time = 0x0;
	child->pending_signal_bitmap = 0x0;

	// Signal handlers are also copied
	for (int i = 0; i < MAX_SIGNALS; i++)
		child->sighandlers[i] = current->sighandlers[i];

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
	child->regs.rbp = (u64)user_stack;

	return child->pid;
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
