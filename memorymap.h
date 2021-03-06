#ifndef MEMORYMAP_H
#define MEMORYMAP_H

#ifndef MEGABYTE
#define MEGABYTE 0x100000
#endif

#define MEM_SIZE (256 * MEGABYTE)
#define GPU_MEM_SIZE (64 * MEGABYTE)
#define ARM_MEM_SIZE (MEM_SIZE - GPU_MEM_SIZE)

#define PAGE_SIZE 4096

#define KERNEL_STACK_SIZE 0x20000
#define EXCEPTION_STACK_SIZE 0x8000
#define PAGE_TABLE1_SIZE 0x4000
#define PAGE_RESERVE (4 * MEGABYTE)

#define MEGABYTE 0x100000	// do not change
#define KERNEL_MAX_SIZE (2 * MEGABYTE)
#define MEM_KERNEL_START 0x8000
#define MEM_KERNEL_END (MEM_KERNEL_START + KERNEL_MAX_SIZE)
#define MEM_KERNEL_STACK (MEM_KERNEL_END + KERNEL_STACK_SIZE)
#if RASPPI == 1
#define MEM_ABORT_STACK (MEM_KERNEL_STACK + EXCEPTION_STACK_SIZE)
#define MEM_IRQ_STACK (MEM_ABORT_STACK + EXCEPTION_STACK_SIZE)
#define MEM_FIQ_STACK (MEM_IRQ_STACK + EXCEPTION_STACK_SIZE)
#define MEM_PAGE_TABLE1 MEM_FIQ_STACK
#else
#define CORES 4
#define MEM_ABORT_STACK	(MEM_KERNEL_STACK + KERNEL_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_IRQ_STACK (MEM_ABORT_STACK + EXCEPTION_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_FIQ_STACK (MEM_IRQ_STACK + EXCEPTION_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_PAGE_TABLE1 (MEM_FIQ_STACK + EXCEPTION_STACK_SIZE * (CORES-1))
#endif
#define MEM_PAGE_TABLE1_END	(MEM_PAGE_TABLE1 + PAGE_TABLE1_SIZE)

#define MEM_COHERENT_REGION	((MEM_PAGE_TABLE1_END + 2*MEGABYTE) & ~(MEGABYTE-1))

#define MEM_HEAP_START (MEM_COHERENT_REGION + MEGABYTE)
#endif
