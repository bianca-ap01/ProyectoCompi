.data
print_fmt: .string "%ld \n"
max: .quad 0
.text
.globl main
main:
 pushq %rbp
 movq %rsp, %rbp
 subq $24, %rsp
 movq $5, %rax
 movq %rax, max(%rip)
 movq $0, %rax
 movq %rax, -24(%rbp)
for_0:
 movq -24(%rbp), %rax
 pushq %rax
 movq max(%rip), %rax
 movq %rax, %rcx
 popq %rax
 cmpq %rcx, %rax
 movl $0, %eax
 setle %al
 movzbq %al, %rax
 cmpq $0, %rax
 je endfor_0
 movq -24(%rbp), %rax
 pushq %rax
 movq $3, %rax
 movq %rax, %rcx
 popq %rax
 cmpq %rcx, %rax
 movl $0, %eax
 setle %al
 movzbq %al, %rax
 cmpq $0, %rax
 je ternary_else_1
 movq $100, %rax
 jmp ternary_end_1
ternary_else_1:
 movq $200, %rax
ternary_end_1:
 movq %rax, -16(%rbp)
 movq -16(%rbp), %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movl $0, %eax
 call printf@PLT
 movq -24(%rbp), %rax
 pushq %rax
 movq $1, %rax
 movq %rax, %rcx
 popq %rax
 addq %rcx, %rax
 movq %rax, -24(%rbp)
 jmp for_0
endfor_0:
 movq $0, %rax
 jmp .end_main
.end_main:
leave
ret
.section .note.GNU-stack,"",@progbits
