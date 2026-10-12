# 🔧 Firmware ESP32

Firmware untuk dua perangkat ESP32 pada sistem **Sport Hall Tilt Monitoring**. Kembali ke [README utama](../README.md).

## 📋 Daftar Isi

- [Hardware](#-hardware)
- [Instalasi Firmware](#-instalasi-firmware)
- [Cara Mendapatkan Credential HiveMQ Cloud](#-cara-mendapatkan-credential-hivemq-cloud)
- [Penjelasan Program](#-penjelasan-program)
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

**4. Upload file web ke SPIFFS (ESP1 dan ESP2)**

File `index.html`, `script.js`, `style.css`, dan `unika.png` (dari folder [`web/`](../web/)) perlu diunggah ke memori SPIFFS ESP32 agar bisa diakses via IP lokal. Langkah ini wajib untuk **kedua** ESP32, karena firmware menghentikan program jika SPIFFS gagal di-mount.

1. Pastikan setiap `.ino` sudah berada di folder sketch bernama sama (`FINAL_PROJECT_ESP1/` dan `FINAL_PROJECT_ESP2/`, lihat catatan di [README utama](../README.md#-struktur-repository)), lalu buat folder `data/` di dalam masing-masing folder sketch
2. Salin keempat file tersebut ke masing-masing folder `data/`
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

## 🧠 Penjelasan Program

### Alur Umum

**`setup()`** — dijalankan sekali saat ESP32 menyala:

1. Mount **SPIFFS** (file web). Jika gagal, program berhenti.
2. *(ESP1)* Inisialisasi **SD Card** lewat SPI (VSPI, pin 14/19/23/27). Jika gagal, program tetap jalan tanpa SD.
3. Inisialisasi **I²C** (SDA 21, SCL 22, 400 kHz) dan sensor IMU. *(ESP1)* jika MPU9250 tidak terdeteksi, program berhenti. *(ESP2)* kalibrasi gyro otomatis via `calcGyroOffsets()`.
4. Konek **WiFi** (maksimal ±12 detik). Jika gagal, program tetap lanjut.
5. Sinkronisasi waktu **NTP** (`pool.ntp.org`, `time.nist.gov`, UTC+7, maksimal 10 percobaan).
6. Siapkan **MQTT** (TLS, port 8883, keep-alive 60 detik) dan jalankan server **HTTP** (port 80) serta **WebSocket** (port 81).

**`loop()`** — diulang terus-menerus tanpa `delay()`, memakai `millis()` sebagai penjadwal:

| Tugas | Interval |
|---|---|
| `webSocket.loop()` dan `mqttClient.loop()` | Setiap putaran |
| Baca sensor + hitung sudut + kirim ke WebSocket lokal | Setiap 200 ms |
| Publish ke HiveMQ Cloud | Setiap 500 ms |
| `checkSchedule()` (Google Sheets, SD Card) | Setiap putaran |
| Layani request HTTP | Setiap ada client |

### Perbedaan ESP1 dan ESP2

| Aspek | ESP1 (titik ukur) | ESP2 (titik referensi) |
|---|---|---|
| Sensor | MPU9250 (library `MPU9250`) | MPU6050 (library `MPU6050_tockn`) |
| Kalibrasi akselerometer | Offset & scale tetap di kode | Offset & scale tetap di kode |
| Kalibrasi gyro saat startup | Tidak ada | Ada (`calcGyroOffsets`) |
| Koreksi sudut (`rollOffset`, `pitchOffset`) | Tidak ada | Ada, nilai saat ini `0.0` |
| Simpan ke SD Card | Ya (CSV per hari) | Tidak ada |
| Endpoint `/download-sd` & perintah SD | Ya | Tidak ada |
| Google Sheets | Ya, setiap jam | Ya, setiap jam |
| Cluster HiveMQ | Cluster sensor 1 | Cluster sensor 2 |

### Pengolahan Data Sensor

Fungsi `readMPUData()` bekerja dalam tiga tahap:

**1. Koreksi kalibrasi** — nilai akselerometer (satuan *g*) dikoreksi dengan offset dan scale tiap sumbu:
```
ax = (ax_raw - offsetX) / scaleX      (sama untuk ay dan az)
```

**2. Low-pass filter** — meredam noise getaran dengan filter eksponensial, `alpha = 0.05`:
```
ax_f = alpha * ax + (1 - alpha) * ax_f_sebelumnya
```
Semakin kecil `alpha`, semakin halus tetapi semakin lambat respons data.

**3. Hitung sudut** (derajat) dari akselerometer yang sudah difilter:
```
roll  = atan2( ay_f, sqrt(ax_f² + az_f²) ) × 180/π
pitch = atan2(-ax_f, sqrt(ay_f² + az_f²) ) × 180/π
```

Sudut dihitung dari akselerometer saja, jadi pembacaan akurat saat gedung statis. Gyro tidak dipakai dalam perhitungan sudut.

### Fungsi-Fungsi Utama

| Fungsi | Tugas | Ada di |
|---|---|---|
| `readMPUData()` | Baca sensor, kalibrasi, filter, hitung roll & pitch | ESP1, ESP2 |
| `makeSensorJson()` | Bentuk payload `{"roll":..,"pitch":..}` (2 desimal) | ESP1, ESP2 |
| `publishToHiveMQ()` | Publish JSON ke topic `building-tilt/sensor-data` | ESP1, ESP2 |
| `reconnectMQTT()` | Konek ulang ke broker dan subscribe `building-tilt/commands` | ESP1, ESP2 |
| `mqttCallback()` | Terima pesan MQTT. Di ESP1 menangani perintah `GET_SD_DATA` | ESP1, ESP2 |
| `broadcastSensorData()` | Kirim data terbaru ke semua client WebSocket lokal | ESP1, ESP2 |
| `webSocketEvent()` | Tangani koneksi/putus client dan pesan teks WebSocket | ESP1, ESP2 |
| `checkSchedule()` | Baca jam NTP, picu pengiriman sesuai jadwal | ESP1, ESP2 |
| `sendToGoogleSheets()` | Kirim tanggal, waktu, pitch, roll, rata-rata ke Google Apps Script via HTTP GET | ESP1, ESP2 |
| `saveToSDCard()` | Tambah satu baris ke `/data_YYYY-MM-DD.csv` (header dibuat otomatis untuk file baru) | ESP1 |
| `sendSDCardFileList()` / `sendSDCardFile()` | Kirim daftar file / isi file CSV via WebSocket (per 512 karakter) | ESP1 |
| `sendSDCardFileListViaMQTT()` | Kirim daftar file CSV via MQTT | ESP1 |
| `handleHttpClient()` | Routing HTTP sederhana untuk file web dan endpoint | ESP1, ESP2 |
| `serveFile()` / `serveImage()` | Kirim file dari SPIFFS (teks / PNG) | ESP1, ESP2 |
| `handleSDDownload()` | Tampilkan daftar CSV atau unduh satu file CSV | ESP1 |
| `setupTime()` | Sinkronisasi NTP | ESP1, ESP2 |

### Penjadwalan Pengiriman Data

`checkSchedule()` membaca jam dari NTP lalu memakai *flag* agar data tidak terkirim berulang dalam satu jendela waktu:

- **Google Sheets** — saat menit 0 dan detik 0–5, jika `hasSentThisHour` masih `false`. Flag di-reset saat menit 0 detik 0.
- **SD Card (ESP1)** — saat menit 0 atau 30 dan detik 0–5, jika `hasSavedThisHalfHour` masih `false`.

Data yang dikirim adalah nilai roll dan pitch pada saat itu. Kolom rata-rata dihitung `(|roll| + |pitch|) / 2`.

### Komunikasi

| Jalur | Port | Fungsi |
|---|---|---|
| MQTT over TLS → HiveMQ | 8883 | Publish data sensor ke dashboard web |
| WebSocket lokal | 81 | Data real-time untuk dashboard yang dibuka dari IP ESP32; di ESP1 juga menerima `GET_SD_DATA` dan `GET_FILE:<nama>` |
| HTTP lokal | 80 | Melayani `index.html`, `style.css`, `script.js`, `unika.png`, `/mqtt-status`, dan (ESP1) `/download-sd` |
| HTTP GET → Google Apps Script | 443 | Arsip data per jam ke Google Sheets |

Pada `index.html`, placeholder `%IP%`, `%MQTT_STATUS%`, dan `%MQTT_SERVER%` diganti oleh firmware sebelum halaman dikirim.

### Catatan Penting

- **Sertifikat TLS tidak diverifikasi.** Firmware memakai `wifiClient.setInsecure()`, sehingga koneksi tetap terenkripsi tetapi identitas broker tidak dicek.
- **`reconnectMQTT()` bersifat blocking.** Jika broker tidak terjangkau, fungsi ini mengulang dengan jeda 5 detik dan selama itu pembacaan sensor serta server lokal tertahan.
- **Program berhenti (`while(1)`)** jika SPIFFS gagal di-mount atau MPU9250 (ESP1) tidak terdeteksi. Cek Serial Monitor.
- **`mqtt_client_id` sama di kedua firmware** (`esp32-tilt-sensor-01`). Aman selama keduanya memakai cluster berbeda; jika dipindah ke satu cluster, ubah salah satunya agar tidak saling memutus koneksi.

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
