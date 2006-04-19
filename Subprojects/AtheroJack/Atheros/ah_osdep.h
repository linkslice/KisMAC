/*-
 * Copyright (c) 2002, 2003 Sam Leffler, Errno Consulting, Atheros
 * Communications, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the following conditions are met:
 * 1. The materials contained herein are unmodified and are used
 *    unmodified.
 * 2. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following NO
 *    ''WARRANTY'' disclaimer below (''Disclaimer''), without
 *    modification.
 * 3. Redistributions in binary form must reproduce at minimum a
 *    disclaimer similar to the Disclaimer below and any redistribution
 *    must be conditioned upon including a substantially similar
 *    Disclaimer requirement for further binary redistribution.
 * 4. Neither the names of the above-listed copyright holders nor the
 *    names of any contributors may be used to endorse or promote
 *    product derived from this software without specific prior written
 *    permission.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT,
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES.
 *
 * $Id: ah_osdep.h,v 1.1.1.1 2004/01/02 11:26:11 kismac Exp $
 */
#ifndef _ATH_AH_OSDEP_H_
#define _ATH_AH_OSDEP_H_
/*
 * Atheros Hardware Access Layer (HAL) OS Dependent Definitions.
 */

#include <sys/types.h>

/*typedef SInt8 int8_t;
typedef SInt16 int16_t;
typedef SInt32 int32_t;
typedef SInt64 int64_t;

typedef UInt8 u_int8_t;
typedef UInt16 u_int16_t;
typedef UInt32 u_int32_t;
typedef UInt64 u_int64_t;*/

/*
 * Linux/BSD gcc compatibility shims.
 */
#define	__printflike(_a,_b) \
	__attribute__ ((__format__ (__printf__, _a, _b)))
#define	__va_list	va_list 
#define	OS_INLINE	__inline

typedef void* HAL_SOFTC;
typedef int HAL_BUS_TAG;
typedef void* HAL_BUS_HANDLE;
typedef u_int32_t HAL_BUS_ADDR;			/* XXX architecture dependent */

/*
 * Delay n microseconds.
 */
extern	void ath_hal_delay(int);
#define	OS_DELAY(_n)	ath_hal_delay(_n)

/*
 * Memory zero, copy (non-overlapped), compare, and sort.
 */
#define	OS_MEMZERO(_a, _size)		memset((_a), 0, (_size))
#define	OS_MEMCPY(_dst, _src, _size)	memcpy((_dst), (_src), (_size))
#define	OS_MACEQU(_a, _b) \
	(memcmp((_a), (_b), IEEE80211_ADDR_LEN) == 0)

struct ath_hal;
extern	u_int32_t ath_hal_getuptime(struct ath_hal *);
#define	OS_GETUPTIME(_ah)	ath_hal_getuptime(_ah)

/*
 * Register read/write; we assume the registers will always
 * be memory-mapped.  Note that register accesses are done
 * using target-specific functions when debugging is enabled
 * (AH_DEBUG) or we are explicitly configured this way.  The
 * latter is used on some platforms where the full i/o space
 * cannot be directly mapped.
 */
#if defined(AH_DEBUG) || defined(AH_REGOPS_FUNC)
#define	OS_REG_WRITE(_ah, _reg, _val)	ath_hal_reg_write(_ah, _reg, _val)
#define	OS_REG_READ(_ah, _reg)		ath_hal_reg_read(_ah, _reg)

extern	void ath_hal_reg_write(struct ath_hal *ah, u_int reg, u_int32_t val);
extern	u_int32_t ath_hal_reg_read(struct ath_hal *ah, u_int reg);
#else

/*
 * The hardware registers are native little-endian byte order.
 * Big-endian hosts are handled by enabling hardware byte-swap
 * of register reads and writes at reset.  But the PCI clock
 * domain registers are not byte swapped!  Thus, on big-endian
 * platforms we have to byte-swap thoese registers specifically.
 * Most of this code is collapsed at compile time because the
 * register values are constants.
 */
#define	AH_LITTLE_ENDIAN	1234
#define	AH_BIG_ENDIAN		4321

#if AH_BYTE_ORDER == AH_BIG_ENDIAN
/*
 * This could be optimized but since we only use it for
 * a few registers there's little reason to do so.
 */
static inline u_int32_t
__bswap32(u_int32_t _x)
{
 	return ((u_int32_t)(
	      (((const u_int8_t *)(&_x))[0]    ) |
	      (((const u_int8_t *)(&_x))[1]<< 8) |
	      (((const u_int8_t *)(&_x))[2]<<16) |
	      (((const u_int8_t *)(&_x))[3]<<24))
	);
}
#define OS_REG_WRITE(_ah, _reg, _val) do {				    \
	if ( (_reg) >= 0x4000 && (_reg) < 0x5000)			    \
		*((volatile u_int32_t *)((_ah)->ah_sh + (_reg))) =	    \
			__bswap32((_val));				    \
	else								    \
		*((volatile u_int32_t *)((_ah)->ah_sh + (_reg))) = (_val);  \
} while (0)
#define OS_REG_READ(_ah, _reg) \
	(((_reg) >= 0x4000 && (_reg) < 0x5000) ? \
		__bswap32(*((volatile u_int32_t *)((_ah)->ah_sh + (_reg)))) : \
		*((volatile u_int32_t *)((_ah)->ah_sh + (_reg))))
#else /* AH_LITTLE_ENDIAN */
#define OS_REG_WRITE(_ah, _reg, _val) do { \
	*((volatile u_int32_t *)((_ah)->ah_sh + (_reg))) = (_val); \
} while (0)
#define OS_REG_READ(_ah, _reg) \
	*((volatile u_int32_t *)((_ah)->ah_sh + (_reg)))
#endif /* AH_BYTE_ORDER */
#endif /* AH_DEBUG || AH_REGFUNC */
#define	OS_MARK(_ah, _id, _v)

/*
 * Linux-specific attach/detach methods needed for module reference counting.
 *
 * XXX We can't use HAL_STATUS because the type isn't defined at this
 *     point (circular dependency); we wack the type and patch things
 *     up in the function.
 */
extern	struct ath_hal *_ath_hal_attach(u_int16_t devid, HAL_SOFTC,
		HAL_BUS_TAG, HAL_BUS_HANDLE, void* status);
extern	void ath_hal_detach(struct ath_hal *);

#endif /* _ATH_AH_OSDEP_H_ */
