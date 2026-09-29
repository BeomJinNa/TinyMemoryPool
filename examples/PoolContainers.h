#pragma once

#include <TinyMemoryPool/Allocator.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace TinyMemoryPool::Examples
{

// 컨테이너의 동작은 STL 그대로 두고, 내부 메모리 할당에 TinyMemoryPool을 사용한다.
template <typename T>
using TVector = std::vector<T, Allocator<T>>;

using TString = std::basic_string<char, std::char_traits<char>, Allocator<char>>;

// unordered_map의 원소 타입에 맞춰 키가 const인 pair를 할당한다.
template <typename K, typename V, typename Hash = std::hash<K>, typename Equal = std::equal_to<K>>
using THashMap = std::unordered_map<K, V, Hash, Equal, Allocator<std::pair<const K, V>>>;

} // namespace TinyMemoryPool::Examples
