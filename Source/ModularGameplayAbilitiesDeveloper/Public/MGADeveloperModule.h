#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMGADeveloperModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
protected:
    void HandlePostEngineInit();
};
