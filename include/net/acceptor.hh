#ifndef IZUMO_NET_ACCEPTOR_HH_
#define IZUMO_NET_ACCEPTOR_HH_

#include <cstdint>

#include <core/ev_watcher.hh>

namespace izumo::net {
    class acceptor: public core::ev_watcher {
    private:
	acceptor(int fd);
	
	/** on_new_connection: new connection callback
	 *   called each time a new socket is accepted
	 *   @parameters:
	 *      client_fd: fd of new client socket
	 */
	virtual void on_new_connection(int client_fd) = 0;

	bool on_event(bool r, bool w, bool e) override;
	void on_deferred() override;
	void on_timeout(uint64_t timer_id) override;
    };
}

#endif	// IZUMO_NET_ACCEPTOR_HH_
