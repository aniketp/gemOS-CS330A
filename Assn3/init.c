#include<init.h>
static void exit(int);
static int main(void);


void init_start()
{
  int retval = main();
  exit(0);
}

/*Invoke system call with no additional arguments*/
static long _syscall0(int syscall_num)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

/*Invoke system call with one argument*/

static long _syscall1(int syscall_num, int exit_code)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}
/*Invoke system call with two arguments*/

static long _syscall2(int syscall_num, u64 arg1, u64 arg2)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

//////////////////////////////////////////////////////////

// alarm() system call setup
static long _syscall3(int syscall_num, u32 arg1)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

// signal() system call setup
static long _syscall4(int syscall_num, int signo, unsigned long handler)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

// clone() system call setup
static long _syscall5(int syscall_num, void *th_func, void *user_stack)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

static long _syscall6(int syscall_num, u32 ticks)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}
//////////////////////////////////////////////////////////

static void exit(int code)
{
  _syscall1(SYSCALL_EXIT, code);
}

static long getpid()
{
  return(_syscall0(SYSCALL_GETPID));
}

static long write(char *ptr, int size)
{
   return(_syscall2(SYSCALL_WRITE, (u64)ptr, size));
}

////////////////////

static long alarm(u32 ticks)
{
   return(_syscall3(SYSCALL_ALARM, ticks));
}

static long signal(int signo, unsigned long handler)
{
   return(_syscall4(SYSCALL_SIGNAL, signo, handler));
}

static long clone(void *th_func, void *user_stack)
{
   return(_syscall5(SYSCALL_CLONE, th_func, user_stack));
}

static long sleep(u32 ticks)
{
   return(_syscall6(SYSCALL_SLEEP, ticks));
}
////////////////////

static void apple() {
	write("apple\n\n\n", 11);
	return;
}

static int main()
{
	unsigned long i, j;
	unsigned long buff[4096];
	i = getpid();

	signal(SIGALRM, (unsigned long)&apple);
	alarm(2);
	// clone(NULL, NULL);

	while(1);
	// for(i=0; i<409600; ++i)
	// 	j = buff[i];
	
	i=0x100034;
	j = i / (i-0x100034);

	// u64 *r = NULL;
	// u64 s = *r;
	exit(-5);
	return 0;
}
