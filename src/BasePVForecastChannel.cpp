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
            if (ko.value(DPT_Trigger))
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
    logInfoP("Fetching PV forecast data (channel %d)", _channelIndex);
    int16_t count = fillForecast(_hourlyForecast, PVF_MAX_HOURLY_SLOTS);
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

    time_t now = time(nullptr);

    // Today/tomorrow day boundaries
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    tmNow.tm_hour = 0; tmNow.tm_min = 0; tmNow.tm_sec = 0;
    time_t todayStart    = mktime(&tmNow);
    time_t tomorrowStart = todayStart + 86400;
    time_t dayAfterStart = tomorrowStart + 86400;

    // Reset derived values
    _today    = {};
    _tomorrow = {};
    _powerNow_W       = 0.0f;
    _powerNextHour_W  = 0.0f;

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

        // Current hour power
        if (ts <= now && now < ts + 3600)
            _powerNow_W = pwr;

        // Next hour power
        if (ts > now && ts <= now + 3600 && _powerNextHour_W == 0.0f)
            _powerNextHour_W = pwr;
    }
}

void BasePVForecastChannel::publishKos()
{
    if (!_available)
        return;

    // Today yield (kWh) - DPT 9.x (2-byte float)
    KoPVF_CHYieldToday.value(_today.yieldTotal_kWh, DPT_Value_Power);

    // Tomorrow yield (kWh)
    KoPVF_CHYieldTomorrow.value(_tomorrow.yieldTotal_kWh, DPT_Value_Power);

    // Current hour power (W)
    KoPVF_CHPowerNow.value(_powerNow_W, DPT_Value_Power);

    // Next hour power (W)
    KoPVF_CHPowerNextHour.value(_powerNextHour_W, DPT_Value_Power);

    // Peak power today (W)
    KoPVF_CHPeakPowerToday.value(_today.peakPower_W, DPT_Value_Power);

    // Peak time today - DPT 10.001 (time of day)
    if (_today.peakTime != 0)
    {
        struct tm tmPeak;
        localtime_r(&_today.peakTime, &tmPeak);
        uint32_t t = ((uint32_t)tmPeak.tm_hour << 16) | ((uint32_t)tmPeak.tm_min << 8) | tmPeak.tm_sec;
        KoPVF_CHPeakTimeToday.value(t, DPT_TimeOfDay);
    }
}
