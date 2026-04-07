#include "PVForecastModule.h"
#include "ForecastSolarChannel.h"
#include "SolcastChannel.h"

OpenKNX::Channel* PVForecastModule::createChannel(uint8_t _channelIndex /* used in param macros, do not rename */)
{
    // ParamPVF_CHProviderType: 0=Deaktiviert, 1=forecast.solar, 2=Solcast
    switch (ParamPVF_CHProviderType)
    {
        case 0:
            return nullptr;
        case 2:
            return new SolcastChannel(_channelIndex);
        case 1:
        default:
            return new ForecastSolarChannel(_channelIndex);
    }
}

PVForecastModule openknxPVForecastModule;
