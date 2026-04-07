#pragma once
#include "BasePVForecastChannel.h"

class SolcastChannel : public BasePVForecastChannel
{
  public:
    SolcastChannel(uint8_t index) : BasePVForecastChannel(index) {}
    const std::string name() override { return "Solcast"; }

  protected:
    int16_t fillForecast(PVForecastHourlyData* slots, uint8_t maxCount) override;
};
