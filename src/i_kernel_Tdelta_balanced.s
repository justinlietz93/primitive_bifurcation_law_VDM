.intel_syntax noprefix
.text
.global i_kernel_step
.type i_kernel_step, @function
# state layout:
# [rdi+0]  = A
# [rdi+8]  = q
# [rdi+16] = u
# [rdi+24] = v
# [rdi+32] = floor_den
# [rdi+40] = event
# [rdi+48] = carried_den_at_transfer
i_kernel_step:
    mov     rax, [rdi+16]
    mov     rbx, [rdi+24]
    mov     rcx, rax
    imul    rcx, rbx
    mov     [rdi+48], rcx
    cmp     rcx, [rdi+32]
    jb      refine
    mov     qword ptr [rdi+40], 1
    mov     rdx, [rdi+8]
    add     rdx, 1
    and     rdx, 3
    mov     [rdi+8], rdx
    mov     rdx, [rdi+0]
    add     rdx, 1
    mov     [rdi+0], rdx
    mov     qword ptr [rdi+16], 1
    mov     qword ptr [rdi+24], 1
    mov     eax, 1
    ret
refine:
    mov     qword ptr [rdi+40], 0
    lea     rdx, [rax + rbx]
    mov     [rdi+16], rbx
    mov     [rdi+24], rdx
    xor     eax, eax
    ret
