// Copyright Epic Games, Inc. All Rights Reserved.

#include "RodinPluginStyle.h"
#include "Rodin.h"
#if WITH_EDITOR
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#endif

#if WITH_EDITOR
#define IMAGE_BRUSH(RelativePath, Size) FSlateImageBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), Size)
#endif


TSharedPtr<FSlateStyleSet> FRodinPluginStyle::StyleInstance = nullptr;

void FRodinPluginStyle::Initialize()
{
#if WITH_EDITOR
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
#endif
}

void FRodinPluginStyle::Shutdown()
{
#if WITH_EDITOR
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
#endif
}

FName FRodinPluginStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("RodinPluginStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FRodinPluginStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("RodinPluginStyle"));
#if WITH_EDITOR
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("Rodin")->GetBaseDir() / TEXT("Resources"));
	UE_LOG(LogTemp, Warning, TEXT("Loading icon from: %s"), *Style->RootToContentDir(TEXT("logo"), TEXT(".png")));

	Style->Set("Rodin.PluginAction", 
		new FSlateImageBrush(
			FName(*Style->RootToContentDir(TEXT("logo"), TEXT(".png"))),
			FVector2D(20.0f, 20.0f)
		));
#endif
	return Style;
}

void FRodinPluginStyle::ReloadTextures()
{
#if WITH_EDITOR
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
#endif
}

const ISlateStyle& FRodinPluginStyle::Get()
{
	return *StyleInstance;
}
