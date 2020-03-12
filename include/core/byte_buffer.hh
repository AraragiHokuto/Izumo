#ifndef IZUMO_CORE_BYTE_BUFFER_HH_
#define IZUMO_CORE_BYTE_BUFFER_HH_

#include <cstdint>
#include <string_view>

namespace izumo::core {
    using byte_t = uint8_t;

    class byte_buffer_view;
    
    /** byte_buffer: contiguous container for raw bytes */
    class byte_buffer {
    private:
	byte_t* m_ptr = nullptr;
	std::size_t m_size = 0;

    public:
	byte_buffer() = default;
	byte_buffer(std::size_t initial_size);
	byte_buffer(const byte_buffer&);
	byte_buffer(byte_buffer&&) = default;

	~byte_buffer();

	std::size_t size() const noexcept { return m_size; };
	
	void resize(std::size_t new_size) noexcept;
	bool try_resize(std::size_t new_size) noexcept;

	void* data() const noexcept { return m_ptr; }
	byte_t* ptr() const noexcept { return m_ptr; }
    };

    class byte_buffer_view {
    private:
	byte_t* m_buf = nullptr;
	std::size_t m_off = 0;
	std::size_t m_len = 0;

    public:
	byte_buffer_view() = default;
	byte_buffer_view(byte_buffer& buffer): byte_buffer_view(buffer, 0, buffer.size()) {}
	byte_buffer_view(byte_buffer& buffer, std::size_t length):
	    byte_buffer_view(buffer, 0, length)
	{}
	byte_buffer_view(byte_buffer& buffer, std::size_t offset, std::size_t length):
	    m_buf(buffer.ptr()), m_off(offset), m_len(length)
	{}
	byte_buffer_view(const byte_buffer_view& rhs) = default;

	void* data() const noexcept;
	std::size_t size() const noexcept;

	byte_t& operator[](std::size_t n) const noexcept;

	byte_buffer_view slice(std::size_t length) const noexcept;
	byte_buffer_view slice(std::size_t offset, std::size_t length) const noexcept;

	operator std::string_view() const noexcept;
    };
}

#endif	// IZUMO_CORE_BYTE_BUFFER_HH_
