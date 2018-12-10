#ifndef _PROFILE_H_
#define _PROFILE_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define CoreDebug_DEMCR *((volatile uint32_t*) 0xE000EDFCUL )
#define DWT_CTRL      *((volatile uint32_t*) 0xE0001000UL )
#define DWT_CYCCNT    *((volatile uint32_t*) 0xE0001004UL )
#define DWT_CPICNT    *((volatile uint32_t*) 0xE0001008UL )
#define DWT_EXCCNT    *((volatile uint32_t*) 0xE000100CUL )
#define DWT_SLEEPCNT  *((volatile uint32_t*) 0xE0001010UL )
#define DWT_LSUCNT    *((volatile uint32_t*) 0xE0001014UL )
#define DWT_FOLDCNT   *((volatile uint32_t*) 0xE0001018UL )
#define DWT_LAR       *((volatile uint32_t*) 0xE0001FB0UL )
/* LAR is present on M7 but not M4 */

#define CoreDebug_DEMCR_TRCENA_Pos         24                                             /*!< CoreDebug DEMCR: TRCENA Position */
#define CoreDebug_DEMCR_TRCENA_Msk         (1UL << CoreDebug_DEMCR_TRCENA_Pos)            /*!< CoreDebug DEMCR: TRCENA Mask */
/* DWT Control Register Definitions */
#define DWT_CTRL_NUMCOMP_Pos               28                                          /*!< DWT CTRL: NUMCOMP Position */
#define DWT_CTRL_NUMCOMP_Msk               (0xFUL << DWT_CTRL_NUMCOMP_Pos)             /*!< DWT CTRL: NUMCOMP Mask */
#define DWT_CTRL_NOTRCPKT_Pos              27                                          /*!< DWT CTRL: NOTRCPKT Position */
#define DWT_CTRL_NOTRCPKT_Msk              (0x1UL << DWT_CTRL_NOTRCPKT_Pos)            /*!< DWT CTRL: NOTRCPKT Mask */
#define DWT_CTRL_NOEXTTRIG_Pos             26                                          /*!< DWT CTRL: NOEXTTRIG Position */
#define DWT_CTRL_NOEXTTRIG_Msk             (0x1UL << DWT_CTRL_NOEXTTRIG_Pos)           /*!< DWT CTRL: NOEXTTRIG Mask */
#define DWT_CTRL_NOCYCCNT_Pos              25                                          /*!< DWT CTRL: NOCYCCNT Position */
#define DWT_CTRL_NOCYCCNT_Msk              (0x1UL << DWT_CTRL_NOCYCCNT_Pos)            /*!< DWT CTRL: NOCYCCNT Mask */
#define DWT_CTRL_NOPRFCNT_Pos              24                                          /*!< DWT CTRL: NOPRFCNT Position */
#define DWT_CTRL_NOPRFCNT_Msk              (0x1UL << DWT_CTRL_NOPRFCNT_Pos)            /*!< DWT CTRL: NOPRFCNT Mask */
#define DWT_CTRL_CYCEVTENA_Pos             22                                          /*!< DWT CTRL: CYCEVTENA Position */
#define DWT_CTRL_CYCEVTENA_Msk             (0x1UL << DWT_CTRL_CYCEVTENA_Pos)           /*!< DWT CTRL: CYCEVTENA Mask */
#define DWT_CTRL_FOLDEVTENA_Pos            21                                          /*!< DWT CTRL: FOLDEVTENA Position */
#define DWT_CTRL_FOLDEVTENA_Msk            (0x1UL << DWT_CTRL_FOLDEVTENA_Pos)          /*!< DWT CTRL: FOLDEVTENA Mask */
#define DWT_CTRL_LSUEVTENA_Pos             20                                          /*!< DWT CTRL: LSUEVTENA Position */
#define DWT_CTRL_LSUEVTENA_Msk             (0x1UL << DWT_CTRL_LSUEVTENA_Pos)           /*!< DWT CTRL: LSUEVTENA Mask */
#define DWT_CTRL_SLEEPEVTENA_Pos           19                                          /*!< DWT CTRL: SLEEPEVTENA Position */
#define DWT_CTRL_SLEEPEVTENA_Msk           (0x1UL << DWT_CTRL_SLEEPEVTENA_Pos)         /*!< DWT CTRL: SLEEPEVTENA Mask */
#define DWT_CTRL_EXCEVTENA_Pos             18                                          /*!< DWT CTRL: EXCEVTENA Position */
#define DWT_CTRL_EXCEVTENA_Msk             (0x1UL << DWT_CTRL_EXCEVTENA_Pos)           /*!< DWT CTRL: EXCEVTENA Mask */
#define DWT_CTRL_CPIEVTENA_Pos             17                                          /*!< DWT CTRL: CPIEVTENA Position */
#define DWT_CTRL_CPIEVTENA_Msk             (0x1UL << DWT_CTRL_CPIEVTENA_Pos)           /*!< DWT CTRL: CPIEVTENA Mask */
#define DWT_CTRL_EXCTRCENA_Pos             16                                          /*!< DWT CTRL: EXCTRCENA Position */
#define DWT_CTRL_EXCTRCENA_Msk             (0x1UL << DWT_CTRL_EXCTRCENA_Pos)           /*!< DWT CTRL: EXCTRCENA Mask */
#define DWT_CTRL_PCSAMPLENA_Pos            12                                          /*!< DWT CTRL: PCSAMPLENA Position */
#define DWT_CTRL_PCSAMPLENA_Msk            (0x1UL << DWT_CTRL_PCSAMPLENA_Pos)          /*!< DWT CTRL: PCSAMPLENA Mask */
#define DWT_CTRL_SYNCTAP_Pos               10                                          /*!< DWT CTRL: SYNCTAP Position */
#define DWT_CTRL_SYNCTAP_Msk               (0x3UL << DWT_CTRL_SYNCTAP_Pos)             /*!< DWT CTRL: SYNCTAP Mask */
#define DWT_CTRL_CYCTAP_Pos                 9                                          /*!< DWT CTRL: CYCTAP Position */
#define DWT_CTRL_CYCTAP_Msk                (0x1UL << DWT_CTRL_CYCTAP_Pos)              /*!< DWT CTRL: CYCTAP Mask */
#define DWT_CTRL_POSTINIT_Pos               5                                          /*!< DWT CTRL: POSTINIT Position */
#define DWT_CTRL_POSTINIT_Msk              (0xFUL << DWT_CTRL_POSTINIT_Pos)            /*!< DWT CTRL: POSTINIT Mask */
#define DWT_CTRL_POSTPRESET_Pos             1                                          /*!< DWT CTRL: POSTPRESET Position */
#define DWT_CTRL_POSTPRESET_Msk            (0xFUL << DWT_CTRL_POSTPRESET_Pos)          /*!< DWT CTRL: POSTPRESET Mask */
#define DWT_CTRL_CYCCNTENA_Pos              0                                          /*!< DWT CTRL: CYCCNTENA Position */
#define DWT_CTRL_CYCCNTENA_Msk             (0x1UL /*<< DWT_CTRL_CYCCNTENA_Pos*/)       /*!< DWT CTRL: CYCCNTENA Mask */

typedef struct {
  char * name;
  uint32_t samples;
  uint32_t cyccnt;
  uint32_t cpicnt;
  uint32_t exccnt;
  uint32_t sleepcnt;
  uint32_t lsucnt;
  uint32_t foldcnt;
} PerfProbe;

PerfProbe * CreatePerfProbe(const char * n);
void EnableProfiling();
void GetPerfData(PerfProbe * p);
void PrintPerfLabels();
void PrintPerfData(PerfProbe * p); 

void ResetPerfData(PerfProbe * p);
void OnePerfData(PerfProbe * p);

#endif
