#include <stdint.h>
#include <stdlib.h>
#include "bcm2835.h"
#include "gpio.h"
#include "del.h"
#include "mem.h"

#if RASPPI == 1
#define PeripheralEntry() DataSyncBarrier()
#define PeripheralExit() DataMemBarrier()
#define DataSyncBarrier() asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")
#define DataMemBarrier() asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")
#endif

#define CLK_CTL_MASH(x)		((x) << 9)
#define CLK_CTL_BUSY		(1 << 7)
#define CLK_CTL_KILL		(1 << 5)
#define CLK_CTL_ENAB		(1 << 4)
#define CLK_CTL_SRC(x)		((x) << 0)

#define CLK_DIV_DIVI(x)		((x) << 12)
#define CLK_DIV_DIVF(x)		((x) << 0)

#define GP0CTL ARM_IO_BASE + ARM_CM_BASE + ARM_CM_GP0CTL
#define GP0DIV ARM_IO_BASE + ARM_CM_BASE + ARM_CM_GP0DIV

struct gpioclock_d {
	enum TGPIOClock m_Clock;
	enum TGPIOClockSource m_Source;
};

struct gpiopin_d {
	unsigned int m_nPin;
	unsigned int m_nRegOffset;
	uint32_t m_nRegMask;
	enum TGPIOMode m_Mode;
	unsigned int m_nValue;

	void *m_pParam;
	enum TGPIOInterrupt m_Interrupt;
};

static uint32_t read32 (uintptr_t addr)
{
	return *(uint32_t volatile *) addr;
}

static void write32 (uintptr_t addr, uint32_t val)
{
	*(uint32_t volatile *) addr = val;
}

void gpioclock_new(gpioclock_d **clk)
{
    gpioclock_d *pclk;
    pclk = calloc(1, sizeof(gpioclock_d));
    *clk = pclk;
}

void gpioclock_del(gpioclock_d **clk)
{
    free(*clk);
}

void gpioclock_init(gpioclock_d *clk,
                    enum TGPIOClock clock,
                    enum TGPIOClockSource source)
{
    clk->m_Clock = clock;
    clk->m_Source = source;
}

void gpioclock_start(gpioclock_d *clk,
                     unsigned int nDivI,
                     unsigned int nDivF,
                     unsigned int nMASH)
{
	unsigned int nCtlReg = GP0CTL + (clk->m_Clock * 8);
	unsigned int nDivReg  = GP0DIV + (clk->m_Clock * 8);

    gpioclock_stop(clk);

	write32 (nDivReg, ARM_CM_PASSWD | CLK_DIV_DIVI (nDivI) | CLK_DIV_DIVF (nDivF));

    del_us(10);

	write32 (nCtlReg,
             ARM_CM_PASSWD |
             CLK_CTL_MASH (nMASH) |
             CLK_CTL_SRC (clk->m_Source));

    del_us(10);

	write32 (nCtlReg, read32 (nCtlReg) | ARM_CM_PASSWD | CLK_CTL_ENAB);

	PeripheralExit ();
}

void gpioclock_stop(gpioclock_d *clk)
{
	unsigned nCtlReg = GP0CTL + (clk->m_Clock * 8);

	PeripheralEntry ();

	write32 (nCtlReg, ARM_CM_PASSWD | CLK_CTL_KILL);
	while (read32 (nCtlReg) & CLK_CTL_BUSY);

	PeripheralExit ();
}

void gpiopin_new(gpiopin_d **pin)
{
    gpiopin_d *ppin;
    ppin = calloc(1, sizeof(gpiopin_d));
    *pin = ppin;
}

void gpiopin_del(gpiopin_d **pin)
{
    free(*pin);
}

void gpiopin_init(gpiopin_d *pin,
                  unsigned int nPin,
                  enum TGPIOMode mode)
{
    pin->m_nPin = GPIO_PINS;
    pin->m_Mode = GPIOModeUnknown;
    pin->m_Interrupt = GPIOInterruptUnknown;
    gpiopin_assign(pin, nPin);
}

static unsigned int get_gpio_pin(enum TGPIOVirtualPin Pin)
{
	unsigned nResult = 0;
	int m_MachineModel;
	int m_nModelMajor;


    /* PB: hard-coded values for Pi 1 model B */
    m_MachineModel = MachineModelBRelease2MB512;
    m_nModelMajor = 1;

	switch (Pin) {
        case GPIOPinAudioLeft:
            if (m_MachineModel <= MachineModelBRelease2MB512) {
                nResult = 40;
            } else {
                if (m_nModelMajor < 3) {
                    nResult = 45;
                } else {
                    nResult = 41;
                }
            }
            break;

        case GPIOPinAudioRight:
            if (m_MachineModel <= MachineModelBRelease2MB512) {
                nResult = 45;
            } else {
                nResult = 40;
            }
            break;

        default:
            break;
	}

	return nResult;
}


void gpiopin_assign(gpiopin_d *pin, unsigned int nPin)
{
	pin->m_nPin = nPin;

	if (pin->m_nPin >= GPIO_PINS) {
		pin->m_nPin = get_gpio_pin((enum TGPIOVirtualPin) nPin);
	}

	pin->m_nRegOffset = (pin->m_nPin / 32) * 4;
	pin->m_nRegMask = 1 << (pin->m_nPin % 32);
}
