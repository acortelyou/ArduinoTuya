#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wiFiMulti;

#include <ArduinoTuya.h>
TuyaDevice plug("01200885ecfabc87b0f9", "fd351bcdb819492f", "192.168.1.159");

void setup() {

  // Serial
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println();

  // WiFi
  WiFi.mode(WIFI_STA);
  wiFiMulti.addAP("WIFI", "thepasswordispassword");
  Serial.print("Waiting for connection...");
  while (wiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" ready.");
  delay(1000);

  // Set plug state
  Serial.println();
  plug.set(TUYA_OFF);
  Serial.print("SET Plug: ");
  printPlugState();
  delay(5000);

}

void loop() {

  // Get plug state
  Serial.println();
  plug.get();
  Serial.print("GET Plug: ");
  printPlugState();
  delay(5000);
  
  // Toggle plug state
  Serial.println();
  plug.toggle();
  Serial.print("SET Plug: ");
  printPlugState();
  delay(5000);

}


void printPlugState() {
  auto error = plug.error();
  if (!error) {
    Serial.print(plug.state() ? "ON" : "OFF");
    Serial.println(" (OK)");
  } else {
    Serial.print("ERROR ");
    Serial.println(error);
  }
}
