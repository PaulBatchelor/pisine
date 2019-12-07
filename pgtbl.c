#include <stdint.h>
#include <stdlib.h>
#include "armv6mmu.h"
/* #include "sysconfig.h" */
#include "memorymap.h"
/* #include "alloc.h" */
#include "pgtbl.h"
#include "mem.h"

#if RASPPI == 1
#define SDRAM_SIZE_MBYTE	512

#define TTBR_MODE	(  ARM_TTBR_INNER_CACHEABLE		\
			 | ARM_TTBR_OUTER_NON_CACHEABLE)
#else
#define SDRAM_SIZE_MBYTE	1024

#define TTBR_MODE	(  ARM_TTBR_INNER_WRITE_BACK		\
			 | ARM_TTBR_OUTER_WRITE_BACK)
#endif

struct pgtbl_d {
	int allocated;
	struct TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR *tbl;
};

static void clean_data_cache()
{
    asm volatile ("mcr p15, 0, %0, c7, c10, 0\n"
                  "mcr p15, 0, %0, c7, c10, 4\n" : :
                  "r" (0) : "memory");

}

void pgtbl_init_shared(pgtbl_d *p)
{
    unsigned int n;
    uint32_t base_addr;
	struct TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR *ent;

    p->allocated = 0;

    p->tbl = (struct TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR *)
           MEM_PAGE_TABLE1;

	for (n= 0; n < 4096; n++) {
		base_addr = MEGABYTE * n;

		ent = &p->tbl[n];

		// shared device
		ent->Value10	= 2;
		ent->BBit    = 1;
		ent->CBit    = 0;
		ent->XNBit   = 0;
		ent->Domain	= 0;
		ent->IMPBit	= 0;
		ent->AP	= AP_SYSTEM_ACCESS;
		ent->TEX     = 0;
		ent->APXBit	= APX_RW_ACCESS;
		ent->SBit    = 1;
		ent->NGBit	= 0;
		ent->Value0	= 0;
		ent->SBZ	= 0;
		ent->Base	= ARMV6MMUL1SECTIONBASE (base_addr);

		if (n >= SDRAM_SIZE_MBYTE)
		{
			ent->XNBit = 1;
		}
	}
	clean_data_cache();
}

void pgtbl_init(pgtbl_d *p, uint32_t sz)
{
    uint32_t n;
    uint32_t base_addr;
    extern uint8_t _etext;
    struct TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR *ent;

    p->allocated = 1;

    p->tbl = (struct TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR *)
        palloc ();

	for (n = 0; n < SDRAM_SIZE_MBYTE; n++) {
		base_addr = MEGABYTE * n;

		ent = &p->tbl[n];

		// outer and inner write back, no write allocate
		ent->Value10	= 2;
		ent->BBit	= 1;
		ent->CBit	= 1;
		ent->XNBit	= 0;
		ent->Domain	= 0;
		ent->IMPBit	= 0;
		ent->AP	= AP_SYSTEM_ACCESS;
		ent->TEX	= 0;
		ent->APXBit	= APX_RW_ACCESS;
#ifndef ARM_ALLOW_MULTI_CORE
		ent->SBit	= 0;
#else
		ent->SBit	= 1;		// required for spin locks, TODO: shared pool
#endif
		ent->NGBit	= 0;
		ent->Value0	= 0;
		ent->SBZ	= 0;
		ent->Base	= ARMV6MMUL1SECTIONBASE (base_addr);

		if (base_addr >= (uint32_t) &_etext) {
			ent->XNBit = 1;

			if (base_addr >= sz) {
				// shared device
				ent->BBit  = 1;
				ent->CBit  = 0;
				ent->TEX   = 0;
				ent->SBit  = 1;
			} else if (base_addr == MEM_COHERENT_REGION)
			{
				// strongly ordered
				ent->BBit  = 0;
				ent->CBit  = 0;
				ent->TEX   = 0;
				ent->SBit  = 1;
			}
		}
	}
	clean_data_cache();
}

void pgtbl_free(pgtbl_d *p)
{
	if (p->allocated) {
		pfree (p->tbl);
		p->tbl = 0;
        p->allocated = 0;
	}
}

uint32_t pgtbl_get_baseaddr(pgtbl_d *p)
{
	return (uint32_t) p->tbl | TTBR_MODE;
}

void pgtbl_new(pgtbl_d **p)
{
    pgtbl_d *pp;
    pp = calloc(1, sizeof (pgtbl_d));
    *p = pp;
}

void pgtbl_del(pgtbl_d **p)
{
    free(*p);
}
