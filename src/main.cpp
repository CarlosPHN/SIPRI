#include <Arduino.h>
#include <stdio.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <driver/adc.h>
#include "SIPRI.pb.h"

#include "pb_common.h"
#include "pb.h"
#include "pb_encode.h"
#include "servoMotor.cpp"

#define DHT_TOPIC "sipri/dht"
#define ENGINE_TOPIC "sipri/engine"
#define ROTATION_TOPIC "sipri/rotation"
#define LIGHT_TOPIC "sipri/light"

#define BROKER_IP "ciberfisicos.ddns.net"
#define BROKER_PORT 2883

#define DHT_PRIORITY 2
#define ENGINE_PRIORITY 1
#define ROTATION_PRIORITY 3
#define LIGHT_PRIORITY 4

#define DHT_DELAY 10000
#define ENGINE_DELAY 15000
#define ROTATION_DELAY 25000
#define LIGHT_DELAY 20000

#define redLed D9
#define greenLed D10
#define blueLed D11

#define HIGH_TEMPERATURE D12
#define LOW_TEMPERATURE D13

#define DHT_HUMIDITY_TRESHOLD 70
#define LIGHT_THRESHOLD 500

#define ROTATION A0
#define LIGHT A1

#define MIN_TEMPERATURE_VALUE 0
#define MAX_TEMPERATURE_VALUE 60

#define TIMER_ALARM_VALUE_SECONDS 360
#define DEEP_MODE_TIME_SECONDS 60
#define uS_TO_S_FACTOR 1000000

hw_timer_t *timer = NULL;

const char *ssid = "YourWifi";
const char *password = "YourPassword";

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(D4, DHT11);
ServoMotor myServo;

TaskHandle_t xTaskDHT11;
TaskHandle_t xTaskEngine;
TaskHandle_t xTaskRotation;
TaskHandle_t xTaskLight;

QueueHandle_t wifiSemaphore;
SemaphoreHandle_t writeTemperatureSemaphore;

// DTH11
float currentTemperature;
float currentHumidity;

// ENGINE
int currentServoStatus = 0; // 0 = down, 1 = up
int currentCoverStatus = 0; // 0 = down, 1 = up

// ROTATION
int currentThresholdTemperature;

// LIGHT
int currentLight;

// SERVO
ServoMotor servoMotor;

void wifiConnect()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void mqttConnect()
{
  client.setServer(BROKER_IP, BROKER_PORT);
  while (!client.connected())
  {
    Serial.print("MQTT connecting ...");
    if (client.connect("ESP32Client1"))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void connect()
{
  wifiConnect();
  mqttConnect();
}

void disconnect()
{
  client.disconnect();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void IRAM_ATTR Timer_handleInterrupt()
{
  esp_deep_sleep_start();
}

void controlRGBLeds(int red, int green, int blue)
{
  digitalWrite(redLed, red);
  digitalWrite(greenLed, green);
  digitalWrite(blueLed, blue);
}

void writeTemperature()
{
  if (xSemaphoreTake(writeTemperatureSemaphore, portMAX_DELAY) == pdTRUE)
  {
    if (currentTemperature < currentThresholdTemperature)
    {
      if (!digitalRead(LOW_TEMPERATURE))
      {
        digitalWrite(LOW_TEMPERATURE, HIGH);
      }
      if (digitalRead(HIGH_TEMPERATURE))
      {
        digitalWrite(HIGH_TEMPERATURE, LOW);
      }
    }
    else
    {
      if (!digitalRead(HIGH_TEMPERATURE))
      {
        digitalWrite(HIGH_TEMPERATURE, HIGH);
      }
      if (digitalRead(LOW_TEMPERATURE))
      {
        digitalWrite(LOW_TEMPERATURE, LOW);
      }
    }
    xSemaphoreGive(writeTemperatureSemaphore);
  }
}

void sendDHT11()
{
  uint8_t buffer[200];
  dhtMessage message = dhtMessage_init_zero;
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  message.temperature = currentTemperature;
  message.has_temperature = true;
  message.humidity = currentHumidity;
  message.has_humidity = true;
  bool status = pb_encode(&stream, dhtMessage_fields, &message);
  if (!status)
  {
    Serial.println("Failed to encode");
    return;
  }
  if (xSemaphoreTake(wifiSemaphore, portMAX_DELAY) == pdTRUE)
  {
    client.publish(DHT_TOPIC, buffer, stream.bytes_written);
    xSemaphoreGive(wifiSemaphore);
  }
}

void vTaskPeriodicDHT11(void *pvParam)
{
  const char *msg = "vTaskPeriodicTemperature is running\r\n";
  portTickType xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    printf(msg);
    currentTemperature = dht.readTemperature();
    currentHumidity = dht.readHumidity();
    Serial.printf("{\"Temperature\": %f, \"Humidity\": %f}\n", currentTemperature, currentHumidity);
    writeTemperature();
    sendDHT11();
    vTaskDelayUntil(&xLastWakeTime, (DHT_DELAY / portTICK_RATE_MS));
  }
  vTaskDelete(NULL);
}

void sendEngine()
{
  uint8_t buffer[200];
  engineMessage message = engineMessage_init_zero;
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  message.engine = currentServoStatus;
  message.has_engine = true;
  message.cover = currentCoverStatus;
  message.has_cover = true;
  bool status = pb_encode(&stream, engineMessage_fields, &message);
  if (!status)
  {
    Serial.println("Failed to encode");
    return;
  }
  if (xSemaphoreTake(wifiSemaphore, portMAX_DELAY) == pdTRUE)
  {
    client.publish(ENGINE_TOPIC, buffer, stream.bytes_written);
    xSemaphoreGive(wifiSemaphore);
  }
}

void liftCover()
{
  if (currentCoverStatus == 0)
  {
    // Servo ON
    currentServoStatus = 1;
    sendEngine();
    servoMotor.up();
    delay(1000);
    // Servo OFF and Cover UP
    currentServoStatus = 0;
    currentCoverStatus = 1;
    sendEngine();
  }
}

void lowerCover()
{
  if (currentCoverStatus == 1)
  {
    // Servo ON
    currentServoStatus = 1;
    sendEngine();
    servoMotor.down();
    delay(1000);
    // Servo OFF and Cover DOWN
    currentServoStatus = 0;
    currentCoverStatus = 0;
    sendEngine();
  }
}

void vTaskPeriodicEngine(void *pvParam)
{
  const char *msg = "vTaskPeriodicEngine is running\r\n";
  portTickType xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    printf(msg);
    if (currentHumidity >= DHT_HUMIDITY_TRESHOLD)
    {
      liftCover();
    }
    else
    {
      lowerCover();
    }
    vTaskDelayUntil(&xLastWakeTime, (ENGINE_DELAY / portTICK_RATE_MS));
  }
  vTaskDelete(NULL);
}

void sendRotation()
{
  uint8_t buffer[200];
  rotationMessage message = rotationMessage_init_zero;
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  message.desiredTemperature = currentThresholdTemperature;
  message.has_desiredTemperature = true;
  bool status = pb_encode(&stream, rotationMessage_fields, &message);
  if (!status)
  {
    Serial.println("Failed to encode");
    return;
  }
  if (xSemaphoreTake(wifiSemaphore, portMAX_DELAY) == pdTRUE)
  {
    client.publish(ROTATION_TOPIC, buffer, stream.bytes_written);
    xSemaphoreGive(wifiSemaphore);
  }
}

void vTaskPeriodicRotation(void *pvParam)
{
  const char *msg = "vTaskPeriodicRotation is running\r\n";
  portTickType xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    printf(msg);
    if (xSemaphoreTake(wifiSemaphore, portMAX_DELAY) == pdTRUE)
    {
      disconnect();
      currentThresholdTemperature = map(analogRead(ROTATION), 0, 1023, MIN_TEMPERATURE_VALUE, MAX_TEMPERATURE_VALUE);
      connect();
      xSemaphoreGive(wifiSemaphore);
    }
    Serial.printf("{\"Rotation\": %d}\n", currentThresholdTemperature);
    writeTemperature();
    sendRotation();
    vTaskDelayUntil(&xLastWakeTime, (ROTATION_DELAY / portTICK_RATE_MS));
  }
  vTaskDelete(NULL);
}

void controLight()
{
  if (currentLight < LIGHT_THRESHOLD)
  {
    controlRGBLeds(255, 255, 255);
  }
  else
  {
    controlRGBLeds(0, 0, 0);
  }
}

void sendLight()
{
  uint8_t buffer[200];
  lightMessage message = lightMessage_init_zero;
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  message.light = currentLight;
  message.has_light = true;
  bool status = pb_encode(&stream, lightMessage_fields, &message);
  if (!status)
  {
    Serial.println("Failed to encode");
    return;
  }
  if (xSemaphoreTake(wifiSemaphore, portMAX_DELAY) == pdTRUE)
  {
    client.publish(LIGHT_TOPIC, buffer, stream.bytes_written);
    xSemaphoreGive(wifiSemaphore);
  }
}

void vTaskPeriodicLight(void *pvParam)
{
  const char *msg = "vTaskPeriodicLight is running\r\n";
  portTickType xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    printf(msg);
    if (xSemaphoreTake(wifiSemaphore, portMAX_DELAY) == pdTRUE)
    {
      disconnect();
      currentLight = analogRead(LIGHT);
      connect();
      xSemaphoreGive(wifiSemaphore);
    }
    Serial.printf("{\"Light\": %d}\n", currentLight);
    controLight();
    sendLight();
    vTaskDelayUntil(&xLastWakeTime, (LIGHT_DELAY / portTICK_RATE_MS));
  }
  vTaskDelete(NULL);
}

void app(void)
{
  wifiSemaphore = xSemaphoreCreateMutex();
  writeTemperatureSemaphore = xSemaphoreCreateMutex();
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    Serial.println("Scheduler is running");
  xTaskCreate(vTaskPeriodicDHT11, "vTaskPeriodicDHT", 4096, NULL, DHT_PRIORITY, &xTaskDHT11);
  xTaskCreate(vTaskPeriodicEngine, "vTaskPeriodicEngine", 4096, NULL, ENGINE_PRIORITY, &xTaskEngine);
  xTaskCreate(vTaskPeriodicLight, "vTaskPeriodicLight", 4096, NULL, LIGHT_PRIORITY, &xTaskLight);
  xTaskCreate(vTaskPeriodicRotation, "vTaskPeriodicRotation", 4096, NULL, ROTATION_PRIORITY, &xTaskRotation);
}

void setup()
{
  Serial.begin(115200);
  pinMode(HIGH_TEMPERATURE, OUTPUT);
  pinMode(LOW_TEMPERATURE, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(ROTATION, INPUT);
  pinMode(LIGHT, INPUT);
  analogReadResolution(10);
  dht.begin();
  servoMotor.setup();
  delay(4000);
  connect();
  esp_sleep_enable_timer_wakeup(DEEP_MODE_TIME_SECONDS * uS_TO_S_FACTOR);
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &Timer_handleInterrupt, true);
  timerAlarmWrite(timer, TIMER_ALARM_VALUE_SECONDS * uS_TO_S_FACTOR, true);
  timerAlarmEnable(timer);
  app();
}

void loop()
{
}