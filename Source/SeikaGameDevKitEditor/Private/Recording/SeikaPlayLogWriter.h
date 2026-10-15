#pragma once

#include "CoreMinimal.h"

/** 1件の項目（名前と、JSON に書く形にした値）。 */
struct FSeikaPlayLogField
{
	FString Name;
	FString JsonValue;

	static FSeikaPlayLogField String(const FString& InName, const FString& Value);
	static FSeikaPlayLogField Number(const FString& InName, double Value);
	static FSeikaPlayLogField Int(const FString& InName, int64 Value);
};

/**
 * プレイの記録を1行1件の JSON（JSON Lines）で書く。UE のエディタには依存しない。
 * 同じもの（ThrottleKey が同じ）が1秒に MaxPerSecond 回を超えたら、その秒の残りは数えるだけにし、
 * 秒が変わったときに「回数と最後の値」の1行（kind = "summary"）にまとめて書く。
 * どのスレッドから呼んでもよい（ログはほかのスレッドからも届くため）。
 */
class FSeikaPlayLogWriter
{
public:
	static constexpr int32 MaxPerSecond = 10;

	/** 書き出すファイルを開く。開けなければ IsOpen() が false になる。 */
	explicit FSeikaPlayLogWriter(const FString& InPath);
	~FSeikaPlayLogWriter();

	bool IsOpen() const { return Archive.IsValid(); }

	/**
	 * 1件書く。Fields は kind・t・frame のあとに続ける項目。
	 * ThrottleKey を渡すと、同じ key が多すぎるときにまとめる。Value はまとめたときの「最後の値」に使う。
	 */
	void Write(const FString& Kind, double Time, int64 Frame, const FString& ThrottleKey, const FString& Value, const TArray<FSeikaPlayLogField>& Fields);

	/** まとめ途中のものをすべて書き出す（記録の終わりに呼ぶ）。 */
	void FlushAll(double Time, int64 Frame);

	static FString EscapeJson(const FString& Text);

private:
	struct FWindow
	{
		int64 Second = 0;
		int32 Count = 0;
		int32 Suppressed = 0;
		FString LastValue;
	};

	void FlushEndedWindows(double Time, int64 Frame);
	void WriteSummary(const FString& Key, const FWindow& Window, double Time, int64 Frame);
	void WriteLine(const FString& Kind, double Time, int64 Frame, const TArray<FSeikaPlayLogField>& Fields);

	TUniquePtr<FArchive> Archive;
	TMap<FString, FWindow> Windows;
	FCriticalSection Lock;
};
