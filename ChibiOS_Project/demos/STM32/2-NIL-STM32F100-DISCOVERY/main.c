#include "ch.h"
#include "hal.h"


static UARTConfig uart_cfg_1 = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  38400,
  0,
  USART_CR2_LINEN,
  0
};

static UARTConfig uart_cfg_2 = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  115200,
  0,
  USART_CR2_LINEN,
  0
};

static const SPIConfig hs_spicfg = {
  NULL,
  GPIOA,
  4,
  SPI_CR1_BR_2 | SPI_CR1_BR_1,
  0
};


char samples[8];
char rx[8];
/*
static char sendSPI()
{
	samples[0] = 'a';
	samples[1] = 'a';
	samples[2] = 'a';
	samples[3] = 'a';
	samples[4] = '\n';

spiAcquireBus(&SPID1); 
  spiStart(&SPID1, &hs_spicfg);
	spiSelect(&SPID1);
	spiExchange(&SPID1, 5, samples, rx);
	spiUnselect(&SPID1);
spiReleaseBus(&SPID1);      
} 

*/

static uint8_t txbuf;
static uint8_t rxbuf;
static THD_WORKING_AREA(spi_thread_1_wa, 256);
static THD_FUNCTION(spi_thread_1, p) {

  (void)p;

  chRegSetThreadName("SPI thread 1");
  while (true) {


    spiAcquireBus(&SPID1);              /* Acquire ownership of the bus.    */
    spiStart(&SPID1, &hs_spicfg);       /* Setup transfer parameters.       */
    spiSelect(&SPID1);                  /* Slave Select assertion.          */
    spiReceive(&SPID1, 1,
               rxbuf);          /* Atomic transfer operations.      */

    spiUnselect(&SPID1);                /* Slave Select de-assertion.       */
    spiReleaseBus(&SPID1);              /* Ownership release.               */
  }
}


static THD_WORKING_AREA(blinker, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;

  chRegSetThreadName("blinker");
  while (true) {
    palSetPad(GPIOD, 2);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOD, 2);
    chThdSleepMilliseconds(500);
  }
}

int main(void) {

	halInit();
	chSysInit();

	palSetPadMode(GPIOA, 5, PAL_MODE_STM32_ALTERNATE_PUSHPULL);     /* SCK. */
	palSetPadMode(GPIOA, 6, PAL_MODE_STM32_ALTERNATE_PUSHPULL);     /* MISO.*/
	palSetPadMode(GPIOA, 7, PAL_MODE_STM32_ALTERNATE_PUSHPULL);     /* MOSI.*/
	palSetPadMode(GPIOA, 4, PAL_MODE_OUTPUT_PUSHPULL);		/* SS .*/
	palSetPad(GPIOA, 4);

	uartStart(&UARTD2, &uart_cfg_1);
	uartStart(&UARTD3, &uart_cfg_2);  
txbuf=(uint8_t) 0xFC;
rxbuf=(uint8_t) 1;

 
	//chThdCreateStatic(spi_thread_1_wa, sizeof(spi_thread_1_wa), NORMALPRIO + 1, spi_thread_1, NULL);
        chThdCreateStatic(blinker, sizeof(blinker), NORMALPRIO+1, Thread1, NULL);

	while (true) 
	{
		uartStartSend(&UARTD2, 8, rxbuf);
		//uartStartSend(&UARTD3, 5, "Hola\n");
		//uartStartSend(&UARTD2,1,"\n");
		chThdSleepMilliseconds(500);
	}
	return 0;
}
