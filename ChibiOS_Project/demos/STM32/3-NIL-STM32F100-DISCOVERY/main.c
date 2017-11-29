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



static BaseSequentialStream * chp3 = (BaseSequentialStream*) &SD3;
static uint8_t TEMP, HR, CHECK_SUM, tmp, bit_counter = 0;

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


/*static SerialConfig uartCfg =
{
38400, // bit rate
};*/

static ICUConfig icucfg = {
  ICU_INPUT_ACTIVE_HIGH,
  ICU_TIM_FREQ,                                /* 1MHz ICU clock frequency.   */
  icuwidthcb,
  NULL,
  NULL,
  ICU_CHANNEL_1,
  0
};

static uint8_t delay;

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
	
	delay =50;
	sdStart(&SD3, NULL);
        chThdCreateStatic(blinker, sizeof(blinker), NORMALPRIO+1, Thread1, NULL);

	while (true) 
	{
 		/*
     		* Making a request
     		*/
    TEMP =  HR =  CHECK_SUM =  tmp =  bit_counter = 0;
	    	palSetPadMode(GPIOA, 8, PAL_MODE_OUTPUT_PUSHPULL);
	    	palWritePad(GPIOA, 8, PAL_LOW);
	    	chThdSleepMicroseconds(18000);
	    	palWritePad(GPIOA, 8, PAL_HIGH);


	     	palSetPadMode(GPIOA, 8, PAL_MODE_INPUT_PULLUP);

	    	/*
	   	  * Initializes the ICU driver 1.
	     	* GPIOA8 is the ICU input.
	     	*/
	   	
	    	icuStart(&ICUD1, &icucfg);
	    	icuStartCapture(&ICUD1);
	    	icuEnableNotifications(&ICUD1);
	    	chThdSleepMilliseconds(700);
		
		//#if ANSI_ESCAPE_CODE_ALLOWED
    		chprintf(chp3, "\033[2J\033[1;1H");
		//#endif
    		icuStopCapture(&ICUD1);
    		icuStop(&ICUD1);

		chprintf(chp3, "%d\n\r", TEMP);
		chprintf(chp3, "%d\n\r", HR);
		
	}
	return 0;
}
