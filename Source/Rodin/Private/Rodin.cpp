// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rodin.h"
#include "RodinPluginStyle.h"
#include "RodinPluginCommands.h"
#if WITH_EDITOR
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "EditorAssetLibrary.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilitySubsystem.h"
#endif
DEFINE_LOG_CATEGORY(LogWSServer);
static const FName RodinPluginTabName("Rodin");
#define LOCTEXT_NAMESPACE "FRodinModule"

void FRodinModule::StartupModule()
{
#if WITH_EDITOR
	FRodinPluginStyle::Initialize();
	FRodinPluginStyle::ReloadTextures();
	FRodinPluginCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FRodinPluginCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FRodinModule::PluginButtonClicked),
		FCanExecuteAction());
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, 
			&FRodinModule::RegisterMenus));
#endif
}

void FRodinModule::ShutdownModule()
{
#if WITH_EDITOR
	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FRodinPluginStyle::Shutdown();

	FRodinPluginCommands::Unregister();
#endif
}

void FRodinModule::PluginButtonClicked()
{
//#if WITH_EDITOR
//	UE_LOG(LogTemp, Warning, TEXT("FRodinPluginModule::PluginButtonClicked()"));
//
//	FString assetPath = TEXT("/Rodin/TaskPanel.TaskPanel");
//	auto asset = UEditorAssetLibrary::LoadAsset(assetPath);
//	UEditorUtilityWidgetBlueprint* EditorWidget = Cast<UEditorUtilityWidgetBlueprint>(asset);
//	UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
//	EditorUtilitySubsystem->SpawnAndRegisterTab(EditorWidget);
//#endif
#if WITH_EDITOR
	UE_LOG(LogTemp, Warning, TEXT("FRodinModule::PluginButtonClicked()"));

	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> WidgetBP(
		FSoftObjectPath(TEXT("/Rodin/TaskPanel.TaskPanel"))
	);

	if (WidgetBP.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("TaskPanel not found at /Rodin/TaskPanel.TaskPanel"));
		return;
	}

	UEditorUtilityWidgetBlueprint* EditorWidget = WidgetBP.LoadSynchronous();
	if (!EditorWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load EditorUtilityWidgetBlueprint TaskPanel"));
		return;
	}

	UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	if (EditorUtilitySubsystem)
	{
		EditorUtilitySubsystem->SpawnAndRegisterTab(EditorWidget);
	}
#endif
}

void FRodinModule::RegisterMenus()
{
#if WITH_EDITOR
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FRodinPluginCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FRodinPluginCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
#endif
}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRodinModule, Rodin)