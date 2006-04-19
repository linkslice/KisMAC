#include "AtherosWifi.h"

extern "C" {
#include <sys/mbuf.h>
}

#include <IOKit/pccard/IOPCCard.h>
#include "AtherosWifiInterface.h"

OSDefineMetaClassAndStructors( AtherosWIFIController, IO80211Controller )

#define super IO80211Controller
#define ROUNDUP(x, inc) ( (x) + (inc) + ((x)%(inc)) )

#define ATH_NUM_RX_DESCS 40
#define ATH_NUM_TX_DESCS 60
#define recalibrationTimeout 30000 //in milli seconds
#define scanTimeout 250 //in milli seconds
#define watchdogTimeout 1 //in milli seconds

AtherosWIFIController::opmode_settings AtherosWIFIController::_opmodeSettings[] = {
    { _operationModeStation, HAL_M_STA,     (HAL_RX_FILTER)(HAL_RX_FILTER_UCAST | HAL_RX_FILTER_BCAST | HAL_RX_FILTER_BEACON) },
    { _operationModeMonitor, HAL_M_MONITOR, (HAL_RX_FILTER)(HAL_RX_FILTER_UCAST | HAL_RX_FILTER_BCAST | HAL_RX_FILTER_MCAST | HAL_RX_FILTER_PROM | HAL_RX_FILTER_PROBEREQ) },
    { _operationModeInvalid, HAL_M_STA,     (HAL_RX_FILTER)(0) }
};

AtherosWIFIController::int_handler AtherosWIFIController::interruptHandlers[] =
{
    { HAL_INT_RX, &AtherosWIFIController::rxInterruptOccurred },
    { HAL_INT_TX, &AtherosWIFIController::txInterruptOccurred },
	{ HAL_INT_FATAL, &AtherosWIFIController::fatalInterruptOccurred },
    { (HAL_INT) 0, 0 }
};

u_int AtherosWIFIController::ath_chan2flags(struct ieee80211_channel *chan)
{
	static const u_int modeflags[] = {
		0,			/* IEEE80211_MODE_AUTO */
		CHANNEL_A,		/* IEEE80211_MODE_11A */
		CHANNEL_B,		/* IEEE80211_MODE_11B */
		CHANNEL_PUREG,		/* IEEE80211_MODE_11G */
		CHANNEL_T		/* IEEE80211_MODE_TURBO */
	};
	return modeflags[netif->ieee80211_chan2mode(chan)];
}

void AtherosWIFIController::send_test_packet()
{
#if 0
    void *data;
    struct ieee80211_frame *fr;
    u_int16_t *base;
    DescBuff *db;
    static unsigned char ap_mac[] = { 0x00, 0x03, 0x93, 0xEB, 0xE7, 0x0F };
    static unsigned char pkt[] = {
        0, 0, // fc
        0, 0, // dur
        0, 0, 0, 0, 0, 0, // Addr 1
        0, 0, 0, 0, 0, 0, // Addr 2
        0, 0, 0, 0, 0, 0, // Addr 3
        0, 0, // seq
        0, 0, // auth alg
        0, 0, // auth seq
        0, 0,  // auth status
		0, 0, 0, 0
    };
	
   //if (tx_data)
   //     return;
	
	if (!txListHead)
		return;
    
    fr = (struct ieee80211_frame*)pkt;
    
    // Header
    fr->i_fc[0] = IEEE80211_FC0_VERSION_0 |
        IEEE80211_FC0_TYPE_MGT | IEEE80211_FC0_SUBTYPE_AUTH;
    fr->i_fc[1] = IEEE80211_FC1_DIR_NODS;
    // fr->i_dur is filled in by the tx code
    memcpy(fr->i_addr1, ap_mac, 6);
    memcpy(fr->i_addr2, myAddress.bytes, 6);
    memcpy(fr->i_addr3, ap_mac, 6);
    fr->i_seq[1] = 10;
    
    // Auth data
    base = (u_int16_t*)( &fr->i_seq[1] + 1);
    base[0] = htons(IEEE80211_AUTH_ALG_OPEN);   // Algorithm type
    base[1] = 0;        // Algorithm step
    base[2] = 0;        // status
    
    int tx_data_size = ROUNDUP(sizeof(pkt) + 32, alignment);
    data = IOMallocContiguous(tx_data_size, alignment, NULL);
    memcpy(data, pkt, sizeof(pkt));
    
    const HAL_RATE_TABLE *rt = sc_currates;

    IOLog("Test packet sent (dumping for reference):\n");
    netif->ieee80211_dump_pkt((u_int8_t*)data, sizeof(pkt),
                                  rt->info[0].dot11Rate
                                  & IEEE80211_RATE_VAL, 0);
        
	db = txListHead;
	txListHead = db->desc_next;
	db->desc_next = 0;
	if (!txListHead)
		txListTail = 0;
	
	txPacket(data, sizeof(pkt), db);
#endif
}

void AtherosWIFIController::check_tx_queue()
{
	while (txListHead && (mgmtHead || dataHead) )
	{
		DescBuff *db;
		struct mbuf_queue *q;
		
		if (mgmtHead)
		{
			q = mgmtHead;
			mgmtHead = q->next;
			if (!mgmtHead)
				mgmtTail = 0;
		}
		else if (dataHead)
		{
			q = dataHead;
			dataHead = q->next;
			if (!dataHead)
				dataHead = 0;
		}
		else
			break;
		
		db = txListHead;
		txListHead = db->desc_next;
		db->desc_next = 0;
		if (!txListHead)
			txListTail = 0;
	
		txPacket(q->m, q->ni, db);
		IOFree(q, sizeof(*q));
	}
}

void AtherosWIFIController::stop_recal_timer()
{
	if (recalibrationTimer) {
		recalibrationTimer->cancelTimeout();
		recalibrationTimer->disable();
	}
}

void AtherosWIFIController::stop_scan_timer()
{
	if (scanTimer) {
		scanTimer->cancelTimeout();
		scanTimer->disable();
	}
}

void AtherosWIFIController::stop_watchdog_timer()
{
	if (watchdogTimer) {
		watchdogTimer->cancelTimeout();
		watchdogTimer->disable();
	}
}

void AtherosWIFIController::start_recal_timer()
{
	if (recalibrationTimer) {
		recalibrationTimer->setTimeout(recalibrationTimeout);
		recalibrationTimer->enable();
	}
}

void AtherosWIFIController::start_scan_timer()
{
	if (scanTimer) {
		scanTimer->setTimeout(scanTimeout);
		scanTimer->enable();
	}
}

void AtherosWIFIController::start_watchdog_timer()
{
	if (watchdogTimer) {
		watchdogTimer->setTimeout(watchdogTimeout);
		watchdogTimer->enable();
	}
}

bool AtherosWIFIController::start(IOService * provider)
{
    bool ret     = false;
    bool started = false;
    
    netif = 0;
    mmap = 0;
    hal = 0;
    chans = 0;
    workLoop = 0;
    interruptSrc = 0;
    rxDescs = 0;
    pciNub = 0;
    _cardGone = false;
    _activeChannelIndex = 0;
    _activeOpMode = 0;
    _enabled = false;
    _activeIEEEMode = _modulationMode80211b;
    _packetQueue = NULL;
    
	mgmtHead = mgmtTail = dataHead = dataTail = 0;
	
    do {
        if ( super::start(provider) == false )
            break;
        
        started = true;
        
        pciNub = OSDynamicCast(IOPCIDevice, provider);
        if ( pciNub == 0 )
            break;
        pciNub->retain();
        if ( pciNub->open(this) == false )
            break;
        
        if ( pciNub->requestPowerDomainState(
                            /* power flags */ kIOPMPowerOn,
                            /* connection  */ (IOPowerConnection *) getParentEntry(gIOPowerPlane),
                            /* spec        */ IOPMLowestState ) != IOPMNoErr )
        {
            break;
        }

        const char *card_name = ath_hal_probe(
                                pciNub->configRead16(kIOPCIConfigVendorID),
                                pciNub->configRead16(kIOPCIConfigDeviceID));
        if (card_name == 0)
            break;
            
		strcpy(vendorStr, "Atheros");
		strcpy(modelStr, card_name);
        alignment = pciNub->configRead8(kIOPCIConfigCacheLineSize) * 8;
        
        rxBuffSize = ROUNDUP(IEEE80211_MAX_LEN, alignment);

        mmap = pciNub->mapDeviceMemoryWithRegister( kIOPCIConfigBaseAddress0 );
        if ( mmap == 0 ) break;
        
        
        HAL_STATUS status;
        int att_cnt = 0;
        
        do
        {
            hal = ath_hal_attach(pciNub->configRead16(kIOPCIConfigDeviceID),
                                 0, 0, (void*)mmap->getVirtualAddress(),
                                 &status);
            if (hal == 0)
                IODelay(10000);
        } while (hal == 0 && (att_cnt++ < 4) );
        
        if (hal == 0L)
        {
            IOLog("Unable to attach to HAL: status %u\n", status);
            hal = 0;
            break;
        }
        
        if (hal->ah_abi != HAL_ABI_VERSION)
        {
            IOLog("HAL ABI mismatch; driver version 0x%X, HAL version 0x%X\n",
                  HAL_ABI_VERSION, hal->ah_abi);
            break;
        }
		
        setledstate(HAL_LED_INIT);
        (*hal->ah_getMacAddress)(hal, myAddress.bytes);
        ath_regdomain = (*hal->ah_getRegDomain)(hal);
        ath_countrycode = hal->ah_countryCode;
        
        chans = (HAL_CHANNEL*)IOMalloc(IEEE80211_CHAN_MAX *
                                       sizeof(HAL_CHANNEL));
        if (!chans)
        {
            IOLog("Failed to allocate channel memory\n");
            break;
        }
        
        if (!ath_hal_init_channels(hal, chans, IEEE80211_CHAN_MAX,
                                &nchans, CTRY_DEFAULT, HAL_MODE_ALL, AH_TRUE))
        {
            IOLog("Unable to collect channel list from HAL\n");
            break;
        }
        
        bool hasA = false, hasB = false, hasG = false, hasT = false;
        for(u_int i = 0; i < nchans; ++i)
        {
            HAL_CHANNEL *c = &chans[i];
            // int ix = ath_hal_mhz2ieee(c->channel, c->channelFlags);
            if ( (c->channelFlags & CHANNEL_A) == CHANNEL_A)
                hasA = true;
            if ( (c->channelFlags & CHANNEL_B) == CHANNEL_B)
                hasB = true;
            if ( (c->channelFlags & CHANNEL_G) == CHANNEL_G)
                hasG = true;
            if ( (c->channelFlags & CHANNEL_T) == CHANNEL_T)
                hasT = true;
        }
        
        //by default prefer g mode...
		if (hasB) {
			_activeIEEEMode = _modulationMode80211b;
		} else
        if (hasG) {
            _activeIEEEMode = _modulationMode80211g;
        } else if (hasA) {
            _activeIEEEMode = _modulationMode80211a;
        }
                
        IOWorkLoop * myWorkLoop = (IOWorkLoop *) getWorkLoop();
        if (!myWorkLoop)
            break;
                
        interruptSrc = IOInterruptEventSource::interruptEventSource(
                                this,
                                &AtherosWIFIController::interruptOccurredWrapper,
                                provider);
        
        if (!interruptSrc ||
            (myWorkLoop->addEventSource(interruptSrc) != kIOReturnSuccess))
            break;
        interruptSrc->enable();
        
        recalibrationTimer = IOTimerEventSource::timerEventSource(this, &AtherosWIFIController::recalibrationWrapper);
        if (!recalibrationTimer) {
            IOLog("Failed to create recalibration timer event source.");
            break;
        }

        if (myWorkLoop->addEventSource(recalibrationTimer) != kIOReturnSuccess) {
            IOLog("Failed to register recalibration timer event source.");
            break;
        }
		
        scanTimer = IOTimerEventSource::timerEventSource(this, &AtherosWIFIController::scanWrapper);
        if (!scanTimer) {
            IOLog("Failed to create scanner timer event source.");
            break;
        }
		
        if (myWorkLoop->addEventSource(scanTimer) != kIOReturnSuccess) {
            IOLog("Failed to register scanner timer event source.");
            break;
        }
		
        watchdogTimer = IOTimerEventSource::timerEventSource(this, &AtherosWIFIController::watchdogWrapper);
        if (!watchdogTimer) {
            IOLog("Failed to create scanner timer event source.");
            break;
        }		
        if (myWorkLoop->addEventSource(watchdogTimer) != kIOReturnSuccess) {
            IOLog("Failed to register watchdog timer event source.");
            break;
        }
		
        txhalq = (*hal->ah_setupTxQueue)(hal,
                                HAL_TX_QUEUE_DATA,
                                      AH_TRUE /* enable interrupts */);
        if (txhalq == (u_int) -1)
        {
            IOLog("unable to setup a data xmit queue!\n");
            break;
        }
        
        ret = true;
    } while (false);
    
    pciNub->close(this);

   do
    {
        if (ret == false) break;
        ret = false;
        
        if (attachInterface((IONetworkInterface**) &netif, false) == false)
            break;
        
        for(u_int i = 0; i < nchans; ++i)
        {
            HAL_CHANNEL *c = &chans[i];
            int ix = ath_hal_mhz2ieee(c->channel, c->channelFlags);
            if (netif->getChannelFreq(ix) == 0)
            {
                netif->setChannelFreq(ix, c->channel);
                netif->setChannelFlags(ix, c->channelFlags);
            }
            else
                netif->setChannelFlags(ix, netif->getChannelFlags(ix)
                                       | c->channelFlags );
        }
        
        ath_rate_setup(IEEE80211_MODE_11A);
        ath_rate_setup(IEEE80211_MODE_11B);
        ath_rate_setup(IEEE80211_MODE_11G);
        ath_rate_setup(IEEE80211_MODE_TURBO);
        netif->init80211Wifi();
		netif->ieee80211_media_init();
        netif->registerService();
        registerService();
		setcurmode(IEEE80211_MODE_11B);
		
        IOLog("Network Interface Ready\n");
        ret = true;
    } while (false);
    
    if (started && !ret)
    {
        if (chans)
            IOFree(chans, IEEE80211_CHAN_MAX * sizeof(HAL_CHANNEL));
        chans = 0;
        
        if ( interruptSrc && workLoop )
            workLoop->removeEventSource( interruptSrc );
        
        if (interruptSrc)
            interruptSrc->release();
        interruptSrc = 0;

        if (hal)
            (*hal->ah_detach)(hal);
        hal = 0;
        if (mmap)
            mmap->release();
        mmap = 0;
        
        if (workLoop)
            workLoop->release();
        workLoop = 0;
        
        super::stop(provider);
    }
return ret;
}

int AtherosWIFIController::ath_rate_setup(u_int mode)
{
    const HAL_RATE_TABLE *rt;
    struct ieee80211_rateset rs;
    int i, maxrates;
    
    switch (mode) {
        case IEEE80211_MODE_11A:
            sc_rates[mode] = (*hal->ah_getRateTable)(hal, HAL_MODE_11A);
            break;
        case IEEE80211_MODE_11B:
            sc_rates[mode] = (*hal->ah_getRateTable)(hal, HAL_MODE_11B);
            break;
        case IEEE80211_MODE_11G:
            sc_rates[mode] = (*hal->ah_getRateTable)(hal, HAL_MODE_11G);
            break;
        case IEEE80211_MODE_TURBO:
            sc_rates[mode] = (*hal->ah_getRateTable)(hal, HAL_MODE_TURBO);
            break;
        default:
            IOLog("invalid mode %u\n",mode);
            return 0;
    }
    rt = sc_rates[mode];
    if (rt == NULL)
        return 0;
    
    if (rt->rateCount > IEEE80211_RATE_MAXSIZE) {
        IOLog("rate table too small (%u > %u)\n",
                 rt->rateCount, IEEE80211_RATE_MAXSIZE);
        maxrates = IEEE80211_RATE_MAXSIZE;
    } else
        maxrates = rt->rateCount;

    for (i = 0; i < maxrates; i++)
        rs.rs_rates[i] = rt->info[i].dot11Rate;
    rs.rs_nrates = maxrates;
	
    netif->set_supported_rates(&rs, mode);
    
    return 1;
}

void AtherosWIFIController::free()
{
#define REL(x) do { if(x) { (x)->release(); (x) = 0; } } while(0)
    
    if (recalibrationTimer) {
        recalibrationTimer->cancelTimeout();
        recalibrationTimer->disable();
        workLoop->removeEventSource( recalibrationTimer );
    }
    
    REL(recalibrationTimer);
    
    if (scanTimer) {
        scanTimer->cancelTimeout();
        scanTimer->disable();
        workLoop->removeEventSource( scanTimer );
    }
    REL(scanTimer);
	
    if (watchdogTimer) {
        watchdogTimer->cancelTimeout();
        watchdogTimer->disable();
        workLoop->removeEventSource( watchdogTimer );
    }
    REL(watchdogTimer);
	
    if ( interruptSrc && workLoop )
        workLoop->removeEventSource( interruptSrc );
    
    REL(interruptSrc);
    
    if (hal)
    {
        (*hal->ah_detach)(hal);
        hal = 0;
    }
    
    REL(mmap);
    REL(workLoop);
    REL(netif);

    if (chans)
        IOFree(chans, IEEE80211_CHAN_MAX * sizeof(HAL_CHANNEL));
    chans = 0;
    
    REL(pciNub);
    
super::free();
#undef REL
}



#define	IEEE80211_ACK_SIZE	(2+2+IEEE80211_ADDR_LEN+4)

// int AtherosWIFIController::txPacket(void *m0, int pktlen, DescBuff *db)
int AtherosWIFIController::txPacket(struct mbuf *m0, struct ieee80211_node *ni, DescBuff *db)
{
    // struct ieee80211com *ic = &sc->sc_ic;
    int i, iswep, hdrlen;
    int pktlen;
    u_int8_t rix, cix, txrate, ctsrate;
    struct ath_desc *ds;
    struct ieee80211_frame *wh;
    u_int32_t iv = 0;
    u_int8_t *ivp;
    u_int8_t hdrbuf[sizeof(struct ieee80211_frame) +
        IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN];
    u_int subtype, flags, ctsduration, antenna;
    HAL_PKT_TYPE atype;
    const HAL_RATE_TABLE *rt;
    HAL_BOOL shortPreamble;
    struct ath_node *an;
    
    wh = mtod(m0, struct ieee80211_frame *);
    // wh = (struct ieee80211_frame*) m0;
    iswep = wh->i_fc[1] & IEEE80211_FC1_WEP;
    hdrlen = sizeof(struct ieee80211_frame);
    pktlen = m0->m_pkthdr.len;
    
	ds = &db->desc;
    
    if (iswep)
    {
        memcpy(hdrbuf, mtod(m0, caddr_t), hdrlen);
        m_adj(m0, hdrlen);
        M_PREPEND(m0, sizeof(hdrbuf), M_DONTWAIT);
        if (m0 == NULL)
        {
            // sc->sc_stats.ast_tx_nombuf++;
            return ENOMEM;
        }
        ivp = hdrbuf + hdrlen;
        wh = mtod(m0, struct ieee80211_frame *);
        /*
         * XXX
         * IV must not duplicate during the lifetime of the key.
         * But no mechanism to renew keys is defined in IEEE 802.11
         * WEP.  And IV may be duplicated between other stations
         * because of the session key itself is shared.
         * So we use pseudo random IV for now, though it is not the
         * right way.
         */
        
        iv = ic_iv;
        
        /*
         * Skip 'bad' IVs from Fluhrer/Mantin/Shamir:
         * (B, 255, N) with 3 <= B < 8
         */
        if (iv >= 0x03ff00 && (iv & 0xf8ff00) == 0x00ff00)
            iv += 0x000100;
        ic_iv = iv + 1;
        for (i = 0; i < IEEE80211_WEP_IVLEN; i++)
        {
            ivp[i] = iv;
            iv >>= 8;
        }
        // JMB FIXME XXX ivp[i] = sc->sc_ic.ic_wep_txkey << 6;	/* Key ID and pad */
        memcpy(mtod(m0, caddr_t), hdrbuf, sizeof(hdrbuf));
        /*
         * The ICV length must be included into hdrlen and pktlen.
         */
        hdrlen = sizeof(hdrbuf) + IEEE80211_WEP_CRCLEN;
        pktlen = m0->m_pkthdr.len + IEEE80211_WEP_CRCLEN;
    }
    pktlen += IEEE80211_CRC_LEN;
    
    /*
     * Load the DMA map so any coalescing is done.  This
     * also calculates the number of descriptors we need.
     */
    // JMB MACOS code needed

    // IOLog("ath_tx_start: m %p len %u\n", m0, pktlen);
    db->mbuf = m0;
    db->node = ATH_NODE(ni);			/* NB: held reference */
    
    /* setup descriptors */
	rt = sc_currates;
    // JMB MACOS port: KASSERT(rt != NULL, ("no rate table, mode %u", sc->sc_curmode));
    
    /*
    * Calculate Atheros packet type from IEEE80211 packet header
    * and setup for rate calculations.
    */
    atype = HAL_PKT_TYPE_NORMAL;			/* default */
    switch (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK)
    {
        case IEEE80211_FC0_TYPE_MGT:
            subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
            if (subtype == IEEE80211_FC0_SUBTYPE_BEACON)
                atype = HAL_PKT_TYPE_BEACON;
            else if (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP)
                atype = HAL_PKT_TYPE_PROBE_RESP;
            else if (subtype == IEEE80211_FC0_SUBTYPE_ATIM)
                atype = HAL_PKT_TYPE_ATIM;
            rix = 0;			/* XXX lowest rate */
            break;
        case IEEE80211_FC0_TYPE_CTL:
            subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
            if (subtype == IEEE80211_FC0_SUBTYPE_PS_POLL)
                atype = HAL_PKT_TYPE_PSPOLL;
            rix = 0;			/* XXX lowest rate */
            break;
        default:
            rix = sc_rixmap[ni->ni_rates.rs_rates[ni->ni_txrate] &
                IEEE80211_RATE_VAL];
            if (rix == 0xff)
            {
                IOLog("bogus xmit rate 0x%x\n",
                        ni->ni_rates.rs_rates[ni->ni_txrate]);
                // sc->sc_stats.ast_tx_badrate++;
                m_freem(m0);
                return EIO;
            }
            break;
    }
    /*
    * NB: the 802.11 layer marks whether or not we should
    * use short preamble based on the current mode and
    * negotiated parameters.
    */
    
    if ((netif->ic_flags & IEEE80211_F_SHPREAMBLE) &&
        (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE)) {
        txrate = rt->info[rix].rateCode | rt->info[rix].shortPreamble;
        shortPreamble = AH_TRUE;
        // sc->sc_stats.ast_tx_shortpre++;
    }
	else
	{
        txrate = rt->info[rix].rateCode;
        shortPreamble = AH_FALSE;
    }
    
    /*
    * Calculate miscellaneous flags.
    */
    flags = HAL_TXDESC_CLRDMASK;		/* XXX needed for wep errors */
    if (IEEE80211_IS_MULTICAST(wh->i_addr1))
    {
        flags |= HAL_TXDESC_NOACK;	/* no ack on broad/multicast */
        // JMB sc->sc_stats.ast_tx_noack++;
    }
    // JMB else if (pktlen > ic->ic_rtsthreshold)
    // JMB {
    // JMB    flags |= HAL_TXDESC_RTSENA;	/* RTS based on frame length */
    // JMB    // JMB sc->sc_stats.ast_tx_rts++;
    // JMB }
    
	netif->ieee80211_dump_pkt((u_int8_t*)wh, pktlen,
								rt->info[rix].dot11Rate & IEEE80211_RATE_VAL,
										  0);

    /*
    * Calculate duration.  This logically belongs in the 802.11
    * layer but it lacks sufficient information to calculate it.
    */
    if ((flags & HAL_TXDESC_NOACK) == 0 &&
        (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) != IEEE80211_FC0_TYPE_CTL) {
        u_int16_t dur;
        /*
        * XXX not right with fragmentation.
        */
        dur = ath_hal_computetxtime(hal, rt, IEEE80211_ACK_SIZE,
                                    rix, shortPreamble);
        *((u_int16_t*) wh->i_dur) = htons(dur);
    }
    
    /*
    * Calculate RTS/CTS rate and duration if needed.
    */
    ctsduration = 0;
    if (flags & (HAL_TXDESC_RTSENA|HAL_TXDESC_CTSENA))
	{
        /*
        * CTS transmit rate is derived from the transmit rate
        * by looking in the h/w rate table.  We must also factor
        * in whether or not a short preamble is to be used.
        */
        cix = rt->info[rix].controlRate;
        ctsrate = rt->info[cix].rateCode;
        if (shortPreamble)
            ctsrate |= rt->info[cix].shortPreamble;
        /*
        * Compute the transmit duration based on the size
        * of an ACK frame.  We call into the HAL to do the
        * computation since it depends on the characteristics
        * of the actual PHY being used.
        */
        if (flags & HAL_TXDESC_RTSENA)
		{	/* SIFS + CTS */
            ctsduration += ath_hal_computetxtime(hal,
                                                rt, IEEE80211_ACK_SIZE, cix, shortPreamble);
        }
		/* SIFS + data */
		ctsduration += ath_hal_computetxtime(hal,
                                        rt, pktlen, rix, shortPreamble);
		if ((flags & HAL_TXDESC_NOACK) == 0)
		{	/* SIFS + ACK */
			ctsduration += ath_hal_computetxtime(hal,
                                        rt, IEEE80211_ACK_SIZE, cix, shortPreamble);
		}
    }
	else
		ctsrate = 0;
    
    /*
    * For now use the antenna on which the last good
    * frame was received on.  We assume this field is
    * initialized to 0 which gives us ``auto'' or the
    * ``default'' antenna.
    */
    an = (struct ath_node *) ni;
    if (an->an_tx_antenna)
		antenna = an->an_tx_antenna;
    else
		antenna = an->an_rx_hist[an->an_rx_hist_next].arh_antenna;
    antenna = 0;
    
    /*
    * Formulate first tx descriptor with tx controls.
    */
    /* XXX check return value? */
	if (!
    (*hal->ah_setupTxDesc)(hal, ds
                        , pktlen		/* packet length */
                        , hdrlen		/* header length */
                        , atype			/* Atheros packet type */
                        , 60			/* txpower XXX */
                        , txrate, 1+10		/* series 0 rate/tries */
                        // JMB , iswep ? sc->sc_ic.ic_wep_txkey : HAL_TXKEYIX_INVALID
                        , HAL_TXKEYIX_INVALID
                        , antenna		/* antenna mode */
                        , flags			/* flags */
                        , ctsrate		/* rts/cts rate */
                        , ctsduration		/* rts/cts duration */
                        ))
		IOLog("ah_setupTxDesc returned 0\n");

    /*
    * Fillin the remainder of the descriptor info.
    */
	struct IOPhysicalSegment phy_vec[ATH_TX_SCATTER_SIZE];
	int cnt = (int)db->tx_mbdesc->getPhysicalSegmentsWithCoalesce(m0, phy_vec, ATH_TX_SCATTER_SIZE);

	for(i = 0; i < cnt; i++)
	{
		db->tx_desc[i]->ds_data = phy_vec[i].location;
		if (i == (cnt - 1) )
			db->tx_desc[i]->ds_link = 0;
		else
			db->tx_desc[i]->ds_link = db->tx_mdesc[i+1]->getPhysicalAddress();
			
		if (!(*hal->ah_fillTxDesc)(hal, db->tx_desc[i]
                      , phy_vec[i].length	/* segment length */
					  , i==0?AH_TRUE:AH_FALSE		/* first segment */
					  , i==(cnt-1)?AH_TRUE:AH_FALSE	/* last segment */
                      ))
		IOLog("ah_fillTxDesc returned 0\n");
        /* IOLog("ath_tx_start: %d: %08x %08x %08x %08x %08x %08x\n",
			  i, ds->ds_link, ds->ds_data, ds->ds_ctl0, ds->ds_ctl1,
			  ds->ds_hw[0], ds->ds_hw[1]); */
	}
	db->tx_desc_cnt = cnt;

    /*
    * Insert the frame on the outbound list and
    * pass it on to the hardware.
    */
	if (!(*hal->ah_setTxDP)(hal, txhalq, db->tx_mdesc[0]->getPhysicalAddress()))
		IOLog("ah_setTxDP returned 0\n");
		
    
    if (!(*hal->ah_startTxDma)(hal, txhalq))
		IOLog("ah_startTxDma returned 0\n");

	db->desc_next = 0;
	activeTxListTail = db;
	if (!activeTxListHead)
		activeTxListHead = db;

	// IOLog("Transmit started!!\n");
	
	/*IOLog("TX Desc:\n");
	IOLog("ds_link: 0x%08X\n", ds->ds_link);
	IOLog("ds_data: 0x%08X\n", ds->ds_data);
	IOLog("ds_ctl0: 0x%08X\n", ds->ds_ctl0);
	IOLog("ds_ctl1: 0x%08X\n", ds->ds_ctl1);
	for(i = 0; i < 4; i++)
		IOLog("ds_hw[%d]: 0x%08X\n", i, ds->ds_hw[i]);
	IOLog("\n");

	IOLog("State Dump:\n");
	(*hal->ah_dumpState)(hal);
	IOLog("\n");*/

    return 0;
}

void AtherosWIFIController::txInterruptOccurred(IOInterruptEventSource * src, int count, HAL_INT mask)
{
    HAL_STATUS status;
    struct ath_desc *ds;
    DescBuff *db;
	
	// IOLog("TX Interrupt!!\n");

	for(;;)
	{
		if (!activeTxListHead)
			break;
		/* only the last descriptor is needed */
		db = activeTxListHead;
		ds = db->tx_desc[db->tx_desc_cnt - 1];
		status = (*hal->ah_procTxDesc)(hal, ds);
		if (status == HAL_EINPROGRESS)
		{
			break;
		}
		activeTxListHead = db->desc_next;
		if (!activeTxListHead)
			activeTxListTail = 0;
		
		if (ds->ds_txstat.ts_status == 0)
		{
			// IOLog("TX OK\n");
		}
		else
		{
			IOLog("TX Error!! Status: %X\n", ds->ds_txstat.ts_status);
		}
		
		ieee80211_unref_node((struct ieee80211_node**)&db->node);
		db->initTx(this);
	}
	
	check_tx_queue();
}

void AtherosWIFIController::stop(IOService *provider)
{
    this->terminate(kIOServiceRequired);

super::stop(provider);
}

IOReturn AtherosWIFIController::enable(IONetworkInterface * nif)
{
    IOReturn ret;
	struct ieee80211_node *ni;
	enum ieee80211_phymode mode;
    
    if (nif != netif)
        return kIOReturnError;
    
    pciNub->open(this);
    
    HAL_STATUS status = HAL_OK;
    
    if ((ret = allocRxResources()) != kIOReturnSuccess)
    {
        pciNub->close(this);
        return ret;
    }
    if ((ret = allocTxResources()) != kIOReturnSuccess)
    {
        pciNub->close(this);
        return ret;
    }
    
    intr_mask = (HAL_INT)0;
    (*hal->ah_setInterrupts)(hal, intr_mask);
    if (!(*hal->ah_reset)(hal, (HAL_OPMODE)netif->ic_opmode, &chans[0], AH_TRUE, &status))
	{
		IOLog("Reset failed status %d\n", status);
	}
    
    enableRx();
    enableTx();
    
	 netif->ic_state = IEEE80211_S_INIT;
	ni = netif->ic_bss;
	ni->ni_chan = netif->ic_ibss_chan;
	mode = netif->ieee80211_chan2mode(ni->ni_chan);
	
	if (mode != sc_curmode)
		setcurmode(mode);
	if (netif->ic_opmode != IEEE80211_M_MONITOR)
		netif->ieee80211_new_state(IEEE80211_S_SCAN, -1);
	else
		netif->ieee80211_new_state(IEEE80211_S_RUN, -1);

	start_watchdog_timer();
    _enabled = true;
    
    IOLog("AtherosWIFIController Enabled!\n");
    send_test_packet();
    return kIOReturnSuccess;
}

IOReturn AtherosWIFIController::disable(IONetworkInterface * nif)
{
    HAL_STATUS status;
    
    if (nif != netif)
        return kIOReturnError;

    _enabled = false;
    
    (*hal->ah_setInterrupts)(hal, (HAL_INT)0);
	netif->ieee80211_new_state(IEEE80211_S_INIT, -1);

    disableTx();
    disableRx();
	drainTxq();
        
    freeRxResources();
    // freeTxResources();
    pciNub->close(this);
    
    IOLog("AtherosWIFIController Disabled!!\n");
    return kIOReturnSuccess;
}

IOReturn AtherosWIFIController::message(UInt32 type, IOService * provider, void * argument) {
    if (type == kIOPCCardCSEventMessage) {
        switch ((unsigned int)argument) {
	    case CS_EVENT_CARD_REMOVAL:
		IOLog("PC Card was removed.\n");
                
                _cardGone = true; //make sure nobody does something stupid
                
		// Message our clients and tell them we're no longer available...
		super::message(kIOMessageServiceIsTerminated, this, 0);
                this->terminate(kIOServiceRequired); // we're gone, lets save memory.
		break;
            case CS_EVENT_CARD_INSERTION:
                //TODO Handle this              
		break;
	    default:
		IOLog("PC Card generated untrapped message: %ud\n", (unsigned int)argument);
		break;
        }
    }

    return super::message(type, provider, argument);
}

IOReturn AtherosWIFIController::getHardwareAddress(IOEthernetAddress * addrP)
{
    bcopy(&myAddress, addrP, sizeof(*addrP));
    return kIOReturnSuccess;
}

bool AtherosWIFIController::createWorkLoop()
{
    workLoop = IOWorkLoop::workLoop();
    
    return ( workLoop != 0 );
}

IOWorkLoop * AtherosWIFIController::getWorkLoop() const
{
    return workLoop;
}

void AtherosWIFIController::rxInterruptOccurred(IOInterruptEventSource * src, int count, HAL_INT mask)
{
    HAL_STATUS status;
    struct mbuf *mb = 0;
    DescBuff *curr;
	struct ieee80211_frame *wh, whbuf;
	struct ieee80211_node *ni;
	// struct ath_node *an;
	struct ath_desc *ds;
	
    do
    {
        curr = rxListHead;
        
        if (curr->desc.ds_link == curr->mdesc->getPhysicalAddress() ) {
            /* NB: never process the self-linked entry at the end */
            break;
        }
        
        if (!curr->mbuf) {
            IOLog("no mbuf present!!!!");
            break;
        }
        
        /* XXX sync descriptor memory */
        /*
         * Must provide the virtual address of the current
         * descriptor, the physical address, and the virtual
         * address of the next descriptor in the h/w chain.
         * This allows the HAL to look ahead to see if the
         * hardware is done with a descriptor by checking the
         * done bit in the following descriptor and the address
         * of the current descriptor the DMA engine is working
         * on.  All this is necessary because of our use of
         * a self-linked list to avoid rx overruns.
         */
        status = (*hal->ah_procRxDesc)(hal, &curr->desc, curr->mdesc->getPhysicalAddress(),
                                    &curr->desc_next->desc);
        
        // IOLog("Rx Interrupt status %d\n", status);
        if (status == HAL_EINPROGRESS)
        {
            // IOLog("Rx Interrupt EINPROGRESS error\n");
            break;
        }
        
        rxListHead = curr->desc_next;
        curr->desc_next = 0;
        if (!rxListHead)
            rxListTail = 0;
		
        mb = curr->mbuf;
        curr->mbuf = 0;
		ds = &curr->desc;

        mb->m_pkthdr.rcvif = netif->getIfnetPtr();
        mb->m_pkthdr.len = mb->m_len = curr->desc.ds_rxstat.rs_datalen;
		
        struct ath_desc *ds = &curr->desc;
        
        // struct ieee80211_frame *fr = (struct ieee80211_frame*)mb->m_data;
		
		if (ds->ds_rxstat.rs_status != 0)
		{
			freePacket(mb);
			goto rx_next;
		}
		
		m_adj(mb, -IEEE80211_CRC_LEN);
        wh = mtod(mb, struct ieee80211_frame *);
        if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
            /*
             * WEP is decrypted by hardware. Clear WEP bit
             * and trim WEP header for ieee80211_input().
             */
            wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
            memcpy(&whbuf, wh, sizeof(whbuf));
            m_adj(mb, IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN);
            wh = mtod(mb, struct ieee80211_frame *);
            memcpy(wh, &whbuf, sizeof(whbuf));
            /*
             * Also trim WEP ICV from the tail.
             */
            m_adj(mb, -IEEE80211_WEP_CRCLEN);
        }
		
        if (netif->ic_opmode != IEEE80211_M_STA) {
            ni = netif->ieee80211_find_node(wh->i_addr2);
            if (ni == NULL)
                ni = ieee80211_ref_node(netif->ic_bss);
        } else
            ni = ieee80211_ref_node(netif->ic_bss);

        /*
         * Record driver-specific state.
         */
        /*an = ATH_NODE(ni);
        if (++(an->an_rx_hist_next) == ATH_RHIST_SIZE)
            an->an_rx_hist_next = 0;
        rh = &an->an_rx_hist[an->an_rx_hist_next];
        rh->arh_ticks = ticks;
        rh->arh_rssi = ds->ds_rxstat.rs_rssi;
        rh->arh_antenna = ds->ds_rxstat.rs_antenna;*/
		
		netif->inputPacket(mb, ni, ds->ds_rxstat.rs_rssi, ds->ds_rxstat.rs_tstamp, mb->m_pkthdr.len, 0, 0);

        //XXX clean this up!
        /* if ((_opmodeSettings[_activeOpMode].mode==_operationModeMonitor) && (_packetQueue!=NULL)) {
            UInt8 packet[2600];
            WLFrame * f;
            
            if ((ds->ds_rxstat.rs_datalen < 2500) && ((ds->ds_rxstat.rs_status & ~HAL_RXERR_DECRYPT) == 0)) {
                bzero(packet, 2600);
                
                f = (WLFrame*)packet;
                f->status  = ds->ds_rxstat.rs_status;
                f->channel = ath_hal_mhz2ieee(chans[_activeChannelIndex].channel, chans[_activeChannelIndex].channelFlags);
                f->signal  = ds->ds_rxstat.rs_rssi;
                f->silence = 0;
                f->len     = ds->ds_rxstat.rs_datalen;
                
                memcpy(&f->ieee, mtod(mb, void*), f->len);
                
                // _packetQueue->enqueue(packet, f->len);
                //if (!_packetQueue->enqueue(packet, f->dataLen))
                //    IOLog("AtherosWifi:: packet queue overflow\n");
                
                //IOLog("AtherosWifi:: got packet! with len %d\n", ds->ds_rxstat.rs_datalen);
            }
        }
		else */
		{
			/*if ( (fr->i_fc[0] & IEEE80211_FC0_TYPE_MASK) != IEEE80211_FC0_TYPE_MGT ||
				 (fr->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) != IEEE80211_FC0_SUBTYPE_BEACON)
			{
				netif->ieee80211_dump_pkt((u_int8_t*)mb->m_data, ds->ds_rxstat.rs_datalen,
                           rt->info[rt->rateCodeToIndex[ds->ds_rxstat.rs_rate]].dot11Rate
                                  & IEEE80211_RATE_VAL,
                           ds->ds_rxstat.rs_rssi);
			}
			else
			{
				static int first_beacon = 1;
				if (first_beacon)
				{
					netif->ieee80211_dump_pkt((u_int8_t*)mb->m_data, ds->ds_rxstat.rs_datalen,
											  rt->info[rt->rateCodeToIndex[ds->ds_rxstat.rs_rate]].dot11Rate
											  & IEEE80211_RATE_VAL,
											  ds->ds_rxstat.rs_rssi);
					IOLog("Output of future becaons is suppressed\n");
				}
				
				first_beacon = 0;
			}*/
			
        }
		if (ni == netif->ic_bss)
			ieee80211_unref_node(&ni);
		else
			netif->ieee80211_free_node(ni);
		
rx_next:
        if (!curr->initRx(this))
            break;

    } while(1);
    
    (*hal->ah_rxMonitor)(hal);
    (*hal->ah_enableReceive)(hal);
}


void AtherosWIFIController::interruptOccurred(IOInterruptEventSource * src, int count)
{
    if (_cardGone) return; //there are phantom interrupts upon card removal

    HAL_INT ints;
    (*hal->ah_getPendingInterrupts)(hal, &ints);
    if ( (ints & HAL_INT_NOCARD) == HAL_INT_NOCARD)
    {
        IOLog("Int no card\n");
        return;
    }
    
    int_handler *hand = interruptHandlers;
    while(hand->mask)
    {
        if (ints & hand->mask)
            (this->*hand->handler)(src, count, hand->mask);
        ints = (HAL_INT)(ints & ~hand->mask);
        hand++;
    }

    if (ints & intr_mask)
        IOLog("Unknown Interrupt 0x%X: %d\n", ints, count);
}

void AtherosWIFIController::interruptOccurredWrapper(OSObject *s, IOInterruptEventSource * src, int count)
{
    AtherosWIFIController *c = OSDynamicCast(AtherosWIFIController, s);
    if (c)
        c->interruptOccurred(src, count);
}

void AtherosWIFIController::disable_ints(HAL_INT ints)
{
	intr_mask = (HAL_INT)(intr_mask & (~ints));
    (*hal->ah_setInterrupts)(hal, intr_mask);
    IODelay(3000);
}

void AtherosWIFIController::enableTx()
{
    // Enable the TX interrupts
    intr_mask = (HAL_INT)(intr_mask | HAL_INT_TXURN | HAL_INT_TX | HAL_INT_TXDESC);
    (*hal->ah_setInterrupts)(hal, intr_mask);
    IODelay(3000);
}

void AtherosWIFIController::disableTx()
{
    // Disable the TX interrupts
    intr_mask = (HAL_INT)(intr_mask & (~(HAL_INT_TXURN | HAL_INT_TX | HAL_INT_TXDESC )));
    (*hal->ah_setInterrupts)(hal, intr_mask);
    IODelay(3000);
}

void AtherosWIFIController::enableRx()
{
    // Enable the RX interrupts
    intr_mask = (HAL_INT)(intr_mask | HAL_INT_RX | HAL_INT_RXEOL | HAL_INT_RXORN |
                          HAL_INT_FATAL | HAL_INT_GLOBAL);
    (*hal->ah_setInterrupts)(hal, intr_mask);
    
    // Enable the RX process
    (*hal->ah_setRxDP)(hal, rxListHead->mdesc->getPhysicalAddress());
    (*hal->ah_enableReceive)(hal);
    (*hal->ah_setRxFilter)(hal, _opmodeSettings[_activeOpMode].filter);
    (*hal->ah_startPcuReceive)(hal);
    IODelay(3000);
}

void AtherosWIFIController::disableRx()
{
    // Disable the RX interrupts
    intr_mask = (HAL_INT)(intr_mask & (~(HAL_INT_RX | HAL_INT_RXEOL | HAL_INT_RXORN) ));
    (*hal->ah_setInterrupts)(hal, intr_mask);

    // Disable RX process
    (*hal->ah_setRxFilter)(hal, 0);
    (*hal->ah_stopPcuReceive)(hal);
    (*hal->ah_stopDmaReceive)(hal);
    (*hal->ah_setRxDP)(hal, 0);
    IODelay(3000);    
}

IOReturn AtherosWIFIController::allocRxResources()
{
    rxDescs = (DescBuff**)IOMallocContiguous(sizeof(DescBuff*)*ATH_NUM_RX_DESCS, alignment, NULL);

    if (!rxDescs)
    {
        IOLog("Unable to allocate Rx Descriptor Array\n");
        return kIOReturnError;
    }
    bzero(rxDescs, sizeof(DescBuff*) * ATH_NUM_RX_DESCS);
    
    for(int i = 0; i < ATH_NUM_RX_DESCS; i++)
    {
        rxDescs[i] = (DescBuff*) IOMallocContiguous(sizeof(DescBuff), alignment, NULL);
        if (!rxDescs[i])
        {
            IOLog("Unable to allocate Rx Descriptor %d\n", i);
            return kIOReturnError;
        }
    }
    
    rxListHead = rxListTail = 0;
    for (int i = 0; i < ATH_NUM_RX_DESCS; i++) {
        rxDescs[i]->ctor(this);
        rxDescs[i]->initRx(this);
    }
    
    return kIOReturnSuccess;
}

IOReturn AtherosWIFIController::allocTxResources()
{
    txDescs = (DescBuff**)IOMalloc(sizeof(DescBuff*)*ATH_NUM_TX_DESCS);
	
    if (!txDescs)
    {
        IOLog("Unable to allocate Tx Descriptor Array\n");
        return kIOReturnError;
    }
    bzero(txDescs, sizeof(DescBuff*) * ATH_NUM_TX_DESCS);
    
    for(int i = 0; i < ATH_NUM_TX_DESCS; i++)
    {
        txDescs[i] = (DescBuff*) IOMallocContiguous(sizeof(DescBuff), alignment, NULL);
        if (!txDescs[i])
        {
            IOLog("Unable to allocate Tx Descriptor %d\n", i);
            return kIOReturnError;
        }
    }
    
    txListHead = txListTail = activeTxListHead = activeTxListTail = 0;
    for (int i = 0; i < ATH_NUM_TX_DESCS; i++) {
        txDescs[i]->ctor(this);
        txDescs[i]->initTx(this);
    }
    
    return kIOReturnSuccess;
}

void AtherosWIFIController::DescBuff::ctor(AtherosWIFIController *cont)
{
	for(int i = 0; i < ATH_TX_SCATTER_SIZE; i++)
	{
		tx_desc[i] = 0;
		tx_mdesc[i] = 0;
	}
    desc_next = 0;
    mbuf = 0;
    mdesc = 0;
    mbdesc = 0;
	tx_mbdesc = 0;
	node = 0;
}

bool AtherosWIFIController::DescBuff::initRx(AtherosWIFIController *cont)
{
    if (!mdesc)
    {
        mdesc = IOMemoryDescriptor::withAddress(&desc,
                                                sizeof(ath_desc), kIODirectionInOut);
    }
    if (!mdesc)
    {
        IOLog("Unable to allocate rx descriptor\n");
        return FALSE;
    }
        
    if (!mbuf)
    {
        mbuf = cont->allocatePacket(cont->rxBuffSize);
        // IOLog("Mbuf Flags: %X\n", mbuf->m_flags);
    }
    
    if (!mbuf)
    {
        IOLog("Unable to allocate rx mbuf\n");
        return FALSE;
    }
    
    mbuf->m_pkthdr.len = mbuf->m_len = mbuf->m_ext.ext_size;
    // IOLog("m_data %p, m_ext.ext_buf %p\n", mbuf->m_data, mbuf->m_ext.ext_buf);

    /*if (!mbdesc)
    {
        mbdesc = IOMemoryDescriptor::withAddress(mbuf->m_data,
                                                 cont->rxBuffSize, kIODirectionInOut);    
    }
    else
    {
        mbdesc->initWithAddress(mbuf->m_data, cont->rxBuffSize, kIODirectionInOut);
    }
    if (!mbdesc)
    {
        IOLog("Unable to allocate rx mbuf cursor\n");
        return FALSE;
    }*/
    
    /* mbuf->m_hdr.mh_next = mbuf->m_hdr.mh_nextpkt = 0;
    mbuf->m_hdr.mh_len = cont->rxBuffSize;
    mbuf->m_hdr.mh_data = &mbuf->M_dat.M_databuf;
    mbuf->m_hdr.mh_flags = 0; */
    
    desc.ds_link = mdesc->getPhysicalAddress();
    desc.ds_data = (UInt32)mcl_to_paddr(mtod(mbuf, char*));//  mbdesc->getPhysicalAddress();
        
    // IOLog("Physical pointers: %X, %X (%p)\n", desc.ds_link, desc.ds_data, (void*)mbdesc->getPhysicalAddress());

    if (AH_FALSE == (*cont->hal->ah_setupRxDesc)(cont->hal, &desc,
                                                 cont->rxBuffSize, 0))
    {
        IOLog("Unable to initialize rx descriptor with HAL\n");
        return FALSE;
    }
    
    if (cont->rxListTail)
    {
        cont->rxListTail->desc.ds_link = mdesc->getPhysicalAddress();
        cont->rxListTail->desc_next = this;
        cont->rxListTail = this;
    }
    else
        cont->rxListHead = cont->rxListTail = this;
    desc_next = 0;
    
    return TRUE;
}

bool AtherosWIFIController::DescBuff::initTx(AtherosWIFIController *cont)
{
	int i;
    if (!mdesc)
    {
        mdesc = IOMemoryDescriptor::withAddress(&desc,
                                                sizeof(ath_desc), kIODirectionInOut);
    }
    if (!mdesc)
    {
        IOLog("Unable to allocate tx descriptor\n");
        return FALSE;
    }
	
    if (!tx_mbdesc)
    {
        tx_mbdesc = IOMbufNaturalMemoryCursor::withSpecification(2048, ATH_TX_SCATTER_SIZE);
    }
    if (!tx_mbdesc)
    {
        IOLog("Unable to allocate tx mbuf cursor\n");
        return FALSE;
    }
    
    desc.ds_link = 0;
    desc.ds_data = 0;
	
	tx_desc[0] = &desc;
	tx_mdesc[0] = mdesc;
	
	for(i = 1; i < ATH_TX_SCATTER_SIZE; i++)
	{
		if (!tx_desc[i])
		{
			tx_desc[i] = (struct ath_desc*)
					IOMallocContiguous(sizeof(struct ath_desc), cont->alignment, NULL);
			if (!tx_desc[i])
			{
				IOLog("Unable to allocate tx scatter descriptor %d\n", i);
				return FALSE;
			}
		}

		if (!tx_mdesc[i])
		{
			tx_mdesc[i] = IOMemoryDescriptor::withAddress(tx_desc[i],
													sizeof(ath_desc), kIODirectionInOut);
			if (!tx_mdesc[i])
			{
				IOLog("Unable to allocate tx scatter memory descriptor %d\n", i);
				return FALSE;
			}
		}
		tx_desc[i]->ds_link = 0;
		tx_desc[i]->ds_data = 0;
	}
		
    if (cont->txListTail)
    {
        cont->txListTail->desc_next = this;
        cont->txListTail = this;
    }
    else
        cont->txListHead = cont->txListTail = this;
    desc_next = 0;
    
    return TRUE;
}

void AtherosWIFIController::DescBuff::dtor(AtherosWIFIController *cont)
{
    desc.ds_link = 0;
    desc.ds_data = 0;
    
    if (mdesc) {
        mdesc->release();
    }
    mdesc = 0;
    
    /*if (mbdesc) {
        mdesc->release();
    }
    mbdesc = 0;*/
	
    if (tx_mbdesc) {
        tx_mbdesc->release();
    }
	tx_mbdesc = 0;
	
    if (mbuf)
        cont->freePacket(mbuf);
    mbuf = 0;
	
	for(int i = 1; i < ATH_TX_SCATTER_SIZE; i++)
	{
		if (tx_desc[i])
			IOFreeContiguous(tx_desc[i], sizeof(struct ath_desc));
		tx_desc[i] = 0;
		if (tx_mdesc[i])
			tx_mdesc[i]->release();
		tx_mdesc[i] = 0;
	}
}

IOReturn AtherosWIFIController::freeRxResources()
{
    
    /*for(int i = 0; i < ATH_NUM_RX_DESCS; i++)
    {
        if (!rxDescs[i])
            continue;
        rxDescs[i]->dtor(this);
        IOFreeContiguous(rxDescs[i], sizeof(DescBuff));
        rxDescs[i] = 0;
    }*/

    rxListHead = rxListTail = 0;
    IOFreeContiguous(rxDescs, sizeof(DescBuff*)*ATH_NUM_RX_DESCS);
    rxDescs = 0;

    //XXX figure out why this crashes and how to avoid it
    //releaseFreePackets();
    
    return kIOReturnSuccess;
}

IOReturn AtherosWIFIController::freeTxResources()
{
    for(int i = 0; i < ATH_NUM_TX_DESCS; i++)
    {
        if (!txDescs[i])
            continue;
        txDescs[i]->dtor(this);
        IOFreeContiguous(txDescs[i], sizeof(DescBuff));
        txDescs[i] = 0;
    }
	
    txListHead = txListTail = activeTxListHead = activeTxListTail = 0;
    IOFree(txDescs, sizeof(DescBuff*)*ATH_NUM_TX_DESCS);
    txDescs = 0;
	
    return kIOReturnSuccess;
}

void AtherosWIFIController::setcurmode(enum ieee80211_phymode mode)
{
	const HAL_RATE_TABLE *rt;
	int i;
	
	memset(sc_rixmap, 0xff, sizeof(sc_rixmap));
	rt = sc_rates[mode];
	// KASSERT(rt != NULL, ("no h/w rate set for phy mode %u", mode));
	assert(rt != NULL);
	for (i = 0; i < rt->rateCount; i++)
		sc_rixmap[rt->info[i].dot11Rate & IEEE80211_RATE_VAL] = i;
	memset(sc_hwmap, 0, sizeof(sc_hwmap));
	for (i = 0; i < 32; i++)
		sc_hwmap[i] = rt->info[rt->rateCodeToIndex[i]].dot11Rate;
	sc_currates = rt;
	sc_curmode = mode;
}

void
AtherosWIFIController::watchdogWrapper(OSObject *owner, IOTimerEventSource *sender) {
    AtherosWIFIController *c = OSDynamicCast(AtherosWIFIController, owner);
    sender->setTimeoutMS(watchdogTimeout);

    if (c)
        c->watchdog(sender);
}

void
AtherosWIFIController::watchdog(IOTimerEventSource *sender) 
{
	if (!_enabled || !_cardGone)
	{
		stop_watchdog_timer();
		return;
	}
#if 0
	if ((ifp->if_flags & IFF_RUNNING) == 0 || sc->sc_invalid)
		return;
	if (sc->sc_tx_timer) {
		if (--sc->sc_tx_timer == 0) {
			if_printf(ifp, "device timeout\n");
#ifdef AR_DEBUG
			if (ath_debug)
				ath_hal_dumpstate(sc->sc_ah);
#endif /* AR_DEBUG */
			ath_init(ifp);		/* XXX ath_reset??? */
			ifp->if_oerrors++;
			sc->sc_stats.ast_watchdog++;
			return;
		}
		ifp->if_timer = 1;
	}
	if (ic->ic_fixed_rate == -1) {
		/*
		 * Run the rate control algorithm if we're not
		 * locked at a fixed rate.
		 */
		if (ic->ic_opmode == IEEE80211_M_STA)
			ath_rate_ctl(sc, ic->ic_bss);
		else
			ieee80211_iterate_nodes(ic, ath_rate_ctl, sc);
	}
#endif
	netif->ieee80211_watchdog();
}

void
AtherosWIFIController::scanWrapper(OSObject *owner, IOTimerEventSource *sender) {
    AtherosWIFIController *c = OSDynamicCast(AtherosWIFIController, owner);
    if (c)
        c->scan(sender);
    
    sender->setTimeoutMS(scanTimeout);
}

void
AtherosWIFIController::scan(IOTimerEventSource *sender) 
{
	if (netif->ic_state == IEEE80211_S_SCAN)
		netif->ieee80211_next_scan();
}

/*
 * Periodically recalibrate the PHY to account
 * for temperature/environment changes.
 */
void
AtherosWIFIController::recalibrationWrapper(OSObject *owner, IOTimerEventSource *sender) {
    AtherosWIFIController *c = OSDynamicCast(AtherosWIFIController, owner);
    if (c)
        c->recalibration(sender);
    
    sender->setTimeoutMS(recalibrationTimeout);
}

void
AtherosWIFIController::recalibration(IOTimerEventSource *sender) 
{
	if (_cardGone) return;
    
    if ((*hal->ah_getRfGain)(hal) == HAL_RFGAIN_NEED_CHANGE) {
            /*
             * Rfgain is out of bounds, reset the chip
             * to load new gain values.
             */
            IOLog("Need to change rfGain\n");
            reset();
    }
    if (!(*hal->ah_perCalibration)(hal, &chans[_activeChannelIndex])) {
            IOLog("calibration of channel %u failed\n", chans[_activeChannelIndex].channel);
    }
}

void AtherosWIFIController::fatalInterruptOccurred(IOInterruptEventSource *src, int count, HAL_INT intr)
{
	IOLog("Fatal interrupt occurred, reseting device.\n");
	reset();
}

IOReturn
AtherosWIFIController::reset() {
    HAL_STATUS status = HAL_OK;
    IOReturn ret = kIOReturnSuccess;
    
    disableTx();                /* stop the transmit side */
    disableRx();		/* stop recv side */
    
    drainTxq();		/* stop xmit side */
    
    /* NB: indicate channel change so we do a full reset */
    if (!(*hal->ah_reset)(hal, (HAL_OPMODE)netif->ic_opmode, &chans[_activeChannelIndex], AH_TRUE, &status)) {
        IOLog("unable to reset hardware; hal status %u\n", status);
        ret = kIOReturnError;
    }
    
    enableRx();
    enableTx();
    
    //XXX needs to be done!
    //if (ic->ic_state == IEEE80211_S_RUN) {
    //        ath_beacon_config(sc);	/* restart beacons */
    //        netif_wake_queue(dev);	/* restart xmit */
    //}
    return ret;
}

#pragma mark -

IOReturn AtherosWIFIController::getFrequency(UInt32* freq) {
    *freq = chans[_activeChannelIndex].channel;
    return kIOReturnSuccess;
}

IOReturn AtherosWIFIController::setFrequency(UInt32 freq) {
    unsigned int i;
    UInt32 flags;
    
    switch(_activeIEEEMode) {
        case _modulationMode80211a:
            flags = CHANNEL_A;
            break;
        case _modulationMode80211b:
            flags = CHANNEL_B;
            break;
        case _modulationMode80211g:
            flags = CHANNEL_G;
            break;
        case _modulationModeAtherosT:
            flags = CHANNEL_T;
            break;
        default:
            IOLog("Warning: Unknown IEEE mode! Defaulting to 802.11b\n");
            flags = CHANNEL_B;
            break;
    }
    
    for (i = 0; i < nchans; i++) {
        if (chans[i].channel == freq && (chans[i].channelFlags & flags) == flags) break;
    }
    if (i == nchans) return kIOReturnUnsupportedMode;
    
    _activeChannelIndex = i;
    
    if (_enabled) reset();
    return kIOReturnSuccess;
}

IOReturn AtherosWIFIController::getOpMode(UInt32* mode) {
    *mode = _opmodeSettings[_activeOpMode].mode;
    return kIOReturnSuccess;
}
IOReturn AtherosWIFIController::setOpMode(UInt32 mode) {
    unsigned int i;
    
    i = 0;
    while (_opmodeSettings[i].mode != (int)mode) {
        if (_opmodeSettings[i].mode == _operationModeInvalid) return kIOReturnUnsupportedMode;
        i++;
    }
    
    _activeOpMode = i;
    if ((mode == _operationModeMonitor) && (_packetQueue == NULL)) {
        _packetQueue = IODataQueue::withCapacity(16384); // allocate 16KB for caching output in monitor mode
    }
    
    IOLog("Setting operation mode to 0x%x\n", _opmodeSettings[_activeOpMode].halmode);
    
    if (_enabled) reset();
    return kIOReturnSuccess;
}
IOReturn AtherosWIFIController::supportedOpModes(UInt32* modes) {
    UInt32 s, i;
    s = i = 0;
    
    while (_opmodeSettings[i].mode != _operationModeInvalid) {
        s |= _opmodeSettings[i].mode;
        i++;
    }

    *modes = s;
    return kIOReturnSuccess;
}

IONetworkInterface * AtherosWIFIController::createInterface()
{
    AtherosWIFIInterface * netif = new AtherosWIFIInterface;
	
    if ( netif && ( netif->init( this ) == false ) )
    {
        netif->release();
        netif = 0;
    }
	netif->ctlr = this;
	netif->ic_phytype = IEEE80211_T_OFDM;
	netif->ic_opmode = IEEE80211_M_STA;
	netif->ic_caps = IEEE80211_C_WEP		/* wep supported */
		| IEEE80211_C_IBSS		/* ibss, nee adhoc, mode */
		| IEEE80211_C_HOSTAP		/* hostap mode */
		| IEEE80211_C_MONITOR		/* monitor mode */
		| IEEE80211_C_SHPREAMBLE	/* short preamble supported */
		| IEEE80211_C_RCVMGT;		/* recv management frames */
	
    return netif;
}

void AtherosWIFIController::setledstate( enum HAL_LED_STATE state)
{
	if (hal)
		(*hal->ah_setLedState)(hal, state);
}

void AtherosWIFIController::purge_node_refs(struct ath_node *ni)
{
	DescBuff *bf = txListHead;
	
	while(bf)
	{
		if (bf->node == ni)
			bf->node = 0;
		bf = bf->desc_next;
	}
}


int AtherosWIFIController::ath_chan_set(struct ieee80211_channel *chan)
{
	IOLog("ath_chan_set: %u (%u MHz) -> %u (%u MHz)\n",
			 netif->ieee80211_chan2ieee(netif->ic_ibss_chan),
			 netif->ic_ibss_chan->ic_freq,
			 netif->ieee80211_chan2ieee(chan), chan->ic_freq);
	
	if (chan != netif->ic_ibss_chan)
	{
		HAL_STATUS status;
		HAL_CHANNEL hchan;
		enum ieee80211_phymode mode;
		
		/*
		 * To switch channels clear any pending DMA operations;
		 * wait long enough for the RX fifo to drain, reset the
		 * hardware at the new frequency, and then re-enable
		 * the relevant bits of the h/w.
		 */
		(*hal->ah_setInterrupts)(hal, (HAL_INT)0);   /* disable interrupts */
		disableTx();
		drainTxq();		/* clear pending tx frames */
		disableRx();		/* turn off frame recv */
		IODelay(3000);
		/*
		 * Convert to a HAL channel description with
		 * the flags constrained to reflect the current
		 * operating mode.
		 */
		hchan.channel = chan->ic_freq;
		hchan.channelFlags = ath_chan2flags(chan);
		if (!(*hal->ah_reset)(hal, (HAL_OPMODE)netif->ic_opmode, &hchan, AH_TRUE, &status))
		{
			IOLog("ath_chan_set: unable to reset "
					  "channel %u (%u Mhz, 0x%X flags) opmode %d, status %d\n",
					  netif->ieee80211_chan2ieee(chan), hchan.channel, hchan.channelFlags, netif->ic_opmode, status);

			return EIO;
		}
		/*
		 * Re-enable rx framework.
		 */
		/*
		 * Update BPF state.
		 */
		// JMB FIX ME
		/* sc->sc_tx_th.wt_chan_freq = sc->sc_rx_th.wr_chan_freq =
			htole16(chan->ic_freq);
		sc->sc_tx_th.wt_chan_flags = sc->sc_rx_th.wr_chan_flags =
			htole16(chan->ic_flags); */
		
		/*
		 * Change channels and update the h/w rate map
		 * if we're switching; e.g. 11a to 11b/g.
		 */
		netif->ic_ibss_chan = chan;
		mode = netif->ieee80211_chan2mode(chan);
		if (mode != sc_curmode)
			setcurmode(mode);
		
		/*
		 * Re-enable interrupts.
		 */
		enableRx();
		enableTx();
		(*hal->ah_setInterrupts)(hal, intr_mask);   /* enable interrupts */
	}
	return 0;
}

/*
 * Calculate the receive filter according to the
 * operating mode and state:
 *
 * o always accept unicast, broadcast, and multicast traffic
 * o maintain current state of phy error reception
 * o probe request frames are accepted only when operating in
 *   hostap, adhoc, or monitor modes
 * o enable promiscuous mode according to the interface state
 * o accept beacons:
 *   - when operating in adhoc mode so the 802.11 layer creates
 *     node table entries for peers,
 *   - when operating in station mode for collecting rssi data when
 *     the station is otherwise quiet, or
 *   - when scanning
 */
u_int32_t AtherosWIFIController::ath_calcrxfilter()
{
	u_int32_t rfilt;
	
	rfilt = (getrxfilter() & HAL_RX_FILTER_PHYERR)
		| HAL_RX_FILTER_UCAST | HAL_RX_FILTER_BCAST | HAL_RX_FILTER_MCAST;
	if (netif->ic_opmode != IEEE80211_M_STA)
		rfilt |= HAL_RX_FILTER_PROBEREQ;
	// JMB FIX ME
	/* if (netif->ic_opmode != IEEE80211_M_HOSTAP &&
	    (ifp->if_flags & IFF_PROMISC))
		rfilt |= HAL_RX_FILTER_PROM; */
	if (netif->ic_opmode == IEEE80211_M_STA ||
	    netif->ic_opmode == IEEE80211_M_IBSS ||
	    netif->ic_state == IEEE80211_S_SCAN)
		rfilt |= HAL_RX_FILTER_BEACON;
	return rfilt;
}

u_int8_t *AtherosWIFIController::broadcastaddr()
{
	// JMB HACK FIXME
	static u_int8_t addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	
	return addr;
}

void AtherosWIFIController::sendMgmtFrame(struct mbuf *m, struct ieee80211_node *ni)
{
	struct mbuf_queue *q = (struct mbuf_queue*)IOMalloc(sizeof(struct mbuf_queue));
	q->m = m;
	q->ni = ni;
	q->next = 0;
	ieee80211_ref_node(ni);
	
	if (!mgmtTail)
		mgmtHead = q;
	else
		mgmtTail->next = q;
	mgmtTail = q;
	
	check_tx_queue();
}

void AtherosWIFIController::sendDataFrame(struct mbuf *m, struct ieee80211_node *ni)
{
	struct mbuf_queue *q = (struct mbuf_queue*)IOMalloc(sizeof(struct mbuf_queue));
	q->m = m;
	q->ni = ni;
	q->next = 0;
	ieee80211_ref_node(ni);

	if (!dataTail)
		dataHead = q;
	else
		dataTail->next = q;
	dataTail = q;
	
	check_tx_queue();
}

void AtherosWIFIController::drainTxq()
{
	if (!_cardGone)
	{
		(*hal->ah_stopTxDma)(hal, txhalq);
	}

	while(activeTxListHead)
	{
		DescBuff *db;
		
		db = activeTxListHead;
		activeTxListHead = db->desc_next;
		if (!activeTxListHead)
			activeTxListTail = 0;
		
		ieee80211_unref_node((struct ieee80211_node**)&db->node);
		if (db->mbuf)
			freePacket(db->mbuf);
		db->initTx(this);
	}
}

const OSString * AtherosWIFIController::newVendorString() const
{
	return OSString::withCString(vendorStr);
}

const OSString * AtherosWIFIController::newModelString() const
{
	return OSString::withCString(modelStr);
}

void AtherosWIFIController::ath_rate_ctl_reset(enum ieee80211_state state)
{
}

