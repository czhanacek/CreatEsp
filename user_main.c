#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "gpio.h"
#include "user_interface.h"
#include "uart.c"
#include "main.h"
#include "missingincludes.h"
#include "user_config.h"
/**
 * Main entry
 */

os_timer_t myTimer;
uint16_t interruptCounter;
uint8_t currentByte;
uint8_t bitCounter;

void timerFunc(void) {
    int8_t inputVal = GPIO_INPUT_GET(GPIO_ID_PIN(5));
    uint8_t cpuFreq = system_get_cpu_freq();
    uint_fast32_t time = system_get_time();
    os_printf("%ld\n", time);

}


void interruptHandler(uint16_t * counter) {
    // disable interrupts during interrupt handler
    ETS_GPIO_INTR_DISABLE();
    // figure out which GPIO triggered interrupt
    uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    // AND gpio_status and bit number for relevant GPIO pin to test whether that GPIO triggered the interrupt
    if (gpio_status & BIT(5)) {
        // do thing
        (*bitCounter)++;

        // clear GPIO interrupt status
        GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(5));

        // re-enable interrupt for GPIO pin
        gpio_pin_intr_state_set(GPIO_ID_PIN(5), GPIO_PIN_INTR_LOLEVEL);
    }
    

}

void handleBitIn(void) {
    return;
}

void ICACHE_FLASH_ATTR
user_init()
{
    // set things up
    interruptCounter = 0;
    bitCounter = 0;
    // just disable watchdog
    ets_wdt_disable();
    // run overclocked
    system_update_cpu_freq(160);
    // set up timer
    os_timer_setfn(&myTimer, (os_timer_func_t *)timerFunc, NULL);

    
    // set up gpio
    gpio_init();
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
    GPIO_DIS_OUTPUT(GPIO_ID_PIN(5));


    PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO5_U);

    // set up gpio interrupts
    ETS_GPIO_INTR_DISABLE();
    ETS_GPIO_INTR_ATTACH(interruptHandler, (&interruptCounter));

    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(5));
    
    gpio_pin_intr_state_set(GPIO_ID_PIN(5), GPIO_PIN_INTR_HILEVEL);
    // finally enable gpio interrupts
    ETS_GPIO_INTR_ENABLE();

    os_timer_arm(&myTimer, 2000, TRUE);

    uart_init(BIT_RATE_57600, BIT_RATE_57600);
    
    //os_printf("\nESP8266 says hello\n\n");
}

