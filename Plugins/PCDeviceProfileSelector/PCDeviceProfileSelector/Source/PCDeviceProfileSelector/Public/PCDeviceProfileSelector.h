// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDeviceProfileSelectorModule.h"

enum EDeviceChipType {
    CPU,
    GPU,
};

class FPCDeviceProfileSelectorModule : public IDeviceProfileSelectorModule
{
public:

    //~ Begin IDeviceProfileSelectorModule Interface
    virtual const FString GetRuntimeDeviceProfileName() override;
    //~ End IDeviceProfileSelectorModule Interface


    //~ Begin IModuleInterface Interface
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    //~ End IModuleInterface Interface


    /**
     * Virtual destructor.
     */
    virtual ~FPCDeviceProfileSelectorModule() {
    }

private:
    FString FindMatchingProfile(const FString DeviceChipName, const TCHAR* DeviceMarkSection,
        const TCHAR* DeviceQualitySection, int32& OutQuality);
    FString GetCleanedDeviceName(const FString Original, const EDeviceChipType ChipType);
};
