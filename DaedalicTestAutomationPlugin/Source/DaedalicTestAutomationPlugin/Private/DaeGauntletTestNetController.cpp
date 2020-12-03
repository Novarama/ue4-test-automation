#include "DaeGauntletTestNetController.h"
#include "DaeSyncClientActor.h"
#include "Kismet/GameplayStatics.h"
#include "DaeGauntletStates.h"
#include "DaeTestAutomationPluginSettings.h"
#include "DaeTestLogCategory.h"
#include "DaeTestReportWriter.h"
#include "DaeTestReportWriterSet.h"
#include "DaeTestSuiteActor.h"
#include <AssetRegistryModule.h>
#include <EngineUtils.h>
#include <Engine/AssetManager.h>
#include <Misc/FileHelper.h>

void UDaeGauntletTestNetController::OnInit()
{
	Super::OnInit();

	// Get tests path.
	const UDaeTestAutomationPluginSettings* TestAutomationPluginSettings = GetDefault<UDaeTestAutomationPluginSettings>();

	for (const FString& TestMapFolder : TestAutomationPluginSettings->TestMapFolders)
	{
		UE_LOG(LogDaeTest, Display, TEXT("Discovering tests from: %s"), *TestMapFolder);
	}

	// Build list of tests (based on FAutomationEditorCommonUtils::CollectTestsByClass).
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetDataArray;

	AssetRegistryModule.Get().SearchAllAssets(true);
	AssetRegistryModule.Get().GetAssetsByClass(UWorld::StaticClass()->GetFName(), AssetDataArray);

	for (auto ObjIter = AssetDataArray.CreateConstIterator(); ObjIter; ++ObjIter)
	{
		const FAssetData& AssetData = *ObjIter;

		FString FileName = FPackageName::LongPackageNameToFilename(AssetData.ObjectPath.ToString());
		FName MapName = AssetData.AssetName;

		bool bIsTestMap = TestAutomationPluginSettings->IsTestMap(FileName, MapName);

		if (bIsTestMap)
		{
			MapNames.Add(MapName);

			UE_LOG(LogDaeTest, Display, TEXT("Discovered test: %s"), *MapName.ToString());
		}
	}

	// Set console variables.
	for (auto& ConsoleVariable : TestAutomationPluginSettings->ConsoleVariables)
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*ConsoleVariable.Key);

		if (CVar)
		{
			CVar->Set(*ConsoleVariable.Value);

			UE_LOG(LogDaeTest, Log, TEXT("Setting console variable %s = %s"), *ConsoleVariable.Key, *ConsoleVariable.Value);
		}
	}

	GetGauntlet()->BroadcastStateChange(FDaeGauntletStates::Initialized);
}

void UDaeGauntletTestNetController::OnPostMapChange(UWorld* World)
{
	Super::OnPostMapChange(World);

	if (World)
	{
		UE_LOG(LogDaeTest, Log, TEXT("UDaeGauntletTestNetController::OnPostMapChange - World: %s"), *World->GetName());
	}

	if (GetCurrentState() != FDaeGauntletStates::LoadingNextMap)
	{
		return;
	}

	WaitTimer = 0.f;
	GetGauntlet()->BroadcastStateChange(FDaeGauntletStates::DiscoveringTests);
}

void UDaeGauntletTestNetController::OnTick(float TimeDelta)
{
	if (GetCurrentState() == FDaeGauntletStates::Initialized)
	{
		// Wait some time for clients to connect to server
		WaitTimer += TimeDelta;
		if (WaitTimer < InitializeClientsWaitTime)
		{
			return;
		}

		// If this isn't a test map (e.g. immediately after startup), load first test map now.
		if (!MapNames.Contains(FName(*GetCurrentMap())))
		{
			UE_LOG(LogDaeTest, Log, TEXT("FDaeGauntletStates::Initialized - World is not a test world, loading first test world."));

			MapIndex = -1;
			LoadNextTestMap();
			return;
		}
		else
		{
			WaitTimer = 0.f;
			GetGauntlet()->BroadcastStateChange(FDaeGauntletStates::DiscoveringTests);
		}
	}
	else if (GetCurrentState() == FDaeGauntletStates::LoadingNextMap)
	{
		UE_LOG(LogDaeTest, Display, TEXT("FDaeGauntletStates::LoadingNextMap - Loading map: %s"), *MapNames[MapIndex].ToString());

		GetWorld()->ServerTravel(MapNames[MapIndex].ToString(), true);
	}
	else if (GetCurrentState() == FDaeGauntletStates::DiscoveringTests)
	{
		// Spawn the sync actors after some time so the player controllers are properly replicated
		if (WaitTimer < MapReplicationMapWaitTime && (WaitTimer + TimeDelta) >= MapReplicationMapWaitTime)
		{
			GenerateSyncActors();
		}
		WaitTimer += TimeDelta;

		// Wait until all clients notified they are properly replicated
		if (!ADaeSyncClientActor::AllClientsSynched())
		{
			return;
		}


		// Find test suite.
		ADaeTestSuiteActor* TestSuite = nullptr;

		for (TActorIterator<ADaeTestSuiteActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
		{
			TestSuite = *ActorIt;
		}

		if (!IsValid(TestSuite))
		{
			UE_LOG(LogDaeTest, Error, TEXT("FDaeGauntletStates::DiscoveringTests - No DaeGauntletTestSuiteActor found."));
			LoadNextTestMap();
			return;
		}

		// Start first test.
		GetGauntlet()->BroadcastStateChange(FDaeGauntletStates::Running);

		TestSuite->OnTestSuiteSuccessful.AddDynamic(this, &UDaeGauntletTestNetController::OnTestSuiteFinished);
		TestSuite->OnTestSuiteFailed.AddDynamic(this, &UDaeGauntletTestNetController::OnTestSuiteFinished);

		TestSuite->RunAllTests();
	}
	else if (GetCurrentState() == FDaeGauntletStates::Finished)
	{
		// Wait some time so clients clients can finish
		WaitTimer += TimeDelta;
		if (WaitTimer < FinalizeClientsWaitTime)
		{
			return;
		}

		for (const FDaeTestSuiteResult& Result : Results)
		{
			if (Result.NumFailedTests() > 0)
			{
				EndTest(1);
				return;
			}
		}

		EndTest(0);
	}
}

void UDaeGauntletTestNetController::LoadNextTestMap()
{
	++MapIndex;

	// Check if we just want to run a single test.
	FString SingleTestName = ParseCommandLineOption(TEXT("TestName"));

	if (!SingleTestName.IsEmpty())
	{
		while (MapNames.IsValidIndex(MapIndex) && MapNames[MapIndex].ToString() != SingleTestName)
		{
			++MapIndex;
		}
	}

	if (MapNames.IsValidIndex(MapIndex))
	{
		// Load next test map in next tick. This is to avoid invocation list changes during OnPostMapChange.
		GetGauntlet()->BroadcastStateChange(FDaeGauntletStates::LoadingNextMap);
	}
	else
	{
		// All tests finished.
		UE_LOG(LogDaeTest, Display, TEXT("UDaeGauntletTestNetController::LoadNextTestMap - All tests finished."));

		// Notify clients we're finished
		ADaeSyncClientActor::NotifyAllTestsFinished();

		// Finish Gauntlet.
		WaitTimer = 0.f;
		GetGauntlet()->BroadcastStateChange(FDaeGauntletStates::Finished);
	}
}

void UDaeGauntletTestNetController::GenerateSyncActors()
{
	if (UWorld* World = GetWorld())
	{
		TArray<ADaeSyncClientActor*> SyncClientActors;
		SyncClientActors.Reserve(World->GetNumPlayerControllers());

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PlayerController = It->Get();
			if (PlayerController)
			{
				FActorSpawnParameters Params;
				Params.Owner = PlayerController;

				UClass* Class = ADaeSyncClientActor::StaticClass();
				FTransform Transform = FTransform::Identity;
				AActor* Owner = PlayerController;

				ADaeSyncClientActor* SyncClientActor = World->SpawnActorDeferred<ADaeSyncClientActor>(Class, Transform, Owner);
				SyncClientActor->SetClientId(SyncClientActors.Num());
				SyncClientActor->FinishSpawning(Transform);

				SyncClientActors.Add(SyncClientActor);
			}
		}

		ADaeSyncClientActor::UpdateSyncClientActors(SyncClientActors);
		UE_LOG(LogDaeTest, Display, TEXT("Generated %d client sync actors"), SyncClientActors.Num());
	}
	else
	{
		UE_LOG(LogDaeTest, Error, TEXT("UDaeGauntletTestNetController::GenerateSyncActors World is not valid."));
	}
}

void UDaeGauntletTestNetController::OnTestSuiteFinished(ADaeTestSuiteActor* TestSuite)
{
	// Store result.
	Results.Add(TestSuite->GetResult());

	// Update test reports on disk.
	FString ReportPath = ParseCommandLineOption(TEXT("ReportPath"));

	FDaeTestReportWriterSet ReportWriters = TestSuite->GetReportWriters();

	for (TSharedPtr<FDaeTestReportWriter> ReportWriter : ReportWriters.GetReportWriters())
	{
		ReportWriter->WriteReport(Results, ReportPath);
	}

	// Proceed with next test.
	LoadNextTestMap();
}

FString UDaeGauntletTestNetController::ParseCommandLineOption(const FString& Key)
{
	FString Value;
	FParse::Value(FCommandLine::Get(), *Key, Value);
	return Value.Mid(1);
}
