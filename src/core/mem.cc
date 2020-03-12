#include <core/mem.hh>

#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <memory>

namespace izumo::core {
    // allocate a chunk of memory for pool
    // return nullptr when allocation failed
    static _mem_chunk_header*
    alloc_chunk() noexcept
    {
	auto mem = std::malloc(mem_pool::CHUNK_SIZE);
	if (!mem) return nullptr;
	
	auto ret = new (mem) _mem_chunk_header();
	ret->remaining = mem_pool::CHUNK_SIZE - sizeof(ret);
	return ret;
    }

    // deallocate a chunk of memory for pool
    static void
    dealloc_chunk(_mem_chunk_header* ptr) noexcept
    {
	free(ptr);
    }
    
    // allocate memory for a large object
    // return nullptr when allocation failed
    static _mem_large_meta*
    alloc_large(std::size_t size, std::size_t alignment) noexcept
    {
	// c++ standard require alignment to be a power of two
	// and aligned_alloc require size to be a multiplication of alignment
	// so using the largest alignment guarantees both object has their 
	// requirement satisfied

	auto _alignment = std::max(alignment, alignof(_mem_large_meta));
	auto _size = size + sizeof(_mem_large_meta);
	auto mem = std::aligned_alloc(_alignment, _size);

	// put meta in the tail of memory only if the large object
	// has a more struct align requirement
	// otherwise in the head of memory
	auto meta_mem_ptr = static_cast<char*>(mem);
	auto obj_mem_ptr = static_cast<char*>(mem);
	if (alignment > alignof(_mem_large_meta)) {
	    meta_mem_ptr += size;
	} else {
	    obj_mem_ptr += sizeof(_mem_large_meta);
	}

	auto meta = new (meta_mem_ptr) _mem_large_meta;
	meta->ptr = obj_mem_ptr;

	return meta;
    }

    // deallocate memory for a large object
    // object must be destructed beforehand
    static void
    dealloc_large(_mem_large_meta* meta) noexcept
    {
	// reinterpret_cast to char* does not break strict-aliasing
	auto meta_ptr = reinterpret_cast<char*>(meta);
	auto obj_ptr = static_cast<char*>(meta->ptr);

	// the ptr in front is the ptr return by aligned_alloc
	free(std::min(meta_ptr, obj_ptr));
    }

    // allocate memory from memory chunk
    // return pointer to memory, or nullptr if there's no enough space
    static void*
    alloc_from_chunk(_mem_chunk_header* chunk, std::size_t size, std::size_t alignment) noexcept
    {
	auto ptr = reinterpret_cast<char*>(chunk) + sizeof(_mem_chunk_header);
	auto used_size = mem_pool::CHUNK_SIZE - sizeof(_mem_chunk_header) - chunk->remaining;
	ptr += used_size;

	auto _ptr = static_cast<void*>(ptr);
	auto ret = static_cast<char*>(std::align(alignment, size, _ptr, chunk->remaining));
	if (!ret) return nullptr;

	ret += size;
	chunk->remaining -= size;

	return ret;
    }

    // alloc a new memory chunk. return success or not.
    bool
    mem_pool::m_alloc_chunk() noexcept
    {
	auto new_chunk = alloc_chunk();
	if (!new_chunk) return false;
	
	new_chunk->prev = m_chunk_p;
	m_chunk_p = new_chunk;
	return true;
    }

    // alloc memory for a large object. return nullptr on failure.
    _mem_large_meta*
    mem_pool::m_alloc_large(std::size_t size, std::size_t alignment) noexcept
    {
	auto ret = alloc_large(size, alignment);
	if (!ret) return nullptr;
	
	ret->prev = m_large_p;
	m_large_p = ret;
	return ret;
    }

    void*
    mem_pool::try_allocate(std::size_t size, std::size_t alignment) noexcept
    {
	if (size >= mem_pool::LARGE_THRESHOLD) {
	    // large object allocation
	    auto large = m_alloc_large(size, alignment);

	    return large ? large->ptr : nullptr;
	}

	if (m_chunk_p) {
	    // try allocating from current chunk
	    auto ret = alloc_from_chunk(m_chunk_p, size, alignment);
	    if (ret) return ret;
	}

	// try allocating a new chunk
	if (!m_alloc_chunk()) return nullptr; // no new chunk avaliable, fail

	auto ret = alloc_from_chunk(m_chunk_p, size, alignment);
	assert(ret);
	return ret;
    }

    void*
    mem_pool::allocate(std::size_t size, std::size_t alignment) noexcept
    {
	auto ret = try_allocate(size, alignment);
	if (!ret) std::abort();
	return ret;
    }

    mem_pool::mem_pool(mem_pool&& rhs)
    {
	m_chunk_p = rhs.m_chunk_p;
	m_large_p = rhs.m_large_p;

	rhs.m_chunk_p = nullptr;
	rhs.m_large_p = nullptr;
    }

    mem_pool::~mem_pool()
    {
	// deallocate every chunk
	auto cp = m_chunk_p;
	while (cp) {
	    cp = cp->prev;
	    dealloc_chunk(cp);
	}

	// deallocate every large object
	auto lp = m_large_p;
	while (lp) {
	    lp = lp->prev;
	    dealloc_large(lp);
	}
    }
}
