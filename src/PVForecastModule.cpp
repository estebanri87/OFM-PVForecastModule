#include "PVForecastModule.h"
#include "ForecastSolarChannel.h"

OpenKNX::Channel* PVForecastModule::createChannel(uint8_t index)
{
    // ParamPVF_CHProviderType: 0=forecast.solar
    switch (ParamPVF_CHProviderType)
    {
        case 0:
        default:
            return new ForecastSolarChannel(index);
    }
}

PVForecastModule openknxPVForecastModule;
