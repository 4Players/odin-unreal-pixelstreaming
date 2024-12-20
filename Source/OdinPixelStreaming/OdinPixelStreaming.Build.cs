// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OdinPixelStreaming : ModuleRules
{
	public OdinPixelStreaming(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"PixelStreaming", "AudioMixer", "Odin",
			"OdinLibrary", "SignalProcessing"
		});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",

				// ... add private dependencies that you statically link with here ...	
			}
		);
	}
}