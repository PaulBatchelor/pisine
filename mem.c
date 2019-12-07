#include <stdint.h>
#include <stdlib.h>

#include "memorymap.h"
#include "armv6mmu.h"
#include "mem.h"
#include "pgtbl.h"
#include "sync.h"

#define KERNEL_MAX_SIZE		(2 * MEGABYTE)

#define IRQ_LEVEL		1		/* IRQs disabled, FIQs enabled */

#if RASPPI == 1
#define MMU_MODE	(  ARM_CONTROL_MMU			\
			 | ARM_CONTROL_L1_CACHE			\
			 | ARM_CONTROL_L1_INSTRUCTION_CACHE	\
			 | ARM_CONTROL_BRANCH_PREDICTION	\
			 | ARM_CONTROL_EXTENDED_PAGE_TABLE)

#define TTBCR_SPLIT	3
#else
#define MMU_MODE	(  ARM_CONTROL_MMU			\
			 | ARM_CONTROL_L1_CACHE			\
			 | ARM_CONTROL_L1_INSTRUCTION_CACHE	\
			 | ARM_CONTROL_BRANCH_PREDICTION)

#define TTBCR_SPLIT	2
#endif

#if RASPPI == 1
#define DataSyncBarrier()	asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")

#define InvalidateInstructionCache()	\
				asm volatile ("mcr p15, 0, %0, c7, c5,  0" : : "r" (0) : "memory")

#define FlushBranchTargetCache()	\
				asm volatile ("mcr p15, 0, %0, c7, c5,  6" : : "r" (0) : "memory")

#define InvalidateDataCache()	asm volatile ("mcr p15, 0, %0, c7, c6,  0\n" \
					      "mcr p15, 0, %0, c7, c10, 4\n" : : "r" (0) : "memory")
#else
#define DataSyncBarrier()	asm volatile ("dsb" ::: "memory")
#endif

#define FlushPrefetchBuffer()	asm volatile ("mcr p15, 0, %0, c7, c5,  4" : : "r" (0) : "memory")
struct mem_d {
    uint32_t memsize;
    /* int enable_mmu; */
    pgtbl_d *pt0;
    pgtbl_d *pt1;
};

void mem_setup(mem_d *m)
{
    /* m->enable_mmu = enable_mmu; */
    m->memsize = 0;
    m->pt0 = 0;
    m->pt1 = 0;

    m->memsize = ARM_MEM_SIZE;
	mem_init (0, m->memsize);

    pgtbl_new(&m->pt0);
    pgtbl_init(m->pt0, m->memsize);

    pgtbl_new(&m->pt1);
    pgtbl_init_shared(m->pt1);

    mem_enable_mmu(m);
}

void mem_enable_mmu(mem_d *m)
{
	uint32_t nAuxControl;
	asm volatile ("mrc p15, 0, %0, c1, c0,  1" : "=r" (nAuxControl));
#if RASPPI == 1
	nAuxControl |= ARM_AUX_CONTROL_CACHE_SIZE;	// restrict cache size (no page coloring)
#else
	nAuxControl |= ARM_AUX_CONTROL_SMP;
#endif
	asm volatile ("mcr p15, 0, %0, c1, c0,  1" : : "r" (nAuxControl));

	uint32_t nTLBType;
	asm volatile ("mrc p15, 0, %0, c0, c0,  3" : "=r" (nTLBType));

	/* set TTB control */
	asm volatile ("mcr p15, 0, %0, c2, c0,  2" : : "r" (TTBCR_SPLIT));

	/* set TTBR0 */
	asm volatile ("mcr p15, 0, %0, c2, c0,  0" : : "r"
                  (pgtbl_get_baseaddr(m->pt0)));

	/* set TTBR1 */
	asm volatile ("mcr p15, 0, %0, c2, c0,  1" : : "r"
                  (pgtbl_get_baseaddr(m->pt1)));

	/* set Domain Access Control register (Domain 0 and 1 to client) */
	asm volatile ("mcr p15, 0, %0, c3, c0,  0" : : "r" (  DOMAIN_CLIENT << 0
							    | DOMAIN_CLIENT << 2));

	InvalidateDataCache ();

	InvalidateInstructionCache ();
	FlushBranchTargetCache ();
	asm volatile ("mcr p15, 0, %0, c8, c7,  0" : : "r" (0));	// invalidate unified TLB
	DataSyncBarrier ();
	FlushPrefetchBuffer ();

	/* enable MMU */
	uint32_t nControl;
	asm volatile ("mrc p15, 0, %0, c1, c0,  0" : "=r" (nControl));

#if RASPPI == 1
#ifdef ARM_STRICT_ALIGNMENT
	nControl &= ~ARM_CONTROL_UNALIGNED_PERMITTED;
	nControl |= ARM_CONTROL_STRICT_ALIGNMENT;
#else
	nControl &= ~ARM_CONTROL_STRICT_ALIGNMENT;
	nControl |= ARM_CONTROL_UNALIGNED_PERMITTED;
#endif
#endif

	nControl |= MMU_MODE;

	asm volatile ("mcr p15, 0, %0, c1, c0,  0" : : "r" (nControl) : "memory");
}

void mem_new(mem_d **m)
{
    mem_d *pm;
    pm = calloc(1, sizeof(mem_d));
    *m = pm;
}

void mem_del(mem_d **m)
{
    free(*m);
}

void mem_clean(mem_d *m)
{
    uint32_t nControl;
    asm volatile ("mrc p15, 0, %0, c1, c0,  0" : "=r" (nControl));
    nControl &=  ~MMU_MODE;
    asm volatile ("mcr p15, 0, %0, c1, c0,  0" : : "r" (nControl) : "memory");

    /* invalidate unified TLB (if MMU is re-enabled later) */
    asm volatile ("mcr p15, 0, %0, c8, c7,  0" : : "r" (0) : "memory");

    pgtbl_free(m->pt0);
    pgtbl_del(&m->pt0);
    pgtbl_free(m->pt1);
    pgtbl_del(&m->pt1);
}


typedef struct TFreePage
{
	unsigned int	nMagic;
#define FREEPAGE_MAGIC	0x50474D43
	struct TFreePage	*pNext;
} TFreePage;

typedef struct {
	TFreePage	*pFreeList;
} TPageBucket;

typedef struct TBlockHeader {
	unsigned int	nMagic;
#define BLOCK_MAGIC	0x424C4D43
	unsigned int	nSize;
	struct TBlockHeader	*pNext;
	unsigned int	nPadding;
	unsigned char	*Data;
} TBlockHeader;

typedef struct TBlockBucket {
	unsigned int	nSize;
	TBlockHeader	*pFreeList;
} TBlockBucket;

static TPageBucket s_PageBucket;
static unsigned char *s_pNextPage;
static unsigned char *s_pPageLimit;
static TBlockBucket s_BlockBucket[] = {{0x40}, {0x400}, {0x1000}, {0x4000}, {0x10000}, {0x40000}, {0x80000}, {0}};
static unsigned char *s_pNextBlock;
static unsigned char *s_pBlockLimit;
static unsigned int s_nBlockReserve = 0x40000;

#define BLOCK_ALIGN	16
#define ALIGN_MASK	(BLOCK_ALIGN-1)
#define PAGE_MASK	(PAGE_SIZE-1)

void *palloc (void)
{
	TFreePage *pFreePage;
	if ((pFreePage = s_PageBucket.pFreeList) != 0) {
		s_PageBucket.pFreeList = pFreePage->pNext;
		pFreePage->nMagic = 0;
	} else {
		pFreePage = (TFreePage *) s_pNextPage;
		s_pNextPage += PAGE_SIZE;

		if (s_pNextPage > s_pPageLimit) {
			return 0;
		}
	}

	return pFreePage;
}

void pfree (void *pPage)
{
	if (pPage == 0) {
		return;
	}

	TFreePage *pFreePage = (TFreePage *) pPage;

	pFreePage->nMagic = FREEPAGE_MAGIC;

	pFreePage->pNext = s_PageBucket.pFreeList;
	s_PageBucket.pFreeList = pFreePage;
}

void *malloc (size_t nSize)
{
	TBlockBucket *pBucket;
	for (pBucket = s_BlockBucket; pBucket->nSize > 0; pBucket++) {
		if (nSize <= pBucket->nSize) {
			nSize = pBucket->nSize;
			break;
		}
	}

	TBlockHeader *pBlockHeader;
	if (   pBucket->nSize > 0
	    && (pBlockHeader = pBucket->pFreeList) != 0) {
		pBucket->pFreeList = pBlockHeader->pNext;
	} else {
		pBlockHeader = (TBlockHeader *) s_pNextBlock;

		unsigned char *pNextBlock = s_pNextBlock;
		pNextBlock += (sizeof (TBlockHeader) + nSize + BLOCK_ALIGN-1) & ~ALIGN_MASK;

		if (   pNextBlock <= s_pNextBlock			// may have wrapped
		    || pNextBlock > s_pBlockLimit-s_nBlockReserve)
		{
			s_nBlockReserve = 0;
			return 0;
		}

		s_pNextBlock = pNextBlock;

		pBlockHeader->nMagic = BLOCK_MAGIC;
		pBlockHeader->nSize = (unsigned) nSize;
	}

	pBlockHeader->pNext = 0;

	void *pResult = &(*pBlockHeader).Data;

	return pResult;
}

void free (void *pBlock)
{
	if (pBlock == 0) {
		return;
	}

	TBlockHeader *pBlockHeader = (TBlockHeader *) ((unsigned long) pBlock - sizeof (TBlockHeader));

	for (TBlockBucket *pBucket = s_BlockBucket; pBucket->nSize > 0; pBucket++)
	{
		if (pBlockHeader->nSize == pBucket->nSize) {
			pBlockHeader->pNext = pBucket->pFreeList;
			pBucket->pFreeList = pBlockHeader;
			return;
		}
	}
}

void *calloc (size_t nBlocks, size_t nSize)
{
	nSize *= nBlocks;
	if (nSize == 0) {
		nSize = 1;
	}

	void *pNewBlock = malloc (nSize);

	if (pNewBlock != 0) {
		memset (pNewBlock, 0, nSize);
	}

	return pNewBlock;
}

void mem_init (unsigned long ulBase, unsigned long ulSize)
{
	unsigned long ulLimit = ulBase + ulSize;

	if (ulBase < MEM_HEAP_START)
	{
		ulBase = MEM_HEAP_START;
	}

	ulSize = ulLimit - ulBase;
	unsigned long ulBlockReserve = ulSize - PAGE_RESERVE;

	s_pNextBlock = (unsigned char *) ulBase;
	s_pBlockLimit = (unsigned char *) (ulBase + ulBlockReserve);

	s_pNextPage = (unsigned char *) ((ulBase + ulBlockReserve + PAGE_SIZE) & ~PAGE_MASK);
	s_pPageLimit = (unsigned char *) ulLimit;
}
