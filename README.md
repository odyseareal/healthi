# Healthi

Healthi is an Arduino-connected fitness, nutrition and wellbeing tracker.

- `arduino/Healthi/Healthi.ino` — the single Arduino entry point.
- `arduino/Healthi/*.cpp` and `arduino/Healthi/*.h` — modular sensor, wellbeing, interface, state and telemetry code.
- `standalone/index.html` — a single-file dashboard using browser storage and Web Serial.
- `app/` — the hosted dashboard with persistent D1 records and an Arduino ingestion API.

## Hardware

- Arduino UNO R4 WiFi
- GY-87 module: MPU6050, BMP180 and QMC5883L
- SSD1306 128×64 I2C OLED
- Analogue joystick with push switch
- HC-SR04 ultrasonic sensor

The wiring and required Arduino libraries are listed at the top of the `.ino` file.

## Upload the firmware

1. Install the required libraries listed in the sketch.
2. Open `arduino/Healthi/Healthi.ino` in Arduino IDE. Keep every file in the `Healthi` folder together.
3. Select **Arduino UNO R4 WiFi** and the correct USB port.
4. Upload the sketch and leave the USB cable connected.

The sketch outputs one newline-delimited JSON packet each second at **115200 baud**.

## Run the standalone app

Open `standalone/index.html`, or serve it locally:

```bash
python3 -m http.server 8080 --directory standalone
```

Open `http://localhost:8080` in Chrome or Edge and select **Connect Arduino**. Food, mood, workouts and sensor history are saved in browser storage. Export backups regularly if the records are important.

## GitHub Pages

The included workflow publishes the standalone app automatically. In repository settings, choose **GitHub Actions** as the Pages source. Web Serial requires Chrome or Edge and a secure page or localhost.

## Hosted dashboard

The hosted application uses D1 for persistent food, mood, activity and workout records. Its API accepts tracker JSON at `POST /api/arduino`.

## Limitations

Calorie use, sleep quality, mood patterns and fall detection are estimates, not medical measurements. The ultrasonic sensor must be calibrated for the exercise position and target surface.
