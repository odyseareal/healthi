/*
  JOYSTICK FITNESS + WELLNESS TRACKER
  Arduino UNO R4 WiFi + GY-87 + SSD1306 OLED + joystick + HC-SR04

  JOYSTICK CONTROLS (rotated for sideways mounting)
    Physical UP = screen LEFT; RIGHT = screen UP;
    physical DOWN = screen RIGHT; LEFT = screen DOWN.
    Click selects/saves/starts/stops; hold 0.9 s goes back.
    Fitness is one main-menu dropdown. For an exercise, use LEFT/RIGHT
    to choose sets and reps, then click to confirm each choice.

  I2C WIRING (all share the same I2C bus)
    UNO R4       OLED SSD1306      GY-87
    5V           VCC               VCC
    GND          GND               GND
    SDA          SDA               SDA
    SCL          SCL               SCL

  JOYSTICK WIRING (SunFounder lesson pinout)
    UNO R4       JOYSTICK
    5V           5V/VCC
    GND          GND
    A0           VRX
    A1           VRY
    D8           SW

  HC-SR04 WIRING (SunFounder lesson pinout)
    UNO R4       HC-SR04
    5V           VCC
    GND          GND
    D4           TRIG
    D3           ECHO

  LIBRARIES TO INSTALL IN ARDUINO IDE
    Adafruit SSD1306
    Adafruit GFX Library
    Adafruit MPU6050
    Adafruit Unified Sensor
    Adafruit BMP085 Library          (also supports BMP180)
    QMC5883LCompass

  ACCURACY NOTES
    - Put the GY-87 securely in a trouser pocket for step counting.
    - Calibrate STEP_LENGTH_M by walking a known distance and dividing it by
      the number of steps. This improves distance and pace more than using a
      height-only estimate.
    - A barometric flight is 3 m of accumulated ascent. Pressure changes and
      doors/air-conditioning can affect the result.
    - Exercise calories are estimates. The code uses age/sex/mass-specific
      Schofield BMR for users aged 10-18, then MET values; adults use the
      standard MET calorie equation. Direct calorimetry or a validated heart-
      rate wearable would be more accurate.
    - The ultrasonic sensor needs a broad, unobstructed target. During its
      12-second calibration: hold the start pose for 2 seconds, then perform
      three slow, complete repetitions.
      Push-ups: point it at the centre of the chest. Squats: point it at the
      hips/torso. Pull-ups: use a flat target beneath the body if the sensor
      cannot obtain a consistent reflection from shoes or legs.
    - Mood is a self-report check-in, not an objective diagnosis. Sleep quality
      and fall detection are prototype estimates and must not replace medical
      assessment or a certified emergency device.
*/

#include "Healthi.h"

// ---------------- Arduino setup / loop ----------------
void setup() {
  Serial.begin(115200);
  pinMode(JOYSTICK_SW_PIN,INPUT_PULLUP);
  pinMode(ULTRASONIC_ECHO_PIN,INPUT);
  pinMode(ULTRASONIC_TRIG_PIN,OUTPUT); digitalWrite(ULTRASONIC_TRIG_PIN,LOW);
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC,OLED_ADDRESS)) {
    Serial.println(F("SSD1306 OLED not found at 0x3C"));
    while (true) delay(100);
  }
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  display.setCursor(0,16); display.println(F("Fitness tracker"));
  display.println(F("Starting sensors...")); display.display();
  if (!mpu.begin()) showFatalError(F("MPU6050 not found"));
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  mpu.setI2CBypass(true);
  compass.init();
  if (USE_COMPASS_CALIBRATION) {
    compass.setCalibrationOffsets(MAG_OFFSET_X,MAG_OFFSET_Y,MAG_OFFSET_Z);
    compass.setCalibrationScales(MAG_SCALE_X,MAG_SCALE_Y,MAG_SCALE_Z);
  }
  bmpAvailable=bmp.begin();
  calibrateGyroscope();
  previousRollDeg=rollDeg; previousPitchDeg=pitchDeg;
  lastSampleUs=micros(); lastLoopMs=millis();
  Serial.println(F("Fitness tracker ready. Hold joystick click for menu."));
}

void loop() {
  uint32_t now=millis(), dtMs=now-lastLoopMs; lastLoopMs=now;
  InputEvents input=readInput(); handleInput(input);
  updateMotion(); updateSleepTracker(); updateFallDetector();
  updateStepTracker(); updateBarometer(); updateSportTracker();
  updateTimeAndCalories(dtMs); updateDisplay(); printSerialData();
  delay(12);
}
