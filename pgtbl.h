#ifndef PGTBL_H
#define PGTBL_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pgtbl_d pgtbl_d;
void pgtbl_init_shared(pgtbl_d *p);
void pgtbl_init(pgtbl_d *p, uint32_t sz);
void pgtbl_free(pgtbl_d *p);
uint32_t pgtbl_get_baseaddr(pgtbl_d *p);
void pgtbl_new(pgtbl_d **p);
void pgtbl_del(pgtbl_d **p);
#ifdef __cplusplus
}
#endif
#endif
