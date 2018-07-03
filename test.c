#include "msp.h"
#include "driverlib.h"
#include "stdio.h"

#define MAIN_1_SECTOR_31 0x0003F000 // Address of Main Memory, Bank 1, Sector 31
#define LED1 GPIO_PIN0
#define Port1 GPIO_PORT_P1

int inc = 0;
float data[30];

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
    printf('the');
    WDT_A_holdTimer();

    //LED setup
    MAP_GPIO_setAsOutputPin(Port1,LED1);
    MAP_GPIO_setOutputLowOnPin(Port1, LED1);


    // Set switch 1 (S1) as input button (connected to P1.1)
    MAP_GPIO_setAsInputPinWithPullUpResistor( GPIO_PORT_P1, GPIO_PIN1 );
    // Set switch 2 (S2) as input button (connected to P1.4)
    MAP_GPIO_setAsInputPinWithPullUpResistor( GPIO_PORT_P1, GPIO_PIN4 );

    while(1){
        // Read button 1
        float usiButton1 = MAP_GPIO_getInputPinValue ( GPIO_PORT_P1, GPIO_PIN1 );
        // Read button 2
        float usiButton2 = MAP_GPIO_getInputPinValue ( GPIO_PORT_P1, GPIO_PIN4 );

        //if button 1 is pressed enter data acquisition mode
        if ( usiButton1 == GPIO_INPUT_PIN_LOW ) {
            //setting DCO frequency and initializing clock signal
            MAP_CS_setDCOFrequency(1.5E+6);
            MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
            MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig_0);

            //Enable ADC
            MAP_ADC14_enableModule();

            //STEP 1: Select input pin you want to use and configure it
            MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5,GPIO_PIN5, GPIO_TERTIARY_MODULE_FUNCTION);
            //STEP 2: Set resolution
            MAP_ADC14_setResolution(ADC_10BIT);
            //STEP 3: Set clock sources using initModule function
            MAP_ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_1, ADC_MAPINTCH0);
            //STEP 4: Configure sampling mode
            MAP_ADC14_configureSingleSampleMode(ADC_MEM0, false);
            //STEP 5: Configure conversion memory and voltage range
            MAP_ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A0, false);
            MAP_ADC14_enableSampleTimer(ADC_MANUAL_ITERATION);

            // Enabling Interrupts
            Interrupt_enableInterrupt(INT_TA0_0);
            MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
            Interrupt_enableMaster();

            // Starting sample
            MAP_ADC14_enableConversion();
            MAP_ADC14_toggleConversionTrigger();

            //run until 30 cycles have been completed
            while (inc < 30);

            Interrupt_disableMaster();

            // Unprotecting Main Bank 1, Sector 31
            FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,FLASH_SECTOR31);
            // Trying to erase the sector. Within this function, the API will automatically try to
            // erase the maximum number of tries. If it fails, notify user.
            if(!FlashCtl_eraseSector(MAIN_1_SECTOR_31))
            printf('Erase failed\r\n') ;
            // Trying to program the memory. Within this function, the API will automatically try
            // to program the maximum number of tries. If it fails, notify user.
            if(!FlashCtl_programMemory(data, (void*) MAIN_1_SECTOR_31, 120))
            printf('Write failed\r\n') ;
            /* Setting the sector back to protected */
            FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,FLASH_SECTOR31);

            float* FlashPointer= (float*) MAIN_1_SECTOR_31;
            int z;
            for (z=0; z<30; z=z+1){
                float value= *FlashPointer;
                printf('Temp: %f',value);
                FlashPointer++;
            }
            MAP_GPIO_setOutputHighOnPin(Port1, LED1); //Turn on LED 1


        }


    }

}

void TA0_0_IRQHandler(void)
{
    if (inc < 30){
    MAP_GPIO_setOutputHighOnPin(Port1, LED1);

    //MAP_GPIO_setOutputLowOnPin(Port1, LED1);
    inc = inc + 1;
    float vo = MAP_ADC14_getResult(ADC_MEM0);
    vo = vo*1.25/510; //these numbers were found by measuring the op amp output with a multimeter, comparing it to the msp reading, and establishing a conversion constant.
    float temp = vo * 150/2.5;  // voltage converted to temp

    data[inc - 1]=temp;

    MAP_GPIO_setOutputLowOnPin(Port1, LED1);

    sprintf('data: %f', data);


    MAP_ADC14_toggleConversionTrigger();
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
    Timer_A_clearInterruptFlag(TIMER_A0_BASE);
    }

};
