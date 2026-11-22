// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "HDCheatManager.generated.h"

UCLASS()
class HELLDIVERS_API UHDCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
    UFUNCTION(Exec)
    void        HDDamageSelf(float Amount = 50.f);

    // 플레이어 앞 1500 에 Count 마리를 좌우 250 간격으로 스폰한다. Type 은 대소문자 무시(Charger)
    UFUNCTION(Exec)
    void        HDSpawnEnemy(const FString& Type = TEXT("Charger"), int32 Count = 1);

    // 살아 있는 적 전부에 CurrentHealth -9999 (사망 경로를 그대로 탄다)
    UFUNCTION(Exec)
    void        HDKillEnemies();

    // 스폰 디렉터: start / stop / status
    UFUNCTION(Exec)
    void        HDDirector(const FString& Command);

    // 플레이어를 대상으로 증원 웨이브를 바로 부른다
    UFUNCTION(Exec)
    void        HDBreach();

    // 미션 상태 한 줄: Phase=... Objectives=a/b Reinforcements=n Extraction=s InZone=0|1 Called=0|1 Ship=0|1
    UFUNCTION(Exec)
    void        HDMission();

    // 가장 가까운 미완료 시설을 실제 파괴 경로(체력 0)로 부순다. 남은 시설이 없으면 목표 하나 완료로 친다
    UFUNCTION(Exec)
    void        HDCompleteObjective();

    // 추출 지점이 있으면 플레이어 폰을 그 위 200 으로 옮긴다
    UFUNCTION(Exec)
    void        HDTeleportToExtraction();

    // 추출을 부른다(호출 커맨드 Ctrl + ← ↑ → ↓ 와 같은 AHDGameMode::RequestExtractionCall 경로). 결과는 [HDMission] extraction called / call refused 줄
    UFUNCTION(Exec)
    void        HDCallExtraction();

    // 남은 증원 수를 Count 로(0 이상으로 자른다)
    UFUNCTION(Exec)
    void        HDSetReinforcements(int32 Count);

    // 미션 제한 시간을 Seconds 초로(0 = 제한 없음). 지난 시간이 이미 넘으면 다음 프레임에 시간 초과 Fail
    UFUNCTION(Exec)
    void        HDSetTimeLimit(float Seconds);

    // 로드아웃 칸마다 한 줄: [HDStratagems] <칸> <이름> cooldown=<남은 초> uses=<남은/최대|inf> available=0|1 maxCooldown=<칸 대기> delay=<호출 지연> rearm=<재무장>
    UFUNCTION(Exec)
    void        HDStratagems();

    // 로드아웃 사용 횟수를 처음 값으로, 모든 칸의 재사용 대기를 지운다
    UFUNCTION(Exec)
    void        HDStratagemReset();

    // 로드아웃을 주어진 이름(Config/HDStratagems.json 의 "Name", 1~4개)으로 바꾸고 화면 목록을 다시 만든다. 빈 칸은 뺀다.
    // 칸을 새로 만들므로 모든 칸의 남은 횟수·재사용 대기가 처음으로 돌아간다. 카탈로그에 없는 이름은 건너뛴다
    UFUNCTION(Exec)
    void        HDStratagemLoadout(const FString& Slot1, const FString& Slot2 = TEXT(""), const FString& Slot3 = TEXT(""), const FString& Slot4 = TEXT(""));

    // 플레이어 앞 300 의 지면에 지원 무기(AHDSupportWeapon)를 놓는다
    UFUNCTION(Exec)
    void        HDGiveSupportWeapon();

    // 플레이어 폰의 무기 칸을 고른다. 1~4 = 주무기·보조무기·지원무기·수류탄(숫자 키와 같은 AHDCharacterPlayer::SelectWeaponSlot 경로)
    UFUNCTION(Exec)
    void        HDSelectWeapon(int32 SlotNumber);

    // 플레이어 수류탄 수(Grenade base)를 Count 로. [0, MaxGrenade] 로 자른다
    UFUNCTION(Exec)
    void        HDSetGrenades(int32 Count);

    // 플레이어 스팀 수(Stim base)를 Count 로. [0, MaxStim] 으로 자른다
    UFUNCTION(Exec)
    void        HDSetStims(int32 Count);

    // 플레이어 폰이 스팀을 쓴다(Q 키와 같은 AHDCharacterPlayer::UseStim 경로)
    UFUNCTION(Exec)
    void        HDUseStim();

    // 플레이어 폰이 급강하한다(왼쪽 Alt 와 같은 AHDCharacterPlayer::Dive 경로). 인자가 status 면 던지지 않고 [HDDive] status stance=… diving=… airborne=… 만 찍는다
    UFUNCTION(Exec)
    void        HDDive(const FString& Command = TEXT(""));

    // 원시 키 이벤트를 프레임별로 PlayerController::InputKey 에 보낸다(실제 키보드와 같은 경로). 공백이 있으니 따옴표로 감싼다.
    // '|' 로 프레임을 나누고, 토큰은 <키 이름>+(누름) / <키 이름>-(뗌), 빈 프레임은 한 틱 기다림. 예: HDSimulateKeys "LeftControl+ | | Up+ | Up- | LeftControl-"
    // 누른 채 남지 않게 끝에 누른 키를 모두 떼는 토큰을 넣는다. 문자열이 틀리면 아무 키도 보내지 않는다
    UFUNCTION(Exec)
    void        HDSimulateKeys(const FString& Sequence);

    // 플레이어 폰의 몸 몽타주를 끊는다(검증용, GA 가 끊김을 처리하는지 본다). 무기 메시의 몽타주는 건드리지 않는다
    UFUNCTION(Exec)
    void        HDStopMontage();

    // 살아 있는 가장 가까운 적의 BoneName 본을 겨눠 플레이어 눈앞 150 에서 BPP_Bullet_Light 한 발을 쏜다(소유자 = 플레이어 폰).
    // Penetration 이 0 이면 탄의 자동 관통 등급(2), 1 이상이면 그 값. 판정은 로그 [HDArmor] hit 줄로 본다
    UFUNCTION(Exec)
    void        HDShootEnemy(const FString& BoneName = TEXT("spine_03"), int32 Penetration = 0);

    // 살아 있는 가장 가까운 적의 몸 위치에서 플레이어 쪽 수평으로 Offset 떨어진 자리에 수류탄(AHDProjectileGrenade)을 놓고 바로 터뜨린다(소유자 = 플레이어 폰).
    // 로그 [HDExplode] target=<적> body=<몸까지> capsule=<액터 위치까지> ragdoll=0|1, 판정은 [HDArmor] explosion 줄로 본다
    UFUNCTION(Exec)
    void        HDExplodeAtEnemy(float Offset = 150.f);

    // 프로필 한 줄: [HDProfile] slot=<슬롯> file=0|1 Requisition=… Samples=c/r/s Difficulty=… Played=… Won=… Upgrades=… Unlocked=… Loadout=…
    UFUNCTION(Exec)
    void        HDProfile();

    // 프로필에 징발·샘플을 더하고 저장한다(음수도 받는다). 각각 [0, 상한] 으로 자른다. 저장 로그는 서브시스템이 찍는다
    UFUNCTION(Exec)
    void        HDGiveResources(int32 Requisition, int32 Common = 0, int32 Rare = 0, int32 Super = 0);

    // 프로필 저장 파일을 지우고 새 프로필로 돌린다(저장하지 않는다)
    UFUNCTION(Exec)
    void        HDResetProfile();

    // 함선 단계에서 바로 출격한다(메뉴 DEPLOY 와 같은 AHDGameMode::RequestDeploy 경로, 타이틀 화면에서도 된다). 함선 단계가 아니면 deploy refused
    UFUNCTION(Exec)
    void        HDDeploy();

    // 함선 메뉴 한 줄: [HDShip] inShip=0|1 active=0|1 screen=<화면> cursor=<n> items=<Label|Label…>
    UFUNCTION(Exec)
    void        HDShip();

    // 프로필 난이도를 Level 로(메뉴 DIFFICULTY 줄과 같은 ModifyProfile 경로, 정리가 표 안으로 자른다) → [HDShip] difficulty <n> <Label> (next deploy).
    // 다음 출격부터 들어간다. 이미 시작한 미션은 바뀌지 않는다
    UFUNCTION(Exec)
    void        HDSetDifficulty(int32 Level);

    // 작전 포기(확인 창 없이 AHDGameMode::RequestAbandon). 함선 단계·미션 없음·끝난 뒤면 [HDMission] abandon refused 줄
    UFUNCTION(Exec)
    void        HDAbandon();

    // 결과 화면의 입력 지연을 보지 않고 함선으로 돌아간다(AHDGameMode::ReturnToShip). 미션이 안 끝났으면 [HDShip] return refused 줄
    UFUNCTION(Exec)
    void        HDReturnToShip();

    // 이 월드의 미션 결과 한 줄: [HDMission] result <AHDGameMode 의 결과 로그와 같은 글자>, 없으면 [HDMission] no result
    UFUNCTION(Exec)
    void        HDMissionResult();

    // 샘플 한 줄: [HDSample] carried=c/r/s world common=… rare=… super=… bundles=… nearest=<m>m (맵에 남은 액터 수, 가장 가까운 것까지, 없으면 -1)
    UFUNCTION(Exec)
    void        HDSamples();

    // 가장 가까운 샘플 위 200 으로 폰을 옮긴다. 빈 글자 = 아무 샘플, bundle = 떨군 묶음, common·rare·super = 그 종류의 낱개
    UFUNCTION(Exec)
    void        HDTeleportToSample(const FString& Type = TEXT(""));

    // 플레이어스테이트 소지에 더한다(프로필이 아니다. 종류별로 0 미만이 되지 않게) → [HDSample] carried=c/r/s
    UFUNCTION(Exec)
    void        HDGiveSamples(int32 Common, int32 Rare = 0, int32 Super = 0);

    // 업그레이드 행마다 [HDUpgrade] <이름> level=<산>/<수> next=<가격|max>, 마지막에 [HDUpgrade] active <프로필 효과 합>
    UFUNCTION(Exec)
    void        HDUpgrades();

    // 함선 메뉴 SHIP UPGRADES 줄 확정과 같은 ModifyProfile(TryBuy) 경로. [HDUpgrade] buy <이름> -> <단계> (bought|cannot afford|max level|unknown) requisition <전> -> <후>
    UFUNCTION(Exec)
    void        HDBuyUpgrade(const FString& Name);
};
