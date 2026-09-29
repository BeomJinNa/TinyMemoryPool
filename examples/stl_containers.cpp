#include "PoolContainers.h"

#include <iostream>

int main()
{
    using namespace TinyMemoryPool::Examples;

    // TVector: std::vector처럼 객체 ID를 순서대로 보관한다.
    TVector<int> objectIds = {101, 102, 103};
    objectIds.push_back(104);

    // TString: std::string처럼 문자열을 만들고 이어 붙인다.
    TString sceneName = "TinyRenderer";
    sceneName += " - STL allocator example";

    // THashMap: 객체 ID를 키로 사용해 이름을 찾는다.
    THashMap<int, TString> objectNames;
    objectNames.emplace(101, "Camera");
    objectNames.emplace(102, "Light");
    objectNames.emplace(103, "Mesh");
    objectNames.emplace(104, "Ground");

    std::cout << "Scene: " << sceneName << '\n';
    std::cout << "Objects: " << objectIds.size() << '\n';
    for(const int objectId : objectIds)
    {
        std::cout << objectId << ": " << objectNames.at(objectId) << '\n';
    }

    // 지역 컨테이너가 먼저 소멸하며 메모리를 반납한다. 공유 풀은 그 뒤 종료된다.
    return 0;
}
