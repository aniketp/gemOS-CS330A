#include<init.h>
static void exit(int);
static int main(void);


void init_start()
{
  int retval = main();
  exit(0);
}

/*Invoke system call with no additional arguments*/
static int _syscall0(int syscall_num)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

/*Invoke system call with one argument*/

static int _syscall1(int syscall_num, int exit_code)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

/*Invoke write system call */
static int _syscall2(int syscall_num, char *buf, int length)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

/*Invoke expand system call */
static int _syscall3(int syscall_num, u32 size, int flags)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}


static void exit(int code)
{
  _syscall1(SYSCALL_EXIT, code);
}

static int getpid()
{
  return(_syscall0(SYSCALL_GETPID));
}

static int write(char *buf, int length)
{
  return(_syscall2(SYSCALL_WRITE, buf, length));
}

static void *expand(u32 size, int flags)
{
  return(_syscall3(SYSCALL_EXPAND, size, flags));
}


static int main()
{
  unsigned long i;
#if 0
  unsigned long *ptr = (unsigned long *)0x100032;
  i = *ptr;
#endif
  i = getpid();
  i = i/0;

  write((char *)0x14FFFFFFF, 3);
  return 0;
}
