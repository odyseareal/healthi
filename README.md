# Healthi

Healthi is an Arduino UNO R4 WiFi fitness and wellbeing tracker with a live GitHub Pages dashboard. The Arduino identifies itself with its Wi-Fi MAC address, restores its latest saved metrics at startup and uploads a current snapshot to Supabase. The website signs in with the same MAC address and polls the same record.

## Repository map

- `arduino/Healthi/Healthi.ino` — the single Arduino entry point.
- `arduino/Healthi/HealthiNetwork.cpp` — Wi-Fi, HTTPS upload and startup restoration.
- `arduino/Healthi/arduino_secrets.example.h` — safe configuration template.
- `standalone/index.html` — GitHub Pages dashboard.
- `standalone/config.js` — public Supabase URL and publishable key.
- `supabase/healthi.sql` — database table and exact-device RPC functions.

## 1. Create the free database

1. Create a free project at [Supabase](https://supabase.com).
2. Open **SQL Editor**, paste all of `supabase/healthi.sql`, then select **Run**.
3. Open **Project Settings → API** (or **Integrations → Data API**).
4. Copy the project URL, such as `https://abcxyz.supabase.co`.
5. Copy the **publishable** key (`sb_publishable_...`). Never use a secret or `service_role` key in this repository, website or Arduino.

The table is not directly granted to public clients. Three database functions validate one exact MAC address per request; there is no public function for listing devices. A MAC address is still an identifier, not secure authentication, so do not store sensitive medical information in this passwordless version.

## 2. Configure GitHub Pages

Edit `standalone/config.js`:

```js
window.HEALTHI_CLOUD = {
  supabaseUrl: "https://YOUR_PROJECT_REF.supabase.co",
  publishableKey: "sb_publishable_YOUR_KEY",
  pollIntervalMs: 1000
};
```

The publishable key is designed for public clients and is protected by database permissions. Do not paste a Supabase secret key here.

The existing GitHub Actions workflow publishes every file in `standalone/`. In **Settings → Pages**, set the source to **GitHub Actions**. After the workflow finishes, open [odyseareal.github.io/healthi](https://odyseareal.github.io/healthi), enter the Arduino MAC address and select **Open device**.

## 3. Configure the Arduino

1. In `arduino/Healthi/`, copy `arduino_secrets.example.h` to a new file named `arduino_secrets.h`.
2. Change `HEALTHI_CLOUD_ENABLED` to `true`.
3. Enter the Wi-Fi name/password, the Supabase hostname without `https://`, and the same publishable key:

```cpp
#define HEALTHI_CLOUD_ENABLED true
#define HEALTHI_WIFI_SSID "YOUR_WIFI_NAME"
#define HEALTHI_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define HEALTHI_SUPABASE_HOST "YOUR_PROJECT_REF.supabase.co"
#define HEALTHI_SUPABASE_PUBLISHABLE_KEY "sb_publishable_YOUR_KEY"
#define HEALTHI_SYNC_INTERVAL_MS 1000UL
```

`arduino_secrets.h` is ignored by Git. Do not commit it because it contains the Wi-Fi password.

## 4. Install libraries and upload

Install these Arduino IDE libraries:

- WiFiS3
- ArduinoHttpClient
- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit MPU6050
- Adafruit Unified Sensor
- Adafruit BMP085 Library
- QMC5883LCompass

Open `arduino/Healthi/Healthi.ino`, keep every `.ino`, `.cpp` and `.h` file in the same `Healthi` folder, select **Arduino UNO R4 WiFi**, then upload. Open Serial Monitor at **115200 baud**. It prints the device ID in `AA:BB:CC:DD:EE:FF` format.

At startup the firmware:

1. connects to Wi-Fi;
2. derives the device ID from the UNO R4 WiFi MAC address;
3. requests the saved snapshot for that exact ID;
4. restores steps, ascent/flights, calories, exercise totals, sleep, mood and fall-alert state;
5. uploads a replacement snapshot every second.

The database stores one current row per device, not one row per second. This keeps the free database small while the dashboard remains near-live. If HTTPS requests interfere with sensor sampling on a slow network, change `HEALTHI_SYNC_INTERVAL_MS` to `5000UL`.

## Website data

Food entries, mood check-ins, workouts and daily charts are saved in the row's separate `website_data` field. Arduino uploads only replace `metrics`, so they cannot overwrite website-entered data. The website keeps a local copy for offline display and synchronises changes to Supabase when signed in.

## Hardware

- Arduino UNO R4 WiFi
- GY-87: MPU6050, BMP180 and QMC5883L
- SSD1306 128×64 I2C OLED
- analogue joystick with push switch
- HC-SR04 ultrasonic sensor

The complete wiring guide remains at the top of `Healthi.ino`.

## Limitations

- A MAC-only login has no proof of ownership. Anyone who knows or guesses the MAC can request that device's data. Add proper Supabase Auth or a separate device secret before storing private health records.
- A one-second HTTPS interval is network-dependent and may temporarily pause sensor processing.
- Calories, sleep quality, mood patterns and fall detection are estimates, not medical measurements.
