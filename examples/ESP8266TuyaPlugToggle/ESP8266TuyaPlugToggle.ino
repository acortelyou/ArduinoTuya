#include <ESP8266WiFiMulti.h>

#define TUYA_DEBUG
#include "ArduinoTuya.h"

ESP8266WiFiMulti WiFiMulti;

TuyaDevice plug = TuyaDevice("01200885ecfabc87b0f9", "fd351bcdb819492f", "192.168.1.159");

void setup() {

  // Serial
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("WIFI", "PASSWORD");
  Serial.print("Waiting for connection...");
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" ready.");
  delay(1000);    

  // Set plug state
  Serial.println();
  int result = plug.set(false);
  Serial.print("SET Plug: ");
  Serial.print(plug.state() ? "ON" : "OFF");
  printResult(result);
  delay(5000);

}

void loop() {

  // Get plug state
  Serial.println();
  int result = plug.get();
  Serial.print("GET Plug: ");
  Serial.print(plug.state() ? "ON" : "OFF");
  printResult(result);
  delay(5000);
  
  // Toggle plug state
  Serial.println();
  result = plug.toggle();
  Serial.print("SET Plug: ");
  Serial.print(plug.state() ? "ON" : "OFF");
  printResult(result);
  delay(5000);

}

void printResult(int result) {
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
}
