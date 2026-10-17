using UnrealBuildTool;

public class KernelGlowEditor : ModuleRules
{
	public KernelGlowEditor(ReadOnlyTargetRules Target) : base(Target)
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
			"KernelGlow",
			"UnrealEd",
			"ToolMenus",
			"Slate",
			"SlateCore",
			"EditorStyle",
			"Projects"
		});
	}
}
