#include <Wire.h>
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
#include <HTTPClient.h>
#include <time.h>
#include <MPU6050_tockn.h>      // Library sensor MPU6050
#include <PubSubClient.h>       // Library MQTT
#include <WiFiClientSecure.h>

// ========= KONFIGURASI WIFI =========
const char* ssid     = "Sporthall";
const char* password = "";

// ========= KONFIGURASI HIVEMQ CLOUD =========
const char* mqtt_server = "022f2c5e9cbd472497f3ba11a75c43fa.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "hivemq.webclient.1779087469304";
const char* mqtt_password = "9pVH<0T12uyzwK,Ra#*B";
const char* mqtt_client_id = "esp32-tilt-sensor-01";

const char* topic_sensor = "building-tilt/sensor-data";
const char* topic_commands = "building-tilt/commands";

// ========= KONFIGURASI SERVER =========
WiFiServer server(80);
WebSocketsServer webSocket(81);

// ========= MPU6050 =========
MPU6050 mpu6050(Wire);

// ========= PARAMETER KALIBRASI (dari Kode 2) =========
const float offsetX =  0.0154;
const float offsetY = -0.0107;
const float offsetZ = -0.0141;

const float scaleX  = 0.9955;
const float scaleY  = 0.9989;
const float scaleZ  = 1.0094;

// ========= LOW PASS FILTER =========
const float alpha = 0.05;
float ax_f = 0.0;
float ay_f = 0.0;
float az_f = 0.0;

// ========= ANGLE OFFSET CORRECTION =========
const float rollOffset  = 0.0;
const float pitchOffset = 0.0;

// ========= VARIABEL SENSOR =========
float roll = 0.0;
float pitch = 0.0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 200;
unsigned long lastMqttPublish = 0;
const unsigned long mqttPublishInterval = 500;

// ========= PENGATURAN WAKTU =========
bool hasSentThisHour = false;

// ========= GOOGLE SHEETS URL =========
const char* GAS_URL = "https://script.google.com/macros/s/AKfycbxnoI70zKZ15bpLvfuJe_K7zjFZhY9jvTzt5NM-FhIfVh9xjrNSd360KZsI-BIu_b6X9g/exec";

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
    if (mqttClient.publish(topic_sensor, json.c_str())) {
      Serial.println("Published to HiveMQ: " + json);
    } else {
      Serial.println("Failed to publish to HiveMQ");
    }
  } else {
    Serial.println("MQTT not connected, attempting reconnect...");
    reconnectMQTT();
  }
}

// ========= MQTT CALLBACK =========
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  // Tidak ada aksi karena fitur SD telah dihapus
}

// ========= KONEKSI MQTT =========
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to HiveMQ...");
    if (mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println(" connected");
      mqttClient.subscribe(topic_commands);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}

// ========= KIRIM DATA VIA WEBSOCKET =========
void broadcastSensorData() {
  String json = makeSensorJson();
  webSocket.broadcastTXT(json.c_str(), json.length());
}

// ========= KIRIM DATA KE GOOGLE SHEETS (SETIAP JAM) =========
void sendToGoogleSheets() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    float average = (abs(roll) + abs(pitch)) / 2.0;
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }
    
    char dateStr[20], timeStr[20];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    
    String url = String(GAS_URL);
    url += "?date=" + String(dateStr);
    url += "&time=" + String(timeStr);
    url += "&pitch=" + String(pitch, 2);
    url += "&roll=" + String(roll, 2);
    url += "&average=" + String(average, 2);
    
    Serial.println("Sending to Google Sheets...");
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("HTTP " + String(httpCode) + ": " + http.getString());
    } else {
      Serial.println("HTTP Error " + String(httpCode));
    }
    http.end();
    hasSentThisHour = true;
  }
}

// ========= CEK JADWAL PENGIRIMAN =========
void checkSchedule() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  
  int currentMinute = timeinfo.tm_min;
  int currentSecond = timeinfo.tm_sec;
  
  // Reset flag setiap jam baru
  if (currentMinute == 0 && currentSecond == 0) {
    hasSentThisHour = false;
  }
  
  // Kirim ke Google Sheets setiap jam tepat (menit 0, detik 0-5)
  if (currentMinute == 0 && currentSecond >= 0 && currentSecond <= 5 && !hasSentThisHour) {
    sendToGoogleSheets();
  }
}

// ========= WEBSOCKET EVENT HANDLER =========
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.printf("WebSocket client %u connected\n", num);
      // Kirim data terbaru ke klien yang baru terhubung
      {
        String json = makeSensorJson();
        webSocket.sendTXT(num, json.c_str(), json.length());
      }
      break;
    case WStype_DISCONNECTED:
      Serial.printf("WebSocket client %u disconnected\n", num);
      break;
    case WStype_TEXT:
      // Tidak ada perintah yang perlu ditangani karena SD telah dihapus
      break;
  }
}

// ========= SETUP WAKTU NTP =========
void setupTime() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP sync");
  struct tm timeinfo;
  for (int i = 0; i < 10; i++) {
    if (getLocalTime(&timeinfo)) {
      Serial.println("\nTime synced: ");
      Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
      return;
    }
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nNTP sync failed!");
}

// ========= SETUP =========
void setup() {
  Serial.begin(115200);
  delay(200);

  // ----- SPIFFS (untuk halaman web) -----
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS mount failed!");
    while (1);
  }

  // ----- MPU6050 -----
  Wire.begin(21, 22);
  Wire.setClock(400000);
  mpu6050.begin();
  
  Serial.println("Calibrating gyro offsets, DO NOT MOVE sensor...");
  mpu6050.calcGyroOffsets(true);
  Serial.println("MPU6050 Ready!");

  // ----- WiFi -----
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 30) {
    delay(400);
    Serial.print(".");
    timeout++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected, IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed!");
  }

  // ----- NTP -----
  setupTime();

  // ----- MQTT -----
  wifiClient.setInsecure();
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(30);

  // ----- Server HTTP & WebSocket -----
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("HTTP & WebSocket server started");
  Serial.println("Data will be sent to Google Sheets every hour");
  Serial.println("Data will be published to HiveMQ every 500ms");
}

// ========= BACA DATA MPU6050 =========
void readMPUData() {
  mpu6050.update();
  
  float ax_raw = mpu6050.getAccX();
  float ay_raw = mpu6050.getAccY();
  float az_raw = mpu6050.getAccZ();

  float ax = (ax_raw - offsetX) / scaleX;
  float ay = (ay_raw - offsetY) / scaleY;
  float az = (az_raw - offsetZ) / scaleZ;

  ax_f = alpha * ax + (1.0 - alpha) * ax_f;
  ay_f = alpha * ay + (1.0 - alpha) * ay_f;
  az_f = alpha * az + (1.0 - alpha) * az_f;

  roll  = atan2(ay_f, sqrt(ax_f * ax_f + az_f * az_f)) * 180.0 / PI;
  pitch = atan2(-ax_f, sqrt(ay_f * ay_f + az_f * az_f)) * 180.0 / PI;

  roll  -= rollOffset;
  pitch -= pitchOffset;

  Serial.print("Roll: "); Serial.print(roll,2);
  Serial.print(" | Pitch: "); Serial.println(pitch,2);
}

// ========= LOOP =========
void loop() {
  webSocket.loop();
  
  // MQTT loop
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();
  }
  
  // Baca sensor & broadcast via WebSocket
  if (millis() - lastUpdate >= updateInterval) {
    readMPUData();
    broadcastSensorData();
    lastUpdate = millis();
  }
  
  // Publish ke HiveMQ
  if (millis() - lastMqttPublish >= mqttPublishInterval && mqttClient.connected()) {
    publishToHiveMQ();
    lastMqttPublish = millis();
  }
  
  // Cek jadwal kirim ke Google Sheets
  checkSchedule();
  
  // HTTP client
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

  // Routing
  if (req.indexOf("GET /style.css") >= 0) {
    serveFile(client, "/style.css", "text/css");
  } else if (req.indexOf("GET /script.js") >= 0) {
    serveFile(client, "/script.js", "application/javascript");
  } else if (req.indexOf("GET /unika.png") >= 0) {
    serveImage(client, "/unika.png");
  } else if (req.indexOf("GET /mqtt-status") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.print("{\"mqtt_connected\":");
    client.print(mqttClient.connected() ? "true" : "false");
    client.print(",\"hivemq_server\":\"");
    client.print(mqtt_server);
    client.println("\"}");
  } else if (req.indexOf("GET /") >= 0) {
    File f = SPIFFS.open("/index.html", "r");
    if (f) {
      String html = f.readString();
      f.close();
      html.replace("%IP%", WiFi.localIP().toString());
      html.replace("%MQTT_STATUS%", mqttClient.connected() ? "Terhubung ke HiveMQ" : "Tidak terhubung");
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
    while (f.available()) {
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