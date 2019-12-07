#ifndef ALLOC_H
#define ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

void mem_init (unsigned long ulBase, unsigned long ulSize);

unsigned long mem_get_size (void);

void *malloc (size_t nSize);	// resulting block is always 16 bytes aligned
void free (void *pBlock);

void *calloc (size_t nBlocks, size_t nSize);
void *realloc (void *pBlock, size_t nSize);

void *palloc (void);
void pfree (void *pPage);

#ifdef __cplusplus
}
#endif

#endif
