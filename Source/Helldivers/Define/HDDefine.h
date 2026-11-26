#pragma once

#if PLATFORM_WINDOWS
#define SHORT_FILE_NAME \
        (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define SHORT_FILE_NAME \
        (strrchr(__FILE__, '/')  ? strrchr(__FILE__, '/')  + 1 : __FILE__)
#endif

// 아래 검사 매크로가 로그를 남기는 때: PIE, 또는 에디터가 아닌 실행(Standalone -game·패키징 개발 빌드).
// 에디터에서 PIE 밖(에셋 열기·자동화 테스트)은 남기지 않는다
#define HD_CHECK_LOG_ENABLED (GIsEditor == false || (GWorld && GWorld->IsPlayInEditor()))

#define LOG(Format, ...) \
if (HD_CHECK_LOG_ENABLED) \
{ \
    UE_LOG(LogTemp, Error, TEXT("[%s] At Function: %s (%s:%d)"), \
        *FString::Printf(Format, ##__VA_ARGS__), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(SHORT_FILE_NAME), __LINE__); \
} \

#define CONDITION_CHECK(condition) \
if((condition) == false) \
{ \
    if (HD_CHECK_LOG_ENABLED) \
    { \
        UE_LOG(LogTemp, Error, TEXT("[%s] is false!, At Function: %s (%s:%d)"), \
            TEXT(#condition), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(SHORT_FILE_NAME), __LINE__); \
    } \
    return; \
} \

#define CONDITION_CHECK_WITH_RETURNTYPE(condition, ret) \
if((condition) == false) \
{ \
    if (HD_CHECK_LOG_ENABLED) \
    { \
        UE_LOG(LogTemp, Error, TEXT("[%s] is false!, At Function: %s (%s:%d)"), \
            TEXT(#condition), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(SHORT_FILE_NAME), __LINE__); \
    } \
    return ret; \
} \

#define CONDITION_CHECK_WITHOUT_LOG(condition) \
if((condition) == false) \
{ \
    return; \
} \

#define CONDITION_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(condition, ret) \
if((condition) == false) \
{ \
    return ret; \
} \

#define NULL_CHECK(ptr) \
if((ptr) == nullptr) \
{ \
    if (HD_CHECK_LOG_ENABLED) \
    { \
        UE_LOG(LogTemp, Error, TEXT("[%s] is null, At Function: %s (%s:%d)"), \
            TEXT(#ptr), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(SHORT_FILE_NAME), __LINE__); \
    } \
    return; \
} \

#define NULL_CHECK_WITH_RETURNTYPE(ptr, ret) \
if((ptr) == nullptr) \
{ \
    if (HD_CHECK_LOG_ENABLED) \
    { \
        UE_LOG(LogTemp, Error, TEXT("[%s] is null, At Function: %s (%s:%d)"), \
            TEXT(#ptr), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(SHORT_FILE_NAME), __LINE__); \
    } \
    return ret; \
} \

#define NULL_CHECK_WITHOUT_LOG(ptr) \
if((ptr) == nullptr) \
{ \
    return; \
} \

#define NULL_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(ptr, ret) \
if((ptr) == nullptr) \
{ \
    return ret; \
} \

#define VALID_CHECK(ptr) \
if(IsValid(ptr) == false) \
{ \
    if (HD_CHECK_LOG_ENABLED) \
        { \
        const UObject* CheckedObject = static_cast<const UObject*>(ptr); \
        FString ObjectName = CheckedObject ? CheckedObject->GetName() : TEXT("null"); \
        UE_LOG(LogTemp, Error, TEXT("Invalid : [%s], Name: [%s], At Function: %s (%s:%d)"), \
        TEXT(#ptr), *ObjectName, ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(SHORT_FILE_NAME), __LINE__); \
        } \
    return; \
} \

#define VALID_CHECK_WITH_RETURNTYPE(ptr, ret) \
if(IsValid(ptr) == false) \
{ \
    if (HD_CHECK_LOG_ENABLED) \
    { \
        const UObject* CheckedObject = static_cast<const UObject*>(ptr); \
        FString ObjectName = CheckedObject ? CheckedObject->GetName() : TEXT("null"); \
        UE_LOG(LogTemp, Error, TEXT("Invalid : [%s], Name: [%s], At Function: %s (%s:%d)"), \
        TEXT(#ptr), *ObjectName, ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(SHORT_FILE_NAME), __LINE__); \
    } \
    return ret; \
} \

#define VALID_CHECK_WITHOUT_LOG(ptr) \
if(IsValid(ptr) == false) \
{ \
    return; \
} \

#define VALID_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(ptr, ret) \
if(IsValid(ptr) == false) \
{ \
    return ret; \
} \
