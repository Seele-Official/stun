#pragma once
#include <atomic>
#include <array>
#include <cstddef>
#include <list>
#include <algorithm>
#include <ranges>

namespace seele::hp {

    constexpr std::size_t HP_MAX_THREADS   = 128;    // 最大线程数
    constexpr std::size_t HP_SLOTS_PER_THD = 2;     // 每线程可保护的指针数
    constexpr std::size_t HP_RECLAIM_BATCH = 64;    // 每收集 64 个待回收节点尝试回收

    struct retired_ptr {
        void* ptr;
        auto (*deleter)(void*) -> void;
    };

    struct hazard_record {
        std::array<std::atomic<void*>, HP_SLOTS_PER_THD> haz{}; // 每 slot 是一个被保护指针
        std::atomic<bool>                                active{false};
    };

    extern std::array<hazard_record, HP_MAX_THREADS> g_records;      // 全局固定大小表

    // 记录当前线程的 record + 待回收链表
    struct tls_data {
        hazard_record*               record_hzd{nullptr};
        std::list<retired_ptr>       retired{};

        ~tls_data();
        void clear(std::size_t idx);
        void clear_all();
        void push_retired(retired_ptr r);
        void scan();
    };

    extern tls_data& local_tls();
    hazard_record* acquire_record();
    void* protect(std::size_t idx, void* p);
    void clear(std::size_t idx);

    template<typename T>
    void retire(T* p) {
        hp::local_tls().push_retired(
            {
                p, 
                [](void* q){ delete static_cast<T*>(q); }
            }
        );
    }

} 