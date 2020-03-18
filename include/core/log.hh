#ifndef IZUMO_CORE_LOG_HH_
#define IZUMO_CORE_LOG_HH_

#include <cassert>
#include <cstdint>
#include <string>
#include <memory>
#include <chrono>
#include <ctime>
#include <fmt/chrono.h>
#include <fmt/format.h>

namespace izumo::core {
    enum class log_level {
	debug = 1,
	info = 2,
	warn,
	error,
	fatal
    };
    
    class log_output {
    public:
	virtual void out(const char* str, std::size_t len) = 0;
    };

    class log_output_stdout: public log_output {
    public:
	void out(const char* str, std::size_t len) override;
    };

    class logger {
    public:
	static logger& get();

	static std::unique_ptr<log_output> default_output;
	static void set_default_output(std::unique_ptr<log_output> output);

    private:
	std::string m_name;
	std::unique_ptr<log_output> m_output = nullptr;

	log_level m_min_level = log_level::info;

	logger() = default;

	log_output& m_get_output() { return m_output ? *m_output : *default_output; }
    public:
	void set_name(std::string name);
	void set_output(std::unique_ptr<log_output> output);

	void
	set_level(log_level min_level)
	{
	    m_min_level = min_level;
	}
	
	template <typename _s, typename... _args_t> void
	log(log_level level, const _s& fmt, _args_t&&... args)
	{
	    if (level < m_min_level) return;
	    
	    const std::size_t BUFSIZE=512;
	    char buf[BUFSIZE];
	    const char* level_chars = "VDIWEF";

	    auto now = std::time(nullptr);
	    auto ret = fmt::format_to_n(buf, BUFSIZE, "{:%Y-%m-%d %H:%M:%S} {} {}{}",
					*std::localtime(&now), level_chars[static_cast<std::size_t>(level)], m_name, m_name.size() ? " " : "");
	    
	    // XXX: boundary check. should not be necessary
	    assert(ret.size < BUFSIZE);

	    auto size = ret.size;
	    ret = fmt::format_to_n(buf + size, BUFSIZE - size, fmt, std::forward<_args_t>(args)...);
	    size += ret.size;

	    m_get_output().out(buf, size);
	}

    };
    
    namespace log {
    	template <typename _s, typename... _args_t> void
	debug(const _s& fmt, _args_t&&... args)
	{
	    logger::get().log(log_level::debug, fmt, std::forward<_args_t>(args)...);
	}

	template <typename _s, typename... _args_t> void
	info(const _s& fmt, _args_t&&... args)
	{
	    logger::get().log(log_level::info, fmt, std::forward<_args_t>(args)...);
	}

	template <typename _s, typename... _args_t> void
	warn(const _s& fmt, _args_t&&... args)
	{
	    logger::get().log(log_level::warn, fmt, std::forward<_args_t>(args)...);
	}

	template <typename _s, typename... _args_t> void
	error(const _s& fmt, _args_t&&... args)
	{
	    logger::get().log(log_level::error, fmt, std::forward<_args_t>(args)...);
	}

	template <typename _s, typename... _args_t> void
	fatal(const _s& fmt, _args_t&&... args)
	{
	    logger::get().log(log_level::fatal, fmt, std::forward<_args_t>(args)...);
	}
    }
}

#endif	// IZUMO_CORE_LOG_HH_
