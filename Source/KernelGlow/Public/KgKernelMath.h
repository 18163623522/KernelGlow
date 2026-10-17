#pragma once

#include "CoreMinimal.h"

/**
 * KernelGlow 卷积核心数学（纯函数，可自动化测试）。
 *
 * 多尺度卷积的载体选择：尺度半径 R（屏幕占比）在金字塔第 L 层上卷积，
 * 选取 L 使 kernel 在该层覆盖约 TapCount×TapCount 像素：
 *   2^L = R * BaseWidth / TapCount  →  L = log2(R * BaseWidth / TapCount)
 * 连续半径由层内 tap 跨度（ExtentPx = R * BaseWidth / 2^L）继续表达。
 */
namespace KgKernelMath
{
	/** 半径→金字塔层选择，结果 clamp 到 [0, MaxLevel] */
	int32 ChooseLevelForRadius(float RadiusScreenFrac, float BaseWidthPx, int32 TapCount, int32 MaxLevel);

	/** kernel 采样 LOD：log2(KernelTexSize / TapCount)，最低 0 */
	float ComputeKernelLod(int32 KernelTexSize, int32 TapCount);

	/** 半径在该层的覆盖像素数（ExtentPx） */
	float ComputeExtentPx(float RadiusScreenFrac, float BaseWidthPx, int32 Level);

	/** T×T tap 网格的 kernel 采样 UV（cell 中心），行主序，共 T*T 个 */
	void BuildTapUVs(int32 TapCount, TArray<FVector2f>& OutUVs);

	/** 权重归一化：和归一到 1；零和时回退均匀权重 */
	void NormalizeWeights(TArrayView<float> Weights);

	/** 程序化 kernel：0=圆盘 1=六边形 2=星芒；输出 Size×Size RGBA（RGB=形状×Tint 前的白色形状） */
	void GenerateProceduralKernel(int32 Shape, int32 Size, TArray<FLinearColor>& OutPixels);

	/** 为 UTexture2D 计算 CPU mip 链数据（简单 box 2x2 降采样），Mip0 除外 */
	void ComputeCpuMips(const TArray<FLinearColor>& Mip0, int32 Size, TArray<TArray<FLinearColor>>& OutMips);
}
