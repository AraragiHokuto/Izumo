#include <core/byte_buffer.hh>
#include <core/exception.hh>

#include <cstdlib>
#include <cstring>
#include <cassert>

namespace izumo::core {
    byte_buffer::byte_buffer(std::size_t initial_size)
    {
	m_ptr = static_cast<byte_t*>(std::malloc(initial_size));
	if (!m_ptr) throw std::bad_alloc();
	
	m_size = initial_size;
    }

    byte_buffer::byte_buffer(const byte_buffer& rhs)
    {
	// TODO: COW
	m_ptr = static_cast<byte_t*>(std::malloc(rhs.m_size));
	if (!m_ptr) throw std::bad_alloc();

	m_size = rhs.m_size;
	std::memcpy(m_ptr, rhs.m_ptr, m_size);
    }

    byte_buffer::~byte_buffer()
    {
	free(m_ptr);
    }

    bool
    byte_buffer::try_resize(std::size_t n) noexcept
    {
	auto newptr = realloc(m_ptr, n);
	if (!newptr) return false;

	m_size = n;
	m_ptr = static_cast<byte_t*>(newptr);
	return true;
    }

    void
    byte_buffer::resize(std::size_t n) noexcept
    {
	if (!try_resize(n)) std::abort();
    }

    byte_buffer_view
    byte_buffer::view(std::size_t end)
    {
	return view(0, end);
    }

    byte_buffer_view
    byte_buffer::view(std::size_t begin, std::size_t end)
    {
	return byte_buffer_view(*this, begin, end);
    }

    void*
    byte_buffer_view::data() const noexcept
    {
	return ptr();
    }

    byte_t*
    byte_buffer_view::ptr() const noexcept
    {
	return m_buf + m_begin;
    }
    
    std::size_t
    byte_buffer_view::size() const noexcept
    {
	return m_end - m_begin;
    }

    byte_t&
    byte_buffer_view::operator[](std::size_t n) const noexcept
    {
	return ptr()[n];
    }

    byte_buffer_view
    byte_buffer_view::slice(std::size_t end) const noexcept
    {
	return slice(0, end);
    }

    byte_buffer_view
    byte_buffer_view::slice(std::size_t begin, std::size_t end) const noexcept
    {
	auto ret = *this;
	ret.m_begin = m_begin + begin;
	ret.m_end = m_begin + end;
	return ret;
    }

    byte_buffer_view::operator std::string_view() const noexcept
    {
	auto ptr = static_cast<std::string_view::value_type*>(data());
	return std::string_view(ptr, m_end);
    }

    byte_buffer_writer::byte_buffer_writer(byte_buffer& buffer, std::size_t begin):
	m_buf(buffer.ptr()), m_begin(begin), m_current(begin), m_end(buffer.size())
    {}

    void*
    byte_buffer_writer::current() const noexcept
    {
	return m_buf + m_current;
    }

    void*
    byte_buffer_writer::begin() const noexcept
    {
	return m_buf + m_begin;
    }

    void*
    byte_buffer_writer::end() const noexcept
    {
	return m_buf + m_end;
    }

    std::size_t
    byte_buffer_writer::size() const noexcept
    {
	return m_end - m_begin;
    }

    std::size_t
    byte_buffer_writer::space() const noexcept
    {
	return m_end - m_current;
    }

    void
    byte_buffer_writer::append(std::size_t size) noexcept
    {
	assert(size <= m_end - m_current);
	m_current += size;
    }

    void
    byte_buffer_writer::write_byte(byte_t byte) noexcept
    {
	assert(space());
	m_buf[m_current++] = byte;
    }

    byte_buffer_view
    byte_buffer_writer::to_view() const noexcept
    {
	byte_buffer_view ret;
	ret.m_buf = m_buf;
	ret.m_begin = m_begin;
	ret.m_end = m_current;
	return ret;
    }
}
