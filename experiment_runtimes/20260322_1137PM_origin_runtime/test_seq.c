#include <stdio.h>
#include <stdint.h>

int main(){
    uint32_t eax=0;
    unsigned char zf;
    printf("start eax=%08x\n", eax);

    // 0F BD C0 ; BSR EAX,EAX
    asm volatile(
        "bsrl %[x], %[x]\n\t"
        "setz %[zf]\n\t"
        : [x] "+r" (eax), [zf] "=qm" (zf)
        :
        : "cc"
    );
    printf("after bsr1 eax=%08x zf=%u\n", eax, zf);

    // F7 D0 ; NOT EAX
    asm volatile("notl %[x]" : [x] "+r"(eax));
    printf("after not1 eax=%08x\n", eax);

    asm volatile(
        "bsrl %[x], %[x]\n\t"
        "setz %[zf]\n\t"
        : [x] "+r" (eax), [zf] "=qm" (zf)
        :
        : "cc"
    );
    printf("after bsr2 eax=%08x zf=%u\n", eax, zf);

    asm volatile("notl %[x]" : [x] "+r"(eax));
    printf("after not2 eax=%08x\n", eax);

    return 0;
}
