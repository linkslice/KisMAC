/*
 *  OpenHAL5212.h
 *  AtheroJack
 *
 *  Created by mick on 30.03.2005.
 *  Copyright 2005 Michael Rossberg, Beat Zahnd. All rights reserved.
 *
 */

#include "OpenHAL.h"

class OpenHAL5212 : public OpenHAL {
	OSDeclareDefaultStructors(OpenHAL5212);

public:

#pragma mark -

	virtual HAL_BOOL ar5k_ar5212_attach(u_int16_t device, bus_space_tag_t st, bus_space_handle_t sh, HAL_STATUS *status);
	virtual const HAL_RATE_TABLE * ar5k_ar5212_get_rate_table(u_int mode);
	virtual void ar5k_ar5212_detach();
	virtual HAL_BOOL ar5k_ar5212_reset(HAL_OPMODE op_mode, HAL_CHANNEL *channel, HAL_BOOL change_channel, HAL_STATUS *status);
	virtual void ar5k_ar5212_set_opmode();
	virtual HAL_BOOL ar5k_ar5212_calibrate(HAL_CHANNEL *channel);

#pragma mark -

/*
 * Transmit functions
 */

	virtual HAL_BOOL ar5k_ar5212_update_tx_triglevel(HAL_BOOL increase);
	virtual int ar5k_ar5212_setup_tx_queue(HAL_TX_QUEUE queue_type, const HAL_TXQ_INFO *queue_info);
	virtual HAL_BOOL ar5k_ar5212_setup_tx_queueprops(int queue, const HAL_TXQ_INFO *queue_info);
	virtual HAL_BOOL ar5k_ar5212_release_tx_queue(u_int queue);
	virtual HAL_BOOL ar5k_ar5212_reset_tx_queue(u_int queue);
	virtual u_int32_t ar5k_ar5212_get_tx_buf(u_int queue);
	virtual HAL_BOOL ar5k_ar5212_put_tx_buf(u_int queue, u_int32_t phys_addr);
	virtual HAL_BOOL ar5k_ar5212_tx_start(u_int queue);
	virtual HAL_BOOL ar5k_ar5212_stop_tx_dma(u_int queue);
	virtual HAL_BOOL ar5k_ar5212_setup_tx_desc(struct ath_desc *desc, u_int packet_length, u_int header_length, HAL_PKT_TYPE type, u_int tx_power, u_int tx_rate0, u_int tx_tries0, u_int key_index, u_int antenna_mode, u_int flags, u_int rtscts_rate, u_int rtscts_duration);
	virtual HAL_BOOL ar5k_ar5212_fill_tx_desc(struct ath_desc *desc, u_int segment_length, HAL_BOOL first_segment, HAL_BOOL last_segment);
	virtual HAL_BOOL ar5k_ar5212_setup_xtx_desc(struct ath_desc *desc, u_int tx_rate1, u_int tx_tries1, u_int tx_rate2, u_int tx_tries2, u_int tx_rate3, u_int tx_tries3);
	virtual HAL_STATUS ar5k_ar5212_proc_tx_desc(struct ath_desc *desc);
	virtual HAL_BOOL ar5k_ar5212_has_veol();

#pragma mark -

/*
 * Receive functions
 */

	virtual u_int32_t ar5k_ar5212_get_rx_buf();
	virtual void ar5k_ar5212_put_rx_buf(u_int32_t phys_addr);
	virtual void ar5k_ar5212_start_rx();
	virtual HAL_BOOL ar5k_ar5212_stop_rx_dma();
	virtual void ar5k_ar5212_start_rx_pcu();
	virtual void ar5k_ar5212_stop_pcu_recv();
	virtual void ar5k_ar5212_set_mcast_filter(u_int32_t filter0, u_int32_t filter1);
	virtual HAL_BOOL ar5k_ar5212_set_mcast_filterindex(u_int32_t index);
	virtual HAL_BOOL ar5k_ar5212_clear_mcast_filter_idx(u_int32_t index);
	virtual u_int32_t ar5k_ar5212_get_rx_filter();
	virtual void ar5k_ar5212_set_rx_filter(u_int32_t filter);
	virtual HAL_BOOL ar5k_ar5212_setup_rx_desc(struct ath_desc *desc, u_int32_t size, u_int flags);
	virtual HAL_STATUS ar5k_ar5212_proc_rx_desc(struct ath_desc *desc, u_int32_t phys_addr, struct ath_desc *next);
	virtual void ar5k_ar5212_set_rx_signal();

#pragma mark -

/*
 * Misc functions
 */

	virtual void ar5k_ar5212_dump_state();
	virtual HAL_BOOL ar5k_ar5212_get_diag_state(int id, void **device, u_int *size);
	virtual void ar5k_ar5212_get_lladdr(u_int8_t *mac);
	virtual HAL_BOOL ar5k_ar5212_set_lladdr(const u_int8_t *mac);
	virtual HAL_BOOL ar5k_ar5212_set_regdomain(u_int16_t regdomain, HAL_STATUS *status);
	virtual void ar5k_ar5212_set_ledstate(HAL_LED_STATE state);
	virtual void ar5k_ar5212_set_associd(const u_int8_t *bssid, u_int16_t assoc_id, u_int16_t tim_offset);
	virtual HAL_BOOL ar5k_ar5212_set_gpio_output(u_int32_t gpio);
	virtual HAL_BOOL ar5k_ar5212_set_gpio_input(u_int32_t gpio);
	virtual u_int32_t ar5k_ar5212_get_gpio(u_int32_t gpio);
	virtual HAL_BOOL ar5k_ar5212_set_gpio(u_int32_t gpio, u_int32_t val);
	virtual void ar5k_ar5212_set_gpio_intr(u_int gpio, u_int32_t interrupt_level);
	virtual u_int32_t ar5k_ar5212_get_tsf32();
	virtual u_int64_t ar5k_ar5212_get_tsf64();
	virtual void ar5k_ar5212_reset_tsf();
	virtual u_int16_t ar5k_ar5212_get_regdomain();
	virtual HAL_BOOL ar5k_ar5212_detect_card_present();
	virtual void ar5k_ar5212_update_mib_counters(HAL_MIB_STATS *statistics);
	virtual HAL_RFGAIN ar5k_ar5212_get_rf_gain();
	virtual HAL_BOOL ar5k_ar5212_set_slot_time(u_int slot_time);
	virtual u_int ar5k_ar5212_get_slot_time();
	virtual HAL_BOOL ar5k_ar5212_set_ack_timeout(u_int timeout);
	virtual u_int ar5k_ar5212_get_ack_timeout();
	virtual HAL_BOOL ar5k_ar5212_set_cts_timeout(u_int timeout);
	virtual u_int ar5k_ar5212_get_cts_timeout();

#pragma mark -

/*
 * Key table (WEP) functions
 */

	virtual HAL_BOOL ar5k_ar5212_is_cipher_supported(HAL_CIPHER cipher);
	virtual u_int32_t ar5k_ar5212_get_keycache_size();
	virtual HAL_BOOL ar5k_ar5212_reset_key(u_int16_t entry);
	virtual HAL_BOOL ar5k_ar5212_is_key_valid(u_int16_t entry);
	virtual HAL_BOOL ar5k_ar5212_set_key(u_int16_t entry, const HAL_KEYVAL *keyval, const u_int8_t *mac, int xor_notused);
	virtual HAL_BOOL ar5k_ar5212_set_key_lladdr(u_int16_t entry, const u_int8_t *mac);

#pragma mark -

/*
 * Power management functions
 */

	virtual HAL_BOOL ar5k_ar5212_set_power(HAL_POWER_MODE mode, HAL_BOOL set_chip, u_int16_t sleep_duration);
	virtual HAL_POWER_MODE ar5k_ar5212_get_power_mode();
	virtual HAL_BOOL ar5k_ar5212_query_pspoll_support();
	virtual HAL_BOOL ar5k_ar5212_init_pspoll();
	virtual HAL_BOOL ar5k_ar5212_enable_pspoll(u_int8_t *bssid, u_int16_t assoc_id);
	virtual HAL_BOOL ar5k_ar5212_disable_pspoll();
	
#pragma mark -

/*
 * Beacon functions
 */

	virtual void ar5k_ar5212_init_beacon(u_int32_t next_beacon, u_int32_t interval);
	virtual void ar5k_ar5212_set_beacon_timers(const HAL_BEACON_STATE *state, u_int32_t tsf, u_int32_t dtim_count, u_int32_t cfp_count);
	virtual void ar5k_ar5212_reset_beacon();
	virtual HAL_BOOL ar5k_ar5212_wait_for_beacon(bus_addr_t phys_addr);

#pragma mark -

/*
 * Interrupt handling
 */

	virtual HAL_BOOL ar5k_ar5212_is_intr_pending();
	virtual HAL_BOOL ar5k_ar5212_get_isr(u_int32_t *interrupt_mask);
	virtual u_int32_t ar5k_ar5212_get_intr();
	virtual HAL_INT ar5k_ar5212_set_intr(HAL_INT new_mask);
	
#pragma mark -

/*
 * Misc internal functions
 */

	virtual HAL_BOOL ar5k_ar5212_get_capabilities();
	virtual void ar5k_ar5212_radar_alert(HAL_BOOL enable);

#pragma mark -

/*
 * EEPROM access functions
 */

	virtual HAL_BOOL ar5k_ar5212_eeprom_is_busy();
	virtual HAL_STATUS ar5k_ar5212_eeprom_read(u_int32_t offset, u_int16_t *data);
	virtual HAL_STATUS ar5k_ar5212_eeprom_write(u_int32_t offset, u_int16_t data);
	
#pragma mark -

private:

	virtual const void ar5k_ar5212_fill();
	virtual HAL_BOOL ar5k_ar5212_nic_reset(u_int32_t val);
	virtual HAL_BOOL ar5k_ar5212_nic_wakeup(u_int16_t flags);
	virtual u_int16_t ar5k_ar5212_radio_revision(HAL_CHIP chip);

	virtual HAL_BOOL ar5k_ar5212_txpower(HAL_CHANNEL *channel, u_int txpower);

};
