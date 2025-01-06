// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TribeMIDI : ModuleRules
{
    public TribeMIDI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", System.IO.Path.Combine(ModuleDirectory, "Android", "TribeMIDI_APL.xml"));
            // ensure android jni support:
            PublicDependencyModuleNames.AddRange(new string[] { "ApplicationCore", "Launch" });
        }

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new string[] { });

        PublicIncludePaths.AddRange(new string[] {
            // add if needed
        });

        PrivateIncludePaths.AddRange(new string[] {
            "TribeMIDI/Private"
        });
    }
}
