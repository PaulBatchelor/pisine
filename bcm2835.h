#ifndef BCM2835_H
#define BCM2835_H
/* from https://github.com/PeterLemon/RaspberryPi/ */
#if RASPPI == 1
#define ARM_IO_BASE 0x20000000
#else
#define ARM_IO_BASE 0x3F000000
#endif

/* Memory */

/* bus address for l2 cache disabled */
#define ARM_MEM_BASE 0xC0000000

/* not sure what this is */
#define GPU_IO_BASE		0x7E000000

/* GPIO */

#define ARM_GPIO_BASE 0x2000000
#define ARM_GPIO_GPFSEL0 0x0
#define ARM_GPIO_GPSET0 0x1C
#define ARM_GPIO_GPCLR0 0x28
#define ARM_GPIO_GPREN0 0x4C
#define ARM_GPIO_GPAFENO 0x88
#define ARM_GPIO_GPPUD 0x94
#define ARM_GPIO_GPPUDCLK0 0x98

/* Clock */

#define ARM_TIMER_BASE 0x3000
#define ARM_TIMER_CNT 0x4

/* PWM */
#define ARM_PWM_BASE 0x20C000
#define ARM_PWM_CTL 0x0
#define ARM_PWM_DMAC 0x8
#define ARM_PWM_RNG1 0x10
#define ARM_PWM_FIF1 0x18
#define ARM_PWM_RNG2 0x20

#define ARM_CM_BASE 0x101000
#define ARM_CM_GP0CTL 0x70
#define ARM_CM_GP0DIV 0x74
#define ARM_CM_PASSWD 0x5A000000

/* Interrupt control */

/* taken from section 7.5 */

#define ARM_IC_BASE 0xB000
#define ARM_IC_IRQ_PENDING_BASIC 0x200
#define ARM_IC_IRQ_PENDING_1 0x204
#define ARM_IC_IRQ_PENDING_2 0x208
#define ARM_IC_FIQ_CTL 0x20C
#define ARM_IC_ENABLE_IRQS_1 0x210
#define ARM_IC_ENABLE_IRQS_2 0x214
#define ARM_IC_ENABLE_IRQS_BASIC 0x218
#define ARM_IC_DISABLE_IRQS_1 0x21C
#define ARM_IC_DISABLE_IRQS_2 0x220
#define ARM_IC_DISABLE_IRQS_BASIC 0x224

/* DMA */
#define ARM_DMA0_BASE 0x7000

#endif
