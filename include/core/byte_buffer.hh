#ifndef IZUMO_CORE_BYTE_BUFFER_HH_
#define IZUMO_CORE_BYTE_BUFFER_HH_

#include <cstdint>
#include <string_view>

namespace izumo::core {
    using byte_t = uint8_t;

    class byte_buffer_view;
    class byte_buffer_writer;
    
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

	byte_buffer_view view(std::size_t end) noexcept;
	byte_buffer_view view(std::size_t offset, std::size_t end) noexcept;

	byte_buffer_writer writer(std::size_t offset = 0) noexcept;
	byte_buffer_writer writer(std::size_t offset, std::size_t end) noexcept;

	void* data() const noexcept { return m_ptr; }
	byte_t* ptr() const noexcept { return m_ptr; }
    };

    class byte_buffer_view {
    private:
	byte_t* m_buf = nullptr;
	std::size_t m_begin = 0;
	std::size_t m_end = 0;

	friend byte_buffer_writer;

    public:
	byte_buffer_view() = default;
	byte_buffer_view(byte_buffer& buffer): byte_buffer_view(buffer, 0, buffer.size()) {}
	byte_buffer_view(byte_buffer& buffer, std::size_t end):
	    byte_buffer_view(buffer, 0, end)
	{}
	byte_buffer_view(byte_buffer& buffer, std::size_t begin, std::size_t end):
	    m_buf(buffer.ptr()), m_begin(begin), m_end(end)
	{}
	byte_buffer_view(const byte_buffer_view& rhs) = default;

	void* data() const noexcept;
	byte_t* ptr() const noexcept;
	std::size_t size() const noexcept;

	byte_t& operator[](std::size_t n) const noexcept;

	byte_buffer_view slice(std::size_t end) const noexcept;
	byte_buffer_view slice(std::size_t begin, std::size_t end) const noexcept;

	operator std::string_view() const noexcept;
    };

    class byte_buffer_writer {
    private:
	byte_t* m_buf = nullptr;
	std::size_t m_begin = 0;
	std::size_t m_current = 0;
	std::size_t m_end = 0;

    public:
	byte_buffer_writer() = default;
	byte_buffer_writer(byte_buffer& buffer, std::size_t begin = 0):
	    byte_buffer_writer(buffer, begin, buffer.size())
	{}
	byte_buffer_writer(byte_buffer& buffer, std::size_t begin, std::size_t end);

	void* current() const noexcept;
	void* begin() const noexcept;
	void* end() const noexcept;

	std::size_t size() const noexcept;
	std::size_t space() const noexcept;

	void move_current(std::size_t size) noexcept;

	std::size_t strcpy(const char* src) noexcept;
	std::size_t strcpy(const std::string_view& view) noexcept;
	std::size_t memcpy(const void* src, std::size_t size) noexcept;

	void write_byte(byte_t byte) noexcept;
	std::size_t try_write_byte(byte_t byte) noexcept;

	byte_buffer_view to_view() const noexcept;
    };
}

#endif	// IZUMO_CORE_BYTE_BUFFER_HH_
