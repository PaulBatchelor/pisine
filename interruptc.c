#include <stdint.h>
#include <stdlib.h>
#include "bcm2835.h"
#include "interrupt.h"
#include "sync.h"
#include "mem.h"

#define FIQ_CTL ARM_IO_BASE + ARM_IC_BASE + ARM_IC_FIQ_CTL

#define IC_BASE ARM_IO_BASE + ARM_IC_BASE
#define ENABLE_IRQS_1 IC_BASE + ARM_IC_ENABLE_IRQS_1
#define ENABLE_IRQS_2 IC_BASE + ARM_IC_ENABLE_IRQS_2
#define ENABLE_IRQS_BASIC IC_BASE + ARM_IC_ENABLE_IRQS_BASIC
#define IRQ_PENDING_1 IC_BASE + ARM_IC_IRQ_PENDING_1
#define IRQ_PENDING_2 IC_BASE + ARM_IC_IRQ_PENDING_2
#define IRQ_PENDING_BASIC IC_BASE + ARM_IC_IRQ_PENDING_BASIC



/* bcm interrupts */

#define IRQ_LINES		(ARM_IRQS_PER_REG * 2 + 8)
#define ARM_IRQS_PER_REG	32
#define ARM_IRQ1_BASE		0
#define ARM_IRQ2_BASE		(ARM_IRQ1_BASE + ARM_IRQS_PER_REG)
#define ARM_IRQBASIC_BASE	(ARM_IRQ2_BASE + ARM_IRQS_PER_REG)

#define ARM_EXCEPTION_TABLE_BASE	0x00000004
#define ARM_OPCODE_BRANCH(distance)	(0xEA000000 | (distance))
#define ARM_DISTANCE(from, to) ((uint32_t *) &(to) - (uint32_t *) &(from) - 2)

#define	EnableIRQs()		asm volatile ("cpsie i")
#define	DisableIRQs()		asm volatile ("cpsid i")

void FIQStub(void);
void IRQStub(void);

#if RASPPI == 1
#define PeripheralEntry()	DataSyncBarrier()
#define PeripheralExit()	DataMemBarrier()
#define DataSyncBarrier()	asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")
#define DataMemBarrier() 	asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")
#endif

#define ARM_IC_IRQ_REGS		3

#define ARM_IC_IRQS_ENABLE(irq)	(  (irq) < ARM_IRQ2_BASE	\
				 ? ENABLE_IRQS_1		\
				 : ((irq) < ARM_IRQBASIC_BASE	\
				   ? ENABLE_IRQS_2	\
				   : ENABLE_IRQS_BASIC))

#define ARM_IRQ_MASK(irq)	(1 << ((irq) & (ARM_IRQS_PER_REG-1)))

typedef struct {
	uint32_t UndefinedInstruction;
	uint32_t SupervisorCall;
	uint32_t PrefetchAbort;
	uint32_t DataAbort;
	uint32_t Unused;
	uint32_t IRQ;
	uint32_t FIQ;
} TExceptionTable;

struct interrupt_d {
	TIRQHandler	*m_apIRQHandler[IRQ_LINES];
	void *m_pParam[IRQ_LINES];
};

static uint32_t read32 (uintptr_t nAddress)
{
	return *(uint32_t volatile *) nAddress;
}

static void write32 (uintptr_t nAddress, uint32_t nValue)
{
	*(uint32_t volatile *) nAddress = nValue;
}

void interrupt_new(interrupt_d **i)
{
    interrupt_d *ip;
    ip = calloc(1, sizeof(interrupt_d));
    *i = ip;
}

void interrupt_del(interrupt_d **i)
{
    free(*i);
}

/* can we get rid of s_this */
static interrupt_d *s_this = 0;

void interrupt_init(interrupt_d *i)
{
	for (unsigned int nIRQ = 0; nIRQ < IRQ_LINES; nIRQ++)
	{
		i->m_apIRQHandler[nIRQ] = 0;
		i->m_pParam[nIRQ] = 0;
	}

	TExceptionTable *pTable = (TExceptionTable *)
        ARM_EXCEPTION_TABLE_BASE;

	pTable->IRQ = ARM_OPCODE_BRANCH (ARM_DISTANCE (pTable->IRQ, IRQStub));
	pTable->FIQ = ARM_OPCODE_BRANCH (ARM_DISTANCE (pTable->FIQ, FIQStub));

	SyncDataAndInstructionCache ();
	EnableIRQs ();
    /* TODO: try to remove this hack */
    s_this = i;
}

void interrupt_connect_irq(interrupt_d *i,
                           unsigned int nIRQ,
                           TIRQHandler *pHandler,
                           void *pParam)
{
	i->m_apIRQHandler[nIRQ] = pHandler;
	i->m_pParam[nIRQ] = pParam;

	interrupt_enable_irq(i, nIRQ);
}

void interrupt_disconnect_irq(interrupt_d *i,
                              unsigned int nIRQ)
{
	interrupt_disable_irq(i, nIRQ);
	i->m_apIRQHandler[nIRQ] = 0;
	i->m_pParam[nIRQ] = 0;
}

void interrupt_enable_irq(interrupt_d *i,
                          unsigned int nIRQ)
{
	PeripheralEntry ();

	write32 (ARM_IC_IRQS_ENABLE (nIRQ), ARM_IRQ_MASK (nIRQ));

	PeripheralExit ();
}

void interrupt_disable_irq(interrupt_d *i,
                           unsigned int nIRQ)
{
	PeripheralEntry ();

	write32 (FIQ_CTL, 0);

	PeripheralExit ();
}

int interrupt_call_irq_handler(interrupt_d *i, unsigned nIRQ)
{
	//assert (nIRQ < IRQ_LINES);
	TIRQHandler *pHandler = i->m_apIRQHandler[nIRQ];

	if (pHandler != 0) {
		(*pHandler) (i->m_pParam[nIRQ]);
		return 1;
	} else {
		interrupt_disable_irq(i, nIRQ);
	}

	return 0;
}

static void the_handler(void)
{
	PeripheralEntry ();

	uint32_t Pending[ARM_IC_IRQ_REGS];
	Pending[0] = read32 (IRQ_PENDING_1);
	Pending[1] = read32 (IRQ_PENDING_2);
	Pending[2] = read32 (IRQ_PENDING_BASIC) & 0xFF;

	PeripheralExit ();

	for (unsigned int nReg = 0; nReg < ARM_IC_IRQ_REGS; nReg++)
	{
		uint32_t nPending = Pending[nReg];
		if (nPending != 0)
		{
			unsigned int nIRQ = nReg * ARM_IRQS_PER_REG;

			do
			{
                /* TODO: can we get rid of s_this? */
				if (   (nPending & 1)
				    && interrupt_call_irq_handler(s_this, nIRQ))
				{
					return;
				}

				nPending >>= 1;
				nIRQ++;
			}
			while (nPending != 0);
		}
	}
}

void InterruptHandler (void)
{
	PeripheralExit ();

	the_handler();

	PeripheralEntry ();
}
