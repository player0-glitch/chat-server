#pragma once
#include <cstddef>
#include <cstdlib>
#include <unordered_map>
#include <vector>

#define PROTO_IMPL
#include "proto.hpp"
using std::array;
using std::unordered_map;
using std::vector;

constexpr int INVALID_FD = -1;
/* A Slab Arena Allocator with indexing (file descriptors)
 * An additional string indexable array has been added for indexing usernames
 */
class slab_arena {
  public:
    explicit slab_arena(int capacity) {
        _use_list.resize(capacity);
        fd_to_idx.fill(INVALID_FD);
    };

    int alloc() {
        // if the free_list is non empty get the last block and mark it as unused
        if (!_free_list.empty()) {
            int idx = _free_list.back(); // return the index value
            _free_list.pop_back();       // destructively remove the last index
            return idx;
        }

        int used_size = static_cast<int>(_use_list.size());
        if (_next >= used_size)
            return -1;

        // continue allocating as usual
        return _next++;
    }

    void dealloc(int idx) {
        // populate the list that keeps track of our allocations
        _free_list.push_back(idx);
    }

    client_t &getSlab(int idx) { return _use_list.at(idx); }

  private:
    vector<client_t> _use_list{};
    vector<int> _free_list{};
    array<int, MAX_FDS> fd_to_idx{};
    unordered_map<std::string, int> usernames_to_idx{};
    int _next{0};
};
