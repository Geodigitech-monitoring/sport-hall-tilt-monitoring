# 🖥️ Web Dashboard

Dashboard pemantauan kemiringan gedung **Sport Hall Tilt Monitoring** yang berjalan di browser: tampilan **real-time** lewat MQTT dan **historis** dari Google Sheets. Kembali ke [README utama](../README.md).

## 📋 Daftar Isi

- [Fitur](#-fitur)
- [Struktur Folder](#-struktur-folder)
- [Cara Kerja](#-cara-kerja)
- [Konfigurasi MQTT (script.js)](#konfigurasi-mqtt-scriptjs)
- [Konfigurasi Google Sheets (historis.html)](#konfigurasi-google-sheets-historishtml)
- [Tombol Unduh & Tautan Sheet (index.html)](#tombol-unduh--tautan-sheet-indexhtml)
- [Menjalankan Dashboard](#-menjalankan-dashboard)
- [Level Status Peringatan](#-level-status-peringatan)
- [Kustomisasi](#-kustomisasi)
- [Keamanan](#-keamanan)
- [Troubleshooting](#-troubleshooting)

---

## ✨ Fitur

**Dashboard real-time (`index.html`)**

- Dua kolom sensor: **Sensor 1** (titik ukur) dan **Sensor 2** (titik referensi).
- Sudut **Roll**, **Pitch**, dan status gedung diperbarui langsung saat data MQTT masuk.
- Banner peringatan otomatis berdasarkan **Sensor 1** (titik ukur).
- Indikator koneksi HiveMQ terpisah untuk tiap sensor.
- Tombol unduh XLSX dan tautan Google Sheets untuk kedua sensor.

**Dashboard historis (`historis.html`)**

- Kalender untuk memilih tanggal, dengan penanda tanggal yang memiliki data per sensor.
- Grafik **Roll** dan **Pitch** yang membandingkan Sensor 1 dan Sensor 2 (Chart.js).
- Kartu statistik dan tabel data per jam dengan kolom pencarian waktu.

---

## 📁 Struktur Folder

```
web/
├── README.md        # Dokumentasi dashboard (file ini)
├── index.html       # Dashboard real-time
├── historis.html    # Grafik & tabel historis
├── script.js        # Koneksi MQTT, update tampilan, level peringatan
├── style.css        # Stylesheet responsif
└── unika.png        # Logo Universitas Katolik Soegijapranata
```

Library dimuat dari CDN, jadi tidak perlu `npm install`:

| Library | Dipakai di | Sumber |
|---|---|---|
| MQTT.js | `index.html` | `unpkg.com/mqtt` |
| Chart.js | `historis.html` | `cdn.jsdelivr.net/npm/chart.js` |
| Font Awesome 6.4 | keduanya | `cdnjs.cloudflare.com` |
| Poppins | keduanya | Google Fonts |

---

## 🔄 Cara Kerja

```
ESP32 #1 ──MQTT/TLS──► HiveMQ cluster 1 ──WSS──┐
                                                ├──► index.html (script.js)
ESP32 #2 ──MQTT/TLS──► HiveMQ cluster 2 ──WSS──┘

ESP32 #1/#2 ──HTTP GET (tiap jam)──► Apps Script ──► Google Sheet 1 / 2
                                                          │
                                  historis.html ◄── gviz ─┘
```

- Tiap sensor memakai **cluster HiveMQ sendiri**, sehingga `script.js` membuka **dua koneksi WebSocket** (`s1` dan `s2`) yang berjalan terpisah.
- Browser terhubung lewat **WSS port 8884** (path `/mqtt`), sedangkan ESP32 memakai MQTT TLS port 8883.
- Kedua sensor memublikasikan ke topik yang sama, `building-tilt/sensor-data`, di cluster masing-masing.
- `historis.html` tidak memakai MQTT. Halaman ini membaca Google Sheet langsung dari browser lewat endpoint **gviz**, bukan lewat Apps Script. Cara menyiapkan Sheet ada di [gsheet/README.md](../gsheet/README.md).

### Format pesan MQTT

Dashboard membaca payload JSON berikut dan menolak pesan yang `roll` atau `pitch`-nya bukan angka:

```json
{"roll": 1.23, "pitch": -0.45}
```

Field lain di payload diabaikan. Firmware memublikasikan setiap **500 ms**.

---

## Konfigurasi MQTT (script.js)

Kredensial kedua cluster ada di objek `MQTT_CONFIGS` pada bagian atas `script.js`:

```javascript
const MQTT_CONFIGS = {
    s1: {
        hostname: '<CLUSTER_1>.s1.eu.hivemq.cloud',
        port: 8884,
        username: '<USERNAME_WEB_1>',
        password: '<PASSWORD_WEB_1>',
        // ...
    },
    s2: {
        hostname: '<CLUSTER_2>.s1.eu.hivemq.cloud',
        port: 8884,
        username: '<USERNAME_WEB_2>',
        password: '<PASSWORD_WEB_2>',
        // ...
    }
};

const TOPIC = 'building-tilt/sensor-data';
```

| Parameter | Isi | Catatan |
|---|---|---|
| `hostname` | Hostname cluster HiveMQ | Sama dengan `mqtt_server` di firmware sensor terkait |
| `port` | `8884` | Port **WebSocket TLS**, berbeda dari port 8883 milik firmware |
| `username` / `password` | Kredensial untuk browser | Lihat di bawah |
| `TOPIC` | `building-tilt/sensor-data` | Harus sama dengan `topic_sensor` di firmware |

**Pasangan yang harus cocok:** `s1` ↔ cluster ESP1 (`FINAL_PROJECT_ESP1.ino`), `s2` ↔ cluster ESP2 (`FINAL_PROJECT_ESP2.ino`). Jika tertukar, data Sensor 1 akan tampil di kolom Sensor 2.

### Membuat kredensial untuk browser

1. Buka [HiveMQ Cloud Console](https://console.hivemq.cloud/) → pilih cluster → **Web Client**.
2. Atau buat kredensial khusus di **Access Management** → **Add Credentials**.
3. Beri izin **Subscribe** pada topik `building-tilt/sensor-data`. Dashboard hanya membaca data, jadi izin **Publish** tidak diperlukan. Alasannya ada di bagian [Keamanan](#-keamanan).
4. Salin username dan password ke `MQTT_CONFIGS` di `script.js`.

---

## Konfigurasi Google Sheets (historis.html)

Isi ID kedua Sheet di bagian `CONFIG` pada `historis.html`:

```javascript
const SHEET_IDS = {
    s1: 'ID_SPREADSHEET_SENSOR_1',
    s2: 'ID_SPREADSHEET_SENSOR_2'
};
```

ID diambil dari URL Sheet:

```
https://docs.google.com/spreadsheets/d/<SPREADSHEET_ID>/edit
```

Syarat agar data bisa terbaca:

1. **ID harus sesuai sensornya.** `s1` harus sama dengan `SPREADSHEET_ID` di `sensor1.gs`, dan `s2` dengan `sensor2.gs`.
2. **Sheet harus bisa dibaca publik.** Atur **Share** → **Anyone with the link** → **Viewer**. Tanpa ini, `fetch` ke gviz gagal dan halaman historis kosong.
3. **Tab pertama** di Sheet yang dibaca. Kolom yang dipakai: `A` Date, `B` Time, `C` Pitch, `D` Roll. Urutan kolom ini sudah sesuai dengan yang ditulis `gsheet/sensor*.gs`, jadi jangan diubah.

Baris yang tanggal, pitch, atau roll-nya kosong akan dilewati.

---

## Tombol Unduh & Tautan Sheet (index.html)

Tombol di bagian atas `index.html` memakai tautan yang ditulis langsung di HTML. Ganti sesuai Sheet milikmu:

| Tombol | Jenis tautan |
|---|---|
| Download XLSX Sensor 1 / 2 | `https://docs.google.com/spreadsheets/d/e/<PUBLISH_ID>/pub?output=xlsx` |
| Google Sheets Sensor 1 / 2 | `https://docs.google.com/spreadsheets/d/<SPREADSHEET_ID>/edit?usp=sharing` |

`<PUBLISH_ID>` (diawali `2PACX-`) berbeda dari `SPREADSHEET_ID`. Dapatkan dengan **File** → **Share** → **Publish to web** → pilih **Entire Document**, format **Microsoft Excel (.xlsx)** → **Publish**, lalu salin URL yang diberikan.

Tombol **Data Historis** menuju `historis.html` dan tidak perlu diubah.

---

## 🚀 Menjalankan Dashboard

> Buka lewat **server HTTP**, jangan klik dua kali `index.html` dari folder (`file://`). Sebagian browser memblokir koneksi WebSocket dan `fetch` dari halaman `file://`.

### Lokal (untuk uji coba)

```bash
cd web
python -m http.server 8000
```

Buka `http://localhost:8000`.

### Hosting statis (akses dari mana saja)

Folder `web/` hanya berisi file statis, jadi bisa di-host di GitHub Pages, Netlify, atau Cloudflare Pages. Contoh GitHub Pages:

1. **Settings** → **Pages** → **Source:** branch utama, folder `/web`. Jika folder `/web` tidak tersedia di pilihan, gunakan Netlify atau Cloudflare Pages dengan *publish directory* `web`.
2. Tunggu proses deploy, lalu buka URL yang diberikan.

Halaman harus dibuka lewat **HTTPS** karena koneksi MQTT memakai `wss://`.

### Dari ESP32 (jaringan lokal)

Firmware ESP1 dan ESP2 menyajikan dashboard dari memori SPIFFS lewat IP perangkat. Salin `index.html`, `script.js`, `style.css`, dan `unika.png` ke folder `data/` tiap sketch, lalu unggah dengan **ESP32 Sketch Data Upload**. Detail ada di [firmware/README.md](../firmware/README.md).

`historis.html` tidak termasuk dan firmware tidak punya route untuk halaman itu, jadi tombol **Data Historis** pada dashboard lokal tidak akan berfungsi. Pakai versi yang di-host untuk melihat data historis.

---

## 🚦 Level Status Peringatan

Status per sensor dihitung dari nilai absolut terbesar antara Roll dan Pitch. **Banner peringatan hanya mengikuti Sensor 1.**

| Sudut Maks. | Status | Warna | Banner |
|---|---|---|---|
| < 5° | **AMAN** | 🟢 Hijau | Tidak tampil |
| 5° – 10° | **WASPADA** | 🟡 Kuning | Pantau terus kondisi |
| 10° – 20° | **SIAGA** | 🟠 Oranye | Lakukan pemeriksaan segera |
| ≥ 20° | **BAHAYA** | 🔴 Merah | Segera evakuasi dan hubungi petugas |

Angka Roll dan Pitch juga berwarna sesuai ambang yang sama. Status BAHAYA berkedip untuk menarik perhatian.

---

## 🎨 Kustomisasi

| Yang ingin diubah | Lokasi |
|---|---|
| Ambang batas status | `getAngleColor()`, `getBuildingStatus()`, dan `updateAlertBanner()` di `script.js` (ubah ketiganya agar konsisten) |
| Teks dan pesan banner | `updateAlertBanner()` di `script.js` |
| Warna tema | Variabel `:root` di `style.css` (`--primary-color`, `--secondary-color`, dll.) |
| Judul dan informasi gedung | `index.html` |
| Topik MQTT | `TOPIC` di `script.js` dan `topic_sensor` di kedua firmware |

---

## 🔒 Keamanan

Semua kode di `web/` dikirim ke browser, jadi **siapa pun yang membuka halaman bisa melihat username dan password MQTT** di `script.js` lewat *View Source*.

- Gunakan kredensial HiveMQ **khusus dashboard** dengan izin **Subscribe saja**. Jangan memakai ulang kredensial milik firmware, karena kredensial itu punya izin Publish.
- Dengan izin Subscribe saja, kredensial yang bocor hanya bisa membaca data, tidak bisa mengirim data palsu ke dashboard.
- Jika repository dibuat publik, pertimbangkan untuk tidak meng-commit kredensial asli. Simpan di file terpisah yang masuk `.gitignore`, atau putar (*rotate*) kredensial di HiveMQ Console bila sudah terlanjur ter-commit.
- Sheet historis bersifat **publik hanya-baca** lewat tautan. Pastikan isinya memang boleh dilihat umum.

---

## 🛠️ Troubleshooting

| Gejala | Kemungkinan penyebab |
|---|---|
| Indikator tetap "Menghubungkan ke HiveMQ..." atau "Error koneksi" | Username/password salah, hostname salah, atau halaman dibuka lewat `file://`. Buka Console browser (F12) dan lihat log `[S1]` / `[S2]`. |
| Terhubung, tetapi angka tetap `0.00°` | ESP32 belum online atau belum memublikasikan. Cek Serial Monitor firmware untuk log `Published to HiveMQ`. Pastikan `TOPIC` sama dengan firmware. |
| Data Sensor 1 muncul di kolom Sensor 2 | `s1` dan `s2` di `MQTT_CONFIGS` tertukar terhadap cluster firmware. |
| Hanya satu sensor yang terhubung | Kredensial atau hostname sensor yang lain salah. Dua koneksi berjalan terpisah, jadi satu gagal tidak mempengaruhi yang lain. |
| Halaman historis kosong | Sheet belum dibagikan sebagai **Anyone with the link**, `SHEET_IDS` salah, atau Sheet belum berisi data. |
| Grafik historis hanya menampilkan satu sensor | ID Sheet sensor lain salah, atau sensor itu belum mengirim data pada tanggal yang dipilih. |
| Kedua grafik menampilkan data yang sama | `SPREADSHEET_ID` di `sensor1.gs` dan `sensor2.gs` sama, sehingga kedua sensor menulis ke Sheet yang sama. |
| Tombol **Data Historis** error di IP ESP32 | Memang tidak didukung firmware. Gunakan versi yang di-host. |
| Tombol **Download XLSX** membuka halaman error | Sheet belum di-*publish* dalam format `.xlsx`, atau tautan memakai `SPREADSHEET_ID`, bukan `PUBLISH_ID`. |
