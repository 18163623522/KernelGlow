using UnrealBuildTool;

public class KernelGlow : ModuleRules
{
	public KernelGlow(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"EngineSettings",
			"DeveloperSettings",
			"RenderCore",
			"RHI",
			"Renderer",
			"Projects"
		});
	}
}
