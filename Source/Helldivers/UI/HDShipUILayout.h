// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

// 한 화면 크기에서의 함선 화면 영역(화면 좌표). HDShipUILayout::ComputeFrame 이 만든다
struct FHDShipUIFrame
{
    FBox2D              View            = FBox2D(ForceInit);    // 화면 전체
    FBox2D              Safe            = FBox2D(ForceInit);    // 가운데 16:9
    float               Scale           = 1.f;
    FBox2D              TopBar          = FBox2D(ForceInit);
    FBox2D              Content         = FBox2D(ForceInit);
    FBox2D              BottomBar       = FBox2D(ForceInit);
    FBox2D              Main            = FBox2D(ForceInit);    // 본문 왼쪽
    FBox2D              Detail          = FBox2D(ForceInit);    // 본문 오른쪽 상세 패널
};

// 함선 화면 배치. 월드·캔버스와 무관한 순수 함수라 자동 테스트(Helldivers.FrontEnd.ShipUI)로 고정한다
namespace HDShipUILayout
{
    // 안전 영역 기준 설계 영역: 위 바 y 0~88, 본문 104~1000(x 48~1872), 아래 바 1016~1080, 주 영역 x 48~1336, 상세 x 1352~1872. 바는 안전 영역 폭 전체
    HELLDIVERS_API FHDShipUIFrame   ComputeFrame(const FVector2D& ViewSize);
    // 안전 영역 기준 설계 좌표 → 화면 좌표
    HELLDIVERS_API FBox2D           ToScreen(const FHDShipUIFrame& Frame, const FBox2D& DesignRect);
    // 주 영역 위에서부터 RowWidth×RowHeight 줄(사이 RowGap). bCentered 면 주 영역+상세(본문) 가운데(System)
    HELLDIVERS_API TArray<FBox2D>   ComputeListRows(const FHDShipUIFrame& Frame, int32 Count, bool bCentered);

    // 글자 폭(px)을 재는 함수. 실행 중에는 캔버스 도구가, 테스트에서는 가짜가 준다
    using FMeasureFn = TFunctionRef<float(const FString& Text, int32 FontPx)>;

    struct FHDFitResult
    {
        FString         Text;
        int32           FontPx          = 0;
        bool            bTruncated      = false;
    };
    // FontPx 부터 MinFontPx 까지 1px 씩 줄여 맞추고, 그래도 넘치면 최소 크기에서 뒤를 잘라 "..." 를 붙인다. "..." 도 안 들어가면 빈 글자
    HELLDIVERS_API FHDFitResult     FitText(const FString& Text, float MaxWidth, int32 FontPx, int32 MinFontPx, FMeasureFn Measure);
    // 단어 사이(공백)에서 줄을 바꾼다. 한 단어가 넘치면 그 줄을 말줄임한다. MaxLines(0 이하면 제한 없음)를 넘으면 마지막 줄을 "..." 로 끝낸다. 글꼴은 줄이지 않는다
    HELLDIVERS_API TArray<FString>  WrapText(const FString& Text, float MaxWidth, int32 FontPx, int32 MaxLines, FMeasureFn Measure);

    // 그린 글자 하나. Region 은 그 글자가 있어야 할 영역(비어 있으면 화면만 본다)
    struct FHDDrawnText
    {
        FBox2D          Rect            = FBox2D(ForceInit);
        FBox2D          Region          = FBox2D(ForceInit);
        FString         Text;
        bool            bTruncated      = false;
    };
    struct FHDTextProblems
    {
        int32           Texts           = 0;
        int32           Overlaps        = 0;
        int32           Clipped         = 0;
        int32           Truncated       = 0;
        FString         FirstProblem;
    };
    // 넓이가 있는 겹침만 센다(모서리 닿음은 아님, 쌍마다 1). 화면이나 Region 밖으로 0.5px 넘게 나가면 잘림(글자마다 1).
    // FirstProblem = 겹침·잘림이 처음 난 글자(겹침은 뒤에 그린 쪽)
    HELLDIVERS_API FHDTextProblems  FindTextProblems(const TArray<FHDDrawnText>& Texts, const FBox2D& View);

    // 함선 화면 히트박스 종류: ShipItem_<A>, ShipCard_<A>_<B>, ShipTab_<A>, ShipPick_<A>, ShipKey_<EHDShipMenuKey 정수>
    enum class EHDShipHitKind : uint8
    {
        None,
        Item,
        Card,
        Tab,
        Pick,
        Key
    };
    HELLDIVERS_API FName            MakeHitBoxName(EHDShipHitKind Kind, int32 A, int32 B = 0);
    // 위 다섯 꼴이 아니면 거짓(ShipTitle·ResultReturn 도 거짓). 숫자는 0 이상
    HELLDIVERS_API bool             ParseHitBoxName(FName Name, EHDShipHitKind& OutKind, int32& OutA, int32& OutB);

    // 화면을 열거나 바꾼 뒤 마우스가 Threshold(px) 넘게 움직였는가
    HELLDIVERS_API bool             HasMouseMoved(const FVector2D& Anchor, const FVector2D& Now, float Threshold = 3.f);
}

// 열기·닫기·화면 바꾸기의 투명도와 미끄러짐. HUD 가 RenderDelta 로 돌린다
class HELLDIVERS_API FHDShipUITransition
{
public:
    // 지금 투명도에서 1 까지 Seconds 동안. 본문 투명도는 1 로. 0 이하면 바로
    void                Open(float Seconds);
    // 지금 투명도에서 0 까지 Seconds 동안. 0 이하면 바로
    void                Close(float Seconds);
    // 본문 투명도만 0 에서 1 까지 Seconds 동안
    void                ChangeScreen(float Seconds);
    void                Tick(float DeltaSeconds);
    float               GetAlpha() const;           // 틀 전체
    float               GetContentAlpha() const;    // 본문(화면 바꾸기). 그릴 때 틀 투명도와 곱한다
    float               GetSlide() const;           // 여는 중 1 → 0(닫는 중 0). 그릴 때 아래로 16 × 배율 × Slide 에서 올라온다
    bool                IsVisible() const;

private:
    float               Alpha           = 0.f;
    float               TargetAlpha     = 0.f;
    float               AlphaRate       = 0.f;
    float               ContentAlpha    = 1.f;
    float               ContentRate     = 0.f;
};
