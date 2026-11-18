#include <Arduino.h>
#include <ArduinoJson.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <GxEPD2_BW.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
// A remplir avec tes infos
#define WIFI_SSID "Le nom de ton WIFI"
#define WIFI_PASSWORD "Mot de passe des ton WIFI"
#define SCRIPT_URL                                                             \
  "url de ton google app script"
//Laisse les guillemets lorsque tu rentres tes information, sinon le programme ne pourra pas les lires
// Ecran 4.2" (400x300), broches : CS=5, DC=17, RST=16, BUSY=4
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>
    display(GxEPD2_420_GDEY042T81(5, 17, 16, 4));
const unsigned long FETCH_INTERVAL = 600000; // donnees : toutes les 10 min
unsigned long lastFetch = 0;
int lastMinute = -1;
String lastSync = "--:--";
JsonDocument gDoc;
bool hasData = false;
const char *JOURS[] = {"dim.", "lun.", "mar.", "mer.", "jeu.", "ven.", "sam."};
const char *MOIS[] = {"janv.", "fevr.", "mars",  "avr.", "mai",  "juin",
                      "juil.", "aout",  "sept.", "oct.", "nov.", "dec."};
// Les polices GFX ne gerent pas les accents : on les retire
String toAscii(const char *s) {
  String out;
  for (const uint8_t *p = (const uint8_t *)s; *p; p++) {
    if (*p < 0x80) {
      out += (char)*p;
    } else if (*p == 0xC3 && p[1]) {
      p++;
      uint8_t c = *p + 0x40; // code Latin-1
      char r = '?';
      if (c >= 0xE0 && c <= 0xE5)
        r = 'a';
      else if (c == 0xE7)
        r = 'c';
      else if (c >= 0xE8 && c <= 0xEB)
        r = 'e';
      else if (c >= 0xEC && c <= 0xEF)
        r = 'i';
      else if (c >= 0xF2 && c <= 0xF6)
        r = 'o';
      else if (c >= 0xF9 && c <= 0xFC)
        r = 'u';
      else if (c >= 0xC0 && c <= 0xC5)
        r = 'A';
      else if (c == 0xC7)
        r = 'C';
      else if (c >= 0xC8 && c <= 0xCB)
        r = 'E';
      else if (c >= 0xCC && c <= 0xCF)
        r = 'I';
      else if (c >= 0xD2 && c <= 0xD6)
        r = 'O';
      else if (c >= 0xD9 && c <= 0xDC)
        r = 'U';
      out += r;
    } else if (p[0] == 0xE2 && p[1] == 0x80 && (p[2] == 0x98 || p[2] == 0x99)) {
      out += '\'';
      p += 2;
    } else {
      while ((p[1] & 0xC0) == 0x80)
        p++;
      out += '?';
    }
  }
  return out;
}
String fit(String s, unsigned int n) {
  if (s.length() > n)
    return s.substring(0, n - 1) + ".";
  return s;
}
void fetchData() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(5000);
    if (WiFi.status() != WL_CONNECTED)
      return;
  }
  WiFiClientSecure client;
  client.setInsecure(); // projet perso : pas de verification du certificat
  HTTPClient http;
  http.begin(client, SCRIPT_URL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    DeserializationError err = deserializeJson(gDoc, payload);
    if (!err) {
      hasData = true;
      struct tm t;
      if (getLocalTime(&t, 0)) {
        char buf[6];
        strftime(buf, sizeof(buf), "%H:%M", &t);
        lastSync = buf;
      }
    } else {
      Serial.print("Erreur JSON : ");
      Serial.println(err.c_str());
    }
  } else {
    Serial.printf("Erreur HTTP : %d\n", code);
  }
  http.end();
}
void draw(bool full) {
  struct tm t;
  bool okTime = getLocalTime(&t, 0);
  char hhmm[6] = "--:--";
  String date = "";
  if (okTime) {
    strftime(hhmm, sizeof(hhmm), "%H:%M", &t);
    date = String(JOURS[t.tm_wday]) + " " + t.tm_mday + " " + MOIS[t.tm_mon];
  }
  if (full)
    display.setFullWindow();
  else
    display.setPartialWindow(0, 0, display.width(), display.height());
  display.firstPage();
  do {
    display.fillScreen(GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);
    // Heure, date, derniere synchro
    display.setFont(&FreeMonoBold24pt7b);
    display.setCursor(12, 56);
    display.print(hhmm);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(12, 82);
    display.print(date);
    display.setCursor(12, 104);
    display.print("maj " + lastSync);
    // Traits de separation
    display.drawFastVLine(170, 10, 160, GxEPD_WHITE);
    display.drawFastHLine(10, 180, 380, GxEPD_WHITE);
    // Agenda (colonne de droite)
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(182, 30);
    display.print("AGENDA");
    display.setFont(&FreeMonoBold9pt7b);
    int y = 58;
    if (!hasData) {
      display.setCursor(182, y);
      display.print("Pas de donnees");
    } else {
      JsonArray events = gDoc["events"].as<JsonArray>();
      unsigned int total = events.size();
      if (total == 0) {
        display.setCursor(182, y);
        display.print("Aucun rdv");
      } else {
        unsigned int show = total > 5 ? 4 : total;
        unsigned int i = 0;
        for (JsonObject ev : events) {
          if (i >= show)
            break;
          String title = toAscii(ev["title"] | "");
          String when = toAscii(ev["time"] | "");
          String line = when.length() ? when + " " + title : "- " + title;
          display.setCursor(182, y);
          display.print(fit(line, 19));
          y += 22;
          i++;
        }
        if (total > show) {
          display.setCursor(182, y);
          display.print("+" + String(total - show) + " autres");
        }
      }
    }
    // Rappels (bas de l'ecran)
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(12, 206);
    display.print("RAPPELS");
    display.setFont(&FreeMonoBold9pt7b);
    y = 232;
    if (!hasData) {
      display.setCursor(12, y);
      display.print("Pas de donnees");
    } else {
      JsonArray tasks = gDoc["tasks"].as<JsonArray>();
      unsigned int total = tasks.size();
      if (total == 0) {
        display.setCursor(12, y);
        display.print("Aucun rappel");
      } else {
        unsigned int show = total > 4 ? 3 : total;
        unsigned int i = 0;
        for (JsonVariant v : tasks) {
          if (i >= show)
            break;
          display.setCursor(12, y);
          display.print("[ ] " + fit(toAscii(v.as<const char *>()), 30));
          y += 20;
          i++;
        }
        if (total > show) {
          display.setCursor(12, y);
          display.print("+" + String(total - show) + " autres");
        }
      }
    }
  } while (display.nextPage());
}
void setup() {
  Serial.begin(115200);
  delay(1000);
  SPI.begin(12, -1, 11, 5);
  display.init(115200, true, 2, false);
  display.setRotation(0);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connexion Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  // Heure de Paris via NTP (gere l'heure d'ete)
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.google.com");
  struct tm t;
  getLocalTime(&t, 10000);
  fetchData();
  lastFetch = millis();
  draw(true);
  if (getLocalTime(&t, 0))
    lastMinute = t.tm_min;
}
void loop() {
  struct tm t;
  if (getLocalTime(&t, 0) && t.tm_min != lastMinute) {
    lastMinute = t.tm_min;
    if (millis() - lastFetch >= FETCH_INTERVAL) {
      fetchData();
      lastFetch = millis();
      draw(true); // rafraichissement complet toutes les 10 min
    } else {
      draw(false); // mise a jour rapide chaque minute
    }
  }
  delay(1000);
}//
// Created by super on 18/11/2026.
//
