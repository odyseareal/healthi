#include "Healthi.h"

// ---------------- Sensor updates ----------------
void calibrateGyroscope() {
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  display.setCursor(0,8); display.println(F("Calibrating gyro"));
  display.println(F("Keep completely still")); display.println(F("for 2 seconds..."));
  display.display();
  constexpr int SAMPLE_COUNT=400;
  float sumX=0.0f,sumY=0.0f,sumZ=0.0f;
  for (int i=0;i<SAMPLE_COUNT;i++) {
    sensors_event_t a,g,temp; mpu.getEvent(&a,&g,&temp);
    sumX+=g.gyro.x; sumY+=g.gyro.y; sumZ+=g.gyro.z; delay(5);
  }
  gyroBiasX=sumX/SAMPLE_COUNT; gyroBiasY=sumY/SAMPLE_COUNT; gyroBiasZ=sumZ/SAMPLE_COUNT;
  sensors_event_t a,g,temp; mpu.getEvent(&a,&g,&temp);
  rollDeg=atan2f(a.acceleration.y,a.acceleration.z)*RAD_TO_DEG;
  pitchDeg=atan2f(-a.acceleration.x,sqrtf(a.acceleration.y*a.acceleration.y+
    a.acceleration.z*a.acceleration.z))*RAD_TO_DEG;
}

void updateMotion() {
  uint32_t nowUs=micros();
  float dt=(nowUs-lastSampleUs)/1000000.0f; lastSampleUs=nowUs;
  if (dt<=0.0f || dt>0.10f) dt=0.01f;
  sensors_event_t a,g,temp; mpu.getEvent(&a,&g,&temp);
  rawAccelMagnitude=vectorMagnitude(a.acceleration.x,a.acceleration.y,a.acceleration.z);
  omegaX=g.gyro.x-gyroBiasX; omegaY=g.gyro.y-gyroBiasY; omegaZ=g.gyro.z-gyroBiasZ;
  omegaMagnitude=vectorMagnitude(omegaX,omegaY,omegaZ);
  float accelRollDeg=atan2f(a.acceleration.y,a.acceleration.z)*RAD_TO_DEG;
  float accelPitchDeg=atan2f(-a.acceleration.x,sqrtf(a.acceleration.y*a.acceleration.y+
    a.acceleration.z*a.acceleration.z))*RAD_TO_DEG;
  rollDeg=COMPLEMENTARY_GYRO_WEIGHT*(rollDeg+omegaX*dt*RAD_TO_DEG)+
    (1.0f-COMPLEMENTARY_GYRO_WEIGHT)*accelRollDeg;
  pitchDeg=COMPLEMENTARY_GYRO_WEIGHT*(pitchDeg+omegaY*dt*RAD_TO_DEG)+
    (1.0f-COMPLEMENTARY_GYRO_WEIGHT)*accelPitchDeg;
  compass.read();
  headingDeg=wrap360(static_cast<float>(compass.getAzimuth())+MAGNETIC_DECLINATION_DEG);

  float rollRad=rollDeg*DEG_TO_RAD, pitchRad=pitchDeg*DEG_TO_RAD;
  float gravityX=-GRAVITY*sinf(pitchRad);
  float gravityY=GRAVITY*sinf(rollRad)*cosf(pitchRad);
  float gravityZ=GRAVITY*cosf(rollRad)*cosf(pitchRad);
  linearAx=applyDeadband(a.acceleration.x-gravityX,ACCELERATION_DEADBAND);
  linearAy=applyDeadband(a.acceleration.y-gravityY,ACCELERATION_DEADBAND);
  linearAz=applyDeadband(a.acceleration.z-gravityZ,ACCELERATION_DEADBAND);
  linearAccelMagnitude=vectorMagnitude(linearAx,linearAy,linearAz);
  velocityX=(velocityX+linearAx*dt)*VELOCITY_LEAK;
  velocityY=(velocityY+linearAy*dt)*VELOCITY_LEAK;
  velocityZ=(velocityZ+linearAz*dt)*VELOCITY_LEAK;
  bool appearsStill=linearAccelMagnitude<0.20f && omegaMagnitude<0.04f;
  if (appearsStill) {
    if (stillSinceMs==0) stillSinceMs=millis();
    if (millis()-stillSinceMs>=500UL) velocityX=velocityY=velocityZ=0.0f;
  } else stillSinceMs=0;
  momentumX=OBJECT_MASS_KG*velocityX; momentumY=OBJECT_MASS_KG*velocityY;
  momentumZ=OBJECT_MASS_KG*velocityZ;
  momentumMagnitude=vectorMagnitude(momentumX,momentumY,momentumZ);
  angularMomentumX=I_X*omegaX; angularMomentumY=I_Y*omegaY; angularMomentumZ=I_Z*omegaZ;
  angularMomentumMagnitude=vectorMagnitude(angularMomentumX,angularMomentumY,angularMomentumZ);
}

void recordStep(uint32_t now) {
  stepCount++; lastStepMs=now;
  stepTimes[stepTimeHead]=now; stepTimeHead=(stepTimeHead+1)%8;
  if (stepTimeCount<8) stepTimeCount++;
  if (stepTimeCount>=2) {
    uint8_t newest=(stepTimeHead+7)%8;
    uint8_t oldest=(stepTimeHead+8-stepTimeCount)%8;
    uint32_t period=stepTimes[newest]-stepTimes[oldest];
    if (period>0) cadenceSpm=60000.0f*(stepTimeCount-1)/period;
  }
}
void updateStepTracker() {
  if (uiState!=UI_MODE || currentMode!=MODE_STEPS || paused) return;
  uint32_t now=millis();
  float signal=fabsf(rawAccelMagnitude-GRAVITY);
  // Learn normal motion noise and keep the trigger above it. Real step peaks
  // are excluded from the noise model so the threshold does not chase them.
  if (!stepSignalHigh && signal<2.5f)
    stepNoiseEma=0.98f*stepNoiseEma+0.02f*signal;
  dynamicStepThreshold=clampFloat(0.90f+1.80f*stepNoiseEma,1.0f,2.2f);
  float releaseThreshold=dynamicStepThreshold*0.42f;
  if (!stepSignalHigh && signal>dynamicStepThreshold && now-lastStepMs>=280UL) {
    stepSignalHigh=true; recordStep(now);
  } else if (stepSignalHigh && signal<releaseThreshold) stepSignalHigh=false;
  if (now-lastStepMs>3000UL) cadenceSpm=0.0f;
}

void updateBarometer() {
  if (!bmpAvailable || millis()-lastBarometerMs<200UL) return;
  lastBarometerMs=millis();
  float altitude=bmp.readAltitude();
  if (!isfinite(altitude)) return;
  if (filteredAltitudeM==0.0f) {
    filteredAltitudeM=altitude; climbAnchorM=altitude; return;
  }
  filteredAltitudeM=0.90f*filteredAltitudeM+0.10f*altitude;
  // Like commercial trackers, only credit altitude gain while steps are
  // occurring; this rejects much of the slow pressure drift while stationary.
  bool recentlyWalking=millis()-lastStepMs<3000UL;
  if (uiState==UI_MODE && currentMode==MODE_STEPS && !paused && recentlyWalking) {
    if (filteredAltitudeM>climbAnchorM+0.35f) {
      ascentM+=filteredAltitudeM-climbAnchorM; climbAnchorM=filteredAltitudeM;
    } else if (filteredAltitudeM<climbAnchorM-0.80f) climbAnchorM=filteredAltitudeM;
  } else climbAnchorM=filteredAltitudeM;
}

void sortFloatArray(float values[], uint8_t count) {
  for (uint8_t i=1;i<count;i++) {
    float value=values[i];
    int j=i-1;
    while (j>=0 && values[j]>value) {
      values[j+1]=values[j];
      j--;
    }
    values[j+1]=value;
  }
}

float readUltrasonicCm() {
  digitalWrite(ULTRASONIC_TRIG_PIN,LOW); delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN,HIGH); delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN,LOW);
  unsigned long duration=pulseIn(ULTRASONIC_ECHO_PIN,HIGH,25000UL);
  if (duration==0) return -1.0f;
  float measured=duration/58.0f;
  const SportTuning& tune=SPORT_TUNING[sportIndex(currentMode)];
  if (measured<4.0f || measured>tune.maximumDistanceCm) return -1.0f;

  // Median filtering rejects isolated echoes from arms, clothing and nearby
  // objects without delaying a normal repetition excessively.
  ultrasonicHistory[ultrasonicHistoryHead]=measured;
  ultrasonicHistoryHead=(ultrasonicHistoryHead+1)%ULTRASONIC_FILTER_SIZE;
  if (ultrasonicHistoryCount<ULTRASONIC_FILTER_SIZE) ultrasonicHistoryCount++;
  if (ultrasonicHistoryCount<3) return -1.0f;

  float sorted[ULTRASONIC_FILTER_SIZE];
  for (uint8_t i=0;i<ultrasonicHistoryCount;i++) sorted[i]=ultrasonicHistory[i];
  sortFloatArray(sorted,ultrasonicHistoryCount);
  return sorted[ultrasonicHistoryCount/2];
}
void beginSportCalibration() {
  sportCalibrating=true; sportCalibrationFailed=false; sportArmed=false;
  workoutPhase=WORKOUT_CALIBRATING;
  calibrationStartMs=millis(); calibrationMinCm=999.0f; calibrationMaxCm=0.0f;
  calibrationSampleCount=0;
  distanceCm=0.0f; distanceValid=false;
  ultrasonicHistoryCount=0; ultrasonicHistoryHead=0;
  lowThresholdHoldCount=0; highThresholdHoldCount=0;
}
void applySportThresholds() {
  uint8_t index=sportIndex(currentMode);
  const SportTuning& tune=SPORT_TUNING[index];
  float range=calibrationMaxCm-calibrationMinCm;
  // Positive sensitivity narrows the gap, so a smaller movement counts.
  float adjustment=sportSensitivity[index]*0.035f;
  float lowFraction=clampFloat(tune.lowTriggerFraction+adjustment,0.20f,0.48f);
  float highFraction=clampFloat(tune.highTriggerFraction-adjustment,0.52f,0.80f);
  sportLowThresholdCm=calibrationMinCm+range*lowFraction;
  sportHighThresholdCm=calibrationMinCm+range*highFraction;
}
void finishSportCalibration() {
  sportCalibrating=false;
  if (calibrationSampleCount<MINIMUM_CALIBRATION_SAMPLES) {
    sportCalibrationFailed=true;
    return;
  }

  // Percentiles ignore a few false minimum/maximum echoes during movement.
  sortFloatArray(calibrationSamples,calibrationSampleCount);
  uint8_t lowIndex=calibrationSampleCount/10;
  uint8_t highIndex=(calibrationSampleCount*9)/10;
  calibrationMinCm=calibrationSamples[lowIndex];
  calibrationMaxCm=calibrationSamples[highIndex];
  float range=calibrationMaxCm-calibrationMinCm;
  const SportTuning& tune=SPORT_TUNING[sportIndex(currentMode)];
  if (range<tune.minimumMovementCm) { sportCalibrationFailed=true; return; }

  applySportThresholds();
  sportCalibrationFailed=false;
  workoutPhase=WORKOUT_ACTIVE;
  sportArmed=false;
  lowThresholdHoldCount=0; highThresholdHoldCount=0;
}
void startPlannedWorkout() {
  uint8_t index=sportIndex(currentMode);
  currentWorkoutSet=1; currentWorkoutRep=0;
  workoutActiveMs=0; completedWorkoutMs=0;
  workoutCalories=0.0f; completedWorkoutCalories=0.0f;
  sportLastRepMs[index]=0; sportRepRate[index]=0.0f;
  paused=false; pageIndex=0;
  beginSportCalibration();
}
void recordSportRep() {
  if (workoutPhase!=WORKOUT_ACTIVE) return;
  uint8_t index=sportIndex(currentMode); uint32_t now=millis();
  sportReps[index]++;
  currentWorkoutRep++;
  if (sportLastRepMs[index]>0 && now-sportLastRepMs[index]<10000UL) {
    float instantaneousRate=60000.0f/(now-sportLastRepMs[index]);
    sportRepRate[index]=sportRepRate[index]==0.0f?instantaneousRate:
      0.70f*sportRepRate[index]+0.30f*instantaneousRate;
  }
  sportLastRepMs[index]=now;
  if (currentWorkoutRep>=plannedReps[index]) {
    pageIndex=0; paused=false;
    if (currentWorkoutSet>=plannedSets[index]) {
      workoutPhase=WORKOUT_COMPLETE;
      completedWorkoutMs=workoutActiveMs;
      completedWorkoutCalories=workoutCalories;
    } else workoutPhase=WORKOUT_SET_COMPLETE;
  }
}
void updateSportTracker() {
  if (uiState!=UI_MODE || !isSportMode(currentMode) || paused) return;
  if (workoutPhase!=WORKOUT_CALIBRATING && workoutPhase!=WORKOUT_ACTIVE) return;
  if (millis()-lastUltrasonicMs<ULTRASONIC_SAMPLE_INTERVAL_MS) return;
  lastUltrasonicMs=millis();
  // End calibration even if the sensor never receives a valid echo.
  if (sportCalibrating && millis()-calibrationStartMs>=SPORT_CALIBRATION_TIME_MS) {
    finishSportCalibration();
    return;
  }
  float measured=readUltrasonicCm();
  distanceValid=measured>0.0f;
  if (!distanceValid) return;
  distanceCm=distanceCm<=0.0f?measured:0.40f*distanceCm+0.60f*measured;
  if (sportCalibrating) {
    // First two seconds establish a clean filtered reading in the start pose.
    if (millis()-calibrationStartMs>=SPORT_CALIBRATION_SETTLE_MS) {
      calibrationMinCm=min(calibrationMinCm,distanceCm);
      calibrationMaxCm=max(calibrationMaxCm,distanceCm);
      if (calibrationSampleCount<MAXIMUM_CALIBRATION_SAMPLES)
        calibrationSamples[calibrationSampleCount++]=distanceCm;
    }
    return;
  }
  if (sportCalibrationFailed) return;
  const SportTuning& tune=SPORT_TUNING[sportIndex(currentMode)];

  if (currentMode==MODE_PULLUPS) {
    // Pulling up increases the distance from a sensor placed below the body.
    if (!sportArmed) {
      highThresholdHoldCount=distanceCm>=sportHighThresholdCm?highThresholdHoldCount+1:0;
      if (highThresholdHoldCount>=THRESHOLD_HOLD_SAMPLES) {
        sportArmed=true; highThresholdHoldCount=0;
      }
    } else {
      lowThresholdHoldCount=distanceCm<=sportLowThresholdCm?lowThresholdHoldCount+1:0;
      uint8_t index=sportIndex(currentMode);
      if (lowThresholdHoldCount>=THRESHOLD_HOLD_SAMPLES &&
          millis()-sportLastRepMs[index]>=tune.minimumRepTimeMs) {
        recordSportRep(); sportArmed=false; lowThresholdHoldCount=0;
      }
    }
  } else {
    // Lowering a push-up/squat decreases distance; count only after returning.
    if (!sportArmed) {
      lowThresholdHoldCount=distanceCm<=sportLowThresholdCm?lowThresholdHoldCount+1:0;
      if (lowThresholdHoldCount>=THRESHOLD_HOLD_SAMPLES) {
        sportArmed=true; lowThresholdHoldCount=0;
      }
    } else {
      highThresholdHoldCount=distanceCm>=sportHighThresholdCm?highThresholdHoldCount+1:0;
      uint8_t index=sportIndex(currentMode);
      if (highThresholdHoldCount>=THRESHOLD_HOLD_SAMPLES &&
          millis()-sportLastRepMs[index]>=tune.minimumRepTimeMs) {
        recordSportRep(); sportArmed=false; highThresholdHoldCount=0;
      }
    }
  }
}
