#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    unsigned char *buf = mmap(NULL, 64, PROT_READ|PROT_WRITE|PROT_EXEC,
                              MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) { perror("mmap"); return 1; }

    // Cell 0: bsr eax,eax ; setz byte ptr [rip+rel32_to_cell1] ; ret
    const unsigned char cell0_prefix[] = {0x0F,0xBD,0xC0, 0x0F,0x94,0x05};
    memcpy(buf, cell0_prefix, sizeof(cell0_prefix));
    int32_t rel = (int32_t)((buf + 16) - (buf + 10)); // target cell1[0], RIP after rel32
    memcpy(buf + 6, &rel, 4);
    buf[10] = 0xC3; // ret

    // Cell 1 starts as 00 C3 = add bl, al ; after mutation becomes 01 C3 = add ebx, eax
    buf[16] = 0x00;
    buf[17] = 0xC3;
    buf[18] = 0xC3; // ret, not used in mutation proof

    printf("before: cell1[0]=0x%02x cell1[1]=0x%02x\n", buf[16], buf[17]);

    void (*fn)(void) = (void(*)(void))buf;
    __asm__ volatile(
        "xor %%eax, %%eax\n\t"
        "call *%0\n\t"
        : : "r"(fn) : "rax", "memory", "cc");

    printf("after : cell1[0]=0x%02x cell1[1]=0x%02x\n", buf[16], buf[17]);
    printf("pivot: %s -> %s\n",
           buf[16] == 0x01 ? "00/C3" : "unexpected",
           buf[16] == 0x01 ? "01/C3" : "unexpected");

    munmap(buf, 64);
    return 0;
}
