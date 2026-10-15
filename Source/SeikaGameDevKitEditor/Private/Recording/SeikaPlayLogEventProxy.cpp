#include "SeikaPlayLogEventProxy.h"
#include "SeikaPlayRecorder.h"

void USeikaPlayLogEventProxy::OnAnyRaised(const FString& ValueText)
{
	if (Recorder != nullptr)
	{
		Recorder->RecordEvent(GameEvent.Get(), ValueText);
	}
}
