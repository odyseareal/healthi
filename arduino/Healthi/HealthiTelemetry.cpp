#include "Healthi.h"

String buildTelemetryJson() {
  float totalCalories=stepCalories;
  for (uint8_t i=0;i<3;i++) totalCalories+=sportCalories[i];
  float distanceKm=stepCount*effectiveStepLengthM()/1000.0f;
  float speedMPerMin=cadenceSpm*effectiveStepLengthM();
  float paceSecondsPerKm=speedMPerMin>=10.0f?60000.0f/speedMPerMin:0.0f;
  uint32_t sleepMs=sleepTracking?millis()-sleepStartMs:lastSleepDurationMs;

  String json;
  json.reserve(850);
  json=F("{\"mode\":\""); json+=MODE_NAMES[currentMode];
  json+=F("\",\"steps\":"); json+=String(stepCount);
  json+=F(",\"distanceKm\":"); json+=String(distanceKm,3);
  json+=F(",\"cadence\":"); json+=String(cadenceSpm,1);
  json+=F(",\"paceSecondsPerKm\":"); json+=String(paceSecondsPerKm,0);
  json+=F(",\"flights\":"); json+=String(static_cast<uint32_t>(ascentM/METRES_PER_FLIGHT));
  json+=F(",\"ascentM\":"); json+=String(ascentM,1);
  json+=F(",\"activeCalories\":"); json+=String(totalCalories,2);
  json+=F(",\"pushupsTotal\":"); json+=String(sportReps[0]);
  json+=F(",\"squatsTotal\":"); json+=String(sportReps[1]);
  json+=F(",\"pullupsTotal\":"); json+=String(sportReps[2]);
  json+=F(",\"sleepMinutes\":"); json+=String(sleepMs/60000UL);
  json+=F(",\"sleepScore\":"); json+=String(lastSleepScore);
  json+=F(",\"moodAverage\":"); json+=String(averageMood(),1);
  json+=F(",\"fallAlert\":"); json+=(fallAlertActive?F("true"):F("false"));
  json+=F(",\"heading\":"); json+=String(headingDeg,1);

  if (isSportMode(currentMode)) {
    uint8_t index=sportIndex(currentMode);
    json+=F(",\"exercise\":\""); json+=MODE_NAMES[currentMode];
    json+=F("\",\"sets\":"); json+=String(plannedSets[index]);
    json+=F(",\"reps\":"); json+=String(plannedReps[index]);
    json+=F(",\"currentSet\":"); json+=String(currentWorkoutSet);
    json+=F(",\"currentRep\":"); json+=String(currentWorkoutRep);
    json+=F(",\"workoutCalories\":"); json+=String(workoutCalories,2);
    json+=F(",\"durationSeconds\":"); json+=String(workoutActiveMs/1000UL);
    json+=F(",\"workoutComplete\":");
    json+=(workoutPhase==WORKOUT_COMPLETE?F("true"):F("false"));
  }
  json+='}';
  return json;
}

void printSerialData() {
  // Healthi reads this newline-delimited JSON over USB at 115200 baud.
  if (millis()-lastSerialMs<1000UL) return;
  lastSerialMs=millis();
  Serial.println(buildTelemetryJson());
}
