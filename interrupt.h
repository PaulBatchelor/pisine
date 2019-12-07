#ifndef INTERRUPT_H
#define INTERRUPT_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct interrupt_d interrupt_d;
typedef void TIRQHandler (void *pParam);

void interrupt_new(interrupt_d **i);
void interrupt_del(interrupt_d **i);
void interrupt_init(interrupt_d *i);
void interrupt_connect_irq(interrupt_d *i,
                           unsigned int nIRQ,
                           TIRQHandler *pHandler,
                           void *pParam);
void interrupt_disconnect_irq(interrupt_d *i,
                              unsigned int nIRQ);
void interrupt_enable_irq(interrupt_d *i,
                          unsigned int nIRQ);
void interrupt_disable_irq(interrupt_d *i,
                           unsigned int nIRQ);
#ifdef __cplusplus
}
#endif
#endif
