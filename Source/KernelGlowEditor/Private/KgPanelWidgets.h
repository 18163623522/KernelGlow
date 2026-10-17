#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"
#include "Styling/SlateTypes.h"
#include "Framework/SlateDelegates.h"
#include "Widgets/Input/SNumericEntryBox.h"


/**
 * KernelGlow 编辑器 Fluent 风格自绘控件（WinUI3 风格，Task 10）
 */

// 强调色（青蓝，与 UI 原型一致）
DECLARE_DELEGATE_OneParam(FSimpleIntDelegate, int32)

namespace KgPanelColor
{
	static const FLinearColor Accent = FLinearColor(0.059f, 0.655f, 0.902f);           // #0FA7E6
	static const FLinearColor Card = FLinearColor(0.086f, 0.086f, 0.086f, 0.9f);
	static const FLinearColor CardHover = FLinearColor(0.12f, 0.12f, 0.12f, 0.95f);
	static const FLinearColor Stroke = FLinearColor(1, 1, 1, 0.08f);
	static const FLinearColor Text = FLinearColor(1, 1, 1, 0.95f);
	static const FLinearColor Text2 = FLinearColor(1, 1, 1, 0.6f);
	static const FLinearColor Text3 = FLinearColor(1, 1, 1, 0.4f);
	static const FLinearColor Track = FLinearColor(1, 1, 1, 0.22f);
	static const FLinearColor InfoBarBg = FLinearColor(0.059f, 0.655f, 0.902f, 0.12f);
	static const FLinearColor Control = FLinearColor(1, 1, 1, 0.06f);
}

/** Fluent 开关（40x20 轨道 + 圆形滑块，自绘） */
class SKgToggleSwitch : public SButton
{
public:
	SLATE_BEGIN_ARGS(SKgToggleSwitch)
		: _bIsOn(false)
		, _OnToggle()
		{}
		SLATE_ARGUMENT(bool, bIsOn)
		SLATE_EVENT(FSimpleDelegate, OnToggle)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		bIsOn = InArgs._bIsOn;
		OnToggle = InArgs._OnToggle;
		SButton::FArguments Args;
		SButton::Construct(Args
			.OnClicked(this, &SKgToggleSwitch::HandleClicked)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(0))
			[
				SNullWidget::NullWidget
			]);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(40, 20);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FLinearColor TrackCol = bIsOn ? KgPanelColor::Accent : KgPanelColor::Track;
		const FSlateRoundedBoxBrush TrackBrush(TrackCol, 10.0f);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D(40, 20), FSlateLayoutTransform()),
			&TrackBrush);

		const float KnobX = bIsOn ? 23.0f : 3.0f;
		const FSlateRoundedBoxBrush KnobBrush(bIsOn ? FLinearColor::White : KgPanelColor::Text2, 6.0f);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(12, 12), FSlateLayoutTransform(FVector2D(KnobX, 3.0f))),
			&KnobBrush);
		return LayerId + 1;
	}

	void SetIsOn(bool InOn) { bIsOn = InOn; }

private:
	FReply HandleClicked()
	{
		bIsOn = !bIsOn;
		if (OnToggle.IsBound())
		{
			OnToggle.Execute();
		}
		return FReply::Handled();
	}

	bool bIsOn = false;
	FSimpleDelegate OnToggle;
};

/** Fluent 折叠卡片：标题行（可点击展开/收起）+ 内容插槽 */
class SKgExpanderCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKgExpanderCard)
		: _Title()
		, _Subtitle()
		, _bExpanded(false)
		{}
		SLATE_ATTRIBUTE(FText, Title)
		SLATE_ATTRIBUTE(FText, Subtitle)
		SLATE_ARGUMENT(bool, bExpanded)
		SLATE_DEFAULT_SLOT(FArguments, Body)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		bExpanded = InArgs._bExpanded;
		TitleAttr = InArgs._Title;
		SubtitleAttr = InArgs._Subtitle;

		static FSlateRoundedBoxBrush CardBrush(KgPanelColor::Card, 6.0f, KgPanelColor::Stroke, 1.0f);
		static FSlateRoundedBoxBrush CardHoverBrush(KgPanelColor::CardHover, 6.0f, KgPanelColor::Stroke, 1.0f);

		ChildSlot
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage_Lambda([]() -> const FSlateBrush*
			{
				// 简化：使用常驻卡片底色（hover 态略）
				static FSlateRoundedBoxBrush B(KgPanelColor::Card, 6.0f, KgPanelColor::Stroke, 1.0f);
				return &B;
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(14, 11))
					.OnClicked(this, &SKgExpanderCard::HandleHeaderClicked)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(TitleAttr).ColorAndOpacity(KgPanelColor::Text)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(SubtitleAttr).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)).ColorAndOpacity(KgPanelColor::Text2)
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return bExpanded ? FText::FromString(TEXT("  ^  ")) : FText::FromString(TEXT("  v  ")); })
							.ColorAndOpacity(KgPanelColor::Text3)
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return bExpanded ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						InArgs._Body.Widget
					]
				]
			]
		];
	}

private:
	FReply HandleHeaderClicked()
	{
		bExpanded = !bExpanded;
		return FReply::Handled();
	}

	bool bExpanded = false;
	TAttribute<FText> TitleAttr;
	TAttribute<FText> SubtitleAttr;
};

/** Fluent 选择条：互斥按钮组，选中项强调色填充 */
class SKgSelectorBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKgSelectorBar)
		: _Options()
		, _Current(0)
		, _OnChanged()
		{}
		SLATE_ARGUMENT(TArray<FText>, Options)
		SLATE_ARGUMENT(int32, Current)
		SLATE_EVENT(FSimpleIntDelegate, OnChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Current = InArgs._Current;
		OnChanged = InArgs._OnChanged;

		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		const TArray<FText> Opts = InArgs._Options;
		for (int32 I = 0; I < Opts.Num(); ++I)
		{
			const int32 Idx = I;
			Row->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
			[
				SNew(SButton)
				.OnClicked(this, &SKgSelectorBar::HandleOpt, Idx)
				.ButtonColorAndOpacity_Lambda([this, Idx]() { return Idx == Current ? KgPanelColor::Accent : KgPanelColor::Control; })
				.ContentPadding(FMargin(11, 5))
				[
					SNew(STextBlock)
					.Text(Opts[I])
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					.ColorAndOpacity_Lambda([this, Idx]() { return Idx == Current ? FLinearColor::White : KgPanelColor::Text2; })
				]
			];
		}

		ChildSlot
		[
			Row
		];
	}

	void SetCurrent(int32 InCurrent) { Current = InCurrent; }

private:
	FReply HandleOpt(int32 Idx)
	{
		Current = Idx;
		if (OnChanged.IsBound())
		{
			OnChanged.ExecuteIfBound(Idx);
		}
		return FReply::Handled();
	}

	int32 Current = 0;
	FSimpleIntDelegate OnChanged;
};
