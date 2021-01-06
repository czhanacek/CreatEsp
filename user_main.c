#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "gpio.h"
#include "user_interface.h"
#include "uart.c"
#include "esp-gdbstub/gdbstub.h"
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

uint8_t isHigh;
uint16_t bitTime;

Node * uartBuf = NULL;

void makeNode(Node ** lst, uint32_t data) {
    Node * t = *lst;
    Node * newNode = NULL;
    if(t == NULL) {
        newNode = (Node *)os_malloc(sizeof(Node));
        *lst = newNode;
        newNode->pNext = NULL;
        newNode->d = data;
        return;
    }
    if(t->pNext != NULL) {
        while(t->pNext != NULL) {
            t = t->pNext;
        }
    }
    newNode = (Node *)os_malloc(sizeof(Node));
    newNode->pNext = NULL;
    newNode->d = data;
    t->pNext = newNode;
    return;
}



uint32_t popNode(Node ** lst) {
    uint32_t data = 0;
    if((*lst) != NULL) {
        Node * oldHead = (*lst);
        data = oldHead->d;
        *lst = oldHead->pNext;
        os_free(oldHead);
    }
    return data;
}

uint16_t listLen(Node * lst) {
    uint16_t count = 0;
    while(lst != NULL) {
        count++;
        lst = lst->pNext;
    }
    return count;
}

static inline u8 chbit(u8 data, u8 bit)
{
    if ((data & bit) != 0)
    {
       return 1;
    }
    else
    {
       return 0;
    }
}

// read one byte from serial
// mostly lifted from plieningerweb/esp8266-software-uart
// https://github.com/plieningerweb/esp8266-software-uart/blob/master/softuart/softuart.c
void readByte(void) {
    // validate that we have a start bit
    unsigned i;
    uint32_t d = 0;
    unsigned start_time = 0x7FFFFFFF & system_get_time();
    for(i = 0; i < 8; i ++ ) {
        while ((0x7FFFFFFF & system_get_time()) < (start_time + (bitTime*(i+1)))){
            //If system timer overflow, escape from while loop
            if ((0x7FFFFFFF & system_get_time()) < start_time){break;}
        }
        //shift d to the right
        d >>= 1;

        //read bit
        if(GPIO_INPUT_GET(GPIO_ID_PIN(5))) {
            //if high, set msb of 8bit to 1
            d |= 0x80;
        }
    }
    
    makeNode(&uartBuf, d);
}

// write one byte to serial
// mostly lifted from plieningerweb/esp8266-software-uart
// https://github.com/plieningerweb/esp8266-software-uart/blob/master/softuart/softuart.c
void writeByte(uint8_t d) {
    unsigned start_time = 0x7FFFFFFF & system_get_time();
    unsigned i;
    //Start Bit
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4), 0);
	for(i = 0; i < 8; i ++ )
	{
		while ((0x7FFFFFFF & system_get_time()) < (start_time + bitTime))
		{
			//If system timer overflow, escape from while loop
			if ((0x7FFFFFFF & system_get_time()) < start_time){break;}
		}
		GPIO_OUTPUT_SET(GPIO_ID_PIN(4), chbit(d,1<<i));
	}

	// Stop bit
	while ((0x7FFFFFFF & system_get_time()) < (start_time + (bitTime*9)))
	{
		//If system timer overflow, escape from while loop
		if ((0x7FFFFFFF & system_get_time()) < start_time){break;}
	}
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4), 1);
}

void interruptHandler(uint16_t * counter) {
    // disable interrupts during interrupt handler
    gpio_pin_intr_state_set(GPIO_ID_PIN(5), GPIO_PIN_INTR_DISABLE);
    // figure out which GPIO triggered interrupt
    uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    // AND gpio_status and bit number for relevant GPIO pin to test whether that GPIO triggered the interrupt
    if (gpio_status & BIT(5)) {
        // do thing
        int level = GPIO_INPUT_GET(GPIO_ID_PIN(5));
        if(level == 0) {
            readByte();
        }
        // clear GPIO interrupt status
        GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(5));

        // re-enable interrupt for GPIO pin
        gpio_pin_intr_state_set(GPIO_ID_PIN(5), GPIO_PIN_INTR_ANYEDGE);
    
        
    }
   

}
void
timerFunc(void) {
    //os_printf("\n:)\nHello from timer\n\n");
    int i = listLen(uartBuf);
    int x = 0;
    for(; x < i; x++) {
        os_printf("%c", popNode(&uartBuf));
    }
}

void ICACHE_FLASH_ATTR
user_init()
{
    // set things up
    interruptCounter = 0;
    bitCounter = 0;
    isHigh = 0;
    bitTime = 18; // (1000000 / 57600 baudrate) + 1
    // just disable watchdog
    ets_wdt_disable();
    // run overclocked
    system_update_cpu_freq(160);
    // set up timer
    os_timer_setfn(&myTimer, (os_timer_func_t *)timerFunc, NULL);

    
    // set up gpio
    gpio_init();
    // PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
    PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO5_U);



    // PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
    // GPIO_OUTPUT_SET(GPIO_ID_PIN(4), 1);
    // set up gpio interrupts
    ETS_GPIO_INTR_DISABLE();
    ETS_GPIO_INTR_ATTACH(interruptHandler, (&interruptCounter));


    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(5));
    
    gpio_pin_intr_state_set(GPIO_ID_PIN(5), GPIO_PIN_INTR_ANYEDGE);
    // finally enable gpio interrupts
    ETS_GPIO_INTR_ENABLE();

    os_timer_arm(&myTimer, 200, TRUE);

    uart_init(115200, 115200);
    
    os_printf("\nESP8266 says hello\n\n");
}

