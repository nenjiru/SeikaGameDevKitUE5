#include "SeikaGameDevKitEditor.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSeikaGameDevKit);

class FSeikaGameDevKitEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// 読み込めたことを出力ログに残す（ビルド済みの DLL で開けたかを確かめるため）。版は .uplugin の VersionName を正とする
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SeikaGameDevKitUE5"));
		const FString Version = Plugin.IsValid() ? Plugin->GetDescriptor().VersionName : FString();
		UE_LOG(LogSeikaGameDevKit, Log, TEXT("Seika Game Dev Kit %s を読み込みました"), *Version);
	}
};

IMPLEMENT_MODULE(FSeikaGameDevKitEditorModule, SeikaGameDevKitEditor)
