#include "ForecastSolarChannel.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// forecast.solar API – no API key required for basic use.
// Endpoint: GET https://api.forecast.solar/estimate/:lat/:lon/:dec/:az/:kwp
//   lat  - latitude  (degrees, float)
//   lon  - longitude (degrees, float)
//   dec  - plane declination 0..90 (tilt from horizontal, int)
//   az   - azimuth -180..180 (0=South, -90=East, 90=West, int) — converted from compass internally
//   kwp  - installed peak power (kWp, float)
// Response contains "result": { "watts": { "YYYY-MM-DD HH:MM:SS": <W>, ... }, ... }

int16_t ForecastSolarChannel::fillForecast(PVForecastHourlyData* slots, uint8_t maxCount)
{
    // Read ETS parameters
    // LocationType: false = use device location from Common module, true = use channel-specific location
    float lat, lon;
    if (ParamPVF_CHLocationType)
    {
        // Channel-specific location ("Anderer Ort")
        lat = ParamPVF_CHLatitude;
        lon = ParamPVF_CHLongitude;
    }
    else
    {
        // Device location from "Allgemein" (Common module)
        lat = ParamBASE_Latitude;
        lon = ParamBASE_Longitude;
    }
    int16_t dec = (int16_t)ParamPVF_CHTilt;   // degrees 0..90
    // Compass (0=N,90=E,180=S,270=W) → forecast.solar (0=S,-90=E,90=W): subtract 180
    int16_t az  = (int16_t)ParamPVF_CHAzimuth - 180;
    float kwp   = (float)ParamPVF_CHPeakPower / 100.0f;  // stored as kWp * 100

    char url[128];
    snprintf(url, sizeof(url),
             "https://api.forecast.solar/estimate/%.4f/%.4f/%d/%d/%.2f",
             lat, lon, dec, az, kwp);
    logDebugP("forecast.solar URL: %s", url);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(8000);  // 8s: well under 16s watchdog window
#ifdef ARDUINO_ARCH_RP2040
    if (String(url).startsWith("https://"))
        http.setInsecure();
#endif
    openknx.watchdog.loop();  // pet immediately before blocking http.GET()
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        logErrorP("forecast.solar HTTP %d", httpCode);
        http.end();
        return -1;
    }

    // Buffer full response before parsing — getStream() can fail on chunked HTTPS
    String responseBody = http.getString();
    http.end();

    openknx.watchdog.loop();  // pet after data received, before JSON parse
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, responseBody);

    if (err)
    {
        logErrorP("forecast.solar JSON parse error: %s", err.c_str());
        return -1;
    }

    // The "watts" object maps "YYYY-MM-DD HH:MM:SS" → instantaneous power in W
    JsonObject wattsObj = doc["result"]["watts"].as<JsonObject>();
    uint8_t count = 0;

    for (JsonPair kv : wattsObj)
    {
        if (count >= maxCount)
            break;

        // Parse timestamp string "YYYY-MM-DD HH:MM:SS" to time_t
        const char* tsStr = kv.key().c_str();
        struct tm t = {};
        // sscanf is safe here: input is API-controlled format
        if (sscanf(tsStr, "%4d-%2d-%2d %2d:%2d:%2d",
                   &t.tm_year, &t.tm_mon, &t.tm_mday,
                   &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
            continue;
        t.tm_year -= 1900;
        t.tm_mon  -= 1;
        t.tm_isdst = -1;

        slots[count].startTimestamp = mktime(&t);
        slots[count].power_W        = kv.value().as<float>();
        count++;
    }

    logDebugP("forecast.solar: parsed %d slots", count);

    // Read accurate daily yields from watt_hours_day (avoids summing non-uniform slots)
    // Keys are "YYYY-MM-DD", values are Wh for that day.
    time_t now = time(nullptr);
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    char todayKey[11], tomorrowKey[11];
    strftime(todayKey,    sizeof(todayKey),    "%Y-%m-%d", &tmNow);
    tmNow.tm_mday++;
    mktime(&tmNow);  // normalise (handles month/year rollover)
    strftime(tomorrowKey, sizeof(tomorrowKey), "%Y-%m-%d", &tmNow);

    JsonObject whdObj = doc["result"]["watt_hours_day"].as<JsonObject>();
    if (whdObj[todayKey].is<float>())
        _todayYield_Wh = whdObj[todayKey].as<float>();
    if (whdObj[tomorrowKey].is<float>())
        _tomorrowYield_Wh = whdObj[tomorrowKey].as<float>();

    logDebugP("forecast.solar: today yield %.0fWh, tomorrow yield %.0fWh",
              _todayYield_Wh, _tomorrowYield_Wh);

    return (int16_t)count;
}
