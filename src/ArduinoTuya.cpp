#include "ArduinoTuya.h"

void TuyaDevice::initGetRequest(JsonDocument &jsonRequest) {
  jsonRequest["gwId"] = _id;  //device id
  jsonRequest["devId"] = _id; //device id
}

void TuyaDevice::initSetRequest(JsonDocument &jsonRequest) {
  jsonRequest["t"] = 0;       //epoch time (required but value doesn't appear to be used)
  jsonRequest["devId"] = _id; //device id
  jsonRequest.createNestedObject("dps");
  jsonRequest["uid"] = "";    //user id (required but value doesn't appear to be used)
}

String TuyaDevice::createPayload(JsonDocument &jsonRequest, bool encrypt) {

  // Serialize json request
  String jsonString;
  serializeJson(jsonRequest, jsonString);

  DEBUG_PRINT("REQUEST  ");
  DEBUG_PRINTLN(jsonString);
    
  if (!encrypt) return jsonString;

  // Determine lengths and padding
  const int jsonLength = jsonString.length();
  const int cipherPadding = TUYA_BLOCK_LENGTH - jsonLength % TUYA_BLOCK_LENGTH;
  const int cipherLength = jsonLength + cipherPadding;

  // Allocate encrypted data buffer
  byte cipherData[cipherLength];

  // Use PKCS7 padding mode
  memcpy(cipherData, jsonString.c_str(), jsonLength);
  memset(&cipherData[jsonLength], cipherPadding, cipherPadding);

  // AES ECB encrypt each block
  for (int i = 0; i < cipherLength; i += TUYA_BLOCK_LENGTH) {
    AES_ECB_encrypt(&_aes, &cipherData[i]);
  }

  // Base64 encode encrypted data
  String base64Data = base64::encode(cipherData, cipherLength, false);
  
  // Calculate MD5 hash signature
  _md5.begin();
  _md5.add("data=");
  _md5.add(base64Data);
  _md5.add("||lpv=");
  _md5.add(_version);
  _md5.add("||");
  _md5.add(_key);
  _md5.calculate();
  String md5 = _md5.toString().substring(8, 24);

  // Create signed payload
  String payload = String(_version + md5 + base64Data);
  
  DEBUG_PRINT("PAYLOAD  ");
  DEBUG_PRINTLN(payload);

  return payload;
}

String TuyaDevice::sendCommand(String &payload, byte command) {

  // Attempt to send command at least once
  int tries = 0;
  while (tries++ <= TUYA_RETRY_COUNT) {

    // Determine lengths and offsets
    const int payloadLength = payload.length();
    const int bodyOffset = TUYA_PREFIX_LENGTH;
    const int bodyLength = payloadLength + TUYA_SUFFIX_LENGTH;    
    const int suffixOffset = TUYA_PREFIX_LENGTH + payloadLength;    
    const int requestLength = TUYA_PREFIX_LENGTH + payloadLength + TUYA_SUFFIX_LENGTH;

    // Assemble request buffer
    byte request[requestLength];
    memcpy(request, prefix, 11);
    request[11] = command;
    request[12] = (byte) ((bodyLength>>24) & 0xFF);
    request[13] = (byte) ((bodyLength>>16) & 0xFF);
    request[14] = (byte) ((bodyLength>> 8) & 0xFF);
    request[15] = (byte) ((bodyLength>> 0) & 0xFF);
    memcpy(&request[bodyOffset], payload.c_str(), payloadLength);
    memcpy(&request[suffixOffset], suffix, TUYA_SUFFIX_LENGTH);

    // Connect to device
    _client.setTimeout(TUYA_TIMEOUT);
    if (!_client.connect(_host, _port)) {
      DEBUG_PRINTLN("TUYA SOCKET ERROR");
      _error = TUYA_ERROR_SOCKET;
      delay(TUYA_RETRY_DELAY);
      continue;
    }

    // Wait for socket to be ready for write
    while (_client.connected() && _client.availableForWrite() < requestLength) delay(10);

    // Write request to device
    _client.write(request, requestLength);

    // Wait for socket to be ready for read
    while (_client.connected() && _client.available() < 11) delay(10);

    // Read response prefix   (bytes 1 to 11)
    byte buffer[11];
    _client.read(buffer, 11);

    // Check prefix match
    if (memcmp(prefix, buffer, 11) != 0) {
      DEBUG_PRINTLN("TUYA PREFIX MISMATCH");
      _error = TUYA_ERROR_PREFIX;
      _client.stop();
      delay(TUYA_RETRY_DELAY);
      continue;
    }

    // Read response command  (byte 12) (ignored)
    _client.read(buffer, 1);

    // Read response length   (bytes 13 to 16)
    _client.read(buffer, 4);

    // Assemble big-endian response length
    size_t length = (buffer[0]<<24)|(buffer[1]<<16)|(buffer[2]<<8)|(buffer[3])-12;

    // Read response unknown  (bytes 17 to 20) (ignored)
    _client.read(buffer, 4);

    // Allocate response buffer
    byte response[length+1];
    memset(response, 0, length+1);

    // Read response          (bytes 21 to N-8)
    _client.read(response, length);
    
    // Read response suffix   (bytes N-7 to N)
    _client.read(buffer, 8);

    // Check last four bytes of suffix match
    if (memcmp(&suffix[4], &buffer[4], 4) != 0) {
      DEBUG_PRINTLN("TUYA SUFFIX MISMATCH");
      _error = TUYA_ERROR_SUFFIX;
      _client.stop();
      delay(TUYA_RETRY_DELAY);
      continue;
    }

    // Check length match
    if (_client.available() > 0) {
      DEBUG_PRINTLN("TUYA LENGTH MISMATCH");
      _error = TUYA_ERROR_LENGTH;
      _client.stop();
      delay(TUYA_RETRY_DELAY);
      continue;
    }

    // Close connection
    _client.stop();

    if (length > 0) {
      DEBUG_PRINT("RESPONSE ");
      DEBUG_PRINTLN((const char*)response);
    }
    
    _error = TUYA_OK;
    return String((const char*)response);
  }

  return String("");
}

int TuyaDevice::get() {

  // Allocate json objects
  StaticJsonDocument<512> jsonRequest;
  StaticJsonDocument<512> jsonResponse;
  
  // Build request
  initGetRequest(jsonRequest);
  
  String payload = createPayload(jsonRequest, false);

  String response = sendCommand(payload, 10);

  // Check for errors
  if (_error != TUYA_OK) return _error;

  // Deserialize json response
  auto error = deserializeJson(jsonResponse, response);
  if (error) return _error = TUYA_ERROR_PARSE;

  // Check response
  JsonVariant state = jsonResponse["dps"]["1"];
  if (state.isNull()) return _error = TUYA_ERROR_PARSE;

  _state = state.as<bool>() ? TUYA_ON : TUYA_OFF;
  return _error = TUYA_OK;
}

int TuyaDevice::set(bool state) {

  // Allocate json object
  StaticJsonDocument<512> jsonRequest;
  
  // Build request
  initSetRequest(jsonRequest);
  jsonRequest["dps"]["1"] = state;    //state
  jsonRequest["dps"]["2"] = 0;        //delay  
  
  String payload = createPayload(jsonRequest);
 
  String response = sendCommand(payload, 7);

  // Check for errors
  if (_error != TUYA_OK) return _error;
  if (response.length() != 0) return _error = TUYA_ERROR_LENGTH;

  _state = state;

  return _error = TUYA_OK;
}

int TuyaDevice::toggle() {
  return set(!_state);
}

int TuyaBulb::setColorRGB(byte r, byte g, byte b) {
  //https://gist.github.com/postspectacular/2a4a8db092011c6743a7
  float R = asFloat(r);
  float G = asFloat(g);
  float B = asFloat(b);
  float s = step(B, G);
  float px = mix(B, G, s);
  float py = mix(G, B, s);
  float pz = mix(-1.0, 0.0, s);
  float pw = mix(0.6666666, -0.3333333, s);
  s = step(px, R);
  float qx = mix(px, R, s);
  float qz = mix(pw, pz, s);
  float qw = mix(R, px, s);
  float d = qx - min(qw, py);
  float H = abs(qz + (qw - py) / (6.0 * d + 1e-10));
  float S = d / (qx + 1e-10);
  float V = qx;

  return setColorHSV(asByte(H), asByte(S), asByte(V));
}

int TuyaBulb::setColorHSV(byte h, byte s, byte v) {

  // Format color as hex string
  char hexColor[7];
  sprintf(hexColor, "%02x%02x%02x", h, s, v);

  // Allocate json object
  StaticJsonDocument<512> jsonRequest;
  
  // Build request
  initSetRequest(jsonRequest);
  jsonRequest["dps"]["5"] = hexColor;
  jsonRequest["dps"]["2"] = "colour";

  String payload = createPayload(jsonRequest);
 
  String response = sendCommand(payload, 7);

  return _error;
}

int TuyaBulb::setWhite(byte brightness, byte temp) {

  if (brightness < 25 || brightness > 255) {
    DEBUG_PRINTLN("BRIGHTNESS MUST BE BETWEEN 25 AND 255");
    return _error = TUYA_ERROR_ARGS;
  }

  // Allocate json object
  StaticJsonDocument<512> jsonRequest;
  
  // Build request
  initSetRequest(jsonRequest);
  jsonRequest["dps"]["2"] = "white";
  jsonRequest["dps"]["3"] = brightness;
  jsonRequest["dps"]["4"] = temp;

  String payload = createPayload(jsonRequest);
 
  String response = sendCommand(payload, 7);

  return _error;  
}
