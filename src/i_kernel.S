.intel_syntax noprefix
.global i_kernel_step
.text

# IN:
#   edi = phase state
#   bl  = axis depth
#   dl  = accumulated burden
#   cl  = saturation ceiling
# OUT:
#   edi = updated phase state
#   bl  = updated axis depth
#   dl  = updated burden
#   al  = articulation event flag (0 or 1)

i_kernel_step:
    xor eax, eax            # default: no articulation event
    bsr edi, edi            # self-interrogation
    jnz .checked            # if nonzero, no unresolved witness this step
    inc dl                  # unresolvedness witness accumulates into burden
.checked:
    cmp dl, cl              # saturation test
    jb .done                # below ceiling -> same-channel
    neg edi                 # orthogonal re-articulation
    inc bl                  # new axis depth
    xor dl, dl              # clear local burden
    mov al, 1               # articulation event
.done:
    ret
