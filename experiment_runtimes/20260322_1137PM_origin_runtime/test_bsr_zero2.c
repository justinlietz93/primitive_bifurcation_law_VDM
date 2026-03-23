#include <stdio.h>
#include <stdint.h>

static void test_bsr_memsrc(uint32_t src, uint32_t preset, uint32_t *out_reg, uint8_t *out_zf) {
    uint32_t eax_out;
    uint8_t zf;
    __asm__ volatile (
        "mov %[preset], %%eax\n\t"
        "bsr %[src], %%eax\n\t"
        "setz %[zf]\n\t"
        : "=a"(eax_out), [zf] "=qm"(zf)
        : [src] "m"(src), [preset] "r"(preset)
        : "cc"
    );
    *out_reg = eax_out;
    *out_zf = zf;
}

static void test_bsf_memsrc(uint32_t src, uint32_t preset, uint32_t *out_reg, uint8_t *out_zf) {
    uint32_t eax_out;
    uint8_t zf;
    __asm__ volatile (
        "mov %[preset], %%eax\n\t"
        "bsf %[src], %%eax\n\t"
        "setz %[zf]\n\t"
        : "=a"(eax_out), [zf] "=qm"(zf)
        : [src] "m"(src), [preset] "r"(preset)
        : "cc"
    );
    *out_reg = eax_out;
    *out_zf = zf;
}

int main(void) {
    uint32_t out;
    uint8_t zf;

    puts("BSR with memory source = 0, preset dst = 0xCAFEBABE:");
    for (int i = 0; i < 5; ++i) {
        test_bsr_memsrc(0, 0xCAFEBABEu, &out, &zf);
        printf("run %d: out=0x%08x zf=%u\n", i+1, out, zf);
    }

    puts("\nBSF with memory source = 0, preset dst = 0xCAFEBABE:");
    for (int i = 0; i < 5; ++i) {
        test_bsf_memsrc(0, 0xCAFEBABEu, &out, &zf);
        printf("run %d: out=0x%08x zf=%u\n", i+1, out, zf);
    }

    puts("\nControl BSR with source = 0x80001000:");
    for (int i = 0; i < 3; ++i) {
        test_bsr_memsrc(0x80001000u, 0xCAFEBABEu, &out, &zf);
        printf("run %d: out=%u (0x%08x) zf=%u\n", i+1, out, out, zf);
    }
    return 0;
}
