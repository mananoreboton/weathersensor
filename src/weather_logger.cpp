#include <Arduino.h>
#include <DHT.h> 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <stdlib.h>
#include <ProjectSecrets.h> //Create this file in /include folder

unsigned long currentTime = millis();

/* Set these to your desired credentials. */
const char *ssid = WIFI_SSID; //Enter your WIFI ssid
const char *password = WIFI_PASSWORD; //Enter your WIFI password
WiFiClient wifiClient;
/***************************/

PubSubClient pubSubClient(wifiClient);
unsigned long publishMQTTMessagePeriod = 3000;
unsigned long publishMQTTMessageLastExecutionTime = currentTime + publishMQTTMessagePeriod;
unsigned long receiveMQTTMessagePeriod = 1000;
unsigned long receiveMQTTMessageLastExecutionTime = currentTime + receiveMQTTMessagePeriod;
/***************************/
#define DHTTYPE DHT11 // DHT 11
#define DHTPIN 0          //Pin D3 printed in the Nodemcu = Pin 0 in the Arduino IDE
DHT dht(DHTPIN, DHTTYPE);
float humidity = 0.000;
float temperature = 0.000;
float heatIndex = 0.000;
/***************************/
void setupDht();
void setupWiFi();
void setupMQTT();
void readDht();
void publishMQTTMessage();
bool isScheduled(unsigned long &lastExecutionTime, unsigned long period);
/***************************/
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  delay(3000);
  Serial.begin(115200);
  Serial.println();
  setupDht();
  setupWiFi();  
  setupMQTT();
}
void loop() {
  currentTime = millis();
  readDht();
  if(isScheduled(publishMQTTMessageLastExecutionTime, publishMQTTMessagePeriod)) {
    publishMQTTMessage();
  }
  if(isScheduled(receiveMQTTMessageLastExecutionTime, receiveMQTTMessagePeriod)) {
    pubSubClient.loop();
  }
}
/*****************************/
void setupDht() {
  delay(10);
  dht.begin();
}
void setupWiFi() {
  Serial.print("[WIFI]\t Configuring connection...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("[WIFI]\t Connecting...");
  }
  Serial.println("");
  Serial.println("[WIFI]\t Connected");
  Serial.println("[WIFI]\t IP address: ");
  Serial.println(WiFi.localIP());  
}
void receiveMQTTMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("[MQTT]\t Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  } 
}
void setupMQTT() {
  while (!pubSubClient.connected()) {
    pubSubClient.setServer(MQTT_SERVER, MQTT_PORT);
    pubSubClient.setCallback(receiveMQTTMessage);
    Serial.println("[MQTT]\t Attempting MQTT connection...");
    if (pubSubClient.connect("ESP8266Client")) {
      Serial.println("[MQTT]\t Connected");
      pubSubClient.publish("clients/connected", "123");
      pubSubClient.subscribe("feedback/popular");
    } else {
      Serial.println("[MQTT] Connection failed, rc=");
      Serial.print(pubSubClient.state());
      delay(2000);
    }
  }

}

void publishMQTTMessage() {
  if (pubSubClient.connected()) {
    String buf(temperature, 3);
    bool published = pubSubClient.publish("temp", buf.c_str());
    if(published) {
      Serial.printf("[MQTT] Message published: %s\n", buf.c_str());
    } else {
      Serial.printf("[MQTT] Error publishing message: %s\n", buf.c_str());
    }
  } else {
    setupMQTT();
  }
}

bool isScheduled(unsigned long &lastExecutionTime, unsigned long period) {
  bool isTime = currentTime >= lastExecutionTime;
  if(isTime) {
    Serial.println(lastExecutionTime);
    lastExecutionTime += period;
  }
  currentTime = millis();
  return isTime;
}
void readDht() {
  humidity = dht.readHumidity(false);
  temperature = dht.readTemperature(false, false);
  heatIndex = dht.computeHeatIndex(false);
  if (isnan(humidity) || isnan(temperature) || isnan(heatIndex)) {
    Serial.println("[DHT]\t Failed to read from DHT sensor!");
    return;
  }
}

