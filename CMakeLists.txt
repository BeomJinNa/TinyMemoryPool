cmake_minimum_required(VERSION 3.15)

project(TinyMemoryPool 
    VERSION 1.0.0
    LANGUAGES CXX
)

option(TMP_BUILD_TESTS "Build tests for TinyMemoryPool" ON)
option(TMP_BUILD_EXAMPLES "Build standalone STL container examples" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 지원되는 도구 모음에서만 링크 시점 최적화(LTO)를 활성화한다.
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if(result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(WARNING "IPO / LTO not supported: ${output}")
endif()

find_package(TBB CONFIG REQUIRED)

# IDE에서도 내부 구조를 확인할 수 있도록 헤더를 소스 목록에 포함한다.
set(TMP_SOURCES
    # 내부 구현
    src/internal/MemoryApi.cpp
    src/internal/MemoryManager.cpp
    src/internal/Pool.cpp
    src/internal/PoolManager.cpp
    
    # 내부 헤더
    src/internal/Common.h
    src/internal/MemoryManager.h
    src/internal/PlatformMemory.h
    src/internal/Pool.h
    src/internal/PoolManager.h
    src/internal/backends/PosixMemory.h
    src/internal/backends/WindowsMemory.h

    # 공개 헤더
    include/TinyMemoryPool/Allocator.h
    include/TinyMemoryPool/Config.h
    include/TinyMemoryPool/Detail/MemoryApi.h
)

add_library(TinyMemoryPool STATIC ${TMP_SOURCES})

# 사용하는 프로젝트에서 동일한 이름으로 라이브러리를 연결한다.
add_library(TinyMemoryPool::TinyMemoryPool ALIAS TinyMemoryPool)

target_include_directories(TinyMemoryPool 
    PUBLIC
        # 사용부에는 공개 헤더 경로만 노출한다.
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
    PRIVATE
        # 구현 파일은 내부 헤더를 파일명으로 참조한다.
        "${CMAKE_CURRENT_SOURCE_DIR}/src/internal"
)

if(MSVC)
    target_compile_options(TinyMemoryPool PRIVATE /W4 /utf-8)
else()
    target_compile_options(TinyMemoryPool PRIVATE -Wall -Wextra)
endif()

target_link_libraries(TinyMemoryPool PRIVATE TBB::tbb)

install(TARGETS TinyMemoryPool EXPORT TinyMemoryPoolTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/TinyMemoryPool DESTINATION include)

if(TMP_BUILD_EXAMPLES)
    add_executable(TMP_StlExample examples/stl_containers.cpp examples/PoolContainers.h)
    target_link_libraries(TMP_StlExample PRIVATE TinyMemoryPool::TinyMemoryPool)
    if(MSVC)
        target_compile_options(TMP_StlExample PRIVATE /utf-8)
    endif()
endif()

# 다른 프로젝트의 하위 라이브러리로 포함되면 테스트를 빌드하지 않는다.
if(TMP_BUILD_TESTS AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "Building TinyMemoryPool Tests...")
    enable_testing()
    add_executable(TMP_Test tests/main.cpp)
    target_link_libraries(TMP_Test PRIVATE TinyMemoryPool::TinyMemoryPool)
    if(MSVC)
        target_compile_options(TMP_Test PRIVATE /utf-8)
    endif()
    add_test(NAME TinyMemoryPool.Smoke COMMAND TMP_Test)
    add_executable(TMP_Regression tests/regression.cpp)
    target_link_libraries(TMP_Regression PRIVATE TinyMemoryPool::TinyMemoryPool)
    if(MSVC)
        target_compile_options(TMP_Regression PRIVATE /utf-8)
    endif()
    foreach(test_case fundamental extended boundaries reuse containers)
        add_test(NAME TinyMemoryPool.${test_case} COMMAND TMP_Regression ${test_case})
    endforeach()

    add_executable(TMP_Failure tests/failure.cpp)
    target_link_libraries(TMP_Failure PRIVATE TinyMemoryPool::TinyMemoryPool)
    target_include_directories(TMP_Failure PRIVATE src/internal)
    if(MSVC)
        target_compile_options(TMP_Failure PRIVATE /utf-8)
    endif()
    foreach(test_case header_max header_wrap header_array multiply aligned_multiply system aligned_system block_round reserve_exhaustion)
        if(test_case MATCHES "multiply")
            set(expected_message "Allocation size overflow.")
        elseif(test_case STREQUAL "block_round")
            set(expected_message "Block size overflow (MemoryManager).")
        elseif(test_case STREQUAL "reserve_exhaustion")
            set(expected_message "Out of reserved memory (MemoryManager). Increase Reserve Size.")
        else()
            set(expected_message "Memory allocation failed.")
        endif()
        add_test(NAME TinyMemoryPool.failure.${test_case}
            COMMAND ${CMAKE_COMMAND}
                "-DTEST_PROGRAM=$<TARGET_FILE:TMP_Failure>"
                "-DTEST_CASE=${test_case}"
                "-DEXPECTED_MESSAGE=${expected_message}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_failure.cmake"
        )
    endforeach()

    find_package(Threads REQUIRED)
    add_executable(TMP_Concurrency tests/concurrency.cpp)
    target_link_libraries(TMP_Concurrency PRIVATE TinyMemoryPool::TinyMemoryPool Threads::Threads)
    add_test(NAME TinyMemoryPool.concurrent_allocations COMMAND TMP_Concurrency)

    # 경합 재현용 코드는 빌드 디렉터리의 사본에만 넣고 라이브러리 원본에는 넣지 않는다.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS src/internal/Pool.cpp)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/internal/Pool.cpp" pool_test_source)
    set(growth_return "    return reservedChunk;")
    string(FIND "${pool_test_source}" "${growth_return}" growth_return_position)
    if(growth_return_position EQUAL -1)
        message(FATAL_ERROR "Pool.cpp changed: update the deterministic growth test instrumentation.")
    endif()
    string(REPLACE "${growth_return}"
        "    TestAfterGrow(numChunks);\n${growth_return}" pool_test_source "${pool_test_source}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/PoolGrowthTest.cpp"
        "#include <cstddef>\nnamespace TinyMemoryPool::Detail { void TestAfterGrow(std::size_t); }\n${pool_test_source}")
    add_executable(TMP_GrowthRace tests/growth_race.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/PoolGrowthTest.cpp" src/internal/MemoryManager.cpp)
    target_include_directories(TMP_GrowthRace PRIVATE include src/internal)
    target_link_libraries(TMP_GrowthRace PRIVATE TBB::tbb Threads::Threads)
    if(MSVC)
        target_compile_options(TMP_Concurrency PRIVATE /utf-8)
        target_compile_options(TMP_GrowthRace PRIVATE /utf-8)
    endif()
    add_test(NAME TinyMemoryPool.growth_contention COMMAND TMP_GrowthRace)
    set_tests_properties(TinyMemoryPool.concurrent_allocations TinyMemoryPool.growth_contention
        PROPERTIES TIMEOUT 30)

    if(TMP_BUILD_EXAMPLES)
        add_test(NAME TinyMemoryPool.stl_example COMMAND TMP_StlExample)
    endif()
endif()
