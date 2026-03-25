#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

namespace ray {

using gen_index = uint64_t;

class index_pool_free_list {
private:
        std::vector<gen_index> free_list;
        gen_index last_max_index = 0;

public:
        gen_index alloc() {
                gen_index idx;

                if (!free_list.empty()) {
                        idx = free_list.back();
                        free_list.pop_back();
                } else {
                        assert((last_max_index != UINT64_MAX) && "Index pool overflow, what are u fucking doing there???");

                        last_max_index += 1;
                        idx = last_max_index;
                }

                return idx;
        }

        void free(gen_index h) {
#if RAY_DEBUG_NO_OPT
                assert(std::find(free_list.begin(), free_list.end(), h) == free_list.end()
                        && "Severe user-code bug: index double free detected.");
                assert(free_list.size() < 32000
                        && "Too long free_list of index pool.");
#endif
                free_list.push_back(h);
        }
};

using index_pool = index_pool_free_list;

};