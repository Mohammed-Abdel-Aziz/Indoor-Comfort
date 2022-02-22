#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <Arduino_JSON.h>
#include "DHT.h"

#define DHTPIN 2 // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // DHT 11

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor.

char ssid[] = "Your-wifi-name";
char pass[] = "Your-wifi-name";

char MQTT_SERVER[] = "broker.hivemq.com";
int  MQTT_SERVER_PORT = 1883;
char MQTT_USERNAME[] = "your-mqtt-username";
char MQTT_PASSWORD[] = "your-mqtt-password";
char MQTT_DEVICE_NAME[] = "NodeMCU";

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

int ledPin = 16;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect(MQTT_DEVICE_NAME, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  //you can change the topic
  client.subscribe("Mqtt/Node/bedRoom/");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if (payload == "Off") {
    digitalWrite(ledPin, LOW);
  } else if (payload == "On") {
    digitalWrite(ledPin, HIGH);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  dht.begin();

  WiFi.begin(ssid, pass);

  client.begin(MQTT_SERVER, MQTT_SERVER_PORT, net);
  client.onMessage(messageReceived);

  connect();
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

  // publish a message roughly every second 1000 = 1s.
  // publish a message roughly every second 30000 = 30s.
  if (millis() - lastMillis > 30000) {
    lastMillis = millis();

    float h = dht.readHumidity();
    String hum = String(h, 2);
    float t = dht.readTemperature();
    String tem = String(t, 2);

    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }
    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.print(F("°C "));

    JSONVar myObject;
    myObject["temp"] = tem +  " °C";
    myObject["hum"] = hum +  " %";

    // JSON.stringify(myVar) can be used to convert the json var to a String
    String jsonString = JSON.stringify(myObject);
    client.publish("Mqtt/Node/", jsonString);
    Serial.println(jsonString);
  }
  client.onMessage(messageReceived);
}
