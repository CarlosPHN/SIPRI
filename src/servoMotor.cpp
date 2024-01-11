#include <Arduino.h>
#include "driver/mcpwm.h"

class ServoMotor {
private:
  bool ENCENDIDO;

public:
  void setup() {
    ENCENDIDO = false;

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_NUM_14);

    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;
    pwm_config.cmpr_a = 0;
    pwm_config.cmpr_b = 0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);  
  }

  void start() {
    mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    
    this->setEncendido(true);
  }

  void stop() {
    mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    this->setEncendido(false);
  }

  void subir() {
    if (this->isEncendido()) {
      mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 10.0);
    }else{
      Serial.print("Está apagado y no funciona");
    }
  }

  void bajar() {
    if (this->isEncendido()) {
      mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 5.0);
    }else{
      Serial.print("Está apagado y no funciona");
    }
  }

  bool isEncendido() {
    return ENCENDIDO;
  }

  void setEncendido(bool estado) {
    ENCENDIDO = estado;
  }
};

// Ejemplo de prueba


// ServoMotor myServo;

// volatile bool encendido = false;
// const int botonOff = D3;

// void IRAM_ATTR off_handleInterrupt(){
//   encendido = !encendido;
//   Serial.print("CAMBIA DE ESTADO:\n");
//   Serial.print(encendido);
// }

// void setup() {
//   pinMode(botonOff, INPUT);
//   attachInterrupt(digitalPinToInterrupt(botonOff), &off_handleInterrupt, FALLING);
//   Serial.begin(9600);
//   myServo.setup();
// }

// void encenderMotor(){

//   while(encendido){
//     myServo.start();
//     delay(250);
//     myServo.subir();
//     delay(1500);
//     Serial.print("Subiendo...");

//   }
// }

// void apagarMotor(){
//     Serial.print("Bajando...");
//     myServo.bajar();
//     delay(750);
//     myServo.stop();
//     delay(750);
// }
// void loop() {
//   // Esperar un tiempo antes de mostrar el menú nuevamente

//   if (encendido){
//     encenderMotor();
//   }else{
//     apagarMotor();
//   }
// }