/*
 *  OpenHAL.h
 *  AtheroJack
 *
 *  Created by mick on 30.03.2005.
 *  Copyright 2005 Michael Rossberg, Beat Zahnd. All rights reserved.
 *
 */

#include "OpenHALDefinitions.h"
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>

#define AR5K_MAX_GPIO		10
#define AR5K_MAX_RF_BANKS	8

typedef int bus_space_tag_t;
typedef void* bus_space_handle_t;
typedef u_int32_t bus_addr_t;			/* XXX architecture dependent */

class OpenHAL : public OSObject {
	OSDeclareDefaultStructors(OpenHAL);
	
public:
	virtual bool init();
	
	virtual HAL_BOOL ath_hal_attach(u_int16_t device, void *sc, bus_space_tag_t st, bus_space_handle_t sh, HAL_STATUS *status);
	virtual u_int16_t ath_hal_computetxtime(const HAL_RATE_TABLE *rates, u_int32_t frame_length, u_int16_t rate_index, HAL_BOOL short_preamble);
	virtual u_int ath_hal_mhz2ieee(u_int mhz, u_int flags);
	virtual u_int ath_hal_ieee2mhz(u_int ieee, u_int flags);
	virtual HAL_BOOL ath_hal_init_channels(HAL_CHANNEL *channels, u_int max_channels, u_int *channels_size, u_int16_t mode, HAL_BOOL outdoor, HAL_BOOL extended);
	
	virtual const char * ar5k_printver(enum ar5k_srev_type type, u_int32_t val);
	virtual void ar5k_radar_alert();
	virtual u_int16_t ar5k_regdomain_from_ieee(ieee80211_regdomain_t ieee);
	virtual ieee80211_regdomain_t ar5k_regdomain_to_ieee(u_int16_t regdomain);
	virtual u_int16_t ar5k_get_regdomain();
	virtual u_int32_t ar5k_bitswap(u_int32_t val, u_int bits);
	virtual u_int ar5k_htoclock(u_int usec, HAL_BOOL turbo);
	virtual u_int ar5k_clocktoh(u_int clock, HAL_BOOL turbo);
	virtual void ar5k_rt_copy(HAL_RATE_TABLE *dst, const HAL_RATE_TABLE *src);
	virtual HAL_BOOL ar5k_register_timeout(u_int32_t reg, u_int32_t flag, u_int32_t val, HAL_BOOL is_set);
	
	virtual int ar5k_eeprom_init();
	virtual HAL_STATUS ar5k_eeprom_read_mac(u_int8_t *mac);
	virtual HAL_BOOL ar5k_eeprom_regulation_domain(HAL_BOOL write, ieee80211_regdomain_t *regdomain);
	
	virtual HAL_BOOL ar5k_channel(HAL_CHANNEL *channel);
	virtual u_int32_t ar5k_rfregs_gainf_corr();
	virtual HAL_BOOL ar5k_rfregs_gain_readback();
	virtual int32_t ar5k_rfregs_gain_adjust();
	virtual HAL_BOOL ar5k_rfregs(HAL_CHANNEL *channel, u_int mode);
	virtual HAL_BOOL ar5k_rfgain(u_int phy, u_int freq);
	
	virtual void ar5k_txpower_table(HAL_CHANNEL *channel, int16_t max_power);

#pragma mark -

private:
	virtual HAL_BOOL ar5k_check_channel(u_int16_t freq, u_int flags);
	
	virtual int ar5k_eeprom_read_ants(u_int32_t *offset, u_int mode);
	virtual int ar5k_eeprom_read_modes(u_int32_t *offset, u_int mode);
	
	virtual u_int16_t ar5k_eeprom_bin2freq(u_int16_t bin, u_int mode);
	
	virtual u_int32_t ar5k_ar5110_chan2athchan(HAL_CHANNEL *channel);
	virtual HAL_BOOL ar5k_ar5110_channel(HAL_CHANNEL *channel);
	virtual HAL_BOOL ar5k_ar5111_chan2athchan(u_int ieee, struct ar5k_athchan_2ghz *athchan);
	virtual HAL_BOOL ar5k_ar5111_channel(HAL_CHANNEL *channel);
	virtual HAL_BOOL ar5k_ar5112_channel(HAL_CHANNEL *channel);
	
	virtual u_int ar5k_rfregs_op(u_int32_t *rf, u_int32_t offset, u_int32_t reg, u_int32_t bits, u_int32_t first, u_int32_t col, HAL_BOOL set);
	virtual HAL_BOOL ar5k_ar5111_rfregs(HAL_CHANNEL *channel, u_int mode);
	virtual HAL_BOOL ar5k_ar5112_rfregs(HAL_CHANNEL *channel, u_int mode);

#pragma mark -

public:
	virtual u_int ieee80211_mhz2ieee(u_int freq, u_int flags);
	virtual u_int ieee80211_ieee2mhz(u_int chan, u_int flags);
	
	virtual u_int32_t ieee80211_regdomain2flag(u_int16_t regdomain, u_int16_t mhz);

	virtual HAL_BOOL ar5k_ar5212_attach(u_int16_t device, bus_space_tag_t st, bus_space_handle_t sh, HAL_STATUS *status); 
	
	virtual void ar5k_ar5212_dump_state();
	virtual HAL_BOOL ar5k_ar5212_set_lladdr(const u_int8_t *mac);
	
	virtual HAL_BOOL ar5k_ar5212_get_capabilities();
	
	virtual HAL_BOOL ar5k_ar5212_eeprom_is_busy();
	virtual HAL_STATUS ar5k_ar5212_eeprom_read(u_int32_t offset, u_int16_t *data);
	virtual HAL_STATUS ar5k_ar5212_eeprom_write(u_int32_t offset, u_int16_t data);

#pragma mark -

public:
	u_int32_t		ah_magic;
	u_int32_t		ah_abi;
	u_int16_t		ah_device;
	u_int16_t		ah_sub_vendor;

	void			*ah_sc;
	bus_space_tag_t		ah_st;
	bus_space_handle_t	ah_sh;

	HAL_INT			ah_imr;

	HAL_OPMODE		ah_op_mode;
	HAL_POWER_MODE		ah_power_mode;
	HAL_CHANNEL		ah_current_channel;
	HAL_BOOL		ah_turbo;
	HAL_BOOL		ah_calibration;
	HAL_BOOL		ah_running;
	HAL_RFGAIN		ah_rf_gain;

	HAL_RATE_TABLE		ah_rt_11a;
	HAL_RATE_TABLE		ah_rt_11b;
	HAL_RATE_TABLE		ah_rt_11g;
	HAL_RATE_TABLE		ah_rt_turbo;
	HAL_RATE_TABLE		ah_rt_xr;

	u_int32_t		ah_mac_srev;
	u_int16_t		ah_mac_version;
	u_int16_t		ah_mac_revision;
	u_int16_t		ah_phy_revision;
	u_int16_t		ah_radio_5ghz_revision;
	u_int16_t		ah_radio_2ghz_revision;

	enum ar5k_version	ah_version;
	enum ar5k_radio		ah_radio;
	u_int32_t		ah_phy;

	HAL_BOOL		ah_5ghz;
	HAL_BOOL		ah_2ghz;

#define ah_regdomain		ah_capabilities.cap_regdomain.reg_current
#define ah_regdomain_hw		ah_capabilities.cap_regdomain.reg_hw
#define ah_modes		ah_capabilities.cap_mode
#define ah_ee_version		ah_capabilities.cap_eeprom.ee_version

	u_int32_t		ah_atim_window;
	u_int32_t		ah_aifs;
	u_int32_t		ah_cw_min;
	u_int32_t		ah_cw_max;
	HAL_BOOL		ah_software_retry;
	u_int32_t		ah_limit_tx_retries;

	u_int32_t		ah_antenna[AR5K_EEPROM_N_MODES][HAL_ANT_MAX];
	HAL_BOOL		ah_ant_diversity;

	u_int8_t		ah_sta_id[IEEE80211_ADDR_LEN];
	u_int8_t		ah_bssid[IEEE80211_ADDR_LEN];

	u_int32_t		ah_gpio[AR5K_MAX_GPIO];
	int			ah_gpio_npins;

	ar5k_capabilities_t	ah_capabilities;

	HAL_TXQ_INFO		ah_txq[HAL_NUM_TX_QUEUES];
	u_int32_t		ah_txq_interrupts;

	u_int32_t		*ah_rf_banks;
	size_t			ah_rf_banks_size;
	struct ar5k_gain	ah_gain;
	u_int32_t		ah_offset[AR5K_MAX_RF_BANKS];

	struct {
		u_int16_t	txp_pcdac[AR5K_EEPROM_POWER_TABLE_SIZE];
		u_int16_t	txp_rates[AR5K_MAX_RATES];
		int16_t		txp_min, txp_max;
		HAL_BOOL	txp_tpc;
		int16_t		txp_ofdm;
	} ah_txpower;

	struct {
		HAL_BOOL	r_enabled;
		int		r_last_alert;
		HAL_CHANNEL	r_last_channel;
	} ah_radar;

};
