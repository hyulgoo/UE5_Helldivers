
#include "Weapon/HDWeapon.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "TimerManager.h"
#include "Define/HDDefine.h"
#include "Define/HDSocketNames.h"
#include "Collision/HDCollision.h"
#include "Interface/HDWeaponInterface.h"
#include "Projectile/HDProjectileBase.h"
#include "Weapon/HDCasing.h"

AHDWeapon::AHDWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionProfileName(HDCOLLISION_PROFILE_TRIGGER);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AHDWeapon::OnSphereOverlap);
}

void AHDWeapon::EquipWeapon(AActor* OwnerActor)
{
    NULL_CHECK(OwnerActor);

    SetOwner(OwnerActor);
    WeaponState = EWeaponState::Equip;

    // 장착한 무기의 줍기 구는 끈다. 켜 두면 스트라타젬 투척 뒤 SetActorEnableCollision(true) 에서 주인 캡슐과 다시 겹쳐 EquipWeapon 이 또 불리고, 탄약이 AmmoData 처음 값으로 돌아간다
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHDWeapon::DropAt(const FVector& Location)
{
    constexpr float PickupRearmDelay = 2.f;

    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetOwner(nullptr);
    WeaponState = EWeaponState::Drop;

    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorLocation(Location);

    UWorld* World = GetWorld();
    NULL_CHECK(World);
    World->GetTimerManager().SetTimer(PickupRearmTimerHandle, this, &AHDWeapon::RearmPickupSphere, PickupRearmDelay, false);
}

void AHDWeapon::RearmPickupSphere()
{
    // 2초 안에 다시 장착됐으면 줍기 구를 켜지 않는다
    CONDITION_CHECK_WITHOUT_LOG(WeaponState == EWeaponState::Drop);

    AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

const void AHDWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	NULL_CHECK(WeaponMesh);

	const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName(HDSOCKETNAME_MUZZLEFLASH);
	NULL_CHECK(MuzzleFlashSocket);

	const FVector& Start		 = MuzzleFlashSocket->GetSocketLocation(WeaponMesh);
	const FVector ToTarge		 = HitTarget - Start;
	const float DistanceToSphere = ToTarge.Size();
	const FVector SphereCenter	 = Start + ToTarge.GetSafeNormal() * DistanceToSphere;

	DrawDebugSphere(GetWorld(), HitTarget, 15.f, 12, FColor::Orange, false, 0.1f);
}

void AHDWeapon::SpawnProjectile(const FVector& HitTarget)
{
    UWorld* World = GetWorld();

    const FTransform& MuzzleSocketTransform = WeaponMesh->GetSocketTransform(HDSOCKETNAME_MUZZLEFLASH);
    const FVector ToTarget = HitTarget - MuzzleSocketTransform.GetLocation();
    const FTransform SpawnTransform(ToTarget.Rotation(), MuzzleSocketTransform.GetLocation());

    // Instigator 를 넘겨야 탄이 BeginPlay 에서 쏜 캐릭터를 무시한다
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetOwner();
    SpawnParams.Instigator = Cast<APawn>(GetOwner());

    AHDProjectileBase* SpawnedProjectile = World->SpawnActor<AHDProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams);
    NULL_CHECK(SpawnedProjectile);

    // 판정은 맞을 때 읽으므로 BeginPlay 뒤에 넣어도 된다
    SpawnedProjectile->SetArmorPenetration(ArmorPenetration);

    const FTransform& EjectSocketTransform = WeaponMesh->GetSocketTransform(HDSOCKETNAME_AMMOEJECT);
    CONDITION_CHECK(EjectSocketTransform.IsValid());

    World->SpawnActor<AHDCasing>(CasingClass, EjectSocketTransform);
}

void AHDWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
    TScriptInterface<IHDWeaponInterface> WeaponInterface = OtherActor;
	if(WeaponInterface)
	{
        WeaponInterface->EquipWeapon(this);
	}
}