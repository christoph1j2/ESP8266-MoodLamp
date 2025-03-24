/**
 * @file src.ino
 * @brief Kód pro ESP8266, který se připojuje k WiFi, MQTT, ovládá Neopixel kruh a DHT sensor.
 *
 * Tento kód se snaží připojit k WiFi sítím uvedeným v poli. Pokud se nepodaří připojit k primární síti,
 * pokusí se připojit k dalším sítím cyklicky (while loop), dokud se nepřipojí.
 * Dále se připojuje k MQTT brokeru a odesílá data z DHT11 senzoru ve formátu JSON.
 */

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobot_DHT11.h>

// *************************************************************
// Definice WiFi sítí - pole struktur s názvy a hesly WiFi sítí
// *************************************************************
struct WifiNetwork {
  const char* ssid;
  const char* password;
};

// Zadej sem seznam WiFi sítí, ke kterým se kód pokusí připojit
WifiNetwork wifiNetworks[] = {
  {"3301-IoT", "mikrobus"},
  {"doma", "etchristoph1022"},
  {"Lukyno_", "MatyVihrava"}
};

const int wifiNetworkCount = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// *****************************************
// Definice MQTT serveru a připojení klienta
// *****************************************
const char* MQTT_SERVER = "test.mosquitto.org";
const int MQTT_PORT = 1883;
// const char* MQTT_USER = "hivemq.webclient.1741295015571";
// const char* MQTT_PW = "V7g4%.y;LA5>h3npKjNF";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
int value = 0;

unsigned long lastUpdateDisplay = 0;
unsigned long lastUpdateNeopixel = 0;
unsigned long lastUpdateDht = 0;

// *********************************
// Definice pinů a inicializace Neopixel
// *********************************
#define PIN D1
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, PIN, NEO_GRB + NEO_KHZ800);

// Interval mezi kroky vzoru pro Neopixel (nastavitelný)
// intervals[] definuje rychlost pro každý vzor – pokud přidáš další "case", přidej sem odpovídající interval
unsigned long patternInterval = 20; // doba mezi jednotlivými kroky vzoru
unsigned long intervals[] = {50, 50};
int numberOfCases = 1; // počet případů (např. 1 = 0-1)


// ***************************
// Inicializace DHT11 senzoru
// ***************************
DFRobot_DHT11 DHT;
#define DHT11_PIN D0

/**
 * @brief Připojí ESP8266 k WiFi sítím.
 *
 * Funkce prochází pole wifiNetworks a pokouší se o připojení ke každé síti.
 * Pokud se nepodaří připojit během timeoutu (10 sekund), vyzkouší další síť.
 * Cyklus běží, dokud se zařízení nepřipojí k některé síti.
 */
void setup_wifi() {
  delay(10);
  Serial.println();
  
  bool connected = false;
  int networkIndex = 0; // index aktuální WiFi sítě
  while (!connected) {
    // Vybere aktuální síť z pole
    const char* ssid = wifiNetworks[networkIndex].ssid;
    const char* password = wifiNetworks[networkIndex].password;
    
    Serial.print("Pokus o připojení k WiFi: ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // Čeká, dokud se nepřipojí nebo nedojde k timeoutu (10 sekund)
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      Serial.println();
      Serial.print("Připojeno k WiFi: ");
      Serial.println(ssid);
      Serial.print("IP adresa: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println();
      Serial.print("Nepodařilo se připojit k WiFi: ");
      Serial.println(ssid);
      // Vybere další síť (cyklicky)
      networkIndex = (networkIndex + 1) % wifiNetworkCount;
      Serial.println("Zkusím další WiFi...");
    }
  }
  
  // Inicializace generátoru náhodných čísel (používá se při generování klientského ID pro MQTT)
  randomSeed(micros());
}

// Proměnné pro práci s Neopixel kruhem
int pattern = 0; // Inicializace proměnné pattern s hodnotou 0 (rainbow mód)
int lamp_R = 255;
int lamp_G = 255;
int lamp_B = 255;

/**
 * @brief Převádí pole bajtů na řetězec.
 *
 * @param payload Ukazatel na pole bajtů (obsah MQTT zprávy).
 * @param length Délka pole.
 * @return String Řetězec vytvořený z bajtů.
 */
String getPayloadString(byte* payload, uint8_t length) {
  String req = "";
  for (int i = 0; i < length; i++) {
    req += (char)payload[i];
  }
  return req;
}

/**
 * @brief Callback funkce pro zpracování příchozích MQTT zpráv.
 *
 * Podle tématu zprávy mění chování lampy (např. mód a barvu).
 *
 * @param topic Téma zprávy.
 * @param payload Data zprávy.
 * @param length Délka zprávy.
 */
void callback(char* topic, byte* payload, unsigned int length) {
  // Změna režimu lampy (např. rainbow nebo statická barva)
  if (strcmp(topic, "MoodLamp/LampMode") == 0) {
    const String req = getPayloadString(payload, length);
    pattern = req.toInt();
  }

  // Nastavení červené složky barvy lampy
  if (strcmp(topic, "MoodLamp/Lamp/Red") == 0) {
    const String req = getPayloadString(payload, length);
    lamp_R = req.toInt();
  }

  // Nastavení zelené složky barvy lampy
  if (strcmp(topic, "MoodLamp/Lamp/Green") == 0) {
    const String req = getPayloadString(payload, length);
    lamp_G = req.toInt();
  }

  // Nastavení modré složky barvy lampy
  if (strcmp(topic, "MoodLamp/Lamp/Blue") == 0) {
    const String req = getPayloadString(payload, length);
    lamp_B = req.toInt();
  }
}

/**
 * @brief Opakovaně se připojuje k MQTT brokeru.
 *
 * Pokud není klient připojen, pokusí se o nové připojení s náhodně vygenerovaným ID.
 * Po úspěšném připojení odešle uvítací zprávu a přihlásí se k odběru potřebných témat.
 */
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Vytvoření náhodného klientského ID
    String clientId = "ESPClient-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Odešle uvítací zprávu
      client.publish("MoodLamp/hello", "hello world");
      // Přihlásí se k odběru témat
      client.subscribe("MoodLamp/LampMode");
      client.subscribe("MoodLamp/Lamp/Red");
      client.subscribe("MoodLamp/Lamp/Green");
      client.subscribe("MoodLamp/Lamp/Blue");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(", try again in 5 seconds");
      delay(5000);
    }
  }
}

/**
 * @brief Inicializační funkce setup().
 *
 * Nastaví sériovou komunikaci, připojí se k WiFi, nakonfiguruje MQTT klienta,
 * inicializuje Neopixel pásek a vymaže jeho předchozí stav.
 */
void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  // Nastavení MQTT klienta
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  // Inicializace Neopixel pásku
  strip.begin();            // Inicializace knihovny Neopixel
  strip.setBrightness(100); // Nastavení jasu LED
  wipe();                 // Vymazání předchozího stavu LED (vypnutí)
}

/**
 * @brief Aktualizuje vzor LED na Neopixel pásku.
 *
 * Na základě hodnoty parametru "pat" (mód) zavolá příslušnou funkci
 * pro vykreslení vzoru nebo nastavení barev.
 *
 * @param pat Index módu (0 = rainbow, 1 = statická barva).
 */
void updatePattern(int pat) {
  switch(pat) {
    case 1:
      // Nastaví všechny LED na barvu definovanou proměnnými lamp_R, lamp_G a lamp_B
      colorWipe(strip.Color(lamp_R, lamp_G, lamp_B));
      // Možnost ladění: výpis hodnot barev do sériového monitoru
      // Serial.println("Hodnoty ze switche:");
      // Serial.println(lamp_R);
      // Serial.println(lamp_G);
      // Serial.println(lamp_B);
      strip.show();
      break;
    case 0:
      // Zobrazí režim rainbow
      rainbow();
      break;
  }
}

/**
 * @brief Zobrazí efekt "rainbow" na Neopixel pásku.
 *
 * Postupně mění barvy LED pomocí funkce Wheel.
 */
void rainbow() {
  static uint16_t j = 0;
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel((i + j) & 255));
  }
  strip.show();
  j++;
  if(j >= 256) j = 0;
}

/**
 * @brief Nastaví všechny LED na zadanou barvu.
 *
 * @param c Barva ve formátu RGB.
 */
void colorWipe(uint32_t c) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
}

/**
 * @brief Vypne všechny LED na Neopixel pásku.
 */
void wipe() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
}

/**
 * @brief Vrací barvu podle pozice pro efekt "rainbow".
 *
 * @param WheelPos Pozice (0-255) v "kolovém" přechodu barev.
 * @return uint32_t Vrácená barva ve formátu RGB.
 */
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

/**
 * @brief Hlavní funkce loop().
 *
 * Zajišťuje opakované spouštění hlavních funkcí:
 * - Kontroluje MQTT připojení a zpracovává zprávy.
 * - Aktualizuje Neopixel efekt podle nastaveného intervalu.
 * - Čte data z DHT11 senzoru a odesílá je ve formátu JSON.
 */
void loop() {
  // Pokud není MQTT klient připojen, pokusí se o opětovné připojení
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  char buffer[350];

  // Aktualizace LED podle nastaveného intervalu
  patternInterval = intervals[pattern]; // Rychlost efektu podle aktuálního módu
  if(millis() - lastUpdateNeopixel >= patternInterval) {
    updatePattern(pattern);
    lastUpdateNeopixel = millis();
  }

  // Čtení dat z DHT11 senzoru
  DHT.read(DHT11_PIN);
  // Pro ladění lze využít následující výpis
  // Serial.print(DHT.temperature);
  // Serial.print(DHT.humidity);

  // Odesílání dat ve formátu JSON každé 2 sekundy
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
