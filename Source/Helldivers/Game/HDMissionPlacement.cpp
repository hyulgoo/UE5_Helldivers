// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/HDMissionPlacement.h"

namespace HDMissionPlacement
{
    namespace
    {
        // 간격을 못 맞춘 점을 다시 뽑는 최대 횟수(첫 뽑기 제외)
        constexpr int32 MaxRetries = 30;

        // 이미 받은 점들 중 가장 가까운 점까지의 거리. 받은 점이 없으면 최댓값
        double NearestDistance(const TArray<FVector2D>& Accepted, const FVector2D& Candidate)
        {
            double Nearest = TNumericLimits<double>::Max();
            for (const FVector2D& Point : Accepted)
            {
                Nearest = FMath::Min(Nearest, FVector2D::Distance(Point, Candidate));
            }
            return Nearest;
        }
    }

    TArray<FVector2D> ComputeRingOffsets(int32 Count, float MinRadius, float MaxRadius, float MinSeparation, FRandomStream& Stream)
    {
        TArray<FVector2D> Offsets;
        if (Count <= 0)
        {
            return Offsets;
        }

        Offsets.Reserve(Count);

        // 점 i 의 기본 각도 = 시작 + i * 360/Count, 흔들림 ±(180/Count * 0.5)
        const double StartAngle = Stream.FRandRange(0.0, 360.0);
        const double AngleStep = 360.0 / Count;
        const double AngleJitter = 180.0 / Count * 0.5;

        for (int32 Index = 0; Index < Count; ++Index)
        {
            const double BaseAngle = StartAngle + Index * AngleStep;

            // 간격을 못 맞추면 지금까지 중 가장 가까운 점과의 거리가 가장 큰 후보를 받는다
            FVector2D BestCandidate = FVector2D::ZeroVector;
            double BestNearest = -1.0;

            for (int32 Attempt = 0; Attempt <= MaxRetries; ++Attempt)
            {
                const double Angle = FMath::DegreesToRadians(BaseAngle + Stream.FRandRange(-AngleJitter, AngleJitter));
                const double Radius = Stream.FRandRange(MinRadius, MaxRadius);
                const FVector2D Candidate(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);

                const double Nearest = NearestDistance(Offsets, Candidate);
                if (Nearest >= MinSeparation)
                {
                    BestCandidate = Candidate;
                    break;
                }

                if (Nearest > BestNearest)
                {
                    BestCandidate = Candidate;
                    BestNearest = Nearest;
                }
            }

            Offsets.Add(BestCandidate);
        }

        return Offsets;
    }
}
