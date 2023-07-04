#include "wifiFunctions.h"
#include "arduino_secrets.h"   // ssid and password stored in header file

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// is there a way to have a dynamic board ID instead of hard coded? GPIO maybe use those input-only pins
#define BOARD_ID 1
#define MAX_CHANNEL 11  // for North America // 13 in Europe
uint8_t serverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// wifi Server or Sender mode determines whether this module will host the
// web interface and coordinate ESP-NOW auto-pairing or else act as a regular ESP-NOW node
const bool wifiServer = false;     // logic low  on jumper means wifi Server mode
const bool wifiSender = true;      // logic high on jumper means wifi Sender mode
bool wifiMode = wifiServer;        // default to server mode

esp_now_peer_info_t slave;
int chan;

enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};
PairingStatus pairingStatus = NOT_PAIRED;

enum MessageType {PAIRING, DATA,};
MessageType messageType;

#ifdef SAVE_CHANNEL
int lastChannel;
#endif
int channel = 1;

int counter = 0;

// ***sender simulate temperature and humidity data
float t = 0;
float h = 0;
unsigned long currentMillis = millis();
unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings
unsigned long start;                // used to measure Pairing time
unsigned int readingId = 0;

// simulate temperature reading
float readDHTTemperature() {
  t = random(0, 40);
  return t;
}

// simulate humidity reading
float readDHTHumidity() {
  h = random(0, 100);
  return h;
}

/*
  const byte slic_fwdRev   = 27;          // slic forward/reverse
  const byte slic_ringMode = 33;          // slic ring mode
  const byte slic_swHook   = 32;          // slic switch hook
*/

typedef struct struct_message {         // structure for sending data between nodes
  uint8_t msgType;
  uint8_t id;
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;

typedef struct struct_pairing {        // structure for pairing
  uint8_t msgType;
  uint8_t id;
  uint8_t macAddr[6];
  uint8_t channel;
} struct_pairing;

struct_message incomingMsg;
struct_message outgoingMsg;
struct_pairing pairingData;

AsyncWebServer server(80);
AsyncEventSource events("/events");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Telephone Central Office Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .card.temperature { color: #fd7e14; }
    .card.humidity { color: #1b78e2; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>Telephone Central Office Dashboard</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4>Hook Status</h4><p><span class="reading"><span id="t1"></span> </span></p><p class="packet">Reading ID: <span id="rt1"></span></p>
      </div>
      <div class="card humidity">
        <h4>Ring Status</h4><p><span class="reading"><span id="h1"></span> </span></p><p class="packet">Reading ID: <span id="rh1"></span></p>
      </div>
      <div class="card temperature">
        <h4>Misc 1</h4><p><span class="reading"><span id="t2"></span> </span></p><p class="packet">Reading ID: <span id="rt2"></span></p>
      </div>
      <div class="card humidity">
        <h4>Misc 2</h4><p><span class="reading"><span id="h2"></span> </span></p><p class="packet">Reading ID: <span id="rh2"></span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');

 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);

 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);

 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("t"+obj.id).innerHTML = obj.temperature.toFixed(2);
  document.getElementById("h"+obj.id).innerHTML = obj.humidity.toFixed(2);
  document.getElementById("rt"+obj.id).innerHTML = obj.readingId;
  document.getElementById("rh"+obj.id).innerHTML = obj.readingId;
 }, false);
}
</script>
</body>
</html>)rawliteral";

void readDataToSend() {
  outgoingMsg.msgType = DATA;
  outgoingMsg.id = 0;
  outgoingMsg.temp = random(0, 40);
  outgoingMsg.hum = random(0, 100);
  outgoingMsg.readingId = counter++;
}


// ---------------------------- esp_ now -------------------------
void printMAC(const uint8_t * mac_addr) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

bool addPeer(const uint8_t *peer_addr) {      // add pairing
  memset(&slave, 0, sizeof(slave));
  const esp_now_peer_info_t *peer = &slave;
  memcpy(slave.peer_addr, peer_addr, 6);

  slave.channel = chan; // pick a channel
  slave.encrypt = 0; // no encryption
  // check if the peer exists
  bool exists = esp_now_is_peer_exist(slave.peer_addr);
  if (exists) {
    // Slave already paired.
    Serial.println("Already Paired");
    return true;
  }
  else {
    esp_err_t addStatus = esp_now_add_peer(peer);
    if (addStatus == ESP_OK) {
      // Pair success
      Serial.println("Pair success");
      return true;
    }
    else
    {
      Serial.println("Pair failed");
      return false;
    }
  }
}

// *** Sender function different from Server?
void addPeerSender(const uint8_t * mac_addr, uint8_t chan) {
  esp_now_peer_info_t peer;
  ESP_ERROR_CHECK(esp_wifi_set_channel(chan , WIFI_SECOND_CHAN_NONE));
  esp_now_del_peer(mac_addr);
  memset(&peer, 0, sizeof(esp_now_peer_info_t));
  peer.channel = chan;
  peer.encrypt = false;
  memcpy(peer.peer_addr, mac_addr, sizeof(uint8_t[6]));
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  memcpy(serverAddress, mac_addr, sizeof(uint8_t[6]));
}



// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
  printMAC(mac_addr);
  Serial.println();
}

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  if (wifiMode == wifiServer) {
    Serial.print(len);
    Serial.print(" bytes of data received from : ");
    printMAC(mac_addr);
    Serial.println();
    StaticJsonDocument<1000> root;
    String payload;
    uint8_t type = incomingData[0];       // first message byte is the type of message
    switch (type) {
      case DATA :                           // the message is data type
        memcpy(&incomingMsg, incomingData, sizeof(incomingMsg));
        // create a JSON document with received data and send it by event to the web page
        root["id"] = incomingMsg.id;
        root["temperature"] = incomingMsg.temp;
        root["humidity"] = incomingMsg.hum;
        root["readingId"] = String(incomingMsg.readingId);
        serializeJson(root, payload);
        Serial.print("event send :");
        serializeJson(root, Serial);
        events.send(payload.c_str(), "new_readings", millis());
        Serial.println();
        break;

      case PAIRING:                            // the message is a pairing request
        memcpy(&pairingData, incomingData, sizeof(pairingData));
        Serial.println(pairingData.msgType);
        Serial.println(pairingData.id);
        Serial.print("Pairing request from: ");
        printMAC(mac_addr);
        Serial.println();
        Serial.println(pairingData.channel);
        if (pairingData.id > 0) {     // do not replay to server itself
          if (pairingData.msgType == PAIRING) {
            pairingData.id = 0;       // 0 is server
            // Server is in AP_STA mode: peers need to send data to server soft AP MAC address
            WiFi.softAPmacAddress(pairingData.macAddr);
            pairingData.channel = chan;
            Serial.println("send response");
            esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &pairingData, sizeof(pairingData));
            addPeer(mac_addr);
          }
        }
        break;
    }
  }
  else if (wifiMode == wifiSender) {
    Serial.print("Packet received from: ");
    printMAC(mac_addr);
    Serial.println();
    Serial.print("data size = ");
    Serial.println(sizeof(incomingData));
    uint8_t type = incomingData[0];
    switch (type) {
      case DATA :      // we received data from server
        memcpy(&incomingMsg, incomingData, sizeof(incomingMsg));
        Serial.print("ID  = ");
        Serial.println(incomingMsg.id);
        Serial.print("Setpoint temp = ");
        Serial.println(incomingMsg.temp);
        Serial.print("SetPoint humidity = ");
        Serial.println(incomingMsg.hum);
        Serial.print("reading Id  = ");
        Serial.println(incomingMsg.readingId);

        break;

      case PAIRING:    // we received pairing data from server
        memcpy(&pairingData, incomingData, sizeof(pairingData));
        if (pairingData.id == 0) {              // the message comes from server
          printMAC(mac_addr);
          Serial.print("Pairing done for ");
          printMAC(pairingData.macAddr);
          Serial.print(" on channel " );
          Serial.print(pairingData.channel);    // channel used by the server
          Serial.print(" in ");
          Serial.print(millis() - start);
          Serial.println("ms");
          addPeerSender(pairingData.macAddr, pairingData.channel); // add the server  to the peer list
#ifdef SAVE_CHANNEL
          lastChannel = pairingData.channel;
          EEPROM.write(0, pairingData.channel);
          EEPROM.commit();
#endif
          pairingStatus = PAIR_PAIRED;             // set the pairing status
        }
        break;
    }
  }
}

PairingStatus autoPairing() {
  switch (pairingStatus) {
    case PAIR_REQUEST:
      Serial.print("Pairing request on channel "  );
      Serial.println(channel);

      // set WiFi channel
      ESP_ERROR_CHECK(esp_wifi_set_channel(channel,  WIFI_SECOND_CHAN_NONE));
      if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
      }

      // set callback routines
      esp_now_register_send_cb(OnDataSent);
      esp_now_register_recv_cb(OnDataRecv);

      // set pairing data to send to the server
      pairingData.msgType = PAIRING;
      pairingData.id = BOARD_ID;
      pairingData.channel = channel;

      // add peer and send request
      addPeerSender(serverAddress, channel);
      esp_now_send(serverAddress, (uint8_t *) &pairingData, sizeof(pairingData));
      previousMillis = millis();
      pairingStatus = PAIR_REQUESTED;
      break;

    case PAIR_REQUESTED:
      // time out to allow receiving response from server
      currentMillis = millis();
      if (currentMillis - previousMillis > 250) {
        previousMillis = currentMillis;
        // time out expired,  try next channel
        channel ++;
        if (channel > MAX_CHANNEL) {
          channel = 1;
        }
        pairingStatus = PAIR_REQUEST;
      }
      break;

    case PAIR_PAIRED:
      // nothing to do here
      break;
  }
  return pairingStatus;
}

void initESP_NOW() {
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

/*
  // configure SLIC pins and ring patterns based on region
  void initSlic() {

  pinMode(slic_fwdRev, OUTPUT);
  pinMode(slic_ringMode, OUTPUT);
  pinMode(slic_swHook, INPUT_PULLUP);

  digitalWrite(slic_fwdRev, LOW);
  digitalWrite(slic_ringMode, LOW);
  }
*/

void initServer() {
  Serial.println("Init Wifi Server...");
  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }

  // create a function that periodically checks various things like
  // is wifi connected or not and directly update LED that way
  digitalWrite(LED_BUILTIN, HIGH);  // show wifi connected on LED

  Serial.print("Server SOFT AP MAC Address:  ");
  Serial.println(WiFi.softAPmacAddress());

  chan = WiFi.channel();
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  initESP_NOW();

  // Start Web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });


  // Events
  events.onConnect([](AsyncEventSourceClient * client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // start server
  server.begin();

}

void initSender() {
  Serial.println("Init Wifi Sender...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  start = millis();

#ifdef SAVE_CHANNEL
  EEPROM.begin(10);
  lastChannel = EEPROM.read(0);
  Serial.println(lastChannel);
  if (lastChannel >= 1 && lastChannel <= MAX_CHANNEL) {
    channel = lastChannel;
  }
  Serial.println(channel);
#endif
  pairingStatus = PAIR_REQUEST;
  while (pairingStatus != PAIR_PAIRED) {
    autoPairing();
    };

}

void doServerEvent() {
  // Serial.println("***5000 mS interval");
  events.send("ping", NULL, millis());
  readDataToSend();
  esp_now_send(NULL, (uint8_t *) &outgoingMsg, sizeof(outgoingMsg));
}

void doSenderEvent() {
  if (autoPairing() == PAIR_PAIRED) {
  //Set values to send
        outgoingMsg.msgType = DATA;
        outgoingMsg.id = BOARD_ID;
        outgoingMsg.temp = readDHTTemperature();
        outgoingMsg.hum = readDHTHumidity();
        outgoingMsg.readingId = readingId++;
        esp_err_t result = esp_now_send(serverAddress, (uint8_t *) &outgoingMsg, sizeof(outgoingMsg));
  }
}



/*
  void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println();
  Serial.print("ESP32 MAC Address:  ");
  Serial.println(WiFi.macAddress());

  pinMode(senderServerPin, INPUT);           // pin 35 is input only so there is an external pull up resistor
  wifiMode = digitalRead(senderServerPin);   // set this node as a wifi Sender or Server based on jumper setting

  Serial.println();
  Serial.print("ESP32 Wifi Mode: ");
  Serial.println(wifiMode ? "Sender" : "Server");

  if (wifiMode == wifiServer) initServer();
  else initSender();

  initSlic();

  // Mozzi test
  startMozzi(CONTROL_RATE); // :)
  aSin.setFreq(440); // set the frequency
  aSample1.setFreq((float) ThisIsARecording1_SAMPLERATE / (float) ThisIsARecording1_NUM_CELLS); // play at the speed it was recorded
  aSample2.setFreq((float) ThisIsARecording2_SAMPLERATE / (float) ThisIsARecording2_NUM_CELLS); // play at the speed it was recorded
  aSample3.setFreq((float) ThisIsARecording3_SAMPLERATE / (float) ThisIsARecording3_NUM_CELLS); // play at the speed it was recorded
  kTriggerDelay.set(15000); // 15000 msec countdown, within resolution of CONTROL_RATE

  gain1 = 1;
  curSample = 1;
  aSample1.start();

  }
*/




/*
  void disableWiFi() {
  WiFi.disconnect(true);  // Disconnect from the network
  WiFi.mode(WIFI_OFF);    // Switch WiFi off
  Serial.println("");
  Serial.println("WiFi disconnected!");
  }


  void enableWiFi() {

  //WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);

  Serial.println("START WIFI");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  }
*/

/*
  void loop() {
  static unsigned long lastEventTime = millis();
  static unsigned long lastEventTime2 = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;
  static const unsigned long WIFI_TEST_INTERVAL_MS = 20000;

  audioHook(); // Mozzi process

  if (wifiMode == wifiServer) {
    if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
      // Serial.println("***5000 mS interval");
      events.send("ping", NULL, millis());
      lastEventTime = millis();
      readDataToSend();
      esp_now_send(NULL, (uint8_t *) &outgoingMsg, sizeof(outgoingMsg));
      Serial.print("***SLIC Hook State: ");
      Serial.println(digitalRead(slic_swHook) ? "Off Hook" : "On Hook");
    }
  }

  if (wifiMode == wifiSender) {
    if (autoPairing() == PAIR_PAIRED) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        // Save the last time a new reading was published
        previousMillis = currentMillis;
        //Set values to send
        outgoingMsg.msgType = DATA;
        outgoingMsg.id = BOARD_ID;
        outgoingMsg.temp = readDHTTemperature();
        outgoingMsg.hum = readDHTHumidity();
        outgoingMsg.readingId = readingId++;
        esp_err_t result = esp_now_send(serverAddress, (uint8_t *) &outgoingMsg, sizeof(outgoingMsg));
      }
    }
  }

  // this if statement should be commented out
    if ((millis() - lastEventTime2) > WIFI_TEST_INTERVAL_MS) {

        Serial.print("***Toggle WiFi interval: ");
        static boolean wifiOn = true;
        lastEventTime2 = millis();
        if (wifiOn) {
        disableWiFi();
        wifiOn = false;
        }
        else {
        enableWiFi();
        wifiOn = true;
        Serial.println("WiFi enabled");
        }
    }


  if (curSample == 1 && !aSample1.isPlaying()) {
    gain1 = 0;
    gain2 = 1;
    gain3 = 0;
    gain4 = 0;
    curSample = 2;
    aSample2.start();
    Serial.println("Sample 2 starting...");
  }

  if (curSample == 2 && !aSample2.isPlaying()) {
    gain1 = 0;
    gain2 = 0;
    gain3 = 1;
    gain4 = 0;
    curSample = 3;
    aSample3.start();
    Serial.println("Sample 3 starting...");
  }

  if (curSample == 3 && !aSample3.isPlaying()) {
    gain1 = 1;
    gain2 = 0;
    gain3 = 0;
    gain4 = 0;
    curSample = 1;
    aSample1.start();
    Serial.println("Sample 1 starting...");
  }

  }

*/

/*
                          Is delivery success 2 way comm still working after letting it run?
                          Can the slave be set to auto join after a time out of not getting packets due to
                          wifi always dropping?
                          Does web page stay updated after letting it run a long time?
                          Do we need to block/delay when rejoining wifi or can we just tell it to do it?
                          If we need to block, can we assign that to core 0?

                          what does on board LED mean on slave code?
                          Add recording to call progress class
                          sketch got stuck in endless loop trying to restart wifi setting as station, fix so it
                            will time out and try shutting off and on wifi again? best if it is in core 0 so espnow works?



*/
