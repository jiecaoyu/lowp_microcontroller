#include "profile.h"
#include <stdlib.h>


PerfProbe * CreatePerfProbe(const char * n) {
    PerfProbe * p = (PerfProbe*)malloc(sizeof(PerfProbe));
    p->name = (char*)malloc(strlen(n)+1);
    strcpy(p->name, n);
    p->samples = 0;
    p->cyccnt = 0;
    p->cpicnt = 0;
    p->exccnt = 0;
    p->sleepcnt = 0;
    p->lsucnt = 0;
    p->foldcnt = 0;
    return p;
}


void EnableProfiling() {
  CoreDebug_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT_LAR = 0xC5ACCE55; //unlock needed on Cortex-M7
  DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk; //| DWT_CTRL_FOLDEVTENA_Msk | DWT_CTRL_SLEEPEVTENA_Msk | DWT_CTRL_EXCEVTENA_Msk | DWT_CTRL_CPIEVTENA_Msk | DWT_CTRL_LSUEVTENA_Msk;
  //DWT_EXCCNT = 0;
  //DWT_SLEEPCNT = 0;
  //DWT_FOLDCNT = 0;
  //DWT_LSUCNT = 0;
  //DWT_CPICNT = 0;
  DWT_CYCCNT = 0;
}

void GetPerfData(PerfProbe * p) {
  uint32_t cyccnt = DWT_CYCCNT;
  p->cyccnt += cyccnt;
  p->samples++;
  DWT_CYCCNT = 0;
}

void PrintPerfLabels() {
  //printf("NAME SAMPLES CYCCNT CPICNT EXCCNT SLEEPCNT LSUCNT FOLDCNT\r\n");
  printf("NAME SAMPLES CYCCNT\r\n");
}

void PrintPerfData(PerfProbe * p) {
  printf("%s %lu %lu\r\n", p->name, p->samples, p->cyccnt); 
}

void ResetPerfData(PerfProbe * p) {
    p->cyccnt = 0;
    DWT_CYCCNT = 0;
    return;
}
void OnePerfData(PerfProbe * p) {
    uint32_t cyccnt = DWT_CYCCNT;
    p->cyccnt = cyccnt;
    return;
}
