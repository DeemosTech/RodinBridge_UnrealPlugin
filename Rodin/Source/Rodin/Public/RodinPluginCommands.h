#pragma once

#include "CoreMinimal.h"
#if WITH_EDITOR
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#endif
#include "RodinPluginStyle.h"

class FRodinPluginCommands : public TCommands<FRodinPluginCommands>
{
public:
	FRodinPluginCommands()
		: TCommands<FRodinPluginCommands>(TEXT("Rodin"), NSLOCTEXT("Contexts", "Rodin", "Rodin "), NAME_None, FRodinPluginStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands();

public:
	TSharedPtr< FUICommandInfo, ESPMode::ThreadSafe > PluginAction;

};