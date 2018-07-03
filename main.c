#include "msp.h"
#include "driverlib.h"
#include "stdio.h"


int timer = 0;
const Timer_A_UpModeConfig upConfig_0 = {
    TIMER_A_CLOCKSOURCE_SMCLK,
    TIMER_A_CLOCKSOURCE_DIVIDER_32,
    46875,
    TIMER_A_TAIE_INTERRUPT_DISABLE,
    TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
    TIMER_A_DO_CLEAR
};

int main(void)
{
    WDT_A_holdTimer();

    //setting dco frequency and initializing clock signal
    MAP_CS_setDCOFrequency(1.5E+6);
    MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig_0);
    //Enable ADC
    MAP_ADC14_enableModule();

    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5,GPIO_PIN5, GPIO_TERTIARY_MODULE_FUNCTION); // P5.5 for 5V connection
    MAP_ADC14_setResolution(ADC_10BIT); // 10 bit resolution
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_1, ADC_MAPINTCH0); // intialize module
    MAP_ADC14_configureSingleSampleMode(ADC_MEM0, false);//only one sampling
    MAP_ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A0, false);
    MAP_ADC14_enableSampleTimer(ADC_MANUAL_ITERATION);

    // Enabling Interrupts
    Interrupt_enableInterrupt(INT_TA0_0);
    MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
    Interrupt_enableMaster();

    // Starting sample
    MAP_ADC14_enableConversion(); //Enable conversion
    MAP_ADC14_toggleConversionTrigger (); //Initiates a single conversion (trigger)

    //run forever
    while (1);

};

void TA0_0_IRQHandler(void)
{
    timer = timer + 1;
    float volt = MAP_ADC14_getvolt(ADC_MEM0);
    volt = volt*1.25/510; //convert to voltage
    float temp = volt * 150/2.5;  // temp interpolated at 150 F at 2.5 V
    printf("Temperature: %f, Time: %d, Output Voltage: %f\n",  temp, timer, volt);
    MAP_ADC14_toggleConversionTrigger();
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
    Timer_A_clearInterruptFlag(TIMER_A0_BASE);


};
