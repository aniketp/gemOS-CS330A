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
			return list[i];
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
			ticks_to_sleep--;
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
	if (current->ticks_to_alarm) {
		current->ticks_to_alarm--;
		if (current->ticks_to_alarm == 0)
			invoke_sync_signal(SIGALRM, baseptr, baseptr+1);
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

	struct exec_context *current = get_current_ctx();
	struct exec_context *list = get_ctx_list();
	current->state = UNUSED;

	bool flag = false;
	for (int i = 0; i < MAX_PROCESSES; i++)
		if (list[i].state == READY)
			schedule();

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

}

long invoke_sync_signal(int signo, u64 *ustackp, u64 *urip)
{
	struct exec_context *current = get_current_ctx();
	if (current->sighandlers[signo] != NULL);

   /*If signal handler is registered, manipulate user stack and RIP to execute signal handler*/
   /*ustackp and urip are pointers to user RSP and user RIP in the exception/interrupt stack*/
   printf("Called signal with ustackp=%x urip=%x\n", *ustackp, *urip);
   /*Default behavior is exit( ) if sighandler is not registered for SIGFPE or SIGSEGV.
    Ignore for SIGALRM*/
    if(signo != SIGALRM)
      do_exit();
}
/*system call handler for signal, to register a handler*/
long do_signal(int signo, unsigned long handler)
{
	struct exec_context *current = get_current_ctx();
	current->sighandlers[signo] = handler;
	return 0;
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
