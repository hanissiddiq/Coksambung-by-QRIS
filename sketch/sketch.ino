#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// // Konfigurasi WiFi
// const char* ssid = "NAMA_WIFI_ANDA";
// const char* password = "PASSWORD_WIFI_ANDA";

// Konfigurasi WiFi Simulator Wokwi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// URL Endpoint Server Anda (Tempat ESP32 mengecek status pembayaran)
// const char* serverUrl = "https://domain-anda.com";
const char* serverUrl = "https://b49b-110-136-83-241.ngrok-free.app";

// Definisi Pin ESP32
const int RELAY_SLOT1 = 12;
const int LED_SLOT1   = 14;
const int RELAY_SLOT2 = 27;
const int LED_SLOT2   = 26;

// Durasi aktif stopkontak setelah bayar (Contoh: 1 jam = 3600000 ms)
const unsigned long DURASI_AKTIF = 3600000; 

unsigned long waktuSelesaiSlot1 = 0;
unsigned long waktuSelesaiSlot2 = 0;

void setup() {
  Serial.begin(115200);
  
  // Atur Pin sebagai OUTPUT
  pinMode(RELAY_SLOT1, OUTPUT);
  pinMode(LED_SLOT1, OUTPUT);
  pinMode(RELAY_SLOT2, OUTPUT);
  pinMode(LED_SLOT2, OUTPUT);

  // Kondisi awal: Matikan semua (Relay LOW/HIGH tergantung jenis Active Low/High)
  digitalWrite(RELAY_SLOT1, LOW); 
  digitalWrite(LED_SLOT1, LOW);
  digitalWrite(RELAY_SLOT2, LOW);
  digitalWrite(LED_SLOT2, LOW);

  // Koneksi ke WiFi
  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("\nWiFi Terkoneksi!");
  // Serial.println();
   Serial.println();
   // ----------------------------------------
  // WIFI
  // ----------------------------------------

  Serial.println();
  Serial.println("Menghubungkan ke Wokwi-GUEST...");

  WiFi.mode(WIFI_STA);

  // Channel 6 sesuai jaringan virtual Wokwi
  WiFi.begin(ssid, password, 6);

  int retry = 0;

  while (WiFi.status() != WL_CONNECTED && retry < 30) {

    delay(500);

    Serial.print(".");

    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("==============================");
    Serial.println("WIFI TERHUBUNG!");
    Serial.println("==============================");

    Serial.print("SSID       : ");
    Serial.println(WiFi.SSID());

    Serial.print("IP ESP32   : ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI       : ");
    Serial.println(WiFi.RSSI());
    
}
}

void loop() {
  // 1. Cek Durasi Aktif Slot 1
  if (waktuSelesaiSlot1 > 0 && millis() >= waktuSelesaiSlot1) {
    digitalWrite(RELAY_SLOT1, LOW);  // Matikan stopkontak
    digitalWrite(LED_SLOT1, LOW);   // Matikan LED Hijau
    waktuSelesaiSlot1 = 0;
    Serial.println("Slot 1 Waktu Habis. Dimatikan.");
  }

  // 2. Cek Durasi Aktif Slot 2
  if (waktuSelesaiSlot2 > 0 && millis() >= waktuSelesaiSlot2) {
    digitalWrite(RELAY_SLOT2, LOW);  // Matikan stopkontak
    digitalWrite(LED_SLOT2, LOW);   // Matikan LED Hijau
    waktuSelesaiSlot2 = 0;
    Serial.println("Slot 2 Waktu Habis. Dimatikan.");
  }

  // 3. Cek Status Pembayaran ke Server setiap 5 detik
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    if (WiFi.status() == WL_CONNECTED) {
      checkPaymentStatus();
    }
  }
}

void checkPaymentStatus() {
 HTTPClient http;
  http.begin(serverUrl);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<200> doc;
    deserializeJson(doc, payload);

    // Ambil nilai durasi dari server (jika 0 artinya tidak ada pembayaran baru)
    unsigned long durasiSlot1 = doc["slot1"];
    unsigned long durasiSlot2 = doc["slot2"];

    if (durasiSlot1 > 0 && waktuSelesaiSlot1 == 0) {
      digitalWrite(RELAY_SLOT1, HIGH);
      digitalWrite(LED_SLOT1, HIGH);
      // Atur timer mundur berdasarkan nilai kiriman server
      waktuSelesaiSlot1 = millis() + durasiSlot1; 
      Serial.print("Slot 1 Aktif. Durasi (ms): ");
      Serial.println(durasiSlot1);
      resetStatusServer(1);
    }

    if (durasiSlot2 > 0 && waktuSelesaiSlot2 == 0) {
      digitalWrite(RELAY_SLOT2, HIGH);
      digitalWrite(LED_SLOT2, HIGH);
      waktuSelesaiSlot2 = millis() + durasiSlot2;
      Serial.print("Slot 2 Aktif. Durasi (ms): ");
      Serial.println(durasiSlot2);
      resetStatusServer(2);
    }
  }
  http.end();
}

void resetStatusServer(int slot) {
  HTTPClient http;
  // Endpoint untuk mereset status di database agar tidak dibaca berulang kali
  // String resetUrl = "https://domain-anda.com" + String(slot);
  // String resetUrl = "https://b49b-110-136-83-241.ngrok-free.app" + String(slot);
  String resetUrl = "https://b49b-110-136-83-241.ngrok-free.app/api/reset_status?slot=" + String(slot);
  http.begin(resetUrl);
  int httpResponseCode = http.GET();
  http.end();
}
