// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4668)
#	pragma warning(disable: 4582)
#endif

THIRD_PARTY_INCLUDES_START
#include <thread>
#include <string_view>
THIRD_PARTY_INCLUDES_END

#if PLATFORM_WINDOWS
#	pragma warning(pop)
#endif

#include "Async/Async.h"
#include "RodinWSServer.h"
#include "RodinWSInternal.h"
#include "Rodin.h"

DECLARE_DELEGATE_OneParam(
	FOnOpened,
	class URodinWS*
);

DECLARE_DELEGATE_ThreeParams(
	FOnMessage,
	class URodinWS*,
	TArray<uint8>,
	ERodinWSOpCode
);

DECLARE_DELEGATE_ThreeParams(
	FOnClosed,
	class URodinWS*,
	const int32,
	const FString&
);

DECLARE_DELEGATE(
	FOnServerClosed
);

#define DEFINE_TYPES()															\
	using FRodinWS                    = TRodinWS<bSSL>;						\
	using FRodinWSProxy               = TRodinWSProxy<bSSL>;				\
	using FRodinWSProxyPtr            = TRodinWSProxyPtr<bSSL>;				\
	using FRodinWSServerInternal      = TRodinWSServerInternal<bSSL>;		\
	using FRodinWSData                = TRodinWSData<bSSL>;					\
	using FRodinWSServerInternalPtr	= TRodinWSServerInternalPtr <bSSL>;	\
	using uWSApp						= uWS::TemplatedApp<bSSL>;

#define DEFINE_FRIEND(Class)	\
	template<bool b_SSL>		\
	friend class Class;

template<bool bSSL>
class TRodinWSServerInternal;

template<bool bSSL>
class TRodinWSData;

template<bool bSSL>
using TRodinWS = uWS::WebSocket<bSSL, true, TRodinWSData<bSSL>>;

template<bool bSSL>
class TRodinWSProxy;

template<bool bSSL>
using TRodinWSProxyPtr = TSharedPtr<TRodinWSProxy<bSSL>, ESPMode::ThreadSafe>;

template<bool bSSL>
using TRodinWSServerInternalPtr = TSharedPtr<TRodinWSServerInternal<bSSL>, ESPMode::ThreadSafe>;

class FRodinWSServerSharedRessourcesManager
{
private:
	using FListenSocketPtr = struct us_listen_socket_t*;
	using FAtomicStatus = TAtomic<ERodinWSServerState>;

public:
	FRodinWSServerSharedRessourcesManager();

	FListenSocketPtr ListenSocket;

	FAtomicStatus ServerStatus;

	FCriticalSection LoopAccess;

	uWS::Loop* Loop;
};
using FSharedRessourcesPtr = TSharedPtr<class FRodinWSServerSharedRessourcesManager, ESPMode::ThreadSafe>;

class IRodinWSProxy
{
public:
	virtual void SendMessage(FString&& Message) = 0;
	virtual void SendData(TArray<uint8>&& Data) = 0;
	virtual void Close() = 0;
	virtual void End(const int32 OpCode, FString&& Message) = 0;
	virtual void Ping(FString&& Message) = 0;
	virtual void Pong(FString&& Message) = 0;
	virtual bool IsSocketValid() const = 0;
	virtual void Subscribe(FString&& Topic, FOnRodinWSSubscribed&& Callback, bool bNonStrict) = 0;
	virtual void Unsubscribe(FString&& Topic, FOnRodinWSSubscribed&& Callback, bool bNonStrict) = 0;
	virtual void Publish(FString&& Topic, FString&& Message, FOnRodinWSPublished&& Callback) = 0;
};

template<bool bSSL>
class TRodinWSProxy final :
	public IRodinWSProxy,
	public TSharedFromThis<TRodinWSProxy<bSSL>, ESPMode::ThreadSafe>
{
private:
	DEFINE_TYPES();
	DEFINE_FRIEND(TRodinWSData);;
	DEFINE_FRIEND(TRodinWSServerInternal);

public:
	TRodinWSProxy();
	TRodinWSProxy(const TRodinWSProxy&) = delete;

	void OnOpen(FRodinWSServerInternalPtr InServer, FRodinWS* RawRodinWS, FOnOpened UserCallback);

	void OnMessage(std::string_view Message, uWS::OpCode Code, FOnMessage UserCallback);
	void OnClosed(const int Code, const std::string_view Message, FOnClosed UserCallback);
	void OnPing(const std::string_view Message, FOnMessage UserCallback);
	void OnPong(const std::string_view Message, FOnMessage UserCallback);

	~TRodinWSProxy();

	virtual void SendMessage(FString&& Message) override;
	virtual void SendData(TArray<uint8>&& Data) override;
	virtual void Close() override;
	virtual void End(const int32 OpCode, FString&& Message) override;
	virtual void Ping(FString&& Message) override;
	virtual void Pong(FString&& Message) override;
	virtual void Subscribe(FString&& Topic, FOnRodinWSSubscribed&& Callback, bool bNonStrict) override;
	virtual void Unsubscribe(FString&& Topic, FOnRodinWSSubscribed&& Callback, bool bNonStrict) override;
	virtual void Publish(FString&& Topic, FString&& Message, FOnRodinWSPublished&& Callback) override;

	virtual bool IsSocketValid() const;

private:
	void ExecuteOnServerThread(TUniqueFunction<void(FRodinWS*)> Function);

	void SendInternal(FString&& Message, const uWS::OpCode Code);

private:
	FRodinWS* RawRodinWS;

	TAtomic<bool> bIsSocketValid;

	TStrongObjectPtr<class URodinWS> RodinWS;

	TWeakPtr<FRodinWSServerInternal, ESPMode::ThreadSafe> RodinWSServer;
};

template<bool bSSL>
class TRodinWSData
{
private:
	DEFINE_TYPES();

public:
	TRodinWSData();
	TRodinWSData(TRodinWSData&& Other);
	TRodinWSData(const TRodinWSData&) = delete;

	~TRodinWSData();

	FRodinWSProxyPtr GetProxy();

	void SetSocket(FRodinWS* const InRodinWS);

	void OnClosed();

private:
	FRodinWSProxyPtr Proxy;

	FRodinWS* RodinWS;
};

class IRodinWSServerInternal
{
private:
	using FThreadPtr = TUniquePtr<std::thread>;

public:
	IRodinWSServerInternal();

	virtual void Listen(FString&& Host, FString&& URI, const uint16 Port, FOnRodinWSServerListening&& Callback,
		FOnServerClosed&& OnClosed) = 0;
	virtual void Close() = 0;
	virtual void Publish(FString&& Topic, FString&& Message, ERodinWSOpCode OpCode) = 0;

	virtual ~IRodinWSServerInternal() = default;

public:
	FORCEINLINE FOnOpened& OnOpened() { return OnOpenedEvent; }
	FORCEINLINE FOnMessage& OnMessage() { return OnMessageEvent; }
	FORCEINLINE FOnMessage& OnPong() { return OnPongEvent; }
	FORCEINLINE FOnMessage& OnPing() { return OnPingEvent; }
	FORCEINLINE FOnClosed& OnClosed() { return OnClosedEvent; }

protected:
	FOnOpened  OnOpenedEvent;
	FOnMessage OnMessageEvent;
	FOnMessage OnPongEvent;
	FOnMessage OnPingEvent;
	FOnClosed  OnClosedEvent;

public:
	void SetMaxLifetime(const int64 InMaxLifetime);
	void SetMaxPayloadLength(const int64 InMaxLifetime);
	void SetIdleTimeout(const int64 InIdleTimeout);
	void SetMaxBackPressure(const int64 InMaxBackPressure);
	void SetCloseOnBackpressureLimit(const bool bInCloseOnBackpressureLimit);
	void SetResetIdleTimeoutOnSend(const bool bInResetIdleTimeoutOnSend);
	void SetSendPingsAutomatically(const bool bInSendPingsAutomatically);
	void SetCompression(ERodinWSCompressOptions InCompression);

	ERodinWSServerState GetServerState() const;

	void SetSSLOptions(FString&& InKeyFile, FString&& InCertFile, FString&& InPassPhrase,
		FString&& InDhParamsFile, FString&& InCaFileName);

protected:
	int64 MaxLifetime;
	int64 MaxPayloadLength;
	int64 IdleTimeout;
	int64 MaxBackPressure;
	bool  bCloseOnBackpressureLimit;
	bool  bResetIdleTimeoutOnSend;
	bool  bSendPingsAutomatically;
	ERodinWSCompressOptions Compression;

	FString KeyFile;
	FString CertFile;
	FString PassPhrase;
	FString DhParamsFile;
	FString CaFileName;

protected:
	FThreadPtr Thread;

	const FSharedRessourcesPtr SharedRessources;
};

template<bool bSSL>
class TRodinWSServerInternal final
	: public TSharedFromThis<TRodinWSServerInternal<bSSL>, ESPMode::ThreadSafe>
	, public IRodinWSServerInternal
{
private:
	DEFINE_TYPES();
	DEFINE_FRIEND(TRodinWSProxy);

public:
	TRodinWSServerInternal();
	~TRodinWSServerInternal();

	virtual void Listen(FString&& Host, FString&& URI, const uint16 Port, FOnRodinWSServerListening&& Callback, FOnServerClosed&& OnClosed);
	virtual void Close();
	virtual void Publish(FString&& Topic, FString&& Message, ERodinWSOpCode OpCode);

private:
	uWSApp App;
};

namespace NRodinWSUtils
{
	uWS::CompressOptions Convert(const ERodinWSCompressOptions Option);
	ERodinWSOpCode     Convert(const uWS::OpCode Code);
	uWS::OpCode          Convert(const ERodinWSOpCode Code);
	FString              Convert(const std::string_view& Value);
}

#if CPP
#	include "RodinWSServerInternal.inl"
#endif

#undef DEFINE_FRIEND
#undef DEFINE_TYPES


