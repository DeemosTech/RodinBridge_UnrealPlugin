// Fill out your copyright notice in the Description page of Project Settings.

#include "RodinWSServer.h"

#include "Misc/Guid.h"
#include "Misc/FileHelper.h"
#include "Misc/Base64.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "RodinWSServerInternal.h"
#include "Http.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/DateTime.h"
#include "DesktopPlatformModule.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "TextureResource.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include <IImageWrapper.h>
#include <IImageWrapperModule.h>
#include <Interfaces/IHttpRequest.h>
#include <RodinWSServerNode.h>

FString baseURL = "https://hyper3d.ai/";
FString port = "";
FString onlySID = "";
FString URL = "";

UPARAM(DisplayName = "RodinWS Server")URodinWSServer* URodinWSServer::CreateRodinWSServer()
{
	return NewObject<URodinWSServer>();
}

UPARAM(DisplayName = "RodinWS Server")URodinWSServer* URodinWSServer::CreateSecureRodinWSServer(FString KeyFile, FString CertFile, FString PassPhrase, FString DhParamsFile, FString CaFileName)
{
	URodinWSServer* const Server = NewObject<URodinWSServer>();

	Server->Internal = MakeShared<TRodinWSServerInternal<true>, ESPMode::ThreadSafe>();

	Server->Internal->SetSSLOptions(
		MoveTemp(KeyFile),
		MoveTemp(CertFile),
		MoveTemp(PassPhrase),
		MoveTemp(DhParamsFile),
		MoveTemp(CaFileName));

	return Server;
}

void URodinWSServer::StringToBinary(const FString& InData, TArray<uint8>& OutData)
{
	OutData = TArray<uint8>((uint8*)InData.GetCharArray().GetData(), InData.GetAllocatedSize());
}

FString URodinWSServer::WebURL()
{
	FGuid Guid = FGuid::NewGuid();
	onlySID = Guid.ToString(EGuidFormats::Digits);
	URL = baseURL;
	return URL;
}

bool URodinWSServer::BP_OpenFileDialog(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, FString& OutFilePath)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return false;
	}

	TArray<FString> OutFiles;
	int32 SelectionFlag = 0;
	bool bSuccess = DesktopPlatform->OpenFileDialog(
		nullptr,
		DialogTitle,
		DefaultPath,
		TEXT(""),
		FileTypes,
		SelectionFlag,
		OutFiles
	);

	if (bSuccess && OutFiles.Num() > 0)
	{
		OutFilePath = OutFiles[0];
		return true;
	}

	OutFilePath = TEXT("");
	return false;
}

UTexture2D* URodinWSServer::BP_PreviewImg(FString imgPath)
{
	const FString AbsolutePath = FPaths::ConvertRelativePathToFull(imgPath);
	if (!FPaths::FileExists(AbsolutePath)) {
		UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *AbsolutePath);
		return nullptr;
	}

	const FString Extension = FPaths::GetExtension(AbsolutePath).ToLower();
	EImageFormat ImageFormat = EImageFormat::Invalid;
	if (Extension == TEXT("png")) ImageFormat = EImageFormat::PNG;
	else if (Extension == TEXT("jpg") || Extension == TEXT("jpeg")) ImageFormat = EImageFormat::JPEG;
	else if (Extension == TEXT("bmp")) ImageFormat = EImageFormat::BMP;
	else if (Extension == TEXT("exr")) ImageFormat = EImageFormat::EXR;

	if (ImageFormat == EImageFormat::Invalid) {
		UE_LOG(LogTemp, Error, TEXT("Unsupported image format: %s"), *Extension);
		return nullptr;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *AbsolutePath)) {
		UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *AbsolutePath);
		return nullptr;
	}

	TArray<EImageFormat> FormatsToTry = { EImageFormat::PNG, EImageFormat::JPEG, EImageFormat::BMP, EImageFormat::EXR };
	for (EImageFormat Format : FormatsToTry) {
		TSharedPtr<IImageWrapper> Wrapper = ImageWrapperModule.CreateImageWrapper(Format);
		if (Wrapper->SetCompressed(FileData.GetData(), FileData.Num())) {
			ImageWrapper = Wrapper;
			break;
		}
	}
	if (!ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num())) {
		//UE_LOG(LogTemp, Error, TEXT("Failed to parse image data"));
		UE_LOG(LogTemp, Error, TEXT("Failed to parse image data for file: %s. File size: %d bytes. This may be due to corrupt or unsupported image format."), *AbsolutePath, FileData.Num());

		return nullptr;
	}

	TArray64<uint8> RawData;
	if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData)) {
		UE_LOG(LogTemp, Error, TEXT("Failed to get raw image data"));
		return nullptr;
	}

	const int32 Width = ImageWrapper->GetWidth();
	const int32 Height = ImageWrapper->GetHeight();
	if (Width <= 0 || Height <= 0) {
		UE_LOG(LogTemp, Error, TEXT("Invalid image dimensions"));
		return nullptr;
	}

	UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!NewTexture) {
		UE_LOG(LogTemp, Error, TEXT("Failed to create transient texture"));
		return nullptr;
	}

	void* TextureData = NewTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
	NewTexture->GetPlatformData()->Mips[0].BulkData.Unlock();

	NewTexture->Filter = TF_Default;
	NewTexture->SRGB = true;
	NewTexture->UpdateResource();

	return NewTexture;
}

void URodinWSServer::ST_SubmitInfo(
	FString mode_controlNet, FString prompt_partMode, FString mode_windowsClick,
	bool bUseShaded, bool bUsePBR, bool bBypass, bool bTextTo,
	FString resolution_mat, FString align_noneMode, FString ImagePath, FString polygons,
	FString modelFilePath, FString voxel_condition_cfg, FString modeGenerationExpansion,
	float height_model, float voxel_condition_weight, float pcd_condition_uncertainty, int quality,
	bool& loadFileSuccess, bool& sendSuccess, FString& OutJson, FString& taskID)
{
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetStringField("type", "fetch_task_return");
	RootObject->SetStringField("sid", onlySID);

	TSharedPtr<FJsonObject> TaskObject = MakeShareable(new FJsonObject);
	TaskObject->SetStringField("type", mode_controlNet);
	TaskObject->SetStringField("id", onlySID);
	if (!prompt_partMode.IsEmpty())
	{
		TaskObject->SetStringField("prompt", prompt_partMode);
	}
	else
	{
		TaskObject->SetStringField("prompt", "");
	}

	TSharedPtr<FJsonObject> ConfigObject = MakeShareable(new FJsonObject);
	ConfigObject->SetStringField("type", mode_windowsClick);

	TSharedPtr<FJsonObject> MaterialObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> TypeArray;
	if (bUseShaded) TypeArray.Add(MakeShared<FJsonValueString>(TEXT("Shaded")));
	if (bUsePBR)    TypeArray.Add(MakeShared<FJsonValueString>(TEXT("PBR")));
	MaterialObject->SetArrayField("type", TypeArray);
	MaterialObject->SetStringField("resolution", resolution_mat);

	ConfigObject->SetObjectField("material", MaterialObject);
	ConfigObject->SetNumberField("height", height_model);
	ConfigObject->SetStringField("align", align_noneMode);

	ConfigObject->SetStringField("voxel_condition_cfg", voxel_condition_cfg);
	ConfigObject->SetNumberField("voxel_condition_weight", voxel_condition_weight);
	ConfigObject->SetNumberField("pcd_condition_uncertainty", pcd_condition_uncertainty);
	ConfigObject->SetStringField("polygons", polygons);
	ConfigObject->SetStringField("mode", modeGenerationExpansion);
	ConfigObject->SetNumberField("quality", quality);
	ConfigObject->SetBoolField("textTo", bTextTo);
	ConfigObject->SetBoolField("bypass", bBypass);
	ConfigObject->SetStringField("text", prompt_partMode);

	TaskObject->SetObjectField("config", ConfigObject);

	TArray<TSharedPtr<FJsonValue>> ImageArray;
	if (!ImagePath.IsEmpty())
	{
		TArray<uint8> ImageData;
		if (!FFileHelper::LoadFileToArray(ImageData, *ImagePath))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load image file: %s"), *ImagePath);
			loadFileSuccess = false;
		}

		FString MD5Hash = FMD5::HashBytes(ImageData.GetData(), ImageData.Num());

		TSharedPtr<FJsonObject> ImageObject = MakeShareable(new FJsonObject);
		ImageObject->SetStringField("format", "png");
		ImageObject->SetNumberField("length", ImageData.Num());
		ImageObject->SetStringField("md5", MD5Hash);
		ImageObject->SetStringField("content", "data:image/png;base64," + FBase64::Encode(ImageData));

		ImageArray.Add(MakeShareable(new FJsonValueObject(ImageObject)));
	}
	TaskObject->SetArrayField("image", ImageArray);

	TSharedPtr<FJsonObject> ConditionObject = nullptr;

	if (!modelFilePath.IsEmpty())
	{
		TArray<uint8> fbxData;
		if (FFileHelper::LoadFileToArray(fbxData, *modelFilePath))
		{
			ConditionObject = MakeShareable(new FJsonObject);
			FString MD5Hash = FMD5::HashBytes(fbxData.GetData(), fbxData.Num());
			FString Base64Content = "data:model/fbx;base64," + FBase64::Encode(fbxData);

			ConditionObject->SetStringField("format", "fbx");
			ConditionObject->SetNumberField("length", fbxData.Num());
			ConditionObject->SetStringField("md5", MD5Hash);
			ConditionObject->SetStringField("content", Base64Content);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load fbx file: %s"), *modelFilePath);
			loadFileSuccess = false;
		}
	}

	if (ConditionObject.IsValid()) {
		TaskObject->SetObjectField("condition", ConditionObject);
	}
	else {
		TaskObject->SetObjectField("condition", MakeShareable(new FJsonObject));
	}

	RootObject->SetObjectField("task", TaskObject);

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
	if (!FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to serialize JSON"));
	}
	taskID = onlySID;
}

void URodinWSServer::ST_SubmitInfo_Multi(
	FString mode_controlNet, FString prompt_partMode, FString mode_windowsClick, 
	bool bUseShaded, bool bUsePBR, bool bBypass, bool bTextTo, 
	FString resolution_mat, FString align_noneMode, TArray<FString> ImagePaths,
	FString polygons, FString modelFilePath, FString voxel_condition_cfg, 
	FString modeGenerationExpansion, float height_model, float voxel_condition_weight, float pcd_condition_uncertainty, int quality, 
	bool& loadFileSuccess, bool& sendSuccess, FString& OutJson, FString& taskID)
{
	loadFileSuccess = true;
	sendSuccess = false;

	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetStringField("type", "fetch_task_return");
	RootObject->SetStringField("sid", onlySID);

	TSharedPtr<FJsonObject> TaskObject = MakeShareable(new FJsonObject);
	TaskObject->SetStringField("type", mode_controlNet);
	TaskObject->SetStringField("id", onlySID);
	TaskObject->SetStringField("prompt", prompt_partMode.IsEmpty() ? "" : prompt_partMode);

	TSharedPtr<FJsonObject> ConfigObject = MakeShareable(new FJsonObject);
	ConfigObject->SetStringField("type", mode_windowsClick);

	TSharedPtr<FJsonObject> MaterialObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> TypeArray;
	if (bUseShaded) TypeArray.Add(MakeShared<FJsonValueString>(TEXT("Shaded")));
	if (bUsePBR)    TypeArray.Add(MakeShared<FJsonValueString>(TEXT("PBR")));
	MaterialObject->SetArrayField("type", TypeArray);
	MaterialObject->SetStringField("resolution", resolution_mat);

	ConfigObject->SetObjectField("material", MaterialObject);
	ConfigObject->SetNumberField("height", height_model);
	ConfigObject->SetStringField("align", align_noneMode);
	ConfigObject->SetStringField("voxel_condition_cfg", voxel_condition_cfg);
	ConfigObject->SetNumberField("voxel_condition_weight", voxel_condition_weight);
	ConfigObject->SetNumberField("pcd_condition_uncertainty", pcd_condition_uncertainty);
	ConfigObject->SetStringField("polygons", polygons);
	ConfigObject->SetStringField("mode", modeGenerationExpansion);
	ConfigObject->SetNumberField("quality", quality);
	ConfigObject->SetBoolField("textTo", bTextTo);
	ConfigObject->SetBoolField("bypass", bBypass);
	ConfigObject->SetStringField("text", prompt_partMode);

	TaskObject->SetObjectField("config", ConfigObject);

	TArray<TSharedPtr<FJsonValue>> ImageArray;
	for (const FString& ImagePath : ImagePaths)
	{
		if (ImagePath.IsEmpty()) continue;

		TArray<uint8> ImageData;
		if (!FFileHelper::LoadFileToArray(ImageData, *ImagePath))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load image file: %s"), *ImagePath);
			loadFileSuccess = false;
			continue;
		}

		FString Extension = FPaths::GetExtension(ImagePath, true).ToLower(); 
		if (Extension.StartsWith(TEXT(".")))
		{
			Extension = Extension.RightChop(1);
		}
		if (Extension.IsEmpty())
		{
			Extension = TEXT("png");
		}

		FString MD5Hash = FMD5::HashBytes(ImageData.GetData(), ImageData.Num());

		TSharedPtr<FJsonObject> ImageObject = MakeShareable(new FJsonObject);
		ImageObject->SetStringField("format", Extension);
		ImageObject->SetNumberField("length", ImageData.Num());
		ImageObject->SetStringField("md5", MD5Hash);
		ImageObject->SetStringField("content", FString::Printf(TEXT("data:image/%s;base64,%s"), *Extension, *FBase64::Encode(ImageData)));

		ImageArray.Add(MakeShareable(new FJsonValueObject(ImageObject)));
	}
	TaskObject->SetArrayField("image", ImageArray);

	TSharedPtr<FJsonObject> ConditionObject = nullptr;
	if (!modelFilePath.IsEmpty())
	{
		TArray<uint8> fbxData;
		if (FFileHelper::LoadFileToArray(fbxData, *modelFilePath))
		{
			ConditionObject = MakeShareable(new FJsonObject);
			FString MD5Hash = FMD5::HashBytes(fbxData.GetData(), fbxData.Num());
			FString Base64Content = "data:model/fbx;base64," + FBase64::Encode(fbxData);

			ConditionObject->SetStringField("format", "fbx");
			ConditionObject->SetNumberField("length", fbxData.Num());
			ConditionObject->SetStringField("md5", MD5Hash);
			ConditionObject->SetStringField("content", Base64Content);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load fbx file: %s"), *modelFilePath);
			loadFileSuccess = false;
		}
	}

	if (ConditionObject.IsValid()) {
		TaskObject->SetObjectField("condition", ConditionObject);
	}
	else {
		TaskObject->SetObjectField("condition", MakeShareable(new FJsonObject));
	}

	RootObject->SetObjectField("task", TaskObject);

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
	if (!FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to serialize JSON"));
	}

	taskID = onlySID;
	sendSuccess = true;
}

void URodinWSServer::ST_MessageParse(FString JsonString, bool& endDownload, FString& modelPath)
{
	endDownload = false;

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	TSharedPtr<FJsonObject> JsonObject;

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON."));
		return;
	}

	const TSharedPtr<FJsonObject>* DataObject;
	if (!JsonObject->TryGetObjectField("data", DataObject))
	{
		UE_LOG(LogTemp, Error, TEXT("Missing 'data' field."));
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* FilesArray;
	if (!(*DataObject)->TryGetArrayField("files", FilesArray) || FilesArray->Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Missing or empty 'files' array."));
		return;
	}

	const TSharedPtr<FJsonObject>* FileObject;
	if (!(*FilesArray)[0]->TryGetObject(FileObject))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid file object."));
		return;
	}

	FDateTime Now = FDateTime::Now();
	FString Filename = FString::Printf(TEXT("%02d%02d%02d%02d%02d.usdz"),
		Now.GetMonth(), Now.GetDay(), Now.GetHour(), Now.GetMinute(), Now.GetSecond());

	FString Content = (*FileObject)->GetStringField("content");

	const FString Base64Prefix = TEXT("data:model/usdz;base64,");
	if (Content.StartsWith(Base64Prefix))
	{
		Content = Content.RightChop(Base64Prefix.Len());
	}

	TArray<uint8> DecodedBytes;
	if (!FBase64::Decode(Content, DecodedBytes))
	{
		UE_LOG(LogTemp, Error, TEXT("Base64 decoding failed."));
		return;
	}

	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("Models");
	IFileManager::Get().MakeDirectory(*SaveDir, true);

	FString SavePath = SaveDir / Filename;

	if (FFileHelper::SaveArrayToFile(DecodedBytes, *SavePath))
	{
		UE_LOG(LogTemp, Log, TEXT("Model saved to: %s"), *SavePath);
		endDownload = true;
		modelPath = SavePath;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save model file."));
	}
}
void URodinWSServer::BP_actorSize(AActor* TargetActor, float& sizeX, float& sizeY, float& sizeZ)
{
	sizeX = sizeY = sizeZ = 0.0f;
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Actor"));
		return;
	}

	TArray<AActor*> ActorsToCheck;
	TSet<AActor*> VisitedActors;
	ActorsToCheck.Add(TargetActor);
	VisitedActors.Add(TargetActor);

	TArray<UStaticMeshComponent*> AllMeshComponents;

	while (ActorsToCheck.Num() > 0)
	{
		AActor* CurrentActor = ActorsToCheck.Pop();
		TArray<UStaticMeshComponent*> CurrentComponents;
		CurrentActor->GetComponents<UStaticMeshComponent>(CurrentComponents);
		AllMeshComponents.Append(CurrentComponents);

		TArray<AActor*> ChildActors;
		CurrentActor->GetAttachedActors(ChildActors);
		for (AActor* Child : ChildActors)
		{
			if (IsValid(Child) && !VisitedActors.Contains(Child))
			{
				ActorsToCheck.Add(Child);
				VisitedActors.Add(Child);
			}
		}
	}

	if (AllMeshComponents.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No StaticMeshComponents in actor hierarchy"));
		return;
	}

	FVector TotalMin(FLT_MAX);
	FVector TotalMax(-FLT_MAX);
	for (UStaticMeshComponent* MeshComp : AllMeshComponents)
	{
		if (MeshComp && MeshComp->GetStaticMesh())
		{
			FVector Origin, BoxExtent;
			MeshComp->GetLocalBounds(Origin, BoxExtent);
			const FTransform& Transform = MeshComp->GetComponentTransform();
			FVector WorldOrigin = Transform.TransformPosition(Origin);
			FVector WorldExtent = Transform.GetScale3D() * BoxExtent;

			FVector Min = WorldOrigin - WorldExtent;
			FVector Max = WorldOrigin + WorldExtent;

			TotalMin = TotalMin.ComponentMin(Min);
			TotalMax = TotalMax.ComponentMax(Max);
		}
	}

	sizeX = TotalMax.X - TotalMin.X;
	sizeY = TotalMax.Y - TotalMin.Y;
	sizeZ = TotalMax.Z - TotalMin.Z;

	UE_LOG(LogTemp, Log, TEXT("Actor Size - X:%.2f Y:%.2f Z:%.2f"), sizeX, sizeY, sizeZ);
}


void URodinWSServer::ST_Push()
{

}

void URodinWSServer::ST_Run()
{
}

void URodinWSServer::ST_bFinish()
{
}

void URodinWSServer::ST_setStatus(ERodinTaskStatus newStatus)
{
	Status = newStatus;

}



URodinWSServer::URodinWSServer()
	: Internal(MakeShared<TRodinWSServerInternal<false>, ESPMode::ThreadSafe>())
{
}

URodinWSServer::~URodinWSServer()
{
	if (GetServerState() != ERodinWSServerState::Closed)
	{
		Internal->Close();
	}
}

void URodinWSServer::Listen(FString Host, FString URI, const int32 Port, FOnRodinWSServerListening Callback)
{
	Internal->OnOpened().BindUObject(this, &URodinWSServer::InternalOnRodinWSOpened);
	Internal->OnMessage().BindUObject(this, &URodinWSServer::InternalOnRodinWSMessage);
	Internal->OnPing().BindUObject(this, &URodinWSServer::InternalOnRodinWSMessage);
	Internal->OnPong().BindUObject(this, &URodinWSServer::InternalOnRodinWSMessage);
	Internal->OnClosed().BindUObject(this, &URodinWSServer::InternalOnRodinWSClosed);

	Internal->Listen(MoveTemp(Host), MoveTemp(URI), Port, MoveTemp(Callback),
		FOnServerClosed::CreateUObject(this, &URodinWSServer::InternalOnServerClosed));

}

void URodinWSServer::Listen(FString URI, const int32 Port, FOnRodinWSServerListening Callback)
{
	Listen(TEXT("0.0.0.0"), MoveTemp(URI), Port, MoveTemp(Callback));
}

void URodinWSServer::StopListening()
{
	if (Internal->GetServerState() == ERodinWSServerState::Running)
	{
		Internal->Close();
	}
}

void URodinWSServer::SetIdleTimeout(const int64 InIdleTimeout)
{
	ensureMsgf(InIdleTimeout == 0 || InIdleTimeout >= 8, TEXT("Idle timeout must be 0 or in [8;inf[. Provided: %d."), InIdleTimeout);

	Internal->SetIdleTimeout(InIdleTimeout);
}

void URodinWSServer::SetMaxLifetime(const int64 InMaxLifetime)
{
	Internal->SetMaxLifetime(InMaxLifetime);
}

void URodinWSServer::SetCompression(const ERodinWSCompressOptions InCompression)
{
	Internal->SetCompression(InCompression);
}

void URodinWSServer::Publish(const FString& Topic, const FString& Message, ERodinWSOpCode OpCode)
{
	Publish(FString(Topic), FString(Message), OpCode);
}

void URodinWSServer::Publish(const FString& Topic, FString&& Message, ERodinWSOpCode OpCode)
{
	Publish(FString(Topic), MoveTemp(Message), OpCode);
}

void URodinWSServer::Publish(FString&& Topic, const FString& Message, ERodinWSOpCode OpCode)
{
	Publish(MoveTemp(Topic), FString(Message), OpCode);
}

void URodinWSServer::Publish(FString&& Topic, FString&& Message, ERodinWSOpCode OpCode)
{
	Internal->Publish(MoveTemp(Topic), MoveTemp(Message), OpCode);
}

void URodinWSServer::SetSendPingsAutomatically(const bool bInSendPingsAutomatically)
{
	Internal->SetSendPingsAutomatically(bInSendPingsAutomatically);
}

void URodinWSServer::SetResetIdleTimeoutOnSend(const bool bInResetIdleTimeoutOnSend)
{
	Internal->SetResetIdleTimeoutOnSend(bInResetIdleTimeoutOnSend);
}

void URodinWSServer::SetCloseOnBackpressureLimit(const bool bInCloseOnBackpressureLimit)
{
	Internal->SetCloseOnBackpressureLimit(bInCloseOnBackpressureLimit);
}

void URodinWSServer::SetMaxPayloadLength(const int64 InMaxPayloadLength)
{
	ensure(InMaxPayloadLength >= 0);

	Internal->SetMaxPayloadLength(InMaxPayloadLength);
}

void URodinWSServer::SetMaxBackPressure(const int64 InMaxBackPressure)
{
	Internal->SetMaxBackPressure(InMaxBackPressure);
}

UPARAM(DisplayName = "State")ERodinWSServerState URodinWSServer::GetServerState() const
{
	return UPARAM(DisplayName = "State")ERodinWSServerState();
}

void URodinWSServer::InternalOnServerClosed()
{
	OnRodinWSServerClosed.Broadcast();
}

void URodinWSServer::InternalOnRodinWSOpened(URodinWS* Socket)
{
	OnRodinWSOpened.Broadcast(Socket);
}

void URodinWSServer::InternalOnRodinWSMessage(URodinWS* Socket, TArray<uint8> Message, ERodinWSOpCode Code)
{
	auto ConvertMessage = [&]() -> FString
		{
			const FUTF8ToTCHAR Converter((const char*)Message.GetData(), Message.Num());
			return FString(Converter.Length(), Converter.Get());
		};

	switch (Code)
	{
	case ERodinWSOpCode::PING:
		OnRodinWSPing.Broadcast(Socket, ConvertMessage(), Code);
		break;

	case ERodinWSOpCode::PONG:
		OnRodinWSPong.Broadcast(Socket, ConvertMessage(), Code);
		break;

	case ERodinWSOpCode::BINARY:
		if (OnRodinWSRawMessage.IsBound())
		{
			OnRodinWSRawMessage.Broadcast(Socket, Message, Code);
			break;
		}

	case ERodinWSOpCode::TEXT:
	default:

		OnRodinWSMessage.Broadcast(Socket, ConvertMessage(), Code);
		break;
	}
}

void URodinWSServer::InternalOnRodinWSClosed(URodinWS* Socket, const int32 Code, const FString& Message)
{
	OnRodinWSClosed.Broadcast(Socket, Code, Message);
}
