#include<context.h>
#include<memory.h>
#include<schedule.h>
#include<apic.h>
#include<lib.h>
#include<idt.h>

static u64 numticks;

static void save_current_context()
{
	struct exec_context *current = get_current_ctx();
	asm volatile(
		"mov %%r15, %0"
		:"=r" (current->regs.r15)
		::"memory"
	);

	asm volatile(
		"mov %%r14, %0"
		:"=r" (current->regs.r14)
		::"memory"
	);

	asm volatile(
		"mov %%r13, %0"
		:"=r" (current->regs.r13)
		::"memory"
	);

	asm volatile(
		"mov %%r12, %0"
		:"=r" (current->regs.r12)
		::"memory"
	);

	asm volatile(
		"mov %%r11, %0"
		:"=r" (current->regs.r11)
		::"memory"
	);

	asm volatile(
		"mov %%r10, %0"
		:"=r" (current->regs.r10)
		::"memory"
	);

	asm volatile(
		"mov %%r9, %0"
		:"=r" (current->regs.r9)
		::"memory"
	);

	asm volatile(
		"mov %%r8, %0"
		:"=r" (current->regs.r8)
		::"memory"
	);

	asm volatile(
		"mov %%rbp, %0"
		:"=r" (current->regs.rbp)
		::"memory"
	);

	asm volatile(
		"mov %%rdi, %0"
		:"=r" (current->regs.rdi)
		::"memory"
	);

	asm volatile(
		"mov %%rsi, %0"
		:"=r" (current->regs.rsi)
		::"memory"
	);

	asm volatile(
		"mov %%rdx, %0"
		:"=r" (current->regs.rdx)
		::"memory"
	);
}

static void schedule_context(struct exec_context *next)
{
  /*Your code goes in here. get_current_ctx() still returns the old context*/
 struct exec_context *current = get_current_ctx();
 printf("scheduling: old pid = %d  new pid  = %d\n", current->pid, next->pid); /*XXX: Don't remove*/
/*These two lines must be executed*/
 set_tss_stack_ptr(next);
 set_current_ctx(next);
/*Your code for scheduling context*/
 return;
}

static struct exec_context *pick_next_context(struct exec_context *list)
{
	for (int i = 0; i < MAX_PROCESSES; i++)
		if (list[i].state == READY)
			return &list[i];
	return NULL;
}

// Scheduling routine
static void schedule()
{
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
}

/*The five functions above are just a template. You may change the signatures as you wish*/
void handle_timer_tick()
{
 /*
   This is the timer interrupt handler.
   You should account timer ticks for alarm and sleep
   and invoke schedule
 */
	u64 *baseptr;
	asm volatile(
		"mov %%rbp, %0"
		:"=r" (baseptr)
		::"memory"
	);

	struct exec_context *current = get_current_ctx();
	struct exec_context *swapper = get_ctx_by_pid(0);

	if (swapper->state == RUNNING) {
		// Check the ticks to sleep of stored process and if requires
		// rescheduling, do that.
	}

	if (current->ticks_to_alarm) {
		current->ticks_to_alarm--;
		if (!(current->ticks_to_alarm) && (current->sighandlers[SIGALRM] != NULL)) {
			invoke_sync_signal(SIGALRM, baseptr+4, baseptr+1);
		}
	}

  asm volatile("cli;"
                :::"memory");
  printf("Got a tick. #ticks = %u\n", numticks++);   /*XXX Do not modify this line*/
  ack_irq();  /*acknowledge the interrupt, before calling iretq */
  asm volatile("mov %%rbp, %%rsp;"
               "pop %%rbp;"
               "iretq;"
               :::"memory");
}

void do_exit()
{
  /*You may need to invoke the scheduler from here if there are
    other processes except swapper in the system. Make sure you make
    the status of the current process to UNUSED before scheduling
    the next process. If the only process alive in system is swapper,
    invoke do_cleanup() to shutdown gem5 (by crashing it, huh!)
    */
	//
	// struct exec_context *current = get_current_ctx();
	// struct exec_context *list = get_ctx_list();
	// current->state = UNUSED;
	//
	// int flag = 0;
	// for (int i = 0; i < MAX_PROCESSES; i++)
	// 	if (list[i].state == READY)
	// 		schedule();

	do_cleanup();  /*Call this conditionally, see comments above*/
}

/*system call handler for sleep*/
long do_sleep(u32 ticks)
{
	struct exec_context *current = get_current_ctx();
	struct exec_context *swapper = get_ctx_by_pid(0);

	current->ticks_to_sleep = ticks;
	current->state = WAITING;
	save_current_context();
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
	child->used_mem = current->used_mem;
	child->pgd = current->pgd;
	child->os_rsp = current->os_rsp;
	child->ticks_to_sleep = current->ticks_to_sleep;
	child->ticks_to_alarm = current->ticks_to_alarm;
	child->alarm_config_time = current->alarm_config_time;

	// Signal handlers are also copied
	for (int i = 0; i < MAX_SIGNALS; i++)
		child->sighandlers[i] = current->sighandlers[i];
	// But not the pending signals
	child->pending_signal_bitmap = 0;

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

	child->regs.entry_cs = 0x2b;
	child->regs.entry_ss = 0x23;
	child->entry_rip = (u64)th_func;

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

	if(signo != SIGALRM && (current->sighandlers[signo] == NULL))
		do_exit();

	// Main routine
	if (current->sighandlers[signo] != NULL) {
		*(u64 *)(*ustackp) = *urip + 4;		// Confirm whether 4
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
