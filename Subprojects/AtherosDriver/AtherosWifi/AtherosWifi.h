

#ifndef ATHEROSWIFI_H
#define ATHEROSWIFI_H

extern "C" {    
#include "ah.h"
#include "ah_desc.h"    
};

#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IOGatedOutputQueue.h>
#include <IOKit/network/IOMbufMemoryCursor.h>
#include <IOKit/network/IOPacketQueue.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IODeviceMemory.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/network/IOMbufMemoryCursor.h>
#include <IOKit/assert.h>
#include <IOKit/IODataQueue.h>
#include "IO80211Controller.h"
#include "IO80211Interface.h"
#include "AtherosWifiInterface.h"

#include "AtherosSettings.h"

#define kAtherosWIFIControllerClass     "AtherosWIFIController"

struct ath_desc;

#define ATH_TX_SCATTER_SIZE 8

class AtherosWIFIController : public IO80211Controller
{
    OSDeclareDefaultStructors( AtherosWIFIController )
    
public:
    virtual bool start(IOService * provider);
    virtual void stop(IOService * provider);
    virtual void free();
    virtual IOReturn getHardwareAddress(IOEthernetAddress * addrP);
    virtual IOReturn enable(IONetworkInterface * netif);
    virtual IOReturn disable(IONetworkInterface * netif);
    
    virtual IOReturn message(UInt32 type, IOService *provider, void *argument = 0);

    IOWorkLoop *getWorkLoop() const;
    
    IOReturn getFrequency(UInt32* freq);
    IOReturn setFrequency(UInt32 freq);
    IOReturn getOpMode(UInt32* mode);
    IOReturn setOpMode(UInt32  mode);
    IOReturn supportedOpModes(UInt32* mode);
	
	void setledstate(enum HAL_LED_STATE state);
	void purge_node_refs(struct ath_node *ni);
    
    //for monitor mode
    IODataQueue* getPacketQueue() { return _packetQueue; }
    IO80211Interface* getNetworkInterface() { return netif; }
	int ath_chan_set(struct ieee80211_channel *chan);
	void stop_scan_timer();
	void stop_recal_timer();
	void stop_watchdog_timer();
	void start_scan_timer();
	void start_recal_timer();
	void start_watchdog_timer();
	void disable_ints(HAL_INT ints);
	u_int32_t ath_calcrxfilter();
	u_int32_t getrxfilter() { return (*hal->ah_getRxFilter)(hal); }
	void setrxfilter(u_int32_t val) { (*hal->ah_setRxFilter)(hal, val); }
	void ath_rate_ctl_reset(enum ieee80211_state state);
	u_int8_t *broadcastaddr();
	void setassocid(u_int8_t *bss, u_int16_t id) { (*hal->ah_writeAssocid)(hal, bss, id, 0); }

	void sendMgmtFrame(struct mbuf*, struct ieee80211_node *ni);
	void sendDataFrame(struct mbuf*, struct ieee80211_node *ni);
	void check_tx_queue();
	const OSString * newVendorString() const;
	const OSString * newModelString() const;
	
	
protected:
        
    friend class DescBufff;
    class DescBuff
    {
public:
        struct DescBuff        *desc_next;
        struct  ath_desc        desc;
		struct  ath_desc		*tx_desc[ATH_TX_SCATTER_SIZE];
        struct mbuf             *mbuf;
        IOMemoryDescriptor      *mdesc, *mbdesc;
		IOMbufNaturalMemoryCursor   *tx_mbdesc;
		IOMemoryDescriptor		*tx_mdesc[ATH_TX_SCATTER_SIZE];
		int						tx_desc_cnt;
		struct ath_node			*node;
        //  IOMemoryDescriptor      *mdesc; //, *mbdesc;
        
        bool initRx(AtherosWIFIController *cont);
        bool initTx(AtherosWIFIController *cont);
        void ctor(AtherosWIFIController *cont);
        void dtor(AtherosWIFIController *cont);
    };
    
    IOPCIDevice *                  pciNub;
    IOMemoryMap *mmap;
    AtherosWIFIInterface *netif;
    struct ath_hal *hal;
    HAL_CHANNEL *chans;
    unsigned int nchans;
    IOWorkLoop *                    workLoop;
    IOInterruptEventSource *        interruptSrc;
    IOTimerEventSource              *recalibrationTimer, *scanTimer, *watchdogTimer;
    IOEthernetAddress               myAddress;
    int                             rxBuffSize;
    DescBuff                        **rxDescs;
    DescBuff                        **txDescs;
    HAL_INT                         intr_mask;
    int                             ath_regdomain;
    int                             ath_countrycode;
    UInt32                          alignment;
	
    u_int                           txhalq;
    u_int32_t                       ic_iv;
	char							vendorStr[32];
	char							modelStr[32];

	const HAL_RATE_TABLE	*sc_rates[IEEE80211_MODE_MAX];
	const HAL_RATE_TABLE	*sc_currates;	/* current rate table */
	enum ieee80211_phymode	sc_curmode;	/* current phy mode */
	u_int8_t		sc_rixmap[256];	/* IEEE to h/w rate table ix */
	u_int8_t		sc_hwmap[32];	/* h/w rate ix to IEEE table */
	
    struct DescBuff *rxListHead, *rxListTail;
    struct DescBuff *txListHead, *txListTail;
    struct DescBuff *activeTxListHead, *activeTxListTail;
    
    bool createWorkLoop();
    virtual IONetworkInterface * createInterface();
    void interruptOccurred(IOInterruptEventSource * src, int count);
    static void interruptOccurredWrapper(OSObject *, IOInterruptEventSource * src, int count);
    
    void recalibration(IOTimerEventSource *sender);
    static void recalibrationWrapper(OSObject *, IOTimerEventSource *sender);
    void scan(IOTimerEventSource *sender);
    static void scanWrapper(OSObject *, IOTimerEventSource *sender);
    void watchdog(IOTimerEventSource *sender);
    static void watchdogWrapper(OSObject *, IOTimerEventSource *sender);

    void rxInterruptOccurred(IOInterruptEventSource *src, int count, HAL_INT intr);
    void txInterruptOccurred(IOInterruptEventSource *src, int count, HAL_INT intr);
    void fatalInterruptOccurred(IOInterruptEventSource *src, int count, HAL_INT intr);
	int txPacket(struct mbuf *mbuf, struct ieee80211_node *ni, DescBuff *db);
    // int txPacket(void *mbuf, int pktlen, DescBuff *db);
    int ath_rate_setup(u_int mode);
	void setcurmode(enum ieee80211_phymode mode);
    void send_test_packet();
    
    IOReturn    allocRxResources();
    void        enableRx();
    IOReturn    freeRxResources();
    void        disableRx();
    IOReturn    reset();
    IOReturn    allocTxResources();
    IOReturn    freeTxResources();
    void        enableTx();
    void        disableTx();
	void		drainTxq();
	u_int		ath_chan2flags(struct ieee80211_channel *chan);
	
    struct int_handler
    {
        HAL_INT     mask;
        void (AtherosWIFIController::*handler)(IOInterruptEventSource *src, int count, HAL_INT intr);
    };
    static int_handler      interruptHandlers[];
    bool                    _cardGone;
    bool                    _enabled;
    
    //settings
    struct opmode_settings {
        enum _operationMode mode;
        HAL_OPMODE          halmode;
        HAL_RX_FILTER       filter;
    };
	
	struct mbuf_queue
	{
		struct mbuf *m;
		struct ieee80211_node *ni;
		struct mbuf_queue *next;
	};
	
	struct mbuf_queue *mgmtHead, *mgmtTail;
	struct mbuf_queue *dataHead, *dataTail;
	

    static opmode_settings  _opmodeSettings[];
    int                     _activeChannelIndex;
    int                     _activeOpMode;
    enum _modulationMode    _activeIEEEMode;
    IODataQueue*            _packetQueue;
};

#endif