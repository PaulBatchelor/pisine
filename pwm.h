#ifndef PWM_H
#define PWM_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pwm_d pwm_d;

enum TSoundFormat
{
	SoundFormatUnsigned8,
	SoundFormatSigned16,
	SoundFormatSigned24,
	SoundFormatUnsigned32,
	SoundFormatUnknown
};

void pwm_new(pwm_d **p);
void pwm_del(pwm_d **p);
void pwm_init(pwm_d *p,
              unsigned int sr,
              unsigned int chunksz,
              interrupt_d *i);
void pwm_start(pwm_d *p);
void pwm_alloc_queue(pwm_d *p, unsigned int msecs);
unsigned int pwm_sizeframes(pwm_d *p);
unsigned int pwm_framesavail(pwm_d *p);
void pwm_write_format(pwm_d *p,
                      enum TSoundFormat Format,
                      unsigned int nChannels);
int pwm_write(pwm_d *p, const void *pbuf, unsigned int count);
unsigned int pwm_getchunk16(pwm_d *p,
                            int16_t *buf,
                            unsigned int sz);
unsigned int pwm_getchunk32(pwm_d *p,
                            uint32_t *buf,
                            unsigned int sz);
unsigned int pwm_getchunk(pwm_d *p,
                          void *buf,
                          unsigned int sz);
int pwm_getnextchunk(pwm_d *p);
void pwm_runpwm(pwm_d *p);
void pwm_stop(pwm_d *p);
void pwm_interrupthandler(pwm_d *p);
void pwm_setup_dma_control_block(pwm_d *p, unsigned int nID);
unsigned int pwm_bytes_free(pwm_d *p);
unsigned int pwm_bytes_avail(pwm_d *p);
void pwm_enqueue(pwm_d *p, const void *buf, unsigned int count);
void pwm_dequeue(pwm_d *p, void *buf, unsigned int count);
void pwm_convert_sound_format(pwm_d *p,
                              void *to,
                              const void *from);
void pwm_interrupt_stub(void *pParam);
#ifdef __cplusplus
}
#endif
#endif
