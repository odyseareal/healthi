#include "Healthi.h"

// ---------------- Mood, sleep and fall tracking ----------------
const char* moodLabel(float mood) {
  if (mood<1.5f) return "Very low";
  if (mood<2.5f) return "Low";
  if (mood<3.5f) return "Okay";
  if (mood<4.5f) return "Good";
  return "Great";
}

void saveMoodCheckIn() {
  moodHistory[moodHead]=moodSelection;
  moodHead=(moodHead+1)%10;
  if (moodCount<10) moodCount++;
}

float averageMood() {
  if (moodCount==0) return 0.0f;
  uint16_t total=0;
  for (uint8_t i=0;i<moodCount;i++) total+=moodHistory[i];
  return static_cast<float>(total)/moodCount;
}

int wellbeingScore() {
  if (moodCount==0) return -1;
  float moodScore=(averageMood()-1.0f)*25.0f;
  if (lastSleepDurationMs==0) return static_cast<int>(roundf(moodScore));
  return static_cast<int>(roundf(0.75f*moodScore+0.25f*lastSleepScore));
}

void startSleepSession() {
  sleepTracking=true;
  sleepStartMs=millis();
  sleepMovementCount=0;
  lastSleepMovementMs=0;
}

void stopSleepSession() {
  if (!sleepTracking) return;
  lastSleepDurationMs=millis()-sleepStartMs;
  sleepTracking=false;
  float hours=lastSleepDurationMs/3600000.0f;
  float targetMinimum=USER_AGE_YEARS<=18?8.0f:7.0f;
  float targetMaximum=USER_AGE_YEARS<=18?10.0f:9.0f;
  float durationScore;
  if (hours<targetMinimum) durationScore=100.0f*hours/targetMinimum;
  else if (hours<=targetMaximum) durationScore=100.0f;
  else durationScore=clampFloat(100.0f-(hours-targetMaximum)*10.0f,50.0f,100.0f);
  float movementRate=sleepMovementCount/max(hours,0.25f);
  float movementPenalty=min(35.0f,movementRate*2.0f);
  lastSleepScore=static_cast<uint8_t>(clampFloat(durationScore-movementPenalty,0.0f,100.0f));
}

void updateSleepTracker() {
  if (!sleepTracking) return;
  bool movement=linearAccelMagnitude>SLEEP_MOVEMENT_ACCEL_MS2 ||
    omegaMagnitude>SLEEP_MOVEMENT_GYRO_RADS;
  if (movement && millis()-lastSleepMovementMs>=SLEEP_MOVEMENT_REFRACTORY_MS) {
    sleepMovementCount++;
    lastSleepMovementMs=millis();
  }
}

void beginFallImpact(uint32_t now) {
  fallState=FALL_IMPACT_SEEN;
  fallStateStartMs=now;
  fallStillStartMs=0;
}

void updateFallDetector() {
  if (fallAlertActive) return;
  uint32_t now=millis();
  bool still=linearAccelMagnitude<0.75f && omegaMagnitude<0.30f;

  if (fallState==FALL_NORMAL) {
    if (rawAccelMagnitude<FALL_LOW_G_THRESHOLD) {
      fallState=FALL_LOW_G_SEEN;
      fallStateStartMs=now;
      fallReferenceRoll=previousRollDeg;
      fallReferencePitch=previousPitchDeg;
    } else if (rawAccelMagnitude>FALL_DIRECT_IMPACT_THRESHOLD) {
      fallReferenceRoll=previousRollDeg;
      fallReferencePitch=previousPitchDeg;
      beginFallImpact(now);
    }
  } else if (fallState==FALL_LOW_G_SEEN) {
    if (rawAccelMagnitude>FALL_IMPACT_THRESHOLD && now-fallStateStartMs<=FALL_EVENT_WINDOW_MS)
      beginFallImpact(now);
    else if (now-fallStateStartMs>FALL_EVENT_WINDOW_MS)
      fallState=FALL_NORMAL;
  } else {
    float rollChange=fabsf(rollDeg-fallReferenceRoll);
    float pitchChange=fabsf(pitchDeg-fallReferencePitch);
    float orientationChange=sqrtf(rollChange*rollChange+pitchChange*pitchChange);
    if (still) {
      if (fallStillStartMs==0) fallStillStartMs=now;
      if (now-fallStillStartMs>=FALL_STILL_TIME_MS &&
          orientationChange>=FALL_ORIENTATION_CHANGE_DEG) {
        fallAlertActive=true;
        fallState=FALL_NORMAL;
      }
    } else fallStillStartMs=0;
    if (now-fallStateStartMs>FALL_CANCEL_WINDOW_MS) fallState=FALL_NORMAL;
  }

  previousRollDeg=rollDeg;
  previousPitchDeg=pitchDeg;
}

void updateTimeAndCalories(uint32_t dtMs) {
  if (uiState!=UI_MODE || paused || dtMs>250UL) return;
  float minutes=dtMs/60000.0f;
  if (currentMode==MODE_STEPS) {
    stepActiveMs+=dtMs;
    float met=cadenceMet(cadenceSpm);
    if (met>0.0f) stepCalories+=caloriesPerMinuteFromMet(met)*minutes;
  } else if (isSportMode(currentMode) && workoutPhase==WORKOUT_ACTIVE &&
             !sportCalibrating && !sportCalibrationFailed) {
    uint8_t index=sportIndex(currentMode);
    sportActiveMs[index]+=dtMs;
    float calories=caloriesPerMinuteFromMet(sportMet(currentMode))*minutes;
    sportCalories[index]+=calories;
    workoutCalories+=calories;
    workoutActiveMs+=dtMs;
  }
}
