#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/** KernelGlow WinUI3 风格设置面板（单窗口 4 卡片布局，Task 10） */
class SKgPanelWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKgPanelWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
