#pragma once

#include <list>

class IdAllocator {
public:
    IdAllocator() {
        reallocate(0, _init_size);
    }

    size_t allocate() {
        if (_id_pool.empty()) {
            size_t cur_cap = capacity();
            size_t new_cap = cur_cap * 2;
            size_t size = new_cap - cur_cap;

            reallocate(cur_cap, new_cap);
            ++_reallocate_count;
        }
        
        size_t id = _id_pool.front();
        _id_pool.pop_front();
        return id;
    }

    void deallocate(size_t id) {
        _id_pool.push_front(id);
    }

    size_t capacity() {
        return _init_size * (size_t(1) << _reallocate_count);
    }

private:
    void reallocate(size_t beg, size_t end) {
        _id_pool.resize(end - beg, 0);
        size_t i = beg;
        for (auto iter = _id_pool.begin(); iter != _id_pool.end(); ++iter) {
            *iter = i++;
        }
    }

    const size_t _init_size = 16;
    int _reallocate_count = 0;

    std::list<size_t> _id_pool;
    
};