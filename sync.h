#ifndef SYNC_H
#define SYNC_H
#ifdef __cplusplus
extern "C" {
#endif
void EnterCritical(unsigned nTargetLevel);
void LeaveCritical(void);
void SyncDataAndInstructionCache(void);
void CleanAndInvalidateDataCacheRange (uint32_t nAddress,
                                       uint32_t nLength);
#ifdef __cplusplus
}
#endif
#endif
