// core/clock.hh -- timing utilities
#ifndef IZUMO_CORE_CLOCK_HH_
#define IZUMO_CORE_CLOCK_HH_

#include <cstdint>

namespace izumo::core {
    using timestamp_ms_t = uint64_t; // representing a timestamp in milliseconds
    using timedelta_ms_t = int64_t;  // representintg the difference of two timestamps in milliseconds
    
    class clock {
    public:
	/** now: return current timestamp
	 *   @return:
	 *      current timestamp in milliseconds
	 */
	static timestamp_ms_t now(); 
    };
}

#endif	// IZUMO_CORE_CLOCK_HH_
