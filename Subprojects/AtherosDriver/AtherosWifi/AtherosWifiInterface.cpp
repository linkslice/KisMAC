#include "AtherosWifiInterface.h"
#include "AtherosWifi.h"

OSDefineMetaClassAndStructors( AtherosWIFIInterface, IO80211Interface )

#define super IO80211Interface

struct ieee80211_node* AtherosWIFIInterface::intf_node_alloc()
{
	struct ath_node *an =
			(struct ath_node *)IOMalloc(sizeof(struct ath_node));

	if (an) {
		int i;
		for (i = 0; i < ATH_RHIST_SIZE; i++)
			an->an_rx_hist[i].arh_ticks = ATH_RHIST_NOTIME;
		an->an_rx_hist_next = ATH_RHIST_SIZE-1;
		return &an->an_node;
	}
	
	return NULL;
}

void AtherosWIFIInterface::intf_node_free(struct ieee80211_node *ni)
{
	ctlr->purge_node_refs(ATH_NODE(ni));
	IOFree(ni, sizeof(struct ath_node));
}

void AtherosWIFIInterface::intf_node_copy(struct ieee80211_node *dst, const struct ieee80211_node *src)
{
	*(struct ath_node *)dst = *(const struct ath_node *)src;
}


u_int8_t AtherosWIFIInterface::intf_node_getrssi(struct ieee80211_node *ni)
{
	struct ath_node *an = ATH_NODE(ni);
	int i, nsamples, rssi;
	
	/*
	 * Calculate the average over the last second of sampled data.
	 */
	nsamples = 0;
	rssi = 0;
	i = an->an_rx_hist_next;
	goto done;
/*
	int now = ticks;
		do {
		struct ath_recv_hist *rh = &an->an_rx_hist[i];
		if (rh->arh_ticks == ATH_RHIST_NOTIME)
			goto done;
		if (now - rh->arh_ticks > hz)
			goto done;
		rssi += rh->arh_rssi;
		nsamples++;
		if (i == 0)
			i = ATH_RHIST_SIZE-1;
		else
			i--;
	} while (i != an->an_rx_hist_next);*/
done:
		/*
		 * Return either the average or the last known
		 * value if there is no recent data.
		 */
		return (nsamples ? rssi / nsamples : an->an_rx_hist[i].arh_rssi);
}

int AtherosWIFIInterface::intf_newstate(enum ieee80211_state nstate, int arg)
{
	struct ieee80211_node *ni;
	int error = 0;
	const u_int8_t *bssid;
	u_int32_t rfilt;
	static const HAL_LED_STATE leds[] = {
	    HAL_LED_INIT,	/* IEEE80211_S_INIT */
	    HAL_LED_SCAN,	/* IEEE80211_S_SCAN */
	    HAL_LED_AUTH,	/* IEEE80211_S_AUTH */
	    HAL_LED_ASSOC, 	/* IEEE80211_S_ASSOC */
	    HAL_LED_RUN, 	/* IEEE80211_S_RUN */
	};
	
	IOLog("%s: %s -> %s\n", __func__,
			 ieee80211_state_name[ic_state],
			 ieee80211_state_name[nstate]);
	
	ctlr->setledstate(leds[nstate]);	/* set LED */
	
	if (nstate == IEEE80211_S_INIT) {
		ctlr->disable_ints( (HAL_INT)(HAL_INT_SWBA | HAL_INT_BMISS));
		ctlr->stop_recal_timer();
		ctlr->stop_scan_timer();
		return super::intf_newstate(nstate, arg);
	}
	ni = ic_bss;
	error = ctlr->ath_chan_set(ni->ni_chan);
	if (error != 0)
		goto bad;
	rfilt = ctlr->ath_calcrxfilter();
	if (nstate == IEEE80211_S_SCAN) {
		ctlr->start_scan_timer();
		bssid = ctlr->broadcastaddr();
	} else {
		ctlr->stop_scan_timer();
		bssid = ni->ni_bssid;
	}
	ctlr->setrxfilter(rfilt);
	IOLog("%s: RX filter 0x%x bssid %s\n",
			 __func__, rfilt, ether_sprintf((void*)bssid));
	
	if (nstate == IEEE80211_S_RUN && ic_opmode == IEEE80211_M_STA)
		ctlr->setassocid((u_int8_t*)bssid, ni->ni_associd);
	else
		ctlr->setassocid((u_int8_t*)bssid, 0);
	
	// JMB FIXME
	/*
	if (ic->ic_flags & IEEE80211_F_WEPON) {
		for (i = 0; i < IEEE80211_WEP_NKID; i++)
			if (ath_hal_keyisvalid(ah, i))
				ath_hal_keysetmac(ah, i, bssid);
	}*/
	
	if (nstate == IEEE80211_S_RUN) {
		IOLog("%s(RUN): ic_flags=0x%08x iv=%d bssid=%s "
				 "capinfo=0x%04x chan=%d\n"
				 , __func__
				 , ic_flags
				 , ni->ni_intval
				 , ether_sprintf(ni->ni_bssid)
				 , ni->ni_capinfo
				 , ieee80211_chan2ieee(ni->ni_chan));
		
		/*
		 * Allocate and setup the beacon frame for AP or adhoc mode.
		 */
		// JMB FIXME
		/*if (ic_opmode == IEEE80211_M_HOSTAP ||
		    ic_opmode == IEEE80211_M_IBSS) {
			error = ath_beacon_alloc(sc, ni);
			if (error != 0)
				goto bad;
		}*/
		
		/*
		 * Configure the beacon and sleep timers.
		 */
		// JMB FIXME
		// ath_beacon_config(sc);
		
		/* start periodic recalibration timer */
		ctlr->start_recal_timer();
	} else {
		ctlr->disable_ints( (HAL_INT)(HAL_INT_SWBA | HAL_INT_BMISS));
		ctlr->stop_recal_timer();
	}
	/*
	 * Reset the rate control state.
	 */
	ctlr->ath_rate_ctl_reset(nstate);
	/*
	 * Invoke the parent method to complete the work.
	 */
	return super::intf_newstate(nstate, arg);
	
bad:
	ctlr->stop_scan_timer();
	ctlr->stop_recal_timer();
	/* NB: do not invoke the parent */
	return error;
}

void AtherosWIFIInterface::sendMgmtFrame(struct mbuf *m, struct ieee80211_node *ni)
{
	ctlr->sendMgmtFrame(m, ni);
}

void AtherosWIFIInterface::sendDataFrame(struct mbuf *m, struct ieee80211_node *ni)
{
	ctlr->sendDataFrame(m, ni);
}