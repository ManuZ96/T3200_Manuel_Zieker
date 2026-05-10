#include <time.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <WebServer.h>
#include <DHT.h> // Sensor

// ==================================================
// Globale Konfiguration: WLAN
// ==================================================
const char* WIFI_SSID     = "ubnt2.4";
const char* WIFI_PASSWORD = "ubnt-zieker2.4";

// ==================================================
// Globale Konfiguration: HTTP-Server
// ==================================================
WebServer server(80);

// ==================================================
// Globale Konfiguration: LED-Streifen
// ==================================================
#define ONBOARD_RGB_PIN 8
#define ONBOARD_RGB_COUNT 1

#define LED_PIN            1
#define LED_COUNT          256

#define LED_PIN_SEC        7
#define LED_COUNT_SEC      60

#define LED_PIN_THIRD       10
#define LED_COUNT_THIRD     38

#define STRIP_BRIGHTNESS   255

Adafruit_NeoPixel onboardLed(1, ONBOARD_RGB_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel secondStrip(LED_COUNT_SEC, LED_PIN_SEC, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel thirdStrip(LED_COUNT_THIRD, LED_PIN_THIRD, NEO_GRB + NEO_KHZ800);

// ==================================================
// Globale Konfiguration: Zeit und NTP
// ==================================================
const char* TZ_INFO     = "CET-1CEST,M3.5.0,M10.5.0/3";
const char* NTP_SERVER1 = "pool.ntp.org";
const char* NTP_SERVER2 = "time.nist.gov";

// ==================================================
// Globale Konfiguration: Sensor
// ==================================================
#define DHT_PIN   6
#define DHT_TYPE  DHT22

DHT dht(DHT_PIN, DHT_TYPE);

float indoorTemperature = 0.0;
float indoorHumidity = 0.0;
bool indoorSensorValid = false;

unsigned long lastIndoorSensorUpdate = 0;
const unsigned long INDOOR_SENSOR_UPDATE_INTERVAL_MS = 30UL * 1000UL;

// ==================================================
// Globale Variablen: Zeitstatus
// ==================================================
int currentHour   = 0;
int currentMinute = 0;
int currentSecond = 0;
int lastSyncDayOfYear = -1;
bool dailySyncDone = false;

// ==================================================
// Globale Variablen: Lampenzustand
// ==================================================
bool lampPower = true;
int webBrightness = 100;   // 0..100

uint8_t ledR = 150;
uint8_t ledG = 150;
uint8_t ledB = 150;

// ==================================================
// Globale Variablen: Wetterdaten
// ==================================================
const float WEATHER_LAT = 48.86339;
const float WEATHER_LON = 9.24852;

float outsideTemperature = 0.0;
int weatherCode = -1;
int isDay = 1;

bool weatherValid = false;
unsigned long lastWeatherUpdate = 0;
const unsigned long WEATHER_UPDATE_INTERVAL_MS = 15UL * 60UL * 1000UL;

// ==================================================
// Wortdefinitionen der Uhr
// ==================================================

// --------------------------------------------------
// Dauerhaft aktive Woerter (strip)
// --------------------------------------------------
int IN[]            = {0, 1};
int REMSECK[]       = {4, 5, 6, 7, 8, 9, 10};
int IST[]           = {13, 14, 15};
int ES[]            = {30, 31};
int MIT[]           ={132, 133, 134};
int TEMPERATUREN[]  ={192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203};
int UND[]           ={205, 206, 207};

// --------------------------------------------------
// Stundenwoerter (strip)
// --------------------------------------------------
int HOUR_1_EIN[]  = {66, 67, 68};
int HOUR_1_EINS[]  = {66, 67, 68, 69};

int HOUR_2[]  = {71, 72, 73, 74};
int HOUR_3[]  = {86, 87, 88, 89};
int HOUR_4[]  = {76, 77, 78, 79};
int HOUR_5[]  = {92, 93, 94, 95};
int HOUR_6[]  = {102, 103, 104, 105, 106};
int HOUR_7[]  = {122, 123, 124, 125, 126, 127};
int HOUR_8[]  = {97, 98, 99, 100};
int HOUR_9[]  = {108, 109, 110, 111};
int HOUR_10[] = {80, 81, 82, 83};
int HOUR_11[] = {112, 113, 114};

int HOUR_12[] = {116, 117, 118, 119, 120};
int HOUR_0[]  = {48, 49, 50, 51};

// --------------------------------------------------
// Minutenwoerter (strip)
// --------------------------------------------------
int MINUTE_FUENF[]        = {25, 26, 27, 28};
int MINUTE_ZEHN[]         = {21, 22, 23, 24};
int MINUTE_FUENFZEHN[]    = {21, 22, 23, 24, 25, 26, 27, 28};
int MINUTE_NACH[]         = {33, 34, 35, 36};
int MINUTE_VOR[]          = {39, 40, 41};
int MINUTE_PLUS1[]        = {19};
int MINUTE_PLUS2[]        = {18, 19};
int MINUTE_PLUS3[]        = {17, 18, 19};
int MINUTE_PLUS4[]        = {16, 17, 18, 19};
int MINUTE_VIERTEL[]      = {53, 54, 55, 56, 57, 58, 59};
int MINUTE_HALB[]         = {44, 45, 46, 47};
int MINUTE_DREIVIERTEL[]  = {53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};
int UHR[]                 = {128, 129, 130};

// --------------------------------------------------
// Aussentemperaturwoerter (strip)
// --------------------------------------------------
int EISIGEN[]       = {177, 178, 179, 180, 181, 182, 183}; 
int KUEHLEN[]        = {185, 186, 187, 188, 189, 190};
int FRISCHEN[]      = {136, 137, 138, 139, 140, 141, 142, 143};
int WARMEN[]        = {144, 145, 146, 147, 148, 149};
int SOMMERLICHEN[]  = {162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173};
int HEISSEN[]       = {152, 153, 154, 155, 156, 157, 158};

// --------------------------------------------------
// Wetterwoerter (strip)
// --------------------------------------------------
int KLAREM_HIMMEL[]     = {234, 235, 236, 237, 238, 239, 250, 251, 252, 253, 254, 255};
int BEWOELKTEM_HIMMEL[]  = {224, 225, 226, 227, 228, 229, 230, 231, 232, 250, 251, 252, 253, 254, 255};
int REGEN[]             = {219, 220, 221, 222, 223};
int SCHNEE[]            = {213, 214, 215, 216, 218};
int HAGEL[]             = {208, 209, 210, 211, 212};
int GEWITTER[]          = {241, 242, 243, 244, 245, 246, 247, 248};

// --------------------------------------------------
// Raumfeuchtigkeit-LEDs (thirdStrip)
// --------------------------------------------------
int ZEHN_PROZENT[]      = {0};
int ZWANZIG_PROZENT[]   = {1};
int DREISIG_PROZENT[]   = {2};
int VIERZIG_PROZENT[]   = {3};
int FUENFZIG_PROZENT[]   = {4};
int SECHZIG_PROZENT[]   = {5};
int SIEBZIG_PROZENT[]   = {6};
int ACHTZIG_PROZENT[]   = {7};
int NEUNZIG_PROZENT[]   = {8};
int EINHUNDERT_PROZENT[]= {9};

// --------------------------------------------------
// Mondphasen-LEDs (thirdStrip)
// --------------------------------------------------
int ZUNEHMEND_SICHEL[]      = {10};
int ZUNEHMEND_HALB[]        = {12};
int VOLLMOND[]              = {14};
int ABNEHMEND_DREIVIERTEL[] = {16};
int ABNEHMEND_VIERTEL[]     = {18};
int ABNEHMEND_SICHEL[]      = {20};

// --------------------------------------------------
// Innenraumtemperatur-LEDs (thirdStrip)
// --------------------------------------------------
int TEMP_18_GRAD[] = {21};
int TEMP_19_GRAD[] = {22};
int TEMP_20_GRAD[] = {23};
int TEMP_21_GRAD[] = {24};
int TEMP_22_GRAD[] = {25};
int TEMP_23_GRAD[] = {26};
int TEMP_24_GRAD[] = {27};
int TEMP_25_GRAD[] = {28};
int TEMP_26_GRAD[] = {29};
int TEMP_27_GRAD[] = {30};

// --------------------------------------------------
// Tageszeit-LEDs (thirdStrip)
// --------------------------------------------------
int MORGEN[] = {37};  // 04:00 - 09:59
int MITTAG[] = {35};  // 10:00 - 15:59
int ABEND[]  = {33};  // 16:00 - 21:59
int NACHT[]  = {31};  // 22:00 - 03:59

// ==================================================
// Wettergruppen zur Klassifikation der Wettercodes
// ==================================================
enum WeatherGroup {
  WEATHER_UNKNOWN,
  WEATHER_CLEAR,
  WEATHER_CLOUDY,
  WEATHER_RAIN,
  WEATHER_SNOW,
  WEATHER_HAIL,
  WEATHER_STORM,
  WEATHER_FOG
};

// ==================================================
// Funktionsprototypen
// ==================================================
void setLed(int ledNumber, uint8_t r, uint8_t g, uint8_t b);
void setLeds(const int leds[], int count, uint8_t r, uint8_t g, uint8_t b);

void setThirdLed(int ledNumber, uint8_t r, uint8_t g, uint8_t b);
void setThirdLeds(const int leds[], int count, uint8_t r, uint8_t g, uint8_t b);

void bootModeUpdate();
void bootModeOff();
String rgbToHex(uint8_t r, uint8_t g, uint8_t b);

uint8_t applyBrightness(uint8_t colorValue);

bool updateTimeVariables();
bool syncTimeFromServer();
void syncTimeUntilSuccessAtStartup();
bool syncTimeWithMaxRetries(int maxRetries);
void checkDailyResync();

WeatherGroup classifyWeatherCode(int code);
bool updateWeatherFromServer();
void checkPeriodicWeatherUpdate();

void connectToWiFi();

String buildHtmlPage();
String buildJsonStatus();
void handleRoot();
void handleApiStatus();
void handleSet();
void handleWeatherNow();
void handleNotFound();
void setupHttpServer();

void updateIndoorSensor();
void checkPeriodicIndoorSensorUpdate();

void showDaytimeStatus();
void showHumidityStatus();
void showIndoorTemperatureStatus();
void showMoonPhaseStatus();

int calculateMoonAge();

void showAlwaysOnLeds();
void showWeatherStatusLeds();
void showSecondLed(int second);
void showThirdStripStatus();
void showHourNumber(int hour12, int minute);
void showMinuteWords(int minute);
void showMinutePlus(int plusMinutes);
int getDisplayHour(int hour24, int minute);
void showHourWord(int hour24);
void showOutsideTemperatureWords();
void showWeatherWords();

// ==================================================
// LED-Hilfsfunktionen
// Grundlegende Ansteuerung einzelner und mehrerer LEDs
// sowie Anpassung der Farbwerte an die Helligkeit.
// ==================================================

// --------------------------------------------------
// Setzt eine einzelne LED auf einen RGB-Farbwert.
// --------------------------------------------------
void setLed(int ledNumber, uint8_t r, uint8_t g, uint8_t b) {
  if (ledNumber >= 0 && ledNumber < LED_COUNT) {
    strip.setPixelColor(ledNumber, strip.Color(r, g, b));
  }
}

// --------------------------------------------------
// Setzt mehrere LEDs aus einem Array auf denselben
// RGB-Farbwert.
// --------------------------------------------------
void setLeds(const int leds[], int count, uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < count; i++) {
    setLed(leds[i], r, g, b);
  }
}

void setThirdLed(int ledNumber, uint8_t r, uint8_t g, uint8_t b) {
  if (ledNumber >= 0 && ledNumber < LED_COUNT_THIRD) {
    thirdStrip.setPixelColor(ledNumber, thirdStrip.Color(r, g, b));
  }
}

void setThirdLeds(const int leds[], int count, uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < count; i++) {
    setThirdLed(leds[i], r, g, b);
  }
}

// --------------------------------------------------
// Skaliert einen Farbkanal mit der eingestellten
// Helligkeit.
// --------------------------------------------------
uint8_t applyBrightness(uint8_t colorValue) {
  return (uint8_t)((colorValue * webBrightness) / 100);
}

void bootModeUpdate() {
  static int state = 0;
  static int index = 0;
  static unsigned long lastStep = 0;
  static unsigned long waitStart = 0;

  const unsigned long STEP_INTERVAL_MS = 8;
  const unsigned long WAIT_INTERVAL_MS = 1000;

  unsigned long now = millis();

  switch (state) {
    case 0: // vorwaerts einschalten
      if (now - lastStep >= STEP_INTERVAL_MS) {
        lastStep = now;

        strip.setPixelColor(index, strip.Color(ledR, ledG, ledB));
        strip.show();

        index++;

        if (index >= LED_COUNT) {
          index = LED_COUNT - 1;
          waitStart = now;
          state = 1;
        }
      }
      break;

    case 1: // 1 Sekunde komplett an
      if (now - waitStart >= WAIT_INTERVAL_MS) {
        state = 2;
      }
      break;

    case 2: // rueckwaerts ausschalten
      if (now - lastStep >= STEP_INTERVAL_MS) {
        lastStep = now;

        strip.setPixelColor(index, strip.Color(0, 0, 0));
        strip.show();

        index--;

        if (index < 0) {
          index = 0;
          waitStart = now;
          state = 3;
        }
      }
      break;

    case 3: // 1 Sekunde komplett aus
      if (now - waitStart >= WAIT_INTERVAL_MS) {
        state = 0;
      }
      break;
  }
}

void bootModeOff() {
  strip.clear();
  strip.show();
}

// ==================================================
// Zeitfunktionen
// Lesen der lokalen Zeit, NTP-Synchronisation beim
// Start und taegliche Nachsynchronisation.
// ==================================================

// --------------------------------------------------
// Liest die lokale Zeit und speichert Stunde, Minute
// und Sekunde in globalen Variablen.
// --------------------------------------------------
bool updateTimeVariables() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 50)) {
    return false;
  }

  currentHour   = timeinfo.tm_hour;
  currentMinute = timeinfo.tm_min;
  currentSecond = timeinfo.tm_sec;

  return true;
}

// --------------------------------------------------
// Synchronisiert die Zeit ueber NTP-Server und
// aktualisiert die Zeitvariablen.
// --------------------------------------------------
bool syncTimeFromServer() {
  configTzTime(TZ_INFO, NTP_SERVER1, NTP_SERVER2);

  struct tm timeinfo;
  bool ok = getLocalTime(&timeinfo, 10000);

  if (ok) {
    currentHour   = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSecond = timeinfo.tm_sec;
    lastSyncDayOfYear = timeinfo.tm_yday;

    Serial.printf("Zeit geholt: %02d:%02d:%02d\n",
                  currentHour, currentMinute, currentSecond);
  } else {
    Serial.println("Zeitabfrage fehlgeschlagen.");
  }

  return ok;
}

// --------------------------------------------------
// Wiederholt die Start-Synchronisation, bis eine
// gueltige Zeit verfuegbar ist.
// --------------------------------------------------
void syncTimeUntilSuccessAtStartup() {
  bool syncOk = false;

  while (!syncOk) {
    configTzTime(TZ_INFO, NTP_SERVER1, NTP_SERVER2);

    unsigned long startAttempt = millis();

    while (millis() - startAttempt < 10000) {
      bootModeUpdate();

      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 50)) {
        currentHour = timeinfo.tm_hour;
        currentMinute = timeinfo.tm_min;
        currentSecond = timeinfo.tm_sec;
        lastSyncDayOfYear = timeinfo.tm_yday;

        Serial.printf("Zeit geholt: %02d:%02d:%02d\n", currentHour, currentMinute, currentSecond);
        syncOk = true;
        break;
      }
    }

    if (!syncOk) {
      Serial.println("Start-Sync fehlgeschlagen, neuer Versuch...");
    }
  }
}

// --------------------------------------------------
// Fuehrt eine begrenzte Anzahl an Sync-Versuchen aus.
// --------------------------------------------------
bool syncTimeWithMaxRetries(int maxRetries) {
  for (int i = 1; i <= maxRetries; i++) {
    Serial.print("Sync-Versuch ");
    Serial.print(i);
    Serial.print(" von ");
    Serial.println(maxRetries);

    if (syncTimeFromServer()) {
      return true;
    }

    delay(2000);
  }

  return false;
}

// --------------------------------------------------
// Prueft nach Mitternacht auf eine erneute
// Zeitsynchronisation.
// --------------------------------------------------
void checkDailyResync() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 50)) {
    return;
  }

  if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && timeinfo.tm_sec >= 5) {
    if (!dailySyncDone && timeinfo.tm_yday != lastSyncDayOfYear) {
      Serial.println("Neuer Tag -> neue Zeitsynchronisation");

      bool syncOk = syncTimeWithMaxRetries(10);

      if (!syncOk) {
        Serial.println("Alle 10 Sync-Versuche fehlgeschlagen.");
        Serial.println("Alte Zeit wird weiterverwendet.");
      }

      if (updateTimeVariables()) {
        showHourWord(currentHour);
      }

      dailySyncDone = true;
    }
  }

  if (timeinfo.tm_hour != 0 || timeinfo.tm_min != 0) {
    dailySyncDone = false;
  }
}

// ==================================================
// Wetterfunktionen
// Klassifikation der Wettercodes, Abruf aktueller
// Wetterdaten und periodische Aktualisierung.
// ==================================================

// --------------------------------------------------
// Ordnet einen numerischen Wettercode einer
// Wettergruppe zu.
// --------------------------------------------------
WeatherGroup classifyWeatherCode(int code) {
  if (code == 0) {
    return WEATHER_CLEAR;
  }

  if (code >= 1 && code <= 3) {
    return WEATHER_CLOUDY;
  }

  if (code == 45 || code == 48) {
    return WEATHER_FOG;
  }

  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
    return WEATHER_RAIN;
  }

  if (code >= 71 && code <= 77) {
    return WEATHER_SNOW;
  }

  if (code == 96 || code == 99) {
    return WEATHER_HAIL;
  }

  if (code == 95) {
    return WEATHER_STORM;
  }

  return WEATHER_UNKNOWN;
}

// --------------------------------------------------
// Ruft aktuelle Wetterdaten per HTTPS ab und speichert
// Temperatur, Wettercode und Tag-/Nachtstatus.
// --------------------------------------------------
bool updateWeatherFromServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wetterabfrage nicht moeglich: WLAN nicht verbunden.");
    weatherValid = false;
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += String(WEATHER_LAT, 4);
  url += "&longitude=";
  url += String(WEATHER_LON, 4);
  url += "&current=temperature_2m,weather_code,is_day";

  Serial.println("Wetterabfrage startet...");
  Serial.println(url);

  if (!http.begin(client, url)) {
    Serial.println("HTTP begin fehlgeschlagen.");
    weatherValid = false;
    return false;
  }

  int httpCode = http.GET();

  if (httpCode <= 0) {
    Serial.printf("HTTP-Fehler bei Wetterabfrage: %d\n", httpCode);
    http.end();
    weatherValid = false;
    return false;
  }

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Server antwortet mit HTTP-Code: %d\n", httpCode);
    http.end();
    weatherValid = false;
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("JSON-Fehler: ");
    Serial.println(error.c_str());
    weatherValid = false;
    return false;
  }

  if (!doc["current"].is<JsonObject>()) {
    Serial.println("JSON enthaelt kein current-Objekt.");
    weatherValid = false;
    return false;
  }

  outsideTemperature = doc["current"]["temperature_2m"] | 0.0;
  weatherCode = doc["current"]["weather_code"] | -1;
  isDay = doc["current"]["is_day"] | 1;

  weatherValid = true;
  lastWeatherUpdate = millis();

  Serial.printf("Wetter aktualisiert -- Temp=%.1f C | Code=%d | Tag=%d\n",
                outsideTemperature, weatherCode, isDay);

  return true;
}

// --------------------------------------------------
// Prueft, ob neue Wetterdaten geladen werden sollen.
// --------------------------------------------------
void checkPeriodicWeatherUpdate() {
  unsigned long nowMs = millis();

  if (!weatherValid || (nowMs - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL_MS)) {
    updateWeatherFromServer();
  }
}

// ==================================================
// WLAN-Funktionen
// Verbindung des ESP32 mit dem lokalen WLAN.
// ==================================================

// --------------------------------------------------
// Baut die WLAN-Verbindung auf und gibt die IP aus.
// --------------------------------------------------
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Verbinde mit WLAN");

  while (WiFi.status() != WL_CONNECTED) {
    bootModeUpdate();

    static unsigned long lastDot = 0;
    if (millis() - lastDot >= 500) {
      lastDot = millis();
      Serial.print(".");
    }
  }

  Serial.println();
  Serial.println("WLAN verbunden.");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());
}

// ==================================================
// HTTP- und Webserver-Funktionen
// HTML-Ausgabe, JSON-Ausgabe und Verarbeitung der
// Browseranfragen.
// ==================================================

String rgbToHex(uint8_t r, uint8_t g, uint8_t b) {
  char buffer[8];
  snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", r, g, b);
  return String(buffer);
}

// --------------------------------------------------
// Erzeugt die HTML-Statusseite fuer den Browser.
// --------------------------------------------------
String buildHtmlPage() {
  String colorHex = rgbToHex(ledR, ledG, ledB);

  String html = "";
  html += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>WordClock</title>";

  html += "<style>";
  html += "body{font-family:Arial,sans-serif;background:#111;color:#eee;margin:20px;}";
  html += "h1{color:#fff;}";
  html += ".card{background:#1e1e1e;padding:16px;border-radius:12px;margin-bottom:16px;}";
  html += ".grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;max-width:520px;}";
  html += ".label{color:#aaa;}";
  html += "a,button{display:inline-block;margin:6px 6px 6px 0;padding:10px 14px;background:#2d6cdf;color:white;text-decoration:none;border-radius:8px;border:none;}";
  html += "input[type=range]{width:100%;max-width:280px;}";
  html += "input[type=color]{width:80px;height:44px;border:none;background:none;}";
  html += "</style>";

  html += "<script>";
  html += "function colorChanged(){";
  html += "let c=document.getElementById('color').value;";
  html += "document.getElementById('r').value=parseInt(c.substr(1,2),16);";
  html += "document.getElementById('g').value=parseInt(c.substr(3,2),16);";
  html += "document.getElementById('b').value=parseInt(c.substr(5,2),16);";
  html += "document.getElementById('colorText').innerText=c;";
  html += "}";
  html += "function brightnessChanged(v){document.getElementById('brightnessText').innerText=v+' %';}";
  html += "</script>";

  html += "</head><body>";
  html += "<h1>WordClock</h1>";

  html += "<div class='card'>";
  html += "<h2>Anzeige</h2>";
  html += "<div class='grid'>";

  char timeBuffer[16];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);

  html += "<div class='label'>Uhrzeit</div><div>" + String(timeBuffer) + "</div>";
  html += "<div class='label'>Power</div><div>" + String(lampPower ? "AN" : "AUS") + "</div>";
  html += "<div class='label'>Helligkeit</div><div>" + String(webBrightness) + " %</div>";
  html += "<div class='label'>Farbe</div><div>" + colorHex + " / RGB(" + String(ledR) + "," + String(ledG) + "," + String(ledB) + ")</div>";
  html += "<div class='label'>Aussentemperatur</div><div>" + String(outsideTemperature, 1) + " &deg;C</div>";
  html += "<div class='label'>Wettercode</div><div>" + String(weatherCode) + "</div>";
  html += "<div class='label'>Wetter gueltig</div><div>" + String(weatherValid ? "ja" : "nein") + "</div>";
  html += "<div class='label'>Tag/Nacht</div><div>" + String(isDay ? "Tag" : "Nacht") + "</div>";
  html += "<div class='label'>Innenraumtemperatur</div><div>" + String(indoorTemperature, 1) + " &deg;C</div>";
  html += "<div class='label'>Luftfeuchtigkeit</div><div>" + String(indoorHumidity, 1) + " %</div>";
  html += "<div class='label'>Innenraumsensor</div><div>" + String(indoorSensorValid ? "gueltig" : "ungueltig") + "</div>";
  html += "<div class='label'>IP</div><div>" + WiFi.localIP().toString() + "</div>";

  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h2>Steuerung</h2>";
  html += "<a href='/set?power=1'>EIN</a>";
  html += "<a href='/set?power=0'>AUS</a>";
  html += "<a href='/weather'>Wetter aktualisieren</a>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h2>Farbe / Helligkeit</h2>";
  html += "<form action='/set' method='get'>";
  html += "<p>Farbe: <input id='color' type='color' value='" + colorHex + "' oninput='colorChanged()'> ";
  html += "<span id='colorText'>" + colorHex + "</span></p>";

  html += "<input id='r' name='r' type='hidden' value='" + String(ledR) + "'>";
  html += "<input id='g' name='g' type='hidden' value='" + String(ledG) + "'>";
  html += "<input id='b' name='b' type='hidden' value='" + String(ledB) + "'>";

  html += "<p>Helligkeit: <span id='brightnessText'>" + String(webBrightness) + " %</span></p>";
  html += "<input name='brightness' type='range' min='0' max='100' value='" + String(webBrightness) + "' oninput='brightnessChanged(this.value)'>";

  html += "<p><button type='submit'>Speichern</button></p>";
  html += "</form>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h2>API</h2>";
  html += "<p><a href='/api/status'>JSON Status</a></p>";
  html += "</div>";

  html += "</body></html>";

  return html;
}

// --------------------------------------------------
// Erzeugt eine JSON-Ausgabe des Systemzustands.
// --------------------------------------------------
String buildJsonStatus() {
  DynamicJsonDocument doc(1024);

  doc["ip"] = WiFi.localIP().toString();
  doc["lampPower"] = lampPower;
  doc["brightness"] = webBrightness;
  doc["ledR"] = ledR;
  doc["ledG"] = ledG;
  doc["ledB"] = ledB;

  doc["currentHour"] = currentHour;
  doc["currentMinute"] = currentMinute;
  doc["currentSecond"] = currentSecond;
  doc["lastSyncDayOfYear"] = lastSyncDayOfYear;
  doc["dailySyncDone"] = dailySyncDone;

  doc["outsideTemperature"] = outsideTemperature;
  doc["weatherCode"] = weatherCode;
  doc["isDay"] = isDay;
  doc["weatherValid"] = weatherValid;
  doc["lastWeatherUpdateMs"] = lastWeatherUpdate;

  doc["indoorTemperature"] = indoorTemperature;
  doc["indoorHumidity"] = indoorHumidity;
  doc["indoorSensorValid"] = indoorSensorValid;
  doc["lastIndoorSensorUpdateMs"] = lastIndoorSensorUpdate;

  String json;
  serializeJsonPretty(doc, json);
  return json;
}

// --------------------------------------------------
// Liefert die Startseite aus.
// --------------------------------------------------
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", buildHtmlPage());
}

// --------------------------------------------------
// Liefert den Status als JSON aus.
// --------------------------------------------------
void handleApiStatus() {
  server.send(200, "application/json; charset=utf-8", buildJsonStatus());
}

// --------------------------------------------------
// Verarbeitet Parameter fuer Power, Helligkeit und
// RGB-Farbwerte.
// --------------------------------------------------
void handleSet() {
  bool changed = false;

  if (server.hasArg("power")) {
    int p = server.arg("power").toInt();
    lampPower = (p != 0);
    changed = true;
  }

  if (server.hasArg("brightness")) {
    int b = server.arg("brightness").toInt();
    if (b < 0) b = 0;
    if (b > 100) b = 100;
    webBrightness = b;
    changed = true;
  }

  if (server.hasArg("r")) {
    int v = server.arg("r").toInt();
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    ledR = (uint8_t)v;
    changed = true;
  }

  if (server.hasArg("g")) {
    int v = server.arg("g").toInt();
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    ledG = (uint8_t)v;
    changed = true;
  }

  if (server.hasArg("b")) {
    int v = server.arg("b").toInt();
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    ledB = (uint8_t)v;
    changed = true;
  }

  if (changed) {
    showHourWord(currentHour);

    Serial.printf("HTTP SET -> On=%d Brightness=%d RGB=(%d,%d,%d)\n",
                  lampPower, webBrightness, ledR, ledG, ledB);
  }

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "OK");
}

// --------------------------------------------------
// Startet eine manuelle Aktualisierung der Wetterdaten.
// --------------------------------------------------
void handleWeatherNow() {
  bool ok = updateWeatherFromServer();
  showHourWord(currentHour);

  if (ok) {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Weather updated");
  } else {
    server.send(500, "text/plain", "Weather update failed");
  }
}

// --------------------------------------------------
// Antwort fuer unbekannte Pfade.
// --------------------------------------------------
void handleNotFound() {
  String message = "404 Not Found\n\n";
  message += "Verfuegbare Pfade:\n";
  message += "/\n";
  message += "/api/status\n";
  message += "/set?power=1&brightness=80&r=255&g=100&b=20\n";
  message += "/weather\n";
  server.send(404, "text/plain; charset=utf-8", message);
}

// --------------------------------------------------
// Registriert die Routen und startet den HTTP-Server.
// --------------------------------------------------
void setupHttpServer() {
  server.on("/", handleRoot);
  server.on("/api/status", handleApiStatus);
  server.on("/set", handleSet);
  server.on("/weather", handleWeatherNow);
  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("HTTP-Server gestartet.");
  Serial.print("Im Browser oeffnen: http://");
  Serial.println(WiFi.localIP());
}

// ==================================================
// Zusatzfunktionen fuer Sensorik und Zusatzanzeige
// ==================================================

// --------------------------------------------------
// Sensorwerte des DHT22 einlesen.
// --------------------------------------------------
void updateIndoorSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("DHT-Sensor konnte nicht gelesen werden.");
    indoorSensorValid = false;
    return;
  }

  indoorHumidity = h;
  indoorTemperature = t;
  indoorSensorValid = true;
  lastIndoorSensorUpdate = millis();

  Serial.printf("Innenraum -> Temp=%.1f C | Feuchte=%.1f %%\n",
                indoorTemperature, indoorHumidity);
}

void checkPeriodicIndoorSensorUpdate() {
  unsigned long nowMs = millis();

  if (!indoorSensorValid || nowMs - lastIndoorSensorUpdate >= INDOOR_SENSOR_UPDATE_INTERVAL_MS) {
    updateIndoorSensor();
  }
}

// --------------------------------------------------
// Tageszeit auf dem Zusatzstreifen anzeigen.
// --------------------------------------------------
void showDaytimeStatus() {
  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  if (currentHour >= 4 && currentHour < 10) {
    setThirdLeds(MORGEN, sizeof(MORGEN)/sizeof(MORGEN[0]), r, g, b);
  } else if (currentHour >= 10 && currentHour < 16) {
    setThirdLeds(MITTAG, sizeof(MITTAG)/sizeof(MITTAG[0]), r, g, b);
  } else if (currentHour >= 16 && currentHour < 22) {
    setThirdLeds(ABEND, sizeof(ABEND)/sizeof(ABEND[0]), r, g, b);
  } else {
    setThirdLeds(NACHT, sizeof(NACHT)/sizeof(NACHT[0]), r, g, b);
  }
}

// --------------------------------------------------
// Raumfeuchtigkeit anzeigen.
// --------------------------------------------------
void showHumidityStatus() {
  if (!indoorSensorValid) return;

  int humidityRounded = round(indoorHumidity / 10.0) * 10;

  if (humidityRounded < 10) humidityRounded = 10;
  if (humidityRounded > 100) humidityRounded = 100;

  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  switch (humidityRounded) {
    case 10:  setThirdLeds(ZEHN_PROZENT, sizeof(ZEHN_PROZENT)/sizeof(ZEHN_PROZENT[0]), r, g, b); break;
    case 20:  setThirdLeds(ZWANZIG_PROZENT, sizeof(ZWANZIG_PROZENT)/sizeof(ZWANZIG_PROZENT[0]), r, g, b); break;
    case 30:  setThirdLeds(DREISIG_PROZENT, sizeof(DREISIG_PROZENT)/sizeof(DREISIG_PROZENT[0]), r, g, b); break;
    case 40:  setThirdLeds(VIERZIG_PROZENT, sizeof(VIERZIG_PROZENT)/sizeof(VIERZIG_PROZENT[0]), r, g, b); break;
    case 50:  setThirdLeds(FUENFZIG_PROZENT, sizeof(FUENFZIG_PROZENT)/sizeof(FUENFZIG_PROZENT[0]), r, g, b); break;
    case 60:  setThirdLeds(SECHZIG_PROZENT, sizeof(SECHZIG_PROZENT)/sizeof(SECHZIG_PROZENT[0]), r, g, b); break;
    case 70:  setThirdLeds(SIEBZIG_PROZENT, sizeof(SIEBZIG_PROZENT)/sizeof(SIEBZIG_PROZENT[0]), r, g, b); break;
    case 80:  setThirdLeds(ACHTZIG_PROZENT, sizeof(ACHTZIG_PROZENT)/sizeof(ACHTZIG_PROZENT[0]), r, g, b); break;
    case 90:  setThirdLeds(NEUNZIG_PROZENT, sizeof(NEUNZIG_PROZENT)/sizeof(NEUNZIG_PROZENT[0]), r, g, b); break;
    case 100: setThirdLeds(EINHUNDERT_PROZENT, sizeof(EINHUNDERT_PROZENT)/sizeof(EINHUNDERT_PROZENT[0]), r, g, b); break;
  }
}

// --------------------------------------------------
// Innenraumtemperatur anzeigen.
// --------------------------------------------------
void showIndoorTemperatureStatus() {
  if (!indoorSensorValid) return;

  int tempRounded = round(indoorTemperature);

  if (tempRounded < 18) tempRounded = 18;
  if (tempRounded > 28) tempRounded = 28;

  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  switch (tempRounded) {
    case 18: setThirdLeds(TEMP_18_GRAD, sizeof(TEMP_18_GRAD)/sizeof(TEMP_18_GRAD[0]), r, g, b); break;
    case 19: setThirdLeds(TEMP_19_GRAD, sizeof(TEMP_19_GRAD)/sizeof(TEMP_19_GRAD[0]), r, g, b); break;
    case 20: setThirdLeds(TEMP_20_GRAD, sizeof(TEMP_20_GRAD)/sizeof(TEMP_20_GRAD[0]), r, g, b); break;
    case 21: setThirdLeds(TEMP_21_GRAD, sizeof(TEMP_21_GRAD)/sizeof(TEMP_21_GRAD[0]), r, g, b); break;
    case 22: setThirdLeds(TEMP_22_GRAD, sizeof(TEMP_22_GRAD)/sizeof(TEMP_22_GRAD[0]), r, g, b); break;
    case 23: setThirdLeds(TEMP_23_GRAD, sizeof(TEMP_23_GRAD)/sizeof(TEMP_23_GRAD[0]), r, g, b); break;
    case 24: setThirdLeds(TEMP_24_GRAD, sizeof(TEMP_24_GRAD)/sizeof(TEMP_24_GRAD[0]), r, g, b); break;
    case 25: setThirdLeds(TEMP_25_GRAD, sizeof(TEMP_25_GRAD)/sizeof(TEMP_25_GRAD[0]), r, g, b); break;
    case 26: setThirdLeds(TEMP_26_GRAD, sizeof(TEMP_26_GRAD)/sizeof(TEMP_26_GRAD[0]), r, g, b); break;
    case 27: setThirdLeds(TEMP_27_GRAD, sizeof(TEMP_27_GRAD)/sizeof(TEMP_27_GRAD[0]), r, g, b); break;
    //case 28: setThirdLeds(TEMP_28_GRAD, sizeof(TEMP_28_GRAD)/sizeof(TEMP_28_GRAD[0]), r, g, b); break;
  }
}

// --------------------------------------------------
// Mondphase berechnen und anzeigen.
// --------------------------------------------------
int calculateMoonAge() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 50)) {
    return -1;
  }

  int year = timeinfo.tm_year + 1900;
  int month = timeinfo.tm_mon + 1;
  int day = timeinfo.tm_mday;

  if (month < 3) {
    year--;
    month += 12;
  }

  ++month;

  long c = 365.25 * year;
  long e = 30.6 * month;
  long jd = c + e + day - 694039.09;

  jd = jd % 29;

  return jd;
}

void showMoonPhaseStatus() {
  int moonAge = calculateMoonAge();
  if (moonAge < 0) return;

  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  if (moonAge < 6) {
    setThirdLeds(ZUNEHMEND_SICHEL, sizeof(ZUNEHMEND_SICHEL)/sizeof(ZUNEHMEND_SICHEL[0]), r, g, b);
  } else if (moonAge < 11) {
    setThirdLeds(ZUNEHMEND_HALB, sizeof(ZUNEHMEND_HALB)/sizeof(ZUNEHMEND_HALB[0]), r, g, b);
  } else if (moonAge < 15) {
    setThirdLeds(VOLLMOND, sizeof(VOLLMOND)/sizeof(VOLLMOND[0]), r, g, b);
  } else if (moonAge < 22) {
    setThirdLeds(ABNEHMEND_DREIVIERTEL, sizeof(ABNEHMEND_DREIVIERTEL)/sizeof(ABNEHMEND_DREIVIERTEL[0]), r, g, b);
  } else if (moonAge < 26) {
    setThirdLeds(ABNEHMEND_VIERTEL, sizeof(ABNEHMEND_VIERTEL)/sizeof(ABNEHMEND_VIERTEL[0]), r, g, b);
  } else {
    setThirdLeds(ABNEHMEND_SICHEL, sizeof(ABNEHMEND_SICHEL)/sizeof(ABNEHMEND_SICHEL[0]), r, g, b);
  }
}

// ==================================================
// Anzeige- und Wortuhrfunktionen
// Ausgabe auf dem LED-Streifen mit festen Woertern,
// Wetter-LEDs und Stundenanzeige.
// ==================================================

// --------------------------------------------------
// Aktiviert dauerhaft leuchtende Woerter der Wortuhr.
// --------------------------------------------------
void showAlwaysOnLeds() {
  if (!lampPower) return;

  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  setLeds(IN, sizeof(IN)/sizeof(IN[0]), r, g, b);
  setLeds(REMSECK, sizeof(REMSECK)/sizeof(REMSECK[0]), r, g, b);
  setLeds(IST, sizeof(IST)/sizeof(IST[0]), r, g, b);
  setLeds(ES, sizeof(ES)/sizeof(ES[0]), r, g, b);
}

// --------------------------------------------------
// Zeigt zusaetzliche Wetter- und Temperaturangaben.
// --------------------------------------------------
void showWeatherStatusLeds() {
  if (!lampPower || !weatherValid) return;

  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  setLeds(MIT, sizeof(MIT)/sizeof(MIT[0]), r, g, b);
  setLeds(TEMPERATUREN, sizeof(TEMPERATUREN)/sizeof(TEMPERATUREN[0]), r, g, b);
  setLeds(UND, sizeof(UND)/sizeof(UND[0]), r, g, b);

  showOutsideTemperatureWords();
  showWeatherWords();
}

// --------------------------------------------------
// Zeigt die aktuelle Sekunde auf dem Sekundenstreifen.
// --------------------------------------------------
void showSecondLed(int second) {
  secondStrip.clear();

  if (!lampPower) {
    secondStrip.show();
    return;
  }

  int ledIndex;

  if (second == 0) {
    ledIndex = LED_COUNT_SEC - 1;   // letzte LED
  } else {
    ledIndex = second - 1;
  }

  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  secondStrip.setPixelColor(ledIndex, secondStrip.Color(r, g, b));
  secondStrip.show();
}

// --------------------------------------------------
// Aktualisiert den Zusatzstreifen.
// --------------------------------------------------
void showThirdStripStatus() {
  thirdStrip.clear();

  if (!lampPower) {
    thirdStrip.show();
    return;
  }

  showHumidityStatus();
  showMoonPhaseStatus();
  showIndoorTemperatureStatus();
  showDaytimeStatus();

  thirdStrip.show();
}

// --------------------------------------------------
// Zeigt das passende Stundenwort von 1 bis 12 an.
// --------------------------------------------------
void showHourNumber(int hour12, int minute) {
  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  switch (hour12) {
    case 0:
      setLeds(HOUR_0, sizeof(HOUR_0)/sizeof(HOUR_0[0]), r, g, b);
      if (minute == 0) {
        setLeds(UHR, sizeof(UHR)/sizeof(UHR[0]), r, g, b);
      }
      break;

    case 1:
      if (minute == 0) {
        setLeds(HOUR_1_EIN, sizeof(HOUR_1_EIN)/sizeof(HOUR_1_EIN[0]), r, g, b);
      } else {
        setLeds(HOUR_1_EINS, sizeof(HOUR_1_EINS)/sizeof(HOUR_1_EINS[0]), r, g, b);
      }
      break;

    case 2:
      setLeds(HOUR_2, sizeof(HOUR_2)/sizeof(HOUR_2[0]), r, g, b);
      break;

    case 3:
      setLeds(HOUR_3, sizeof(HOUR_3)/sizeof(HOUR_3[0]), r, g, b);
      break;

    case 4:
      setLeds(HOUR_4, sizeof(HOUR_4)/sizeof(HOUR_4[0]), r, g, b);
      break;

    case 5:
      setLeds(HOUR_5, sizeof(HOUR_5)/sizeof(HOUR_5[0]), r, g, b);
      break;

    case 6:
      setLeds(HOUR_6, sizeof(HOUR_6)/sizeof(HOUR_6[0]), r, g, b);
      break;

    case 7:
      setLeds(HOUR_7, sizeof(HOUR_7)/sizeof(HOUR_7[0]), r, g, b);
      break;

    case 8:
      setLeds(HOUR_8, sizeof(HOUR_8)/sizeof(HOUR_8[0]), r, g, b);
      break;

    case 9:
      setLeds(HOUR_9, sizeof(HOUR_9)/sizeof(HOUR_9[0]), r, g, b);
      break;

    case 10:
      setLeds(HOUR_10, sizeof(HOUR_10)/sizeof(HOUR_10[0]), r, g, b);
      break;

    case 11:
      setLeds(HOUR_11, sizeof(HOUR_11)/sizeof(HOUR_11[0]), r, g, b);
      break;

    case 12:
      setLeds(HOUR_12, sizeof(HOUR_12)/sizeof(HOUR_12[0]), r, g, b);
      break;

    default:
      break;
  }
}

// --------------------------------------------------
// Zeigt zusaetzliche Minuten 1 bis 4 an.
// --------------------------------------------------
void showMinutePlus(int plusMinutes) {
  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  switch (plusMinutes) {
    case 1:
      setLeds(MINUTE_PLUS1, sizeof(MINUTE_PLUS1)/sizeof(MINUTE_PLUS1[0]), r, g, b);
      break;

    case 2:
      setLeds(MINUTE_PLUS2, sizeof(MINUTE_PLUS2)/sizeof(MINUTE_PLUS2[0]), r, g, b);
      break;

    case 3:
      setLeds(MINUTE_PLUS3, sizeof(MINUTE_PLUS3)/sizeof(MINUTE_PLUS3[0]), r, g, b);
      break;

    case 4:
      setLeds(MINUTE_PLUS4, sizeof(MINUTE_PLUS4)/sizeof(MINUTE_PLUS4[0]), r, g, b);
      break;

    default:
      break;
  }
}

// --------------------------------------------------
// Zeigt die passenden Minutenwoerter an.
// --------------------------------------------------
void showMinuteWords(int minute) {
  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  switch (minute) {
    case 0:
      setLeds(UHR, sizeof(UHR)/sizeof(UHR[0]), r, g, b);
      break;

    case 1:
    case 2:
    case 3:
    case 4:
      showMinutePlus(minute);
      setLeds(MINUTE_NACH, sizeof(MINUTE_NACH)/sizeof(MINUTE_NACH[0]), r, g, b);
      break;

    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      showMinutePlus(minute - 5);
      setLeds(MINUTE_NACH, sizeof(MINUTE_NACH)/sizeof(MINUTE_NACH[0]), r, g, b);
      break;

    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      setLeds(MINUTE_ZEHN, sizeof(MINUTE_ZEHN)/sizeof(MINUTE_ZEHN[0]), r, g, b);
      showMinutePlus(minute - 10);
      setLeds(MINUTE_NACH, sizeof(MINUTE_NACH)/sizeof(MINUTE_NACH[0]), r, g, b);
      break;

    case 15:
      setLeds(MINUTE_VIERTEL, sizeof(MINUTE_VIERTEL)/sizeof(MINUTE_VIERTEL[0]), r, g, b);
      setLeds(MINUTE_NACH, sizeof(MINUTE_NACH)/sizeof(MINUTE_NACH[0]), r, g, b);
      break;

    case 16:
    case 17:
    case 18:
    case 19:
      setLeds(MINUTE_FUENFZEHN, sizeof(MINUTE_FUENFZEHN)/sizeof(MINUTE_FUENFZEHN[0]), r, g, b);
      showMinutePlus(minute - 15);
      setLeds(MINUTE_NACH, sizeof(MINUTE_NACH)/sizeof(MINUTE_NACH[0]), r, g, b);
      break;

    case 20:
      setLeds(MINUTE_ZEHN, sizeof(MINUTE_ZEHN)/sizeof(MINUTE_ZEHN[0]), r, g, b);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 21:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      showMinutePlus(4);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 22:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      showMinutePlus(3);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 23:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      showMinutePlus(2);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 24:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      showMinutePlus(1);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 25:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 26:
      showMinutePlus(4);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 27:
      showMinutePlus(3);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    //case 28:
    //  showMinutePlus(2);
    //  setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
    //  setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
    //  break;

    case 29:
      showMinutePlus(1);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 30:
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 31:
    case 32:
    case 33:
    case 34:
      showMinutePlus(minute - 30);
      setLeds(MINUTE_NACH, sizeof(MINUTE_NACH)/sizeof(MINUTE_NACH[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      showMinutePlus(minute - 35);
      setLeds(MINUTE_NACH, sizeof(MINUTE_NACH)/sizeof(MINUTE_NACH[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 40:
      setLeds(MINUTE_ZEHN, sizeof(MINUTE_ZEHN)/sizeof(MINUTE_ZEHN[0]), r, g, b);
      setLeds(MINUTE_NACH, sizeof(MINUTE_NACH)/sizeof(MINUTE_NACH[0]), r, g, b);
      setLeds(MINUTE_HALB, sizeof(MINUTE_HALB)/sizeof(MINUTE_HALB[0]), r, g, b);
      break;

    case 41:
      setLeds(MINUTE_FUENFZEHN, sizeof(MINUTE_FUENFZEHN)/sizeof(MINUTE_FUENFZEHN[0]), r, g, b);
      showMinutePlus(4);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 42:
      setLeds(MINUTE_FUENFZEHN, sizeof(MINUTE_FUENFZEHN)/sizeof(MINUTE_FUENFZEHN[0]), r, g, b);
      showMinutePlus(3);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 43:
      setLeds(MINUTE_FUENFZEHN, sizeof(MINUTE_FUENFZEHN)/sizeof(MINUTE_FUENFZEHN[0]), r, g, b);
      showMinutePlus(2);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 44:
      setLeds(MINUTE_FUENFZEHN, sizeof(MINUTE_FUENFZEHN)/sizeof(MINUTE_FUENFZEHN[0]), r, g, b);
      showMinutePlus(1);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 45:
      setLeds(MINUTE_DREIVIERTEL, sizeof(MINUTE_DREIVIERTEL)/sizeof(MINUTE_DREIVIERTEL[0]), r, g, b);
      break;

    case 46:
      setLeds(MINUTE_ZEHN, sizeof(MINUTE_ZEHN)/sizeof(MINUTE_ZEHN[0]), r, g, b);
      showMinutePlus(4);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 47:
      setLeds(MINUTE_ZEHN, sizeof(MINUTE_ZEHN)/sizeof(MINUTE_ZEHN[0]), r, g, b);
      showMinutePlus(3);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 48:
      setLeds(MINUTE_ZEHN, sizeof(MINUTE_ZEHN)/sizeof(MINUTE_ZEHN[0]), r, g, b);
      showMinutePlus(2);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 49:
      setLeds(MINUTE_ZEHN, sizeof(MINUTE_ZEHN)/sizeof(MINUTE_ZEHN[0]), r, g, b);
      showMinutePlus(1);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 50:
      setLeds(MINUTE_ZEHN, sizeof(MINUTE_ZEHN)/sizeof(MINUTE_ZEHN[0]), r, g, b);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 51:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      showMinutePlus(4);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 52:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      showMinutePlus(3);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 53:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      showMinutePlus(2);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 54:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      showMinutePlus(1);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 55:
      setLeds(MINUTE_FUENF, sizeof(MINUTE_FUENF)/sizeof(MINUTE_FUENF[0]), r, g, b);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 56:
      showMinutePlus(4);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 57:
      showMinutePlus(3);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 58:
      showMinutePlus(2);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;

    case 59:
      showMinutePlus(1);
      setLeds(MINUTE_VOR, sizeof(MINUTE_VOR)/sizeof(MINUTE_VOR[0]), r, g, b);
      break;
  }
}

// --------------------------------------------------
// Bestimmt, ob die aktuelle oder folgende Stunde
// angezeigt wird.
// --------------------------------------------------
int getDisplayHour(int hour24, int minute) {

  // Mitternacht komplett als "0 Uhr"
  if (hour24 == 0) {
    if (minute < 20) {
      return 0;   // 00:00 bis 00:19 -> NULL
    } else {
      return 1;   // ab 00:20 -> Richtung 1 Uhr
    }
  }

  int displayHour = hour24;

  if (minute >= 20) {
    displayHour++;
  }

  int hour12 = displayHour % 12;

  if (hour12 == 0) {
    hour12 = 12;
  }

  return hour12;
}

// --------------------------------------------------
// Zentrale Anzeigefunktion der Uhrzeit.
// --------------------------------------------------
void showHourWord(int hour24) {
  strip.clear();

  if (!lampPower) {
    strip.show();
    return;
  }

  showAlwaysOnLeds();

  int hourToShow = getDisplayHour(hour24, currentMinute);

  showMinuteWords(currentMinute);
  showHourNumber(hourToShow, currentMinute);

  showWeatherStatusLeds();

  strip.show();
}

// --------------------------------------------------
// Zeigt das passende Aussentemperaturwort an.
// --------------------------------------------------
void showOutsideTemperatureWords() {
  if (!lampPower || !weatherValid) return;

  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  if (outsideTemperature <= 0.0) {
    setLeds(EISIGEN, sizeof(EISIGEN)/sizeof(EISIGEN[0]), r, g, b);
  } else if (outsideTemperature <= 10.0) {
    setLeds(KUEHLEN, sizeof(KUEHLEN)/sizeof(KUEHLEN[0]), r, g, b);
  } else if (outsideTemperature <= 16.0) {
    setLeds(FRISCHEN, sizeof(FRISCHEN)/sizeof(FRISCHEN[0]), r, g, b);
  } else if (outsideTemperature <= 24.0) {
    setLeds(WARMEN, sizeof(WARMEN)/sizeof(WARMEN[0]), r, g, b);
  } else if (outsideTemperature <= 30.0) {
    setLeds(SOMMERLICHEN, sizeof(SOMMERLICHEN)/sizeof(SOMMERLICHEN[0]), r, g, b);
  } else {
    setLeds(HEISSEN, sizeof(HEISSEN)/sizeof(HEISSEN[0]), r, g, b);
  }
}

// --------------------------------------------------
// Zeigt das passende Wetterwort an.
// --------------------------------------------------
void showWeatherWords() {
  if (!lampPower || !weatherValid) return;

  uint8_t r = applyBrightness(ledR);
  uint8_t g = applyBrightness(ledG);
  uint8_t b = applyBrightness(ledB);

  WeatherGroup group = classifyWeatherCode(weatherCode);

  switch (group) {
    case WEATHER_CLEAR:
      setLeds(KLAREM_HIMMEL, sizeof(KLAREM_HIMMEL)/sizeof(KLAREM_HIMMEL[0]), r, g, b);
      break;

    case WEATHER_CLOUDY:
    case WEATHER_FOG:
      setLeds(BEWOELKTEM_HIMMEL, sizeof(BEWOELKTEM_HIMMEL)/sizeof(BEWOELKTEM_HIMMEL[0]), r, g, b);
      break;

    case WEATHER_RAIN:
      setLeds(REGEN, sizeof(REGEN)/sizeof(REGEN[0]), r, g, b);
      break;

    case WEATHER_SNOW:
      setLeds(SCHNEE, sizeof(SCHNEE)/sizeof(SCHNEE[0]), r, g, b);
      break;

    case WEATHER_HAIL:
      setLeds(HAGEL, sizeof(HAGEL)/sizeof(HAGEL[0]), r, g, b);
      break;

    case WEATHER_STORM:
      setLeds(GEWITTER, sizeof(GEWITTER)/sizeof(GEWITTER[0]), r, g, b);
      break;

    default:
      break;
  }
}

// ==================================================
// Hauptfunktion: setup
// Initialisierung von Hardware, Netzwerk, Zeit,
// Wetterdaten, erster Anzeige und HTTP-Server.
// ==================================================
void setup() {
  Serial.begin(115200);

  onboardLed.begin();
  onboardLed.setPixelColor(0, 0, 0, 0);
  onboardLed.show();

  strip.begin();
  strip.setBrightness(STRIP_BRIGHTNESS);
  strip.clear();
  strip.show();

  secondStrip.begin();
  secondStrip.setBrightness(STRIP_BRIGHTNESS);
  secondStrip.clear();
  secondStrip.show();

  thirdStrip.begin();
  thirdStrip.setBrightness(STRIP_BRIGHTNESS);
  thirdStrip.clear();
  thirdStrip.show();
  dht.begin();

  connectToWiFi();

  configTzTime(TZ_INFO, NTP_SERVER1, NTP_SERVER2);

  syncTimeUntilSuccessAtStartup();
  updateWeatherFromServer();
  updateIndoorSensor();

  bootModeOff();

  if (updateTimeVariables()) {
    showHourWord(currentHour);
  }

  setupHttpServer();

  Serial.println("System gestartet.");
}

// ==================================================
// Hauptfunktion: loop
// Webserver-Anfragen, Zeit, Wetter, Sensorik und
// Anzeige werden zyklisch aktualisiert.
// ==================================================
void loop() {
  server.handleClient();

  static unsigned long lastClockUpdate = 0;
  static unsigned long lastTimePrint = 0;

  unsigned long nowMs = millis();

  if (nowMs - lastClockUpdate >= 500) {
    lastClockUpdate = nowMs;

    if (updateTimeVariables()) {
      checkDailyResync();
      checkPeriodicWeatherUpdate();
      checkPeriodicIndoorSensorUpdate();

      showHourWord(currentHour);
      showSecondLed(currentSecond);
      showThirdStripStatus();
    }
  }

  if (nowMs - lastTimePrint >= 2000) {
    lastTimePrint = nowMs;

    Serial.printf("Aktuelle Zeit: %02d:%02d:%02d | RGB=(%d,%d,%d) | On=%d | Brightness=%d | Temp=%.1f | Code=%d | WetterValid=%d\n",
                  currentHour, currentMinute, currentSecond,
                  ledR, ledG, ledB, lampPower, webBrightness,
                  outsideTemperature, weatherCode, weatherValid);
  }
}