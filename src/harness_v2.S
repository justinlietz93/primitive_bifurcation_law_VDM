.intel_syntax noprefix
.global _start
.extern i_kernel_step_v2
.text

.equ SYS_openat, 257
.equ SYS_write, 1
.equ SYS_close, 3
.equ SYS_exit, 60
.equ AT_FDCWD, -100
.equ O_WRONLY, 1
.equ O_CREAT, 64
.equ O_TRUNC, 512
.equ FLAGS, O_WRONLY|O_CREAT|O_TRUNC
.equ MODE644, 420
.equ RECORD_SIZE, 80
.equ STEPS, 192

_start:
    mov rax, SYS_openat
    mov rdi, AT_FDCWD
    lea rsi, [rip + outpath]
    mov rdx, FLAGS
    mov r10, MODE644
    syscall
    mov r12, rax                  # fd

    mov r10d, 1                   # phase/potential word
    xor ebx, ebx                  # axis depth
    mov byte ptr [rip + burden_state], 0
    xor r8d, r8d                  # tick
    xor r13d, r13d                # x
    xor r14d, r14d                # y
    xor r15d, r15d                # z
    xor ebp, ebp                  # w

.loop:
    cmp r8d, STEPS
    jge .done

    mov eax, 1
    mov ecx, ebx
    cmp ecx, 30
    jle .shift_ok
    mov ecx, 30
.shift_ok:
    shl eax, cl                   # ceiling = 1 << axis
    mov cl, al                    # kernel ceiling input

    mov edi, r10d
    mov dl, byte ptr [rip + burden_state]
    call i_kernel_step_v2
    mov r10d, edi
    mov byte ptr [rip + burden_state], dl

    movzx r11d, bl                # axis snapshot
    movzx edx, cl                 # ceiling snapshot
    movzx esi, byte ptr [rip + burden_state]
    movzx edi, al                 # event snapshot

    # substrate inscription sign from highest-set-bit parity of phase word
    bsr eax, r10d
    test eax, 1
    mov ecx, -1
    jz .sign_ready
    mov ecx, 1
.sign_ready:

    mov eax, r11d                 # active component = min(axis,3)
    cmp eax, 3
    jle .axis_ok
    mov eax, 3
.axis_ok:

    cmp eax, 0
    je .step_x
    cmp eax, 1
    je .step_y
    cmp eax, 2
    je .step_z
    add ebp, ecx
    jmp .coords_done
.step_x:
    add r13d, ecx
    jmp .coords_done
.step_y:
    add r14d, ecx
    jmp .coords_done
.step_z:
    add r15d, ecx
.coords_done:

    lea rcx, [rip + rec]
    mov [rcx + 0],  r8            # tick
    mov [rcx + 8],  r11           # axis
    mov [rcx + 16], rsi           # burden
    mov [rcx + 24], rdx           # ceiling
    mov [rcx + 32], r10           # phase word
    mov [rcx + 40], rdi           # event
    movsx rax, r13d
    mov [rcx + 48], rax           # x
    movsx rax, r14d
    mov [rcx + 56], rax           # y
    movsx rax, r15d
    mov [rcx + 64], rax           # z
    movsx rax, ebp
    mov [rcx + 72], rax           # w

    mov rax, SYS_write
    mov rdi, r12
    lea rsi, [rip + rec]
    mov rdx, RECORD_SIZE
    syscall

    inc r8d
    jmp .loop

.done:
    mov rax, SYS_close
    mov rdi, r12
    syscall

    mov rax, SYS_exit
    xor rdi, rdi
    syscall

.section .rodata
outpath:
    .asciz "/mnt/data/kernel_trace_v2.bin"

.section .bss
    .lcomm rec, RECORD_SIZE
    .lcomm burden_state, 1
