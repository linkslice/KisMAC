
#include "WiFiController.h"

/**************************************************************************
* POWER-MANAGEMENT CODE
***************************************************************************
* These definitions and functions allow the driver to handle power state
* changes to support sleep and wake.
**************************************************************************/

// Two power states are supported by the driver, On and Off.

enum {
    kWiFiControllerPowerStateOff = 0,
    kWiFiControllerPowerStateOn,
    kWiFiControllerPowerStateCount
};

// An IOPMPowerState structure is added to the array for each supported
// power state. This array is used by the power management policy-maker
// to determine our capabilities and requirements for each power state.

static IOPMPowerState _wifiControllerPowerStateArray[ kWiFiControllerPowerStateCount ] =
{
    { 1,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,IOPMDeviceUsable,IOPMPowerOn,IOPMPowerOn,0,0,0,0,0,0,0,0 }
};

enum {
    kFiveSeconds = 5000000
};

//---------------------------------------------------------------------------
// handleSetPowerStateOff()
//
// The policy-maker has told the driver to turn off the device, and the
// driver has started a new thread to do this. This C function is the start
// of that thread. This function then calls itself through the command gate
// to gain exclusive access to the device.
//---------------------------------------------------------------------------

void handleSetPowerStateOff(thread_call_param_t param0, thread_call_param_t param1) {
    WiFiController *self = (WiFiController *) param0;

    assert(self);

    if (param1 == 0) {
        self->getCommandGate()->runAction( (IOCommandGate::Action)
                                           handleSetPowerStateOff,
                                           (void *) 1 /* param1 */ );
    } else {
        self->setPowerStateOff();
        self->release();  // offset the retain in setPowerState()
    }
}

//---------------------------------------------------------------------------
// handleSetPowerStateOn()
//
// The policy-maker has told the driver to turn on the device, and the
// driver has started a new thread to do this. This C function is the start
// of that thread. This function then calls itself through the command gate
// to gain exclusive access to the device.
//---------------------------------------------------------------------------

void handleSetPowerStateOn(thread_call_param_t param0, thread_call_param_t param1) {
    WiFiController* self = ( WiFiController *) param0;

    assert(self);

    if (param1 == 0) {
        self->getCommandGate()->runAction( (IOCommandGate::Action)
                                           handleSetPowerStateOn,
                                           (void *) 1 /* param1 */ );
    } else {
        self->setPowerStateOn();
        self->release();  // offset the retain in setPowerState()
    }
}

//---------------------------------------------------------------------------
// registerWithPolicyMaker()
//
// The superclass invokes this function when it is time for the driver to
// register with the power management policy-maker of the device.
//
// The driver registers by passing to the policy-maker an array which
// describes the power states supported by the hardware and the driver.
//
// Argument:
//
// policyMaker - A pointer to the power management policy-maker of the
//               device.
//---------------------------------------------------------------------------

IOReturn WiFiController::registerWithPolicyMaker(IOService * policyMaker) {
    IOReturn ret;

    // Initialize power management support state.

    _pmPowerState  = kWiFiControllerPowerStateOn;
    _pmPolicyMaker = policyMaker;

    // No cheating here. Decouple the driver's handling of power change
    // requests from the power management work loop thread. Not strictly
    // necessary for the work that this driver is doing currently, but
    // useful as an example of how to do things in general.
    //
    // Allocate thread callouts for asynchronous power on and power off.
    // Allocation failure is not detected here, but before each use in
    // the setPowerState() method.

    _powerOffThreadCall = thread_call_allocate( 
                           (thread_call_func_t)  handleSetPowerStateOff,
                           (thread_call_param_t) this );

    _powerOnThreadCall  = thread_call_allocate(
                           (thread_call_func_t)  handleSetPowerStateOn,
                           (thread_call_param_t) this );

    ret = _pmPolicyMaker->registerPowerDriver(this,
                                              _wifiControllerPowerStateArray,
                                              kWiFiControllerPowerStateCount);
    
    return ret;
}

//---------------------------------------------------------------------------
// setPowerState()
//
// The power management policy-maker for this device invokes this function
// to switch the power state of the device.
//
// Arguments:
//
// powerStateOrdinal - an index into the power state array for the state
//                     to switch to.
//
// policyMaker       - a pointer to the policy-maker.
//---------------------------------------------------------------------------

IOReturn WiFiController::setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker) {
    // The default return value is for an implied acknowledgement.
    // If the appropriate thread wasn't allocated earlier in
    // registerWithPolicyMaker(), then this is what will be returned.

    IOReturn result = IOPMAckImplied;

    // There's nothing to do if our current power state is the one
    // we're being asked to change to.
    if (_pmPowerState == powerStateOrdinal) {
        return result;
    }

    switch (powerStateOrdinal) {
        case kWiFiControllerPowerStateOff:

            // The driver is being told to turn off the device for some reason.
            // It saves whatever state and context it needs to and then shuts
            // off the device.
            //
            // It may take some time to turn off the HW, so the driver spawns
            // a thread to power down the device and returns immediately to
            // the policy-maker giving an upper bound on the time it will need
            // to complete the power state transition.

            if (_powerOffThreadCall) {
                // Prevent the object from being freed while a call is pending.
                // If thread_call_enter() returns TRUE, then a call is already
                // pending, and the extra retain is dropped.
                
                retain();
                if (thread_call_enter(_powerOffThreadCall) == TRUE) {
                    release();
                }
                result = kFiveSeconds;
            }
            break;

        case kWiFiControllerPowerStateOn:

            // The driver is being told to turn on the device.  It does so
            // and then restores any state or context which it has previously
            // saved.
            //
            // It may take some time to turn on the HW, so the driver spawns
            // a thread to power up the device and returns immediately to
            // the policy-maker giving an upper bound on the time it will need
            // to complete the power state transition.

            if (_powerOnThreadCall) {
                // Prevent the object from being freed while a call is pending.
                // If thread_call_enter() returns TRUE, then a call is already
                // pending, and the extra retain is dropped.

                retain();
                if (thread_call_enter(_powerOnThreadCall) == TRUE) {
                    release();
                }
                result = kFiveSeconds;
            }
            break;
        
        default:
            WLLogEmerg("invalid power state (%ld)", powerStateOrdinal);
            break;
    }

    return result;
}

//---------------------------------------------------------------------------
// setPowerStateOff()
//
// The policy-maker has told the driver to turn off the device, and this
// function is called by a new kernel thread, while holding the gate in
// the driver's work loop. Exclusive hardware access is assured.
//---------------------------------------------------------------------------

void WiFiController::setPowerStateOff() {
    // At this point, all clients have been notified of the driver's
    // imminent transition to a power state that renders it "unusable".
    // And beacuse of the response to this notification by all clients,
    // the controller driver is guaranteed to be disabled when this
    // function is called.

    //freeHardware();
    _cardGone = true;
    getReadyForSleep();
    
    _pmPowerState = kWiFiControllerPowerStateOff;

    // Since the driver returned a non-acknowledgement when called at
    // setPowerState(), it sends an ACK to the policy-maker here to
    // indicate that our power state transition is complete.

    _pmPolicyMaker->acknowledgeSetPowerState();
}

//---------------------------------------------------------------------------
// setPowerStateOn()
//
// The policy-maker has told the driver to turn on the device, and this
// function is called by a new kernel thread, while holding the gate in
// the driver's work loop. Exclusive hardware access is assured.
//---------------------------------------------------------------------------

void WiFiController::setPowerStateOn() {
    _pmPowerState = kWiFiControllerPowerStateOn;

    IOSleep(2);
    _cardGone = false;
    wakeUp();
    
    // Since the driver returned a non-acknowledgement when called at
    // setPowerState(), it sends an ACK to the policy-maker here to
    // indicate that our power state transition is complete.

    _pmPolicyMaker->acknowledgeSetPowerState();

    // With power restored, all clients will be notified that the driver
    // has became "usable". If a client wishes to use the driver, then the
    // driver can expect a call to its enable() method to start things off.
}

