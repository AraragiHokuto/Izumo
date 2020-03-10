#include <core/clock.hh>

#include <chrono>

namespace izumo::core {
    timestamp_ms_t
    clock::now()
    {
	auto stdnow = std::chrono::high_resolution_clock::now();
	auto ts = stdnow.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(ts).count();
    }
}
