0x100000:    push %rbp              /* Push RBP register onto the stack */
0x100001:    mov  $100, 8(%rbp)     /* Store 100 at the memory address RBP + 8 */
0x100004:    mov $1100, %rcx        /* Stote 1100 in register RCX */
0x100007:    add 8(%rbp), %rcx      /* Add value in RCX register to the value stored at memory address RBP + 8 */
0x10000a:    pop %rbp               /* Pop from the stack to RBP register*/
