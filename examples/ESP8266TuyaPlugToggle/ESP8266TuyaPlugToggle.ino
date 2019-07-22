#include <ESP8266WiFiMulti.h>

#define TUYA_DEBUG
#include "ArduinoTuya.h"

ESP8266WiFiMulti WiFiMulti;

TuyaDevice plug = TuyaDevice("01200885ecfabc87b0f9", "fd351bcdb819492f", "192.168.1.159");

bool state = false;

void setup() {

  // Set up serial  
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println();

  // Set up wifi
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("WIFI", "PASSWORD");
    
  Serial.print("Waiting for connection...");
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" ready.");

  delay(1000);    
}

void loop() {

  // Get plug state
  Serial.println();
  int result = plug.get();  
  Serial.print("GET Plug: ");
  switch (result) {
    case TUYA_ON:
      state = true;
      Serial.println("ON");
      break;
    case TUYA_OFF:
      state = false;
      Serial.println("OFF");
      break;
    default:
      Serial.print("ERROR ");
      Serial.println(result);
      break;
  }
  
  delay(5000);
  
  // Toggle plug state
  state = !state;
  Serial.println();
  result = plug.set(state);
  Serial.print("SET Plug: ");
  Serial.print(state ? "ON" : "OFF");
  switch (result) {
    case TUYA_OK:
      Serial.println(" (OK)");
      break;
    default:
      Serial.print(" (ERROR ");
      Serial.print(result);
      Serial.println(")");
      break;
  }

  delay(25000);
}

