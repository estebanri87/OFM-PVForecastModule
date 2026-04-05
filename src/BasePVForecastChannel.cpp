#include "BasePVForecastChannel.h"
#ifdef WLAN_WifiSSID
    #include "WiFi.h"
#else
    #include "NetworkModule.h"
#endif

BasePVForecastChannel::BasePVForecastChannel(uint8_t index)
{
    _channelIndex = index;
}

void BasePVForecastChannel::setup()
{
    // <Enumeration Text="Keine"       Value="0" Id="%ENID%" />
    // <Enumeration Text="30 Minuten"  Value="1" Id="%ENID%" />
    // <Enumeration Text="Jede Stunde" Value="2" Id="%ENID%" />
    // <Enumeration Text="Täglich"     Value="3" Id="%ENID%" />
    switch (ParamPVF_CHRefreshInterval)
    {
        case 1:
            _updateIntervalInMs = 30 * 60 * 1000;
            break;
        case 2:
            _updateIntervalInMs = 60 * 60 * 1000;
            break;
        case 3:
            _updateIntervalInMs = 24 * 60 * 60 * 1000;
            break;
        default:
            _updateIntervalInMs = 0;
            break;
    }
    logDebugP("Update interval: %ldms", _updateIntervalInMs);
}

void BasePVForecastChannel::loop()
{
#ifdef WLAN_WifiSSID
    if (WiFi.isConnected())
#else
    if (openknxNetwork.established())
#endif
    {
        auto now = millis();
        if (now == 0)
            now++;  // 0 is used as "uninitialized" marker

        if (_updateIntervalInMs > 0 &&
            now >= 60000 &&  // wait 60s after boot for NTP sync
            (_lastApiCall == 0 || (now - _lastApiCall > _updateIntervalInMs)))
        {
            _lastApiCall = now;
            fetchData();
        }
    }
}

void BasePVForecastChannel::processInputKo(GroupObject& ko)
{
    switch (ko.asap())
    {
        case PVF_KoRefreshData:
            if (time(nullptr) >= 1577836800LL)  // only fetch after NTP is synced
                fetchData();
            break;
    }
}

bool BasePVForecastChannel::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd == "update")
    {
        fetchData();
        return true;
    }
    return false;
}

void BasePVForecastChannel::fetchData()
{
    openknx.watchdog.loop();  // pet before blocking HTTP call (16s watchdog)
    logInfoP("Fetching PV forecast data (channel %d)", _channelIndex);
    _todayYield_Wh    = 0.0f;
    _tomorrowYield_Wh = 0.0f;
    int16_t count = fillForecast(_hourlyForecast, PVF_MAX_HOURLY_SLOTS);
    openknx.watchdog.loop();  // pet after HTTP call returned
    if (count < 0)
    {
        logErrorP("Failed to fetch PV forecast (channel %d)", _channelIndex);
        _available = false;
        return;
    }
    _numSlots = (uint8_t)count;
    _available = true;
    logInfoP("Received %d hourly forecast slots", _numSlots);
    calculateDerivedValues();
    publishKos();
}

void BasePVForecastChannel::calculateDerivedValues()
{
    if (_numSlots == 0)
        return;

    // Sanity check: require NTP time to be synced (any time after 2020-01-01)
    time_t now = time(nullptr);
    if (now < 1577836800LL)  // 2020-01-01 00:00 UTC
    {
        logWarningP("System time not synced (time=%ld), skipping PV calculation", (long)now);
        return;
    }

    // Today/tomorrow day boundaries
    // Use mktime(mday+N) instead of +86400 to handle DST transitions correctly.
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    tmNow.tm_hour = 0; tmNow.tm_min = 0; tmNow.tm_sec = 0;
    time_t todayStart = mktime(&tmNow);
    tmNow.tm_mday++;
    time_t tomorrowStart = mktime(&tmNow);
    tmNow.tm_mday++;
    time_t dayAfterStart = mktime(&tmNow);

    // Reset derived values
    _today    = {};
    _tomorrow = {};
    _powerNow_W       = 0.0f;
    _powerNextHour_W  = 0.0f;

    bool nextHourFound = false;

    for (uint8_t i = 0; i < _numSlots; i++)
    {
        time_t ts  = _hourlyForecast[i].startTimestamp;
        float  pwr = _hourlyForecast[i].power_W;

        // Accumulate today
        if (ts >= todayStart && ts < tomorrowStart)
        {
            _today.yieldTotal_kWh += pwr / 1000.0f;  // Wh/slot -> kWh (1h slots)
            if (pwr > _today.peakPower_W)
            {
                _today.peakPower_W = pwr;
                _today.peakTime    = ts;
            }
        }
        // Accumulate tomorrow
        else if (ts >= tomorrowStart && ts < dayAfterStart)
        {
            _tomorrow.yieldTotal_kWh += pwr / 1000.0f;
            if (pwr > _tomorrow.peakPower_W)
            {
                _tomorrow.peakPower_W = pwr;
                _tomorrow.peakTime    = ts;
            }
        }

        // Current hour power: slot whose period [ts, ts+3600) contains now
        if (ts <= now && now < ts + 3600)
            _powerNow_W = pwr;

        // Next hour power: first slot that starts strictly after now
        if (!nextHourFound && ts > now)
        {
            _powerNextHour_W = pwr;
            nextHourFound    = true;
        }
    }
}

void BasePVForecastChannel::publishKos()
{
    if (!_available)
        return;

    // Today yield (kWh) - DPST-13-13 (DPT 13.013, 4-byte signed int, unit kWh)
    // Prefer accurate watt_hours_day value from API; fall back to summed watts.
    int32_t todayKwh    = (int32_t)((_todayYield_Wh    > 0.0f) ? _todayYield_Wh    / 1000.0f : _today.yieldTotal_kWh);
    int32_t tomorrowKwh = (int32_t)((_tomorrowYield_Wh > 0.0f) ? _tomorrowYield_Wh / 1000.0f : _tomorrow.yieldTotal_kWh);
    KoPVF_CHYieldToday.value(todayKwh, DPT_ActiveEnergy_kWh);

    // Tomorrow yield (kWh)
    KoPVF_CHYieldTomorrow.value(tomorrowKwh, DPT_ActiveEnergy_kWh);

    // Power values in W (DPST-14-56 = DPT 14.056 "Leistung", 4-byte IEEE 754)
    KoPVF_CHPowerNow.value(_powerNow_W, DPT_Value_Power);
    KoPVF_CHPowerNextHour.value(_powerNextHour_W, DPT_Value_Power);
    KoPVF_CHPeakPowerToday.value(_today.peakPower_W, DPT_Value_Power);

    // Peak time today - DPT 10.001 (time of day)
    if (_today.peakTime != 0)
    {
        struct tm tmPeak;
        localtime_r(&_today.peakTime, &tmPeak);
        KoPVF_CHPeakTimeToday.value(tmPeak, DPT_TimeOfDay);
    }
}
