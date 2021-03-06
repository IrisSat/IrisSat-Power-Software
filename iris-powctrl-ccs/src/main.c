
#include "main.h"
#include "eusci_a_spi.h"
#include "peripheral_driver.h"
#include "adcbankAB_driver.h"
#include "spi.h"
#include "can.h"
#include "application.h"
#include "power_modes.h"
#include "checkout_activities.h"
#include "ait_functions.h"
#include "thermal_control.h"
#include "fram_driver.h"


void Init_GPIO(void);
void Init_interrupts(void);

unsigned int CC_milis=0;


int main(void) {

//     Initialization
    WDT_A_hold(WDT_A_BASE);
//    WDTCTL = WDTPW | WDTHOLD;
    // Get mode from NVM
    Init_interrupts();
    Init_GPIO();

    //////////////////////////////////////////////////////////////////////////////////
    // the set of activities that this program runs are as follows      //  Status  //
    //////////////////////////////////////////////////////////////////////////////////
    // reset WDT: done through timer overflow interrupt                 //  done!   //
    // Read ADC bank A:                                                 //  done!   //
    // Read ADC bank B:                                                 //  done!   //
    // Read battery SOC: done through the pin interrupts configured     //  done!   //
    // Turn on solar panel inputs:                                      //  done!   //
    // do load shedding                                                 //  TBD     //
    // switch mode between load shedding and detumbling                 //  TBD     //
    // check CAN messages                                               //  done!   //
    // make Telemetry data                                              //  done!   //
    // send CAN message                                                 //  done!   //
    //////////////////////////////////////////////////////////////////////////////////
    // Progress                                                         //  80%     //
    //////////////////////////////////////////////////////////////////////////////////

    // Initialize non-volatile storage used for telemetry logging
    NvsInit();
    // MOVE initTelemetry to commandHandler task once FreeRTOS is implemented
    initTelemetry();
    // Perform post-ejection chcekout activities
    CheckoutActivities();
    // Initialize state for SoC estimation
    InitEstimateSoc(1.0);

#ifdef AIT_MODE
    while(1)
    {
        /* Attend to CDH commands */
        checkCommands();
        /* Check Battery State of Charge */
        AitMonitorSoc();
        /* ADCS control algorithm */
        AitAdcsControl();
        /* Thermal control algorithm */
#ifdef THERMAL_CONTROL
        MainThermalControl();
#endif


    }
#else
    // Once FreeRTOS is integrated, these will become FreeRTOS taskS
    while(1)
    {
        /* Attend to CDH commands */
        checkCommands();
        /* Check Battery State of Charge */
        monitorSoc();
        /* ADCS control algorithm */
        testAdcsSpi();
    }
#endif

}



void Init_GPIO(void)
{
    Init_Ports();
    Init_ADC_A();
    Init_ADC_B();
    Init_SPI();
    CAN_Wake();
    Init_CAN();

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();

//    __enable_interrupt();
}



void Init_interrupts(void) // this function configures the timer and pin interrupts
{


    //setup the interrupt pins: P4.5 & P4.6 are for Coulomb counter
    P4DIR &= ~(BIT5 + BIT6);
    P4REN |= BIT5 + BIT6; //enable resistor
    P4OUT |= BIT5 + BIT6;

//    P4IES |= BIT5; // sensitivity high-to-low
    P4IES |= BIT6; // sensitivity high-to-low


    //setup IRQ
    P4IE |= BIT6; // enable P4.5 IRG


    //setup timer
    TB0CTL |= TBCLR;            // reset timer
    TB0CTL |= TBSSEL__SMCLK;    // clock = SMCLK
    TB0CTL |= MC__CONTINOUS;    // mode=continuous

    //setup interrupt
    TB0CTL |= TBIE;

    __enable_interrupt();


//    P4IFG &= ~BIT5; // clear P4.5 IRQ flag
    P4IFG &= ~BIT6; // clear P4.5 IRQ flag
    TB0CTL &= ~TBIFG;   // clear flag


}

//#pragma vector = PORT4_VECTOR
//__interrupt void ISR_Port4_S6(void)
//{
//    //each time this interrupt happens, 1 Coulomb is passed
//    if (P4IN & BIT6)
//    {
//        COULOMB++;
//        BATT_CURR = -(111300/(32.75*0.01))/CC_milis;
//    }
//    else
//    {
//        COULOMB--;
//        BATT_CURR = (111300/(32.75*0.01))/CC_milis;
//    }
//    // also add a timer read option here to estimate current
//
//
////    P4IFG &= ~BIT5; // clear P4.5 IRQ flag
//    P4IFG &= ~BIT6; // clear P4.5 IRQ flag
//    CC_milis = 0;
//}


#pragma vector = TIMER0_B1_VECTOR
__interrupt void ISR_TB0_Overflow(void)
{
    // toggle the WDT heart beat
    P6OUT ^= BIT2;
    P6OUT ^= BIT1; // this is the USER_LED
    TB0R = 0xFC16; // this makes sure we have 1 ms intervals

    // add one millisecond to CC_millis

    CC_milis++;
    // clear flag
    TB0CTL &= ~TBIFG;   // clear flag
}

