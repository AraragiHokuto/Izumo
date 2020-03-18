#ifndef IZUMO_HTTP_WRITER_HH_
#define IZUMO_HTTP_WRITER_HH_

#include <core/byte_buffer.hh>

namespace izumo::http {
    std::size_t start_request(
	core::byte_buffer_writer& writer,
	const std::string_view& method,
	const std::string_view& target,
	int httpver_minor = 1) noexcept;
    
    std::size_t start_response(
	core::byte_buffer_writer& writer,
	int status_code,
	const std::string_view& status_message,
	int httpver_minor = 1) noexcept;

    std::size_t add_header(
	core::byte_buffer_writer& writer,
	const std::string_view& field,
	const std::string_view& value) noexcept;

    std::size_t write_eoh(core::byte_buffer_writer& writer) noexcept;
}

#endif	// IZUMO_HTTP_WRITER_HH_
