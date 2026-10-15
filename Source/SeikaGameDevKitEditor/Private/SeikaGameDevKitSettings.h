#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SeikaGameDevKitSettings.generated.h"

/**
 * キットの設定。Project Settings > Plugins > Seika Game Dev Kit に出る。
 * 値は Config/DefaultEditor.ini に書かれ、チームで共有される。
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Seika Game Dev Kit"))
class USeikaGameDevKitSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** PIE で遊ぶたびに、起きたことを Saved/PlayLog/<日時>.jsonl に記録する。 */
	UPROPERTY(config, EditAnywhere, Category = "Play Log", meta = (DisplayName = "プレイを記録する"))
	bool bRecordPlayLog = true;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
