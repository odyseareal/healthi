#include "Healthi.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;
Adafruit_BMP085 bmp;
QMC5883LCompass compass;

const char* const ROOT_MENU_NAMES[ROOT_COUNT] = {
  "Fitness >", "Mood", "Sleep", "Motion", "Reset all"
};

const char* const FITNESS_MENU_NAMES[5] = {
  "Steps", "Push-ups", "Squats", "Pull-ups", "< Back"
};

const char* const MODE_NAMES[MODE_COUNT] = {
  "Steps", "Push-ups", "Squats", "Pull-ups",
  "Mood", "Sleep", "Motion", "Reset all"
};

UiState uiState = UI_MENU;
AppMode currentMode = MODE_STEPS;
uint8_t menuIndex = 0;
uint8_t fitnessMenuIndex = 0;
uint8_t pageIndex = 0;
bool paused = false;

bool joystickNeutral = true;
bool buttonWasDown = false;
bool longPressSent = false;
uint32_t buttonDownMs = 0;

// ---------------- IMU / motion state ----------------
float gyroBiasX = 0.0f, gyroBiasY = 0.0f, gyroBiasZ = 0.0f;
float rollDeg = 0.0f, pitchDeg = 0.0f, headingDeg = 0.0f;
float omegaX = 0.0f, omegaY = 0.0f, omegaZ = 0.0f, omegaMagnitude = 0.0f;
float linearAx = 0.0f, linearAy = 0.0f, linearAz = 0.0f;
float velocityX = 0.0f, velocityY = 0.0f, velocityZ = 0.0f;
float momentumX = 0.0f, momentumY = 0.0f, momentumZ = 0.0f;
float angularMomentumX = 0.0f, angularMomentumY = 0.0f, angularMomentumZ = 0.0f;
float momentumMagnitude = 0.0f, angularMomentumMagnitude = 0.0f;
float rawAccelMagnitude = GRAVITY, linearAccelMagnitude = 0.0f;
uint32_t lastSampleUs = 0;
uint32_t stillSinceMs = 0;

// ---------------- Step tracker state ----------------
uint32_t stepCount = 0;
uint32_t lastStepMs = 0;
uint32_t stepTimes[8] = {0};
uint8_t stepTimeCount = 0, stepTimeHead = 0;
bool stepSignalHigh = false;
float cadenceSpm = 0.0f;
float stepNoiseEma = 0.25f;
float dynamicStepThreshold = 1.25f;
float stepCalories = 0.0f;
uint32_t stepActiveMs = 0;

// ---------------- Barometer / stairs ----------------
bool bmpAvailable = false;
float filteredAltitudeM = 0.0f, climbAnchorM = 0.0f, ascentM = 0.0f;
uint32_t lastBarometerMs = 0;

// ---------------- Sport tracker state ----------------
uint32_t sportReps[3] = {0, 0, 0};
float sportCalories[3] = {0.0f, 0.0f, 0.0f};
uint32_t sportActiveMs[3] = {0, 0, 0};
uint32_t sportLastRepMs[3] = {0, 0, 0};
float sportRepRate[3] = {0.0f, 0.0f, 0.0f};

bool sportCalibrating = false, sportCalibrationFailed = false, sportArmed = false;
uint32_t calibrationStartMs = 0;
float calibrationMinCm = 999.0f, calibrationMaxCm = 0.0f;
float calibrationSamples[MAXIMUM_CALIBRATION_SAMPLES];
uint8_t calibrationSampleCount = 0;
float sportLowThresholdCm = 0.0f, sportHighThresholdCm = 0.0f;
float distanceCm = 0.0f;
bool distanceValid = false;
uint32_t lastUltrasonicMs = 0;
float ultrasonicHistory[ULTRASONIC_FILTER_SIZE] = {0};
uint8_t ultrasonicHistoryCount = 0, ultrasonicHistoryHead = 0;
uint8_t lowThresholdHoldCount = 0, highThresholdHoldCount = 0;
int8_t sportSensitivity[3] = {0, 0, 0};
uint8_t plannedSets[3] = {3, 3, 3};
uint8_t plannedReps[3] = {10, 10, 10};
WorkoutPhase workoutPhase = WORKOUT_SETUP_SETS;
uint8_t currentWorkoutSet = 1, currentWorkoutRep = 0;
uint32_t workoutActiveMs = 0, completedWorkoutMs = 0;
float workoutCalories = 0.0f, completedWorkoutCalories = 0.0f;

// ---------------- Mood / wellbeing state ----------------
uint8_t moodSelection = 3;
uint8_t moodHistory[10] = {0};
uint8_t moodCount = 0, moodHead = 0;

// ---------------- Sleep state ----------------
bool sleepTracking = false;
uint32_t sleepStartMs = 0, lastSleepDurationMs = 0;
uint32_t sleepMovementCount = 0, lastSleepMovementMs = 0;
uint8_t lastSleepScore = 0;

// ---------------- Fall detector state ----------------
FallState fallState = FALL_NORMAL;
uint32_t fallStateStartMs = 0, fallStillStartMs = 0;
float fallReferenceRoll = 0.0f, fallReferencePitch = 0.0f;
float previousRollDeg = 0.0f, previousPitchDeg = 0.0f;
bool fallAlertActive = false;

uint32_t lastLoopMs = 0, lastDisplayMs = 0, lastSerialMs = 0;
