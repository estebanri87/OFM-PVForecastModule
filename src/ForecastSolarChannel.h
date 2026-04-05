#pragma once
#include "BasePVForecastChannel.h"

class ForecastSolarChannel : public BasePVForecastChannel
{
  public:
ForecastSolarChannel(uint8_t index) : BasePVForecastChannel(index) {}
    const std::string name() override { return "ForecastSolar"; }

  protected:
    int16_t fillForecast(PVForecastHourlyData* slots, uint8_t maxCount) override;
};
