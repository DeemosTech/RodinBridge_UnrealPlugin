// Copyright Pandores Marketplace 2021. All Righst Reserved.
#include "RodinWSClient.h"
#include "RodinWSServer.h"
#include "RodinWSServerInternal.h"

void URodinWS::Send(const FString& Message)
{
	Send(FString(Message));
}

void URodinWS::Send(FString&& Message)
{
	auto Proxy = SocketProxy.Pin();
	if (Proxy)
	{
		Proxy->SendMessage(MoveTemp(Message));
	}
}

void URodinWS::Close()
{
	auto Proxy = SocketProxy.Pin();
	if (Proxy)
	{
		Proxy->Close();
	}
}

void URodinWS::End(const int32 Code, const FString& Message)
{
	End(Code, FString(Message));
}

void URodinWS::End(const int32 Code, FString&& Message)
{
	auto Proxy = SocketProxy.Pin();
	if (Proxy)
	{
		Proxy->End(Code, MoveTemp(Message));
	}
}

void URodinWS::Ping(const FString& Message)
{
	Ping(FString(Message));
}

void URodinWS::Ping(FString&& Message)
{
	auto Proxy = SocketProxy.Pin();
	if (Proxy)
	{
		Proxy->Ping(MoveTemp(Message));
	}
}

void URodinWS::Pong(const FString& Message)
{
	Pong(FString(Message));
}

void URodinWS::Pong(FString&& Message)
{
	auto Proxy = SocketProxy.Pin();
	if (Proxy)
	{
		Proxy->Pong(MoveTemp(Message));
	}
}

bool URodinWS::IsConnected() const
{
	if (!SocketProxy.IsValid())
	{
		return false;
	}

	auto Proxy = SocketProxy.Pin();
	return Proxy && Proxy->IsSocketValid();
}

void URodinWS::Subscribe(const FString& Topic, const FOnRodinWSSubscribed& Callback, bool bNonStrict)
{
	Subscribe(FString(Topic), FOnRodinWSSubscribed(Callback), bNonStrict);
}

void URodinWS::Subscribe(const FString& Topic, FOnRodinWSSubscribed&& Callback, bool bNonStrict)
{
	Subscribe(FString(Topic), MoveTemp(Callback), bNonStrict);
}

void URodinWS::Subscribe(FString&& Topic, const FOnRodinWSSubscribed& Callback, bool bNonStrict)
{
	Subscribe(MoveTemp(Topic), FOnRodinWSSubscribed(Callback), bNonStrict);
}

void URodinWS::Subscribe(FString&& Topic, FOnRodinWSSubscribed&& Callback, bool bNonStrict)
{
	auto Proxy = SocketProxy.Pin();
	if (Proxy)
	{
		Proxy->Subscribe(MoveTemp(Topic), MoveTemp(Callback), bNonStrict);
	}
	else
	{
		Callback.ExecuteIfBound(false, 0);
	}
}

void URodinWS::Unsubscribe(const FString& Topic, const FOnRodinWSSubscribed& Callback, bool bNonStrict)
{
	Unsubscribe(FString(Topic), FOnRodinWSSubscribed(Callback), bNonStrict);
}

void URodinWS::Unsubscribe(const FString& Topic, FOnRodinWSSubscribed&& Callback, bool bNonStrict)
{
	Unsubscribe(FString(Topic), MoveTemp(Callback), bNonStrict);
}

void URodinWS::Unsubscribe(FString&& Topic, const FOnRodinWSSubscribed& Callback, bool bNonStrict)
{
	Unsubscribe(MoveTemp(Topic), FOnRodinWSSubscribed(Callback), bNonStrict);
}

void URodinWS::Unsubscribe(FString&& Topic, FOnRodinWSSubscribed&& Callback, bool bNonStrict)
{
	auto Proxy = SocketProxy.Pin();
	if (Proxy)
	{
		Proxy->Unsubscribe(MoveTemp(Topic), MoveTemp(Callback), bNonStrict);
	}
	else
	{
		Callback.ExecuteIfBound(false, 0);
	}
}

void URodinWS::Publish(const FString& Topic, const FString& Message, const FOnRodinWSPublished& Callback)
{
	Publish(FString(Topic), FString(Message), FOnRodinWSPublished(Callback));
}

void URodinWS::Publish(FString&& Topic, FString&& Message, FOnRodinWSPublished&& Callback)
{
	auto Proxy = SocketProxy.Pin();
	if (Proxy)
	{
		Proxy->Publish(MoveTemp(Topic), MoveTemp(Message), MoveTemp(Callback));
	}
	else
	{
		Callback.ExecuteIfBound(false);
	}
}

void URodinWS::Send_Blueprint(const TArray<uint8>& Data)
{
	Send(Data);
}

void URodinWS::Send(const TArray<uint8>& Data)
{
	Send(CopyTemp(Data));
}

void URodinWS::Send(TArray<uint8>&& Data)
{
	auto Proxy = SocketProxy.Pin();
	if (Proxy)
	{
		Proxy->SendData(MoveTemp(Data));
	}
}
