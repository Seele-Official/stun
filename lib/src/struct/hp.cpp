#include <ranges>
#include <algorithm>
#include "struct/hp.h"
#include "log.h"
namespace seele::structs {

    hazard_manager::tls_map_t::~tls_map_t() {
        auto& map = hazard_manager::tls_map;
        for (auto& [manager, data] : map) {
            manager->scan_tls_retired();
            if(data.retired_list.size() > 0) {
                manager->collect_thread_unretired(data.retired_list);
            }
            data.record->active.store(false, std::memory_order_release);
        }

    }



    hazard_record_t* hazard_manager::allocate_record(){
        for (auto& record : records) {
            if (!record.active.load(std::memory_order_acquire)) {
                record.active.store(true, std::memory_order_release);
                return &record;
            }
        }
        return nullptr; 
    }


    inline void hazard_manager::deallocate_record(hazard_record_t* record){
        record->active.store(false, std::memory_order_release);
    }

    tls_data_t& hazard_manager::local_tls() {
        auto it = tls_map.find(this);
        if (it != tls_map.end()) {
            return it->second;
        } else {
            hazard_record_t* record = allocate_record();
            // here we assume that allocate_record never returns nullptr
            return tls_map.emplace(this, tls_data_t{record, {}}).first->second;
        }
    }
    void hazard_manager::collect_thread_unretired(std::list<retired_ptr_t>& retireds){
        std::lock_guard<std::mutex> lock(g_retired_mutex);
        this->g_retired.splice(g_retired.end(), retireds);
        if (g_retired.size() > max_retired_count) {
            this->scan_retired(g_retired);
        }
        
    }

    void hazard_manager::scan_retired(std::list<retired_ptr_t>& retired_list){

        auto hps = records
            | std::views::filter([](hazard_record_t& rec) {
                return rec.active.load(std::memory_order_acquire);
            })
            | std::views::transform([](hazard_record_t& rec) -> std::array<std::atomic<void*>, max_hazard_count>& {
                return rec.hps;
            })
            | std::views::join;
        


        retired_list.remove_if([&hps](retired_ptr_t& rp) {


            auto& [ptr, deleter] = rp;
            if (std::ranges::any_of(hps, [&ptr](std::atomic<void*>& hazard) {
                    return hazard.load(std::memory_order_acquire) == ptr;
                })) {
                // If the retired pointer is still in use, we need to keep it
                return false;
            }
            // Otherwise, we can safely delete it
            if (deleter) {
                deleter(ptr);
                return true; // Remove from the list
            }
            log::sync().error("retired pointer has no deleter.");
            std::terminate();
        });
    }


    void hazard_manager::scan_tls_retired() {
        auto& r = local_tls().retired_list;
        if (r.size() > max_retired_count) {
            this->scan_retired(r);
        }
    }


    void hazard_manager::clear_all(){
        for(auto& hp : this->local_tls().record->hps) {
            hp.store(nullptr, std::memory_order_release);
        }
    }



    hazard_manager::~hazard_manager() {            
        //because tls destructor is called after the this thread exits,
        //deallocator cant refer to this object anymore.
        //so we need to do destruction manually.
        auto it = tls_map.find(this);
        if (it != tls_map.end()) {
            if(it->second.retired_list.size() > 0) {
                this->collect_thread_unretired(it->second.retired_list);
            }
            it->second.record->active.store(false, std::memory_order_release);
            tls_map.erase(it);
        }

        std::lock_guard<std::mutex> lock(g_retired_mutex);

        this->scan_retired(g_retired);
        if (g_retired.size() > 0) {
            log::sync().error("Hazard manager still has {} retired pointers after destruction.", g_retired.size());
            for (const auto& [index, record] : std::views::enumerate(this->records)) {
                if (record.active.load(std::memory_order_acquire)) {
                    log::sync().error("Hazard record {} is still active.", index);
                }
            }

            std::terminate();
        }
    }

    thread_local hazard_manager::tls_map_t hazard_manager::tls_map{};

} // namespace seele::structs
