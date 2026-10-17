#include "KgKernelMath.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKgChooseLevelTest, "KernelGlow.KernelMath.ChooseLevelForRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FKgChooseLevelTest::RunTest(const FString& Parameters)
{
	// R=0.06, W=2560, T=8：2^L = 19.2 → L=4
	TestEqual("small radius", KgKernelMath::ChooseLevelForRadius(0.06f, 2560.f, 8, 4), 4);
	// R=0.01：2^L = 3.2 → L=1
	TestEqual("tiny radius", KgKernelMath::ChooseLevelForRadius(0.01f, 2560.f, 8, 4), 1);
	// R=0.9：2^L = 288 → 超 MaxLevel，clamp 4
	TestEqual("huge radius clamps", KgKernelMath::ChooseLevelForRadius(0.9f, 2560.f, 8, 4), 4);
	// MaxLevel=0 时永远 0
	TestEqual("zero max", KgKernelMath::ChooseLevelForRadius(0.9f, 2560.f, 8, 0), 0);
	// 极小半径不产生负数
	TestEqual("no negative", KgKernelMath::ChooseLevelForRadius(0.001f, 1280.f, 16, 4), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKgKernelLodTest, "KernelGlow.KernelMath.ComputeKernelLod",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FKgKernelLodTest::RunTest(const FString& Parameters)
{
	// K=256, T=8 → LOD=5
	TestEqual("256/8", (int32)FMath::RoundToInt(KgKernelMath::ComputeKernelLod(256, 8)), 5);
	// K=T → LOD=0
	TestEqual("equal", (int32)FMath::RoundToInt(KgKernelMath::ComputeKernelLod(8, 8)), 0);
	// K<T → clamp 0
	TestEqual("clamp", (int32)FMath::RoundToInt(KgKernelMath::ComputeKernelLod(4, 8)), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKgExtentTest, "KernelGlow.KernelMath.ComputeExtentPx",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FKgExtentTest::RunTest(const FString& Parameters)
{
	// R=0.5, W=1024, L=0 → 512
	TestEqual("level0", (int32)KgKernelMath::ComputeExtentPx(0.5f, 1024.f, 0), 512);
	// L=3 → 64
	TestEqual("level3", (int32)KgKernelMath::ComputeExtentPx(0.5f, 1024.f, 3), 64);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKgTapUVsTest, "KernelGlow.KernelMath.BuildTapUVs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FKgTapUVsTest::RunTest(const FString& Parameters)
{
	TArray<FVector2f> UVs;
	KgKernelMath::BuildTapUVs(8, UVs);
	TestEqual("count", UVs.Num(), 64);
	// 首个 cell 中心 (0.5/8, 0.5/8)
	TestTrue("first", FMath::Abs(UVs[0].X - 0.0625f) < 1e-5 && FMath::Abs(UVs[0].Y - 0.0625f) < 1e-5);
	// 末个 cell 中心 (7.5/8)
	TestTrue("last", FMath::Abs(UVs[63].X - 0.9375f) < 1e-5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKgNormalizeTest, "KernelGlow.KernelMath.NormalizeWeights",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FKgNormalizeTest::RunTest(const FString& Parameters)
{
	{
		TArray<float> W = { 1.f, 1.f, 1.f, 1.f };
		KgKernelMath::NormalizeWeights(W);
		TestEqual("uniform", (int32)FMath::RoundToInt(W[0] * 100), 25);
	}
	{
		TArray<float> W = { 3.f, 1.f };
		KgKernelMath::NormalizeWeights(W);
		TestTrue("ratio kept", FMath::Abs(W[0] - 0.75f) < 1e-4 && FMath::Abs(W[1] - 0.25f) < 1e-4);
		float Sum = 0; for (float V : W) { Sum += V; }
		TestTrue("sum one", FMath::Abs(Sum - 1.f) < 1e-4);
	}
	{
		TArray<float> W = { 0.f, 0.f };
		KgKernelMath::NormalizeWeights(W); // 零和 → 均匀，不崩溃
		TestTrue("zero fallback", FMath::Abs(W[0] - 0.5f) < 1e-4);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKgProceduralKernelTest, "KernelGlow.KernelMath.GenerateProceduralKernel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FKgProceduralKernelTest::RunTest(const FString& Parameters)
{
	for (int32 Shape = 0; Shape < 3; ++Shape)
	{
		TArray<FLinearColor> Pixels;
		KgKernelMath::GenerateProceduralKernel(Shape, 64, Pixels);
		TestEqual(FString::Printf(TEXT("size shape%d"), Shape), Pixels.Num(), 64 * 64);
		// 中心必亮（三种形状的半径函数在原点都在半径内）
		TestTrue(FString::Printf(TEXT("center bright shape%d"), Shape), Pixels[32 * 64 + 32].R > 0.9f);
		// 角落必暗（[-1,-1] 超出所有形状半径）
		TestTrue(FString::Printf(TEXT("corner dark shape%d"), Shape), Pixels[0].R < 0.05f);
	}
	// CPU mip 链
	TArray<FLinearColor> Mip0;
	KgKernelMath::GenerateProceduralKernel(0, 16, Mip0);
	TArray<TArray<FLinearColor>> Mips;
	KgKernelMath::ComputeCpuMips(Mip0, 16, Mips);
	TestEqual("mip count", Mips.Num(), 4); // 16→8→4→2→1
	TestEqual("mip1 size", Mips[0].Num(), 8 * 8);
	return true;
}

#endif
