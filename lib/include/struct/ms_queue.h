#pragma once
#include <atomic>
#include <memory>
#include <optional>
#include "struct/hp.h"

namespace seele::structs {
    template <typename T>
    class ms_queue {
    private:
        struct node_t;
        struct node_t{
            T data;
            std::atomic<node_t*> next;
            node_t() : next(nullptr) {}
            node_t(const T& item) : data(item), next(nullptr) {}

            template<typename... args_t>
            node_t(args_t&&... args) : data(std::forward<args_t>(args)...), next(nullptr) {}

            ~node_t() = default;
        };

    public:
        ms_queue();
        ~ms_queue();

        void push_back(const T& item) { emplace_back(item); }

        void push_back(T&& item) { emplace_back(std::move(item)); }

        template<typename... args_t>
        void emplace_back(args_t&&... args);
        std::optional<T> pop_front();

        bool is_empty() const;

    private:
        std::atomic<node_t*> head;
        std::atomic<node_t*> tail;
    };
    template <typename T>
    ms_queue<T>::ms_queue(){
        node_t* dummy = new node_t();
        head.store(dummy, std::memory_order_relaxed);
        tail.store(dummy, std::memory_order_relaxed);
    }

    template <typename T>
    ms_queue<T>::~ms_queue() {
        while (pop_front());          // 通过 hp::retire 回收真实节点
        hp::retire(head.load());      // 最后把 dummy 节点也放进 retired 列表
        hp::local_tls().scan();             // 主动做一次回收，清理剩余节点
    }

    template <typename T>
    template<typename... args_t>
    void ms_queue<T>::emplace_back(args_t&&... args) {
        node_t* new_node = new node_t(std::forward<args_t>(args)...);
        while (true) {
            auto old_tail = this->tail.load();
            auto next = old_tail->next.load();
            if (next == nullptr) {
                if (old_tail->next.compare_exchange_weak(
                    next, new_node,
                    std::memory_order_release,
                    std::memory_order_relaxed
                )) {
                    this->tail.compare_exchange_weak(
                        old_tail, new_node,
                        std::memory_order_release,
                        std::memory_order_relaxed
                    );
                    return;
                }
            } else {
                this->tail.compare_exchange_weak(
                    old_tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed
                );
            }
        }
    }

    template <typename T>
    std::optional<T> ms_queue<T>::pop_front() {
        constexpr std::size_t HAZ_HEAD = 0;
        constexpr std::size_t HAZ_NEXT = 1;

        while (true) {
            node_t* dummy = head.load(std::memory_order_acquire);
            hp::protect(HAZ_HEAD, dummy);                    

            if (dummy != head.load(std::memory_order_acquire))
                continue;                                    

            node_t* next = dummy->next.load(std::memory_order_acquire);
            if (next == nullptr) {
                hp::clear(HAZ_HEAD);
                return std::nullopt;
            }

            hp::protect(HAZ_NEXT, next);                     
            if (dummy != head.load(std::memory_order_acquire) ||
                next != dummy->next.load(std::memory_order_acquire))
                continue;                                   

            if (head.compare_exchange_strong(dummy, next,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
                auto value = std::make_optional<T>(std::move(next->data));
                hp::clear(HAZ_NEXT);                         
                hp::clear(HAZ_HEAD);
                hp::retire(dummy);                           // 延迟删除
                return value;
            }
        }
    }

}