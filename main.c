/*
 * main.c
 *
 *  Created on: 12. 12. 2023
 *      Author: vojtechlukas
 */

// === INCLUDES =================================================================================================

#include <stdint.h>

#include <xdc/std.h>

#include <xdc/runtime/System.h>

#include <xdc/runtime/Memory.h>

#include <xdc/runtime/Error.h>

#include <ti/sysbios/knl/Task.h>

#include <ti/sysbios/BIOS.h>

#include <ti/drivers/Board.h>

#include <ti/sysbios/knl/Semaphore.h>

#include "ti_drivers_config.h"

#include "ti_radio_config.h"

#include <ti/drivers/GPIO.h>

#include <ti/drivers/I2C.h>

#include <source/radio_api/radio_api.h>

#include <source/driverlib/ssd1306/ssd1306.h>

#include <source/oled_gui/gui.h>

#include <source/ethernet/Ethernet.h>

#include <dashboard_task.h>

#include <source/log/log.h>

// ==============================================================================================================


// === DEFINES ==================================================================================================

#define MAC_ADDRESS_STV     (0x50000)
#define DVC_IP_ADD_STV      (MAC_ADDRESS_STV + 6)
#define TGT_IP_ADD_STV      ()

// ==============================================================================================================


// === INTERNAL FUNCTIONS =======================================================================================

Task_Params initTaskParams;
Task_Handle initTaskHandle;
void InitTask()
{
    uint8_t retVal = 0;
    IPAddress tmpIp;
    char ipBuf[17] = {0};

    GUI_Init();

    SPI_begin();

    retVal = Ethernet_begin_mac((uint8_t*)MAC_ADDRESS_STV);

    if ( retVal == 1 )
    {
        tmpIp = Ethernet_localIP();
    }
    else
    {
        IPAddress_Init_str(&tmpIp, (uint8_t*)DVC_IP_ADD_STV);
        Ethernet_begin_mac_ip((uint8_t*)MAC_ADDRESS_STV, tmpIp);
    }

    IPAddress_toString(tmpIp, ipBuf);

    GUI_ChangeDeviceIp(ipBuf);

    return;
}

// ==============================================================================================================

int main()
{
    Board_init();

    GPIO_init();

    Task_Params_init(&initTaskParams);
    initTaskParams.stackSize = 1024;
    initTaskHandle = Task_create((Task_FuncPtr)InitTask, &initTaskParams, NULL);

    BIOS_start();

    return 0;
}
