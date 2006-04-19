#ifndef ATHEROS_WIFI_INTERFACE_H
#define ATHEROS_WIFI_INTERFACE_H

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

#include "IO80211Interface.h"

#define	ATH_RHIST_SIZE		16	/* number of samples */
#define	ATH_RHIST_NOTIME	(~0)
#define	ATH_NODE(_n)	((struct ath_node *)(_n))	
extern "C"
{
	struct ath_recv_hist {
		int		arh_ticks;	/* sample time by system clock */
		u_int8_t	arh_rssi;	/* rssi */
		u_int8_t	arh_antenna;	/* antenna */
	};
	
	struct ath_node {
		struct ieee80211_node an_node;	/* base class */
		u_int		an_tx_ok;	/* tx ok pkt */
		u_int		an_tx_err;	/* tx !ok pkt */
		u_int		an_tx_retr;	/* tx retry count */
		int		an_tx_upper;	/* tx upper rate req cnt */
		u_int		an_tx_antenna;	/* antenna for last good frame */
		u_int		an_rx_antenna;	/* antenna for last rcvd frame */
		struct ath_recv_hist an_rx_hist[ATH_RHIST_SIZE];
		u_int		an_rx_hist_next;/* index of next ``free entry'' */
	};
}

#define kAtherosWIFIInterfaceClass     "AtherosWIFIInterface"

class AtherosWIFIInterface : public IO80211Interface
{
	OSDeclareDefaultStructors( AtherosWIFIInterface )
	
public:
	class AtherosWIFIController *ctlr;
	
protected:
	virtual int  intf_newstate(enum ieee80211_state, int);
    virtual u_int8_t   intf_node_getrssi(struct ieee80211_node *);
    virtual struct ieee80211_node * intf_node_alloc();
    virtual void  intf_node_free(struct ieee80211_node *);
    virtual void intf_node_copy( struct ieee80211_node *,
								 const struct ieee80211_node *);

	void sendMgmtFrame(struct mbuf*, struct ieee80211_node *ni);
	void sendDataFrame(struct mbuf*, struct ieee80211_node *ni);
};


#endif