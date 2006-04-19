extern "C" {
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#include <string.h>
#include <libkern/OSByteOrder.h>
#include "ah_osdep.h"
#include "ah.h"
}

#include <IOKit/IOLib.h>

extern "C" {
    
int	ath_hal_dma_beacon_response_time = 2;	/* in TU's */
int	ath_hal_sw_beacon_response_time = 10;	/* in TU's */
int	ath_hal_additional_swba_backoff = 0;	/* in TU's */


void ath_hal_free(void* p)
{
    if (!p)
        return;
    int *sz = (int*)(((char*)p)-8);
    IOFree((void*)sz, *sz);
}

void* ath_hal_malloc(size_t size)
{
    int *header = (int*)IOMalloc(size + 8);
    *header = size;
    return (void*)(((char*)header)+8);
}

void ath_hal_printf(struct ath_hal *ah, const char* fmt, ...)
{
    static char buff[2048];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    IOLog(buff);
    va_end(ap);
}

void ath_hal_reg_write(struct ath_hal *ah, u_int reg, u_int32_t val)
{
#if AH_BYTE_ORDER == AH_BIG_ENDIAN
    if ( reg < 0x4000 || reg >= 0x5000 )
        OSWriteBigInt32(ah->ah_sh, reg, val);
#endif
    OSWriteLittleInt32(ah->ah_sh, reg, val);
}

u_int32_t ath_hal_reg_read(struct ath_hal *ah, u_int reg)
{
#if AH_BYTE_ORDER == AH_BIG_ENDIAN
    if ( reg < 0x4000 || reg >= 0x5000 )
        return OSReadBigInt32(ah->ah_sh, reg);
#endif
    return OSReadLittleInt32(ah->ah_sh, reg);
}

void ath_hal_delay(int microsec)
{
    IODelay(microsec);
}

u_int32_t ath_hal_getuptime(struct ath_hal *)
{
    struct timeval tv;
    microuptime(&tv);
    return tv.tv_sec * 1000 + (tv.tv_usec / 100);
}
}