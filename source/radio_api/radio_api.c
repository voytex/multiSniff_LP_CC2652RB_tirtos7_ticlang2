/*
 * radio_api.c
 *
 *  Created on: 11. 9. 2023
 *      Author: vojtechlukas
 */

// === INCLUDES =================================================================================================

#include "ti_radio_config.h"

#include "ti_drivers_config.h"

#include <source/utils/stv.h>

#include <source/ethernet/Ethernet.h>

#include <source/ethernet/EthernetUdp.h>

#include <source/queue/radio_queue.h>

#include <source/utils/log.h>

#include <source/radio_api/radio_api.h>

// ==============================================================================================================


// === INCLUDES =================================================================================================

#define MAX_PACKET_LEN (2047)

// ==============================================================================================================


// === INTERNAL FUNCTIONS =======================================================================================

RF_Op* getRXCmdByProto(RF_Protocol_t proto)
{
    if ( proto == BluetoothLowEnergy )
    {
        return (RF_Op*) &RFCMD_bleGenericRX;
    }

    if ( proto == IEEE_802_15_4 )
    {
        return (RF_Op*) &RFCMD_ieeeRX;
    }

    return NULL;
}

RF_Op* getFSCmdByProto(RF_Protocol_t proto)
{
    if ( proto == BluetoothLowEnergy )
    {
        return (RF_Op*) &RFCMD_bleFrequencySynthesizer;
    }

    if ( proto == IEEE_802_15_4 )
    {
        return (RF_Op*) &RFCMD_ieeeFrequencySynthesizer;
    }

    return NULL;
}

RF_RadioSetup* getRSCmdByProto(RF_Protocol_t proto)
{
    if ( proto == BluetoothLowEnergy )
    {
        return (RF_RadioSetup*) &RFCMD_bleRadioSetup;
    }

    if ( proto == IEEE_802_15_4 )
    {
        return (RF_RadioSetup*) &RFCMD_ieeeRadioSetup;
    }

    return NULL;
}

RF_Mode* getRFModeByProto(RF_Protocol_t proto)
{
    if ( proto == BluetoothLowEnergy )
    {
        return (RF_Mode*) &RFCMD_bleModeObject;
    }

    if ( proto == IEEE_802_15_4 )
    {
        return (RF_Mode*) &RFCMD_ieeeModeObject;
    }

    return NULL;
}


// ==============================================================================================================


// === FUNCTION DEFINITIONS =====================================================================================

/*
 * === Radio_openRadioCore
 * Initializes Radio Core of MCU according to parameters.
 *
 * Parameters:
 *      pParams[in]  - pointer to parameters structure
 *      pObj[in]     - pointer to object storing internal configuration
 *      proto[in     - RF protocol to which Radio Core should be opened
 *      pHandle[out] - pointer to RF_Handle to control Radio Core
 * Returns:
 *      N/A
 *
 */
void Radio_openRadioCore(RF_Params* pParams, RF_Object* pObj, RF_Protocol_t proto, RF_Handle* pHandle)
{
    RF_Params_init(pParams);

    RF_Mode* pMode = getRFModeByProto(proto);

    RF_RadioSetup* pSetup = getRSCmdByProto(proto);

    *pHandle = RF_open(pObj, pMode, pSetup, pParams);

    return;
}


/*
 * === Radio_setFrequencySynthesizer
 * Sets frequency synthesizer according in order to tune to a given
 * BLE or IEEE channel
 *
 * Parameters:
 *      pHandle[in]  - handle to Radio Core
 *      pFsCmd[in]   - pointer to Frequency Synthesizer Command
 * Returns:
 *      RF_EventMask - Event mask signaling status of the command
 *
 */
RF_EventMask Radio_setFrequencySynthesizer(RF_Handle *pHandle,  RF_Protocol_t proto)
{
    RF_Op* pFsCmd = getFSCmdByProto(proto);

    RF_EventMask retVal = RF_postCmd(*pHandle, pFsCmd, RF_PriorityNormal, NULL, 0);

    Log_print("SetFrequencySynthesizer: ", &retVal, RfEvent);

    Log_print("SetFSCmd: ", pFsCmd, CmdStatus);

    return retVal;


}


/*
 * === Radio_initRXCmd
 * Initializes RX commands of given protocol for sniffing.
 *
 * Parameters:
 *      pRXCmd[in,out]  - pointer to Command struct (BLE/IEEE)
 *      proto[in]       - protocol to determine typecasting
 * Returns:
 *      N/A
 */
void Radio_initRXCmd(rfc_bleGenericRxOutput_t* bleStats, rfc_ieeeRxOutput_t* ieeeStats)
{
    //RFCMD_bleGenericRX.status                             = 0x0;
    RFCMD_bleGenericRX.pParams->pRxQ                      = RadioQueue_getDQpointer(); //todo: rx queue
    RFCMD_bleGenericRX.pParams->rxConfig.bAutoFlushCrcErr = 1;
    RFCMD_bleGenericRX.pParams->rxConfig.bIncludeLenByte  = 1;
    RFCMD_bleGenericRX.pParams->rxConfig.bIncludeCrc      = 1;
    RFCMD_bleGenericRX.pParams->rxConfig.bAppendRssi      = 0;
    RFCMD_bleGenericRX.pParams->rxConfig.bAppendStatus    = 0;
    RFCMD_bleGenericRX.whitening.init                     = 0x65;
    RFCMD_bleGenericRX.pOutput                            = bleStats; //todo: stats
    RFCMD_bleGenericRX.channel                            = 0x66; //todo: initial channel

    //RFCMD_ieeeRX.status                                     = 0x0;
    RFCMD_ieeeRX.pRxQ                                       = RadioQueue_getDQpointer();
    RFCMD_ieeeRX.rxConfig.bAutoFlushCrc                     = 1;
    RFCMD_ieeeRX.rxConfig.bAutoFlushIgn                     = 0;
    RFCMD_ieeeRX.rxConfig.bIncludePhyHdr                    = 0;
    RFCMD_ieeeRX.rxConfig.bIncludeCrc                       = 1;
    RFCMD_ieeeRX.rxConfig.bAppendRssi                       = 0;
    RFCMD_ieeeRX.rxConfig.bAppendCorrCrc                    = 0;
    RFCMD_ieeeRX.rxConfig.bAppendSrcInd                     = 0;
    RFCMD_ieeeRX.rxConfig.bAppendTimestamp                  = 0;
    RFCMD_ieeeRX.pOutput                                    = ieeeStats;
    RFCMD_ieeeRX.channel                                    = 0;

    return;
}


/*
 * === Radio_beginRX
 * Posts (sends and does not wait for execution end) RX command according to
 * selected protocol.
 * Main purpose is to shadow TI's API for better readability
 *
 * Parameters:
 *      pHandle[in]             - handle to Radio Core
 *      proto[in]               - protocol whose RX cmd will be sent
 *      callbackFunction[in]    - pointer to callback function that gets
 *                                executed when event set by <events> occurs
 *      events                  - events on which <callbackFunction> gets called
 * Returns:
 *      RF_CmdHandle            - handle to control given command
 */
RF_CmdHandle Radio_beginRX(RF_Handle pHandle, RF_Protocol_t proto, void* callbackFunction, RF_EventMask events)
{
    RF_Op* pRXCmd = getRXCmdByProto(proto);

    RF_CmdHandle retVal;

    retVal = RF_postCmd(pHandle, pRXCmd, RF_PriorityNormal, callbackFunction, events);

    Log_print("BeginRX: ", pRXCmd, CmdStatus);

    return retVal;

}


/*
 * === Radio_stopRX
 * Stops listening to RF frames.
 * Main purpose is to shadow TI's API for better readability
 *
 * Parameters:
 *      pHandle[in]             - handle to Radio Core
 * Returns:
 *      RF_Stat                 - Enum signaling successful completition
 */
RF_Stat Radio_stopRX(RF_Handle pHandle)
{
    RF_Stat retVal = RF_flushCmd(pHandle, RF_CMDHANDLE_FLUSH_ALL, 1);

    Log_print("StopAllStat: ", &retVal, RfStat);

    Log_print("StopAll[BLE]: ", getRXCmdByProto(BluetoothLowEnergy), CmdStatus);

    Log_print("StopAll[IEEE]: ", getRXCmdByProto(IEEE_802_15_4), CmdStatus);

    RF_close(pHandle);

    return retVal;
}


/*
 * === TODO
 * Stops listening to RF frames.
 * Main purpose is to shadow TI's API for better readability
 *
 * Parameters:
 *      pHandle[in]             - handle to Radio Core
 * Returns:
 *      RF_Stat                 - Enum signaling successful completition
 */
RF_Protocol_t Radio_GetCurrentProtocol()
{
    if ( STV_ReadFromAddress(STVW_RF_PROTOCOL) == STV_RF_PROTO_BLE )
    {
        return BluetoothLowEnergy;
    }

    if ( STV_ReadFromAddress(STVW_RF_PROTOCOL) == STV_RF_PROTO_IEEE )
    {
        return IEEE_802_15_4;
    }

    return 0;
}

/*
 * === TODO
 * Stops listening to RF frames.
 * Main purpose is to shadow TI's API for better readability
 *
 * Parameters:
 *      pHandle[in]             - handle to Radio Core
 * Returns:
 *      RF_Stat                 - Enum signaling successful completition
 */
void Radio_HandleQueueOverflow(RF_Handle rfHnd, RF_CmdHandle rfCmdHnd, RF_EventMask eventMsk)
{
    // TODO increment nRxBufFull

    RadioQueue_reset();

    Radio_beginRX(rfHnd, BluetoothLowEnergy, &Radio_HandleQueueOverflow, RF_EventRxBufFull);

    return;
}




// ==============================================================================================================
