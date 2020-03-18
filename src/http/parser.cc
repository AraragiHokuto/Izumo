#include <http/parser.hh>

#include <cassert>
#include <cstring>

static unsigned char*
find_equal(unsigned char* begin, unsigned char* end, char target)
{
    for (auto p = begin; p < end; ++p) {
	if (*p == target) return p;
    }

    return end;
}

static unsigned char*
find_not_equal(unsigned char* begin, unsigned char* end, char target)
{
    for (auto p = begin; p < end; ++p) {
	if (*p != target) return p;
    }

    return end;
}

static unsigned char*
find_in_range(unsigned char* begin, unsigned char* end, char min, char max)
{
    for (auto p = begin; p < end; ++p) {
	if (*p >= min && *p <= max) return p;
    }

    return end;
}

static unsigned char* 
find_not_in_range(unsigned char* begin, unsigned char* end, char min, char max)
{
    for (auto p = begin; p < end; ++p) {
	if (*p < min || *p > max) return p;
    }

    return end;
}

static bool
is_tchar(char c)
{
    // XXX: ASCII only? it's unlikely that this will cause problem through
    return (c >= 'a' && c <= 'z')
	|| (c >= 'A' && c <= 'Z')
	|| (c >= '0' && c <= '9')
	|| c == '!' || c == '#' || c == '$' || c == '\''
	|| c == '*' || c == '+' || c == '-' || c == '.'
	|| c == '^' || c == '_' || c == '`' || c == '~';
}

static unsigned char*
find_not_token(unsigned char* begin, unsigned char* end)
{
    for (auto p = begin; p < end; ++p) {
	if (!is_tchar(*p)) return p;
    }

    return end;
}

// for removing trailing spaces
static unsigned char*
rfind_not_equal(unsigned char* begin, unsigned char* end, char target)
{
    for (auto p = end - 1; p >= begin; --p) {
	if (target != *p) return p;
    }

    return begin - 1;
}

static auto
to_string_view(const unsigned char* begin, const unsigned char* end)
{
    auto _begin = static_cast<const void*>(begin);
    
    return std::string_view(static_cast<const char*>(_begin), end - begin);
}

namespace izumo::http {
    static void
    expect_crlf(unsigned char* p)
    {
	if (p[0] != '\r' || p[1] != '\n') throw bad_request();
    }

    static void
    parse_header(header& h, unsigned char* p, unsigned char* end)
    {
	while (*p != '\r') {
	    // parse field
	    auto field_begin = p;
	    p = find_not_token(p, end);
	    assert(p != end); // never return end since `header_completed` must be called beforehand

	    // no space allowed before colon 
	    if (*p != ':') throw bad_request();
	    auto field_end = p;

	    p = find_not_equal(p, end, ' '); 
	    assert(p != end);
	    
	    auto value_begin = p;

	    p = find_equal(p, end, '\r'); 
	    assert(p != end);
	    expect_crlf(p);

	    auto value_end = find_not_equal(value_begin, p, ' ') + 1;

	    // empty value is not allowed
	    if (value_end == value_begin) throw bad_request();

	    p += 2; // skip CRLF

	    h.emplace(to_string_view(field_begin, field_end),
		      to_string_view(value_begin, value_end));
	}

	expect_crlf(p);
    }

    static bool
    is_header_end(unsigned char* p)
    {
	return p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n';
    }
    
    std::size_t
    header_completed(const core::byte_buffer_view& view) noexcept
    {
	if (view.size() < 4) return 0;
	
	auto p = view.ptr();
	auto end = view.ptr() + view.size();

	while (true) {
	    p = find_equal(p, end, '\r');
	    if (end - p < 4) return 0;

	    if (is_header_end(p)) break;
	    ++p;
	}
	return p - view.ptr() + 4;
    }

    // return only minor version, since major is always 1
    static int
    parse_httpver(unsigned char* p, unsigned char* end)
    {
	if (end - p < 8) throw bad_request();
	if (   p[0] != 'H' || p[1] != 'T' || p[2] != 'T' || p[3] != 'P'
	    || p[4] != '/' || p[5] != '1' || p[6] != '.') {
	    throw bad_request();
	}

	if (p[7] != '0' && p[7] != '1') throw bad_request();
	return p[7] - '0';
    }

    void
    request::parse(const core::byte_buffer_view& view)
    {
	auto p = view.ptr();
	auto end = view.ptr() + view.size();

	// parse request-line

	// method
	auto method_begin = p;
	p = find_not_token(p, end);
	assert(p != end);

	if (*p != ' ') throw bad_request();
	auto method_end = p++;

	// target (URI)
	auto target_begin = p;
	p = find_equal(p, end, ' ');
	if (p == end) throw bad_request();
	if (*p != ' ') throw bad_request();
	auto target_end = p++;

	method = to_string_view(method_begin, method_end);
	target = to_string_view(target_begin, target_end);

	// HTTP VERSION
	httpver_major = 1;
	httpver_minor = parse_httpver(p, end);

	p += 8;
	expect_crlf(p);
	p += 2;

	parse_header(headers, p, end);
    }

    void
    response::parse(const core::byte_buffer_view& view)
    {
	auto p = view.ptr();
	auto end = view.ptr() + view.size();

	// parse status-line

	// httpver
	httpver_major = 1;
	httpver_minor = parse_httpver(p, end);

	p += 8;
	if (p == end) throw bad_request();
	if (*p++ != ' ') throw bad_request();

	// status-code (3DIGIT)
	auto status_begin = p;
	auto status_end = find_not_in_range(p, end, '0', '9');
	if (status_end - status_begin != 3) throw bad_request();
	status_code = (p[0] - '0') * 100;
	status_code += (p[1] - '0') * 10;
	status_code += p[2] - '0';

	p = status_end;
	assert(p != end);
	
	if (*p++ == ' ') throw bad_request();

	// reason-phrase
	auto reason_begin = p;
	auto reason_end = p = find_equal(p, end, '\r');
	expect_crlf(p);

	status_message = to_string_view(reason_begin, reason_end);
    }
}
