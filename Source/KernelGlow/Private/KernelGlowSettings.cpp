#include "KernelGlowSettings.h"

UKernelGlowSettings::UKernelGlowSettings()
{
	// 默认分层：近距青白 / 中距蓝 / Wide 深蓝（默认关）——设计文档 7 节
	Scales[0].Radius = 0.06f;  Scales[0].Intensity = 0.80f;  Scales[0].Tint = FLinearColor(0.62f, 0.85f, 1.00f);
	Scales[1].Radius = 0.16f;  Scales[1].Intensity = 0.35f;  Scales[1].Tint = FLinearColor(0.44f, 0.71f, 1.00f);
	Scales[2].Radius = 0.42f;  Scales[2].Intensity = 0.10f;  Scales[2].Tint = FLinearColor(0.29f, 0.49f, 1.00f);
	Scales[2].bEnabled = true;
}

FName UKernelGlowSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}
