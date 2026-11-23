// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class FHDShipUICanvas;
class FHDShipMenu;
struct FHDShipMenuItem;
struct FHDKeyHint;
struct FHDResourceView;

// 함선 화면별 그리기. 로직 상태(FHDShipMenu·줄·화면 데이터)는 읽기만 한다. 좌표는 설계 단위(1920×1080)를 캔버스 틀로 바꿔 쓴다
class FHDShipScreenPainter
{
public:
    // 화면 전체를 Backdrop 으로
    static void         PaintBackdrop(FHDShipUICanvas& Canvas);
    // 위 바: 왼쪽 제목(Title, Accent), 오른쪽 자원 타일 넷(RQ·C·R·S)과 수, 그 아래 LAST MISSION 줄(Small, TextDim)
    static void         PaintTopBar(FHDShipUICanvas& Canvas, const FString& Title, const FHDResourceView& Resources);
    // 아래 바: 왼쪽부터 키 상자 + 동작 이름. bClickable 이면 ShipKey_<키> 히트박스
    static void         PaintBottomBar(FHDShipUICanvas& Canvas, const TArray<FHDKeyHint>& Hints);
    // 목록 화면: 줄 버튼(선택 강조·꺼진 줄)과 ShipItem_<i> 히트박스, 오른쪽 상세 패널(고른 줄의 Label·Detail). System 은 가운데, 상세 없음
    static void         PaintList(FHDShipUICanvas& Canvas, const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items);
    // 출격 대기: 한 겹 더 어둡게, 가운데 HELLPOD LAUNCH·큰 숫자·[BACKSPACE] CANCEL
    static void         PaintCountdown(FHDShipUICanvas& Canvas, int32 DisplaySeconds);
    // 함선 안(메뉴 닫힘): 오른쪽 위 자원(배경 없음), 왼쪽 아래 [P] SHIP MENU, Prompt 가 있으면 아래 가운데
    static void         PaintInteriorOverlay(FHDShipUICanvas& Canvas, const FHDResourceView& Resources, const FString& Prompt);

private:
    // 오른쪽 끝(설계 x 1872)에 맞춘 자원 줄과 그 아래 마지막 미션 줄. 자원 줄의 왼쪽 끝(캔버스 x)을 돌려준다
    static float        PaintResources(FHDShipUICanvas& Canvas, const FHDResourceView& Resources);
};
