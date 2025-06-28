#include "struct/hp.h"

namespace seele::hp {

    std::array<hazard_record, HP_MAX_THREADS> g_records;      // 全局固定大小表

    tls_data::~tls_data() {
        this->clear_all();
        this->scan();                       // 线程退出前做最后一次回收
        if (record_hzd) 
            this->record_hzd->active.store(false, std::memory_order_release);
    }

    void tls_data::clear(std::size_t idx) {
        if (this->record_hzd) {
            this->record_hzd->haz[idx].store(nullptr, std::memory_order_release);
        }
    }

    void tls_data::clear_all() {
        if (this->record_hzd){
            for (auto& h : this->record_hzd->haz) 
                h.store(nullptr, std::memory_order_release);                
        }
    }

    void tls_data::push_retired(retired_ptr r) {
        retired.push_back(r);
        if (retired.size() >= HP_RECLAIM_BATCH) this->scan();
    }

    void tls_data::scan(){
        // Step 1: 收集所有在用 hazard 指针
        auto hp_list = g_records 
                | std::views::filter([](auto& record) {
                    return record.active.load(std::memory_order_acquire);
                }) 
                | std::views::transform([](auto& record) {
                    return record.haz 
                        | std::views::transform([](const std::atomic<void*>& h) {
                            return h.load(std::memory_order_acquire);
                        })
                        | std::views::filter([](void* p) { return p != nullptr; });
                }) 
                | std::views::join;

        // Step 2: 遍历 retired 列表，若不在 hp_list 中则真正 delete
        auto end_it = std::ranges::remove_if(retired, [&](const auto& item) {
            if (!std::ranges::any_of(hp_list, [&](void* p) { 
                return p == item.ptr; 
            })) {
                item.deleter(item.ptr);
                return true;
            }
            return false;
        });

        retired.erase(end_it.begin(), end_it.end());
    }

    tls_data& local_tls() {
        static thread_local tls_data data;
        return data;
    }

    hazard_record* acquire_record() {
        auto& tls = hp::local_tls();
        if (tls.record_hzd) return tls.record_hzd;

        for (auto& record : g_records) {
            bool expected = false;
            if (record.active.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                tls.record_hzd = &record;
                return tls.record_hzd;
            }
        }
        return nullptr; 
    }

    void* protect(std::size_t idx, void* p) {
        hazard_record* record = acquire_record();
        if (!record) return nullptr;
        record->haz[idx].store(p, std::memory_order_release);
        return p;
    }

    void clear(std::size_t idx) {
        hp::local_tls().clear(idx);
    }


} // namespace seele::hp