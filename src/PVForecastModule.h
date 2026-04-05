#pragma once
#include "ChannelOwnerModule.h"
#include "ModuleVersionCheck.h"

class PVForecastModule : public PVFChannelOwnerModule
{
  public:
    PVForecastModule() : PVFChannelOwnerModule(PVF_ChannelCount) {}
    const std::string name() override { return "PVForecastModule"; }
    const std::string version() override { return std::to_string(PVF_ModuleVersion); }

  protected:
    OpenKNX::Channel* createChannel(uint8_t _channelIndex /* used in param macros, do not rename */) override;
};

extern PVForecastModule openknxPVForecastModule;
