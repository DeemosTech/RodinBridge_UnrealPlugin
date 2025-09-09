// Fill out your copyright notice in the Description page of Project Settings.


#include "RodinWSServerNode.h"

FString baseURI = "/*";
int32 basePort = 61893;

URodinWSListenProxy* URodinWSListenProxy::Listen(URodinWSServer* RodinWSServer, FString Host, FString URI, const int32 Port)
{
	ThisClass* const Proxy = NewObject<ThisClass>();

	Proxy->Server = RodinWSServer;
	Proxy->Host = MoveTemp(Host);
	Proxy->URI = MoveTemp(URI);
	Proxy->Port = Port;

	return Proxy;
}

void URodinWSListenProxy::Activate()
{
	if (!Server)
	{
		FFrame::KismetExecutionMessage(TEXT("Passed an invalid Server to Listen()."), ELogVerbosity::Error);
		OnTaskOver(false);
		return;
	}
	Server->Listen(MoveTemp(Host), MoveTemp(URI), Port, FOnRodinWSServerListening::CreateUObject(this, &ThisClass::OnTaskOver));
}


void URodinWSListenProxy::OnTaskOver(const bool bSuccess)
{
	(bSuccess ? Listening : OnError).Broadcast();
	SetReadyToDestroy();
}

void URodinWSPublishProxy::Activate()
{
	if (!RodinWS)
	{
		FFrame::KismetExecutionMessage(TEXT("Passed an invalid RodinWS to Publish()."), ELogVerbosity::Error);
		OnTaskOver(false);
		return;
	}

	RodinWS->Publish(MoveTemp(Topic), MoveTemp(Message),
		FOnRodinWSPublished::CreateUObject(this, &URodinWSPublishProxy::OnTaskOver));
}

URodinWSPublishProxy* URodinWSPublishProxy::Publish(URodinWS* RodinWS, const FString& Topic, const FString& Message)
{
	ThisClass* const Proxy = NewObject<ThisClass>();

	Proxy->RodinWS = RodinWS;
	Proxy->Topic = Topic;
	Proxy->Message = Message;

	return Proxy;
}

void URodinWSPublishProxy::OnTaskOver(bool bSuccess)
{
	(bSuccess ? Published : Error).Broadcast();
	SetReadyToDestroy();
}

void URodinWSSubscribeProxy::Activate()
{
	if (!RodinWS)
	{
		FFrame::KismetExecutionMessage(TEXT("Passed an invalid RodinWS to Subscribe()."), ELogVerbosity::Error);
		OnTaskOver(false, 0);
		return;
	}
	RodinWS->Subscribe(MoveTemp(Topic), FOnRodinWSSubscribed::CreateUObject(this, &ThisClass::OnTaskOver));
}

URodinWSSubscribeProxy* URodinWSSubscribeProxy::Subscribe(URodinWS* RodinWS, const FString& Topic)
{
	ThisClass* const Proxy = NewObject<ThisClass>();

	Proxy->RodinWS = RodinWS;
	Proxy->Topic = Topic;

	return Proxy;
}

void URodinWSSubscribeProxy::OnTaskOver(bool bSuccess, int32 Count)
{
	(bSuccess ? Subscribed : Error).Broadcast(Count);
	SetReadyToDestroy();
}

void URodinWSUnsubscribeProxy::Activate()
{
	if (!RodinWS)
	{
		FFrame::KismetExecutionMessage(TEXT("Passed an invalid WS to unsubscribe()."), ELogVerbosity::Error);
		OnTaskOver(false, 0);
		return;
	}
	RodinWS->Unsubscribe(MoveTemp(Topic), FOnRodinWSSubscribed::CreateUObject(this, &ThisClass::OnTaskOver));
}

URodinWSUnsubscribeProxy* URodinWSUnsubscribeProxy::Unsubscribe(URodinWS* RodinWS, const FString& Topic)
{
	ThisClass* const Proxy = NewObject<ThisClass>();

	Proxy->RodinWS = RodinWS;
	Proxy->Topic = Topic;

	return Proxy;
}

void URodinWSUnsubscribeProxy::OnTaskOver(bool bSuccess, int32 Count)
{
	(bSuccess ? Unsubscribed : Error).Broadcast(Count);
	SetReadyToDestroy();
}

void URodinWSListenEventsProxy::Activate()
{
	if (!Server)
	{
		FFrame::KismetExecutionMessage(TEXT("Passed an invalid RodinWSServer to ListenAndBindEvents()."), ELogVerbosity::Error);
		SetReadyToDestroy();
		return;
	}

	Server->OnRodinWSMessage.AddDynamic(this, &URodinWSListenEventsProxy::InternalOnMessage);
	Server->OnRodinWSPing.AddDynamic(this, &URodinWSListenEventsProxy::InternalOnMessage);
	Server->OnRodinWSPong.AddDynamic(this, &URodinWSListenEventsProxy::InternalOnMessage);
	Server->OnRodinWSClosed.AddDynamic(this, &URodinWSListenEventsProxy::InternalOnClientLeft);
	Server->OnRodinWSOpened.AddDynamic(this, &URodinWSListenEventsProxy::InternalOnNewClient);

	Server->OnRodinWSServerClosed.AddDynamic(this, &URodinWSListenEventsProxy::InternalOnClosed);

	Server->Listen(MoveTemp(URI), Port, FOnRodinWSServerListening::CreateUObject(this, &ThisClass::InternalOnListening));

}

URodinWSListenEventsProxy* URodinWSListenEventsProxy::ListenAndBindEvents(URodinWSServer* RodinWSServer)
{
	URodinWSListenEventsProxy* const Proxy = NewObject<URodinWSListenEventsProxy>();

	Proxy->Server = RodinWSServer;
	Proxy->URI = baseURI;
	Proxy->Port = basePort;

	return Proxy;
}

void URodinWSListenEventsProxy::InternalOnListening(const bool bSuccess)
{
	if (!bSuccess)
	{
		OnError.Broadcast(nullptr, nullptr, TEXT("Failed to listen"), ERodinWSOpCode::None, 0);
		SetReadyToDestroy();
		return;
	}

	OnListening.Broadcast(Server, nullptr, TEXT("Listening"), ERodinWSOpCode::None, 0);
}

void URodinWSListenEventsProxy::InternalOnMessage(URodinWS* Socket, const FString& Message, ERodinWSOpCode Code)
{
	OnMessage.Broadcast(Server, Socket, Message, Code, 0);
}

void URodinWSListenEventsProxy::InternalOnNewClient(URodinWS* Socket)
{
	OnNewClient.Broadcast(Server, Socket, TEXT(""), ERodinWSOpCode::None, 0);
}

void URodinWSListenEventsProxy::InternalOnClientLeft(class URodinWS* Socket, int32 Code, const FString& Message)
{
	OnClientLeft.Broadcast(Server, Socket, Message, ERodinWSOpCode::CLOSE, Code);
}

void URodinWSListenEventsProxy::InternalOnClosed()
{
	OnServerClosed.Broadcast(Server, nullptr, TEXT(""), ERodinWSOpCode::CLOSE, 0);
	SetReadyToDestroy();
}

void URodinWSCreateProxy::Activate()
{
	Server = URodinWSServer::CreateRodinWSServer();

	Server->SetCompression(Compression);
	Server->SetSendPingsAutomatically(bSendPingsAutomatically);
	Server->SetIdleTimeout(IdleTimeout);

	Server->OnRodinWSMessage.AddDynamic(this, &URodinWSCreateProxy::InternalOnMessage);
	Server->OnRodinWSPing.AddDynamic(this, &URodinWSCreateProxy::InternalOnMessage);
	Server->OnRodinWSPong.AddDynamic(this, &URodinWSCreateProxy::InternalOnMessage);
	Server->OnRodinWSClosed.AddDynamic(this, &URodinWSCreateProxy::InternalOnClientLeft);
	Server->OnRodinWSOpened.AddDynamic(this, &URodinWSCreateProxy::InternalOnNewClient);

	Server->OnRodinWSServerClosed.AddDynamic(this, &URodinWSCreateProxy::InternalOnClosed);

	Server->Listen(MoveTemp(URI), Port, FOnRodinWSServerListening::CreateUObject(this, &URodinWSCreateProxy::InternalOnListening));
}

URodinWSCreateProxy* URodinWSCreateProxy::StartRodinWSServer(const FString& URI, const int32 Port, 
	ERodinWSCompressOptions Compression, bool bInSendPingsAutomatically, const int64 InIdleTimeout)
{
	URodinWSCreateProxy* const Proxy = NewObject<URodinWSCreateProxy>();

	Proxy->URI = URI;
	Proxy->Port = Port;
	Proxy->Compression = Compression;
	Proxy->bSendPingsAutomatically = bInSendPingsAutomatically;
	Proxy->IdleTimeout = InIdleTimeout;

	return Proxy;
}

void URodinWSCreateProxy::InternalOnListening(const bool bSuccess)
{
	if (!bSuccess)
	{
		OnError.Broadcast(nullptr, nullptr, TEXT("Failed to listen"), ERodinWSOpCode::None, 0);
		SetReadyToDestroy();
		return;
	}

	OnListening.Broadcast(Server, nullptr, TEXT("Listening"), ERodinWSOpCode::None, 0);
}

void URodinWSCreateProxy::InternalOnMessage(class URodinWS* Socket, const FString& Message, ERodinWSOpCode Code)
{
	OnMessage.Broadcast(Server, Socket, Message, Code, 0);
}

void URodinWSCreateProxy::InternalOnNewClient(class URodinWS* Socket)
{
	OnNewClient.Broadcast(Server, Socket, TEXT(""), ERodinWSOpCode::None, 0);
}

void URodinWSCreateProxy::InternalOnClientLeft(class URodinWS* Socket, int32 Code, const FString& Message)
{
	OnClientLeft.Broadcast(Server, Socket, Message, ERodinWSOpCode::CLOSE, Code);
}

void URodinWSCreateProxy::InternalOnClosed()
{
	OnServerClosed.Broadcast(Server, nullptr, TEXT(""), ERodinWSOpCode::CLOSE, 0);
	SetReadyToDestroy();
}

