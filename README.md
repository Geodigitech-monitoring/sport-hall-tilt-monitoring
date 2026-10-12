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
- [Memulai](#-memulai)
- [Level Status Peringatan](#-level-status-peringatan)
- [Jadwal Pengiriman Data](#-jadwal-pengiriman-data)
- [Informasi Gedung](#-informasi-gedung)
- [Teknologi](#-teknologi)

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
  │ (Sensor 1)  │------│ /data_*.csv  │
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
├── README.md                          # Ringkasan proyek (file ini)
│
├── firmware/
│   ├── README.md                      # Hardware, instalasi firmware, credential HiveMQ
│   ├── FINAL_PROJECT_ESP1/
│   │   └── FINAL_PROJECT_ESP1.ino     # ESP32 #1 — titik ukur (MPU9250 + SD Card)
│   └── FINAL_PROJECT_ESP2/
│       └── FINAL_PROJECT_ESP2.ino     # ESP32 #2 — titik referensi (MPU6050)
│
└── web/
    ├── README.md                      # Setup dashboard & Google Sheets
    ├── index.html                     # Dashboard real-time
    ├── historis.html                  # Grafik & tabel data historis
    ├── script.js                      # Logika MQTT, update UI, level alert
    ├── style.css                      # Stylesheet responsif
    └── unika.png                      # Logo Universitas Katolik Soegijapranata
```

| Folder | Isi | Dokumentasi |
|---|---|---|
| `firmware/` | Kode ESP32 (pembacaan sensor, MQTT, Google Sheets, SD Card) | [firmware/README.md](firmware/README.md) |
| `web/` | Web dashboard real-time dan historis | [web/README.md](web/README.md) |

---

## 🚀 Memulai

1. **Siapkan broker MQTT** dan dapatkan credential HiveMQ Cloud → [firmware/README.md](firmware/README.md#-cara-mendapatkan-credential-hivemq-cloud)
2. **Flash firmware** ke kedua ESP32 → [firmware/README.md](firmware/README.md#-instalasi-firmware)
3. **Jalankan web dashboard** → [web/README.md](web/README.md)

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
