// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Rodin.h"
#include "RodinWSServer.h"
#include "RodinWSServerNode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMultiListen);

UCLASS()
class URodinWSListenProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable)
	FMultiListen Listening;

	UPROPERTY(BlueprintAssignable)
	FMultiListen OnError;

public:

	UFUNCTION(BlueprintCallable, Category = "RodinWS|Server", meta = (BlueprintInternalUseOnly = "true"))
	static URodinWSListenProxy* Listen(URodinWSServer* RodinWSServer,
		FString Host = TEXT("0.0.0.0"),FString URI = TEXT("/*"),const int32 Port = 8080);

	virtual void Activate();

private:
	UFUNCTION()
	void OnTaskOver(const bool bSuccess);

private:
	UPROPERTY()
	URodinWSServer* Server;
	int32 Port;
	FString Host;
	FString URI;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMultiPublish);
UCLASS()
class URodinWSPublishProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable)
	FMultiPublish Published;

	UPROPERTY(BlueprintAssignable)
	FMultiPublish Error;

public:
	virtual void Activate();

	UFUNCTION(BlueprintCallable, Category = "RodinWS|Server", meta = (BlueprintInternalUseOnly = "true"))
	static URodinWSPublishProxy* Publish(URodinWS* RodinWS, const FString& Topic, const FString& Message);

private:
	void OnTaskOver(bool bSuccess);

private:
	FString Topic;
	FString Message;

	UPROPERTY()
	URodinWS* RodinWS;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FMultiSubscibe,
	int32, SubscriberCount
);

UCLASS()
class URodinWSSubscribeProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable)
	FMultiSubscibe Subscribed;

	UPROPERTY(BlueprintAssignable)
	FMultiSubscibe Error;

	virtual void Activate();

	UFUNCTION(BlueprintCallable, Category = "RodinWS|Server", meta = (BlueprintInternalUseOnly = "true"))
	static URodinWSSubscribeProxy* Subscribe(URodinWS* RodinWS, const FString& Topic);

private:
	void OnTaskOver(bool bSuccess, int32 Count);

	FString Topic;

	UPROPERTY()
	URodinWS* RodinWS;
};

UCLASS()
class URodinWSUnsubscribeProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	
	UPROPERTY(BlueprintAssignable)
	FMultiSubscibe Unsubscribed;

	UPROPERTY(BlueprintAssignable)
	FMultiSubscibe Error;

	virtual void Activate();

	UFUNCTION(BlueprintCallable, Category = "RodinWS|Server", meta = (BlueprintInternalUseOnly = "true"))
	static URodinWSUnsubscribeProxy* Unsubscribe(URodinWS* RodinWS, const FString& Topic);

private:
	void OnTaskOver(bool, int32);

	FString Topic;

	UPROPERTY()
	URodinWS* RodinWS;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FWSServerPin,
				URodinWSServer*, RodinWSServer,
				class URodinWS*, ClientRodinWS,
				const FString&, Message,
				ERodinWSOpCode, OpCode,
				int32, CloseCode
);


UCLASS()
class URodinWSListenEventsProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
	FWSServerPin OnListening;

	UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
	FWSServerPin OnError;

	UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
	FWSServerPin OnNewClient;

	UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
	FWSServerPin OnMessage;

	UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
	FWSServerPin OnClientLeft;

	UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
	FWSServerPin OnServerClosed;

public:
	virtual void Activate();

	UFUNCTION(BlueprintCallable, Category = "RodinWS|Server|allEvents", meta = (BlueprintInternalUseOnly = "true"))
	static URodinWSListenEventsProxy* ListenAndBindEvents(
		URodinWSServer* RodinWSServer);

private:
	UFUNCTION()
	void InternalOnListening(const bool bSuccess);
	UFUNCTION()
	void InternalOnMessage(class URodinWS* Socket, const FString& Message, ERodinWSOpCode Code);
	UFUNCTION()
	void InternalOnNewClient(class URodinWS* Socket);
	UFUNCTION()
	void InternalOnClientLeft(class URodinWS* Socket, int32 Code, const FString& Message);
	UFUNCTION()
	void InternalOnClosed();

private:
	UPROPERTY()
	URodinWSServer* Server;

	FString URI;
	int32 Port;
};

UCLASS()
class URodinWSCreateProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable)
	FWSServerPin OnListening;

	UPROPERTY(BlueprintAssignable)
	FWSServerPin OnError;

	UPROPERTY(BlueprintAssignable)
	FWSServerPin OnNewClient;

	UPROPERTY(BlueprintAssignable)
	FWSServerPin OnMessage;

	UPROPERTY(BlueprintAssignable)
	FWSServerPin OnClientLeft;

	UPROPERTY(BlueprintAssignable)
	FWSServerPin OnServerClosed;

public:
	virtual void Activate();

	UFUNCTION(BlueprintCallable, Category = "RodinWS|Server", meta = (BlueprintInternalUseOnly = "true"))
	static URodinWSCreateProxy* StartRodinWSServer(
		const FString& URI = TEXT("/*"),
		const int32 Port = 8080,
		ERodinWSCompressOptions Compression = ERodinWSCompressOptions::DISABLED,
		bool bInSendPingsAutomatically = true,
		const int64 InIdleTimeout = 120);

private:
	UFUNCTION()
	void InternalOnListening(const bool bSuccess);
	UFUNCTION()
	void InternalOnMessage(class URodinWS* Socket, const FString& Message, ERodinWSOpCode Code);
	UFUNCTION()
	void InternalOnNewClient(class URodinWS* Socket);
	UFUNCTION()
	void InternalOnClientLeft(class URodinWS* Socket, int32 Code, const FString& Message);
	UFUNCTION()
	void InternalOnClosed();

private:
	UPROPERTY()
	URodinWSServer* Server;

	FString URI;
	int32 Port;
	ERodinWSCompressOptions Compression;
	bool bSendPingsAutomatically;
	int64 IdleTimeout;
};
