/*
 *  OpenHAL.h
 *  AtheroJack
 *
 *  Created by mick on 30.03.2005.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#include "OpenHALDefinitions.h"
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>

#define AR5K_MAX_GPIO		10
#define AR5K_MAX_RF_BANKS	8

typedef void* HAL_SOFTC;
typedef int HAL_BUS_TAG;
typedef void* HAL_BUS_HANDLE;
typedef u_int32_t HAL_BUS_ADDR;			/* XXX architecture dependent */

class OpenHAL : public OSObject {
	OSDeclareDefaultStructors(OpenHAL);
	
public:
    virtual bool init();

	virtual bool ath_hal_attach(u_int16_t device, void *sc, HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status);
	virtual u_int16_t ath_hal_computetxtime(const HAL_RATE_TABLE *rates, u_int32_t frame_length, u_int16_t rate_index, HAL_BOOL short_preamble);
	virtual HAL_BOOL ath_hal_init_channels(HAL_CHANNEL *channels, u_int max_channels, u_int *channels_size, HAL_CTRY_CODE country, u_int16_t mode, HAL_BOOL outdoor, HAL_BOOL extended);
	virtual u_int ieee80211_mhz2ieee(u_int freq, u_int flags);
	virtual u_int ieee80211_ieee2mhz(u_int chan, u_int flags);

protected:
	virtual HAL_BOOL ar5k_check_channel(u_int16_t freq, u_int flags);
	virtual u_int32_t ieee80211_regdomain2flag(u_int16_t regdomain, u_int16_t mhz);
	virtual HAL_BOOL nic_ath_hal_attach(u_int16_t device, HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status); 
	virtual HAL_BOOL nic_eeprom_is_busy();
	virtual HAL_STATUS nic_eeprom_read(u_int32_t offset, u_int16_t *data);
	virtual HAL_STATUS nic_eeprom_write(u_int32_t offset, u_int16_t data);
	virtual HAL_BOOL nic_get_capabilities();
	virtual HAL_BOOL nic_channel(HAL_CHANNEL *channel);
	
public:
	virtual void nic_setPCUConfig();
	virtual u_int32_t nic_getRxDP();
	virtual void nic_setRxDP(u_int32_t phys_addr);
	virtual void nic_enableReceive();
	virtual HAL_BOOL nic_stopDmaReceive();
	virtual void nic_startPcuReceive();
	virtual void nic_stopPcuReceive();
	virtual void nic_getMacAddress(u_int8_t *mac);
	virtual HAL_BOOL nic_setMacAddress(const u_int8_t *mac);
	virtual HAL_BOOL nic_setRegulatoryDomain(u_int16_t regdomain, HAL_STATUS *status);
	virtual void nic_setLedState(HAL_LED_STATE state);
	virtual void nic_writeAssocid(const u_int8_t *bssid, u_int16_t assoc_id, u_int16_t tim_offset);
	virtual HAL_BOOL nic_resetKeyCacheEntry(u_int16_t entry);
	virtual HAL_BOOL nic_queryPSPollSupport();
	virtual HAL_BOOL nic_setPowerMode(HAL_POWER_MODE mode, HAL_BOOL set_chip, u_int16_t sleep_duration);
	virtual HAL_POWER_MODE nic_getPowerMode();
	virtual HAL_BOOL nic_initPSPoll();
	virtual HAL_BOOL nic_enablePSPoll(u_int8_t *bssid, u_int16_t assoc_id);
	virtual HAL_BOOL nic_disablePSPoll();
	virtual HAL_BOOL nic_isInterruptPending();
	virtual HAL_BOOL nic_getPendingInterrupts(u_int32_t *interrupt_mask);
	virtual u_int32_t nic_getInterrupts();
	virtual HAL_INT nic_setInterrupts(HAL_INT new_mask);
	
protected:
	virtual int ar5k_check_eeprom();
	virtual int ar5k_eeprom_init();
	virtual HAL_STATUS ar5k_eeprom_read_mac(u_int8_t *mac);
	virtual HAL_BOOL ar5k_eeprom_regulation_domain(HAL_BOOL write, ieee80211_regdomain_t *regdomain);
	virtual u_int16_t ar5k_eeprom_bin2freq(u_int16_t bin, u_int mode);
	virtual int ar5k_eeprom_read_ants(u_int32_t *offset, u_int mode);
	virtual int ar5k_eeprom_read_modes(u_int32_t *offset, u_int mode);
	virtual void ar5k_rt_copy(HAL_RATE_TABLE *dst, HAL_RATE_TABLE *src);
	virtual HAL_BOOL ar5k_register_timeout(u_int32_t reg, u_int32_t flag, u_int32_t val, HAL_BOOL is_set);
	virtual ieee80211_regdomain_t ar5k_regdomain_to_ieee(u_int16_t regdomain);
	virtual u_int16_t ar5k_regdomain_from_ieee(ieee80211_regdomain_t ieee);
	virtual u_int16_t ar5k_get_regdomain();
	virtual u_int32_t ar5k_bitswap(u_int32_t val, u_int bits);
	
	virtual HAL_BOOL ar5k_channel(HAL_CHANNEL *channel);
	
	virtual u_int ar5k_rfregs_op(u_int32_t *rf, u_int32_t offset, u_int32_t reg, u_int32_t bits, u_int32_t first, u_int32_t col, HAL_BOOL set);
	virtual u_int32_t ar5k_rfregs_gainf_corr();
	virtual HAL_BOOL ar5k_rfregs_gain_readback();
	virtual int32_t ar5k_rfregs_gain_adjust();
	virtual HAL_BOOL ar5k_rfregs(HAL_CHANNEL *channel, u_int mode);
	virtual HAL_BOOL ar5k_ar5111_rfregs(HAL_CHANNEL *channel, u_int mode);
	virtual HAL_BOOL ar5k_ar5112_rfregs(HAL_CHANNEL *channel, u_int mode);
	virtual HAL_BOOL ar5k_rfgain(u_int phy, u_int freq);
	virtual void ar5k_txpower_table(HAL_CHANNEL *channel, int16_t max_power);

public:
	u_int32_t		ah_magic;
	u_int32_t		ah_abi;
	u_int16_t		ah_device;
	u_int16_t		ah_sub_vendor;

protected:
	void			*ah_sc;
	HAL_BUS_TAG		ah_st;
	HAL_BUS_HANDLE	ah_sh;

	HAL_INT			ah_imr;

	HAL_CTRY_CODE		ah_country_code;
	HAL_OPMODE		ah_op_mode;
	HAL_POWER_MODE		ah_power_mode;
	HAL_CHANNEL		ah_current_channel;
	HAL_BOOL		ah_turbo;
	HAL_BOOL		ah_calibration;
	HAL_BOOL		ah_running;
	HAL_RFGAIN		ah_rf_gain;

#define ah_countryCode		ah_country_code

	HAL_RATE_TABLE		ah_rt_11a;
	HAL_RATE_TABLE		ah_rt_11b;
	HAL_RATE_TABLE		ah_rt_11g;
	HAL_RATE_TABLE		ah_rt_turbo;
	HAL_RATE_TABLE		ah_rt_xr;

	u_int32_t		ah_mac_version;
	u_int16_t		ah_mac_revision;
	u_int16_t		ah_phy_revision;
	u_int16_t		ah_radio_5ghz_revision;
	u_int16_t		ah_radio_2ghz_revision;

	enum ar5k_version	ah_version;
	enum ar5k_radio		ah_radio;
	u_int32_t		ah_phy;

	HAL_BOOL		ah_5ghz;
	HAL_BOOL		ah_2ghz;

#define ah_macVersion		ah_mac_version
#define ah_macRev		ah_mac_revision
#define ah_phyRev		ah_phy_revision
#define ah_analog5GhzRev	ah_radio_5ghz_revision
#define ah_analog2GhzRev	ah_radio_2ghz_revision
#define ah_regdomain		ah_capabilities.cap_regdomain.reg_current
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