#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Styling/SlateTypes.h"
#include "Framework/SlateDelegates.h"

/**
 * KernelGlow 编辑器 Fluent 风格自绘控件（WinUI3 风格 + 过渡动画，Task 10）
 */

// 强调色（青蓝，与 UI 原型一致）
DECLARE_DELEGATE_OneParam(FOnIntDelegate, int32)

namespace KgPanelColor
{
	static const FLinearColor Accent = FLinearColor(0.059f, 0.655f, 0.902f);           // #0FA7E6
	static const FLinearColor AccentHover = FLinearColor(0.13f, 0.78f, 1.0f);
	static const FLinearColor Card = FLinearColor(0.086f, 0.086f, 0.086f, 0.9f);
	static const FLinearColor CardHover = FLinearColor(0.13f, 0.13f, 0.13f, 0.95f);
	static const FLinearColor Stroke = FLinearColor(1, 1, 1, 0.08f);
	static const FLinearColor Text = FLinearColor(1, 1, 1, 0.95f);
	static const FLinearColor Text2 = FLinearColor(1, 1, 1, 0.65f);
	static const FLinearColor Text3 = FLinearColor(1, 1, 1, 0.45f);
	static const FLinearColor TrackOff = FLinearColor(1, 1, 1, 0.38f);                 // 提高关闭态对比
	static const FLinearColor InfoBarBg = FLinearColor(0.059f, 0.655f, 0.902f, 0.12f);
	static const FLinearColor Control = FLinearColor(1, 1, 1, 0.10f);
	static const FLinearColor ControlStroke = FLinearColor(1, 1, 1, 0.24f);
}

/** 按钮三态样式（选中=实心强调色；未选中=描边高对比） */
namespace KgPanelStyle
{
	static FButtonStyle Selected()
	{
		FButtonStyle St;
		St.SetNormal(FSlateRoundedBoxBrush(KgPanelColor::Accent, 4.0f));
		St.SetHovered(FSlateRoundedBoxBrush(KgPanelColor::AccentHover, 4.0f));
		St.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.5f, 0.72f), 4.0f));
		return St;
	}
	static FButtonStyle Unselected()
	{
		FButtonStyle St;
		St.SetNormal(FSlateRoundedBoxBrush(FLinearColor(1, 1, 1, 0.10f), 4.0f, FLinearColor(1, 1, 1, 0.26f), 1.0f));
		St.SetHovered(FSlateRoundedBoxBrush(FLinearColor(1, 1, 1, 0.18f), 4.0f, FLinearColor(1, 1, 1, 0.40f), 1.0f));
		St.SetPressed(FSlateRoundedBoxBrush(FLinearColor(1, 1, 1, 0.24f), 4.0f, FLinearColor(1, 1, 1, 0.32f), 1.0f));
		return St;
	}
}

/** WinUI3 风格开关：40x20 轨道 + 圆形滑块，滑动/变色带过渡动画 */
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
		Anim = bIsOn ? 1.0f : 0.0f;
		OnToggle = InArgs._OnToggle;
		SetCanTick(true);
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

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float DeltaTime) override
	{
		const float Target = bIsOn ? 1.0f : 0.0f;
		if (!FMath::IsNearlyEqual(Anim, Target, 0.005f))
		{
			Anim = FMath::FInterpTo(Anim, Target, DeltaTime, 16.0f);
			if (FMath::IsNearlyEqual(Anim, Target, 0.005f)) { Anim = Target; }
			Invalidate(EInvalidateWidgetReason::Paint);
		}
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		// 颜色烘焙进 static brush（临时 brush 延迟绘制时会析构变白）；动画只动滑块位置
		static FSlateRoundedBoxBrush TrackOn(KgPanelColor::Accent, 10.0f);
		static FSlateRoundedBoxBrush TrackOff(KgPanelColor::TrackOff, 10.0f);
		const FSlateBrush* Track = (Anim >= 0.5f) ? static_cast<const FSlateBrush*>(&TrackOn) : static_cast<const FSlateBrush*>(&TrackOff);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D(40, 20), FSlateLayoutTransform()),
			Track);

		const float KnobX = FMath::Lerp(3.0f, 23.0f, Anim);
		static FSlateRoundedBoxBrush KnobBrush(FLinearColor::White, 6.0f);
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
	float Anim = 0.0f;
	FSimpleDelegate OnToggle;
};

/** WinUI3 风格滑杆：强调色已填充轨道 + 圆形滑块（自绘，替代引擎默认 SSlider 外观） */
class SKgAccentSlider : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKgAccentSlider)
		: _Min(0.f), _Max(1.f), _Value(0.5f), _OnChanged()
		{}
		SLATE_ARGUMENT(float, Min)
		SLATE_ARGUMENT(float, Max)
		SLATE_ARGUMENT(float, Value)
		SLATE_EVENT(FOnFloatValueChanged, OnChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Min = InArgs._Min;
		Max = FMath::Max(InArgs._Max, InArgs._Min + 0.0001f);
		Value = FMath::Clamp(InArgs._Value, Min, Max);
		OnChanged = InArgs._OnChanged;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(220, 20);
	}

	float Fraction() const
	{
		return FMath::Clamp((Value - Min) / (Max - Min), 0.0f, 1.0f);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const float W = AllottedGeometry.GetLocalSize().X;
		const float H = AllottedGeometry.GetLocalSize().Y;
		const float TrackY = H * 0.5f - 2.0f;
		const float Frac = Fraction();

		static FSlateRoundedBoxBrush TrackBrush(FLinearColor::White, 2.0f);
		TrackBrush.TintColor = FSlateColor(KgPanelColor::TrackOff);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D(W, 4.0f), FSlateLayoutTransform(FVector2D(0, TrackY))),
			&TrackBrush);

		if (Frac > 0.001f)
		{
			static FSlateRoundedBoxBrush FillBrush(FLinearColor::White, 2.0f);
			FillBrush.TintColor = FSlateColor(KgPanelColor::Accent);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(FVector2D(FMath::Max(W * Frac, 4.0f), 4.0f), FSlateLayoutTransform(FVector2D(0, TrackY))),
				&FillBrush);
		}

		const bool bHover = IsHovered() || bDragging;
		const float ThumbR = bHover ? 9.0f : 7.0f;
		const float ThumbCx = W * Frac;
		static FSlateRoundedBoxBrush ThumbRing(FLinearColor::White, 9.0f);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2D(ThumbR * 2, ThumbR * 2), FSlateLayoutTransform(FVector2D(ThumbCx - ThumbR, H * 0.5f - ThumbR))),
			&ThumbRing);
		static FSlateRoundedBoxBrush ThumbBrush(FLinearColor::White, 8.0f);
		ThumbBrush.TintColor = FSlateColor(KgPanelColor::Accent);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FVector2D((ThumbR - 1.0f) * 2, (ThumbR - 1.0f) * 2), FSlateLayoutTransform(FVector2D(ThumbCx - ThumbR + 1.0f, H * 0.5f - ThumbR + 1.0f))),
			&ThumbBrush);

		return LayerId + 3;
	}

protected:
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			return FReply::Unhandled();
		}
		bDragging = true;
		return SetValueFromGeometry(MyGeometry, MouseEvent.GetScreenSpacePosition());
	}

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (bDragging && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			return SetValueFromGeometry(MyGeometry, MouseEvent.GetScreenSpacePosition());
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		bDragging = false;
		return FReply::Handled();
	}

private:
	FReply SetValueFromGeometry(const FGeometry& G, const FVector2D& ScreenPos)
	{
		const FVector2D Local = G.AbsoluteToLocal(ScreenPos);
		const float W = FMath::Max(G.GetLocalSize().X, 1.0f);
		const float Frac = FMath::Clamp(Local.X / W, 0.0f, 1.0f);
		const float NewValue = Min + Frac * (Max - Min);
		if (!FMath::IsNearlyEqual(NewValue, Value, (Max - Min) / 500.0f))
		{
			Value = NewValue;
			if (OnChanged.IsBound())
			{
				OnChanged.Execute(Value);
			}
		}
		return FReply::Handled();
	}

	float Min = 0.0f;
	float Max = 1.0f;
	float Value = 0.5f;
	bool bDragging = false;
	FOnFloatValueChanged OnChanged;
};

/** 卡片头图标：强调色圆角方块 + 单字 */
class SKgIconBadge : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKgIconBadge) : _Char(FText::FromString(TEXT("启"))) {}
		SLATE_ARGUMENT(FText, Char)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SBox).WidthOverride(22).HeightOverride(22)
			[
				SNew(SBorder)
				.Padding(FMargin(0))
				.BorderImage_Lambda([]() { static FSlateRoundedBoxBrush B(KgPanelColor::Accent, 5.0f); return &B; })
				[
					SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(InArgs._Char)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
			]
		];
	}
};

/** Fluent 折叠卡片：图标 + 标题 + 右侧摘要 + 展开/收起滑入淡入动画 */
class SKgExpanderCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKgExpanderCard)
		: _Icon()
		, _Title()
		, _Subtitle()
		, _Summary()
		, _bExpanded(false)
		{}
		SLATE_ARGUMENT(FText, Icon)
		SLATE_ATTRIBUTE(FText, Title)
		SLATE_ATTRIBUTE(FText, Subtitle)
		SLATE_ATTRIBUTE(FText, Summary)
		SLATE_ARGUMENT(bool, bExpanded)
		SLATE_DEFAULT_SLOT(FArguments, Body)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		bExpanded = InArgs._bExpanded;
		AnimOpen = bExpanded ? 1.0f : 0.0f;
		IconAttr = InArgs._Icon;
		TitleAttr = InArgs._Title;
		SubtitleAttr = InArgs._Subtitle;
		SummaryAttr = InArgs._Summary;
		SetCanTick(true);

		static FSlateRoundedBoxBrush CardBrush(KgPanelColor::Card, 6.0f, KgPanelColor::Stroke, 1.0f);

		TSharedRef<SBox> BodyBoxRef = SAssignNew(BodyBox, SBox)
			.Visibility(bExpanded ? EVisibility::Visible : EVisibility::Collapsed)
			.RenderOpacity(AnimOpen)
			[
				InArgs._Body.Widget
			];

		ChildSlot
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(&CardBrush)
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
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
						[
							SNew(SKgIconBadge).Char(IconAttr.Get())
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(10, 0, 0, 0)
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
							SNew(STextBlock).Text(SummaryAttr).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)).ColorAndOpacity(KgPanelColor::Text3)
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
					BodyBoxRef
				]
			]
		];
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float DeltaTime) override
	{
		const float Target = bExpanded ? 1.0f : 0.0f;
		if (!FMath::IsNearlyEqual(AnimOpen, Target, 0.01f))
		{
			AnimOpen = FMath::FInterpTo(AnimOpen, Target, DeltaTime, 14.0f);
			if (FMath::IsNearlyEqual(AnimOpen, Target, 0.01f)) { AnimOpen = Target; }
			if (BodyBox.IsValid())
			{
				BodyBox->SetRenderOpacity(AnimOpen);
				BodyBox->SetRenderTransform(FSlateRenderTransform(FVector2D(0.0f, -10.0f * (1.0f - AnimOpen))));
			}
			Invalidate(EInvalidateWidgetReason::Paint);
		}
	}

private:
	FReply HandleHeaderClicked()
	{
		bExpanded = !bExpanded;
		if (bExpanded && BodyBox.IsValid())
		{
			BodyBox->SetVisibility(EVisibility::Visible);
			BodyBox->SetRenderOpacity(0.0f);
		}
		Invalidate(EInvalidateWidgetReason::Paint);
		return FReply::Handled();
	}

	bool bExpanded = false;
	float AnimOpen = 0.0f;
	TSharedPtr<SBox> BodyBox;
	TAttribute<FText> IconAttr;
	TAttribute<FText> TitleAttr;
	TAttribute<FText> SubtitleAttr;
	TAttribute<FText> SummaryAttr;
};

/** Fluent 选择条：互斥按钮组（高对比描边/实心两态），选中即时切换 */
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
		SLATE_EVENT(FOnIntDelegate, OnChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Current = InArgs._Current;
		OnChanged = InArgs._OnChanged;
		Options = InArgs._Options;

		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		RowBox = Row;
		RebuildButtons(&Row.Get());
		ChildSlot[Row];
	}

	void SetCurrent(int32 InCurrent)
	{
		if (Current != InCurrent)
		{
			Current = InCurrent;
			RebuildButtons(&RowBox.ToSharedRef().Get());
		}
	}

private:
	void RebuildButtons(SHorizontalBox* Row)
	{
		Row->ClearChildren();
		static FButtonStyle SelStyle = KgPanelStyle::Selected();
		static FButtonStyle UnselStyle = KgPanelStyle::Unselected();
		for (int32 I = 0; I < Options.Num(); ++I)
		{
			const int32 Idx = I;
			Row->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
			[
				SNew(SButton)
				.ButtonStyle(Idx == Current ? &SelStyle : &UnselStyle)
				.ContentPadding(FMargin(11, 5))
				[
					SNew(STextBlock)
					.Text(Options[I])
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					.ColorAndOpacity(Idx == Current ? FLinearColor::White : FLinearColor(1, 1, 1, 0.88f))
				]
			];
		}
	}

	FReply HandleOpt(int32 Idx)
	{
		if (Current != Idx)
		{
			Current = Idx;
			RebuildButtons(&RowBox.ToSharedRef().Get());
			if (OnChanged.IsBound())
			{
				OnChanged.ExecuteIfBound(Idx);
			}
		}
		return FReply::Handled();
	}

	int32 Current = 0;
	TArray<FText> Options;
	TSharedPtr<SHorizontalBox> RowBox;
	FOnIntDelegate OnChanged;

	};
