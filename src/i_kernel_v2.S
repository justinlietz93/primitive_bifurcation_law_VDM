.intel_syntax noprefix
.global i_kernel_step_v2
.text

# IN:
#   edi = phase/potential word
#   bl  = axis depth
#   dl  = accumulated burden
#   cl  = caller-supplied saturation ceiling
# OUT:
#   edi = updated phase/potential word
#   bl  = updated axis depth
#   dl  = updated burden
#   al  = articulation event flag (0/1)

i_kernel_step_v2:
    xor eax, eax            # default: no articulation event
    rol edi, 1              # same-channel transit every tick
    bsr esi, edi            # self-interrogation side effect
    inc dl                  # burden accumulates under continued transit
    cmp dl, cl
    jb .done
    neg edi                 # orthogonal re-articulation only at saturation
    inc bl                  # new axis depth
    xor dl, dl              # clear local burden after transfer
    mov al, 1               # articulation event
.done:
    ret
