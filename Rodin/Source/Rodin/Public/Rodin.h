// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#	include "ToolMenus.h"
#	include "Interfaces/IPluginManager.h"
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogWSServer, Log, All);

#if PLATFORM_WINDOWS
#	pragma comment(lib, "Userenv.lib")
#endif

class FToolBarBuilder;
class FMenuBuilder;

class FRodinModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void PluginButtonClicked();
private:

	void RegisterMenus();

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
