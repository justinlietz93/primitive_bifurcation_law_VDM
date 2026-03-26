.intel_syntax noprefix
.text
.global i_kernel_step
.type i_kernel_step, @function
# IN:  rax = live word
# OUT: rax = updated live word
#      bl  = articulation event (0/1)
i_kernel_step:
    mov     rdx, rax
    popcnt  rcx, rax
    add     rax, rcx
    inc     rax
    bsr     rcx, rdx
    bsr     rdx, rax
    cmp     edx, ecx
    seta    bl
    ret
