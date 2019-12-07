#ifndef MEM_H
#define MEM_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct mem_d mem_d;
void mem_setup(mem_d *m);
uint32_t mem_size(mem_d *m);
uint32_t mem_ttbr0(mem_d *m);
uint32_t mem_context_id(mem_d *m);
void mem_enable_mmu(mem_d *m);
void mem_new(mem_d **m);
void mem_del(mem_d **m);
void mem_clean(mem_d *m);

void mem_init (unsigned long ulBase, unsigned long ulSize);

unsigned long mem_get_size (void);

void *malloc (size_t nSize);	// resulting block is always 16 bytes aligned
void free (void *pBlock);

void *calloc (size_t nBlocks, size_t nSize);
void *realloc (void *pBlock, size_t nSize);

void *palloc (void);
void pfree (void *pPage);

void *memset (void *pBuffer, int nValue, size_t nLength);
void *memcpy (void *pDest, const void *pSrc, size_t nLength);

#ifdef __cplusplus
}
#endif
#endif
