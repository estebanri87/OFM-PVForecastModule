#pragma once
#include "ChannelOwnerModule.h"
#include "ModuleVersionCheck.h"

class PVForecastModule : public PVFChannelOwnerModule
{
  public:
    const char* name() override { return "PVForecastModule"; }
    const char* version() override { return PVF_ModuleVersion; }

  protected:
    OpenKNX::Channel* createChannel(uint8_t index) override;
};

extern PVForecastModule openknxPVForecastModule;
