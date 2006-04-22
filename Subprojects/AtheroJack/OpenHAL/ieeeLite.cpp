/*
 *  ieeeLite.c
 *  AtheroJack
 *
 *  Created by Beat Zahnd on 4/9/06.
 *  Copyright 2006 Beat Zahnd. All rights reserved.
 *
 */

#include "ieeeLite.h"
#include "OpenHAL.h"

static const struct ieee80211_regdomainmap ieee80211_r_map[] = IEEE80211_REGDOMAIN_MAP;


#pragma mark -
#pragma mark OpenBSD sys/net80211/ieee80211_regdomain.c

/*
 * Code ported from OpenBSD sys/net80211/ieee80211_regdomain.c r1.6
 */

u_int32_t
OpenHAL::ieee80211_regdomain2flag(u_int16_t regdomain, u_int16_t mhz)
{
	int i;
	
	for(i = 0; i < (sizeof(ieee80211_r_map) / 
		sizeof(ieee80211_r_map[0])); i++) {
		if(ieee80211_r_map[i].rm_domain == regdomain) {
			if(mhz >= 2000 && mhz <= 3000)
				return((u_int32_t)ieee80211_r_map[i].rm_domain_2ghz);
			if(mhz >= IEEE80211_CHANNELS_5GHZ_MIN && 
			    mhz <= IEEE80211_CHANNELS_5GHZ_MAX)
				return((u_int32_t)ieee80211_r_map[i].rm_domain_5ghz);
		}
	}

	return((u_int32_t)DMN_DEBUG);
}

#pragma mark -
#pragma mark OpenBSD src/sys/net80211/ieee80211.c

/*
 * Code ported from OpenBSD src/sys/net80211/ieee80211.c r1.16
 */

/*
 * Convert MHz frequency to IEEE channel number.
 */
u_int
OpenHAL::ieee80211_mhz2ieee(u_int freq, u_int flags)
{
	if (flags & IEEE80211_CHAN_2GHZ) {	/* 2GHz band */
		if (freq == 2484)
			return 14;
		if (freq < 2484)
			return (freq - 2407) / 5;
		else
			return 15 + ((freq - 2512) / 20);
	} else if (flags & IEEE80211_CHAN_5GHZ) {	/* 5Ghz band */
		return (freq - 5000) / 5;
	} else {				/* either, guess */
		if (freq == 2484)
			return 14;
		if (freq < 2484)
			return (freq - 2407) / 5;
		if (freq < 5000)
			return 15 + ((freq - 2512) / 20);
		return (freq - 5000) / 5;
	}
}

/*
 * Convert IEEE channel number to MHz frequency.
 */
u_int
OpenHAL::ieee80211_ieee2mhz(u_int chan, u_int flags)
{
	if (flags & IEEE80211_CHAN_2GHZ) {	/* 2GHz band */
		if (chan == 14)
			return 2484;
		if (chan < 14)
			return 2407 + chan*5;
		else
			return 2512 + ((chan-15)*20);
	} else if (flags & IEEE80211_CHAN_5GHZ) {/* 5Ghz band */
		return 5000 + (chan*5);
	} else {				/* either, guess */
		if (chan == 14)
			return 2484;
		if (chan < 14)			/* 0-13 */
			return 2407 + chan*5;
		if (chan < 27)			/* 15-26 */
			return 2512 + ((chan-15)*20);
		return 5000 + (chan*5);
	}
}
