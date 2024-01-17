#include <Arduino.h>
#include "driver/mcpwm.h"

class ServoMotor
{

public:
  void setup()
  {
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_NUM_14);
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;
    pwm_config.cmpr_a = 0;
    pwm_config.cmpr_b = 0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
  }

  void up()
  {
    for (float duty_cycle = 2.5; duty_cycle <= 12.5; duty_cycle += 0.1)
    {
      mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty_cycle);
      delay(100);
    }
  }

  void down()
  {
    for (float duty_cycle = 12.5; duty_cycle >= 2.5; duty_cycle -= 0.1)
    {
      mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty_cycle);
      delay(100);
    }
  }
};