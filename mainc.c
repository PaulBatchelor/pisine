#include <stdlib.h>
#include "sp_base.h"
#include "ftbl.h"
#include "osc.h"
#include "mem.h"
#include "del.h"
#include "interrupt.h"
#include "gpio.h"
#include "pwm.h"

#define FORMAT		SoundFormatSigned16
#define TYPE		int16_t
#define TYPE_SIZE	sizeof (int16_t)
#define FACTOR		((1 << 15)-1)
#define NULL_LEVEL	0

#define SAMPLE_RATE	44100

#define QUEUE_SIZE_MSECS 100
#define CHUNK_SIZE	2000
#define FRAMES_PER_WRITE 1000

int main(void)
{
    mem_d *mem;
    interrupt_d *interrupt;
    pwm_d *pwm;
    sp_data sp;
    sp_ftbl *ft;
    sp_osc *osc;
    sp_osc *lfo;
    unsigned int nframes;
	unsigned int queue_sz;
    uint8_t buf[FRAMES_PER_WRITE * TYPE_SIZE];

    /* alloc */

    mem_new(&mem);
    mem_setup(mem);
    sp.sr = SAMPLE_RATE;
    sp_ftbl_create(&sp, &ft, 8192);
    sp_gen_sine(&sp, ft);
    sp_osc_create(&osc);
    sp_osc_init(&sp, osc, ft, 0);
    osc->freq = 500;
    osc->amp = 0.8;
    sp_osc_create(&lfo);
    sp_osc_init(&sp, lfo, ft, 0);
    lfo->freq = 1;
    lfo->amp = 200;

    /* init */

    interrupt_new(&interrupt);
    interrupt_init(interrupt);
    pwm_new(&pwm);
    pwm_init(pwm, SAMPLE_RATE, CHUNK_SIZE, interrupt);
    pwm_alloc_queue(pwm, QUEUE_SIZE_MSECS);

    /* run */

	queue_sz = pwm_sizeframes(pwm);
    pwm_start(pwm);

    while(1) {
        unsigned int i;
        del_ms(QUEUE_SIZE_MSECS / 2);
        nframes = queue_sz - pwm_framesavail(pwm);

        while (nframes > 0) {
            unsigned nwritebytes;
            unsigned nwriteframes = nframes < FRAMES_PER_WRITE ? nframes : FRAMES_PER_WRITE;

            for (i = 0; i < nwriteframes; i++) {
                float fLevel = 0;
                float tmp;
                TYPE nLevel;
                sp_osc_compute(&sp, lfo, 0, &tmp);
                osc->freq = 500 + tmp;
                sp_osc_compute(&sp, osc, 0, &fLevel);
                nLevel = (TYPE) (fLevel * ((1 << 15) - 1));
                memcpy (&buf[i * TYPE_SIZE], &nLevel, TYPE_SIZE);
            }

            nwritebytes = nwriteframes * 1 * TYPE_SIZE;
            pwm_write(pwm, buf, nwritebytes);
            nframes -= nwriteframes;
        }
	}

    /* free */

    interrupt_del(&interrupt);
    sp_ftbl_destroy(&ft);
    sp_osc_destroy(&osc);
    sp_osc_destroy(&lfo);
    mem_del(&mem);
    pwm_del(&pwm);
    return 0;
}
