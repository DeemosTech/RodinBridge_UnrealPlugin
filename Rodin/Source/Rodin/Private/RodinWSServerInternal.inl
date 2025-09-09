// Copyright Pandores Marketplace 2022. All Rights Reserved.

// Default 
static constexpr int64 DefaultMaxLifetime		= 0;
static constexpr int64 DefaultMaxPayloadLength	= 256 * 1024;
static constexpr int64 DefaultIdleTimeout		= 120;
static constexpr int64 DefaultMaxBackPressure	= 256 * 1024;

///////////////////////////////////////////////////////////////
// FRodinWSData

template<bool bSSL>
TRodinWSData<bSSL>::TRodinWSData()
	: Proxy(MakeShared<TRodinWSProxy<bSSL>, ESPMode::ThreadSafe>())
	, RodinWS(nullptr)
{
}

template<bool bSSL>
TRodinWSData<bSSL>::TRodinWSData(TRodinWSData&& Other)
	: Proxy(MoveTemp(Other.Proxy))
	, RodinWS(Other.RodinWS)
{
	Other.RodinWS = nullptr;
}

template<bool bSSL>
TRodinWSProxyPtr<bSSL> TRodinWSData<bSSL>::GetProxy()
{
	return Proxy;
}

template<bool bSSL>
TRodinWSData<bSSL>::~TRodinWSData()
{
	// Close wasn't graceful.
	if (Proxy)
	{
		Proxy->OnClosed(-1, "Server Closed", FOnClosed());
		OnClosed();
	}
}

template<bool bSSL>
void TRodinWSData<bSSL>::SetSocket(FRodinWS* const InRodinWS)
{
	// Must be called once and only once.
	check(!RodinWS);

	RodinWS = InRodinWS;
}

template<bool bSSL>
void TRodinWSData<bSSL>::OnClosed()
{
	Proxy->bIsSocketValid = false;
	Proxy     = nullptr;
	RodinWS = nullptr;
}


///////////////////////////////////////////////////////////////
// FRodinWSProxy

template<bool bSSL>
TRodinWSProxy<bSSL>::TRodinWSProxy()
	: RawRodinWS(nullptr)
	, bIsSocketValid(true)
{
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::OnOpen(FRodinWSServerInternalPtr InServer, TRodinWS<bSSL>* InRawRodinWS,
	FOnOpened UserCallback)
{
	RawRodinWS    = InRawRodinWS;
	RodinWSServer = InServer;

	AsyncTask(ENamedThreads::GameThread,
		[
			Self = this->AsShared(),
			UserCallback = MoveTemp(UserCallback)
		]() -> void
	{
		URodinWS* const NewRodinWS = NewObject<URodinWS>();

		Self->RodinWS.Reset(NewRodinWS);

		NewRodinWS->SocketProxy = Self;

		UserCallback.ExecuteIfBound(NewRodinWS);
	});
}


template<bool bSSL>
void TRodinWSProxy<bSSL>::OnMessage(std::string_view Message, uWS::OpCode Code, FOnMessage UserCallback)
{
	AsyncTask(ENamedThreads::GameThread,
		[
			Self         = this->AsShared(),
			BinMessage   = TArray<uint8>((uint8*)Message.data(), Message.size()),
			UserCallback = MoveTemp(UserCallback),
			Code
		]() -> void
	{
		check(Self->RodinWS.IsValid());

		UserCallback.ExecuteIfBound(Self->RodinWS.Get(), BinMessage, NRodinWSUtils::Convert(Code));
	});
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::OnClosed(const int Code, const std::string_view Message, FOnClosed UserCallback)
{
	bIsSocketValid = false;
	RawRodinWS = nullptr;

	AsyncTask(ENamedThreads::GameThread,
		[
			Self         = this->AsShared(),
			StrMessage   = NRodinWSUtils::Convert(Message),
			UserCallback = MoveTemp(UserCallback),
			Code
		]() -> void
	{		
		check(Self->RodinWS.IsValid());

		UserCallback.ExecuteIfBound(Self->RodinWS.Get(), Code, StrMessage);

		if (Self->RodinWS)
		{
			Self->RodinWS->SocketProxy = nullptr;
		}
	});
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::OnPing(const std::string_view Message, FOnMessage UserCallback)
{
	OnMessage(Message, uWS::OpCode::PING, MoveTemp(UserCallback));
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::OnPong(const std::string_view Message, FOnMessage UserCallback)
{
	OnMessage(Message, uWS::OpCode::PONG, MoveTemp(UserCallback));
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::ExecuteOnServerThread(TUniqueFunction<void(TRodinWS<bSSL>*)> Function)
{
	if (!RodinWSServer.IsValid() || !bIsSocketValid)
	{
		return;
	}

	FRodinWSServerInternalPtr Internal = RodinWSServer.Pin();
	if (!Internal)
	{
		return;
	}

	auto SharedRessources = Internal->SharedRessources;

	auto ServerThreadWork = [Self = this->AsShared(), Function = MoveTemp(Function)]() -> void
	{
		if (Self->RawRodinWS)
		{
			Function(Self->RawRodinWS);
		}
	};

	{
		FScopeLock Lock(&SharedRessources->LoopAccess);

		if (SharedRessources->Loop)
		{
			SharedRessources->Loop->defer(MoveTemp(ServerThreadWork));
		}
	}
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::SendInternal(FString&& Message, const uWS::OpCode Code)
{
	ExecuteOnServerThread([Message = MoveTemp(Message), Code](FRodinWS* Socket) -> void
	{
		Socket->send(TCHAR_TO_UTF8(*Message), Code); 
	});
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::SendMessage(FString&& Message)
{
	SendInternal(MoveTemp(Message), uWS::OpCode::TEXT);
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::SendData(TArray<uint8>&& Data)
{
	ExecuteOnServerThread([Data = MoveTemp(Data)](FRodinWS* Socket) -> void
	{
		std::string_view StrData(reinterpret_cast<const char*>(Data.GetData()), Data.Num());
		Socket->send(StrData, uWS::OpCode::BINARY);
	});
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::Close()
{
	ExecuteOnServerThread([](FRodinWS* Socket) -> void
	{
		Socket->close();
	});
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::End(const int32 Code, FString&& Message)
{
	ExecuteOnServerThread([Code, Message = MoveTemp(Message)](FRodinWS* Socket) -> void
	{
		Socket->end(Code, TCHAR_TO_UTF8(*Message));
	});
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::Ping(FString&& Message)
{
	SendInternal(MoveTemp(Message), uWS::OpCode::PING);
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::Pong(FString&& Message)
{
	SendInternal(MoveTemp(Message), uWS::OpCode::PONG);
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::Subscribe(FString&& Topic, FOnRodinWSSubscribed&& Callback, bool bNonStrict)
{
	ExecuteOnServerThread([
		Topic    = MoveTemp(Topic),
		Callback = MoveTemp(Callback),
		bNonStrict
	](FRodinWS* Socket) mutable -> void
	{
		const auto Result = Socket->subscribe(TCHAR_TO_UTF8(*Topic), bNonStrict);

		if (Callback.IsBound())
		{
			AsyncTask(ENamedThreads::GameThread, 
				[Callback = MoveTemp(Callback), bSuccess = Result.second, NumSubscribers = Result.first]() -> void
			{
				Callback.ExecuteIfBound(bSuccess, static_cast<int32>(NumSubscribers));
			});
		}
	});
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::Unsubscribe(FString&& Topic, FOnRodinWSSubscribed&& Callback, bool bNonStrict)
{
	ExecuteOnServerThread([
		Topic    = MoveTemp(Topic),
		Callback = MoveTemp(Callback),
		bNonStrict
	](FRodinWS* Socket) mutable -> void
	{
		const auto Result = Socket->unsubscribe(TCHAR_TO_UTF8(*Topic), bNonStrict);

		if (Callback.IsBound())
		{
			AsyncTask(ENamedThreads::GameThread, 
				[Callback = MoveTemp(Callback), bSuccess = Result.second, NumSubscribers = Result.first]() -> void
			{
				Callback.ExecuteIfBound(bSuccess, static_cast<int32>(NumSubscribers));
			});
		}
	});
}

template<bool bSSL>
void TRodinWSProxy<bSSL>::Publish(FString&& Topic, FString&& Message, FOnRodinWSPublished&& Callback)
{
	ExecuteOnServerThread([
		Topic    = MoveTemp(Topic),
		Message  = MoveTemp(Message),
		Callback = MoveTemp(Callback)
	](FRodinWS* Socket) mutable -> void
	{
		const bool bSuccess = Socket->publish(TCHAR_TO_UTF8(*Topic), TCHAR_TO_UTF8(*Message));

		if (Callback.IsBound())
		{
			AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), bSuccess]() -> void
			{
				Callback.ExecuteIfBound(bSuccess);
			});
		}
	});
}

template<bool bSSL>
bool TRodinWSProxy<bSSL>::IsSocketValid() const
{
	return bIsSocketValid;
}

template<bool bSSL>
TRodinWSProxy<bSSL>::~TRodinWSProxy()
{
	// This proxy should always be destroyed from Game Thread
	// as it holds a strong reference to an UObject.
	check(IsInGameThread());
}


///////////////////////////////////////////////////////////////
// TRodinWSServerInternal

template<bool bSSL>
TRodinWSServerInternal<bSSL>::TRodinWSServerInternal() 
	: IRodinWSServerInternal()
{
}

template<bool bSSL>
TRodinWSServerInternal<bSSL>::~TRodinWSServerInternal()
{
	if (SharedRessources->ServerStatus == ERodinWSServerState::Running)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server wasn't closed. Closing it now."));
		Close();
	}
}

template<bool bSSL>
void TRodinWSServerInternal<bSSL>::Listen(FString&& Host, FString&& URI, const uint16 Port,
	FOnRodinWSServerListening&& Callback, FOnServerClosed&& OnServerClosed)
{
	using FRodinWSBehavior = typename uWS::template TemplatedApp<bSSL>::template WebSocketBehavior<FRodinWSData>;

	ERodinWSServerState Expected = ERodinWSServerState::Closed;
	if (!SharedRessources->ServerStatus.CompareExchange(Expected, ERodinWSServerState::Starting))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Starting server on %s:%d..."), *URI, Port);

	Thread.Reset(new std::thread(
	[
		// Settings
		Compression					= this->Compression,
		MaxPayloadLength			= this->MaxPayloadLength,
		IdleTimeout					= this->IdleTimeout,
		MaxBackPressure				= this->MaxBackPressure,
		bCloseOnBackpressureLimit	= this->bCloseOnBackpressureLimit,
		bResetIdleTimeoutOnSend		= this->bResetIdleTimeoutOnSend,
		bSendPingsAutomatically		= this->bSendPingsAutomatically,
		MaxLifetime					= this->MaxLifetime,

		// SSL options
		KeyFile			= std::string(TCHAR_TO_UTF8(*KeyFile)),
		CertFile		= std::string(TCHAR_TO_UTF8(*CertFile)),
		PassPhrase		= std::string(TCHAR_TO_UTF8(*PassPhrase)),
		DhParamsFile	= std::string(TCHAR_TO_UTF8(*DhParamsFile)),
		CaFileName		= std::string(TCHAR_TO_UTF8(*CaFileName)),

		// Events
		OnOpened		= MoveTemp(this->OnOpenedEvent),
		OnMessage		= MoveTemp(this->OnMessageEvent),
		OnClosed		= MoveTemp(this->OnClosedEvent),
		OnPing			= MoveTemp(this->OnPingEvent),
		OnPong			= MoveTemp(this->OnPongEvent),
		OnServerClosed	= MoveTemp(OnServerClosed),

		// Parameters
		Port,
		URI			= MoveTemp(URI),
		Host		= MoveTemp(Host),
		Callback	= MoveTemp(Callback),

		SharedRessources = this->SharedRessources,
		Server			 = this->AsShared()
	]() mutable -> void
	{			
		FRodinWSBehavior Behavior;

		Behavior.compression				= NRodinWSUtils::Convert(Compression);
		Behavior.maxPayloadLength			= MaxPayloadLength;
		Behavior.idleTimeout				= IdleTimeout;
		Behavior.maxBackpressure			= MaxBackPressure;
		Behavior.closeOnBackpressureLimit	= bCloseOnBackpressureLimit;
		Behavior.resetIdleTimeoutOnSend		= bResetIdleTimeoutOnSend;
		Behavior.sendPingsAutomatically		= bSendPingsAutomatically;
		Behavior.maxLifetime				= MaxLifetime;

		// We let default upgrade.
		//Behavior.upgrade = [](uWS::HttpResponse<bSSL>* Response, uWS::HttpRequest* Request, struct us_socket_context_t* Context) -> void
		//{
		//	UE_LOG(LogTemp, Verbose, TEXT("New connection received for upgrade."));
		//
		//	Response->upgrade<FRodinWSData>(FRodinWSData(),
		//		Request->getHeader("sec-RodinWS-key"),
		//		Request->getHeader("sec-RodinWS-protocol"),
		//		Request->getHeader("sec-RodinWS-extensions"),
		//		Context);
		//};

		Behavior.open = [&Server, &OnOpened](FRodinWS* Socket) -> void
		{
			UE_LOG(LogTemp, Verbose, TEXT("New RodinWS connection opened."));

			FRodinWSData* const SocketData = Socket->getUserData();
			SocketData->SetSocket(Socket);
			SocketData->GetProxy()->OnOpen(Server, Socket, OnOpened);
		};

		Behavior.message = [&OnMessage](FRodinWS* Socket, std::string_view Message, uWS::OpCode Code) -> void
		{
			FRodinWSData* const SocketData = Socket->getUserData();
			SocketData->GetProxy()->OnMessage(Message, Code, OnMessage);
		};

		//Behavior.drain = [](FRodinWS* Socket) -> void
		//{
		//
		//};

		if (!bSendPingsAutomatically)
		{
			Behavior.ping = [&OnPing](FRodinWS* Socket, std::string_view Data) -> void
			{
				FRodinWSData* const SocketData = Socket->getUserData();
				SocketData->GetProxy()->OnPing(Data, OnPing);
			};

			Behavior.pong = [&OnPong](FRodinWS* Socket, std::string_view Data) -> void
			{
				FRodinWSData* const SocketData = Socket->getUserData();
				SocketData->GetProxy()->OnPong(Data, OnPong);
			};
		}

		Behavior.close = [&OnClosed](FRodinWS* Socket, int Code, std::string_view Message) -> void
		{
			UE_LOG(LogTemp, Verbose, TEXT("RodinWS closed."));

			FRodinWSData* const SocketData = Socket->getUserData();
			
			auto Proxy = SocketData->GetProxy();

			SocketData->OnClosed();
			Proxy     ->OnClosed(Code, Message, OnClosed);
		};

		if constexpr (bSSL == true)
		{
			Server->App = uWSApp(
			{
				/* .key_file_name		 = */ KeyFile     .size() == 0 ? nullptr : KeyFile     .c_str(),
				/* .cert_file_name		 = */ CertFile    .size() == 0 ? nullptr : CertFile    .c_str(),
				/* .passphrase			 = */ PassPhrase  .size() == 0 ? nullptr : PassPhrase  .c_str(),
				/* .dh_params_file_name  = */ DhParamsFile.size() == 0 ? nullptr : DhParamsFile.c_str(),
				/* .ca_file_name		 = */ CaFileName  .size() == 0 ? nullptr : CaFileName  .c_str()					
			});
		}
		else
		{
			Server->App = uWSApp();
		}
		
		Server->App
		
		.template ws<FRodinWSData>(TCHAR_TO_UTF8(*URI), MoveTemp(Behavior))

		.listen(TCHAR_TO_UTF8(*Host), Port, [&SharedRessources, &Callback, _URI = URI](us_listen_socket_t* ListenSocket) -> void
		{
			check(SharedRessources->ServerStatus == ERodinWSServerState::Starting);

			const bool bServerStarted = ([&]() -> bool
			{
				if (!ListenSocket)
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to start RodinWS server."));

					return false;
				}

				UE_LOG(LogTemp, Log, TEXT("RodinWS Server started at %s."), *_URI);

				SharedRessources->Loop         = uWS::Loop::get();
				SharedRessources->ListenSocket = ListenSocket;
				SharedRessources->ServerStatus = ERodinWSServerState::Running;

				return true;
			}());

			if (Callback.IsBound())
			{
				AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), bServerStarted]() -> void
				{
					Callback.ExecuteIfBound(bServerStarted);
				});
			}
		})
			
		.run();

		UE_LOG(LogTemp, Log, TEXT("RodinWS loop exited."));
		
		{
			FScopeLock Lock(&SharedRessources->LoopAccess);
			if (SharedRessources->Loop)
			{
				SharedRessources->Loop->free();
				SharedRessources->Loop = nullptr;
			}
		}

		SharedRessources->ListenSocket = nullptr;
		SharedRessources->ServerStatus.Exchange(ERodinWSServerState::Closed);

		UE_LOG(LogTemp, Log, TEXT("RodinWS Server closed."));

		AsyncTask(ENamedThreads::GameThread, [OnServerClosed = MoveTemp(OnServerClosed)]() -> void
		{
			OnServerClosed.ExecuteIfBound();
		});
	}));

	Thread->detach();
}

template<bool bSSL>
void TRodinWSServerInternal<bSSL>::Close()
{
	ERodinWSServerState Expected = ERodinWSServerState::Running;
	if (SharedRessources->ServerStatus.CompareExchange(Expected, ERodinWSServerState::Closing))
	{
		auto LoopWork = [SharedRessources = this->SharedRessources]() -> void
		{
			if (SharedRessources->ListenSocket)
			{
				us_listen_socket_close(0, SharedRessources->ListenSocket);
				SharedRessources->ListenSocket = nullptr;

				UE_LOG(LogTemp, Verbose, TEXT("Closed listening socket."));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to close server: ListenSocket was nullptr."));
			}
		};

		{
			FScopeLock Lock(&SharedRessources->LoopAccess);

			if (SharedRessources->Loop)
			{
				SharedRessources->Loop->defer(MoveTemp(LoopWork));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to close server: Loop was nullptr."));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Called Close() on RodinWS Server but the server wasn't running."));
	}
}

template<bool bSSL>
void TRodinWSServerInternal<bSSL>::Publish(FString&& Topic, FString&& Message, ERodinWSOpCode OpCode)
{
	auto LoopWork = 
	[
		Self	= this->AsShared(), 
		Topic	= MoveTemp(Topic), 
		Message = MoveTemp(Message), 
		Code    = NRodinWSUtils::Convert(OpCode)
	]() -> void
	{
		Self->App.publish(TCHAR_TO_UTF8(*Topic), TCHAR_TO_UTF8(*Message), static_cast<uWS::OpCode>(Code));
	};

	{
		FScopeLock Lock(&SharedRessources->LoopAccess);

		if (SharedRessources->Loop)
		{
			SharedRessources->Loop->defer(MoveTemp(LoopWork));
		}
	}
}

