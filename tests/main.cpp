#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#include <TinyMemoryPool/Allocator.h>

using namespace TinyMemoryPool;

class Timer
{
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;

  public:
    Timer(std::string name) : m_name(name), m_start(std::chrono::high_resolution_clock::now())
    {
    }
    ~Timer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> diff = end - m_start;
        std::cout << "[" << m_name << "] : " << diff.count() << " ms" << std::endl;
    }
};

void TestFunctional()
{
    std::cout << "=== 1. Functional Test (Address Check) ===" << std::endl;

    std::vector<int, Allocator<int>> v;

    for(int i = 0; i < 10; ++i)
    {
        v.push_back(i);
    }

    std::cout << "Vector Element Addresses:" << std::endl;
    for(int i = 0; i < 10; ++i)
    {
        std::cout << "Index " << i << ": " << &v[i] << " (Value: " << v[i] << ")" << std::endl;
    }
    std::cout << "-> If no crash, Allocator -> Bridge -> PoolManager works!" << std::endl << std::endl;
}

void TestBenchmark()
{
    std::cout << "=== 2. Benchmark (std vs TinyMemoryPool) ===" << std::endl;
    const int ITEM_COUNT = 1'000'000; // 100¸¸ °³

    {
        Timer t("Standard Allocator (std::vector)");
        std::vector<int> v_std;
        v_std.reserve(ITEM_COUNT);
        for(int i = 0; i < ITEM_COUNT; ++i)
            v_std.push_back(i);
    }

    {
        Timer t("TinyMemoryPool Allocator (std::vector)");
        std::vector<int, Allocator<int>> v_tmp;
        v_tmp.reserve(ITEM_COUNT);
        for(int i = 0; i < ITEM_COUNT; ++i)
            v_tmp.push_back(i);
    }

    std::cout << "\n--- Small Object Allocation (Node-like) ---" << std::endl;

    struct Node
    {
        int data;
        Node* next;
    };

    {
        Timer t("Standard new/delete");
        for(int i = 0; i < ITEM_COUNT; ++i)
        {
            Node* p = new Node{i, nullptr};
            delete p;
        }
    }

    {
        Timer t("TinyMemoryPool Allocate/Deallocate");
        Allocator<Node> alloc;
        for(int i = 0; i < ITEM_COUNT; ++i)
        {
            Node* p = alloc.allocate(1);
            alloc.deallocate(p, 1);
        }
    }
}

int main()
{
    try
    {
        TestFunctional();
        TestBenchmark();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}