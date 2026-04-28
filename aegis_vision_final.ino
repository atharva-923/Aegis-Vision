#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ─── WIFI CONFIG ─────────────────────────────────
#define WIFI_SSID     "Atharva"
#define WIFI_PASSWORD "8149293167"

// ─── FIREBASE CONFIG ─────────────────────────────
#define FIREBASE_HOST       "aegis-vision-d193f-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH       "dABPADhkQYoOGLqY9R06wKI4aIV41xernaX2qqWS"
#define FIREBASE_API_KEY    "AIzaSyAJAXoGhjP10mZIQfkrlqYKI9EtzJcYg6w"
#define FIREBASE_PROJECT_ID "aegis-vision-d193f"

#define USER_EMAIL    "124kajal7025@sjcem.edu.in"
#define USER_PASSWORD "123456"

// ─── PIN DEFINITIONS ─────────────────────────────
#define TRIG_PIN    5
#define ECHO_PIN    18
#define VIBRO_PIN   13

// ─── THRESHOLDS ──────────────────────────────────
#define DANGER_DISTANCE 50.0

// ─── INTERVALS ───────────────────────────────────
#define SENSOR_INTERVAL    150
#define RTDB_INTERVAL      500
#define FIRESTORE_INTERVAL 2000

// ─── FIREBASE OBJECTS ────────────────────────────
FirebaseData fbdo_rtdb;
FirebaseData fbdo_firestore;
FirebaseAuth auth;
FirebaseConfig config;

// ─── GPS ─────────────────────────────────────────
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);  // RX=16, TX=17

// ─── GLOBALS ─────────────────────────────────────
unsigned long lastSensorRead   = 0;
unsigned long lastRTDBSend     = 0;
unsigned long lastFirebaseSend = 0;
unsigned long lastVibroToggle  = 0;
float currentDistance          = 999.0;
bool vibroState                = false;

// ─────────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  pinMode(TRIG_PIN,  OUTPUT);
  pinMode(ECHO_PIN,  INPUT);
  pinMode(VIBRO_PIN, OUTPUT);

  digitalWrite(TRIG_PIN,  LOW);
  digitalWrite(VIBRO_PIN, LOW);

  // Startup vibration test
  Serial.println("Testing vibration motor...");
  digitalWrite(VIBRO_PIN, HIGH); delay(300);
  digitalWrite(VIBRO_PIN, LOW);  delay(200);
  digitalWrite(VIBRO_PIN, HIGH); delay(300);
  digitalWrite(VIBRO_PIN, LOW);
  Serial.println("Motor OK.");

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());

  // ── Firebase config (exactly Shield-sense style) ──
  config.api_key      = FIREBASE_API_KEY;
  config.database_url = FIREBASE_HOST;
  auth.user.email     = USER_EMAIL;
  auth.user.password  = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  fbdo_rtdb.setResponseSize(4096);
  fbdo_firestore.setResponseSize(4096);

  Serial.print("Waiting for Firebase auth");
  while (!Firebase.ready()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nFirebase ready! Starting sensor loop...\n");
}

// ─────────────────────────────────────────────────
// READ ULTRASONIC — Median of 3
// ─────────────────────────────────────────────────
float readUltrasonic() {
  float readings[3];

  for (int i = 0; i < 3; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(4);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 38000);

    if (duration == 0) {
      readings[i] = 999.0;
    } else {
      float dist = (duration * 0.034) / 2.0;
      readings[i] = (dist < 2.0 || dist > 400.0) ? 999.0 : dist;
    }

    delayMicroseconds(500);
  }

  for (int i = 0; i < 2; i++) {
    for (int j = i + 1; j < 3; j++) {
      if (readings[j] < readings[i]) {
        float temp  = readings[i];
        readings[i] = readings[j];
        readings[j] = temp;
      }
    }
  }

  return readings[1];
}

// ─────────────────────────────────────────────────
// VIBRATION HANDLER — 3 zones
// ─────────────────────────────────────────────────
void handleVibration() {
  unsigned long now = millis();

  if (currentDistance < 15.0) {
    digitalWrite(VIBRO_PIN, HIGH);
    vibroState = true;

  } else if (currentDistance < 30.0) {
    if (now - lastVibroToggle >= 250) {
      vibroState = !vibroState;
      digitalWrite(VIBRO_PIN, vibroState ? HIGH : LOW);
      lastVibroToggle = now;
    }

  } else if (currentDistance < 50.0) {
    if (now - lastVibroToggle >= 500) {
      vibroState = !vibroState;
      digitalWrite(VIBRO_PIN, vibroState ? HIGH : LOW);
      lastVibroToggle = now;
    }

  } else {
    digitalWrite(VIBRO_PIN, LOW);
    vibroState = false;
  }
}

// ─────────────────────────────────────────────────
// MAIN LOOP
// ─────────────────────────────────────────────────
void loop() {

  // 1. Feed GPS parser continuously
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // 2. Read ultrasonic every 150ms
  unsigned long now = millis();
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead  = now;
    currentDistance = readUltrasonic();

    if (currentDistance < 999.0) {
      Serial.printf("Distance : %.1f cm | %s\n",
        currentDistance,
        currentDistance < DANGER_DISTANCE ? "DANGER ⚠" : "SAFE ✓"
      );
    } else {
      Serial.println("Distance : No echo (out of range / blocked)");
    }

    if (gps.location.isValid()) {
      Serial.printf("GPS      : Lat %.6f | Lng %.6f | FIXED ✓\n",
        gps.location.lat(), gps.location.lng());
    } else {
      Serial.println("GPS      : No fix ✗");
    }
  }

  // 3. Vibration motor
  handleVibration();

  // 4. Realtime Database — every 500ms
  if (millis() - lastRTDBSend >= RTDB_INTERVAL && Firebase.ready()) {
    lastRTDBSend = millis();

    float distToSend = (currentDistance >= 999.0) ? 0.0 : currentDistance;
    float latToSend  = gps.location.isValid() ? gps.location.lat() : 0.0;
    float lngToSend  = gps.location.isValid() ? gps.location.lng() : 0.0;

    Firebase.RTDB.setFloat(&fbdo_rtdb, "/aegis/live/distance", distToSend);
    Firebase.RTDB.setFloat(&fbdo_rtdb, "/aegis/live/lat",      latToSend);
    Firebase.RTDB.setFloat(&fbdo_rtdb, "/aegis/live/lng",      lngToSend);

    Serial.printf("⚡ RTDB → Dist: %.1f cm | Lat: %.6f | Lng: %.6f | GPS: %s\n",
      distToSend, latToSend, lngToSend,
      gps.location.isValid() ? "FIXED ✓" : "NO FIX ✗"
    );
  }

  // 5. Firestore — every 2s (history log)
  if (millis() - lastFirebaseSend >= FIRESTORE_INTERVAL && Firebase.ready()) {
    lastFirebaseSend = millis();

    float distToSend = (currentDistance >= 999.0) ? 0.0 : currentDistance;
    float lat        = gps.location.isValid() ? gps.location.lat() : 0.0;
    float lng        = gps.location.isValid() ? gps.location.lng() : 0.0;

    FirebaseJson content;
    content.set("fields/distance/doubleValue",   distToSend);
    content.set("fields/lat/doubleValue",        lat);
    content.set("fields/lng/doubleValue",        lng);
    content.set("fields/timestamp/integerValue", String(millis()));

    if (Firebase.Firestore.createDocument(&fbdo_firestore, FIREBASE_PROJECT_ID, "(default)", "sensorData", content.raw())) {
      Serial.println("✓ Firestore doc written");
    } else {
      Serial.println("✗ Firestore Error: " + fbdo_firestore.errorReason());
    }
  }
}
