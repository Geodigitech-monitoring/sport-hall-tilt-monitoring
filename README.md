# 🏗️ Sport Hall Tilt Monitoring

Sistem pemantauan kemiringan gedung berbasis web secara **real-time** untuk **Gedung Sport Hall Universitas Katolik Soegijapranata**, Semarang.

![Status](https://img.shields.io/badge/status-active-brightgreen)
![Platform](https://img.shields.io/badge/platform-web-blue)
![MQTT](https://img.shields.io/badge/broker-HiveMQ%20Cloud-orange)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

---

## 📋 Daftar Isi

- [Tentang Proyek](#-tentang-proyek)
- [Fitur](#-fitur)
- [Arsitektur Sistem](#-arsitektur-sistem)
- [Struktur File](#-struktur-file)
- [Cara Penggunaan](#-cara-penggunaan)
- [Konfigurasi Sensor](#-konfigurasi-sensor)
- [Level Status Peringatan](#-level-status-peringatan)
- [Penyimpanan Data](#-penyimpanan-data)
- [Informasi Gedung](#-informasi-gedung)

---

## 📖 Tentang Proyek

Sistem ini dirancang untuk memantau pergeseran dan kemiringan struktural Gedung Sport Hall Unika Soegijapranata secara berkelanjutan. Dua sensor IMU dipasang pada titik strategis gedung dan mengirimkan data sudut kemiringan (**Roll** dan **Pitch**) secara real-time melalui protokol MQTT ke dashboard berbasis web.

Data juga disimpan secara berkala ke Google Sheets dan SD Card sebagai arsip historis yang dapat dianalisis kapan saja melalui halaman Data Historis.

---

## ✨ Fitur

- **Real-time monitoring** — data diperbarui setiap 200ms via MQTT over WebSocket
- **Dual sensor** — Sensor 1 (titik ukur) dan Sensor 2 (titik referensi)
- **Sistem peringatan otomatis** — banner alert dengan 4 level status
- **Dashboard historis** — grafik interaktif dengan kalender navigasi tanggal
- **Tabel perbandingan** — menampilkan delta kemiringan antar kedua sensor
- **Download data** — ekspor data sensor ke format Excel (.xlsx)
- **Responsive design** — tampilan optimal di desktop maupun mobile

---

## 🏛️ Arsitektur Sistem

```
┌─────────────┐     MQTT (200ms)     ┌──────────────────┐
│  Sensor 1   │ ──────────────────►  │                  │
│ (Titik Ukur)│                      │  HiveMQ Cloud    │
└─────────────┘                      │  MQTT Broker     │
                                     │                  │
┌─────────────┐     MQTT (200ms)     │                  │
│  Sensor 2   │ ──────────────────►  │                  │
│ (Referensi) │                      └────────┬─────────┘
└─────────────┘                               │ WebSocket
       │                                      ▼
       │ Setiap jam          ┌────────────────────────────┐
       ├──────────────────►  │     Web Dashboard          │
       │   Google Sheets     │  index.html + script.js    │
       │                     └────────────────────────────┘
       │ Setiap 30 menit
       └──────────────────►  SD Card (lokal)
```

---

## 📁 Struktur File

```
sport-hall-tilt-monitoring/
│
├── index.html        # Dashboard utama real-time
├── historis.html     # Halaman grafik & tabel data historis
├── script.js         # Logika MQTT, update UI, dan status alert
├── style.css         # Stylesheet dashboard utama
└── unika.png         # Logo Universitas Katolik Soegijapranata
```

| File | Deskripsi |
|---|---|
| `index.html` | Halaman utama yang menampilkan data real-time kedua sensor, status koneksi, dan banner peringatan |
| `historis.html` | Halaman data historis dengan kalender navigasi, grafik Chart.js, dan tabel perbandingan antar sensor |
| `script.js` | Mengelola koneksi MQTT untuk dua broker, parsing data JSON, update tampilan, dan logika level alert |
| `style.css` | Desain responsif berbasis CSS variables dengan dukungan animasi dan layout dua kolom |

---

## 🚀 Cara Penggunaan

### 1. Clone repository

```bash
git clone https://github.com/Geodigitech-monitoring/sport-hall-tilt-monitoring.git
cd sport-hall-tilt-monitoring
```

### 2. Konfigurasi MQTT

Buka `script.js` dan sesuaikan konfigurasi broker MQTT untuk masing-masing sensor:

```javascript
const MQTT_CONFIGS = {
    s1: {
        hostname: 'YOUR_S1_HIVEMQ_HOSTNAME',
        port: 8884,
        username: 'YOUR_USERNAME',
        password: 'YOUR_PASSWORD',
        // ...
    },
    s2: {
        hostname: 'YOUR_S2_HIVEMQ_HOSTNAME',
        port: 8884,
        username: 'YOUR_USERNAME',
        password: 'YOUR_PASSWORD',
        // ...
    }
};
```

> ⚠️ **Jangan menyimpan credential langsung di kode.** Gunakan environment variable atau backend proxy untuk keamanan.

### 3. Konfigurasi Google Sheets

Buka `historis.html` dan ganti Sheet ID dengan milikmu:

```javascript
const SHEET_IDS = {
    s1: 'YOUR_GOOGLE_SHEET_ID_SENSOR_1',
    s2: 'YOUR_GOOGLE_SHEET_ID_SENSOR_2'
};
```

Pastikan Google Sheet kamu sudah dipublikasikan ke web: **File → Share → Publish to web**.

Format kolom yang diharapkan (dari kiri):

| Kolom A | Kolom B | Kolom C | Kolom D |
|---|---|---|---|
| Tanggal | Waktu | Pitch (°) | Roll (°) |

### 4. Jalankan

Tidak diperlukan build process. Cukup buka `index.html` di browser, atau deploy ke web server statis seperti GitHub Pages, Netlify, atau Apache/Nginx.

```bash
# Contoh dengan Python local server
python -m http.server 8080
# Buka: http://localhost:8080
```

---

## ⚙️ Konfigurasi Sensor

Sensor mengirim data ke MQTT broker dengan format JSON berikut:

```json
{
  "roll": -0.45,
  "pitch": 1.23
}
```

**Topic MQTT:** `building-tilt/sensor-data`

**Jadwal pengiriman data:**

| Tujuan | Interval | Jumlah/hari |
|---|---|---|
| MQTT (real-time) | Setiap 200ms | — |
| Google Sheets | Setiap jam tepat | 24 data |
| SD Card | Setiap 30 menit | 48 data |

---

## 🚦 Level Status Peringatan

Status ditentukan berdasarkan nilai absolut sudut Roll dan Pitch terbesar dari **Sensor 1 (titik ukur)**:

| Sudut Maksimum | Status | Warna | Tindakan |
|---|---|---|---|
| < 5° | **AMAN** | 🟢 Hijau | Tidak diperlukan tindakan |
| 5° – 10° | **WASPADA** | 🟡 Kuning | Pantau terus kondisi |
| 10° – 20° | **SIAGA** | 🟠 Oranye | Lakukan pemeriksaan segera |
| ≥ 20° | **BAHAYA** | 🔴 Merah | Segera evakuasi dan hubungi petugas |

---

## 💾 Penyimpanan Data

### Google Sheets

Data dari setiap sensor disimpan di Google Sheets masing-masing dan dapat diakses langsung dari dashboard:

- [Google Sheets Sensor 1](https://docs.google.com/spreadsheets/d/1mrrOjf3kJfSZfiNj-cJmOWlMSGfqpblrkNCGmkKKIDc)
- [Google Sheets Sensor 2](https://docs.google.com/spreadsheets/d/1HjXdats-6j_nTqGa1x98_PCKedIaKza6Vd9QtlCUE9s)

Data juga dapat diunduh langsung dalam format `.xlsx` melalui tombol di dashboard.

### SD Card

Data tersimpan secara lokal pada modul SD Card yang terpasang di perangkat sensor sebagai backup offline.

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

- **Frontend:** HTML5, CSS3, Vanilla JavaScript
- **Real-time:** [MQTT.js](https://github.com/mqttjs/MQTT.js) via WebSocket
- **Broker:** [HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/)
- **Grafik:** [Chart.js](https://www.chartjs.org/)
- **Data Historis:** Google Sheets (gviz API)
- **Ikon:** [Font Awesome 6](https://fontawesome.com/)
- **Font:** [Poppins](https://fonts.google.com/specimen/Poppins) (Google Fonts)

---

## 📄 Lisensi

Proyek ini dikembangkan untuk keperluan monitoring struktural Universitas Katolik Soegijapranata.

---

*Sistem Pemantauan Pergeseran Gedung Sport Hall © 2024 — Universitas Katolik Soegijapranata*
