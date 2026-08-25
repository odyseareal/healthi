#pragma once

// Copy this file to arduino_secrets.h, then replace the placeholder values.
// arduino_secrets.h is ignored by Git so your Wi-Fi password is not published.
#define HEALTHI_CLOUD_ENABLED false
#define HEALTHI_WIFI_SSID "YOUR_WIFI_NAME"
#define HEALTHI_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define HEALTHI_SUPABASE_HOST "YOUR_PROJECT_REF.supabase.co"
#define HEALTHI_SUPABASE_PUBLISHABLE_KEY "sb_publishable_REPLACE_ME"

// One second gives near-live updates. Increase to 5000 if your network is slow.
#define HEALTHI_SYNC_INTERVAL_MS 1000UL
