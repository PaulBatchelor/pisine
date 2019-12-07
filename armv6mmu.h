#ifndef ARMV6MMU_H
#define ARMV6MMU_H
#define AP_SYSTEM_ACCESS	1
#define APX_RW_ACCESS		0
/* #define APX_RO_ACCESS		1 */
#define DOMAIN_CLIENT		1

struct TARMV6MMU_LEVEL1_SECTION_DESCRIPTOR		// subpages disabled
{
	uint32_t	Value10		: 2,		// set to 2
		BBit		: 1,		// bufferable bit
		CBit		: 1,		// cacheable bit
		XNBit		: 1,		// execute never bit
		Domain		: 4,		// 0..15
		IMPBit		: 1,		// implementation defined bit
		AP		: 2,		// access permissions
		TEX		: 3,		// memory type extension field
		APXBit		: 1,		// access permissions extended bit
		SBit		: 1,		// shareable bit
		NGBit		: 1,		// non-global bit
		Value0		: 1,		// set to 0
		SBZ		: 1,		// should be 0
		Base		: 12;		// base address [31:20]
};

#define ARMV6MMUL1SECTIONBASE(addr)	(((addr) >> 20) & 0xFFF)

#define ARM_CONTROL_MMU (1 << 0)
#define ARM_CONTROL_STRICT_ALIGNMENT (1 << 1)
#define ARM_CONTROL_L1_CACHE (1 << 2)
#define ARM_CONTROL_BRANCH_PREDICTION (1 << 11)
#define ARM_CONTROL_L1_INSTRUCTION_CACHE (1 << 12)

#if RASPPI == 1
#define ARM_CONTROL_UNALIGNED_PERMITTED	(1 << 22)
#define ARM_CONTROL_EXTENDED_PAGE_TABLE	(1 << 23)
#endif

#if RASPPI == 1
#define ARM_TTBR_INNER_CACHEABLE (1 << 0)
#else
#define ARM_TTBR_INNER_NON_CACHEABLE ((0 << 6) | (0 << 0))
#define ARM_TTBR_INNER_WRITE_ALLOCATE ((1 << 6) | (0 << 0))
#define ARM_TTBR_INNER_WRITE_THROUGH ((0 << 6) | (1 << 0))
#define ARM_TTBR_INNER_WRITE_BACK ((1 << 6) | (1 << 0))
#endif

#define ARM_TTBR_OUTER_NON_CACHEABLE (0 << 3)

#if RASPPI == 1
#define ARM_AUX_CONTROL_CACHE_SIZE	(1 << 6) /* restrict cache size to 16K (no page coloring) */
#else
#define ARM_AUX_CONTROL_SMP (1 << 6)
#endif

#endif
