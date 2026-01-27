# TinyMemoryPool

**TinyMemoryPool**은 C++20 표준을 준수하는 스레드 안전(Thread-Safe) 메모리 풀 라이브러리입니다.
STL 컨테이너(`std::vector`, `std::map` 등)와 호환되는 `Allocator` 인터페이스를 제공하며, 내부적으로 Intel TBB의 `concurrent_queue`를 사용하여 멀티 스레드 환경에서의 락 경합(Lock Contention)을 최소화합니다.

## 1. 주요 특징 (Features)

* **STL 호환**: `std::allocator_traits`를 지원하여 모든 STL 컨테이너에 즉시 적용 가능.
* **Thread-Safe**: Lock-Free 자료구조를 사용하여 멀티 스레드 환경에서 안전하게 동작.
* **Header Isolation**: Bridge 패턴을 적용하여, 라이브러리 사용 시 `<windows.h>`나 `tbb` 헤더 의존성이 외부로 전파되지 않음.
* **구조**:
    * **Layer 1**: STL Allocator Interface (Stateless)
    * **Layer 2**: Pool Manager (Singleton Engine, 64B~4KB Buckets)
    * **Layer 3**: OS Memory Interface (`VirtualAlloc` / `mmap`)

## 2. 요구 사항 (Requirements)

이 라이브러리를 사용하기 위해서는 다음 환경이 필요합니다.

* **C++ Standard**: C++20 이상 (`std::bit_width`, `std::byte`, `concept` 사용)
* **Build System**: CMake 3.15 이상
* **Dependencies**:
    * **Intel TBB (Threading Building Blocks)**: 필수 의존성입니다.
    * CMake 설정 시 `find_package(TBB CONFIG REQUIRED)`가 실행되므로, 시스템이나 vcpkg 등을 통해 TBB가 설치되어 있어야 합니다.

## 3. 통합 가이드 (Integration Guide)

이 프로젝트는 **CMake Subdirectory** 방식을 권장합니다. 소스 코드를 직접 포함하여 빌드하므로 디버깅이 용이합니다.

### 3.1. 프로젝트 추가
`git submodule` 또는 코드를 복사하여 프로젝트의 하위 디렉토리(예: `third_party`)에 위치시킵니다.

```bash
# 예시
git submodule add [Repository URL] third_party/TinyMemoryPool

```

### 3.2. CMakeLists.txt 설정

사용하려는 상위 프로젝트(게임 엔진 등)의 `CMakeLists.txt`에 다음을 추가합니다.

```cmake
# 1. 라이브러리 하위 디렉토리 추가
add_subdirectory(third_party/TinyMemoryPool)

# ... (타겟 정의) ...
add_executable(MyGameEngine src/main.cpp)

# 2. 라이브러리 링크
# TinyMemoryPool::TinyMemoryPool 별칭(ALIAS)을 사용하면
# 향후 find_package로 변경해도 코드 수정이 필요 없습니다.
target_link_libraries(MyGameEngine PRIVATE TinyMemoryPool::TinyMemoryPool)

# [권장] LTO (Link Time Optimization) 활성화
# 브릿지 함수(MemoryApi)의 인라인 최적화를 위해 활성화를 권장합니다.
set_property(TARGET MyGameEngine PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

```

## 4. 사용 방법 (Usage)

외부에 공개된 헤더는 오직 `<TinyMemoryPool/Allocator.h>` 하나입니다.

### 4.1. STL 컨테이너 적용

```cpp
#include <vector>
#include <map>
#include <TinyMemoryPool/Allocator.h>

void Example() {
    // 1. std::vector 사용 예시
    // 4KB 이하 객체는 Pool에서, 4KB 초과는 malloc으로 자동 처리됨
    std::vector<int, TinyMemoryPool::Allocator<int>> v;
    v.reserve(1000);
    v.push_back(10);

    // 2. std::map 사용 예시 (Node 기반 컨테이너에서 효율적)
    std::map<int, float, std::less<int>, 
             TinyMemoryPool::Allocator<std::pair<const int, float>>> m;
    m[1] = 3.14f;
}

```

### 4.2. 동작 방식 참고

* **초기화**: `Allocator`가 최초로 인스턴스화되는 시점에 내부 엔진(`PoolManager`)이 자동으로 초기화됩니다. 별도의 `Init()` 함수 호출이 필요 없습니다.
* **폴백(Fallback)**: 단일 할당 요청 크기가 **4096 Bytes(4KB)**를 초과할 경우, 메모리 풀을 거치지 않고 시스템 `malloc`을 직접 사용합니다.

## 5. 빌드 및 테스트 (Build & Test)

라이브러리를 단독으로 빌드하거나 테스트를 실행할 때 사용합니다.

```bash
# 구성 (Configure)
cmake -B out

# 빌드 (Build) - 반드시 Release 모드로 빌드해야 정확한 성능이 나옵니다.
cmake --build out --config Release

# 테스트 실행
./out/Release/TMP_Test

```

> **주의**: Debug 모드에서는 Intel TBB의 내부 검증 로직과 인라인 최적화 부재로 인해 성능이 시스템 할당자보다 느리게 측정될 수 있습니다. 벤치마킹은 반드시 **Release/RelWithDebInfo** 모드에서 수행하십시오.

## 6. 디렉토리 구조 (Directory Structure)

```text
TinyMemoryPool/
├── include/
│   └── TinyMemoryPool/          # [Public] 외부 공개 헤더 경로
│       ├── Allocator.h          # 사용자가 include 하는 메인 헤더
│       ├── Config.h             # 내부 설정값
│       └── Detail/              # 구현 은닉용 브릿지 헤더
├── src/
│   └── internal/                # [Private] 내부 구현 소스 (Pool, Manager 등)
└── tests/                       # 기능 테스트 및 벤치마크 코드

```
