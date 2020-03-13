#ifndef IZUMO_HTTP_PARSER_HH_
#define IZUMO_HTTP_PARSER_HH_

#include <http/types.hh>
#include <core/exception.hh>
#include <core/byte_buffer.hh>

#include <map>
#include <string_view>

namespace izumo::http {
    // which way is better, exception or return value?
    struct bad_request: std::runtime_error { bad_request(): std::runtime_error("bad_request") {} };
    
    // check if http header is completely received 
    // return the length of the header if completed, or 0 if incomplete
    std::size_t header_completed(const core::byte_buffer_view& view) noexcept;
    
    // parse request or response
    // call these functions after `header_completed`
    void parse_request(request& req, const core::byte_buffer_view& view);
    void parse_response(response& res, const core::byte_buffer_view& view);
}

#endif	// IZUMO_HTTP_PARSER_HH_
