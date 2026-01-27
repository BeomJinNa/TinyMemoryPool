#pragma once

#include <exception>
#include <iostream>

// 디버그 모드 감지
#if !defined(NDEBUG) || defined(_DEBUG)
#include <cassert>
#define TMP_ASSERT(condition) assert(condition)
#else
// 릴리즈 모드에서는 아무것도 안 함 (최적화)
#define TMP_ASSERT(condition) ((void) 0)
#endif

// 치명적 오류는 언제나 프로그램 종료
#define TMP_FATAL_ERROR(message)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        std::cerr << "[TinyMemoryPool Fatal] " << message << std::endl;                                                \
        std::terminate();                                                                                              \
    } while(0)