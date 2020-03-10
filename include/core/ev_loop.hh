#ifndef IZUMO_CORE_EV_LOOP_HH_
#define IZUMO_CORE_EV_LOOP_HH_

#include <cstdint>

#include <core/ev_watcher.hh>
#include <core/clock.hh>

namespace izumo::core {
    class ev_loop {
    public:
	static ev_loop& instance();

    public:
	/** add_watcher: add a watcher to monitor
	 *   @parameters:
	 *      watcher: watcher to be added
	 */
	virtual void add_watcher(ev_watcher& watcher) = 0;

	/** remove_watcher: stop monitoring a watcher
	 *   @parameters:
	 *      watcher: watcher to be removed
	 */
	virtual void remove_watcher(ev_watcher& watcher) = 0;

	/** add_timer: add a timer
	 *   @parameters:
	 *      watcher: the owner of timer
	 *      timeout: timeout in milliseconds; must be larger than 0
	 *   @return:
	 *      an id for the added timer
	 */
	virtual void add_timer(ev_watcher& watcher, timedelta_ms_t timeout) = 0;

	/** run_once: run ev_loop only once */
	virtual void run_once() = 0;

	/** run_forever(): run ev_loop forever */
	void run_forever();
    };
}

#endif	// IZUMO_CORE_EV_LOOP_HH_
