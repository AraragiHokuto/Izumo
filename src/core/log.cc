#include <core/log.hh>

#include <iostream>
#include <string_view>

namespace izumo::core {
    std::unique_ptr<log_output> logger::default_output { new log_output_stdout };

    logger&
    logger::get()
    {
	static thread_local logger ret;
	return ret;
    }

    void
    logger::set_default_output(std::unique_ptr<log_output> output)
    {
	// XXX: thread safety?
	logger::default_output = std::move(output);
    }

    void
    log_output_stdout::out(const char *str, std::size_t len)
    {
	// C++11 standard guarantees concurrent access to std::cout
	// will not result in a data race. there is a possibility
	// that characters are interleaved, but it won't cause much
	// problem for simple logging
	std::cout << std::string_view(str, len) << std::endl;
    }
}
