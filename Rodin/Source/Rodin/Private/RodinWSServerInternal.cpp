// Fill out your copyright notice in the Description page of Project Settings.


#include "RodinWSServerInternal.h"

FRodinWSServerSharedRessourcesManager::FRodinWSServerSharedRessourcesManager()
	: ListenSocket(nullptr)
	, ServerStatus(ERodinWSServerState::Closed)
	, Loop(nullptr)
{
}

IRodinWSServerInternal::IRodinWSServerInternal()
	: MaxLifetime(DefaultMaxLifetime)
	, MaxPayloadLength(DefaultMaxPayloadLength)
	, IdleTimeout(DefaultIdleTimeout)
	, MaxBackPressure(DefaultMaxBackPressure)
	, bCloseOnBackpressureLimit(false)
	, bResetIdleTimeoutOnSend(false)
	, bSendPingsAutomatically(true)
	, Compression(ERodinWSCompressOptions::DISABLED)
	, SharedRessources(MakeShared<FRodinWSServerSharedRessourcesManager, ESPMode::ThreadSafe>())
{
}


void IRodinWSServerInternal::SetMaxLifetime(const int64 InMaxLifetime)
{
	MaxLifetime = InMaxLifetime;
}

void IRodinWSServerInternal::SetMaxPayloadLength(const int64 InMaxLifetime)
{
	MaxPayloadLength = InMaxLifetime;
}

void IRodinWSServerInternal::SetIdleTimeout(const int64 InIdleTimeout)
{
	IdleTimeout = InIdleTimeout;
}

void IRodinWSServerInternal::SetMaxBackPressure(const int64 InMaxBackPressure)
{
	MaxBackPressure = InMaxBackPressure;
}

void IRodinWSServerInternal::SetCloseOnBackpressureLimit(const bool bInCloseOnBackpressureLimit)
{
	bCloseOnBackpressureLimit = bInCloseOnBackpressureLimit;
}

void  IRodinWSServerInternal::SetResetIdleTimeoutOnSend(const bool bInResetIdleTimeoutOnSend)
{
	bResetIdleTimeoutOnSend = bInResetIdleTimeoutOnSend;
}

void  IRodinWSServerInternal::SetSendPingsAutomatically(const bool bInSendPingsAutomatically)
{
	bSendPingsAutomatically = bInSendPingsAutomatically;
}

void IRodinWSServerInternal::SetCompression(ERodinWSCompressOptions InCompression)
{
	Compression = InCompression;
}

ERodinWSServerState IRodinWSServerInternal::GetServerState() const
{
	return SharedRessources->ServerStatus;
}

void IRodinWSServerInternal::SetSSLOptions(
	FString&& InKeyFile,
	FString&& InCertFile,
	FString&& InPassPhrase,
	FString&& InDhParamsFile,
	FString&& InCaFileName)
{
	KeyFile = MoveTemp(InKeyFile);
	CertFile = MoveTemp(InCertFile);
	PassPhrase = MoveTemp(InPassPhrase);
	DhParamsFile = MoveTemp(InDhParamsFile);
	CaFileName = MoveTemp(InCaFileName);
}

namespace NRodinWSUtils
{
	uWS::CompressOptions Convert(const ERodinWSCompressOptions Option)
	{
		switch (Option)
		{
#define MakeCase(type) \
			case ERodinWSCompressOptions:: type: return uWS:: type;
			MakeCase(DISABLED)
				MakeCase(SHARED_COMPRESSOR)
				MakeCase(DEDICATED_COMPRESSOR_3KB)
				MakeCase(DEDICATED_COMPRESSOR_4KB)
				MakeCase(DEDICATED_COMPRESSOR_8KB)
				MakeCase(DEDICATED_COMPRESSOR_16KB)
				MakeCase(DEDICATED_COMPRESSOR_32KB)
				MakeCase(DEDICATED_COMPRESSOR_64KB)
				MakeCase(DEDICATED_COMPRESSOR_128KB)
				MakeCase(DEDICATED_COMPRESSOR_256KB)
				MakeCase(DEDICATED_COMPRESSOR)
#undef MakeCase
		}

		return uWS::CompressOptions::DISABLED;
	}

	ERodinWSOpCode Convert(const uWS::OpCode Code)
	{
		switch (Code)
		{
#define MakeCase(type) \
			case uWS::OpCode:: type : return ERodinWSOpCode:: type ; 
			MakeCase(TEXT)
				MakeCase(BINARY)
				MakeCase(CLOSE)
				MakeCase(PING)
				MakeCase(PONG)
#undef MakeCase
		}

		return ERodinWSOpCode::None;
	}

	uWS::OpCode Convert(const ERodinWSOpCode Code)
	{
		switch (Code)
		{
#define MakeCase(type) \
			case ERodinWSOpCode:: type : return uWS::OpCode:: type ; 
			MakeCase(TEXT)
				MakeCase(BINARY)
				MakeCase(CLOSE)
				MakeCase(PING)
				MakeCase(PONG)
#undef MakeCase
		}

		return uWS::OpCode::TEXT;
	}

	FString Convert(const std::string_view& Value)
	{
		const FUTF8ToTCHAR Converter(Value.data(), Value.size());
		return FString(Converter.Length(), Converter.Get());
	}
};

