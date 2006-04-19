/*
 *  OpenHAL.cpp
 *  AtheroJack
 *
 *  Created by mick on 30.03.2005.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#include "OpenHAL.h"
#include "../WiFiLogger.h"

OSDefineMetaClassAndStructors(OpenHAL, OSObject);

bool OpenHAL::init() {
	ah_rf_banks = NULL;
	return true;
}

/*
 * Supported channels
 */
static const struct
ieee80211_regchannel ar5k_5ghz_channels[] = IEEE80211_CHANNELS_5GHZ;
static const struct
ieee80211_regchannel ar5k_2ghz_channels[] = IEEE80211_CHANNELS_2GHZ;

/*
 * Initial gain optimization values
 */
static const struct ar5k_gain_opt ar5111_gain_opt = AR5K_AR5111_GAIN_OPT;
static const struct ar5k_gain_opt ar5112_gain_opt = AR5K_AR5112_GAIN_OPT;

/*
 * Initial register for the radio chipsets
 */
static const struct ar5k_ini_rf ar5111_rf[] = AR5K_AR5111_INI_RF;
static const struct ar5k_ini_rf ar5112_rf[] = AR5K_AR5112_INI_RF;
static const struct ar5k_ini_rfgain ar5k_rfg[] = AR5K_INI_RFGAIN;

static const struct ieee80211_regdomainmap ieee80211_r_map[] = IEEE80211_REGDOMAIN_MAP;

#pragma mark -

//implementation
bool OpenHAL::ath_hal_attach(u_int16_t device, void *sc, HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status)
{
	ieee80211_regdomain_t ieee_regdomain;
	HAL_RATE_TABLE rt_11a = AR5K_RATES_11A;
	HAL_RATE_TABLE rt_11b = AR5K_RATES_11B;
	HAL_RATE_TABLE rt_11g = AR5K_RATES_11G;
	HAL_RATE_TABLE rt_turbo = AR5K_RATES_TURBO;
	HAL_RATE_TABLE rt_xr = AR5K_RATES_XR;
	u_int16_t regdomain;
	u_int8_t mac[IEEE80211_ADDR_LEN];
	int i;

	*status = HAL_EINVAL;

	ah_sc = sc;
	ah_st = st;
	ah_sh = sh;
	ah_device = device;
	ah_sub_vendor = 0; /* XXX unknown?! */

	/*
	 * HAL information
	 */
	ah_abi = HAL_ABI_VERSION;
	ah_country_code = CTRY_DEFAULT;
	ah_capabilities.cap_regdomain.reg_current = AR5K_TUNE_REGDOMAIN;
	ah_op_mode = HAL_M_STA;
	ah_radar.r_enabled = AR5K_TUNE_RADAR_ALERT;
	ah_turbo = AH_FALSE;
	ah_txpower.txp_tpc = AR5K_TUNE_TPC_TXPOWER;
	ah_imr = 0;
	ah_atim_window = 0;
	ah_aifs = AR5K_TUNE_AIFS;
	ah_cw_min = AR5K_TUNE_CWMIN;
	ah_limit_tx_retries = AR5K_INIT_TX_RETRY;
	ah_software_retry = AH_FALSE;
	ah_ant_diversity = AR5K_TUNE_ANT_DIVERSITY;
	
	if (nic_ath_hal_attach(device, st, sh, status) == AH_FALSE)
		goto failed;

#ifdef AR5K_DEBUG
	ah_dumpState(hal);
#endif

	/*
	 * Get card capabilities, values, ...
	 */
	
	if ((i = ar5k_eeprom_init()) != 0) {
		AR5K_PRINTF("unable to init EEPROM: %u\n", i);
		goto failed;
	}

	/* Set regulation domain */
	if ((regdomain =
		ah_capabilities.cap_eeprom.ee_regdomain) != 0) {
		ah_capabilities.cap_regdomain.reg_current =
		    ieee_regdomain = ar5k_regdomain_to_ieee(regdomain);
	} else {
		ieee_regdomain =
		    ah_capabilities.cap_regdomain.reg_current;

		/* Try to write default regulation domain to EEPROM */
		ar5k_eeprom_regulation_domain(AH_TRUE, &ieee_regdomain);
	}

	ah_capabilities.cap_regdomain.reg_hw = ieee_regdomain;

	/* Get misc capabilities */
	if (nic_get_capabilities() != AH_TRUE) {
		AR5K_PRINTF("unable to get device capabilities: 0x%04x\n",
		    device);
		goto failed;
	}

	/* Get MAC address */
	if ((*status = ar5k_eeprom_read_mac(mac)) != HAL_OK) {
		AR5K_PRINTF("unable to read address from EEPROM: 0x%04x\n",
		    device);
		goto failed;
	}

	nic_setMacAddress(mac);

	/* Get rate tables */
	if (ah_capabilities.cap_mode & HAL_MODE_11A)
		ar5k_rt_copy(&ah_rt_11a, &rt_11a);
	if (ah_capabilities.cap_mode & HAL_MODE_11B)
		ar5k_rt_copy(&ah_rt_11b, &rt_11b);
	if (ah_capabilities.cap_mode & HAL_MODE_11G)
		ar5k_rt_copy(&ah_rt_11g, &rt_11g);
	if (ah_capabilities.cap_mode & HAL_MODE_TURBO)
		ar5k_rt_copy(&ah_rt_turbo, &rt_turbo);
	if (ah_capabilities.cap_mode & HAL_MODE_XR)
		ar5k_rt_copy(&ah_rt_xr, &rt_xr);

	/* Initialize the gain optimization values */
	if (ah_radio == AR5K_AR5111) {
		ah_gain.g_step_idx = ar5111_gain_opt.go_default;
		ah_gain.g_step =
		    &ar5111_gain_opt.go_step[ah_gain.g_step_idx];
		ah_gain.g_low = 20;
		ah_gain.g_high = 35;
		ah_gain.g_active = 1;
	} else if (ah_radio == AR5K_AR5112) {
		ah_gain.g_step_idx = ar5112_gain_opt.go_default;
		ah_gain.g_step =
		    &ar5111_gain_opt.go_step[ah_gain.g_step_idx];
		ah_gain.g_low = 20;
		ah_gain.g_high = 85;
		ah_gain.g_active = 1;
	}

	*status = HAL_OK;

	return true;

 failed:
	return false;
}

u_int16_t OpenHAL::ath_hal_computetxtime(const HAL_RATE_TABLE *rates, u_int32_t frame_length, u_int16_t rate_index, HAL_BOOL short_preamble) {
	const HAL_RATE *rate;
	u_int32_t value;

	AR5K_ASSERT_ENTRY(rate_index, rates->rateCount);

	/*
	 * Get rate by index
	 */
	rate = &rates->info[rate_index];

	/*
	 * Calculate the transmission time by operation (PHY) mode
	 */
	switch (rate->phy) {
	case IEEE80211_T_CCK:
		/*
		 * CCK / DS mode (802.11b)
		 */
		value = AR5K_CCK_TX_TIME(rate->rateKbps, frame_length,
		    (short_preamble && rate->shortPreamble));
		break;

	case IEEE80211_T_OFDM:
		/*
		 * Orthogonal Frequency Division Multiplexing
		 */
		if (AR5K_OFDM_NUM_BITS_PER_SYM(rate->rateKbps) == 0)
			return (0);
		value = AR5K_OFDM_TX_TIME(rate->rateKbps, frame_length);
		break;

	case IEEE80211_T_TURBO:
		/*
		 * Orthogonal Frequency Division Multiplexing
		 * Atheros "Turbo Mode" (doubled rates)
		 */
		if (AR5K_TURBO_NUM_BITS_PER_SYM(rate->rateKbps) == 0)
			return (0);
		value = AR5K_TURBO_TX_TIME(rate->rateKbps, frame_length);
		break;

	case IEEE80211_T_XR:
		/*
		 * Orthogonal Frequency Division Multiplexing
		 * Atheros "eXtended Range" (XR)
		 */
		if (AR5K_XR_NUM_BITS_PER_SYM(rate->rateKbps) == 0)
			return (0);
		value = AR5K_XR_TX_TIME(rate->rateKbps, frame_length);
		break;

	default:
		return (0);
	}

	return (value);
}

/*
 * Convert MHz frequency to IEEE channel number.
 */
u_int OpenHAL::ieee80211_mhz2ieee(u_int freq, u_int flags) {
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
u_int OpenHAL::ieee80211_ieee2mhz(u_int chan, u_int flags) {
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

HAL_BOOL OpenHAL::ar5k_check_channel(u_int16_t freq, u_int flags) {
	/* Check if the channel is in our supported range */
	if (flags & IEEE80211_CHAN_2GHZ) {
		if ((freq >= ah_capabilities.cap_range.range_2ghz_min) &&
		    (freq <= ah_capabilities.cap_range.range_2ghz_max))
			return (AH_TRUE);
	} else if (flags & IEEE80211_CHAN_5GHZ) {
		if ((freq >= ah_capabilities.cap_range.range_5ghz_min) &&
		    (freq <= ah_capabilities.cap_range.range_5ghz_max))
			return (AH_TRUE);
	}

	return (AH_FALSE);
}

HAL_BOOL OpenHAL::ath_hal_init_channels(HAL_CHANNEL *channels, u_int max_channels, u_int *channels_size, HAL_CTRY_CODE country, u_int16_t mode, HAL_BOOL outdoor, HAL_BOOL extended) {
	u_int i, c;
	u_int32_t domain_current;
	u_int domain_5ghz, domain_2ghz;
	HAL_CHANNEL all_channels[max_channels];

	i = c = 0;
	domain_current = ar5k_get_regdomain();

	/*
	 * In debugging mode, enable all channels supported by the chipset
	 */
	if (domain_current == DMN_DEFAULT) {
		int min, max, freq;
		u_int flags;

		min = ieee80211_mhz2ieee(IEEE80211_CHANNELS_2GHZ_MIN,
		    IEEE80211_CHAN_2GHZ);
		max = ieee80211_mhz2ieee(IEEE80211_CHANNELS_2GHZ_MAX,
		    IEEE80211_CHAN_2GHZ);
		flags = CHANNEL_B | CHANNEL_TG |
		    (ah_version == AR5K_AR5211 ?
		    CHANNEL_PUREG : CHANNEL_G);

 debugchan:
		for (i = min; i <= max && c < max_channels; i++) {
			freq = ieee80211_ieee2mhz(i, flags);
			if (ar5k_check_channel(freq, flags) == AH_FALSE)
				continue;
			all_channels[c].c_channel = freq;
			all_channels[c++].c_channel_flags = flags;
		}

		if (flags & IEEE80211_CHAN_2GHZ) {
			min = ieee80211_mhz2ieee(IEEE80211_CHANNELS_5GHZ_MIN,
			    IEEE80211_CHAN_5GHZ);
			max = ieee80211_mhz2ieee(IEEE80211_CHANNELS_5GHZ_MAX,
			    IEEE80211_CHAN_5GHZ);
			flags = CHANNEL_A | CHANNEL_T | CHANNEL_XR;
			goto debugchan;
		}

		goto done;
	}

	domain_5ghz = ieee80211_regdomain2flag(domain_current,
	    IEEE80211_CHANNELS_5GHZ_MIN);
	domain_2ghz = ieee80211_regdomain2flag(domain_current,
	    IEEE80211_CHANNELS_2GHZ_MIN);

	/*
	 * Create channel list based on chipset capabilities, regulation domain
	 * and mode. 5GHz...
	 */
	for (i = 0; (ah_capabilities.cap_range.range_5ghz_max > 0) &&
		 (i < AR5K_ELEMENTS(ar5k_5ghz_channels)) &&
		 (c < max_channels); i++) {
		/* Check if channel is supported by the chipset */
		if (ar5k_check_channel(ar5k_5ghz_channels[i].rc_channel,
		    IEEE80211_CHAN_5GHZ) == AH_FALSE)
			continue;

		/* Match regulation domain */
		if ((IEEE80211_DMN(ar5k_5ghz_channels[i].rc_domain) &
			IEEE80211_DMN(domain_5ghz)) == 0)
			continue;

		/* Match modes */
		if (ar5k_5ghz_channels[i].rc_mode & IEEE80211_CHAN_TURBO) {
			all_channels[c].c_channel_flags = CHANNEL_T;
		} else if (ar5k_5ghz_channels[i].rc_mode &
		    IEEE80211_CHAN_OFDM) {
			all_channels[c].c_channel_flags = CHANNEL_A;
		} else
			continue;

		/* Write channel and increment counter */
		all_channels[c++].channel = ar5k_5ghz_channels[i].rc_channel;
	}

	/*
	 * ...and 2GHz.
	 */
	for (i = 0; (ah_capabilities.cap_range.range_2ghz_max > 0) &&
		 (i < AR5K_ELEMENTS(ar5k_2ghz_channels)) &&
		 (c < max_channels); i++) {
		/* Check if channel is supported by the chipset */
		if (ar5k_check_channel(ar5k_2ghz_channels[i].rc_channel,
		    IEEE80211_CHAN_2GHZ) == AH_FALSE)
			continue;

		/* Match regulation domain */
		if ((IEEE80211_DMN(ar5k_2ghz_channels[i].rc_domain) &
			IEEE80211_DMN(domain_2ghz)) == 0)
			continue;

		/* Match modes */
		if (ar5k_2ghz_channels[i].rc_mode & IEEE80211_CHAN_CCK)
			all_channels[c].c_channel_flags = CHANNEL_B;

		if (ar5k_2ghz_channels[i].rc_mode & IEEE80211_CHAN_OFDM) {
			all_channels[c].c_channel_flags |=
			    ah_version == AR5K_AR5211 ?
			    CHANNEL_PUREG : CHANNEL_G;
			if (ar5k_2ghz_channels[i].rc_mode &
			    IEEE80211_CHAN_TURBO)
				all_channels[c].c_channel_flags |= CHANNEL_TG;
		}

		/* Write channel and increment counter */
		all_channels[c++].channel = ar5k_2ghz_channels[i].rc_channel;
	}

 done:
	bcopy(all_channels, channels, sizeof(all_channels));
	*channels_size = c;

	return (AH_TRUE);
}

#pragma mark -

u_int16_t OpenHAL::ar5k_regdomain_from_ieee(ieee80211_regdomain_t ieee) {
	u_int32_t regdomain = (u_int32_t)ieee;

	if (regdomain & 0xf0000000)
		return ((u_int16_t)AR5K_TUNE_REGDOMAIN);

	return (regdomain & 0xff);
}

ieee80211_regdomain_t OpenHAL::ar5k_regdomain_to_ieee(u_int16_t regdomain) {
	ieee80211_regdomain_t ieee = (ieee80211_regdomain_t)regdomain & 0xff;
	return (ieee);
}


u_int16_t OpenHAL::ar5k_get_regdomain() {
#ifndef COUNTRYCODE
	/*
	 * Use the regulation domain found in the EEPROM, if not
	 * forced by a static country code.
	 */
	u_int16_t regdomain;
	ieee80211_regdomain_t ieee_regdomain;

	if (ar5k_eeprom_regulation_domain(AH_FALSE, &ieee_regdomain) == AH_TRUE) {
		if ((regdomain = ar5k_regdomain_from_ieee(ieee_regdomain)))
			return (regdomain);
	}

	return (ah_regdomain);
#else
	/*
	 * Get the regulation domain by country code. This will ignore
	 * the settings found in the EEPROM.
	 */
	u_int16_t code;

	code = ieee80211_name2countrycode(COUNTRYCODE);
	return (ieee80211_countrycode2regdomain(code));
#endif
}

u_int32_t OpenHAL::ar5k_bitswap(u_int32_t val, u_int bits) {
	u_int32_t retval = 0, bit, i;

	for (i = 0; i < bits; i++) {
		bit = (val >> i) & 1;
		retval = (retval << 1) | bit;
	}

	return (retval);
}

void OpenHAL::ar5k_rt_copy(HAL_RATE_TABLE *dst, HAL_RATE_TABLE *src) {
	bzero(dst, sizeof(HAL_RATE_TABLE));
	dst->rateCount = src->rateCount;
	bcopy(src->info, dst->info, sizeof(dst->info));
}

HAL_BOOL OpenHAL::ar5k_register_timeout(u_int32_t reg, u_int32_t flag, u_int32_t val, HAL_BOOL is_set) {
	int i;
	u_int32_t data;

	for (i = AR5K_TUNE_REGISTER_TIMEOUT; i > 0; i--) {
		data = AR5K_REG_READ(reg);
		if ((is_set == AH_TRUE) && (data & flag))
			break;
		else if ((data & flag) == val)
			break;
		AR5K_DELAY(15);
	}

	if (i <= 0)
		return (AH_FALSE);

	return (AH_TRUE);
}

#pragma mark -

u_int16_t OpenHAL::ar5k_eeprom_bin2freq(u_int16_t bin, u_int mode) {
	u_int16_t val;

	if (bin == AR5K_EEPROM_CHANNEL_DIS)
		return (bin);

	if (mode == AR5K_EEPROM_MODE_11A) {
		if (ah_ee_version > AR5K_EEPROM_VERSION_3_2)
			val = (5 * bin) + 4800;
		else
			val = bin > 62 ?
			    (10 * 62) + (5 * (bin - 62)) + 5100 :
			    (bin * 10) + 5100;
	} else {
		if (ah_ee_version > AR5K_EEPROM_VERSION_3_2)
			val = bin + 2300;
		else
			val = bin + 2400;
	}

	return (val);
}

int OpenHAL::ar5k_eeprom_read_ants(u_int32_t *offset, u_int mode) {
	struct ar5k_eeprom_info *ee = &ah_capabilities.cap_eeprom;
	u_int32_t o = *offset;
	u_int16_t val;
	int ret, i = 0;

	AR5K_EEPROM_READ(o++, val);
	ee->ee_switch_settling[mode]	= (val >> 8) & 0x7f;
	ee->ee_ant_tx_rx[mode]		= (val >> 2) & 0x3f;
	ee->ee_ant_control[mode][i]	= (val << 4) & 0x3f;

	AR5K_EEPROM_READ(o++, val);
	ee->ee_ant_control[mode][i++]	|= (val >> 12) & 0xf;
	ee->ee_ant_control[mode][i++]	= (val >> 6) & 0x3f;
	ee->ee_ant_control[mode][i++]	= val & 0x3f;

	AR5K_EEPROM_READ(o++, val);
	ee->ee_ant_control[mode][i++]	= (val >> 10) & 0x3f;
	ee->ee_ant_control[mode][i++]	= (val >> 4) & 0x3f;
	ee->ee_ant_control[mode][i]	= (val << 2) & 0x3f;

	AR5K_EEPROM_READ(o++, val);
	ee->ee_ant_control[mode][i++]	|= (val >> 14) & 0x3;
	ee->ee_ant_control[mode][i++]	= (val >> 8) & 0x3f;
	ee->ee_ant_control[mode][i++]	= (val >> 2) & 0x3f;
	ee->ee_ant_control[mode][i]	= (val << 4) & 0x3f;

	AR5K_EEPROM_READ(o++, val);
	ee->ee_ant_control[mode][i++]	|= (val >> 12) & 0xf;
	ee->ee_ant_control[mode][i++]	= (val >> 6) & 0x3f;
	ee->ee_ant_control[mode][i++]	= val & 0x3f;

	/* Get antenna modes */
	ah_antenna[mode][0] =
	    (ee->ee_ant_control[mode][0] << 4) | 0x1;
	ah_antenna[mode][HAL_ANT_FIXED_A] =
	    ee->ee_ant_control[mode][1] |
	    (ee->ee_ant_control[mode][2] << 6) |
	    (ee->ee_ant_control[mode][3] << 12) |
	    (ee->ee_ant_control[mode][4] << 18) |
	    (ee->ee_ant_control[mode][5] << 24);
	ah_antenna[mode][HAL_ANT_FIXED_B] =
	    ee->ee_ant_control[mode][6] |
	    (ee->ee_ant_control[mode][7] << 6) |
	    (ee->ee_ant_control[mode][8] << 12) |
	    (ee->ee_ant_control[mode][9] << 18) |
	    (ee->ee_ant_control[mode][10] << 24);

	/* return new offset */
	*offset = o;

	return (0);
}

int OpenHAL::ar5k_eeprom_read_modes(u_int32_t *offset, u_int mode) {
	struct ar5k_eeprom_info *ee = &ah_capabilities.cap_eeprom;
	u_int32_t o = *offset;
	u_int16_t val;
	int ret;

	AR5K_EEPROM_READ(o++, val);
	ee->ee_tx_end2xlna_enable[mode]	= (val >> 8) & 0xff;
	ee->ee_thr_62[mode]		= val & 0xff;

	if (ah_ee_version <= AR5K_EEPROM_VERSION_3_2)
		ee->ee_thr_62[mode] =
		    mode == AR5K_EEPROM_MODE_11A ? 15 : 28;

	AR5K_EEPROM_READ(o++, val);
	ee->ee_tx_end2xpa_disable[mode]	= (val >> 8) & 0xff;
	ee->ee_tx_frm2xpa_enable[mode]	= val & 0xff;

	AR5K_EEPROM_READ(o++, val);
	ee->ee_pga_desired_size[mode]	= (val >> 8) & 0xff;

	if ((val & 0xff) & 0x80)
		ee->ee_noise_floor_thr[mode] = -((((val & 0xff) ^ 0xff)) + 1);
	else
		ee->ee_noise_floor_thr[mode] = val & 0xff;

	if (ah_ee_version <= AR5K_EEPROM_VERSION_3_2)
		ee->ee_noise_floor_thr[mode] =
		    mode == AR5K_EEPROM_MODE_11A ? -54 : -1;

	AR5K_EEPROM_READ(o++, val);
	ee->ee_xlna_gain[mode]		= (val >> 5) & 0xff;
	ee->ee_x_gain[mode]		= (val >> 1) & 0xf;
	ee->ee_xpd[mode]		= val & 0x1;

	if (ah_ee_version >= AR5K_EEPROM_VERSION_4_0)
		ee->ee_fixed_bias[mode] = (val >> 13) & 0x1;

	if (ah_ee_version >= AR5K_EEPROM_VERSION_3_3) {
		AR5K_EEPROM_READ(o++, val);
		ee->ee_false_detect[mode] = (val >> 6) & 0x7f;

		if (mode == AR5K_EEPROM_MODE_11A)
			ee->ee_xr_power[mode] = val & 0x3f;
		else {
			ee->ee_ob[mode][0] = val & 0x7;
			ee->ee_db[mode][0] = (val >> 3) & 0x7;
		}
	}

	if (ah_ee_version < AR5K_EEPROM_VERSION_3_4) {
		ee->ee_i_gain[mode] = AR5K_EEPROM_I_GAIN;
		ee->ee_cck_ofdm_power_delta = AR5K_EEPROM_CCK_OFDM_DELTA;
	} else {
		ee->ee_i_gain[mode] = (val >> 13) & 0x7;

		AR5K_EEPROM_READ(o++, val);
		ee->ee_i_gain[mode] |= (val << 3) & 0x38;

		if (mode == AR5K_EEPROM_MODE_11G)
			ee->ee_cck_ofdm_power_delta = (val >> 3) & 0xff;
	}

	if (ah_ee_version >= AR5K_EEPROM_VERSION_4_0 &&
	    mode == AR5K_EEPROM_MODE_11A) {
		ee->ee_i_cal[mode] = (val >> 8) & 0x3f;
		ee->ee_q_cal[mode] = (val >> 3) & 0x1f;
	}

	if (ah_ee_version >= AR5K_EEPROM_VERSION_4_6 &&
	    mode == AR5K_EEPROM_MODE_11G)
		ee->ee_scaled_cck_delta = (val >> 11) & 0x1f;

	/* return new offset */
	*offset = o;

	return (0);
}

int OpenHAL::ar5k_eeprom_init() {
	struct ar5k_eeprom_info *ee = &ah_capabilities.cap_eeprom;
	u_int32_t offset;
	u_int16_t val;
	int ret, i;
	u_int mode;

	/* Check if EEPROM is busy */
	if (nic_eeprom_is_busy() == AH_TRUE) {
		AR5K_PRINT("EEPROM locked!");
		return (HAL_EELOCKED);
	}
	
	/* Initial TX thermal adjustment values */
	ee->ee_tx_clip = 4;
	ee->ee_pwd_84 = ee->ee_pwd_90 = 1;
	ee->ee_gain_select = 1;

	/*
	 * Read values from EEPROM and store them in the capability structure
	 */
	AR5K_EEPROM_READ_HDR(AR5K_EEPROM_MAGIC, ee_magic);
	AR5K_EEPROM_READ_HDR(AR5K_EEPROM_PROTECT, ee_protect);
	AR5K_EEPROM_READ_HDR(AR5K_EEPROM_REG_DOMAIN, ee_regdomain);
	AR5K_EEPROM_READ_HDR(AR5K_EEPROM_VERSION, ee_version);
	AR5K_EEPROM_READ_HDR(AR5K_EEPROM_HDR, ee_header);

	/* Return if we have an old EEPROM */
	if (ah_ee_version < AR5K_EEPROM_VERSION_3_0)
		return (0);
		
	WLLogDebug("EEPROM version 0x%x", ah_ee_version);
	ar5k_check_eeprom();
	
	AR5K_EEPROM_READ_HDR(AR5K_EEPROM_ANT_GAIN(ah_ee_version),
	    ee_ant_gain);

	if (ah_ee_version >= AR5K_EEPROM_VERSION_4_0) {
		AR5K_EEPROM_READ_HDR(AR5K_EEPROM_MISC0, ee_misc0);
		AR5K_EEPROM_READ_HDR(AR5K_EEPROM_MISC1, ee_misc1);
	}

	if (ah_ee_version < AR5K_EEPROM_VERSION_3_3) {
		AR5K_EEPROM_READ(AR5K_EEPROM_OBDB0_2GHZ, val);
		ee->ee_ob[AR5K_EEPROM_MODE_11B][0] = val & 0x7;
		ee->ee_db[AR5K_EEPROM_MODE_11B][0] = (val >> 3) & 0x7;

		AR5K_EEPROM_READ(AR5K_EEPROM_OBDB1_2GHZ, val);
		ee->ee_ob[AR5K_EEPROM_MODE_11G][0] = val & 0x7;
		ee->ee_db[AR5K_EEPROM_MODE_11G][0] = (val >> 3) & 0x7;
	}

	/*
	 * Get conformance test limit values
	 */
	offset = AR5K_EEPROM_CTL(ah_ee_version);
	ee->ee_ctls = AR5K_EEPROM_N_CTLS(ah_ee_version);

	for (i = 0; i < ee->ee_ctls; i++) {
		AR5K_EEPROM_READ(offset++, val);
		ee->ee_ctl[i] = (val >> 8) & 0xff;
		ee->ee_ctl[i + 1] = val & 0xff;
	}

	/*
	 * Get values for 802.11a (5GHz)
	 */
	mode = AR5K_EEPROM_MODE_11A;

	ee->ee_turbo_max_power[mode] =
	    AR5K_EEPROM_HDR_T_5GHZ_DBM(ee->ee_header);

	offset = AR5K_EEPROM_MODES_11A(ah_ee_version);

	if ((ret = ar5k_eeprom_read_ants(&offset, mode)) != 0) {
		AR5K_PRINT("Error reading antenna config 11a");
		return (ret);
	}
	
	AR5K_EEPROM_READ(offset++, val);
	ee->ee_adc_desired_size[mode]	= (int8_t)((val >> 8) & 0xff);
	ee->ee_ob[mode][3]		= (val >> 5) & 0x7;
	ee->ee_db[mode][3]		= (val >> 2) & 0x7;
	ee->ee_ob[mode][2]		= (val << 1) & 0x7;

	AR5K_EEPROM_READ(offset++, val);
	ee->ee_ob[mode][2]		|= (val >> 15) & 0x1;
	ee->ee_db[mode][2]		= (val >> 12) & 0x7;
	ee->ee_ob[mode][1]		= (val >> 9) & 0x7;
	ee->ee_db[mode][1]		= (val >> 6) & 0x7;
	ee->ee_ob[mode][0]		= (val >> 3) & 0x7;
	ee->ee_db[mode][0]		= val & 0x7;

	if ((ret = ar5k_eeprom_read_modes(&offset, mode)) != 0) {
		AR5K_PRINT("Error reading modes 11a");
		return (ret);
	}
	
	if (ah_ee_version >= AR5K_EEPROM_VERSION_4_1) {
		AR5K_EEPROM_READ(offset++, val);
		ee->ee_margin_tx_rx[mode] = val & 0x3f;
	}

	/*
	 * Get values for 802.11b (2.4GHz)
	 */
	mode = AR5K_EEPROM_MODE_11B;
	offset = AR5K_EEPROM_MODES_11B(ah_ee_version);

	if ((ret = ar5k_eeprom_read_ants(&offset, mode)) != 0) {
		AR5K_PRINT("Error reading antenna config 11b");
		return (ret);
	}
	
	AR5K_EEPROM_READ(offset++, val);
	ee->ee_adc_desired_size[mode]	= (int8_t)((val >> 8) & 0xff);
	ee->ee_ob[mode][1]		= (val >> 4) & 0x7;
	ee->ee_db[mode][1]		= val & 0x7;

	if ((ret = ar5k_eeprom_read_modes(&offset, mode)) != 0)
		return (ret);

	if (ah_ee_version >= AR5K_EEPROM_VERSION_4_0) {
		AR5K_EEPROM_READ(offset++, val);
		ee->ee_cal_pier[mode][0] =
		    ar5k_eeprom_bin2freq(val & 0xff, mode);
		ee->ee_cal_pier[mode][1] =
		    ar5k_eeprom_bin2freq((val >> 8) & 0xff, mode);

		AR5K_EEPROM_READ(offset++, val);
		ee->ee_cal_pier[mode][2] =
		    ar5k_eeprom_bin2freq(val & 0xff, mode);
	}

	if (ah_ee_version >= AR5K_EEPROM_VERSION_4_1) {
		ee->ee_margin_tx_rx[mode] = (val >> 8) & 0x3f;
	}

	/*
	 * Get values for 802.11g (2.4GHz)
	 */
	mode = AR5K_EEPROM_MODE_11G;
	offset = AR5K_EEPROM_MODES_11G(ah_ee_version);

	if ((ret = ar5k_eeprom_read_ants(&offset, mode)) != 0)
		return (ret);

	AR5K_EEPROM_READ(offset++, val);
	ee->ee_adc_desired_size[mode]	= (int8_t)((val >> 8) & 0xff);
	ee->ee_ob[mode][1]		= (val >> 4) & 0x7;
	ee->ee_db[mode][1]		= val & 0x7;

	if ((ret = ar5k_eeprom_read_modes(&offset, mode)) != 0)
		return (ret);

	if (ah_ee_version >= AR5K_EEPROM_VERSION_4_0) {
		AR5K_EEPROM_READ(offset++, val);
		ee->ee_cal_pier[mode][0] =
		    ar5k_eeprom_bin2freq(val & 0xff, mode);
		ee->ee_cal_pier[mode][1] =
		    ar5k_eeprom_bin2freq((val >> 8) & 0xff, mode);

		AR5K_EEPROM_READ(offset++, val);
		ee->ee_turbo_max_power[mode] = val & 0x7f;
		ee->ee_xr_power[mode] = (val >> 7) & 0x3f;

		AR5K_EEPROM_READ(offset++, val);
		ee->ee_cal_pier[mode][2] =
		    ar5k_eeprom_bin2freq(val & 0xff, mode);

		if (ah_ee_version >= AR5K_EEPROM_VERSION_4_1) {
			ee->ee_margin_tx_rx[mode] = (val >> 8) & 0x3f;
		}

		AR5K_EEPROM_READ(offset++, val);
		ee->ee_i_cal[mode] = (val >> 8) & 0x3f;
		ee->ee_q_cal[mode] = (val >> 3) & 0x1f;

		if (ah_ee_version >= AR5K_EEPROM_VERSION_4_2) {
			AR5K_EEPROM_READ(offset++, val);
			ee->ee_cck_ofdm_gain_delta = val & 0xff;
		}
	}

	/*
	 * Read 5GHz EEPROM channels
	 */

	return (0);
}

HAL_STATUS OpenHAL::ar5k_eeprom_read_mac(u_int8_t *mac) {
	u_int32_t total, offset;
	u_int16_t data;
	int octet;
	u_int8_t mac_d[IEEE80211_ADDR_LEN];

	bzero(mac, IEEE80211_ADDR_LEN);
	bzero(&mac_d, IEEE80211_ADDR_LEN);

	if (nic_eeprom_is_busy())
		return (HAL_EELOCKED);

	if (nic_eeprom_read(0x20, &data) != 0)
		return (HAL_EIO);

	for (offset = 0x1f, octet = 0, total = 0;
	     offset >= 0x1d; offset--) {
		if (nic_eeprom_read(offset, &data) != 0)
			return (HAL_EIO);

		total += data;
		mac_d[octet + 1] = data & 0xff;
		mac_d[octet] = data >> 8;
		octet += 2;
	}

	bcopy(mac_d, mac, IEEE80211_ADDR_LEN);

	if ((!total) || total == (3 * 0xffff))
		return (HAL_EINVAL);

	return (HAL_OK);
}

HAL_BOOL OpenHAL::ar5k_eeprom_regulation_domain(HAL_BOOL write, ieee80211_regdomain_t *regdomain)
{
	/* Read current value */
	if (write != AH_TRUE) {
		*regdomain = ah_capabilities.cap_regdomain.reg_current;
		return (AH_TRUE);
	}

	/* Try to write a new value */
	ah_capabilities.cap_regdomain.reg_current = *regdomain;

	if (ah_capabilities.cap_eeprom.ee_protect &
	    AR5K_EEPROM_PROTECT_WR_128_191)
		return (AH_FALSE);

	if (nic_eeprom_write(AR5K_EEPROM_REG_DOMAIN,
	    ah_capabilities.cap_eeprom.ee_regdomain) != 0)
		return (AH_FALSE);

	ah_capabilities.cap_eeprom.ee_regdomain =
	    ar5k_regdomain_from_ieee(*regdomain);

	return (AH_TRUE);
}

int OpenHAL::ar5k_check_eeprom() {
	u_int32_t addr;
	u_int16_t data[0x400], chksum = 0;
	int ret;
	
    for (addr = 0; addr < AR5K_EEPROM_INFO_BASE + AR5K_EEPROM_INFO_MAX; addr++) {
        AR5K_EEPROM_READ(addr, data[addr]);
        if (addr >= AR5K_EEPROM_INFO_BASE) chksum ^= data[addr];
    }

    /*for (addr = 0; addr < AR5K_EEPROM_INFO_BASE + AR5K_EEPROM_INFO_MAX; addr+=16) {
        IOLog("0x%04x %04x%04x%04x%04x %04x%04x%04x%04x %04x%04x%04x%04x %04x%04x%04x%04x\n", addr, data[addr], data[addr + 1], data[addr + 2], data[addr + 3], data[addr + 4], data[addr + 5], data[addr + 6], data[addr + 7], data[addr + 8], data[addr + 9], data[addr + 10], data[addr + 11], data[addr + 12], data[addr + 13], data[addr + 14], data[addr + 15]);
		IOSleep(100);
    }*/


    if (chksum != 0xffff) {
        AR5K_PRINTF("Checksum 0x%04x != 0xffff\n", chksum);
        return HAL_EEBADSUM;
    }

    return HAL_OK;
}

#pragma mark -

HAL_BOOL OpenHAL::ar5k_channel(HAL_CHANNEL *channel) {
	HAL_BOOL ret;

	/*
	 * Check bounds supported by the PHY
	 * (don't care about regulation restrictions at this point)
	 */
	if ((channel->channel < ah_capabilities.cap_range.range_2ghz_min ||
	    channel->channel > ah_capabilities.cap_range.range_2ghz_max) &&
	    (channel->channel < ah_capabilities.cap_range.range_5ghz_min ||
	    channel->channel > ah_capabilities.cap_range.range_5ghz_max)) {
		AR5K_PRINTF("channel out of supported range (%u MHz)\n",
		    channel->channel);
		return (AH_FALSE);
	}

	/*
	 * Set the channel and wait
	 */
	if (nic_channel(channel) == AH_FALSE) {
		AR5K_PRINT("Could not set channel!!!");
		return (AH_FALSE);
	}
	
	ah_current_channel.c_channel = channel->c_channel;
	ah_current_channel.c_channel_flags = channel->c_channel_flags;
	ah_turbo = channel->c_channel_flags == CHANNEL_T ?
	    AH_TRUE : AH_FALSE;

	return (AH_TRUE);
}

u_int OpenHAL::ar5k_rfregs_op(u_int32_t *rf, u_int32_t offset, u_int32_t reg, u_int32_t bits, u_int32_t first, u_int32_t col, HAL_BOOL set) {
	u_int32_t mask, entry, last, data, shift, position;
	int32_t left;
	int i;

	if (rf == NULL) {
		/* should not happen */
		return (0);
	}

	if (!(col <= 3 && bits <= 32 && first + bits <= 319)) {
		AR5K_PRINTF("invalid values at offset %u\n", offset);
		return (0);
	}

	entry = ((first - 1) / 8) + offset;
	position = (first - 1) % 8;

	if (set == AH_TRUE)
		data = ar5k_bitswap(reg, bits);

	for (i = shift = 0, left = bits; left > 0; position = 0, entry++, i++) {
		last = (position + left > 8) ? 8 : position + left;
		mask = (((1 << last) - 1) ^ ((1 << position) - 1)) <<
		    (col * 8);

		if (set == AH_TRUE) {
			rf[entry] &= ~mask;
			rf[entry] |= ((data << position) << (col * 8)) & mask;
			data >>= (8 - position);
		} else {
			data = (((rf[entry] & mask) >> (col * 8)) >>
			    position) << shift;
			shift += last - position;
		}

		left -= 8 - position;
	}

	data = set == AH_TRUE ? 1 : ar5k_bitswap(data, bits);

	return (data);
}

u_int32_t OpenHAL::ar5k_rfregs_gainf_corr() {
	u_int32_t mix, step;
	u_int32_t *rf;

	if (ah_rf_banks == NULL)
		return (0);

	rf = ah_rf_banks;
	ah_gain.g_f_corr = 0;

	if (ar5k_rfregs_op(rf, ah_offset[7], 0, 1, 36, 0, AH_FALSE) != 1)
		return (0);

	step = ar5k_rfregs_op(rf, ah_offset[7], 0, 4, 32, 0, AH_FALSE);
	mix = ah_gain.g_step->gos_param[0];

	switch (mix) {
	case 3:
		ah_gain.g_f_corr = step * 2;
		break;
	case 2:
		ah_gain.g_f_corr = (step - 5) * 2;
		break;
	case 1:
		ah_gain.g_f_corr = step;
		break;
	default:
		ah_gain.g_f_corr = 0;
		break;
	}

	return (ah_gain.g_f_corr);
}

HAL_BOOL OpenHAL::ar5k_rfregs_gain_readback() {
	u_int32_t step, mix, level[4];
	u_int32_t *rf;

	if (ah_rf_banks == NULL)
		return (AH_FALSE);

	rf = ah_rf_banks;

	if (ah_radio == AR5K_AR5111) {
		step = ar5k_rfregs_op(rf, ah_offset[7],
		    0, 6, 37, 0, AH_FALSE);
		level[0] = 0;
		level[1] = (step == 0x3f) ? 0x32 : step + 4;
		level[2] = (step != 0x3f) ? 0x40 : level[0];
		level[3] = level[2] + 0x32;

		ah_gain.g_high = level[3] -
		    (step == 0x3f ? AR5K_GAIN_DYN_ADJUST_HI_MARGIN : -5);
		ah_gain.g_low = level[0] +
		    (step == 0x3f ? AR5K_GAIN_DYN_ADJUST_LO_MARGIN : 0);
	} else {
		mix = ar5k_rfregs_op(rf, ah_offset[7],
		    0, 1, 36, 0, AH_FALSE);
		level[0] = level[2] = 0;

		if (mix == 1) {
			level[1] = level[3] = 83;
		} else {
			level[1] = level[3] = 107;
			ah_gain.g_high = 55;
		}
	}

	return ((ah_gain.g_current >= level[0] &&
	    ah_gain.g_current <= level[1]) ||
	    (ah_gain.g_current >= level[2] &&
	    ah_gain.g_current <= level[3])) ? AH_TRUE : AH_FALSE;
}

int32_t OpenHAL::ar5k_rfregs_gain_adjust() {
	int ret = 0;
	const struct ar5k_gain_opt *go;

	go = ah_radio == AR5K_AR5111 ?
	    &ar5111_gain_opt : &ar5112_gain_opt;

	ah_gain.g_step = &go->go_step[ah_gain.g_step_idx];

	if (ah_gain.g_current >= ah_gain.g_high) {
		if (ah_gain.g_step_idx == 0)
			return (-1);
		for (ah_gain.g_target = ah_gain.g_current;
		    ah_gain.g_target >=  ah_gain.g_high &&
		    ah_gain.g_step_idx > 0;
		    ah_gain.g_step =
		    &go->go_step[ah_gain.g_step_idx]) {
			ah_gain.g_target -= 2 *
			    (go->go_step[--(ah_gain.g_step_idx)].gos_gain -
			    ah_gain.g_step->gos_gain);
		}

		ret = 1;
		goto done;
	}

	if (ah_gain.g_current <= ah_gain.g_low) {
		if (ah_gain.g_step_idx == (go->go_steps_count - 1))
			return (-2);
		for (ah_gain.g_target = ah_gain.g_current;
		    ah_gain.g_target <=  ah_gain.g_low &&
		    ah_gain.g_step_idx < (go->go_steps_count - 1);
		    ah_gain.g_step =
		    &go->go_step[ah_gain.g_step_idx]) {
			ah_gain.g_target -= 2 *
			    (go->go_step[++(ah_gain.g_step_idx)].gos_gain -
			    ah_gain.g_step->gos_gain);
		}

		ret = 2;
		goto done;
	}

 done:
#ifdef AR5K_DEBUG
	AR5K_PRINTF("ret %d, gain step %u, current gain %u, target gain %u\n",
	    g,
	    ah_gain.g_step_idx,
	    ah_gain.g_current,
	    ah_gain.g_target);
#endif

	return (ret);
}

HAL_BOOL OpenHAL::ar5k_rfregs(HAL_CHANNEL *channel, u_int mode) {
	HAL_BOOL ret;

	if (ah_radio == AR5K_AR5111) {
		ah_rf_banks_size = sizeof(ar5111_rf);
	} else if (ah_radio == AR5K_AR5112) {		
		ah_rf_banks_size = sizeof(ar5112_rf);
	} else
		return (AH_FALSE);

	if (ah_rf_banks == NULL) {
		/* XXX do extra checks? */
		if ((ah_rf_banks = (u_int32_t*)IOMalloc(ah_rf_banks_size)) == NULL) {
			AR5K_PRINT("out of memory\n");
			return (AH_FALSE);
		}
	}

	if (ah_radio == AR5K_AR5111) {
		ret = ar5k_ar5111_rfregs(channel, mode);
	} else if (ah_radio == AR5K_AR5112) {		
		ret = ar5k_ar5112_rfregs(channel, mode);
	}

	if (ret == AH_TRUE)
		ah_rf_gain = HAL_RFGAIN_INACTIVE;

	return (ret);
}

HAL_BOOL OpenHAL::ar5k_ar5111_rfregs(HAL_CHANNEL *channel, u_int mode) {
	struct ar5k_eeprom_info *ee = &ah_capabilities.cap_eeprom;
	const u_int rf_size = AR5K_ELEMENTS(ar5111_rf);
	u_int32_t *rf;
	int i, obdb = -1, bank = -1;
	u_int32_t ee_mode;

	AR5K_ASSERT_ENTRY(mode, AR5K_INI_VAL_MAX);

	rf = ah_rf_banks;

	/* Copy values to modify them */
	for (i = 0; i < rf_size; i++) {
		if (ar5111_rf[i].rf_bank >=
		    AR5K_AR5111_INI_RF_MAX_BANKS) {
			AR5K_PRINT("invalid bank\n");
			return (AH_FALSE);
		}

		if (bank != ar5111_rf[i].rf_bank) {
			bank = ar5111_rf[i].rf_bank;
			ah_offset[bank] = i;
		}

		rf[i] = ar5111_rf[i].rf_value[mode];
	}

	if (channel->c_channel_flags & IEEE80211_CHAN_2GHZ) {
		if (channel->c_channel_flags & IEEE80211_CHAN_B)
			ee_mode = AR5K_EEPROM_MODE_11B;
		else
			ee_mode = AR5K_EEPROM_MODE_11G;
		obdb = 0;

		if (!ar5k_rfregs_op(rf, ah_offset[0],
			ee->ee_ob[ee_mode][obdb], 3, 119, 0, AH_TRUE))
			return (AH_FALSE);

		if (!ar5k_rfregs_op(rf, ah_offset[0],
			ee->ee_ob[ee_mode][obdb], 3, 122, 0, AH_TRUE))
			return (AH_FALSE);

		obdb = 1;
	} else {
		/* For 11a, Turbo and XR */
		ee_mode = AR5K_EEPROM_MODE_11A;
		obdb = channel->c_channel >= 5725 ? 3 :
		    (channel->c_channel >= 5500 ? 2 :
			(channel->c_channel >= 5260 ? 1 :
			    (channel->c_channel > 4000 ? 0 : -1)));

		if (!ar5k_rfregs_op(rf, ah_offset[6],
			ee->ee_pwd_84, 1, 51, 3, AH_TRUE))
			return (AH_FALSE);

		if (!ar5k_rfregs_op(rf, ah_offset[6],
			ee->ee_pwd_90, 1, 45, 3, AH_TRUE))
			return (AH_FALSE);
	}

	if (!ar5k_rfregs_op(rf, ah_offset[6],
		!ee->ee_xpd[ee_mode], 1, 95, 0, AH_TRUE))
		return (AH_FALSE);

	if (!ar5k_rfregs_op(rf, ah_offset[6],
		ee->ee_x_gain[ee_mode], 4, 96, 0, AH_TRUE))
		return (AH_FALSE);

	if (!ar5k_rfregs_op(rf, ah_offset[6],
		obdb >= 0 ? ee->ee_ob[ee_mode][obdb] : 0, 3, 104, 0, AH_TRUE))
		return (AH_FALSE);

	if (!ar5k_rfregs_op(rf, ah_offset[6],
		obdb >= 0 ? ee->ee_db[ee_mode][obdb] : 0, 3, 107, 0, AH_TRUE))
		return (AH_FALSE);

	if (!ar5k_rfregs_op(rf, ah_offset[7],
		ee->ee_i_gain[ee_mode], 6, 29, 0, AH_TRUE))
		return (AH_FALSE);

	if (!ar5k_rfregs_op(rf, ah_offset[7],
		ee->ee_xpd[ee_mode], 1, 4, 0, AH_TRUE))
		return (AH_FALSE);

	/* Write RF values */
	for (i = 0; i < rf_size; i++) {
		AR5K_REG_WAIT(i);
		AR5K_REG_WRITE(ar5111_rf[i].rf_register, rf[i]);
	}

	return (AH_TRUE);
}

HAL_BOOL OpenHAL::ar5k_ar5112_rfregs(HAL_CHANNEL *channel, u_int mode) {
	struct ar5k_eeprom_info *ee = &ah_capabilities.cap_eeprom;
	const u_int rf_size = AR5K_ELEMENTS(ar5112_rf);
	u_int32_t *rf;
	int i, obdb = -1, bank = -1;
	u_int32_t ee_mode;

	AR5K_ASSERT_ENTRY(mode, AR5K_INI_VAL_MAX);

	rf = ah_rf_banks;

	/* Copy values to modify them */
	for (i = 0; i < rf_size; i++) {
		if (ar5112_rf[i].rf_bank >=
		    AR5K_AR5112_INI_RF_MAX_BANKS) {
			AR5K_PRINT("invalid bank\n");
			return (AH_FALSE);
		}

		if (bank != ar5112_rf[i].rf_bank) {
			bank = ar5112_rf[i].rf_bank;
			ah_offset[bank] = i;
		}

		rf[i] = ar5112_rf[i].rf_value[mode];
	}

	if (channel->c_channel_flags & IEEE80211_CHAN_2GHZ) {
		if (channel->c_channel_flags & IEEE80211_CHAN_B)
			ee_mode = AR5K_EEPROM_MODE_11B;
		else
			ee_mode = AR5K_EEPROM_MODE_11G;
		obdb = 0;

		if (!ar5k_rfregs_op(rf, ah_offset[6],
			ee->ee_ob[ee_mode][obdb], 3, 287, 0, AH_TRUE))
			return (AH_FALSE);

		if (!ar5k_rfregs_op(rf, ah_offset[6],
			ee->ee_ob[ee_mode][obdb], 3, 290, 0, AH_TRUE))
			return (AH_FALSE);
	} else {
		/* For 11a, Turbo and XR */
		ee_mode = AR5K_EEPROM_MODE_11A;
		obdb = channel->c_channel >= 5725 ? 3 :
		    (channel->c_channel >= 5500 ? 2 :
			(channel->c_channel >= 5260 ? 1 :
			    (channel->c_channel > 4000 ? 0 : -1)));

		if (!ar5k_rfregs_op(rf, ah_offset[6],
			ee->ee_ob[ee_mode][obdb], 3, 279, 0, AH_TRUE))
			return (AH_FALSE);

		if (!ar5k_rfregs_op(rf, ah_offset[6],
			ee->ee_ob[ee_mode][obdb], 3, 282, 0, AH_TRUE))
			return (AH_FALSE);
	}

#ifdef notyet
	ar5k_rfregs_op(rf, ah_offset[6],
	    ee->ee_x_gain[ee_mode], 2, 270, 0, AH_TRUE);
	ar5k_rfregs_op(rf, ah_offset[6],
	    ee->ee_x_gain[ee_mode], 2, 257, 0, AH_TRUE);
#endif

	if (!ar5k_rfregs_op(rf, ah_offset[6],
		ee->ee_xpd[ee_mode], 1, 302, 0, AH_TRUE))
		return (AH_FALSE);

	if (!ar5k_rfregs_op(rf, ah_offset[7],
		ee->ee_i_gain[ee_mode], 6, 14, 0, AH_TRUE))
		return (AH_FALSE);

	/* Write RF values */
	for (i = 0; i < rf_size; i++) {
		AR5K_REG_WRITE(ar5112_rf[i].rf_register, rf[i]);
		AR5K_DELAY(1);
	}

	return (AH_TRUE);
}

HAL_BOOL OpenHAL::ar5k_rfgain(u_int phy, u_int freq) {
	int i;

	switch (phy) {
	case AR5K_INI_PHY_5111:
	case AR5K_INI_PHY_5112:
		break;
	default:
		return (AH_FALSE);
	}

	switch (freq) {
	case AR5K_INI_RFGAIN_2GHZ:
	case AR5K_INI_RFGAIN_5GHZ:
		break;
	default:
		return (AH_FALSE);
	}

	for (i = 0; i < AR5K_ELEMENTS(ar5k_rfg); i++) {
		AR5K_REG_WAIT(i);
		AR5K_REG_WRITE((u_int32_t)ar5k_rfg[i].rfg_register,
		    ar5k_rfg[i].rfg_value[phy][freq]);
	}

	return (AH_TRUE);
}

#pragma mark -

void OpenHAL::ar5k_txpower_table(HAL_CHANNEL *channel, int16_t max_power) {
	u_int16_t txpower, *rates;
	int i;
	
	rates = ah_txpower.txp_rates;

	txpower = AR5K_TUNE_DEFAULT_TXPOWER * 2;
	if (max_power > txpower) {
		txpower = max_power > AR5K_TUNE_MAX_TXPOWER ?
		    AR5K_TUNE_MAX_TXPOWER : max_power;
	}

	for (i = 0; i < AR5K_MAX_RATES; i++)
		rates[i] = txpower;

	/* XXX setup target powers by rate */

	ah_txpower.txp_min = rates[7];
	ah_txpower.txp_max = rates[0];
	ah_txpower.txp_ofdm = rates[0];

	for (i = 0; i < AR5K_ELEMENTS(ah_txpower.txp_pcdac); i++)
		ah_txpower.txp_pcdac[i] = AR5K_EEPROM_PCDAC_START;
}

#pragma mark -

u_int32_t OpenHAL::ieee80211_regdomain2flag(u_int16_t regdomain, u_int16_t mhz) {
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