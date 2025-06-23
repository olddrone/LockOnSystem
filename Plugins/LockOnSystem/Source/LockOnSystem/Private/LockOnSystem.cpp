// Copyright Epic Games, Inc. All Rights Reserved.

#include "LockOnSystem.h"


#define LOCTEXT_NAMESPACE "FLockOnSystemModule"

void FLockOnSystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FLockOnSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLockOnSystemModule, LockOnSystem)