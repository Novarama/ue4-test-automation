#pragma once

#include <CoreMinimal.h>
#include "DaeGauntletTestController.h"
#include "DaeGauntletTestNetController.generated.h"

UCLASS()
class DAEDALICTESTAUTOMATIONPLUGIN_API UDaeGauntletTestNetController : public UGauntletTestController
{
	GENERATED_BODY()

public:

	const float InitializeClientsWaitTime = 5.f;
	const float MapReplicationMapWaitTime = 2.5f;
	const float FinalizeClientsWaitTime = 1.f;

	virtual void OnInit() override;
	virtual void OnPostMapChange(UWorld* World) override;
	virtual void OnTick(float TimeDelta) override;

private:
	TArray<FName> MapNames;
	int32 MapIndex;
	TArray<FDaeTestSuiteResult> Results;
	float WaitTimer = 0.f;

	void LoadNextTestMap();
	void GenerateSyncActors();

	UFUNCTION()
	void OnTestSuiteFinished(ADaeTestSuiteActor* TestSuite);

	FString ParseCommandLineOption(const FString& Key);
};