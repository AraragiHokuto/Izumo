#ifndef IZUMO_HTTP_TYPES_HH_
#define IZUMO_HTTP_TYPES_HH_

#include <core/mem.hh>

#include <map>

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
    };

    struct response {
	int status_code;
	std::string_view status_message;
	int httpver_major, httpver_minor;

	header headers;

	response(core::mem_pool& pool):
	    headers(core::mem_pool_allocator<header::value_type>(pool))
	{}
    };
}

#endif	// IZUMO_HTTP_TYPES_HH_
