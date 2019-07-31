// ArduinoTuya
// Copyright Alex Cortelyou 2018
// MIT License

#ifndef TUYA_H
#define TUYA_H

#ifndef TUYA_PORT_DEFAULT
#define TUYA_PORT_DEFAULT     6668
#endif

#ifndef TUYA_VERSION_DEFAULT
#define TUYA_VERSION_DEFAULT  "3.1"
#endif

#ifndef TUYA_TIMEOUT
#define TUYA_TIMEOUT          10000
#endif

#ifndef TUYA_RETRY_DELAY
#define TUYA_RETRY_DELAY      5000
#endif

#ifndef TUYA_RETRY_COUNT
#define TUYA_RETRY_COUNT      4
#endif

#ifdef TUYA_DEBUG
 #define DEBUG_PRINT(x)       Serial.print (x)
 #define DEBUG_PRINTDEC(x)    Serial.print (x, DEC)
 #define DEBUG_PRINTHEX(x)    Serial.print (x, HEX)
 #define DEBUG_PRINTLN(x)     Serial.println (x)
 #define DEBUG_PRINTJSON(x)   serializeJson(doc, Serial);Serial.println()
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTHEX(x)
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINTJSON(x)
#endif

#define TUYA_BLOCK_LENGTH     16
#define TUYA_PREFIX_LENGTH    16
#define TUYA_SUFFIX_LENGTH    8

#define ECB 1
#define CBC 0
#define CTR 0

#ifndef ARDUINO
 #include <stdint.h>
#elif ARDUINO >= 100
 #include "Arduino.h"
 #include "Print.h"
#else
 #include "WProgram.h"
#endif

#include <aes.hpp>
#include <base64.h>
#include <MD5Builder.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

typedef enum
{
  TUYA_OFF              = (0),
  TUYA_ON               = (1)
} tuya_state_t;

typedef enum
{
  TUYA_OK               = (0),
  TUYA_ERROR_UNINIT     = (1),
  TUYA_ERROR_SOCKET     = (2),
  TUYA_ERROR_PREFIX     = (3),
  TUYA_ERROR_LENGTH     = (4),
  TUYA_ERROR_SUFFIX     = (5),
  TUYA_ERROR_PARSE      = (6),
  TUYA_ERROR_ARGS       = (7)
} tuya_error_t;

class TuyaDevice {

  protected:

    AES_ctx _aes;
    MD5Builder _md5;
    WiFiClient _client;

    const byte prefix[16] = { 0, 0, 85, 170, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    const byte suffix[8]  = { 0, 0, 0, 0, 0, 0, 170, 85 };

    String _id;
    String _key;
    const char* _host;
    uint16_t _port;
    String _version;

    tuya_state_t _state = TUYA_OFF;
    tuya_error_t _error = TUYA_ERROR_UNINIT;

    void initGetRequest(JsonDocument &jsonRequest);
    void initSetRequest(JsonDocument &jsonRequest);
    String createPayload(JsonDocument &jsonRequest, bool encrypt = true);
    String sendCommand(String &payload, byte command);
    
  public:

    inline TuyaDevice(const char* id, const char* key, const char* host = NULL, uint16_t port = TUYA_PORT_DEFAULT, const char* version = TUYA_VERSION_DEFAULT) {
      _id = String(id);
      _key = String(key);
      _host = host;
      _port = port;
      _version = String(version);
      AES_init_ctx(&_aes, (uint8_t*) key);
    }

    inline tuya_state_t state() { return _state; }
    inline tuya_error_t error() { return _error; }

    tuya_error_t get();
    tuya_error_t set(bool state);
    tuya_error_t toggle();

};

class TuyaPlug : public TuyaDevice {
  
  public:
  
    TuyaPlug(const char* id, const char* key, const char* host = NULL, uint16_t port = TUYA_PORT_DEFAULT, const char* version = TUYA_VERSION_DEFAULT) : TuyaDevice(id,key,host,port,version) {}    

};

class TuyaBulb : public TuyaDevice {
  
  public:
  
    TuyaBulb(const char* id, const char* key, const char* host = NULL, uint16_t port = TUYA_PORT_DEFAULT, const char* version = TUYA_VERSION_DEFAULT) : TuyaDevice(id,key,host,port,version) {}
    
    tuya_error_t setColorRGB(byte r, byte g, byte b);
    tuya_error_t setColorHSV(byte h, byte s, byte v);
    tuya_error_t setWhite(byte brightness, byte temp);

  private:

    inline byte asByte(float n) { return n <= 0.0 ? 0 : floor(n >= 1.0 ? 255 : n * 256.f); }
    inline float asFloat(byte n) { return n * (1.f/255.f); }        
    inline float step(float e, float x) { return x < e ? 0.0 : 1.0; }
    inline float mix(float a, float b, float t) { return a + (b - a) * t; }

};

#endif
