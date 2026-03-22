#define _GNU_SOURCE
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
enum { ITERS = 100000, NUM_CELLS = 3 };
typedef struct { int cpu; cell_fn_t fn; uint64_t *deltas; } worker_args_t;

static void pin_to_cpu(int cpu){ cpu_set_t set; CPU_ZERO(&set); CPU_SET(cpu,&set); if(sched_setaffinity(0,sizeof(set),&set)!=0) perror("sched_setaffinity"); }
static size_t cell_size(int d){ return (size_t)(3*d+8); }
static uint8_t *build(const int d[3]){
 uint8_t*buf=mmap(NULL,4096,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0); if(buf==MAP_FAILED){perror("mmap");exit(1);} memset(buf,0x90,4096);
 size_t base[3],off=0; for(int i=0;i<3;i++){base[i]=off;off+=cell_size(d[i]);}
 for(int i=0;i<3;i++){
  size_t p=base[i]; for(int k=0;k<d[i];k++){buf[p++]=0x0F;buf[p++]=0xBD;buf[p++]=0xC0;}
  buf[p++]=0xF0;buf[p++]=0x0F;buf[p++]=0xB0;buf[p++]=0x15; size_t target=base[(i+1)%3]; int32_t disp=(int32_t)(target-(p+4)); memcpy(&buf[p],&disp,4); p+=4;
 }
 buf[off]=0xC3; __builtin___clear_cache((char*)buf,(char*)buf+off+1); return buf; }
static void*worker(void*argp){ worker_args_t*arg=(worker_args_t*)argp; pin_to_cpu(arg->cpu); unsigned aux=0; for(int i=0;i<ITERS;i++){ uint64_t t0=__rdtscp(&aux); asm volatile("xor %%eax,%%eax\n\tmov $1,%%edx\n\tcall *%0\n\t"::"r"(arg->fn):"rax","rdx","rcx","rsi","rdi","r8","r9","r10","r11","cc","memory"); uint64_t t1=__rdtscp(&aux); arg->deltas[i]=t1-t0; } return NULL; }
static void run(cell_fn_t f0, cell_fn_t f1, uint64_t *a, uint64_t *b){ pthread_t t0,t1; worker_args_t w0={0,f0,a}, w1={1,f1,b}; pthread_create(&t0,NULL,worker,&w0); pthread_create(&t1,NULL,worker,&w1); pthread_join(t0,NULL); pthread_join(t1,NULL); }
static double mean(const uint64_t *x){ long double s=0; for(int i=0;i<ITERS;i++) s+=x[i]; return (double)(s/ITERS); }
int main(){ int patterns[][3]={{3,3,3},{3,2,4},{2,1,4},{1,4,4}}; int n=sizeof(patterns)/sizeof(patterns[0]);
 for(int j=0;j<n;j++){
  int *d=patterns[j]; uint8_t*shared=build(d),*sep0=build(d),*sep1=build(d); uint64_t *sh0=calloc(ITERS,8),*sh1=calloc(ITERS,8),*sp0=calloc(ITERS,8),*sp1=calloc(ITERS,8);
  run((cell_fn_t)shared,(cell_fn_t)shared,sh0,sh1); run((cell_fn_t)sep0,(cell_fn_t)sep1,sp0,sp1);
  double msh0=mean(sh0),msh1=mean(sh1),msp0=mean(sp0),msp1=mean(sp1);
  printf("pattern=%d,%d,%d\n",d[0],d[1],d[2]);
  printf(" shared %.6f %.6f\n",msh0,msh1); printf(" separate %.6f %.6f\n",msp0,msp1); printf(" ratio_shared_sep %.12f %.12f\n",msh0/msp0,msh1/msp1);
  free(sh0);free(sh1);free(sp0);free(sp1); munmap(shared,4096);munmap(sep0,4096);munmap(sep1,4096);
 }
 }
