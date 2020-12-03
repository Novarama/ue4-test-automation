#include "DaeSyncClientActor.h"
#include "DaeTestNetActor.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

TArray<ADaeSyncClientActor*> ADaeSyncClientActor::SyncClientActors;
ADaeSyncClientActor* ADaeSyncClientActor::LocalSyncClientActor = nullptr;

ADaeSyncClientActor::ADaeSyncClientActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetReplicates(true);
	bAlwaysRelevant = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ADaeSyncClientActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADaeSyncClientActor, InternalClientId);
}

void ADaeSyncClientActor::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* LocalPlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (!HasAuthority() && LocalPlayerController == GetOwner())
	{
		LocalSyncClientActor = this;
		NotifyClientSynched();
	}
}

int ADaeSyncClientActor::GetClientId() const
{
	return InternalClientId;
}

void ADaeSyncClientActor::SetClientId(int ClientId)
{
	// Only server can assign the id to clients
	if (HasAuthority())
	{
		InternalClientId = ClientId;
	}
}

int ADaeSyncClientActor::NumClients()
{
	return SyncClientActors.Num();
}

ADaeSyncClientActor* ADaeSyncClientActor::GetSyncClientActor(int ClientId)
{
	if (SyncClientActors.IsValidIndex(ClientId))
	{
		return SyncClientActors[ClientId];
	}

	return nullptr;
}

ADaeSyncClientActor* ADaeSyncClientActor::GetSyncClientActor()
{
	return LocalSyncClientActor;
}

void ADaeSyncClientActor::UpdateSyncClientActors(TArray<ADaeSyncClientActor*> NewSyncClientActors)
{
	SyncClientActors = NewSyncClientActors;
}

bool ADaeSyncClientActor::AllClientsSynched()
{
	bool bAllClientsSynched = SyncClientActors.Num() > 0; // false when no sync actors, probably not spawned yet

	for (int i = 0; i < SyncClientActors.Num(); ++i)
	{
		ADaeSyncClientActor* SyncClientActor = SyncClientActors[i];

		bAllClientsSynched &= IsValid(SyncClientActor) && !SyncClientActor->bClientSynched;
	}

	return bAllClientsSynched;
}

void ADaeSyncClientActor::NotifyOnAssume(ADaeTestNetActor* TestActor, UObject* Parameter)
{
	for (int i = 0; i < SyncClientActors.Num(); ++i)
	{
		ADaeSyncClientActor* SyncClientActor = SyncClientActors[i];

		if (IsValid(SyncClientActor))
		{
			SyncClientActor->ReceiveOnAssume(TestActor, Parameter, i);
		}
	}
}

void ADaeSyncClientActor::NotifyOnArrange(ADaeTestNetActor* TestActor, UObject* Parameter)
{
	for (int i = 0; i < SyncClientActors.Num(); ++i)
	{
		ADaeSyncClientActor* SyncClientActor = SyncClientActors[i];

		if (IsValid(SyncClientActor))
		{
			SyncClientActor->ReceiveOnArrange(TestActor, Parameter, i);
		}
	}
}

void ADaeSyncClientActor::NotifyOnAct(ADaeTestNetActor* TestActor, UObject* Parameter)
{
	for (int i = 0; i < SyncClientActors.Num(); ++i)
	{
		ADaeSyncClientActor* SyncClientActor = SyncClientActors[i];

		if (IsValid(SyncClientActor))
		{
			SyncClientActor->ReceiveOnAct(TestActor, Parameter, i);
		}
	}
}

void ADaeSyncClientActor::NotifyOnAssert(ADaeTestNetActor* TestActor, UObject* Parameter)
{
	for (int i = 0; i < SyncClientActors.Num(); ++i)
	{
		ADaeSyncClientActor* SyncClientActor = SyncClientActors[i];

		if (IsValid(SyncClientActor))
		{
			SyncClientActor->ReceiveOnAssert(TestActor, Parameter, i);
		}
	}
}

void ADaeSyncClientActor::NotifyClientFinishAct(ADaeTestNetActor* TestActor)
{
	ADaeSyncClientActor* SyncClientActor = ADaeSyncClientActor::GetSyncClientActor();

	if (IsValid(SyncClientActor))
	{
		SyncClientActor->ReceiveFinishAct(TestActor, SyncClientActor->GetClientId());
	}
}

void ADaeSyncClientActor::NotifyAllTestsFinished()
{
	for (int i = 0; i < SyncClientActors.Num(); ++i)
	{
		ADaeSyncClientActor* SyncClientActor = SyncClientActors[i];

		if (IsValid(SyncClientActor))
		{
			SyncClientActor->CloseInstance();
		}
	}
}

void ADaeSyncClientActor::ReceiveOnAssume_Implementation(ADaeTestNetActor* TestActor, UObject* Parameter, int ClientId)
{
	if (TestActor)
	{
		TestActor->ReceiveOnAssumeClient(ClientId, Parameter);
	}
}

void ADaeSyncClientActor::ReceiveOnArrange_Implementation(ADaeTestNetActor* TestActor, UObject* Parameter, int ClientId)
{
	if (TestActor)
	{
		TestActor->ReceiveOnArrangeClient(ClientId, Parameter);
	}
}

void ADaeSyncClientActor::ReceiveOnAct_Implementation(ADaeTestNetActor* TestActor, UObject* Parameter, int ClientId)
{
	if (TestActor)
	{
		TestActor->ReceiveOnActClient(ClientId, Parameter);
	}
}

void ADaeSyncClientActor::ReceiveOnAssert_Implementation(ADaeTestNetActor* TestActor, UObject* Parameter, int ClientId)
{
	if (TestActor)
	{
		TestActor->ReceiveOnAssertClient(ClientId, Parameter);
	}
}

void ADaeSyncClientActor::CloseInstance_Implementation()
{
	FGenericPlatformMisc::RequestExit(false);
}

void ADaeSyncClientActor::ReceiveFinishAct_Implementation(ADaeTestNetActor* TestActor, int ClientId)
{
	if (TestActor)
	{
		TestActor->NotifyClientFinishAct(ClientId);
	}
}

void ADaeSyncClientActor::NotifyClientSynched_Implementation()
{
	bClientSynched = true;
}