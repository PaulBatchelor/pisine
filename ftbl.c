#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "sp_base.h"
#include "ftbl.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	/* pi */
#endif

#define tpd360  0.0174532925199433

int sp_ftbl_init(sp_data *sp, sp_ftbl *ft, size_t size)
{
    ft->size = size;
    ft->sicvt = 1.0 * SP_FT_MAXLEN / sp->sr;
    ft->lobits = log2(SP_FT_MAXLEN / size);
    ft->lomask = (1<<ft->lobits) - 1;
    ft->lodiv = 1.0 / (1<<ft->lobits);
    ft->del = 1;
    return SP_OK;
}

int sp_ftbl_create(sp_data *sp, sp_ftbl **ft, size_t size)
{
    *ft = malloc(sizeof(sp_ftbl));
    sp_ftbl *ftp = *ft;
    ftp->tbl = malloc(sizeof(SPFLOAT) * (size + 1));
    memset(ftp->tbl, 0, sizeof(SPFLOAT) * (size + 1));
    sp_ftbl_init(sp, ftp, size);
    return SP_OK;
}

int sp_ftbl_bind(sp_data *sp, sp_ftbl **ft, SPFLOAT *tbl, size_t size)
{
    *ft = malloc(sizeof(sp_ftbl));
    sp_ftbl *ftp = *ft;
    ftp->tbl = tbl;
    sp_ftbl_init(sp, ftp, size);
    ftp->del = 0;
    return SP_OK;
}

int sp_ftbl_destroy(sp_ftbl **ft)
{
    sp_ftbl *ftp = *ft;
    if(ftp->del) free(ftp->tbl);
    free(*ft);
    return SP_OK;
}

int sp_gen_sine(sp_data *sp, sp_ftbl *ft)
{
    uint32_t i;
    SPFLOAT step = 2 * M_PI / ft->size;
    for(i = 0; i < ft->size; i++){
        ft->tbl[i] = sinf(i * step);
    }
    return SP_OK;
}
