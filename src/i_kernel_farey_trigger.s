.intel_syntax noprefix
.text
.global i_kernel_step
.type i_kernel_step, @function
i_kernel_step:
    mov     rax, [rdi+8]
    add     rax, 1
    mov     [rdi+8], rax
    mov     rcx, rax
    shr     rcx, 4
    mov     [rdi+16], rcx
    mov     rdx, rax
    and     rdx, 15
    xor     r8d, r8d
    cmp     rdx, 4
    sete    r8b
    movzx   r8, r8b
    mov     [rdi+40], r8
    test    r8b, r8b
    jz      1f
    mov     r9, [rdi+0]
    add     r9, 1
    mov     [rdi+0], r9
1:
    mov     r10, [rdi+24]
    mov     r11, [rdi+32]
    lea     rcx, [r10+r11]
    mov     [rdi+24], r11
    mov     [rdi+32], rcx
    mov     eax, r8d
    ret
