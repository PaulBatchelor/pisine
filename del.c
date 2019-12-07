#include <stdint.h>
#include "bcm2835.h"
#include "del.h"

#if RASPPI == 1
#define DataSyncBarrier()	asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")
#define DataMemBarrier() 	asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")
#define PeripheralEntry()	DataSyncBarrier()
#define PeripheralExit()	DataMemBarrier()
#endif

#define CLOCKHZ	1000000

#define TIMER ARM_IO_BASE + ARM_TIMER_BASE + ARM_TIMER_CNT

static uint32_t read32 (uintptr_t nAddress)
{
	return *(uint32_t volatile *) nAddress;
}

void del_us(unsigned int us)
{
	if (us > 0) {
		unsigned nStartTicks;
		unsigned nTicks;
		nTicks = us * (CLOCKHZ / 1000000) + 1;

		PeripheralEntry ();

		nStartTicks = read32 (TIMER);
		while (read32 (TIMER) - nStartTicks < nTicks);
		PeripheralExit ();
	}
}

void del_ms(unsigned int ms)
{
	if (ms > 0) {
        del_us(ms * 1000);
	}
}
