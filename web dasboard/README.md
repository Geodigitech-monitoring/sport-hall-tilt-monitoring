# 🌐 Web Dashboard

Dashboard real-time dan historis untuk sistem **Sport Hall Tilt Monitoring**. Kembali ke [README utama](../README.md).

## 📋 Daftar Isi

- [Cara Menjalankan](#-cara-menjalankan)
- [Konfigurasi MQTT (script.js)](#konfigurasi-mqtt-scriptjs)
- [Konfigurasi Google Sheets (historis.html)](#konfigurasi-google-sheets-historishtml)

---

## 🚀 Cara Menjalankan

### Cara A — Hosting statis (GitHub Pages / Netlify)

1. Buka `script.js` dan pastikan [konfigurasi MQTT](#konfigurasi-mqtt-scriptjs) sudah benar
2. Deploy folder `web/` ke hosting statis
3. Akses via URL hosting

### Cara B — Akses langsung dari ESP32

Setelah firmware ESP1 berhasil diflash dan SPIFFS terisi (lihat [firmware/README.md](../firmware/README.md#-instalasi-firmware)):

1. Hubungkan perangkat ke jaringan WiFi yang sama dengan ESP32
2. Buka Serial Monitor untuk melihat IP address ESP32
3. Akses `http://192.168.x.x` di browser

---

## Konfigurasi MQTT (script.js)

Dashboard terhubung ke HiveMQ Cloud lewat **WebSocket Secure** (port `8884`), terpisah dari koneksi ESP32 (port `8883`). Setiap sensor punya cluster sendiri di `MQTT_CONFIGS`:

```javascript
const MQTT_CONFIGS = {
    s1: { hostname: 'CLUSTER_SENSOR_1.s1.eu.hivemq.cloud', port: 8884, username: '...', password: '...' },
    s2: { hostname: 'CLUSTER_SENSOR_2.s1.eu.hivemq.cloud', port: 8884, username: '...', password: '...' }
};
```

**Cara mendapatkan credential web:**

1. Buka [console.hivemq.cloud](https://console.hivemq.cloud/) → **Manage Cluster** → tab **Web Client** → klik **Connect**. HiveMQ membuat credential otomatis berformat `hivemq.webclient.<angka>`.
2. Alternatif: buat credential terpisah di **Access Management** dengan permission **Subscribe Only**, karena dashboard hanya perlu membaca data.
3. Isi `hostname`, `username`, dan `password` untuk `s1` dan `s2` sesuai cluster masing-masing. Cara mendapatkan hostname ada di [firmware/README.md](../firmware/README.md#-cara-mendapatkan-credential-hivemq-cloud).

> ⚠️ Karena `script.js` berjalan di browser, credential di dalamnya bisa dilihat siapa pun yang membuka dashboard. Pakai credential **Subscribe Only** khusus web, jangan credential yang sama dengan ESP32.

---

## Konfigurasi Google Sheets (historis.html)

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
