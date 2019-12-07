#include <stdint.h>
#include "sync.h"

#define	EnableIRQs()		asm volatile ("cpsie i")
#define	DisableIRQs()		asm volatile ("cpsid i")
#define	EnableInterrupts()	EnableIRQs()			// deprecated
#define	DisableInterrupts()	DisableIRQs()			// deprecated

#define	EnableFIQs()		asm volatile ("cpsie f")
#define	DisableFIQs()		asm volatile ("cpsid f")

#define TASK_LEVEL		0		// IRQs and FIQs enabled
#define IRQ_LEVEL		1		// IRQs disabled, FIQs enabled
#define FIQ_LEVEL		2		// IRQs and FIQs disabled

#define MAX_CRITICAL_LEVEL	20		// maximum nested level of EnterCritical()

#if RASPPI == 1
#define DataSyncBarrier()	asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")
#define DataMemBarrier() 	asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")
#define CleanDataCache()	asm volatile ("mcr p15, 0, %0, c7, c10, 0\n" \
					      "mcr p15, 0, %0, c7, c10, 4\n" : : "r" (0) : "memory")
#define InvalidateInstructionCache()	\
				asm volatile ("mcr p15, 0, %0, c7, c5,  0" : : "r" (0) : "memory")
#define FlushPrefetchBuffer()	asm volatile ("mcr p15, 0, %0, c7, c5,  4" : : "r" (0) : "memory")
#define FlushBranchTargetCache()	\
				asm volatile ("mcr p15, 0, %0, c7, c5,  6" : : "r" (0) : "memory")
#define InstructionSyncBarrier() FlushPrefetchBuffer()
#endif

static volatile unsigned int s_nCriticalLevel = 0;
static volatile uint32_t s_nCPSR[MAX_CRITICAL_LEVEL];

void EnterCritical(unsigned nTargetLevel)
{
	//assert (nTargetLevel == IRQ_LEVEL || nTargetLevel == FIQ_LEVEL);

	uint32_t nCPSR;
	asm volatile ("mrs %0, cpsr" : "=r" (nCPSR));

	// if we are already on FIQ_LEVEL, we must not go back to IRQ_LEVEL here
	//assert (nTargetLevel == FIQ_LEVEL || !(nCPSR & 0x40));

	DisableIRQs ();
	if (nTargetLevel == FIQ_LEVEL)
	{
		DisableFIQs ();
	}

	//assert (s_nCriticalLevel < MAX_CRITICAL_LEVEL);
	s_nCPSR[s_nCriticalLevel++] = nCPSR;

	DataMemBarrier ();
}

void LeaveCritical(void)
{
	DataMemBarrier ();

	//assert (s_nCriticalLevel > 0);
	uint32_t nCPSR = s_nCPSR[--s_nCriticalLevel];

	asm volatile ("msr cpsr_c, %0" :: "r" (nCPSR));
}

#if RASPPI == 1
#define DATA_CACHE_LINE_LENGTH		32
void CleanAndInvalidateDataCacheRange (uint32_t nAddress,
                                       uint32_t nLength)
{

	nLength += DATA_CACHE_LINE_LENGTH;

	while (1)
	{
		asm volatile ("mcr p15, 0, %0, c7, c14,  1" : : "r" (nAddress) : "memory");

		if (nLength < DATA_CACHE_LINE_LENGTH)
		{
			break;
		}

		nAddress += DATA_CACHE_LINE_LENGTH;
		nLength  -= DATA_CACHE_LINE_LENGTH;
	}

	DataSyncBarrier ();
}
#endif

void SyncDataAndInstructionCache (void)
{
	CleanDataCache ();
	//DataSyncBarrier ();		// included in CleanDataCache()

	InvalidateInstructionCache ();
	FlushBranchTargetCache ();
	DataSyncBarrier ();

	InstructionSyncBarrier ();
}
