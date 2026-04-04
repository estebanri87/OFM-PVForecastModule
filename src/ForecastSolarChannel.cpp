#include "ForecastSolarChannel.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// forecast.solar API – no API key required for basic use.
// Endpoint: GET https://api.forecast.solar/estimate/:lat/:lon/:dec/:az/:kwp
//   lat  - latitude  (degrees, float)
//   lon  - longitude (degrees, float)
//   dec  - plane declination 0..90 (tilt from horizontal, int)
//   az   - azimuth -180..180 (0=South, -90=East, 90=West, int)
//   kwp  - installed peak power (kWp, float)
// Response contains "result": { "watts": { "YYYY-MM-DD HH:MM:SS": <W>, ... }, ... }

int16_t ForecastSolarChannel::fillForecast(PVForecastHourlyData* slots, uint8_t maxCount)
{
    // Read ETS parameters
    // ParamPVF_CHLatitude / ParamPVF_CHLongitude: stored as degrees * 100 (int16), so divide by 100.0
    float lat  = (float)ParamPVF_CHLatitude  / 100.0f;
    float lon  = (float)ParamPVF_CHLongitude / 100.0f;
    int16_t dec = ParamPVF_CHTilt;      // degrees 0..90
    int16_t az  = ParamPVF_CHAzimuth;   // degrees -180..180
    float kwp   = (float)ParamPVF_CHPeakPower / 100.0f;  // stored as kWp * 100

    char url[128];
    snprintf(url, sizeof(url),
             "https://api.forecast.solar/estimate/%.4f/%.4f/%d/%d/%.2f",
             lat, lon, dec, az, kwp);
    logDebugP("forecast.solar URL: %s", url);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(15000);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        logErrorP("forecast.solar HTTP %d", httpCode);
        http.end();
        return -1;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();

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
    return (int16_t)count;
}
