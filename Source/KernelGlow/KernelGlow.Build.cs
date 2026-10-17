using UnrealBuildTool;
using System.IO;

public class KernelGlow : ModuleRules
{
	public KernelGlow(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 访问 Renderer Private/Internal 头（FPostProcessingInputs 完整定义等，RealBloom 5.7 同款）
		PrivateIncludePaths.AddRange(new string[]
		{
			Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"),
			Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Internal")
		});

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
