#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <Adafruit_NeoPixel.h>

#include <DFRobot_DHT11.h>


const char* SSID = "doma";
const char* PASSWORD = "etchristoph1022";

const char* MQTT_SERVER = "test.mosquitto.org";
const int MQTT_PORT = 1883;
//const char* MQTT_USER = "hivemq.webclient.1741295015571";
//const char* MQTT_PW = "V7g4%.y;LA5>h3npKjNF";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
int value = 0;

unsigned long lastUpdateDisplay = 0;
unsigned long lastUpdateNeopixel = 0;
unsigned long lastUpdateDht = 0;

#define PIN D1
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, PIN, NEO_GRB + NEO_KHZ800);

unsigned long patternInterval = 20 ; // doba mezi jednotlivými kroky vzoru
unsigned long intervals [] = { 50, 50 } ; // rychlost pro každý vzor - při přidávání dalšího case přidejte sem
int numberOfCases = 1; // počet case případů (1 = 0-1 case případů)

DFRobot_DHT11 DHT;
#define DHT11_PIN D0

void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}

// Proměnné pro práci s Neopixel kruhem
int pattern = 0; // Inicializace proměnné pattern s hodnotou 0 (rainbow mód)
int lamp_R = 255;
int lamp_G = 255;
int lamp_B = 255;

String getPayloadString(byte* payload, uint8_t length)
{
  String req = "";
  for (int i = 0; i < length; i++)
  {
    req += (char)payload[i];
  }

  return req;
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Porovnání tématu (v tomto případě volby módu), zda sedí
    if (strcmp(topic,"MoodLamp/LampMode") == 0) {
      const String req = getPayloadString(payload, length);
      pattern = req.toInt();
    }

  // Následovně to samé porovnání pro R,G,B
    if (strcmp(topic,"MoodLamp/Lamp/Red") == 0) {
      const String req = getPayloadString(payload, length);
      lamp_R = req.toInt();
    }

    if (strcmp(topic,"MoodLamp/Lamp/Green") == 0) {
      const String req = getPayloadString(payload, length);
      lamp_G = req.toInt();
    }

    if (strcmp(topic,"MoodLamp/Lamp/Blue") == 0) {
      const String req = getPayloadString(payload, length);
      lamp_B = req.toInt();
    }
}

void reconnect() {
  // Loop, dokud se znovu nespojí
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Vytvoření náhodného ID klienta
    String clientId = "ESPClient-";
    clientId += String(random(0xffff), HEX);
    // Pokus o připojení
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Po připojení zveřejníme oznámení...
      client.publish("MoodLamp/hello", "hello world");
      // ... a znovu se přihlásíme k odběru
      client.subscribe("MoodLamp/LampMode");
      client.subscribe("MoodLamp/Lamp/Red");
      client.subscribe("MoodLamp/Lamp/Green");
      client.subscribe("MoodLamp/Lamp/Blue");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Před dalším pokusem počkáme 5 sekund
      delay(5000);
    }
  }
}

void setup() {
  // MQTT setup
    Serial.begin(115200);
    setup_wifi();
    
    //espClient.setInsecure();

    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);

  // Neopixel Kruh setup
    strip.begin(); // Inicializace knihovny NeoPixel
    strip.setBrightness(50);
    wipe(); // vymaže vyrovnávací paměť LED
}

// Funkce pro neopixel
  // Funkce updatePattern (volá právě vytvářený vzor)
  void  updatePattern(int pat){
    switch(pat) {
      case 1:
          colorWipe(strip.Color(lamp_R, lamp_G, lamp_B));
          //Serial.println("Hodnoty ze switche:");
          //Serial.println(lamp_R);
          //Serial.println(lamp_G);
          //Serial.println(lamp_B);
          strip.show();
          break;
      case 0:
          rainbow(); // Rainbow Mode
          break;
    }
  }

  // Funkce rainbow (pro rainbow mode)
  void rainbow() {
    static uint16_t j=0;
      for(int i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel((i+j) & 255));
      }
      strip.show();
      j++;
    if(j >= 256) j=0;
  }

  // Funkce colorWipe (pro zvolenou barvu)
  void colorWipe(uint32_t c) {
    for(int i=0;i<strip.numPixels();i++){
      strip.setPixelColor(i,c);
    }
  }

  // Funkce wipe (pro vymazání vyrovnávací paměti LEDek)
  void wipe(){
    for(int i=0;i<strip.numPixels();i++){
      strip.setPixelColor(i, strip.Color(0,0,0));
    }
  }

  // Funkce Wheel (pro práci s pozicemi LEDek na kruhu)
  uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85) {
      return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if(WheelPos < 170) {
      WheelPos -= 85;
      return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }


void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  char buffer[350];

  // Neopixel kruh
      patternInterval = intervals[pattern]; // nastavení rychlosti pro vzory

      if(millis() - lastUpdateNeopixel >= patternInterval) {
        updatePattern(pattern);
        lastUpdateNeopixel = millis();
      }

  DHT.read(DHT11_PIN);
  Serial.print(DHT.temperature);  Serial.print(DHT.humidity);


  // JSON část:
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    JsonDocument doc;
    doc["teplota"]["hodnota"] = DHT.temperature;
    doc["teplota"]["jednotka"] = "°C";
    doc["vlhkost"]["hodnota"] = DHT.humidity;
    doc["vlhkost"]["jednotka"] = "%";
    size_t n = serializeJson(doc, buffer);
    lastMsg = now;
    client.publish("MoodLamp", buffer, n);
  }
}