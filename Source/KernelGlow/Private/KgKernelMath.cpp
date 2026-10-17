#include "KgKernelMath.h"

namespace KgKernelMath
{
	int32 ChooseLevelForRadius(float RadiusScreenFrac, float BaseWidthPx, int32 TapCount, int32 MaxLevel)
	{
		const float Ratio = FMath::Max(RadiusScreenFrac * BaseWidthPx / (float)TapCount, 1.0f);
		const int32 Level = (int32)FMath::Log2(Ratio);
		return FMath::Clamp(Level, 0, FMath::Max(MaxLevel, 0));
	}

	float ComputeKernelLod(int32 KernelTexSize, int32 TapCount)
	{
		const float Ratio = (float)FMath::Max(KernelTexSize, 1) / (float)FMath::Max(TapCount, 1);
		return FMath::Max(FMath::Log2(Ratio), 0.0f);
	}

	float ComputeExtentPx(float RadiusScreenFrac, float BaseWidthPx, int32 Level)
	{
		const float Div = FMath::Pow(2.0f, (float)Level);
		return RadiusScreenFrac * BaseWidthPx / Div;
	}

	void BuildTapUVs(int32 TapCount, TArray<FVector2f>& OutUVs)
	{
		OutUVs.Reset();
		const int32 T = FMath::Max(TapCount, 1);
		OutUVs.Reserve(T * T);
		for (int32 Y = 0; Y < T; ++Y)
		{
			for (int32 X = 0; X < T; ++X)
			{
				OutUVs.Add(FVector2f((X + 0.5f) / T, (Y + 0.5f) / T));
			}
		}
	}

	void NormalizeWeights(TArrayView<float> Weights)
	{
		float Sum = 0.0f;
		for (float W : Weights) { Sum += W; }
		const int32 N = Weights.Num();
		if (N == 0) { return; }
		if (Sum <= 1e-8f)
		{
			for (float& W : Weights) { W = 1.0f / N; }
			return;
		}
		const float Inv = 1.0f / Sum;
		for (float& W : Weights) { W *= Inv; }
	}

	void GenerateProceduralKernel(int32 Shape, int32 Size, TArray<FLinearColor>& OutPixels)
	{
		const int32 N = FMath::Max(Size, 8);
		OutPixels.SetNumUninitialized(N * N);
		for (int32 Y = 0; Y < N; ++Y)
		{
			for (int32 X = 0; X < N; ++X)
			{
				const float Px = (X + 0.5f) / N * 2.0f - 1.0f;
				const float Py = (Y + 0.5f) / N * 2.0f - 1.0f;
				const float R = FMath::Sqrt(Px * Px + Py * Py);
				float Radius = 0.85f;

				float Mask;
				if (Shape == 1) // 六边形（平顶）：d = max(|x|·cos30 + |y|·sin30, |y|·1) 近似
				{
					const float D = FMath::Max(FMath::Abs(Px) * 0.866025f + FMath::Abs(Py) * 0.5f, FMath::Abs(Py));
					Mask = 1.0f - FMath::SmoothStep(Radius - 0.03f, Radius + 0.03f, D);
				}
				else if (Shape == 2) // 四芒星
				{
					const float Angle = FMath::Atan2(Py, Px);
					const float Ray = 0.15f + 0.7f * FMath::Pow(FMath::Abs(FMath::Cos(Angle * 2.0f)), 8.0f);
					Mask = 1.0f - FMath::SmoothStep(Ray - 0.03f, Ray + 0.03f, R);
				}
				else // 圆盘
				{
					Mask = 1.0f - FMath::SmoothStep(Radius - 0.03f, Radius + 0.03f, R);
				}

				const float V = Mask;
				OutPixels[Y * N + X] = FLinearColor(V, V, V, 1.0f);
			}
		}
	}

	void ComputeCpuMips(const TArray<FLinearColor>& Mip0, int32 Size, TArray<TArray<FLinearColor>>& OutMips)
	{
		OutMips.Reset();
		TArray<FLinearColor> Current = Mip0;
		int32 Cur = Size;
		while (Cur > 1)
		{
			const int32 Next = FMath::Max(Cur / 2, 1);
			TArray<FLinearColor> Down;
			Down.SetNumUninitialized(Next * Next);
			for (int32 Y = 0; Y < Next; ++Y)
			{
				for (int32 X = 0; X < Next; ++X)
				{
					const int32 Sx = FMath::Min(X * 2, Cur - 1);
					const int32 Sy = FMath::Min(Y * 2, Cur - 1);
					const int32 Sx1 = FMath::Min(Sx + 1, Cur - 1);
					const int32 Sy1 = FMath::Min(Sy + 1, Cur - 1);
					const FLinearColor A = Current[Sy * Cur + Sx];
					const FLinearColor B = Current[Sy * Cur + Sx1];
					const FLinearColor C = Current[Sy1 * Cur + Sx];
					const FLinearColor D = Current[Sy1 * Cur + Sx1];
					Down[Y * Next + X] = (A + B + C + D) * 0.25f;
				}
			}
			OutMips.Add(Down);
			Current = MoveTemp(Down);
			Cur = Next;
		}
	}
}
