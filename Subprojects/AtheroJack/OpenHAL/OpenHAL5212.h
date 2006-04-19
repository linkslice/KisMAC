/*
 *  OpenHAL5212.h
 *  AtheroJack
 *
 *  Created by mick on 30.03.2005.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#include "OpenHAL.h"

class OpenHAL5212 : public OpenHAL {
	OSDeclareDefaultStructors(OpenHAL5212);

	virtual HAL_BOOL nic_ath_hal_attach(u_int16_t device, HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status);
	
public:
	virtual void nic_detach();
	
	virtual const HAL_RATE_TABLE * nic_getRateTable(u_int mode);
	virtual HAL_BOOL nic_reset(HAL_OPMODE op_mode, HAL_CHANNEL *channel, HAL_BOOL change_channel, HAL_STATUS *status);
	virtual void nic_setPCUConfig();
	virtual HAL_BOOL nic_perCalibration(HAL_CHANNEL *channel);
	
	virtual HAL_BOOL nic_resetTxQueue(u_int queue);
	
	virtual u_int32_t nic_getRxDP();
	virtual void nic_setRxDP(u_int32_t phys_addr);
	virtual void nic_enableReceive();
	virtual HAL_BOOL nic_stopDmaReceive();
	virtual void nic_startPcuReceive();
	virtual void nic_stopPcuReceive();
	virtual u_int32_t nic_getRxFilter();
	virtual void nic_setRxFilter(u_int32_t filter);
	virtual HAL_BOOL nic_setupRxDesc(struct ath_desc *desc, u_int32_t size, u_int flags);
	virtual HAL_STATUS nic_procRxDesc(struct ath_desc *desc, u_int32_t phys_addr, struct ath_desc *next);
	virtual void nic_rxMonitor();
	virtual void nic_getMacAddress(u_int8_t *mac);
	virtual HAL_BOOL nic_setMacAddress(const u_int8_t *mac);
	virtual HAL_BOOL nic_setRegulatoryDomain(u_int16_t regdomain, HAL_STATUS *status);
	virtual void nic_setLedState(HAL_LED_STATE state);
	virtual void nic_writeAssocid(const u_int8_t *bssid, u_int16_t assoc_id, u_int16_t tim_offset);
	
	virtual HAL_BOOL nic_gpioCfgInput(u_int32_t gpio);
	virtual u_int32_t nic_gpioGet(u_int32_t gpio);
	virtual void nic_gpioSetIntr(u_int gpio, u_int32_t interrupt_level);

	virtual HAL_RFGAIN nic_getRfGain();
	virtual HAL_BOOL nic_resetKeyCacheEntry(u_int16_t entry);
	virtual HAL_BOOL nic_setPowerMode(HAL_POWER_MODE mode, HAL_BOOL set_chip, u_int16_t sleep_duration);
	virtual HAL_BOOL nic_isInterruptPending();
	virtual HAL_BOOL nic_getPendingInterrupts(u_int32_t *interrupt_mask);
	virtual u_int32_t nic_getInterrupts();
	virtual HAL_INT nic_setInterrupts(HAL_INT new_mask);
	
	virtual void dumpState();

	virtual HAL_BOOL nic_eeprom_is_busy();
	virtual HAL_STATUS nic_eeprom_read(u_int32_t offset, u_int16_t *data);
	virtual HAL_STATUS nic_eeprom_write(u_int32_t offset, u_int16_t data);
	
protected:
	virtual HAL_BOOL nic_get_capabilities();
	virtual HAL_BOOL nic_channel(HAL_CHANNEL *channel);
	
private:
	virtual HAL_BOOL ar5k_ar5212_nic_wakeup(u_int16_t flags);
	virtual u_int16_t ar5k_ar5212_radio_revision(HAL_CHIP chip);
	virtual HAL_BOOL ar5k_ar5212_nic_reset(u_int32_t val);
	virtual HAL_BOOL ar5k_ar5212_txpower(HAL_CHANNEL *channel, u_int txpower);
};