# 📊 Google Sheets & Apps Script

Penyimpanan arsip data per jam untuk sistem **Sport Hall Tilt Monitoring**. Kembali ke [README utama](../README.md).

## 📋 Daftar Isi

- [Cara Kerja](#-cara-kerja)
- [Langkah 1 — Buat Google Sheet](#langkah-1--buat-google-sheet)
- [Langkah 2 — Pasang Apps Script](#langkah-2--pasang-apps-script)
- [Langkah 3 — Deploy sebagai Web App](#langkah-3--deploy-sebagai-web-app)
- [Langkah 4 — Isi GAS_URL di Firmware](#langkah-4--isi-gas_url-di-firmware)
- [Langkah 5 — Uji Coba](#langkah-5--uji-coba)
- [Langkah 6 — Publish untuk Dashboard](#langkah-6--publish-untuk-dashboard)
- [Parameter & Format Data](#-parameter--format-data)
- [Memperbarui Script](#-memperbarui-script)
- [Keamanan](#-keamanan)
- [Troubleshooting](#-troubleshooting)

---

## 🔄 Cara Kerja

```
ESP32 ──HTTP GET (tiap jam)──► Apps Script (Web App) ──appendRow──► Google Sheet
                                                                        │
                                  historis.html ◄── Publish to web ─────┘
```

- Firmware memanggil URL Web App lewat `sendToGoogleSheets()` dengan parameter `date`, `time`, `pitch`, `roll`, `average`.
- Script menambahkan satu baris baru ke Sheet.
- `historis.html` membaca Sheet yang sudah dipublikasikan, bukan memanggil script ini.

Dibutuhkan **dua** Google Sheet dan **dua** deployment, satu per sensor. Isi script identik, hanya `SPREADSHEET_ID` yang berbeda:

| File | Untuk | Dipakai di |
|---|---|---|
| [`sensor1.gs`](sensor1.gs) | Sensor 1 (titik ukur) | `GAS_URL` di `FINAL_PROJECT_ESP1.ino` |
| [`sensor2.gs`](sensor2.gs) | Sensor 2 (titik referensi) | `GAS_URL` di `FINAL_PROJECT_ESP2.ino` |

---

## Langkah 1 — Buat Google Sheet

1. Buka [sheets.google.com](https://sheets.google.com) → **Blank spreadsheet**.
2. Beri nama, misalnya `Data Sensor 1 - Sport Hall`.
3. Salin **Spreadsheet ID** dari URL:
   ```
   https://docs.google.com/spreadsheets/d/<SPREADSHEET_ID>/edit
   ```
4. Tab dan header **tidak perlu dibuat manual**. Jika tab `Data_Pergeseran_Sheet` belum ada, script membuatnya otomatis beserta header `Date, Time, Pitch, Roll, Average`.

Ulangi untuk Sensor 2.

---

## Langkah 2 — Pasang Apps Script

1. Buka [script.google.com](https://script.google.com) → **New project**.
2. Hapus isi `Code.gs`, lalu tempel isi [`sensor1.gs`](sensor1.gs) (atau `sensor2.gs` untuk Sensor 2).
3. Ganti nilai `SPREADSHEET_ID` dengan ID dari Langkah 1:
   ```javascript
   var SPREADSHEET_ID = 'ID_SPREADSHEET_KAMU';
   var SHEET_NAME = 'Data_Pergeseran_Sheet';
   ```
4. Beri nama proyek, lalu klik **Save**.

> Gunakan satu proyek Apps Script terpisah untuk tiap sensor.

---

## Langkah 3 — Deploy sebagai Web App

1. Klik **Deploy** → **New deployment**.
2. Klik ikon roda gigi di samping *Select type* → pilih **Web app**.
3. Isi pengaturan berikut:

   | Pengaturan | Nilai |
   |---|---|
   | **Execute as** | **Me** (akun kamu) |
   | **Who has access** | **Anyone** |

   > Jika memilih selain **Anyone**, ESP32 tidak bisa mengakses script karena tidak bisa login.

4. Klik **Deploy**. Saat diminta izin: **Authorize access** → pilih akun → **Advanced** → **Go to (nama proyek) (unsafe)** → **Allow**. Peringatan ini normal untuk script buatan sendiri.
5. Salin **Web app URL** yang berakhiran `/exec`:
   ```
   https://script.google.com/macros/s/AKfycb.../exec
   ```

---

## Langkah 4 — Isi GAS_URL di Firmware

Tempel URL tadi ke variabel `GAS_URL` di firmware yang sesuai:

```cpp
// FINAL_PROJECT_ESP1.ino  → URL deployment Sensor 1
// FINAL_PROJECT_ESP2.ino  → URL deployment Sensor 2
const char* GAS_URL = "https://script.google.com/macros/s/XXXXXXXX/exec";
```

Lalu flash ulang firmware.

---

## Langkah 5 — Uji Coba

Sebelum flash, uji dulu lewat browser dengan menambahkan parameter ke URL `/exec`:

```
https://script.google.com/macros/s/XXXXXXXX/exec?date=2026-01-01&time=08:00:00&pitch=0.12&roll=-0.34&average=0.23
```

Respons yang benar:
```json
{"status":"success","message":"Data saved successfully"}
```

Cek Google Sheet: harus ada baris baru. Hapus baris uji tersebut sebelum dipakai sungguhan.

Untuk membaca semua data sebagai JSON (opsional):
```
https://script.google.com/macros/s/XXXXXXXX/exec?action=getData
```

---

## Langkah 6 — Publish untuk Dashboard

Agar `historis.html` dan tombol **Download XLSX** di dashboard bisa membaca data:

1. Buka Google Sheet → **File** → **Share** → **Publish to web**.
2. Pilih **Entire Document**, format **Web page** (untuk grafik historis) atau **Microsoft Excel (.xlsx)** (untuk link download), lalu **Publish**.
3. Isi `SHEET_IDS` di `historis.html` dan link di `index.html` sesuai ID sheet. Detail ada di [web/README.md](../web/README.md#konfigurasi-google-sheets-historishtml).

---

## 📝 Parameter & Format Data

| Parameter | Isi | Jika kosong |
|---|---|---|
| `date` | `YYYY-MM-DD` | Tanggal server Google (UTC) |
| `time` | `HH:MM:SS` | Jam server sesuai zona waktu proyek |
| `pitch` | Sudut pitch (°) | `0` |
| `roll` | Sudut roll (°) | `0` |
| `average` | Rata-rata `(|roll| + |pitch|) / 2` | Dihitung script |
| `action=getData` | Mengembalikan seluruh isi Sheet sebagai JSON | — |

Firmware selalu mengirim `date` dan `time` dari NTP (WIB), jadi nilai bawaan server tidak terpakai.

Kolom di Sheet:

| A | B | C | D | E |
|---|---|---|---|---|
| Date | Time | Pitch | Roll | Average |

---

## 🔁 Memperbarui Script

Jika kode script diubah setelah deploy, perubahan **belum aktif** sampai dibuat versi baru:

1. **Deploy** → **Manage deployments** → klik ikon pensil.
2. **Version** → **New version** → **Deploy**.

URL `/exec` tetap sama, jadi firmware tidak perlu diubah. Jika memakai **New deployment**, URL berubah dan `GAS_URL` harus diperbarui.

---

## 🔒 Keamanan

Karena akses diatur **Anyone**, siapa pun yang mengetahui URL `/exec` bisa menulis data palsu ke Sheet.

- Jangan commit URL asli ke repository publik. Simpan di file terpisah yang masuk `.gitignore`.
- Opsional, tambahkan token sederhana. Di script, cek di awal `handleRequest()`:
  ```javascript
  if (e.parameter.token !== 'TOKEN_RAHASIA') {
    return ContentService.createTextOutput(JSON.stringify({status: 'error', message: 'Unauthorized'}))
      .setMimeType(ContentService.MimeType.JSON);
  }
  ```
  Lalu di firmware, tambahkan ke URL pada `sendToGoogleSheets()`:
  ```cpp
  url += "&token=TOKEN_RAHASIA";
  ```

---

## 🛠️ Troubleshooting

| Gejala | Kemungkinan penyebab |
|---|---|
| Serial Monitor menampilkan HTTP `302` | Umumnya normal: Apps Script mengalihkan respons. Cek apakah baris baru muncul di Sheet. |
| Data tidak masuk, Serial HTTP `401`/`403`/`404` | **Who has access** bukan **Anyone**, URL salah, atau belum di-deploy ulang setelah perubahan. |
| Respons `status: error` | `SPREADSHEET_ID` salah, atau akun belum memberi izin akses ke Sheet. |
| Jam di Sheet berbeda dari WIB | Hanya terjadi jika `time` tidak dikirim. Atur zona waktu di **Project Settings** Apps Script. |
| Data tidak masuk pada jam tertentu | WiFi/NTP belum siap saat jadwal berjalan. Lihat log `Failed to obtain time` di Serial Monitor. |
