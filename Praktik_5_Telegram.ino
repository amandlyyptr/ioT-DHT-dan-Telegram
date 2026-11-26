#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

#define DHTPIN D4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// ================= WIFI =================
const char* ssid = "REALMEC55";
const char* password = "00000000";

// ================= GOOGLE SHEETS =================
String serverName = "https://script.google.com/macros/s/AKfycbz28t8Ivu1dQB4rEzH3or0BR6qoNX7vxa6OofweXtkAImcg_OATGC3aLRTY9voffouDCg/exec";

// ================= TELEGRAM =================
String botToken = "8710057191:AAHCzLLr53WCpv4reOja63sumKCs5fEA684";
String chatID   = "1575306766";

// ================= PIN LED =================
int merah = D5;
int hijau = D2;
int biru  = D7;

// smoothing
float lastTemp = 0;

// timer
unsigned long lastSend = 0;

// ================= FUNCTION TELEGRAM =================
void kirimTelegram(String pesan) {

  WiFiClientSecure client;
  client.setInsecure();

  Serial.println("Connecting to Telegram...");

  delay(1000);

  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("Gagal connect Telegram");
    return;
  }

  // encoding
  pesan.replace(" ", "%20");
  pesan.replace("\n", "%0A");

  String url = "/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + pesan;

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.telegram.org\r\n" +
               "Connection: close\r\n\r\n");

  // tunggu header selesai
  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 5000) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  // ambil body
  String body = client.readString();
  Serial.println("Response:");
  Serial.println(body);

  client.stop();

  delay(500);
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);

  pinMode(merah, OUTPUT);
  pinMode(hijau, OUTPUT);
  pinMode(biru, OUTPUT);

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("\nConnecting WiFi...");

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 40) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi CONNECTED!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi FAILED");
  }
}

// ================= LOOP =================
void loop() {

  float suhu = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (isnan(suhu) || isnan(hum)) {
    Serial.println("DHT ERROR");
    delay(2000);
    return;
  }

  // smoothing
  suhu = (suhu + lastTemp) / 2;
  lastTemp = suhu;

  // ================= STATUS =================
  String status;

  if (suhu < 30) {
    status = "COLD";
    digitalWrite(merah, LOW);
    digitalWrite(hijau, LOW);
    digitalWrite(biru, HIGH);
  }
  else if (suhu <= 34) {
    status = "NORMAL";
    digitalWrite(merah, LOW);
    digitalWrite(hijau, HIGH);
    digitalWrite(biru, LOW);
  }
  else {
    status = "HOT";
    digitalWrite(merah, HIGH);
    digitalWrite(hijau, LOW);
    digitalWrite(biru, LOW);
  }

  // ================= SERIAL =================
  Serial.print("SUHU: ");
  Serial.print(suhu);
  Serial.print(" | HUM: ");
  Serial.print(hum);
  Serial.print(" | STATUS: ");
  Serial.println(status);

  // ================= KIRIM DATA =================
  if (millis() - lastSend > 10000) {
    lastSend = millis();

    if (WiFi.status() == WL_CONNECTED) {

      WiFiClientSecure client;
      client.setInsecure();

      HTTPClient http;

      String url = serverName +
                   "?suhu=" + String(suhu) +
                   "&hum=" + String(hum) +
                   "&status=" + status;

      Serial.println("Kirim ke: " + url);

      http.begin(client, url);
      http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

      int httpCode = http.GET();

      Serial.print("HTTP CODE: ");
      Serial.println(httpCode);

      http.end();

      // 🔥 RELEASE MEMORY (PENTING)
      client.stop();
      delay(2000);
      yield();

      // ================= TELEGRAM =================
      String pesan = "SUHU: " + String(suhu) +
                     "\nHUM: " + String(hum) +
                     "\nSTATUS: " + status;

      kirimTelegram(pesan);

    } else {
      Serial.println("WiFi OFFLINE");
    }
  }

  delay(2000);
}