#include <Wire.h>
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <SD.h>
#include <SPI.h>
#include <WebSocketsServer.h>
#include <HTTPClient.h>
#include <time.h>
#include "MPU9250.h"
#include <PubSubClient.h>  // Tambahkan library MQTT untuk HiveMQ
#include <WiFiClientSecure.h>

// ========= KONFIGURASI WIFI =========
const char* ssid     = "Sporthall";
const char* password = "";

// ========= KONFIGURASI HIVEMQ CLOUD =========
// GANTI DENGAN KONFIGURASI CLUSTER ANDA
const char* mqtt_server = "d97fa97042434738a712d06b663db901.s1.eu.hivemq.cloud";  // Hostname cluster Anda
const int mqtt_port = 8883;  // Port MQTT SSL
const char* mqtt_username = "hivemq.webclient.1769348223798";  // Username HiveMQ
const char* mqtt_password = "E0MR9o&!I7dP1zct#:bD";  // Password HiveMQ
const char* mqtt_client_id = "esp32-tilt-sensor-01";

// Topics MQTT
const char* topic_sensor = "building-tilt/sensor-data";
const char* topic_commands = "building-tilt/commands";

// ========= KONFIGURASI SERVER =========
WiFiServer server(80);
WebSocketsServer webSocket(81);

// ========= MPU9250 =========
MPU9250 mpu;

// ========= PARAMETER KALIBRASI =========
const float offsetX =  0.001;
const float offsetY = -0.025;
const float offsetZ = -0.108;
const float scaleX  = 0.997;
const float scaleY  = 0.999;
const float scaleZ  = 1.018;

// ========= LOW PASS FILTER =========
const float alpha = 0.05;
float ax_f = 0.0;
float ay_f = 0.0;
float az_f = 0.0;

// ========= VARIABEL SENSOR =========
float roll = 0.0;
float pitch = 0.0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 200;
unsigned long lastMqttPublish = 0;
const unsigned long mqttPublishInterval = 500; // Publish ke HiveMQ setiap 500ms
unsigned long lastSheetUpdate = 0;
unsigned long lastSDUpdate = 0;

// ========= PENGATURAN WAKTU =========
bool hasSentThisHour = false;
bool hasSavedThisHalfHour = false;

// ========= GOOGLE SHEETS URL =========
const char* GAS_URL = "https://script.google.com/macros/s/AKfycbzGM-UCADFQa6E_NyA4swfzhWb40wjyIwgmgH-3pLmLr6YauYU6Dm1Vrw8Z7NiCT2QILw/exec";

// ========= KONFIGURASI SD CARD =========
#define SD_SCK  14
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS   27

SPIClass sdSPI(VSPI);
File sdFile;

// ========= MQTT CLIENT =========
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// ========= FUNGSI UNTUK MEMBUAT JSON =========
String makeSensorJson() {
  String json = "{";
  json += "\"roll\":" + String(roll, 2) + ",";
  json += "\"pitch\":" + String(pitch, 2);
  json += "}";
  return json;
}

// ========= PUBLISH DATA KE HIVEMQ CLOUD =========
void publishToHiveMQ() {
  if (mqttClient.connected()) {
    String json = makeSensorJson();
    bool success = mqttClient.publish(topic_sensor, json.c_str());
    
    if (success) {
      Serial.println("Published to HiveMQ: " + json);
    } else {
      Serial.println("Failed to publish to HiveMQ");
    }
  } else {
    Serial.println("MQTT not connected, attempting reconnect...");
    reconnectMQTT();
  }
}

// ========= MQTT CALLBACK (untuk menerima perintah) =========
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(" - Message: ");
  Serial.println(message);
  
  // Handle commands dari web
  if (String(topic) == topic_commands) {
    // Parsing JSON sederhana
    if (message.indexOf("\"type\":\"GET_SD_DATA\"") >= 0) {
      Serial.println("Received SD card data request");
      sendSDCardFileListViaMQTT();
    }
    // Anda bisa menambahkan command lain di sini
  }
}

// ========= KIRIM DAFTAR FILE SD CARD VIA MQTT =========
void sendSDCardFileListViaMQTT() {
  if (!mqttClient.connected()) return;
  
  File root = SD.open("/");
  String fileList = "[";
  
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }
    
    if (!entry.isDirectory() && String(entry.name()).endsWith(".csv")) {
      if (fileList != "[") {
        fileList += ",";
      }
      String fileName = String(entry.name());
      uint64_t fileSize = entry.size();
      fileList += "{\"name\":\"" + fileName + "\",\"size\":" + String(fileSize) + "}";
    }
    entry.close();
  }
  root.close();
  
  fileList += "]";
  
  // Kirim daftar file via MQTT
  String response = "{\"type\":\"SD_FILE_LIST\",\"files\":" + fileList + "}";
  mqttClient.publish(topic_commands, response.c_str());
  Serial.println("Sent SD file list via MQTT");
}

// ========= KONEKSI DAN RECONNECT MQTT =========
void reconnectMQTT() {
  // Loop sampai terhubung
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection to HiveMQ...");
    
    // Coba koneksi
    if (mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected to HiveMQ Cloud!");
      
      // Subscribe ke topic commands
      if (mqttClient.subscribe(topic_commands)) {
        Serial.println("Subscribed to commands topic");
      } else {
        Serial.println("Failed to subscribe to commands topic");
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// ========= KIRIM DATA KE CLIENT VIA WEBSOCKET =========
void broadcastSensorData() {
  String json = makeSensorJson();
  webSocket.broadcastTXT(json.c_str(), json.length());
}

// ========= KIRIM DATA KE GOOGLE SHEETS SETIAP JAM =========
void sendToGoogleSheets() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Hitung rata-rata
    float average = (abs(roll) + abs(pitch)) / 2.0;
    
    // Dapatkan waktu saat ini
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }
    
    char dateStr[20];
    char timeStr[20];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    
    // Buat URL dengan parameter
    String url = String(GAS_URL);
    url += "?date=" + String(dateStr);
    url += "&time=" + String(timeStr);
    url += "&pitch=" + String(pitch, 2);
    url += "&roll=" + String(roll, 2);
    url += "&average=" + String(average, 2);
    
    Serial.println("Mengirim data ke Google Sheets...");
    Serial.println("URL: " + url);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response: " + String(httpCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("HTTP Error: " + String(httpCode));
    }
    
    http.end();
    hasSentThisHour = true;
  }
}

// ========= SIMPAN DATA KE SD CARD SETIAP 30 MENIT =========
void saveToSDCard() {
  // Dapatkan waktu saat ini
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time for SD card");
    return;
  }
  
  char dateStr[20];
  char timeStr[20];
  strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  
  // Buat nama file berdasarkan tanggal
  String fileName = "/data_" + String(dateStr) + ".csv";
  
  // Buka file untuk ditulis (append)
  sdFile = SD.open(fileName.c_str(), FILE_APPEND);
  
  if (sdFile) {
    // Tulis header jika file baru
    if (sdFile.size() == 0) {
      sdFile.println("Tanggal,Waktu,Roll (derajat),Pitch (derajat),Rata-rata (derajat)");
    }
    
    // Hitung rata-rata
    float average = (abs(roll) + abs(pitch)) / 2.0;
    
    // Tulis data
    String dataLine = String(dateStr) + "," + String(timeStr) + "," + 
                     String(roll, 2) + "," + String(pitch, 2) + "," + 
                     String(average, 2);
    
    sdFile.println(dataLine);
    sdFile.close();
    
    Serial.println("Data disimpan ke SD card: " + dataLine);
  } else {
    Serial.println("Error membuka file SD card");
  }
  
  hasSavedThisHalfHour = true;
}

// ========= CEK JAM UNTUK PENGIRIMAN DATA =========
void checkSchedule() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }
  
  int currentMinute = timeinfo.tm_min;
  int currentSecond = timeinfo.tm_sec;
  
  // Reset flag setiap jam baru
  if (currentMinute == 0 && currentSecond == 0) {
    hasSentThisHour = false;
    hasSavedThisHalfHour = false;
  }
  
  // Kirim ke Google Sheets setiap jam tepat (menit 0, detik 0-5)
  if (currentMinute == 0 && currentSecond >= 0 && currentSecond <= 5 && !hasSentThisHour) {
    sendToGoogleSheets();
  }
  
  // Simpan ke SD card setiap 30 menit (0 dan 30)
  if ((currentMinute == 0 || currentMinute == 30) && 
      currentSecond >= 0 && currentSecond <= 5 && 
      !hasSavedThisHalfHour) {
    saveToSDCard();
  }
}

// ========= WEBSOCKET EVENT HANDLER =========
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.printf("WebSocket Client %u connected\n", num);
      {
        String json = makeSensorJson();
        webSocket.sendTXT(num, json.c_str(), json.length());
      }
      break;
      
    case WStype_DISCONNECTED:
      Serial.printf("WebSocket Client %u disconnected\n", num);
      break;
      
    case WStype_TEXT:
      {
        String msg = String((char*)payload);
        Serial.printf("WebSocket Message: %s\n", msg.c_str());
        
        // Handle request untuk data SD card
        if (msg == "GET_SD_DATA") {
          sendSDCardFileList(num);
        }
        // Handle request untuk download file tertentu
        else if (msg.startsWith("GET_FILE:")) {
          String fileName = msg.substring(9);
          sendSDCardFile(num, fileName);
        }
      }
      break;
  }
}

// ========= KIRIM DAFTAR FILE SD CARD VIA WEBSOCKET =========
void sendSDCardFileList(uint8_t num) {
  File root = SD.open("/");
  String fileList = "FILE_LIST:";
  
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }
    
    if (!entry.isDirectory() && String(entry.name()).endsWith(".csv")) {
      fileList += String(entry.name()) + ";";
    }
    entry.close();
  }
  root.close();
  
  webSocket.sendTXT(num, fileList.c_str());
}

// ========= KIRIM FILE SD CARD VIA WEBSOCKET =========
void sendSDCardFile(uint8_t num, String fileName) {
  File file = SD.open("/" + fileName);
  if (!file) {
    webSocket.sendTXT(num, "ERROR:File tidak ditemukan");
    return;
  }
  
  // Kirim file dalam chunk
  while (file.available()) {
    String chunk = "FILE_CHUNK:";
    for (int i = 0; i < 512 && file.available(); i++) {
      chunk += (char)file.read();
    }
    webSocket.sendTXT(num, chunk.c_str());
    delay(10);
  }
  
  file.close();
  webSocket.sendTXT(num, "FILE_END:");
}

// ========= SETUP WAKTU NTP =========
void setupTime() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  
  Serial.print("Waiting for NTP time sync");
  struct tm timeinfo;
  for (int i = 0; i < 10; i++) {
    if (getLocalTime(&timeinfo)) {
      Serial.println("\nTime synchronized");
      Serial.print("Current time: ");
      Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
      return;
    }
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nFailed to get NTP time");
}

// ========= SETUP =========
void setup() {
  Serial.begin(115200);
  delay(200);

  // ========= INIT SPIFFS =========
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS mount failed!");
    while (1);
  }

  // ========= INIT SD CARD =========
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  if (!SD.begin(SD_CS, sdSPI)) {
    Serial.println("SD Card initialization failed!");
  } else {
    Serial.println("SD Card initialized successfully.");
    
    // Cek kapasitas SD card
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
  }

  // ========= INIT MPU9250 =========
  Wire.begin(21, 22);
  Wire.setClock(400000);

  if (!mpu.setup(0x68)) {
    Serial.println("MPU9250 tidak terdeteksi!");
    while (1);
  }
  
  Serial.println("MPU9250 initialized");

  // ========= CONNECT TO WIFI =========
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 30) {
    delay(400);
    Serial.print(".");
    wifiTimeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    // Tetap lanjut untuk testing dengan data dummy
  }

  // ========= SETUP TIME =========
  setupTime();

  // ========= SETUP MQTT CLIENT =========
  wifiClient.setInsecure(); // Nonaktifkan verifikasi sertifikat untuk testing
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(30);
  
  // ========= START SERVERS =========
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("HTTP & WebSocket server started");
  Serial.println("Data akan dikirim ke Google Sheets setiap jam tepat");
  Serial.println("Data akan disimpan ke SD Card setiap 30 menit");
  Serial.println("Data akan dipublish ke HiveMQ Cloud setiap 500ms");
}

// ========= BACA DATA SENSOR =========
void readMPUData() {
  if (mpu.update()) {
    float ax_raw = mpu.getAccX();
    float ay_raw = mpu.getAccY();
    float az_raw = mpu.getAccZ();

    float ax = (ax_raw - offsetX) / scaleX;
    float ay = (ay_raw - offsetY) / scaleY;
    float az = (az_raw - offsetZ) / scaleZ;

    ax_f = alpha * ax + (1.0 - alpha) * ax_f;
    ay_f = alpha * ay + (1.0 - alpha) * ay_f;
    az_f = alpha * az + (1.0 - alpha) * az_f;

    roll = atan2(ay_f, sqrt(ax_f * ax_f + az_f * az_f)) * 180.0 / PI;
    pitch = atan2(-ax_f, sqrt(ay_f * ay_f + az_f * az_f)) * 180.0 / PI;

    Serial.print("Roll: ");
    Serial.print(roll, 2);
    Serial.print(" deg | Pitch: ");
    Serial.print(pitch, 2);
    Serial.println(" deg");
  }
}

// ========= LOOP =========
void loop() {
  webSocket.loop();
  
  // Handle MQTT connection
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();
  }
  
  // Baca data sensor
  if (millis() - lastUpdate >= updateInterval) {
    readMPUData();
    broadcastSensorData(); // Kirim via WebSocket lokal
    lastUpdate = millis();
  }
  
  // Kirim data ke HiveMQ Cloud
  if (millis() - lastMqttPublish >= mqttPublishInterval && mqttClient.connected()) {
    publishToHiveMQ();
    lastMqttPublish = millis();
  }
  
  // Cek jadwal pengiriman data
  checkSchedule();
  
  // Handle HTTP client
  WiFiClient client = server.available();
  if (client) {
    handleHttpClient(client);
  }
}

// ========= HANDLE HTTP CLIENT =========
void handleHttpClient(WiFiClient client) {
  String req = "";
  unsigned long timeout = millis() + 250;
  
  while (client.connected() && millis() < timeout) {
    if (client.available()) {
      char c = client.read();
      req += c;
      if (req.endsWith("\r\n\r\n")) break;
    }
  }

  // Routing HTTP
  if (req.indexOf("GET /style.css") >= 0) {
    serveFile(client, "/style.css", "text/css");
  } else if (req.indexOf("GET /script.js") >= 0) {
    serveFile(client, "/script.js", "application/javascript");
  } else if (req.indexOf("GET /unika.png") >= 0) {
    serveImage(client, "/unika.png");
  } else if (req.indexOf("GET /download-sd") >= 0) {
    // Endpoint untuk download file dari SD card
    handleSDDownload(client, req);
  } else if (req.indexOf("GET /mqtt-status") >= 0) {
    // Endpoint untuk cek status MQTT
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println("{\"mqtt_connected\":" + String(mqttClient.connected() ? "true" : "false") + 
                   ",\"hivemq_server\":\"" + String(mqtt_server) + "\"}");
  } else if (req.indexOf("GET /") >= 0) {
    File f = SPIFFS.open("/index.html", "r");
    if (f) {
      String html = f.readString();
      f.close();
      
      html.replace("%IP%", WiFi.localIP().toString());
      html.replace("%MQTT_STATUS%", mqttClient.connected() ? "Terhubung ke HiveMQ" : "Tidak terhubung ke HiveMQ");
      html.replace("%MQTT_SERVER%", mqtt_server);
      
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.print(html);
    } else {
      client.println("HTTP/1.1 404 Not Found");
      client.println();
    }
  }
  
  client.stop();
}

// ========= HELPER FUNCTIONS =========
void serveFile(WiFiClient client, String filename, String contentType) {
  File f = SPIFFS.open(filename, "r");
  if (f) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: " + contentType);
    client.println("Connection: close");
    client.println();
    while(f.available()) {
      client.write(f.read());
    }
    f.close();
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println();
  }
}

void serveImage(WiFiClient client, String filename) {
  File f = SPIFFS.open(filename, "r");
  if (f) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/png");
    client.println("Content-Length: " + String(f.size()));
    client.println("Connection: close");
    client.println();
    
    uint8_t buffer[1024];
    while (f.available()) {
      size_t len = f.read(buffer, sizeof(buffer));
      client.write(buffer, len);
    }
    f.close();
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println();
  }
}

// Handle download dari SD card via HTTP
void handleSDDownload(WiFiClient client, String req) {
  // Ekstrak nama file dari request
  int startIdx = req.indexOf("GET /download-sd/");
  if (startIdx >= 0) {
    int endIdx = req.indexOf(" HTTP/", startIdx);
    String fileName = req.substring(startIdx + 17, endIdx);
    
    File file = SD.open("/" + fileName);
    if (file) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/csv");
      client.println("Content-Disposition: attachment; filename=\"" + fileName + "\"");
      client.println("Connection: close");
      client.println();
      
      uint8_t buffer[1024];
      while (file.available()) {
        size_t len = file.read(buffer, sizeof(buffer));
        client.write(buffer, len);
      }
      file.close();
    } else {
      client.println("HTTP/1.1 404 Not Found");
      client.println();
      client.println("File not found: " + fileName);
    }
  } else {
    // List semua file CSV di SD card
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<html><body>");
    client.println("<h1>File Data SD Card</h1>");
    client.println("<ul>");
    
    File root = SD.open("/");
    while (true) {
      File entry = root.openNextFile();
      if (!entry) {
        break;
      }
      
      if (!entry.isDirectory() && String(entry.name()).endsWith(".csv")) {
        client.println("<li><a href=\"/download-sd/" + String(entry.name()) + "\">" + 
                       String(entry.name()) + "</a> (" + 
                       String(entry.size() / 1024.0, 1) + " KB)</li>");
      }
      entry.close();
    }
    root.close();
    
    client.println("</ul>");
    client.println("<p><a href=\"/\">Kembali ke Dashboard</a></p>");
    client.println("</body></html>");
  }
}