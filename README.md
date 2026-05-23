# 🏗️ Sport Hall Tilt Monitoring

Sistem pemantauan kemiringan gedung berbasis **ESP32 + Web Dashboard** secara real-time untuk **Gedung Sport Hall Universitas Katolik Soegijapranata**, Semarang.

![Status](https://img.shields.io/badge/status-active-brightgreen)
![Platform](https://img.shields.io/badge/platform-ESP32%20%2B%20Web-blue)
![MQTT](https://img.shields.io/badge/broker-HiveMQ%20Cloud-orange)
![Sensor](https://img.shields.io/badge/sensor-MPU9250%20%7C%20MPU6050-purple)

---

## 📋 Daftar Isi

- [Tentang Proyek](#-tentang-proyek)
- [Fitur](#-fitur)
- [Arsitektur Sistem](#-arsitektur-sistem)
- [Struktur Repository](#-struktur-repository)
- [Hardware](#-hardware)
- [Instalasi Firmware](#-instalasi-firmware)
- [Setup Web Dashboard](#-setup-web-dashboard)
- [Format Data](#-format-data)
- [Level Status Peringatan](#-level-status-peringatan)
- [Jadwal Pengiriman Data](#-jadwal-pengiriman-data)
- [Informasi Gedung](#-informasi-gedung)

---

## 📖 Tentang Proyek

Sistem ini memantau pergeseran dan kemiringan struktural Gedung Sport Hall Unika Soegijapranata secara berkelanjutan. Dua perangkat **ESP32** dipasang pada titik strategis gedung, masing-masing dilengkapi sensor IMU untuk mengukur sudut **Roll** dan **Pitch**.

Data dikirim secara real-time ke web dashboard melalui protokol MQTT, disimpan ke Google Sheets sebagai arsip berkala, dan dicatat ke SD Card sebagai backup lokal (khusus ESP1).

---

## ✨ Fitur

- **Real-time monitoring** — data diperbarui setiap 200ms via MQTT over WebSocket
- **Dual sensor** — ESP1 sebagai titik ukur, ESP2 sebagai titik referensi
- **Low-pass filter** — peredam noise pada pembacaan akselerometer (α = 0.05)
- **Kalibrasi sensor** — offset & scale factor tersimpan di firmware
- **Sinkronisasi waktu NTP** — timestamp akurat via `pool.ntp.org` (UTC+7)
- **Sistem peringatan otomatis** — banner alert dengan 4 level status
- **Dashboard historis** — grafik interaktif dengan navigasi kalender
- **WebSocket lokal** — akses dashboard langsung dari IP ESP32
- **Download data SD Card** — unduh file CSV historis via HTTP (ESP1)
- **Responsive design** — tampilan optimal di desktop maupun mobile

---

## 🏛️ Arsitektur Sistem

```
                        WiFi: "Sporthall"
                       ┌─────────────────────┐
  ┌──────────────┐     │                     │     ┌─────────────────────────┐
  │    ESP32 #1  │─────┤   HiveMQ Cloud      ├────►│    Web Dashboard        │
  │  (Sensor 1)  │MQTT │   (MQTT Broker)     │WSS  │  index.html             │
  │  MPU9250     │SSL  │                     │     │  + script.js            │
  │  + SD Card   │     └─────────────────────┘     └─────────────────────────┘
  └──────┬───────┘
         │ HTTP GET (setiap jam)
         ▼
  ┌─────────────┐      ┌──────────────┐
  │Google Sheets│      │  SD Card     │
  │ (Sensor 1)  │      │ /data_*.csv  │
  └─────────────┘      │(setiap 30min)│
                        └──────────────┘

  ┌──────────────┐
  │    ESP32 #2  │─────► HiveMQ Cloud ──► Web Dashboard
  │  (Sensor 2)  │MQTT
  │  MPU6050     │SSL   HTTP GET (setiap jam)
  └──────┬───────┘           │
         └──────────────────►▼
                        ┌─────────────┐
                        │Google Sheets│
                        │ (Sensor 2)  │
                        └─────────────┘
```

---

## 📁 Struktur Repository

```
sport-hall-tilt-monitoring/
│
├── FINAL_PROJECT_ESP1.ino   # Firmware ESP32 #1 — titik ukur (MPU9250 + SD Card)
├── FINAL_PROJECT_ESP2.ino   # Firmware ESP32 #2 — titik referensi (MPU6050)
│
├── index.html               # Web dashboard real-time
├── historis.html            # Halaman grafik & tabel data historis
├── script.js                # Logika MQTT, update UI, dan level alert
├── style.css                # Stylesheet responsif
└── unika.png                # Logo Universitas Katolik Soegijapranata
```

| File | Deskripsi |
|---|---|
| `FINAL_PROJECT_ESP1.ino` | Firmware sensor utama: baca MPU9250, publish MQTT setiap 500ms, simpan ke Google Sheets tiap jam, simpan ke SD Card tiap 30 menit, sediakan server HTTP + WebSocket lokal |
| `FINAL_PROJECT_ESP2.ino` | Firmware sensor referensi: baca MPU6050 (dengan kalibrasi gyro otomatis), publish MQTT setiap 500ms, kirim ke Google Sheets tiap jam |
| `index.html` | Dashboard utama yang menampilkan data real-time, status koneksi, dan banner peringatan |
| `historis.html` | Halaman data historis dengan kalender, grafik Chart.js, dan tabel delta antar sensor |
| `script.js` | Mengelola koneksi MQTT dua broker, parsing JSON, dan logika 4 level alert |
| `style.css` | Desain responsif berbasis CSS variables dengan animasi status dan layout dua kolom |

---

## 🔧 Hardware

### ESP32 #1 — Titik Ukur

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

### ESP32 #2 — Titik Referensi

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

**Untuk ESP1 (`FINAL_PROJECT_ESP1.ino`):**
- `MPU9250` (by hideakitai)
- `PubSubClient`
- `WebSocketsServer` (by Links2004)
- `WiFiClientSecure` (bawaan ESP32)

**Untuk ESP2 (`FINAL_PROJECT_ESP2.ino`):**
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

Ganti dengan credential HiveMQ Cloud milikmu:
```cpp
const char* mqtt_server   = "YOUR_CLUSTER.s1.eu.hivemq.cloud";
const int   mqtt_port     = 8883;
const char* mqtt_username = "YOUR_USERNAME";
const char* mqtt_password = "YOUR_PASSWORD";
```

> ⚠️ **Jangan hardcode credential di kode yang dipush ke repository publik.** Pertimbangkan menggunakan file konfigurasi terpisah yang masuk ke `.gitignore`.

**4. Upload file web ke SPIFFS (khusus ESP1)**

File `index.html`, `script.js`, `style.css`, dan `unika.png` perlu diunggah ke memori SPIFFS ESP32 agar bisa diakses via IP lokal.

Di Arduino IDE: **Tools → ESP32 Sketch Data Upload**

> Jika menu tersebut tidak muncul, install plugin [arduino-esp32fs-plugin](https://github.com/me-no-dev/arduino-esp32fs-plugin) dan letakkan file web di folder `data/` di dalam direktori sketch.

**5. Flash firmware**

- Pilih board: **ESP32 Dev Module**
- Upload speed: `921600`
- Flash masing-masing `.ino` ke ESP32 yang sesuai

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

## 🌐 Setup Web Dashboard

Dashboard dapat diakses dengan dua cara:

### Cara A — Hosting statis (GitHub Pages / Netlify)

1. Buka `script.js` dan pastikan konfigurasi MQTT sudah benar
2. Deploy folder proyek ke hosting statis
3. Akses via URL hosting

### Cara B — Akses langsung dari ESP32

Setelah firmware berhasil diflash dan SPIFFS terisi:
1. Hubungkan device ke jaringan WiFi yang sama dengan ESP32
2. Buka Serial Monitor untuk melihat IP address ESP32
3. Akses `http://192.168.x.x` di browser

### Konfigurasi Google Sheets untuk historis.html

1. Buka `historis.html` dan ganti Sheet ID:
```javascript
const SHEET_IDS = {
    s1: 'YOUR_GOOGLE_SHEET_ID_SENSOR_1',
    s2: 'YOUR_GOOGLE_SHEET_ID_SENSOR_2'
};
```

2. Pastikan Google Sheet sudah dipublikasikan: **File → Share → Publish to web**

3. Format kolom yang diharapkan:

| Kolom A | Kolom B | Kolom C | Kolom D | Kolom E |
|---|---|---|---|---|
| Tanggal | Waktu | Pitch (°) | Roll (°) | Rata-rata (°) |

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

---

## 🚦 Level Status Peringatan

Status ditentukan dari nilai absolut terbesar antara sudut Roll dan Pitch **Sensor 1 (titik ukur)**:

| Sudut Maks. | Status | Warna | Tindakan |
|---|---|---|---|
| < 5° | **AMAN** | 🟢 Hijau | Tidak diperlukan tindakan |
| 5° – 10° | **WASPADA** | 🟡 Kuning | Pantau terus kondisi gedung |
| 10° – 20° | **SIAGA** | 🟠 Oranye | Lakukan pemeriksaan segera |
| ≥ 20° | **BAHAYA** | 🔴 Merah | Segera evakuasi & hubungi petugas |

---

## 🕐 Jadwal Pengiriman Data

| Tujuan | Interval | Catatan |
|---|---|---|
| MQTT → HiveMQ | Setiap 500ms | Publish dari firmware |
| WebSocket lokal | Setiap 200ms | Update UI dashboard lokal |
| Google Sheets | Setiap jam tepat | Menit 0, detik 0–5 |
| SD Card (ESP1) | Setiap 30 menit | Menit 0 dan 30 |

Waktu disinkronkan via NTP ke zona **UTC+7 (WIB)** menggunakan server `pool.ntp.org`.

---

## 🏢 Informasi Gedung

| Atribut | Detail |
|---|---|
| **Nama** | Gedung Sport Hall |
| **Institusi** | Universitas Katolik Soegijapranata |
| **Alamat** | Jl. Pawiyatan Luhur IV/1, Bendan Duwur, Gajahmungkur, Semarang, Jawa Tengah |
| **Tahun Dibangun** | 2013 |
| **Luas Bangunan** | 2.470 m² |
| **Kapasitas** | 2.000 orang |

---

## 🛠️ Teknologi

**Firmware:**
- ESP32 Arduino Core
- [MPU9250](https://github.com/hideakitai/MPU9250) (sensor ESP1)
- [MPU6050_tockn](https://github.com/tockn/MPU6050_tockn) (sensor ESP2)
- [PubSubClient](https://github.com/knolleary/pubsubclient) (MQTT)
- [WebSocketsServer](https://github.com/Links2004/arduinoWebSockets)

**Web Dashboard:**
- HTML5, CSS3, Vanilla JavaScript
- [MQTT.js](https://github.com/mqttjs/MQTT.js) via WebSocket
- [HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/) (MQTT broker)
- [Chart.js](https://www.chartjs.org/) (grafik historis)
- Google Sheets gviz API (data historis)
- [Font Awesome 6](https://fontawesome.com/) & [Poppins](https://fonts.google.com/specimen/Poppins)

---

*Sistem Pemantauan Pergeseran Gedung Sport Hall © 2024 — Universitas Katolik Soegijapranata*
