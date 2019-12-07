#define	EnableIRQs()		asm volatile ("cpsie i")
#define	DisableIRQs()		asm volatile ("cpsid i")
#define	EnableInterrupts()	EnableIRQs()			// deprecated
#define	DisableInterrupts()	DisableIRQs()			// deprecated

#define	EnableFIQs()		asm volatile ("cpsie f")
#define	DisableFIQs()		asm volatile ("cpsid f")

#if RASPPI == 1
#define FlushPrefetchBuffer()	asm volatile ("mcr p15, 0, %0, c7, c5,  4" : : "r" (0) : "memory")
#define InstructionMemBarrier()	FlushPrefetchBuffer()
#else
#endif
void *__dso_handle;

/* void __aeabi_atexit (void *pThis, void (*pFunc)(void *pThis), void *pHandle) WEAK; */

void __aeabi_atexit (void *pThis, void (*pFunc)(void *pThis), void *pHandle)
{
	
}

void halt (void)
{
	DisableIRQs ();
#ifndef USE_RPI_STUB_AT
	DisableFIQs ();
#endif

	for (;;)
	{
	}
}

static void vfpinit (void)
{
	// Coprocessor Access Control Register
	unsigned nCACR;
	__asm volatile ("mrc p15, 0, %0, c1, c0, 2" : "=r" (nCACR));
	nCACR |= 3 << 20;	// cp10 (single precision)
	nCACR |= 3 << 22;	// cp11 (double precision)
	__asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r" (nCACR));
	InstructionMemBarrier ();

#define VFP_FPEXC_EN	(1 << 30)
	__asm volatile ("fmxr fpexc, %0" : : "r" (VFP_FPEXC_EN));

#define VFP_FPSCR_DN	(1 << 25)	// enable Default NaN mode
	__asm volatile ("fmxr fpscr, %0" : : "r" (VFP_FPSCR_DN));
}

void sysinit (void)
{
	EnableFIQs ();		// go to IRQ_LEVEL, EnterCritical() will not work otherwise

#if RASPPI != 1
#ifndef USE_RPI_STUB_AT
	// L1 data cache may contain random entries after reset, delete them
	InvalidateDataCacheL1Only ();
#endif
#ifndef ARM_ALLOW_MULTI_CORE
	// put all secondary cores to sleep
	for (unsigned nCore = 1; nCore < CORES; nCore++)
	{
		write32 (ARM_LOCAL_MAILBOX3_SET0 + 0x10 * nCore, (u32) &_start_secondary);
	}
#endif
#endif

	vfpinit ();

	// clear BSS
	extern unsigned char __bss_start;
	extern unsigned char _end;
	for (unsigned char *pBSS = &__bss_start; pBSS < &_end; pBSS++)
	{
		*pBSS = 0;
	}

    /* PB: This is so stupid */
	// CMachineInfo MachineInfo;

#if STDLIB_SUPPORT >= 2
	CMemorySystem Memory;
#endif

	// call construtors of static objects
	extern void (*__init_start) (void);
	extern void (*__init_end) (void);
	for (void (**pFunc) (void) = &__init_start; pFunc < &__init_end; pFunc++)
	{
		(**pFunc) ();
	}

	extern int main (void);
    main();
	halt ();
}
