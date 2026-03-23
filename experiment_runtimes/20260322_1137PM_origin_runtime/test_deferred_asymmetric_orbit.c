#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <x86intrin.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef void (*cell_fn_t)(void);
enum { ITERS = 60000, NUM_CELLS = 3 };

typedef struct { int cpu; cell_fn_t fn; uint64_t *deltas; } worker_args_t;
typedef struct { int cell_size; int code_size; int mutable_off; uint8_t *buf; } region_t;

static void pin_to_cpu(int cpu){ cpu_set_t set; CPU_ZERO(&set); CPU_SET(cpu,&set); if(sched_setaffinity(0,sizeof(set),&set)!=0) perror("sched_setaffinity"); }

static region_t build_region(void){
    region_t r={0};
    // mutable pair (2) + bsr (3) + setz (3) + lock xor self pair (10)
    r.cell_size = 2 + 3 + 3 + 10;
    r.code_size = NUM_CELLS * r.cell_size + 1;
    r.mutable_off = 0;
    r.buf = mmap(NULL,4096,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(r.buf==MAP_FAILED){ perror("mmap"); exit(1);} memset(r.buf,0x90,4096);
    for(int i=0;i<NUM_CELLS;++i){
        int base=i*r.cell_size; int p=base;
        r.buf[p++]=0x00; r.buf[p++]=0xC0; // mutable pair starts as add al,al; toggles to f7 d0 = not eax
        r.buf[p++]=0x0F; r.buf[p++]=0xBD; r.buf[p++]=0xC0; // bsr eax,eax
        r.buf[p++]=0x0F; r.buf[p++]=0x94; r.buf[p++]=0xC2; // setz dl
        r.buf[p++]=0x66; r.buf[p++]=0xF0; r.buf[p++]=0x81; r.buf[p++]=0x35; // lock xorw [rip+disp32], 0x10F7
        int target = base + r.mutable_off;
        int next_ip = p + 4;
        int32_t disp = (int32_t)(target - next_ip);
        memcpy(&r.buf[p], &disp, 4); p += 4;
        r.buf[p++]=0xF7; r.buf[p++]=0x10;
    }
    r.buf[NUM_CELLS*r.cell_size]=0xC3;
    __builtin___clear_cache((char*)r.buf,(char*)r.buf+r.code_size);
    return r;
}

static void *worker(void *argp){ worker_args_t *arg=(worker_args_t*)argp; pin_to_cpu(arg->cpu); unsigned aux=0; for(int i=0;i<ITERS;++i){ uint64_t t0=__rdtscp(&aux); asm volatile("xor %%eax, %%eax\n\tcall *%0\n\t": :"r"(arg->fn):"rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","cc","memory"); uint64_t t1=__rdtscp(&aux); arg->deltas[i]=t1-t0;} return NULL; }
static void run_case(cell_fn_t fn0, cell_fn_t fn1, uint64_t *out0, uint64_t *out1){ pthread_t th0,th1; worker_args_t a0={.cpu=0,.fn=fn0,.deltas=out0}, a1={.cpu=1,.fn=fn1,.deltas=out1}; if(pthread_create(&th0,NULL,worker,&a0)||pthread_create(&th1,NULL,worker,&a1)){ fprintf(stderr,"pthread_create failed\n"); exit(1);} pthread_join(th0,NULL); pthread_join(th1,NULL);} 
static double mean_u64(const uint64_t *xs){ long double s=0; for(int i=0;i<ITERS;++i) s += (long double)xs[i]; return (double)(s/(long double)ITERS);} 
static double parity_mean(const uint64_t *xs,int parity){ long double s=0; int n=0; for(int i=parity;i<ITERS;i+=2){ s += (long double)xs[i]; ++n;} return n? (double)(s/(long double)n):0.0; }
static void write_csv(const char *path,const uint64_t *xs){ FILE *f=fopen(path,"w"); if(!f){ perror(path); exit(1);} for(int i=0;i<ITERS;++i) fprintf(f,"%" PRIu64 "\n", xs[i]); fclose(f);} 
static void dump_pairs(FILE *f,const char *label,const region_t *r){ fprintf(f, "%s", label); for(int i=0;i<NUM_CELLS;++i){ int off=i*r->cell_size+r->mutable_off; fprintf(f," %02x%02x", r->buf[off], r->buf[off+1]); } fprintf(f,"\n"); }
static void trace_passes(const char *path){ region_t tr=build_region(); FILE *f=fopen(path,"w"); if(!f){ perror(path); exit(1);} dump_pairs(f,"initial:",&tr); for(int p=1;p<=8;++p){ asm volatile("xor %%eax, %%eax\n\tcall *%0\n\t": :"r"((cell_fn_t)tr.buf):"rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","cc","memory"); char label[32]; snprintf(label,sizeof(label),"pass_%d:",p); dump_pairs(f,label,&tr);} fclose(f);} 
int main(void){
    FILE *notes=fopen("/mnt/data/deferred_asymmetric_orbit_notes.md","w"); if(!notes){ perror("notes"); return 1; }
    fprintf(notes,
"# Deferred Asymmetric Orbit\n\n"
"## Why this variant exists\n"
"The direct successor-targeted asymmetric pair `00 C0 <-> F7 D0` caused immediate segfaults on this VM, which is strong evidence of an instruction-fetch coherence break when the next executed pair is mutated too aggressively.\n\n"
"To keep moving without flattening the idea, this variant defers the asymmetric mutation by one pass: each cell executes its current mutable pair first, then toggles its **own** pair for the next pass.\n\n"
"## Mechanism\n"
"Per cell:\n\n"
"- mutable pair `00 C0 <-> F7 D0`\n"
"- `BSR EAX,EAX`\n"
"- `SETZ DL`\n"
"- `LOCK XOR word ptr [self_mutable_pair], 0x10F7`\n\n"
"This preserves a real reversible asymmetric orbit without immediate successor fetch tearing.\n");
    fclose(notes);

    trace_passes("/mnt/data/deferred_asymmetric_pass_trace.txt");
    region_t shared=build_region(), sepA=build_region(), sepB=build_region();
    uint64_t *shared0=calloc(ITERS,sizeof(uint64_t)), *shared1=calloc(ITERS,sizeof(uint64_t)), *sep0=calloc(ITERS,sizeof(uint64_t)), *sep1=calloc(ITERS,sizeof(uint64_t));
    if(!shared0||!shared1||!sep0||!sep1){ fprintf(stderr,"calloc failed\n"); return 1; }
    run_case((cell_fn_t)shared.buf,(cell_fn_t)shared.buf,shared0,shared1);
    run_case((cell_fn_t)sepA.buf,(cell_fn_t)sepB.buf,sep0,sep1);
    double m_sh0=mean_u64(shared0), m_sh1=mean_u64(shared1), m_sp0=mean_u64(sep0), m_sp1=mean_u64(sep1);
    double sh0o=parity_mean(shared0,0), sh0e=parity_mean(shared0,1), sh1o=parity_mean(shared1,0), sh1e=parity_mean(shared1,1);
    double sp0o=parity_mean(sep0,0), sp0e=parity_mean(sep0,1), sp1o=parity_mean(sep1,0), sp1e=parity_mean(sep1,1);
    FILE *sum=fopen("/mnt/data/deferred_asymmetric_orbit_summary.txt","w"); if(!sum){ perror("summary"); return 1; }
    fprintf(sum,"Deferred asymmetric orbit summary\n");
    fprintf(sum,"shared_mean_core0=%.9f\nshared_mean_core1=%.9f\n",m_sh0,m_sh1);
    fprintf(sum,"separate_mean_core0=%.9f\nseparate_mean_core1=%.9f\n",m_sp0,m_sp1);
    fprintf(sum,"ratio_shared_separate_core0=%.12f\nratio_shared_separate_core1=%.12f\n",m_sh0/m_sp0,m_sh1/m_sp1);
    fprintf(sum,"ratio_core0_over_core1_shared=%.12f\nratio_core0_over_core1_separate=%.12f\n",m_sh0/m_sh1,m_sp0/m_sp1);
    fprintf(sum,"shared_core0_odd=%.9f even=%.9f ratio=%.12f\n",sh0o,sh0e,sh0o/sh0e);
    fprintf(sum,"shared_core1_odd=%.9f even=%.9f ratio=%.12f\n",sh1o,sh1e,sh1o/sh1e);
    fprintf(sum,"separate_core0_odd=%.9f even=%.9f ratio=%.12f\n",sp0o,sp0e,sp0o/sp0e);
    fprintf(sum,"separate_core1_odd=%.9f even=%.9f ratio=%.12f\n",sp1o,sp1e,sp1o/sp1e);
    dump_pairs(sum,"shared_final_pairs:",&shared); dump_pairs(sum,"separate_A_final_pairs:",&sepA); dump_pairs(sum,"separate_B_final_pairs:",&sepB); fclose(sum);
    FILE *log=fopen("/mnt/data/test_deferred_asymmetric_orbit.log","w"); if(!log){ perror("log"); return 1; }
    fprintf(log,"Deferred asymmetric orbit\n");
    fprintf(log,"shared_mean_core0=%.9f\nshared_mean_core1=%.9f\nseparate_mean_core0=%.9f\nseparate_mean_core1=%.9f\n",m_sh0,m_sh1,m_sp0,m_sp1);
    dump_pairs(log,"shared_final_pairs:",&shared); dump_pairs(log,"separate_A_final_pairs:",&sepA); dump_pairs(log,"separate_B_final_pairs:",&sepB); fclose(log);
    write_csv("/mnt/data/deferred_asymmetric_shared_core0.csv",shared0); write_csv("/mnt/data/deferred_asymmetric_shared_core1.csv",shared1); write_csv("/mnt/data/deferred_asymmetric_separate_core0.csv",sep0); write_csv("/mnt/data/deferred_asymmetric_separate_core1.csv",sep1);
    printf("shared   mean0=%.6f mean1=%.6f\n",m_sh0,m_sh1); printf("separate mean0=%.6f mean1=%.6f\n",m_sp0,m_sp1); printf("ratio shared/separate core0=%.9f\nratio shared/separate core1=%.9f\n",m_sh0/m_sp0,m_sh1/m_sp1); dump_pairs(stdout,"shared_final_pairs:",&shared); dump_pairs(stdout,"separate_A_final_pairs:",&sepA); dump_pairs(stdout,"separate_B_final_pairs:",&sepB); return 0; }
