using UnrealBuildTool;

// キットの C++ の部品。エディタだけで動き、ゲームのパッケージには入らない
// （ゲームに入るモジュールがあると、学生の PC でパッケージ化にコンパイラが要るため）
public class SeikaGameDevKitEditor : ModuleRules
{
	public SeikaGameDevKitEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Projects",
			"DeveloperSettings",
		});

		// Recording/ などのサブフォルダのヘッダーを、フォルダ名なしでも含められるようにする
		PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "Private"));
	}
}
