#include "SolcastChannel.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Solcast Rooftop PV Power Forecast API
// Free tier: 10 API calls/day per site (registration at solcast.com required).
// Endpoint: GET https://api.solcast.com.au/data/forecast/rooftop_pv_power
//   latitude  - degrees N (float)
//   longitude - degrees E (float)
//   capacity  - installed peak power in kWp (float)
//   tilt      - panel tilt 0=horizontal, 90=vertical (int)
//   azimuth   - compass bearing: 0=North, 90=East, 180=South, 270=West (int)
//   hours     - forecast horizon in hours (max 336 = 14 days; we use 48)
//   format    - "json"
//   api_key   - user's Solcast API key
//
// Note: ETS parameter uses compass convention (0=North, 90=East, 180=South, 270=West),
//       which matches Solcast directly. No conversion needed.
//
// Response: {"forecasts": [{"pv_estimate": kW, "period_end": "ISO8601Z", "period": "PT30M"}, ...]}
// pv_estimate is in kW (not W). Periods are 30 minutes.
// Consecutive 30-min slots are aggregated to hourly slots for the base class.

int16_t SolcastChannel::fillForecast(PVForecastHourlyData* slots, uint8_t maxCount)
{
    // Read location parameters
    float lat, lon;
    if (ParamPVF_CHLocationType)
    {
        lat = ParamPVF_CHLatitude;
        lon = ParamPVF_CHLongitude;
    }
    else
    {
        lat = ParamBASE_Latitude;
        lon = ParamBASE_Longitude;
    }
    int16_t dec = (int16_t)ParamPVF_CHTilt;
    float kwp   = (float)ParamPVF_CHPeakPower / 100.0f;
    // ETS parameter is already compass convention (0=North, 90=East, 180=South, 270=West)
    int16_t solcastAz = (int16_t)ParamPVF_CHAzimuth;

    const char* apiKey = ParamPVF_CHSolcastApiKeyStr.c_str();
    const char* siteId = ParamPVF_CHSolcastSiteIdStr.c_str();

    // Build URL:
    // - If Site Resource ID is set → use registered rooftop site endpoint (free hobbyist plan)
    // - Otherwise → use generic rooftop_pv_power endpoint (requires lat/lon/capacity/tilt/azimuth)
    char url[384];
    if (siteId != nullptr && siteId[0] != '\0')
    {
        snprintf(url, sizeof(url),
                 "https://api.solcast.com.au/rooftop_sites/%s/forecasts"
                 "?format=json&hours=48&api_key=%s",
                 siteId, apiKey);
        logDebugP("Solcast URL: rooftop_sites/%s/forecasts (api_key hidden)", siteId);
    }
    else
    {
        snprintf(url, sizeof(url),
                 "https://api.solcast.com.au/data/forecast/rooftop_pv_power"
                 "?latitude=%.4f&longitude=%.4f&capacity=%.2f"
                 "&tilt=%d&azimuth=%d&hours=48&format=json&api_key=%s",
                 lat, lon, kwp, dec, solcastAz, apiKey);
        logDebugP("Solcast URL: rooftop_pv_power lat=%.4f lon=%.4f cap=%.2f tilt=%d az=%d (api_key hidden)",
                  lat, lon, kwp, dec, solcastAz);
    }

    HTTPClient http;
    http.begin(url);
    http.setTimeout(8000);
#ifdef ARDUINO_ARCH_RP2040
    if (String(url).startsWith("https://"))
        http.setInsecure();
#endif
    openknx.watchdog.loop();
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        logErrorP("Solcast HTTP %d", httpCode);
        http.end();
        return -1;
    }

    String responseBody = http.getString();
    http.end();

    openknx.watchdog.loop();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, responseBody);
    if (err)
    {
        logErrorP("Solcast JSON parse error: %s", err.c_str());
        return -1;
    }

    JsonArray forecasts = doc["forecasts"].as<JsonArray>();
    if (forecasts.isNull())
    {
        logErrorP("Solcast: missing 'forecasts' array in response");
        return -1;
    }

    // Pre-compute the UTC-to-local offset once (Solcast returns UTC timestamps).
    // mktime() treats struct tm as local time. To convert a UTC struct tm to correct
    // time_t: utcTs = mktime(utc_struct) + utcOffset
    // where utcOffset = time(nullptr) - mktime(gmtime(time(nullptr))) [seconds east of UTC]
    time_t now = time(nullptr);
    struct tm tmUtcNow;
    gmtime_r(&now, &tmUtcNow);
    time_t utcOffset = now - mktime(&tmUtcNow);

    // Aggregate consecutive 30-min slots into hourly slots.
    // Solcast periods are always aligned to :00 and :30 minute boundaries.
    // Slot starting at :00 = first half-hour; :30 = second half-hour of that hour.
    uint8_t count = 0;
    float   firstHalfPower_W = 0.0f;
    bool    haveFirstHalf    = false;
    time_t  firstHalfStart   = 0;

    for (JsonObject slot : forecasts)
    {
        if (count >= maxCount)
            break;

        float power_W = (slot["pv_estimate"] | 0.0f) * 1000.0f;  // kW → W

        // Parse period_end: "2026-04-05T12:30:00.0000000Z" (UTC)
        const char* periodEnd = slot["period_end"] | "";
        struct tm t = {};
        if (sscanf(periodEnd, "%4d-%2d-%2dT%2d:%2d:%2d",
                   &t.tm_year, &t.tm_mon, &t.tm_mday,
                   &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
            continue;
        t.tm_year -= 1900;
        t.tm_mon  -= 1;
        t.tm_isdst = 0;

        // Convert UTC struct → correct time_t
        time_t periodEndTs = mktime(&t) + utcOffset;
        time_t slotStart   = periodEndTs - 30 * 60;  // period is 30 min

        // Determine half-hour by local minute
        struct tm tmSlot;
        localtime_r(&slotStart, &tmSlot);

        if (tmSlot.tm_min == 0)
        {
            // First half-hour of the hour (covers :00 – :30)
            firstHalfPower_W = power_W;
            firstHalfStart   = slotStart;
            haveFirstHalf    = true;
        }
        else if (tmSlot.tm_min == 30 && haveFirstHalf)
        {
            // Second half-hour (covers :30 – :00): average → one hourly slot at :00
            slots[count].startTimestamp = firstHalfStart;
            slots[count].power_W        = (firstHalfPower_W + power_W) / 2.0f;
            count++;
            haveFirstHalf = false;
        }
        // Slots starting at other minutes (e.g. partial first slot) are skipped
    }

    logDebugP("Solcast: aggregated %d hourly slots from 30-min data", count);
    return (int16_t)count;
}
