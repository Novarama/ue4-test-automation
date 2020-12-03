#pragma once

#include <CoreMinimal.h>
#include "DaeSyncClientActor.generated.h"

class ADaeTestNetActor;

UCLASS()
class DAEDALICTESTAUTOMATIONPLUGIN_API ADaeSyncClientActor : public AActor
{
	GENERATED_BODY()

public:

	ADaeSyncClientActor(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;

	int GetClientId() const;
	void SetClientId(int ClientId);

	static int NumClients();
	static ADaeSyncClientActor* GetSyncClientActor(int ClientId); // Get the actor on server
	static ADaeSyncClientActor* GetSyncClientActor(); // Get the actor on client
	static void UpdateSyncClientActors(TArray<ADaeSyncClientActor*> NewSyncClientActors);
	static bool AllClientsSynched();

	// These functions are used to call the clients from server side and the server from client side
	static void NotifyOnAssume(ADaeTestNetActor* TestActor, UObject* Parameter);
	static void NotifyOnArrange(ADaeTestNetActor* TestActor, UObject* Parameter);
	static void NotifyOnAct(ADaeTestNetActor* TestActor, UObject* Parameter);
	static void NotifyOnAssert(ADaeTestNetActor* TestActor, UObject* Parameter);
	static void NotifyClientFinishAct(ADaeTestNetActor* TestActor);

	static void NotifyAllTestsFinished();

private:

	UPROPERTY(Replicated)
	int InternalClientId = INDEX_NONE;
	bool bClientSynched = false;

	static TArray<ADaeSyncClientActor*> SyncClientActors;
	static ADaeSyncClientActor* LocalSyncClientActor;

	// RPCs used to call functions on clients or server
	UFUNCTION(Client, Reliable)
	void ReceiveOnAssume(ADaeTestNetActor* TestActor, UObject* Parameter, int ClientId);
	UFUNCTION(Client, Reliable)
	void ReceiveOnArrange(ADaeTestNetActor* TestActor, UObject* Parameter, int ClientId);
	UFUNCTION(Client, Reliable)
	void ReceiveOnAct(ADaeTestNetActor* TestActor, UObject* Parameter, int ClientId);
	UFUNCTION(Client, Reliable)
	void ReceiveOnAssert(ADaeTestNetActor* TestActor, UObject* Parameter, int ClientId);

	UFUNCTION(Client, Reliable)
	void CloseInstance();

	UFUNCTION(Server, Reliable)
	void ReceiveFinishAct(ADaeTestNetActor* TestActor, int ClientId);
	UFUNCTION(Server, Reliable)
	void NotifyClientSynched();
};