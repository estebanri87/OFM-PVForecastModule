#include "PVForecastModule.h"
#include "ForecastSolarChannel.h"

OpenKNX::Channel* PVForecastModule::createChannel(uint8_t _channelIndex /* used in param macros, do not rename */)
{
    // ParamPVF_CHProviderType: 0=forecast.solar
    switch (ParamPVF_CHProviderType)
    {
        case 0:
        default:
            return new ForecastSolarChannel(_channelIndex);
    }
}

PVForecastModule openknxPVForecastModule;
