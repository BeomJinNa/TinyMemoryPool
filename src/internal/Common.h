#pragma once

#include <exception>
#include <iostream>

/// @name TinyMemoryPool Debug Macros
/// {@

#if !defined(NDEBUG) || defined(_DEBUG)
#include <cassert>
#define TMP_ASSERT(condition) assert(condition)
#else
#define TMP_ASSERT(condition) ((void) 0)
#endif

/// @brief 복구 불가능한 치명적 오류 시 로그 출력 후 프로그램을 종료한다.
#define TMP_FATAL_ERROR(message)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        std::cerr << "[TinyMemoryPool Fatal] " << message << std::endl;                                                \
        std::terminate();                                                                                              \
    } while(0)

/// @}
