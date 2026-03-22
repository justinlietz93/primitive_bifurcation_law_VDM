.intel_syntax noprefix
.global _start

.section .text
_start:
    xor eax, eax
loop:
    .byte 0xF7, 0xD0      # not eax
    .byte 0xEB, 0xFC      # jmp short loop
