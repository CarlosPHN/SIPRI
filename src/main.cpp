#include <Arduino.h>
#include <stdio.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "SIPRI.pb.h"

#include "pb_common.h"
#include "pb.h"
#include "pb_encode.h"

const char *ssid = "SSID";
const char *password = "PASSWORD";

#define DHT_TOPIC "sipri/dht"
#define ENGINE_TOPIC "sipri/engine"
#define LIGHT_TOPIC "sipri/light"
#define BROKER_IP "ciberfisicos.ddns.net"
#define BROKER_PORT 2883

#define DHT11_READ_PRIORITY 1
#define DHT11_READ_DELAY 20000

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(D4, DHT11);

TaskHandle_t xTaskDHT11;

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

void sendDHT11(float currentTemperature, float currrentHumidity)
{
  uint8_t buffer[200];
  dhtMessage message = dhtMessage_init_zero;
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  message.temperature = currentTemperature;
  message.has_temperature = true;
  message.humidity = currrentHumidity;
  message.has_humidity = true;
  bool status = pb_encode(&stream, dhtMessage_fields, &message);
  if (!status)
  {
    Serial.println("Failed to encode");
    return;
  }
  client.publish(DHT_TOPIC, buffer, stream.bytes_written);
}

void vTaskPeriodicDHT11(void *pvParam)
{
  const char *msg = "vTaskPeriodicTemperature is running\r\n";
  portTickType xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    float currentTemperature = dht.readTemperature();
    float currentHumidity = dht.readHumidity();
    Serial.printf("{\"Temperature\": %f, \"Humidity\": %f}\n", currentTemperature, currentHumidity);
    sendDHT11(currentTemperature, currentHumidity);
    vTaskDelayUntil(&xLastWakeTime, (DHT11_READ_DELAY / portTICK_RATE_MS));
  }
  vTaskDelete(NULL);
}

void app(void)
{
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    Serial.println("Scheduler is running");
  xTaskCreate(vTaskPeriodicDHT11, "vTaskPeriodicLight", 4096, NULL, DHT11_READ_PRIORITY, &xTaskDHT11);
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

void loop()
{
  delay(3000);
  uint8_t buffer[200];
  engineMessage message = engineMessage_init_zero;
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  message.engine = 1;
  message.has_engine = true;
  message.cover = 0;
  message.has_cover = true;
  bool status = pb_encode(&stream, engineMessage_fields, &message);
  if (!status)
  {
    Serial.println("Failed to encode");
    return;
  }
  client.publish(ENGINE_TOPIC, buffer, stream.bytes_written);
}