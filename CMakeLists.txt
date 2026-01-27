cmake_minimum_required(VERSION 3.15)

# [프로젝트 정보]
project(TinyMemoryPool 
    VERSION 1.0.0
    LANGUAGES CXX
)

# [옵션 설정] 상위 프로젝트에서 포함할 때 테스트 빌드 여부를 제어하기 위함
option(TMP_BUILD_TESTS "Build tests for TinyMemoryPool" ON)

# [표준 설정]
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# [최적화: LTO] 브릿지 함수 인라인화를 위해 필수
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if(result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    # message(STATUS "IPO / LTO enabled") # (너무 시끄러우면 주석 처리)
else()
    message(WARNING "IPO / LTO not supported: ${output}")
endif()

# [의존성] TBB
find_package(TBB CONFIG REQUIRED)

# [소스 파일] 헤더도 리스트에 넣어야 IDE(Visual Studio 등) 솔루션 탐색기에 뜸
set(TMP_SOURCES
    # Internal Implementation
    src/internal/MemoryApi.cpp
    src/internal/MemoryManager.cpp
    src/internal/Pool.cpp
    src/internal/PoolManager.cpp
    
    # Internal Headers (IDE Display)
    src/internal/Common.h
    src/internal/MemoryManager.h
    src/internal/PlatformMemory.h
    src/internal/Pool.h
    src/internal/PoolManager.h
    src/internal/backends/PosixMemory.h
    src/internal/backends/WindowsMemory.h

    # Public Headers
    include/TinyMemoryPool/Allocator.h
    include/TinyMemoryPool/Config.h
    include/TinyMemoryPool/Detail/MemoryApi.h
)

# [라이브러리 타겟 정의]
add_library(TinyMemoryPool STATIC ${TMP_SOURCES})

# [별칭(Alias) 추가] 
# 내부 테스트 코드나 상위 프로젝트가 항상 'TinyMemoryPool::TinyMemoryPool'로 
# 일관되게 참조할 수 있게 함 (find_package 스타일)
add_library(TinyMemoryPool::TinyMemoryPool ALIAS TinyMemoryPool)

# [인클루드 경로 전략]
target_include_directories(TinyMemoryPool 
    PUBLIC
        # 외부 사용자 & 나: <TinyMemoryPool/Allocator.h> 형태로 접근
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
    PRIVATE
        # 내부 구현(src): "PoolManager.h" 처럼 파일명만으로 접근
        "${CMAKE_CURRENT_SOURCE_DIR}/src/internal"
)

# [컴파일 옵션] 경고 수준 상향 (선택 사항)
if(MSVC)
    target_compile_options(TinyMemoryPool PRIVATE /W4)
else()
    target_compile_options(TinyMemoryPool PRIVATE -Wall -Wextra)
endif()

# [링킹]
target_link_libraries(TinyMemoryPool PRIVATE TBB::tbb)

# [설치 설정]
install(TARGETS TinyMemoryPool EXPORT TinyMemoryPoolTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/TinyMemoryPool DESTINATION include)

# [테스트] 옵션이 켜져 있고, 이 프로젝트가 메인일 때만 빌드
if(TMP_BUILD_TESTS AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "Building TinyMemoryPool Tests...")
    add_executable(TMP_Test tests/main.cpp)
    target_link_libraries(TMP_Test PRIVATE TinyMemoryPool::TinyMemoryPool)
endif()