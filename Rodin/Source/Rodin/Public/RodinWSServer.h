// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RodinWSServer.generated.h"

class URodinWS;
class URodinWSServer;

UENUM(BlueprintType)
enum class ERodinTaskStatus : uint8
{
    NONE UMETA(DisplayName = "None"),
    CONNECTING UMETA(DisplayName = "Connecting"),
    PROCESSING UMETA(DisplayName = "Processing"),
    TIMEOUT UMETA(DisplayName = "Timeout"),
    SUCCEEDED UMETA(DisplayName = "Succeeded"),
    FAILED UMETA(DisplayName = "Failed"),
    SKIPPED UMETA(DisplayName = "Skipped"),
    EXIT UMETA(DisplayName = "Exit")
};

UENUM(BlueprintType)
enum class ERodinWSCompressOptions : uint8
{
    DISABLED = 0                UMETA(DisplayName = "Disabled"),

    SHARED_COMPRESSOR = 1       UMETA(DisplayName = "Shared Compressor"),
    DEDICATED_COMPRESSOR_3KB    UMETA(DisplayName = "Shared Compressor 3kb"),
    DEDICATED_COMPRESSOR_4KB    UMETA(DisplayName = "Shared Compressor 4kb"),
    DEDICATED_COMPRESSOR_8KB    UMETA(DisplayName = "Shared Compressor 8kb"),
    DEDICATED_COMPRESSOR_16KB   UMETA(DisplayName = "Shared Compressor 16kb"),
    DEDICATED_COMPRESSOR_32KB   UMETA(DisplayName = "Shared Compressor 32kb"),
    DEDICATED_COMPRESSOR_64KB   UMETA(DisplayName = "Shared Compressor 64kb"),
    DEDICATED_COMPRESSOR_128KB  UMETA(DisplayName = "Shared Compressor 128kb"),
    DEDICATED_COMPRESSOR_256KB  UMETA(DisplayName = "Shared Compressor 256kb"),

    DEDICATED_COMPRESSOR UMETA(DisplayName = "Dedicated Compressor")
};

UENUM(BlueprintType)
enum class ERodinWSOpCode : uint8
{
    None UMETA(hidden),

    TEXT = 1    UMETA(DisplayName = "Text"),

    BINARY = 2    UMETA(DisplayName = "Binary"),

    CLOSE = 8    UMETA(DisplayName = "Close"),

    PING = 9    UMETA(DisplayName = "Ping"),

    PONG = 10    UMETA(DisplayName = "Pong")
};

UENUM(BlueprintType)
enum class ERodinWSServerState : uint8
{
    Closing,

    Closed,

    Starting,

    Running
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnRodinWSOpened,
    class URodinWS*, Socket
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnRodinWSMessage,
    class URodinWS*, Socket,
    const FString&, Message,
    ERodinWSOpCode, OpCode
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnRodinWSRawMessage,
    class URodinWS*, Socket,
    const TArray<uint8>&, Message,
    ERodinWSOpCode, OpCode
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnRodinWSClosed,
    class URodinWS*, Socket,
    const int32, Code,
    const FString&, Message
);

DECLARE_DELEGATE_OneParam(
    FOnRodinWSServerListening,
    bool 
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRodinWSServerClosed);


DECLARE_DELEGATE_TwoParams(
    FOnRodinWSSubscribed,
    bool ,
    int32 
);

DECLARE_DELEGATE_OneParam(
    FOnRodinWSPublished,
    bool 
);


UCLASS(BlueprintType)
class RODIN_API URodinWS : public UObject
{
    GENERATED_BODY()
private:
    template<bool bSSL>
    friend class TRodinWSProxy;

public:
    URodinWS() = default;
    ~URodinWS() = default;

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void Send(const FString& Message);
    void Send(FString&& Message);
    void Send(const TArray<uint8>& Data);
    void Send(TArray<uint8>&& Data);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server", meta = (DisplayName = "Send Binary"))
    void Send_Blueprint(const TArray<uint8>& Data);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void Ping(const FString& Message = TEXT(""));
    void Ping(FString&& Message);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void Pong(const FString& Message = TEXT(""));
    void Pong(FString&& Message);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void Close();

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void End(const int32 Code = 0, const FString& Message = TEXT(""));
    void End(const int32 Code, FString&& Message);

    void Subscribe(const FString& Topic, const FOnRodinWSSubscribed& Callback, bool bNonStrict = false);
    void Subscribe(const FString& Topic,            FOnRodinWSSubscribed&& Callback = FOnRodinWSSubscribed(), bool bNonStrict = false);
    void Subscribe(          FString&& Topic, const FOnRodinWSSubscribed& Callback, bool bNonStrict = false);
    void Subscribe(          FString&& Topic,           FOnRodinWSSubscribed&& Callback = FOnRodinWSSubscribed(), bool bNonStrict = false);

    void Unsubscribe(const FString& Topic, const FOnRodinWSSubscribed& Callback, bool bNonStrict = false);
    void Unsubscribe(const FString& Topic, FOnRodinWSSubscribed&& Callback = FOnRodinWSSubscribed(), bool bNonStrict = false);
    void Unsubscribe(FString&& Topic, const FOnRodinWSSubscribed& Callback, bool bNonStrict = false);
    void Unsubscribe(FString&& Topic, FOnRodinWSSubscribed&& Callback = FOnRodinWSSubscribed(), bool bNonStrict = false);

    void Publish(const FString& Topic, const FString& Message, const FOnRodinWSPublished& Callback = FOnRodinWSPublished());
    void Publish(FString&& Topic, FString&& Message, FOnRodinWSPublished&& Callback = FOnRodinWSPublished());

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RodinWS|Server")
    UPARAM(DisplayName = "Is Connected") bool IsConnected() const;

private:
    TWeakPtr<class IRodinWSProxy, ESPMode::ThreadSafe> SocketProxy;
};

UCLASS(BlueprintType)
class RODIN_API URodinWSServer : public UObject
{
    GENERATED_BODY()
public:
   
    UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
    FOnRodinWSOpened OnRodinWSOpened;

    UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
    FOnRodinWSMessage OnRodinWSMessage;

    UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
    FOnRodinWSRawMessage OnRodinWSRawMessage;

    UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
    FOnRodinWSClosed OnRodinWSClosed;

    UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
    FOnRodinWSMessage OnRodinWSPing;

    UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
    FOnRodinWSMessage OnRodinWSPong;

    UPROPERTY(BlueprintAssignable, Category = "RodinWS|Server")
    FOnRodinWSServerClosed OnRodinWSServerClosed;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RodinWS|Server")
    static UPARAM(DisplayName = "Create Normal Server") URodinWSServer* CreateRodinWSServer();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RodinWS|Server")
    static UPARAM(DisplayName = "Create WSS Server") URodinWSServer* CreateSecureRodinWSServer(
        FString KeyFile,FString CertFile,FString PassPhrase,FString DhParamsFile,FString CaFileName);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RodinWS|Server")
    static void StringToBinary(const FString& InData, TArray<uint8>& OutData);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RodinWS|Server")
    static UPARAM(DisplayName = "URL")FString WebURL();

    UFUNCTION(BlueprintCallable, Category = "RodinWS|BP")
    static bool BP_OpenFileDialog(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, FString& OutFilePath);
    
    UFUNCTION(BlueprintCallable, Category = "RodinWS|BP")
    static UTexture2D* BP_PreviewImg(FString imgPath);

    UFUNCTION(BlueprintCallable,  Category = "RodinWS|Server|BP")
    void ST_SubmitInfo(
        FString mode_controlNet, FString prompt_partMode, FString mode_windowsClick,
        bool bUseShaded, bool bUsePBR, bool bBypass, bool bTextTo,
        FString resolution_mat, FString align_noneMode, FString ImagePath, FString polygons,
        FString modelFilePath, FString voxel_condition_cfg, FString modeGenerationExpansion,
        float height_model, float voxel_condition_weight, float pcd_condition_uncertainty, int quality,
        bool& loadFileSuccess, bool& sendSuccess, FString& OutJson, FString& taskID);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server|BP")
    void ST_SubmitInfo_Multi(
        FString mode_controlNet, FString prompt_partMode, FString mode_windowsClick,
        bool bUseShaded, bool bUsePBR, bool bBypass, bool bTextTo,
        FString resolution_mat, FString align_noneMode, TArray<FString> ImagePaths, 
        FString polygons, FString modelFilePath, FString voxel_condition_cfg, FString modeGenerationExpansion,
        float height_model, float voxel_condition_weight, float pcd_condition_uncertainty, int quality,
        bool& loadFileSuccess, bool& sendSuccess, FString& OutJson, FString& taskID);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server|BP")
    void ST_MessageParse(FString JsonString, bool& endDownload, FString& modelPath);
    
    UFUNCTION(BlueprintCallable, Category = "RodinWS|BP", meta = (DisplayName = "Get Actor Size"))
    static void BP_actorSize(AActor* TargetActor, float& sizeX, float& sizeY, float& sizeZ);
    
    void ST_Push();
    void ST_Run();
    void ST_bFinish();
    void ST_setStatus(ERodinTaskStatus newStatus);

    URodinWSServer();
    ~URodinWSServer();
    
    void Listen(FString Host, FString URI, const int32 Port, FOnRodinWSServerListening Callback);
    void Listen(FString URI, const int32 Port, FOnRodinWSServerListening Callback);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void StopListening();

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void SetIdleTimeout( UPARAM(DisplayName = "Idle Timeout") const int64 InIdleTimeout);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void SetMaxLifetime(const int64 InMaxLifetime);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void SetMaxPayloadLength(const int64 InMaxPayloadLength);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void SetMaxBackPressure(const int64 InMaxBackPressure);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void SetCloseOnBackpressureLimit(const bool bInCloseOnBackpressureLimit);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void SetResetIdleTimeoutOnSend(const bool bInResetIdleTimeoutOnSend);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void SetSendPingsAutomatically(const bool bInSendPingsAutomatically);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void SetCompression(const ERodinWSCompressOptions InCompression);

    UFUNCTION(BlueprintCallable, Category = "RodinWS|Server")
    void Publish(const FString& Topic, const FString& Message, ERodinWSOpCode OpCode = ERodinWSOpCode::TEXT);
    void Publish(const FString& Topic, FString&& Message, ERodinWSOpCode OpCode = ERodinWSOpCode::TEXT);
    void Publish(FString&& Topic, const FString& Message, ERodinWSOpCode OpCode = ERodinWSOpCode::TEXT);
    void Publish(FString&& Topic, FString&& Message, ERodinWSOpCode OpCode = ERodinWSOpCode::TEXT);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RodinWS|Server")
    UPARAM(DisplayName = "State") ERodinWSServerState GetServerState() const;

private:
    void InternalOnServerClosed();
    void InternalOnRodinWSOpened(URodinWS*);
    void InternalOnRodinWSMessage(URodinWS*, TArray<uint8>, ERodinWSOpCode);
    void InternalOnRodinWSClosed(URodinWS*, const int32, const FString&);

    TSharedPtr<class IRodinWSServerInternal, ESPMode::ThreadSafe> Internal;
    
    ERodinTaskStatus Status;
};
