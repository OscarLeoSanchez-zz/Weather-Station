#include "ch.h"
#include "hal.h"
#include "chprintf.h"




/*
 * Enable if your terminal supports ANSI ESCAPE CODE
 */
#define ANSI_ESCAPE_CODE_ALLOWED                  TRUE
/*===========================================================================*/
/* DHT11 related defines                                                     */
/*===========================================================================*/
/*
 * Width are in useconds
 */
#define    MCU_REQUEST_WIDTH                     18000
#define    DHT_ERROR_WIDTH                         200
#define    DHT_START_BIT_WIDTH                      80
#define    DHT_LOW_BIT_WIDTH                        28
#define    DHT_HIGH_BIT_WIDTH                       70
/*===========================================================================*/
/* ICU related code                                                          */
/*===========================================================================*/
#define    ICU_TIM_FREQ                        1000000

static BaseSequentialStream * chp = (BaseSequentialStream*) &SD2;

static uint8_t TEMP, HR, CHECK_SUM, tmp, bit_counter = 0;;
static icucnt_t widths [40];
static void icuwidthcb(ICUDriver *icup) {
  icucnt_t width = icuGetWidthX(icup);
  if(width >= DHT_START_BIT_WIDTH){
    /* starting bit resetting the bit counter */
    bit_counter = 0;
  }
  else{
    /* Recording current width. Just for fun  */
    widths[bit_counter] = width;
    if(width > DHT_LOW_BIT_WIDTH){
      tmp |= (1 << (7 - (bit_counter % 8)));
    }
    else{
      tmp &= ~(1 << (7 - (bit_counter % 8)));
    }
    /* When bit_counter is 7, tmp contains the bit from 0 to 7 corresponding to
       The Humidity Rate integer part (Decimal part is 0 on DHT 11) */
    if(bit_counter == 7)
      HR = tmp;
    /* When bit_counter is 23, tmp contains the bit from 16 to 23 corresponding to
       The Temperature integer part (Decimal part is 0 on DHT 11) */
    if(bit_counter == 23)
      TEMP = tmp;
    /* When bit_counter is 39, tmp contains the bit from 32 to 39 corresponding to
       The Check sum value */
    if(bit_counter == 39)
      CHECK_SUM = tmp;
    bit_counter++;
  }
}


static SerialConfig uartCfg =
{
38400, // bit rate
};

static ICUConfig icucfg = {
  ICU_INPUT_ACTIVE_HIGH,
  ICU_TIM_FREQ,                                /* 1MHz ICU clock frequency.   */
  icuwidthcb,
  NULL,
  NULL,
  ICU_CHANNEL_1,
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
static uint8_t delay;
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
    chThdSleepMilliseconds(delay);
    palClearPad(GPIOD, 2);
    chThdSleepMilliseconds(delay);
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
	
	delay =500;
	uartStart(&UARTD3, &uart_cfg_2);  
	txbuf=(uint8_t) 0xFC;
	rxbuf=(uint8_t) 1;

	sdStart(&SD2, &uartCfg); // starts the serial driver with uartCfg as a config
 
	//chThdCreateStatic(spi_thread_1_wa, sizeof(spi_thread_1_wa), NORMALPRIO + 1, spi_thread_1, NULL);
        chThdCreateStatic(blinker, sizeof(blinker), NORMALPRIO+1, Thread1, NULL);

	while (true) 
	{


 		/*
     		* Making a request
     		*/
	    	palSetPadMode(GPIOA, 8, PAL_MODE_OUTPUT_PUSHPULL);
	    	palWritePad(GPIOA, 8, PAL_LOW);
	    	chThdSleepMicroseconds(18000);
	    	palWritePad(GPIOA, 8, PAL_HIGH);

	    	/*
	   	  * Initializes the ICU driver 1.
	     	* GPIOA8 is the ICU input.
	     	*/
	   	palSetPadMode(GPIOA, 8, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
	    	icuStart(&ICUD1, &icucfg);
	    	icuStartCapture(&ICUD1);
	    	icuEnableNotifications(&ICUD1);
	    	chThdSleepMilliseconds(700);
		
		#if ANSI_ESCAPE_CODE_ALLOWED
		//uartStartSend(&UARTD2, 13, "Starting...\r\n");
    		//chprintf(chp, "\033[2J\033[1;1H");
		#endif
    		icuStopCapture(&ICUD1);
    		icuStop(&ICUD1);

		//uartStartSend(&UARTD2, sizeof(TEMP), TEMP);
		//uartStartSend(&UARTD2, 14, "Temperature: %d C, Humidity Rate: %d %% \n\r", TEMP, HR);
    		chprintf(chp, "Temperature: %d C, Humidity Rate: %d %% \n\r", TEMP, HR);
    		if(CHECK_SUM == (TEMP + HR)){

		//	uartStartSend(&UARTD2, 14, "Checksum OK!\n\r");
      			//chprintf(chp, "Checksum OK!\n\r");
    		}
    		else{
		//	uartStartSend(&UARTD2, 18, "Checksum FAILED!\n\r");
      			//chprintf(chp, "Checksum FAILED!\n\r");
    		}
		if (TEMP >15 && TEMP <30 )
		{
			delay = 50;
		}


	

	}
	return 0;
}
