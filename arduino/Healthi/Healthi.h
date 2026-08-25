#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>
#include <QMC5883LCompass.h>
#include <math.h>

// ---------------- Hardware ----------------
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int OLED_RESET = -1;
constexpr uint8_t OLED_ADDRESS = 0x3C;

constexpr uint8_t JOYSTICK_X_PIN = A0;
constexpr uint8_t JOYSTICK_Y_PIN = A1;
constexpr uint8_t JOYSTICK_SW_PIN = 8;
constexpr uint8_t ULTRASONIC_ECHO_PIN = 3;
constexpr uint8_t ULTRASONIC_TRIG_PIN = 4;

extern Adafruit_SSD1306 display;
extern Adafruit_MPU6050 mpu;
extern Adafruit_BMP085 bmp;
extern QMC5883LCompass compass;

// ---------------- User settings: change these ----------------
constexpr uint8_t USER_AGE_YEARS = 16;
constexpr bool USER_IS_MALE = true;
constexpr float USER_WEIGHT_KG = 65.0f;
constexpr float USER_HEIGHT_M = 1.75f;
constexpr float STEP_LENGTH_M = 0.0f;

constexpr float OBJECT_MASS_KG = 0.200f;
constexpr float OBJECT_LENGTH_X_M = 0.100f;
constexpr float OBJECT_WIDTH_Y_M = 0.060f;
constexpr float OBJECT_HEIGHT_Z_M = 0.030f;
constexpr float MAGNETIC_DECLINATION_DEG = 0.0f;

constexpr bool USE_COMPASS_CALIBRATION = false;
constexpr float MAG_OFFSET_X = 0.0f;
constexpr float MAG_OFFSET_Y = 0.0f;
constexpr float MAG_OFFSET_Z = 0.0f;
constexpr float MAG_SCALE_X = 1.0f;
constexpr float MAG_SCALE_Y = 1.0f;
constexpr float MAG_SCALE_Z = 1.0f;

constexpr bool INVERT_JOYSTICK_X = false;
constexpr bool INVERT_JOYSTICK_Y = false;
constexpr bool ROTATE_JOYSTICK_90_CCW = true;

// ---------------- Evidence-based constants ----------------
constexpr float PUSHUP_MET_ADULT = 3.0f;
constexpr float SQUAT_MET_ADULT = 5.0f;
constexpr float PULLUP_MET_ADULT = 7.5f;
constexpr float PUSHUP_MET_YOUTH_16_18 = 4.1f;

struct SportTuning {
  float minimumMovementCm;
  float lowTriggerFraction;
  float highTriggerFraction;
  uint16_t minimumRepTimeMs;
  float maximumDistanceCm;
};

constexpr SportTuning SPORT_TUNING[3] = {
  {10.0f, 0.38f, 0.62f, 550, 100.0f},
  {18.0f, 0.35f, 0.65f, 700, 200.0f},
  {12.0f, 0.38f, 0.62f, 700, 300.0f}
};

constexpr uint16_t ULTRASONIC_SAMPLE_INTERVAL_MS = 60;
constexpr uint16_t SPORT_CALIBRATION_SETTLE_MS = 2000;
constexpr uint16_t SPORT_CALIBRATION_TIME_MS = 12000;
constexpr uint8_t ULTRASONIC_FILTER_SIZE = 5;
constexpr uint8_t THRESHOLD_HOLD_SAMPLES = 2;
constexpr uint8_t MINIMUM_CALIBRATION_SAMPLES = 30;
constexpr uint8_t MAXIMUM_CALIBRATION_SAMPLES = 180;

constexpr float FALL_LOW_G_THRESHOLD = 4.0f;
constexpr float FALL_IMPACT_THRESHOLD = 24.5f;
constexpr float FALL_DIRECT_IMPACT_THRESHOLD = 29.0f;
constexpr float FALL_ORIENTATION_CHANGE_DEG = 45.0f;
constexpr uint16_t FALL_EVENT_WINDOW_MS = 1500;
constexpr uint16_t FALL_STILL_TIME_MS = 1800;
constexpr uint16_t FALL_CANCEL_WINDOW_MS = 6500;

constexpr float SLEEP_MOVEMENT_ACCEL_MS2 = 2.0f;
constexpr float SLEEP_MOVEMENT_GYRO_RADS = 0.80f;
constexpr uint16_t SLEEP_MOVEMENT_REFRACTORY_MS = 10000;

constexpr float METRES_PER_FLIGHT = 3.0f;
constexpr float GRAVITY = 9.80665f;
constexpr float COMPLEMENTARY_GYRO_WEIGHT = 0.98f;
constexpr float ACCELERATION_DEADBAND = 0.15f;
constexpr float VELOCITY_LEAK = 0.998f;
constexpr float I_X = (OBJECT_MASS_KG / 12.0f) *
  (OBJECT_WIDTH_Y_M * OBJECT_WIDTH_Y_M + OBJECT_HEIGHT_Z_M * OBJECT_HEIGHT_Z_M);
constexpr float I_Y = (OBJECT_MASS_KG / 12.0f) *
  (OBJECT_LENGTH_X_M * OBJECT_LENGTH_X_M + OBJECT_HEIGHT_Z_M * OBJECT_HEIGHT_Z_M);
constexpr float I_Z = (OBJECT_MASS_KG / 12.0f) *
  (OBJECT_LENGTH_X_M * OBJECT_LENGTH_X_M + OBJECT_WIDTH_Y_M * OBJECT_WIDTH_Y_M);

// ---------------- Shared types ----------------
enum AppMode : uint8_t {
  MODE_STEPS, MODE_PUSHUPS, MODE_SQUATS, MODE_PULLUPS,
  MODE_MOOD, MODE_SLEEP, MODE_MOTION, MODE_RESET, MODE_COUNT
};
enum UiState : uint8_t { UI_MENU, UI_FITNESS_MENU, UI_MODE };
enum RootMenuItem : uint8_t {
  ROOT_FITNESS, ROOT_MOOD, ROOT_SLEEP, ROOT_MOTION, ROOT_RESET, ROOT_COUNT
};
enum WorkoutPhase : uint8_t {
  WORKOUT_SETUP_SETS, WORKOUT_SETUP_REPS, WORKOUT_CALIBRATING,
  WORKOUT_ACTIVE, WORKOUT_SET_COMPLETE, WORKOUT_COMPLETE
};
enum FallState : uint8_t { FALL_NORMAL, FALL_LOW_G_SEEN, FALL_IMPACT_SEEN };
struct InputEvents { int8_t x; int8_t y; bool click; bool longPress; };

// ---------------- Shared state ----------------
extern const char* const ROOT_MENU_NAMES[ROOT_COUNT];
extern const char* const FITNESS_MENU_NAMES[5];
extern const char* const MODE_NAMES[MODE_COUNT];

extern UiState uiState;
extern AppMode currentMode;
extern uint8_t menuIndex, fitnessMenuIndex, pageIndex;
extern bool paused, joystickNeutral, buttonWasDown, longPressSent;
extern uint32_t buttonDownMs;

extern float gyroBiasX, gyroBiasY, gyroBiasZ;
extern float rollDeg, pitchDeg, headingDeg;
extern float omegaX, omegaY, omegaZ, omegaMagnitude;
extern float linearAx, linearAy, linearAz;
extern float velocityX, velocityY, velocityZ;
extern float momentumX, momentumY, momentumZ;
extern float angularMomentumX, angularMomentumY, angularMomentumZ;
extern float momentumMagnitude, angularMomentumMagnitude;
extern float rawAccelMagnitude, linearAccelMagnitude;
extern uint32_t lastSampleUs, stillSinceMs;

extern uint32_t stepCount, lastStepMs, stepTimes[8];
extern uint8_t stepTimeCount, stepTimeHead;
extern bool stepSignalHigh;
extern float cadenceSpm, stepNoiseEma, dynamicStepThreshold, stepCalories;
extern uint32_t stepActiveMs;

extern bool bmpAvailable;
extern float filteredAltitudeM, climbAnchorM, ascentM;
extern uint32_t lastBarometerMs;

extern uint32_t sportReps[3];
extern float sportCalories[3];
extern uint32_t sportActiveMs[3], sportLastRepMs[3];
extern float sportRepRate[3];
extern bool sportCalibrating, sportCalibrationFailed, sportArmed;
extern uint32_t calibrationStartMs;
extern float calibrationMinCm, calibrationMaxCm;
extern float calibrationSamples[MAXIMUM_CALIBRATION_SAMPLES];
extern uint8_t calibrationSampleCount;
extern float sportLowThresholdCm, sportHighThresholdCm, distanceCm;
extern bool distanceValid;
extern uint32_t lastUltrasonicMs;
extern float ultrasonicHistory[ULTRASONIC_FILTER_SIZE];
extern uint8_t ultrasonicHistoryCount, ultrasonicHistoryHead;
extern uint8_t lowThresholdHoldCount, highThresholdHoldCount;
extern int8_t sportSensitivity[3];
extern uint8_t plannedSets[3], plannedReps[3];
extern WorkoutPhase workoutPhase;
extern uint8_t currentWorkoutSet, currentWorkoutRep;
extern uint32_t workoutActiveMs, completedWorkoutMs;
extern float workoutCalories, completedWorkoutCalories;

extern uint8_t moodSelection, moodHistory[10], moodCount, moodHead;
extern bool sleepTracking;
extern uint32_t sleepStartMs, lastSleepDurationMs;
extern uint32_t sleepMovementCount, lastSleepMovementMs;
extern uint8_t lastSleepScore;
extern FallState fallState;
extern uint32_t fallStateStartMs, fallStillStartMs;
extern float fallReferenceRoll, fallReferencePitch;
extern float previousRollDeg, previousPitchDeg;
extern bool fallAlertActive;
extern uint32_t lastLoopMs, lastDisplayMs, lastSerialMs;

// ---------------- Core and input ----------------
float vectorMagnitude(float x, float y, float z);
float clampFloat(float value, float low, float high);
float applyDeadband(float value, float deadband);
float wrap360(float angle);
float effectiveStepLengthM();
const char* compassPoint(float heading);
const char* dominantAxis(float x, float y, float z, float minimumMagnitude);
uint8_t sportIndex(AppMode mode);
bool isSportMode(AppMode mode);
bool isFitnessMode(AppMode mode);
void printDuration(uint32_t elapsedMs);
void printHoursMinutes(uint32_t elapsedMs);
void showFatalError(const __FlashStringHelper* message);
float bmrKcalPerDayYouth();
float caloriesPerMinuteFromMet(float met);
float cadenceMet(float cadence);
float sportMet(AppMode mode);
InputEvents readInput();

// ---------------- Sensors and exercise ----------------
void calibrateGyroscope();
void updateMotion();
void recordStep(uint32_t now);
void updateStepTracker();
void updateBarometer();
void sortFloatArray(float values[], uint8_t count);
float readUltrasonicCm();
void beginSportCalibration();
void applySportThresholds();
void finishSportCalibration();
void startPlannedWorkout();
void recordSportRep();
void updateSportTracker();

// ---------------- Wellbeing ----------------
const char* moodLabel(float mood);
void saveMoodCheckIn();
float averageMood();
int wellbeingScore();
void startSleepSession();
void stopSleepSession();
void updateSleepTracker();
void beginFallImpact(uint32_t now);
void updateFallDetector();
void updateTimeAndCalories(uint32_t dtMs);

// ---------------- Interface ----------------
uint8_t pageCountForMode(AppMode mode);
void resetAllData();
void enterMode(AppMode mode);
void handleInput(const InputEvents& input);
void drawStatusHeader(const char* title);
void drawMenu();
void drawFitnessMenu();
void drawStepsPage0();
void drawStepsPage1();
void drawSportPage();
void drawSportSensorPage();
void drawMoodFace(uint8_t mood);
void drawMoodPage();
void drawSleepPage();
void drawFallAlert();
void drawHeadingPage();
void drawOrientationPage();
void drawMomentumPage();
void updateDisplay();

// ---------------- Telemetry ----------------
String buildTelemetryJson();
void printSerialData();
