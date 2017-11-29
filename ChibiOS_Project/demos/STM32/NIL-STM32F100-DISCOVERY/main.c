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
/*===========================================================================*/
/* ADC variables                                                         */
/*===========================================================================*/
#define ADC_GRP1_NUM_CHANNELS   1
#define ADC_GRP1_BUF_DEPTH      32


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





static adcsample_t samples1[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 8 samples of 1 channel, SW triggered.
 * Channels:    IN10.
 */



static const ADCConversionGroup adcgrpcfg = {
	FALSE,
	ADC_GRP1_NUM_CHANNELS,
	NULL,
	NULL,
	0,
	ADC_CR2_EXTSEL_SWSTART,                     
	0,
	ADC_SMPR2_SMP_AN0(ADC_SAMPLE_7P5),
	ADC_SQR1_NUM_CH(ADC_GRP1_NUM_CHANNELS),
	0,
	ADC_SQR3_SQ1_N(ADC_CHANNEL_IN0)
};

int main(void) {

	halInit();
	chSysInit();
	
	delay =1000;
	sdStart(&SD3, NULL);
        chThdCreateStatic(blinker, sizeof(blinker), NORMALPRIO+1, Thread1, NULL);

	palSetGroupMode(GPIOA, PAL_PORT_BIT(0), 0, PAL_MODE_INPUT_ANALOG);
	adcStart(&ADCD1, NULL);

  	/*
   	* Linear conversion.
   	*/
  	adcConvert(&ADCD1, &adcgrpcfg,(adcsample_t*) samples1, ADC_GRP1_BUF_DEPTH);
  	chThdSleepMilliseconds(1000);

	uint16_t mean = 0;

	while (true) 
	{
 		/*
     		* Making a request
     		*/
		adcConvert(&ADCD1, &adcgrpcfg, samples1, ADC_GRP1_BUF_DEPTH);
		mean=0;		
		for(int j = 0 ; j < ADC_GRP1_BUF_DEPTH ; j++) {
			mean += samples1[j];
		}	
		mean /= ADC_GRP1_BUF_DEPTH;
		
		mean= ((mean*3*4.8)+30);
		TEMP = HR = CHECK_SUM = tmp = bit_counter = 0;
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
		
		#if ANSI_ESCAPE_CODE_ALLOWED
    		//chprintf(chp3, "\033[2J\033[1;1H");
		#endif
    		icuStopCapture(&ICUD1);
    		icuStop(&ICUD1);
		delay= TEMP*(-10)+500;

		chprintf(chp3, "%d,%d,%d\n", TEMP, HR, mean);



	}
	return 0;
}
