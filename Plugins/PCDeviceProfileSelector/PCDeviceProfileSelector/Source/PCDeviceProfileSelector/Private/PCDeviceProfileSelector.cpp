// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCDeviceProfileSelector.h"
#include "Modules/ModuleManager.h"
#include "RHI.h"
#include "Misc/ConfigCacheIni.h"
#include <regex>

#define LOCTEXT_NAMESPACE "FPCDeviceProfileSelectorModule"

void FPCDeviceProfileSelectorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FPCDeviceProfileSelectorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

const FString FPCDeviceProfileSelectorModule::GetRuntimeDeviceProfileName() {
    // Windows, WindowsNoEditor, WindowsClient, or WindowsServer
    FString WindowsDevicsProfileName = FPlatformProperties::PlatformName();

    // Custom graphics quality classification.
    if (FApp::CanEverRender()) {
        // CPU
        int32 OutCPUGrade = 0;
        const FString CleanedCPUBrand = GetCleanedDeviceName(FPlatformMisc::GetCPUBrand(), EDeviceChipType::CPU);
        const FString CPUProfileName = FindMatchingProfile(CleanedCPUBrand, TEXT("DeviceCPUMarks"), TEXT("DeviceQualityCPUGrade"), OutCPUGrade);

        // GPU
        int32 OutGPUGrade = 0;
        FString GPUBrand = FPlatformMisc::GetPrimaryGPUBrand();
        if (!GRHIAdapterName.IsEmpty() && !GRHIAdapterName.Equals(GPUBrand)) {
            GPUBrand = GRHIAdapterName;
        }

        const FString CleanedGPUBrand = GetCleanedDeviceName(GPUBrand, EDeviceChipType::GPU);
        const FString GPUProfileName = FindMatchingProfile(CleanedGPUBrand, TEXT("DeviceGPUMarks"), TEXT("DeviceQualityGPUGrade"), OutGPUGrade);

        // Select lower quality profile.
        WindowsDevicsProfileName = (OutCPUGrade <= OutGPUGrade) ? CPUProfileName : GPUProfileName;
    }
    
    UE_LOG(LogWindows, Log, TEXT("Selected Device Profile: [%s]"), *WindowsDevicsProfileName);
    return WindowsDevicsProfileName;
}

FString FPCDeviceProfileSelectorModule::FindMatchingProfile(const FString DeviceChipName,
    const TCHAR* DeviceMarkSection, const TCHAR* DeviceQualitySection, int32& OutQuality) {
    FString WindowsDevicsProfileName;

    TArray<FString> DeviceMapping;
    if (ensure(GConfig->GetSection(DeviceMarkSection, DeviceMapping, GDeviceProfilesIni))) {
        for (const auto& DeviceKVP : DeviceMapping) {
            FString Name, Mark;
            if (!DeviceKVP.Split(TEXT("="), &Name, &Mark)) {
                UE_LOG(LogWindows, Error, TEXT("Invalid WindowsDeviceMappings: %s"), *DeviceKVP);
                continue;
            }

            const FRegexPattern RegexPattern(Name);
            FRegexMatcher RegexMatcher(RegexPattern, *DeviceChipName);
            if (!RegexMatcher.FindNext()) {
                continue;
            }

            // Profile matching.
            TArray<FString> ProfileMapping;
            if (ensure(GConfig->GetSection(DeviceQualitySection, ProfileMapping, GDeviceProfilesIni))) {
                for (int32 i = 0; i < ProfileMapping.Num(); ++i) {
                    FString ProfileName, Grade;
                    if (!ProfileMapping[i].Split(TEXT("="), &ProfileName, &Grade)) {
                        continue;
                    }

                    int32 MarkInt = FCString::Atoi(*Mark.TrimStartAndEnd());
                    int32 GradeInt = FCString::Atoi(*Grade.TrimStartAndEnd());
                    if (i == 0 || MarkInt >= GradeInt) {
                        WindowsDevicsProfileName = ProfileName;
                        OutQuality = i;
                    }
                }
            }

            UE_LOG(LogWindows, Log, TEXT("Matched %s as %s[%i]"), *Name, *WindowsDevicsProfileName, OutQuality);
            break;
        }
    }

    return WindowsDevicsProfileName;
}

FString FPCDeviceProfileSelectorModule::GetCleanedDeviceName(const FString Original, const EDeviceChipType ChipType) {
    // 1. Lower
    std::string LowerOriginal(TCHAR_TO_UTF8(*Original.ToLower()));

    // 2. Match pattern.
    std::regex CleanPattern("");
    switch (ChipType) {
    case EDeviceChipType::CPU:
        CleanPattern = "\\([^\\(\\)]*\\)|@.*$|^.*th gen|with.*radeon.*graphics|[\\w\\d]*-core processor|cpu|processor|apu";
        break;
    case EDeviceChipType::GPU:
        CleanPattern = "\\([^\\(\\)]*\\)|with.*radeon.*graphics|graphics|amd|ati|nvidia|adapter|series|gpu";
        break;
    default:
        break;
    }
    LowerOriginal = std::regex_replace(LowerOriginal, CleanPattern, "");

    // 3. Replace two space to one and trim.
    FString Result = UTF8_TO_TCHAR(LowerOriginal.c_str());
    Result = Result.Replace(TEXT("  "), TEXT(" ")).TrimStartAndEnd();

    return Result;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPCDeviceProfileSelectorModule, PCDeviceProfileSelector)
