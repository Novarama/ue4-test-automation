#include "DaeTestNetActor.h"
#include "DaeSyncClientActor.h"
#include "Net/UnrealNetwork.h"

ADaeTestNetActor::ADaeTestNetActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetReplicates(true);
}

void ADaeTestNetActor::NotifyOnAssume(UObject* Parameter)
{
	Super::NotifyOnAssume(Parameter);
	ADaeSyncClientActor::NotifyOnAssume(this, Parameter);
}

void ADaeTestNetActor::NotifyOnArrange(UObject* Parameter)
{
	Super::NotifyOnArrange(Parameter);
	ADaeSyncClientActor::NotifyOnArrange(this, Parameter);
}

void ADaeTestNetActor::NotifyOnAct(UObject* Parameter)
{
	Super::NotifyOnAct(Parameter);
	ADaeSyncClientActor::NotifyOnAct(this, Parameter);
}

void ADaeTestNetActor::NotifyOnAssert(UObject* Parameter)
{
	Super::NotifyOnAssert(Parameter);
	ADaeSyncClientActor::NotifyOnAssert(this, Parameter);
}

void ADaeTestNetActor::ReceiveOnAct_Implementation(UObject* Parameter)
{
	ServerFinishAct();
}

void ADaeTestNetActor::NotifyClientFinishAct(int ClientId)
{
	ensureAlways(HasAuthority());


	++NumClientsFinished;
	CheckFinishAct();
}

void ADaeTestNetActor::ClientFinishAct()
{
	if (!HasAuthority())
	{
		ADaeSyncClientActor::NotifyClientFinishAct(this);
	}
}

void ADaeTestNetActor::ServerFinishAct()
{
	if (HasAuthority())
	{
		bServerFinished = true;
		CheckFinishAct();
	}
}

void ADaeTestNetActor::ReceiveOnActClient_Implementation(const int ClientId, UObject* Parameter)
{
	ClientFinishAct();
}

void ADaeTestNetActor::CheckFinishAct()
{
	if (bServerFinished && NumClientsFinished == ADaeSyncClientActor::NumClients())
	{
		FinishAct();
	}
}
