#include <http/writer.hh>

#include <cstring>

static const char HTTPVER_PREF[] = "HTTP/1.";
static const std::size_t HTTPVER_PREF_LEN = sizeof(HTTPVER_PREF) - 1;

namespace izumo::http {
    std::size_t 
    start_request(
	core::byte_buffer_writer& writer,
	const std::string_view& method,
	const std::string_view& target,
	int httpver_minor) noexcept
    {
	std::size_t ret = method.size() + 1; // method ' '
	ret += target.size() + 1; // target ''
	ret += 10; // httpver '\r' '\n'

	if (writer.space() < ret) return ret;

	writer.strcpy(method);
	writer.write_byte(' ');

	writer.strcpy(target);
	writer.write_byte(' ');
	
	writer.strcpy(HTTPVER_PREF);
	writer.write_byte(httpver_minor + '0');

	writer.write_byte('\r');
	writer.write_byte('\n');

	return ret;
    }

    std::size_t
    start_response(
	core::byte_buffer_writer& writer,
	int status_code,
	const std::string_view& status_message,
	int httpver_minor) noexcept
    {
	std::size_t ret = 13; // httpver ' ' 3DIGIT ' '
	ret += status_message.size() + 2; // reason-phrase '\r' '\n'
	
	if (writer.space() < ret) return ret;

	writer.strcpy(HTTPVER_PREF);
	writer.write_byte(httpver_minor + '0');
	writer.write_byte(' ');

	char status_buf[3];
	status_buf[2] = status_code % 10 + '0'; status_code /= 10;
	status_buf[1] = status_code % 10 + '0'; status_code /= 10;
	status_buf[0] = status_code + '0';

	writer.memcpy(status_buf, sizeof(status_buf));
	writer.write_byte(' ');

	writer.strcpy(status_message);
	writer.write_byte('\r');
	writer.write_byte('\n');

	return ret;
    }

    std::size_t
    add_header(
	core::byte_buffer_writer& writer,
	const std::string_view& field,
	const std::string_view& value
	) noexcept
    {
	std::size_t ret = field.size() + value.size() + 4; // field ':' ' ' value '\r' '\n'

	if (writer.space() < ret) return ret;

	writer.strcpy(field);

	writer.write_byte(':');
	writer.write_byte(' ');

	writer.strcpy(value);

	writer.write_byte('\r');
	writer.write_byte('\n');

	return ret;
    }

    std::size_t
    write_eoh(core::byte_buffer_writer& writer) noexcept
    {
	const std::size_t ret = 2;

	if (writer.space() < ret) return ret;

	writer.write_byte('\r');
	writer.write_byte('\n');

	return ret;
    }
}
