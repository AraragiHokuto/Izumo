#ifndef IZUMO_CORE_EV_WATCHER_HH_
#define IZUMO_CORE_EV_WATCHER_HH_

#include <cstdint>

namespace izumo::core {
    // base class for a event watcher
    class ev_watcher {
    protected:
	int m_fd; // file descriptor to watch
    public:
	ev_watcher(int fd): m_fd(fd) {}
	
	int fd() { return m_fd; }
	/** on_event: edge triggered event callback
	 *   called each time watcher state is changed
	 *   @parameters:
	 *      r: readable state
	 *      w: writable state
	 *   @return:
	 *      whether `on_deferred` should be called
	 */
	virtual bool on_event(bool r, bool w) = 0;

	/** on_deferred: deferred callback
	 *   called after every watcher have their `on_event` finished
	 */
	virtual void on_deferred() {};

	/** on_timeout: timeout callback
	 *   called when a registered timer is expired
	 *   @parameters:
	 *      id: parameter id returned by `ev_loop.add_timer`
	 **/
	virtual void on_timeout() {};
    };
}

#endif	// IZUMO_CORE_EV_WATCHER_HH_
