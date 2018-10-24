#include<init.h>
#include<memory.h>
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
//////
static u64 _syscall3(int syscall_num, u32 size,int flags)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

static u64 _syscall4(int syscall_num, u32 size,int flags)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

static u64 _syscall5(int syscall_num, u64 arg1,u64 arg2)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

static u64 _syscall6(int syscall_num, u64 arg1)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

static u64 _syscall7(int syscall_num, u64 arg1)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}

static u64 _syscall8(int syscall_num, u64 arg1, u64 arg2)
{
  asm volatile ( "int $0x80;"
                 "leaveq;"
                 "retq;"
                :::"memory");
  return 0;   /*gcc shutup!*/
}


/////

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
/////
static void* expand(u32 size,int flags){
  return((void *)(_syscall3(SYSCALL_EXPAND,size,flags)));
}

static void* shrink(u32 size,int flags){
  return((void *)(_syscall4(SYSCALL_SHRINK,size,flags)));
}

static long signal(u64 arg1, u64 arg2)
{
   return(_syscall5(SYSCALL_SIGNAL, arg1, arg2));
}
static long alarm(u64 arg1)
{
   return(_syscall6(SYSCALL_ALARM, arg1));
}
static long sleep(u64 arg1)
{
   return(_syscall7(SYSCALL_SLEEP, arg1));
}
static long clone(u64 arg1,u64 arg2)
{
   return(_syscall8(SYSCALL_CLONE, arg1, arg2));
}
void sigfpehandler(){
	write("this is sigfpehandler\n",22);
}
void sigsegvhandler(){
	write("this is sigsegvhandler\n",22);
}
void sigalrmhandler(){
	write("this is sigalrmhandler\n",22);
}
void func(){
	write("in func1\n",9);
  signal(SIGALRM,sigalrmhandler);
	alarm(10);
  // while(1);
	write("in func2\n",9);
}

/////
static void clone_func3(){
  write("in clone3\n",10);
  write("in clone3\n",10);
  write("in clone3\n",10);
  write("in clone3\n",10);
  write("in clone3\n",10);
  write("in clone3\n",10);
  write("in clone3\n",10);
  exit(-5);
}

static void clone_func2(){
  write("in clone2\n",10);
  write("in clone2\n",10);
  write("in clone2\n",10);
  write("in clone2\n",10);
  write("in clone2\n",10);
  write("in clone2\n",10);
  write("in clone2\n",10);
  exit(-5);
}

static void clone_func1(){
    sleep(1);
  write("in clone1\n",10);
  write("in clone1\n",10);
    sleep(1);
  write("in clone1\n",10);
  write("in clone1\n",10);
  write("in clone1\n",10);
  write("in clone1\n",10);
  write("in clone1\n",10);
  exit(-5);
}


static int main()
{
  #if 0
  unsigned long i, j;
  unsigned long buff[4096];
  i = getpid();
  signal(SIGFPE,sigfpehandler);
  for(i=0; i<4096; ++i){
      j = buff[i];
  }
  i=0x100034;
  j = i / (i-0x100034);
  #endif
  #if 0
  char * ptr = expand( 20, MAP_WR);
	if(ptr == NULL)
         write("FAILED\n", 7);

  	*(ptr) = 'A';  /*page fault will occur and handled successfully*/
  #endif
  #if 0
	write("in main1\n",9);
	signal(SIGSEGV,sigsegvhandler);
	write("in main2\n",9);
	int ar[10];
	ar[110]=1;
	write("in main2\n",9);
  #endif
  // sleep(10);
  // write("dsdh\n",5);
  // signal(SIGALRM,sigalrmhandler);
  // alarm(1);
  // while(1);
  #if 0
  write("in main1\n",9);
	sleep(20);
	write("in main2\n",9);
	sleep(20);
	write("in main3\n",9);
	func();
	write("in main4\n",9);
  #endif
  #if 1
  write("in main1\n",9);
  u64 clone1_ustack_end_va = expand(10,MAP_WR);
  u64 clone1_ustack_start_va = clone1_ustack_end_va + 4096;
  clone(clone_func1, clone1_ustack_start_va);

  write("in main1\n",9);
  u64 clone1_ustack_end_va2 = expand(10,MAP_WR);
  u64 clone1_ustack_start_va2 = clone1_ustack_end_va2 + 4096;
  clone(clone_func2, clone1_ustack_start_va2);

  write("in main1\n",9);
  u64 clone1_ustack_end_va3 = expand(10,MAP_WR);
  u64 clone1_ustack_start_va3 = clone1_ustack_end_va3 + 4096;
  clone(clone_func3, clone1_ustack_start_va3);
  
  write("in main2\n",9);
  write("in main2\n",9);
  write("in main2\n",9);
  write("in main2\n",9);
  write("in main2\n",9);
  write("in main2\n",9);
  write("in main2\n",9);
  #endif
  // write("in main1\n",9);
	// sleep(10);
	// write("in main2\n",9);
	// sleep(10);
	// write("in main3\n",9);


  // sleep(3);
  // write("dsdh\n",5);
  // while(1);
  exit(-5);
  return 0;
}
