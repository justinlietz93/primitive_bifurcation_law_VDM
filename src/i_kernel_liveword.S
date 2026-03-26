.intel_syntax noprefix
.text
.global i_kernel_step
.type i_kernel_step, @function
# IN:  RDI -> u64 live word
# OUT: AL = articulation event (0/1)
#       memory at [RDI] updated in place
# Rule:
#   lowbit = w & -w
#   next   = w + lowbit + 1
#   event  = bitlen(next) > bitlen(w)
i_kernel_step:
    mov     rax, [rdi]      # w
    mov     rcx, rax
    neg     rcx
    and     rcx, rax        # lowbit
    lea     rdx, [rax + rcx + 1]   # next

    bsr     r8, rax         # old highest bit
    bsr     r9, rdx         # new highest bit
    xor     eax, eax
    cmp     r9, r8
    seta    al              # event

    mov     [rdi], rdx
    ret
