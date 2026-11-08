# Vector Siege

**Vector Siege** adalah game *tower defense* 3D bergaya sci-fi yang dibangun dari nol menggunakan **C++ dan [raylib](https://www.raylib.com/)**.

Pertahankan pangkalan (base) kamu dari gelombang drone, mech, dan colossus yang menyerbu lewat jalur energi. Bangun dan tingkatkan turret, atur strategi penargetan, dan bertahan sampai **50 wave** — lalu lanjutkan ke mode *endless* jika kamu sanggup.

> **Nol asset eksternal.** Semua visual (terrain, turret, musuh, portal, benteng) dirender secara prosedural dengan primitif 3D, dan **semua suara di-sintesis langsung saat runtime** (PCM generator). Tidak ada file gambar, model, atau audio di repo ini.

---

## Daftar Isi
- [Fitur Utama](#fitur-utama)
- [Screenshot](#screenshot)
- [Persyaratan](#persyaratan)
- [Build & Menjalankan](#build--menjalankan)
- [Cara Bermain](#cara-bermain)
- [Tutorial: Strategi Menang](#tutorial-strategi-menang)
- [Daftar Tower](#daftar-tower)
- [Daftar Musuh](#daftar-musuh)
- [Mekanik Wave](#mekanik-wave)
- [Kontrol Lengkap](#kontrol-lengkap)
- [Save / Load & Pengaturan](#save--load--pengaturan)
- [Tipe Data & Arsitektur](#tipe-data--arsitektur)
- [Troubleshooting](#troubleshooting)
- [Lisensi](#lisensi)

---

## Fitur Utama

- 🏰 **3 tipe turret** dengan 3 level upgrade masing-masing (Archer, Mage, Catapult)
- 👾 **4 tipe musuh** termasuk boss raksasa setiap 5 wave
- 🎯 **4 mode penargetan** (First, Last, Strongest, Closest) per tower
- 💥 **Sistem crit** — 15% chance untuk damage ×2
- ⚡ **Kontrol kecepatan game** 1X / 2X / 3X
- 💾 **Save / Load** berbasis JSON (`F5` / `F9`)
- ⚙️ **Pengaturan volume** (Master / SFX / Music) + fullscreen
- ↩️ **Undo penempatan** tower terakhir dengan refund penuh (`Z`)
- 🏆 **High score** tersimpan otomatis
- ✨ **Juice**: particle burst, shockwave, floating text, ambient particles, slow-motion impact, hit-pause
- 🌅 **Sky dinamis** yang berubah dari pagi ke malam seiring wave
- 🎨 **UI sci-fi** lengkap: tooltip, tutorial overlay, pause menu, dialog konfirmasi

---

## Screenshot

> Tambahkan screenshot kamu sendiri ke folder `docs/` dan ganti path di bawah ini.

```
docs/screenshot-gameplay.png
docs/screenshot-boss.png
```

---

## Persyaratan

| Kebutuhan | Versi |
|---|---|
| **OS** | Windows (diuji di Windows 10/11 x64) |
| **Compiler** | g++ dengan dukungan **C++17** (MSYS2 / MinGW-w64 direkomendasikan) |
| **raylib** | 5.x |
| **nlohmann/json** | Sudah disertakan di repo (`src/json.hpp`, v3.11.2) |

### Dependensi Linux / macOS
Kode ini portabel secara prinsip, tetapi Makefile/script build yang disediakan hanya untuk Windows. Untuk Linux, sesuaikan flag linker (`-lGL -lm -lpthread -ldl -lrt -lX11`).

---

## Build & Menjalankan

### 1. Install MSYS2 + raylib

```powershell
# Buka terminal MSYS2 UCRT64
pacman -S mingw-w64-ucrt-x86_64-gcc
pacman -S mingw-w64-ucrt-x86_64-raylib
```

### 2. Clone repo

```powershell
git clone https://github.com/RiffaYP/Vector-Siege.git
cd Vector-Siege
```

### 3. Compile

```powershell
g++ -std=c++17 -O2 -static-libgcc -static-libstdc++ `
  src/main.cpp src/Game.cpp src/Enemy.cpp src/Bullet.cpp `
  src/Tower.cpp src/Renderer3D.cpp `
  -o game.exe -lraylib -lopengl32 -lgdi32 -lwinmm
```

### 4. Jalankan

```powershell
.\game.exe
```

> **Catatan DLL:** build di atas masih membutuhkan `libraylib.dll`. Jika game gagal start dengan error "DLL not found", salin `libraylib.dll` dari folder `C:\msys64\ucrt64\bin\` ke folder yang sama dengan `game.exe`, atau tambahkan `C:\msys64\ucrt64\bin` ke `PATH`.

### Build mode develop (dengan warning)

```powershell
g++ -std=c++17 -Wall -Wextra -g `
  src/main.cpp src/Game.cpp src/Enemy.cpp src/Bullet.cpp `
  src/Tower.cpp src/Renderer3D.cpp `
  -o game_debug.exe -lraylib -lopengl32 -lgdi32 -lwinmm
```

> Proyek ini **bersih dari warning** saat dikompilasi dengan `-Wall -Wextra`.

---

## Cara Bermain

### Alur Dasar

1. **Mulai game** — kamu punya **$500** dan **10 HP base**.
2. **Tekan `ENTER`** (atau klik tombol **START WAVE**) untuk memulai wave pertama.
   > Game sengaja dijeda seperti "gelap" di antara wave agar kamu punya waktu membangun tower.
3. **Bangun tower** — tekan `1` / `2` / `3` untuk memilih tipe tower, lalu **klik kiri** pada area kosong (bukan di atas jalur).
4. **Musuh berjalan** dari portal (kiri atas) menuju base (kanan bawah). Setiap musuh yang sampai ke base mengurangi **1 HP**.
5. **Habiskan wave** → siapkan diri → **`ENTER`** lagi untuk wave berikutnya.
6. **Menang** jika bertahan sampai **wave 50**. Setelah itu kamu bisa lanjut ke **endless mode**.

### Aturan Penempatan Tower

- ❌ **Tidak boleh di atas jalur** (lebar jalur ±80 px).
- ❌ **Tidak boleh terlalu dekat** dengan tower lain (min. ±75 px).
- ✅ Harus punya **uang cukup**.
- ↩️ Setelah menempatkan, tekan **`Z`** untuk membatalkan (refund 100%) — hanya berlaku untuk tower **terakhir**.

### Memilih & Mengelola Tower

Klik tower yang sudah dibangun untuk membuka panel info:

| Aksi | Hasil |
|---|---|
| **`U`** / tombol UPGRADE | Naikkan level (maks. 3) |
| **`S`** / tombol SELL | Jual tower (kembali **70%** dari total investasi) |
| **`T`** | Ganti mode penargetan |

---

## Tutorial: Strategi Menang

### Wave 1–5 — Fondasi

- Mulai dengan **2–3 Archer** di tikungan pertama. Archer murah ($100) dan punya DPS stabil.
- **Jangan upgrade dulu.** Lebih baik punya lebih banyak tower daripada satu tower level tinggi di awal.
- Simpan uang untuk Catapult saat wave 5 (boss pertama).

### Wave 5, 10, 15… — Boss Wave

- Boss punya **1000 HP** dan bergerak lambat (**speed 35**). Fokuskan damage.
- Sebelum boss wave: pastikan setidaknya **satu Catapult** sudah ada. AoE-nya sangat efektif melawan kerumunan pengawal boss.
- Gunakan **mode STRONGEST** pada tower utama untuk fokus ke boss.

### Posisi Tower yang Efektif

Jalur berbelok 4 kali. Tempat terbaik adalah **di dalam tikungan** karena satu tower bisa menjangkau dua segmen jalur sekaligus:

```
        ┌────────────┐
        │  ⭐ TIKUNGAN 2 (550,200) ← paling strategis
   ┌────┘            └────┐
   │  ⭐ (250,400)        │
   └────┐                └────┐
        └────── ⭐ (850,500) ──┘
```

### Target Mode — Kapan Pakai Apa

| Mode | Kapan dipakai |
|---|---|
| **FIRST** | Default. Membunuh musuh yang hampir sampai base — pilihan aman. |
| **LAST** | Untuk "menahan" musuh di belakang agar tower lain ikut menembak. |
| **STRONGEST** | Saat boss wave / melawan Tank. |
| **CLOSEST** | Tower yang menembak sangat cepat (Mage) — minimalkan waktu peluru melayang. |

### Ekonomi

- **Jangan jual tower** kecuali benar-benar salah posisi — kamu kehilangan 30%.
- Wave awal memberi reward kecil (Normal = $25). Manfaatkan untuk menabung Catapult.
- Di wave tinggi, reward Boss ($200) + Tank ($60) membuat ekonomi jauh lebih longgar.

### Kesalahan Umum

- ❌ Terlalu banyak upgrade di awal → kurang tower → kebobolan.
- ❌ Menaruh semua tower di satu area → musuh lolos di segmen lain.
- ❌ Lupa ganti target mode saat boss muncul.
- ❌ Tidak memakai `ENTER` untuk memulai wave → game terlihat "diam" (ini normal, bukan bug!).

---

## Daftar Tower

| | **Archer** (BASIC) | **Mage** (RAPID) | **Catapult** (CANNON) |
|---|---|---|---|
| **Hotkey** | `1` | `2` | `3` |
| **Biaya** | $100 | $150 | $250 |
| **Damage Lv1** | 20 | 8 | 60 |
| **Cooldown Lv1** | 0.80 s | 0.25 s | 1.80 s |
| **Range Lv1** | 160 | 130 | 200 |
| **AoE Lv1** | — | — | 70 |

### Tabel Upgrade

| Level | Archer DMG / CD / RNG | Mage DMG / CD / RNG | Catapult DMG / CD / RNG / AoE |
|---|---|---|---|
| **Lv1** | 20 / 0.80s / 160 | 8 / 0.25s / 130 | 60 / 1.80s / 200 / 70 |
| **Lv2** (biaya) | 30 / 0.75s / 175 — **$150** | 12 / 0.20s / 140 — **$200** | 85 / 1.70s / 215 / 80 — **$250** |
| **Lv3** (biaya) | 45 / 0.70s / 190 — **$250** | 17 / 0.15s / 150 — **$300** | 120 / 1.60s / 230 / 90 — **$400** |

**Nilai jual** = 70% × total investasi. Contoh: Catapult Lv3 (total $900) dijual **$630**.

### Rekomendasi

- **Archer** — tulang punggung. Rasio damage/biaya terbaik. DPS Lv3 ≈ **64,3/s**.
- **Mage** — DPS tertinggi tapi damage per peluru kecil. Sangat bagus dengan mode CLOSEST. DPS Lv3 ≈ **113/s**.
- **Catapult** — lambat tapi AoE. Wajib untuk boss wave dan kerumunan. DPS Lv3 ≈ **75/s** *per target*.

---

## Daftar Musuh

| Musuh | HP | Speed | Reward | Radius | Catatan |
|---|---|---|---|---|---|
| **Normal** (Drone) | 100 | 80 | $25 | 20 | Musuh standar. |
| **Fast** (Scout) | 60 | 140 | $30 | 15 | Cepat tapi rapuh. |
| **Tank** (Heavy Mech) | 250 | 45 | $60 | 26 | Lambat, tebal. Butuh Catapult / Archer Lv3. |
| **BOSS** (Colossus) | 1000 | 35 | $200 | 30 | Muncul setiap wave kelipatan 5. |

---

## Mekanik Wave

- **Jumlah musuh per wave:** `5 + (wave - 1) × 2`
  → Wave 1 = 5 musuh, Wave 2 = 7, Wave 3 = 9, … Wave 50 = **103 musuh**
- **Boss wave:** setiap wave yang merupakan **kelipatan 5** (5, 10, 15, … 50)
- **Interval spawn:** 1 detik (musuh pertama muncul 0,5 detik setelah wave dimulai)
- **Victory:** setelah menyelesaikan **wave 50**
- **Endless mode:** setelah menang, klik **CONTINUE** → mulai wave 51 dengan skala berlanjut

### Komposisi Musuh

| Wave | Komposisi |
|---|---|
| **1** | Semua **Normal** |
| **2** | Normal & Fast bergantian |
| **3** | Campuran, dengan 2 **Tank** (di tengah & di akhir) |
| **≥ 4** (pola berulang tiap 5) | index ke-4 → **Tank**; index 1 & 3 → **Fast**; sisanya **Normal** |
| **Boss wave** | Musuh terakhir = **BOSS**; sisanya campuran Tank / Fast / Normal |

---

## Kontrol Lengkap

### Keyboard

| Tombol | Fungsi |
|---|---|
| `1` `2` `3` | Pilih / batalkan tower Archer / Mage / Catapult |
| `ESC` | Pause / Resume · batalkan build mode · tutup dialog |
| `SPACE` | Pause / Resume |
| `ENTER` | Mulai wave (saat siap) · tutup tutorial |
| `F` / `TAB` | Ganti kecepatan: **1X → 2X → 3X → 1X** |
| `U` | Upgrade tower yang dipilih |
| `S` | Jual tower yang dipilih (muncul konfirmasi) |
| `T` | Ganti mode penargetan |
| `Z` | Undo penempatan tower terakhir (refund penuh) |
| `Y` | Konfirmasi dialog (Yes) |
| `N` | Batalkan dialog (No) |
| `R` | Restart (buka konfirmasi jika belum game over) |
| `F5` | **Save** game |
| `F9` | **Load** game |
| `BACKSPACE` | Tutup menu Settings |

### Mouse

| Aksi | Fungsi |
|---|---|
| **Klik kiri** | Pilih kartu tower · tempatkan tower · pilih tower · klik tombol |
| **Klik kanan** | Batalkan build mode |
| **Hover** | Tampilkan tooltip (stats tower / info musuh) |
| **Drag** | Geser slider volume di Settings |

---

## Save / Load & Pengaturan

| File | Isi |
|---|---|
| `save1.json` | Progres permainan (uang, HP, wave, tower, musuh) |
| `settings.json` | Volume, fullscreen, resolusi, high score |

- Format save saat ini: **`saveVersion: 2`** (loader tetap menerima v1 lama).
- Kedua file ini **tidak ikut di-push ke GitHub** (sudah ada di `.gitignore`).

### Pengaturan yang tersedia

- **Master Volume** — volume keseluruhan
- **SFX Volume** — efek suara (tembakan, ledakan, dll.)
- **Music Volume** — (slider tersedia; game ini belum memutar musik latar)
- **Fullscreen** — aktif/nonaktif
- **High Score** — tersimpan otomatis saat game over / victory

---

## Tipe Data & Arsitektur

```
src/
├── main.cpp          # Entry point
├── Game.h / .cpp     # Game loop, state, UI, wave system, save/load
├── Tower.h / .cpp    # Tower stats, upgrade, targeting, firing
├── Enemy.h / .cpp    # Enemy stats, path following
├── Bullet.h / .cpp   # Projectile, collision, AoE, particles
├── Renderer3D.h/.cpp # Semua rendering 3D prosedural
└── json.hpp          # nlohmann/json (dependency, single-header)
```

### Detail Teknis

- **Kamera:** orthographic top-down, pitch 80°, fovy 32,4°
- **Koordinat:** pemetaan 2D gameplay ↔ 3D world memakai skala `0.045`
- **Mouse picking:** ray-cast dari kursor ke bidang `Y = 0`
- **Jalur:** 9 waypoint, murni horizontal/vertikal
- **Audio:** PCM 44.100 Hz, 16-bit mono, digenerate runtime
- **Frame rate:** dikunci 60 FPS

### cara kerja "Juice"

- **Hit pause:** bukan freeze, tapi memperlambat simulasi ke ~12% selama beberapa frame
- **Slow-motion:** time scale turun ke 0.2–0.7 saat crit/kill, lalu dipulihkan dalam 0,15 s
- **Crit:** 15% chance, damage ×2, teks "CRIT!" melayang

---

## Troubleshooting

| Masalah | Solusi |
|---|---|
| `libraylib.dll not found` | Salin DLL dari `C:\msys64\ucrt64\bin\` atau tambahkan ke `PATH` |
| Game tidak merespons saat dibuka | Tekan `ENTER` — game menunggu kamu memulai wave |
| Tidak ada suara | Cek **Settings → Master & SFX Volume**, dan pastikan device audio aktif |
| Save tidak ditemukan saat `F9` | Pastikan pernah menekan `F5`, dan jalankan game dari folder yang sama |
| Layar hitam / crash saat start | Update driver GPU; raylib butuh dukungan OpenGL 3.3+ |
| Resolusi aneh | Hapus `settings.json`, jalankan ulang (akan kembali ke 1280×720) |

---

## Lisensi

Kode proyek ini bebas digunakan untuk pembelajaran. **[raylib](https://github.com/raysan5/raylib)** berlisensi `zlib`, dan **[nlohmann/json](https://github.com/nlohmann/json)** berlisensi `MIT` — keduanya kompatibel dengan penggunaan bebas.

---

<div align="center">

**Vector Siege** — dibuat dengan C++ & raylib

</div>
