#pragma once
#include "OpenKNX.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"

// Maximum number of hourly forecast slots stored (today + tomorrow = 48h)
#define PVF_MAX_HOURLY_SLOTS 48

struct PVForecastHourlyData
{
    time_t startTimestamp = 0;  // Unix timestamp (seconds), start of hour
    float  power_W = 0.0f;      // Expected PV power during this hour (W)
};

struct PVForecastDayData
{
    float yieldTotal_kWh = 0.0f;   // Total expected yield for the day (kWh)
    float peakPower_W = 0.0f;      // Peak power expected (W)
    time_t peakTime = 0;           // Timestamp of expected peak hour
};

class BasePVForecastChannel : public OpenKNX::Channel
{
protected:
    uint8_t  _channelIndex = 0;
    uint32_t _lastApiCall = 0;
    uint32_t _updateIntervalInMs = 0;
    bool     _available = false;

    // Raw hourly forecast data
    PVForecastHourlyData _hourlyForecast[PVF_MAX_HOURLY_SLOTS];
    uint8_t _numSlots = 0;

    // Derived day summaries
    PVForecastDayData _today;
    PVForecastDayData _tomorrow;

    // Current and next-hour values
    float _powerNow_W = 0.0f;
    float _powerNextHour_W = 0.0f;

    // Provider-specific HTTP fetch + JSON parse -> fills _hourlyForecast[]
    // Returns number of slots filled, or -1 on error
    virtual int16_t fillForecast(PVForecastHourlyData* slots, uint8_t maxSlots) = 0;

    // Recalculates day summaries and current/next-hour from raw slots
    void calculateDerivedValues();

    // Pushes all derived values to KNX group objects
    void publishKos();

public:
    BasePVForecastChannel(uint8_t index);

    void setup() override;
    void loop() override;
    void processInputKo(GroupObject& ko) override;

    bool processCommand(const std::string cmd, bool diagnoseKo);
    void fetchData();
};
