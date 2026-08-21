#include "Healthi.h"

// ---------------- General helpers ----------------
float vectorMagnitude(float x, float y, float z) { return sqrtf(x*x + y*y + z*z); }
float clampFloat(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}
float applyDeadband(float value, float deadband) { return fabsf(value) < deadband ? 0.0f : value; }
float wrap360(float angle) {
  while (angle < 0.0f) angle += 360.0f;
  while (angle >= 360.0f) angle -= 360.0f;
  return angle;
}
float effectiveStepLengthM() {
  return STEP_LENGTH_M > 0.20f ? STEP_LENGTH_M : USER_HEIGHT_M * 0.415f;
}
const char* compassPoint(float heading) {
  static const char* points[16] = {
    "N","NNE","NE","ENE","E","ESE","SE","SSE",
    "S","SSW","SW","WSW","W","WNW","NW","NNW"
  };
  return points[static_cast<int>((wrap360(heading)+11.25f)/22.5f)%16];
}
const char* dominantAxis(float x, float y, float z, float minimumMagnitude) {
  if (vectorMagnitude(x,y,z) < minimumMagnitude) return "--";
  float ax=fabsf(x), ay=fabsf(y), az=fabsf(z);
  if (ax>=ay && ax>=az) return x>=0.0f ? "+X" : "-X";
  if (ay>=ax && ay>=az) return y>=0.0f ? "+Y" : "-Y";
  return z>=0.0f ? "+Z" : "-Z";
}
uint8_t sportIndex(AppMode mode) { return static_cast<uint8_t>(mode)-static_cast<uint8_t>(MODE_PUSHUPS); }
bool isSportMode(AppMode mode) { return mode>=MODE_PUSHUPS && mode<=MODE_PULLUPS; }
bool isFitnessMode(AppMode mode) { return mode==MODE_STEPS || isSportMode(mode); }
void printDuration(uint32_t elapsedMs) {
  uint32_t totalSeconds=elapsedMs/1000UL;
  uint16_t minutes=totalSeconds/60UL;
  uint8_t seconds=totalSeconds%60UL;
  display.print(minutes); display.print(':');
  if (seconds<10) display.print('0');
  display.print(seconds);
}
void printHoursMinutes(uint32_t elapsedMs) {
  uint32_t totalMinutes=elapsedMs/60000UL;
  display.print(totalMinutes/60UL);
  display.print(F("h "));
  display.print(totalMinutes%60UL);
  display.print(F("m"));
}
void showFatalError(const __FlashStringHelper* message) {
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  display.setCursor(0,0); display.println(F("STARTUP ERROR")); display.println();
  display.println(message); display.display();
  while (true) delay(100);
}

// ---------------- Calorie calculation ----------------
float bmrKcalPerDayYouth() {
  if (USER_IS_MALE) return 17.686f*USER_WEIGHT_KG+658.2f;
  return 13.384f*USER_WEIGHT_KG+692.6f;
}
float caloriesPerMinuteFromMet(float met) {
  if (USER_AGE_YEARS>=10 && USER_AGE_YEARS<=18)
    return met*bmrKcalPerDayYouth()/1440.0f;
  return met*3.5f*USER_WEIGHT_KG/200.0f;
}
float cadenceMet(float cadence) {
  if (cadence<20.0f || millis()-lastStepMs>3000UL) return 0.0f;
  if (cadence<100.0f) return clampFloat(1.5f+(cadence-50.0f)*0.03f,1.5f,3.0f);
  return clampFloat(3.0f+(cadence-100.0f)/10.0f,3.0f,12.0f);
}
float sportMet(AppMode mode) {
  if (mode==MODE_PUSHUPS) {
    if (USER_AGE_YEARS>=16 && USER_AGE_YEARS<=18) return PUSHUP_MET_YOUTH_16_18;
    return PUSHUP_MET_ADULT;
  }
  if (mode==MODE_SQUATS) return SQUAT_MET_ADULT;
  return PULLUP_MET_ADULT;
}

// ---------------- Joystick input ----------------
InputEvents readInput() {
  InputEvents events={0,0,false,false};
  int xRaw=analogRead(JOYSTICK_X_PIN), yRaw=analogRead(JOYSTICK_Y_PIN);
  int8_t xDirection=xRaw<300?-1:(xRaw>700?1:0);
  int8_t yDirection=yRaw<300?-1:(yRaw>700?1:0);
  if (INVERT_JOYSTICK_X) xDirection=-xDirection;
  if (INVERT_JOYSTICK_Y) yDirection=-yDirection;
  if (xDirection==0 && yDirection==0) joystickNeutral=true;
  else if (joystickNeutral) {
    if (ROTATE_JOYSTICK_90_CCW) {
      events.x=yDirection;
      events.y=-xDirection;
    } else {
      events.x=xDirection;
      events.y=yDirection;
    }
    joystickNeutral=false;
  }

  bool buttonDown=digitalRead(JOYSTICK_SW_PIN)==LOW;
  uint32_t now=millis();
  if (buttonDown && !buttonWasDown) { buttonDownMs=now; longPressSent=false; }
  if (buttonDown && !longPressSent && now-buttonDownMs>=900UL) {
    events.longPress=true; longPressSent=true;
  }
  if (!buttonDown && buttonWasDown && !longPressSent && now-buttonDownMs>=30UL)
    events.click=true;
  buttonWasDown=buttonDown;
  return events;
}
