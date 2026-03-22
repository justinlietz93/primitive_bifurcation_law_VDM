#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void){
    const unsigned char cell[] = {0x0F,0xBD,0xC0,0x0F,0x94,0x05,0x02,0x00,0x00,0x00,0xEB,0x01,0x00};
    const int n = 4;
    size_t sz = n * sizeof(cell) + 1;
    unsigned char *buf = mmap(NULL, sz, PROT_READ|PROT_WRITE|PROT_EXEC,
                              MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) { perror("mmap"); return 1; }
    for(int i=0;i<n;i++) memcpy(buf + i*sizeof(cell), cell, sizeof(cell));
    buf[n*sizeof(cell)] = 0xC3; // ret

    void (*fn)(void) = (void(*)(void))buf;
    __asm__ volatile(
        "xor %%eax, %%eax\n\t"
        "call *%0\n\t"
        : : "r"(fn) : "rax", "memory", "cc");

    printf("state bytes:\n");
    for(int i=0;i<n;i++){
        printf("cell %d state = %u\n", i, (unsigned)buf[i*sizeof(cell)+12]);
    }
    return 0;
}
