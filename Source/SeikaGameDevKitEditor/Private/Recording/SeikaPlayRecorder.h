#pragma once

#include "CoreMinimal.h"
#include "Misc/OutputDevice.h"
#include "UObject/GCObject.h"

class FSeikaPlayLogWriter;
class USeikaPlayLogEventProxy;
class UWorld;

/**
 * PIE で遊ぶたびに、何が起きたかを自動で記録する（置くものはない）。
 * 書き出し先：プロジェクトの Saved/PlayLog/<日時>.jsonl（新しいものから KeepFiles 個を残す）。
 * 記録するもの：PIE の開始と終了、レベルの読み込み、キットのイベント、
 * 警告とエラー（どのログの種類でも）、Print String（LogBlueprintUserMessages）。
 * 止めたいときは、Project Settings > Plugins > Seika Game Dev Kit の「プレイを記録する」をオフにする。
 */
class FSeikaPlayRecorder : public FOutputDevice, public FGCObject
{
public:
	static constexpr int32 KeepFiles = 20;

	FSeikaPlayRecorder();
	virtual ~FSeikaPlayRecorder() override;

	/** 記録のフォルダ（プロジェクトの Saved/PlayLog）。 */
	static FString GetFolder();

	/** イベントのアセットから届いた知らせを書く（USeikaPlayLogEventProxy が呼ぶ）。 */
	void RecordEvent(const UObject* GameEvent, const FString& ValueText);

	// FOutputDevice
	virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override;
	virtual bool CanBeUsedOnAnyThread() const override { return true; }
	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FSeikaPlayRecorder"); }

private:
	void OnBeginPIE(bool bIsSimulating);
	void OnEndPIE(bool bIsSimulating);
	void OnPostLoadMap(UWorld* World);
	void OnAssetLoaded(UObject* Asset);

	void Begin(bool bIsSimulating);
	void Stop();
	void BindEvent(UObject* GameEvent);
	void UnbindAllEvents();
	void DeleteOldFiles() const;

	double Now() const;
	int64 Frame() const;
	void Write(const FString& Kind, const FString& ThrottleKey, const FString& Value, const TArray<struct FSeikaPlayLogField>& Fields);

	// ログはほかのスレッドからも届くので、Writer を作る・消す・使うときはこの鍵を取る
	FCriticalSection WriterLock;
	TUniquePtr<FSeikaPlayLogWriter> Writer;
	TArray<TObjectPtr<USeikaPlayLogEventProxy>> Proxies;
	UClass* EventBaseClass = nullptr;
	double StartSeconds = 0.0;
	uint64 StartFrame = 0;
	int32 EventCount = 0;
	FDelegateHandle BeginPIEHandle;
	FDelegateHandle EndPIEHandle;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle AssetLoadedHandle;
};
