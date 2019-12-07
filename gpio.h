#ifndef GPIO_H
#define GPIO_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct gpioclock_d gpioclock_d;
typedef struct gpiopin_d gpiopin_d;

#include "gpio_enums.h"

void gpioclock_new(gpioclock_d **clk);
void gpioclock_del(gpioclock_d **clk);
void gpioclock_init(gpioclock_d *clk,
                    enum TGPIOClock clock,
                    enum TGPIOClockSource Source);

void gpioclock_start(gpioclock_d *clk,
                     unsigned int nDivI,
                     unsigned int nDivF,
                     unsigned int nMASH);
void gpioclock_stop(gpioclock_d *clk);


void gpiopin_new(gpiopin_d **pin);
void gpiopin_del(gpiopin_d **pin);
void gpiopin_init(gpiopin_d *pin,
                  unsigned int nPin,
                  enum TGPIOMode Mode);
void gpiopin_assign(gpiopin_d *pin, unsigned int nPin);
void gpiopin_setmode(gpiopin_d *pin,
                     enum TGPIOMode mode,
                     int bInitpin);
void gpiopin_write(gpiopin_d *pin, unsigned int nValue);
void gpiopin_set_pullupmode(gpiopin_d *pin,
                            unsigned int nMode);
void gpiopin_set_alternatefunction(gpiopin_d *pin,
                                   unsigned int nFunction);
void gpiopin_interrupthandler(gpiopin_d *pin);
void gpiopin_disableallinterrupts(gpiopin_d *pin,
                                  unsigned int nPin);
#ifdef __cplusplus
}
#endif
#endif
