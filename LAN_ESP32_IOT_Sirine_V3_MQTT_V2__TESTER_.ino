#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// define two tasks for Blink & AnalogRead
void TaskBlink( void *pvParameters );
void TaskWebServer( void *pvParameters );

#include <ETH.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ArduinoJson.h>

#ifdef ETH_CLK_MODE
#undef ETH_CLK_MODE
#endif
#define ETH_CLK_MODE    ETH_CLOCK_GPIO0_IN //ETH_CLOCK_GPIO17_OUT

#define ETH_POWER_PIN   -1
#define ETH_TYPE        ETH_PHY_LAN8720
#define ETH_ADDR        1
#define ETH_MDC_PIN     23
#define ETH_MDIO_PIN    18

static bool eth_connected = false;

//MQTT Setting//
//const char* mqtt_server = "xxx.xxx.xxx.xxx";
const char* mqtt_server = "xxx.xxx.xxx.xxx";
WiFiClient espClient;
PubSubClient client(espClient);

String MaCAddr;
int count, sound, duration;

// Assign output variables to GPIO pins
const int outputLED = 33;
const int outputSIRINE = 32;
const int LEDIndikator = 2; //pin kosong D2(on board led), D5, D13, D14, D15
const int latchPin = 15; // IC pin ST_CP
const int dataPin = 5; // IC Pin DS
const int clockPin = 4; // IC Pin SH_CP

int dec[10] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111}; // 0,1,2,3,4,5,6,7,8,9
int digit1, digit2, digit3, digit4, LedOk;

int stringData;
String perBagianData;

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname and IP here
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
//      MaCAddr = ETH.macAddress();
      MaCAddr = "xx:Xx:Xx:Xx";
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);

  count = doc["count"]; //inisialisasi data disesuaikan data dari json web/server
  sound = doc["sound"];
  duration = doc["duration"];

  Serial.print("count=");
  Serial.println(count);
  Serial.print("sound=");
  Serial.println(sound);
  Serial.print("duration=");
  Serial.println(duration);

  if (count > 0) {
    digitalWrite(outputLED, HIGH);
    Serial.println("Lampu ON");
  }
  else if (count == 0 ) {
    digitalWrite(outputLED, LOW);
    Serial.println("Lampu OFF");
  }

  if (count > 0 && sound == 1) {
    Serial.println("Lampu & Sirine ON");
    digitalWrite(outputLED, HIGH);
    digitalWrite(outputSIRINE, HIGH);
    delay(duration);
    digitalWrite(outputSIRINE, LOW);
    sound = 0;
  }

  if (count <= 9) { //1 Digit
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[0]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[0]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[0]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[count]);
    digitalWrite(latchPin, HIGH);
    Serial.print("1 Digit=");
    Serial.println(count);
  }
  else if (count >= 10 && count <= 99) { //2 Digit
    digit1 = (count / 1U) % 10;
    digit2 = (count / 10U) % 10;
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[0]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[0]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[digit2]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[digit1]);
    digitalWrite(latchPin, HIGH);
    Serial.print("2 Digit=");
    Serial.println(count);
  }
  else if (count >= 100 && count <= 999) { //3 Digit
    digit1 = (count / 1U) % 10;
    digit2 = (count / 10U) % 10;
    digit3 = (count / 100U) % 10;
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[0]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[digit3]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[digit2]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[digit1]);
    digitalWrite(latchPin, HIGH);
    Serial.print("3 Digit=");
    Serial.println(count);
  }
  else if (count >= 1000 && count <= 9999) { //4 Digit
    digit1 = (count / 1U) % 10;
    digit2 = (count / 10U) % 10;
    digit3 = (count / 100U) % 10;
    digit4 = (count / 1000U) % 10;
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[digit4]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[digit3]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[digit2]);
    shiftOut(dataPin, clockPin, MSBFIRST, dec[digit1]);
    digitalWrite(latchPin, HIGH);
    Serial.print("4 Digit=");
    Serial.println(count);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String client_id = "izzy.alert." + MaCAddr + "";
    const char* charClient_ID = client_id.c_str();
    if (client.connect(charClient_ID)) {
      Serial.println("connected");
      // Subscribe
      String MAC = "com.com/topic/sirine/" + MaCAddr + "";
      const char* topicMac = MAC.c_str();
      client.subscribe(topicMac);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(outputLED, OUTPUT);
  pinMode(outputSIRINE, OUTPUT);
  pinMode(LEDIndikator, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, dec[0]);
  shiftOut(dataPin, clockPin, MSBFIRST, dec[0]);
  shiftOut(dataPin, clockPin, MSBFIRST, dec[0]);
  shiftOut(dataPin, clockPin, MSBFIRST, dec[0]);
  digitalWrite(latchPin, HIGH);

  duration = 1500; //1500ms for delay

  xTaskCreatePinnedToCore(
    TaskBlink
    ,  "TaskBlink"   // A name just for humans
    ,  10000  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskWebServer
    ,  "WebServer"
    ,  10000  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void TaskBlink(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  for (;;)
  {
    vTaskDelay(10);
  }
}

void TaskWebServer(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  for (;;)
  {
      digitalWrite(LEDIndikator, HIGH);
      delay(1000);
      digitalWrite(LEDIndikator, LOW);
      delay(1000);
  }
}

String ambilData(String dtn, char pemisah, int urutan)
{
  stringData = 0;
  perBagianData = "";
  for (int ii = 0; ii < dtn.length() - 1; ii++)
  {
    if (dtn[ii] == pemisah) {
      stringData++;
    }
    else if (stringData == urutan) {
      perBagianData.concat(dtn[ii]);
    }
    else if (stringData > urutan) {
      return perBagianData;
      break;
    }
  }
  return perBagianData;
}
