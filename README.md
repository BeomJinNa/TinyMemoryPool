# TinyMemoryPool

STL 컨테이너에 연결해 사용하는 작은 C++ 메모리 풀 라이브러리입니다.
TinyRenderer에서 사용하며, 크기별 청크 재사용과 멀티스레드 할당·반납을 지원합니다.

- **STL 연동**: `Allocator<T>`를 `vector`, `string`, `unordered_map` 등에 연결합니다.
- **oneTBB 활용**: `concurrent_queue`로 가용 청크를 관리하고, 풀 확장은 mutex로 직렬화합니다.
- **플랫폼 추상화**: STL allocator, 풀 관리, OS 가상 메모리 계층을 분리하여 사용부에 OS·TBB 헤더를 노출하지 않습니다.

Windows·POSIX backend를 포함하며, 현재 실행 검증 범위는 Windows x64입니다.

## 빠른 시작

C++20, CMake 3.15 이상과 oneTBB가 필요합니다. 프로젝트에 포함하고 라이브러리를 연결합니다.

```cmake
add_subdirectory(third_party/TinyMemoryPool)
target_link_libraries(MyApplication PRIVATE TinyMemoryPool::TinyMemoryPool)
```

컨테이너의 allocator만 지정하면 됩니다. 공유 풀은 최초 사용 시 초기화됩니다.

```cpp
#include <TinyMemoryPool/Allocator.h>
#include <vector>

int main()
{
    std::vector<int, TinyMemoryPool::Allocator<int>> values;
    values.reserve(1000);
    values.push_back(42);
}
```

할당 실패 시에는 예외 대신 프로세스를 종료합니다. 풀 종료 전 컨테이너와 작업 스레드를 정리해야 하며, 같은 컨테이너의 동시 접근은 별도로 동기화해야 합니다.

## STL 래퍼 예제

TinyRenderer에서 사용하는 방식처럼 타입 별칭을 두면 allocator를 매번 적지 않아도 됩니다. `TVector`, `TString`, `THashMap`은 별도 컨테이너가 아니라 STL 컨테이너에 allocator를 지정한 별칭입니다.

```cpp
#include "PoolContainers.h"
#include <iostream>

int main()
{
    using namespace TinyMemoryPool::Examples;

    TVector<int> objectIds = {101, 102};
    TString sceneName = "TinyRenderer - STL allocator example";

    THashMap<int, TString> objectNames;
    objectNames.emplace(101, "Camera");
    objectNames.emplace(102, "Light");

    const TString& firstObjectName = objectNames.at(objectIds.front());
    std::cout << sceneName << ": " << firstObjectName << '\n';
}
```

- [PoolContainers.h](examples/PoolContainers.h): 별칭 정의
- [stl_containers.cpp](examples/stl_containers.cpp): 객체 목록과 이름을 조회·출력하는 전체 예제

예제 헤더는 설치 API에 포함하지 않습니다. `THashMap`의 키로 `TString`을 사용하려면 해당 타입에 맞는 해시 함수를 전달해야 합니다.

## 예제와 테스트 빌드

`TBB_DIR` 또는 `CMAKE_PREFIX_PATH`로 oneTBB 설치 위치를 지정합니다.

```sh
cmake -S . -B out -DTBB_DIR=/path/to/oneTBB/lib/cmake/TBB -DTMP_BUILD_EXAMPLES=ON
cmake --build out --config Release
ctest --test-dir out -C Release --output-on-failure
```

- `TMP_BUILD_EXAMPLES`: 독립 예제 `TMP_StlExample` 빌드, 기본값 `OFF`
- `TMP_BUILD_TESTS`: 최상위 프로젝트로 구성할 때 테스트 빌드, 기본값 `ON`
- 단일 구성 생성기는 구성 단계에 `-DCMAKE_BUILD_TYPE=Release`를 추가합니다.
- 공유 라이브러리 방식의 TBB는 실행 시 DLL 또는 공유 라이브러리 검색 경로도 필요합니다.

## 내부 구조

| 계층 | 역할 |
| --- | --- |
| `Allocator<T>` | STL allocator 인터페이스를 제공하며, 높은 정렬 요청은 시스템 할당으로 분기합니다. |
| `MemoryApi` | 공개 헤더와 내부 풀 구현 사이의 할당·반납 접점입니다. |
| `PoolManager` | 헤더 포함 요청 크기를 64~4096바이트 풀 또는 시스템 할당으로 분기합니다. |
| `Pool` | 반납된 청크를 재사용합니다. 가용 청크가 없으면 블록을 확장하고 호출자 몫 하나를 확보합니다. |
| `MemoryManager` | 하나의 가상 주소 영역을 예약하고 필요한 구간을 페이지 단위로 커밋합니다. |
| `PlatformMemory` | Windows의 `VirtualAlloc/VirtualFree`와 POSIX의 `mmap/mprotect/munmap`을 분리합니다. |

기본 가상 주소 예약 크기는 1GiB이며, 전체를 한 번에 커밋하지 않습니다. 반납된 청크는 재사용하고, 풀 메모리는 관리자가 종료될 때 일괄 해제합니다. 공개 런타임 설정이나 풀 재시작 API는 제공하지 않습니다.

할당은 우선 `try_pop`으로 청크를 가져옵니다. 실패하면 mutex 획득 후 다시 확인하고 필요할 때만 확장합니다. 확장 시 첫 청크는 공유 큐에 넣지 않고 호출자에게 반환하므로, 다른 스레드가 가용 청크를 소비하더라도 호출자 몫은 유지됩니다. 확장에 mutex를 사용하므로 전체 할당 경로가 lock-free인 구조는 아닙니다.

## 사용 계약

### 동시성과 수명

공유 풀의 최초 초기화는 함수 지역 static으로 처리합니다. 풀 수명 내에서는 여러 스레드가 할당·반납할 수 있으며, 다른 스레드에서 할당한 메모리를 반납하는 것도 가능합니다.

- **allocator의 동시성 지원이 같은 STL 컨테이너의 동시 수정을 허용하는 것은 아닙니다.** 컨테이너와 객체의 동기화는 별도입니다.
- 스레드 간 객체 인계도 호출자가 동기화해야 합니다. 내부 초기화·종료 함수를 사용 중인 풀에 호출해서는 안 되며, 풀 종료 전 모든 작업 스레드를 합류시키고 객체·컨테이너를 정리해야 합니다.
- **전역·정적 객체의 소멸 순서는 자동으로 해결하지 않습니다.** 예를 들어 빈 전역 컨테이너가 먼저 생성되고 `main`에서 풀을 처음 사용하면, 풀 종료 후 컨테이너가 메모리를 반납할 수 있습니다. 수명을 명시적으로 보장할 수 없다면 예제처럼 지역 객체로 사용합니다.

### 크기와 정렬

- `PoolAlignment`는 16과 `alignof(std::max_align_t)` 중 큰 값입니다. 검증한 Windows x64에서는 16바이트입니다.
- 청크 크기가 64바이트 이상이어도 **payload의 64바이트 정렬을 뜻하지는 않습니다.** 반환 주소 앞에 내부 헤더가 배치됩니다.
- `alignas(64)`처럼 `PoolAlignment`보다 높은 정렬이 필요한 타입은 풀 대신 정렬 지정 시스템 할당을 사용하며, 대응하는 aligned delete로 반납합니다.
- 풀 상한 4096바이트에는 헤더가 포함됩니다. 16바이트 헤더인 환경에서는 payload 4080바이트까지 풀을 사용하고, 더 큰 요청은 시스템 할당으로 처리합니다.

### 실패 처리

`Allocator<T>::allocate`는 `noexcept`입니다. 원소 수 곱셈과 헤더 크기 덧셈의 오버플로, 시스템 할당 실패는 진단을 출력한 뒤 `std::terminate`로 종료합니다. 시스템 할당은 `std::nothrow`로 실패를 확인하며, 호출자에게 null이나 복구용 예외를 전달하지 않습니다.

페이지 단위 크기 올림의 오버플로, 예약 영역 소진, OS 예약·커밋 실패도 같은 종료 정책을 따릅니다. 객체 생성자·표준 컨테이너·oneTBB의 예외 정책은 별도이며, 이 라이브러리가 사용 프로젝트의 예외 설정을 변경하지는 않습니다.
