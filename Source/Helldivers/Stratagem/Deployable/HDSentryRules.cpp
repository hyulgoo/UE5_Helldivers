// Fill out your copyright notice in the Description page of Project Settings.

#include "Stratagem/Deployable/HDSentryRules.h"

namespace HDSentryRules
{
    int32 SelectTarget(const FVector& Origin, const float Range, const TArray<FHDSentryCandidate>& Candidates)
    {
        if (Range <= 0.f)
        {
            return INDEX_NONE;
        }

        const double RangeSquared = FMath::Square(static_cast<double>(Range));
        int32 BestIndex = INDEX_NONE;
        double BestDistSquared = TNumericLimits<double>::Max();
        for (int32 Index = 0; Index < Candidates.Num(); ++Index)
        {
            const FHDSentryCandidate& Candidate = Candidates[Index];
            if (Candidate.bAlive == false || Candidate.bVisible == false)
            {
                continue;
            }

            // 사거리 경계는 포함한다
            const double DistSquared = FVector::DistSquared(Origin, Candidate.Location);
            if (DistSquared > RangeSquared)
            {
                continue;
            }

            // 같은 거리면 앞 번호를 남긴다
            if (DistSquared < BestDistSquared)
            {
                BestDistSquared = DistSquared;
                BestIndex = Index;
            }
        }

        return BestIndex;
    }
}
