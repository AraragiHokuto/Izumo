#include <core/ev_loop.hh>
#include <core/exception.hh>
#include <core/clock.hh>

#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <vector>
#include <queue>
#include <cassert>

#include <buildconfig.h>
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
#if !defined (IZM_HAVE_EPOLL)
#error "XXX: only epoll is supported now"
#endif
	return _ev_loop_get_impl_instance(IZM_EVLOOP_DEFAULT_IMPL);
    }
}
