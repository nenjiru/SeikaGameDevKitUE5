#include "SeikaPlayLogWriter.h"
#include "HAL/FileManager.h"
#include "Misc/ScopeLock.h"

FSeikaPlayLogField FSeikaPlayLogField::String(const FString& InName, const FString& Value)
{
	return { InName, TEXT("\"") + FSeikaPlayLogWriter::EscapeJson(Value) + TEXT("\"") };
}

FSeikaPlayLogField FSeikaPlayLogField::Number(const FString& InName, double Value)
{
	// NaN と無限大は JSON の数にならないので文字で書く
	if (!FMath::IsFinite(Value))
	{
		return String(InName, LexToString(Value));
	}
	return { InName, FString::Printf(TEXT("%.3f"), Value) };
}

FSeikaPlayLogField FSeikaPlayLogField::Int(const FString& InName, int64 Value)
{
	return { InName, LexToString(Value) };
}

FSeikaPlayLogWriter::FSeikaPlayLogWriter(const FString& InPath)
	: Archive(IFileManager::Get().CreateFileWriter(*InPath, FILEWRITE_AllowRead))
{
}

FSeikaPlayLogWriter::~FSeikaPlayLogWriter()
{
	FScopeLock ScopeLock(&Lock);
	if (Archive.IsValid())
	{
		Archive->Close();
	}
}

void FSeikaPlayLogWriter::Write(const FString& Kind, double Time, int64 Frame, const FString& ThrottleKey, const FString& Value, const TArray<FSeikaPlayLogField>& Fields)
{
	FScopeLock ScopeLock(&Lock);
	if (!Archive.IsValid())
	{
		return;
	}

	FlushEndedWindows(Time, Frame);

	if (!ThrottleKey.IsEmpty())
	{
		const int64 Second = FMath::FloorToInt64(Time);
		FWindow* Window = Windows.Find(ThrottleKey);
		if (Window == nullptr)
		{
			Window = &Windows.Add(ThrottleKey);
			Window->Second = Second;
		}
		if (Window->Second != Second)
		{
			WriteSummary(ThrottleKey, *Window, Time, Frame);
			Window->Second = Second;
			Window->Count = 0;
			Window->Suppressed = 0;
		}
		Window->Count++;
		if (Window->Count > MaxPerSecond)
		{
			Window->Suppressed++;
			Window->LastValue = Value;
			return;
		}
	}

	WriteLine(Kind, Time, Frame, Fields);
}

void FSeikaPlayLogWriter::FlushAll(double Time, int64 Frame)
{
	FScopeLock ScopeLock(&Lock);
	if (!Archive.IsValid())
	{
		return;
	}
	for (const TPair<FString, FWindow>& Pair : Windows)
	{
		WriteSummary(Pair.Key, Pair.Value, Time, Frame);
	}
	Windows.Reset();
	Archive->Flush();
}

// 秒が変わったのに、まとめを書いていないものを書く（静かになったものの、まとめが遅れないように）
void FSeikaPlayLogWriter::FlushEndedWindows(double Time, int64 Frame)
{
	const int64 Second = FMath::FloorToInt64(Time);
	for (TPair<FString, FWindow>& Pair : Windows)
	{
		if (Pair.Value.Second < Second && Pair.Value.Suppressed > 0)
		{
			WriteSummary(Pair.Key, Pair.Value, Time, Frame);
			Pair.Value.Suppressed = 0;
		}
	}
}

void FSeikaPlayLogWriter::WriteSummary(const FString& Key, const FWindow& Window, double Time, int64 Frame)
{
	if (Window.Suppressed <= 0)
	{
		return;
	}
	WriteLine(TEXT("summary"), Time, Frame, {
		FSeikaPlayLogField::String(TEXT("of"), Key),
		FSeikaPlayLogField::Int(TEXT("second"), Window.Second),
		FSeikaPlayLogField::Int(TEXT("total"), Window.Count),
		FSeikaPlayLogField::Int(TEXT("omitted"), Window.Suppressed),
		FSeikaPlayLogField::String(TEXT("last"), Window.LastValue),
	});
}

void FSeikaPlayLogWriter::WriteLine(const FString& Kind, double Time, int64 Frame, const TArray<FSeikaPlayLogField>& Fields)
{
	FString Line = FString::Printf(TEXT("{\"kind\":\"%s\",\"t\":%.3f,\"frame\":%lld"), *EscapeJson(Kind), Time, Frame);
	for (const FSeikaPlayLogField& Field : Fields)
	{
		Line += FString::Printf(TEXT(",\"%s\":%s"), *EscapeJson(Field.Name), *Field.JsonValue);
	}
	Line += TEXT("}\n");

	// 文字は UTF-8 で書く（エージェントや学生が日本語のまま読めるように）
	const FTCHARToUTF8 Utf8(*Line);
	Archive->Serialize(const_cast<void*>(static_cast<const void*>(Utf8.Get())), Utf8.Length());
	Archive->Flush();
}

FString FSeikaPlayLogWriter::EscapeJson(const FString& Text)
{
	FString Result;
	Result.Reserve(Text.Len() + 8);
	for (const TCHAR C : Text)
	{
		switch (C)
		{
		case TEXT('"'): Result += TEXT("\\\""); break;
		case TEXT('\\'): Result += TEXT("\\\\"); break;
		case TEXT('\n'): Result += TEXT("\\n"); break;
		case TEXT('\r'): Result += TEXT("\\r"); break;
		case TEXT('\t'): Result += TEXT("\\t"); break;
		default:
			if (C < 0x20)
			{
				Result += FString::Printf(TEXT("\\u%04x"), static_cast<int32>(C));
			}
			else
			{
				Result.AppendChar(C);
			}
			break;
		}
	}
	return Result;
}
