#include <mchck.h>
#include "rfid.desc.h"

static struct cdc_ctx cdc;

void ConfigureSignalGenerator();
void StartSignalGenerator();
void StopSignalGenerator();
void ConfigureSignalCapture();
void StartSignalCapture();
void StopSignalCapture();

static void new_data(uint8_t*, size_t);
void init_vcdc(int);

void FTM0_Handler();
void FTM1_Handler();

int
main(void){

    onboard_led(ONBOARD_LED_ON);

    ConfigureSignalGenerator();
    StartSignalGenerator();

    ConfigureSignalCapture();
    StartSignalCapture();

    usb_init(&cdc_device);

    sys_yield_for_frogs();

    return 0;
}

void 
ConfigureSignalGenerator(){

    /* Configures Edge-Aligned PWM (125kHz) on PTC1 */

    pin_mode(PIN_PTC1, PIN_MODE_PULLUP | PIN_MODE_MUX_ALT4);
    SIM.scgc6.ftm0 = 1;             // turn on clock for ftm0

    FTM0.cntin = 0x0000;            // Inital value for counter

    FTM0.channel[0].csc.msa = 0;
    FTM0.channel[0].csc.msb = 1;
    FTM0.combine[0].decapen = 0;
    FTM0.combine[0].combine = 0;
    FTM0.sc.cpwms = 0;

    FTM0.channel[0].csc.elsa = 1;
    FTM0.channel[0].csc.elsb = 0;

    FTM0.mod = 95;
    FTM0.channel[0].cv = 48;

    FTM0.sc.ps = FTM_PS_DIV4;

    //  int_enable(IRQ_FTM0);
    //  FTM0.channel[0].csc.chie = 1;

    return;
}

void
StartSignalGenerator(){

    // XXX Stays high until here
    FTM0.sc.clks = FTM_CLKS_SYSTEM;

    return;
}

void
StopSignalGenerator(){

    // XXX Stays high after this
    FTM0.sc.clks = FTM_CLKS_NONE;

    return;
}

void
ConfigureSignalCapture(){

    // Configured for Continuous Dual Edge Capture Mode

    pin_mode(PIN_PTB0, PIN_MODE_PULLUP | PIN_MODE_MUX_ALT3);
    SIM.scgc6.ftm1 = 1;

    FTM1.mode.ftmen = 1;            // All your registers are belong to me
    FTM1.combine[0].decapen = 1;    // Enable channels 0 & 1
    FTM1.channel[0].csc.msa = 1;    // Selects continuous mode
    FTM1.channel[0].csc.elsa = 0;
    FTM1.channel[0].csc.elsb = 1;
    FTM1.channel[1].csc.elsa = 1;
    FTM1.channel[1].csc.elsb = 0;
    FTM1.combine[0].combine = 0;
    FTM1.channel[0].csc.chie = 1;   // Enable interrupts
    FTM1.channel[1].csc.chie = 1;
    FTM1.cntin = 0x0000;            // Initial value for counter
    FTM1.sc.ps = FTM_PS_DIV4;       // 12.5 MHz

    return;
}

void
StartSignalCapture(){

    FTM1.sc.clks = FTM_CLKS_SYSTEM;

    // Make sure flags are cleared
    if(FTM1.channel[0].csc.chf == 1)
        FTM1.channel[0].csc.chf = 0;
    if(FTM1.channel[1].csc.chf == 1)
        FTM1.channel[1].csc.chf = 0;

    int_enable(IRQ_FTM1);

    // Start measuring
    FTM1.combine[0].decap = 1;

    return;
}

void
StopSignalCapture(){
    // Stop measuring
    FTM1.combine[0].decap = 0;

    // No clock
    FTM1.sc.clks = FTM_CLKS_NONE;
    SIM.scgc6.ftm1 = 0;

    return;
}

/* Interrupt handlers */

void
FTM0_Handler(){
    FTM0.channel[0].csc.chf = 0;
}

void
FTM1_Handler(){

    if(FTM1.channel[1].csc.chf == 1){
        // We have both pulse widths.
        // Clear flags to start new measurements.
        FTM1.channel[0].csc.chf = 0;
        FTM1.channel[1].csc.chf = 0;

        onboard_led(ONBOARD_LED_TOGGLE);
    }

    return;
}

/* USB */

static void
new_data(uint8_t *data, size_t len){
    onboard_led(-1);
    cdc_write(data, len, &cdc);
}

void
init_vcdc(int config){
    cdc_init(new_data, NULL, &cdc);
}
