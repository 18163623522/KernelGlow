#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKernelGlowEditor, Log, All);

class FKernelGlowEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<class SDockTab> OnSpawnKernelGlowTab(const class FSpawnTabArgs& SpawnTabArgs);
	void RegisterTabSpawner();
	void UnregisterTabSpawner();

	TSharedPtr<class SWindow> PanelWindow;
};
