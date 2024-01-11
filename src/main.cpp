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

const char *ssid = "DIGIFIBRA-zbYR";
const char *password = "Hk9GzXCdDUeb";

#define DHT_TOPIC "sipri/dht"
#define ENGINE_TOPIC "sipri/engine"
#define LIGHT_TOPIC "sipri/light"

#define BROKER_IP "ciberfisicos.ddns.net"
#define BROKER_PORT 2883

#define DHT_DELAY 20000
#define ENGINE_DELAY 10000

#define UMBRAL_LUZ_ON 2000
#define LIGHT_PIN A1

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(D4, DHT11);
ServoMotor myServo;

TaskHandle_t xTaskDHT11;
TaskHandle_t xTaskEngine;

volatile bool systemOn = true;
volatile int intensidadLuz;
int estadoServo = 0;
int estadoTapa = 0;
float currentTemperature;
float currentHumidity;

enum SystemState
{
  OFF,
  BOILER_ON,
  BOILER_OFF
};

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

void subirTapa(float humidity)
{
  estadoServo = 0;
  if (estadoTapa == 0)
  {
    sendEngine();
    estadoServo = 1;
    myServo.start();
    sendEngine();
    delay(1000);
    myServo.subir();
    delay(5000);
    myServo.stop();
    estadoTapa = 1;
    sendEngine();
    delay(1000);
  }
}

void bajarTapa(float humidity)
{
  estadoServo = 0;
  if (estadoTapa == 1)
  {
    sendEngine();
    estadoServo = 1;
    myServo.start();
    sendEngine();
    delay(1000);
    myServo.bajar();
    delay(1000);
    myServo.stop();
    estadoTapa = 0;
    sendEngine();
    delay(1000);
  }
}

void sendEngine()
{
  uint8_t buffer[200];
  engineMessage message = engineMessage_init_zero;
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  message.engine = estadoServo;
  message.has_engine = true;
  message.cover = estadoTapa;
  message.has_cover = true;
  bool status = pb_encode(&stream, engineMessage_fields, &message);
  if (!status)
  {
    Serial.println("Failed to encode");
    return;
  }
  client.publish(ENGINE_TOPIC, buffer, stream.bytes_written);
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
  client.publish(DHT_TOPIC, buffer, stream.bytes_written);
}

void vTaskPeriodicEngine(void *pvParam)
{
  const char *msg = "vTaskPeriodicEngine is running\r\n";
  portTickType xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    if (currentHumidity <= 23.0)
    {
      subirTapa(currentHumidity);
    }
    else
    {
      bajarTapa(currentHumidity);
    }
    vTaskDelayUntil(&xLastWakeTime, (20000 / portTICK_RATE_MS));
  }
  vTaskDelete(NULL);
}

void vTaskPeriodicDHT11(void *pvParam)
{
  const char *msg = "vTaskPeriodicTemperature is running\r\n";
  portTickType xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    currentTemperature = dht.readTemperature();
    currentHumidity = dht.readHumidity();
    Serial.printf("{\"Temperature\": %f, \"Humidity\": %f}\n", currentTemperature, currentHumidity);
    sendDHT11();
    vTaskDelayUntil(&xLastWakeTime, (DHT_DELAY / portTICK_RATE_MS));
  }
  vTaskDelete(NULL);
}

void app(void)
{
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    Serial.println("Scheduler is running");
  xTaskCreate(vTaskPeriodicEngine, "vTaskPeriodicEngine", 4096, NULL, 2, &xTaskEngine);
  xTaskCreate(vTaskPeriodicDHT11, "vTaskPeriodicDHT", 4096, NULL, 3, &xTaskDHT11);
}

void setup()
{
  Serial.begin(115200);
  dht.begin();
  delay(4000);
  wifiConnect();
  mqttConnect();
  app();
}

void loop() {}