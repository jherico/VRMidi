#pragma once
#include "Modules/ModuleManager.h"


class FTribeMidiModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
