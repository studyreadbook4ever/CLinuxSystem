;nasm -f elf64 pibona.asm -p pibona.o
;ld -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o pibona pibona.o -lc
section .data
	STDIN: equ 0
	STDOUT: equ 1
	SYS_READ: equ 0
	SYS_WRITE: equ 1
	SYS_EXIT: equ 60
	MAX_LEN: equ 3
	EOL_CHAR: equ 10

section .rodata
	start_msg: db "fibonacci means a[i] = a[i - 1] + a[i - 2] and a[1]==1 a[2] == 1", 0
	start_msg_len: equ $ - start_msg
	prompt_msg: db "ENTER A NUMBER (0-92): ", 0
	prompt_msg_len: equ $ - prompt_msg
	
	invalid_msg: db "Invalid number.. try again", 10, 0
	invalid_msg_len: equ $ -invalid_msg

	invalid_digit_msg: db "Error: Invalid digit in input for given base.", 10, 0
	invalid_digit_msg_len: equ $ - invalid_digit_msg - 1 
	fmt_axiom db "%d's fibonacci = %d", 10, 0
	fmt_loop db "%d's fibonacci = %ld + %ld = %ld", 10, 0

section .bss
	input_str: resb MAX_LEN
	input_num: resb 1

default rel		;64bit

section .text
	global _start
	extern printf

_start:
	mov rax, SYS_WRITE
	mov rdi, STDOUT
	mov rsi, prompt_msg
	mov rdx, prompt_msg_len
	syscall

	mov rax, SYS_READ
	mov rdi, STDIN
	lea rsi, [input_str]
	mov rdx, MAX_LEN
	syscall
	cmp rax, 0		;errcheck
	jle exit_program

	lea rdi, [input_str]	;gettin	
	call strip_eol
	
	lea rdi, [input_str]
	mov rsi, 10
	call parse_number
	cmp rax, -1
	je invalid_input
	cmp rax, 93
	jge invalid_input
	mov byte [input_num], al
;logical running

	xor r12, r12
	xor r14, 1
	mov r15, 1

	movzx r13, byte [input_num]
fib_zero:
	mov rdi, fmt_axiom
	mov rsi, r12
	mov rdx, 0
	xor rax, rax
	call printf
	cmp r12, r13
	je exit_program
	inc r12

fib_one:
	mov rdi, fmt_axiom
	mov rsi, r12
	mov rdx, 0
	xor rax, rax
	call printf
	cmp r12, r13
	je exit_program
	inc r12	

fib_two:
	mov rdi, fmt_axiom
	mov rsi, r12
	mov rdx, 1
	xor rax, rax
	call printf
	cmp r12, r13
	je exit_program
	inc r12


fib_loop:
	cmp r12, r13
	jg exit_program		;escape when ind > input
	mov rbx, r14		;put rbx in [i-2](temp)
	add rbx, r15		;rbx += [i-1] on [i-2]
	mov r14, r15		;put[i-1] on [i-2]
	mov r15, rbx		;put[i] on [i-1]
	xor rax, rax		;set 0 on rax
	mov rdi, fmt_loop
	mov rsi, r12
	mov rdx, r14
	mov rcx, r15
	mov r8, rbx
	xor rax, rax
	call printf
	inc r12
	jmp fib_loop

strip_eol:
	xor rcx, rcx
.strip_loop:
	cmp rcx, MAX_LEN
	jge .done_strip
	cmp byte [rdi+rcx], EOL_CHAR
	je .replace_eol
	inc rcx
	jmp .strip_loop
.replace_eol:
	mov byte [rdi+rcx], 0
.done_strip:
	ret

;horner algorithm
parse_number:
	xor rax, rax
	mov r8, rdi

.parse_loop:
	mov dl, [r8]
	cmp dl, 0
	je .parse_done
	cmp dl, '0'
	jb .parse_error
	cmp dl, '9'
	jbe .digit_num
	jmp .parse_error

.digit_num:
	sub dl, '0'
	movzx ecx, dl
	cmp ecx, esi
	jae .parse_error
	xor rdx, rdx
	mul rsi
	add rax, rcx
	inc r8
	jmp .parse_loop
.parse_done:
	ret
.parse_error:
	mov rax, SYS_WRITE
	mov rdi, STDOUT
	lea rsi, [invalid_digit_msg]
	mov rdx, invalid_digit_msg_len
	syscall				;rax fd rdi fd rsi string rdx .len
	jmp exit_program

invalid_input:
	mov rax, SYS_WRITE
	mov rdi, STDOUT
	mov rsi, invalid_msg
	mov rdx, invalid_msg_len
	syscall				;rax fd rdi fd rsi string rdx .len
	jmp exit_program

exit_program:
	mov rax, SYS_EXIT		;60
	xor rdi, rdi
	syscall
