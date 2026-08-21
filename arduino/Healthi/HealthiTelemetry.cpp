#include "Healthi.h"

void printSerialData() {
  // Healthi reads this newline-delimited JSON over USB at 115200 baud.
  // One packet per second is responsive without flooding the browser.
  if (millis()-lastSerialMs<1000UL) return;
  lastSerialMs=millis();
  float totalCalories=stepCalories;
  for (uint8_t i=0;i<3;i++) totalCalories+=sportCalories[i];
  float distanceKm=stepCount*effectiveStepLengthM()/1000.0f;
  float speedMPerMin=cadenceSpm*effectiveStepLengthM();
  float paceSecondsPerKm=speedMPerMin>=10.0f?60000.0f/speedMPerMin:0.0f;
  uint32_t sleepMs=sleepTracking?millis()-sleepStartMs:lastSleepDurationMs;

  Serial.print(F("{\"mode\":\"")); Serial.print(MODE_NAMES[currentMode]);
  Serial.print(F("\",\"steps\":")); Serial.print(stepCount);
  Serial.print(F(",\"distanceKm\":")); Serial.print(distanceKm,3);
  Serial.print(F(",\"cadence\":")); Serial.print(cadenceSpm,1);
  Serial.print(F(",\"paceSecondsPerKm\":")); Serial.print(paceSecondsPerKm,0);
  Serial.print(F(",\"flights\":")); Serial.print(static_cast<uint32_t>(ascentM/METRES_PER_FLIGHT));
  Serial.print(F(",\"ascentM\":")); Serial.print(ascentM,1);
  Serial.print(F(",\"activeCalories\":")); Serial.print(totalCalories,2);
  Serial.print(F(",\"pushupsTotal\":")); Serial.print(sportReps[0]);
  Serial.print(F(",\"squatsTotal\":")); Serial.print(sportReps[1]);
  Serial.print(F(",\"pullupsTotal\":")); Serial.print(sportReps[2]);
  Serial.print(F(",\"sleepMinutes\":")); Serial.print(sleepMs/60000UL);
  Serial.print(F(",\"sleepScore\":")); Serial.print(lastSleepScore);
  Serial.print(F(",\"moodAverage\":")); Serial.print(averageMood(),1);
  Serial.print(F(",\"fallAlert\":")); Serial.print(fallAlertActive?F("true"):F("false"));
  Serial.print(F(",\"heading\":")); Serial.print(headingDeg,1);

  if (isSportMode(currentMode)) {
    uint8_t index=sportIndex(currentMode);
    Serial.print(F(",\"exercise\":\"")); Serial.print(MODE_NAMES[currentMode]);
    Serial.print(F("\",\"sets\":")); Serial.print(plannedSets[index]);
    Serial.print(F(",\"reps\":")); Serial.print(plannedReps[index]);
    Serial.print(F(",\"currentSet\":")); Serial.print(currentWorkoutSet);
    Serial.print(F(",\"currentRep\":")); Serial.print(currentWorkoutRep);
    Serial.print(F(",\"workoutCalories\":")); Serial.print(workoutCalories,2);
    Serial.print(F(",\"durationSeconds\":")); Serial.print(workoutActiveMs/1000UL);
    Serial.print(F(",\"workoutComplete\":"));
    Serial.print(workoutPhase==WORKOUT_COMPLETE?F("true"):F("false"));
  }
  Serial.println(F("}"));
}
