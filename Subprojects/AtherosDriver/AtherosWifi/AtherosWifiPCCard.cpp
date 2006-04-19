//234567890123456789012345678901234567890123456789012345678901234567890123456789

#include "AtherosWifiPCCard.h"
#include <IOKit/IOLib.h>

#define super IO80211Controller

bool AtherosWIFIPCCardController::start(IOService * provider)
{
    bool ret = false;
    bool started = false;
    struct ath_hal *tmp_hal = 0;
    pciNub = 0;
    u_int nchans;
    HAL_CHANNEL *chans = 0;
    
    do
    {
        if ( super::start(provider) == false )
            break;
        started = true;
        
        pciNub = OSDynamicCast(IOPCIDevice, provider);
        if ( pciNub == 0 ) break;
        pciNub->retain();        
        if ( pciNub->open(this) == false ) break;
        
        // Request domain power.
        // Without this, the PCIDevice may be in state 0, and the
        // PCI config space may be invalid if the machine has been
        // sleeping.
        
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
        IOLog("Found supported card: %s\n", card_name);
            
        pciNub->configWrite8(kIOPCIConfigLatencyTimer, 0xa8);
        
        mmap = pciNub->mapDeviceMemoryWithRegister( kIOPCIConfigBaseAddress0 );
        if ( mmap == 0 ) break;

        HAL_STATUS status;
        tmp_hal = ath_hal_attach(0, 0, 0, (void*)mmap->getVirtualAddress(),
                                 &status);
        if (tmp_hal == 0L)
        {
            IOLog("Unable to attach to HAL: status %u\n", status);
            tmp_hal = 0;
            break;
        }
        
        if (tmp_hal->ah_abi != HAL_ABI_VERSION)
        {
            IOLog("HAL ABI mismatch; driver version 0x%X, HAL version 0x%X\n",
                  HAL_ABI_VERSION, tmp_hal->ah_abi);
            break;
        }
        
        chans = (HAL_CHANNEL*)IOMalloc(IEEE80211_CHAN_MAX *
                                       sizeof(HAL_CHANNEL));
        if (!chans)
        {
            IOLog("Failed to allocate channel memory\n");
            break;
        }
        
        if (!ath_hal_init_channels(tmp_hal, chans, IEEE80211_CHAN_MAX,
                                &nchans, CTRY_DEFAULT, HAL_MODE_ALL, AH_TRUE))
        {
            IOLog("Unable to collect channel list from HAL\n");
            break;
        }
        
        for(u_int i = 0; i < nchans; ++i)
        {
            HAL_CHANNEL *c = &chans[i];
            int ix = ath_hal_mhz2ieee(c->channel, c->channelFlags);
            IOLog("Found channel %d (%d Mhz, %d flags %s%s%s)\n", ix,
                  c->channel, c->channelFlags,
                   c->channelFlags & CHANNEL_A ? "A" : "",
                  c->channelFlags & CHANNEL_B ? "B" : "",
                   c->channelFlags & CHANNEL_G ? "G" : "");
        }
        
        (*tmp_hal->ah_detach)(tmp_hal);
        tmp_hal = 0;
        mmap->release();
        mmap = 0;
        ret = true;
    } while (false);
    
    pciNub->close(this);

   /* do
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
                netif->setChannelFlags(ix, c->channel);
            }
            else
                netif->setChannelFlags(ix, netif->getChannelFlags(ix)
                                       | c->channelFlags );
        }
        
        netif->init80211Wifi();
        netif->registerService();
        
        ret = true;
    } while (false); */
    
    if (chans)
        IOFree(chans, IEEE80211_CHAN_MAX * sizeof(HAL_CHANNEL));
    chans = 0;

    if (started && !ret)
    {
        if (tmp_hal)
            (*tmp_hal->ah_detach)(tmp_hal);
        tmp_hal = 0;
        if (mmap)
            mmap->release();
        mmap = 0;
        super::stop(provider);
    }
    return ret;
}

void AtherosWIFIPCCardController::stop(IOService *provider)
{
super::stop(provider);
    
    if (provider == pciNub)
    {
        // pciNub->close(this);
        pciNub->release();
        pciNub = 0;
    }
}

void AtherosWIFIPCCardController::free()
{
}