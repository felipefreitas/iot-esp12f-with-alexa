#define ESPALEXA_ASYNC
#include <Espalexa.h>

#include <ESPAsyncTCP.h>
#include <tcp_axtls.h>
#include <async_config.h>
#include <AsyncPrinter.h>
#include <ESPAsyncTCPbuffer.h>
#include <SyncClient.h>
#include <DebugPrintMacros.h>

#include "ESP8266WiFi.h"
#include "ESPAsyncWebServer.h"

#define IS_RELAY_NORMALLY_OPEN true
#define NUMBER_OF_RELAYS 4

Espalexa espalexa;
AsyncWebServer server(80);

int relayGPIOs[NUMBER_OF_RELAYS] = {16, 14, 12, 13};

void turnOnOffDevice(int gpioDevice, bool isTurnOn);
void firstDeviceChanged(uint8_t brightness);
void secondDeviceChanged(uint8_t brightness);
void thirdDeviceChanged(uint8_t brightness);
void fourthDeviceChanged(uint8_t brightness);
String getLabelForNormallyOpenTurnedOn(int isTurnedOn);
String getLabelForNormallyClosedTurnedOn(int isTurnedOn);
void setRelayInitialState(int relayPin);
void loadRelaysWithInitialState();
void loadWiFiDevice();
void loadServerRoutes(AsyncWebServer* server);
void setDevices();

const char *ssid = "12341234";
const char *password = "12341234";
const char *PARAM_INPUT_1 = "relay";
const char *PARAM_INPUT_2 = "state";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>
  function toggleCheckbox(element) {
    var xhr = new XMLHttpRequest();
    if(element.checked){ xhr.open("GET", "/update?relay="+element.id+"&state=1", true); }
    else { xhr.open("GET", "/update?relay="+element.id+"&state=0", true); }
    xhr.send();
  }
</script>
</body>
</html>
)rawliteral";

void turnOnOffDevice(int gpioDevice, bool isTurnedOn) {
  IS_RELAY_NORMALLY_OPEN 
    ? digitalWrite(gpioDevice, !isTurnedOn)
    : digitalWrite(gpioDevice, isTurnedOn);
}

void firstDeviceChanged(uint8_t brightness) {
  bool isTurnedOn = brightness == 255;
  turnOnOffDevice(relayGPIOs[0], isTurnedOn);
}

void secondDeviceChanged(uint8_t brightness) {
  bool isTurnedOn = brightness == 255;
  turnOnOffDevice(relayGPIOs[1], isTurnedOn);
}

void thirdDeviceChanged(uint8_t brightness) {
  bool isTurnedOn = brightness == 255;
  turnOnOffDevice(relayGPIOs[2], isTurnedOn);
}

void fourthDeviceChanged(uint8_t brightness) {
  bool isTurnedOn = brightness == 255;
  turnOnOffDevice(relayGPIOs[3], isTurnedOn);
}

String HTMLProcessor(const String &var) {
  if (var == "BUTTONPLACEHOLDER") {
    String buttons = "";
    for (int i = 1; i <= NUMBER_OF_RELAYS; i++) {
      String relayStateValue = getRelayState(i);
      buttons += "<h4>Relay #" + String(i) + " - GPIO " + relayGPIOs[i - 1] + "</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(i) + "\" " + relayStateValue + "><span class=\"slider\"></span></label>";
    }
    return buttons;
  }
  return String();
}

String getRelayState(int relayPin) {
  int isTurnedOn = digitalRead(relayGPIOs[relayPin - 1]);
  return IS_RELAY_NORMALLY_OPEN
    ? getLabelForNormallyOpenTurnedOn(isTurnedOn)
    : getLabelForNormallyClosedTurnedOn(isTurnedOn); 
}

String getLabelForNormallyOpenTurnedOn(int isTurnedOn) {
  return isTurnedOn ? "" : "checked";
}

String getLabelForNormallyClosedTurnedOn(int isTurnedOn) {
  return isTurnedOn ? "checked" : "";
}

void setRelayInitialState(int relayPin) {
  IS_RELAY_NORMALLY_OPEN
    ? digitalWrite(relayPin, HIGH)
    : digitalWrite(relayPin, LOW);
}

void loadRelaysWithInitialState() {
  for (int i = 1; i <= NUMBER_OF_RELAYS; i++) {
    pinMode(relayGPIOs[i-1], OUTPUT);
    setRelayInitialState(relayGPIOs[i-1]);
  }
}

void loadWiFiDevice() {
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());
}

void loadServerRoutes(AsyncWebServer* server) {
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, HTMLProcessor);
  });

  server->on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {
      String inputMessage = request->getParam(PARAM_INPUT_1)->value();
      String inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      bool relayStateString = inputMessage2.toInt();
      int relayPin = relayGPIOs[inputMessage.toInt()-1];

      turnOnOffDevice(relayPin, relayStateString);
    }

    request->send(200, "text/plain", "OK");
  });
  
  server->onNotFound([](AsyncWebServerRequest *request){
    if (!espalexa.handleAlexaApiCall(request)) {
      request->send(404, "text/plain", "Not found");
    }
  });
}

void setDevices() {
  espalexa.addDevice("LUZ 1", firstDeviceChanged);
  espalexa.addDevice("LUZ 2", secondDeviceChanged);
  espalexa.addDevice("LUZ 3", thirdDeviceChanged);
  espalexa.addDevice("LUZ 4", fourthDeviceChanged);
}

void setup() {
  Serial.begin(115200);
  
  loadRelaysWithInitialState();
  loadWiFiDevice();
  loadServerRoutes(&server);
  setDevices();
  
  espalexa.begin(&server);
}

void loop() {
  espalexa.loop();
  delay(1);
}
