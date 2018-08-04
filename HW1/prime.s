# "Equivalent C program"
#
# bool isPrime(int num) {
#	 int sqroot = find_square_root(num);
#	
#	 for (int i = 2; i <= sqroot; i++)
#		 if (!(num % i))
#			 return false;
#	 return true;
# }

.globl	isPrime

isPrime:
	pushq	%rbp		# Push Base pointer to stack frame
	movq	%rsp, %rbp	# Copy Stack pointer to Base pointer 
	subq	$32, %rsp	# Allocate enough space to stack frame
	movl	%edi, -20(%rbp) # Store 'num' argument in EDI
	movl	-20(%rbp), %eax
	movl	%eax, %edi	# EDI contains the arg to function
	call	find_square_root # Call the C function
	movl	%eax, -4(%rbp)	# Store the return value
	movl	$2, -8(%rbp)	# Initialize counter
	movl	-8(%rbp), %eax	# Move the counter variable to EAX
	cmpl	-4(%rbp), %eax	# Whether rbp - 4(sqroot) > counter
	jle	.loop		# Loop condition still satisfies
	movl	$1, %eax	# Return True

.loop:
	movl	-20(%rbp), %eax
	cltd			# Extend the signed long bits of EAX to EDX
	idivl	-8(%rbp)	# Divide EDX:EAX by the operator (num/i == 0)
	movl	%edx, %eax	# IDIVL remainder stored in EDX
	testl	%eax, %eax 	# Check if number is divisible
	jne	.cont		# Continue if not equal
	movl	$0, %eax	# Else Set 0(false) as return value and exit
	jmp	.end

.cont:
	addl	$1, -8(%rbp)	# Increment loop counter

.end:
	leave
	ret
