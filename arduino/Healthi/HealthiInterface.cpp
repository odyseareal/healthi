#include "Healthi.h"

// ---------------- Menu / control ----------------
uint8_t pageCountForMode(AppMode mode) {
  if (mode==MODE_STEPS) return 2;
  if (mode==MODE_MOTION) return 3;
  if (isSportMode(mode)) return 2;
  return 1;
}
void resetAllData() {
  stepCount=0; lastStepMs=0; stepTimeCount=stepTimeHead=0; cadenceSpm=0.0f;
  stepCalories=0.0f; stepActiveMs=0; stepNoiseEma=0.25f;
  dynamicStepThreshold=1.25f; ascentM=0.0f; climbAnchorM=filteredAltitudeM;
  for (uint8_t i=0;i<3;i++) {
    sportReps[i]=0; sportCalories[i]=0.0f; sportActiveMs[i]=0;
    sportLastRepMs[i]=0; sportRepRate[i]=0.0f; sportSensitivity[i]=0;
  }
  workoutPhase=WORKOUT_SETUP_SETS; currentWorkoutSet=1; currentWorkoutRep=0;
  workoutActiveMs=completedWorkoutMs=0;
  workoutCalories=completedWorkoutCalories=0.0f;
  moodCount=0; moodHead=0; moodSelection=3;
  sleepTracking=false; sleepStartMs=0; lastSleepDurationMs=0;
  sleepMovementCount=0; lastSleepMovementMs=0; lastSleepScore=0;
  fallState=FALL_NORMAL; fallAlertActive=false;
}
void enterMode(AppMode mode) {
  if (mode==MODE_RESET) { resetAllData(); return; }
  currentMode=mode; uiState=UI_MODE; pageIndex=0; paused=false;
  if (isSportMode(mode)) {
    workoutPhase=WORKOUT_SETUP_SETS;
    currentWorkoutSet=1; currentWorkoutRep=0;
    sportCalibrating=false; sportCalibrationFailed=false;
  }
}
void handleInput(const InputEvents& input) {
  if (fallAlertActive) {
    if (input.click || input.longPress) fallAlertActive=false;
    return;
  }
  if (uiState==UI_MENU) {
    if (input.y<0) menuIndex=(menuIndex+ROOT_COUNT-1)%ROOT_COUNT;
    if (input.y>0) menuIndex=(menuIndex+1)%ROOT_COUNT;
    if (input.click) {
      if (menuIndex==ROOT_FITNESS) uiState=UI_FITNESS_MENU;
      else if (menuIndex==ROOT_MOOD) enterMode(MODE_MOOD);
      else if (menuIndex==ROOT_SLEEP) enterMode(MODE_SLEEP);
      else if (menuIndex==ROOT_MOTION) enterMode(MODE_MOTION);
      else resetAllData();
    }
    return;
  }
  if (uiState==UI_FITNESS_MENU) {
    if (input.y<0) fitnessMenuIndex=(fitnessMenuIndex+4)%5;
    if (input.y>0) fitnessMenuIndex=(fitnessMenuIndex+1)%5;
    if (input.longPress || (input.click && fitnessMenuIndex==4)) {
      uiState=UI_MENU;
    } else if (input.click) {
      enterMode(static_cast<AppMode>(fitnessMenuIndex));
    }
    return;
  }
  if (input.longPress) {
    uiState=isFitnessMode(currentMode)?UI_FITNESS_MENU:UI_MENU;
    paused=false; return;
  }

  if (currentMode==MODE_MOOD) {
    if (input.x<0 && moodSelection>1) moodSelection--;
    if (input.x>0 && moodSelection<5) moodSelection++;
    if (input.click) saveMoodCheckIn();
    return;
  }
  if (currentMode==MODE_SLEEP) {
    if (input.click) {
      if (sleepTracking) stopSleepSession();
      else startSleepSession();
    }
    return;
  }

  if (isSportMode(currentMode)) {
    uint8_t index=sportIndex(currentMode);
    if (workoutPhase==WORKOUT_SETUP_SETS) {
      if (input.x<0 && plannedSets[index]>1) plannedSets[index]--;
      if (input.x>0 && plannedSets[index]<10) plannedSets[index]++;
      if (input.click) workoutPhase=WORKOUT_SETUP_REPS;
      return;
    }
    if (workoutPhase==WORKOUT_SETUP_REPS) {
      if (input.x<0 && plannedReps[index]>1) plannedReps[index]--;
      if (input.x>0 && plannedReps[index]<50) plannedReps[index]++;
      if (input.click) startPlannedWorkout();
      return;
    }
    if (workoutPhase==WORKOUT_SET_COMPLETE) {
      if (input.click) {
        currentWorkoutSet++; currentWorkoutRep=0;
        sportArmed=false; lowThresholdHoldCount=highThresholdHoldCount=0;
        workoutPhase=WORKOUT_ACTIVE; paused=false;
      }
      return;
    }
    if (workoutPhase==WORKOUT_COMPLETE) {
      if (input.click) workoutPhase=WORKOUT_SETUP_SETS;
      return;
    }
    if (sportCalibrationFailed) {
      if (input.click) beginSportCalibration();
      return;
    }
  }

  uint8_t pages=pageCountForMode(currentMode);
  if (input.x<0) pageIndex=(pageIndex+pages-1)%pages;
  if (input.x>0) pageIndex=(pageIndex+1)%pages;
  if (isSportMode(currentMode) && input.y!=0) {
    if (pageIndex==1 && workoutPhase==WORKOUT_ACTIVE &&
        !sportCalibrating && !sportCalibrationFailed) {
      uint8_t index=sportIndex(currentMode);
      int newSensitivity=sportSensitivity[index]+(input.y<0?1:-1);
      if (newSensitivity<-2) newSensitivity=-2;
      if (newSensitivity>2) newSensitivity=2;
      sportSensitivity[index]=newSensitivity;
      applySportThresholds();
      sportArmed=false;
    }
  }
  if (input.click) {
    if (!isSportMode(currentMode) || workoutPhase==WORKOUT_ACTIVE) paused=!paused;
  }
}

// ---------------- Display pages ----------------
void drawStatusHeader(const char* title) {
  display.setTextSize(1); display.setCursor(0,0); display.print(title);
  if (paused) display.print(F(" PAUSED"));
}
void drawMenu() {
  display.setTextSize(1); display.setCursor(0,0); display.print(F("WELLNESS TRACKER"));
  if (sleepTracking) { display.setCursor(113,0); display.print(F("Zz")); }
  display.drawLine(0,9,127,9,SSD1306_WHITE);
  int first=menuIndex<4?0:menuIndex-3;
  for (int row=0;row<4 && first+row<ROOT_COUNT;row++) {
    int item=first+row; display.setCursor(0,15+row*12);
    display.print(item==menuIndex?F("> "):F("  ")); display.print(ROOT_MENU_NAMES[item]);
  }
}
void drawFitnessMenu() {
  display.setTextSize(1); display.setCursor(0,0); display.print(F("FITNESS"));
  display.drawLine(0,9,127,9,SSD1306_WHITE);
  int first=fitnessMenuIndex<4?0:fitnessMenuIndex-3;
  for (int row=0;row<4 && first+row<5;row++) {
    int item=first+row; display.setCursor(0,15+row*12);
    display.print(item==fitnessMenuIndex?F("> "):F("  "));
    display.print(FITNESS_MENU_NAMES[item]);
  }
}
void drawStepsPage0() {
  drawStatusHeader("STEPS");
  display.setTextSize(2); display.setCursor(0,13); display.print(stepCount);
  display.setTextSize(1); display.setCursor(0,35);
  float distanceKm=stepCount*effectiveStepLengthM()/1000.0f;
  display.print(F("Dist: ")); display.print(distanceKm,2); display.println(F(" km"));
  display.print(F("Cad : ")); display.print(cadenceSpm,0); display.println(F(" steps/min"));
  float speedMPerMin=cadenceSpm*effectiveStepLengthM();
  display.print(F("Pace: "));
  if (speedMPerMin>=10.0f) {
    float pace=1000.0f/speedMPerMin; int mins=static_cast<int>(pace);
    int secs=static_cast<int>((pace-mins)*60.0f);
    display.print(mins); display.print(':'); if (secs<10) display.print('0');
    display.print(secs); display.print(F(" /km"));
  } else display.print(F("--:-- /km"));
}
void drawStepsPage1() {
  drawStatusHeader("CLIMB + ENERGY"); display.setCursor(0,15);
  display.print(F("Flights : "));
  if (bmpAvailable) display.println(static_cast<uint32_t>(ascentM/METRES_PER_FLIGHT));
  else display.println(F("N/A"));
  display.print(F("Ascent  : "));
  if (bmpAvailable) { display.print(ascentM,1); display.println(F(" m")); }
  else display.println(F("BMP180 missing"));
  display.print(F("Calories: ")); display.print(stepCalories,1); display.println(F(" kcal"));
  display.print(F("Time    : ")); printDuration(stepActiveMs); display.println();
  display.setCursor(0,56); display.print(F("Click pause Hold=menu"));
}
void drawSportPage() {
  drawStatusHeader(MODE_NAMES[currentMode]);
  uint8_t index=sportIndex(currentMode);
  if (workoutPhase==WORKOUT_SETUP_SETS) {
    display.setCursor(0,13); display.println(F("PLAN WORKOUT"));
    display.println(F("Choose number of sets"));
    display.setTextSize(3); display.setCursor(48,30); display.print(plannedSets[index]);
    display.setTextSize(1); display.setCursor(0,56); display.print(F("L/R change  Click next"));
    return;
  }
  if (workoutPhase==WORKOUT_SETUP_REPS) {
    display.setCursor(0,13); display.println(F("PLAN WORKOUT"));
    display.println(F("Reps in every set"));
    display.setTextSize(3); display.setCursor(42,30); display.print(plannedReps[index]);
    display.setTextSize(1); display.setCursor(0,56); display.print(F("L/R change Click start"));
    return;
  }
  if (sportCalibrating) {
    uint32_t elapsed=millis()-calibrationStartMs;
    uint8_t left=elapsed>=SPORT_CALIBRATION_TIME_MS?0:
      (SPORT_CALIBRATION_TIME_MS-elapsed+999UL)/1000UL;
    display.setCursor(0,15); display.println(F("CALIBRATING RANGE"));
    if (elapsed<SPORT_CALIBRATION_SETTLE_MS) display.println(F("Hold start position"));
    else display.println(F("Do 3 slow full reps"));
    display.print(F("Time left: ")); display.print(left); display.println(F(" s"));
    display.print(F("Range: "));
    display.print(calibrationMaxCm>calibrationMinCm?calibrationMaxCm-calibrationMinCm:0,0);
    display.println(F(" cm"));
    display.print(F("Good samples: ")); display.print(calibrationSampleCount);
    return;
  }
  if (sportCalibrationFailed) {
    display.setCursor(0,15); display.println(F("CALIBRATION FAILED"));
    display.println(F("Aim at flat target"));
    display.print(F("Need movement: "));
    display.print(SPORT_TUNING[sportIndex(currentMode)].minimumMovementCm,0);
    display.println(F(" cm"));
    display.println(F("Click to try again")); return;
  }
  if (workoutPhase==WORKOUT_SET_COMPLETE) {
    display.setCursor(0,14); display.print(F("SET ")); display.print(currentWorkoutSet);
    display.print('/'); display.print(plannedSets[index]); display.println(F(" COMPLETE"));
    display.setTextSize(2); display.setCursor(18,27); display.print(currentWorkoutRep);
    display.print(F(" reps"));
    display.setTextSize(1); display.setCursor(0,48); display.println(F("Rest timer excluded"));
    display.print(F("Click for next set"));
    return;
  }
  if (workoutPhase==WORKOUT_COMPLETE) {
    display.setCursor(0,13); display.println(F("WORKOUT COMPLETE!"));
    display.print(F("Plan: ")); display.print(plannedSets[index]); display.print(F(" x "));
    display.print(plannedReps[index]); display.println(F(" reps"));
    display.print(F("Active: ")); printDuration(completedWorkoutMs); display.println();
    display.print(F("Energy: ")); display.print(completedWorkoutCalories,1);
    display.println(F(" kcal"));
    display.setCursor(0,56); display.print(F("Click = new workout"));
    return;
  }
  display.setCursor(0,12); display.print(F("Set ")); display.print(currentWorkoutSet);
  display.print('/'); display.print(plannedSets[index]);
  display.setTextSize(2); display.setCursor(0,23);
  display.print(currentWorkoutRep); display.print('/'); display.print(plannedReps[index]);
  display.print(F(" reps"));
  display.setTextSize(1); display.setCursor(0,43);
  display.print(F("Sensor: "));
  if (distanceValid) { display.print(distanceCm,0); display.println(F(" cm")); }
  else display.println(F("no echo"));
  display.print(F("Workout: ")); display.print(workoutCalories,1); display.print(F(" kcal "));
  printDuration(workoutActiveMs);
}
void drawSportSensorPage() {
  drawStatusHeader("ULTRASONIC TUNE");
  display.setCursor(0,14);
  display.print(F("Now : "));
  if (distanceValid) { display.print(distanceCm,1); display.println(F(" cm")); }
  else display.println(F("NO ECHO"));
  display.print(F("Low : ")); display.print(sportLowThresholdCm,1); display.println(F(" cm"));
  display.print(F("High: ")); display.print(sportHighThresholdCm,1); display.println(F(" cm"));
  display.print(F("Next: "));
  if (currentMode==MODE_PULLUPS)
    display.println(sportArmed?F("lower fully"):F("pull upward"));
  else
    display.println(sportArmed?F("return to top"):F("move downward"));
  display.print(F("Sensitivity: "));
  int8_t sensitivity=sportSensitivity[sportIndex(currentMode)];
  if (sensitivity>0) display.print('+');
  display.println(sensitivity);
  display.setCursor(0,56); display.print(F("U/D=tune  L/R=page"));
}
void drawMoodFace(uint8_t mood) {
  constexpr int cx=104,cy=29,r=18;
  display.drawCircle(cx,cy,r,SSD1306_WHITE);
  display.fillCircle(cx-6,cy-5,2,SSD1306_WHITE);
  display.fillCircle(cx+6,cy-5,2,SSD1306_WHITE);
  if (mood>=4) {
    display.drawLine(cx-8,cy+4,cx-3,cy+9,SSD1306_WHITE);
    display.drawLine(cx-3,cy+9,cx+3,cy+9,SSD1306_WHITE);
    display.drawLine(cx+3,cy+9,cx+8,cy+4,SSD1306_WHITE);
  } else if (mood==3) {
    display.drawLine(cx-8,cy+7,cx+8,cy+7,SSD1306_WHITE);
  } else {
    display.drawLine(cx-8,cy+10,cx-3,cy+5,SSD1306_WHITE);
    display.drawLine(cx-3,cy+5,cx+3,cy+5,SSD1306_WHITE);
    display.drawLine(cx+3,cy+5,cx+8,cy+10,SSD1306_WHITE);
  }
}
void drawMoodPage() {
  drawStatusHeader("MOOD CHECK-IN");
  display.setTextSize(2); display.setCursor(0,15);
  display.print(moodSelection); display.print(F("/5"));
  display.setTextSize(1); display.setCursor(0,37);
  display.println(moodLabel(moodSelection));
  drawMoodFace(moodSelection);
  display.setCursor(0,48);
  display.print(F("Avg: "));
  if (moodCount>0) display.print(averageMood(),1); else display.print(F("--"));
  display.print(F("  Well: "));
  int score=wellbeingScore();
  if (score>=0) { display.print(score); display.print('%'); }
  else display.print(F("--"));
  display.setCursor(0,56); display.print(F("L/R choose Click save"));
}
void drawSleepPage() {
  if (sleepTracking) {
    drawStatusHeader("SLEEP TRACKING   Zz");
    display.setTextSize(2); display.setCursor(0,15);
    printHoursMinutes(millis()-sleepStartMs);
    display.setTextSize(1); display.setCursor(0,39);
    display.print(F("Movement events: ")); display.println(sleepMovementCount);
    display.println(F("Wear sensor securely"));
    display.setCursor(0,56); display.print(F("Click when awake"));
    return;
  }
  drawStatusHeader("SLEEP REVIEW");
  if (lastSleepDurationMs==0) {
    display.setCursor(0,17); display.setTextSize(2); display.println(F("Ready?"));
    display.setTextSize(1); display.println(F("Wear tracker securely"));
    display.setCursor(0,56); display.print(F("Click to start sleep"));
    return;
  }
  display.setCursor(0,14); display.print(F("Duration: ")); printHoursMinutes(lastSleepDurationMs);
  display.setTextSize(2); display.setCursor(0,27); display.print(lastSleepScore); display.print('%');
  display.setTextSize(1); display.setCursor(55,31);
  if (lastSleepScore>=85) display.print(F("Excellent"));
  else if (lastSleepScore>=70) display.print(F("Good"));
  else if (lastSleepScore>=50) display.print(F("Fair"));
  else display.print(F("Low"));
  display.setCursor(0,47); display.print(F("Movements: ")); display.print(sleepMovementCount);
  display.setCursor(0,56); display.print(F("Click new session"));
}
void drawFallAlert() {
  if ((millis()/400UL)%2==0) {
    display.drawRect(0,0,128,64,SSD1306_WHITE);
    display.drawRect(2,2,124,60,SSD1306_WHITE);
  }
  display.setTextSize(2); display.setCursor(31,8); display.println(F("FALL?"));
  display.setTextSize(1); display.setCursor(15,31);
  display.println(F("Impact + stillness"));
  display.setCursor(18,45); display.println(F("Check for injury"));
  display.setCursor(20,56); display.print(F("Click: I am OK"));
}
void drawHeadingPage() {
  drawStatusHeader("HEADING (MAG)");
  display.setTextSize(2); display.setCursor(0,17); display.print(headingDeg,0); display.print((char)247);
  display.setCursor(0,40); display.print(compassPoint(headingDeg));
  constexpr int cx=101,cy=35,radius=25;
  display.drawCircle(cx,cy,radius,SSD1306_WHITE);
  display.setTextSize(1); display.setCursor(cx-3,12); display.print(F("N"));
  float angle=headingDeg*DEG_TO_RAD;
  int ex=cx+static_cast<int>(17.0f*sinf(angle));
  int ey=cy-static_cast<int>(17.0f*cosf(angle));
  display.drawLine(cx,cy,ex,ey,SSD1306_WHITE);
  display.fillCircle(ex,ey,2,SSD1306_WHITE); display.fillCircle(cx,cy,2,SSD1306_WHITE);
}
void drawOrientationPage() {
  drawStatusHeader("ORIENTATION / ROTATION"); display.setCursor(0,15);
  display.print(F("Roll : ")); display.print(rollDeg,1); display.println(F(" deg"));
  display.print(F("Pitch: ")); display.print(pitchDeg,1); display.println(F(" deg"));
  display.print(F("|w|  : ")); display.print(omegaMagnitude,2); display.println(F(" rad/s"));
  display.print(F("Spin : ")); display.print(dominantAxis(omegaX,omegaY,omegaZ,0.04f));
}
void drawMomentumPage() {
  drawStatusHeader("ESTIMATED MOMENTUM"); display.setCursor(0,15);
  display.print(F("|p|: ")); display.print(momentumMagnitude,4); display.println(F(" kgm/s"));
  display.print(F("p dir: ")); display.println(dominantAxis(momentumX,momentumY,momentumZ,0.001f));
  display.print(F("|L|: ")); display.print(angularMomentumMagnitude,5); display.println(F(" kgm2/s"));
  display.print(F("L dir: "));
  display.println(dominantAxis(angularMomentumX,angularMomentumY,angularMomentumZ,0.00001f));
}
void updateDisplay() {
  if (millis()-lastDisplayMs<100UL) return;
  lastDisplayMs=millis(); display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  if (fallAlertActive) drawFallAlert();
  else if (uiState==UI_MENU) drawMenu();
  else if (uiState==UI_FITNESS_MENU) drawFitnessMenu();
  else if (currentMode==MODE_STEPS) { if (pageIndex==0) drawStepsPage0(); else drawStepsPage1(); }
  else if (isSportMode(currentMode)) {
    if (sportCalibrating || sportCalibrationFailed || pageIndex==0) drawSportPage();
    else drawSportSensorPage();
  }
  else if (currentMode==MODE_MOOD) drawMoodPage();
  else if (currentMode==MODE_SLEEP) drawSleepPage();
  else if (currentMode==MODE_MOTION) {
    if (pageIndex==0) drawHeadingPage();
    else if (pageIndex==1) drawOrientationPage();
    else drawMomentumPage();
  }
  display.display();
}
