#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KernelGlowSettings.generated.h"

class UTexture2D;
class UTextureRenderTarget2D;

/** 单个卷积尺度层 */
USTRUCT(BlueprintType)
struct FKGScaleLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, config, Category="Scale")
	bool bEnabled = true;

	/** 半径（屏幕占比，长边归一） */
	UPROPERTY(EditAnywhere, config, Category="Scale", meta=(ClampMin="0.01", ClampMax="1.0"))
	float Radius = 0.06f;

	UPROPERTY(EditAnywhere, config, Category="Scale", meta=(ClampMin="0.0", ClampMax="3.0"))
	float Intensity = 0.8f;

	UPROPERTY(EditAnywhere, config, Category="Scale")
	FLinearColor Tint = FLinearColor::White;
};

/** 核心回填（全分辨率小核锐层） */
USTRUCT(BlueprintType)
struct FKGCoreBackfill
{
	GENERATED_BODY()

	/** 半径（像素） */
	UPROPERTY(EditAnywhere, config, Category="Core", meta=(ClampMin="0.0", ClampMax="16.0"))
	float Radius = 3.0f;

	UPROPERTY(EditAnywhere, config, Category="Core", meta=(ClampMin="0.0", ClampMax="2.0"))
	float Strength = 0.6f;
};

/**
 * KernelGlow 设置（config=Engine 持久化；启用后默认开启，重启/切关卡/PIE 保持）
 */
UCLASS(config=Engine, defaultconfig, meta=(DisplayName="KernelGlow"))
class UKernelGlowSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UKernelGlowSettings();

	//~ General
	UPROPERTY(EditAnywhere, config, Category="General")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, config, Category="General", meta=(ClampMin="0.0", ClampMax="5.0"))
	float OverallIntensity = 1.2f;

	/** 亮度阈值（软膝部） */
	UPROPERTY(EditAnywhere, config, Category="General", meta=(ClampMin="0.0", ClampMax="4.0"))
	float Threshold = 1.0f;

	/** 0=低 1=高 2=电影级 */
	UPROPERTY(EditAnywhere, config, Category="General")
	int32 Quality = 1;

	//~ Scales（近/中/Wide 三层）
	UPROPERTY(EditAnywhere, config, Category="Scales")
	FKGScaleLayer Scales[3];

	UPROPERTY(EditAnywhere, config, Category="Scales")
	FKGCoreBackfill Core;

	//~ Composite
	/** true=HDR（Tonemap 前，推荐）false=Tonemap 后 */
	UPROPERTY(EditAnywhere, config, Category="Composite")
	bool bCompositeHDR = true;

	UPROPERTY(EditAnywhere, config, Category="Composite", meta=(ClampMin="0.0", ClampMax="2.0"))
	float Gain = 1.0f;

	//~ Performance
	/** 0=4x4 1=8x8 2=16x16 */
	UPROPERTY(EditAnywhere, config, Category="Performance")
	int32 TapBudget = 1;

	UPROPERTY(EditAnywhere, config, Category="Performance")
	bool bFastMode = false;

	//~ Sources
	UPROPERTY(EditAnywhere, config, Category="Sources")
	TArray<int32> StencilValues;

	/** 外部 RT（运行时注入，不经配置持久化） */
	UPROPERTY(Transient, VisibleAnywhere, Category="Sources")
	TObjectPtr<UTextureRenderTarget2D> SourceRT = nullptr;

	/** 全屏软阈值亮源（默认关） */
	UPROPERTY(EditAnywhere, config, Category="Sources")
	bool bUseFullScreenLuminance = false;

	//~ Kernel
	/** kernel 贴图路径（config 持久化；FSoftObjectPath 规避 config 不支持对象引用） */
	UPROPERTY(EditAnywhere, config, Category="Kernel")
	FSoftObjectPath KernelTexturePath;

	/** 能量归一化（防糊防灰的关键，默认开） */
	UPROPERTY(EditAnywhere, config, Category="Kernel")
	bool bNormalizeKernel = true;

	/** 程序化回退：0=圆盘 1=六边形 2=星芒 */
	UPROPERTY(EditAnywhere, config, Category="Kernel")
	int32 ProceduralKernel = 0;

	/** 亮度钳制（kernel 峰值权重上限） */
	UPROPERTY(EditAnywhere, config, Category="Kernel", meta=(ClampMin="0.2", ClampMax="4.0"))
	float KernelClamp = 1.0f;

	virtual FName GetCategoryName() const override;
};
