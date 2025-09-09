// Fill out your copyright notice in the Description page of Project Settings.


#include "RodinPluginCommands.h"

#define LOCTEXT_NAMESPACE "FRodinModule"

void FRodinPluginCommands::RegisterCommands()
{
#if WITH_EDITOR
	UI_COMMAND(PluginAction, "RodinBridge", "Open RodinBridge Window", EUserInterfaceActionType::Button, FInputChord());
#endif
}

#undef LOCTEXT_NAMESPACE
