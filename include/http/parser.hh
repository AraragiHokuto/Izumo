#ifndef IZUMO_HTTP_PARSER_HH_
#define IZUMO_HTTP_PARSER_HH_

#include <http/types.hh>
#include <core/exception.hh>
#include <core/byte_buffer.hh>

#include <map>
#include <string_view>

namespace izumo::http {
    using header = std::multimap<
	std::string_view,
	std::string_view,
	std::less<std::string_view>,
	core::mem_pool_allocator<
	    std::pair<const std::string_view, std::string_view>>>;

    struct request {
	std::string_view method;
	std::string_view target;
	int httpver_major, httpver_minor;

	header headers;

	request(core::mem_pool& pool):
	    headers(core::mem_pool_allocator<header::value_type>(pool))
	{}

	// call after `header_completed`
	void parse(const core::byte_buffer_view& view);
    };

    struct response {
	int status_code;
	std::string_view status_message;
	int httpver_major, httpver_minor;

	header headers;

	response(core::mem_pool& pool):
	    headers(core::mem_pool_allocator<header::value_type>(pool))
	{}

	// call after `header_completed`
	void parse(const core::byte_buffer_view& view);
    };

    // which way is better, exception or return value?
    struct bad_request: std::runtime_error { bad_request(): std::runtime_error("bad_request") {} };
    
    // check if http header is completely received 
    // return the length of the header if completed, or 0 if incomplete
    std::size_t header_completed(const core::byte_buffer_view& view) noexcept;
}

#endif	// IZUMO_HTTP_PARSER_HH_
