
enum _operationMode {
    _operationModeStation   = 0x01,
    _operationModeIBSS      = 0x02,
    _operationModeHostAP    = 0x04,
    _operationModeMonitor   = 0x08,
    _operationModeInvalid   = 0x00,
};

enum _modulationMode {
    _modulationMode80211a   = 0x01,
    _modulationMode80211b   = 0x02,
    _modulationMode80211g   = 0x04,
    _modulationModeAtherosT = 0x08
};

typedef enum WLUCMethods {
    kWLUserClientOpen,                      // kIOUCScalarIScalarO, 0, 0
    kWLUserClientClose,                     // kIOUCScalarIScalarO, 0, 0
    kWLUserClientEnable,                    // kIOUCScalarIScalarO, 0, 0
    kWLUserClientDisable,                   // kIOUCScalarIScalarO, 0, 0
    kWLUserClientGetFrequency,              // kIOUCScalarIScalarO, 0, 1
    kWLUserClientSetFrequency,              // kIOUCScalarIScalarO, 1, 0
    kWLUserClientGetOpMode,                 // kIOUCScalarIScalarO, 0, 1
    kWLUserClientSetOpMode,                 // kIOUCScalarIScalarO, 1, 0
    kWLUserClientSupportedOpModes,          // kIOUCScalarIScalarO, 0, 1
    kWLUserClientLastMethod,
} WLUCMethod;


struct _IEEE_HEADER {
    /* 802.11 Header Info (Little Endian) */
    UInt16 frameControl;
    UInt8  duration;
    UInt8  id;
    UInt8  address1[6];
    UInt8  address2[6];
    UInt8  address3[6];
    UInt16 sequenceControl;
    UInt8  address4[6];
} __attribute__((packed));

//for dumping of pcap frames
struct _WLFrame {
    /* Control Fields (Little Endian) */
    UInt16 status;
    UInt16 channel;
    UInt16 len;
    UInt8  silence;
    UInt8  signal;
    UInt8  rate;
    UInt8  rx_flow;
    UInt8  tx_rtry;
    UInt8  tx_rate;
    UInt16 txControl;
    
    struct _IEEE_HEADER ieee;
    UInt16 dataLen;

    /* 802.3 Header Info (Big Endian) */
    UInt8  dstAddr[6];
    UInt8  srcAddr[6];
    UInt16 length;
} __attribute__((packed));

typedef struct _WLFrame WLFrame;
