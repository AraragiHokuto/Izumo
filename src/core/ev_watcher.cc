#include <core/ev_watcher.hh>
#include <core/ev_loop.hh>

#include <cassert>

namespace izumo::core {
    void
    ev_watcher::add_to_loop()
    {
	add_to_loop(ev_loop::instance());
    }

    void
    ev_watcher::add_to_loop(ev_loop &loop) 
    {
	loop.add_watcher(*this);
	m_loop = &loop;
    }

    void
    ev_watcher::remove_from_loop()
    {
	assert(m_loop);
	m_loop->remove_watcher(*this);
    }
}
