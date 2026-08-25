#include "Healthi.h"
#include "HealthiNetwork.h"
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>

#if __has_include("arduino_secrets.h")
#include "arduino_secrets.h"
#else
#include "arduino_secrets.example.h"
#endif

namespace {
WiFiSSLClient tlsClient;
HttpClient cloudClient(tlsClient, HEALTHI_SUPABASE_HOST, 443);
String deviceId;
uint32_t lastCloudSyncMs = 0;
uint32_t lastReconnectAttemptMs = 0;
bool startupDataLoaded = false;

String makeDeviceId() {
  byte mac[6];
  WiFi.macAddress(mac);
  char text[18];
  snprintf(text, sizeof(text), "%02X:%02X:%02X:%02X:%02X:%02X",
    mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
  return String(text);
}

bool readNumber(const String& json, const char* key, float& value) {
  String needle = String("\"") + key + "\":";
  int start = json.indexOf(needle);
  if (start < 0) return false;
  start += needle.length();
  while (start < static_cast<int>(json.length()) && (json[start] == ' ' || json[start] == '\"')) start++;
  int finish = start;
  while (finish < static_cast<int>(json.length())) {
    char c = json[finish];
    if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') finish++;
    else break;
  }
  if (finish == start) return false;
  value = json.substring(start, finish).toFloat();
  return true;
}

bool readBool(const String& json, const char* key, bool& value) {
  String needle = String("\"") + key + "\":";
  int start = json.indexOf(needle);
  if (start < 0) return false;
  start += needle.length();
  while (start < static_cast<int>(json.length()) && json[start] == ' ') start++;
  if (json.substring(start, start + 4) == "true") { value = true; return true; }
  if (json.substring(start, start + 5) == "false") { value = false; return true; }
  return false;
}

void restoreMetrics(const String& response) {
  float value;
  if (readNumber(response, "steps", value)) stepCount = max(0.0f, value);
  if (readNumber(response, "ascentM", value)) ascentM = max(0.0f, value);
  if (readNumber(response, "activeCalories", value)) stepCalories = max(0.0f, value);
  if (readNumber(response, "pushupsTotal", value)) sportReps[0] = max(0.0f, value);
  if (readNumber(response, "squatsTotal", value)) sportReps[1] = max(0.0f, value);
  if (readNumber(response, "pullupsTotal", value)) sportReps[2] = max(0.0f, value);
  if (readNumber(response, "sleepMinutes", value)) lastSleepDurationMs = static_cast<uint32_t>(max(0.0f, value) * 60000.0f);
  if (readNumber(response, "sleepScore", value)) lastSleepScore = constrain(static_cast<int>(value), 0, 100);
  if (readNumber(response, "moodAverage", value) && value >= 1.0f) {
    moodSelection = constrain(static_cast<int>(value + 0.5f), 1, 5);
    moodHistory[0] = moodSelection;
    moodCount = 1;
    moodHead = 1;
  }
  bool alert;
  if (readBool(response, "fallAlert", alert)) fallAlertActive = alert;
}

bool requestStartupMetrics() {
  String path = "/rest/v1/healthi_devices?device_id=eq." + deviceId + "&select=metrics&limit=1";
  cloudClient.beginRequest();
  cloudClient.get(path);
  cloudClient.sendHeader("apikey", HEALTHI_SUPABASE_PUBLISHABLE_KEY);
  cloudClient.sendHeader("Authorization", String("Bearer ") + HEALTHI_SUPABASE_PUBLISHABLE_KEY);
  cloudClient.endRequest();

  int status = cloudClient.responseStatusCode();
  String response = cloudClient.responseBody();
  cloudClient.stop();
  if (status == 200 && response != "[]") {
    restoreMetrics(response);
    Serial.println(F("Healthi cloud: startup data restored."));
    return true;
  }
  if (status == 200) {
    Serial.println(F("Healthi cloud: new device; no saved data yet."));
    return true;
  }
  Serial.print(F("Healthi cloud GET failed: HTTP "));
  Serial.println(status);
  return false;
}

bool uploadMetrics() {
  String body;
  body.reserve(1150);
  body = F("{\"device_id\":\"");
  body += deviceId;
  body += F("\",\"metrics\":");
  body += buildTelemetryJson();
  body += F("}");

  cloudClient.beginRequest();
  cloudClient.post("/rest/v1/healthi_devices?on_conflict=device_id");
  cloudClient.sendHeader("apikey", HEALTHI_SUPABASE_PUBLISHABLE_KEY);
  cloudClient.sendHeader("Authorization", String("Bearer ") + HEALTHI_SUPABASE_PUBLISHABLE_KEY);
  cloudClient.sendHeader("Content-Type", "application/json");
  cloudClient.sendHeader("Prefer", "resolution=merge-duplicates,return=minimal");
  cloudClient.sendHeader("Content-Length", body.length());
  cloudClient.beginBody();
  cloudClient.print(body);
  cloudClient.endRequest();

  int status = cloudClient.responseStatusCode();
  cloudClient.responseBody(); // Consume the response so the next request can run.
  cloudClient.stop();
  if (status >= 200 && status < 300) return true;
  Serial.print(F("Healthi cloud POST failed: HTTP "));
  Serial.println(status);
  return false;
}
}

void beginHealthiCloud() {
#if HEALTHI_CLOUD_ENABLED
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println(F("Healthi cloud: WiFi module not found."));
    return;
  }
  Serial.print(F("Healthi cloud: connecting to WiFi"));
  WiFi.begin(HEALTHI_WIFI_SSID, HEALTHI_WIFI_PASSWORD);
  uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 15000UL) {
    delay(300);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Healthi cloud: connection timed out; tracker will retry."));
    return;
  }
  deviceId = makeDeviceId();
  Serial.print(F("Healthi device ID: "));
  Serial.println(deviceId);
  startupDataLoaded = requestStartupMetrics();
#else
  Serial.println(F("Healthi cloud disabled. Copy arduino_secrets.example.h to arduino_secrets.h and configure it."));
#endif
}

void updateHealthiCloud() {
#if HEALTHI_CLOUD_ENABLED
  uint32_t now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastReconnectAttemptMs < 10000UL) return;
    lastReconnectAttemptMs = now;
    WiFi.begin(HEALTHI_WIFI_SSID, HEALTHI_WIFI_PASSWORD);
    return;
  }
  if (deviceId.length() == 0) deviceId = makeDeviceId();
  if (!startupDataLoaded) startupDataLoaded = requestStartupMetrics();
  if (now - lastCloudSyncMs < HEALTHI_SYNC_INTERVAL_MS) return;
  lastCloudSyncMs = now;
  uploadMetrics();
#endif
}

const String& healthiDeviceId() { return deviceId; }
bool healthiCloudConnected() { return WiFi.status() == WL_CONNECTED; }
