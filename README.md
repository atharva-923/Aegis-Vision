# 👁️ Aegis Vision

> **Smart Assistive Goggles for the Visually Impaired**  
> Real-time obstacle detection, GPS tracking, and live safety dashboard — built on ESP32 + Firebase.

---

## 📌 About

Aegis Vision is an assistive technology project designed to enhance the independence and safety of visually impaired individuals. The system uses an ESP32 microcontroller mounted on a pair of goggles to detect nearby obstacles using an ultrasonic sensor and alert the user through haptic (vibration) feedback — no sight required.

Live sensor data is streamed to a Firebase backend and displayed on a real-time web dashboard that caregivers or family members can monitor remotely.

---

## ✨ Features

- 🔊 **Haptic Obstacle Detection** — Vibration motor with 3 intensity zones based on distance
- 📡 **Real-time GPS Tracking** — Live location streamed to dashboard via Firebase RTDB
- 🗺️ **Interactive Map** — CartoDB-powered map with movement history and path trail
- 📊 **Live Dashboard** — Distance readings + safety status updated every 500ms
- 🔐 **User Authentication** — Firebase Auth with login & sign-up support
- 🌙 **Dark / Light Mode** — Theme toggle with map tile swap included
- 📜 **Location History** — GPS path logged to Firestore for historical review

---

## 🛠️ Hardware

| Component | Purpose |
|---|---|
| ESP32 | Main microcontroller |
| HC-SR04 Ultrasonic Sensor | Obstacle distance measurement |
| Vibration Motor | Haptic feedback for the user |
| NEO-6M GPS Module | Real-time location tracking |

### Wiring

| Pin | ESP32 GPIO |
|---|---|
| Ultrasonic TRIG | GPIO 5 |
| Ultrasonic ECHO | GPIO 18 |
| Vibration Motor | GPIO 13 |
| GPS RX | GPIO 16 |
| GPS TX | GPIO 17 |

---

## ⚡ Vibration Zones

| Distance | Feedback |
|---|---|
| > 50 cm | No vibration — safe |
| 30 – 50 cm | Slow pulse (every 500ms) — caution |
| 15 – 30 cm | Fast pulse (every 250ms) — warning |
| < 15 cm | Constant ON — danger |

---

## 🗂️ Project Structure

```
aegis-vision/
├── aegis_vision_final.ino   # ESP32 firmware (sensors + WiFi + Firebase)
├── aegis_vision.html        # Single-file web dashboard (login + live dashboard)
└── README.md
```

---

## 🔧 Setup & Installation

### 1. Arduino Firmware

**Required Libraries** (install via Arduino Library Manager):
- `Firebase ESP Client`
- `TinyGPS++`
- `WiFi` (built-in ESP32)

**Steps:**
1. Open `aegis_vision_final.ino` in Arduino IDE
2. Update your WiFi credentials:
   ```cpp
   #define WIFI_SSID     "your_wifi_name"
   #define WIFI_PASSWORD "your_wifi_password"
   ```
3. Select board: **ESP32 Dev Module**
4. Flash to your ESP32

### 2. Firebase Setup

1. Go to [console.firebase.google.com](https://console.firebase.google.com) and create a project
2. Enable **Realtime Database** (region: `asia-southeast1`)
3. Enable **Firestore Database**
4. Enable **Authentication → Email/Password**
5. Create a user in the Authentication tab
6. Set Realtime Database rules:
   ```json
   {
     "rules": {
       ".read": "auth != null",
       ".write": "auth != null"
     }
   }
   ```
7. Set Firestore rules:
   ```javascript
   rules_version = '2';
   service cloud.firestore {
     match /databases/{database}/documents {
       match /{document=**} {
         allow read, write: if request.auth != null;
       }
     }
   }
   ```

### 3. Web Dashboard

The dashboard is a single HTML file — no build tools needed.

- Open `aegis_vision.html` directly in a browser, **or**
- Host it on **GitHub Pages** for remote access

> ⚠️ Hosting on GitHub Pages is recommended — opening locally may cause map tile issues due to missing referer headers.

---

## 🚀 Deploying to GitHub Pages

1. Push your repository to GitHub
2. Go to **Settings → Pages**
3. Set source to **main branch / root**
4. Your dashboard will be live at `https://yourusername.github.io/aegis-vision/aegis_vision.html`

---

## 📡 Firebase Data Structure

### Realtime Database (live data — updates every 500ms)
```
/aegis/live/
  ├── distance   (float, cm)
  ├── lat        (float)
  └── lng        (float)
```

### Firestore (history log — every 2s)
```
Collection: sensorData
Document fields:
  ├── distance   (double)
  ├── lat        (double)
  ├── lng        (double)
  └── timestamp  (integer, millis)
```

## 📄 License

MIT License — free to use, modify, and distribute.

---

<p align="center">Built with ❤️ for the visually impaired community</p>
