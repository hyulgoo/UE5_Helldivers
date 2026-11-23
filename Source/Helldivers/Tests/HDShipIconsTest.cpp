// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HDShipIcons.h"
#include "UI/HDLetterTile.h"
#include "UI/HDShipUIStyle.h"
#include "Progression/HDWeaponCatalog.h"
#include "Stratagem/HDStratagemCatalog.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDShipIconsTest, "Helldivers.FrontEnd.ShipIcons", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDShipIconsTest
{
    const FColor        Bg(10, 20, 30, 255);
    const FColor        Fg(250, 240, 230, 255);
    const FColor        Edge(100, 110, 120, 255);

    FColor At(const TArray<FColor>& Pixels, int32 Width, int32 X, int32 Y)
    {
        const int32 Index = Y * Width + X;
        return Pixels.IsValidIndex(Index) ? Pixels[Index] : FColor(0, 0, 0, 0);
    }

    // 글자 픽셀 수와 그 경계 상자
    int32 CountForeground(const TArray<FColor>& Pixels, int32 Width, FIntRect& OutBounds)
    {
        int32 Count = 0;
        OutBounds = FIntRect(INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN);
        for (int32 Index = 0; Index < Pixels.Num(); ++Index)
        {
            if (Pixels[Index] != Fg)
            {
                continue;
            }

            const int32 X = Index % Width;
            const int32 Y = Index / Width;
            OutBounds.Min.X = FMath::Min(OutBounds.Min.X, X);
            OutBounds.Min.Y = FMath::Min(OutBounds.Min.Y, Y);
            OutBounds.Max.X = FMath::Max(OutBounds.Max.X, X);
            OutBounds.Max.Y = FMath::Max(OutBounds.Max.Y, Y);
            ++Count;
        }
        return Count;
    }

    const FHDStatBar* FindBar(const TArray<FHDStatBar>& Bars, const TCHAR* Label)
    {
        return Bars.FindByPredicate([Label](const FHDStatBar& Bar) { return Bar.Label == Label; });
    }
}

bool FHDShipIconsTest::RunTest(const FString& Parameters)
{
    using namespace HDShipIconsTest;

    const FHDStratagemCatalog& Catalog = FHDStratagemCatalog::GetProjectCatalog();

    // I1 표
    {
        struct FCase
        {
            const TCHAR*    Id;
            const TCHAR*    Texture;
            const TCHAR*    Abbrev;
            const TCHAR*    Badge;
        };
        const TArray<FCase> Cases = {
            { TEXT("EagleAirstrike"),           TEXT("/Game/Asset/Stratagem/Eagle_Airstrike.Eagle_Airstrike"),                         nullptr,        nullptr },
            { TEXT("OrbitalPrecisionStrike"),   TEXT("/Game/Asset/Stratagem/Orbital_Precision_Strike.Orbital_Precision_Strike"),       nullptr,        nullptr },
            { TEXT("OrbitalGatlingBarrage"),    TEXT("/Game/Asset/Stratagem/Orbital_Gatling_Barrage.Orbital_Gatling_Barrage"),         nullptr,        nullptr },
            { TEXT("Orbital120mmBarrage"),      TEXT(""),   TEXT("120"),    nullptr },
            { TEXT("Resupply"),                 TEXT(""),   TEXT("RS"),     nullptr },
            { TEXT("MachineGunSentry"),         TEXT(""),   TEXT("MG"),     nullptr },
            { TEXT("AntiMaterielRifle"),        TEXT(""),   TEXT("AMR"),    nullptr },
            { TEXT("AssaultRifle"),             TEXT(""),   TEXT("AR"),     nullptr },
            { TEXT("Shotgun"),                  TEXT(""),   TEXT("SG"),     nullptr },
            { TEXT("Pistol"),                   TEXT(""),   TEXT("PS"),     nullptr },
            { TEXT("MachinePistol"),            TEXT(""),   TEXT("MP"),     nullptr },
            { TEXT("FragGrenade"),              TEXT("/Game/Asset/Textures/Grenades/GrenadeTextureOutlined12.GrenadeTextureOutlined12"),  nullptr,  TEXT("F") },
            { TEXT("ImpactGrenade"),            TEXT("/Game/Asset/Textures/Grenades/GrenadeTextureOutlined12.GrenadeTextureOutlined12"),  nullptr,  TEXT("I") },
            { TEXT("Stim"),                     TEXT("/Game/Asset/Textures/Overlay/HealthIcon.HealthIcon"),                            nullptr,        nullptr },
            { TEXT("Requisition"),              TEXT(""),   TEXT("RQ"),     nullptr },
            { TEXT("SampleCommon"),             TEXT(""),   TEXT("C"),      nullptr },
            { TEXT("SampleRare"),               TEXT(""),   TEXT("R"),      nullptr },
            { TEXT("SampleSuper"),              TEXT(""),   TEXT("S"),      nullptr },
            { TEXT("EaglePayloadBay"),          TEXT(""),   TEXT("EPB"),    nullptr },
            { TEXT("Foo"),                      TEXT(""),   TEXT("FO"),     nullptr },
        };

        for (const FCase& Case : Cases)
        {
            const FHDIconRef Ref = HDShipIcons::Resolve(FName(Case.Id), &Catalog);
            TestEqual(FString::Printf(TEXT("I1 %s texture"), Case.Id), Ref.TexturePath, FString(Case.Texture));
            if (Case.Abbrev)
            {
                TestEqual(FString::Printf(TEXT("I1 %s abbrev"), Case.Id), Ref.Abbrev, FString(Case.Abbrev));
            }
            if (Case.Badge)
            {
                TestEqual(FString::Printf(TEXT("I1 %s badge"), Case.Id), Ref.Badge, FString(Case.Badge));
            }
        }

        struct FTintCase
        {
            const TCHAR*    Id;
            FLinearColor    Tint;
        };
        const TArray<FTintCase> TintCases = {
            { TEXT("EagleAirstrike"),           HDShipUIStyle::IconEagle },
            { TEXT("OrbitalGatlingBarrage"),    HDShipUIStyle::IconOrbital },
            { TEXT("Orbital120mmBarrage"),      HDShipUIStyle::IconOrbital },
            { TEXT("Resupply"),                 HDShipUIStyle::IconSupply },
            { TEXT("MachineGunSentry"),         HDShipUIStyle::IconDefensive },
            { TEXT("AntiMaterielRifle"),        HDShipUIStyle::IconSupportWeapon },
            { TEXT("AssaultRifle"),             HDShipUIStyle::IconWeapon },
            { TEXT("MachinePistol"),            HDShipUIStyle::IconWeapon },
            { TEXT("Requisition"),              HDShipUIStyle::Requisition },
            { TEXT("SampleCommon"),             HDShipUIStyle::SampleCommon },
            { TEXT("SampleRare"),               HDShipUIStyle::SampleRare },
            { TEXT("SampleSuper"),              HDShipUIStyle::SampleSuper },
            { TEXT("EaglePayloadBay"),          HDShipUIStyle::IconUpgrade },
            { TEXT("Foo"),                      HDShipUIStyle::IconUpgrade },
        };
        for (const FTintCase& Case : TintCases)
        {
            const FHDIconRef Ref = HDShipIcons::Resolve(FName(Case.Id), &Catalog);
            TestTrue(FString::Printf(TEXT("I1 %s tint"), Case.Id), Ref.Tint.Equals(Case.Tint, 0.001f));
        }

        // 카탈로그 없이는 분류 색을 모른다
        TestTrue(TEXT("I1 no catalog -> IconWeapon"), HDShipIcons::Resolve(FName(TEXT("EagleAirstrike")), nullptr).Tint.Equals(HDShipUIStyle::IconWeapon, 0.001f));
    }

    // I2 약칭
    {
        TestEqual(TEXT("I2 EAGLE PAYLOAD BAY"), HDShipIcons::MakeAbbrev(TEXT("EAGLE PAYLOAD BAY")), FString(TEXT("EPB")));
        TestEqual(TEXT("I2 FIELD KIT"), HDShipIcons::MakeAbbrev(TEXT("FIELD KIT")), FString(TEXT("FK")));
        TestEqual(TEXT("I2 SHOTGUN"), HDShipIcons::MakeAbbrev(TEXT("SHOTGUN")), FString(TEXT("SH")));
        TestEqual(TEXT("I2 EaglePayloadBay"), HDShipIcons::MakeAbbrev(TEXT("EaglePayloadBay")), FString(TEXT("EPB")));
        TestEqual(TEXT("I2 A B C D"), HDShipIcons::MakeAbbrev(TEXT("A B C D")), FString(TEXT("ABC")));
        TestEqual(TEXT("I2 empty"), HDShipIcons::MakeAbbrev(TEXT("")), FString(TEXT("?")));
    }

    // I3 분류
    {
        struct FCase
        {
            const TCHAR*    Name;
            const TCHAR*    Category;
            int32           Order;
        };
        const TArray<FCase> Cases = {
            { TEXT("EagleAirstrike"),           TEXT("EAGLE"),          0 },
            { TEXT("OrbitalGatlingBarrage"),    TEXT("ORBITAL"),        1 },
            { TEXT("Resupply"),                 TEXT("SUPPLY"),         2 },
            { TEXT("MachineGunSentry"),         TEXT("DEFENSIVE"),      3 },
            { TEXT("AntiMaterielRifle"),        TEXT("SUPPORT WEAPON"), 4 },
        };
        for (const FCase& Case : Cases)
        {
            const FHDStratagemDefinition* Definition = Catalog.Find(FName(Case.Name));
            if (TestNotNull(FString::Printf(TEXT("I3 %s in catalog"), Case.Name), Definition) == false)
            {
                continue;
            }

            const FString Category = HDShipIcons::GetStratagemCategory(*Definition);
            TestEqual(FString::Printf(TEXT("I3 %s category"), Case.Name), Category, FString(Case.Category));
            TestEqual(FString::Printf(TEXT("I3 %s order"), Case.Name), HDShipIcons::GetCategoryOrder(Category), Case.Order);
        }
        TestEqual(TEXT("I3 unknown order"), HDShipIcons::GetCategoryOrder(TEXT("NOPE")), 99);
    }

    // I4 화살표
    {
        TestEqual(TEXT("I4 Up"), HDShipIcons::GetCommandArrowTexturePath(EHDCommandInput::Up), FString(TEXT("/Game/Asset/Stratagem/Command/Arrow_Up.Arrow_Up")));
        TestEqual(TEXT("I4 Down"), HDShipIcons::GetCommandArrowTexturePath(EHDCommandInput::Down), FString(TEXT("/Game/Asset/Stratagem/Command/Arrow_Down.Arrow_Down")));
        TestEqual(TEXT("I4 Left"), HDShipIcons::GetCommandArrowTexturePath(EHDCommandInput::Left), FString(TEXT("/Game/Asset/Stratagem/Command/Arrow_Left.Arrow_Left")));
        TestEqual(TEXT("I4 Right"), HDShipIcons::GetCommandArrowTexturePath(EHDCommandInput::Right), FString(TEXT("/Game/Asset/Stratagem/Command/Arrow_Right.Arrow_Right")));
    }

    // I5 글자 타일
    {
        const TArray<FColor> A = HDLetterTile::Rasterize(TEXT("A"), 32, 32, Bg, Fg, Edge);
        TestEqual(TEXT("I5 A count"), A.Num(), 1024);
        TestTrue(TEXT("I5 A (0,0) border"), At(A, 32, 0, 0) == Edge);
        TestTrue(TEXT("I5 A (31,31) border"), At(A, 32, 31, 31) == Edge);
        TestTrue(TEXT("I5 A (1,1) background"), At(A, 32, 1, 1) == Bg);
        TestTrue(TEXT("I5 A (15,6) foreground"), At(A, 32, 15, 6) == Fg);
        TestTrue(TEXT("I5 A (9,6) background"), At(A, 32, 9, 6) == Bg);

        FIntRect Bounds;
        const TArray<FColor> Amr = HDLetterTile::Rasterize(TEXT("AMR"), 32, 32, Bg, Fg, Edge);
        const int32 AmrCount = CountForeground(Amr, 32, Bounds);
        TestTrue(TEXT("I5 AMR has pixels"), AmrCount > 0);
        TestTrue(TEXT("I5 AMR inside margin"), AmrCount > 0 && Bounds.Min.X >= 4 && Bounds.Min.Y >= 4 && Bounds.Max.X <= 27 && Bounds.Max.Y <= 27);

        const TArray<FColor> Ar = HDLetterTile::Rasterize(TEXT("AR"), 64, 24, Bg, Fg, Edge);
        TestEqual(TEXT("I5 AR count"), Ar.Num(), 1536);
        const int32 ArCount = CountForeground(Ar, 64, Bounds);
        TestTrue(TEXT("I5 AR inside vertical margin"), ArCount > 0 && Bounds.Min.Y >= 3 && Bounds.Max.Y <= 20);

        TestEqual(TEXT("I5 empty no pixels"), CountForeground(HDLetterTile::Rasterize(TEXT(""), 32, 32, Bg, Fg, Edge), 32, Bounds), 0);
        TestEqual(TEXT("I5 WXYZ1 8x8 no pixels"), CountForeground(HDLetterTile::Rasterize(TEXT("WXYZ1"), 8, 8, Bg, Fg, Edge), 8, Bounds), 0);
        TestTrue(TEXT("I5 lowercase a == A"), HDLetterTile::Rasterize(TEXT("a"), 32, 32, Bg, Fg, Edge) == A);
    }

    // I6 무기 수치
    {
        struct FCase
        {
            const TCHAR*    Id;
            float           Damage;
            int32           Pellets;
            float           Rpm;
            int32           Magazine;
            int32           Spare;
            int32           ArmorPen;
            float           Radius;
            float           Fuse;
        };
        const TArray<FCase> Cases = {
            { TEXT("AssaultRifle"),     50.f, 1, 600.f,     45, 7, 2, 0.f,   0.f },
            { TEXT("Shotgun"),          50.f, 8, 66.67f,    6,  5, 1, 0.f,   0.f },
            { TEXT("Pistol"),           50.f, 1, 240.f,     15, 4, 1, 0.f,   0.f },
            { TEXT("MachinePistol"),    50.f, 1, 750.f,     24, 4, 1, 0.f,   0.f },
            { TEXT("FragGrenade"),      80.f, 1, 0.f,       0,  0, 3, 350.f, 3.f },
            { TEXT("ImpactGrenade"),    80.f, 1, 0.f,       0,  0, 3, 300.f, 0.f },
        };
        for (const FCase& Case : Cases)
        {
            const FHDWeaponOption* Option = HDWeaponCatalog::Find(FName(Case.Id));
            if (TestNotNull(FString::Printf(TEXT("I6 %s option"), Case.Id), Option) == false)
            {
                continue;
            }

            const FHDWeaponStats& Stats = Option->Stats;
            TestTrue(FString::Printf(TEXT("I6 %s damage"), Case.Id), FMath::IsNearlyEqual(Stats.Damage, Case.Damage, 0.01f));
            TestEqual(FString::Printf(TEXT("I6 %s pellets"), Case.Id), Stats.Pellets, Case.Pellets);
            TestTrue(FString::Printf(TEXT("I6 %s rpm"), Case.Id), FMath::IsNearlyEqual(Stats.RoundsPerMinute, Case.Rpm, 0.01f));
            TestEqual(FString::Printf(TEXT("I6 %s magazine"), Case.Id), Stats.Magazine, Case.Magazine);
            TestEqual(FString::Printf(TEXT("I6 %s spare"), Case.Id), Stats.SpareMagazines, Case.Spare);
            TestEqual(FString::Printf(TEXT("I6 %s armor pen"), Case.Id), Stats.ArmorPen, Case.ArmorPen);
            TestTrue(FString::Printf(TEXT("I6 %s radius"), Case.Id), FMath::IsNearlyEqual(Stats.Radius, Case.Radius, 0.01f));
            TestTrue(FString::Printf(TEXT("I6 %s fuse"), Case.Id), FMath::IsNearlyEqual(Stats.FuseSeconds, Case.Fuse, 0.01f));
        }

        auto CheckBar = [this](const TArray<FHDStatBar>& Bars, const TCHAR* Weapon, const TCHAR* Label, const TCHAR* Value, float Fraction)
        {
            const FHDStatBar* Bar = FindBar(Bars, Label);
            if (TestNotNull(FString::Printf(TEXT("I6 %s %s"), Weapon, Label), Bar) == false)
            {
                return;
            }
            TestEqual(FString::Printf(TEXT("I6 %s %s value"), Weapon, Label), Bar->Value, FString(Value));
            TestTrue(FString::Printf(TEXT("I6 %s %s fraction %.3f"), Weapon, Label, Bar->Fraction), FMath::IsNearlyEqual(Bar->Fraction, Fraction, 0.01f));
        };

        const FHDWeaponOption* Rifle = HDWeaponCatalog::Find(FName(TEXT("AssaultRifle")));
        const FHDWeaponOption* Shotgun = HDWeaponCatalog::Find(FName(TEXT("Shotgun")));
        const FHDWeaponOption* Frag = HDWeaponCatalog::Find(FName(TEXT("FragGrenade")));
        const FHDWeaponOption* Impact = HDWeaponCatalog::Find(FName(TEXT("ImpactGrenade")));
        if (Rifle && Shotgun && Frag && Impact)
        {
            const TArray<FHDStatBar> RifleBars = HDWeaponCatalog::MakeStatBars(*Rifle);
            CheckBar(RifleBars, TEXT("rifle"), TEXT("DAMAGE"), TEXT("50"), 0.125f);
            CheckBar(RifleBars, TEXT("rifle"), TEXT("FIRE RATE"), TEXT("600 RPM"), 0.667f);
            CheckBar(RifleBars, TEXT("rifle"), TEXT("CAPACITY"), TEXT("45"), 0.75f);
            CheckBar(RifleBars, TEXT("rifle"), TEXT("ARMOR PEN"), TEXT("2"), 0.5f);

            const TArray<FHDStatBar> ShotgunBars = HDWeaponCatalog::MakeStatBars(*Shotgun);
            CheckBar(ShotgunBars, TEXT("shotgun"), TEXT("DAMAGE"), TEXT("50 X8"), 1.f);
            CheckBar(ShotgunBars, TEXT("shotgun"), TEXT("FIRE RATE"), TEXT("67 RPM"), 66.67f / 900.f);

            const TArray<FHDStatBar> FragBars = HDWeaponCatalog::MakeStatBars(*Frag);
            CheckBar(FragBars, TEXT("frag"), TEXT("DAMAGE"), TEXT("80"), 0.2f);
            CheckBar(FragBars, TEXT("frag"), TEXT("RADIUS"), TEXT("3.5 M"), 0.7f);
            CheckBar(FragBars, TEXT("frag"), TEXT("ARMOR PEN"), TEXT("3"), 0.75f);
            CheckBar(FragBars, TEXT("frag"), TEXT("FUSE"), TEXT("3.0 S"), -1.f);

            const TArray<FHDStatBar> ImpactBars = HDWeaponCatalog::MakeStatBars(*Impact);
            const FHDStatBar* ImpactFuse = FindBar(ImpactBars, TEXT("FUSE"));
            TestTrue(TEXT("I6 impact FUSE IMPACT"), ImpactFuse != nullptr && ImpactFuse->Value == TEXT("IMPACT"));

            // 비율은 0~1(막대 없음 -1 제외)
            for (const TArray<FHDStatBar>* Bars : { &RifleBars, &ShotgunBars, &FragBars, &ImpactBars })
            {
                for (const FHDStatBar& Bar : *Bars)
                {
                    TestTrue(FString::Printf(TEXT("I6 %s fraction in range"), *Bar.Label), Bar.Fraction == -1.f || (Bar.Fraction >= 0.f && Bar.Fraction <= 1.f));
                }
            }
        }
    }

    return true;
}

#endif
