.intel_syntax noprefix
.text
.global i_kernel_step
.type i_kernel_step, @function
# state layout:
# [rdi+0]  = local chart phase u   (Q32.32, same-class)
# [rdi+8]  = step du               (Q32.32)
# [rdi+16] = axis depth
# returns al = articulation event
# threshold = 1.0 in Q32.32

i_kernel_step:
    mov     rax, [rdi]
    add     rax, [rdi+8]
    xor     ecx, ecx
    mov     rdx, 0x100000000
    cmp     rax, rdx
    jb      1f
    sub     rax, rdx
    shr     qword ptr [rdi+8], 1
    inc     qword ptr [rdi+16]
    mov     cl, 1
1:
    mov     [rdi], rax
    mov     eax, ecx
    ret
