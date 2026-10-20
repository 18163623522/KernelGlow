#include "SKgPanelWindow.h"
#include "KgPanelWidgets.h"
#include "KernelGlowSettings.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "DesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "KernelGlowPanel"



namespace
{
	const FText Str(const TCHAR* T) { return FText::FromString(T); }

	TSharedRef<SWidget> MakeInfoBar()
	{
		return SNew(SBorder)
			.Padding(FMargin(12, 9))
			.BorderImage(new FSlateRoundedBoxBrush(KgPanelColor::InfoBarBg, 6.0f, FLinearColor(0.06f, 0.655f, 0.902f, 0.25f), 1.0f))
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Text(Str(TEXT("独立于引擎 Bloom：关闭引擎 Bloom（含卷积）后辉光完整保留；设置已持久化，重启 / 切关卡 / PIE 无需再开。")))
			];
	}

	TSharedRef<SWidget> MakeCardHead(const FText& Title, const FText& Subtitle, const TSharedPtr<SWidget>& Right = nullptr)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Title).ColorAndOpacity(KgPanelColor::Text)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Subtitle).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)).ColorAndOpacity(KgPanelColor::Text2)
				]
			];
		if (Right.IsValid())
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[Right.ToSharedRef()];
		}
		return Row;
	}

	TSharedRef<SWidget> MakeSliderRow(const FText& Label, float Min, float Max, float Value,
		const FOnFloatValueChanged& OnChanged, TSharedPtr<STextBlock>& OutValueText)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().FillWidth(0.42f)
			[
				SNew(STextBlock).Text(Label).ColorAndOpacity(KgPanelColor::Text)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SKgAccentSlider)
				.Min(Min).Max(Max).Value(Value)
				.OnChanged_Lambda([OnChanged](float V)
				{
					if (OnChanged.IsBound()) { OnChanged.ExecuteIfBound(V); }
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
			[
				SAssignNew(OutValueText, STextBlock)
				.Text(FText::AsNumber(Value))
				.ColorAndOpacity(KgPanelColor::Text2)
			];
	}

	TSharedRef<SWidget> MakeToggleRow(const FText& Label, bool bOn, const FSimpleDelegate& OnToggle)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Label).ColorAndOpacity(KgPanelColor::Text)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SKgToggleSwitch).bIsOn(bOn).OnToggle(OnToggle)
			];
	}

	TSharedRef<SWidget> MakeCard(const TSharedRef<SWidget>& Content)
	{
		return SNew(SBorder)
			.Padding(0)
			.BorderImage(new FSlateRoundedBoxBrush(KgPanelColor::Card, 6.0f, KgPanelColor::Stroke, 1.0f))
			[
				Content
			];
	}
}

void SKgPanelWindow::Construct(const FArguments& InArgs)
{
	UKernelGlowSettings* S = GetMutableDefault<UKernelGlowSettings>();

	// ---- 首卡：启用 / 强度 / 阈值 / kernel ----
	TSharedPtr<STextBlock> OverallVal, ThresholdVal;
	TSharedPtr<SKgToggleSwitch> EnableSwitch;

	TSharedRef<SVerticalBox> Card1 = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(14, 11)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0, 0, 10, 0)
			[
				SNew(SKgIconBadge).Char(Str(TEXT("启")))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Str(TEXT("启用 KernelGlow"))).ColorAndOpacity(KgPanelColor::Text)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Str(TEXT("默认开启 · 设置存入项目配置，跨会话保持"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)).ColorAndOpacity(KgPanelColor::Text2)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SAssignNew(EnableSwitch, SKgToggleSwitch)
				.bIsOn(S->bEnabled)
				.OnToggle(FSimpleDelegate::CreateLambda([S]()
				{
					S->bEnabled = !S->bEnabled;
					S->SaveConfig();
				}))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(14, 2, 14, 8)
		[
			MakeSliderRow(Str(TEXT("总强度")), 0.f, 5.f, S->OverallIntensity,
				FOnFloatValueChanged::CreateLambda([S, OverallVal](float V)
				{
					S->OverallIntensity = V;
					S->SaveConfig();
					if (OverallVal.IsValid()) { OverallVal->SetText(FText::AsNumber(V)); }
				}), OverallVal)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(14, 2, 14, 10)
		[
			MakeSliderRow(Str(TEXT("亮度阈值")), 0.f, 4.f, S->Threshold,
				FOnFloatValueChanged::CreateLambda([S, ThresholdVal](float V)
				{
					S->Threshold = V;
					S->SaveConfig();
					if (ThresholdVal.IsValid()) { ThresholdVal->SetText(FText::AsNumber(V)); }
				}), ThresholdVal)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(14, 0, 14, 12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Str(TEXT("Kernel 贴图 · 当前为程序化回退"))).ColorAndOpacity(KgPanelColor::Text2)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
			[
				SNew(SButton)
				.Text(Str(TEXT("选择贴图…")))
				.OnClicked_Lambda([S]()
				{
					IDesktopPlatform* Platform = FDesktopPlatformModule::Get();
					void* ParentHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
					if (Platform && ParentHandle)
					{
						TArray<FString> OutFiles;
						if (Platform->OpenFileDialog(
							ParentHandle,
							TEXT("选择 Kernel 贴图"),
							FPaths::ProjectContentDir(),
							TEXT(""),
							TEXT("贴图文件 (*.png;*.tga;*.exr)|*.png;*.tga;*.exr|全部文件 (*.*)|*.*"),
							EFileDialogFlags::None,
							OutFiles) && OutFiles.Num() > 0)
						{
							S->KernelTexturePath = FSoftObjectPath(FPackageName::CreateShortName(OutFiles[0]));
							S->SaveConfig();
						}
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SKgSelectorBar)
				.Options({ Str(TEXT("圆盘")), Str(TEXT("六边形")), Str(TEXT("星芒")) })
				.Current(S->ProceduralKernel)
				.OnChanged(FOnIntDelegate::CreateLambda([S](int32 Idx)
				{
					S->ProceduralKernel = Idx;
					S->SaveConfig();
				}))
			]
		];

	// ---- 光晕形状（折叠）----
	TSharedRef<SVerticalBox> ShapeBody = SNew(SVerticalBox);
	for (int32 I = 0; I < 3; ++I)
	{
		const FText Names[3] = { Str(TEXT("尺度 1 · 近距辉光")), Str(TEXT("尺度 2 · 中距光晕")), Str(TEXT("尺度 3 · Wide Halo")) };
		const int32 Idx = I;
		TSharedPtr<STextBlock> RadiusVal, IntensityVal;
		ShapeBody->AddSlot().AutoHeight().Padding(14, 6, 14, 0)
		[
			SNew(STextBlock).Text(Names[I]).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)).ColorAndOpacity(KgPanelColor::Text2)
		];
		ShapeBody->AddSlot().AutoHeight().Padding(14, 2, 14, 2)
		[
			MakeSliderRow(Str(TEXT("半径")), 0.01f, 1.0f, S->Scales[I].Radius,
				FOnFloatValueChanged::CreateLambda([S, Idx, RadiusVal](float V)
				{
					S->Scales[Idx].Radius = V;
					S->SaveConfig();
					if (RadiusVal.IsValid()) { RadiusVal->SetText(FText::AsNumber(V)); }
				}), RadiusVal)
		];
		ShapeBody->AddSlot().AutoHeight().Padding(14, 2, 14, 6)
		[
			MakeSliderRow(Str(TEXT("强度")), 0.f, 3.f, S->Scales[I].Intensity,
				FOnFloatValueChanged::CreateLambda([S, Idx, IntensityVal](float V)
				{
					S->Scales[Idx].Intensity = V;
					S->Scales[Idx].bEnabled = V > 0.0f;
					S->SaveConfig();
					if (IntensityVal.IsValid()) { IntensityVal->SetText(FText::AsNumber(V)); }
				}), IntensityVal)
		];
	}
	TSharedPtr<STextBlock> CoreRadiusVal, CoreStrengthVal;
	ShapeBody->AddSlot().AutoHeight().Padding(14, 6, 14, 0)
	[
		SNew(STextBlock).Text(Str(TEXT("核心回填 · 鸣潮式清晰核心"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)).ColorAndOpacity(KgPanelColor::Text2)
	];
	ShapeBody->AddSlot().AutoHeight().Padding(14, 2, 14, 10)
	[
		MakeSliderRow(Str(TEXT("核心半径")), 0.f, 16.f, S->Core.Radius,
			FOnFloatValueChanged::CreateLambda([S, CoreRadiusVal](float V)
			{
				S->Core.Radius = V;
				S->SaveConfig();
				if (CoreRadiusVal.IsValid()) { CoreRadiusVal->SetText(FText::AsNumber(V)); }
			}), CoreRadiusVal)
	];
	ShapeBody->AddSlot().AutoHeight().Padding(14, 2, 14, 10)
	[
		MakeSliderRow(Str(TEXT("核心强度")), 0.f, 2.f, S->Core.Strength,
			FOnFloatValueChanged::CreateLambda([S, CoreStrengthVal](float V)
			{
				S->Core.Strength = V;
				S->SaveConfig();
				if (CoreStrengthVal.IsValid()) { CoreStrengthVal->SetText(FText::AsNumber(V)); }
			}), CoreStrengthVal)
	];

	// ---- 来源（折叠）----
	TSharedPtr<SKgToggleSwitch> LuminanceSwitch;
	TSharedRef<SVerticalBox> SourcesBody = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(14, 8, 14, 8)
		[
			MakeToggleRow(Str(TEXT("全屏软阈值亮源（引擎式全局溢出）")), S->bUseFullScreenLuminance,
				FSimpleDelegate::CreateLambda([S]()
				{
					S->bUseFullScreenLuminance = !S->bUseFullScreenLuminance;
					S->SaveConfig();
				}))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(14, 2, 14, 10)
		[
			SNew(STextBlock)
			.Text(Str(TEXT("Custom Stencil 白名单：实验性（当前引擎版本 SRV 绑定已知问题）；自定义 RT 源接口已预留")))
			.AutoWrapText(true)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(KgPanelColor::Text3)
		];

	// ---- 高级（折叠）----
	TSharedPtr<SKgSelectorBar> QualitySelector;
	TSharedRef<SVerticalBox> AdvancedBody = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(14, 4, 14, 8)
		[
			MakeToggleRow(Str(TEXT("HDR 合成（Tonemap 前）")), S->bCompositeHDR,
				FSimpleDelegate::CreateLambda([S]()
				{
					S->bCompositeHDR = !S->bCompositeHDR;
					S->SaveConfig();
				}))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(14, 2, 14, 8)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Str(TEXT("质量档"))).ColorAndOpacity(KgPanelColor::Text)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SAssignNew(QualitySelector, SKgSelectorBar)
				.Options({ Str(TEXT("低")), Str(TEXT("高")), Str(TEXT("电影级")) })
				.Current(S->Quality)
				.OnChanged(FOnIntDelegate::CreateLambda([S](int32 Idx)
				{
					S->Quality = Idx;
					S->SaveConfig();
				}))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(14, 2, 14, 10)
		[
			MakeToggleRow(Str(TEXT("Fast 模式")), S->bFastMode,
				FSimpleDelegate::CreateLambda([S]()
				{
					S->bFastMode = !S->bFastMode;
					S->SaveConfig();
				}))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(14, 2, 14, 10)
		[
			SNew(STextBlock)
			.Text(Str(TEXT("成本由 Tap 预算与金字塔级数决定，与 kernel 尺寸无关 · 2560×1440 高档目标 ≤ 1.5 ms")))
			.AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)).ColorAndOpacity(KgPanelColor::Text3)
		];

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(14, 12)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				MakeInfoBar()
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				MakeCard(Card1)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SKgExpanderCard)
				.Icon(Str(TEXT("形")))
				.Title(Str(TEXT("光晕形状")))
				.Subtitle(Str(TEXT("三层尺度 + 核心回填")))
				.Summary(Str(TEXT("3 层 · 回填开")))
				.Body()
				[
					ShapeBody
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SKgExpanderCard)
				.Icon(Str(TEXT("源")))
				.Title(Str(TEXT("来源")))
				.Subtitle(Str(TEXT("只有标记的高光参与卷积")))
				.Summary(FText::FromString(S->bUseFullScreenLuminance ? TEXT("亮度源") : TEXT("白名单")))
				.Body()
				[
					SourcesBody
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SKgExpanderCard)
				.Icon(Str(TEXT("高")))
				.Title(Str(TEXT("高级")))
				.Subtitle(Str(TEXT("合成 · 调试 · 性能")))
				.Summary(Str(TEXT("HDR · 高")))
				.Body()
				[
					AdvancedBody
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(Str(TEXT("v0.1.0 · GPU 成本与 kernel 尺寸无关 · UE 5.7 Win64")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(KgPanelColor::Text3)
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
