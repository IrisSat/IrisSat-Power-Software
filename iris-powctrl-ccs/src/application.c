/*
 * application.c
 *
 *  Created on: Jan. 22, 2022
 *      Author: jayden
 */

#include "application.h"
#include "main.h"
#include "drivers/device/tcan4x5x/TCAN4550.h"

#define RX_CANMSG_DELAY 1000
extern volatile uint8_t TCAN_Int_Cnt;

void commandHandler(void)
{
    initTelemetry();

    // Wait for CAN message
    while (1)
    {
        if (TCAN_Int_Cnt > 0 )
        {
            TCAN_Int_Cnt--;
            TCAN4x5x_Device_Interrupts dev_ir = {0};            // Define a new Device IR object for device (non-CAN) interrupt checking
            TCAN4x5x_MCAN_Interrupts mcan_ir = {0};             // Setup a new MCAN IR object for easy interrupt checking
            TCAN4x5x_Device_ReadInterrupts(&dev_ir);            // Read the device interrupt register
            TCAN4x5x_MCAN_ReadInterrupts(&mcan_ir);             // Read the interrupt register

            if (dev_ir.SPIERR)                                  // If the SPIERR flag is set
                TCAN4x5x_Device_ClearSPIERR();                  // Clear the SPIERR flag

            if (mcan_ir.RF0N)                                   // If a new message in RX FIFO 0
            {
                TCAN4x5x_MCAN_RX_Header MsgHeader = {0};        // Initialize to 0 or you'll get garbage
                uint8_t numBytes = 0;                           // Used since the ReadNextFIFO function will return how many bytes of data were read
                uint8_t dataPayload[64] = {0};                  // Used to store the received data

                TCAN4x5x_MCAN_ClearInterrupts(&mcan_ir);        // Clear any of the interrupt bits that are set.

                numBytes = TCAN4x5x_MCAN_ReadNextFIFO( RXFIFO0, &MsgHeader, dataPayload);   // This will read the next element in the RX FIFO 0

                // Process the command
                telemetryPacket_t command;
                unpackTelemetry(dataPayload, &command);
                handleCommand(&command);



                // numBytes will have the number of bytes it transfered in it. Or you can decode the DLC value in MsgHeader.DLC
                // The data is now in dataPayload[], and message specific information is in the MsgHeader struct.
//                if (MsgHeader.RXID == 0x0AA)        // Example of how you can do an action based off a received address
//                {
//                    // Do something
//                }
            }
        }
        else
        {
            __delay_cycles(RX_CANMSG_DELAY);
        }
    }
}


void handleCommand(telemetryPacket_t* command)
{
    switch(command->telem_id)
    {
        case POWER_READ_TEMP_CMD:
        {
            // Get data
            uint8_t therm = command->data[0];
            float temp = read_temperature(therm);
            // Send telemetry
            telemetryPacket_t telemetry;
            telemetry.telem_id = POWER_READ_TEMP_ID;
            Calendar_t timestamp = {0};
            telemetry.timestamp = timestamp;
            telemetry.length = sizeof(float);
            uint8_t data[sizeof(float)];
            memcpy(data,&temp,sizeof(float));
            telemetry.data = data;
            sendTelemetry(&telemetry);
            break;
        }
        case POWER_READ_SOLAR_CURRENT_CMD:
        {
            // Get data
            uint8_t solar = command->data[0];
            float solar_current = read_solar_current(solar);
            // Send telemetry
            telemetryPacket_t telemetry;
            telemetry.telem_id = POWER_READ_SOLAR_CURRENT_ID;
            Calendar_t timestamp = {0};
            telemetry.timestamp = timestamp;
            telemetry.length = sizeof(float);
            uint8_t data[sizeof(float)];
            memcpy(data,&solar_current,sizeof(float));
            telemetry.data = data;
            sendTelemetry(&telemetry);
            break;
        }
        case POWER_READ_LOAD_CURRENT_CMD:
        {
            // Get data
            uint8_t load = command->data[0];
            float load_current = read_load_current(load);
            // Send telemetry
            telemetryPacket_t telemetry;
            telemetry.telem_id = POWER_READ_LOAD_CURRENT_ID;
            Calendar_t timestamp = {0};
            telemetry.timestamp = timestamp;
            telemetry.length = sizeof(float);
            uint8_t data[sizeof(float)];
            memcpy(data,&load_current,sizeof(float));
            telemetry.data = data;
            sendTelemetry(&telemetry);
            break;
        }
        case POWER_READ_MSB_VOLTAGE_CMD:
        {
            // Get data
            float MSB_voltage = read_MSB_voltage();
            uint8_t data[sizeof(float)];
            // Send telemetry
            telemetryPacket_t telemetry;
            telemetry.telem_id = POWER_READ_MSB_VOLTAGE_ID;
            Calendar_t timestamp = {0};
            telemetry.timestamp = timestamp;
            telemetry.length = sizeof(float);
            memcpy(data,&MSB_voltage,sizeof(float));
            telemetry.data = data;
            sendTelemetry(&telemetry);
            break;
        }
        case POWER_SET_POW_MODE_CMD:
        {
            uint8_t mode = command->data[0];
            set_POW_mode(mode);
            break;
        }
        default:
        {
            break;
        }
    }
}


void commandHandler_noInterrupt(void)
{
    TCAN4x5x_Device_Interrupts dev_ir = {0};            // Define a new Device IR object for device (non-CAN) interrupt checking
    TCAN4x5x_MCAN_Interrupts mcan_ir = {0};             // Setup a new MCAN IR object for easy interrupt checking
    while (1)
    {
        TCAN4x5x_Device_ReadInterrupts(&dev_ir);            // Read the device interrupt register
        TCAN4x5x_MCAN_ReadInterrupts(&mcan_ir);             // Read the interrupt register

        if (dev_ir.SPIERR)                                  // If the SPIERR flag is set
            TCAN4x5x_Device_ClearSPIERR();                  // Clear the SPIERR flag

        if (mcan_ir.RF0N)                                   // If a new message in RX FIFO 0
        {
            TCAN4x5x_MCAN_RX_Header MsgHeader = {0};        // Initialize to 0 or you'll get garbage
            uint8_t numBytes = 0;                           // Used since the ReadNextFIFO function will return how many bytes of data were read
            uint8_t dataPayload[64] = {0};                  // Used to store the received data

            TCAN4x5x_MCAN_ClearInterrupts(&mcan_ir);        // Clear any of the interrupt bits that are set.

            numBytes = TCAN4x5x_MCAN_ReadNextFIFO( RXFIFO0, &MsgHeader, dataPayload);   // This will read the next element in the RX FIFO 0

            // Process the command
            telemetryPacket_t command;
            unpackTelemetry(dataPayload, &command);
            handleCommand(&command);

        }
        else
        {
            __delay_cycles(RX_CANMSG_DELAY);
        }
    }
}