.global switch_to
switch_to:
    movl 4(%esp), %eax
    movl 8(%esp), %edx

    pushl %esi
    pushl %edi
    pushl %ebx
    pushl %ebp

    movl %esp, (%eax)
    
    movl (%edx), %esp

    movl 4(%edx), %eax
    movl %eax, %cr3

    popl %ebp
    popl %ebx
    popl %edi
    popl %esi

    ret
