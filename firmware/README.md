# 🔧 Firmware ESP32

Firmware untuk dua perangkat ESP32 pada sistem **Sport Hall Tilt Monitoring**. Kembali ke [README utama](../README.md).

## 📋 Daftar Isi

- [Hardware](#-hardware)
- [Instalasi Firmware](#-instalasi-firmware)
- [Cara Mendapatkan Credential HiveMQ Cloud](#-cara-mendapatkan-credential-hivemq-cloud)
- [Format Data](#-format-data)

---

## 🔧 Hardware

### ESP32 #1 — Titik Ukur (`FINAL_PROJECT_ESP1`)

Baca MPU9250, publish MQTT setiap 500ms, simpan ke Google Sheets tiap jam, simpan ke SD Card tiap 30 menit, sediakan server HTTP + WebSocket lokal.

| Komponen | Detail |
|---|---|
| **Mikrokontroler** | ESP32 (DevKit) |
| **Sensor IMU** | MPU9250 (I²C, alamat `0x68`) |
| **Penyimpanan lokal** | Modul SD Card (SPI) |
| **Koneksi I²C** | SDA → GPIO 21, SCL → GPIO 22 |
| **Koneksi SD Card** | SCK → 14, MISO → 19, MOSI → 23, CS → 27 |

**Parameter kalibrasi MPU9250:**
```
Offset X: +0.001  |  Scale X: 0.997
Offset Y: -0.025  |  Scale Y: 0.999
Offset Z: -0.108  |  Scale Z: 1.018
```

### ESP32 #2 — Titik Referensi (`FINAL_PROJECT_ESP2`)

Baca MPU6050 (dengan kalibrasi gyro otomatis), publish MQTT setiap 500ms, kirim ke Google Sheets tiap jam.

| Komponen | Detail |
|---|---|
| **Mikrokontroler** | ESP32 (DevKit) |
| **Sensor IMU** | MPU6050 (I²C, alamat default) |
| **Koneksi I²C** | SDA → GPIO 21, SCL → GPIO 22 |

**Parameter kalibrasi MPU6050:**
```
Offset X: +0.0154  |  Scale X: 0.9955
Offset Y: -0.0107  |  Scale Y: 0.9989
Offset Z: -0.0141  |  Scale Z: 1.0094
```
> Sensor 2 juga melakukan **kalibrasi gyro otomatis** saat startup (`calcGyroOffsets`) — pastikan perangkat tidak bergerak selama proses ini.

---

## 🚀 Instalasi Firmware

### Prasyarat

Pastikan library berikut sudah terinstal di Arduino IDE:

**Untuk ESP1:**
- `MPU9250` (by hideakitai)
- `PubSubClient`
- `WebSocketsServer` (by Links2004)
- `WiFiClientSecure` (bawaan ESP32)

**Untuk ESP2:**
- `MPU6050_tockn`
- `PubSubClient`
- `WebSocketsServer` (by Links2004)
- `WiFiClientSecure` (bawaan ESP32)

### Langkah-langkah

**1. Clone repository**
```bash
git clone https://github.com/Geodigitech-monitoring/sport-hall-tilt-monitoring.git
```

**2. Konfigurasi WiFi**

Buka file `.ino` dan sesuaikan nama jaringan:
```cpp
const char* ssid     = "Sporthall";  // Ganti dengan SSID WiFi kamu
const char* password = "";            // Isi jika ada password
```

**3. Konfigurasi MQTT**

Ganti dengan credential HiveMQ Cloud milikmu (cara mendapatkannya ada di [bagian berikutnya](#-cara-mendapatkan-credential-hivemq-cloud)):
```cpp
const char* mqtt_server   = "YOUR_CLUSTER.s1.eu.hivemq.cloud";
const int   mqtt_port     = 8883;
const char* mqtt_username = "YOUR_USERNAME";
const char* mqtt_password = "YOUR_PASSWORD";
```

> ⚠️ **Jangan hardcode credential di kode yang dipush ke repository publik.** Pertimbangkan menggunakan file konfigurasi terpisah yang masuk ke `.gitignore`.

**4. Upload file web ke SPIFFS (khusus ESP1)**

File `index.html`, `script.js`, `style.css`, dan `unika.png` (dari folder [`web/`](../web/)) perlu diunggah ke memori SPIFFS ESP32 agar bisa diakses via IP lokal.

1. Buat folder `data/` di dalam folder sketch `FINAL_PROJECT_ESP1/`
2. Salin keempat file tersebut ke folder `data/`
3. Di Arduino IDE: **Tools → ESP32 Sketch Data Upload**

> Jika menu tersebut tidak muncul, install plugin [arduino-esp32fs-plugin](https://github.com/me-no-dev/arduino-esp32fs-plugin).

**5. Flash firmware**

- Pilih board: **ESP32 Dev Module**
- Upload speed: `921600`
- Flash masing-masing `.ino` ke ESP32 yang sesuai

> Arduino IDE mengharuskan nama folder sketch sama dengan nama file `.ino`, karena itu setiap firmware berada di foldernya sendiri.

**6. Verifikasi via Serial Monitor**

Buka Serial Monitor (115200 baud). Output yang diharapkan:
```
SD Card initialized successfully.
MPU9250 initialized
WiFi Connected
IP Address: 192.168.x.x
Time synchronized
connected to HiveMQ Cloud!
Roll: 0.12 deg | Pitch: -0.34 deg
Published to HiveMQ: {"roll":0.12,"pitch":-0.34}
```

---

## 🔑 Cara Mendapatkan Credential HiveMQ Cloud

Keempat nilai pada langkah 3 diperoleh dari akun HiveMQ Cloud (gratis, paket *Serverless*):

1. **Daftar / login** di [console.hivemq.cloud](https://console.hivemq.cloud/).
2. **Buat cluster** — klik **Create New Cluster**, pilih paket **Serverless (Free)**, lalu pilih region (misalnya `eu`). Tunggu hingga status cluster menjadi *Running*.
3. **Ambil `mqtt_server` dan `mqtt_port`** — klik **Manage Cluster**, lalu lihat bagian **Overview**:

   | Info di HiveMQ | Isi ke kode | Keterangan |
   |---|---|---|
   | **Cluster URL** (`xxxxxxxx.s1.eu.hivemq.cloud`) | `mqtt_server` | Salin tanpa `mqtt://` / `wss://` |
   | **TLS MQTT Port** (`8883`) | `mqtt_port` | Dipakai oleh ESP32 |
   | **TLS WebSocket Port** (`8884`) | `mqtt_port` di `script.js` | Dipakai oleh web dashboard |

4. **Buat `mqtt_username` dan `mqtt_password`** — buka tab **Access Management** → **Add Credentials**, isi username dan password sesuai keinginan, pilih permission **Publish and Subscribe**, lalu klik **Save**.
   > ⚠️ Password hanya ditampilkan saat dibuat. Simpan di tempat aman; jika lupa, hapus credential lalu buat ulang.
5. **Credential untuk web dashboard** — lihat [web/README.md](../web/README.md#konfigurasi-mqtt-scriptjs).

> 💡 Proyek ini memakai **dua cluster** terpisah (satu per sensor). Ulangi langkah 2–4 untuk ESP2.

Setelah terisi, ESP32 akan terhubung ke broker lewat port `8883` (TLS) dengan topic `building-tilt/sensor-data`.

---

## 📡 Format Data

### Payload MQTT (JSON)

Dikirim ke topic `building-tilt/sensor-data` setiap 500ms:

```json
{
  "roll": -0.45,
  "pitch": 1.23
}
```

### Format CSV SD Card (ESP1)

Disimpan sebagai `/data_YYYY-MM-DD.csv` dengan struktur:

```
Tanggal,Waktu,Roll (derajat),Pitch (derajat),Rata-rata (derajat)
2026-05-23,08:00:00,0.12,-0.34,0.23
2026-05-23,08:30:00,0.15,-0.31,0.23
```

### Endpoint HTTP Lokal (ESP1)

| Endpoint | Deskripsi |
|---|---|
| `GET /` | Dashboard utama |
| `GET /mqtt-status` | Status koneksi MQTT (JSON) |
| `GET /download-sd` | Daftar file CSV di SD Card |
| `GET /download-sd/{filename}` | Download file CSV tertentu |
