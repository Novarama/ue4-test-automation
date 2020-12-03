#pragma once

#include <CoreMinimal.h>
#include "DaeTestActor.h"
#include "DaeTestNetActor.generated.h"

UCLASS()
class DAEDALICTESTAUTOMATIONPLUGIN_API ADaeTestNetActor : public ADaeTestActor
{
	GENERATED_BODY()

public:

	ADaeTestNetActor(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Client Assume"))
	void ReceiveOnAssumeClient(const int ClientId, UObject* Parameter);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Client Arrange"))
	void ReceiveOnArrangeClient(const int ClientId, UObject* Parameter);

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Client Act"))
	void ReceiveOnActClient(const int ClientId, UObject* Parameter);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Client Assert"))
	void ReceiveOnAssertClient(const int ClientId, UObject* Parameter);


	UFUNCTION(BlueprintCallable)
	void ClientFinishAct();

	UFUNCTION(BlueprintCallable)
	void ServerFinishAct();


	// These functions are called on server. They override the behavior to RPC the clients to execute the test
	virtual void NotifyOnAssume(UObject* Parameter) override;
	virtual void NotifyOnArrange(UObject* Parameter) override;
	virtual void NotifyOnAct(UObject* Parameter) override;
	virtual void NotifyOnAssert(UObject* Parameter) override;

	// Called when a client finishes acting
	virtual void NotifyClientFinishAct(int ClientId);

	// Override default ReceiveOnAct so the server does not finish acting when it does nothing
	virtual void ReceiveOnAct_Implementation(UObject* Parameter) override;

private:

	bool bServerFinished = false;
	int NumClientsFinished = 0;

	void CheckFinishAct();
};