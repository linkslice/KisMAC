/*
 *  OpenHALAbstracts.cpp
 *  AtheroJack
 *
 *  Created by mick on 31.03.2005.
 *  Copyright 2005 Michael Rossberg, Beat Zahnd. All rights reserved.
 *
 */

#include "OpenHAL.h"

HAL_BOOL OpenHAL::ar5k_ar5212_attach(u_int16_t device, bus_space_tag_t st, bus_space_handle_t sh, HAL_STATUS *status) {
	return AH_FALSE;
}
void OpenHAL::ar5k_ar5212_dump_state() {
}
HAL_BOOL OpenHAL::ar5k_ar5212_set_lladdr(const u_int8_t *mac) {
	return AH_FALSE;
}
HAL_BOOL OpenHAL::ar5k_ar5212_get_capabilities() {
	return AH_FALSE;
}
HAL_BOOL OpenHAL::ar5k_ar5212_eeprom_is_busy() {
	return AH_TRUE;
}
HAL_STATUS OpenHAL::ar5k_ar5212_eeprom_read(u_int32_t offset, u_int16_t *data) {
	return HAL_ENOTSUPP;
}
HAL_STATUS OpenHAL::ar5k_ar5212_eeprom_write(u_int32_t offset, u_int16_t data) {
	return HAL_ENOTSUPP;
}
