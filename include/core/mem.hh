#ifndef IZUMO_CORE_MEM_HH_
#define IZUMO_CORE_MEM_HH_

#include <cstdlib>
#include <cstddef>
#include <memory>

namespace izumo::core {
    struct _mem_chunk_header {
	_mem_chunk_header* prev = nullptr;
	std::size_t remaining = 0; // remaining unallocated bytes in this chunk
    };
    
    // this structure might present in either head or tail
    // depend on alignment requirement of the large object
    struct _mem_large_meta {
	_mem_large_meta* prev = nullptr;
	void* ptr = nullptr;	// pointer to the head of memory
    };

    // deleter for mem_pool based unique ptr
    template <typename _t>
    struct _mem_pool_delete {
	// only destruct the object since memory is released by mem_pool's destructor
	void operator() (_t* ptr) { ptr->~_t(); }
    };

    // unique_ptr to be used with mem_pool
    template <typename _t>
    using mp_unique_ptr = std::unique_ptr<_t, _mem_pool_delete<_t>>;

    /** mem_pool: simple memory pool implementation */
    class mem_pool {
    private:
	_mem_chunk_header *m_chunk_p = nullptr;
	_mem_large_meta *m_large_p = nullptr;

	bool m_alloc_chunk() noexcept;
	_mem_large_meta* m_alloc_large(std::size_t size, std::size_t alignment) noexcept;
	
    public:
	constexpr inline static std::size_t CHUNK_SIZE = 4096;
	constexpr inline static std::size_t LARGE_THRESHOLD = CHUNK_SIZE / 2;
	
	mem_pool() noexcept = default;
	mem_pool(const mem_pool&) = delete;
	mem_pool(mem_pool&&);
	~mem_pool();	

	/** allocate, try_allocate: allocate a chunk of raw memory
	 *    `allocate` calls `std::abort` on allocation failure
	 *    use `try_allocate` if such behaviour is undesired
	 *  @parameters:
	 *    size: size of memory to allocate
	 *    alignment: alignment requirement to use
	 *  @return:
	 *    a void* pointer to allocated memory, or nullptr (try_allocate only)
	 */
	void* allocate(std::size_t size, size_t alignment = alignof(std::max_align_t)) noexcept;
	void* try_allocate(std::size_t size, size_t alignment = alignof(std::max_align_t)) noexcept;

	/** construct, try_construct: allocate and construct an object of type T
	 *    the user is responsible to destruct the object 
	 *    **before** corresponding mem_pool object is destructed
	 *  @parameters:
	 *    args: arguments to be forwarded to object's constructor
	 *  @return:
	 *    a pointer to new-ly constructed object; guaranteed to be non-null
	 */
	template <typename _t, typename... _args_t> auto
	construct(_args_t&&... args)
	{
	    auto mem = allocate(sizeof(_t), alignof(_t));
	    auto ret = new (mem) _t(std::forward<_args_t>(args)...);
	    return ret;
	}

	template <typename _t, typename... _args_t> auto
	try_construct(_args_t&&... args)
	{
	    auto mem = try_allocate(sizeof(_t), alignof(_t));
	    auto ret = new (mem) _t(std::forward<_args_t>(args)...);
	    return ret;
	}

	/** make_unique, try_make_unique: allocate and construct an object of type T
	 *    the only difference with `construct` series is that these function returns a
	 *    `mp_unique_ptr<T>` rather than raw pointer
	 */
	template <typename _t, typename... _args_t> auto
	make_unique(_args_t&&... args)
	{
	    return mp_unique_ptr<_t>(construct<_t>(std::forward<_args_t>(args)...));
	}

	template <typename _t, typename... _args_t> auto
	try_make_unique(_args_t&&... args)
	{
	    return mp_unique_ptr<_t>(try_construct<_t>(std::forward<_args_t>(args)...));
	}
    };

    /** std allocator interface adapter */
    template <typename _t>
    class mem_pool_allocator {
    public:
	using value_type = _t;

    private:
	mem_pool& m_pool;

    public:
	mem_pool_allocator(mem_pool& p) noexcept: m_pool(p) {}
	~mem_pool_allocator() = default;
	mem_pool_allocator(const mem_pool_allocator&) = default;

	template <typename _u>
	mem_pool_allocator(const mem_pool_allocator<_u>& rhs) noexcept:
	    m_pool(rhs.p)
	{}

	bool
	operator==(const mem_pool_allocator& rhs)
	{
	    return &m_pool == &rhs.m_pool;
	}

	bool
	operator!=(const mem_pool_allocator& rhs)
	{
	    return &m_pool != &rhs.m_pool;
	};

	value_type*
	allocate(std::size_t n)
	{
	    // XXX: integer overflow?
	    return m_pool.allocate(n * sizeof(value_type), alignof(value_type));
	}

	void deallocate(value_type*, std::size_t) {}
    };
}

#endif	// IZUMO_CORE_MEM_HH_
