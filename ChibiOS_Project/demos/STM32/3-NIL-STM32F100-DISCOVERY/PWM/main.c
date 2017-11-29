#include "ch.h"
#include "hal.h"


static PWMConfig pwmcfg = {
  10000,                                    /* 10kHz PWM clock frequency.   */
  100,                                    /* Initial PWM period 1S.       */
  NULL,
  {
   {PWM_OUTPUT_ACTIVE_HIGH, NULL},
   {PWM_OUTPUT_ACTIVE_HIGH, NULL},
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL}
  },
  0,
  0,
#if STM32_PWM_USE_ADVANCED
  0
#endif
};

int main(void) {



  halInit();
  chSysInit();


  pwmStart(&PWMD1, &pwmcfg);
  pwmEnablePeriodicNotification(&PWMD1);
  palSetPadMode(GPIOA, 8, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
  chThdSleepMilliseconds(2000);

  /*
   * Starts the PWM channel 0 using 75% duty cycle.
   */
  pwmEnableChannel(&PWMD1, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD1, 7500));
  pwmEnableChannelNotification(&PWMD1, 0);
  chThdSleepMilliseconds(5000);

  /*
   * Changes the PWM channel 0 to 50% duty cycle.
   */
  pwmEnableChannel(&PWMD1, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD1, 5000));
  chThdSleepMilliseconds(5000);

  /*
   * Changes the PWM channel 0 to 25% duty cycle.
   */
  pwmEnableChannel(&PWMD1, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD1, 2500));
  chThdSleepMilliseconds(5000);

  /*
   * Changes PWM period to half second the duty cycle becomes 50%
   * implicitly.
   */
  pwmChangePeriod(&PWMD1, 5000);
  chThdSleepMilliseconds(5000);

  /*
   * Disables channel 0 and stops the drivers.
   */
  pwmDisableChannel(&PWMD1, 0);
  pwmStop(&PWMD1);




  while (true) {
    chThdSleepMilliseconds(500);
  }
  return 0;
}
