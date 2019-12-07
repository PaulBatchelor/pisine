#include "bcm2835.h"

#define FIQ_CTL ARM_IO_BASE + ARM_IC_BASE + ARM_IC_FIQ_CTL

	.globl	IRQStub
IRQStub:
	sub	lr, lr, #4			/* lr: return address */
	stmfd	sp!, {r0-r12, lr}		/* save r0-r12 and return address */
	bl	InterruptHandler
	ldmfd	sp!, {r0-r12, pc}^		/* restore registers and return */

	.globl	FIQStub
FIQStub:
	sub	lr, lr, #4			/* lr: return address */
	stmfd	sp!, {r0-r3, lr}		/* save r0-r3 and return address */
#if RASPPI == 1
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 5		/* PeripheralExit() */
#endif
	ldr	r2, =FIQData
	ldr	r1, [r2]			/* get FIQData.pHandler */
	cmp	r1, #0				/* is handler set? */
	beq	1f
	ldr	r0, [r2, #4]			/* get FIQData.pParam */
	blx	r1				/* call handler (saves r4-r7) */
#if RASPPI == 1
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 4		/* PeripheralEntry() */
#endif
	ldmfd	sp!, {r0-r3, pc}^		/* restore registers and return */

1:
#if RASPPI == 1
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 4		/* PeripheralEntry() */
#endif
	ldr	r1, =FIQ_CTL		/* disable fiq (if handler is not set) */
	mov	r0, #0
	str	r0, [r1]
#if RASPPI == 1
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 5		/* PeripheralExit() */
	mcr	p15, 0, r3, c7, c10, 4		/* PeripheralEntry() */
#endif
	ldmfd	sp!, {r0-r3, pc}^		/* restore registers and return */

	.data

	.globl	FIQData
FIQData:					/* matches TFIQData: */
	.word	0				/* pHandler */
	.word	0				/* pParam */
