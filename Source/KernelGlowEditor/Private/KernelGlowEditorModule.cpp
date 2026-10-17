#include "KernelGlowEditorModule.h"
#include "SKgPanelWindow.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Styling/AppStyle.h"
#include "Misc/EngineVersionComparison.h"
#include "HAL/IConsoleManager.h"
#include "Misc/EngineVersionComparison.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

static TAutoConsoleVariable<int32> CVarKgAutoOpenPanel(
	TEXT("kg.AutoOpenPanel"), 0,
	TEXT("KernelGlow: 编辑器启动后自动打开面板（视觉验收用）"),
	ECVF_Default);

#define LOCTEXT_NAMESPACE "KernelGlowEditor"

DEFINE_LOG_CATEGORY(LogKernelGlowEditor);

static const FName KernelGlowTabName("KernelGlow");

void FKernelGlowEditorModule::StartupModule()
{
	RegisterTabSpawner();
	UE_LOG(LogKernelGlowEditor, Log, TEXT("KernelGlowEditor module loaded (Window -> KernelGlow)."));

	// 验收：kg.AutoOpenPanel=1 时延迟自动打开面板
	FCoreDelegates::OnPostEngineInit.AddLambda([]()
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			static int32 Attempts = 0;
			++Attempts;
			if (CVarKgAutoOpenPanel.GetValueOnGameThread() == 0)
			{
				return false;
			}
			if (Attempts < 3)
			{
				return true;
			}
			if (GEditor)
			{
				FGlobalTabmanager::Get()->TryInvokeTab(KernelGlowTabName);
				UE_LOG(LogKernelGlowEditor, Log, TEXT("KernelGlow panel auto-opened."));
				return false;
			}
			return Attempts < 100;
		}), 1.0f);
	});
}

void FKernelGlowEditorModule::ShutdownModule()
{
	UnregisterTabSpawner();
}

TSharedRef<SDockTab> FKernelGlowEditorModule::OnSpawnKernelGlowTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SKgPanelWindow)
		];
}

void FKernelGlowEditorModule::RegisterTabSpawner()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		KernelGlowTabName,
		FOnSpawnTab::CreateRaw(this, &FKernelGlowEditorModule::OnSpawnKernelGlowTab))
		.SetDisplayName(LOCTEXT("KernelGlowTabTitle", "KernelGlow"))
		.SetTooltipText(LOCTEXT("KernelGlowTabTooltip", "Open the KernelGlow convolution glow panel"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
}

void FKernelGlowEditorModule::UnregisterTabSpawner()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(KernelGlowTabName);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKernelGlowEditorModule, KernelGlowEditor)
