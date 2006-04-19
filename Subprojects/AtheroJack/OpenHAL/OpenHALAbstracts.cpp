/*
 *  OpenHALAbstracts.cpp
 *  AtheroJack
 *
 *  Created by mick on 31.03.2005.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#include "OpenHAL.h"

//place holder
HAL_BOOL OpenHAL::nic_ath_hal_attach(u_int16_t device, HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status) {
	return AH_FALSE;
}
void OpenHAL::nic_setPCUConfig() {
}
u_int32_t OpenHAL::nic_getRxDP() {
}
void OpenHAL::nic_setRxDP(u_int32_t phys_addr) {
}
void OpenHAL::nic_enableReceive() {
}
HAL_BOOL OpenHAL::nic_stopDmaReceive() { 
	return AH_FALSE;
}
void OpenHAL::nic_startPcuReceive() {
}
void OpenHAL::nic_stopPcuReceive() {
}
void OpenHAL::nic_getMacAddress(u_int8_t *mac) {
}
HAL_BOOL OpenHAL::nic_setMacAddress(const u_int8_t *mac) {
	return AH_FALSE;
}
HAL_BOOL OpenHAL::nic_setRegulatoryDomain(u_int16_t regdomain, HAL_STATUS *status) {
	return AH_FALSE;
}
void OpenHAL::nic_setLedState(HAL_LED_STATE state) {
}
void OpenHAL::nic_writeAssocid(const u_int8_t *bssid, u_int16_t assoc_id, u_int16_t tim_offset) {
}
HAL_BOOL OpenHAL::nic_resetKeyCacheEntry(u_int16_t entry) {
	return (AH_FALSE);
}
HAL_POWER_MODE OpenHAL::nic_getPowerMode() {
	return (ah_power_mode);
}
HAL_BOOL OpenHAL::nic_setPowerMode(HAL_POWER_MODE mode, HAL_BOOL set_chip, u_int16_t sleep_duration) {
	return (AH_FALSE);
}
HAL_BOOL OpenHAL::nic_queryPSPollSupport() {
	return (AH_FALSE);
}
HAL_BOOL OpenHAL::nic_initPSPoll() {
	return (AH_FALSE);
}
HAL_BOOL OpenHAL::nic_enablePSPoll(u_int8_t *bssid, u_int16_t assoc_id) {
	return (AH_FALSE);
}
HAL_BOOL OpenHAL::nic_disablePSPoll() {
	return (AH_FALSE);
}
HAL_BOOL OpenHAL::nic_isInterruptPending() {
	return (AH_FALSE);
}
HAL_BOOL OpenHAL::nic_getPendingInterrupts(u_int32_t *interrupt_mask) {
	return (AH_FALSE);
}
u_int32_t OpenHAL::nic_getInterrupts() {
	return HAL_INT_NOCARD;
}
HAL_INT OpenHAL::nic_setInterrupts(HAL_INT new_mask) {
	return (0);
}
HAL_BOOL OpenHAL::nic_get_capabilities() {
	return AH_FALSE;
}
HAL_BOOL OpenHAL::nic_eeprom_is_busy() {
	return AH_TRUE;
}
HAL_STATUS OpenHAL::nic_eeprom_read(u_int32_t offset, u_int16_t *data) {
	return HAL_ENOTSUPP;
}
HAL_STATUS OpenHAL::nic_eeprom_write(u_int32_t offset, u_int16_t data) {
	return HAL_ENOTSUPP;
}
HAL_BOOL OpenHAL::nic_channel(HAL_CHANNEL *channel) {
	return AH_FALSE;
}
	