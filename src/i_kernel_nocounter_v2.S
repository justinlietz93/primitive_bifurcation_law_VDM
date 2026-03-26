.intel_syntax noprefix
.text
.global i_kernel_step
.type i_kernel_step, @function
i_kernel_step:
    mov     rdx, rax
    popcnt  rcx, rax
    lea     rax, [rax + rcx + 1]
    bsr     rcx, rdx
    bsr     rdx, rax
    cmp     edx, ecx
    seta    bl
    ret
