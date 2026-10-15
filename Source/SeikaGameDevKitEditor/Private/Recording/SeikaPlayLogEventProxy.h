#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SeikaPlayLogEventProxy.generated.h"

class FSeikaPlayRecorder;

/**
 * イベントのアセット1つの On Any Raised に登録し、送られたことを記録係に伝える。
 * どのイベントが送られたかを知るため、イベントのアセットごとに1つ作る。
 */
UCLASS(Transient)
class USeikaPlayLogEventProxy : public UObject
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UObject> GameEvent;
	FSeikaPlayRecorder* Recorder = nullptr;

	/** On Any Raised から呼ばれる（引数はイベントのディスパッチャーと同じ形にする）。 */
	UFUNCTION()
	void OnAnyRaised(const FString& ValueText);
};
