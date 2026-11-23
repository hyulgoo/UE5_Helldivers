// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipIcons.h"
#include "UI/HDShipUIStyle.h"
#include "Stratagem/HDStratagemCatalog.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"

namespace HDShipIcons
{
    namespace
    {
        enum class EHDIconGroup : uint8
        {
            Stratagem,
            Weapon,
            Fixed
        };

        struct FHDIconRow
        {
            const TCHAR*    Id;
            EHDIconGroup    Group;
            const TCHAR*    TexturePath;
            const TCHAR*    Abbrev;
            const TCHAR*    Badge;
            FLinearColor    Tint;
        };

        const TCHAR* const  GrenadeTexture  = TEXT("/Game/Asset/Textures/Grenades/GrenadeTextureOutlined12.GrenadeTextureOutlined12");

        const TArray<FHDIconRow>& GetRows()
        {
            using namespace HDShipUIStyle;

            // 스트라타젬 색은 Resolve 가 카탈로그에서 정한다(여기 Tint 는 쓰지 않는다)
            static const TArray<FHDIconRow> Rows = {
                { TEXT("EagleAirstrike"),           EHDIconGroup::Stratagem,    TEXT("/Game/Asset/Stratagem/Eagle_Airstrike.Eagle_Airstrike"),                     TEXT("EA"),     TEXT(""),   IconWeapon },
                { TEXT("OrbitalPrecisionStrike"),   EHDIconGroup::Stratagem,    TEXT("/Game/Asset/Stratagem/Orbital_Precision_Strike.Orbital_Precision_Strike"),   TEXT("OPS"),    TEXT(""),   IconWeapon },
                { TEXT("OrbitalGatlingBarrage"),    EHDIconGroup::Stratagem,    TEXT("/Game/Asset/Stratagem/Orbital_Gatling_Barrage.Orbital_Gatling_Barrage"),     TEXT("OGB"),    TEXT(""),   IconWeapon },
                { TEXT("Orbital120mmBarrage"),      EHDIconGroup::Stratagem,    TEXT(""),   TEXT("120"),    TEXT(""),   IconWeapon },
                { TEXT("Resupply"),                 EHDIconGroup::Stratagem,    TEXT(""),   TEXT("RS"),     TEXT(""),   IconWeapon },
                { TEXT("MachineGunSentry"),         EHDIconGroup::Stratagem,    TEXT(""),   TEXT("MG"),     TEXT(""),   IconWeapon },
                { TEXT("AntiMaterielRifle"),        EHDIconGroup::Stratagem,    TEXT(""),   TEXT("AMR"),    TEXT(""),   IconWeapon },
                { TEXT("AssaultRifle"),             EHDIconGroup::Weapon,       TEXT(""),   TEXT("AR"),     TEXT(""),   IconWeapon },
                { TEXT("Shotgun"),                  EHDIconGroup::Weapon,       TEXT(""),   TEXT("SG"),     TEXT(""),   IconWeapon },
                { TEXT("Pistol"),                   EHDIconGroup::Weapon,       TEXT(""),   TEXT("PS"),     TEXT(""),   IconWeapon },
                { TEXT("MachinePistol"),            EHDIconGroup::Weapon,       TEXT(""),   TEXT("MP"),     TEXT(""),   IconWeapon },
                { TEXT("FragGrenade"),              EHDIconGroup::Weapon,       GrenadeTexture, TEXT("FG"), TEXT("F"),  IconWeapon },
                { TEXT("ImpactGrenade"),            EHDIconGroup::Weapon,       GrenadeTexture, TEXT("IG"), TEXT("I"),  IconWeapon },
                { TEXT("Stim"),                     EHDIconGroup::Fixed,        TEXT("/Game/Asset/Textures/Overlay/HealthIcon.HealthIcon"),                        TEXT("ST"),     TEXT(""),   Good },
                { TEXT("Requisition"),              EHDIconGroup::Fixed,        TEXT(""),   TEXT("RQ"),     TEXT(""),   Requisition },
                { TEXT("SampleCommon"),             EHDIconGroup::Fixed,        TEXT(""),   TEXT("C"),      TEXT(""),   SampleCommon },
                { TEXT("SampleRare"),               EHDIconGroup::Fixed,        TEXT(""),   TEXT("R"),      TEXT(""),   SampleRare },
                { TEXT("SampleSuper"),              EHDIconGroup::Fixed,        TEXT(""),   TEXT("S"),      TEXT(""),   SampleSuper },
            };
            return Rows;
        }

        // 소문자 다음 대문자 앞에 공백("EaglePayloadBay" → "Eagle Payload Bay")
        FString SplitCamelCase(const FString& Text)
        {
            FString Result;
            for (int32 Index = 0; Index < Text.Len(); ++Index)
            {
                const TCHAR Char = Text[Index];
                if (Index > 0 && FChar::IsUpper(Char) && FChar::IsLower(Text[Index - 1]))
                {
                    Result.AppendChar(TEXT(' '));
                }
                Result.AppendChar(Char);
            }
            return Result;
        }
    }

    FHDIconRef Resolve(FName Id, const FHDStratagemCatalog* Catalog)
    {
        FHDIconRef Ref;
        const FString IdText = Id.ToString();
        const FHDIconRow* Row = GetRows().FindByPredicate([&IdText](const FHDIconRow& Candidate) { return IdText.Equals(Candidate.Id, ESearchCase::CaseSensitive); });
        if (Row == nullptr)
        {
            Ref.Abbrev  = MakeAbbrev(SplitCamelCase(IdText));
            Ref.Tint    = HDShipUIStyle::IconUpgrade;
            return Ref;
        }

        Ref.TexturePath = Row->TexturePath;
        Ref.Abbrev      = Row->Abbrev;
        Ref.Badge       = Row->Badge;
        Ref.Tint        = Row->Tint;

        if (Row->Group == EHDIconGroup::Stratagem)
        {
            const FHDStratagemDefinition* Definition = Catalog ? Catalog->Find(Id) : nullptr;
            Ref.Tint = Definition ? GetCategoryTint(GetStratagemCategory(*Definition)) : HDShipUIStyle::IconWeapon;
        }

        return Ref;
    }

    FString MakeAbbrev(const FString& DisplayName)
    {
        TArray<FString> Words;
        SplitCamelCase(DisplayName).ParseIntoArrayWS(Words);
        if (Words.Num() == 0)
        {
            return TEXT("?");
        }

        FString Abbrev;
        if (Words.Num() == 1)
        {
            Abbrev = Words[0].Left(2);
        }
        else
        {
            for (int32 Index = 0; Index < Words.Num() && Index < 3; ++Index)
            {
                Abbrev.AppendChar(Words[Index][0]);
            }
        }

        return Abbrev.ToUpper();
    }

    FString GetStratagemCategory(const FHDStratagemDefinition& Definition)
    {
        if (Definition.bIsEagle)
        {
            return TEXT("EAGLE");
        }

        switch (Definition.Kind)
        {
        case EHDStratagemKind::Projectile:      return TEXT("ORBITAL");
        case EHDStratagemKind::Resupply:        return TEXT("SUPPLY");
        case EHDStratagemKind::Sentry:          return TEXT("DEFENSIVE");
        case EHDStratagemKind::SupportWeapon:   return TEXT("SUPPORT WEAPON");
        default:                                return TEXT("");
        }
    }

    int32 GetCategoryOrder(const FString& Category)
    {
        static const TCHAR* const Order[] = { TEXT("EAGLE"), TEXT("ORBITAL"), TEXT("SUPPLY"), TEXT("DEFENSIVE"), TEXT("SUPPORT WEAPON") };
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Order); ++Index)
        {
            if (Category.Equals(Order[Index], ESearchCase::CaseSensitive))
            {
                return Index;
            }
        }
        return 99;
    }

    FLinearColor GetCategoryTint(const FString& Category)
    {
        switch (GetCategoryOrder(Category))
        {
        case 0:     return HDShipUIStyle::IconEagle;
        case 1:     return HDShipUIStyle::IconOrbital;
        case 2:     return HDShipUIStyle::IconSupply;
        case 3:     return HDShipUIStyle::IconDefensive;
        case 4:     return HDShipUIStyle::IconSupportWeapon;
        default:    return HDShipUIStyle::IconWeapon;
        }
    }

    FString GetCommandArrowTexturePath(EHDCommandInput Input)
    {
        const TCHAR* Name = nullptr;
        switch (Input)
        {
        case EHDCommandInput::Up:       Name = TEXT("Arrow_Up"); break;
        case EHDCommandInput::Down:     Name = TEXT("Arrow_Down"); break;
        case EHDCommandInput::Left:     Name = TEXT("Arrow_Left"); break;
        case EHDCommandInput::Right:    Name = TEXT("Arrow_Right"); break;
        default:                        return FString();
        }
        return FString::Printf(TEXT("/Game/Asset/Stratagem/Command/%s.%s"), Name, Name);
    }
}
