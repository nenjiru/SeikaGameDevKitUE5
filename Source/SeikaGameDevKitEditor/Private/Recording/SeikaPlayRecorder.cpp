#include "SeikaPlayRecorder.h"
#include "SeikaGameDevKitEditor.h"
#include "SeikaGameDevKitSettings.h"
#include "SeikaPlayLogEventProxy.h"
#include "SeikaPlayLogWriter.h"
#include "Editor.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/EngineVersion.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace
{
	// イベントの型の共通の親（キットの Content にある Blueprint）と、その窓口のディスパッチャーの名前
	const TCHAR* EventBaseClassPath = TEXT("/SeikaGameDevKitUE5/Events/PDA_GameEventBase.PDA_GameEventBase_C");
	const FName AnyRaisedName(TEXT("OnAnyRaised"));

	// Print String が出すログの種類
	const FName BlueprintUserMessages(TEXT("LogBlueprintUserMessages"));

	// エディタの道具が出すログ（ゲームとは関係がないので記録しない）
	const FName IgnoredCategories[] = {
		FName(TEXT("LogModelContextProtocol")),
	};

	FString LevelName(const UWorld* World)
	{
		return World != nullptr ? UWorld::RemovePIEPrefix(World->GetMapName()) : FString();
	}
}

FSeikaPlayRecorder::FSeikaPlayRecorder()
{
	BeginPIEHandle = FEditorDelegates::BeginPIE.AddRaw(this, &FSeikaPlayRecorder::OnBeginPIE);
	EndPIEHandle = FEditorDelegates::EndPIE.AddRaw(this, &FSeikaPlayRecorder::OnEndPIE);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &FSeikaPlayRecorder::OnPostLoadMap);
	AssetLoadedHandle = FCoreUObjectDelegates::OnAssetLoaded.AddRaw(this, &FSeikaPlayRecorder::OnAssetLoaded);
}

FSeikaPlayRecorder::~FSeikaPlayRecorder()
{
	Stop();
	FEditorDelegates::BeginPIE.Remove(BeginPIEHandle);
	FEditorDelegates::EndPIE.Remove(EndPIEHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	FCoreUObjectDelegates::OnAssetLoaded.Remove(AssetLoadedHandle);
}

FString FSeikaPlayRecorder::GetFolder()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("PlayLog"));
}

// PIE を始めたとき（アクターの BeginPlay より前）
void FSeikaPlayRecorder::OnBeginPIE(bool bIsSimulating)
{
	Begin(bIsSimulating);
}

void FSeikaPlayRecorder::OnEndPIE(bool bIsSimulating)
{
	if (Writer.IsValid())
	{
		Write(TEXT("session_end"), FString(), FString(), {
			FSeikaPlayLogField::Number(TEXT("duration"), Now()),
			FSeikaPlayLogField::Int(TEXT("events"), EventCount),
		});
	}
	Stop();
}

// PIE の中でレベルを開き直したとき（Open Level など）
void FSeikaPlayRecorder::OnPostLoadMap(UWorld* World)
{
	if (!Writer.IsValid() || World == nullptr || World->WorldType != EWorldType::PIE)
	{
		return;
	}
	Write(TEXT("level_loaded"), FString(), FString(), {
		FSeikaPlayLogField::String(TEXT("level"), LevelName(World)),
	});
}

// PIE の途中で読み込まれたイベントのアセットも記録する
void FSeikaPlayRecorder::OnAssetLoaded(UObject* Asset)
{
	if (Writer.IsValid() && EventBaseClass != nullptr && Asset != nullptr && Asset->IsA(EventBaseClass))
	{
		BindEvent(Asset);
	}
}

void FSeikaPlayRecorder::Begin(bool bIsSimulating)
{
	Stop();
	if (!GetDefault<USeikaGameDevKitSettings>()->bRecordPlayLog)
	{
		return;
	}

	const FString Folder = GetFolder();
	IFileManager::Get().MakeDirectory(*Folder, true);
	const FString Path = Folder / (FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT(".jsonl"));
	{
		TUniquePtr<FSeikaPlayLogWriter> NewWriter = MakeUnique<FSeikaPlayLogWriter>(Path);
		if (!NewWriter->IsOpen())
		{
			UE_LOG(LogSeikaGameDevKit, Warning, TEXT("記録のファイルを作れなかったので、今回は記録しません：%s"), *Path);
			return;
		}
		FScopeLock ScopeLock(&WriterLock);
		Writer = MoveTemp(NewWriter);
	}
	StartSeconds = FPlatformTime::Seconds();
	StartFrame = GFrameCounter;
	EventCount = 0;
	DeleteOldFiles();

	const UWorld* EditorWorld = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	TArray<FSeikaPlayLogField> Fields = {
		FSeikaPlayLogField::String(TEXT("date"), FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"))),
		FSeikaPlayLogField::String(TEXT("unreal"), FEngineVersion::Current().ToString(EVersionComponent::Patch)),
		FSeikaPlayLogField::String(TEXT("level"), LevelName(EditorWorld)),
	};
	if (bIsSimulating)
	{
		Fields.Add(FSeikaPlayLogField::String(TEXT("mode"), TEXT("simulate")));
	}
	Write(TEXT("session_start"), FString(), FString(), Fields);

	GLog->AddOutputDevice(this);

	// すでに読み込まれているイベントのアセットに登録する
	EventBaseClass = LoadObject<UClass>(nullptr, EventBaseClassPath);
	if (EventBaseClass != nullptr)
	{
		TArray<UObject*> Events;
		GetObjectsOfClass(EventBaseClass, Events, true, RF_ClassDefaultObject | RF_ArchetypeObject);
		for (UObject* GameEvent : Events)
		{
			BindEvent(GameEvent);
		}
	}
}

void FSeikaPlayRecorder::Stop()
{
	if (GLog != nullptr)
	{
		GLog->RemoveOutputDevice(this);
	}
	UnbindAllEvents();

	FScopeLock ScopeLock(&WriterLock);
	if (Writer.IsValid())
	{
		Writer->FlushAll(Now(), Frame());
		Writer.Reset();
	}
}

void FSeikaPlayRecorder::BindEvent(UObject* GameEvent)
{
	if (GameEvent == nullptr || GameEvent->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}
	for (const USeikaPlayLogEventProxy* Proxy : Proxies)
	{
		if (Proxy->GameEvent.Get() == GameEvent)
		{
			return;
		}
	}
	FMulticastDelegateProperty* AnyRaised = FindFProperty<FMulticastDelegateProperty>(GameEvent->GetClass(), AnyRaisedName);
	if (AnyRaised == nullptr)
	{
		return;
	}

	USeikaPlayLogEventProxy* Proxy = NewObject<USeikaPlayLogEventProxy>(GetTransientPackage());
	Proxy->GameEvent = GameEvent;
	Proxy->Recorder = this;
	FScriptDelegate Delegate;
	Delegate.BindUFunction(Proxy, GET_FUNCTION_NAME_CHECKED(USeikaPlayLogEventProxy, OnAnyRaised));
	AnyRaised->AddDelegate(MoveTemp(Delegate), GameEvent);
	Proxies.Add(Proxy);
}

// イベントのアセットはエディタの中に残り続けるので、PIE が終わったら登録を外す（次の PIE で二重に記録しないため）
void FSeikaPlayRecorder::UnbindAllEvents()
{
	for (USeikaPlayLogEventProxy* Proxy : Proxies)
	{
		UObject* GameEvent = Proxy->GameEvent.Get();
		if (GameEvent == nullptr)
		{
			continue;
		}
		if (FMulticastDelegateProperty* AnyRaised = FindFProperty<FMulticastDelegateProperty>(GameEvent->GetClass(), AnyRaisedName))
		{
			FScriptDelegate Delegate;
			Delegate.BindUFunction(Proxy, GET_FUNCTION_NAME_CHECKED(USeikaPlayLogEventProxy, OnAnyRaised));
			AnyRaised->RemoveDelegate(Delegate, GameEvent);
		}
		Proxy->Recorder = nullptr;
	}
	Proxies.Reset();
}

void FSeikaPlayRecorder::RecordEvent(const UObject* GameEvent, const FString& ValueText)
{
	if (GameEvent == nullptr)
	{
		return;
	}
	EventCount++;
	const FString Name = GameEvent->GetName();
	TArray<FSeikaPlayLogField> Fields = { FSeikaPlayLogField::String(TEXT("name"), Name) };
	if (!ValueText.IsEmpty())
	{
		Fields.Add(FSeikaPlayLogField::String(TEXT("value"), ValueText));
	}
	Write(TEXT("event"), TEXT("event:") + Name, ValueText, Fields);
}

void FSeikaPlayRecorder::Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category)
{
	const ELogVerbosity::Type Level = static_cast<ELogVerbosity::Type>(Verbosity & ELogVerbosity::VerbosityMask);
	const bool bProblem = Level != ELogVerbosity::NoLogging && Level <= ELogVerbosity::Warning;
	const bool bPrintString = Category == BlueprintUserMessages;
	if (!bProblem && !bPrintString)
	{
		return;
	}
	for (const FName& Ignored : IgnoredCategories)
	{
		if (Category == Ignored)
		{
			return;
		}
	}

	const TCHAR* LevelText = Level <= ELogVerbosity::Error ? TEXT("error") : (Level == ELogVerbosity::Warning ? TEXT("warning") : TEXT("log"));
	const FString CategoryText = Category.ToString();
	const FString MessageText(Message);
	Write(TEXT("log"), FString::Printf(TEXT("log:%s:%s:%s"), LevelText, *CategoryText, *MessageText), MessageText, {
		FSeikaPlayLogField::String(TEXT("level"), LevelText),
		FSeikaPlayLogField::String(TEXT("category"), CategoryText),
		FSeikaPlayLogField::String(TEXT("message"), MessageText),
	});
}

void FSeikaPlayRecorder::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(Proxies);
}

double FSeikaPlayRecorder::Now() const
{
	return FPlatformTime::Seconds() - StartSeconds;
}

int64 FSeikaPlayRecorder::Frame() const
{
	return static_cast<int64>(GFrameCounter - StartFrame);
}

void FSeikaPlayRecorder::Write(const FString& Kind, const FString& ThrottleKey, const FString& Value, const TArray<FSeikaPlayLogField>& Fields)
{
	FScopeLock ScopeLock(&WriterLock);
	if (Writer.IsValid())
	{
		Writer->Write(Kind, Now(), Frame(), ThrottleKey, Value, Fields);
	}
}

void FSeikaPlayRecorder::DeleteOldFiles() const
{
	const FString Folder = GetFolder();
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Folder / TEXT("*.jsonl")), true, false);
	// 名前が日時なので、名前の降順が新しい順
	Files.Sort([](const FString& A, const FString& B) { return A > B; });
	for (int32 Index = KeepFiles; Index < Files.Num(); ++Index)
	{
		IFileManager::Get().Delete(*(Folder / Files[Index]));
	}
}
