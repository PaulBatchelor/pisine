#include <stdint.h>
#include <stdlib.h>
#include "bcm2835.h"
#include "interrupt.h"
#include "gpio.h"
#include "pwm.h"
#include "del.h"
#include "mem.h"
#include "sync.h"

#define ARM_IRQ1_BASE		0
#define ARM_IRQ_DMA0		(ARM_IRQ1_BASE + 16)

#define SOUND_HW_CHANNELS 2
#define SOUND_MAX_SAMPLE_SIZE (sizeof (uint32_t))
#define SOUND_MAX_FRAME_SIZE (SOUND_HW_CHANNELS * SOUND_MAX_SAMPLE_SIZE)
#define CLOCK_FREQ 500000000
#define CLOCK_DIVIDER 2


#define CS_RESET (1 << 31)
#define CS_WAIT_FOR_OUTSTANDING_WRITES	(1 << 28)
#define CS_PANIC_PRIORITY_SHIFT 20
#define DEFAULT_PANIC_PRIORITY 15
#define CS_PRIORITY_SHIFT 16
#define DEFAULT_PRIORITY 1
#define CS_ERROR (1 << 8)
#define CS_ACTIVE (1 << 0)
#define DMA_BASE ARM_IO_BASE + ARM_DMA0_BASE
#define ARM_DMACHAN_CONBLK_AD(chan)	(DMA_BASE + ((chan) * 0x100) + 0x04)
#define TI_PERMAP_SHIFT 16
#define TI_BURST_LENGTH_SHIFT 12
#define DEFAULT_BURST_LENGTH 0
#define TI_SRC_WIDTH (1 << 9)
#define TI_SRC_INC (1 << 8)
#define TI_DEST_DREQ (1 << 6)
#define TI_WAIT_RESP (1 << 3)
#define TI_INTEN (1 << 0)
#define ARM_DMACHAN_NEXTCONBK(chan)	(DMA_BASE + ((chan) * 0x100) + 0x1C)
#define ARM_DMA_INT_STATUS (DMA_BASE + 0xFE0)
#define ARM_DMA_ENABLE (DMA_BASE + 0xFF0)
#define DMA_CHANNEL_PWM 0


#define ARM_PWM_CTL_PWEN1	(1 << 0)
#define ARM_PWM_CTL_RPTL1	(1 << 2)
#define ARM_PWM_CTL_USEF1	(1 << 5)
#define ARM_PWM_CTL_CLRF1	(1 << 6)
#define ARM_PWM_CTL_PWEN2	(1 << 8)
#define ARM_PWM_CTL_RPTL2	(1 << 10)
#define ARM_PWM_CTL_USEF2	(1 << 13)

#define ARM_PWM_DMAC_DREQ__SHIFT 0
#define ARM_PWM_DMAC_PANIC__SHIFT 8
#define ARM_PWM_DMAC_ENAB (1 << 31)

#define PWM_BASE (ARM_IO_BASE + ARM_PWM_BASE)
#define PWM_RNG1 (PWM_BASE + ARM_PWM_RNG1)
#define PWM_RNG2 (PWM_BASE + ARM_PWM_RNG2)
#define PWM_CTL (PWM_BASE + ARM_PWM_CTL)
#define PWM_DMAC (PWM_BASE + ARM_PWM_DMAC)
#define PWM_FIF1 (PWM_BASE + ARM_PWM_FIF1)

typedef void TSoundNeedDataCallback (void *pParam);

enum TPWMSoundState
{
	PWMSoundIdle,
	PWMSoundRunning,
	PWMSoundCancelled,
	PWMSoundTerminating,
	PWMSoundError,
	PWMSoundUnknown
};

enum TDREQ
{
	DREQSourcePWM	 = 5,
};

typedef struct
{
	uint32_t nTransferInformation;
	uint32_t nSourceAddress;
	uint32_t nDestinationAddress;
	uint32_t nTransferLength;
	uint32_t n2DModeStride;
	uint32_t nNextControlBlockAddress;
	uint32_t nReserved[2];
} TDMAControlBlock;

struct pwm_d {
	unsigned int m_nChunkSize;
	unsigned int m_nRange;
    gpioclock_d *clock;
	gpiopin_d *audio1;
	gpiopin_d *audio2;
	uint32_t *m_pDMABuffer[2];
	uint8_t *m_pControlBlockBuffer[2];
	TDMAControlBlock *m_pControlBlock[2];
	unsigned int m_nNextBuffer;
    interrupt_d *interrupt;
	unsigned int m_nQueueSize;
	unsigned int m_nHWFrameSize;
	unsigned int m_nSampleRate;
	uint8_t *m_pQueue;
	unsigned int m_nNeedDataThreshold;
	enum TSoundFormat m_WriteFormat;
	unsigned int m_nWriteChannels;
	unsigned int m_nWriteSampleSize;
	unsigned int m_nWriteFrameSize;
	enum TSoundFormat m_HWFormat;
	unsigned int m_nHWSampleSize;
	unsigned int m_nInPtr;
	unsigned int m_nOutPtr;
	int m_nRangeMin;
	int m_nRangeMax;
	uint8_t m_NullFrame[SOUND_MAX_FRAME_SIZE];
	TSoundNeedDataCallback *m_pCallback;
	void *m_pCallbackParam;
};

#if RASPPI == 1
#define PeripheralEntry()	DataSyncBarrier()
#define PeripheralExit()	DataMemBarrier()
#define DataSyncBarrier()	asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")
#define DataMemBarrier() 	asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")
#endif

#define ARM_DMACHAN_CS(chan) (DMA_BASE + ((chan) * 0x100) + 0x00)

static uint32_t read32 (uintptr_t nAddress)
{
	return *(uint32_t volatile *) nAddress;
}

static void write32 (uintptr_t nAddress, uint32_t nValue)
{
	*(uint32_t volatile *) nAddress = nValue;
}

void pwm_new(pwm_d **p)
{
    pwm_d *pp;
    pp = calloc(1, sizeof(pwm_d));
    *p = pp;
}

void pwm_del(pwm_d **p)
{
    free(*p);
}

void pwm_init(pwm_d *p,
              unsigned int sr,
              unsigned int chunksz,
              interrupt_d *i)
{
    p->m_nChunkSize = chunksz;
    p->m_nRange =
    ((CLOCK_FREQ / CLOCK_DIVIDER + sr/2) / sr);
    p->m_nSampleRate = sr;

    gpioclock_new(&p->clock);
    gpioclock_init(p->clock, GPIOClockPWM, GPIOClockSourcePLLD);

    gpiopin_new(&p->audio1);
	gpiopin_init(p->audio1,
                 GPIOPinAudioLeft,
                 GPIOModeAlternateFunction0);

    gpiopin_new(&p->audio2);
	gpiopin_init(p->audio2,
                 GPIOPinAudioRight,
                 GPIOModeAlternateFunction0);

    p->m_nQueueSize = 0;
    p->m_nNeedDataThreshold = 0;
    p->m_nWriteChannels = 0;
    p->m_pQueue = 0;
    p->m_nInPtr = 0;
    p->m_nOutPtr = 0;
    p->m_pCallback = 0;

    p->m_HWFormat = SoundFormatUnsigned32;

    uint32_t nRange32 = (CLOCK_FREQ / CLOCK_DIVIDER + sr/2) / sr;
    p->m_nSampleRate = sr;

	memset (p->m_NullFrame, 0, sizeof(p->m_NullFrame));

    p->m_nHWSampleSize = sizeof (uint32_t);
    p->m_nRangeMin = 0;
    p->m_nRangeMax = (int) (nRange32-1);
    uint32_t *pSample32 = (uint32_t *) (p->m_NullFrame);
    pSample32[0] = pSample32[1] = p->m_nRangeMax / 2;

	p->m_nHWFrameSize = SOUND_HW_CHANNELS * p->m_nHWSampleSize;
    p->interrupt = i;

    pwm_setup_dma_control_block(p, 0);
    pwm_setup_dma_control_block(p, 1);

	p->m_pControlBlock[0]->nNextControlBlockAddress =
        (uint32_t) p->m_pControlBlock[1] + ARM_MEM_BASE;
	p->m_pControlBlock[1]->nNextControlBlockAddress =
        (uint32_t) p->m_pControlBlock[0] + ARM_MEM_BASE;

    pwm_runpwm(p);

	PeripheralEntry ();
	write32(ARM_DMA_ENABLE,
            read32(ARM_DMA_ENABLE) | (1 << DMA_CHANNEL_PWM));

    del_us(1000);

	write32(ARM_DMACHAN_CS (DMA_CHANNEL_PWM), CS_RESET);

	while (read32 (ARM_DMACHAN_CS (DMA_CHANNEL_PWM)) & CS_RESET)
	{
	}

	p->m_WriteFormat = SoundFormatSigned16;

	p->m_nWriteChannels = 1;
    p->m_nWriteSampleSize = sizeof (int16_t);
	p->m_nWriteFrameSize = p->m_nWriteChannels *
        p->m_nWriteSampleSize;

	PeripheralExit ();
}

void pwm_start(pwm_d *p)
{
	if (!pwm_getnextchunk(p)) {
		return;
	}

    interrupt_connect_irq(p->interrupt,
                          ARM_IRQ_DMA0+DMA_CHANNEL_PWM,
                          pwm_interrupt_stub,
                          p);

    PeripheralEntry ();

	write32 (PWM_DMAC,
             ARM_PWM_DMAC_ENAB
             | (7 << ARM_PWM_DMAC_PANIC__SHIFT)
             | (7 << ARM_PWM_DMAC_DREQ__SHIFT));

	write32 (PWM_CTL,
             read32(PWM_CTL) &
             ~(ARM_PWM_CTL_RPTL1 | ARM_PWM_CTL_RPTL2));

	PeripheralExit ();

	PeripheralEntry ();

	write32 (ARM_DMACHAN_CONBLK_AD (DMA_CHANNEL_PWM),
             (uint32_t) p->m_pControlBlock[0] + ARM_MEM_BASE);


    write32 (ARM_DMACHAN_CS (DMA_CHANNEL_PWM),
             CS_WAIT_FOR_OUTSTANDING_WRITES
             | (DEFAULT_PANIC_PRIORITY << CS_PANIC_PRIORITY_SHIFT)
             | (DEFAULT_PRIORITY << CS_PRIORITY_SHIFT)
             | CS_ACTIVE);

	PeripheralExit ();
}

void pwm_alloc_queue(pwm_d *p, unsigned int msecs)
{
    p->m_nQueueSize = (p->m_nHWFrameSize *
                       p->m_nSampleRate *
                       msecs + 999) / 1000 + 1;

    p->m_pQueue = calloc(1, p->m_nQueueSize);

	p->m_nNeedDataThreshold = p->m_nQueueSize / 2;
}

unsigned int pwm_sizeframes(pwm_d *p)
{
	return p->m_nQueueSize / p->m_nHWFrameSize;
}

unsigned int pwm_framesavail(pwm_d *p)
{
	unsigned int nQueueBytesAvail = pwm_bytes_avail(p);

	return nQueueBytesAvail / p->m_nHWFrameSize;
}

int pwm_write(pwm_d *p, const void *pbuf, unsigned int count)
{
	const uint8_t *pBuffer8 = (const uint8_t *) pbuf;
	int nResult = 0;
    while (count >= p->m_nWriteFrameSize
            && pwm_bytes_free(p) >= p->m_nHWFrameSize)
    {
        uint8_t Frame[SOUND_MAX_FRAME_SIZE];

        /* conversion */

        int32_t nValue = 0;

        const int16_t *pValue16 = (const int16_t *) pBuffer8;
        nValue = *pValue16;
        nValue <<= 16;

        int64_t llValue = (int64_t) nValue;
        llValue += 1U << 31;
        llValue *= p->m_nRangeMax;
        llValue >>= 32;

        uint32_t *pValue32 = (uint32_t *) (Frame);
        *pValue32 = (uint32_t) llValue;


        pBuffer8 += p->m_nWriteSampleSize;

        memcpy (Frame+p->m_nHWSampleSize,
                Frame,
                p->m_nHWSampleSize);

        pwm_enqueue(p, Frame, p->m_nHWFrameSize);

        count -= p->m_nWriteFrameSize;
        nResult += p->m_nWriteFrameSize;
    }

	return nResult;
}

unsigned int pwm_getchunk(pwm_d *p,
                          void *buf,
                          unsigned int sz)
{
	uint8_t *pBuffer8 = (uint8_t *)buf;
	unsigned int nChunkSizeBytes = sz * p->m_nHWSampleSize;

	unsigned int nQueueBytesAvail = pwm_bytes_avail(p);
	unsigned int nBytes = nQueueBytesAvail;

	if (nBytes > nChunkSizeBytes) {
		nBytes = nChunkSizeBytes;
	}

	if (nBytes > 0) {
        pwm_dequeue(p, pBuffer8, nBytes);

		pBuffer8 += nBytes;
		nQueueBytesAvail -= nBytes;
	}

	while (nBytes < nChunkSizeBytes) {
		memcpy (pBuffer8, p->m_NullFrame, p->m_nHWFrameSize);

		pBuffer8 += p->m_nHWFrameSize;
		nBytes += p->m_nHWFrameSize;
	}

	if (p->m_pCallback != 0
	    && nQueueBytesAvail < p->m_nNeedDataThreshold)
	{
		(*p->m_pCallback) (p->m_pCallbackParam);
	}

	return sz;
}

int pwm_getnextchunk(pwm_d *p) {
	unsigned int nChunkSize =
        pwm_getchunk(p,
                     p->m_pDMABuffer[p->m_nNextBuffer],
                     p->m_nChunkSize);
	unsigned int nTransferLength = nChunkSize * sizeof (uint32_t);

	p->m_pControlBlock[p->m_nNextBuffer]->nTransferLength =
        nTransferLength;

	CleanAndInvalidateDataCacheRange (
        (uint32_t) p->m_pDMABuffer[p->m_nNextBuffer],
        nTransferLength);
	CleanAndInvalidateDataCacheRange (
        (uint32_t) p->m_pControlBlock[p->m_nNextBuffer],
        sizeof(TDMAControlBlock));

	p->m_nNextBuffer ^= 1;

	return 1;
}

void pwm_runpwm(pwm_d *p)
{
	PeripheralEntry ();

    gpioclock_start(p->clock, CLOCK_DIVIDER, 0, 0);
    del_us(2000);

	write32 (PWM_RNG1, p->m_nRange);
	write32 (PWM_RNG2, p->m_nRange);

	write32 (PWM_CTL,
             ARM_PWM_CTL_PWEN1 | ARM_PWM_CTL_USEF1
             | ARM_PWM_CTL_PWEN2 | ARM_PWM_CTL_USEF2
             | ARM_PWM_CTL_CLRF1);

    del_us(2000);

	PeripheralExit ();
}

void pwm_clean(pwm_d *p)
{
    pwm_stop(p);

    interrupt_disconnect_irq(p->interrupt,
                             ARM_IRQ_DMA0+DMA_CHANNEL_PWM);

    free(p->m_pControlBlockBuffer[0]);
    free(p->m_pControlBlockBuffer[1]);

    free(p->m_pDMABuffer[0]);
    free(p->m_pDMABuffer[1]);
    gpioclock_stop(p->clock);
    gpioclock_del(&p->clock);

    gpiopin_del(&p->audio2);
    gpiopin_del(&p->audio1);
}

void pwm_stop(pwm_d *p)
{
	PeripheralEntry ();

	write32 (PWM_DMAC, 0);
	write32 (PWM_CTL, 0);
    del_us(2000);

    gpioclock_stop(p->clock);
    del_us(2000);

	PeripheralExit ();
}

void pwm_interrupthandler(pwm_d *p)
{
	PeripheralEntry ();

	uint32_t nIntMask = 1 << DMA_CHANNEL_PWM;
	write32 (ARM_DMA_INT_STATUS, nIntMask);

	uint32_t nCS = read32 (ARM_DMACHAN_CS (DMA_CHANNEL_PWM));
	write32 (ARM_DMACHAN_CS (DMA_CHANNEL_PWM), nCS);	// reset CS_INT

	PeripheralExit ();

    pwm_getnextchunk(p);
}

void pwm_setup_dma_control_block(pwm_d *p, unsigned int nID)
{
	p->m_pDMABuffer[nID] = calloc(1, sizeof(uint32_t) * p->m_nChunkSize);

	p->m_pControlBlockBuffer[nID] =
        calloc(1, sizeof(uint8_t) *
               (sizeof(TDMAControlBlock) +  31));

    p->m_pControlBlock[nID] = (TDMAControlBlock *)
        (((uint32_t) p->m_pControlBlockBuffer[nID] + 31) & ~31);

	p->m_pControlBlock[nID]->nTransferInformation =
        (DREQSourcePWM << TI_PERMAP_SHIFT)
        | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
        | TI_SRC_WIDTH
        | TI_SRC_INC
        | TI_DEST_DREQ
        | TI_WAIT_RESP
        | TI_INTEN;
    p->m_pControlBlock[nID]->nSourceAddress =
        (uint32_t) p->m_pDMABuffer[nID] + ARM_MEM_BASE;
	p->m_pControlBlock[nID]->nDestinationAddress =
        (PWM_FIF1 & 0xFFFFFF) + GPU_IO_BASE;
	p->m_pControlBlock[nID]->n2DModeStride = 0;
	p->m_pControlBlock[nID]->nReserved[0] = 0;
	p->m_pControlBlock[nID]->nReserved[1] = 0;
}

unsigned int pwm_bytes_free(pwm_d *p)
{
	if (p->m_nOutPtr <= p->m_nInPtr)
	{
		return p->m_nQueueSize+p->m_nOutPtr-p->m_nInPtr-1;
	}

	return p->m_nOutPtr - p->m_nInPtr - 1;
}

unsigned int pwm_bytes_avail(pwm_d *p)
{
	if (p->m_nInPtr < p->m_nOutPtr)
	{
		return p->m_nQueueSize + p->m_nInPtr - p->m_nOutPtr;
	}

	return p->m_nInPtr - p->m_nOutPtr;
}

void pwm_enqueue(pwm_d *p, const void *data, unsigned int count)
{

	const uint8_t *buf = (const uint8_t *)data;
	while (count-- > 0)
	{
		p->m_pQueue[p->m_nInPtr] = *buf++;

		if (++p->m_nInPtr == p->m_nQueueSize)
		{
			p->m_nInPtr = 0;
		}
	}
}

void pwm_dequeue(pwm_d *p, void *data, unsigned int count)
{
	uint8_t *buf = (uint8_t *)data;
	while (count -- > 0) {
		*buf++ = p->m_pQueue[p->m_nOutPtr];

		if (++p->m_nOutPtr == p->m_nQueueSize) {
			p->m_nOutPtr = 0;
		}
	}
}

void pwm_interrupt_stub(void *pParam)
{
	pwm_d *p = (pwm_d *) pParam;
    pwm_interrupthandler(p);
}
