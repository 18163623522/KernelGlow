#include "KernelGlowEditorModule.h"
#include "KernelGlowSettings.h"
#include "Misc/ConfigUtilities.h"

DEFINE_LOG_CATEGORY(LogKernelGlowEditor);

void FKernelGlowEditorModule::StartupModule()
{
	// Task 10 在此注册 Window 菜单入口与 WinUI3 风格面板
	UE_LOG(LogKernelGlowEditor, Log, TEXT("KernelGlowEditor module loaded."));
}

void FKernelGlowEditorModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FKernelGlowEditorModule, KernelGlowEditor)
