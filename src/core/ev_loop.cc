#include <core/ev_loop.hh>
#include <core/exception.hh>
#include <core/clock.hh>

#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <vector>
#include <queue>
#include <cassert>

#include "ev_loop.in.cc"

IMPL_MAP_DEF;

namespace izumo::core {
    void
    ev_loop::run_forever()
    {
	while(true) {
	    this->run_once();
	}
    }

    ev_loop&
    ev_loop::instance()
    {
	return _ev_loop_get_impl_instance("epoll");
    }
}
